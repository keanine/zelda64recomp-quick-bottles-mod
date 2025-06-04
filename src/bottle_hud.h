#ifndef __BOTTLE_HUD__
#define __BOTTLE_HUD__

#include "modding.h"
#include "global.h"

typedef enum {
    BOTTLE_HUD_SINGLE,
    BOTTLE_HUD_HORIZONTAL,
    BOTTLE_HUD_VERTICAL_TOP_DOWN,
    BOTTLE_HUD_VERTICAL_BOTTOM_UP,
    BOTTLE_HUD_TWO_ROWS,
    BOTTLE_HUD_MAX
} BottleHudLayoutType;

typedef enum {
    BOTTLE_SELECTION_FADE,
    BOTTLE_SELECTION_BORDER,
    BOTTLE_SELECTION_MAX
} BottleHudSelectionType;

typedef enum {
    BOTTLE_ROUND_ROBIN_OFF,
    BOTTLE_ROUND_ROBIN_ON,
    BOTTLE_ROUND_ROBIN_REVERSE
} BottleHudRoundRobin;

typedef enum {
    BOTTLE_AUTOHIDE_OFF,
    BOTTLE_AUTOHIDE_ON,
    BOTTLE_AUTOHIDE_INSTANT
} BottleHudAutoHide;

typedef struct {
    int up;
    int down;
    int left;
    int right;
} BottleCycleDirections;

typedef struct {
    int x;
    int y;
} BottleScreenPosition;

typedef struct {
    bool gapless;
    BottleCycleDirections cycle_directions;
    BottleScreenPosition screen_positions[6];
} BottleHudLayoutConfig;

extern BottleHudLayoutConfig hud_layouts[9];

#endif