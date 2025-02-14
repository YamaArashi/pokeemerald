#include "global.h"
#include "main.h"
#include "battle.h"
#include "battle_records.h"
#include "battle_setup.h"
#include "cable_club.h"
#include "data.h"
#include "event_data.h"
#include "field_message_box.h"
#include "field_specials.h"
#include "field_weather.h"
#include "international_string_util.h"
#include "link.h"
#include "link_rfu.h"
#include "load_save.h"
#include "m4a.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "union_room.h"
#include "mystery_gift.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "sound.h"
#include "start_menu.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "trade.h"
#include "trainer_card.h"
#include "party_menu.h"
#include "window.h"
#include "constants/battle_frontier.h"
#include "constants/cable_club.h"
#include "constants/songs.h"
#include "constants/trainers.h"

static const struct WindowTemplate sWindowTemplate_LinkPlayerCount = {
    .bg = 0,
    .tilemapLeft = 16,
    .tilemapTop = 11,
    .width = 11,
    .height = 2,
    .paletteNum = 15,
    .baseBlock = 0x0125,
};

static const u8 *const sTrainerCardColorNames[] = {
    gText_BronzeCard,
    gText_CopperCard,
    gText_SilverCard,
    gText_GoldCard
};

static void Task_LinkupStart(u8 taskId);
static void Task_LinkupAwaitConnection(u8 taskId);
static void Task_LinkupConfirmWhenReady(u8 taskId);
static void Task_LinkupAwaitConfirmation(u8 taskId);
static void Task_LinkupTryConfirmation(u8 taskId);
static void Task_LinkupConfirm(u8 taskId);
static void Task_LinkupExchangeDataWithLeader(u8 taskId);
static void Task_LinkupCheckStatusAfterConfirm(u8 taskId);
static void Task_LinkupAwaitTrainerCardData(u8 taskId);
static void Task_StopLinkup(u8 taskId);
static void Task_LinkupFailed(u8 taskId);
static void Task_LinkupConnectionError(u8 taskId);
static bool8 TryLinkTimeout(u8 taskId);
static void Task_ValidateMixingGameLanguage(u8 taskId);
static void Task_ReestablishLink(u8 taskId);
static void Task_ReestablishLinkAwaitConnection(u8 taskId);
static void Task_ReestablishLinkLeader(u8 taskId);
static void Task_ReestablishLinkAwaitConfirmation(u8 taskId);

#define tState      data[0]


#define tMinPlayers data[1]
#define tMaxPlayers data[2]
#define tNumPlayers data[3]
#define tTimer      data[4]
#define tWindowId   data[5]

static void CreateLinkupTask(u8 minPlayers, u8 maxPlayers)
{
    if (FindTaskIdByFunc(Task_LinkupStart) == TASK_NONE)
    {
        u8 taskId1;

        taskId1 = CreateTask(Task_LinkupStart, 80);
        gTasks[taskId1].tMinPlayers = minPlayers;
        gTasks[taskId1].tMaxPlayers = maxPlayers;
    }
}

