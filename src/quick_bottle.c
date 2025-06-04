#include "quick_bottle.h"
#include "bottle_hud.h"
#include "input_handling.h"
#include "proxymm_kv.h"

#include "recomputils.h"
#include "recompconfig.h"

#include "macros.h"
#include "z64interface.h"
#include "z64inventory.h"
#include "objects/gameplay_keep/gameplay_keep.h"

#define LOAD_SAVE_SELECTION_KEY "LT_Schmiddy.QuickBottles.bottleIndex"

// EXTERNALS:
#define BOTTLE_CATCH_PARAMS_ANY -1
typedef struct struct_8085D798 {
    /* 0x0 */ s16 actorId;
    /* 0x2 */ s8 actorParams;
    /* 0x3 */ u8 itemId;
    /* 0x4 */ u8 itemAction;
    /* 0x5 */ u8 textId;
} struct_8085D798; // size = 0x6

typedef struct struct_8085D200 {
    /* 0x0 */ PlayerAnimationHeader* unk_0;
    /* 0x4 */ PlayerAnimationHeader* unk_4;
    /* 0x8 */ u8 unk_8;
    /* 0x9 */ u8 unk_9;
} struct_8085D200; // size = 0xC

extern LinkAnimationHeader gPlayerAnim_link_bottle_bug_in;
extern LinkAnimationHeader gPlayerAnim_link_bottle_bug_miss;
extern LinkAnimationHeader gPlayerAnim_link_bottle_fish_in;
extern LinkAnimationHeader gPlayerAnim_link_bottle_fish_miss;
extern struct_8085D200 D_8085D200[2];
extern struct_8085D798 D_8085D798[14];
void Player_UseItem(PlayState* play, Player* this, ItemId item);
void Interface_StartBottleTimer(s16 seconds, s16 timerId);
s32 Player_DecelerateToZero(Player* this);
bool func_808323C0(Player* this, s16 csId);
void Player_StopCutscene(Player* this);
void Player_StartTalking(PlayState* play, Actor* actor);
void func_80839E74(Player* this, PlayState* play);
void Player_Anim_PlayOnceAdjusted(PlayState* play, Player* this, PlayerAnimationHeader* anim);

// Quick Bottle Functions and Data:
ItemId bottle_items[NUMBER_BOTTLE_ITEMS] = {
    /* 0x12 */ ITEM_BOTTLE,
    /* 0x13 */ ITEM_POTION_RED,
    /* 0x14 */ ITEM_POTION_GREEN,
    /* 0x15 */ ITEM_POTION_BLUE,
    /* 0x16 */ ITEM_FAIRY,
    /* 0x17 */ ITEM_DEKU_PRINCESS,
    /* 0x18 */ ITEM_MILK_BOTTLE,
    /* 0x19 */ ITEM_MILK_HALF,
    /* 0x1A */ ITEM_FISH,
    /* 0x1B */ ITEM_BUG,
    /* 0x1C */ ITEM_BLUE_FIRE,
    /* 0x1D */ ITEM_POE,
    /* 0x1E */ ITEM_BIG_POE,
    /* 0x1F */ ITEM_SPRING_WATER,
    /* 0x20 */ ITEM_HOT_SPRING_WATER,
    /* 0x21 */ ITEM_ZORA_EGG,
    /* 0x22 */ ITEM_GOLD_DUST,
    /* 0x23 */ ITEM_MUSHROOM,
    /* 0x24 */ ITEM_SEAHORSE,
    /* 0x25 */ ITEM_CHATEAU,
    /* 0x26 */ ITEM_HYLIAN_LOACH,
    /* 0x27 */ ITEM_OBABA_DRINK,
};

QuickBottleController quickBottle;

int QuickBottle_GetSelectedInventorySlot() {
    return FIRST_BOTTLE_INVENTORY_SLOT + quickBottle.bottleIndex;
}

ItemId QuickBottle_GetBottleId(int index) {
    return gSaveContext.save.saveInfo.inventory.items[FIRST_BOTTLE_INVENTORY_SLOT + index];
}

