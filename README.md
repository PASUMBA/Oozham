# Oozham (ഊഴം): Problem Flow & System Understanding

> **"Your Turn"**: a dynamic signage system that decides whose turn it is at a hairpin bend.

Built by **PASU Labs** for **TinkerHub LAYOVER 2026** (TinkerSpace Calicut). Theme: Travel.

---

## Hackathon Tracks (TinkerHub LAYOVER 2026)

| Track | Fit | Why |
|---|---|---|
| **05 Travel hardware & IoT** | Primary | reCamera + ESP32 + LoRa + LED signs, a real roadside device |
| **03 Navigation & mobility** | Primary | Decides who moves first through the bend, so traffic keeps flowing on ghat roads |
| **02 AI-powered travel experiences** | Secondary | YOLO11n edge AI classifies vehicles as HEAVY / LIGHT |
| **01 Travel utilities & tools** | Secondary | Saves travellers hours stuck at Thamarassery Churam |
| **07 Accessibility & sustainable travel** | Secondary | Keeps emergency routes open, reduces accidents, cuts fuel wasted idling in jams |

---

## 1. The Core Problem

Hairpin bends on ghat roads (like Thamarassery Churam) are physically narrow.
A loaded truck or bus needs the **full width of the road** to navigate the curve.

When a heavy vehicle swings into the wrong lane to make the turn and another vehicle enters
from the opposite side, neither can easily reverse.
The result is a deadlock that can last **hours**.

### Real-World Impact: July 2026, Thamarassery Ghat

- A single multi-axle truck stuck at the 4th hairpin caused a **10+ hour gridlock**
- Thousands of travellers stranded
- Ambulances delayed
- Airport passengers walked uphill to find other transport
- Churam Samrakshana Samithi: jams could be avoided if heavy vehicles were **stopped at both ends** whenever one is stuck on the bend

Kerala recorded about **49,889 road accidents in 2025**; experts point to road design, curves and enforcement gaps.

### Why Existing Systems Fail

| System | What it does | What it cannot do |
|---|---|---|
| Valparai Ghat Sensors (TN, 5th hairpin) | Warns drivers a vehicle is coming | Does not decide who goes first |
| #RoadsThatHonk (Patnitop, J&K) | Audio/visual alert at bends | Does not decide who goes first |
| **Oozham** | **Decides whose turn it is** | — |

> Existing systems convert a blind spot into a warning.
> **Oozham converts a warning into a decision.**

---

## 2. How Oozham Works

AI cameras at both approaches detect and classify incoming vehicles as **HEAVY** (truck, bus)
or **LIGHT** (car, bike). A controller applies priority rules and drives LoRa-linked LED signs
at each end of the bend.

### Signs show one of these states

- `GO`: you may enter the bend
- `STOP`: wait, the bend is occupied
- `HEAVY VEHICLE TURNING`: a large vehicle is using the full width; do not enter
- `BEND BLOCKED`: vehicle stuck inside the bend; nobody enters

---

## 3. System Components

| Component | Role |
|---|---|
| **AI Camera (Seeed reCamera, YOLO11n)** | Detects and classifies vehicles as HEAVY or LIGHT. Currently covers **one approach only** |
| **Height sensor (IR / Ultrasonic, mounted ~2.5 m high)** | Covers the other approach. Only tall vehicles (trucks, buses) trigger it, so it works as a cheap **HEAVY detector** |
| **Entry + exit counters (both ends)** | Count vehicles going in and out of the bend. Bend is **clear when count in = count out** |
| **Oozham Controller (ESP32)** | Applies priority rules, runs the state machine, drives the signs |
| **LoRa radio link** | Wireless link between the two ends of the bend (no wiring on ghat roads) |
| **LED signs** | Display STOP / GO / HEAVY VEHICLE TURNING / BEND BLOCKED at each approach |

---

## 4. Decision Logic: All 8 Cases

### Case 1: No vehicle on either side
- Both signs: **CAUTION / normal**

### Case 2: Heavy vehicle downhill only
- Downhill: **GO**
- Uphill: **STOP** + `HEAVY VEHICLE TURNING`

### Case 3: Heavy vehicle uphill only
- Uphill: **GO**
- Downhill: **STOP** + `HEAVY VEHICLE TURNING`

### Case 4: Heavy downhill + Heavy uphill (conflict)
- **Downhill goes first**: a loaded truck going downhill is harder to stop
- Uphill: **STOP** + `HEAVY VEHICLE TURNING`
- Uphill only gets GO once the **counters confirm the bend is clear**

### Case 5: Heavy on one side + Light on the other
- Heavy side: **GO**
- Light side: **STOP** + `HEAVY VEHICLE TURNING`

### Case 6: Light + Light
- **Both: GO**
- Cars and bikes can pass each other within the bend width

### Case 7: Many vehicles queued on one side
- That side goes as a **batch**
- A **max wait timer** fires so the opposite side is never held indefinitely

