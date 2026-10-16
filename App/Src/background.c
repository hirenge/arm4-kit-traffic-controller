/*
 * background.c
 *
 * Draws the static four-way intersection background on the 240×320 LCD.
 * Render order:
 *   1. Base layer  — asphalt fill + grass quadrants
 *   2. Zebra crossings (4 sides)
 *   3. Road center dashes (N-S and E-W)
 *
 * All pixel math uses the shared layout constants in traffic_ui.h.
 * Nothing here should be called more than once.
 */

#include "traffic_ui.h"
#include "lcd.h"

typedef struct
{
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
} background_rect_t;

static void fill_clipped(const background_rect_t *clip,
                         uint16_t x1, uint16_t y1,
                         uint16_t x2, uint16_t y2,
                         uint16_t color)
{
    const uint16_t clipped_x1 = (x1 > clip->x1) ? x1 : clip->x1;
    const uint16_t clipped_y1 = (y1 > clip->y1) ? y1 : clip->y1;
    const uint16_t clipped_x2 = (x2 < clip->x2) ? x2 : clip->x2;
    const uint16_t clipped_y2 = (y2 < clip->y2) ? y2 : clip->y2;

    if ((clipped_x1 < clipped_x2) && (clipped_y1 < clipped_y2))
    {
        lcd_Fill(clipped_x1, clipped_y1, clipped_x2, clipped_y2, color);
    }
}

/* ── 1. Base layer ───────────────────────────────────────────────── */

/**
 * @brief  Fill the asphalt cross, then overlay grass in the 4 corners.
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

}

/* ── 2. Zebra crossings ──────────────────────────────────────────── */

/** Top pedestrian crossing — vertical stripes across the N-S road. */
static void draw_zebra_top(void)
{
    const uint16_t first_x = NS_X1 + ((NS_X2 - NS_X1 - ZEBRA_SPAN) / 2U);
    for (uint8_t i = 0U; i < ZEBRA_COUNT; i++)
    {
        const uint16_t x = first_x + i * (ZEBRA_STRIPE + ZEBRA_GAP);
        lcd_Fill(x, CROSS_TOP_Y, x + ZEBRA_STRIPE,
                 CROSS_TOP_Y + CROSSING_DEPTH, CLR_LINE);
    }
}

/** Bottom pedestrian crossing — vertical stripes across the N-S road. */
static void draw_zebra_bottom(void)
{
    const uint16_t first_x = NS_X1 + ((NS_X2 - NS_X1 - ZEBRA_SPAN) / 2U);
    for (uint8_t i = 0U; i < ZEBRA_COUNT; i++)
    {
        const uint16_t x = first_x + i * (ZEBRA_STRIPE + ZEBRA_GAP);
        lcd_Fill(x, CROSS_BOTTOM_Y, x + ZEBRA_STRIPE,
                 CROSS_BOTTOM_Y + CROSSING_DEPTH, CLR_LINE);
    }
}

/** Left pedestrian crossing — horizontal stripes across the E-W road. */
static void draw_zebra_left(void)
{
    const uint16_t first_y = EW_Y1 + ((EW_Y2 - EW_Y1 - ZEBRA_SPAN) / 2U);
    for (uint8_t i = 0U; i < ZEBRA_COUNT; i++)
    {
        const uint16_t y = first_y + i * (ZEBRA_STRIPE + ZEBRA_GAP);
        lcd_Fill(CROSS_LEFT_X, y, CROSS_LEFT_X + CROSSING_DEPTH,
                 y + ZEBRA_STRIPE, CLR_LINE);
    }
}

