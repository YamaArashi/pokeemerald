#include "global.h"
#include "malloc.h"
#include "battle.h"
#include "berry_crush.h"
#include "bg.h"
#include "cable_club.h"
#include "data.h"
#include "decompress.h"
#include "dodrio_berry_picking.h"
#include "dynamic_placeholder_text_util.h"
#include "easy_chat.h"
#include "event_data.h"
#include "event_object_lock.h"
#include "fieldmap.h"
#include "field_control_avatar.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_weather.h"
#include "international_string_util.h"
#include "librfu.h"
#include "link.h"
#include "link_rfu.h"
#include "list_menu.h"
#include "load_save.h"
#include "menu.h"
#include "menu_helpers.h"
#include "mystery_gift.h"
#include "mystery_gift_menu.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pokemon_jump.h"
#include "random.h"
#include "save_location.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "sound.h"
#include "start_menu.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "trade.h"
#include "trainer_card.h"
#include "union_room.h"
#include "union_room_battle.h"
#include "union_room_chat.h"
#include "union_room_player_avatar.h"
#include "window.h"
#include "constants/battle_frontier.h"
#include "constants/cable_club.h"
#include "constants/game_stat.h"
#include "constants/party_menu.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// States for Task_RunUnionRoom
enum {
    UR_STATE_INIT,
    UR_STATE_INIT_OBJECTS,
    UR_STATE_INIT_LINK,
    UR_STATE_CHECK_SELECTING_MON,
    UR_STATE_MAIN,
    UR_STATE_DO_SOMETHING_PROMPT,
    UR_STATE_HANDLE_DO_SOMETHING_PROMPT_INPUT,
    UR_STATE_DO_SOMETHING_PROMPT_2,
    UR_STATE_PRINT_MSG,
    UR_STATE_HANDLE_ACTIVITY_REQUEST,
    UR_STATE_DECLINE_ACTIVITY_REQUEST,
    UR_STATE_PLAYER_CONTACTED_YOU,
    UR_STATE_RECV_CONTACT_DATA,
    UR_STATE_PRINT_START_ACTIVITY_MSG,
    UR_STATE_START_ACTIVITY_LINK,
    UR_STATE_START_ACTIVITY_WAIT_FOR_LINK,
    UR_STATE_START_ACTIVITY_FREE_UROOM,
    UR_STATE_START_ACTIVITY_FADE,
    UR_STATE_START_ACTIVITY,
    UR_STATE_RECV_JOIN_CHAT_REQUEST,
    UR_STATE_TRY_ACCEPT_CHAT_REQUEST_DELAY,
    UR_STATE_TRY_ACCEPT_CHAT_REQUEST,
    UR_STATE_ACCEPT_CHAT_REQUEST,
    UR_STATE_WAIT_FOR_START_MENU,
    UR_STATE_INTERACT_WITH_PLAYER,
    UR_STATE_TRY_COMMUNICATING,
    UR_STATE_PRINT_AND_EXIT,
    UR_STATE_SEND_ACTIVITY_REQUEST,
    UR_STATE_TRAINER_APPEARS_BUSY,
    UR_STATE_WAIT_FOR_RESPONSE_TO_REQUEST,
    UR_STATE_CANCEL_ACTIVITY_LINK_ERROR,
    UR_STATE_SEND_TRADE_REQUST,
    UR_STATE_REQUEST_DECLINED,
    UR_STATE_PRINT_CONTACT_MSG,
    UR_STATE_HANDLE_CONTACT_DATA,
    UR_STATE_RECV_ACTIVITY_REQUEST,
    UR_STATE_CANCEL_REQUEST_PRINT_MSG,
    UR_STATE_CANCEL_REQUEST_RESTART_LINK,
    UR_STATE_COMMUNICATING_WAIT_FOR_DATA,
    UR_STATE_WAIT_FOR_CONTACT_DATA,
    UR_STATE_PRINT_CARD_INFO,
    UR_STATE_WAIT_FINISH_READING_CARD,
    UR_STATE_INTERACT_WITH_ATTENDANT,
    UR_STATE_REGISTER_PROMPT,
    UR_STATE_CANCEL_REGISTRATION_PROMPT,
    UR_STATE_CHECK_TRADING_BOARD,
    UR_STATE_TRADING_BOARD_LOAD,
    UR_STATE_REGISTER_PROMPT_HANDLE_INPUT,
    UR_STATE_TRADING_BOARD_HANDLE_INPUT,
    UR_STATE_TRADE_PROMPT,
    UR_STATE_TRADE_SELECT_MON,
    UR_STATE_TRADE_OFFER_MON,
    UR_STATE_REGISTER_REQUEST_TYPE,
    UR_STATE_REGISTER_SELECT_MON_FADE,
    UR_STATE_REGISTER_SELECT_MON,
    UR_STATE_REGISTER_COMPLETE,
    UR_STATE_CANCEL_REGISTRATION,
};

// States for sUnionRoomTrade.state
enum {
    URTRADE_STATE_NONE,
    URTRADE_STATE_REGISTERING,
    URTRADE_STATE_OFFERING,
};

// States for Task_TryBecomeLinkLeader
enum {
    LL_STATE_INIT,
    LL_STATE_INIT2 = 3,
    LL_STATE_GET_AWAITING_PLAYERS_TEXT,
    LL_STATE_PRINT_AWAITING_PLAYERS,
    LL_STATE_AWAIT_PLAYERS,
    LL_STATE_ACCEPT_NEW_MEMBER_PROMPT,
    LL_STATE_WAIT_DISCONNECT_CHILD = 9,
    LL_STATE_MEMBER_LEFT,
    LL_STATE_ACCEPT_NEW_MEMBER_PROMPT_HANDLE_INPUT,
    LL_STATE_UPDATE_AFTER_JOIN_REQUEST,
    LL_STATE_ACCEPTED_FINAL_MEMBER,
    LL_STATE_WAIT_AND_CONFIRM_MEMBERS,
    LL_STATE_MEMBERS_OK_PROMPT,
    LL_STATE_MEMBERS_OK_PROMPT_HANDLE_INPUT,
    LL_STATE_CONFIRMED_MEMBERS,
    LL_STATE_FINAL_MEMBER_CHECK,
    LL_STATE_CANCEL_PROMPT,
    LL_STATE_CANCEL_PROMPT_HANDLE_INPUT,
    LL_STATE_SHUTDOWN_AND_RETRY,
    LL_STATE_RETRY,
    LL_STATE_SHUTDOWN_AND_FAIL,
    LL_STATE_FAILED,
    LL_STATE_TRY_START_ACTIVITY = 26,
    LL_STATE_MEMBER_DISCONNECTED = 29,
    LL_STATE_CANCEL_WITH_MSG
};

// States for Task_TryJoinLinkGroup
enum {
    LG_STATE_INIT,
    LG_STATE_CHOOSE_LEADER_MSG,
    LG_STATE_INIT_WINDOWS,
    LG_STATE_CHOOSE_LEADER_HANDLE_INPUT,
    LG_STATE_ASK_JOIN_GROUP = 5,
    LG_STATE_MAIN,
    LG_STATE_ASK_LEAVE_GROUP,
    LG_STATE_ASK_LEAVE_GROUP_HANDLE_INPUT,
    LG_STATE_WAIT_LEAVE_GROUP,
    LG_STATE_CANCEL_CHOOSE_LEADER,
    LG_STATE_CANCELED,
    LG_STATE_RFU_ERROR,
    LG_STATE_RFU_ERROR_SHUTDOWN,
    LG_STATE_DISCONNECTED,
    LG_STATE_RETRY_CONNECTION,
    LG_STATE_TRADE_NOT_READY = 18,
    LG_STATE_TRADE_NOT_READY_RETRY,
    LG_STATE_READY_START_ACTIVITY,
    LG_STATE_START_ACTIVITY,
    LG_STATE_SHUTDOWN = 23,
};

// Color types for PrintUnionRoomText
enum {
    UR_COLOR_DEFAULT,
    UR_COLOR_RED,
    UR_COLOR_GREEN,
    UR_COLOR_WHITE,
    UR_COLOR_CANCEL,
    UR_COLOR_TRADE_BOARD_SELF,
    UR_COLOR_TRADE_BOARD_OTHER,
};

// Return values for HandlePlayerListUpdate
enum {
    PLIST_NONE,
    PLIST_NEW_PLAYER,
    PLIST_RECENT_UPDATE,
    PLIST_UNUSED,
    PLIST_CONTACTED,
};

static EWRAM_DATA u8 sUnionRoomPlayerName[12] = {};
EWRAM_DATA u8 gPlayerCurrActivity = 0;
static EWRAM_DATA u8 sPlayerActivityGroupSize = 0;
static EWRAM_DATA union
{
    struct WirelessLink_Leader *leader;
    struct WirelessLink_Group *group;
    struct WirelessLink_URoom *uRoom;
} sWirelessLinkMain = {};
static EWRAM_DATA u32 sUnused = 0;
EWRAM_DATA struct RfuGameCompatibilityData gRfuPartnerCompatibilityData = {};
EWRAM_DATA u16 gUnionRoomOfferedSpecies = 0;
EWRAM_DATA u8 gUnionRoomRequestedMonType = 0;
static EWRAM_DATA struct UnionRoomTrade sUnionRoomTrade = {};

static struct WirelessLink_Leader *sLeader;
static struct WirelessLink_Group *sGroup;
static struct WirelessLink_URoom *sURoom;

static void PrintUnionRoomText(u8, u8, const u8 *, u8, u8, u8);
static u16 ReadAsU16(const u8 *);
static void Task_TryBecomeLinkLeader(u8);
static void Task_TryJoinLinkGroup(u8);
static void Task_ListenToWireless(u8);
static void Task_SendMysteryGift(u8);
static void Task_CardOrNewsWithFriend(u8);
static void Task_CardOrNewsOverWireless(u8);
static void Task_RunUnionRoom(u8);
static void ClearIncomingPlayerList(struct RfuIncomingPlayerList *, u8);
static void ClearRfuPlayerList(struct RfuPlayer *, u8);
static u8 CreateTask_ListenForCompatiblePartners(struct RfuIncomingPlayerList *, u32);
static u8 CreateTask_ListenForWonderDistributor(struct RfuIncomingPlayerList *, u32 );
static bool8 PrintOnTextbox(u8 *, const u8 *);
static bool8 Leader_SetStateIfMemberListChanged(struct WirelessLink_Leader *, u32, u32);
static u8 LeaderPrunePlayerList(struct RfuPlayerList *);
static s8 UnionRoomHandleYesNo(u8 *, bool32);
static void CopyAndTranslatePlayerName(u8 *, struct RfuPlayer *);
static void Leader_DestroyResources(struct WirelessLink_Leader *);
static void CreateTask_RunScriptAndFadeToActivity(void);
static u8 LeaderUpdateGroupMembership(struct RfuPlayerList *);
static void PrintGroupCandidateOnWindow(u8, u8, u8, struct RfuPlayer *, u8, u8 );
static u32 GetNewIncomingPlayerId(struct RfuPlayer *, struct RfuIncomingPlayer *);
static u8 TryAddIncomingPlayerToList(struct RfuPlayer *, struct RfuIncomingPlayer *, u8);
static u8 GetNewLeaderCandidate(void);
static u32 IsTryingToTradeAcrossVersionTooSoon(struct WirelessLink_Group *, s32);
static void AskToJoinRfuGroup(struct WirelessLink_Group *, s32);
static void JoinGroup_EnableScriptContexts(void);
static void PrintGroupMemberOnWindow(u8, u8, u8, struct RfuPlayer *, u8, u8);
static bool32 ArePlayerDataDifferent(struct RfuPlayerData *, struct RfuPlayerData *);
static u32 GetPartyPositionOfRegisteredMon(struct UnionRoomTrade *, u8);
static void ResetUnionRoomTrade(struct UnionRoomTrade *);
static void CreateTask_StartActivity(void);
static bool32 HasWonderCardOrNewsByLinkGroup(struct RfuGameData *, s16);
static u8 CreateTask_SearchForChildOrParent(struct RfuIncomingPlayerList *, struct RfuIncomingPlayerList *, u32);
static bool32 RegisterTradeMonAndGetIsEgg(u32, struct UnionRoomTrade *);
static void RegisterTradeMon(u32, struct UnionRoomTrade *);
static void StartScriptInteraction(void);
static bool32 IsPlayerFacingTradingBoard(void);
static u8 HandlePlayerListUpdate(void);
static bool32 PollPartnerYesNoResponse(struct WirelessLink_URoom *);
static void ReceiveUnionRoomActivityPacket(struct WirelessLink_URoom *);
static u8 GetActivePartnersInfo(struct WirelessLink_URoom *);
static bool32 HandleContactFromOtherPlayer(struct WirelessLink_URoom *);
static bool32 UR_RunTextPrinters(void);
static s32 GetUnionRoomPlayerGender(s32, struct RfuPlayerList *);
static s32 UnionRoomGetPlayerInteractionResponse(struct RfuPlayerList *, u8, u8, u32);
static void HandleCancelActivity(bool32);
static s32 ListMenuHandler_AllItemsAvailable(u8 *, u8 *, u8 *, const struct WindowTemplate *, const struct ListMenuTemplate *);
static s32 TradeBoardMenuHandler(u8 *, u8 *, u8 *, u8 *, const struct WindowTemplate *, const struct ListMenuTemplate *, struct RfuPlayerList *);
static s32 GetIndexOfNthTradeBoardOffer(struct RfuPlayer *, s32);
static bool32 HasAtLeastTwoMonsOfLevel30OrLower(void);
static u32 GetResponseIdx_InviteToURoomActivity(s32);
static void ViewURoomPartnerTrainerCard(u8 *, struct WirelessLink_URoom *, bool8);
static void GetURoomActivityRejectMsg(u8 *, s32, u32);
static u32 ConvPartnerUnameAndGetWhetherMetAlready(struct RfuPlayer *);
static void GetURoomActivityStartMsg(u8 *, u8);
static void UR_ClearBg0(void);
static s32 IsRequestedTradeInPlayerParty(u32, u32);
static bool32 UR_PrintFieldMessage(const u8 *);
static s32 GetChatLeaderActionRequestMessage(u8 *, u32, u16 *, struct WirelessLink_URoom *);
static void Task_InitUnionRoom(u8 taskId);
static bool8 ArePlayersDifferent(struct RfuPlayerData *, const struct RfuPlayerData *);
static void ItemPrintFunc_PossibleGroupMembers(u8, u32, u8);
static void ListMenuItemPrintFunc_UnionRoomGroups(u8, u32, u8);
static void TradeBoardListMenuItemPrintFunc(u8, u32, u8);
static void ItemPrintFunc_EmptyList(u8, u32, u8);

#include "data/union_room.h"

