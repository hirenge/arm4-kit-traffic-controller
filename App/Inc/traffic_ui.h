#ifndef APP_INC_TRAFFIC_UI_H_
#define APP_INC_TRAFFIC_UI_H_

#include <stdint.h>

/* LCD and intersection layout (240 x 320, portrait L2R_U2D). */
#define SCR_W            240U
#define SCR_H            320U

#define NS_X1            86U
#define NS_X2            154U
#define EW_Y1            125U
#define EW_Y2            195U

#define ZEBRA_STRIPE     4U
#define ZEBRA_GAP        4U
#define ZEBRA_COUNT      8U
#define ZEBRA_SPAN       (ZEBRA_COUNT * ZEBRA_STRIPE + \
                          (ZEBRA_COUNT - 1U) * ZEBRA_GAP)
#define CROSSING_DEPTH   18U
#define CROSSING_SETBACK 4U

#define CROSS_TOP_Y      (EW_Y1 - CROSSING_SETBACK - CROSSING_DEPTH)
#define CROSS_BOTTOM_Y   (EW_Y2 + CROSSING_SETBACK)
#define CROSS_LEFT_X     (NS_X1 - CROSSING_SETBACK - CROSSING_DEPTH)
#define CROSS_RIGHT_X    (NS_X2 + CROSSING_SETBACK)

#define DASH_LEN         16U
#define DASH_GAP         12U
#define DASH_THICK       2U

#define CLR_GRASS        GREEN
#define CLR_ROAD         0x4208U
#define CLR_LINE         WHITE

typedef enum
{
    TRAFFIC_LAMP_RED = 0,
    TRAFFIC_LAMP_YELLOW,
    TRAFFIC_LAMP_GREEN
} traffic_lamp_t;

void background_draw(void);
void background_restore_rect(int16_t x, int16_t y,
                             uint16_t width, uint16_t height);

void traffic_ui_init(void);
void traffic_ui_render(traffic_lamp_t ns_lamp,
                       traffic_lamp_t ew_lamp,
                       uint8_t ns_countdown,
                       uint8_t ew_countdown);

#endif /* APP_INC_TRAFFIC_UI_H_ */
