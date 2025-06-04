#include "bottle_hud.h"
#include "input_handling.h"
#include "quick_bottle.h"

#include "sys_cmpdma.h"
#include "rt64_extended_gbi.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "z64item.h"
#include "z64interface.h"
#include "kaleido_manager.h"
#include "macros.h"
#include "variables.h"
#include "functions.h"
#include "z64player.h"

#define CONFIG_EQUIPSLOTS (bool)recomp_get_config_u32("bottle-equipslots")

#define ICON_IMG_SIZE 32
#define ICON_SIZE 16
#define ICON_DIST 14
#define ICON_POS_X 24
#define ICON_POS_Y 186

#define ICON_SPACING_X 14
#define ICON_SPACING_Y 16

#define BOTTLE_GRID(x, y) { ICON_POS_X + (ICON_SPACING_X * x), ICON_POS_Y + (ICON_SPACING_Y * -y)}

#define UNSELECTED_BOTTLE_FADE 3

#define ICON_DSDX (s32)(1024.0f * (float)(ICON_IMG_SIZE) / (ICON_SIZE))
#define ICON_DTDY (s32)(1024.0f * (float)(ICON_IMG_SIZE) / (ICON_SIZE))

extern u8 gEquippedItemOutlineTex[];
extern u8 gHookshotReticleTex[];

static bool bottle_item_icons_loaded = false;
static u8 bottle_item_textures[NUMBER_BOTTLE_ITEMS][ICON_IMG_SIZE * ICON_IMG_SIZE * 4] __attribute__((aligned(8)));
static u8 bottle_hud_status = 0;
static s16 bottle_hud_alpha = 0b011111111111111;

BottleHudLayoutConfig hud_layouts[] = {
    { // SINGLE
        false,
        {0, 0, -1, 1},
        {
            BOTTLE_GRID(0, 0),
            BOTTLE_GRID(0, 0),
            BOTTLE_GRID(0, 0),
            BOTTLE_GRID(0, 0),
            BOTTLE_GRID(0, 0),
            BOTTLE_GRID(0, 0),
        },
    },{ // HORIZONTAL
        false,
        {0, 0, -1, 1},
        {
            BOTTLE_GRID(0, 0),
            BOTTLE_GRID(1, 0),
            BOTTLE_GRID(2, 0),
            BOTTLE_GRID(3, 0),
            BOTTLE_GRID(4, 0),
            BOTTLE_GRID(5, 0),
        }
    },{ // VERTICAL TOP DOWN
        false,
        {1, -1, 0, 0},
        {
            BOTTLE_GRID(0, 0),
            BOTTLE_GRID(0, 1),
            BOTTLE_GRID(0, 2),
            BOTTLE_GRID(0, 3),
            BOTTLE_GRID(0, 4),
            BOTTLE_GRID(0, 5),
        }
    },{ // VERTICAL BOTTOM UP
        false,
        {-1, 1, 0, 0},
        {
            BOTTLE_GRID(0, 5),
            BOTTLE_GRID(0, 4),
            BOTTLE_GRID(0, 3),
            BOTTLE_GRID(0, 2),
            BOTTLE_GRID(0, 1),
            BOTTLE_GRID(0, 0),
        },
    },{ // TWO ROWS
        false,
        {3, -3, -1, 1},
        {
            BOTTLE_GRID(0, 0),
            BOTTLE_GRID(1, 0),
            BOTTLE_GRID(2, 0),
            BOTTLE_GRID(0, 1),
            BOTTLE_GRID(1, 1),
            BOTTLE_GRID(2, 1),
        }
    },{ // TWO COLUMNS
        false,
        {2, -2, -1, 1},
        {
            BOTTLE_GRID(0, 0),
            BOTTLE_GRID(1, 0),
            BOTTLE_GRID(0, 1),
            BOTTLE_GRID(1, 1),
            BOTTLE_GRID(0, 2),
            BOTTLE_GRID(1, 2),
        }
    },{ // HORIZONTAL - GAPLESS
        true,
        {0, 0, -1, 1},
        {
            BOTTLE_GRID(0, 0),
            BOTTLE_GRID(1, 0),
            BOTTLE_GRID(2, 0),
            BOTTLE_GRID(3, 0),
            BOTTLE_GRID(4, 0),
            BOTTLE_GRID(5, 0),
        }
    },{ // VERTICAL TOP DOWN - GAPLESS
        true,
        {1, -1, 0, 0},
        {
            BOTTLE_GRID(0, 0),
            BOTTLE_GRID(0, 1),
            BOTTLE_GRID(0, 2),
            BOTTLE_GRID(0, 3),
            BOTTLE_GRID(0, 4),
            BOTTLE_GRID(0, 5),
        }
    },{ // VERTICAL BOTTOM UP - GAPLESS
        true,
        {-1, 1, 0, 0},
        {
            BOTTLE_GRID(0, 5),
            BOTTLE_GRID(0, 4),
            BOTTLE_GRID(0, 3),
            BOTTLE_GRID(0, 2),
            BOTTLE_GRID(0, 1),
            BOTTLE_GRID(0, 0),
        },
    }
};