/** Right pedestrian crossing — horizontal stripes across the E-W road. */
static void draw_zebra_right(void)
{
    const uint16_t first_y = EW_Y1 + ((EW_Y2 - EW_Y1 - ZEBRA_SPAN) / 2U);
    for (uint8_t i = 0U; i < ZEBRA_COUNT; i++)
    {
        const uint16_t y = first_y + i * (ZEBRA_STRIPE + ZEBRA_GAP);
        lcd_Fill(CROSS_RIGHT_X, y, CROSS_RIGHT_X + CROSSING_DEPTH,
                 y + ZEBRA_STRIPE, CLR_LINE);
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
    for (uint16_t y = DASH_GAP; y + DASH_LEN <= CROSS_TOP_Y - CROSSING_SETBACK; y += DASH_LEN + DASH_GAP)
    {
        lcd_Fill(cx, y, cx + DASH_THICK, y + DASH_LEN, CLR_LINE);
    }

    /* N-S dashes — below E-W road */
    for (uint16_t y = CROSS_BOTTOM_Y + CROSSING_DEPTH + DASH_GAP;
         y + DASH_LEN < SCR_H; y += DASH_LEN + DASH_GAP)
    {
        lcd_Fill(cx, y, cx + DASH_THICK, y + DASH_LEN, CLR_LINE);
    }

    /* E-W dashes — left of N-S road */
    for (uint16_t x = DASH_GAP; x + DASH_LEN <= CROSS_LEFT_X - CROSSING_SETBACK; x += DASH_LEN + DASH_GAP)
    {
        lcd_Fill(x, cy, x + DASH_LEN, cy + DASH_THICK, CLR_LINE);
    }

    /* E-W dashes — right of N-S road */
    for (uint16_t x = CROSS_RIGHT_X + CROSSING_DEPTH + DASH_GAP;
         x + DASH_LEN < SCR_W; x += DASH_LEN + DASH_GAP)
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

void background_restore_rect(int16_t x, int16_t y,
                             uint16_t width, uint16_t height)
{
    const int32_t requested_x2 = (int32_t)x + width;
    const int32_t requested_y2 = (int32_t)y + height;
    background_rect_t clip;

    if ((width == 0U) || (height == 0U) ||
        (requested_x2 <= 0) || (requested_y2 <= 0) ||
        (x >= (int16_t)SCR_W) || (y >= (int16_t)SCR_H))
    {
        return;
    }

    clip.x1 = (x > 0) ? (uint16_t)x : 0U;
    clip.y1 = (y > 0) ? (uint16_t)y : 0U;
    clip.x2 = (requested_x2 < (int32_t)SCR_W) ?
              (uint16_t)requested_x2 : SCR_W;
    clip.y2 = (requested_y2 < (int32_t)SCR_H) ?
              (uint16_t)requested_y2 : SCR_H;

    /* Rebuild the base layer inside the damaged rectangle. */
    fill_clipped(&clip, 0U, 0U, SCR_W, SCR_H, CLR_ROAD);
    fill_clipped(&clip, 0U, 0U, NS_X1, EW_Y1, CLR_GRASS);
    fill_clipped(&clip, NS_X2, 0U, SCR_W, EW_Y1, CLR_GRASS);
    fill_clipped(&clip, 0U, EW_Y2, NS_X1, SCR_H, CLR_GRASS);
    fill_clipped(&clip, NS_X2, EW_Y2, SCR_W, SCR_H, CLR_GRASS);

    /* Restore zebra stripes that intersect the damaged rectangle. */
    const uint16_t first_x = NS_X1 + ((NS_X2 - NS_X1 - ZEBRA_SPAN) / 2U);
    const uint16_t first_y = EW_Y1 + ((EW_Y2 - EW_Y1 - ZEBRA_SPAN) / 2U);
    for (uint8_t i = 0U; i < ZEBRA_COUNT; i++)
    {
        const uint16_t stripe_x = first_x + i * (ZEBRA_STRIPE + ZEBRA_GAP);
        const uint16_t stripe_y = first_y + i * (ZEBRA_STRIPE + ZEBRA_GAP);

        fill_clipped(&clip, stripe_x, CROSS_TOP_Y,
                     stripe_x + ZEBRA_STRIPE,
                     CROSS_TOP_Y + CROSSING_DEPTH, CLR_LINE);
        fill_clipped(&clip, stripe_x, CROSS_BOTTOM_Y,
                     stripe_x + ZEBRA_STRIPE,
                     CROSS_BOTTOM_Y + CROSSING_DEPTH, CLR_LINE);
        fill_clipped(&clip, CROSS_LEFT_X, stripe_y,
                     CROSS_LEFT_X + CROSSING_DEPTH,
                     stripe_y + ZEBRA_STRIPE, CLR_LINE);
        fill_clipped(&clip, CROSS_RIGHT_X, stripe_y,
                     CROSS_RIGHT_X + CROSSING_DEPTH,
                     stripe_y + ZEBRA_STRIPE, CLR_LINE);
    }

    /* Restore lane-divider dashes. */
    const uint16_t center_x = ((NS_X1 + NS_X2) / 2U) - (DASH_THICK / 2U);
    const uint16_t center_y = ((EW_Y1 + EW_Y2) / 2U) - (DASH_THICK / 2U);

    for (uint16_t dash_y = DASH_GAP;
         dash_y + DASH_LEN <= CROSS_TOP_Y - CROSSING_SETBACK;
         dash_y += DASH_LEN + DASH_GAP)
    {
        fill_clipped(&clip, center_x, dash_y,
                     center_x + DASH_THICK, dash_y + DASH_LEN, CLR_LINE);
    }
    for (uint16_t dash_y = CROSS_BOTTOM_Y + CROSSING_DEPTH + DASH_GAP;
         dash_y + DASH_LEN < SCR_H;
         dash_y += DASH_LEN + DASH_GAP)
    {
        fill_clipped(&clip, center_x, dash_y,
                     center_x + DASH_THICK, dash_y + DASH_LEN, CLR_LINE);
    }
    for (uint16_t dash_x = DASH_GAP;
         dash_x + DASH_LEN <= CROSS_LEFT_X - CROSSING_SETBACK;
         dash_x += DASH_LEN + DASH_GAP)
    {
        fill_clipped(&clip, dash_x, center_y,
                     dash_x + DASH_LEN, center_y + DASH_THICK, CLR_LINE);
    }
    for (uint16_t dash_x = CROSS_RIGHT_X + CROSSING_DEPTH + DASH_GAP;
         dash_x + DASH_LEN < SCR_W;
         dash_x += DASH_LEN + DASH_GAP)
    {
        fill_clipped(&clip, dash_x, center_y,
                     dash_x + DASH_LEN, center_y + DASH_THICK, CLR_LINE);
    }
}