static void PrintNumPlayersInLink(u16 windowId, u32 numPlayers)
{
    u8 xPos;

    ConvertIntToDecimalStringN(gStringVar1, numPlayers, STR_CONV_MODE_LEFT_ALIGN, 1);
    SetStandardWindowBorderStyle(windowId, FALSE);
    StringExpandPlaceholders(gStringVarBuffer, gText_NumPlayerLink);
    xPos = GetStringCenterAlignXOffset(FONT_NORMAL, gStringVarBuffer, 88);
    AddTextPrinterParameterized(windowId, FONT_NORMAL, gStringVarBuffer, xPos, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void ClearLinkPlayerCountWindow(u16 windowId)
{
    // Following this call with a copy-to-vram with mode COPYWIN_FULL is identical to
    // calling ClearStdWindowAndFrame(windowId, TRUE).
    ClearStdWindowAndFrame(windowId, FALSE);
    CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void UpdateLinkPlayerCountDisplay(u8 taskId, u8 numPlayers)
{
    s16 *data = gTasks[taskId].data;

    if (numPlayers != tNumPlayers)
    {
        if (numPlayers <= 1)
            ClearLinkPlayerCountWindow(tWindowId);
        else
            PrintNumPlayersInLink(tWindowId, numPlayers);
        tNumPlayers = numPlayers;
    }
}

static u32 ExchangeDataAndGetLinkupStatus(u8 minPlayers, u8 maxPlayers)
{
    switch (GetLinkPlayerDataExchangeStatusTimed(minPlayers, maxPlayers))
    {
    case EXCHANGE_COMPLETE:
        return LINKUP_SUCCESS;
    case EXCHANGE_DIFF_SELECTIONS:
        return LINKUP_DIFF_SELECTIONS;
    case EXCHANGE_PLAYER_NOT_READY:
        return LINKUP_PLAYER_NOT_READY;
    case EXCHANGE_PARTNER_NOT_READY:
        return LINKUP_PARTNER_NOT_READY;
    case EXCHANGE_WRONG_NUM_PLAYERS:
        ConvertIntToDecimalStringN(gStringVar1, GetLinkPlayerCount_2(), STR_CONV_MODE_LEFT_ALIGN, 1);
        return LINKUP_WRONG_NUM_PLAYERS;
    case EXCHANGE_STAT_7:
        return LINKUP_FAILED_CONTEST_GMODE;
    case EXCHANGE_TIMED_OUT:
    default:
        return LINKUP_ONGOING;
    }
}

static bool32 CheckLinkErrored(u8 taskId)
{
    if (HasLinkErrorOccurred() == TRUE)
    {
        gTasks[taskId].func = Task_LinkupConnectionError;
        return TRUE;
    }
    return FALSE;
}

static bool32 CheckLinkCanceledBeforeConnection(u8 taskId)
{
    if ((JOY_NEW(B_BUTTON))
     && IsLinkConnectionEstablished() == FALSE)
    {
        gLinkType = 0;
        gTasks[taskId].func = Task_LinkupFailed;
        return TRUE;
    }
    return FALSE;
}

static bool32 CheckLinkCanceled(u8 taskId)
{
    if (IsLinkConnectionEstablished())
        SetSuppressLinkErrorMessage(TRUE);

    if (JOY_NEW(B_BUTTON))
    {
        gLinkType = 0;
        gTasks[taskId].func = Task_LinkupFailed;
        return TRUE;
    }
    return FALSE;
}

static bool32 CheckSioErrored(u8 taskId)
{
    if (GetSioMultiSI() == TRUE)
    {
        gTasks[taskId].func = Task_LinkupConnectionError;
        return TRUE;
    }
    return FALSE;
}

static void UNUSED Task_DelayedBlockRequest(u8 taskId)
{
    gTasks[taskId].data[0]++;
    if (gTasks[taskId].data[0] == 10)
    {
        SendBlockRequest(BLOCK_REQ_SIZE_100);
        DestroyTask(taskId);
    }
}

static void Task_LinkupStart(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (data[0] == 0)
    {
        OpenLinkTimed();
        ResetLinkPlayerCount();
        ResetLinkPlayers();
        tWindowId = AddWindow(&sWindowTemplate_LinkPlayerCount);
    }
    else if (data[0] > 9)
    {
        gTasks[taskId].func = Task_LinkupAwaitConnection;
    }
    data[0]++;
}

static void Task_LinkupAwaitConnection(u8 taskId)
{
    u32 playerCount = GetLinkPlayerCount_2();

    if (CheckLinkCanceledBeforeConnection(taskId) == TRUE
     || CheckLinkCanceled(taskId) == TRUE
     || playerCount < 2)
        return;

    SetSuppressLinkErrorMessage(TRUE);
    gTasks[taskId].data[3] = 0;
    if (IsLinkMaster() == TRUE)
    {
        PlaySE(SE_PIN);
        ShowFieldAutoScrollMessage(gText_ConfirmLinkWhenPlayersReady);
        gTasks[taskId].func = Task_LinkupConfirmWhenReady;
    }
    else
    {
        PlaySE(SE_BOO);
        ShowFieldAutoScrollMessage(gText_AwaitingLinkup);
        gTasks[taskId].func = Task_LinkupExchangeDataWithLeader;
    }
}

static void Task_LinkupConfirmWhenReady(u8 taskId)
{
    if (CheckLinkCanceledBeforeConnection(taskId) == TRUE
     || CheckSioErrored(taskId) == TRUE
     || CheckLinkErrored(taskId) == TRUE)
        return;

    if (GetFieldMessageBoxMode() == FIELD_MESSAGE_BOX_HIDDEN)
    {
        gTasks[taskId].tNumPlayers = 0;
        gTasks[taskId].func = Task_LinkupAwaitConfirmation;
    }
}

static void Task_LinkupAwaitConfirmation(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    s32 linkPlayerCount = GetLinkPlayerCount_2();

    if (CheckLinkCanceledBeforeConnection(taskId) == TRUE
     || CheckSioErrored(taskId) == TRUE
     || CheckLinkErrored(taskId) == TRUE)
        return;

    UpdateLinkPlayerCountDisplay(taskId, linkPlayerCount);

    if (!(JOY_NEW(A_BUTTON)))
        return;

    if (linkPlayerCount < tMinPlayers)
        return;

    SaveLinkPlayers(linkPlayerCount);
    ClearLinkPlayerCountWindow(tWindowId);
    ConvertIntToDecimalStringN(gStringVar1, linkPlayerCount, STR_CONV_MODE_LEFT_ALIGN, 1);
    ShowFieldAutoScrollMessage(gText_ConfirmStartLinkWithXPlayers);
    gTasks[taskId].func = Task_LinkupTryConfirmation;
}

static void Task_LinkupTryConfirmation(u8 taskId)
{
    if (CheckLinkCanceledBeforeConnection(taskId) == TRUE
     || CheckSioErrored(taskId) == TRUE
     || CheckLinkErrored(taskId) == TRUE)
        return;

    if (GetFieldMessageBoxMode() == FIELD_MESSAGE_BOX_HIDDEN)
    {
        if (GetSavedPlayerCount() != GetLinkPlayerCount_2())
        {
            ShowFieldAutoScrollMessage(gText_ConfirmLinkWhenPlayersReady);
            gTasks[taskId].func = Task_LinkupConfirmWhenReady;
        }
        else if (JOY_HELD(B_BUTTON))
        {
            ShowFieldAutoScrollMessage(gText_ConfirmLinkWhenPlayersReady);
            gTasks[taskId].func = Task_LinkupConfirmWhenReady;
        }
        else if (JOY_HELD(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            CheckShouldAdvanceLinkState();
            gTasks[taskId].func = Task_LinkupConfirm;
        }
    }
}

static void Task_LinkupConfirm(u8 taskId)
{
    u8 minPlayers = gTasks[taskId].tMinPlayers;
    u8 maxPlayers = gTasks[taskId].tMaxPlayers;

    if (CheckLinkErrored(taskId) == TRUE
     || TryLinkTimeout(taskId) == TRUE)
        return;

    if (GetLinkPlayerCount_2() != GetSavedPlayerCount())
    {
        gTasks[taskId].func = Task_LinkupConnectionError;
    }
    else
    {
        gSpecialVar_Result = ExchangeDataAndGetLinkupStatus(minPlayers, maxPlayers);
        if (gSpecialVar_Result != LINKUP_ONGOING)
            gTasks[taskId].func = Task_LinkupCheckStatusAfterConfirm;
    }
}

static void Task_LinkupExchangeDataWithLeader(u8 taskId)
{
    u8 minPlayers, maxPlayers;
    struct TrainerCard *card;

    minPlayers = gTasks[taskId].tMinPlayers;
    maxPlayers = gTasks[taskId].tMaxPlayers;

    if (CheckLinkCanceledBeforeConnection(taskId) == TRUE
     || CheckLinkErrored(taskId) == TRUE)
        return;

    gSpecialVar_Result = ExchangeDataAndGetLinkupStatus(minPlayers, maxPlayers);
    if (gSpecialVar_Result == LINKUP_ONGOING)
        return;
    if (gSpecialVar_Result == LINKUP_DIFF_SELECTIONS
     || gSpecialVar_Result == LINKUP_WRONG_NUM_PLAYERS)
    {
        SetCloseLinkCallback();
        HideFieldMessageBox();
        gTasks[taskId].func = Task_StopLinkup;
    }
    else if (gSpecialVar_Result == LINKUP_PLAYER_NOT_READY
          || gSpecialVar_Result == LINKUP_PARTNER_NOT_READY)
    {
        CloseLink();
        HideFieldMessageBox();
        gTasks[taskId].func = Task_StopLinkup;
    }
    else
    {
        gFieldLinkPlayerCount = GetLinkPlayerCount_2();
        gLocalLinkPlayerId = GetMultiplayerId();
        SaveLinkPlayers(gFieldLinkPlayerCount);
        card = (struct TrainerCard *)gBlockSendBuffer;
        TrainerCard_GenerateCardForLinkPlayer(card);
        card->monSpecies[0] = GetMonData(&gPlayerParty[gSelectedOrderFromParty[0] - 1], MON_DATA_SPECIES, NULL);
        card->monSpecies[1] = GetMonData(&gPlayerParty[gSelectedOrderFromParty[1] - 1], MON_DATA_SPECIES, NULL);
        gTasks[taskId].func = Task_LinkupAwaitTrainerCardData;
    }
}

static void Task_LinkupCheckStatusAfterConfirm(u8 taskId)
{
    struct TrainerCard *card;

    if (CheckLinkErrored(taskId) == TRUE)
        return;

    if (gSpecialVar_Result == LINKUP_WRONG_NUM_PLAYERS)
    {
        if (!Link_AnyPartnersPlayingRubyOrSapphire())
        {
            SetCloseLinkCallback();
            HideFieldMessageBox();
            gTasks[taskId].func = Task_StopLinkup;
        }
        else
        {
            CloseLink();
            HideFieldMessageBox();
            gTasks[taskId].func = Task_StopLinkup;
        }
    }
    else if (gSpecialVar_Result == LINKUP_DIFF_SELECTIONS)
    {
        SetCloseLinkCallback();
        HideFieldMessageBox();
        gTasks[taskId].func = Task_StopLinkup;
    }
    else if (gSpecialVar_Result == LINKUP_PLAYER_NOT_READY
          || gSpecialVar_Result == LINKUP_PARTNER_NOT_READY)
    {
        CloseLink();
        HideFieldMessageBox();
        gTasks[taskId].func = Task_StopLinkup;
    }
    else
    {
        gFieldLinkPlayerCount = GetLinkPlayerCount_2();
        gLocalLinkPlayerId = GetMultiplayerId();
        SaveLinkPlayers(gFieldLinkPlayerCount);
        card = (struct TrainerCard *)gBlockSendBuffer;
        TrainerCard_GenerateCardForLinkPlayer(card);
        card->monSpecies[0] = GetMonData(&gPlayerParty[gSelectedOrderFromParty[0] - 1], MON_DATA_SPECIES, NULL);
        card->monSpecies[1] = GetMonData(&gPlayerParty[gSelectedOrderFromParty[1] - 1], MON_DATA_SPECIES, NULL);
        gTasks[taskId].func = Task_LinkupAwaitTrainerCardData;
        SendBlockRequest(BLOCK_REQ_SIZE_100);
    }
}

bool32 AreBattleTowerLinkSpeciesSame(u16 *speciesList1, u16 *speciesList2)
{
    int i;
    int j;
    bool32 haveSameSpecies = FALSE;
    int numSameSpecies = 0;

    gStringVar1[0] = EOS;
    gStringVar2[0] = EOS;

    for (i = 0; i < FRONTIER_MULTI_PARTY_SIZE; i++)
    {
        for (j = 0; j < FRONTIER_MULTI_PARTY_SIZE; j++)
        {
            if (speciesList1[i] == speciesList2[j])
            {
                if (numSameSpecies == 0)
                {
                    StringCopy(gStringVar1, gSpeciesNames[speciesList1[i]]);
                    haveSameSpecies = TRUE;
                }

                if (numSameSpecies == 1)
                {
                    StringCopy(gStringVar2, gSpeciesNames[speciesList1[i]]);
                    haveSameSpecies = TRUE;
                }

                numSameSpecies++;
            }
        }
    }

    // var below is read by BattleFrontier_BattleTowerLobby_EventScript_AbortLink
    gSpecialVar_0x8005 = numSameSpecies;

    return haveSameSpecies;
}

static void FinishLinkup(u16 *linkupStatus, u32 taskId)
{
    struct TrainerCard *trainerCards = gTrainerCards;

    if (*linkupStatus == LINKUP_SUCCESS)
    {
        if (gLinkType == LINKTYPE_BATTLE_TOWER_50 || gLinkType == LINKTYPE_BATTLE_TOWER_OPEN)
        {
            if (AreBattleTowerLinkSpeciesSame(trainerCards[0].monSpecies, trainerCards[1].monSpecies))
            {
                // Unsuccessful battle tower linkup
                *linkupStatus = LINKUP_FAILED_BATTLE_TOWER;
                SetCloseLinkCallback();
                gTasks[taskId].func = Task_StopLinkup;
            }
            else
            {
                // Successful battle tower linkup
                ClearLinkPlayerCountWindow(gTasks[taskId].tWindowId);
                ScriptContext_Enable();
                DestroyTask(taskId);
            }
        }
        else
        {
            // Successful linkup
            ClearLinkPlayerCountWindow(gTasks[taskId].tWindowId);
            ScriptContext_Enable();
            DestroyTask(taskId);
        }
    }
    else
    {
        // Unsuccessful linkup
        SetCloseLinkCallback();
        gTasks[taskId].func = Task_StopLinkup;
    }
}

static void Task_LinkupAwaitTrainerCardData(u8 taskId)
{
    u8 index;

    if (CheckLinkErrored(taskId) == TRUE)
        return;

    if (GetBlockReceivedStatus() != GetSavedLinkPlayerCountAsBitFlags())
        return;

    for (index = 0; index < GetLinkPlayerCount(); index++)
    {
        CopyTrainerCardData(&gTrainerCards[index], (struct TrainerCard *)gBlockRecvBuffer[index], gLinkPlayers[index].version);
    }

    SetSuppressLinkErrorMessage(FALSE);
    ResetBlockReceivedFlags();
    FinishLinkup(&gSpecialVar_Result, taskId);
}

static void Task_StopLinkup(u8 taskId)
{
    if (!gReceivedRemoteLinkPlayers)
    {
        ClearLinkPlayerCountWindow(gTasks[taskId].tWindowId);
        ScriptContext_Enable();
        RemoveWindow(gTasks[taskId].tWindowId);
        DestroyTask(taskId);
    }
}

static void Task_LinkupFailed(u8 taskId)
{
    gSpecialVar_Result = LINKUP_FAILED;
    ClearLinkPlayerCountWindow(gTasks[taskId].tWindowId);
    StopFieldMessage();
    RemoveWindow(gTasks[taskId].tWindowId);
    ScriptContext_Enable();
    DestroyTask(taskId);
}

static void Task_LinkupConnectionError(u8 taskId)
{
    gSpecialVar_Result = LINKUP_CONNECTION_ERROR;
    ClearLinkPlayerCountWindow(gTasks[taskId].tWindowId);
    RemoveWindow(gTasks[taskId].tWindowId);
    HideFieldMessageBox();
    ScriptContext_Enable();
    DestroyTask(taskId);
}

static bool8 TryLinkTimeout(u8 taskId)
{
    gTasks[taskId].tTimer++;
    if (gTasks[taskId].tTimer > 600)
    {
        gTasks[taskId].func = Task_LinkupConnectionError;
        return TRUE;
    }

    return FALSE;
}

void TryBattleLinkup(void)
{
    u8 minPlayers = 2;
    u8 maxPlayers = 2;

    switch (gSpecialVar_0x8004)
    {
    case USING_SINGLE_BATTLE:
        minPlayers = 2;
        gLinkType = LINKTYPE_SINGLE_BATTLE;
        break;
    case USING_DOUBLE_BATTLE:
        minPlayers = 2;
        gLinkType = LINKTYPE_DOUBLE_BATTLE;
        break;
    case USING_MULTI_BATTLE:
        minPlayers = 4;
        maxPlayers = 4;
        gLinkType = LINKTYPE_MULTI_BATTLE;
        break;
    case USING_BATTLE_TOWER:
        minPlayers = 2;
        if (gSaveBlock2Ptr->frontier.lvlMode == FRONTIER_LVL_50)
            gLinkType = LINKTYPE_BATTLE_TOWER_50;
        else
            gLinkType = LINKTYPE_BATTLE_TOWER_OPEN;

        break;
    }

    CreateLinkupTask(minPlayers, maxPlayers);
}

#undef tMinPlayers
#undef tMaxPlayers
#undef tNumPlayers
#undef tTimer
#undef tWindowId

void TryTradeLinkup(void)
{
    gLinkType = LINKTYPE_TRADE_SETUP;
    gBattleTypeFlags = 0;
    CreateLinkupTask(2, 2);
}

void TryRecordMixLinkup(void)
{
    gSpecialVar_Result = LINKUP_ONGOING;
    gLinkType = LINKTYPE_RECORD_MIX_BEFORE;
    gBattleTypeFlags = 0;
    CreateLinkupTask(2, 4);
}

void ValidateMixingGameLanguage(void)
{
    u32 taskId = FindTaskIdByFunc(Task_ValidateMixingGameLanguage);

    if (taskId == TASK_NONE)
    {
        taskId = CreateTask(Task_ValidateMixingGameLanguage, 80);
        gTasks[taskId].tState = 0;
    }
}

static void Task_ValidateMixingGameLanguage(u8 taskId)
{
    int playerCount;
    int i;

    switch (gTasks[taskId].tState)
    {
    case 0:
        if (gSpecialVar_Result == LINKUP_SUCCESS)
        {
            bool32 mixingForeignGames = FALSE;
            bool32 isEnglishRSLinked = FALSE;
            bool32 isJapaneseEmeraldLinked = FALSE;

            playerCount = GetLinkPlayerCount();
            for (i = 0; i < playerCount; i++)
            {
                u32 version = (u8)gLinkPlayers[i].version;
                u32 language = gLinkPlayers[i].language;

                if (version == VERSION_RUBY || version == VERSION_SAPPHIRE)
                {
                    if (language == LANGUAGE_JAPANESE)
                    {
                        mixingForeignGames = TRUE;
                        break;
                    }
                    else
                    {
                        isEnglishRSLinked = TRUE;
                    }
                }
                else if (version == VERSION_EMERALD)
                {
                    if (language == LANGUAGE_JAPANESE)
                    {
                        isJapaneseEmeraldLinked = TRUE;
                    }
                }
            }

            if (isEnglishRSLinked && isJapaneseEmeraldLinked)
            {
                mixingForeignGames = TRUE;
            }

            if (mixingForeignGames)
            {
                gSpecialVar_Result = LINKUP_FOREIGN_GAME;
                SetCloseLinkCallbackHandleJP();
                gTasks[taskId].tState = 1;
                return;
            }
        }
        ScriptContext_Enable();
        DestroyTask(taskId);
        break;
    case 1:
        if (!gReceivedRemoteLinkPlayers)
        {
            ScriptContext_Enable();
            DestroyTask(taskId);
        }
        break;
    }
}

void TryBerryBlenderLinkup(void)
{
    gLinkType = LINKTYPE_BERRY_BLENDER_SETUP;
    gBattleTypeFlags = 0;
    CreateLinkupTask(2, 4);
}

void TryContestGModeLinkup(void)
{
    gLinkType = LINKTYPE_CONTEST_GMODE;
    gBattleTypeFlags = 0;
    CreateLinkupTask(4, 4);
}

void TryContestEModeLinkup(void)
{
    gLinkType = LINKTYPE_CONTEST_EMODE;
    gBattleTypeFlags = 0;
    CreateLinkupTask(2, 4);
}

u8 CreateTask_ReestablishCableClubLink(void)
{
    if (FuncIsActiveTask(Task_ReestablishLink) != FALSE)
        return TASK_NONE;

    switch (gSpecialVar_0x8004)
    {
    case USING_SINGLE_BATTLE:
        gLinkType = LINKTYPE_SINGLE_BATTLE;
        break;
    case USING_DOUBLE_BATTLE:
        gLinkType = LINKTYPE_DOUBLE_BATTLE;
        break;
    case USING_MULTI_BATTLE:
        gLinkType = LINKTYPE_MULTI_BATTLE;
        break;
    case USING_BATTLE_TOWER:
        if (gSaveBlock2Ptr->frontier.lvlMode == FRONTIER_LVL_50)
            gLinkType = LINKTYPE_BATTLE_TOWER_50;
        else
            gLinkType = LINKTYPE_BATTLE_TOWER_OPEN;
        break;
    case USING_TRADE_CENTER:
        gLinkType = LINKTYPE_TRADE;
        break;
    case USING_RECORD_CORNER:
        gLinkType = LINKTYPE_RECORD_MIX_AFTER;
        break;
    }

    return CreateTask(Task_ReestablishLink, 80);
}

static void Task_ReestablishLink(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (data[0] == 0)
    {
        OpenLink();
        ResetLinkPlayers();
        CreateTask(Task_WaitForLinkPlayerConnection, 80);
    }
    else if (data[0] >= 10)
    {
        gTasks[taskId].func = Task_ReestablishLinkAwaitConnection;
    }
    data[0]++;
}

static void Task_ReestablishLinkAwaitConnection(u8 taskId)
{
    if (GetLinkPlayerCount_2() >= 2)
    {
        if (IsLinkMaster() == TRUE)
            gTasks[taskId].func = Task_ReestablishLinkLeader;
        else
            gTasks[taskId].func = Task_ReestablishLinkAwaitConfirmation;
    }
}

static void Task_ReestablishLinkLeader(u8 taskId)
{
    if (GetSavedPlayerCount() == GetLinkPlayerCount_2())
    {
        CheckShouldAdvanceLinkState();
        gTasks[taskId].func = Task_ReestablishLinkAwaitConfirmation;
    }
}

static void Task_ReestablishLinkAwaitConfirmation(u8 taskId)
{
    if (gReceivedRemoteLinkPlayers == TRUE
     && IsLinkPlayerDataExchangeComplete() == TRUE)
    {
        CheckLinkPlayersMatchSaved();
        StartSendingKeysToLink();
        DestroyTask(taskId);
    }
}

// Unused
void CableClubSaveGame(void)
{
    SaveGame();
}

static void SetLinkBattleTypeFlags(int linkService)
{
    switch (linkService)
    {
    case USING_SINGLE_BATTLE:
        gBattleTypeFlags = BATTLE_TYPE_LINK | BATTLE_TYPE_TRAINER;
        break;
    case USING_DOUBLE_BATTLE:
        gBattleTypeFlags = BATTLE_TYPE_DOUBLE | BATTLE_TYPE_LINK | BATTLE_TYPE_TRAINER;
        break;
    case USING_MULTI_BATTLE:
        ReducePlayerPartyToSelectedMons();
        gBattleTypeFlags = BATTLE_TYPE_DOUBLE | BATTLE_TYPE_LINK | BATTLE_TYPE_TRAINER | BATTLE_TYPE_MULTI;
        break;
    case USING_BATTLE_TOWER:
        gBattleTypeFlags = BATTLE_TYPE_BATTLE_TOWER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_LINK | BATTLE_TYPE_TRAINER | BATTLE_TYPE_MULTI;
        break;
    }
}

#define tTimer data[1]

static void Task_StartWiredCableClubBattle(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->tState)
    {
    case 0:
        FadeScreen(FADE_TO_BLACK, 0);
        gLinkType = LINKTYPE_BATTLE;
        ClearLinkCallback_2();
        task->tState++;
        break;
    case 1:
        if (!gPaletteFade.active)
            task->tState++;
        break;
    case 2:
        task->tTimer++;
        if (task->tTimer > 20)
            task->tState++;
        break;
    case 3:
        SetCloseLinkCallback();
        task->tState++;
        break;
    case 4:
        if (!gReceivedRemoteLinkPlayers)
            task->tState++;
        break;
    case 5:
        if (gLinkPlayers[0].trainerId & 1)
            PlayMapChosenOrBattleBGM(MUS_VS_GYM_LEADER);
        else
            PlayMapChosenOrBattleBGM(MUS_VS_TRAINER);

        SetLinkBattleTypeFlags(gSpecialVar_0x8004);
        CleanupOverworldWindowsAndTilemaps();
        gTrainerBattleOpponent_A = TRAINER_LINK_OPPONENT;
        SetMainCallback2(CB2_InitBattle);
        gMain.savedCallback = CB2_ReturnFromCableClubBattle;
        DestroyTask(taskId);
        break;
    }
}

static void Task_StartWirelessCableClubBattle(u8 taskId)
{
    int i;
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        FadeScreen(FADE_TO_BLACK, 0);
        gLinkType = LINKTYPE_BATTLE;
        ClearLinkCallback_2();
        tState = 1;
        break;
    case 1:
        if (!gPaletteFade.active)
            tState = 2;
        break;
    case 2:
        SendBlock(0, &gLocalLinkPlayer, sizeof(gLocalLinkPlayer));
        tState = 3;
        break;
    case 3:
        if (GetBlockReceivedStatus() == GetLinkPlayerCountAsBitFlags())
        {
            for (i = 0; i < GetLinkPlayerCount(); i++)
            {
                struct LinkPlayer *player = (struct LinkPlayer *)gBlockRecvBuffer[i];
                gLinkPlayers[i] = *player;
                ConvertLinkPlayerName(&gLinkPlayers[i]);
                ResetBlockReceivedFlag(i);
            }
            tState = 4;
        }
        break;
    case 4:
        tTimer++;
        if (tTimer > 20)
            tState = 5;
        break;
    case 5:
        SetLinkStandbyCallback();
        tState = 6;
        break;
    case 6:
        if (IsLinkTaskFinished())
        {
            tState = 7;
        }
        break;
    case 7:
        if (gLinkPlayers[0].trainerId & 1)
            PlayMapChosenOrBattleBGM(MUS_VS_GYM_LEADER);
        else
            PlayMapChosenOrBattleBGM(MUS_VS_TRAINER);

        gLinkPlayers[0].linkType = LINKTYPE_BATTLE;
        SetLinkBattleTypeFlags(gSpecialVar_0x8004);
        CleanupOverworldWindowsAndTilemaps();
        gTrainerBattleOpponent_A = TRAINER_LINK_OPPONENT;
        SetMainCallback2(CB2_InitBattle);
        gMain.savedCallback = CB2_ReturnFromCableClubBattle;
        DestroyTask(taskId);
        break;
    }
}

#undef tTimer

static void CB2_ReturnFromUnionRoomBattle(void)
{
    u8 playerCount;
    int i;
    bool32 linkedWithFRLG;

    switch (gMain.state)
    {
    case 0:
        playerCount = GetLinkPlayerCount();
        linkedWithFRLG = FALSE;
        for (i = 0; i < playerCount; i++)
        {
            u32 version = (u8)gLinkPlayers[i].version;
            if (version == VERSION_FIRE_RED || version == VERSION_LEAF_GREEN)
            {
                linkedWithFRLG = TRUE;
                break;
            }
        }

        if (linkedWithFRLG)
        {
            gMain.state = 2;
        }
        else
        {
            SetCloseLinkCallback();
            gMain.state = 1;
        }
        break;
    case 1:
        if (!gReceivedRemoteLinkPlayers)
        {
            SetMainCallback2(CB2_ReturnToField);
        }
        break;
    case 2:
        SetMainCallback2(CB2_ReturnToField);
        break;
    }
    RunTasks();
}

void CB2_ReturnFromCableClubBattle(void)
{
    gBattleTypeFlags &= ~BATTLE_TYPE_LINK_IN_BATTLE;
    Overworld_ResetMapMusic();
    LoadPlayerParty();
    SavePlayerBag();
    UpdateTrainerFansAfterLinkBattle();

    if (gSpecialVar_0x8004 == USING_SINGLE_BATTLE || gSpecialVar_0x8004 == USING_DOUBLE_BATTLE)
    {
        UpdatePlayerLinkBattleRecords(gLocalLinkPlayerId ^ 1);
        if (gWirelessCommType)
        {
            switch (gBattleOutcome)
            {
            case B_OUTCOME_WON:
                MysteryGift_TryIncrementStat(CARD_STAT_BATTLES_WON, gLinkPlayers[GetMultiplayerId() ^ 1].trainerId);
                break;
            case B_OUTCOME_LOST:
                MysteryGift_TryIncrementStat(CARD_STAT_BATTLES_LOST, gLinkPlayers[GetMultiplayerId() ^ 1].trainerId);
                break;
            }
        }
    }

    if (InUnionRoom() == TRUE)
        gMain.savedCallback = CB2_ReturnFromUnionRoomBattle;
    else
        gMain.savedCallback = CB2_ReturnToFieldFromMultiplayer;

    SetMainCallback2(CB2_SetUpSaveAfterLinkBattle);
}

void CleanupLinkRoomState(void)
{
    if (gSpecialVar_0x8004 == USING_SINGLE_BATTLE
     || gSpecialVar_0x8004 == USING_DOUBLE_BATTLE
     || gSpecialVar_0x8004 == USING_MULTI_BATTLE
     || gSpecialVar_0x8004 == USING_BATTLE_TOWER)
    {
        LoadPlayerParty();
        SavePlayerBag();
    }
    SetWarpDestinationToDynamicWarp(WARP_ID_DYNAMIC);
}

void ExitLinkRoom(void)
{
    QueueExitLinkRoomKey();
}

// Note: gSpecialVar_0x8005 contains the id of the seat the player entered
static void Task_EnterCableClubSeat(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->tState)
    {
    case 0:
        ShowFieldMessage(gText_PleaseWaitForLink);
        task->tState = 1;
        break;
    case 1:
        if (IsFieldMessageBoxHidden())
        {
            SetInCableClubSeat();
            SetLocalLinkPlayerId(gSpecialVar_0x8005);
            task->tState = 2;
        }
        break;
    case 2:
        switch (GetCableClubPartnersReady())
        {
        case CABLE_SEAT_WAITING:
            break;
        case CABLE_SEAT_SUCCESS:
            // Partners linked and ready, switch to relevant link function
            HideFieldMessageBox();
            task->tState = 0;
            SetStartedCableClubActivity();
            SwitchTaskToFollowupFunc(taskId);
            break;
        case CABLE_SEAT_FAILED:
            task->tState = 3;
            break;
        }
        break;
    case 3:
        // Exit, failure
        SetLinkWaitingForScript();
        EraseFieldMessageBox(TRUE);
        DestroyTask(taskId);
        ScriptContext_Enable();
        break;
    }
}

void CreateTask_EnterCableClubSeat(TaskFunc followupFunc)
{
    u8 taskId = CreateTask(Task_EnterCableClubSeat, 80);
    SetTaskFuncWithFollowupFunc(taskId, Task_EnterCableClubSeat, followupFunc);
    ScriptContext_Stop();
}

static void Task_StartWiredTrade(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->tState)
    {
    case 0:
        LockPlayerFieldControls();
        FadeScreen(FADE_TO_BLACK, 0);
        ClearLinkCallback_2();
        task->tState++;
        break;
    case 1:
        if (!gPaletteFade.active)
            task->tState++;
        break;
    case 2:
        gSelectedTradeMonPositions[TRADE_PLAYER] = 0;
        gSelectedTradeMonPositions[TRADE_PARTNER] = 0;
        m4aMPlayAllStop();
        SetCloseLinkCallback();
        task->tState++;
        break;
    case 3:
        if (!gReceivedRemoteLinkPlayers)
        {
            SetMainCallback2(CB2_StartCreateTradeMenu);
            DestroyTask(taskId);
        }
        break;
    }
}

