#include "global.h"
#include "battle.h"
#include "battle_ai_script_commands.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_message.h"
#include "cable_club.h"
#include "link.h"
#include "link_rfu.h"
#include "party_menu.h"
#include "recorded_battle.h"
#include "task.h"
#include "util.h"
#include "constants/abilities.h"

static EWRAM_DATA u8 sLinkSendTaskId = 0;
static EWRAM_DATA u8 sLinkReceiveTaskId = 0;
static EWRAM_DATA u8 sUnused = 0; // Debug? Never read
EWRAM_DATA struct UnusedControllerStruct gUnusedControllerStruct = {}; // Debug? Unused code that writes to it, never read
static EWRAM_DATA u8 sBattleBuffersTransferData[0x100] = {};

static void CreateTasksForSendRecvLinkBuffers(void);
static void InitLinkBtlControllers(void);
static void InitSinglePlayerBtlControllers(void);
static void SetBattlePartyIds(void);
static void Task_HandleSendLinkBuffersData(u8 taskId);
static void Task_HandleCopyReceivedLinkBuffersData(u8 taskId);

void HandleLinkBattleSetup(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
    {
        if (gWirelessCommType)
            SetWirelessCommType1();
        if (!gReceivedRemoteLinkPlayers)
            OpenLink();
        CreateTask(Task_WaitForLinkPlayerConnection, 0);
        CreateTasksForSendRecvLinkBuffers();
    }
}

void SetUpBattleVarsAndBirchZigzagoon(void)
{
    s32 i;

    gBattleMainFunc = BeginBattleIntroDummy;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        gBattlerControllerFuncs[i] = BattleControllerDummy;
        gBattlerPositions[i] = 0xFF;
        gActionSelectionCursor[i] = 0;
        gMoveSelectionCursor[i] = 0;
    }

    HandleLinkBattleSetup();
    gBattleControllerExecFlags = 0;
    ClearBattleAnimationVars();
    ClearBattleMonForms();
    BattleAI_HandleItemUseBeforeAISetup(0xF);

    if (gBattleTypeFlags & BATTLE_TYPE_FIRST_BATTLE)
    {
        ZeroEnemyPartyMons();
        CreateMon(&gEnemyParty[0], SPECIES_ZIGZAGOON, 2, USE_RANDOM_IVS, 0, 0, OT_ID_PLAYER_ID, 0);
        i = 0;
        SetMonData(&gEnemyParty[0], MON_DATA_HELD_ITEM, &i);
    }

    // Below are never read
    gUnusedFirstBattleVar1 = 0;
    gUnusedFirstBattleVar2 = 0;
}

void InitBattleControllers(void)
{
    s32 i;

    if (!(gBattleTypeFlags & BATTLE_TYPE_RECORDED))
        RecordedBattle_Init(B_RECORD_MODE_RECORDING);
    else
        RecordedBattle_Init(B_RECORD_MODE_PLAYBACK);

    if (!(gBattleTypeFlags & BATTLE_TYPE_RECORDED))
        RecordedBattle_SaveParties();

    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        InitLinkBtlControllers();
    else
        InitSinglePlayerBtlControllers();

    SetBattlePartyIds();

    if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI))
    {
        for (i = 0; i < gBattlersCount; i++)
            BufferBattlePartyCurrentOrderBySide(i, 0);
    }

    for (i = 0; i < sizeof(gBattleStruct->tvMovePoints); i++)
        *((u8 *)(&gBattleStruct->tvMovePoints) + i) = 0;

    for (i = 0; i < sizeof(gBattleStruct->tv); i++)
        *((u8 *)(&gBattleStruct->tv) + i) = 0;
}