int GetBottleIconIndex(int bottle_index) {
    ItemId bottle = gSaveContext.save.saveInfo.inventory.items[FIRST_BOTTLE_INVENTORY_SLOT + bottle_index];
    return bottle - 0x12;
}

int GetBottleSelectedIconIndex() {
    return GetBottleIconIndex(quickBottle.bottleIndex);
}

RECOMP_HOOK("Interface_DrawCButtonIcons") void DrawBottleIcon(PlayState* play) {
    Player* player = GET_PLAYER(play);
    // If there aren't any bottles, do nothing.
    if (!quickBottle.numberOfBottles || player->currentMask == PLAYER_MASK_GIANT) {
        
        return;
    }

    PauseContext* pauseCtx = &play->pauseCtx;
    if (pauseCtx->state == PAUSE_STATE_OFF) {

        BottleHudLayoutType layout_index = recomp_get_config_u32("bottle-hud-layout");
        BottleHudSelectionType selection_type = recomp_get_config_u32("bottle-selection-style");
        BottleHudRoundRobin rr = recomp_get_config_u32("bottle-round-robin");
        // Load bottle icons once:
        if (!bottle_item_icons_loaded) {
            for (int i = 0; i < NUMBER_BOTTLE_ITEMS; i++) {
                CmpDma_LoadFile(SEGMENT_ROM_START(icon_item_static_yar), bottle_items[i], bottle_item_textures[i], sizeof(bottle_item_textures[i]));
            }
            recomp_printf("Bottle Icons Loaded\n");
            bottle_item_icons_loaded = true;
        }

        OPEN_DISPS(play->state.gfxCtx);

        gEXForceUpscale2D(OVERLAY_DISP++, 1);
        // Set a fullscreen scissor.
        gEXPushScissor(OVERLAY_DISP++);
        gEXSetScissor(OVERLAY_DISP++, G_SC_NON_INTERLACE, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, 0, 0, SCREEN_HEIGHT);
        
        // Special handling for the single display mode.
        if (layout_index == BOTTLE_HUD_SINGLE) {
            int i = quickBottle.bottleIndex;
            gDPLoadTextureBlock(OVERLAY_DISP++, bottle_item_textures[GetBottleIconIndex(i)], G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_NOMIRROR | G_TX_WRAP,
                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, bottle_hud_alpha);
            gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT,
                (hud_layouts[layout_index].screen_positions[i].x - (ICON_SIZE/2)) * 4, (hud_layouts[layout_index].screen_positions[i].y - (ICON_SIZE/2)) * 4,
                (hud_layouts[layout_index].screen_positions[i].x + (ICON_SIZE/2)) * 4, (hud_layouts[layout_index].screen_positions[i].y + (ICON_SIZE/2)) * 4,
                0,
                0, 0,
                ICON_DSDX, ICON_DTDY);
        } else {
            // Draws the bottle icons based on the hud_layout index.
            // i represents the index of the current grid position.
            // draw_bottle is the bottle index to draw.
            // max_bottle_draws is used to stop early in gapless mode.
            int draw_bottle = (rr ? quickBottle.bottleIndex : 0) - 1;
            int max_bottle_draws = hud_layouts[layout_index].gapless ? quickBottle.numberOfBottles : 6;
            for (int i = 0; i < max_bottle_draws; i++) {
                do {
                    draw_bottle++;
                    if (rr) { // Round Robin wrap-around.
                        if (draw_bottle > 5) {
                            draw_bottle = 0;
                        }
                    }
                // If we're in gapless mode, skip empty slots until we find another bottle to draw.
                // Calculating max_bottle_draws ahead of time ensures we never draw duplicates.
                } while ( (hud_layouts[layout_index].gapless && !QuickBottle_IsValidBottleItem(QuickBottle_GetBottleId(draw_bottle))));

                if (recomp_get_config_u32("bottle-autohide") != 0 && quickBottle.quick_press_timer < BOTTLE_QUICK_PRESS_TIME) {
                    max_bottle_draws = 1;
                    i = 0;
                    draw_bottle = quickBottle.bottleIndex;
                }

                if (selection_type == BOTTLE_SELECTION_BORDER && ((i == quickBottle.bottleIndex && !rr) || (i == 0 && rr))){
                    // Drawing the item outline for border selection mode:
                    gDPLoadTextureBlock(OVERLAY_DISP++, gEquippedItemOutlineTex, G_IM_FMT_I, G_IM_SIZ_8b, 32, 32, 0, G_TX_NOMIRROR | G_TX_WRAP,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, bottle_hud_alpha);
                    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT,
                        (hud_layouts[layout_index].screen_positions[i].x - (ICON_SIZE/2)) * 4, (hud_layouts[layout_index].screen_positions[i].y - (ICON_SIZE/2)) * 4,
                        (hud_layouts[layout_index].screen_positions[i].x + (ICON_SIZE/2)) * 4, (hud_layouts[layout_index].screen_positions[i].y + (ICON_SIZE/2)) * 4,
                        0,
                        0, 0,
                        ICON_DSDX, ICON_DTDY);
                }

                if (QuickBottle_IsValidBottleItem(QuickBottle_GetBottleId(draw_bottle))) {
                    gDPLoadTextureBlock(OVERLAY_DISP++, bottle_item_textures[GetBottleIconIndex(draw_bottle)], G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_NOMIRROR | G_TX_WRAP,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                    // Fadeout for fade selection mode:
                    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, bottle_hud_alpha / ((selection_type != BOTTLE_SELECTION_FADE || (draw_bottle == quickBottle.bottleIndex && !CONFIG_EQUIPSLOTS) || (CONFIG_EQUIPSLOTS && BtnStateL.cur && draw_bottle == quickBottle.bottleIndex)) ? 1 : UNSELECTED_BOTTLE_FADE));
                    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT,
                        (hud_layouts[layout_index].screen_positions[i].x - (ICON_SIZE/2)) * 4, (hud_layouts[layout_index].screen_positions[i].y - (ICON_SIZE/2)) * 4,
                        (hud_layouts[layout_index].screen_positions[i].x + (ICON_SIZE/2)) * 4, (hud_layouts[layout_index].screen_positions[i].y + (ICON_SIZE/2)) * 4,
                        0,
                        0, 0,
                        ICON_DSDX, ICON_DTDY);
                }
            }
        }
        
        gEXForceUpscale2D(OVERLAY_DISP++, 0);
        // Restore the previous scissor.
        gEXPopScissor(OVERLAY_DISP++);

        CLOSE_DISPS(play->state.gfxCtx);
    }
}

