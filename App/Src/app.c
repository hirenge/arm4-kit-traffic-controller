/*
 * app.c
 */

#include "app.h"
#include "lcd.h"
#include "software_timer.h"
#include "traffic_ui.h"
#include "vehicle_sim.h"

#define AUTO_GREEN_SECONDS  7U
#define AUTO_YELLOW_SECONDS 3U
#define APP_TICK_MS          100U
#define TICKS_PER_SECOND     (1000U / APP_TICK_MS)

typedef enum
{
    AUTO_NS_GREEN = 0,
    AUTO_NS_YELLOW,
    AUTO_EW_GREEN,
    AUTO_EW_YELLOW
} auto_phase_t;

static auto_phase_t phase;
static uint8_t seconds_remaining;
static uint8_t subsecond_ticks;

static void current_lamps(traffic_lamp_t *ns_lamp,
                          traffic_lamp_t *ew_lamp)
{
    *ns_lamp = TRAFFIC_LAMP_RED;
    *ew_lamp = TRAFFIC_LAMP_RED;

    if (phase == AUTO_NS_GREEN)
    {
        *ns_lamp = TRAFFIC_LAMP_GREEN;
    }
    else if (phase == AUTO_NS_YELLOW)
    {
        *ns_lamp = TRAFFIC_LAMP_YELLOW;
    }
    else if (phase == AUTO_EW_GREEN)
    {
        *ew_lamp = TRAFFIC_LAMP_GREEN;
    }
    else if (phase == AUTO_EW_YELLOW)
    {
        *ew_lamp = TRAFFIC_LAMP_YELLOW;
    }
}

static void render_phase(void)
{
    traffic_lamp_t ns_lamp;
    traffic_lamp_t ew_lamp;
    uint8_t ns_countdown = seconds_remaining;
    uint8_t ew_countdown = seconds_remaining;

    current_lamps(&ns_lamp, &ew_lamp);

    switch (phase)
    {
        case AUTO_NS_GREEN:
            ew_countdown = seconds_remaining + AUTO_YELLOW_SECONDS;
            break;

        case AUTO_NS_YELLOW:
            break;

        case AUTO_EW_GREEN:
            ns_countdown = seconds_remaining + AUTO_YELLOW_SECONDS;
            break;

        case AUTO_EW_YELLOW:
            break;

        default:
            break;
    }

    traffic_ui_render(ns_lamp, ew_lamp, ns_countdown, ew_countdown);
}

static void advance_phase(void)
{
    switch (phase)
    {
        case AUTO_NS_GREEN:
            phase = AUTO_NS_YELLOW;
            seconds_remaining = AUTO_YELLOW_SECONDS;
            break;

        case AUTO_NS_YELLOW:
            phase = AUTO_EW_GREEN;
            seconds_remaining = AUTO_GREEN_SECONDS;
            break;

        case AUTO_EW_GREEN:
            phase = AUTO_EW_YELLOW;
            seconds_remaining = AUTO_YELLOW_SECONDS;
            break;

        case AUTO_EW_YELLOW:
        default:
            phase = AUTO_NS_GREEN;
            seconds_remaining = AUTO_GREEN_SECONDS;
            break;
    }
}

void app_init(void)
{
    lcd_init();
    background_draw();
    traffic_ui_init();

    phase = AUTO_NS_GREEN;
    seconds_remaining = AUTO_GREEN_SECONDS;
    subsecond_ticks = 0U;
    vehicle_sim_init();
    render_phase();

    timer_init();
    setTimer2(APP_TICK_MS);
}

void app_run(void)
{
    if (flag_timer2 != 0U)
    {
        flag_timer2 = 0U;

        subsecond_ticks++;
        if (subsecond_ticks >= TICKS_PER_SECOND)
        {
            subsecond_ticks = 0U;
            if (seconds_remaining > 1U)
            {
                seconds_remaining--;
            }
            else
            {
                advance_phase();
            }
        }

        traffic_lamp_t ns_lamp;
        traffic_lamp_t ew_lamp;
        current_lamps(&ns_lamp, &ew_lamp);
        vehicle_sim_step(ns_lamp, ew_lamp);
        render_phase();
    }
}
