# AGENTS.md — Arm4-Kit Traffic Controller

> Concise reference for coding agents and contributors. Read this before making any changes.

---

## 1. Project Identity

| Item | Detail |
|------|--------|
| **MCU** | STM32F407ZGTx (Cortex-M4, 168 MHz, LQFP144) |
| **HAL** | STM32Cube FW_F4 V1.28.3 (HAL, no RTOS) |
| **Language** | C11 (GNU extensions on) |
| **Build** | CMake + Ninja, toolchain `arm-none-eabi-gcc` |
| **IDE / Flash** | STM32CubeMX (IOC), OpenOCD / ST-Link |
| **Output** | `build/Debug/TRAFFIC_CONTROLLER.elf` |

---

## 2. Hardware Peripherals (verified from IOC + gpio.c)

| Signal | MCU Pin | Direction | Purpose |
|--------|---------|-----------|---------|
| `DEBUG_LED` | PE4 | Output | Debug heartbeat LED |
| `OUTPUT_Y0` | PE5 | Output | General-purpose output 0 |
| `OUTPUT_Y1` | PE6 | Output | General-purpose output 1 |
| `INPUT_X0` | PA6 | Input | Push-button / switch 0 |
| `INPUT_X1` | PA7 | Input | Push-button / switch 1 |
| `INPUT_X2` | PC4 | Input | Push-button / switch 2 |
| `INPUT_X3` | PC5 | Input | Push-button / switch 3 |
| `LD_LATCH` | PG6 | Output | 74HC595 latch for 7-seg |
| `SPI1` | PB3/4/5 | SPI Master | Drives shift register → 7-seg |
| `TIM2` | — | IRQ | 1 ms base tick (Prescaler=839, Period=99, APB1×2=84 MHz) |

**7-Segment display** — 4-digit, multiplexed via SPI + 74HC595 shift register.  
`led7_Scan()` is called every 1 ms from the `TIM2` ISR; do not block in the ISR.

There are **no onboard traffic-light LEDs** beyond the 3 corner LEDs (`led_On/Off` index 6–8) controlled through the same SPI shift register. Traffic-signal color state must be represented either by those LEDs or by the 7-seg display.

---

## 3. Source-Tree Layout

```
App/
  Inc/app.h          – Public init + run API
  Src/app.c          – Application logic (USER code here)
Bsp/
  Inc/led_7_seg.h    – 7-seg API
  Inc/software_timer.h
  Src/led_7_seg.c    – SPI-based 4-digit display driver
  Src/software_timer.c – TIM2-based software timer
Core/
  Inc/main.h         – GPIO pin aliases, Error_Handler()
  Src/main.c         – HAL init → app_init() → while(app_run())
  Src/gpio.c         – MX_GPIO_Init()
  Src/spi.c          – MX_SPI1_Init()
  Src/tim.c          – MX_TIM2_Init()  (1 ms tick)
  Src/stm32f4xx_it.c – ISR stubs; TIM2_IRQHandler → HAL_TIM_IRQHandler
Drivers/             – ST HAL (do not modify)
docs/
  PLAN.md            – High-level product plan
  AGENTS.md          – This file
FIRMWARE1.ioc        – STM32CubeMX project
CMakeLists.txt       – User-managed build additions
cmake/stm32cubemx/CMakeLists.txt – CubeMX-managed (minimize edits)
```

**Rule**: Only edit files inside `App/`, `Bsp/`, and the `/* USER CODE */` sections of `Core/`. Never modify `Drivers/` or CubeMX-generated boilerplate outside those guards.

---

## 4. Build Commands

> Requires `arm-none-eabi-gcc` and `ninja` on PATH.

```powershell
# Configure (first time or after CMakeLists changes)
cmake --preset Debug

# Build
cmake --build --preset Debug

# Check binary size
arm-none-eabi-size build/Debug/TRAFFIC_CONTROLLER.elf
```

The `.elf` is ready to flash via OpenOCD or STM32CubeProgrammer.  
There are **no automated unit tests** in the repository at this time.

---

## 5. Timing Architecture

| Layer | Mechanism |
|-------|-----------|
| **1 ms ISR** | `TIM2_IRQHandler → HAL_TIM_PeriodElapsedCallback` |
| **7-seg scan** | Called every 1 ms inside the TIM2 callback |
| **Software timer 2** | `setTimer2(ms)` — fires `flag_timer2` every N ms |
| **App loop** | `main()` calls `app_run()` in a tight while(1) |

