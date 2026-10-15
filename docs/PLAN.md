# BKIT Traffic-Light Simulator — High-Level Plan

> Status: Draft for initial implementation planning.
> Goal: Build a clear, visual simulation of a four-way intersection with traffic lights, countdown timers, pedestrian signals, Auto mode, Manual mode, and configurable timing.

## 1. Product Vision

Create a simulation of a normal four-way intersection, inspired by the supplied reference images. The main screen should make it easy to understand which road direction has permission to move, how many seconds remain in the active phase, and what the pedestrian signal indicates.

The first milestone is a reliable basic intersection simulation. Keep the design and implementation small enough to understand, test, and extend.

## 2. Visual Requirements

The simulation should show:
- A top-down four-way road intersection with lane markings and pedestrian crossings.
- Traffic-light signals for the two crossing traffic axes.
- A visible red/yellow/green state for each applicable signal.
- A numeric countdown associated with the active traffic phase, visually near the corresponding signal.
- A pedestrian signal with red/stop and green/walk symbols.
- Clear indication of the current operating mode: Auto or Manual.
- A configuration area for the supported timing settings.

The supplied images are visual references, not exact UI specifications. The agent should inspect the existing repository and choose an implementation consistent with its current framework and structure. Do not add a new framework without explaining why it is needed.

### Countdown display rules
- In Auto mode, show the remaining seconds for the active timed phase.
- In Manual mode, hide countdown numbers in the stable manually selected state, as requested.
- During the transition from green to yellow to red, define and implement the countdown behavior consistently. The initial assumption is that the yellow transition is timed and may show its remaining time; after the new Manual state is established, hide the countdown.
- Never show a countdown that disagrees with the controller's actual phase timer.
- For the initial simulation, countdowns are UI elements; do not assume physical seven-segment hardware unless repository requirements explicitly establish it.

## 3. Functional Scope

### 3.1 Basic intersection operation
Implement the basic traffic-light cycle for two conflicting road axes:
- Axis A is green while Axis B is red.
- Axis A transitions from green to yellow, then red.
- Axis B becomes green only after Axis A is red.
- Axis B transitions from green to yellow, then red.
- Axis A becomes green again only after Axis B is red.
- Conflicting axes must never display green simultaneously.

Represent this as explicit states rather than unrelated UI conditions.

### 3.2 Auto mode
- Run the intersection cycle continuously.
- Make the green and red durations configurable.
- Make yellow duration explicit and configurable if the current architecture allows; otherwise document a safe initial default and why.
- Display remaining seconds for the active timed phase.
- Apply a configurable flashing-yellow schedule based on time of day.
- In flashing-yellow mode, all four directional vehicle signals should flash yellow as requested; ordinary red/green operation is suspended while this mode is active.
- Make the start and end times of the flashing-yellow window configurable.
- Define behavior at schedule boundaries and when leaving flashing-yellow mode. Resume through a deterministic, non-conflicting state rather than arbitrarily enabling green signals.
- If the app has no reliable system clock, provide a documented way to set or simulate the time for testing.

### 3.3 Manual mode
Manual mode lets the user request which road axis should have right of way.
- Provide a clear control to request a change from the currently green axis to the perpendicular axis.
- A change request should be handled immediately by the controller, but the active axis must still transition green → yellow → red.
- The perpendicular axis may become green only after the current axis has reached red.
- Hide countdown numbers once a stable Manual state is reached.
- Manual mode does not advance through the ordinary Auto cycle on its own.
- Prevent conflicting green signals even if the user requests a change repeatedly or rapidly.
- Define how the user returns to Auto mode; on return, enter a deterministic safe phase and restart the cycle cleanly.

### 3.4 Pedestrian signal
- Show a pedestrian signal with red/stop and green/walk states.
- The pedestrian signal must not indicate walk while the conflicting vehicle axis is permitted to move.
- For the first milestone, implement only the pedestrian behavior needed to visually represent normal intersection operation. A pedestrian request button/priority mode is out of scope unless it already exists in the accepted project specification and is explicitly retained.
- If pedestrian priority is retained, define its request, safe vehicle transition, walk duration, and return behavior separately before implementation. Do not invent these details silently.

### 3.5 Configuration
Expose configuration for:
- Auto green duration.
- Auto red duration or equivalent phase timing for both axes.
- Yellow transition duration.
- Flashing-yellow start and end time.
- Flashing-yellow interval/rate, if configurable in the UI.

Validate values and prevent invalid settings from breaking the state machine. Persist settings only if existing requirements and framework make persistence appropriate; otherwise session-only configuration is acceptable for the first milestone.

## 4. State and Timing Design

Keep the traffic controller as the source of truth. The UI should render controller state rather than independently calculating traffic behavior.

Suggested conceptual states:
- `AUTO_AXIS_A_GREEN`
- `AUTO_AXIS_A_YELLOW`
- `AUTO_AXIS_B_GREEN`
- `AUTO_AXIS_B_YELLOW`
- `MANUAL_AXIS_A_GREEN`
- `MANUAL_AXIS_B_GREEN`
- `MANUAL_TRANSITION_A_TO_B`
- `MANUAL_TRANSITION_B_TO_A`
- `FLASHING_YELLOW`