static void PrintNumPlayersWaitingForMsg(u8 windowId, u8 capacityCode, u8 stringId)
{
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    switch (capacityCode << 8)
    {
    case LINK_GROUP_CAPACITY(0, 2):
        PrintUnionRoomText(windowId, FONT_NORMAL, sPlayersNeededOrModeTexts[0][stringId - 1], 0, 1, UR_COLOR_DEFAULT);
        break;
    case LINK_GROUP_CAPACITY(0, 4):
        PrintUnionRoomText(windowId, FONT_NORMAL, sPlayersNeededOrModeTexts[1][stringId - 1], 0, 1, UR_COLOR_DEFAULT);
        break;
    case LINK_GROUP_CAPACITY(2, 5):
        PrintUnionRoomText(windowId, FONT_NORMAL, sPlayersNeededOrModeTexts[2][stringId - 1], 0, 1, UR_COLOR_DEFAULT);
        break;
    case LINK_GROUP_CAPACITY(3, 5):
        PrintUnionRoomText(windowId, FONT_NORMAL, sPlayersNeededOrModeTexts[3][stringId - 1], 0, 1, UR_COLOR_DEFAULT);
        break;
    case LINK_GROUP_CAPACITY(2, 4):
        PrintUnionRoomText(windowId, FONT_NORMAL, sPlayersNeededOrModeTexts[4][stringId - 1], 0, 1, UR_COLOR_DEFAULT);
        break;
    }

    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void PrintPlayerNameAndIdOnWindow(u8 windowId)
{
    u8 text[30];
    u8 *txtPtr;

    PrintUnionRoomText(windowId, FONT_NORMAL, gSaveBlock2Ptr->playerName, 0, 1, UR_COLOR_DEFAULT);
    txtPtr = StringCopy(text, sText_ID);
    ConvertIntToDecimalStringN(txtPtr, ReadAsU16(gSaveBlock2Ptr->playerTrainerId), STR_CONV_MODE_LEADING_ZEROS, 5);
    PrintUnionRoomText(windowId, FONT_NORMAL, text, 0, 17, UR_COLOR_DEFAULT);
}

static void GetAwaitingCommunicationText(u8 *dst, u8 activity)
{
    switch (activity)
    {
    case ACTIVITY_BATTLE_SINGLE:
    case ACTIVITY_BATTLE_DOUBLE:
    case ACTIVITY_BATTLE_MULTI:
    case ACTIVITY_TRADE:
    case ACTIVITY_POKEMON_JUMP:
    case ACTIVITY_BERRY_CRUSH:
    case ACTIVITY_BERRY_PICK:
    case ACTIVITY_BATTLE_TOWER:
    case ACTIVITY_BATTLE_TOWER_OPEN:
    case ACTIVITY_RECORD_CORNER:
    case ACTIVITY_BERRY_BLENDER:
    case ACTIVITY_WONDER_CARD:
    case ACTIVITY_WONDER_NEWS:
    case ACTIVITY_CONTEST_COOL:
    case ACTIVITY_CONTEST_BEAUTY:
    case ACTIVITY_CONTEST_CUTE:
    case ACTIVITY_CONTEST_SMART:
    case ACTIVITY_CONTEST_TOUGH:
        // BUG: argument *dst isn't used, instead it always prints to gStringVarBuffer
        // not an issue in practice since Gamefreak never used any other arguments here besides gStringVarBuffer
    #ifndef BUGFIX
        StringExpandPlaceholders(gStringVarBuffer, sText_AwaitingCommunication);
    #else
        StringExpandPlaceholders(dst, sText_AwaitingCommunication);
    #endif
        break;
    }
}

static bool32 IsActivityWithVariableGroupSize(u32 activity)
{
    switch (activity)
    {
    case ACTIVITY_POKEMON_JUMP:
    case ACTIVITY_BERRY_CRUSH:
    case ACTIVITY_BERRY_PICK:
    case ACTIVITY_RECORD_CORNER:
    case ACTIVITY_BERRY_BLENDER:
    case ACTIVITY_CONTEST_COOL:
    case ACTIVITY_CONTEST_BEAUTY:
    case ACTIVITY_CONTEST_CUTE:
    case ACTIVITY_CONTEST_SMART:
    case ACTIVITY_CONTEST_TOUGH:
        return TRUE;
    default:
        return FALSE;
    }
}

void TryBecomeLinkLeader(void)
{
    u8 taskId;
    struct WirelessLink_Leader *data;

    taskId = CreateTask(Task_TryBecomeLinkLeader, 0);
    sWirelessLinkMain.leader = data = (void *)(gTasks[taskId].data);
    sLeader = data;

    data->state = LL_STATE_INIT;
    data->textState = 0;
    gSpecialVar_Result = LINKUP_ONGOING;
}

static void Task_TryBecomeLinkLeader(u8 taskId)
{
    u32 id, val;
    struct WirelessLink_Leader *data = sWirelessLinkMain.leader;

    switch (data->state)
    {
    case LL_STATE_INIT:
        if (gSpecialVar_0x8004 == LINK_GROUP_BATTLE_TOWER && gSaveBlock2Ptr->frontier.lvlMode == FRONTIER_LVL_OPEN)
            gSpecialVar_0x8004++;
        gPlayerCurrActivity = sLinkGroupToActivityAndCapacity[gSpecialVar_0x8004];
        sPlayerActivityGroupSize = sLinkGroupToActivityAndCapacity[gSpecialVar_0x8004] >> 8;
        SetHostRfuGameData(gPlayerCurrActivity, 0, FALSE);
        SetWirelessCommType1();
        OpenLink();
        InitializeRfuLinkManager_LinkLeader(GROUP_MAX(sPlayerActivityGroupSize));
        data->state = LL_STATE_INIT2;
        break;
    case LL_STATE_INIT2:
        data->incomingPlayerList = AllocZeroed(RFU_CHILD_MAX * sizeof(struct RfuIncomingPlayer));
        data->playerList = AllocZeroed(MAX_RFU_PLAYERS * sizeof(struct RfuPlayer));
        data->playerListBackup = AllocZeroed(MAX_RFU_PLAYERS * sizeof(struct RfuPlayer));
        ClearIncomingPlayerList(data->incomingPlayerList, RFU_CHILD_MAX);
        ClearRfuPlayerList(data->playerList->players, MAX_RFU_PLAYERS);
        CopyHostRfuGameDataAndUsername(&data->playerList->players[0].rfu.data, data->playerList->players[0].rfu.name);
        data->playerList->players[0].timeoutCounter = 0;
        data->playerList->players[0].groupScheduledAnim = UNION_ROOM_SPAWN_IN;
        data->playerList->players[0].useRedText = FALSE;
        data->playerList->players[0].newPlayerCountdown = 0;
        data->listenTaskId = CreateTask_ListenForCompatiblePartners(data->incomingPlayerList, 0xFF);
        data->bButtonCancelWindowId = AddWindow(&sWindowTemplate_BButtonCancel);
        switch (GROUP_MAX(sPlayerActivityGroupSize))
        {
        case 2:
        case 3:
        case 4:
            data->listWindowId = AddWindow(&sWindowTemplate_PlayerList);
            break;
        case 5:
            data->listWindowId = AddWindow(&sWindowTemplate_5PlayerList);
            break;
        }
        data->nPlayerModeWindowId = AddWindow(&sWindowTemplate_NumPlayerMode);

        FillWindowPixelBuffer(data->bButtonCancelWindowId, PIXEL_FILL(2));
        PrintUnionRoomText(data->bButtonCancelWindowId, FONT_SMALL, sText_BButtonCancel, 8, 1, UR_COLOR_CANCEL);
        PutWindowTilemap(data->bButtonCancelWindowId);
        CopyWindowToVram(data->bButtonCancelWindowId, COPYWIN_GFX);

        DrawStdWindowFrame(data->listWindowId, FALSE);
        gMultiuseListMenuTemplate = sListMenuTemplate_PossibleGroupMembers;
        gMultiuseListMenuTemplate.windowId = data->listWindowId;
        data->listTaskId = ListMenuInit(&gMultiuseListMenuTemplate, 0, 0);

        DrawStdWindowFrame(data->nPlayerModeWindowId, FALSE);
        PutWindowTilemap(data->nPlayerModeWindowId);
        CopyWindowToVram(data->nPlayerModeWindowId, COPYWIN_GFX);

        CopyBgTilemapBufferToVram(0);
        data->playerCount = 1;
        data->state = LL_STATE_GET_AWAITING_PLAYERS_TEXT;
        break;
    case LL_STATE_GET_AWAITING_PLAYERS_TEXT:
        StringCopy(gStringVar1, sLinkGroupActivityNameTexts[gPlayerCurrActivity]);
        if (GROUP_MIN(sPlayerActivityGroupSize) != 0)
        {
            if (data->playerCount > GROUP_MIN(sPlayerActivityGroupSize) - 1 && GROUP_MAX(sPlayerActivityGroupSize) != 0)
                StringExpandPlaceholders(gStringVarBuffer, sText_AwaitingLinkPressStart);
            else
                StringExpandPlaceholders(gStringVarBuffer, sText_AwaitingCommunication);
        }
        else
        {
            GetAwaitingCommunicationText(gStringVarBuffer, gPlayerCurrActivity);
        }

        PrintNumPlayersWaitingForMsg(data->nPlayerModeWindowId, sPlayerActivityGroupSize, data->playerCount);
        data->state = LL_STATE_PRINT_AWAITING_PLAYERS;
        break;
    case LL_STATE_PRINT_AWAITING_PLAYERS:
        if (PrintOnTextbox(&data->textState, gStringVarBuffer))
            data->state = LL_STATE_AWAIT_PLAYERS;
        break;
    case LL_STATE_AWAIT_PLAYERS:
        Leader_SetStateIfMemberListChanged(data, LL_STATE_ACCEPT_NEW_MEMBER_PROMPT, LL_STATE_MEMBER_LEFT);
        if (JOY_NEW(B_BUTTON))
        {
            if (data->playerCount == 1)
                data->state = LL_STATE_SHUTDOWN_AND_FAIL;
            else if (GROUP_MIN2(sPlayerActivityGroupSize) != 0)
                data->state = LL_STATE_CANCEL_WITH_MSG;
            else
                data->state = LL_STATE_CANCEL_PROMPT;
        }
        if (GROUP_MIN(sPlayerActivityGroupSize) != 0
            && data->playerCount > GROUP_MIN(sPlayerActivityGroupSize) - 1
            && GROUP_MAX(sPlayerActivityGroupSize) != 0
            && IsRfuCommunicatingWithAllChildren()
            && JOY_NEW(START_BUTTON))
        {
            data->state = LL_STATE_MEMBERS_OK_PROMPT;
            LinkRfu_StopManagerAndFinalizeSlots();
        }
        if (data->state == LL_STATE_AWAIT_PLAYERS && RfuTryDisconnectLeavingChildren())
        {
            // At least 1 group member has left or is trying to leave
            data->state = LL_STATE_WAIT_DISCONNECT_CHILD;
        }
        break;
    case LL_STATE_WAIT_DISCONNECT_CHILD:
        // Resume after ensuring all members trying to leave have left
        if (!RfuTryDisconnectLeavingChildren())
        {
            data->state = LL_STATE_AWAIT_PLAYERS;
            data->playerCount = LeaderPrunePlayerList(data->playerList);
        }
        break;
    case LL_STATE_MEMBER_LEFT:
        // BUG: sPlayerActivityGroupSize was meant below, not gPlayerCurrActivity
        //      This will be false for all but ACTIVITY_BATTLE_DOUBLE and ACTIVITY_DECLINE
        //      All this changes is which of two texts gets printed
    #ifdef BUGFIX
        id = (GROUP_MAX(sPlayerActivityGroupSize) == 2) ? 0 : 1;
    #else
        id = (GROUP_MAX(gPlayerCurrActivity) == 2) ? 1 : 0;
    #endif
        if (PrintOnTextbox(&data->textState, sPlayerUnavailableTexts[id]))
        {
            data->playerCount = LeaderPrunePlayerList(data->playerList);
            RedrawListMenu(data->listTaskId);
            data->state = LL_STATE_GET_AWAITING_PLAYERS_TEXT;
        }
        break;
    case LL_STATE_MEMBER_DISCONNECTED:
        id = (GROUP_MAX(sPlayerActivityGroupSize) == 2) ? 0 : 1;
        if (PrintOnTextbox(&data->textState, sPlayerUnavailableTexts[id]))
        {
            data->state = LL_STATE_SHUTDOWN_AND_RETRY;
        }
        break;
    case LL_STATE_ACCEPT_NEW_MEMBER_PROMPT:
        if (PrintOnTextbox(&data->textState, gStringVarBuffer))
        {
            data->state = LL_STATE_ACCEPT_NEW_MEMBER_PROMPT_HANDLE_INPUT;
        }
        break;
    case LL_STATE_ACCEPT_NEW_MEMBER_PROMPT_HANDLE_INPUT:
        switch (UnionRoomHandleYesNo(&data->textState, HasTrainerLeftPartnersList(ReadAsU16(data->playerList->players[data->playerCount].rfu.data.compatibility.playerTrainerId), data->playerList->players[data->playerCount].rfu.name)))
        {
        case 0: // YES
            LoadWirelessStatusIndicatorSpriteGfx();
            CreateWirelessStatusIndicatorSprite(0, 0);
            data->joinRequestAnswer = RFU_STATUS_JOIN_GROUP_OK;
            SendRfuStatusToPartner(data->joinRequestAnswer, ReadAsU16(data->playerList->players[data->playerCount].rfu.data.compatibility.playerTrainerId), data->playerList->players[data->playerCount].rfu.name);
            data->state = LL_STATE_UPDATE_AFTER_JOIN_REQUEST;
            break;
        case 1: // NO
        case MENU_B_PRESSED:
            data->joinRequestAnswer = RFU_STATUS_JOIN_GROUP_NO;
            SendRfuStatusToPartner(data->joinRequestAnswer, ReadAsU16(data->playerList->players[data->playerCount].rfu.data.compatibility.playerTrainerId), data->playerList->players[data->playerCount].rfu.name);
            data->state = LL_STATE_UPDATE_AFTER_JOIN_REQUEST;
            break;
        case -3:
            data->state = LL_STATE_WAIT_DISCONNECT_CHILD;
            break;
        }
        break;
    case LL_STATE_UPDATE_AFTER_JOIN_REQUEST:
        val = WaitSendRfuStatusToPartner(ReadAsU16(data->playerList->players[data->playerCount].rfu.data.compatibility.playerTrainerId), data->playerList->players[data->playerCount].rfu.name);
        if (val == 1) // Send complete
        {
            if (data->joinRequestAnswer == RFU_STATUS_JOIN_GROUP_OK)
            {
                // Sent "OK"
                data->playerList->players[data->playerCount].newPlayerCountdown = 0;
                RedrawListMenu(data->listTaskId);
                data->playerCount++;
                if (data->playerCount == GROUP_MAX(sPlayerActivityGroupSize))
                {
                    if (GROUP_MIN2(sPlayerActivityGroupSize) != 0 || data->playerCount == RFU_CHILD_MAX)
                    {
                        data->state = LL_STATE_MEMBERS_OK_PROMPT;
                    }
                    else
                    {
                        CopyAndTranslatePlayerName(gStringVar1, &data->playerList->players[data->playerCount - 1]);
                        StringExpandPlaceholders(gStringVarBuffer, sText_AnOKWasSentToPlayer);
                        data->state = LL_STATE_ACCEPTED_FINAL_MEMBER;
                    }

                    LinkRfu_StopManagerAndFinalizeSlots();
                    PrintNumPlayersWaitingForMsg(data->nPlayerModeWindowId, sPlayerActivityGroupSize, data->playerCount);
                }
                else
                {
                    data->state = LL_STATE_GET_AWAITING_PLAYERS_TEXT;
                }
            }
            else // Member disconnected
            {
                RequestDisconnectSlotByTrainerNameAndId(data->playerList->players[data->playerCount].rfu.name, ReadAsU16(data->playerList->players[data->playerCount].rfu.data.compatibility.playerTrainerId));
                data->playerList->players[data->playerCount].groupScheduledAnim = UNION_ROOM_SPAWN_NONE;
                LeaderPrunePlayerList(data->playerList);
                RedrawListMenu(data->listTaskId);
                data->state = LL_STATE_GET_AWAITING_PLAYERS_TEXT;
            }

            data->joinRequestAnswer = 0;
        }
        else if (val == 2)
        {
            // Disconnect
            RfuSetStatus(RFU_STATUS_OK, 0);
            data->state = LL_STATE_GET_AWAITING_PLAYERS_TEXT;
        }
        break;
    case LL_STATE_ACCEPTED_FINAL_MEMBER:
        if (PrintOnTextbox(&data->textState, gStringVarBuffer))
            data->state = LL_STATE_WAIT_AND_CONFIRM_MEMBERS;
        break;
    case LL_STATE_WAIT_AND_CONFIRM_MEMBERS:
        if (++data->delayTimerAfterOk > 120)
            data->state = LL_STATE_CONFIRMED_MEMBERS;
        break;
    case LL_STATE_MEMBERS_OK_PROMPT:
        if (PrintOnTextbox(&data->textState, sText_AreTheseMembersOK))
            data->state = LL_STATE_MEMBERS_OK_PROMPT_HANDLE_INPUT;
        break;
    case LL_STATE_MEMBERS_OK_PROMPT_HANDLE_INPUT:
        switch (UnionRoomHandleYesNo(&data->textState, FALSE))
        {
        case 0: // YES
            data->state = LL_STATE_CONFIRMED_MEMBERS;
            break;
        case 1: // NO
        case MENU_B_PRESSED:
            if (GROUP_MIN2(sPlayerActivityGroupSize) != 0)
                data->state = LL_STATE_CANCEL_WITH_MSG;
            else
                data->state = LL_STATE_CANCEL_PROMPT;
            break;
        }
        break;
    case LL_STATE_CANCEL_PROMPT:
        if (PrintOnTextbox(&data->textState, sText_CancelModeWithTheseMembers))
            data->state = LL_STATE_CANCEL_PROMPT_HANDLE_INPUT;
        break;
    case LL_STATE_CANCEL_PROMPT_HANDLE_INPUT:
        switch (UnionRoomHandleYesNo(&data->textState, FALSE))
        {
        case 0: // YES
            data->state = LL_STATE_SHUTDOWN_AND_FAIL;
            break;
        case 1: // NO
        case MENU_B_PRESSED:
            if (GROUP_MIN2(sPlayerActivityGroupSize) != 0)
                data->state = LL_STATE_MEMBERS_OK_PROMPT;
            else if (data->playerCount == GROUP_MAX(sPlayerActivityGroupSize))
                data->state = LL_STATE_MEMBERS_OK_PROMPT;
            else
                data->state = LL_STATE_GET_AWAITING_PLAYERS_TEXT;
            break;
        }
        break;
    case LL_STATE_CONFIRMED_MEMBERS:
        if (!Leader_SetStateIfMemberListChanged(data, LL_STATE_ACCEPT_NEW_MEMBER_PROMPT, LL_STATE_MEMBER_DISCONNECTED))
            data->state = LL_STATE_FINAL_MEMBER_CHECK;
        break;
    case LL_STATE_FINAL_MEMBER_CHECK:
        if (LmanAcceptSlotFlagIsNotZero())
        {
            if (WaitRfuState(FALSE))
            {
                data->state = LL_STATE_TRY_START_ACTIVITY;
            }
            else
            {
                if (++data->memberConfirmTimeout > 300)
                {
                    data->state = LL_STATE_MEMBER_DISCONNECTED;
                    data->textState = 0;
                }
            }
        }
        else
        {
            data->state = LL_STATE_MEMBER_DISCONNECTED;
            data->textState = 0;
        }
        break;
    case LL_STATE_CANCEL_WITH_MSG:
        if (PrintOnTextbox(&data->textState, sText_ModeWithTheseMembersWillBeCanceled))
            data->state = LL_STATE_SHUTDOWN_AND_FAIL;
        break;
    case LL_STATE_SHUTDOWN_AND_RETRY:
    case LL_STATE_SHUTDOWN_AND_FAIL:
        DestroyWirelessStatusIndicatorSprite();
        LinkRfu_Shutdown();
        Leader_DestroyResources(data);
        data->state++; // LL_STATE_RETRY or LL_STATE_FAILED
        break;
    case LL_STATE_FAILED:
        ScriptContext_Enable();
        DestroyTask(taskId);
        gSpecialVar_Result = LINKUP_FAILED;
        break;
    case LL_STATE_RETRY:
        ScriptContext_Enable();
        DestroyTask(taskId);
        gSpecialVar_Result = LINKUP_RETRY_ROLE_ASSIGN;
        break;
    case LL_STATE_TRY_START_ACTIVITY:
        if (RfuHasErrored())
        {
            data->state = LL_STATE_MEMBER_DISCONNECTED;
        }
        else
        {
            if (gReceivedRemoteLinkPlayers)
            {
                if (IsActivityWithVariableGroupSize(gPlayerCurrActivity))
                    GetOtherPlayersInfoFlags();
                UpdateGameData_GroupLockedIn(TRUE);
                CreateTask_RunScriptAndFadeToActivity();
                Leader_DestroyResources(data);
                DestroyTask(taskId);
            }
        }
        break;
    }
}

static void Leader_DestroyResources(struct WirelessLink_Leader *data)
{
    ClearWindowTilemap(data->nPlayerModeWindowId);
    ClearStdWindowAndFrame(data->nPlayerModeWindowId, FALSE);
    DestroyListMenuTask(data->listTaskId, 0, 0);
    ClearWindowTilemap(data->bButtonCancelWindowId);
    ClearStdWindowAndFrame(data->listWindowId, FALSE);
    CopyBgTilemapBufferToVram(0);
    RemoveWindow(data->nPlayerModeWindowId);
    RemoveWindow(data->listWindowId);
    RemoveWindow(data->bButtonCancelWindowId);
    DestroyTask(data->listenTaskId);

    Free(data->playerListBackup);
    Free(data->playerList);
    Free(data->incomingPlayerList);
}

static void Leader_GetAcceptNewMemberPrompt(u8 *dst, u8 activity)
{
    switch (activity)
    {
    case ACTIVITY_BATTLE_SINGLE:
    case ACTIVITY_BATTLE_DOUBLE:
    case ACTIVITY_TRADE:
    case ACTIVITY_BATTLE_TOWER_OPEN:
    case ACTIVITY_BATTLE_TOWER:
        StringExpandPlaceholders(dst, sText_PlayerContactedYouForXAccept);
        break;
    case ACTIVITY_WONDER_CARD:
    case ACTIVITY_WONDER_NEWS:
        StringExpandPlaceholders(dst, sText_PlayerContactedYouShareX);
        break;
    case ACTIVITY_BATTLE_MULTI:
    case ACTIVITY_POKEMON_JUMP:
    case ACTIVITY_BERRY_CRUSH:
    case ACTIVITY_BERRY_PICK:
    case ACTIVITY_RECORD_CORNER:
    case ACTIVITY_BERRY_BLENDER:
    case ACTIVITY_CONTEST_COOL:
    case ACTIVITY_CONTEST_BEAUTY:
    case ACTIVITY_CONTEST_CUTE:
    case ACTIVITY_CONTEST_SMART:
    case ACTIVITY_CONTEST_TOUGH:
        StringExpandPlaceholders(dst, sText_PlayerContactedYouAddToMembers);
        break;
    }
}

static void GetYouDeclinedTheOfferMessage(u8 *dst, u8 activity)
{
    switch (activity)
    {
    case ACTIVITY_BATTLE_SINGLE | IN_UNION_ROOM:
    case ACTIVITY_TRADE | IN_UNION_ROOM:
        StringExpandPlaceholders(dst, sText_OfferDeclined1);
        break;
    case ACTIVITY_CHAT | IN_UNION_ROOM:
    case ACTIVITY_CARD | IN_UNION_ROOM:
        StringExpandPlaceholders(dst, sText_OfferDeclined2);
        break;
    }
}

static void GetYouAskedToJoinGroupPleaseWaitMessage(u8 *dst, u8 activity)
{
    switch (activity)
    {
    case ACTIVITY_BATTLE_SINGLE:
    case ACTIVITY_BATTLE_DOUBLE:
    case ACTIVITY_TRADE:
    case ACTIVITY_BATTLE_TOWER:
    case ACTIVITY_BATTLE_TOWER_OPEN:
    case ACTIVITY_WONDER_CARD:
    case ACTIVITY_WONDER_NEWS:
        StringExpandPlaceholders(dst, sText_AwaitingPlayersResponse);
        break;
    case ACTIVITY_BATTLE_MULTI:
    case ACTIVITY_POKEMON_JUMP:
    case ACTIVITY_BERRY_CRUSH:
    case ACTIVITY_BERRY_PICK:
    case ACTIVITY_RECORD_CORNER:
    case ACTIVITY_BERRY_BLENDER:
    case ACTIVITY_CONTEST_COOL:
    case ACTIVITY_CONTEST_BEAUTY:
    case ACTIVITY_CONTEST_CUTE:
    case ACTIVITY_CONTEST_SMART:
    case ACTIVITY_CONTEST_TOUGH:
        StringExpandPlaceholders(dst, sText_PlayerHasBeenAskedToRegisterYouPleaseWait);
        break;
    }
}

static void GetGroupLeaderSentAnOKMessage(u8 *dst, u8 activity)
{
    switch (activity)
    {
    case ACTIVITY_BATTLE_SINGLE:
    case ACTIVITY_BATTLE_DOUBLE:
    case ACTIVITY_TRADE:
    case ACTIVITY_BATTLE_TOWER:
    case ACTIVITY_BATTLE_TOWER_OPEN:
    case ACTIVITY_WONDER_CARD:
    case ACTIVITY_WONDER_NEWS:
        StringExpandPlaceholders(dst, sText_PlayerSentBackOK);
        break;
    case ACTIVITY_BATTLE_MULTI:
    case ACTIVITY_POKEMON_JUMP:
    case ACTIVITY_BERRY_CRUSH:
    case ACTIVITY_BERRY_PICK:
    case ACTIVITY_RECORD_CORNER:
    case ACTIVITY_BERRY_BLENDER:
    case ACTIVITY_CONTEST_COOL:
    case ACTIVITY_CONTEST_BEAUTY:
    case ACTIVITY_CONTEST_CUTE:
    case ACTIVITY_CONTEST_SMART:
    case ACTIVITY_CONTEST_TOUGH:
        StringExpandPlaceholders(dst, sText_PlayerOKdRegistration);
        break;
    }
}

static bool8 Leader_SetStateIfMemberListChanged(struct WirelessLink_Leader *data, u32 joinedState, u32 droppedState)
{
    switch (LeaderUpdateGroupMembership(data->playerList))
    {
    case UNION_ROOM_SPAWN_IN:
        PlaySE(SE_PC_LOGIN);
        RedrawListMenu(data->listTaskId);
        CopyAndTranslatePlayerName(gStringVar2, &data->playerList->players[data->playerCount]);
        Leader_GetAcceptNewMemberPrompt(gStringVarBuffer, gPlayerCurrActivity);
        data->state = joinedState;
        break;
    case UNION_ROOM_SPAWN_OUT:
        RfuSetStatus(RFU_STATUS_OK, 0);
        RedrawListMenu(data->listTaskId);
        data->state = droppedState;
        return TRUE;
    }

    return FALSE;
}

static void ItemPrintFunc_PossibleGroupMembers(u8 windowId, u32 id, u8 y)
{
    struct WirelessLink_Leader *data = sWirelessLinkMain.leader;
    u8 colorIdx = UR_COLOR_DEFAULT;

    switch (data->playerList->players[id].groupScheduledAnim)
    {
    case UNION_ROOM_SPAWN_IN:
        if (data->playerList->players[id].newPlayerCountdown != 0)
            colorIdx = UR_COLOR_GREEN;
        break;
    case UNION_ROOM_SPAWN_OUT:
        colorIdx = UR_COLOR_RED;
        break;
    }

    PrintGroupCandidateOnWindow(windowId, 0, y, &data->playerList->players[id], colorIdx, id);
}

static u8 LeaderUpdateGroupMembership(struct RfuPlayerList *list)
{
    struct WirelessLink_Leader *data = sWirelessLinkMain.leader;
    u8 ret = UNION_ROOM_SPAWN_NONE;
    u8 i;
    s32 id;

    for (i = 1; i < MAX_RFU_PLAYERS; i++)
    {
        u16 var = data->playerList->players[i].groupScheduledAnim;
        if (var == UNION_ROOM_SPAWN_IN)
        {
            id = GetNewIncomingPlayerId(&data->playerList->players[i], data->incomingPlayerList->players);
            if (id != 0xFF)
            {
                // New incoming player
                data->playerList->players[i].rfu = data->incomingPlayerList->players[id].rfu;
                data->playerList->players[i].timeoutCounter = 1;
            }
            else
            {
                // No new incoming player
                data->playerList->players[i].groupScheduledAnim = UNION_ROOM_SPAWN_OUT;
                ret = UNION_ROOM_SPAWN_OUT;
            }
        }
    }

    for (id = 0; id < RFU_CHILD_MAX; id++)
        TryAddIncomingPlayerToList(data->playerList->players, &data->incomingPlayerList->players[id], MAX_RFU_PLAYERS);

    if (ret != UNION_ROOM_SPAWN_OUT)
    {
        for (id = 0; id < MAX_RFU_PLAYERS; id++)
        {
            if (data->playerList->players[id].newPlayerCountdown != 0)
                ret = UNION_ROOM_SPAWN_IN;
        }
    }

    return ret;
}

static u8 LeaderPrunePlayerList(struct RfuPlayerList *list)
{
    struct WirelessLink_Leader *data = sWirelessLinkMain.leader;
    u8 copiedCount;
    s32 i;
    u8 playerCount;

    for (i = 0; i < MAX_RFU_PLAYERS; i++)
        data->playerListBackup->players[i] = data->playerList->players[i];

    copiedCount = 0;
    for (i = 0; i < MAX_RFU_PLAYERS; i++)
    {
        if (data->playerListBackup->players[i].groupScheduledAnim == UNION_ROOM_SPAWN_IN)
        {
            data->playerList->players[copiedCount] = data->playerListBackup->players[i];
            copiedCount++;
        }
    }

    playerCount = copiedCount;
    for (; copiedCount < MAX_RFU_PLAYERS; copiedCount++)
    {
        data->playerList->players[copiedCount].rfu = sUnionRoomPlayer_DummyRfu;
        data->playerList->players[copiedCount].timeoutCounter = 0;
        data->playerList->players[copiedCount].groupScheduledAnim = UNION_ROOM_SPAWN_NONE;
        data->playerList->players[copiedCount].useRedText = FALSE;
        data->playerList->players[copiedCount].newPlayerCountdown = 0;
    }

    for (i = 0; i < MAX_RFU_PLAYERS; i++)
    {
        if (data->playerList->players[i].groupScheduledAnim != UNION_ROOM_SPAWN_IN)
            continue;
        if (data->playerList->players[i].newPlayerCountdown != 64)
            continue;

        playerCount = i;
        break;
    }

    return playerCount;
}

void TryJoinLinkGroup(void)
{
    u8 taskId;
    struct WirelessLink_Group *data;

    taskId = CreateTask(Task_TryJoinLinkGroup, 0);
    sWirelessLinkMain.group = data = (void *)(gTasks[taskId].data);
    sGroup = data;

    data->state = LG_STATE_INIT;
    data->textState = 0;
    gSpecialVar_Result = LINKUP_ONGOING;
}

static void Task_TryJoinLinkGroup(u8 taskId)
{
    s32 id;
    struct WirelessLink_Group *data = sWirelessLinkMain.group;

    switch (data->state)
    {
    case LG_STATE_INIT:
        if (gSpecialVar_0x8004 == LINK_GROUP_BATTLE_TOWER && gSaveBlock2Ptr->frontier.lvlMode == FRONTIER_LVL_OPEN)
            gSpecialVar_0x8004++;
        gPlayerCurrActivity = sLinkGroupToURoomActivity[gSpecialVar_0x8004];
        SetHostRfuGameData(gPlayerCurrActivity, 0, FALSE);
        SetWirelessCommType1();
        OpenLink();
        InitializeRfuLinkManager_JoinGroup();
        data->incomingPlayerList = AllocZeroed(RFU_CHILD_MAX * sizeof(struct RfuIncomingPlayer));
        data->playerList = AllocZeroed(MAX_RFU_PLAYER_LIST_SIZE * sizeof(struct RfuPlayer));
        data->state = LG_STATE_CHOOSE_LEADER_MSG;
        break;
    case LG_STATE_CHOOSE_LEADER_MSG:
        if (PrintOnTextbox(&data->textState, sChooseTrainerTexts[gSpecialVar_0x8004]))
            data->state = LG_STATE_INIT_WINDOWS;
        break;
    case LG_STATE_INIT_WINDOWS:
        ClearIncomingPlayerList(data->incomingPlayerList, RFU_CHILD_MAX);
        ClearRfuPlayerList(data->playerList->players, MAX_RFU_PLAYER_LIST_SIZE);
        data->listenTaskId = CreateTask_ListenForCompatiblePartners(data->incomingPlayerList, gSpecialVar_0x8004);
        data->bButtonCancelWindowId = AddWindow(&sWindowTemplate_BButtonCancel);
        data->listWindowId = AddWindow(&sWindowTemplate_GroupList);
        data->playerNameAndIdWindowId = AddWindow(&sWindowTemplate_PlayerNameAndId);

        FillWindowPixelBuffer(data->bButtonCancelWindowId, PIXEL_FILL(2));
        PrintUnionRoomText(data->bButtonCancelWindowId, FONT_SMALL, sText_ChooseJoinCancel, 8, 1, UR_COLOR_CANCEL);
        PutWindowTilemap(data->bButtonCancelWindowId);
        CopyWindowToVram(data->bButtonCancelWindowId, COPYWIN_GFX);

        DrawStdWindowFrame(data->listWindowId, FALSE);
        gMultiuseListMenuTemplate = sListMenuTemplate_UnionRoomGroups;
        gMultiuseListMenuTemplate.windowId = data->listWindowId;
        data->listTaskId = ListMenuInit(&gMultiuseListMenuTemplate, 0, 0);

        DrawStdWindowFrame(data->playerNameAndIdWindowId, FALSE);
        PutWindowTilemap(data->playerNameAndIdWindowId);
        PrintPlayerNameAndIdOnWindow(data->playerNameAndIdWindowId);
        CopyWindowToVram(data->playerNameAndIdWindowId, COPYWIN_GFX);

        CopyBgTilemapBufferToVram(0);
        data->leaderId = 0;
        data->state = LG_STATE_CHOOSE_LEADER_HANDLE_INPUT;
        break;
    case LG_STATE_CHOOSE_LEADER_HANDLE_INPUT:
        id = GetNewLeaderCandidate();
        switch (id)
        {
        case 1:
            PlaySE(SE_PC_LOGIN);
            RedrawListMenu(data->listTaskId);
            break;
        case 0:
            id = ListMenu_ProcessInput(data->listTaskId);
            if (JOY_NEW(A_BUTTON) && id != LIST_NOTHING_CHOSEN)
            {
                // Needed to match
                u32 UNUSED activity = data->playerList->players[id].rfu.data.activity;

                if (data->playerList->players[id].groupScheduledAnim == UNION_ROOM_SPAWN_IN && !data->playerList->players[id].rfu.data.startedActivity)
                {
                    u32 readyStatus = IsTryingToTradeAcrossVersionTooSoon(data, id);
                    if (readyStatus == UR_TRADE_READY)
                    {
                        // Trading is allowed, or not trading at all
                        AskToJoinRfuGroup(data, id);
                        data->state = LG_STATE_ASK_JOIN_GROUP;
                        PlaySE(SE_POKENAV_ON);
                    }
                    else
                    {
                        StringCopy(gStringVarBuffer, sCantTransmitToTrainerTexts[readyStatus - 1]);
                        data->state = LG_STATE_TRADE_NOT_READY;
                        PlaySE(SE_POKENAV_ON);
                    }
                }
                else
                {
                    PlaySE(SE_WALL_HIT);
                }
            }
            else if (JOY_NEW(B_BUTTON))
            {
                data->state = LG_STATE_CANCEL_CHOOSE_LEADER;
            }
            break;
        default:
            RedrawListMenu(data->listTaskId);
            break;
        }
        break;
    case LG_STATE_ASK_JOIN_GROUP:
        GetYouAskedToJoinGroupPleaseWaitMessage(gStringVarBuffer, gPlayerCurrActivity);
        if (PrintOnTextbox(&data->textState, gStringVarBuffer))
        {
            CopyAndTranslatePlayerName(gStringVar1, &data->playerList->players[data->leaderId]);
            data->state = LG_STATE_MAIN;
        }
        break;
    case LG_STATE_MAIN:
        if (gReceivedRemoteLinkPlayers)
        {
            gPlayerCurrActivity = data->playerList->players[data->leaderId].rfu.data.activity;
            RfuSetStatus(RFU_STATUS_OK, 0);
            switch (gPlayerCurrActivity)
            {
            case ACTIVITY_BATTLE_SINGLE:
            case ACTIVITY_BATTLE_DOUBLE:
            case ACTIVITY_BATTLE_MULTI:
            case ACTIVITY_TRADE:
            case ACTIVITY_CHAT:
            case ACTIVITY_POKEMON_JUMP:
            case ACTIVITY_BERRY_CRUSH:
            case ACTIVITY_BERRY_PICK:
            case ACTIVITY_SPIN_TRADE:
            case ACTIVITY_BATTLE_TOWER:
            case ACTIVITY_BATTLE_TOWER_OPEN:
            case ACTIVITY_RECORD_CORNER:
            case ACTIVITY_BERRY_BLENDER:
            case ACTIVITY_WONDER_CARD:
            case ACTIVITY_WONDER_NEWS:
            case ACTIVITY_CONTEST_COOL:
            case ACTIVITY_CONTEST_BEAUTY:
            case ACTIVITY_CONTEST_CUTE:
            case ACTIVITY_CONTEST_SMART:
            case ACTIVITY_CONTEST_TOUGH:
                data->state = LG_STATE_READY_START_ACTIVITY;
                return;
            }
        }

        switch (RfuGetStatus())
        {
        case RFU_STATUS_FATAL_ERROR:
            data->state = LG_STATE_RFU_ERROR;
            break;
        case RFU_STATUS_CONNECTION_ERROR:
        case RFU_STATUS_JOIN_GROUP_NO:
        case RFU_STATUS_LEAVE_GROUP:
            data->state = LG_STATE_DISCONNECTED;
            break;
        case RFU_STATUS_JOIN_GROUP_OK:
            GetGroupLeaderSentAnOKMessage(gStringVarBuffer, gPlayerCurrActivity);
            if (PrintOnTextbox(&data->textState, gStringVarBuffer))
            {
                if (gPlayerCurrActivity == ACTIVITY_BATTLE_TOWER || gPlayerCurrActivity == ACTIVITY_BATTLE_TOWER_OPEN)
                {
                    RfuSetStatus(RFU_STATUS_ACK_JOIN_GROUP, 0);
                }
                else
                {
                    RfuSetStatus(RFU_STATUS_WAIT_ACK_JOIN_GROUP, 0);
                    StringCopy(gStringVar1, sLinkGroupActivityNameTexts[gPlayerCurrActivity]);
                    StringExpandPlaceholders(gStringVarBuffer, sText_AwaitingOtherMembers);
                }
            }
            break;
        case RFU_STATUS_WAIT_ACK_JOIN_GROUP:
            if (data->delayBeforePrint > 240)
            {
                if (PrintOnTextbox(&data->textState, gStringVarBuffer))
                {
                    RfuSetStatus(RFU_STATUS_ACK_JOIN_GROUP, 0);
                    data->delayBeforePrint = 0;
                }
            }
            else
            {
                switch (gPlayerCurrActivity)
                {
                case ACTIVITY_BATTLE_SINGLE:
                case ACTIVITY_BATTLE_DOUBLE:
                case ACTIVITY_TRADE:
                case ACTIVITY_BATTLE_TOWER:
                case ACTIVITY_BATTLE_TOWER_OPEN:
                    break;
                default:
                    data->delayBeforePrint++;
                    break;
                }
            }
            break;
        }

        if (RfuGetStatus() == RFU_STATUS_OK && JOY_NEW(B_BUTTON))
            data->state = LG_STATE_ASK_LEAVE_GROUP;
        break;
    case LG_STATE_ASK_LEAVE_GROUP:
        if (PrintOnTextbox(&data->textState, sText_QuitBeingMember))
            data->state = LG_STATE_ASK_LEAVE_GROUP_HANDLE_INPUT;
        break;
    case LG_STATE_ASK_LEAVE_GROUP_HANDLE_INPUT:
        switch (UnionRoomHandleYesNo(&data->textState, RfuGetStatus()))
        {
        case 0: // YES
            SendLeaveGroupNotice();
            data->state = LG_STATE_WAIT_LEAVE_GROUP;
            RedrawListMenu(data->listTaskId);
            break;
        case 1: // NO
        case MENU_B_PRESSED:
            data->state = LG_STATE_ASK_JOIN_GROUP;
            RedrawListMenu(data->listTaskId);
            break;
        case -3:
            data->state = LG_STATE_MAIN;
            RedrawListMenu(data->listTaskId);
            break;
        }
        break;
    case LG_STATE_WAIT_LEAVE_GROUP:
        if (RfuGetStatus())
            data->state = LG_STATE_MAIN;
        break;
    case LG_STATE_CANCEL_CHOOSE_LEADER: // next: LG_STATE_CANCELED
    case LG_STATE_RFU_ERROR:            // next: LG_STATE_RFU_ERROR_SHUTDOWN
    case LG_STATE_DISCONNECTED:         // next: LG_STATE_RETRY_CONNECTION
    case LG_STATE_TRADE_NOT_READY:      // next: LG_STATE_TRADE_NOT_READY_RETRY
    case LG_STATE_READY_START_ACTIVITY: // next: LG_STATE_START_ACTIVITY
        ClearWindowTilemap(data->playerNameAndIdWindowId);
        ClearStdWindowAndFrame(data->playerNameAndIdWindowId, FALSE);
        DestroyListMenuTask(data->listTaskId, 0, 0);
        ClearWindowTilemap(data->bButtonCancelWindowId);
        ClearStdWindowAndFrame(data->listWindowId, FALSE);
        CopyBgTilemapBufferToVram(0);
        RemoveWindow(data->playerNameAndIdWindowId);
        RemoveWindow(data->listWindowId);
        RemoveWindow(data->bButtonCancelWindowId);
        DestroyTask(data->listenTaskId);
        Free(data->playerList);
        Free(data->incomingPlayerList);
        data->state++;
        break;
    case LG_STATE_RFU_ERROR_SHUTDOWN:
        DestroyWirelessStatusIndicatorSprite();
        if (PrintOnTextbox(&data->textState, sPlayerDisconnectedTexts[RfuGetStatus()]))
        {
            gSpecialVar_Result = LINKUP_CONNECTION_ERROR;
            data->state = LG_STATE_SHUTDOWN;
        }
        break;
    case LG_STATE_CANCELED:
        DestroyWirelessStatusIndicatorSprite();
        gSpecialVar_Result = LINKUP_FAILED;
        data->state = LG_STATE_SHUTDOWN;
        break;
    case LG_STATE_RETRY_CONNECTION:
        // Failure from disconnection
        // Happens if player or required member(s) leave group
        // or if player is rejected from joining group
        DestroyWirelessStatusIndicatorSprite();
        if (PrintOnTextbox(&data->textState, sPlayerDisconnectedTexts[RfuGetStatus()]))
        {
            gSpecialVar_Result = LINKUP_RETRY_ROLE_ASSIGN;
            data->state = LG_STATE_SHUTDOWN;
        }
        break;
    case LG_STATE_TRADE_NOT_READY_RETRY:
        if (PrintOnTextbox(&data->textState, gStringVarBuffer))
        {
            gSpecialVar_Result = LINKUP_RETRY_ROLE_ASSIGN;
            data->state = LG_STATE_SHUTDOWN;
        }
        break;
    case LG_STATE_SHUTDOWN:
        DestroyTask(taskId);
        JoinGroup_EnableScriptContexts();
        LinkRfu_Shutdown();
        break;
    case LG_STATE_START_ACTIVITY:
        CreateTask_RunScriptAndFadeToActivity();
        DestroyTask(taskId);
        break;
    }
}

static u32 IsTryingToTradeAcrossVersionTooSoon(struct WirelessLink_Group *data, s32 id)
{
    struct RfuPlayer *partner = &data->playerList->players[id];

    if (gPlayerCurrActivity == ACTIVITY_TRADE && partner->rfu.data.compatibility.version != VERSION_EMERALD)
    {
        if (!(gSaveBlock2Ptr->specialSaveWarpFlags & CHAMPION_SAVEWARP))
            return UR_TRADE_PLAYER_NOT_READY;
        else if (partner->rfu.data.compatibility.canLinkNationally)
            return UR_TRADE_READY;
    }
    else
    {
        return UR_TRADE_READY;
    }

    return UR_TRADE_PARTNER_NOT_READY;
}

static void AskToJoinRfuGroup(struct WirelessLink_Group *data, s32 id)
{
    data->leaderId = id;
    LoadWirelessStatusIndicatorSpriteGfx();
    CreateWirelessStatusIndicatorSprite(0, 0);
    RedrawListMenu(data->listTaskId);
    CopyAndTranslatePlayerName(gStringVar1, &data->playerList->players[data->leaderId]);
    UpdateGameData_SetActivity(sLinkGroupToURoomActivity[gSpecialVar_0x8004], 0, TRUE);
    CreateTask_RfuReconnectWithParent(data->playerList->players[data->leaderId].rfu.name, ReadAsU16(data->playerList->players[data->leaderId].rfu.data.compatibility.playerTrainerId));
}

u8 CreateTask_ListenToWireless(void)
{
    u8 taskId;
    struct WirelessLink_Group *data;

    taskId = CreateTask(Task_ListenToWireless, 0);
    sWirelessLinkMain.group = data = (void *)(gTasks[taskId].data);

    data->state = 0;
    data->textState = 0;

    sGroup = data;

    return taskId;
}

static void Task_ListenToWireless(u8 taskId)
{
    struct WirelessLink_Group *data = sWirelessLinkMain.group;

    switch (data->state)
    {
    case 0:
        SetHostRfuGameData(ACTIVITY_NONE, 0, FALSE);
        SetWirelessCommType1();
        OpenLink();
        InitializeRfuLinkManager_JoinGroup();
        RfuSetIgnoreError(TRUE);
        data->incomingPlayerList = AllocZeroed(RFU_CHILD_MAX * sizeof(struct RfuIncomingPlayer));
        data->playerList = AllocZeroed(MAX_RFU_PLAYER_LIST_SIZE * sizeof(struct RfuPlayer));
        data->state = 2;
        break;
    case 2:
        ClearIncomingPlayerList(data->incomingPlayerList, RFU_CHILD_MAX);
        ClearRfuPlayerList(data->playerList->players, MAX_RFU_PLAYER_LIST_SIZE);
        data->listenTaskId = CreateTask_ListenForCompatiblePartners(data->incomingPlayerList, 0xFF);
        data->leaderId = 0;
        data->state = 3;
        break;
    case 3:
        if (GetNewLeaderCandidate() == 1)
            PlaySE(SE_PC_LOGIN);
        if (gTasks[taskId].data[15] == 0xFF)
            data->state = 10;
        break;
    case 10:
        DestroyTask(data->listenTaskId);
        Free(data->playerList);
        Free(data->incomingPlayerList);
        LinkRfu_Shutdown();
        data->state++;
        break;
    case 11:
        LinkRfu_Shutdown();
        DestroyTask(taskId);
        break;
    }
}

static bool32 IsPartnerActivityAcceptable(u32 activity, u32 linkGroup)
{
    if (linkGroup == 0xFF)
        return TRUE;

    #ifdef UBFIX
    if (linkGroup < ARRAY_COUNT(sAcceptedActivityIds))
    #else
    if (linkGroup <= ARRAY_COUNT(sAcceptedActivityIds))
    #endif
    {
        const u8 *bytes = sAcceptedActivityIds[linkGroup];

        while ((*(bytes) != 0xFF))
        {
            if ((*bytes) == activity)
                return TRUE;
            bytes++;
        }
    }

    return FALSE;
}

static u8 GetGroupListTextColor(struct WirelessLink_Group *data, u32 id)
{
    if (data->playerList->players[id].groupScheduledAnim == UNION_ROOM_SPAWN_IN)
    {
        if (data->playerList->players[id].rfu.data.startedActivity)
            return UR_COLOR_WHITE;
        else if (data->playerList->players[id].useRedText)
            return UR_COLOR_RED;
        else if (data->playerList->players[id].newPlayerCountdown != 0)
            return UR_COLOR_GREEN;
    }
    return UR_COLOR_DEFAULT;
}

static void ListMenuItemPrintFunc_UnionRoomGroups(u8 windowId, u32 id, u8 y)
{
    struct WirelessLink_Group *data = sWirelessLinkMain.group;
    u8 colorId = GetGroupListTextColor(data, id);

    PrintGroupMemberOnWindow(windowId, 8, y, &data->playerList->players[id], colorId, id);
}

static u8 GetNewLeaderCandidate(void)
{
    struct WirelessLink_Group *data = sWirelessLinkMain.group;
    u8 ret = 0;
    u8 i;
    s32 id;

    for (i = 0; i < MAX_RFU_PLAYER_LIST_SIZE; i++)
    {
        if (data->playerList->players[i].groupScheduledAnim != UNION_ROOM_SPAWN_NONE)
        {
            id = GetNewIncomingPlayerId(&data->playerList->players[i], data->incomingPlayerList->players);
            if (id != 0xFF)
            {
                if (data->playerList->players[i].groupScheduledAnim == UNION_ROOM_SPAWN_IN)
                {
                    if (ArePlayerDataDifferent(&data->playerList->players[i].rfu, &data->incomingPlayerList->players[id].rfu))
                    {
                        data->playerList->players[i].rfu = data->incomingPlayerList->players[id].rfu;
                        data->playerList->players[i].newPlayerCountdown = 64;
                        ret = 1;
                    }
                    else
                    {
                        if (data->playerList->players[i].newPlayerCountdown != 0)
                        {
                            data->playerList->players[i].newPlayerCountdown--;
                            if (data->playerList->players[i].newPlayerCountdown == 0)
                                ret = 2;
                        }
                    }
                }
                else
                {
                    data->playerList->players[i].groupScheduledAnim = UNION_ROOM_SPAWN_IN;
                    data->playerList->players[i].newPlayerCountdown = 64;
                    ret = 1;
                }

                data->playerList->players[i].timeoutCounter = 0;
            }
            else
            {
                if (data->playerList->players[i].groupScheduledAnim != UNION_ROOM_SPAWN_OUT)
                {
                    data->playerList->players[i].timeoutCounter++;
                    if (data->playerList->players[i].timeoutCounter >= 300)
                    {
                        data->playerList->players[i].groupScheduledAnim = UNION_ROOM_SPAWN_OUT;
                        ret = 2;
                    }
                }
            }
        }
    }

    for (id = 0; id < RFU_CHILD_MAX; id++)
    {
        if (TryAddIncomingPlayerToList(data->playerList->players, &data->incomingPlayerList->players[id], MAX_RFU_PLAYER_LIST_SIZE) != 0xFF)
            ret = 1;
    }

    return ret;
}

static void Task_CreateTradeMenu(u8 taskId)
{
    CB2_StartCreateTradeMenu();
    DestroyTask(taskId);
}

u8 CreateTask_CreateTradeMenu(void)
{
    return CreateTask(Task_CreateTradeMenu, 0);
}

static void Task_StartUnionRoomTrade(u8 taskId)
{
    u32 monId = GetPartyPositionOfRegisteredMon(&sUnionRoomTrade, GetMultiplayerId());

    switch (gTasks[taskId].data[0])
    {
    case 0:
        gTasks[taskId].data[0]++;
        SendBlock(0, &gPlayerParty[monId], sizeof(struct Pokemon));
        break;
    case 1:
        if (GetBlockReceivedStatus() == 3)
        {
            gEnemyParty[0] = *(struct Pokemon *)(gBlockRecvBuffer[GetMultiplayerId() ^ 1]);
            IncrementGameStat(GAME_STAT_NUM_UNION_ROOM_BATTLES);
            ResetBlockReceivedFlags();
            gTasks[taskId].data[0]++;
        }
        break;
    case 2:
        memcpy(gBlockSendBuffer, gSaveBlock1Ptr->mail, sizeof(struct Mail) * PARTY_SIZE + 4);
        if (SendBlock(0, gBlockSendBuffer, sizeof(struct Mail) * PARTY_SIZE + 4))
            gTasks[taskId].data[0]++;
        break;
    case 3:
        if (GetBlockReceivedStatus() == 3)
        {
            memcpy(gTradeMail, gBlockRecvBuffer[GetMultiplayerId() ^ 1], sizeof(struct Mail) * PARTY_SIZE);
            ResetBlockReceivedFlags();
            gSelectedTradeMonPositions[TRADE_PLAYER] = monId;
            gSelectedTradeMonPositions[TRADE_PARTNER] = PARTY_SIZE;
            gMain.savedCallback = CB2_ReturnToField;
            SetMainCallback2(CB2_LinkTrade);
            ResetUnionRoomTrade(&sUnionRoomTrade);
            DestroyTask(taskId);
        }
        break;
    }
}

static void Task_ExchangeCards(u8 taskId)
{
    switch (gTasks[taskId].data[0])
    {
    case 0:
        if (GetMultiplayerId() == 0)
            SendBlockRequest(BLOCK_REQ_SIZE_100);
        gTasks[taskId].data[0]++;
        break;
    case 1:
        if (GetBlockReceivedStatus() == GetLinkPlayerCountAsBitFlags())
        {
            s32 i;
            u16 *recvBuff;

            for (i = 0; i < GetLinkPlayerCount(); i++)
            {
                recvBuff = gBlockRecvBuffer[i];
                CopyTrainerCardData(&gTrainerCards[i], (struct TrainerCard *)recvBuff, gLinkPlayers[i].version);
            }

            if (GetLinkPlayerCount() == 2)
            {
                // Note: hasAllFrontierSymbols is a re-used field.
                // Here it is set by CreateTrainerCardInBuffer.
                // If the player has a saved Wonder Card and it is the same Wonder Card
                // as their partner then mystery gift stats are enabled.
                recvBuff = gBlockRecvBuffer[GetMultiplayerId() ^ 1];
                MysteryGift_TryEnableStatsByFlagId(((struct TrainerCard *)recvBuff)->hasAllFrontierSymbols);
            }
            else
            {
                MysteryGift_DisableStats();
            }

            ResetBlockReceivedFlags();
            DestroyTask(taskId);
        }
        break;
    }
}

static void CB2_ShowCard(void)
{
    switch (gMain.state)
    {
    case 0:
        CreateTask(Task_ExchangeCards, 5);
        gMain.state++;
        break;
    case 1:
        if (!FuncIsActiveTask(Task_ExchangeCards))
            ShowTrainerCardInLink(GetMultiplayerId() ^ 1, CB2_ReturnToField);
        break;
    }

    RunTasks();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
}

void StartUnionRoomBattle(u16 battleFlags)
{
    HealPlayerParty();
    SavePlayerParty();
    LoadPlayerBag();
    gLinkPlayers[0].linkType = LINKTYPE_BATTLE;
    gLinkPlayers[GetMultiplayerId()].id = GetMultiplayerId();
    gLinkPlayers[GetMultiplayerId() ^ 1].id = GetMultiplayerId() ^ 1;
    gMain.savedCallback = CB2_ReturnFromCableClubBattle;
    gBattleTypeFlags = battleFlags;
    PlayBattleBGM();
}

static void WarpForWirelessMinigame(u16 linkService, u16 x, u16 y)
{
    VarSet(VAR_CABLE_CLUB_STATE, linkService);
    SetWarpDestination(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum, WARP_ID_NONE, x, y);
    SetDynamicWarpWithCoords(0, gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum, WARP_ID_NONE, x, y);
    WarpIntoMap();
}

static void WarpForCableClubActivity(s8 mapGroup, s8 mapNum, s32 x, s32 y, u16 linkService)
{
    gSpecialVar_0x8004 = linkService;
    VarSet(VAR_CABLE_CLUB_STATE, linkService);
    gFieldLinkPlayerCount = GetLinkPlayerCount();
    gLocalLinkPlayerId = GetMultiplayerId();
    SetCableClubWarp();
    SetWarpDestination(mapGroup, mapNum, WARP_ID_NONE, x, y);
    WarpIntoMap();
}

static void CB2_TransitionToCableClub(void)
{
    switch (gMain.state)
    {
    case 0:
        CreateTask(Task_ExchangeCards, 5);
        gMain.state++;
        break;
    case 1:
        if (!FuncIsActiveTask(Task_ExchangeCards))
            SetMainCallback2(CB2_ReturnToFieldCableClub);
        break;
    }

    RunTasks();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
}

static void CreateTrainerCardInBuffer(void *dest, bool32 setWonderCard)
{
    struct TrainerCard * card = (struct TrainerCard *)dest;
    TrainerCard_GenerateCardForLinkPlayer(card);

    // Below field is re-used, to be read by Task_ExchangeCards
    if (setWonderCard)
        card->hasAllFrontierSymbols = GetWonderCardFlagID();
    else
        card->hasAllFrontierSymbols = 0;
}

static void Task_StartActivity(u8 taskId)
{
    MysteryGift_DisableStats();
    switch (gPlayerCurrActivity)
    {
    case ACTIVITY_BATTLE_SINGLE:
    case ACTIVITY_BATTLE_DOUBLE:
    case ACTIVITY_BATTLE_MULTI:
    case ACTIVITY_TRADE:
    case ACTIVITY_POKEMON_JUMP:
    case ACTIVITY_BERRY_CRUSH:
    case ACTIVITY_BERRY_PICK:
    case ACTIVITY_SPIN_TRADE:
    case ACTIVITY_RECORD_CORNER:
        SaveLinkTrainerNames();
        break;
    }

    switch (gPlayerCurrActivity)
    {
    case ACTIVITY_BATTLE_SINGLE | IN_UNION_ROOM:
    case ACTIVITY_ACCEPT | IN_UNION_ROOM:
        CleanupOverworldWindowsAndTilemaps();
        gMain.savedCallback = CB2_UnionRoomBattle;
        InitChooseHalfPartyForBattle(3);
        break;
    case ACTIVITY_BATTLE_SINGLE:
        CleanupOverworldWindowsAndTilemaps();
        CreateTrainerCardInBuffer(gBlockSendBuffer, TRUE);
        HealPlayerParty();
        SavePlayerParty();
        LoadPlayerBag();
        WarpForCableClubActivity(MAP_GROUP(BATTLE_COLOSSEUM_2P), MAP_NUM(BATTLE_COLOSSEUM_2P), 6, 8, USING_SINGLE_BATTLE);
        SetMainCallback2(CB2_TransitionToCableClub);
        break;
    case ACTIVITY_BATTLE_DOUBLE:
        CleanupOverworldWindowsAndTilemaps();
        HealPlayerParty();
        SavePlayerParty();
        LoadPlayerBag();
        CreateTrainerCardInBuffer(gBlockSendBuffer, TRUE);
        WarpForCableClubActivity(MAP_GROUP(BATTLE_COLOSSEUM_2P), MAP_NUM(BATTLE_COLOSSEUM_2P), 6, 8, USING_DOUBLE_BATTLE);
        SetMainCallback2(CB2_TransitionToCableClub);
        break;
    case ACTIVITY_BATTLE_MULTI:
        CleanupOverworldWindowsAndTilemaps();
        HealPlayerParty();
        SavePlayerParty();
        LoadPlayerBag();
        CreateTrainerCardInBuffer(gBlockSendBuffer, TRUE);
        WarpForCableClubActivity(MAP_GROUP(BATTLE_COLOSSEUM_4P), MAP_NUM(BATTLE_COLOSSEUM_4P), 5, 8, USING_MULTI_BATTLE);
        SetMainCallback2(CB2_TransitionToCableClub);
        break;
    case ACTIVITY_TRADE:
        CreateTrainerCardInBuffer(gBlockSendBuffer, TRUE);
        CleanupOverworldWindowsAndTilemaps();
        WarpForCableClubActivity(MAP_GROUP(TRADE_CENTER), MAP_NUM(TRADE_CENTER), 5, 8, USING_TRADE_CENTER);
        SetMainCallback2(CB2_TransitionToCableClub);
        break;
    case ACTIVITY_RECORD_CORNER:
        CreateTrainerCardInBuffer(gBlockSendBuffer, TRUE);
        CleanupOverworldWindowsAndTilemaps();
        WarpForCableClubActivity(MAP_GROUP(RECORD_CORNER), MAP_NUM(RECORD_CORNER), 8, 9, USING_RECORD_CORNER);
        SetMainCallback2(CB2_TransitionToCableClub);
        break;
    case ACTIVITY_TRADE | IN_UNION_ROOM:
        CleanupOverworldWindowsAndTilemaps();
        CreateTask(Task_StartUnionRoomTrade, 0);
        break;
    case ACTIVITY_CHAT:
    case ACTIVITY_CHAT | IN_UNION_ROOM:
        if (GetMultiplayerId() == 0)
        {
            LinkRfu_CreateConnectionAsParent();
        }
        else
        {
            LinkRfu_StopManagerBeforeEnteringChat();
            SetHostRfuGameData(ACTIVITY_CHAT | IN_UNION_ROOM, 0, TRUE);
        }
        EnterUnionRoomChat();
        break;
    case ACTIVITY_CARD:
    case ACTIVITY_CARD | IN_UNION_ROOM:
        CreateTrainerCardInBuffer(gBlockSendBuffer, FALSE);
        SetMainCallback2(CB2_ShowCard);
        break;
    case ACTIVITY_POKEMON_JUMP:
        WarpForWirelessMinigame(USING_MINIGAME, 5, 1);
        StartPokemonJump(GetCursorSelectionMonId(), CB2_LoadMap);
        break;
    case ACTIVITY_BERRY_CRUSH:
        WarpForWirelessMinigame(USING_BERRY_CRUSH, 9, 1);
        StartBerryCrush(CB2_LoadMap);
        break;
    case ACTIVITY_BERRY_PICK:
        WarpForWirelessMinigame(USING_MINIGAME, 5, 1);
        StartDodrioBerryPicking(GetCursorSelectionMonId(), CB2_LoadMap);
        break;
    }

    DestroyTask(taskId);
    gSpecialVar_Result = LINKUP_SUCCESS;
    if (gPlayerCurrActivity != (ACTIVITY_TRADE | IN_UNION_ROOM))
        UnlockPlayerFieldControls();
}

static void Task_RunScriptAndFadeToActivity(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 *sendBuff = (u16 *)(gBlockSendBuffer);

    switch (data[0])
    {
    case 0:
        gSpecialVar_Result = LINKUP_SUCCESS;
        switch (gPlayerCurrActivity)
        {
        case ACTIVITY_BATTLE_TOWER:
        case ACTIVITY_BATTLE_TOWER_OPEN:
            gLinkPlayers[0].linkType = LINKTYPE_BATTLE;
            gLinkPlayers[0].id = 0;
            gLinkPlayers[1].id = 2;
            sendBuff[0] = GetMonData(&gPlayerParty[gSelectedOrderFromParty[0] - 1], MON_DATA_SPECIES);
            sendBuff[1] = GetMonData(&gPlayerParty[gSelectedOrderFromParty[1] - 1], MON_DATA_SPECIES, NULL);
            gMain.savedCallback = NULL;
            data[0] = 4;
            SaveLinkTrainerNames();
            ResetBlockReceivedFlags();
            break;
        case ACTIVITY_BERRY_BLENDER:
        case ACTIVITY_CONTEST_COOL:
        case ACTIVITY_CONTEST_BEAUTY:
        case ACTIVITY_CONTEST_CUTE:
        case ACTIVITY_CONTEST_SMART:
        case ACTIVITY_CONTEST_TOUGH:
            SaveLinkTrainerNames();
            DestroyTask(taskId);
        default:
            ScriptContext_Enable();
            data[0] = 1;
            break;
        }
        break;
    case 1:
        if (!ScriptContext_IsEnabled())
        {
            FadeScreen(FADE_TO_BLACK, 0);
            data[0] = 2;
        }
        break;
    case 2:
        if (!gPaletteFade.active)
        {
            if (gPlayerCurrActivity == ACTIVITY_29)
            {
                DestroyTask(taskId);
                SetMainCallback2(CB2_StartCreateTradeMenu);
            }
            else
            {
                SetLinkStandbyCallback();
                data[0] = 3;
            }
        }
        break;
    case 3:
        if (IsLinkTaskFinished())
        {
            DestroyTask(taskId);
            CreateTask_StartActivity();
        }
        break;
    case 4:
        if (SendBlock(0, gBlockSendBuffer, 0xE))
            data[0] = 5;
        break;
    case 5:
        if (GetBlockReceivedStatus() == 3)
        {
            ResetBlockReceivedFlags();
            if (AreBattleTowerLinkSpeciesSame(gBlockRecvBuffer[0], gBlockRecvBuffer[1]))
            {
                gSpecialVar_Result = LINKUP_FAILED_BATTLE_TOWER;
                data[0] = 7;
            }
            else
            {
                data[0] = 6;
            }
        }
        break;
    case 6:
        ScriptContext_Enable();
        DestroyTask(taskId);
        break;
    case 7:
        SetCloseLinkCallback();
        data[0] = 8;
        break;
    case 8:
        if (gReceivedRemoteLinkPlayers == 0)
        {
            DestroyWirelessStatusIndicatorSprite();
            ScriptContext_Enable();
            DestroyTask(taskId);
        }
        break;
    }
}

static void CreateTask_RunScriptAndFadeToActivity(void)
{
    CreateTask(Task_RunScriptAndFadeToActivity, 0);
}

static void CreateTask_StartActivity(void)
{
    u8 taskId = CreateTask(Task_StartActivity, 0);
    gTasks[taskId].data[0] = 0;
}

// Sending Wonder Card/News
void CreateTask_SendMysteryGift(u32 activity)
{
    u8 taskId;
    struct WirelessLink_Leader *data;

    taskId = CreateTask(Task_SendMysteryGift, 0);
    sWirelessLinkMain.leader = data = (void *)(gTasks[taskId].data);

    data->state = 0;
    data->textState = 0;
    data->activity = activity;
    gSpecialVar_Result = LINKUP_ONGOING;
}

static void Task_SendMysteryGift(u8 taskId)
{
    struct WirelessLink_Leader *data = sWirelessLinkMain.leader;
    struct WindowTemplate winTemplate;
    s32 val;

    switch (data->state)
    {
    case 0:
        gPlayerCurrActivity = data->activity;
        sPlayerActivityGroupSize = 2;
        SetHostRfuGameData(data->activity, 0, FALSE);
        SetHostRfuWonderFlags(FALSE, FALSE);
        SetWirelessCommType1();
        OpenLink();
        InitializeRfuLinkManager_LinkLeader(2);
        data->state = 1;
        break;
    case 1:
        data->incomingPlayerList = AllocZeroed(RFU_CHILD_MAX * sizeof(struct RfuIncomingPlayer));
        data->playerList = AllocZeroed(MAX_RFU_PLAYERS * sizeof(struct RfuPlayer));
        data->playerListBackup = AllocZeroed(MAX_RFU_PLAYERS * sizeof(struct RfuPlayer));
        ClearIncomingPlayerList(data->incomingPlayerList, RFU_CHILD_MAX);
        ClearRfuPlayerList(data->playerList->players, MAX_RFU_PLAYERS);
        CopyHostRfuGameDataAndUsername(&data->playerList->players[0].rfu.data, data->playerList->players[0].rfu.name);
        data->playerList->players[0].timeoutCounter = 0;
        data->playerList->players[0].groupScheduledAnim = UNION_ROOM_SPAWN_IN;
        data->playerList->players[0].useRedText = FALSE;
        data->playerList->players[0].newPlayerCountdown = 0;
        data->listenTaskId = CreateTask_ListenForCompatiblePartners(data->incomingPlayerList, 0xFF);

        winTemplate = sWindowTemplate_PlayerList;
        winTemplate.baseBlock = GetMysteryGiftBaseBlock();
        winTemplate.paletteNum = 12;
        data->listWindowId = AddWindow(&winTemplate);
        MG_DrawTextBorder(data->listWindowId);
        gMultiuseListMenuTemplate = sListMenuTemplate_PossibleGroupMembers;
        gMultiuseListMenuTemplate.windowId = data->listWindowId;
        data->listTaskId = ListMenuInit(&gMultiuseListMenuTemplate, 0, 0);

        CopyBgTilemapBufferToVram(0);
        data->playerCount = 1;
        data->state = 2;
        break;
    case 2:
        StringCopy(gStringVar1, sLinkGroupActivityNameTexts[gPlayerCurrActivity]);
        GetAwaitingCommunicationText(gStringVarBuffer, gPlayerCurrActivity);
        data->state = 3;
        break;
    case 3:
        MG_AddMessageTextPrinter(gStringVarBuffer);
        data->state = 4;
        break;
    case 4:
        Leader_SetStateIfMemberListChanged(data, 5, 6);
        if (JOY_NEW(B_BUTTON))
        {
            data->state = 13;
            DestroyWirelessStatusIndicatorSprite();
        }
        break;
    case 6:
        if (PrintMysteryGiftMenuMessage(&data->textState, sText_LinkWithFriendDropped))
        {
            data->playerCount = LeaderPrunePlayerList(data->playerList);
            RedrawListMenu(data->listTaskId);
            data->state = 2;
        }
        break;
    case 5:
        data->state = 7;
        break;
    case 7:
        switch (DoMysteryGiftYesNo(&data->textState, &data->yesNoWindowId, FALSE, gStringVarBuffer))
        {
        case 0:
            LoadWirelessStatusIndicatorSpriteGfx();
            CreateWirelessStatusIndicatorSprite(0, 0);
            data->playerList->players[data->playerCount].newPlayerCountdown = 0;
            RedrawListMenu(data->listTaskId);
            data->joinRequestAnswer = RFU_STATUS_JOIN_GROUP_OK;
            SendRfuStatusToPartner(data->joinRequestAnswer, ReadAsU16(data->playerList->players[data->playerCount].rfu.data.compatibility.playerTrainerId), data->playerList->players[data->playerCount].rfu.name);
            data->state = 8;
            break;
        case 1:
        case MENU_B_PRESSED:
            data->joinRequestAnswer = RFU_STATUS_JOIN_GROUP_NO;
            SendRfuStatusToPartner(data->joinRequestAnswer, ReadAsU16(data->playerList->players[data->playerCount].rfu.data.compatibility.playerTrainerId), data->playerList->players[data->playerCount].rfu.name);
            data->state = 8;
            break;
        }
        break;
    case 8:
        val = WaitSendRfuStatusToPartner(ReadAsU16(data->playerList->players[data->playerCount].rfu.data.compatibility.playerTrainerId), data->playerList->players[data->playerCount].rfu.name);
        if (val == 1) // Send complete
        {
            if (data->joinRequestAnswer == RFU_STATUS_JOIN_GROUP_OK)
            {
                data->playerList->players[data->playerCount].newPlayerCountdown = 0;
                RedrawListMenu(data->listTaskId);
                data->playerCount++;
                CopyAndTranslatePlayerName(gStringVar1, &data->playerList->players[data->playerCount - 1]);
                StringExpandPlaceholders(gStringVarBuffer, sText_AnOKWasSentToPlayer);
                data->state = 9;
                LinkRfu_StopManagerAndFinalizeSlots();
            }
            else
            {
                RequestDisconnectSlotByTrainerNameAndId(data->playerList->players[data->playerCount].rfu.name, ReadAsU16(data->playerList->players[data->playerCount].rfu.data.compatibility.playerTrainerId));
                data->playerList->players[data->playerCount].groupScheduledAnim = UNION_ROOM_SPAWN_NONE;
                LeaderPrunePlayerList(data->playerList);
                RedrawListMenu(data->listTaskId);
                data->state = 2;
            }

            data->joinRequestAnswer = 0;
        }
        else if (val == 2) // Member disconnected
        {
            RfuSetStatus(RFU_STATUS_OK, 0);
            data->state = 2;
        }
        break;
    case 9:
        MG_AddMessageTextPrinter(gStringVarBuffer);
        data->state = 10;
        break;
    case 10:
        if (++data->delayTimerAfterOk > 120)
            data->state = 11;
        break;
    case 11:
        if (!Leader_SetStateIfMemberListChanged(data, 5, 6))
            data->state = 12;
        break;
    case 12:
        if (LmanAcceptSlotFlagIsNotZero())
        {
            WaitRfuState(FALSE);
            data->state = 15;
        }
        else
        {
            data->state = 6;
        }
        break;
    case 13:
        DestroyWirelessStatusIndicatorSprite();
        LinkRfu_Shutdown();
        DestroyListMenuTask(data->listTaskId, 0, 0);
        CopyBgTilemapBufferToVram(0);
        RemoveWindow(data->listWindowId);
        DestroyTask(data->listenTaskId);
        Free(data->playerListBackup);
        Free(data->playerList);
        Free(data->incomingPlayerList);
        data->state++;
        break;
    case 14:
        if (PrintMysteryGiftMenuMessage(&data->textState, sText_PleaseStartOver))
        {
            DestroyTask(taskId);
            gSpecialVar_Result = LINKUP_FAILED;
        }
        break;
    case 15:
        if (RfuGetStatus() == RFU_STATUS_FATAL_ERROR || RfuGetStatus() == RFU_STATUS_CONNECTION_ERROR)
        {
            data->state = 13;
        }
        else if (gReceivedRemoteLinkPlayers)
        {
            UpdateGameData_GroupLockedIn(TRUE);
            data->state++;
        }
        break;
    case 16:
        DestroyListMenuTask(data->listTaskId, 0, 0);
        CopyBgTilemapBufferToVram(0);
        RemoveWindow(data->listWindowId);
        DestroyTask(data->listenTaskId);
        Free(data->playerListBackup);
        Free(data->playerList);
        Free(data->incomingPlayerList);
        SetLinkStandbyCallback();
        data->state++;
        break;
    case 17:
        if (IsLinkTaskFinished())
            DestroyTask(taskId);
        break;
    }
}

void CreateTask_LinkMysteryGiftWithFriend(u32 activity)
{
    u8 taskId;
    struct WirelessLink_Group *data;

    taskId = CreateTask(Task_CardOrNewsWithFriend, 0);
    sWirelessLinkMain.group = data = (void *)(gTasks[taskId].data);
    sGroup = data;

    data->state = 0;
    data->textState = 0;
    data->isWonderNews = activity - ACTIVITY_WONDER_CARD;
    gSpecialVar_Result = LINKUP_ONGOING;
}

static void Task_CardOrNewsWithFriend(u8 taskId)
{
    s32 id;
    struct WindowTemplate listWinTemplate, playerNameWinTemplate;
    struct WirelessLink_Group *data = sWirelessLinkMain.group;

    switch (data->state)
    {
    case 0:
        SetHostRfuGameData(data->isWonderNews + ACTIVITY_WONDER_CARD, 0, FALSE);
        SetWirelessCommType1();
        OpenLink();
        InitializeRfuLinkManager_JoinGroup();
        data->incomingPlayerList = AllocZeroed(RFU_CHILD_MAX * sizeof(struct RfuIncomingPlayer));
        data->playerList = AllocZeroed(MAX_RFU_PLAYER_LIST_SIZE * sizeof(struct RfuPlayer));
        data->state = 1;
        break;
    case 1:
        MG_AddMessageTextPrinter(sText_ChooseTrainer);
        data->state = 2;
        break;
    case 2:
        ClearIncomingPlayerList(data->incomingPlayerList, RFU_CHILD_MAX);
        ClearRfuPlayerList(data->playerList->players, MAX_RFU_PLAYER_LIST_SIZE);
        data->listenTaskId = CreateTask_ListenForCompatiblePartners(data->incomingPlayerList, data->isWonderNews + LINK_GROUP_WONDER_CARD);

        listWinTemplate = sWindowTemplate_GroupList;
        listWinTemplate.baseBlock = GetMysteryGiftBaseBlock();
        listWinTemplate.paletteNum = 12;
        data->listWindowId = AddWindow(&listWinTemplate);

        playerNameWinTemplate = sWindowTemplate_PlayerNameAndId;
        playerNameWinTemplate.paletteNum = 12;
        data->playerNameAndIdWindowId = AddWindow(&playerNameWinTemplate);

        MG_DrawTextBorder(data->listWindowId);
        gMultiuseListMenuTemplate = sListMenuTemplate_UnionRoomGroups;
        gMultiuseListMenuTemplate.windowId = data->listWindowId;
        data->listTaskId = ListMenuInit(&gMultiuseListMenuTemplate, 0, 0);

        MG_DrawTextBorder(data->playerNameAndIdWindowId);
        FillWindowPixelBuffer(data->playerNameAndIdWindowId, PIXEL_FILL(1));
        PutWindowTilemap(data->playerNameAndIdWindowId);
        PrintPlayerNameAndIdOnWindow(data->playerNameAndIdWindowId);
        CopyWindowToVram(data->playerNameAndIdWindowId, COPYWIN_GFX);

        CopyBgTilemapBufferToVram(0);
        data->leaderId = 0;
        data->state = 3;
        break;
    case 3:
        id = GetNewLeaderCandidate();
        switch (id)
        {
        case 1:
            PlaySE(SE_PC_LOGIN);
        default:
            RedrawListMenu(data->listTaskId);
            break;
        case 0:
            id = ListMenu_ProcessInput(data->listTaskId);
            if (JOY_NEW(A_BUTTON) && id != LIST_NOTHING_CHOSEN)
            {
                // Needed to match
                u32 UNUSED activity = data->playerList->players[id].rfu.data.activity;

                if (data->playerList->players[id].groupScheduledAnim == UNION_ROOM_SPAWN_IN && !data->playerList->players[id].rfu.data.startedActivity)
                {
                    data->leaderId = id;
                    LoadWirelessStatusIndicatorSpriteGfx();
                    CreateWirelessStatusIndicatorSprite(0, 0);
                    RedrawListMenu(data->listTaskId);
                    CopyAndTranslatePlayerName(gStringVar1, &data->playerList->players[data->leaderId]);
                    CreateTask_RfuReconnectWithParent(data->playerList->players[data->leaderId].rfu.name, ReadAsU16(data->playerList->players[data->leaderId].rfu.data.compatibility.playerTrainerId));
                    PlaySE(SE_POKENAV_ON);
                    data->state = 4;
                }
                else
                {
                    PlaySE(SE_WALL_HIT);
                }
            }
            else if (JOY_NEW(B_BUTTON))
            {
                data->state = 6;
            }
            break;
        }
        break;
    case 4:
        MG_AddMessageTextPrinter(sText_AwaitingPlayersResponse);
        CopyAndTranslatePlayerName(gStringVar1, &data->playerList->players[data->leaderId]);
        data->state = 5;
        break;
    case 5:
        if (gReceivedRemoteLinkPlayers)
        {
            gPlayerCurrActivity = data->playerList->players[data->leaderId].rfu.data.activity;
            data->state = 10;
        }

        switch (RfuGetStatus())
        {
        case RFU_STATUS_FATAL_ERROR:
        case RFU_STATUS_CONNECTION_ERROR:
        case RFU_STATUS_JOIN_GROUP_NO:
            data->state = 8;
            break;
        case RFU_STATUS_JOIN_GROUP_OK:
            MG_AddMessageTextPrinter(sText_PlayerSentBackOK);
            RfuSetStatus(RFU_STATUS_OK, 0);
            break;
        }
        break;
    case 6:
    case 8:
    case 10:
        DestroyListMenuTask(data->listTaskId, 0, 0);
        CopyBgTilemapBufferToVram(0);
        RemoveWindow(data->playerNameAndIdWindowId);
        RemoveWindow(data->listWindowId);
        DestroyTask(data->listenTaskId);
        Free(data->playerList);
        Free(data->incomingPlayerList);
        data->state++;
        break;
    case 9:
        if (PrintMysteryGiftMenuMessage(&data->textState, sLinkDroppedTexts[RfuGetStatus()]))
        {
            DestroyWirelessStatusIndicatorSprite();
            DestroyTask(taskId);
            LinkRfu_Shutdown();
            gSpecialVar_Result = LINKUP_FAILED;
        }
        break;
    case 7:
        DestroyWirelessStatusIndicatorSprite();
        MG_AddMessageTextPrinter(sText_PleaseStartOver);
        DestroyTask(taskId);
        LinkRfu_Shutdown();
        gSpecialVar_Result = LINKUP_FAILED;
        break;
    case 11:
        data->state++;
        SetLinkStandbyCallback();
        break;
    case 12:
        if (IsLinkTaskFinished())
            DestroyTask(taskId);
        break;
    }
}

void CreateTask_LinkMysteryGiftOverWireless(u32 activity)
{
    u8 taskId;
    struct WirelessLink_Group *data;

    taskId = CreateTask(Task_CardOrNewsOverWireless, 0);
    sWirelessLinkMain.group = data = (void *)(gTasks[taskId].data);
    sGroup = data;

    data->state = 0;
    data->textState = 0;
    data->isWonderNews = activity - ACTIVITY_WONDER_CARD;
    gSpecialVar_Result = LINKUP_ONGOING;
}

static void Task_CardOrNewsOverWireless(u8 taskId)
{
    s32 id;
    struct WindowTemplate winTemplate;
    struct WirelessLink_Group *data = sWirelessLinkMain.group;

    switch (data->state)
    {
    case 0:
        SetHostRfuGameData(ACTIVITY_NONE, 0, FALSE);
        SetWirelessCommType1();
        OpenLink();
        InitializeRfuLinkManager_JoinGroup();
        data->incomingPlayerList = AllocZeroed(RFU_CHILD_MAX * sizeof(struct RfuIncomingPlayer));
        data->playerList = AllocZeroed(MAX_RFU_PLAYER_LIST_SIZE * sizeof(struct RfuPlayer));
        data->state = 1;
        break;
    case 1:
        MG_AddMessageTextPrinter(sText_SearchingForWirelessSystemWait);
        data->state = 2;
        break;
    case 2:
        ClearIncomingPlayerList(data->incomingPlayerList, RFU_CHILD_MAX);
        ClearRfuPlayerList(data->playerList->players, MAX_RFU_PLAYER_LIST_SIZE);
        data->listenTaskId = CreateTask_ListenForWonderDistributor(data->incomingPlayerList, data->isWonderNews + LINK_GROUP_WONDER_CARD);

        if (data->showListMenu)
        {
            winTemplate = sWindowTemplate_GroupList;
            winTemplate.baseBlock = GetMysteryGiftBaseBlock();
            data->listWindowId = AddWindow(&winTemplate);

            MG_DrawTextBorder(data->listWindowId);
            gMultiuseListMenuTemplate = sListMenuTemplate_UnionRoomGroups;
            gMultiuseListMenuTemplate.windowId = data->listWindowId;
            data->listTaskId = ListMenuInit(&gMultiuseListMenuTemplate, 0, 0);

            CopyBgTilemapBufferToVram(0);
        }

        data->leaderId = 0;
        data->state = 3;
        break;
    case 3:
        id = GetNewLeaderCandidate();
        switch (id)
        {
        case 1:
            PlaySE(SE_PC_LOGIN);
        default:
            if (data->showListMenu)
                RedrawListMenu(data->listTaskId);
            break;
        case 0:
            if (data->showListMenu)
                id = ListMenu_ProcessInput(data->listTaskId);
            if (data->refreshTimer > 120)
            {
                if (data->playerList->players[0].groupScheduledAnim == UNION_ROOM_SPAWN_IN && !data->playerList->players[0].rfu.data.startedActivity)
                {
                    if (HasWonderCardOrNewsByLinkGroup(&data->playerList->players[0].rfu.data, data->isWonderNews + LINK_GROUP_WONDER_CARD))
                    {
                        data->leaderId = 0;
                        data->refreshTimer = 0;
                        LoadWirelessStatusIndicatorSpriteGfx();
                        CreateWirelessStatusIndicatorSprite(0, 0);
                        CreateTask_RfuReconnectWithParent(data->playerList->players[0].rfu.name, ReadAsU16(data->playerList->players[0].rfu.data.compatibility.playerTrainerId));
                        PlaySE(SE_POKENAV_ON);
                        data->state = 4;
                    }
                    else
                    {
                        PlaySE(SE_BOO);
                        data->state = 10;
                    }
                }
            }
            else if (JOY_NEW(B_BUTTON))
            {
                data->state = 6;
                data->refreshTimer = 0;
            }
            data->refreshTimer++;
            break;
        }
        break;
    case 4:
        MG_AddMessageTextPrinter(sText_AwaitingResponseFromWirelessSystem);
        CopyAndTranslatePlayerName(gStringVar1, &data->playerList->players[data->leaderId]);
        data->state = 5;
        break;
    case 5:
        if (gReceivedRemoteLinkPlayers)
        {
            gPlayerCurrActivity = data->playerList->players[data->leaderId].rfu.data.activity;
            data->state = 12;
        }

        switch (RfuGetStatus())
        {
        case RFU_STATUS_FATAL_ERROR:
        case RFU_STATUS_CONNECTION_ERROR:
        case RFU_STATUS_JOIN_GROUP_NO:
            data->state = 8;
            break;
        case RFU_STATUS_JOIN_GROUP_OK:
            MG_AddMessageTextPrinter(sText_WirelessLinkEstablished);
            RfuSetStatus(RFU_STATUS_OK, 0);
            break;
        }
        break;
    case 6:
    case 8:
    case 10:
    case 12:
        if (data->showListMenu)
        {
            DestroyListMenuTask(data->listTaskId, 0, 0);
            CopyBgTilemapBufferToVram(0);
            RemoveWindow(data->listWindowId);
        }
        DestroyTask(data->listenTaskId);
        Free(data->playerList);
        Free(data->incomingPlayerList);
        data->state++;
        break;
    case 9:
        if (PrintMysteryGiftMenuMessage(&data->textState, sText_WirelessLinkDropped))
        {
            DestroyWirelessStatusIndicatorSprite();
            DestroyTask(taskId);
            LinkRfu_Shutdown();
            gSpecialVar_Result = LINKUP_FAILED;
        }
        break;
    case 7:
        if (PrintMysteryGiftMenuMessage(&data->textState, sText_WirelessSearchCanceled))
        {
            DestroyWirelessStatusIndicatorSprite();
            DestroyTask(taskId);
            LinkRfu_Shutdown();
            gSpecialVar_Result = LINKUP_FAILED;
        }
        break;
    case 11:
        if (PrintMysteryGiftMenuMessage(&data->textState, sNoWonderSharedTexts[data->isWonderNews]))
        {
            DestroyWirelessStatusIndicatorSprite();
            DestroyTask(taskId);
            LinkRfu_Shutdown();
            gSpecialVar_Result = LINKUP_FAILED;
        }
        break;
    case 13:
        data->state++;
        SetLinkStandbyCallback();
        break;
    case 14:
        if (IsLinkTaskFinished())
            DestroyTask(taskId);
        break;
    }
}

void RunUnionRoom(void)
{
    struct WirelessLink_URoom *uroom;

    ResetHostRfuGameData();
    CreateTask(Task_RunUnionRoom, 10);

    // dumb line needed to match
    sWirelessLinkMain.uRoom = sWirelessLinkMain.uRoom;

    uroom = AllocZeroed(sizeof(*sWirelessLinkMain.uRoom));
    sWirelessLinkMain.uRoom = uroom;
    sURoom = uroom;

    uroom->state = UR_STATE_INIT;
    uroom->textState = 0;
    uroom->unknown = 0;
    uroom->unreadPlayerId = 0;

    gSpecialVar_Result = 0;
    ListMenuLoadStdPalAt(BG_PLTT_ID(13), 1);
}

static u16 ReadAsU16(const u8 *ptr)
{
    return (ptr[1] << 8) | (ptr[0]);
}

static void ScheduleFieldMessageWithFollowupState(u32 nextState, const u8 *src)
{
    struct WirelessLink_URoom *uroom = sWirelessLinkMain.uRoom;

    uroom->state = UR_STATE_PRINT_MSG;
    uroom->stateAfterPrint = nextState;
    if (src != gStringVarBuffer)
        StringExpandPlaceholders(gStringVarBuffer, src);
}

static void ScheduleFieldMessageAndExit(const u8 *src)
{
    struct WirelessLink_URoom *uroom = sWirelessLinkMain.uRoom;

    uroom->state = UR_STATE_PRINT_AND_EXIT;
    if (src != gStringVarBuffer)
        StringExpandPlaceholders(gStringVarBuffer, src);
}

static void CopyPlayerListToBuffer(struct WirelessLink_URoom *uroom)
{
    memcpy(&gDecompressionBuffer[sizeof(gDecompressionBuffer) - (MAX_UNION_ROOM_LEADERS * sizeof(struct RfuPlayer))],
            uroom->playerList,
            MAX_UNION_ROOM_LEADERS * sizeof(struct RfuPlayer));
}

static void CopyPlayerListFromBuffer(struct WirelessLink_URoom *uroom)
{
    memcpy(uroom->playerList,
           &gDecompressionBuffer[sizeof(gDecompressionBuffer) - (MAX_UNION_ROOM_LEADERS * sizeof(struct RfuPlayer))],
           MAX_UNION_ROOM_LEADERS * sizeof(struct RfuPlayer));
}

static void Task_RunUnionRoom(u8 taskId)
{
    u32 id = 0;
    s32 input = 0;
    s32 playerGender = MALE;
    struct WirelessLink_URoom *uroom = sWirelessLinkMain.uRoom;
    s16 *taskData = gTasks[taskId].data;

    switch (uroom->state)
    {
    case UR_STATE_INIT:
        uroom->incomingChildList = AllocZeroed(RFU_CHILD_MAX * sizeof(struct RfuIncomingPlayer));
        uroom->incomingParentList = AllocZeroed(RFU_CHILD_MAX * sizeof(struct RfuIncomingPlayer));
        uroom->playerList = AllocZeroed(MAX_UNION_ROOM_LEADERS * sizeof(struct RfuPlayer));
        uroom->spawnPlayer = AllocZeroed(sizeof(struct RfuPlayer));
        ClearRfuPlayerList(uroom->playerList->players, MAX_UNION_ROOM_LEADERS);
        gPlayerCurrActivity = IN_UNION_ROOM;
        uroom->searchTaskId = CreateTask_SearchForChildOrParent(uroom->incomingParentList, uroom->incomingChildList, LINK_GROUP_UNION_ROOM_RESUME);
        InitUnionRoomPlayerObjects(uroom->objects);
        SetTilesAroundUnionRoomPlayersPassable();
        uroom->state = UR_STATE_INIT_OBJECTS;
        break;
    case UR_STATE_INIT_OBJECTS:
        CreateUnionRoomPlayerSprites(uroom->spriteIds, taskData[0]);
        if (++taskData[0] == MAX_UNION_ROOM_LEADERS)
            uroom->state = UR_STATE_INIT_LINK;
        break;
    case UR_STATE_INIT_LINK:
        SetHostRfuGameData(IN_UNION_ROOM, 0, FALSE);
        SetTradeBoardRegisteredMonInfo(sUnionRoomTrade.type, sUnionRoomTrade.playerSpecies, sUnionRoomTrade.playerLevel);
        SetWirelessCommType1();
        OpenLink();
        InitializeRfuLinkManager_EnterUnionRoom();
        ClearRfuPlayerList(&uroom->spawnPlayer->players[0], 1);
        ClearIncomingPlayerList(uroom->incomingChildList, RFU_CHILD_MAX);
        ClearIncomingPlayerList(uroom->incomingParentList, RFU_CHILD_MAX);
        gSpecialVar_Result = 0;
        uroom->state = UR_STATE_CHECK_SELECTING_MON;
        break;
    case UR_STATE_CHECK_SELECTING_MON:
        if ((GetPartyMenuType() == PARTY_MENU_TYPE_UNION_ROOM_REGISTER
          || GetPartyMenuType() == PARTY_MENU_TYPE_UNION_ROOM_TRADE)
           && sUnionRoomTrade.state != URTRADE_STATE_NONE)
        {
            id = GetCursorSelectionMonId();
            switch (sUnionRoomTrade.state)
            {
            case URTRADE_STATE_REGISTERING:
                UpdateGameData_SetActivity(ACTIVITY_PLYRTALK | IN_UNION_ROOM, 0, TRUE);
                if (id >= PARTY_SIZE)
                {
                    ResetUnionRoomTrade(&sUnionRoomTrade);
                    SetTradeBoardRegisteredMonInfo(TYPE_NORMAL, SPECIES_NONE, 0);
                    ScheduleFieldMessageAndExit(sText_RegistrationCanceled);
                }
                else if (!RegisterTradeMonAndGetIsEgg(GetCursorSelectionMonId(), &sUnionRoomTrade))
                {
                    ScheduleFieldMessageWithFollowupState(UR_STATE_REGISTER_REQUEST_TYPE, sText_ChooseRequestedMonType);
                }
                else
                {
                    uroom->state = UR_STATE_REGISTER_COMPLETE;
                }
                break;
            case URTRADE_STATE_OFFERING:
                CopyPlayerListFromBuffer(uroom);
                taskData[1] = sUnionRoomTrade.offerPlayerId;
                if (id >= PARTY_SIZE)
                {
                    ScheduleFieldMessageAndExit(sText_TradeCanceled);
                }
                else
                {
                    UpdateGameData_SetActivity(ACTIVITY_PLYRTALK | IN_UNION_ROOM, 0, TRUE);
                    gPlayerCurrActivity = ACTIVITY_TRADE | IN_UNION_ROOM;
                    RegisterTradeMon(GetCursorSelectionMonId(), &sUnionRoomTrade);
                    uroom->state = UR_STATE_TRADE_OFFER_MON;
                }
                break;
            }
            sUnionRoomTrade.state = URTRADE_STATE_NONE;
        }
        else
        {
            uroom->state = UR_STATE_MAIN;
        }
        break;
    case UR_STATE_MAIN:
        if (gSpecialVar_Result != 0)
        {
            if (gSpecialVar_Result == UR_INTERACT_ATTENDANT)
            {
                UpdateGameData_SetActivity(ACTIVITY_PLYRTALK | IN_UNION_ROOM, 0, TRUE);
                PlaySE(SE_PC_LOGIN);
                StringCopy(gStringVar1, gSaveBlock2Ptr->playerName);
                uroom->state = UR_STATE_INTERACT_WITH_ATTENDANT;
                gSpecialVar_Result = 0;
            }
            else if (gSpecialVar_Result == UR_INTERACT_START_MENU)
            {
                UpdateGameData_SetActivity(ACTIVITY_PLYRTALK | IN_UNION_ROOM, 0, TRUE);
                uroom->state = UR_STATE_WAIT_FOR_START_MENU;
                gSpecialVar_Result = 0;
            }
            else // UR_INTERACT_PLAYER_# (1-8)
            {
                taskData[0] = 0;
                taskData[1] = gSpecialVar_Result - 1;
                uroom->state = UR_STATE_INTERACT_WITH_PLAYER;
                gSpecialVar_Result = 0;
            }
        }
        else if (ArePlayerFieldControlsLocked() != TRUE)
        {
            if (JOY_NEW(A_BUTTON))
            {
                if (TryInteractWithUnionRoomMember(uroom->playerList, &taskData[0], &taskData[1], uroom->spriteIds))
                {
                    PlaySE(SE_SELECT);
                    StartScriptInteraction();
                    uroom->state = UR_STATE_INTERACT_WITH_PLAYER;
                    break;
                }
                else if (IsPlayerFacingTradingBoard())
                {
                    UpdateGameData_SetActivity(ACTIVITY_PLYRTALK | IN_UNION_ROOM, 0, TRUE);
                    PlaySE(SE_PC_LOGIN);
                    StartScriptInteraction();
                    StringCopy(gStringVar1, gSaveBlock2Ptr->playerName);
                    uroom->state = UR_STATE_CHECK_TRADING_BOARD;
                    break;
                }
            }

            switch (HandlePlayerListUpdate())
            {
            case PLIST_NEW_PLAYER:
                PlaySE(SE_PC_LOGIN);
            case PLIST_RECENT_UPDATE:
                ScheduleUnionRoomPlayerRefresh(uroom);
                break;
            case PLIST_CONTACTED:
                uroom->state = UR_STATE_PLAYER_CONTACTED_YOU;
                StartScriptInteraction();
                SetTradeBoardRegisteredMonInfo(TYPE_NORMAL, SPECIES_NONE, 0);
                UpdateGameData_SetActivity(ACTIVITY_NPCTALK | IN_UNION_ROOM, GetActivePartnersInfo(uroom), FALSE);
                break;
            }
            HandleUnionRoomPlayerRefresh(uroom);
        }
        break;
    case UR_STATE_WAIT_FOR_START_MENU:
        if (!FuncIsActiveTask(Task_ShowStartMenu))
        {
            UpdateGameData_SetActivity(ACTIVITY_NONE | IN_UNION_ROOM, 0, FALSE);
            uroom->state = UR_STATE_MAIN;
        }
        break;
    case UR_STATE_INTERACT_WITH_PLAYER:
        UR_RunTextPrinters();
        playerGender = GetUnionRoomPlayerGender(taskData[1], uroom->playerList);
        UpdateGameData_SetActivity(ACTIVITY_PLYRTALK | IN_UNION_ROOM, 0, TRUE);
        switch (UnionRoomGetPlayerInteractionResponse(uroom->playerList, taskData[0], taskData[1], playerGender))
        {
        case 0: // Player is or was just doing an activity
            uroom->state = UR_STATE_PRINT_AND_EXIT;
            break;
        case 1: // Link communicating
            TryConnectToUnionRoomParent(uroom->playerList->players[taskData[1]].rfu.name, &uroom->playerList->players[taskData[1]].rfu.data, gPlayerCurrActivity);
            uroom->unreadPlayerId = id; // Should be just 0, but won't match any other way.
            uroom->state = UR_STATE_TRY_COMMUNICATING;
            break;
        case 2: // Ask to join chat
            ScheduleFieldMessageWithFollowupState(UR_STATE_RECV_JOIN_CHAT_REQUEST, gStringVarBuffer);
            break;
        }
        break;
    case UR_STATE_TRY_COMMUNICATING:
        UR_RunTextPrinters();
        switch (RfuGetStatus())
        {
        case RFU_STATUS_NEW_CHILD_DETECTED:
            HandleCancelActivity(TRUE);
            uroom->state = UR_STATE_MAIN;
            break;
        case RFU_STATUS_FATAL_ERROR:
        case RFU_STATUS_CONNECTION_ERROR:
            if (IsUnionRoomListenTaskActive() == TRUE)
                ScheduleFieldMessageAndExit(sText_TrainerAppearsBusy);
            else
                ScheduleFieldMessageWithFollowupState(UR_STATE_CANCEL_ACTIVITY_LINK_ERROR, sText_TrainerAppearsBusy);

            gPlayerCurrActivity = IN_UNION_ROOM;
            break;
        }

        if (gReceivedRemoteLinkPlayers)
        {
            CreateTrainerCardInBuffer(gBlockSendBuffer, TRUE);
            CreateTask(Task_ExchangeCards, 5);
            uroom->state = UR_STATE_COMMUNICATING_WAIT_FOR_DATA;
        }
        break;
    case UR_STATE_COMMUNICATING_WAIT_FOR_DATA:
        if (!FuncIsActiveTask(Task_ExchangeCards))
        {
            if (gPlayerCurrActivity == (ACTIVITY_TRADE | IN_UNION_ROOM))
                ScheduleFieldMessageWithFollowupState(UR_STATE_SEND_TRADE_REQUST, sText_AwaitingPlayersResponseAboutTrade);
            else
                uroom->state = UR_STATE_DO_SOMETHING_PROMPT;
        }
        break;
    case UR_STATE_CANCEL_ACTIVITY_LINK_ERROR:
        if (!gReceivedRemoteLinkPlayers)
        {
            HandleCancelActivity(FALSE);
            UpdateUnionRoomMemberFacing(taskData[0], taskData[1], uroom->playerList);
            uroom->state = UR_STATE_INIT_LINK;
        }
        break;
    case UR_STATE_DO_SOMETHING_PROMPT:
        id = ConvPartnerUnameAndGetWhetherMetAlready(&uroom->playerList->players[taskData[1]]);
        playerGender = GetUnionRoomPlayerGender(taskData[1], uroom->playerList);
        ScheduleFieldMessageWithFollowupState(UR_STATE_HANDLE_DO_SOMETHING_PROMPT_INPUT, sHiDoSomethingTexts[id][playerGender]);
        break;
    case UR_STATE_HANDLE_DO_SOMETHING_PROMPT_INPUT:
        input = ListMenuHandler_AllItemsAvailable(&uroom->textState,
                                                  &uroom->topListMenuWindowId,
                                                  &uroom->topListMenuId,
                                                  &sWindowTemplate_InviteToActivity,
                                                  &sListMenuTemplate_InviteToActivity);
        if (input != LIST_NOTHING_CHOSEN)
        {
            if (!gReceivedRemoteLinkPlayers)
            {
                uroom->state = UR_STATE_TRAINER_APPEARS_BUSY;
            }
            else
            {
                uroom->partnerYesNoResponse = 0;
                playerGender = GetUnionRoomPlayerGender(taskData[1], uroom->playerList);
                if (input == LIST_CANCEL || input == IN_UNION_ROOM)
                {
                    uroom->playerSendBuffer[0] = IN_UNION_ROOM;
                    Rfu_SendPacket(uroom->playerSendBuffer);
                    StringCopy(gStringVarBuffer, sIfYouWantToDoSomethingTexts[gLinkPlayers[0].gender]);
                    uroom->state = UR_STATE_REQUEST_DECLINED;
                }
                else
                {
                    gPlayerCurrActivity = input;
                    sPlayerActivityGroupSize = (u32)input >> 8; // Extract capacity from sInviteToActivityMenuItems
                    if (gPlayerCurrActivity == (ACTIVITY_BATTLE_SINGLE | IN_UNION_ROOM) && !HasAtLeastTwoMonsOfLevel30OrLower())
                    {
                        ScheduleFieldMessageWithFollowupState(UR_STATE_DO_SOMETHING_PROMPT, sText_NeedTwoMonsOfLevel30OrLower1);
                    }
                    else
                    {
                        uroom->playerSendBuffer[0] = gPlayerCurrActivity | IN_UNION_ROOM;
                        Rfu_SendPacket(uroom->playerSendBuffer);
                        uroom->state = UR_STATE_SEND_ACTIVITY_REQUEST;
                    }
                }
            }
        }
        break;
    case UR_STATE_TRAINER_APPEARS_BUSY:
        StringCopy(gStringVarBuffer, sText_TrainerBattleBusy);
        uroom->state = UR_STATE_CANCEL_REQUEST_PRINT_MSG;
        break;
    case UR_STATE_SEND_ACTIVITY_REQUEST:
        PollPartnerYesNoResponse(uroom);
        playerGender = GetUnionRoomPlayerGender(taskData[1], uroom->playerList);
        id = GetResponseIdx_InviteToURoomActivity(uroom->playerSendBuffer[0] & 0x3F);
        if (PrintOnTextbox(&uroom->textState, sText_WaitOrShowCardTexts[playerGender][id]))
        {
            taskData[3] = 0;
            uroom->state = UR_STATE_WAIT_FOR_RESPONSE_TO_REQUEST;
        }
        break;
    case UR_STATE_REQUEST_DECLINED:
        SetCloseLinkCallback();
        uroom->state = UR_STATE_CANCEL_REQUEST_PRINT_MSG;
        break;
    case UR_STATE_SEND_TRADE_REQUST:
        uroom->playerSendBuffer[0] = ACTIVITY_TRADE | IN_UNION_ROOM;
        uroom->playerSendBuffer[1] = sUnionRoomTrade.species;
        uroom->playerSendBuffer[2] = sUnionRoomTrade.level;
        Rfu_SendPacket(uroom->playerSendBuffer);
        uroom->state = UR_STATE_WAIT_FOR_RESPONSE_TO_REQUEST;
        break;
    case UR_STATE_WAIT_FOR_RESPONSE_TO_REQUEST:
        if (!gReceivedRemoteLinkPlayers)
        {
            StringCopy(gStringVarBuffer, sText_TrainerBattleBusy); // Redundant, will be copied again in next state
            uroom->state = UR_STATE_TRAINER_APPEARS_BUSY;
        }
        else
        {
            PollPartnerYesNoResponse(uroom);
            if (uroom->partnerYesNoResponse == (ACTIVITY_ACCEPT | IN_UNION_ROOM))
            {
                if (gPlayerCurrActivity == ACTIVITY_CARD)
                {
                    ViewURoomPartnerTrainerCard(gStringVarBuffer, uroom, FALSE);
                    uroom->state = UR_STATE_PRINT_CARD_INFO;
                }
                else
                {
                    uroom->state = UR_STATE_PRINT_START_ACTIVITY_MSG;
                }
            }
            else if (uroom->partnerYesNoResponse == (ACTIVITY_DECLINE | IN_UNION_ROOM))
            {
                uroom->state = UR_STATE_REQUEST_DECLINED;
                GetURoomActivityRejectMsg(gStringVarBuffer, gPlayerCurrActivity | IN_UNION_ROOM, gLinkPlayers[0].gender);
                gPlayerCurrActivity = ACTIVITY_NONE;
            }
        }
        break;

    case UR_STATE_DO_SOMETHING_PROMPT_2: // Identical to UR_STATE_DO_SOMETHING_PROMPT
        id = ConvPartnerUnameAndGetWhetherMetAlready(&uroom->playerList->players[taskData[1]]);
        playerGender = GetUnionRoomPlayerGender(taskData[1], uroom->playerList);
        ScheduleFieldMessageWithFollowupState(UR_STATE_HANDLE_DO_SOMETHING_PROMPT_INPUT, sHiDoSomethingTexts[id][playerGender]);
        break;
    case UR_STATE_PRINT_CARD_INFO:
        if (PrintOnTextbox(&uroom->textState, gStringVarBuffer))
        {
            uroom->state = UR_STATE_WAIT_FINISH_READING_CARD;
            SetLinkStandbyCallback();
            uroom->partnerYesNoResponse = 0;
            uroom->recvActivityRequest[0] = 0;
        }
        break;
    case UR_STATE_WAIT_FINISH_READING_CARD:
        if (IsLinkTaskFinished())
        {
            if (GetMultiplayerId() == 0)
            {
                StringCopy(gStringVar1, gLinkPlayers[GetMultiplayerId() ^ 1].name);
                id = PlayerHasMetTrainerBefore(gLinkPlayers[1].trainerId, gLinkPlayers[1].name);
                StringExpandPlaceholders(gStringVarBuffer, sAwaitingResponseTexts[id]);
                uroom->state = UR_STATE_PRINT_CONTACT_MSG;
            }
            else
            {
                uroom->state = UR_STATE_DO_SOMETHING_PROMPT_2;
            }
        }
        break;
    case UR_STATE_RECV_JOIN_CHAT_REQUEST:
        switch (UnionRoomHandleYesNo(&uroom->textState, FALSE))
        {
        case 0: // YES
            CopyBgTilemapBufferToVram(0);
            gPlayerCurrActivity = ACTIVITY_CHAT | IN_UNION_ROOM;
            UpdateGameData_SetActivity(ACTIVITY_CHAT | IN_UNION_ROOM, 0, TRUE);
            TryConnectToUnionRoomParent(uroom->playerList->players[taskData[1]].rfu.name, &uroom->playerList->players[taskData[1]].rfu.data, gPlayerCurrActivity);
            uroom->unreadPlayerId = taskData[1];
            uroom->state = UR_STATE_TRY_ACCEPT_CHAT_REQUEST_DELAY;
            taskData[3] = 0;
            break;
        case 1: // NO
        case MENU_B_PRESSED:
            playerGender = GetUnionRoomPlayerGender(taskData[1], uroom->playerList);
            ScheduleFieldMessageAndExit(sDeclineChatTexts[playerGender]);
            break;
        }
        break;
    case UR_STATE_TRY_ACCEPT_CHAT_REQUEST_DELAY:
        if (++taskData[2] > 60)
        {
            uroom->state = UR_STATE_TRY_ACCEPT_CHAT_REQUEST;
            taskData[2] = 0;
        }
        break;
    case UR_STATE_TRY_ACCEPT_CHAT_REQUEST:
        switch (RfuGetStatus())
        {
        case RFU_STATUS_NEW_CHILD_DETECTED:
            HandleCancelActivity(TRUE);
            uroom->state = UR_STATE_MAIN;
            break;
        case RFU_STATUS_FATAL_ERROR:
        case RFU_STATUS_CONNECTION_ERROR:
            playerGender = GetUnionRoomPlayerGender(taskData[1], uroom->playerList);
            UpdateGameData_SetActivity(ACTIVITY_PLYRTALK | IN_UNION_ROOM, 0, TRUE);
            if (IsUnionRoomListenTaskActive() == TRUE)
                ScheduleFieldMessageAndExit(sChatDeclinedTexts[playerGender]);
            else
                ScheduleFieldMessageWithFollowupState(UR_STATE_CANCEL_ACTIVITY_LINK_ERROR, sChatDeclinedTexts[playerGender]);
            break;
        case RFU_STATUS_CHILD_SEND_COMPLETE:
            uroom->state = UR_STATE_ACCEPT_CHAT_REQUEST;
            break;
        }
        taskData[3]++;
        break;
    case UR_STATE_ACCEPT_CHAT_REQUEST:
        if (RfuHasErrored())
        {
            playerGender = GetUnionRoomPlayerGender(taskData[1], uroom->playerList);
            UpdateGameData_SetActivity(ACTIVITY_PLYRTALK | IN_UNION_ROOM, 0, TRUE);
            if (IsUnionRoomListenTaskActive() == TRUE)
                ScheduleFieldMessageAndExit(sChatDeclinedTexts[playerGender]);
            else
                ScheduleFieldMessageWithFollowupState(UR_STATE_CANCEL_ACTIVITY_LINK_ERROR, sChatDeclinedTexts[playerGender]);
        }
        if (gReceivedRemoteLinkPlayers)
            uroom->state = UR_STATE_START_ACTIVITY_FREE_UROOM;
        break;
    case UR_STATE_PLAYER_CONTACTED_YOU:
        PlaySE(SE_DING_DONG);
        StopUnionRoomLinkManager();
        uroom->state = UR_STATE_RECV_CONTACT_DATA;
        uroom->recvActivityRequest[0] = 0;
        break;
    case UR_STATE_RECV_CONTACT_DATA:
        if (RfuHasErrored())
        {
            HandleCancelActivity(FALSE);
            uroom->state = UR_STATE_INIT_LINK;
        }
        else if (gReceivedRemoteLinkPlayers)
        {
            CreateTrainerCardInBuffer(gBlockSendBuffer, TRUE);
            CreateTask(Task_ExchangeCards, 5);
            uroom->state = UR_STATE_WAIT_FOR_CONTACT_DATA;
        }
        break;
    case UR_STATE_WAIT_FOR_CONTACT_DATA:
        ReceiveUnionRoomActivityPacket(uroom);
        if (!FuncIsActiveTask(Task_ExchangeCards))
        {
            uroom->state = UR_STATE_PRINT_CONTACT_MSG;
            StringCopy(gStringVar1, gLinkPlayers[1].name);
            id = PlayerHasMetTrainerBefore(gLinkPlayers[1].trainerId, gLinkPlayers[1].name);
            StringExpandPlaceholders(gStringVarBuffer, sPlayerContactedYouTexts[id]);
        }
        break;
    case UR_STATE_PRINT_CONTACT_MSG:
        ReceiveUnionRoomActivityPacket(uroom);
        if (PrintOnTextbox(&uroom->textState, gStringVarBuffer))
            uroom->state = UR_STATE_HANDLE_CONTACT_DATA;
        break;
    case UR_STATE_HANDLE_CONTACT_DATA:
        ReceiveUnionRoomActivityPacket(uroom);
        if (HandleContactFromOtherPlayer(uroom) && JOY_NEW(B_BUTTON))
        {
            Rfu_DisconnectPlayerById(1);
            StringCopy(gStringVarBuffer, sText_ChatEnded);
            uroom->state = UR_STATE_CANCEL_REQUEST_PRINT_MSG;
        }
        break;
    case UR_STATE_RECV_ACTIVITY_REQUEST:
        ScheduleFieldMessageWithFollowupState(UR_STATE_HANDLE_ACTIVITY_REQUEST, gStringVarBuffer);
        break;
    case UR_STATE_HANDLE_ACTIVITY_REQUEST:
        switch (UnionRoomHandleYesNo(&uroom->textState, FALSE))
        {
        case 0: // ACCEPT
            uroom->playerSendBuffer[0] = ACTIVITY_ACCEPT | IN_UNION_ROOM;
            if (gPlayerCurrActivity == (ACTIVITY_CHAT | IN_UNION_ROOM))
                UpdateGameData_SetActivity(gPlayerCurrActivity | IN_UNION_ROOM, GetLinkPlayerInfoFlags(1), FALSE);
            else
                UpdateGameData_SetActivity(gPlayerCurrActivity | IN_UNION_ROOM, GetLinkPlayerInfoFlags(1), TRUE);

            uroom->spawnPlayer->players[0].newPlayerCountdown = 0;
            taskData[3] = 0;
            if (gPlayerCurrActivity == (ACTIVITY_BATTLE_SINGLE | IN_UNION_ROOM))
            {
                if (!HasAtLeastTwoMonsOfLevel30OrLower())
                {
                    uroom->playerSendBuffer[0] = ACTIVITY_DECLINE | IN_UNION_ROOM;
                    Rfu_SendPacket(uroom->playerSendBuffer);
                    uroom->state = UR_STATE_DECLINE_ACTIVITY_REQUEST;
                    StringCopy(gStringVarBuffer, sText_NeedTwoMonsOfLevel30OrLower2);
                }
                else
                {
                    Rfu_SendPacket(uroom->playerSendBuffer);
                    uroom->state = UR_STATE_PRINT_START_ACTIVITY_MSG;
                }
            }
            else if (gPlayerCurrActivity == (ACTIVITY_CARD | IN_UNION_ROOM))
            {
                Rfu_SendPacket(uroom->playerSendBuffer);
                ViewURoomPartnerTrainerCard(gStringVarBuffer, uroom, TRUE);
                uroom->state = UR_STATE_PRINT_CARD_INFO;
            }
            else
            {
                Rfu_SendPacket(uroom->playerSendBuffer);
                uroom->state = UR_STATE_PRINT_START_ACTIVITY_MSG;
            }
            break;
        case 1: // DECLINE
        case MENU_B_PRESSED:
            uroom->playerSendBuffer[0] = ACTIVITY_DECLINE | IN_UNION_ROOM;
            Rfu_SendPacket(uroom->playerSendBuffer);
            uroom->state = UR_STATE_DECLINE_ACTIVITY_REQUEST;
            GetYouDeclinedTheOfferMessage(gStringVarBuffer, gPlayerCurrActivity);
            break;
        }
        break;
    case UR_STATE_DECLINE_ACTIVITY_REQUEST:
        SetCloseLinkCallback();
        uroom->state = UR_STATE_CANCEL_REQUEST_PRINT_MSG;
        break;
    case UR_STATE_CANCEL_REQUEST_PRINT_MSG:
        if (!gReceivedRemoteLinkPlayers)
        {
            gPlayerCurrActivity = IN_UNION_ROOM;
            ScheduleFieldMessageWithFollowupState(UR_STATE_CANCEL_REQUEST_RESTART_LINK, gStringVarBuffer);
            memset(uroom->playerSendBuffer, 0, sizeof(uroom->playerSendBuffer));
            uroom->recvActivityRequest[0] = 0;
            uroom->partnerYesNoResponse = 0;
        }
        break;
    case UR_STATE_CANCEL_REQUEST_RESTART_LINK:
        uroom->state = UR_STATE_INIT_LINK;
        HandleCancelActivity(FALSE);
        break;
    case UR_STATE_PRINT_START_ACTIVITY_MSG:
        GetURoomActivityStartMsg(gStringVarBuffer, gPlayerCurrActivity | IN_UNION_ROOM);
        ScheduleFieldMessageWithFollowupState(UR_STATE_START_ACTIVITY_LINK, gStringVarBuffer);
        break;
    case UR_STATE_START_ACTIVITY_LINK:
        SetLinkStandbyCallback();
        uroom->state = UR_STATE_START_ACTIVITY_WAIT_FOR_LINK;
        break;
    case UR_STATE_START_ACTIVITY_WAIT_FOR_LINK:
        if (IsLinkTaskFinished())
            uroom->state = UR_STATE_START_ACTIVITY_FREE_UROOM;
        break;
    case UR_STATE_START_ACTIVITY_FREE_UROOM:
        Free(uroom->spawnPlayer);
        Free(uroom->playerList);
        Free(uroom->incomingParentList);
        Free(uroom->incomingChildList);
        DestroyTask(uroom->searchTaskId);
        DestroyUnionRoomPlayerSprites(uroom->spriteIds);
        uroom->state = UR_STATE_START_ACTIVITY_FADE;
        break;
    case UR_STATE_START_ACTIVITY_FADE:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        uroom->state = UR_STATE_START_ACTIVITY;
        break;
    case UR_STATE_START_ACTIVITY:
        if (!UpdatePaletteFade())
        {
            DestroyUnionRoomPlayerObjects();
            DestroyTask(taskId);
            Free(sWirelessLinkMain.uRoom);
            CreateTask_StartActivity();
        }
        break;
    case UR_STATE_INTERACT_WITH_ATTENDANT:
        if (GetHostRfuGameData()->tradeSpecies == SPECIES_NONE)
        {
            uroom->state = UR_STATE_REGISTER_PROMPT;
        }
        else
        {
            if (GetHostRfuGameData()->tradeSpecies == SPECIES_EGG)
            {
                StringCopy(gStringVarBuffer, sText_CancelRegistrationOfEgg);
            }
            else
            {
                StringCopy(gStringVar1, gSpeciesNames[GetHostRfuGameData()->tradeSpecies]);
                ConvertIntToDecimalStringN(gStringVar2, GetHostRfuGameData()->tradeLevel, STR_CONV_MODE_LEFT_ALIGN, 3);
                StringExpandPlaceholders(gStringVarBuffer, sText_CancelRegistrationOfMon);
            }
            ScheduleFieldMessageWithFollowupState(UR_STATE_CANCEL_REGISTRATION_PROMPT, gStringVarBuffer);
        }
        break;
    case UR_STATE_REGISTER_PROMPT:
        if (PrintOnTextbox(&uroom->textState, sText_RegisterMonAtTradingBoard))
            uroom->state = UR_STATE_REGISTER_PROMPT_HANDLE_INPUT;
        break;
    case UR_STATE_REGISTER_PROMPT_HANDLE_INPUT:
        input = ListMenuHandler_AllItemsAvailable(&uroom->textState,
                                                  &uroom->tradeBoardMainWindowId,
                                                  &uroom->tradeBoardHeaderWindowId,
                                                  &sWindowTemplate_RegisterForTrade,
                                                  &sListMenuTemplate_RegisterForTrade);
        if (input != LIST_NOTHING_CHOSEN)
        {
            if (input == LIST_CANCEL || input == 3) // Exit
            {
                uroom->state = UR_STATE_MAIN;
                HandleCancelActivity(TRUE);
            }
            else
            {
                switch (input)
                {
                case 1: // REGISTER
                    ScheduleFieldMessageWithFollowupState(UR_STATE_REGISTER_SELECT_MON_FADE, sText_WhichMonWillYouOffer);
                    break;
                case 2: // INFO
                    ScheduleFieldMessageWithFollowupState(UR_STATE_REGISTER_PROMPT_HANDLE_INPUT, sText_TradingBoardInfo);
                    break;
                }
            }
        }
        break;
    case UR_STATE_REGISTER_SELECT_MON_FADE:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        uroom->state = UR_STATE_REGISTER_SELECT_MON;
        break;
    case UR_STATE_REGISTER_SELECT_MON:
        if (!gPaletteFade.active)
        {
            sUnionRoomTrade.state = URTRADE_STATE_REGISTERING;
            gFieldCallback = FieldCB_ContinueScriptUnionRoom;
            ChooseMonForTradingBoard(PARTY_MENU_TYPE_UNION_ROOM_REGISTER, CB2_ReturnToField);
        }
        break;
    case UR_STATE_REGISTER_REQUEST_TYPE:
        input = ListMenuHandler_AllItemsAvailable(&uroom->textState,
                                                  &uroom->tradeBoardMainWindowId,
                                                  &uroom->tradeBoardHeaderWindowId,
                                                  &sWindowTemplate_TradingBoardRequestType,
                                                  &sMenuTemplate_TradingBoardRequestType);
        if (input != LIST_NOTHING_CHOSEN)
        {
            switch (input)
            {
            case LIST_CANCEL:
            case NUMBER_OF_MON_TYPES: // Exit
                ResetUnionRoomTrade(&sUnionRoomTrade);
                SetTradeBoardRegisteredMonInfo(TYPE_NORMAL, SPECIES_NONE, 0);
                ScheduleFieldMessageAndExit(sText_RegistrationCanceled);
                break;
            default:
                sUnionRoomTrade.type = input;
                uroom->state = UR_STATE_REGISTER_COMPLETE;
                break;
            }
        }
        break;
    case UR_STATE_REGISTER_COMPLETE:
        SetTradeBoardRegisteredMonInfo(sUnionRoomTrade.type, sUnionRoomTrade.playerSpecies, sUnionRoomTrade.playerLevel);
        ScheduleFieldMessageAndExit(sText_RegistraionCompleted);
        break;
    case UR_STATE_CANCEL_REGISTRATION_PROMPT:
        switch (UnionRoomHandleYesNo(&uroom->textState, FALSE))
        {
        case 0: // YES
            uroom->state = UR_STATE_CANCEL_REGISTRATION;
            break;
        case 1: // NO
        case MENU_B_PRESSED:
            HandleCancelActivity(TRUE);
            uroom->state = UR_STATE_MAIN;
            break;
        }
        break;
    case UR_STATE_CANCEL_REGISTRATION:
        if (PrintOnTextbox(&uroom->textState, sText_RegistrationCanceled2))
        {
            SetTradeBoardRegisteredMonInfo(TYPE_NORMAL, SPECIES_NONE, 0);
            ResetUnionRoomTrade(&sUnionRoomTrade);
            HandleCancelActivity(TRUE);
            uroom->state = UR_STATE_MAIN;
        }
        break;
    case UR_STATE_CHECK_TRADING_BOARD:
        if (PrintOnTextbox(&uroom->textState, sText_XCheckedTradingBoard))
            uroom->state = UR_STATE_TRADING_BOARD_LOAD;
        break;
    case UR_STATE_TRADING_BOARD_LOAD:
        UR_ClearBg0();
        uroom->state = UR_STATE_TRADING_BOARD_HANDLE_INPUT;
        break;
    case UR_STATE_TRADING_BOARD_HANDLE_INPUT:
        input = TradeBoardMenuHandler(&uroom->textState,
                                      &uroom->tradeBoardMainWindowId,
                                      &uroom->tradeBoardListMenuId,
                                      &uroom->tradeBoardHeaderWindowId,
                                      &sWindowTemplate_TradingBoardMain,
                                      &sTradeBoardListMenuTemplate,
                                      uroom->playerList);
        if (input != LIST_NOTHING_CHOSEN)
        {
            switch (input)
            {
            case LIST_CANCEL:
            case 8: // EXIT
                HandleCancelActivity(TRUE);
                uroom->state = UR_STATE_MAIN;
                break;
            default:
                UR_ClearBg0();
                switch (IsRequestedTradeInPlayerParty(uroom->playerList->players[input].rfu.data.tradeType, uroom->playerList->players[input].rfu.data.tradeSpecies))
                {
                case UR_TRADE_MATCH:
                    CopyAndTranslatePlayerName(gStringVar1, &uroom->playerList->players[input]);
                    ScheduleFieldMessageWithFollowupState(UR_STATE_TRADE_PROMPT, sText_AskTrainerToMakeTrade);
                    taskData[1] = input;
                    break;
                case UR_TRADE_NOTYPE:
                    CopyAndTranslatePlayerName(gStringVar1, &uroom->playerList->players[input]);
                    StringCopy(gStringVar2, gTypeNames[uroom->playerList->players[input].rfu.data.tradeType]);
                    ScheduleFieldMessageWithFollowupState(UR_STATE_TRADING_BOARD_LOAD, sText_DontHaveTypeTrainerWants);
                    break;
                case UR_TRADE_NOEGG:
                    CopyAndTranslatePlayerName(gStringVar1, &uroom->playerList->players[input]);
                    StringCopy(gStringVar2, gTypeNames[uroom->playerList->players[input].rfu.data.tradeType]);
                    ScheduleFieldMessageWithFollowupState(UR_STATE_TRADING_BOARD_LOAD, sText_DontHaveEggTrainerWants);
                    break;
                }
                break;
            }
        }
        break;
    case UR_STATE_TRADE_PROMPT:
        switch (UnionRoomHandleYesNo(&uroom->textState, FALSE))
        {
        case 0: // YES
            uroom->state = UR_STATE_TRADE_SELECT_MON;
            break;
        case MENU_B_PRESSED:
        case 1: // NO
            HandleCancelActivity(TRUE);
            uroom->state = UR_STATE_MAIN;
            break;
        }
        break;
    case UR_STATE_TRADE_SELECT_MON:
        if (PrintOnTextbox(&uroom->textState, sText_WhichMonWillYouOffer))
        {
            sUnionRoomTrade.state = URTRADE_STATE_OFFERING;
            memcpy(&gRfuPartnerCompatibilityData, &uroom->playerList->players[taskData[1]].rfu.data.compatibility, sizeof(gRfuPartnerCompatibilityData));
            gUnionRoomRequestedMonType = uroom->playerList->players[taskData[1]].rfu.data.tradeType;
            gUnionRoomOfferedSpecies = uroom->playerList->players[taskData[1]].rfu.data.tradeSpecies;
            gFieldCallback = FieldCB_ContinueScriptUnionRoom;
            ChooseMonForTradingBoard(PARTY_MENU_TYPE_UNION_ROOM_TRADE, CB2_ReturnToField);
            CopyPlayerListToBuffer(uroom);
            sUnionRoomTrade.offerPlayerId = taskData[1];
        }
        break;
    case UR_STATE_TRADE_OFFER_MON:
        gPlayerCurrActivity = ACTIVITY_TRADE | IN_UNION_ROOM;
        TryConnectToUnionRoomParent(uroom->playerList->players[taskData[1]].rfu.name, &uroom->playerList->players[taskData[1]].rfu.data, gPlayerCurrActivity);
        CopyAndTranslatePlayerName(gStringVar1, &uroom->playerList->players[taskData[1]]);
        UR_PrintFieldMessage(sCommunicatingWaitTexts[2]);
        uroom->state = UR_STATE_TRY_COMMUNICATING;
        break;
    case UR_STATE_PRINT_AND_EXIT:
        if (PrintOnTextbox(&uroom->textState, gStringVarBuffer))
        {
            HandleCancelActivity(TRUE);
            UpdateUnionRoomMemberFacing(taskData[0], taskData[1], uroom->playerList);
            uroom->state = UR_STATE_MAIN;
        }
        break;
    case UR_STATE_PRINT_MSG:
        if (PrintOnTextbox(&uroom->textState, gStringVarBuffer))
            uroom->state = uroom->stateAfterPrint;
        break;
    }
}

void SetUsingUnionRoomStartMenu(void)
{
    if (InUnionRoom() == TRUE)
        gSpecialVar_Result = UR_INTERACT_START_MENU;
}

static void ReceiveUnionRoomActivityPacket(struct WirelessLink_URoom *data)
{
    if (gRecvCmds[1][1] != 0 && (gRecvCmds[1][0] & RFUCMD_MASK) == RFUCMD_SEND_PACKET)
    {
        data->recvActivityRequest[0] = gRecvCmds[1][1];
        if (gRecvCmds[1][1] == (ACTIVITY_TRADE | IN_UNION_ROOM))
        {
            data->recvActivityRequest[1] = gRecvCmds[1][2];
            data->recvActivityRequest[2] = gRecvCmds[1][3];
        }
    }
}

static bool32 HandleContactFromOtherPlayer(struct WirelessLink_URoom *uroom)
{
    if (uroom->recvActivityRequest[0] != 0)
    {
        s32 id = GetChatLeaderActionRequestMessage(gStringVarBuffer, gLinkPlayers[1].gender, &uroom->recvActivityRequest[0], uroom);
        if (id == 0) // Error
        {
            return TRUE;
        }
        else if (id == 1) // Recieve activity request
        {
            uroom->state = UR_STATE_RECV_ACTIVITY_REQUEST;
            gPlayerCurrActivity = uroom->recvActivityRequest[0];
            return FALSE;
        }
        else if (id == 2) // No activity
        {
            uroom->state = UR_STATE_CANCEL_REQUEST_PRINT_MSG;
            SetCloseLinkCallback();
            return FALSE;
        }
    }

    return TRUE;
}

void InitUnionRoom(void)
{
    struct WirelessLink_URoom *data;

    sUnionRoomPlayerName[0] = EOS;
    CreateTask(Task_InitUnionRoom, 0);
    sWirelessLinkMain.uRoom = sWirelessLinkMain.uRoom; // Needed to match.
    sWirelessLinkMain.uRoom = data = AllocZeroed(sizeof(struct WirelessLink_URoom));
    sURoom = sWirelessLinkMain.uRoom;
    data->state = 0;
    data->textState = 0;
    data->unknown = 0;
    data->unreadPlayerId = 0;
    sUnionRoomPlayerName[0] = EOS;
}

static void Task_InitUnionRoom(u8 taskId)
{
    s32 i;
    u8 text[32];
    struct WirelessLink_URoom *data = sWirelessLinkMain.uRoom;

    switch (data->state)
    {
    case 0:
        data->state = 1;
        break;
    case 1:
        SetHostRfuGameData(ACTIVITY_SEARCH, 0, FALSE);
        SetWirelessCommType1();
        OpenLink();
        InitializeRfuLinkManager_EnterUnionRoom();
        RfuSetIgnoreError(TRUE);
        data->state = 2;
        break;
    case 2:
        data->incomingChildList = AllocZeroed(RFU_CHILD_MAX * sizeof(struct RfuIncomingPlayer));
        ClearIncomingPlayerList(data->incomingChildList, RFU_CHILD_MAX);
        data->incomingParentList = AllocZeroed(RFU_CHILD_MAX * sizeof(struct RfuIncomingPlayer));
        ClearIncomingPlayerList(data->incomingParentList, RFU_CHILD_MAX);
        data->playerList = AllocZeroed(MAX_UNION_ROOM_LEADERS * sizeof(struct RfuPlayer));
        ClearRfuPlayerList(data->playerList->players, MAX_UNION_ROOM_LEADERS);
        data->spawnPlayer = AllocZeroed(sizeof(struct RfuPlayer));
        ClearRfuPlayerList(&data->spawnPlayer->players[0], 1);
        data->searchTaskId = CreateTask_SearchForChildOrParent(data->incomingParentList, data->incomingChildList, LINK_GROUP_UNION_ROOM_INIT);
        data->state = 3;
        break;
    case 3:
        switch (HandlePlayerListUpdate())
        {
        case PLIST_NEW_PLAYER:
        case PLIST_RECENT_UPDATE:
            if (sUnionRoomPlayerName[0] == EOS)
            {
                for (i = 0; i < MAX_UNION_ROOM_LEADERS; i++)
                {
                    if (data->playerList->players[i].groupScheduledAnim == UNION_ROOM_SPAWN_IN)
                    {
                        CopyAndTranslatePlayerName(text, &data->playerList->players[i]);
                        if (PlayerHasMetTrainerBefore(ReadAsU16(data->playerList->players[i].rfu.data.compatibility.playerTrainerId), text))
                        {
                            StringCopy(sUnionRoomPlayerName, text);
                            break;
                        }
                    }
                }
            }
            break;
        case PLIST_UNUSED:
            break;
        }
        break;
    case 4:
        Free(data->spawnPlayer);
        Free(data->playerList);
        Free(data->incomingParentList);
        Free(data->incomingChildList);
        DestroyTask(data->searchTaskId);
        Free(sWirelessLinkMain.uRoom);
        LinkRfu_Shutdown();
        DestroyTask(taskId);
        break;
    }
}

bool16 BufferUnionRoomPlayerName(void)
{
    if (sUnionRoomPlayerName[0] != EOS)
    {
        StringCopy(gStringVar1, sUnionRoomPlayerName);
        sUnionRoomPlayerName[0] = EOS;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static u8 HandlePlayerListUpdate(void)
{
    s32 i;
    u8 j;
    struct WirelessLink_URoom *data = sWirelessLinkMain.uRoom;
    s32 retVal = PLIST_NONE;

    for (i = 0; i < RFU_CHILD_MAX; i++)
    {
        if (ArePlayersDifferent(&data->incomingParentList->players[i].rfu, &sUnionRoomPlayer_DummyRfu) == TRUE)
        {
            data->spawnPlayer->players[0].rfu = data->incomingParentList->players[i].rfu;
            data->spawnPlayer->players[0].timeoutCounter = 0;
            data->spawnPlayer->players[0].groupScheduledAnim = UNION_ROOM_SPAWN_IN;
            data->spawnPlayer->players[0].newPlayerCountdown = 1;
            return PLIST_CONTACTED;
        }
    }
    for (j = 0; j < MAX_UNION_ROOM_LEADERS; j++)
    {
        if (data->playerList->players[j].groupScheduledAnim != UNION_ROOM_SPAWN_NONE)
        {
            i = GetNewIncomingPlayerId(&data->playerList->players[j], &data->incomingChildList->players[0]);
            if (i != 0xFF)
            {
                if (data->playerList->players[j].groupScheduledAnim == UNION_ROOM_SPAWN_IN)
                {
                    if (ArePlayerDataDifferent(&data->playerList->players[j].rfu, &data->incomingChildList->players[i].rfu))
                    {
                        data->playerList->players[j].rfu = data->incomingChildList->players[i].rfu;
                        data->playerList->players[j].newPlayerCountdown = 64;
                        retVal = PLIST_NEW_PLAYER;
                    }
                    else if (data->playerList->players[j].newPlayerCountdown != 0)
                    {
                        data->playerList->players[j].newPlayerCountdown--;
                        if (data->playerList->players[j].newPlayerCountdown == 0)
                            retVal = PLIST_RECENT_UPDATE;
                    }
                }
                else
                {
                    data->playerList->players[j].groupScheduledAnim = UNION_ROOM_SPAWN_IN;
                    data->playerList->players[j].newPlayerCountdown = 0;
                    retVal = PLIST_RECENT_UPDATE;
                }
                data->playerList->players[j].timeoutCounter = 0;
            }
            else if (data->playerList->players[j].groupScheduledAnim != UNION_ROOM_SPAWN_OUT)
            {
                data->playerList->players[j].timeoutCounter++;
                if (data->playerList->players[j].timeoutCounter >= 600)
                {
                    data->playerList->players[j].groupScheduledAnim = UNION_ROOM_SPAWN_OUT;
                    retVal = PLIST_RECENT_UPDATE;
                }
            }
            else if (data->playerList->players[j].groupScheduledAnim == UNION_ROOM_SPAWN_OUT)
            {
                data->playerList->players[j].timeoutCounter++;
                if (data->playerList->players[j].timeoutCounter >= 900)
                    ClearRfuPlayerList(&data->playerList->players[j], 1);
            }
        }
    }
    for (i = 0; i < RFU_CHILD_MAX; i++)
        if (TryAddIncomingPlayerToList(&data->playerList->players[0], &data->incomingChildList->players[i], MAX_UNION_ROOM_LEADERS) != 0xFF)
            retVal = PLIST_NEW_PLAYER;

    return retVal;
}

static void Task_SearchForChildOrParent(u8 taskId)
{
    s32 i, j;
    struct RfuPlayerData rfu;
    struct RfuIncomingPlayerList **list = (void *) gTasks[taskId].data;
    bool8 isParent;

    for (i = 0; i < RFU_CHILD_MAX; i++)
    {
        isParent = Rfu_GetCompatiblePlayerData(&rfu.data, rfu.name, i);

        if (!IsPartnerActivityAcceptable(rfu.data.activity, gTasks[taskId].data[4]))
            rfu = sUnionRoomPlayer_DummyRfu;
        if (rfu.data.compatibility.language == LANGUAGE_JAPANESE)
            rfu = sUnionRoomPlayer_DummyRfu;

        if (!isParent)
        {
            for (j = 0; j < i; j++)
            {
                if (!ArePlayersDifferent(&list[1]->players[j].rfu, &rfu))
                    rfu = sUnionRoomPlayer_DummyRfu;
            }
            list[1]->players[i].rfu = rfu;
            list[1]->players[i].active = ArePlayersDifferent(&list[1]->players[i].rfu, &sUnionRoomPlayer_DummyRfu);
        }
        else
        {
            list[0]->players[i].rfu = rfu;
            list[0]->players[i].active = ArePlayersDifferent(&list[0]->players[i].rfu, &sUnionRoomPlayer_DummyRfu);
        }
    }
}

static u8 CreateTask_SearchForChildOrParent(struct RfuIncomingPlayerList * parentList, struct RfuIncomingPlayerList * childList, u32 linkGroup)
{
    u8 taskId = CreateTask(Task_SearchForChildOrParent, 0);
    struct RfuIncomingPlayerList ** data = (void *)gTasks[taskId].data;
    data[0] = parentList;
    data[1] = childList;
    gTasks[taskId].data[4] = linkGroup;
    return taskId;
}

static void Task_ListenForCompatiblePartners(u8 taskId)
{
    s32 i, j;
    struct RfuIncomingPlayerList **list = (void *) gTasks[taskId].data;

    for (i = 0; i < RFU_CHILD_MAX; i++)
    {
        Rfu_GetCompatiblePlayerData(&list[0]->players[i].rfu.data, list[0]->players[i].rfu.name, i);
        if (!IsPartnerActivityAcceptable(list[0]->players[i].rfu.data.activity, gTasks[taskId].data[2]))
        {
            list[0]->players[i].rfu = sUnionRoomPlayer_DummyRfu;
        }
        for (j = 0; j < i; j++)
        {
            if (!ArePlayersDifferent(&list[0]->players[j].rfu, &list[0]->players[i].rfu))
                list[0]->players[i].rfu = sUnionRoomPlayer_DummyRfu;
        }
        list[0]->players[i].active = ArePlayersDifferent(&list[0]->players[i].rfu, &sUnionRoomPlayer_DummyRfu);
    }
}

static bool32 HasWonderCardOrNewsByLinkGroup(struct RfuGameData *data, s16 linkGroup)
{
    if (linkGroup == LINK_GROUP_WONDER_CARD)
    {
        if (!data->compatibility.hasCard)
            return FALSE;
        else
            return TRUE;
    }
    else if (linkGroup == LINK_GROUP_WONDER_NEWS)
    {
        if (!data->compatibility.hasNews)
            return FALSE;
        else
            return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void Task_ListenForWonderDistributor(u8 taskId)
{
    s32 i;
    struct RfuIncomingPlayerList **list = (void *) gTasks[taskId].data;

    for (i = 0; i < RFU_CHILD_MAX; i++)
    {
        if (Rfu_GetWonderDistributorPlayerData(&list[0]->players[i].rfu.data, list[0]->players[i].rfu.name, i))
            HasWonderCardOrNewsByLinkGroup(&list[0]->players[i].rfu.data, gTasks[taskId].data[2]);

        list[0]->players[i].active = ArePlayersDifferent(&list[0]->players[i].rfu, &sUnionRoomPlayer_DummyRfu);
    }
}

static u8 CreateTask_ListenForCompatiblePartners(struct RfuIncomingPlayerList * list, u32 linkGroup)
{
    u8 taskId = CreateTask(Task_ListenForCompatiblePartners, 0);
    struct RfuIncomingPlayerList **oldList = (void *) gTasks[taskId].data;
    oldList[0] = list;
    gTasks[taskId].data[2] = linkGroup;
    return taskId;
}

static u8 CreateTask_ListenForWonderDistributor(struct RfuIncomingPlayerList * list, u32 linkGroup)
{
    u8 taskId = CreateTask(Task_ListenForWonderDistributor, 0);
    struct RfuIncomingPlayerList **oldList = (void *) gTasks[taskId].data;
    oldList[0] = list;
    gTasks[taskId].data[2] = linkGroup;
    return taskId;
}

static bool32 UR_PrintFieldMessage(const u8 *src)
{
    LoadMessageBoxAndBorderGfx();
    DrawDialogueFrame(0, TRUE);
    StringExpandPlaceholders(gStringVarBuffer, src);
    AddTextPrinterWithCustomSpeedForMessage(FALSE, 1);
    return FALSE;
}

static bool32 UR_RunTextPrinters(void)
{
    if (!RunTextPrintersAndIsPrinter0Active())
        return TRUE;
    else
        return FALSE;
}

static bool8 PrintOnTextbox(u8 *textState, const u8 *str)
{
    switch (*textState)
    {
    case 0:
        LoadMessageBoxAndBorderGfx();
        DrawDialogueFrame(0, TRUE);
        StringExpandPlaceholders(gStringVarBuffer, str);
        AddTextPrinterForMessage_2(TRUE);
        (*textState)++;
        break;
    case 1:
        if (!RunTextPrintersAndIsPrinter0Active())
        {
            *textState = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static s8 UnionRoomHandleYesNo(u8 *state, bool32 noDraw)
{
    s8 input;

    switch (*state)
    {
    case 0:
        if (noDraw)
            return -3;
        DisplayYesNoMenuDefaultYes();
        (*state)++;
        break;
    case 1:
        if (noDraw)
        {
            EraseYesNoWindow();
            *state = 0;
            return -3;
        }
        input = Menu_ProcessInputNoWrapClearOnChoose();
        if (input == MENU_B_PRESSED || input == 0 || input == 1)
        {
            *state = 0;
            return input;
        }
        break;
    }
    return MENU_NOTHING_CHOSEN;
}

static u8 CreateTradeBoardWindow(const struct WindowTemplate * template)
{
    u8 windowId = AddWindow(template);
    DrawStdWindowFrame(windowId, FALSE);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(15));
    PrintUnionRoomText(windowId, FONT_NORMAL, sText_NameWantedOfferLv, 8, 1, UR_COLOR_TRADE_BOARD_OTHER);
    CopyWindowToVram(windowId, COPYWIN_GFX);
    PutWindowTilemap(windowId);
    return windowId;
}

static void DeleteTradeBoardWindow(u8 windowId)
{
    RemoveWindow(windowId);
}

static s32 ListMenuHandler_AllItemsAvailable(u8 *state, u8 *windowId, u8 *listMenuId, const struct WindowTemplate *winTemplate, const struct ListMenuTemplate *menuTemplate)
{
    s32 maxWidth, input;
    struct WindowTemplate winTemplateCopy;

    switch (*state)
    {
    case 0:
        winTemplateCopy = *winTemplate;
        maxWidth = Intl_GetListMenuWidth(menuTemplate);
        if (winTemplateCopy.width > maxWidth)
            winTemplateCopy.width = maxWidth;

        if (winTemplateCopy.tilemapLeft + winTemplateCopy.width >= DISPLAY_TILE_WIDTH)
            winTemplateCopy.tilemapLeft = max(DISPLAY_TILE_WIDTH - 1 - winTemplateCopy.width, 0);

        *windowId = AddWindow(&winTemplateCopy);
        DrawStdWindowFrame(*windowId, FALSE);
        gMultiuseListMenuTemplate = *menuTemplate;
        gMultiuseListMenuTemplate.windowId = *windowId;
        *listMenuId = ListMenuInit(&gMultiuseListMenuTemplate, 0, 0);
        CopyWindowToVram(*windowId, COPYWIN_MAP);
        (*state)++;
        break;
    case 1:
        input = ListMenu_ProcessInput(*listMenuId);
        if (JOY_NEW(A_BUTTON))
        {
            DestroyListMenuTask(*listMenuId, NULL, NULL);
            ClearStdWindowAndFrame(*windowId, TRUE);
            RemoveWindow(*windowId);
            *state = 0;
            return input;
        }
        else if (JOY_NEW(B_BUTTON))
        {
            DestroyListMenuTask(*listMenuId, NULL, NULL);
            ClearStdWindowAndFrame(*windowId, TRUE);
            RemoveWindow(*windowId);
            *state = 0;
            return LIST_CANCEL;
        }
        break;
    }

    return LIST_NOTHING_CHOSEN;
}

static s32 TradeBoardMenuHandler(u8 *state, u8 *mainWindowId, u8 *listMenuId, u8 *headerWindowId,
                                const struct WindowTemplate *winTemplate,
                                const struct ListMenuTemplate *menuTemplate,
                                struct RfuPlayerList *list)
{
    s32 input;
    s32 idx;

    switch (*state)
    {
    case 0:
        *headerWindowId = CreateTradeBoardWindow(&sWindowTemplate_TradingBoardHeader);
        *mainWindowId = AddWindow(winTemplate);
        DrawStdWindowFrame(*mainWindowId, FALSE);
        gMultiuseListMenuTemplate = *menuTemplate;
        gMultiuseListMenuTemplate.windowId = *mainWindowId;
        *listMenuId = ListMenuInit(&gMultiuseListMenuTemplate, 0, 1);
        (*state)++;
        break;
    case 1:
        CopyWindowToVram(*mainWindowId, COPYWIN_MAP);
        (*state)++;
        break;
    case 2:
        input = ListMenu_ProcessInput(*listMenuId);
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            // Exit or B button
            if (input == 8 || JOY_NEW(B_BUTTON))
            {
                DestroyListMenuTask(*listMenuId, NULL, NULL);
                RemoveWindow(*mainWindowId);
                DeleteTradeBoardWindow(*headerWindowId);
                *state = 0;
                return LIST_CANCEL;
            }
            else
            {
                idx = GetIndexOfNthTradeBoardOffer(list->players, input);
                if (idx >= 0)
                {
                    DestroyListMenuTask(*listMenuId, NULL, NULL);
                    RemoveWindow(*mainWindowId);
                    DeleteTradeBoardWindow(*headerWindowId);
                    *state = 0;
                    return idx;
                }
                else
                {
                    PlaySE(SE_WALL_HIT);
                }
            }
        }
        break;
    }

    return LIST_NOTHING_CHOSEN;
}

static void UR_ClearBg0(void)
{
    FillBgTilemapBufferRect(0, 0, 0, 0, 32, 32, 0);
    CopyBgTilemapBufferToVram(0);
}

static void JoinGroup_EnableScriptContexts(void)
{
    ScriptContext_Enable();
}

static void PrintUnionRoomText(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, u8 colorIdx)
{
    struct TextPrinterTemplate printerTemplate;

    printerTemplate.currentChar = str;
    printerTemplate.windowId = windowId;
    printerTemplate.fontId = fontId;
    printerTemplate.x = x;
    printerTemplate.y = y;
    printerTemplate.currentX = x;
    printerTemplate.currentY = y;
    printerTemplate.unk = 0;

    gTextFlags.useAlternateDownArrow = FALSE;
    switch (colorIdx)
    {
    case UR_COLOR_DEFAULT:
        printerTemplate.letterSpacing = 0;
        printerTemplate.lineSpacing = 0;
        printerTemplate.fgColor = TEXT_COLOR_DARK_GRAY;
        printerTemplate.bgColor = TEXT_COLOR_WHITE;
        printerTemplate.shadowColor = TEXT_COLOR_LIGHT_GRAY;
        break;
    case UR_COLOR_RED:
        printerTemplate.letterSpacing = 0;
        printerTemplate.lineSpacing = 0;
        printerTemplate.fgColor = TEXT_COLOR_RED;
        printerTemplate.bgColor = TEXT_COLOR_WHITE;
        printerTemplate.shadowColor = TEXT_COLOR_LIGHT_RED;
        break;
    case UR_COLOR_GREEN:
        printerTemplate.letterSpacing = 0;
        printerTemplate.lineSpacing = 0;
        printerTemplate.fgColor = TEXT_COLOR_GREEN;
        printerTemplate.bgColor = TEXT_COLOR_WHITE;
        printerTemplate.shadowColor = TEXT_COLOR_LIGHT_GREEN;
        break;
    case UR_COLOR_WHITE:
        printerTemplate.letterSpacing = 0;
        printerTemplate.lineSpacing = 0;
        printerTemplate.fgColor = TEXT_COLOR_WHITE;
        printerTemplate.bgColor = TEXT_COLOR_WHITE;
        printerTemplate.shadowColor = TEXT_COLOR_LIGHT_GRAY;
        break;
    case UR_COLOR_CANCEL:
        printerTemplate.letterSpacing = 0;
        printerTemplate.lineSpacing = 0;
        printerTemplate.fgColor = TEXT_COLOR_WHITE;
        printerTemplate.bgColor = TEXT_COLOR_DARK_GRAY;
        printerTemplate.shadowColor = TEXT_COLOR_LIGHT_GRAY;
        break;
    case UR_COLOR_TRADE_BOARD_SELF:
        printerTemplate.letterSpacing = 0;
        printerTemplate.lineSpacing = 0;
        printerTemplate.fgColor = TEXT_COLOR_LIGHT_GREEN;
        printerTemplate.bgColor = TEXT_DYNAMIC_COLOR_6;
        printerTemplate.shadowColor = TEXT_COLOR_LIGHT_BLUE;
        break;
    case UR_COLOR_TRADE_BOARD_OTHER:
        printerTemplate.letterSpacing = 0;
        printerTemplate.lineSpacing = 0;
        printerTemplate.fgColor = TEXT_DYNAMIC_COLOR_5;
        printerTemplate.bgColor = TEXT_DYNAMIC_COLOR_6;
        printerTemplate.shadowColor = TEXT_COLOR_LIGHT_BLUE;
        break;
    }

    AddTextPrinter(&printerTemplate, TEXT_SKIP_DRAW, NULL);
}

static void ClearRfuPlayerList(struct RfuPlayer *players, u8 count)
{
    s32 i;

    for (i = 0; i < count; i++)
    {
        players[i].rfu = sUnionRoomPlayer_DummyRfu;
        players[i].timeoutCounter = 255;
        players[i].groupScheduledAnim = UNION_ROOM_SPAWN_NONE;
        players[i].useRedText = FALSE;
        players[i].newPlayerCountdown = 0;
    }
}

static void ClearIncomingPlayerList(struct RfuIncomingPlayerList *list, u8 count)
{
    s32 i;

    for (i = 0; i < RFU_CHILD_MAX; i++)
    {
        list->players[i].rfu = sUnionRoomPlayer_DummyRfu;
        list->players[i].active = FALSE;
    }
}

// Checks player name and trainer id, returns TRUE if they are not the same
static bool8 ArePlayersDifferent(struct RfuPlayerData* player1, const struct RfuPlayerData* player2)
{
    s32 i;

    for (i = 0; i < 2; i++)
    {
        if (player1->data.compatibility.playerTrainerId[i] != player2->data.compatibility.playerTrainerId[i])
            return TRUE;
    }

    for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
    {
        if (player1->name[i] != player2->name[i])
            return TRUE;
    }

    return FALSE;
}

static bool32 ArePlayerDataDifferent(struct RfuPlayerData *player1, struct RfuPlayerData *player2)
{
    s32 i;

    if (player1->data.activity != player2->data.activity)
        return TRUE;

    if (player1->data.startedActivity != player2->data.startedActivity)
        return TRUE;

    for (i = 0; i < RFU_CHILD_MAX; i++)
    {
        if (player1->data.partnerInfo[i] != player2->data.partnerInfo[i])
            return TRUE;
    }

    if (player1->data.tradeSpecies != player2->data.tradeSpecies)
        return TRUE;

    if (player1->data.tradeType != player2->data.tradeType)
        return TRUE;

    return FALSE;
}

static u32 GetNewIncomingPlayerId(struct RfuPlayer *player, struct RfuIncomingPlayer *incomingPlayer)
{
    u8 result = 0xFF; // None
    s32 i;

    for (i = 0; i < RFU_CHILD_MAX; i++)
    {
        if (incomingPlayer[i].active && !ArePlayersDifferent(&player->rfu, &incomingPlayer[i].rfu))
        {
            result = i;
            incomingPlayer[i].active = FALSE;
        }
    }

    return result;
}

static u8 TryAddIncomingPlayerToList(struct RfuPlayer *players, struct RfuIncomingPlayer *incomingPlayer, u8 max)
{
    s32 i;

    if (incomingPlayer->active)
    {
        for (i = 0; i < max; i++)
        {
            if (players[i].groupScheduledAnim == UNION_ROOM_SPAWN_NONE)
            {
                players[i].rfu = incomingPlayer->rfu;
                players[i].timeoutCounter = 0;
                players[i].groupScheduledAnim = UNION_ROOM_SPAWN_IN;
                players[i].newPlayerCountdown = 64;
                incomingPlayer->active = FALSE;
                return i;
            }
        }
    }

    return 0xFF;
}

static void PrintGroupMemberOnWindow(u8 windowId, u8 x, u8 y, struct RfuPlayer *player, u8 colorIdx, u8 id)
{
    u8 activity;
    u8 trainerId[6];

    ConvertIntToDecimalStringN(gStringVarBuffer, id + 1, STR_CONV_MODE_LEADING_ZEROS, 2);
    StringAppend(gStringVarBuffer, sText_Colon);
    PrintUnionRoomText(windowId, FONT_NORMAL, gStringVarBuffer, x, y, UR_COLOR_DEFAULT);
    x += 18;
    activity = player->rfu.data.activity;
    if (player->groupScheduledAnim == UNION_ROOM_SPAWN_IN && !(activity & IN_UNION_ROOM))
    {
        CopyAndTranslatePlayerName(gStringVarBuffer, player);
        PrintUnionRoomText(windowId, FONT_NORMAL, gStringVarBuffer, x, y, colorIdx);
        ConvertIntToDecimalStringN(trainerId, player->rfu.data.compatibility.playerTrainerId[0] | (player->rfu.data.compatibility.playerTrainerId[1] << 8), STR_CONV_MODE_LEADING_ZEROS, 5);
        StringCopy(gStringVarBuffer, sText_ID);
        StringAppend(gStringVarBuffer, trainerId);
        PrintUnionRoomText(windowId, FONT_NORMAL, gStringVarBuffer, GetStringRightAlignXOffset(FONT_NORMAL, gStringVarBuffer, 0x88), y, colorIdx);
    }
}

static void PrintGroupCandidateOnWindow(u8 windowId, u8 x, u8 y, struct RfuPlayer *player, u8 colorIdx, u8 id)
{
    u8 trainerId[6];

    if (player->groupScheduledAnim == UNION_ROOM_SPAWN_IN)
    {
        CopyAndTranslatePlayerName(gStringVarBuffer, player);
        PrintUnionRoomText(windowId, FONT_NORMAL, gStringVarBuffer, x, y, colorIdx);
        ConvertIntToDecimalStringN(trainerId, player->rfu.data.compatibility.playerTrainerId[0] | (player->rfu.data.compatibility.playerTrainerId[1] << 8), STR_CONV_MODE_LEADING_ZEROS, 5);
        StringCopy(gStringVarBuffer, sText_ID);
        StringAppend(gStringVarBuffer, trainerId);
        PrintUnionRoomText(windowId, FONT_NORMAL, gStringVarBuffer, GetStringRightAlignXOffset(FONT_NORMAL, gStringVarBuffer, 0x68), y, colorIdx);
    }
}

static bool32 IsPlayerFacingTradingBoard(void)
{
    s16 x, y;
    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);

    if (x != 2 + MAP_OFFSET)
        return FALSE;

    if (y != 1 + MAP_OFFSET)
        return FALSE;

    if (gPlayerAvatar.tileTransitionState == T_TILE_CENTER || gPlayerAvatar.tileTransitionState == T_NOT_MOVING)
        return TRUE;

    return FALSE;
}

static u32 GetResponseIdx_InviteToURoomActivity(s32 activity)
{
    switch (activity)
    {
    case ACTIVITY_CHAT:
        return 1;
    case ACTIVITY_TRADE:
        return 2;
    case ACTIVITY_CARD:
        return 3;
    case ACTIVITY_BATTLE_MULTI:
    default:
        return 0;
    }
}

static u32 ConvPartnerUnameAndGetWhetherMetAlready(struct RfuPlayer *player)
{
    u8 name[30];
    CopyAndTranslatePlayerName(name, player);
    return PlayerHasMetTrainerBefore(ReadAsU16(player->rfu.data.compatibility.playerTrainerId), name);
}

static s32 UnionRoomGetPlayerInteractionResponse(struct RfuPlayerList *list, bool8 overrideGender, u8 playerIdx, u32 playerGender)
{
    bool32 metBefore;

    struct RfuPlayer * player = &list->players[playerIdx];

    if (!player->rfu.data.startedActivity && !overrideGender)
    {
        CopyAndTranslatePlayerName(gStringVar1, player);
        metBefore = PlayerHasMetTrainerBefore(ReadAsU16(player->rfu.data.compatibility.playerTrainerId), gStringVar1);
        if (player->rfu.data.activity == (ACTIVITY_CHAT | IN_UNION_ROOM))
        {
            StringExpandPlaceholders(gStringVarBuffer, sJoinChatTexts[metBefore][playerGender]);
            return 2;
        }
        else
        {
            UR_PrintFieldMessage(sCommunicatingWaitTexts[metBefore]);
            return 1;
        }
    }
    else
    {
        CopyAndTranslatePlayerName(gStringVar1, player);
        if (overrideGender)
        {
            playerGender = (player->rfu.data.compatibility.playerTrainerId[overrideGender + 1] >> 3) & 1;
        }
        switch (player->rfu.data.activity & 0x3F)
        {
        case ACTIVITY_BATTLE_SINGLE:
            StringExpandPlaceholders(gStringVarBuffer, sBattleReactionTexts[playerGender][Random() % ARRAY_COUNT(sBattleReactionTexts[0])]);
            break;
        case ACTIVITY_TRADE:
            StringExpandPlaceholders(gStringVarBuffer, sTradeReactionTexts[playerGender][Random() % 2]);
            break;
        case ACTIVITY_CHAT:
            StringExpandPlaceholders(gStringVarBuffer, sChatReactionTexts[playerGender][Random() % ARRAY_COUNT(sChatReactionTexts[0])]);
            break;
        case ACTIVITY_CARD:
            StringExpandPlaceholders(gStringVarBuffer, sTrainerCardReactionTexts[playerGender][Random() % ARRAY_COUNT(sTrainerCardReactionTexts[0])]);
            break;
        default:
            StringExpandPlaceholders(gStringVarBuffer, sText_TrainerAppearsBusy);
            break;
        }
        return 0;
    }
}

static void ItemPrintFunc_EmptyList(u8 windowId, u32 itemId, u8 y)
{
}

static void TradeBoardPrintItemInfo(u8 windowId, u8 y, struct RfuGameData * data, const u8 * playerName, u8 colorIdx)
{
    u8 levelStr[4];
    u16 species = data->tradeSpecies;
    u8 type = data->tradeType;
    u8 level = data->tradeLevel;

    PrintUnionRoomText(windowId, FONT_NORMAL, playerName, 8, y, colorIdx);
    if (species == SPECIES_EGG)
    {
        PrintUnionRoomText(windowId, FONT_NORMAL, sText_EggTrade, 68, y, colorIdx);
    }
    else
    {
        BlitMenuInfoIcon(windowId, type + 1, 68, y);
        PrintUnionRoomText(windowId, FONT_NORMAL, gSpeciesNames[species], 118, y, colorIdx);
        ConvertIntToDecimalStringN(levelStr, level, STR_CONV_MODE_RIGHT_ALIGN, 3);
        PrintUnionRoomText(windowId, FONT_NORMAL, levelStr, 198, y, colorIdx);
    }
}

static void TradeBoardListMenuItemPrintFunc(u8 windowId, u32 itemId, u8 y)
{
    struct WirelessLink_Leader *leader = sWirelessLinkMain.leader;
    struct RfuGameData *gameData;
    s32 i, j;
    u8 playerName[RFU_USER_NAME_LENGTH + 1];

    if (itemId == LIST_HEADER && y == sTradeBoardListMenuTemplate.upText_Y)
    {
        gameData = GetHostRfuGameData();
        if (gameData->tradeSpecies != SPECIES_NONE)
            TradeBoardPrintItemInfo(windowId, y, gameData, gSaveBlock2Ptr->playerName, UR_COLOR_TRADE_BOARD_SELF);
    }
    else
    {
        j = 0;
        for (i = 0; i < MAX_UNION_ROOM_LEADERS; i++)
        {
            if (leader->playerList->players[i].groupScheduledAnim == UNION_ROOM_SPAWN_IN && leader->playerList->players[i].rfu.data.tradeSpecies != SPECIES_NONE)
                j++;

            if (j == itemId + 1)
            {
                CopyAndTranslatePlayerName(playerName, &leader->playerList->players[i]);
                TradeBoardPrintItemInfo(windowId, y, &leader->playerList->players[i].rfu.data, playerName, UR_COLOR_TRADE_BOARD_OTHER);
                break;
            }
        }
    }
}

static s32 GetIndexOfNthTradeBoardOffer(struct RfuPlayer * players, s32 n)
{
    s32 i;
    s32 j = 0;

    for (i = 0; i < MAX_UNION_ROOM_LEADERS; i++)
    {
        if (players[i].groupScheduledAnim == UNION_ROOM_SPAWN_IN && players[i].rfu.data.tradeSpecies != SPECIES_NONE)
            j++;

        if (j == n + 1)
            return i;
    }

    return -1;
}

static s32 GetUnionRoomPlayerGender(s32 playerIdx, struct RfuPlayerList *list)
{
    return list->players[playerIdx].rfu.data.playerGender;
}

static s32 IsRequestedTradeInPlayerParty(u32 type, u32 species)
{
    s32 i;

    if (species == SPECIES_EGG)
    {
        for (i = 0; i < gPlayerPartyCount; i++)
        {
            species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG);
            if (species == SPECIES_EGG)
                return UR_TRADE_MATCH;
        }
        return UR_TRADE_NOEGG;
    }
    else
    {
        for (i = 0; i < gPlayerPartyCount; i++)
        {
            species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG);
            if (gSpeciesInfo[species].types[0] == type || gSpeciesInfo[species].types[1] == type)
                return UR_TRADE_MATCH;
        }
        return UR_TRADE_NOTYPE;
    }
}

static void GetURoomActivityRejectMsg(u8 *dst, s32 acitivty, u32 playerGender)
{
    switch (acitivty)
    {
    case ACTIVITY_BATTLE_SINGLE | IN_UNION_ROOM:
        StringExpandPlaceholders(dst, sBattleDeclinedTexts[playerGender]);
        break;
    case ACTIVITY_CHAT | IN_UNION_ROOM:
        StringExpandPlaceholders(dst, sChatDeclinedTexts[playerGender]);
        break;
    case ACTIVITY_TRADE | IN_UNION_ROOM:
        StringExpandPlaceholders(dst, sText_TradeOfferRejected);
        break;
    case ACTIVITY_CARD | IN_UNION_ROOM:
        StringExpandPlaceholders(dst, sShowTrainerCardDeclinedTexts[playerGender]);
        break;
    }
}

static void GetURoomActivityStartMsg(u8 *dst, u8 acitivty)
{
    u8 mpId = GetMultiplayerId();
    u8 gender = gLinkPlayers[mpId ^ 1].gender;

    switch (acitivty)
    {
    case ACTIVITY_BATTLE_SINGLE | IN_UNION_ROOM:
        StringCopy(dst, sStartActivityTexts[mpId][gender][0]);
        break;
    case ACTIVITY_TRADE | IN_UNION_ROOM:
        StringCopy(dst, sStartActivityTexts[mpId][gender][2]);
        break;
    case ACTIVITY_CHAT | IN_UNION_ROOM:
        StringCopy(dst, sStartActivityTexts[mpId][gender][1]);
        break;
    }
}

static s32 GetChatLeaderActionRequestMessage(u8 *dst, u32 gender, u16 *activityData, struct WirelessLink_URoom *uroom)
{
    s32 result = 0;
    u16 species = SPECIES_NONE;
    s32 i;

    switch (activityData[0])
    {
    case ACTIVITY_BATTLE_SINGLE | IN_UNION_ROOM:
        StringExpandPlaceholders(dst, sText_BattleChallenge);
        result = 1;
        break;
    case ACTIVITY_CHAT | IN_UNION_ROOM:
        StringExpandPlaceholders(dst, sText_ChatInvitation);
        result = 1;
        break;
    case ACTIVITY_TRADE | IN_UNION_ROOM:
        ConvertIntToDecimalStringN(uroom->activityRequestStrbufs[0], sUnionRoomTrade.playerLevel, STR_CONV_MODE_LEFT_ALIGN, 3);
        StringCopy(uroom->activityRequestStrbufs[1], gSpeciesNames[sUnionRoomTrade.playerSpecies]);
        for (i = 0; i < RFU_CHILD_MAX; i++)
        {
            if (gRfuLinkStatus->partner[i].serialNo == RFU_SERIAL_GAME)
            {
                ConvertIntToDecimalStringN(uroom->activityRequestStrbufs[2], activityData[2], STR_CONV_MODE_LEFT_ALIGN, 3);
                StringCopy(uroom->activityRequestStrbufs[3], gSpeciesNames[activityData[1]]);
                species = activityData[1];
                break;
            }
        }
        if (species == SPECIES_EGG)
        {
            StringCopy(dst, sText_OfferToTradeEgg);
        }
        else
        {
            for (i = 0; i < RFU_CHILD_MAX; i++)
                DynamicPlaceholderTextUtil_SetPlaceholderPtr(i, uroom->activityRequestStrbufs[i]);
            DynamicPlaceholderTextUtil_ExpandPlaceholders(dst, sText_OfferToTradeMon);
        }
        result = 1;
        break;
    case ACTIVITY_CARD | IN_UNION_ROOM:
        StringExpandPlaceholders(dst, sText_ShowTrainerCard);
        result = 1;
        break;
    case ACTIVITY_NONE | IN_UNION_ROOM:
        StringExpandPlaceholders(dst, sText_ChatDropped);
        result = 2;
        break;
    }

    return result;
}

static bool32 PollPartnerYesNoResponse(struct WirelessLink_URoom *data)
{
    if (gRecvCmds[0][1] != 0)
    {
        if (gRecvCmds[0][1] == (ACTIVITY_ACCEPT | IN_UNION_ROOM))
        {
            data->partnerYesNoResponse = ACTIVITY_ACCEPT | IN_UNION_ROOM;
            return TRUE;
        }
        else if (gRecvCmds[0][1] == (ACTIVITY_DECLINE | IN_UNION_ROOM))
        {
            data->partnerYesNoResponse = ACTIVITY_DECLINE | IN_UNION_ROOM;
            return TRUE;
        }
    }
    return FALSE;
}

bool32 InUnionRoom(void)
{
    return gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(UNION_ROOM)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(UNION_ROOM)
        ? TRUE : FALSE;
}

static bool32 HasAtLeastTwoMonsOfLevel30OrLower(void)
{
    s32 i;
    s32 count = 0;

    for (i = 0; i < gPlayerPartyCount; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_LEVEL) <= UNION_ROOM_MAX_LEVEL
         && GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG) != SPECIES_EGG)
            count++;
    }

    if (count > 1)
        return TRUE;
    else
        return FALSE;
}

