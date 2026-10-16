#include "traffic_ui.h"

#include <stdbool.h>

#include "lcd.h"

#define SIGNAL_THICKNESS 24U
#define SIGNAL_LENGTH    76U
#define SIGNAL_POST_LEN  8U
#define ROAD_EDGE_GAP    2U
#define SIDE_EDGE_GAP    4U

#define PED_W           22U
#define PED_H           46U
#define PED_POST_LEN    8U
#define HOUSING_EDGE    LIGHTGRAY
#define LAMP_OFF_RED    0x3000U
#define LAMP_OFF_YELLOW 0x3180U
#define LAMP_OFF_GREEN  0x0180U
#define COUNTDOWN_COLOR 0xFD20U

typedef enum
{
    SIGNAL_ROTATE_0 = 0,
    SIGNAL_ROTATE_90,
    SIGNAL_ROTATE_MINUS_90,
    SIGNAL_ROTATE_180
} signal_rotation_t;

typedef struct
{
    uint16_t signal_x;
    uint16_t signal_y;
    uint16_t ped_x;
    uint16_t ped_y;
    signal_rotation_t rotation;
    bool north_south;
} corner_display_t;

static const corner_display_t corners[] = {
    /* top-left: 180 degrees; green and post point upward */
    {58U,  EW_Y1 - ROAD_EDGE_GAP - SIGNAL_LENGTH,
     33U,  EW_Y1 - ROAD_EDGE_GAP - PED_H,
     SIGNAL_ROTATE_180, true},
    /* top-right: -90 degrees; green and post point right */
    {156U, EW_Y1 - SIDE_EDGE_GAP - SIGNAL_THICKNESS,
     156U, 73U, SIGNAL_ROTATE_MINUS_90, false},
    /* bottom-left: 90 degrees; green and post point left */
    {8U, EW_Y2 + SIDE_EDGE_GAP,
     38U, 227U, SIGNAL_ROTATE_90, false},
    /* bottom-right is the reference orientation */
    {158U, EW_Y2 + ROAD_EDGE_GAP,
     186U, EW_Y2 + ROAD_EDGE_GAP,
     SIGNAL_ROTATE_0, true}
};

static const uint8_t digit_rows[10][5] = {
    {0x07U, 0x05U, 0x05U, 0x05U, 0x07U}, /* 0 */
    {0x02U, 0x06U, 0x02U, 0x02U, 0x07U}, /* 1 */
    {0x07U, 0x01U, 0x07U, 0x04U, 0x07U}, /* 2 */
    {0x07U, 0x01U, 0x07U, 0x01U, 0x07U}, /* 3 */
    {0x05U, 0x05U, 0x07U, 0x01U, 0x01U}, /* 4 */
    {0x07U, 0x04U, 0x07U, 0x01U, 0x07U}, /* 5 */
    {0x07U, 0x04U, 0x07U, 0x05U, 0x07U}, /* 6 */
    {0x07U, 0x01U, 0x02U, 0x02U, 0x02U}, /* 7 */
    {0x07U, 0x05U, 0x07U, 0x05U, 0x07U}, /* 8 */
    {0x07U, 0x05U, 0x07U, 0x01U, 0x07U}  /* 9 */
};

static void map_local_point(uint16_t origin_x,
                            uint16_t origin_y,
                            uint16_t logical_width,
                            uint16_t logical_height,
                            signal_rotation_t rotation,
                            uint16_t local_x,
                            uint16_t local_y,
                            uint16_t *screen_x,
                            uint16_t *screen_y)
{
    switch (rotation)
    {
        case SIGNAL_ROTATE_90:
            *screen_x = origin_x + logical_height - 1U - local_y;
            *screen_y = origin_y + local_x;
            break;

        case SIGNAL_ROTATE_MINUS_90:
            *screen_x = origin_x + local_y;
            *screen_y = origin_y + logical_width - 1U - local_x;
            break;

        case SIGNAL_ROTATE_180:
            *screen_x = origin_x + logical_width - 1U - local_x;
            *screen_y = origin_y + logical_height - 1U - local_y;
            break;

        case SIGNAL_ROTATE_0:
        default:
            *screen_x = origin_x + local_x;
            *screen_y = origin_y + local_y;
            break;
    }
}