These are conceptual names, not a mandate to use these exact identifiers. The agent should adapt them to the actual codebase.

Engineering constraints:
- Keep state transitions explicit and testable.
- Use one authoritative timer per active phase.
- Avoid long blocking delays in the main control flow.
- Separate controller logic from UI rendering and configuration controls.
- Make flashing behavior timer-driven rather than blocking.
- Cancel or invalidate stale timers when changing modes or phases.
- Handle rapid repeated user input deterministically.

## 5. Suggested Implementation Phases

### Phase 0 — Repository reconnaissance and requirements confirmation
- Inspect repository files, current UI/framework, build scripts, tests, and documentation.
- Read the existing project specification and any current plan.
- Identify what is already implemented and avoid duplicating it.
- Report unresolved decisions before large changes.
- Generate or update `AGENTS.md` based on verified repository facts and project rules; do not invent hardware or toolchain details.

### Phase 1 — Static intersection UI
- Draw the four-way intersection and pedestrian crossings.
- Add vehicle signal housings, colored lamps, pedestrian signal, mode indicator, and countdown areas.
- Use mock/static state first.
- Verify layout at the project's expected screen size and ensure the intersection is legible.

### Phase 2 — Core traffic state machine
- Implement the two-axis cycle and safe transitions.
- Ensure the axes never show green simultaneously.
- Add deterministic phase timing and countdown derivation.
- Add unit tests for transitions if a test framework exists.

### Phase 3 — Auto mode and configuration
- Add configurable green/red timing and yellow transition duration.
- Add validation and update the countdown from the controller's active phase.
- Verify repeated cycles and configuration edge cases.

### Phase 4 — Manual mode
- Add manual axis-switch control.
- Implement immediate request handling with green → yellow → red safety sequence.
- Hide countdown in stable Manual states.
- Test repeated inputs and switching back to Auto.

### Phase 5 — Time-of-day flashing yellow
- Add configurable start/end times and flashing interval.
- Flash all four vehicle signals yellow during the configured window.
- Define behavior when entering and leaving the window.
- Make the time controllable in tests if necessary.

### Phase 6 — Pedestrian display integration
- Synchronize pedestrian red/green indications with vehicle phases.
- Ensure walk is never shown during a conflicting vehicle-green phase.
- Keep pedestrian priority as a separate optional feature unless confirmed in scope.

### Phase 7 — Validation and documentation
- Run available lint, build, and tests.
- Add a concise README with setup/run instructions, mode descriptions, configuration behavior, and known limitations.
- Document the state machine and remaining assumptions.

## 6. Acceptance Criteria

- The screen clearly represents a four-way intersection.
- Vehicle lights and countdown values reflect one authoritative controller state.
- Auto cycles indefinitely without conflicting green signals.
- Green/red timing can be configured.
- Yellow transition is visible and has a defined duration.
- Manual mode can request a change to the perpendicular axis; the current green axis transitions through yellow and red before the perpendicular axis becomes green.
- Countdown is hidden in stable Manual states.
- Flashing-yellow schedule and rate can be configured, and all four vehicle signals flash yellow during the configured time window.
- Pedestrian indication remains consistent with vehicle right-of-way.
- Rapid input and mode changes do not produce conflicting green signals or stale countdowns.
- Build/tests pass where available, and documentation describes how to run the simulation.

## 7. Explicitly Out of Scope for the First Milestone

Unless the existing approved specification requires them, do not implement these before the core simulation is complete:
- Physical board GPIO integration or real seven-segment hardware.
- Real-world traffic optimization, sensors, or vehicle detection.
- Multiple intersections or coordinated traffic networks.
- Full pedestrian-priority request workflow.
- Networking, cloud services, accounts, or analytics.
- Elaborate animations that do not improve understanding.

## 8. Open Questions — Resolve by Inspecting the Repository or Asking

1. What framework/runtime is already used by the project?
2. Is the target a browser simulation, desktop UI, embedded LCD, or a combination?
3. Are the supplied images intended only as inspiration, or should the UI closely match them?
4. Should green/red durations be shared across both axes or independently configurable per axis?
5. What should happen to pedestrian signals during flashing yellow?
6. Is pedestrian priority mode still required, or should the first milestone show only a synchronized pedestrian signal?
7. What exact controls should Manual mode expose (e.g. one “switch axis” button or separate axis selection buttons)?
8. Does the project already require physical seven-segment displays? If so, treat them as hardware integration only after inspecting the board and existing specification.

Do not block initial static UI or core state-machine work on optional questions. Make safe, documented assumptions for reversible details and ask before architecture-changing decisions.

## 9. Instruction for the Coding Agent

Start with Phase 0 only. Inspect the repository and the original specification, then:
1. Summarize the current architecture and how to build/run/test it.
2. Compare the current implementation with this plan.
3. Identify contradictions, missing requirements, and assumptions.
4. Create or propose a repository-specific `AGENTS.md` containing verified build/test commands, coding conventions, architecture boundaries, and safety rules. Keep it concise; do not merely copy the entire plan into `AGENTS.md`.
5. Propose the smallest next implementation step and wait for approval before making broad feature changes.

Do not implement the whole project in one pass.