static void InitSinglePlayerBtlControllers(void)
{
    s32 i;

    if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
    {
        gBattleMainFunc = BeginBattleIntro;

        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
        {
            gBattlerControllerFuncs[0] = SetControllerToRecordedPlayer;
            gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;

            gBattlerControllerFuncs[1] = SetControllerToOpponent;
            gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;

            gBattlerControllerFuncs[2] = SetControllerToPlayerPartner;
            gBattlerPositions[2] = B_POSITION_PLAYER_RIGHT;

            gBattlerControllerFuncs[3] = SetControllerToOpponent;
            gBattlerPositions[3] = B_POSITION_OPPONENT_RIGHT;
        }
        else
        {
            gBattlerControllerFuncs[0] = SetControllerToPlayer;
            gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;

            gBattlerControllerFuncs[1] = SetControllerToOpponent;
            gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;

            gBattlerControllerFuncs[2] = SetControllerToPlayerPartner;
            gBattlerPositions[2] = B_POSITION_PLAYER_RIGHT;

            gBattlerControllerFuncs[3] = SetControllerToOpponent;
            gBattlerPositions[3] = B_POSITION_OPPONENT_RIGHT;
        }

        gBattlersCount = MAX_BATTLERS_COUNT;

        BufferBattlePartyCurrentOrderBySide(0, 0);
        BufferBattlePartyCurrentOrderBySide(1, 0);
        BufferBattlePartyCurrentOrderBySide(2, 1);
        BufferBattlePartyCurrentOrderBySide(3, 1);

        gBattlerPartyIndexes[0] = 0;
        gBattlerPartyIndexes[1] = 0;
        gBattlerPartyIndexes[2] = 3;
        gBattlerPartyIndexes[3] = 3;
    }
    else if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
    {
        gBattleMainFunc = BeginBattleIntro;

        if (gBattleTypeFlags & BATTLE_TYPE_SAFARI)
            gBattlerControllerFuncs[0] = SetControllerToSafari;
        else if (gBattleTypeFlags & BATTLE_TYPE_WALLY_TUTORIAL)
            gBattlerControllerFuncs[0] = SetControllerToWally;
        else
            gBattlerControllerFuncs[0] = SetControllerToPlayer;

        gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;

        gBattlerControllerFuncs[1] = SetControllerToOpponent;
        gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;

        gBattlersCount = 2;

        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
        {
            if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
            {
                if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_IS_MASTER)
                {
                    gBattleMainFunc = BeginBattleIntro;

                    gBattlerControllerFuncs[0] = SetControllerToRecordedPlayer;
                    gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;

                    gBattlerControllerFuncs[1] = SetControllerToRecordedOpponent;
                    gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;

                    gBattlersCount = 2;
                }
                else // see how the banks are switched
                {
                    gBattlerControllerFuncs[1] = SetControllerToRecordedPlayer;
                    gBattlerPositions[1] = B_POSITION_PLAYER_LEFT;

                    gBattlerControllerFuncs[0] = SetControllerToRecordedOpponent;
                    gBattlerPositions[0] = B_POSITION_OPPONENT_LEFT;

                    gBattlersCount = 2;
                }
            }
            else
            {
                gBattlerControllerFuncs[0] = SetControllerToRecordedPlayer;
                gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;

                gBattlerControllerFuncs[1] = SetControllerToOpponent;
                gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;
            }
        }
    }
    else
    {
        gBattleMainFunc = BeginBattleIntro;

        gBattlerControllerFuncs[0] = SetControllerToPlayer;
        gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;

        gBattlerControllerFuncs[1] = SetControllerToOpponent;
        gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;

        gBattlerControllerFuncs[2] = SetControllerToPlayer;
        gBattlerPositions[2] = B_POSITION_PLAYER_RIGHT;

        gBattlerControllerFuncs[3] = SetControllerToOpponent;
        gBattlerPositions[3] = B_POSITION_OPPONENT_RIGHT;

        gBattlersCount = MAX_BATTLERS_COUNT;

        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
        {
            if (gBattleTypeFlags & BATTLE_TYPE_MULTI && gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER)
            {
                gBattleMainFunc = BeginBattleIntro;

                gBattlerControllerFuncs[0] = SetControllerToRecordedPlayer;
                gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;

                gBattlerControllerFuncs[1] = SetControllerToOpponent;
                gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;

                gBattlerControllerFuncs[2] = SetControllerToRecordedPlayer;
                gBattlerPositions[2] = B_POSITION_PLAYER_RIGHT;

                gBattlerControllerFuncs[3] = SetControllerToOpponent;
                gBattlerPositions[3] = B_POSITION_OPPONENT_RIGHT;

                gBattlersCount = MAX_BATTLERS_COUNT;

                BufferBattlePartyCurrentOrderBySide(0, 0);
                BufferBattlePartyCurrentOrderBySide(1, 0);
                BufferBattlePartyCurrentOrderBySide(2, 1);
                BufferBattlePartyCurrentOrderBySide(3, 1);

                gBattlerPartyIndexes[0] = 0;
                gBattlerPartyIndexes[1] = 0;
                gBattlerPartyIndexes[2] = 3;
                gBattlerPartyIndexes[3] = 3;
            }
            else if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
            {
                u8 multiplayerId;

                for (multiplayerId = gRecordedBattleMultiplayerId, i = 0; i < MAX_BATTLERS_COUNT; i++)
                {
                    switch (gLinkPlayers[i].id)
                    {
                    case 0:
                    case 3:
                        BufferBattlePartyCurrentOrderBySide(gLinkPlayers[i].id, 0);
                        break;
                    case 1:
                    case 2:
                        BufferBattlePartyCurrentOrderBySide(gLinkPlayers[i].id, 1);
                        break;
                    }

                    if (i == multiplayerId)
                    {
                        gBattlerControllerFuncs[gLinkPlayers[i].id] = SetControllerToRecordedPlayer;
                        switch (gLinkPlayers[i].id)
                        {
                        case 0:
                        case 3:
                            gBattlerPositions[gLinkPlayers[i].id] = B_POSITION_PLAYER_LEFT;
                            gBattlerPartyIndexes[gLinkPlayers[i].id] = 0;
                            break;
                        case 1:
                        case 2:
                            gBattlerPositions[gLinkPlayers[i].id] = B_POSITION_PLAYER_RIGHT;
                            gBattlerPartyIndexes[gLinkPlayers[i].id] = 3;
                            break;
                        }
                    }
                    else if ((!(gLinkPlayers[i].id & 1) && !(gLinkPlayers[multiplayerId].id & 1))
                            || ((gLinkPlayers[i].id & 1) && (gLinkPlayers[multiplayerId].id & 1)))
                    {
                        gBattlerControllerFuncs[gLinkPlayers[i].id] = SetControllerToRecordedPlayer;
                        switch (gLinkPlayers[i].id)
                        {
                        case 0:
                        case 3:
                            gBattlerPositions[gLinkPlayers[i].id] = B_POSITION_PLAYER_LEFT;
                            gBattlerPartyIndexes[gLinkPlayers[i].id] = 0;
                            break;
                        case 1:
                        case 2:
                            gBattlerPositions[gLinkPlayers[i].id] = B_POSITION_PLAYER_RIGHT;
                            gBattlerPartyIndexes[gLinkPlayers[i].id] = 3;
                            break;
                        }
                    }
                    else
                    {
                        gBattlerControllerFuncs[gLinkPlayers[i].id] = SetControllerToRecordedOpponent;
                        switch (gLinkPlayers[i].id)
                        {
                        case 0:
                        case 3:
                            gBattlerPositions[gLinkPlayers[i].id] = B_POSITION_OPPONENT_LEFT;
                            gBattlerPartyIndexes[gLinkPlayers[i].id] = 0;
                            break;
                        case 1:
                        case 2:
                            gBattlerPositions[gLinkPlayers[i].id] = B_POSITION_OPPONENT_RIGHT;
                            gBattlerPartyIndexes[gLinkPlayers[i].id] = 3;
                            break;
                        }
                    }
                }
            }
            else if (gBattleTypeFlags & BATTLE_TYPE_IS_MASTER)
            {
                gBattlerControllerFuncs[0] = SetControllerToRecordedPlayer;
                gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;

                gBattlerControllerFuncs[2] = SetControllerToRecordedPlayer;
                gBattlerPositions[2] = B_POSITION_PLAYER_RIGHT;

                if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
                {
                  gBattlerControllerFuncs[1] = SetControllerToRecordedOpponent;
                  gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;

                  gBattlerControllerFuncs[3] = SetControllerToRecordedOpponent;
                  gBattlerPositions[3] = B_POSITION_OPPONENT_RIGHT;
                }
                else
                {
                  gBattlerControllerFuncs[1] = SetControllerToOpponent;
                  gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;

                  gBattlerControllerFuncs[3] = SetControllerToOpponent;
                  gBattlerPositions[3] = B_POSITION_OPPONENT_RIGHT;
                }
            }
            else
            {
                gBattlerControllerFuncs[1] = SetControllerToRecordedPlayer;
                gBattlerPositions[1] = B_POSITION_PLAYER_LEFT;

                gBattlerControllerFuncs[3] = SetControllerToRecordedPlayer;
                gBattlerPositions[3] = B_POSITION_PLAYER_RIGHT;

                if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
                {
                    gBattlerControllerFuncs[0] = SetControllerToRecordedOpponent;
                    gBattlerPositions[0] = B_POSITION_OPPONENT_LEFT;

                    gBattlerControllerFuncs[2] = SetControllerToRecordedOpponent;
                    gBattlerPositions[2] = B_POSITION_OPPONENT_RIGHT;
                }
                else
                {
                    gBattlerControllerFuncs[0] = SetControllerToOpponent;
                    gBattlerPositions[0] = B_POSITION_OPPONENT_LEFT;

                    gBattlerControllerFuncs[2] = SetControllerToOpponent;
                    gBattlerPositions[2] = B_POSITION_OPPONENT_RIGHT;
                }
            }
        }
    }
}