static void fill_local_rect(uint16_t origin_x,
                            uint16_t origin_y,
                            uint16_t logical_width,
                            uint16_t logical_height,
                            signal_rotation_t rotation,
                            uint16_t local_x,
                            uint16_t local_y,
                            uint16_t width,
                            uint16_t height,
                            uint16_t color)
{
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;

    map_local_point(origin_x, origin_y, logical_width, logical_height,
                    rotation, local_x, local_y, &x1, &y1);
    map_local_point(origin_x, origin_y, logical_width, logical_height,
                    rotation, local_x + width - 1U, local_y + height - 1U,
                    &x2, &y2);

    const uint16_t min_x = (x1 < x2) ? x1 : x2;
    const uint16_t min_y = (y1 < y2) ? y1 : y2;
    const uint16_t max_x = (x1 > x2) ? x1 : x2;
    const uint16_t max_y = (y1 > y2) ? y1 : y2;
    lcd_Fill(min_x, min_y, max_x + 1U, max_y + 1U, color);
}

static void draw_local_line(uint16_t origin_x,
                            uint16_t origin_y,
                            uint16_t logical_width,
                            uint16_t logical_height,
                            signal_rotation_t rotation,
                            uint16_t x1,
                            uint16_t y1,
                            uint16_t x2,
                            uint16_t y2,
                            uint16_t color)
{
    uint16_t screen_x1;
    uint16_t screen_y1;
    uint16_t screen_x2;
    uint16_t screen_y2;

    map_local_point(origin_x, origin_y, logical_width, logical_height,
                    rotation, x1, y1, &screen_x1, &screen_y1);
    map_local_point(origin_x, origin_y, logical_width, logical_height,
                    rotation, x2, y2, &screen_x2, &screen_y2);
    lcd_DrawLine(screen_x1, screen_y1, screen_x2, screen_y2, color);
}

static void draw_signal_housing(const corner_display_t *corner)
{
    const uint16_t x = corner->signal_x;
    const uint16_t y = corner->signal_y;

    if ((corner->rotation == SIGNAL_ROTATE_0) ||
        (corner->rotation == SIGNAL_ROTATE_180))
    {
        lcd_Fill(x, y, x + SIGNAL_THICKNESS, y + SIGNAL_LENGTH, HOUSING_EDGE);
        lcd_Fill(x + 2U, y + 2U, x + SIGNAL_THICKNESS - 2U,
                 y + SIGNAL_LENGTH - 2U, BLACK);

        if (corner->rotation == SIGNAL_ROTATE_0)
        {
            lcd_DrawLine(x + 2U, y + 20U,
                         x + SIGNAL_THICKNESS - 3U, y + 20U, HOUSING_EDGE);
            lcd_Fill(x + 10U, y + SIGNAL_LENGTH, x + 14U,
                     y + SIGNAL_LENGTH + SIGNAL_POST_LEN, BLACK);
        }
        else
        {
            lcd_DrawLine(x + 2U, y + SIGNAL_LENGTH - 21U,
                         x + SIGNAL_THICKNESS - 3U,
                         y + SIGNAL_LENGTH - 21U, HOUSING_EDGE);
            lcd_Fill(x + 10U, y - SIGNAL_POST_LEN, x + 14U, y, BLACK);
        }
    }
    else
    {
        lcd_Fill(x, y, x + SIGNAL_LENGTH, y + SIGNAL_THICKNESS, HOUSING_EDGE);
        lcd_Fill(x + 2U, y + 2U, x + SIGNAL_LENGTH - 2U,
                 y + SIGNAL_THICKNESS - 2U, BLACK);

        if (corner->rotation == SIGNAL_ROTATE_MINUS_90)
        {
            lcd_DrawLine(x + 20U, y + 2U,
                         x + 20U, y + SIGNAL_THICKNESS - 3U, HOUSING_EDGE);
            lcd_Fill(x + SIGNAL_LENGTH, y + 10U,
                     x + SIGNAL_LENGTH + SIGNAL_POST_LEN, y + 14U, BLACK);
        }
        else
        {
            lcd_DrawLine(x + SIGNAL_LENGTH - 21U, y + 2U,
                         x + SIGNAL_LENGTH - 21U,
                         y + SIGNAL_THICKNESS - 3U, HOUSING_EDGE);
            lcd_Fill(x - SIGNAL_POST_LEN, y + 10U, x, y + 14U, BLACK);
        }
    }
}