static void ResetUnionRoomTrade(struct UnionRoomTrade *trade)
{
    trade->state = URTRADE_STATE_NONE;
    trade->type = 0;
    trade->playerPersonality = 0;
    trade->playerSpecies = SPECIES_NONE;
    trade->playerLevel = 0;
    trade->species = SPECIES_NONE;
    trade->level = 0;
    trade->personality = 0;
}

void Script_ResetUnionRoomTrade(void)
{
    ResetUnionRoomTrade(&sUnionRoomTrade);
}

static bool32 RegisterTradeMonAndGetIsEgg(u32 monId, struct UnionRoomTrade *trade)
{
    trade->playerSpecies = GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES_OR_EGG);
    trade->playerLevel = GetMonData(&gPlayerParty[monId], MON_DATA_LEVEL);
    trade->playerPersonality = GetMonData(&gPlayerParty[monId], MON_DATA_PERSONALITY);
    if (trade->playerSpecies == SPECIES_EGG)
        return TRUE;
    else
        return FALSE;
}

static void RegisterTradeMon(u32 monId, struct UnionRoomTrade *trade)
{
    trade->species = GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES_OR_EGG);
    trade->level = GetMonData(&gPlayerParty[monId], MON_DATA_LEVEL);
    trade->personality = GetMonData(&gPlayerParty[monId], MON_DATA_PERSONALITY);
}