static void InitLinkBtlControllers(void)
{
    s32 i;
    u8 multiplayerId;

    if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
    {
        if (gBattleTypeFlags & BATTLE_TYPE_IS_MASTER)
        {
            gBattleMainFunc = BeginBattleIntro;

            gBattlerControllerFuncs[0] = SetControllerToPlayer;
            gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;

            gBattlerControllerFuncs[1] = SetControllerToLinkOpponent;
            gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;

            gBattlersCount = 2;
        }
        else
        {
            gBattlerControllerFuncs[1] = SetControllerToPlayer;
            gBattlerPositions[1] = B_POSITION_PLAYER_LEFT;

            gBattlerControllerFuncs[0] = SetControllerToLinkOpponent;
            gBattlerPositions[0] = B_POSITION_OPPONENT_LEFT;

            gBattlersCount = 2;
        }
    }
    else if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI) && gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_IS_MASTER)
        {
            gBattleMainFunc = BeginBattleIntro;

            gBattlerControllerFuncs[0] = SetControllerToPlayer;
            gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;

            gBattlerControllerFuncs[1] = SetControllerToLinkOpponent;
            gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;

            gBattlerControllerFuncs[2] = SetControllerToPlayer;
            gBattlerPositions[2] = B_POSITION_PLAYER_RIGHT;

            gBattlerControllerFuncs[3] = SetControllerToLinkOpponent;
            gBattlerPositions[3] = B_POSITION_OPPONENT_RIGHT;

            gBattlersCount = MAX_BATTLERS_COUNT;
        }
        else
        {
            gBattlerControllerFuncs[1] = SetControllerToPlayer;
            gBattlerPositions[1] = B_POSITION_PLAYER_LEFT;

            gBattlerControllerFuncs[0] = SetControllerToLinkOpponent;
            gBattlerPositions[0] = B_POSITION_OPPONENT_LEFT;

            gBattlerControllerFuncs[3] = SetControllerToPlayer;
            gBattlerPositions[3] = B_POSITION_PLAYER_RIGHT;

            gBattlerControllerFuncs[2] = SetControllerToLinkOpponent;
            gBattlerPositions[2] = B_POSITION_OPPONENT_RIGHT;

            gBattlersCount = MAX_BATTLERS_COUNT;
        }
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_IS_MASTER)
        {
            gBattleMainFunc = BeginBattleIntro;

            gBattlerControllerFuncs[0] = SetControllerToPlayer;
            gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;

            gBattlerControllerFuncs[1] = SetControllerToOpponent;
            gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;

            gBattlerControllerFuncs[2] = SetControllerToLinkPartner;
            gBattlerPositions[2] = B_POSITION_PLAYER_RIGHT;

            gBattlerControllerFuncs[3] = SetControllerToOpponent;
            gBattlerPositions[3] = B_POSITION_OPPONENT_RIGHT;

            gBattlersCount = MAX_BATTLERS_COUNT;
        }
        else
        {
            gBattlerControllerFuncs[0] = SetControllerToLinkPartner;
            gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;

            gBattlerControllerFuncs[1] = SetControllerToLinkOpponent;
            gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;

            gBattlerControllerFuncs[2] = SetControllerToPlayer;
            gBattlerPositions[2] = B_POSITION_PLAYER_RIGHT;

            gBattlerControllerFuncs[3] = SetControllerToLinkOpponent;
            gBattlerPositions[3] = B_POSITION_OPPONENT_RIGHT;

            gBattlersCount = MAX_BATTLERS_COUNT;
        }

        BufferBattlePartyCurrentOrderBySide(0, 0);
        BufferBattlePartyCurrentOrderBySide(1, 0);
        BufferBattlePartyCurrentOrderBySide(2, 1);
        BufferBattlePartyCurrentOrderBySide(3, 1);
        gBattlerPartyIndexes[0] = 0;
        gBattlerPartyIndexes[1] = 0;
        gBattlerPartyIndexes[2] = 3;
        gBattlerPartyIndexes[3] = 3;
    }
    else
    {
        multiplayerId = GetMultiplayerId();

        if (gBattleTypeFlags & BATTLE_TYPE_IS_MASTER)
            gBattleMainFunc = BeginBattleIntro;

        for (i = 0; i < MAX_BATTLERS_COUNT; i++)
        {
            switch (gLinkPlayers[i].id)
            {
            case 0:
            case 3:
                BufferBattlePartyCurrentOrderBySide(gLinkPlayers[i].id, 0);
                break;
            case 1:
            case 2:
                BufferBattlePartyCurrentOrderBySide(gLinkPlayers[i].id, 1);
                break;
            }

            if (i == multiplayerId)
            {
                gBattlerControllerFuncs[gLinkPlayers[i].id] = SetControllerToPlayer;
                switch (gLinkPlayers[i].id)
                {
                case 0:
                case 3:
                    gBattlerPositions[gLinkPlayers[i].id] = B_POSITION_PLAYER_LEFT;
                    gBattlerPartyIndexes[gLinkPlayers[i].id] = 0;
                    break;
                case 1:
                case 2:
                    gBattlerPositions[gLinkPlayers[i].id] = B_POSITION_PLAYER_RIGHT;
                    gBattlerPartyIndexes[gLinkPlayers[i].id] = 3;
                    break;
                }
            }
            else
            {
                if ((!(gLinkPlayers[i].id & 1) && !(gLinkPlayers[multiplayerId].id & 1))
                 || ((gLinkPlayers[i].id & 1) && (gLinkPlayers[multiplayerId].id & 1)))
                {
                    gBattlerControllerFuncs[gLinkPlayers[i].id] = SetControllerToLinkPartner;
                    switch (gLinkPlayers[i].id)
                    {
                    case 0:
                    case 3:
                        gBattlerPositions[gLinkPlayers[i].id] = B_POSITION_PLAYER_LEFT;
                        gBattlerPartyIndexes[gLinkPlayers[i].id] = 0;
                        break;
                    case 1:
                    case 2:
                        gBattlerPositions[gLinkPlayers[i].id] = B_POSITION_PLAYER_RIGHT;
                        gBattlerPartyIndexes[gLinkPlayers[i].id] = 3;
                        break;
                    }
                }
                else
                {
                    gBattlerControllerFuncs[gLinkPlayers[i].id] = SetControllerToLinkOpponent;
                    switch (gLinkPlayers[i].id)
                    {
                    case 0:
                    case 3:
                        gBattlerPositions[gLinkPlayers[i].id] = B_POSITION_OPPONENT_LEFT;
                        gBattlerPartyIndexes[gLinkPlayers[i].id] = 0;
                        break;
                    case 1:
                    case 2:
                        gBattlerPositions[gLinkPlayers[i].id] = B_POSITION_OPPONENT_RIGHT;
                        gBattlerPartyIndexes[gLinkPlayers[i].id] = 3;
                        break;
                    }
                }
            }
        }

        gBattlersCount = MAX_BATTLERS_COUNT;
    }
}

static void SetBattlePartyIds(void)
{
    s32 i, j;

    if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI))
    {
        for (i = 0; i < gBattlersCount; i++)
        {
            for (j = 0; j < PARTY_SIZE; j++)
            {
                if (i < 2)
                {
                    if (GET_BATTLER_SIDE2(i) == B_SIDE_PLAYER)
                    {
                        if (GetMonData(&gPlayerParty[j], MON_DATA_HP) != 0
                         && GetMonData(&gPlayerParty[j], MON_DATA_SPECIES_OR_EGG) != SPECIES_NONE
                         && GetMonData(&gPlayerParty[j], MON_DATA_SPECIES_OR_EGG) != SPECIES_EGG
                         && !GetMonData(&gPlayerParty[j], MON_DATA_IS_EGG))
                        {
                            gBattlerPartyIndexes[i] = j;
                            break;
                        }
                    }
                    else
                    {
                        if (GetMonData(&gEnemyParty[j], MON_DATA_HP) != 0
                         && GetMonData(&gEnemyParty[j], MON_DATA_SPECIES_OR_EGG) != SPECIES_NONE
                         && GetMonData(&gEnemyParty[j], MON_DATA_SPECIES_OR_EGG) != SPECIES_EGG
                         && !GetMonData(&gEnemyParty[j], MON_DATA_IS_EGG))
                        {
                            gBattlerPartyIndexes[i] = j;
                            break;
                        }
                    }
                }
                else
                {
                    if (GET_BATTLER_SIDE2(i) == B_SIDE_PLAYER)
                    {
                        if (GetMonData(&gPlayerParty[j], MON_DATA_HP) != 0
                         && GetMonData(&gPlayerParty[j], MON_DATA_SPECIES) != SPECIES_NONE  // Probably a typo by Game Freak. The rest use SPECIES2.
                         && GetMonData(&gPlayerParty[j], MON_DATA_SPECIES_OR_EGG) != SPECIES_EGG
                         && !GetMonData(&gPlayerParty[j], MON_DATA_IS_EGG)
                         && gBattlerPartyIndexes[i - 2] != j)
                        {
                            gBattlerPartyIndexes[i] = j;
                            break;
                        }
                    }
                    else
                    {
                        if (GetMonData(&gEnemyParty[j], MON_DATA_HP) != 0
                         && GetMonData(&gEnemyParty[j], MON_DATA_SPECIES_OR_EGG) != SPECIES_NONE
                         && GetMonData(&gEnemyParty[j], MON_DATA_SPECIES_OR_EGG) != SPECIES_EGG
                         && !GetMonData(&gEnemyParty[j], MON_DATA_IS_EGG)
                         && gBattlerPartyIndexes[i - 2] != j)
                        {
                            gBattlerPartyIndexes[i] = j;
                            break;
                        }
                    }
                }
            }
        }

        if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
            gBattlerPartyIndexes[1] = 0, gBattlerPartyIndexes[3] = 3;
    }
}