static void Task_StartWirelessTrade(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        LockPlayerFieldControls();
        FadeScreen(FADE_TO_BLACK, 0);
        ClearLinkRfuCallback();
        tState++;
        break;
    case 1:
        if (!gPaletteFade.active)
            tState++;
        break;
    case 2:
        gSelectedTradeMonPositions[TRADE_PLAYER] = 0;
        gSelectedTradeMonPositions[TRADE_PARTNER] = 0;
        m4aMPlayAllStop();
        SetLinkStandbyCallback();
        tState++;
        break;
    case 3:
        if (IsLinkTaskFinished())
        {
            CreateTask_CreateTradeMenu();
            DestroyTask(taskId);
        }
        break;
    }
}

void PlayerEnteredTradeSeat(void)
{
    if (gWirelessCommType != 0)
        CreateTask_EnterCableClubSeat(Task_StartWirelessTrade);
    else
        CreateTask_EnterCableClubSeat(Task_StartWiredTrade);
}

static void UNUSED CreateTask_StartWiredTrade(void)
{
    CreateTask(Task_StartWiredTrade, 80);
}

// Implemented in Ruby/Sapphire
void UNUSED Script_StartWiredTrade(void)
{
    // CreateTask_StartWiredTrade();
    // ScriptContext_Stop();
}

