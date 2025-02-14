#include "global.h"
#include "menu.h"
#include "party_menu.h"
#include "pokemon.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "constants/party_menu.h"
#include "constants/songs.h"

static void Task_SoftboiledRestoreHealth(u8 taskId);
static void Task_DisplayHPRestoredMessage(u8 taskId);
static void Task_FinishSoftboiled(u8 taskId);
static void CantUseSoftboiledOnMon(u8 taskId);

bool8 SetUpFieldMove_SoftBoiled(void)
{
    u16 maxHp;
    u16 hp;
    u16 minHp;

    maxHp = GetMonData(&gPlayerParty[GetCursorSelectionMonId()], MON_DATA_MAX_HP);
    hp = GetMonData(&gPlayerParty[GetCursorSelectionMonId()], MON_DATA_HP);

    minHp = (maxHp / 5);
    if (hp > minHp)
        return TRUE;
    return FALSE;
}

void ChooseMonForSoftboiled(u8 taskId)
{
    gPartyMenu.action = PARTY_ACTION_SOFTBOILED;
    gPartyMenu.slotId2 =  gPartyMenu.slotId;
    AnimatePartySlot(GetCursorSelectionMonId(), 1);
    DisplayPartyMenuStdMessage(PARTY_MSG_USE_ON_WHICH_MON);
    gTasks[taskId].func = Task_HandleChooseMonInput;
}

void Task_TryUseSoftboiledOnPartyMon(u8 taskId)
{
    u16 hp;

    u8 userPartyId = gPartyMenu.slotId;
    u8 recipientPartyId = gPartyMenu.slotId2;
    if(recipientPartyId > PARTY_SIZE)
    {
        gPartyMenu.action = 0;
        DisplayPartyMenuStdMessage(PARTY_MSG_CHOOSE_MON);
        gTasks[taskId].func = Task_HandleChooseMonInput;
        return;
    }

    hp = GetMonData(&gPlayerParty[recipientPartyId], MON_DATA_HP);
    if(hp == 0 || userPartyId == recipientPartyId || GetMonData(&gPlayerParty[recipientPartyId], MON_DATA_MAX_HP) == hp)
    {
        CantUseSoftboiledOnMon(taskId);
        return;
    }

    // Take away Softboiled user's health first (-1)
    PlaySE(SE_USE_ITEM);
    PartyMenuModifyHP(taskId, userPartyId, -1, GetMonData(&gPlayerParty[userPartyId], MON_DATA_MAX_HP)/5, Task_SoftboiledRestoreHealth);
}

static void Task_SoftboiledRestoreHealth(u8 taskId)
{
    PlaySE(SE_USE_ITEM);
    PartyMenuModifyHP(taskId, gPartyMenu.slotId2, 1, GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_MAX_HP)/5, Task_DisplayHPRestoredMessage);
}

static void Task_DisplayHPRestoredMessage(u8 taskId)
{
    GetMonNickname(&gPlayerParty[gPartyMenu.slotId2], gStringVar1);
    StringExpandPlaceholders(gStringVarBuffer, gText_PkmnHPRestoredByVar2);
    DisplayPartyMenuMessage(gStringVarBuffer, FALSE);
    ScheduleBgCopyTilemapToVram(2);
    gTasks[taskId].func = Task_FinishSoftboiled;
}

static void Task_FinishSoftboiled(u8 taskId)
{
    if(IsPartyMenuTextPrinterActive() == TRUE)
        return;
    gPartyMenu.action = 0;
    AnimatePartySlot(gPartyMenu.slotId, 0);
    gPartyMenu.slotId = gPartyMenu.slotId2;
    AnimatePartySlot(gPartyMenu.slotId2, 1);
    ClearStdWindowAndFrameToTransparent(6, FALSE);
    ClearWindowTilemap(6);
    DisplayPartyMenuStdMessage(PARTY_MSG_CHOOSE_MON);
    gTasks[taskId].func = Task_HandleChooseMonInput;
}

static void Task_ChooseNewMonForSoftboiled(u8 taskId)
{
    if(IsPartyMenuTextPrinterActive() == TRUE)
        return;
    DisplayPartyMenuStdMessage(PARTY_MSG_USE_ON_WHICH_MON);
    gTasks[taskId].func = Task_HandleChooseMonInput;
}

static void CantUseSoftboiledOnMon(u8 taskId)
{
    PlaySE(SE_SELECT);
    DisplayPartyMenuMessage(gText_CantBeUsedOnPkmn, FALSE);
    ScheduleBgCopyTilemapToVram(2);
    gTasks[taskId].func = Task_ChooseNewMonForSoftboiled;
}