static void PrepareBufferDataTransfer(u8 bufferId, u8 *data, u16 size)
{
    s32 i;

    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
    {
        PrepareBufferDataTransferLink(bufferId, size, data);
    }
    else
    {
        switch (bufferId)
        {
        case BUFFER_A:
            for (i = 0; i < size; data++, i++)
                gBattleBufferA[gActiveBattler][i] = *data;
            break;
        case BUFFER_B:
            for (i = 0; i < size; data++, i++)
                gBattleBufferB[gActiveBattler][i] = *data;
            break;
        }
    }
}

static void CreateTasksForSendRecvLinkBuffers(void)
{
    sLinkSendTaskId = CreateTask(Task_HandleSendLinkBuffersData, 0);
    gTasks[sLinkSendTaskId].data[11] = 0;
    gTasks[sLinkSendTaskId].data[12] = 0;
    gTasks[sLinkSendTaskId].data[13] = 0;
    gTasks[sLinkSendTaskId].data[14] = 0;
    gTasks[sLinkSendTaskId].data[15] = 0;

    sLinkReceiveTaskId = CreateTask(Task_HandleCopyReceivedLinkBuffersData, 0);
    gTasks[sLinkReceiveTaskId].data[12] = 0;
    gTasks[sLinkReceiveTaskId].data[13] = 0;
    gTasks[sLinkReceiveTaskId].data[14] = 0;
    gTasks[sLinkReceiveTaskId].data[15] = 0;

    sUnused = 0;
}

enum
{
    LINK_BUFF_BUFFER_ID,
    LINK_BUFF_ACTIVE_BATTLER,
    LINK_BUFF_ATTACKER,
    LINK_BUFF_TARGET,
    LINK_BUFF_SIZE_LO,
    LINK_BUFF_SIZE_HI,
    LINK_BUFF_ABSENT_BATTLER_FLAGS,
    LINK_BUFF_EFFECT_BATTLER,
    LINK_BUFF_DATA,
};

void PrepareBufferDataTransferLink(u8 bufferId, u16 size, u8 *data)
{
    s32 alignedSize;
    s32 i;

    alignedSize = size - size % 4 + 4;
    if (gTasks[sLinkSendTaskId].data[14] + alignedSize + LINK_BUFF_DATA + 1 > BATTLE_BUFFER_LINK_SIZE)
    {
        gTasks[sLinkSendTaskId].data[12] = gTasks[sLinkSendTaskId].data[14];
        gTasks[sLinkSendTaskId].data[14] = 0;
    }
    gLinkBattleSendBuffer[gTasks[sLinkSendTaskId].data[14] + LINK_BUFF_BUFFER_ID] = bufferId;
    gLinkBattleSendBuffer[gTasks[sLinkSendTaskId].data[14] + LINK_BUFF_ACTIVE_BATTLER] = gActiveBattler;
    gLinkBattleSendBuffer[gTasks[sLinkSendTaskId].data[14] + LINK_BUFF_ATTACKER] = gBattlerAttacker;
    gLinkBattleSendBuffer[gTasks[sLinkSendTaskId].data[14] + LINK_BUFF_TARGET] = gBattlerTarget;
    gLinkBattleSendBuffer[gTasks[sLinkSendTaskId].data[14] + LINK_BUFF_SIZE_LO] = alignedSize;
    gLinkBattleSendBuffer[gTasks[sLinkSendTaskId].data[14] + LINK_BUFF_SIZE_HI] = (alignedSize & 0x0000FF00) >> 8;
    gLinkBattleSendBuffer[gTasks[sLinkSendTaskId].data[14] + LINK_BUFF_ABSENT_BATTLER_FLAGS] = gAbsentBattlerFlags;
    gLinkBattleSendBuffer[gTasks[sLinkSendTaskId].data[14] + LINK_BUFF_EFFECT_BATTLER] = gEffectBattler;

    for (i = 0; i < size; i++)
        gLinkBattleSendBuffer[gTasks[sLinkSendTaskId].data[14] + LINK_BUFF_DATA + i] = data[i];

    gTasks[sLinkSendTaskId].data[14] = gTasks[sLinkSendTaskId].data[14] + alignedSize + LINK_BUFF_DATA;
}

static void Task_HandleSendLinkBuffersData(u8 taskId)
{
    u16 numPlayers;
    u16 blockSize;

    switch (gTasks[taskId].data[11])
    {
    case 0:
        gTasks[taskId].data[10] = 100;
        gTasks[taskId].data[11]++;
        break;
    case 1:
        gTasks[taskId].data[10]--;
        if (gTasks[taskId].data[10] == 0)
            gTasks[taskId].data[11]++;
        break;
    case 2:
        if (gWirelessCommType)
        {
            gTasks[taskId].data[11]++;
        }
        else
        {
            if (gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER)
                numPlayers = 2;
            else
                numPlayers = (gBattleTypeFlags & BATTLE_TYPE_MULTI) ? 4 : 2;

            if (GetLinkPlayerCount_2() >= numPlayers)
            {
                if (IsLinkMaster())
                {
                    CheckShouldAdvanceLinkState();
                    gTasks[taskId].data[11]++;
                }
                else
                {
                    gTasks[taskId].data[11]++;
                }
            }
        }
        break;
    case 3:
        if (gTasks[taskId].data[15] != gTasks[taskId].data[14])
        {
            if (gTasks[taskId].data[13] == 0)
            {
                if (gTasks[taskId].data[15] > gTasks[taskId].data[14]
                 && gTasks[taskId].data[15] == gTasks[taskId].data[12])
                {
                    gTasks[taskId].data[12] = 0;
                    gTasks[taskId].data[15] = 0;
                }
                blockSize = (gLinkBattleSendBuffer[gTasks[taskId].data[15] + LINK_BUFF_SIZE_LO] | (gLinkBattleSendBuffer[gTasks[taskId].data[15] + LINK_BUFF_SIZE_HI] << 8)) + LINK_BUFF_DATA;
                SendBlock(BitmaskAllOtherLinkPlayers(), &gLinkBattleSendBuffer[gTasks[taskId].data[15]], blockSize);
                gTasks[taskId].data[11]++;
            }
            else
            {
                gTasks[taskId].data[13]--;
                break;
            }
        }
        break;
    case 4:
        if (IsLinkTaskFinished())
        {
            blockSize = gLinkBattleSendBuffer[gTasks[taskId].data[15] + LINK_BUFF_SIZE_LO] | (gLinkBattleSendBuffer[gTasks[taskId].data[15] + LINK_BUFF_SIZE_HI] << 8);
            gTasks[taskId].data[13] = 1;
            gTasks[taskId].data[15] = gTasks[taskId].data[15] + blockSize + LINK_BUFF_DATA;
            gTasks[taskId].data[11] = 3;
        }
        break;
    case 5:
        if (--gTasks[taskId].data[13] == 0)
        {
            gTasks[taskId].data[13] = 1;
            gTasks[taskId].data[11] = 3;
        }
        break;
    }
}

