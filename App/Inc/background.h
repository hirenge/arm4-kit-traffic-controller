/*
 * background.h
 *
 * Static four-way intersection background for the LCD.
 * Call background_draw() once at startup — never needs to be redrawn.
 */

#ifndef APP_INC_BACKGROUND_H_
#define APP_INC_BACKGROUND_H_

/* ── Layout constants (pixels, 240 × 320 portrait L2R_U2D) ───────── */

#define SCR_W           240U    /* screen width  */
#define SCR_H           320U    /* screen height */

/* N-S road (vertical, centered horizontally) */
#define NS_X1           86U     /* left  edge of N-S road */
#define NS_X2           154U    /* right edge of N-S road */

/* E-W road (horizontal, centered vertically) */
#define EW_Y1           125U    /* top    edge of E-W road */
#define EW_Y2           195U    /* bottom edge of E-W road */

/* Rounded corner radius at the four intersection corners */
#define CORNER_R        22U

/* Zebra crossing */
#define ZEBRA_STRIPE    7U      /* white stripe thickness (px) */
#define ZEBRA_GAP       6U      /* dark gap between stripes  (px) */
#define ZEBRA_COUNT     5U      /* number of white stripes per crossing */

/* Road center dashes */
#define DASH_LEN        16U     /* length of each dash (px) */
#define DASH_GAP        12U     /* gap between dashes  (px) */
#define DASH_THICK      3U      /* thickness of dash   (px) */

/* ── Color palette ───────────────────────────────────────────────── */
#define CLR_GRASS       GREEN   /* 0x07E0 — sidewalk / grass quadrants */
#define CLR_ROAD        0x4208U /* dark asphalt gray                   */
#define CLR_LINE        WHITE   /* center dashes + zebra stripes       */

/* ── Public API ──────────────────────────────────────────────────── */

/**
 * @brief  Draw the complete static intersection background.
 * @note   Must be called once after lcd_init(). Do NOT call again
 *         during normal operation — traffic light objects draw on top.
 */
void background_draw(void);

#endif /* APP_INC_BACKGROUND_H_ */