/*
static InterfaceContext* captured_interfaceCtx;
RECOMP_HOOK("Interface_UpdateHudAlphas") void pre_Interface_UpdateHudAlphas(PlayState* play, s16 dimmingAlpha) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
}

RECOMP_HOOK_RETURN("Interface_UpdateHudAlphas") void post_Interface_UpdateHudAlphas() {
    if (captured_interfaceCtx != NULL) {
        bottle_hud_alpha = captured_interfaceCtx->healthAlpha;
    }
}
*/


RECOMP_HOOK ("Interface_UpdateButtonAlphasByStatus") void Interface_UpdateBottleAlphaByStatus(PlayState* play, s16 risingAlpha) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    

    if (bottle_hud_status == BTN_DISABLED) {
        if (bottle_hud_alpha != 70) {
            bottle_hud_alpha = 70;
        }
    }
    else {
        if (bottle_hud_status != 255) {
            bottle_hud_alpha = risingAlpha;
        }
    }
    
}


RECOMP_HOOK("Interface_UpdateButtonAlphas") void Interface_UpdateBottleAlpha(PlayState* play, s16 dimmingAlpha, s16 risingAlpha) {
    if (gSaveContext.hudVisibilityForceButtonAlphasByStatus) {
        Interface_UpdateBottleAlphaByStatus(play, risingAlpha);
        return;
    }


    if ((bottle_hud_alpha != 0) && (bottle_hud_alpha > dimmingAlpha)) {
        bottle_hud_alpha = dimmingAlpha;
    }
}