void TryReceiveLinkBattleData(void)
{
    u8 i;
    s32 j;
    u8 *recvBuffer;

    if (gReceivedRemoteLinkPlayers && (gBattleTypeFlags & BATTLE_TYPE_LINK_IN_BATTLE))
    {
        DestroyTask_RfuIdle();
        for (i = 0; i < GetLinkPlayerCount(); i++)
        {
            if (GetBlockReceivedStatus() & gBitTable[i])
            {
                ResetBlockReceivedFlag(i);
                recvBuffer = (u8 *)gBlockRecvBuffer[i];
                {
                    u8 *dest, *src;
                    u16 dataSize = gBlockRecvBuffer[i][2];

                    if (gTasks[sLinkReceiveTaskId].data[14] + 9 + dataSize > 0x1000)
                    {
                        gTasks[sLinkReceiveTaskId].data[12] = gTasks[sLinkReceiveTaskId].data[14];
                        gTasks[sLinkReceiveTaskId].data[14] = 0;
                    }

                    dest = &gLinkBattleRecvBuffer[gTasks[sLinkReceiveTaskId].data[14]];
                    src = recvBuffer;

                    for (j = 0; j < dataSize + 8; j++)
                        dest[j] = src[j];

                    gTasks[sLinkReceiveTaskId].data[14] = gTasks[sLinkReceiveTaskId].data[14] + dataSize + 8;
                }
            }
        }
    }
}

static void Task_HandleCopyReceivedLinkBuffersData(u8 taskId)
{
    u16 blockSize;
    u8 battlerId;
    u8 var;

    if (gTasks[taskId].data[15] != gTasks[taskId].data[14])
    {
        if (gTasks[taskId].data[15] > gTasks[taskId].data[14]
         && gTasks[taskId].data[15] == gTasks[taskId].data[12])
        {
            gTasks[taskId].data[12] = 0;
            gTasks[taskId].data[15] = 0;
        }
        battlerId = gLinkBattleRecvBuffer[gTasks[taskId].data[15] + LINK_BUFF_ACTIVE_BATTLER];
        blockSize = gLinkBattleRecvBuffer[gTasks[taskId].data[15] + LINK_BUFF_SIZE_LO] | (gLinkBattleRecvBuffer[gTasks[taskId].data[15] + LINK_BUFF_SIZE_HI] << 8);

        switch (gLinkBattleRecvBuffer[gTasks[taskId].data[15] + 0])
        {
        case 0:
            if (gBattleControllerExecFlags & gBitTable[battlerId])
                return;

            memcpy(gBattleBufferA[battlerId], &gLinkBattleRecvBuffer[gTasks[taskId].data[15] + LINK_BUFF_DATA], blockSize);
            MarkBattlerReceivedLinkData(battlerId);

            if (!(gBattleTypeFlags & BATTLE_TYPE_IS_MASTER))
            {
                gBattlerAttacker = gLinkBattleRecvBuffer[gTasks[taskId].data[15] + LINK_BUFF_ATTACKER];
                gBattlerTarget = gLinkBattleRecvBuffer[gTasks[taskId].data[15] + LINK_BUFF_TARGET];
                gAbsentBattlerFlags = gLinkBattleRecvBuffer[gTasks[taskId].data[15] + LINK_BUFF_ABSENT_BATTLER_FLAGS];
                gEffectBattler = gLinkBattleRecvBuffer[gTasks[taskId].data[15] + LINK_BUFF_EFFECT_BATTLER];
            }
            break;
        case 1:
            memcpy(gBattleBufferB[battlerId], &gLinkBattleRecvBuffer[gTasks[taskId].data[15] + LINK_BUFF_DATA], blockSize);
            break;
        case 2:
            var = gLinkBattleRecvBuffer[gTasks[taskId].data[15] + LINK_BUFF_DATA];
            gBattleControllerExecFlags &= ~(gBitTable[battlerId] << (var * 4));
            break;
        }

        gTasks[taskId].data[15] = gTasks[taskId].data[15] + blockSize + LINK_BUFF_DATA;
    }
}

void BtlController_EmitGetMonData(u8 bufferId, u8 requestId, u8 monToCheck)
{
    sBattleBuffersTransferData[0] = CONTROLLER_GETMONDATA;
    sBattleBuffersTransferData[1] = requestId;
    sBattleBuffersTransferData[2] = monToCheck;
    sBattleBuffersTransferData[3] = 0;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

static void UNUSED BtlController_EmitGetRawMonData(u8 bufferId, u8 monId, u8 bytes)
{
    sBattleBuffersTransferData[0] = CONTROLLER_GETRAWMONDATA;
    sBattleBuffersTransferData[1] = monId;
    sBattleBuffersTransferData[2] = bytes;
    sBattleBuffersTransferData[3] = 0;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitSetMonData(u8 bufferId, u8 requestId, u8 monToCheck, u8 bytes, void *data)
{
    s32 i;

    sBattleBuffersTransferData[0] = CONTROLLER_SETMONDATA;
    sBattleBuffersTransferData[1] = requestId;
    sBattleBuffersTransferData[2] = monToCheck;
    for (i = 0; i < bytes; i++)
        sBattleBuffersTransferData[3 + i] = *(u8 *)(data++);
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 3 + bytes);
}

static void UNUSED BtlController_EmitSetRawMonData(u8 bufferId, u8 monId, u8 bytes, void *data)
{
    s32 i;

    sBattleBuffersTransferData[0] = CONTROLLER_SETRAWMONDATA;
    sBattleBuffersTransferData[1] = monId;
    sBattleBuffersTransferData[2] = bytes;
    for (i = 0; i < bytes; i++)
        sBattleBuffersTransferData[3 + i] = *(u8 *)(data++);
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, bytes + 3);
}