ItemId QuickBottle_GetSelectedBottleId() {
    return gSaveContext.save.saveInfo.inventory.items[FIRST_BOTTLE_INVENTORY_SLOT + quickBottle.bottleIndex];
}


bool QuickBottle_IsValidBottleItem(ItemId item)  {
    return item >= ITEM_BOTTLE && item <= ITEM_OBABA_DRINK;
}

int QuickBottle_GetNumberOfBottles() {
    int retVal = 0;

    for (int i = 0; i < 6; i++) {
        ItemId item = QuickBottle_GetBottleId(i);

        if (QuickBottle_IsValidBottleItem(item)) {
            retVal++;
        }
    }
    quickBottle.numberOfBottles = retVal;
    return retVal;
}

void QuickBottle_UpdateSelectedBottleIfInvalid(int direction) {
    if (!quickBottle.numberOfBottles || direction == 0) {
        return;
    }

    BottleHudRoundRobin rr = recomp_get_config_u32("bottle-round-robin");
    if (rr == BOTTLE_ROUND_ROBIN_REVERSE) {
        direction = - direction;
    }

    int normal_direction = direction / ABS(direction);
    while (!QuickBottle_IsValidBottleItem(QuickBottle_GetSelectedBottleId())) {
        quickBottle.bottleIndex += direction;
        // Wraping:
        if (quickBottle.bottleIndex > MAX_BOTTLE_INDEX) {
            quickBottle.bottleIndex = 0;
        } else if (quickBottle.bottleIndex < 0) {
            quickBottle.bottleIndex = MAX_BOTTLE_INDEX;
        }
    }
}

void QuickBottle_Cycle(s8 offset) {
    // Preventing WHILE from going into an infinite loop.
    if (!quickBottle.numberOfBottles || offset == 0 ) {
        return;
    }

    BottleHudRoundRobin rr = recomp_get_config_u32("bottle-round-robin");
    if (rr == BOTTLE_ROUND_ROBIN_REVERSE) {
        offset = - offset;
    }

    int normal_direction = offset / ABS(offset);

    for (int i = 0; i < ABS(offset); i++) {
        quickBottle.bottleIndex += normal_direction;
        // Wraping:
        if (quickBottle.bottleIndex > MAX_BOTTLE_INDEX) {
            quickBottle.bottleIndex = 0;
        } else if (quickBottle.bottleIndex < 0) {
            quickBottle.bottleIndex = MAX_BOTTLE_INDEX;
        }
    }
    
    // ItemId bottle = QuickBottle_GetSelectedBottleId();
    // recomp_printf("quickBottle.bottleIndex = %i\n", quickBottle.bottleIndex);
    // recomp_printf("bottle = 0x%00X\n", bottle);
    Audio_PlaySfx(NA_SE_SY_CURSOR);
}

// Patches and Callbacks:

int kl = 0b01011000;
RECOMP_CALLBACK("*", recomp_after_play_init) void reset_quickBottle() {
    quickBottle.triggered = false;
    quickBottle.quick_press_timer = 0;
    quickBottle.post_release_timer = BOTTLE_POST_RELEASE_TIME;
    quickBottle.auto_put_away_timer = BOTTLE_AUTO_PUT_AWAY_TIME;
}

RECOMP_CALLBACK("*", recomp_on_init) void setup_quickBottle() {
    quickBottle.bottleIndex = 0;
    reset_quickBottle();
}

RECOMP_CALLBACK("*", recomp_on_load_save) void load_quickBottle() {
    quickBottle.bottleIndex = KV_Slot_Get_S32(LOAD_SAVE_SELECTION_KEY, 0 );
}

RECOMP_CALLBACK("*", recomp_on_owl_save) void save_quickBottle_Owl() {
    KV_Slot_Set_S32(LOAD_SAVE_SELECTION_KEY, quickBottle.bottleIndex);
}

RECOMP_HOOK_RETURN("Sram_SaveEndOfCycle") void save_quickBottle_SOT() {
    KV_Slot_Set_S32(LOAD_SAVE_SELECTION_KEY, quickBottle.bottleIndex);
}