static u32 GetPartyPositionOfRegisteredMon(struct UnionRoomTrade *trade, u8 multiplayerId)
{
    u16 response = 0;
    u16 species;
    u32 personality;
    u32 cur_personality;
    u16 cur_species;
    s32 i;

    if (multiplayerId == 0)
    {
        species = trade->playerSpecies;
        personality = trade->playerPersonality;
    }
    else
    {
        species = trade->species;
        personality = trade->personality;
    }

    // Find party position by comparing to personality and species
    for (i = 0; i < gPlayerPartyCount; i++)
    {
        cur_personality = GetMonData(&gPlayerParty[i], MON_DATA_PERSONALITY);
        if (cur_personality != personality)
            continue;
        cur_species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG);
        if (cur_species != species)
            continue;
        response = i;
        break;
    }

    return response;
}

static void HandleCancelActivity(bool32 setData)
{
    UR_ClearBg0();
    UnlockPlayerFieldControls();
    UnionRoom_UnlockPlayerAndChatPartner();
    gPlayerCurrActivity = ACTIVITY_NONE;
    if (setData)
    {
        SetTradeBoardRegisteredMonInfo(sUnionRoomTrade.type, sUnionRoomTrade.playerSpecies, sUnionRoomTrade.playerLevel);
        UpdateGameData_SetActivity(ACTIVITY_NONE | IN_UNION_ROOM, 0, FALSE);
    }
}