void BtlController_EmitLoadMonSprite(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_LOADMONSPRITE;
    sBattleBuffersTransferData[1] = CONTROLLER_LOADMONSPRITE;
    sBattleBuffersTransferData[2] = CONTROLLER_LOADMONSPRITE;
    sBattleBuffersTransferData[3] = CONTROLLER_LOADMONSPRITE;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitSwitchInAnim(u8 bufferId, u8 partyId, bool8 dontClearSubstituteBit)
{
    sBattleBuffersTransferData[0] = CONTROLLER_SWITCHINANIM;
    sBattleBuffersTransferData[1] = partyId;
    sBattleBuffersTransferData[2] = dontClearSubstituteBit;
    sBattleBuffersTransferData[3] = 5;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitReturnMonToBall(u8 bufferId, bool8 skipAnim)
{
    sBattleBuffersTransferData[0] = CONTROLLER_RETURNMONTOBALL;
    sBattleBuffersTransferData[1] = skipAnim;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 2);
}

void BtlController_EmitDrawTrainerPic(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_DRAWTRAINERPIC;
    sBattleBuffersTransferData[1] = CONTROLLER_DRAWTRAINERPIC;
    sBattleBuffersTransferData[2] = CONTROLLER_DRAWTRAINERPIC;
    sBattleBuffersTransferData[3] = CONTROLLER_DRAWTRAINERPIC;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitTrainerSlide(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_TRAINERSLIDE;
    sBattleBuffersTransferData[1] = CONTROLLER_TRAINERSLIDE;
    sBattleBuffersTransferData[2] = CONTROLLER_TRAINERSLIDE;
    sBattleBuffersTransferData[3] = CONTROLLER_TRAINERSLIDE;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitTrainerSlideBack(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_TRAINERSLIDEBACK;
    sBattleBuffersTransferData[1] = CONTROLLER_TRAINERSLIDEBACK;
    sBattleBuffersTransferData[2] = CONTROLLER_TRAINERSLIDEBACK;
    sBattleBuffersTransferData[3] = CONTROLLER_TRAINERSLIDEBACK;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitFaintAnimation(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_FAINTANIMATION;
    sBattleBuffersTransferData[1] = CONTROLLER_FAINTANIMATION;
    sBattleBuffersTransferData[2] = CONTROLLER_FAINTANIMATION;
    sBattleBuffersTransferData[3] = CONTROLLER_FAINTANIMATION;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

static void UNUSED BtlController_EmitPaletteFade(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_PALETTEFADE;
    sBattleBuffersTransferData[1] = CONTROLLER_PALETTEFADE;
    sBattleBuffersTransferData[2] = CONTROLLER_PALETTEFADE;
    sBattleBuffersTransferData[3] = CONTROLLER_PALETTEFADE;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

static void UNUSED BtlController_EmitSuccessBallThrowAnim(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_SUCCESSBALLTHROWANIM;
    sBattleBuffersTransferData[1] = CONTROLLER_SUCCESSBALLTHROWANIM;
    sBattleBuffersTransferData[2] = CONTROLLER_SUCCESSBALLTHROWANIM;
    sBattleBuffersTransferData[3] = CONTROLLER_SUCCESSBALLTHROWANIM;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitBallThrowAnim(u8 bufferId, u8 caseId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_BALLTHROWANIM;
    sBattleBuffersTransferData[1] = caseId;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 2);
}

static void UNUSED BtlController_EmitPause(u8 bufferId, u8 toWait, void *data)
{
    s32 i;

    sBattleBuffersTransferData[0] = CONTROLLER_PAUSE;
    sBattleBuffersTransferData[1] = toWait;
    for (i = 0; i < toWait * 3; i++)
        sBattleBuffersTransferData[2 + i] = *(u8 *)(data++);
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, toWait * 3 + 2);
}

void BtlController_EmitMoveAnimation(u8 bufferId, u16 move, u8 turnOfMove, u16 movePower, s32 dmg, u8 friendship, struct DisableStruct *disableStructPtr, u8 multihit)
{
    sBattleBuffersTransferData[0] = CONTROLLER_MOVEANIMATION;
    sBattleBuffersTransferData[1] = move;
    sBattleBuffersTransferData[2] = (move & 0xFF00) >> 8;
    sBattleBuffersTransferData[3] = turnOfMove;
    sBattleBuffersTransferData[4] = movePower;
    sBattleBuffersTransferData[5] = (movePower & 0xFF00) >> 8;
    sBattleBuffersTransferData[6] = dmg;
    sBattleBuffersTransferData[7] = (dmg & 0x0000FF00) >> 8;
    sBattleBuffersTransferData[8] = (dmg & 0x00FF0000) >> 16;
    sBattleBuffersTransferData[9] = (dmg & 0xFF000000) >> 24;
    sBattleBuffersTransferData[10] = friendship;
    sBattleBuffersTransferData[11] = multihit;
    if (WEATHER_HAS_EFFECT2)
    {
        sBattleBuffersTransferData[12] = gBattleWeather;
        sBattleBuffersTransferData[13] = (gBattleWeather & 0xFF00) >> 8;
    }
    else
    {
        sBattleBuffersTransferData[12] = 0;
        sBattleBuffersTransferData[13] = 0;
    }
    sBattleBuffersTransferData[14] = 0;
    sBattleBuffersTransferData[15] = 0;
    memcpy(&sBattleBuffersTransferData[16], disableStructPtr, sizeof(struct DisableStruct));
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 16 + sizeof(struct DisableStruct));
}

void BtlController_EmitPrintString(u8 bufferId, u16 stringID)
{
    s32 i;
    struct BattleMsgData *stringInfo;

    sBattleBuffersTransferData[0] = CONTROLLER_PRINTSTRING;
    sBattleBuffersTransferData[1] = gBattleOutcome;
    sBattleBuffersTransferData[2] = stringID;
    sBattleBuffersTransferData[3] = (stringID & 0xFF00) >> 8;

    stringInfo = (struct BattleMsgData *)(&sBattleBuffersTransferData[4]);
    stringInfo->currentMove = gCurrentMove;
    stringInfo->originallyUsedMove = gChosenMove;
    stringInfo->lastItem = gLastUsedItem;
    stringInfo->lastAbility = gLastUsedAbility;
    stringInfo->scrActive = gBattleScripting.battler;
    stringInfo->bakScriptPartyIdx = gBattleStruct->scriptPartyIdx;
    stringInfo->hpScale = gBattleStruct->hpScale;
    stringInfo->itemEffectBattler = gPotentialItemEffectBattler;
    stringInfo->moveType = gBattleMoves[gCurrentMove].type;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
        stringInfo->abilities[i] = gBattleMons[i].ability;
    for (i = 0; i < TEXT_BUFF_ARRAY_COUNT; i++)
    {
        stringInfo->textBuffs[0][i] = gBattleTextBuff1[i];
        stringInfo->textBuffs[1][i] = gBattleTextBuff2[i];
        stringInfo->textBuffs[2][i] = gBattleTextBuff3[i];
    }
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, sizeof(struct BattleMsgData) + 4);
}

void BtlController_EmitPrintSelectionString(u8 bufferId, u16 stringID)
{
    s32 i;
    struct BattleMsgData *stringInfo;

    sBattleBuffersTransferData[0] = CONTROLLER_PRINTSTRINGPLAYERONLY;
    sBattleBuffersTransferData[1] = CONTROLLER_PRINTSTRINGPLAYERONLY;
    sBattleBuffersTransferData[2] = stringID;
    sBattleBuffersTransferData[3] = (stringID & 0xFF00) >> 8;

    stringInfo = (struct BattleMsgData *)(&sBattleBuffersTransferData[4]);
    stringInfo->currentMove = gCurrentMove;
    stringInfo->originallyUsedMove = gChosenMove;
    stringInfo->lastItem = gLastUsedItem;
    stringInfo->lastAbility = gLastUsedAbility;
    stringInfo->scrActive = gBattleScripting.battler;
    stringInfo->bakScriptPartyIdx = gBattleStruct->scriptPartyIdx;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
        stringInfo->abilities[i] = gBattleMons[i].ability;
    for (i = 0; i < TEXT_BUFF_ARRAY_COUNT; i++)
    {
        stringInfo->textBuffs[0][i] = gBattleTextBuff1[i];
        stringInfo->textBuffs[1][i] = gBattleTextBuff2[i];
        stringInfo->textBuffs[2][i] = gBattleTextBuff3[i];
    }
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, sizeof(struct BattleMsgData) + 4);
}

// itemId only relevant for B_ACTION_USE_ITEM
void BtlController_EmitChooseAction(u8 bufferId, u8 action, u16 itemId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_CHOOSEACTION;
    sBattleBuffersTransferData[1] = action;
    sBattleBuffersTransferData[2] = itemId;
    sBattleBuffersTransferData[3] = (itemId & 0xFF00) >> 8;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

// Only used by the forfeit prompt in the Battle Frontier
// For other Yes/No boxes in battle, see Cmd_yesnobox
void BtlController_EmitYesNoBox(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_YESNOBOX;
    sBattleBuffersTransferData[1] = CONTROLLER_YESNOBOX;
    sBattleBuffersTransferData[2] = CONTROLLER_YESNOBOX;
    sBattleBuffersTransferData[3] = CONTROLLER_YESNOBOX;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitChooseMove(u8 bufferId, bool8 isDoubleBattle, bool8 NoPpNumber, struct ChooseMoveStruct *movePpData)
{
    s32 i;

    sBattleBuffersTransferData[0] = CONTROLLER_CHOOSEMOVE;
    sBattleBuffersTransferData[1] = isDoubleBattle;
    sBattleBuffersTransferData[2] = NoPpNumber;
    sBattleBuffersTransferData[3] = 0;
    for (i = 0; i < sizeof(*movePpData); i++)
        sBattleBuffersTransferData[4 + i] = *((u8 *)(movePpData) + i);
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, sizeof(*movePpData) + 4);
}

void BtlController_EmitChooseItem(u8 bufferId, u8 *battlePartyOrder)
{
    s32 i;

    sBattleBuffersTransferData[0] = CONTROLLER_OPENBAG;
    for (i = 0; i < PARTY_SIZE / 2; i++)
        sBattleBuffersTransferData[1 + i] = battlePartyOrder[i];
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitChoosePokemon(u8 bufferId, u8 caseId, u8 slotId, u8 abilityId, u8 *data)
{
    s32 i;

    sBattleBuffersTransferData[0] = CONTROLLER_CHOOSEPOKEMON;
    sBattleBuffersTransferData[1] = caseId;
    sBattleBuffersTransferData[2] = slotId;
    sBattleBuffersTransferData[3] = abilityId;
    for (i = 0; i < 3; i++)
        sBattleBuffersTransferData[4 + i] = data[i];
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 8);  // Only 7 bytes were written.
}

static void UNUSED BtlController_EmitCmd23(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_23;
    sBattleBuffersTransferData[1] = CONTROLLER_23;
    sBattleBuffersTransferData[2] = CONTROLLER_23;
    sBattleBuffersTransferData[3] = CONTROLLER_23;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

// why is the argument u16 if it's being cast to s16 anyway?
void BtlController_EmitHealthBarUpdate(u8 bufferId, u16 hpValue)
{
    sBattleBuffersTransferData[0] = CONTROLLER_HEALTHBARUPDATE;
    sBattleBuffersTransferData[1] = 0;
    sBattleBuffersTransferData[2] = (s16)hpValue;
    sBattleBuffersTransferData[3] = ((s16)hpValue & 0xFF00) >> 8;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

// why is the argument u16 if it's being cast to s16 anyway?
void BtlController_EmitExpUpdate(u8 bufferId, u8 partyId, u16 expPoints)
{
    sBattleBuffersTransferData[0] = CONTROLLER_EXPUPDATE;
    sBattleBuffersTransferData[1] = partyId;
    sBattleBuffersTransferData[2] = (s16)expPoints;
    sBattleBuffersTransferData[3] = ((s16)expPoints & 0xFF00) >> 8;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitStatusIconUpdate(u8 bufferId, u32 status1, u32 status2)
{
    sBattleBuffersTransferData[0] = CONTROLLER_STATUSICONUPDATE;
    sBattleBuffersTransferData[1] = status1;
    sBattleBuffersTransferData[2] = (status1 & 0x0000FF00) >> 8;
    sBattleBuffersTransferData[3] = (status1 & 0x00FF0000) >> 16;
    sBattleBuffersTransferData[4] = (status1 & 0xFF000000) >> 24;
    sBattleBuffersTransferData[5] = status2;
    sBattleBuffersTransferData[6] = (status2 & 0x0000FF00) >> 8;
    sBattleBuffersTransferData[7] = (status2 & 0x00FF0000) >> 16;
    sBattleBuffersTransferData[8] = (status2 & 0xFF000000) >> 24;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 9);
}

void BtlController_EmitStatusAnimation(u8 bufferId, bool8 status2, u32 status)
{
    sBattleBuffersTransferData[0] = CONTROLLER_STATUSANIMATION;
    sBattleBuffersTransferData[1] = status2;
    sBattleBuffersTransferData[2] = status;
    sBattleBuffersTransferData[3] = (status & 0x0000FF00) >> 8;
    sBattleBuffersTransferData[4] = (status & 0x00FF0000) >> 16;
    sBattleBuffersTransferData[5] = (status & 0xFF000000) >> 24;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 6);
}

static void UNUSED BtlController_EmitStatusXor(u8 bufferId, u8 b)
{
    sBattleBuffersTransferData[0] = CONTROLLER_STATUSXOR;
    sBattleBuffersTransferData[1] = b;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 2);
}

void BtlController_EmitDataTransfer(u8 bufferId, u16 size, void *data)
{
    s32 i;

    sBattleBuffersTransferData[0] = CONTROLLER_DATATRANSFER;
    sBattleBuffersTransferData[1] = CONTROLLER_DATATRANSFER;
    sBattleBuffersTransferData[2] = size;
    sBattleBuffersTransferData[3] = (size & 0xFF00) >> 8;
    for (i = 0; i < size; i++)
        sBattleBuffersTransferData[4 + i] = *(u8 *)(data++);
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, size + 4);
}

static void UNUSED BtlController_EmitDMA3Transfer(u8 bufferId, void *dst, u16 size, void *data)
{
    s32 i;

    sBattleBuffersTransferData[0] = CONTROLLER_DMA3TRANSFER;
    sBattleBuffersTransferData[1] = (u32)(dst);
    sBattleBuffersTransferData[2] = ((u32)(dst) & 0x0000FF00) >> 8;
    sBattleBuffersTransferData[3] = ((u32)(dst) & 0x00FF0000) >> 16;
    sBattleBuffersTransferData[4] = ((u32)(dst) & 0xFF000000) >> 24;
    sBattleBuffersTransferData[5] = size;
    sBattleBuffersTransferData[6] = (size & 0xFF00) >> 8;
    for (i = 0; i < size; i++)
        sBattleBuffersTransferData[7 + i] = *(u8 *)(data++);
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, size + 7);
}

static void UNUSED BtlController_EmitPlayBGM(u8 bufferId, u16 songId, void *data)
{
    s32 i;

    sBattleBuffersTransferData[0] = CONTROLLER_PLAYBGM;
    sBattleBuffersTransferData[1] = songId;
    sBattleBuffersTransferData[2] = (songId & 0xFF00) >> 8;

    // Nonsense loop using songId as a size
    // Would go out of bounds for any song id after SE_RG_BAG_POCKET (253)
    for (i = 0; i < songId; i++)
        sBattleBuffersTransferData[3 + i] = *(u8 *)(data++);
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, songId + 3);
}

