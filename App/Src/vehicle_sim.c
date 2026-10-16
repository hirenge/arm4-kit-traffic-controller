#include "vehicle_sim.h"

#include <stdbool.h>
#include <stdint.h>

#include "lcd.h"

#define VEHICLE_CAPACITY      10U
#define CAR_LENGTH            16U
#define CAR_WIDTH             10U
#define SAFE_GAP              5U
#define POSITION_SCALE        4U
#define CRUISE_SPEED          12U
#define ACCELERATION          2U
#define DECELERATION          1U
#define SPAWN_MIN_TICKS       8U
#define SPAWN_RANDOM_TICKS    15U

typedef enum
{
    CAR_SOUTH = 0,
    CAR_NORTH,
    CAR_EAST,
    CAR_WEST
} car_direction_t;

typedef struct
{
    bool active;
    car_direction_t direction;
    uint16_t progress;
    uint8_t speed;
    uint16_t color;
} car_t;

typedef struct
{
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
} car_rect_t;

static car_t cars[VEHICLE_CAPACITY];
static uint32_t random_state;
static uint8_t spawn_ticks;

static uint32_t random_next(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static uint16_t direction_stop(car_direction_t direction)
{
    switch (direction)
    {
        case CAR_SOUTH:
            return CROSS_TOP_Y - 2U;
        case CAR_NORTH:
            return (SCR_H - 1U) -
                   (CROSS_BOTTOM_Y + CROSSING_DEPTH + 2U);
        case CAR_EAST:
            return CROSS_LEFT_X - 2U;
        case CAR_WEST:
        default:
            return (SCR_W - 1U) -
                   (CROSS_RIGHT_X + CROSSING_DEPTH + 2U);
    }
}

static uint16_t direction_length(car_direction_t direction)
{
    return ((direction == CAR_SOUTH) || (direction == CAR_NORTH)) ?
           SCR_H : SCR_W;
}

static bool direction_is_ns(car_direction_t direction)
{
    return (direction == CAR_SOUTH) || (direction == CAR_NORTH);
}

static car_rect_t car_rectangle(const car_t *car)
{
    const int16_t front = (int16_t)(car->progress / POSITION_SCALE);
    car_rect_t rectangle;

    switch (car->direction)
    {
        case CAR_SOUTH:
            rectangle.x = 98;
            rectangle.y = front - (int16_t)CAR_LENGTH + 1;
            rectangle.width = CAR_WIDTH;
            rectangle.height = CAR_LENGTH;
            break;

        case CAR_NORTH:
            rectangle.x = 132;
            rectangle.y = (int16_t)(SCR_H - 1U) - front;
            rectangle.width = CAR_WIDTH;
            rectangle.height = CAR_LENGTH;
            break;

        case CAR_EAST:
            rectangle.x = front - (int16_t)CAR_LENGTH + 1;
            rectangle.y = 172;
            rectangle.width = CAR_LENGTH;
            rectangle.height = CAR_WIDTH;
            break;

        case CAR_WEST:
        default:
            rectangle.x = (int16_t)(SCR_W - 1U) - front;
            rectangle.y = 138;
            rectangle.width = CAR_LENGTH;
            rectangle.height = CAR_WIDTH;
            break;
    }

    return rectangle;
}

static void fill_screen_clipped(int16_t x, int16_t y,
                                uint16_t width, uint16_t height,
                                uint16_t color)
{
    const int32_t x2 = (int32_t)x + width;
    const int32_t y2 = (int32_t)y + height;

    if ((width == 0U) || (height == 0U) ||
        (x2 <= 0) || (y2 <= 0) ||
        (x >= (int16_t)SCR_W) || (y >= (int16_t)SCR_H))
    {
        return;
    }

    const uint16_t clipped_x1 = (x > 0) ? (uint16_t)x : 0U;
    const uint16_t clipped_y1 = (y > 0) ? (uint16_t)y : 0U;
    const uint16_t clipped_x2 = (x2 < (int32_t)SCR_W) ?
                                (uint16_t)x2 : SCR_W;
    const uint16_t clipped_y2 = (y2 < (int32_t)SCR_H) ?
                                (uint16_t)y2 : SCR_H;

    lcd_Fill(clipped_x1, clipped_y1, clipped_x2, clipped_y2, color);
}

static void erase_car(const car_t *car)
{
    const car_rect_t rectangle = car_rectangle(car);
    background_restore_rect(rectangle.x - 1, rectangle.y - 1,
                            rectangle.width + 2U, rectangle.height + 2U);
}

static void draw_car(const car_t *car)
{
    const car_rect_t rectangle = car_rectangle(car);

    fill_screen_clipped(rectangle.x, rectangle.y,
                        rectangle.width, rectangle.height, BLACK);
    fill_screen_clipped(rectangle.x + 1, rectangle.y + 1,
                        rectangle.width - 2U, rectangle.height - 2U,
                        car->color);

    if ((car->direction == CAR_SOUTH) ||
        (car->direction == CAR_NORTH))
    {
        const int16_t window_y = rectangle.y +
            ((car->direction == CAR_SOUTH) ? 10 : 3);
        fill_screen_clipped(rectangle.x + 2, window_y,
                            rectangle.width - 4U, 3U, LIGHTBLUE);
    }
    else
    {
        const int16_t window_x = rectangle.x +
            ((car->direction == CAR_EAST) ? 10 : 3);
        fill_screen_clipped(window_x, rectangle.y + 2,
                            3U, rectangle.height - 4U, LIGHTBLUE);
    }
}

static uint16_t conflict_exit(car_direction_t direction)
{
    switch (direction)
    {
        case CAR_SOUTH:
            return CROSS_BOTTOM_Y + CROSSING_DEPTH + CAR_LENGTH - 1U;
        case CAR_NORTH:
            return (SCR_H - 1U - CROSS_TOP_Y) + CAR_LENGTH;
        case CAR_EAST:
            return CROSS_RIGHT_X + CROSSING_DEPTH + CAR_LENGTH - 1U;
        case CAR_WEST:
        default:
            return (SCR_W - 1U - CROSS_LEFT_X) + CAR_LENGTH;
    }
}

static bool car_reserves_intersection(const car_t *car)
{
    const uint16_t stop = direction_stop(car->direction) * POSITION_SCALE;
    const uint16_t exit = conflict_exit(car->direction) * POSITION_SCALE;

    return (car->progress > stop) && (car->progress < exit);
}

static bool axis_has_conflicting_car(bool north_south)
{
    for (uint8_t i = 0U; i < VEHICLE_CAPACITY; i++)
    {
        if (cars[i].active &&
            (direction_is_ns(cars[i].direction) != north_south) &&
            car_reserves_intersection(&cars[i]))
        {
            return true;
        }
    }
    return false;
}

static uint16_t nearest_limit(uint8_t car_index,
                              traffic_lamp_t ns_lamp,
                              traffic_lamp_t ew_lamp)
{
    const car_t *car = &cars[car_index];
    const bool north_south = direction_is_ns(car->direction);
    const traffic_lamp_t lamp = north_south ? ns_lamp : ew_lamp;
    const uint16_t stop = direction_stop(car->direction) * POSITION_SCALE;
    uint16_t limit = UINT16_MAX;

    if ((car->progress <= stop) &&
        ((lamp != TRAFFIC_LAMP_GREEN) ||
         axis_has_conflicting_car(north_south)))
    {
        limit = stop;
    }

    for (uint8_t i = 0U; i < VEHICLE_CAPACITY; i++)
    {
        if ((i != car_index) && cars[i].active &&
            (cars[i].direction == car->direction) &&
            (cars[i].progress > car->progress))
        {
            const uint16_t spacing =
                (CAR_LENGTH + SAFE_GAP) * POSITION_SCALE;
            const uint16_t candidate = (cars[i].progress > spacing) ?
                                       (uint16_t)(cars[i].progress - spacing) :
                                       0U;
            if (candidate < limit)
            {
                limit = candidate;
            }
        }
    }

    return limit;
}

static uint8_t target_speed(const car_t *car, uint16_t limit)
{
    if (limit == UINT16_MAX)
    {
        return CRUISE_SPEED;
    }
    if (limit <= car->progress)
    {
        return 0U;
    }

    const uint16_t distance = limit - car->progress;
    if (distance <= 3U * POSITION_SCALE)
    {
        return 1U;
    }
    if (distance <= 10U * POSITION_SCALE)
    {
        return 2U;
    }
    if (distance <= 24U * POSITION_SCALE)
    {
        return 4U;
    }
    if (distance <= 40U * POSITION_SCALE)
    {
        return 6U;
    }
    if (distance <= 60U * POSITION_SCALE)
    {
        return 8U;
    }
    return CRUISE_SPEED;
}

static void move_car(uint8_t car_index,
                     traffic_lamp_t ns_lamp,
                     traffic_lamp_t ew_lamp)
{
    car_t *car = &cars[car_index];
    const uint16_t limit = nearest_limit(car_index, ns_lamp, ew_lamp);
    const uint8_t desired_speed = target_speed(car, limit);

    if (car->speed < desired_speed)
    {
        const uint8_t accelerated = car->speed + ACCELERATION;
        car->speed = (accelerated < desired_speed) ?
                     accelerated : desired_speed;
    }
    else if (car->speed > desired_speed)
    {
        car->speed = (car->speed > desired_speed + DECELERATION) ?
                     car->speed - DECELERATION : desired_speed;
    }

    uint16_t movement = car->speed;
    if ((limit != UINT16_MAX) && (limit <= car->progress))
    {
        movement = 0U;
        car->speed = 0U;
    }
    else if ((limit != UINT16_MAX) &&
             ((uint32_t)car->progress + movement > limit))
    {
        movement = limit - car->progress;
        car->speed = 0U;
    }
    car->progress += movement;
}

static bool spawn_path_is_clear(car_direction_t direction)
{
    const uint16_t required_space =
        (CAR_LENGTH + SAFE_GAP + 2U) * POSITION_SCALE;

    for (uint8_t i = 0U; i < VEHICLE_CAPACITY; i++)
    {
        if (cars[i].active && (cars[i].direction == direction) &&
            (cars[i].progress < required_space))
        {
            return false;
        }
    }
    return true;
}

static void try_spawn_car(void)
{
    static const uint16_t colors[] = {RED, GREEN, YELLOW};
    const car_direction_t direction =
        (car_direction_t)(random_next() % 4U);

    if (!spawn_path_is_clear(direction))
    {
        return;
    }

    for (uint8_t i = 0U; i < VEHICLE_CAPACITY; i++)
    {
        if (!cars[i].active)
        {
            cars[i].active = true;
            cars[i].direction = direction;
            cars[i].progress = 0U;
            cars[i].speed = 0U;
            cars[i].color = colors[random_next() % 3U];
            return;
        }
    }
}

void vehicle_sim_init(void)
{
    for (uint8_t i = 0U; i < VEHICLE_CAPACITY; i++)
    {
        cars[i].active = false;
    }
    random_state = 0xA341316CU;
    spawn_ticks = 4U;
}

void vehicle_sim_step(traffic_lamp_t ns_lamp, traffic_lamp_t ew_lamp)
{
    for (uint8_t i = 0U; i < VEHICLE_CAPACITY; i++)
    {
        if (cars[i].active)
        {
            erase_car(&cars[i]);
        }
    }

    for (uint8_t i = 0U; i < VEHICLE_CAPACITY; i++)
    {
        if (cars[i].active)
        {
            move_car(i, ns_lamp, ew_lamp);
            const uint16_t exit_progress =
                (direction_length(cars[i].direction) + CAR_LENGTH) *
                POSITION_SCALE;
            if (cars[i].progress > exit_progress)
            {
                cars[i].active = false;
            }
        }
    }

    if (spawn_ticks > 0U)
    {
        spawn_ticks--;
    }
    else
    {
        try_spawn_car();
        spawn_ticks = SPAWN_MIN_TICKS +
                      (uint8_t)(random_next() % SPAWN_RANDOM_TICKS);
    }

    for (uint8_t i = 0U; i < VEHICLE_CAPACITY; i++)
    {
        if (cars[i].active)
        {
            draw_car(&cars[i]);
        }
    }
}