### Case 8: Vehicle stuck / breakdown inside the bend
- Bend is not cleared (count in ≠ count out) after a set timeout
- Both sides: **STOP** + `BEND BLOCKED`
- This is the fix asked for by the **Churam Samrakshana Samithi**

> **Safe default:** if a vehicle's class is unknown, Oozham treats it as **HEAVY**.

---

## 5. Priority Rule Summary

```
1. Heavy downhill   → highest priority (hardest to stop)
2. Heavy uphill     → second priority
3. Light vehicles   → lowest priority (can share bend width)
4. Batch fairness   → max wait timer prevents starvation
5. Breakdown        → override everything with BEND BLOCKED
6. Unknown vehicle  → treated as HEAVY (safe default)
```

---

## 6. Hardware Architecture

```
Approach A (Downhill)                         Approach B (Uphill)

 [ reCamera ]  HEAVY / LIGHT                   [ Height sensor ~2.5 m ]  HEAVY / none
 [ Entry/exit counter A ]                      [ Entry/exit counter B ]
        │                                              │
 [ ESP32 + LoRa ] ◄────────── LoRa ──────────► [ ESP32 + LoRa ]
  Oozham Controller                              Sign node
        │                                              │
 [ LED Sign A ]                                 [ LED Sign B ]
  Downhill end                                   Uphill end
```

---

## 7. Why Bend-Clear Detection Is Non-Negotiable

Cases 4, 7 and 8 all need to know **when the bend is actually clear**.
A bend has **two ends**, so one exit sensor is not enough. Oozham counts vehicles in and out at both ends.

| Case | Why bend-clear detection is needed |
|---|---|
| Case 4: Heavy vs Heavy | Uphill cannot go until downhill has fully exited the bend |
| Case 7: Batch flow | Next batch cannot start until the bend drains |
| Case 8: Breakdown detection | Timeout fires only if count in ≠ count out for too long |

Without it, the controller is guessing.
A wrong guess recreates the exact deadlock Oozham is designed to prevent.

---

## 8. Hardware Gap for Full Demo

The **reCamera currently covers only one approach (downhill)**.

| Gap | Minimum solution |
|---|---|
| Uphill approach detection | IR / ultrasonic sensor mounted ~2.5 m high (detects HEAVY only) |
| Bend-clear confirmation | Entry + exit counters (IR / ultrasonic) at both ends |

A second reCamera on the uphill side would enable full HEAVY/LIGHT classification
on both approaches, but the height sensor is enough to unblock the demo.

---

## 9. State Machine Overview

```
IDLE
  │
  ├─ vehicle detected on either side
  │
  ▼
EVALUATE
  │
  ├─ apply priority rules (Cases 1–6)
  │
  ▼
ACTIVE (one side has GO, other has STOP)
  │
  ├─ count in = count out → bend clear
  │       │
  │       └─► back to EVALUATE (serve waiting side or IDLE)
  │
  ├─ max wait timer fires → serve other side (Case 7)
  │
  └─ bend timeout fires → BLOCKED state (Case 8)
         │
         └─► both signs STOP + BEND BLOCKED
             manual reset, or auto-clear when count in = count out
```

---

## 10. Current Status

- Detection flow deployed on the reCamera (YOLO11n). A Tata truck photo was detected as **HEAVY**
- In progress: LoRa link and LED signboard

---

## 11. Future Roadmap

| Feature | Description |
|---|---|
| Ambulance priority | Detect emergency vehicles, override all rules, clear the bend immediately |
| Police / control room alerts | Push notifications when BEND BLOCKED state is entered |
| Solar power | Off-grid operation for remote ghat locations |
| Two-camera upgrade | Full HEAVY/LIGHT classification on both approaches |

---

## 12. One-Line Summary

> Oozham is a hairpin-bend traffic controller that uses AI vehicle classification,
> priority rules and LED signs to decide whose turn it is. It replaces guesswork
> with a clear decision and solves the breakdown/deadlock case
> that paralysed Thamarassery Churam for 10+ hours.

---

## Sources

- Onmanorama, 24 Jul 2026: [Midnight gridlock on Thamarassery Ghat leaves thousands stranded](https://www.onmanorama.com/news/kerala/2026/07/24/midnight-gridlock-on-thamarassery-ghat-leaves-thousands-stranded-traffic-snarl-persists-for-over-10-hours.html)
- Times of India, 24 Jul 2026: [Heavy traffic snarl on Thamarassery ghat road, hundreds stranded](https://timesofindia.indiatimes.com/city/kozhikode/heavy-traffic-snarl-on-thamarassery-ghat-road-hundreds-stranded/articleshow/132609630.cms)
- New Indian Express, Sep 2026: Kerala road accidents in 2025 (~49,889)
- The Hindu, 2021: Valparai ghat sensor and display board at the 5th hairpin
- HP Lubricants, 2017: #RoadsThatHonk, Patnitop, J&K