void ColosseumPlayerSpotTriggered(void)
{
    gLinkType = LINKTYPE_BATTLE;

    if (gWirelessCommType)
        CreateTask_EnterCableClubSeat(Task_StartWirelessCableClubBattle);
    else
        CreateTask_EnterCableClubSeat(Task_StartWiredCableClubBattle);
}

static UNUSED void CreateTask_EnterCableClubSeatNoFollowup(void)
{
    u8 UNUSED taskId = CreateTask(Task_EnterCableClubSeat, 80);
    ScriptContext_Stop();
}

void Script_ShowLinkTrainerCard(void)
{
    ShowTrainerCardInLink(gSpecialVar_0x8006, CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

// Returns FALSE if the player has no stars. Returns TRUE otherwise, and puts the name of the
// color into gStringVar2.
bool32 GetLinkTrainerCardColor(u8 linkPlayerIndex)
{
    u32 numStars;

    gSpecialVar_0x8006 = linkPlayerIndex;
    StringCopy(gStringVar1, gLinkPlayers[linkPlayerIndex].name);

    numStars = GetTrainerCardStars(linkPlayerIndex);
    if (numStars == 0)
        return FALSE;

    StringCopy(gStringVar2, sTrainerCardColorNames[numStars - 1]);
    return TRUE;
}

#define tTimer data[0]

void Task_WaitForLinkPlayerConnection(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    task->tTimer++;
    if (task->tTimer > 300)
    {
        CloseLink();
        SetMainCallback2(CB2_LinkError);
        DestroyTask(taskId);
    }

    if (gReceivedRemoteLinkPlayers)
    {
        // Players connected, destroy task
        if (gWirelessCommType == 0)
        {
            if (!DoesLinkPlayerCountMatchSaved())
            {
                CloseLink();
                SetMainCallback2(CB2_LinkError);
            }
            DestroyTask(taskId);
        }
        else
        {
            DestroyTask(taskId);
        }
    }
}

#undef tTimer

static void Task_WaitExitToScript(u8 taskId)
{
    if (!gReceivedRemoteLinkPlayers)
    {
        ScriptContext_Enable();
        DestroyTask(taskId);
    }
}

static void UNUSED ExitLinkToScript(u8 taskId)
{
    SetCloseLinkCallback();
    gTasks[taskId].func = Task_WaitExitToScript;
}

#define tTimer data[1]

// Confirm that all cabled link players are connected
void Task_ReconnectWithLinkPlayers(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        if (gWirelessCommType != 0)
        {
            DestroyTask(taskId);
        }
        else
        {
            OpenLink();
            CreateTask(Task_WaitForLinkPlayerConnection, 1);
            tState++;
        }
        break;
    case 1:
        if (++tTimer > 11)
        {
            tTimer = 0;
            tState++;
        }
        break;
    case 2:
        if (GetLinkPlayerCount_2() >= GetSavedPlayerCount())
        {
            if (IsLinkMaster())
            {
                if (++tTimer > 30)
                {
                    CheckShouldAdvanceLinkState();
                    tState++;
                }
            }
            else
            {
                tState++;
            }
        }
        break;
    case 3:
        if (gReceivedRemoteLinkPlayers == TRUE && IsLinkPlayerDataExchangeComplete() == TRUE)
        {
            DestroyTask(taskId);
        }
        break;
    }
}

#undef tTimer

void TrySetBattleTowerLinkType(void)
{
    if (gWirelessCommType == 0)
        gLinkType = LINKTYPE_BATTLE_TOWER;
}

#undef tState
