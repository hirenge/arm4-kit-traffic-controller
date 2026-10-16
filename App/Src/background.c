/*
 * background.c
 *
 * Draws the static four-way intersection background on the 240×320 LCD.
 * Render order:
 *   1. Base layer  — asphalt fill + grass quadrants + rounded corners
 *   2. Zebra crossings (4 sides)
 *   3. Road center dashes (N-S and E-W)
 *
 * All pixel math uses the constants defined in background.h.
 * Nothing here should be called more than once.
 */

#include "background.h"
#include "lcd.h"

/* ── 1. Base layer ───────────────────────────────────────────────── */

/**
 * @brief  Fill the asphalt cross, then overlay grass in the 4 corners.
 *         Finally round the 4 intersection corners with filled circles.
 */
static void draw_base(void)
{
    /* Entire screen → road color first (simplest way to get the cross) */
    lcd_Fill(0U, 0U, SCR_W, SCR_H, CLR_ROAD);

    /* Grass quadrants */
    lcd_Fill(0U,     0U,     NS_X1, EW_Y1, CLR_GRASS);  /* top-left  */
    lcd_Fill(NS_X2,  0U,     SCR_W, EW_Y1, CLR_GRASS);  /* top-right */
    lcd_Fill(0U,     EW_Y2,  NS_X1, SCR_H, CLR_GRASS);  /* bot-left  */
    lcd_Fill(NS_X2,  EW_Y2,  SCR_W, SCR_H, CLR_GRASS);  /* bot-right */

    /* Round the 4 intersection corners:
     * A filled road-color circle at each corner "eats into" the grass
     * and creates the curved kerb visible in the reference image.        */
    lcd_DrawCircle(NS_X1, EW_Y1, CLR_ROAD, CORNER_R, 1); /* top-left  */
    lcd_DrawCircle(NS_X2, EW_Y1, CLR_ROAD, CORNER_R, 1); /* top-right */
    lcd_DrawCircle(NS_X1, EW_Y2, CLR_ROAD, CORNER_R, 1); /* bot-left  */
    lcd_DrawCircle(NS_X2, EW_Y2, CLR_ROAD, CORNER_R, 1); /* bot-right */
}

/* ── 2. Zebra crossings ──────────────────────────────────────────── */

/** Top pedestrian crossing — horizontal stripes on the N-S road,
 *  just above the E-W intersection. */
static void draw_zebra_top(void)
{
    uint16_t y = EW_Y1;
    for (uint8_t i = 0U; i < ZEBRA_COUNT; i++)
    {
        y -= ZEBRA_STRIPE;
        lcd_Fill(NS_X1, y, NS_X2, y + ZEBRA_STRIPE, CLR_LINE);
        y -= ZEBRA_GAP;
    }
}

/** Bottom pedestrian crossing — horizontal stripes, just below E-W. */
static void draw_zebra_bottom(void)
{
    uint16_t y = EW_Y2 + 1U;
    for (uint8_t i = 0U; i < ZEBRA_COUNT; i++)
    {
        lcd_Fill(NS_X1, y, NS_X2, y + ZEBRA_STRIPE, CLR_LINE);
        y += ZEBRA_STRIPE + ZEBRA_GAP;
    }
}

/** Left pedestrian crossing — vertical stripes on the E-W road,
 *  just left of the N-S intersection. */
static void draw_zebra_left(void)
{
    uint16_t x = NS_X1;
    for (uint8_t i = 0U; i < ZEBRA_COUNT; i++)
    {
        x -= ZEBRA_STRIPE;
        lcd_Fill(x, EW_Y1, x + ZEBRA_STRIPE, EW_Y2, CLR_LINE);
        x -= ZEBRA_GAP;
    }
}

/** Right pedestrian crossing — vertical stripes, just right of N-S. */
static void draw_zebra_right(void)
{
    uint16_t x = NS_X2 + 1U;
    for (uint8_t i = 0U; i < ZEBRA_COUNT; i++)
    {
        lcd_Fill(x, EW_Y1, x + ZEBRA_STRIPE, EW_Y2, CLR_LINE);
        x += ZEBRA_STRIPE + ZEBRA_GAP;
    }
}

/* ── 3. Road center dashes ───────────────────────────────────────── */

/**
 * @brief  Draw dashed center lines on both axes.
 *         Dashes are skipped inside the intersection box and zebra zones.
 */
static void draw_center_dashes(void)
{
    /* Center pixel of each road axis */
    const uint16_t cx = ((NS_X1 + NS_X2) / 2U) - (DASH_THICK / 2U);  /* ~118 */
    const uint16_t cy = ((EW_Y1 + EW_Y2) / 2U) - (DASH_THICK / 2U);  /* ~157 */

    /* N-S dashes — above E-W road */
    for (uint16_t y = DASH_GAP; y + DASH_LEN < EW_Y1; y += DASH_LEN + DASH_GAP)
    {
        lcd_Fill(cx, y, cx + DASH_THICK, y + DASH_LEN, CLR_LINE);
    }

    /* N-S dashes — below E-W road */
    for (uint16_t y = EW_Y2 + DASH_GAP; y + DASH_LEN < SCR_H; y += DASH_LEN + DASH_GAP)
    {
        lcd_Fill(cx, y, cx + DASH_THICK, y + DASH_LEN, CLR_LINE);
    }

    /* E-W dashes — left of N-S road */
    for (uint16_t x = DASH_GAP; x + DASH_LEN < NS_X1; x += DASH_LEN + DASH_GAP)
    {
        lcd_Fill(x, cy, x + DASH_LEN, cy + DASH_THICK, CLR_LINE);
    }

    /* E-W dashes — right of N-S road */
    for (uint16_t x = NS_X2 + DASH_GAP; x + DASH_LEN < SCR_W; x += DASH_LEN + DASH_GAP)
    {
        lcd_Fill(x, cy, x + DASH_LEN, cy + DASH_THICK, CLR_LINE);
    }
}

/* ── Public API ──────────────────────────────────────────────────── */

void background_draw(void)
{
    draw_base();
    draw_zebra_top();
    draw_zebra_bottom();
    draw_zebra_left();
    draw_zebra_right();
    draw_center_dashes();
}