`flag_timer2` is a `volatile uint16_t` polled in `app_run()`. The timer auto-reloads (`timer2_counter` resets to `timer2_MUL`) — it is a **periodic** flag, not a one-shot.

To add a new periodic task at a different rate, either:
- Add a second software-timer slot in `software_timer.c`, or
- Use a divisor counter derived from `flag_timer2`.

**Critical**: `setTimer2(0)` disables the counter (MUL=0, counter stays 0 → never fires).

---

## 6. Current Implementation State

### Implemented ✅
- HAL init, clock (168 MHz HSI-PLL), SPI1, TIM2 at 1 ms
- 4-digit 7-seg driver (`led7_SetDigit`, `led7_Scan`, `led7_SetColon`)
- 3 corner LEDs (`led_On`, `led_Off`, indices 6–8)
- 4 GPIO inputs (`INPUT_X0–X3`)
- 2 GPIO outputs (`OUTPUT_Y0–Y1`) — purpose TBD
- 1-second periodic countdown skeleton in `app.c`

### Not Implemented / Broken ❌
- **`app.c` does not compile**: `display_countdown()` references undefined locals `minutes` and `seconds_remainder`; `TRAFFIC_CYCLE_SECONDS` macro is undefined
- **No traffic state machine** — no phases, no axes, no green/yellow/red logic
- **No mode switching** (Auto / Manual)
- **No button debounce or input handling**
- **No flashing-yellow schedule**
- **No pedestrian signal logic**
- **No README** in the repository root

---

## 7. Coding Conventions

- **C11**, `uint8_t`/`uint16_t` for hardware registers; use explicit-width types in BSP/App
- Compiler flags include `-Wall -Wextra -Wpedantic` — all new code must compile cleanly
- CubeMX user-code is placed only inside `/* USER CODE BEGIN … */ … /* USER CODE END … */` guards
- `app.c` / `app.h` is the approved entry point for application logic
- BSP drivers (`Bsp/`) own hardware abstraction; `App/` must not call HAL directly
- Global variables shared between ISR and main loop must be `volatile`
- Avoid `HAL_Delay()` in the main loop; use flag-based or timer-based scheduling

---

## 8. Architecture Rules (from PLAN.md)

1. **Controller is source of truth** — the state machine owns phase/timing; the display renders it.
2. **Conflicting axes must never both be green** — enforce in the state transition function, not in the display.
3. **One authoritative timer per active phase** — invalidate stale timers on mode/phase change.
4. **No long blocking delays** in the main control loop.
5. **Flashing behavior must be timer-driven**, not `HAL_Delay`-based.
6. **Rapid input must be handled deterministically** — debounce inputs before acting on them.

---

## 9. Open Hardware Questions (resolve before implementing those features)

| # | Question |
|---|----------|
| H1 | What do `OUTPUT_Y0` and `OUTPUT_Y1` drive? (relay, external LED, buzzer?) |
| H2 | Are the 3 corner LEDs (indices 6–8) intended for Red/Yellow/Green traffic signals? |
| H3 | The board has no RTC. For flashing-yellow schedule, is a simulated/settable software clock acceptable? |
| H4 | Inputs `INPUT_X0–X3`: which buttons map to Auto/Manual toggle and axis-switch request? |

---

## 10. Suggested Implementation Order

Following PLAN.md §5, work one phase at a time and verify the build before committing:

| Phase | Focus | Entry Point |
|-------|-------|-------------|
| **1** | Fix `app.c` compile errors; add traffic state enum + placeholder display | `App/Src/app.c` |
| **2** | Core two-axis state machine (green/yellow/red, no simultaneous green) | `App/Src/traffic_ctrl.c` (new) |
| **3** | Auto mode, configurable durations, 1-second countdown on 7-seg | `App/Src/traffic_ctrl.c` |
| **4** | Button debounce + Manual mode (axis-switch, hide countdown in stable state) | `App/Src/button.c` (new) |
| **5** | Flashing-yellow: software clock, configurable window, all-yellow flash rate | `App/Src/traffic_ctrl.c` |
| **6** | Pedestrian signal (corner LED or 7-seg indicator, safe phasing) | `App/Src/traffic_ctrl.c` |
| **7** | README, state-machine diagram, validation | `docs/` |
