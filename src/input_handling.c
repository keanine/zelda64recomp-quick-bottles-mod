#include "input_handling.h"
#include "recomputils.h"
#include "recompconfig.h"

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

BtnState BtnStateL;
BtnState BtnStateDUp;
BtnState BtnStateDRight;
BtnState BtnStateDLeft;
BtnState BtnStateDDown;
BtnState BtnStateEquips;

Player* mThis;
PlayState* mPlay;

#define CONFIG_EQUIPSLOTS (bool)recomp_get_config_u32("bottle-equipslots")
extern void Interface_LoadItemIcon(PlayState* play, u8 btn);

void BtnState_Record( Input* input, BtnState* state, u16 btn, bool should_mask) {
    state->cur = input->cur.button & btn;
    state->prev = input->prev.button & btn;
    state->press = input->press.button & btn;
    state->rel = input->rel.button & btn;

    if (should_mask) {
        input->cur.button &= ~btn;
        input->prev.button &= ~btn;
        input->press.button &= ~btn;
        input->rel.button &= ~btn;
    }
}

void EquipSlotIsQuickBottle(u8 btn, u8 equipSlot, u8* activationButton) {
    ItemId item = Player_GetItemOnButton(mPlay, mThis, equipSlot);
        if ((item >= ITEM_BOTTLE) && (item <= ITEM_OBABA_DRINK)) {
        *activationButton |= btn;
    }
}


RECOMP_HOOK("Player_UpdateCommon") void pre_Player_UpdateCommon(Player* this, PlayState* play, Input* input) {
    // Kafei Prevention:
    if (this->actor.id != ACTOR_PLAYER) {
        return;
    }
    
    // Capture input:
    //BtnState_Record(input, &BtnStateL, BTN_L, false);
    u8 activationButton = BTN_L;

    if (CONFIG_EQUIPSLOTS) {
        EquipSlotIsQuickBottle(BTN_CDOWN, EQUIP_SLOT_C_DOWN, &activationButton);
        EquipSlotIsQuickBottle(BTN_CLEFT, EQUIP_SLOT_C_LEFT, &activationButton);
        EquipSlotIsQuickBottle(BTN_CRIGHT, EQUIP_SLOT_C_RIGHT, &activationButton);
    }
    BtnState_Record(input, &BtnStateL, activationButton, false);

    BtnState_Record(input, &BtnStateDUp, BTN_DUP, BtnStateL.cur);
    BtnState_Record(input, &BtnStateDRight, BTN_DRIGHT, BtnStateL.cur);
    BtnState_Record(input, &BtnStateDLeft, BTN_DLEFT, BtnStateL.cur);
    BtnState_Record(input, &BtnStateDDown, BTN_DDOWN, BtnStateL.cur);
    
    BtnState_Record(input, &BtnStateEquips, BTN_DUP | BTN_DDOWN | BTN_DLEFT | BTN_DRIGHT | BTN_B | BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT, false);

    // if (BtnStateL.press) {
    //     recomp_printf("L Pressed\n");
    // }
}

// Prevent minimap toggle while aiming if L config selected
RECOMP_HOOK("MapDisp_Update")
void pre_MapDisp_Update(PlayState* play) {
    if (BtnStateL.press) {
        AudioSfx_StopById(NA_SE_SY_CAMERA_ZOOM_UP);
        AudioSfx_StopById(NA_SE_SY_CAMERA_ZOOM_DOWN);
    }

    R_MINIMAP_DISABLED = (u8)recomp_get_config_u32("minimap-visible");
}

// Handle EquipSlots

extern u16 sPlayerItemButtons[4];
extern Input* sPlayerControlInput;

u8 storedCDown = 0;
u8 storedCLeft = 0;
u8 storedCRight = 0;

void DisableButtonIfBottle(u8 btn, u8 equipSlot, u8* storage) {
    if (CHECK_BTN_ALL(sPlayerControlInput->press.button, btn)) {
        ItemId item = Player_GetItemOnButton(mPlay, mThis, equipSlot);
        if ((item >= ITEM_BOTTLE) && (item <= ITEM_OBABA_DRINK)) {
            sPlayerControlInput->press.button &= ~btn;
            *storage = btn;
        }
    }
}

RECOMP_HOOK("Player_UpdateItems") void Player_UpdateItems(Player* this, PlayState* play) {
    mThis = this;
    mPlay = play;
}

RECOMP_HOOK("func_8082FDC4") EquipSlot func_8082FDC4_Hook(void) {
    if (CONFIG_EQUIPSLOTS) {
        DisableButtonIfBottle(BTN_CDOWN, EQUIP_SLOT_C_DOWN, &storedCDown);
        DisableButtonIfBottle(BTN_CLEFT, EQUIP_SLOT_C_LEFT, &storedCLeft);
        DisableButtonIfBottle(BTN_CRIGHT, EQUIP_SLOT_C_RIGHT, &storedCRight);
    }
}