static void draw_person(const corner_display_t *corner,
                        uint16_t x,
                        uint16_t y,
                        uint16_t color,
                        bool walking)
{
    uint16_t head_x;
    uint16_t head_y;

    map_local_point(corner->ped_x, corner->ped_y, PED_W, PED_H,
                    corner->rotation, x, y, &head_x, &head_y);
    lcd_DrawCircle((int)head_x, (int)head_y, color, 2, 1);
    draw_local_line(corner->ped_x, corner->ped_y, PED_W, PED_H,
                    corner->rotation, x, y + 3U, x, y + 10U, color);

    if (walking)
    {
        draw_local_line(corner->ped_x, corner->ped_y, PED_W, PED_H,
                        corner->rotation, x, y + 5U, x - 4U, y + 8U, color);
        draw_local_line(corner->ped_x, corner->ped_y, PED_W, PED_H,
                        corner->rotation, x, y + 5U, x + 4U, y + 2U, color);
        draw_local_line(corner->ped_x, corner->ped_y, PED_W, PED_H,
                        corner->rotation, x, y + 10U, x - 4U, y + 15U, color);
        draw_local_line(corner->ped_x, corner->ped_y, PED_W, PED_H,
                        corner->rotation, x, y + 10U, x + 5U, y + 14U, color);
    }
    else
    {
        draw_local_line(corner->ped_x, corner->ped_y, PED_W, PED_H,
                        corner->rotation, x - 4U, y + 5U, x + 4U, y + 5U, color);
        draw_local_line(corner->ped_x, corner->ped_y, PED_W, PED_H,
                        corner->rotation, x, y + 10U, x - 3U, y + 15U, color);
        draw_local_line(corner->ped_x, corner->ped_y, PED_W, PED_H,
                        corner->rotation, x, y + 10U, x + 3U, y + 15U, color);
    }
}

static void draw_pedestrian_housing(const corner_display_t *corner)
{
    const uint16_t x = corner->ped_x;
    const uint16_t y = corner->ped_y;

    /* Draw the post behind the green/walk face. */
    switch (corner->rotation)
    {
        case SIGNAL_ROTATE_180:
            lcd_Fill(x + 9U, y - PED_POST_LEN,
                     x + 13U, y + 1U, BLACK);
            break;

        case SIGNAL_ROTATE_90:
            lcd_Fill(x - PED_POST_LEN, y + 9U,
                     x + 1U, y + 13U, BLACK);
            break;

        case SIGNAL_ROTATE_MINUS_90:
            lcd_Fill(x + PED_H - 1U, y + 9U,
                     x + PED_H - 1U + PED_POST_LEN, y + 13U, BLACK);
            break;

        case SIGNAL_ROTATE_0:
        default:
            lcd_Fill(x + 9U, y + PED_H - 1U,
                     x + 13U, y + PED_H - 1U + PED_POST_LEN, BLACK);
            break;
    }

    fill_local_rect(x, y, PED_W, PED_H, corner->rotation,
                    0U, 0U, PED_W, PED_H, HOUSING_EDGE);
    fill_local_rect(x, y, PED_W, PED_H, corner->rotation,
                    2U, 2U, PED_W - 4U, PED_H - 4U, BLACK);
    draw_local_line(x, y, PED_W, PED_H, corner->rotation,
                    2U, 23U, PED_W - 3U, 23U, HOUSING_EDGE);
}

static void render_digit(const corner_display_t *corner,
                         uint8_t digit,
                         uint16_t local_x,
                         uint16_t local_y)
{
    for (uint8_t row = 0U; row < 5U; row++)
    {
        for (uint8_t column = 0U; column < 3U; column++)
        {
            if ((digit_rows[digit][row] & (1U << (2U - column))) != 0U)
            {
                fill_local_rect(corner->signal_x, corner->signal_y,
                                SIGNAL_THICKNESS, SIGNAL_LENGTH,
                                corner->rotation,
                                local_x + column * 2U,
                                local_y + row * 2U,
                                2U, 2U, COUNTDOWN_COLOR);
            }
        }
    }
}