RECOMP_HOOK_RETURN("Sram_InitNewSave") void init_new_quickBottle() {
    quickBottle.bottleIndex = 0;
}

static u8 L_Timer = 0;
static bool skip_regular_processing = false;
static Player* captured_player = NULL;
RECOMP_HOOK("Player_ProcessItemButtons") void pre_Player_ProcessItemButtons(Player* this, PlayState* play) {
    QuickBottle_GetNumberOfBottles();

    captured_player = this;
    if (!quickBottle.numberOfBottles || this->currentMask == PLAYER_MASK_GIANT) {
        quickBottle.triggered = false;
        return;
    }

    // Used to ensure that empty bottles catch stuff properly:
    if (quickBottle.post_release_timer < BOTTLE_POST_RELEASE_TIME) {
        quickBottle.post_release_timer++;
    }

    if (BtnStateL.press) {
        if (CHECK_BTN_ALL(play->state.input->press.button, BTN_CDOWN)) {
            quickBottle.bottleIndex = GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_C_DOWN) - FIRST_BOTTLE_INVENTORY_SLOT;
        }
        if (CHECK_BTN_ALL(play->state.input->press.button, BTN_CLEFT)) {
            quickBottle.bottleIndex = GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_C_LEFT) - FIRST_BOTTLE_INVENTORY_SLOT;
        }
        if (CHECK_BTN_ALL(play->state.input->press.button, BTN_CRIGHT)) {
            quickBottle.bottleIndex = GET_CUR_FORM_BTN_SLOT(EQUIP_SLOT_C_RIGHT) - FIRST_BOTTLE_INVENTORY_SLOT;
        }
    }

    // Are we using a bottle? Or just done switching:
    if (BtnStateL.rel) {
        if (quickBottle.quick_press_timer < BOTTLE_QUICK_PRESS_TIME) {          
            if (QuickBottle_IsValidBottleItem(QuickBottle_GetSelectedBottleId())){
                // recomp_printf("bottle is valid\n");
                Player_UseItem(play, this, QuickBottle_GetSelectedBottleId());
                quickBottle.triggered = true;
                quickBottle.post_release_timer = 0;
                // Automatically put away bottles.
                quickBottle.auto_put_away_timer = 0;
            }
        }
        else {
            Audio_PlaySfx(NA_SE_SY_DECIDE);
        }
    }
    int validate_direction = 1;
    if (BtnStateL.cur) {
        if (quickBottle.quick_press_timer < BOTTLE_QUICK_PRESS_TIME) {
            quickBottle.quick_press_timer++;
        } else {
            // Audio_PlaySfx(NA_SE_SY_WIN_OPEN);
        }
        BottleHudLayoutType layout_index = recomp_get_config_u32("bottle-hud-layout");
        
        if (BtnStateDUp.press) {
            QuickBottle_Cycle(hud_layouts[layout_index].cycle_directions.up);
            quickBottle.quick_press_timer = BOTTLE_QUICK_PRESS_TIME; // Guess we're a using the bottle, then.
            validate_direction = hud_layouts[layout_index].cycle_directions.up;
        }

        if (BtnStateDLeft.press) {
            QuickBottle_Cycle(hud_layouts[layout_index].cycle_directions.left);
            quickBottle.quick_press_timer = BOTTLE_QUICK_PRESS_TIME; // Guess we're a using the bottle, then.
            validate_direction = hud_layouts[layout_index].cycle_directions.left;
        }

        if (BtnStateDRight.press) {
            QuickBottle_Cycle(hud_layouts[layout_index].cycle_directions.right);
            quickBottle.quick_press_timer = BOTTLE_QUICK_PRESS_TIME; // Guess we're a using the bottle, then.
            validate_direction = hud_layouts[layout_index].cycle_directions.right;
        }

        if (BtnStateDDown.press) {
            QuickBottle_Cycle(hud_layouts[layout_index].cycle_directions.down);
            quickBottle.quick_press_timer = BOTTLE_QUICK_PRESS_TIME; // Guess we're a using the bottle, then.
            validate_direction = hud_layouts[layout_index].cycle_directions.down;
        }

        if (CHECK_BTN_ALL(play->state.input->cur.button, BTN_CDOWN) && C_SLOT_EQUIP(0, EQUIP_SLOT_C_DOWN) != QuickBottle_GetSelectedInventorySlot()) {
            BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_DOWN) = QuickBottle_GetSelectedBottleId();
            C_SLOT_EQUIP(0, EQUIP_SLOT_C_DOWN) = QuickBottle_GetSelectedInventorySlot();
            Interface_LoadItemIcon(play, EQUIP_SLOT_C_DOWN);
        }

        if (CHECK_BTN_ALL(play->state.input->cur.button, BTN_CLEFT) && C_SLOT_EQUIP(0, EQUIP_SLOT_C_LEFT) != QuickBottle_GetSelectedInventorySlot()) {
            BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT) = QuickBottle_GetSelectedBottleId();
            C_SLOT_EQUIP(0, EQUIP_SLOT_C_LEFT) = QuickBottle_GetSelectedInventorySlot();
            Interface_LoadItemIcon(play, EQUIP_SLOT_C_LEFT);
        }

        if (CHECK_BTN_ALL(play->state.input->cur.button, BTN_CRIGHT) && C_SLOT_EQUIP(0, EQUIP_SLOT_C_RIGHT) != QuickBottle_GetSelectedInventorySlot()) {
            BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_RIGHT) = QuickBottle_GetSelectedBottleId();
            C_SLOT_EQUIP(0, EQUIP_SLOT_C_RIGHT) = QuickBottle_GetSelectedInventorySlot();
            Interface_LoadItemIcon(play, EQUIP_SLOT_C_RIGHT);
        }

    } else{
        quickBottle.quick_press_timer = 0;
    }

    QuickBottle_UpdateSelectedBottleIfInvalid(validate_direction);


    if (quickBottle.auto_put_away_timer <= BOTTLE_AUTO_PUT_AWAY_TIME) {
        quickBottle.auto_put_away_timer++;
    } 
    
    if (quickBottle.auto_put_away_timer == BOTTLE_AUTO_PUT_AWAY_TIME && QuickBottle_IsValidBottleItem(this->heldItemId)) {
        quickBottle.triggered = false;
        Player_UseItem(play, this, ITEM_NONE);
    }

    if (quickBottle.triggered) {
        skip_regular_processing = true;
        this->stateFlags1 |= PLAYER_STATE1_20000000;
    }
}