static void StartScriptInteraction(void)
{
    LockPlayerFieldControls();
    FreezeObjects_WaitForPlayer();
}

static u8 GetActivePartnersInfo(struct WirelessLink_URoom *data)
{
    u8 retVal = PINFO_ACTIVE_FLAG;
    u8 i;

    for (i = 0; i < RFU_CHILD_MAX; i++)
    {
        if (data->incomingParentList->players[i].active)
        {
            retVal |= data->incomingParentList->players[i].rfu.data.playerGender << PINFO_GENDER_SHIFT;
            retVal |= data->incomingParentList->players[i].rfu.data.compatibility.playerTrainerId[0] & PINFO_TID_MASK;
            break;
        }
    }

    return retVal;
}

static void ViewURoomPartnerTrainerCard(u8 *unused, struct WirelessLink_URoom *data, bool8 isParent)
{
    struct TrainerCard *trainerCard = &gTrainerCards[GetMultiplayerId() ^ 1];
    s32 i;
    s32 n;

    DynamicPlaceholderTextUtil_Reset();

    StringCopy(data->trainerCardStrBuffer[0], gTrainerClassNames[GetUnionRoomTrainerClass()]);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, data->trainerCardStrBuffer[0]);

    DynamicPlaceholderTextUtil_SetPlaceholderPtr(1, trainerCard->playerName);

    StringCopy(data->trainerCardColorStrBuffer, sCardColorTexts[trainerCard->stars]);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(2, data->trainerCardColorStrBuffer);

    ConvertIntToDecimalStringN(data->trainerCardStrBuffer[2], trainerCard->caughtMonsCount, STR_CONV_MODE_LEFT_ALIGN, 3);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(3, data->trainerCardStrBuffer[2]);

    ConvertIntToDecimalStringN(data->trainerCardStrBuffer[3], trainerCard->playTimeHours, STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(data->trainerCardStrBuffer[4], trainerCard->playTimeMinutes, STR_CONV_MODE_LEADING_ZEROS, 2);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(4, data->trainerCardStrBuffer[3]);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(5, data->trainerCardStrBuffer[4]);

    DynamicPlaceholderTextUtil_ExpandPlaceholders(data->trainerCardMsgStrBuffer, sText_TrainerCardInfoPage1);
    StringCopy(gStringVarBuffer, data->trainerCardMsgStrBuffer);

    n = trainerCard->linkBattleWins;
    if (n > 9999)
        n = 9999;
    ConvertIntToDecimalStringN(data->trainerCardStrBuffer[0], n, STR_CONV_MODE_LEFT_ALIGN, 4);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, data->trainerCardStrBuffer[0]);

    n = trainerCard->linkBattleLosses;
    if (n > 9999)
        n = 9999;
    ConvertIntToDecimalStringN(data->trainerCardStrBuffer[1], n, STR_CONV_MODE_LEFT_ALIGN, 4);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(2, data->trainerCardStrBuffer[1]);

    ConvertIntToDecimalStringN(data->trainerCardStrBuffer[2], trainerCard->pokemonTrades, STR_CONV_MODE_LEFT_ALIGN, 5);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(3, data->trainerCardStrBuffer[2]);

    for (i = 0; i < TRAINER_CARD_PROFILE_LENGTH; i++)
    {
        CopyEasyChatWord(data->trainerCardStrBuffer[i + 3], trainerCard->easyChatProfile[i]);
        DynamicPlaceholderTextUtil_SetPlaceholderPtr(i + 4, data->trainerCardStrBuffer[i + 3]);
    }

    DynamicPlaceholderTextUtil_ExpandPlaceholders(data->trainerCardMsgStrBuffer, sText_TrainerCardInfoPage2);
    StringAppend(gStringVarBuffer, data->trainerCardMsgStrBuffer);

    if (isParent == TRUE)
    {
        DynamicPlaceholderTextUtil_ExpandPlaceholders(data->trainerCardMsgStrBuffer, sText_FinishedCheckingPlayersTrainerCard);
        StringAppend(gStringVarBuffer, data->trainerCardMsgStrBuffer);
    }
    else if (isParent == FALSE)
    {
        DynamicPlaceholderTextUtil_ExpandPlaceholders(data->trainerCardMsgStrBuffer, sGladToMeetYouTexts[trainerCard->gender]);
        StringAppend(gStringVarBuffer, data->trainerCardMsgStrBuffer);
    }
}

static void CopyAndTranslatePlayerName(u8 *dest, struct RfuPlayer *player)
{
    StringCopy_PlayerName(dest, player->rfu.name);
    ConvertInternationalString(dest, player->rfu.data.compatibility.language);
}