static void render_countdown(const corner_display_t *corner, uint8_t countdown)
{
    fill_local_rect(corner->signal_x, corner->signal_y,
                    SIGNAL_THICKNESS, SIGNAL_LENGTH, corner->rotation,
                    2U, 2U, 20U, 17U, BLACK);

    if (countdown >= 10U)
    {
        render_digit(corner, (uint8_t)(countdown / 10U), 4U, 5U);
    }
    render_digit(corner, (uint8_t)(countdown % 10U), 12U, 5U);
}

static void render_signal(const corner_display_t *corner,
                          traffic_lamp_t lamp,
                          uint8_t countdown)
{
    const uint16_t x = corner->signal_x;
    const uint16_t y = corner->signal_y;
    uint16_t red = LAMP_OFF_RED;
    uint16_t yellow = LAMP_OFF_YELLOW;
    uint16_t green = LAMP_OFF_GREEN;
    uint16_t red_x;
    uint16_t red_y;
    uint16_t yellow_x;
    uint16_t yellow_y;
    uint16_t green_x;
    uint16_t green_y;

    if (lamp == TRAFFIC_LAMP_RED)
    {
        red = RED;
    }
    else if (lamp == TRAFFIC_LAMP_YELLOW)
    {
        yellow = YELLOW;
    }
    else
    {
        green = GREEN;
    }

    map_local_point(x, y, SIGNAL_THICKNESS, SIGNAL_LENGTH,
                    corner->rotation, 12U, 30U, &red_x, &red_y);
    map_local_point(x, y, SIGNAL_THICKNESS, SIGNAL_LENGTH,
                    corner->rotation, 12U, 48U, &yellow_x, &yellow_y);
    map_local_point(x, y, SIGNAL_THICKNESS, SIGNAL_LENGTH,
                    corner->rotation, 12U, 66U, &green_x, &green_y);

    render_countdown(corner, countdown);
    lcd_DrawCircle((int)red_x, (int)red_y, red, 6, 1);
    lcd_DrawCircle((int)yellow_x, (int)yellow_y, yellow, 6, 1);
    lcd_DrawCircle((int)green_x, (int)green_y, green, 6, 1);
}

static void render_pedestrian(const corner_display_t *corner, bool walk)
{
    const uint16_t stop_color = walk ? LAMP_OFF_RED : RED;
    const uint16_t walk_color = walk ? GREEN : LAMP_OFF_GREEN;

    fill_local_rect(corner->ped_x, corner->ped_y, PED_W, PED_H,
                    corner->rotation, 2U, 2U, PED_W - 4U, 20U, BLACK);
    fill_local_rect(corner->ped_x, corner->ped_y, PED_W, PED_H,
                    corner->rotation, 2U, 24U, PED_W - 4U, 20U, BLACK);
    draw_person(corner, PED_W / 2U, 5U, stop_color, false);
    draw_person(corner, PED_W / 2U, 29U, walk_color, true);
}

void traffic_ui_init(void)
{
    for (uint8_t i = 0U; i < (uint8_t)(sizeof(corners) / sizeof(corners[0])); i++)
    {
        draw_signal_housing(&corners[i]);
        draw_pedestrian_housing(&corners[i]);
    }

}

void traffic_ui_render(traffic_lamp_t ns_lamp,
                       traffic_lamp_t ew_lamp,
                       uint8_t ns_countdown,
                       uint8_t ew_countdown)
{
    const bool ns_walk = (ns_lamp == TRAFFIC_LAMP_RED) &&
                         (ew_lamp == TRAFFIC_LAMP_GREEN);
    const bool ew_walk = (ew_lamp == TRAFFIC_LAMP_RED) &&
                         (ns_lamp == TRAFFIC_LAMP_GREEN);

    for (uint8_t i = 0U; i < (uint8_t)(sizeof(corners) / sizeof(corners[0])); i++)
    {
        if (corners[i].north_south)
        {
            render_signal(&corners[i], ns_lamp, ns_countdown);
            render_pedestrian(&corners[i], ns_walk);
        }
        else
        {
            render_signal(&corners[i], ew_lamp, ew_countdown);
            render_pedestrian(&corners[i], ew_walk);
        }
    }

}