static void UNUSED BtlController_EmitCmd32(u8 bufferId, u16 size, void *data)
{
    s32 i;

    sBattleBuffersTransferData[0] = CONTROLLER_32;
    sBattleBuffersTransferData[1] = size;
    sBattleBuffersTransferData[2] = (size & 0xFF00) >> 8;
    for (i = 0; i < size; i++)
        sBattleBuffersTransferData[3 + i] = *(u8 *)(data++);
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, size + 3);
}

void BtlController_EmitTwoReturnValues(u8 bufferId, u8 ret8, u16 ret16)
{
    sBattleBuffersTransferData[0] = CONTROLLER_TWORETURNVALUES;
    sBattleBuffersTransferData[1] = ret8;
    sBattleBuffersTransferData[2] = ret16;
    sBattleBuffersTransferData[3] = (ret16 & 0xFF00) >> 8;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitChosenMonReturnValue(u8 bufferId, u8 partyId, u8 *battlePartyOrder)
{
    s32 i;

    sBattleBuffersTransferData[0] = CONTROLLER_CHOSENMONRETURNVALUE;
    sBattleBuffersTransferData[1] = partyId;
    for (i = 0; i < (int)ARRAY_COUNT(gBattlePartyCurrentOrder); i++)
        sBattleBuffersTransferData[2 + i] = battlePartyOrder[i];
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 5);
}