RECOMP_HOOK("Interface_UpdateHudAlphas") void pre_Interface_UpdateHudAlphas(PlayState* play, s16 dimmingAlpha) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    s16 risingAlpha = 255 - dimmingAlpha;
    
    switch (gSaveContext.nextHudVisibility) {
        case HUD_VISIBILITY_NONE:
        case HUD_VISIBILITY_NONE_ALT:
        case HUD_VISIBILITY_B:
            // recomp_printf("HUD_VISIBILITY_B");
            // @mod
            
                if ((bottle_hud_alpha != 0) && (bottle_hud_alpha > dimmingAlpha)) {
                    bottle_hud_alpha = dimmingAlpha;
                }
            

            break;

        case HUD_VISIBILITY_HEARTS_WITH_OVERWRITE:
            // recomp_printf("HUD_VISIBILITY_HEARTS_WITH_OVERWRITE");
            Interface_UpdateBottleAlpha(play, dimmingAlpha, risingAlpha + 0);


            break;

        case HUD_VISIBILITY_A:
            // recomp_printf("HUD_VISIBILITY_A");
            // @mod
            
            if ((bottle_hud_alpha != 0) && (bottle_hud_alpha > dimmingAlpha)) {
                bottle_hud_alpha = dimmingAlpha;
            }
            

            break;

        case HUD_VISIBILITY_A_HEARTS_MAGIC_WITH_OVERWRITE:
            // recomp_printf("HUD_VISIBILITY_A_HEARTS_MAGIC_WITH_OVERWRITE");
            Interface_UpdateBottleAlpha(play, dimmingAlpha, risingAlpha);

            break;

        case HUD_VISIBILITY_A_HEARTS_MAGIC_MINIMAP_WITH_OVERWRITE:
            // recomp_printf("HUD_VISIBILITY_A_HEARTS_MAGIC_MINIMAP_WITH_OVERWRITE");
            Interface_UpdateBottleAlpha(play, dimmingAlpha, risingAlpha);

            break;

        case HUD_VISIBILITY_ALL_NO_MINIMAP_W_DISABLED:
            // recomp_printf("HUD_VISIBILITY_ALL_NO_MINIMAP_W_DISABLED");
            Interface_UpdateBottleAlphaByStatus(play, risingAlpha);

            break;

        case HUD_VISIBILITY_HEARTS_MAGIC:
            // recomp_printf("HUD_VISIBILITY_HEARTS_MAGIC");
            // @mod
            
            if ((bottle_hud_alpha != 0) && (bottle_hud_alpha > dimmingAlpha)) {
                bottle_hud_alpha = dimmingAlpha;
            }
            

            break;

        case HUD_VISIBILITY_B_ALT:
            // recomp_printf("HUD_VISIBILITY_B_ALT");
            // @mod
            
            if ((bottle_hud_alpha != 0) && (bottle_hud_alpha > dimmingAlpha)) {
                bottle_hud_alpha = dimmingAlpha;
            }
            

            break;

        case HUD_VISIBILITY_HEARTS:
            // recomp_printf("HUD_VISIBILITY_HEARTS");
            // @mod
            
            if ((bottle_hud_alpha != 0) && (bottle_hud_alpha > dimmingAlpha)) {
                bottle_hud_alpha = dimmingAlpha;
            }
            

            break;

        case HUD_VISIBILITY_A_B_MINIMAP:
            // recomp_printf("HUD_VISIBILITY_A_B_MINIMAP");
            // @mod
            
            if ((bottle_hud_alpha != 0) && (bottle_hud_alpha > dimmingAlpha)) {
                bottle_hud_alpha = dimmingAlpha;
            }
            

            break;

        case HUD_VISIBILITY_HEARTS_MAGIC_WITH_OVERWRITE:
            // recomp_printf("HUD_VISIBILITY_HEARTS_MAGIC_WITH_OVERWRITE");
            Interface_UpdateBottleAlpha(play, dimmingAlpha, risingAlpha);

            break;

        case HUD_VISIBILITY_HEARTS_MAGIC_C:
            // recomp_printf("HUD_VISIBILITY_HEARTS_MAGIC_C");
            // @mod
            
            if (bottle_hud_alpha != 255) {
                bottle_hud_alpha = risingAlpha;
            }
            

            break;

        case HUD_VISIBILITY_ALL_NO_MINIMAP:
            // recomp_printf("HUD_VISIBILITY_ALL_NO_MINIMAP");
            // @mod
            
            if (bottle_hud_alpha != 255) {
                bottle_hud_alpha = risingAlpha;
            }
            


            break;

        case HUD_VISIBILITY_A_B_C:
            // recomp_printf("HUD_VISIBILITY_A_B_C");
            // @mod
            
            if (bottle_hud_alpha != 255) {
                bottle_hud_alpha = risingAlpha;
            }
            

            break;

        case HUD_VISIBILITY_B_MINIMAP:
            // recomp_printf("HUD_VISIBILITY_B_MINIMAP");

            // @mod
            
            if ((bottle_hud_alpha != 0) && (bottle_hud_alpha > dimmingAlpha)) {
                bottle_hud_alpha = dimmingAlpha;
            }
            


            break;

        case HUD_VISIBILITY_HEARTS_MAGIC_MINIMAP:
            // recomp_printf("HUD_VISIBILITY_HEARTS_MAGIC_MINIMAP");

            // @mod
            
            if ((bottle_hud_alpha != 0) && (bottle_hud_alpha > dimmingAlpha)) {
                bottle_hud_alpha = dimmingAlpha;
            }
            


            break;

        case HUD_VISIBILITY_A_HEARTS_MAGIC_MINIMAP:
            // recomp_printf("HUD_VISIBILITY_A_HEARTS_MAGIC_MINIMAP");

            // @mod
            
            if ((bottle_hud_alpha != 0) && (bottle_hud_alpha > dimmingAlpha)) {
                bottle_hud_alpha = dimmingAlpha;
            }
        

            break;

        case HUD_VISIBILITY_B_MAGIC:
            // recomp_printf("HUD_VISIBILITY_B_MAGIC");
            // @mod
            
            if ((bottle_hud_alpha != 0) && (bottle_hud_alpha > dimmingAlpha)) {
                bottle_hud_alpha = dimmingAlpha;
            }
            

            break;

        case HUD_VISIBILITY_A_B:
            // recomp_printf("HUD_VISIBILITY_A_B");
            // @mod
            if ((bottle_hud_alpha != 0) && (bottle_hud_alpha > dimmingAlpha)) {
                bottle_hud_alpha = dimmingAlpha;
            }
            
            break;

        case HUD_VISIBILITY_A_B_HEARTS_MAGIC_MINIMAP:
            // recomp_printf("HUD_VISIBILITY_A_B_HEARTS_MAGIC_MINIMAP");
            // @mod
            if ((bottle_hud_alpha != 0) && (bottle_hud_alpha > dimmingAlpha)) {
                bottle_hud_alpha = dimmingAlpha;
            }
            
            break;
    }
    // recomp_printf(" dimmingAlpha = %i,", dimmingAlpha);
    // recomp_printf(" bottle_hud_alpha = %i\n", bottle_hud_alpha);
}