// We can aid compatability with DPad mods by preventing the Player_ProcessItemButtons from running, 
// instead of needing to patch it ourselves
RECOMP_HOOK_RETURN("Player_ProcessItemButtons") void post_Player_ProcessItemButtons() {
    if (skip_regular_processing) {
        captured_player->stateFlags1 &= ~PLAYER_STATE1_20000000;
        skip_regular_processing = false;
    }
}

// Handles changing bottle in inventory
RECOMP_PATCH void Inventory_UpdateBottleItem(PlayState* play, u8 item, u8 btn) {
    Player* this = GET_PLAYER(play);

    // If this is true, use quickbottles handling instead.
    if (quickBottle.triggered) {
        quickBottle.triggered = false;
        gSaveContext.save.saveInfo.inventory.items[QuickBottle_GetSelectedInventorySlot()] = item;

        // Updating C Buttons if a bottle is equipped there:
        for (EquipSlot i = EQUIP_SLOT_B; i < EQUIP_SLOT_A; i++) {
            // recomp_printf("Button %i = 0x%02X, (%i)\n", i, GET_CUR_FORM_BTN_SLOT(i), GET_CUR_FORM_BTN_SLOT(i));
            if (GET_CUR_FORM_BTN_SLOT(i) == QuickBottle_GetSelectedInventorySlot()) {
                SET_CUR_FORM_BTN_ITEM(i, item);
                Interface_LoadItemIconImpl(play, i);
                gSaveContext.buttonStatus[i] = BTN_ENABLED;
            }
        }

        if (item == ITEM_HOT_SPRING_WATER) {
            Interface_StartBottleTimer(60, QuickBottle_GetSelectedInventorySlot());
        }
        return;
    }

    // recomp_printf("Button %i = 0x%02X, (%i)\n", btn, GET_CUR_FORM_BTN_SLOT(btn), GET_CUR_FORM_BTN_SLOT(btn));
    // Otherwise, use vanilla behavior:
    gSaveContext.save.saveInfo.inventory.items[GET_CUR_FORM_BTN_SLOT(btn)] = item;
    SET_CUR_FORM_BTN_ITEM(btn, item);

    Interface_LoadItemIconImpl(play, btn);

    play->pauseCtx.cursorItem[PAUSE_ITEM] = item;
    gSaveContext.buttonStatus[btn] = BTN_ENABLED;

}