void BtlController_EmitOneReturnValue(u8 bufferId, u16 ret)
{
    sBattleBuffersTransferData[0] = CONTROLLER_ONERETURNVALUE;
    sBattleBuffersTransferData[1] = ret;
    sBattleBuffersTransferData[2] = (ret & 0xFF00) >> 8;
    sBattleBuffersTransferData[3] = 0;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitOneReturnValue_Duplicate(u8 bufferId, u16 ret)
{
    sBattleBuffersTransferData[0] = CONTROLLER_ONERETURNVALUE_DUPLICATE;
    sBattleBuffersTransferData[1] = ret;
    sBattleBuffersTransferData[2] = (ret & 0xFF00) >> 8;
    sBattleBuffersTransferData[3] = 0;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

static void UNUSED BtlController_EmitClearUnkVar(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_CLEARUNKVAR;
    sBattleBuffersTransferData[1] = CONTROLLER_CLEARUNKVAR;
    sBattleBuffersTransferData[2] = CONTROLLER_CLEARUNKVAR;
    sBattleBuffersTransferData[3] = CONTROLLER_CLEARUNKVAR;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

static void UNUSED BtlController_EmitSetUnkVar(u8 bufferId, u8 b)
{
    sBattleBuffersTransferData[0] = CONTROLLER_SETUNKVAR;
    sBattleBuffersTransferData[1] = b;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 2);
}

static void UNUSED BtlController_EmitClearUnkFlag(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_CLEARUNKFLAG;
    sBattleBuffersTransferData[1] = CONTROLLER_CLEARUNKFLAG;
    sBattleBuffersTransferData[2] = CONTROLLER_CLEARUNKFLAG;
    sBattleBuffersTransferData[3] = CONTROLLER_CLEARUNKFLAG;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

static void UNUSED BtlController_EmitToggleUnkFlag(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_TOGGLEUNKFLAG;
    sBattleBuffersTransferData[1] = CONTROLLER_TOGGLEUNKFLAG;
    sBattleBuffersTransferData[2] = CONTROLLER_TOGGLEUNKFLAG;
    sBattleBuffersTransferData[3] = CONTROLLER_TOGGLEUNKFLAG;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitHitAnimation(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_HITANIMATION;
    sBattleBuffersTransferData[1] = CONTROLLER_HITANIMATION;
    sBattleBuffersTransferData[2] = CONTROLLER_HITANIMATION;
    sBattleBuffersTransferData[3] = CONTROLLER_HITANIMATION;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitCantSwitch(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_CANTSWITCH;
    sBattleBuffersTransferData[1] = CONTROLLER_CANTSWITCH;
    sBattleBuffersTransferData[2] = CONTROLLER_CANTSWITCH;
    sBattleBuffersTransferData[3] = CONTROLLER_CANTSWITCH;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitPlaySE(u8 bufferId, u16 songId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_PLAYSE;
    sBattleBuffersTransferData[1] = songId;
    sBattleBuffersTransferData[2] = (songId & 0xFF00) >> 8;
    sBattleBuffersTransferData[3] = 0;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitPlayFanfareOrBGM(u8 bufferId, u16 songId, bool8 playBGM)
{
    sBattleBuffersTransferData[0] = CONTROLLER_PLAYFANFAREORBGM;
    sBattleBuffersTransferData[1] = songId;
    sBattleBuffersTransferData[2] = (songId & 0xFF00) >> 8;
    sBattleBuffersTransferData[3] = playBGM;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitFaintingCry(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_FAINTINGCRY;
    sBattleBuffersTransferData[1] = CONTROLLER_FAINTINGCRY;
    sBattleBuffersTransferData[2] = CONTROLLER_FAINTINGCRY;
    sBattleBuffersTransferData[3] = CONTROLLER_FAINTINGCRY;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitIntroSlide(u8 bufferId, u8 environmentId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_INTROSLIDE;
    sBattleBuffersTransferData[1] = environmentId;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 2);
}

void BtlController_EmitIntroTrainerBallThrow(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_INTROTRAINERBALLTHROW;
    sBattleBuffersTransferData[1] = CONTROLLER_INTROTRAINERBALLTHROW;
    sBattleBuffersTransferData[2] = CONTROLLER_INTROTRAINERBALLTHROW;
    sBattleBuffersTransferData[3] = CONTROLLER_INTROTRAINERBALLTHROW;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitDrawPartyStatusSummary(u8 bufferId, struct HpAndStatus* hpAndStatus, u8 flags)
{
    s32 i;

    sBattleBuffersTransferData[0] = CONTROLLER_DRAWPARTYSTATUSSUMMARY;
    sBattleBuffersTransferData[1] = flags & ~PARTY_SUMM_SKIP_DRAW_DELAY; // If true, skip player side
    sBattleBuffersTransferData[2] = (flags & PARTY_SUMM_SKIP_DRAW_DELAY) >> 7; // If true, skip delay after drawing. True during intro
    sBattleBuffersTransferData[3] = CONTROLLER_DRAWPARTYSTATUSSUMMARY;
    for (i = 0; i < (s32)(sizeof(struct HpAndStatus) * PARTY_SIZE); i++)
        sBattleBuffersTransferData[4 + i] = *(i + (u8 *)(hpAndStatus));
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, sizeof(struct HpAndStatus) * PARTY_SIZE + 4);
}

void BtlController_EmitHidePartyStatusSummary(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_HIDEPARTYSTATUSSUMMARY;
    sBattleBuffersTransferData[1] = CONTROLLER_HIDEPARTYSTATUSSUMMARY;
    sBattleBuffersTransferData[2] = CONTROLLER_HIDEPARTYSTATUSSUMMARY;
    sBattleBuffersTransferData[3] = CONTROLLER_HIDEPARTYSTATUSSUMMARY;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitEndBounceEffect(u8 bufferId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_ENDBOUNCE;
    sBattleBuffersTransferData[1] = CONTROLLER_ENDBOUNCE;
    sBattleBuffersTransferData[2] = CONTROLLER_ENDBOUNCE;
    sBattleBuffersTransferData[3] = CONTROLLER_ENDBOUNCE;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitSpriteInvisibility(u8 bufferId, bool8 isInvisible)
{
    sBattleBuffersTransferData[0] = CONTROLLER_SPRITEINVISIBILITY;
    sBattleBuffersTransferData[1] = isInvisible;
    sBattleBuffersTransferData[2] = CONTROLLER_SPRITEINVISIBILITY;
    sBattleBuffersTransferData[3] = CONTROLLER_SPRITEINVISIBILITY;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

void BtlController_EmitBattleAnimation(u8 bufferId, u8 animationId, u16 argument)
{
    sBattleBuffersTransferData[0] = CONTROLLER_BATTLEANIMATION;
    sBattleBuffersTransferData[1] = animationId;
    sBattleBuffersTransferData[2] = argument;
    sBattleBuffersTransferData[3] = (argument & 0xFF00) >> 8;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 4);
}

// mode is a LINK_STANDBY_* constant
void BtlController_EmitLinkStandbyMsg(u8 bufferId, u8 mode, bool32 record)
{
    bool8 record_ = record;
    sBattleBuffersTransferData[0] = CONTROLLER_LINKSTANDBYMSG;
    sBattleBuffersTransferData[1] = mode;

    if (record_)
        sBattleBuffersTransferData[3] = sBattleBuffersTransferData[2] = RecordedBattle_BufferNewBattlerData(&sBattleBuffersTransferData[4]);
    else
        sBattleBuffersTransferData[3] = sBattleBuffersTransferData[2] = 0;

    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, sBattleBuffersTransferData[2] + 4);
}

void BtlController_EmitResetActionMoveSelection(u8 bufferId, u8 caseId)
{
    sBattleBuffersTransferData[0] = CONTROLLER_RESETACTIONMOVESELECTION;
    sBattleBuffersTransferData[1] = caseId;
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, 2);
}

void BtlController_EmitEndLinkBattle(u8 bufferId, u8 battleOutcome)
{
    sBattleBuffersTransferData[0] = CONTROLLER_ENDLINKBATTLE;
    sBattleBuffersTransferData[1] = battleOutcome;
    sBattleBuffersTransferData[2] = gSaveBlock2Ptr->frontier.disableRecordBattle;
    sBattleBuffersTransferData[3] = gSaveBlock2Ptr->frontier.disableRecordBattle;
    sBattleBuffersTransferData[5] = sBattleBuffersTransferData[4] = RecordedBattle_BufferNewBattlerData(&sBattleBuffersTransferData[6]);
    PrepareBufferDataTransfer(bufferId, sBattleBuffersTransferData, sBattleBuffersTransferData[4] + 6);
}