RECOMP_PATCH void Player_Action_68(Player* this, PlayState* play) {
    struct_8085D200* sp24 = &D_8085D200[this->av2.actionVar2];

    Player_DecelerateToZero(this);

    if (PlayerAnimation_Update(play, &this->skelAnime)) {
        if (this->av1.actionVar1 != 0) {
            func_808323C0(this, play->playerCsIds[PLAYER_CS_ID_ITEM_SHOW]);

            if (this->av2.actionVar2 == 0) {
                Message_StartTextbox(play, D_8085D798[this->av1.actionVar1 - 1].textId, &this->actor);

                Audio_PlayFanfare(NA_BGM_GET_ITEM | 0x900);
                this->av2.actionVar2 = 1;
            } else if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
                Actor* talkActor;

                this->av1.actionVar1 = 0;
                Player_StopCutscene(this);
                Camera_SetFinishedFlag(Play_GetCamera(play, CAM_ID_MAIN));

                talkActor = this->talkActor;
                if ((talkActor != NULL) && (this->exchangeItemAction <= PLAYER_IA_MINUS1)) {
                    Player_StartTalking(play, talkActor);
                }
            }
        } else {
            func_80839E74(this, play);
        }
    } else {
        if (this->av1.actionVar1 == 0) {
            s32 temp_ft5 = this->skelAnime.curFrame - sp24->unk_8;

            if ((temp_ft5 >= 0) && (sp24->unk_9 >= temp_ft5)) {
                if ((this->av2.actionVar2 != 0) && (temp_ft5 == 0)) {
                    Player_PlaySfx(this, NA_SE_IT_SCOOP_UP_WATER);
                }
                // Literally the only change. Helps mate sure empty bottles can catch items.
                if (Player_GetItemOnButton(play, this, this->heldItemButton) == ITEM_BOTTLE || quickBottle.post_release_timer < BOTTLE_POST_RELEASE_TIME) {
                    Actor* interactRangeActor = this->interactRangeActor;

                    if (interactRangeActor != NULL) {
                        struct_8085D798* entry = D_8085D798;
                        s32 i;

                        for (i = 0; i < ARRAY_COUNT(D_8085D798); i++) {
                            if (((interactRangeActor->id == entry->actorId) &&
                                 ((entry->actorParams <= BOTTLE_CATCH_PARAMS_ANY) ||
                                  (interactRangeActor->params == entry->actorParams)))) {
                                break;
                            }
                            entry++;
                        }

                        if (i < ARRAY_COUNT(D_8085D798)) {
                            this->av1.actionVar1 = i + 1;
                            this->av2.actionVar2 = 0;
                            this->stateFlags1 |= PLAYER_STATE1_10000000 | PLAYER_STATE1_20000000;
                            interactRangeActor->parent = &this->actor;
                            Player_UpdateBottleHeld(play, this, entry->itemId, entry->itemAction);
                            Player_Anim_PlayOnceAdjusted(play, this, sp24->unk_4);
                        }
                    }
                }
            }
        }

        if (this->skelAnime.curFrame <= 7.0f) {
            this->stateFlags3 |= PLAYER_STATE3_800;
        }
    }
}