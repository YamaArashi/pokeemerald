#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_interface.h"
#include "battle_message.h"
#include "battle_setup.h"
#include "battle_tv.h"
#include "bg.h"
#include "data.h"
#include "item.h"
#include "item_menu.h"
#include "link.h"
#include "main.h"
#include "m4a.h"
#include "palette.h"
#include "party_menu.h"
#include "pokeball.h"
#include "pokemon.h"
#include "random.h"
#include "reshow_battle_screen.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "util.h"
#include "window.h"
#include "constants/battle_anim.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/songs.h"
#include "constants/trainers.h"
#include "constants/rgb.h"

// this file's functions
static void WallyHandleGetMonData(void);
static void WallyHandleGetRawMonData(void);
static void WallyHandleSetMonData(void);
static void WallyHandleSetRawMonData(void);
static void WallyHandleLoadMonSprite(void);
static void WallyHandleSwitchInAnim(void);
static void WallyHandleReturnMonToBall(void);
static void WallyHandleDrawTrainerPic(void);
static void WallyHandleTrainerSlide(void);
static void WallyHandleTrainerSlideBack(void);
static void WallyHandleFaintAnimation(void);
static void WallyHandlePaletteFade(void);
static void WallyHandleSuccessBallThrowAnim(void);
static void WallyHandleBallThrowAnim(void);
static void WallyHandlePause(void);
static void WallyHandleMoveAnimation(void);
static void WallyHandlePrintString(void);
static void WallyHandlePrintSelectionString(void);
static void WallyHandleChooseAction(void);
static void WallyHandleYesNoBox(void);
static void WallyHandleChooseMove(void);
static void WallyHandleChooseItem(void);
static void WallyHandleChoosePokemon(void);
static void WallyHandleCmd23(void);
static void WallyHandleHealthBarUpdate(void);
static void WallyHandleExpUpdate(void);
static void WallyHandleStatusIconUpdate(void);
static void WallyHandleStatusAnimation(void);
static void WallyHandleStatusXor(void);
static void WallyHandleDataTransfer(void);
static void WallyHandleDMA3Transfer(void);
static void WallyHandlePlayBGM(void);
static void WallyHandleCmd32(void);
static void WallyHandleTwoReturnValues(void);
static void WallyHandleChosenMonReturnValue(void);
static void WallyHandleOneReturnValue(void);
static void WallyHandleOneReturnValue_Duplicate(void);
static void WallyHandleClearUnkVar(void);
static void WallyHandleSetUnkVar(void);
static void WallyHandleClearUnkFlag(void);
static void WallyHandleToggleUnkFlag(void);
static void WallyHandleHitAnimation(void);
static void WallyHandleCantSwitch(void);
static void WallyHandlePlaySE(void);
static void WallyHandlePlayFanfareOrBGM(void);
static void WallyHandleFaintingCry(void);
static void WallyHandleIntroSlide(void);
static void WallyHandleIntroTrainerBallThrow(void);
static void WallyHandleDrawPartyStatusSummary(void);
static void WallyHandleHidePartyStatusSummary(void);
static void WallyHandleEndBounceEffect(void);
static void WallyHandleSpriteInvisibility(void);
static void WallyHandleBattleAnimation(void);
static void WallyHandleLinkStandbyMsg(void);
static void WallyHandleResetActionMoveSelection(void);
static void WallyHandleEndLinkBattle(void);
static void WallyCmdEnd(void);

static void WallyBufferRunCommand(void);
static void WallyBufferExecCompleted(void);
static void CompleteOnChosenItem(void);
static void Intro_WaitForShinyAnimAndHealthbox(void);
static u32 CopyWallyMonData(u8 monId, u8 *dst);
static void SetWallyMonData(u8 monId);
static void WallyDoMoveAnimation(void);
static void Task_StartSendOutAnim(u8 taskId);

static void (*const sWallyBufferCommands[CONTROLLER_CMDS_COUNT])(void) =
{
    [CONTROLLER_GETMONDATA]               = WallyHandleGetMonData,
    [CONTROLLER_GETRAWMONDATA]            = WallyHandleGetRawMonData,
    [CONTROLLER_SETMONDATA]               = WallyHandleSetMonData,
    [CONTROLLER_SETRAWMONDATA]            = WallyHandleSetRawMonData,
    [CONTROLLER_LOADMONSPRITE]            = WallyHandleLoadMonSprite,
    [CONTROLLER_SWITCHINANIM]             = WallyHandleSwitchInAnim,
    [CONTROLLER_RETURNMONTOBALL]          = WallyHandleReturnMonToBall,
    [CONTROLLER_DRAWTRAINERPIC]           = WallyHandleDrawTrainerPic,
    [CONTROLLER_TRAINERSLIDE]             = WallyHandleTrainerSlide,
    [CONTROLLER_TRAINERSLIDEBACK]         = WallyHandleTrainerSlideBack,
    [CONTROLLER_FAINTANIMATION]           = WallyHandleFaintAnimation,
    [CONTROLLER_PALETTEFADE]              = WallyHandlePaletteFade,
    [CONTROLLER_SUCCESSBALLTHROWANIM]     = WallyHandleSuccessBallThrowAnim,
    [CONTROLLER_BALLTHROWANIM]            = WallyHandleBallThrowAnim,
    [CONTROLLER_PAUSE]                    = WallyHandlePause,
    [CONTROLLER_MOVEANIMATION]            = WallyHandleMoveAnimation,
    [CONTROLLER_PRINTSTRING]              = WallyHandlePrintString,
    [CONTROLLER_PRINTSTRINGPLAYERONLY]    = WallyHandlePrintSelectionString,
    [CONTROLLER_CHOOSEACTION]             = WallyHandleChooseAction,
    [CONTROLLER_YESNOBOX]                 = WallyHandleYesNoBox,
    [CONTROLLER_CHOOSEMOVE]               = WallyHandleChooseMove,
    [CONTROLLER_OPENBAG]                  = WallyHandleChooseItem,
    [CONTROLLER_CHOOSEPOKEMON]            = WallyHandleChoosePokemon,
    [CONTROLLER_23]                       = WallyHandleCmd23,
    [CONTROLLER_HEALTHBARUPDATE]          = WallyHandleHealthBarUpdate,
    [CONTROLLER_EXPUPDATE]                = WallyHandleExpUpdate,
    [CONTROLLER_STATUSICONUPDATE]         = WallyHandleStatusIconUpdate,
    [CONTROLLER_STATUSANIMATION]          = WallyHandleStatusAnimation,
    [CONTROLLER_STATUSXOR]                = WallyHandleStatusXor,
    [CONTROLLER_DATATRANSFER]             = WallyHandleDataTransfer,
    [CONTROLLER_DMA3TRANSFER]             = WallyHandleDMA3Transfer,
    [CONTROLLER_PLAYBGM]                  = WallyHandlePlayBGM,
    [CONTROLLER_32]                       = WallyHandleCmd32,
    [CONTROLLER_TWORETURNVALUES]          = WallyHandleTwoReturnValues,
    [CONTROLLER_CHOSENMONRETURNVALUE]     = WallyHandleChosenMonReturnValue,
    [CONTROLLER_ONERETURNVALUE]           = WallyHandleOneReturnValue,
    [CONTROLLER_ONERETURNVALUE_DUPLICATE] = WallyHandleOneReturnValue_Duplicate,
    [CONTROLLER_CLEARUNKVAR]              = WallyHandleClearUnkVar,
    [CONTROLLER_SETUNKVAR]                = WallyHandleSetUnkVar,
    [CONTROLLER_CLEARUNKFLAG]             = WallyHandleClearUnkFlag,
    [CONTROLLER_TOGGLEUNKFLAG]            = WallyHandleToggleUnkFlag,
    [CONTROLLER_HITANIMATION]             = WallyHandleHitAnimation,
    [CONTROLLER_CANTSWITCH]               = WallyHandleCantSwitch,
    [CONTROLLER_PLAYSE]                   = WallyHandlePlaySE,
    [CONTROLLER_PLAYFANFAREORBGM]         = WallyHandlePlayFanfareOrBGM,
    [CONTROLLER_FAINTINGCRY]              = WallyHandleFaintingCry,
    [CONTROLLER_INTROSLIDE]               = WallyHandleIntroSlide,
    [CONTROLLER_INTROTRAINERBALLTHROW]    = WallyHandleIntroTrainerBallThrow,
    [CONTROLLER_DRAWPARTYSTATUSSUMMARY]   = WallyHandleDrawPartyStatusSummary,
    [CONTROLLER_HIDEPARTYSTATUSSUMMARY]   = WallyHandleHidePartyStatusSummary,
    [CONTROLLER_ENDBOUNCE]                = WallyHandleEndBounceEffect,
    [CONTROLLER_SPRITEINVISIBILITY]       = WallyHandleSpriteInvisibility,
    [CONTROLLER_BATTLEANIMATION]          = WallyHandleBattleAnimation,
    [CONTROLLER_LINKSTANDBYMSG]           = WallyHandleLinkStandbyMsg,
    [CONTROLLER_RESETACTIONMOVESELECTION] = WallyHandleResetActionMoveSelection,
    [CONTROLLER_ENDLINKBATTLE]            = WallyHandleEndLinkBattle,
    [CONTROLLER_TERMINATOR_NOP]           = WallyCmdEnd
};

static void UNUSED SpriteCB_Null7(void)
{
}

void SetControllerToWally(void)
{
    gBattlerControllerFuncs[gActiveBattler] = WallyBufferRunCommand;
    gBattleStruct->wallyBattleState = 0;
    gBattleStruct->wallyMovesState = 0;
    gBattleStruct->wallyWaitFrames = 0;
    gBattleStruct->wallyMoveFrames = 0;
}

static void WallyBufferRunCommand(void)
{
    if (gBattleControllerExecFlags & gBitTable[gActiveBattler])
    {
        if (gBattleBufferA[gActiveBattler][0] < ARRAY_COUNT(sWallyBufferCommands))
            sWallyBufferCommands[gBattleBufferA[gActiveBattler][0]]();
        else
            WallyBufferExecCompleted();
    }
}

static void WallyHandleActions(void)
{
    switch (gBattleStruct->wallyBattleState)
    {
    case 0:
        gBattleStruct->wallyWaitFrames = B_WAIT_TIME_LONG;
        gBattleStruct->wallyBattleState++;
    case 1:
        if (--gBattleStruct->wallyWaitFrames == 0)
        {
            PlaySE(SE_SELECT);
            BtlController_EmitTwoReturnValues(BUFFER_B, B_ACTION_USE_MOVE, 0);
            WallyBufferExecCompleted();
            gBattleStruct->wallyBattleState++;
            gBattleStruct->wallyMovesState = 0;
            gBattleStruct->wallyWaitFrames = B_WAIT_TIME_LONG;
        }
        break;
    case 2:
        if (--gBattleStruct->wallyWaitFrames == 0)
        {
            PlaySE(SE_SELECT);
            BtlController_EmitTwoReturnValues(BUFFER_B, B_ACTION_USE_MOVE, 0);
            WallyBufferExecCompleted();
            gBattleStruct->wallyBattleState++;
            gBattleStruct->wallyMovesState = 0;
            gBattleStruct->wallyWaitFrames = B_WAIT_TIME_LONG;
        }
        break;
    case 3:
        if (--gBattleStruct->wallyWaitFrames == 0)
        {
            BtlController_EmitTwoReturnValues(BUFFER_B, B_ACTION_WALLY_THROW, 0);
            WallyBufferExecCompleted();
            gBattleStruct->wallyBattleState++;
            gBattleStruct->wallyMovesState = 0;
            gBattleStruct->wallyWaitFrames = B_WAIT_TIME_LONG;
        }
        break;
    case 4:
        if (--gBattleStruct->wallyWaitFrames == 0)
        {
            PlaySE(SE_SELECT);
            ActionSelectionDestroyCursorAt(0);
            ActionSelectionCreateCursorAt(1, 0);
            gBattleStruct->wallyWaitFrames = B_WAIT_TIME_LONG;
            gBattleStruct->wallyBattleState++;
        }
        break;
    case 5:
        if (--gBattleStruct->wallyWaitFrames == 0)
        {
            PlaySE(SE_SELECT);
            BtlController_EmitTwoReturnValues(BUFFER_B, B_ACTION_USE_ITEM, 0);
            WallyBufferExecCompleted();
        }
        break;
    }
}

static void CompleteOnBattlerSpriteCallbackDummy(void)
{
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy)
        WallyBufferExecCompleted();
}

static void CompleteOnInactiveTextPrinter(void)
{
    if (!IsTextPrinterActive(B_WIN_MSG))
        WallyBufferExecCompleted();
}

static void CompleteOnFinishedAnimation(void)
{
    if (!gDoingBattleAnim)
        WallyBufferExecCompleted();
}

static void OpenBagAfterPaletteFade(void)
{
    if (!gPaletteFade.active)
    {
        gBattlerControllerFuncs[gActiveBattler] = CompleteOnChosenItem;
        ReshowBattleScreenDummy();
        FreeAllWindowBuffers();
        DoWallyTutorialBagMenu();
    }
}

static void CompleteOnChosenItem(void)
{
    if (gMain.callback2 == BattleMainCB2 && !gPaletteFade.active)
    {
        BtlController_EmitOneReturnValue(BUFFER_B, gSpecialVar_ItemId);
        WallyBufferExecCompleted();
    }
}

static void Intro_TryShinyAnimShowHealthbox(void)
{
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].triedShinyMonAnim
     && !gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].ballAnimActive)
        TryShinyAnimation(gActiveBattler, &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]]);

    if (!gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(gActiveBattler)].triedShinyMonAnim
     && !gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(gActiveBattler)].ballAnimActive)
        TryShinyAnimation(BATTLE_PARTNER(gActiveBattler), &gPlayerParty[gBattlerPartyIndexes[BATTLE_PARTNER(gActiveBattler)]]);

    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].ballAnimActive
        && !gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(gActiveBattler)].ballAnimActive
        && gSprites[gBattleControllerData[gActiveBattler]].callback == SpriteCallbackDummy
        && gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy)
    {
        if (IsDoubleBattle() && !(gBattleTypeFlags & BATTLE_TYPE_MULTI))
        {
            DestroySprite(&gSprites[gBattleControllerData[BATTLE_PARTNER(gActiveBattler)]]);
            UpdateHealthboxAttribute(gHealthboxSpriteIds[BATTLE_PARTNER(gActiveBattler)], &gPlayerParty[gBattlerPartyIndexes[BATTLE_PARTNER(gActiveBattler)]], HEALTHBOX_ALL);
            StartHealthboxSlideIn(BATTLE_PARTNER(gActiveBattler));
            SetHealthboxSpriteVisible(gHealthboxSpriteIds[BATTLE_PARTNER(gActiveBattler)]);
        }
        DestroySprite(&gSprites[gBattleControllerData[gActiveBattler]]);
        UpdateHealthboxAttribute(gHealthboxSpriteIds[gActiveBattler], &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], HEALTHBOX_ALL);
        StartHealthboxSlideIn(gActiveBattler);
        SetHealthboxSpriteVisible(gHealthboxSpriteIds[gActiveBattler]);

        gBattleSpritesDataPtr->animationData->introAnimActive = FALSE;
        gBattlerControllerFuncs[gActiveBattler] = Intro_WaitForShinyAnimAndHealthbox;
    }

}

static void Intro_WaitForShinyAnimAndHealthbox(void)
{
    bool32 healthboxAnimDone = FALSE;

    if (gSprites[gHealthboxSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy)
        healthboxAnimDone = TRUE;

    if (healthboxAnimDone && gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].finishedShinyMonAnim
        && gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(gActiveBattler)].finishedShinyMonAnim)
    {
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].triedShinyMonAnim = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].finishedShinyMonAnim = FALSE;

        gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(gActiveBattler)].triedShinyMonAnim = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(gActiveBattler)].finishedShinyMonAnim = FALSE;

        FreeSpriteTilesByTag(ANIM_TAG_GOLD_STARS);
        FreeSpritePaletteByTag(ANIM_TAG_GOLD_STARS);

        CreateTask(Task_PlayerController_RestoreBgmAfterCry, 10);
        HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);

        WallyBufferExecCompleted();
    }
}

static void CompleteOnHealthbarDone(void)
{
    s16 hpValue = MoveBattleBar(gActiveBattler, gHealthboxSpriteIds[gActiveBattler], HEALTH_BAR, 0);

    SetHealthboxSpriteVisible(gHealthboxSpriteIds[gActiveBattler]);

    if (hpValue != -1)
    {
        UpdateHpTextInHealthbox(gHealthboxSpriteIds[gActiveBattler], hpValue, HP_CURRENT);
    }
    else
    {
        HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
        WallyBufferExecCompleted();
    }
}

static void DoHitAnimBlinkSpriteEffect(void)
{
    u8 spriteId = gBattlerSpriteIds[gActiveBattler];

    if (gSprites[spriteId].data[1] == 32)
    {
        gSprites[spriteId].data[1] = 0;
        gSprites[spriteId].invisible = FALSE;
        gDoingBattleAnim = FALSE;
        WallyBufferExecCompleted();
    }
    else
    {
        if ((gSprites[spriteId].data[1] % 4) == 0)
            gSprites[spriteId].invisible ^= 1;
        gSprites[spriteId].data[1]++;
    }
}

static void DoSwitchOutAnimation(void)
{
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive)
    {
        FreeSpriteOamMatrix(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        DestroySprite(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        SetHealthboxSpriteInvisible(gHealthboxSpriteIds[gActiveBattler]);
        WallyBufferExecCompleted();
    }
}

static void CompleteOnBankSpriteCallbackDummy2(void)
{
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy)
        WallyBufferExecCompleted();
}

static void CompleteOnFinishedBattleAnimation(void)
{
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animFromTableActive)
        WallyBufferExecCompleted();
}

static void WallyBufferExecCompleted(void)
{
    gBattlerControllerFuncs[gActiveBattler] = WallyBufferRunCommand;
    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
    {
        u8 playerId = GetMultiplayerId();

        PrepareBufferDataTransferLink(2, 4, &playerId);
        gBattleBufferA[gActiveBattler][0] = CONTROLLER_TERMINATOR_NOP;
    }
    else
    {
        gBattleControllerExecFlags &= ~gBitTable[gActiveBattler];
    }
}

static void UNUSED CompleteOnFinishedStatusAnimation(void)
{
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].statusAnimActive)
        WallyBufferExecCompleted();
}

static void WallyHandleGetMonData(void)
{
    u8 monData[sizeof(struct Pokemon) * 2 + 56]; // this allows to get full data of two Pokémon, trying to get more will result in overwriting data
    u32 size = 0;
    u8 monToCheck;
    s32 i;

    if (gBattleBufferA[gActiveBattler][2] == 0)
    {
        size += CopyWallyMonData(gBattlerPartyIndexes[gActiveBattler], monData);
    }
    else
    {
        monToCheck = gBattleBufferA[gActiveBattler][2];
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (monToCheck & 1)
                size += CopyWallyMonData(i, monData + size);
            monToCheck >>= 1;
        }
    }
    BtlController_EmitDataTransfer(BUFFER_B, size, monData);
    WallyBufferExecCompleted();
}

static u32 CopyWallyMonData(u8 monId, u8 *dst)
{
    struct BattlePokemon battleMon;
    struct MovePpInfo moveData;
    u8 nickname[POKEMON_NAME_BUFFER_SIZE];
    u8 *src;
    s16 data16;
    u32 data32;
    s32 size = 0;

    switch (gBattleBufferA[gActiveBattler][1])
    {
    case REQUEST_ALL_BATTLE:
        battleMon.species = GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES);
        battleMon.item = GetMonData(&gPlayerParty[monId], MON_DATA_HELD_ITEM);
        for (size = 0; size < MAX_MON_MOVES; size++)
        {
            battleMon.moves[size] = GetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + size);
            battleMon.pp[size] = GetMonData(&gPlayerParty[monId], MON_DATA_PP1 + size);
        }
        battleMon.ppBonuses = GetMonData(&gPlayerParty[monId], MON_DATA_PP_BONUSES);
        battleMon.friendship = GetMonData(&gPlayerParty[monId], MON_DATA_FRIENDSHIP);
        battleMon.experience = GetMonData(&gPlayerParty[monId], MON_DATA_EXP);
        battleMon.hpIV = GetMonData(&gPlayerParty[monId], MON_DATA_HP_IV);
        battleMon.attackIV = GetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV);
        battleMon.defenseIV = GetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV);
        battleMon.speedIV = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV);
        battleMon.spAttackIV = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV);
        battleMon.spDefenseIV = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV);
        battleMon.personality = GetMonData(&gPlayerParty[monId], MON_DATA_PERSONALITY);
        battleMon.status1 = GetMonData(&gPlayerParty[monId], MON_DATA_STATUS);
        battleMon.level = GetMonData(&gPlayerParty[monId], MON_DATA_LEVEL);
        battleMon.hp = GetMonData(&gPlayerParty[monId], MON_DATA_HP);
        battleMon.maxHP = GetMonData(&gPlayerParty[monId], MON_DATA_MAX_HP);
        battleMon.attack = GetMonData(&gPlayerParty[monId], MON_DATA_ATK);
        battleMon.defense = GetMonData(&gPlayerParty[monId], MON_DATA_DEF);
        battleMon.speed = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED);
        battleMon.spAttack = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK);
        battleMon.spDefense = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF);
        battleMon.isEgg = GetMonData(&gPlayerParty[monId], MON_DATA_IS_EGG);
        battleMon.abilityNum = GetMonData(&gPlayerParty[monId], MON_DATA_ABILITY_NUM);
        battleMon.otId = GetMonData(&gPlayerParty[monId], MON_DATA_OT_ID);
        GetMonData(&gPlayerParty[monId], MON_DATA_NICKNAME, nickname);
        StringCopy_Nickname(battleMon.nickname, nickname);
        GetMonData(&gPlayerParty[monId], MON_DATA_OT_NAME, battleMon.otName);
        src = (u8 *)&battleMon;
        for (size = 0; size < sizeof(battleMon); size++)
            dst[size] = src[size];
        break;
    case REQUEST_SPECIES_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_HELDITEM_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_HELD_ITEM);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_MOVES_PP_BATTLE:
        for (size = 0; size < MAX_MON_MOVES; size++)
        {
            moveData.moves[size] = GetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + size);
            moveData.pp[size] = GetMonData(&gPlayerParty[monId], MON_DATA_PP1 + size);
        }
        moveData.ppBonuses = GetMonData(&gPlayerParty[monId], MON_DATA_PP_BONUSES);
        src = (u8 *)(&moveData);
        for (size = 0; size < sizeof(moveData); size++)
            dst[size] = src[size];
        break;
    case REQUEST_MOVE1_BATTLE:
    case REQUEST_MOVE2_BATTLE:
    case REQUEST_MOVE3_BATTLE:
    case REQUEST_MOVE4_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + gBattleBufferA[gActiveBattler][1] - REQUEST_MOVE1_BATTLE);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_PP_DATA_BATTLE:
        for (size = 0; size < MAX_MON_MOVES; size++)
            dst[size] = GetMonData(&gPlayerParty[monId], MON_DATA_PP1 + size);
        dst[size] = GetMonData(&gPlayerParty[monId], MON_DATA_PP_BONUSES);
        size++;
        break;
    case REQUEST_PPMOVE1_BATTLE:
    case REQUEST_PPMOVE2_BATTLE:
    case REQUEST_PPMOVE3_BATTLE:
    case REQUEST_PPMOVE4_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_PP1 + gBattleBufferA[gActiveBattler][1] - REQUEST_PPMOVE1_BATTLE);
        size = 1;
        break;
    case REQUEST_OTID_BATTLE:
        data32 = GetMonData(&gPlayerParty[monId], MON_DATA_OT_ID);
        dst[0] = (data32 & 0x000000FF);
        dst[1] = (data32 & 0x0000FF00) >> 8;
        dst[2] = (data32 & 0x00FF0000) >> 16;
        size = 3;
        break;
    case REQUEST_EXP_BATTLE:
        data32 = GetMonData(&gPlayerParty[monId], MON_DATA_EXP);
        dst[0] = (data32 & 0x000000FF);
        dst[1] = (data32 & 0x0000FF00) >> 8;
        dst[2] = (data32 & 0x00FF0000) >> 16;
        size = 3;
        break;
    case REQUEST_HP_EV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_HP_EV);
        size = 1;
        break;
    case REQUEST_ATK_EV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_ATK_EV);
        size = 1;
        break;
    case REQUEST_DEF_EV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_DEF_EV);
        size = 1;
        break;
    case REQUEST_SPEED_EV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED_EV);
        size = 1;
        break;
    case REQUEST_SPATK_EV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK_EV);
        size = 1;
        break;
    case REQUEST_SPDEF_EV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_EV);
        size = 1;
        break;
    case REQUEST_FRIENDSHIP_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_FRIENDSHIP);
        size = 1;
        break;
    case REQUEST_POKERUS_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_POKERUS);
        size = 1;
        break;
    case REQUEST_MET_LOCATION_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_MET_LOCATION);
        size = 1;
        break;
    case REQUEST_MET_LEVEL_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_MET_LEVEL);
        size = 1;
        break;
    case REQUEST_MET_GAME_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_MET_GAME);
        size = 1;
        break;
    case REQUEST_POKEBALL_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_POKEBALL);
        size = 1;
        break;
    case REQUEST_ALL_IVS_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_HP_IV);
        dst[1] = GetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV);
        dst[2] = GetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV);
        dst[3] = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV);
        dst[4] = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV);
        dst[5] = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV);
        size = 6;
        break;
    case REQUEST_HP_IV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_HP_IV);
        size = 1;
        break;
    case REQUEST_ATK_IV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV);
        size = 1;
        break;
    case REQUEST_DEF_IV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV);
        size = 1;
        break;
    case REQUEST_SPEED_IV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV);
        size = 1;
        break;
    case REQUEST_SPATK_IV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV);
        size = 1;
        break;
    case REQUEST_SPDEF_IV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV);
        size = 1;
        break;
    case REQUEST_PERSONALITY_BATTLE:
        data32 = GetMonData(&gPlayerParty[monId], MON_DATA_PERSONALITY);
        dst[0] = (data32 & 0x000000FF);
        dst[1] = (data32 & 0x0000FF00) >> 8;
        dst[2] = (data32 & 0x00FF0000) >> 16;
        dst[3] = (data32 & 0xFF000000) >> 24;
        size = 4;
        break;
    case REQUEST_CHECKSUM_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_CHECKSUM);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_STATUS_BATTLE:
        data32 = GetMonData(&gPlayerParty[monId], MON_DATA_STATUS);
        dst[0] = (data32 & 0x000000FF);
        dst[1] = (data32 & 0x0000FF00) >> 8;
        dst[2] = (data32 & 0x00FF0000) >> 16;
        dst[3] = (data32 & 0xFF000000) >> 24;
        size = 4;
        break;
    case REQUEST_LEVEL_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_LEVEL);
        size = 1;
        break;
    case REQUEST_HP_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_HP);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_MAX_HP_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_MAX_HP);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_ATK_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_ATK);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_DEF_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_DEF);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_SPEED_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_SPATK_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_SPDEF_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_COOL_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_COOL);
        size = 1;
        break;
    case REQUEST_BEAUTY_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_BEAUTY);
        size = 1;
        break;
    case REQUEST_CUTE_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_CUTE);
        size = 1;
        break;
    case REQUEST_SMART_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SMART);
        size = 1;
        break;
    case REQUEST_TOUGH_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_TOUGH);
        size = 1;
        break;
    case REQUEST_SHEEN_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SHEEN);
        size = 1;
        break;
    case REQUEST_COOL_RIBBON_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_COOL_RIBBON);
        size = 1;
        break;
    case REQUEST_BEAUTY_RIBBON_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_BEAUTY_RIBBON);
        size = 1;
        break;
    case REQUEST_CUTE_RIBBON_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_CUTE_RIBBON);
        size = 1;
        break;
    case REQUEST_SMART_RIBBON_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SMART_RIBBON);
        size = 1;
        break;
    case REQUEST_TOUGH_RIBBON_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_TOUGH_RIBBON);
        size = 1;
        break;
    }

    return size;
}

static void WallyHandleGetRawMonData(void)
{
    PlayerHandleGetRawMonData();
}

static void WallyHandleSetMonData(void)
{
    u8 monToCheck;
    u8 i;

    if (gBattleBufferA[gActiveBattler][2] == 0)
    {
        SetWallyMonData(gBattlerPartyIndexes[gActiveBattler]);
    }
    else
    {
        monToCheck = gBattleBufferA[gActiveBattler][2];
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (monToCheck & 1)
                SetWallyMonData(i);
            monToCheck >>= 1;
        }
    }
    WallyBufferExecCompleted();
}

static void SetWallyMonData(u8 monId)
{
    struct BattlePokemon *battlePokemon = (struct BattlePokemon *)&gBattleBufferA[gActiveBattler][3];
    struct MovePpInfo *moveData = (struct MovePpInfo *)&gBattleBufferA[gActiveBattler][3];
    s32 i;

    switch (gBattleBufferA[gActiveBattler][1])
    {
    case REQUEST_ALL_BATTLE:
        {
            u8 iv;

            SetMonData(&gPlayerParty[monId], MON_DATA_SPECIES, &battlePokemon->species);
            SetMonData(&gPlayerParty[monId], MON_DATA_HELD_ITEM, &battlePokemon->item);
            for (i = 0; i < MAX_MON_MOVES; i++)
            {
                SetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + i, &battlePokemon->moves[i]);
                SetMonData(&gPlayerParty[monId], MON_DATA_PP1 + i, &battlePokemon->pp[i]);
            }
            SetMonData(&gPlayerParty[monId], MON_DATA_PP_BONUSES, &battlePokemon->ppBonuses);
            SetMonData(&gPlayerParty[monId], MON_DATA_FRIENDSHIP, &battlePokemon->friendship);
            SetMonData(&gPlayerParty[monId], MON_DATA_EXP, &battlePokemon->experience);
            iv = battlePokemon->hpIV;
            SetMonData(&gPlayerParty[monId], MON_DATA_HP_IV, &iv);
            iv = battlePokemon->attackIV;
            SetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV, &iv);
            iv = battlePokemon->defenseIV;
            SetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV, &iv);
            iv = battlePokemon->speedIV;
            SetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV, &iv);
            iv = battlePokemon->spAttackIV;
            SetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV, &iv);
            iv = battlePokemon->spDefenseIV;
            SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV, &iv);
            SetMonData(&gPlayerParty[monId], MON_DATA_PERSONALITY, &battlePokemon->personality);
            SetMonData(&gPlayerParty[monId], MON_DATA_STATUS, &battlePokemon->status1);
            SetMonData(&gPlayerParty[monId], MON_DATA_LEVEL, &battlePokemon->level);
            SetMonData(&gPlayerParty[monId], MON_DATA_HP, &battlePokemon->hp);
            SetMonData(&gPlayerParty[monId], MON_DATA_MAX_HP, &battlePokemon->maxHP);
            SetMonData(&gPlayerParty[monId], MON_DATA_ATK, &battlePokemon->attack);
            SetMonData(&gPlayerParty[monId], MON_DATA_DEF, &battlePokemon->defense);
            SetMonData(&gPlayerParty[monId], MON_DATA_SPEED, &battlePokemon->speed);
            SetMonData(&gPlayerParty[monId], MON_DATA_SPATK, &battlePokemon->spAttack);
            SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF, &battlePokemon->spDefense);
        }
        break;
    case REQUEST_SPECIES_BATTLE:
        SetMonData16(&gPlayerParty[monId], MON_DATA_SPECIES, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_HELDITEM_BATTLE:
        SetMonData16(&gPlayerParty[monId], MON_DATA_HELD_ITEM, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_MOVES_PP_BATTLE:
        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            SetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + i, &moveData->moves[i]);
            SetMonData(&gPlayerParty[monId], MON_DATA_PP1 + i, &moveData->pp[i]);
        }
        SetMonData(&gPlayerParty[monId], MON_DATA_PP_BONUSES, &moveData->ppBonuses);
        break;
    case REQUEST_MOVE1_BATTLE:
    case REQUEST_MOVE2_BATTLE:
    case REQUEST_MOVE3_BATTLE:
    case REQUEST_MOVE4_BATTLE:
        SetMonData16(&gPlayerParty[monId], MON_DATA_MOVE1 + gBattleBufferA[gActiveBattler][1] - REQUEST_MOVE1_BATTLE, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_PP_DATA_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_PP1, &gBattleBufferA[gActiveBattler][3]);
        SetMonData(&gPlayerParty[monId], MON_DATA_PP2, &gBattleBufferA[gActiveBattler][4]);
        SetMonData(&gPlayerParty[monId], MON_DATA_PP3, &gBattleBufferA[gActiveBattler][5]);
        SetMonData(&gPlayerParty[monId], MON_DATA_PP4, &gBattleBufferA[gActiveBattler][6]);
        SetMonData(&gPlayerParty[monId], MON_DATA_PP_BONUSES, &gBattleBufferA[gActiveBattler][7]);
        break;
    case REQUEST_PPMOVE1_BATTLE:
    case REQUEST_PPMOVE2_BATTLE:
    case REQUEST_PPMOVE3_BATTLE:
    case REQUEST_PPMOVE4_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_PP1 + gBattleBufferA[gActiveBattler][1] - REQUEST_PPMOVE1_BATTLE, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_OTID_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_OT_ID, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_EXP_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_EXP, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_HP_EV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_HP_EV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_ATK_EV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_ATK_EV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_DEF_EV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_DEF_EV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPEED_EV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPEED_EV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPATK_EV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPATK_EV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPDEF_EV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_EV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_FRIENDSHIP_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_FRIENDSHIP, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_POKERUS_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_POKERUS, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_MET_LOCATION_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_MET_LOCATION, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_MET_LEVEL_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_MET_LEVEL, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_MET_GAME_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_MET_GAME, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_POKEBALL_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_POKEBALL, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_ALL_IVS_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_HP_IV, &gBattleBufferA[gActiveBattler][3]);
        SetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV, &gBattleBufferA[gActiveBattler][4]);
        SetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV, &gBattleBufferA[gActiveBattler][5]);
        SetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV, &gBattleBufferA[gActiveBattler][6]);
        SetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV, &gBattleBufferA[gActiveBattler][7]);
        SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV, &gBattleBufferA[gActiveBattler][8]);
        break;
    case REQUEST_HP_IV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_HP_IV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_ATK_IV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_DEF_IV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPEED_IV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPATK_IV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPDEF_IV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_PERSONALITY_BATTLE:
        SetMonData32(&gPlayerParty[monId], MON_DATA_PERSONALITY, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_CHECKSUM_BATTLE:
        SetMonData16(&gPlayerParty[monId], MON_DATA_CHECKSUM, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_STATUS_BATTLE:
        SetMonData32(&gPlayerParty[monId], MON_DATA_STATUS, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_LEVEL_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_LEVEL, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_HP_BATTLE:
        SetMonData16(&gPlayerParty[monId], MON_DATA_HP, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_MAX_HP_BATTLE:
        SetMonData16(&gPlayerParty[monId], MON_DATA_MAX_HP, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_ATK_BATTLE:
        SetMonData16(&gPlayerParty[monId], MON_DATA_ATK, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_DEF_BATTLE:
        SetMonData16(&gPlayerParty[monId], MON_DATA_DEF, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPEED_BATTLE:
        SetMonData16(&gPlayerParty[monId], MON_DATA_SPEED, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPATK_BATTLE:
        SetMonData16(&gPlayerParty[monId], MON_DATA_SPATK, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPDEF_BATTLE:
        SetMonData16(&gPlayerParty[monId], MON_DATA_SPDEF, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_COOL_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_COOL, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_BEAUTY_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_BEAUTY, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_CUTE_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_CUTE, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SMART_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SMART, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_TOUGH_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_TOUGH, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SHEEN_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SHEEN, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_COOL_RIBBON_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_COOL_RIBBON, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_BEAUTY_RIBBON_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_BEAUTY_RIBBON, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_CUTE_RIBBON_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_CUTE_RIBBON, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SMART_RIBBON_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SMART_RIBBON, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_TOUGH_RIBBON_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_TOUGH_RIBBON, &gBattleBufferA[gActiveBattler][3]);
        break;
    }

    HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
}

static void WallyHandleSetRawMonData(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleLoadMonSprite(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleSwitchInAnim(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleReturnMonToBall(void)
{
    if (gBattleBufferA[gActiveBattler][1] == 0)
    {
        InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_SWITCH_OUT_PLAYER_MON);
        gBattlerControllerFuncs[gActiveBattler] = DoSwitchOutAnimation;
    }
    else
    {
        FreeSpriteOamMatrix(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        DestroySprite(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        SetHealthboxSpriteInvisible(gHealthboxSpriteIds[gActiveBattler]);
        WallyBufferExecCompleted();
    }
}

#define sSpeedX data[0]

static void WallyHandleDrawTrainerPic(void)
{
    DecompressTrainerBackPic(TRAINER_BACK_PIC_WALLY, gActiveBattler);
    SetMultiuseSpriteTemplateToTrainerBack(TRAINER_BACK_PIC_WALLY, GetBattlerPosition(gActiveBattler));
    gBattlerSpriteIds[gActiveBattler] = CreateSprite(&gMultiuseSpriteTemplate,
                                               80,
                                               80 + 4 * (8 - gTrainerBackPicCoords[TRAINER_BACK_PIC_WALLY].size),
                                               30);
    gSprites[gBattlerSpriteIds[gActiveBattler]].oam.paletteNum = gActiveBattler;
    gSprites[gBattlerSpriteIds[gActiveBattler]].x2 = DISPLAY_WIDTH;
    gSprites[gBattlerSpriteIds[gActiveBattler]].sSpeedX = -2;
    gSprites[gBattlerSpriteIds[gActiveBattler]].callback = SpriteCB_TrainerSlideIn;
    gBattlerControllerFuncs[gActiveBattler] = CompleteOnBattlerSpriteCallbackDummy;
}

static void WallyHandleTrainerSlide(void)
{
    DecompressTrainerBackPic(TRAINER_BACK_PIC_WALLY, gActiveBattler);
    SetMultiuseSpriteTemplateToTrainerBack(TRAINER_BACK_PIC_WALLY, GetBattlerPosition(gActiveBattler));
    gBattlerSpriteIds[gActiveBattler] = CreateSprite(&gMultiuseSpriteTemplate,
                                               80,
                                               80 + 4 * (8 - gTrainerBackPicCoords[TRAINER_BACK_PIC_WALLY].size),
                                               30);
    gSprites[gBattlerSpriteIds[gActiveBattler]].oam.paletteNum = gActiveBattler;
    gSprites[gBattlerSpriteIds[gActiveBattler]].x2 = -96;
    gSprites[gBattlerSpriteIds[gActiveBattler]].sSpeedX = 2;
    gSprites[gBattlerSpriteIds[gActiveBattler]].callback = SpriteCB_TrainerSlideIn;
    gBattlerControllerFuncs[gActiveBattler] = CompleteOnBankSpriteCallbackDummy2;
}

#undef sSpeedX

static void WallyHandleTrainerSlideBack(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleFaintAnimation(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandlePaletteFade(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleSuccessBallThrowAnim(void)
{
    gBattleSpritesDataPtr->animationData->ballThrowCaseId = BALL_3_SHAKES_SUCCESS;
    gDoingBattleAnim = TRUE;
    InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT), B_ANIM_BALL_THROW_WITH_TRAINER);
    gBattlerControllerFuncs[gActiveBattler] = CompleteOnFinishedAnimation;
}

static void WallyHandleBallThrowAnim(void)
{
    u8 ballThrowCaseId = gBattleBufferA[gActiveBattler][1];

    gBattleSpritesDataPtr->animationData->ballThrowCaseId = ballThrowCaseId;
    gDoingBattleAnim = TRUE;
    InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT), B_ANIM_BALL_THROW_WITH_TRAINER);
    gBattlerControllerFuncs[gActiveBattler] = CompleteOnFinishedAnimation;
}

static void WallyHandlePause(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleMoveAnimation(void)
{
    u16 move = gBattleBufferA[gActiveBattler][1] | (gBattleBufferA[gActiveBattler][2] << 8);

    gAnimMoveTurn = gBattleBufferA[gActiveBattler][3];
    gAnimMovePower = gBattleBufferA[gActiveBattler][4] | (gBattleBufferA[gActiveBattler][5] << 8);
    gAnimMoveDmg = gBattleBufferA[gActiveBattler][6] | (gBattleBufferA[gActiveBattler][7] << 8) | (gBattleBufferA[gActiveBattler][8] << 16) | (gBattleBufferA[gActiveBattler][9] << 24);
    gAnimFriendship = gBattleBufferA[gActiveBattler][10];
    gWeatherMoveAnim = gBattleBufferA[gActiveBattler][12] | (gBattleBufferA[gActiveBattler][13] << 8);
    gAnimDisableStructPtr = (struct DisableStruct *)&gBattleBufferA[gActiveBattler][16];
    gTransformedPersonalities[gActiveBattler] = gAnimDisableStructPtr->transformedMonPersonality;
    if (IsMoveWithoutAnimation(move, gAnimMoveTurn)) // always returns FALSE
    {
        WallyBufferExecCompleted();
    }
    else
    {
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 0;
        gBattlerControllerFuncs[gActiveBattler] = WallyDoMoveAnimation;
    }

}

static void WallyDoMoveAnimation(void)
{
    u16 move = gBattleBufferA[gActiveBattler][1] | (gBattleBufferA[gActiveBattler][2] << 8);

    switch (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState)
    {
    case 0:
        if (gBattleSpritesDataPtr->battlerData[gActiveBattler].behindSubstitute)
        {
            InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_SUBSTITUTE_TO_MON);
        }
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 1;
        break;
    case 1:
        if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive)
        {
            SetBattlerSpriteAffineMode(ST_OAM_AFFINE_OFF);
            DoMoveAnim(move);
            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 2;
        }
        break;
    case 2:
        gAnimScriptCallback();
        if (!gAnimScriptActive)
        {
            SetBattlerSpriteAffineMode(ST_OAM_AFFINE_NORMAL);
            if (gBattleSpritesDataPtr->battlerData[gActiveBattler].behindSubstitute)
            {
                InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_MON_TO_SUBSTITUTE);
            }
            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 3;
        }
        break;
    case 3:
        if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive)
        {
            CopyAllBattleSpritesInvisibilities();
            TrySetBehindSubstituteSpriteBit(gActiveBattler, gBattleBufferA[gActiveBattler][1] | (gBattleBufferA[gActiveBattler][2] << 8));
            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 0;
            WallyBufferExecCompleted();
        }
        break;
    }
}

static void WallyHandlePrintString(void)
{
    u16 *stringId;

    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    stringId = (u16 *)(&gBattleBufferA[gActiveBattler][2]);
    BufferStringBattle(*stringId);
    BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
    gBattlerControllerFuncs[gActiveBattler] = CompleteOnInactiveTextPrinter;
}

static void WallyHandlePrintSelectionString(void)
{
    if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
        WallyHandlePrintString();
    else
        WallyBufferExecCompleted();
}

static void HandleChooseActionAfterDma3(void)
{
    if (!IsDma3ManagerBusyWithBgCopy())
    {
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = DISPLAY_HEIGHT;
        gBattlerControllerFuncs[gActiveBattler] = WallyHandleActions;
    }
}

static void WallyHandleChooseAction(void)
{
    s32 i;

    gBattlerControllerFuncs[gActiveBattler] = HandleChooseActionAfterDma3;
    BattlePutTextOnWindow(gText_BattleMenu, B_WIN_ACTION_MENU);

    for (i = 0; i < 4; i++)
        ActionSelectionDestroyCursorAt(i);

    ActionSelectionCreateCursorAt(gActionSelectionCursor[gActiveBattler], 0);
    BattleStringExpandPlaceholdersToDisplayedString(gText_WhatWillWallyDo);
    BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_ACTION_PROMPT);
}

static void WallyHandleYesNoBox(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleChooseMove(void)
{
    switch (gBattleStruct->wallyMovesState)
    {
    case 0:
        InitMoveSelectionsVarsAndStrings();
        gBattleStruct->wallyMovesState++;
        gBattleStruct->wallyMoveFrames = 80;
        break;
    case 1:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            gBattle_BG0_X = 0;
            gBattle_BG0_Y = DISPLAY_HEIGHT * 2;
            gBattleStruct->wallyMovesState++;
        }
        break;
    case 2:
        if (--gBattleStruct->wallyMoveFrames == 0)
        {
            PlaySE(SE_SELECT);
            BtlController_EmitTwoReturnValues(BUFFER_B, 10, 0x100);
            WallyBufferExecCompleted();
        }
        break;
    }
}

static void WallyHandleChooseItem(void)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
    gBattlerControllerFuncs[gActiveBattler] = OpenBagAfterPaletteFade;
    gBattlerInMenuId = gActiveBattler;
}

static void WallyHandleChoosePokemon(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleCmd23(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleHealthBarUpdate(void)
{
    s16 hpVal;

    LoadBattleBarGfx(0);
    hpVal = gBattleBufferA[gActiveBattler][2] | (gBattleBufferA[gActiveBattler][3] << 8);

    if (hpVal != INSTANT_HP_BAR_DROP)
    {
        u32 maxHP = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_MAX_HP);
        u32 curHP = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_HP);

        SetBattleBarStruct(gActiveBattler, gHealthboxSpriteIds[gActiveBattler], maxHP, curHP, hpVal);
    }
    else
    {
        u32 maxHP = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_MAX_HP);

        SetBattleBarStruct(gActiveBattler, gHealthboxSpriteIds[gActiveBattler], maxHP, 0, hpVal);
        UpdateHpTextInHealthbox(gHealthboxSpriteIds[gActiveBattler], 0, HP_CURRENT);
    }

    gBattlerControllerFuncs[gActiveBattler] = CompleteOnHealthbarDone;
}

static void WallyHandleExpUpdate(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleStatusIconUpdate(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleStatusAnimation(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleStatusXor(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleDataTransfer(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleDMA3Transfer(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandlePlayBGM(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleCmd32(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleTwoReturnValues(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleChosenMonReturnValue(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleOneReturnValue(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleOneReturnValue_Duplicate(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleClearUnkVar(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleSetUnkVar(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleClearUnkFlag(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleToggleUnkFlag(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleHitAnimation(void)
{
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].invisible == TRUE)
    {
        WallyBufferExecCompleted();
    }
    else
    {
        gDoingBattleAnim = TRUE;
        gSprites[gBattlerSpriteIds[gActiveBattler]].data[1] = 0;
        DoHitAnimHealthboxEffect(gActiveBattler);
        gBattlerControllerFuncs[gActiveBattler] = DoHitAnimBlinkSpriteEffect;
    }
}

static void WallyHandleCantSwitch(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandlePlaySE(void)
{
    PlaySE(gBattleBufferA[gActiveBattler][1] | (gBattleBufferA[gActiveBattler][2] << 8));
    WallyBufferExecCompleted();
}

static void WallyHandlePlayFanfareOrBGM(void)
{
    if (gBattleBufferA[gActiveBattler][3])
    {
        BattleStopLowHpSound();
        PlayBGM(gBattleBufferA[gActiveBattler][1] | (gBattleBufferA[gActiveBattler][2] << 8));
    }
    else
    {
        PlayFanfare(gBattleBufferA[gActiveBattler][1] | (gBattleBufferA[gActiveBattler][2] << 8));
    }

    WallyBufferExecCompleted();
}

static void WallyHandleFaintingCry(void)
{
    u16 species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_SPECIES);

    // Seems that it doesn't bother using CRY_MODE_FAINT because
    // Wally's Pokémon during the tutorial is never intended to faint.
    PlayCry_Normal(species, 25);
    WallyBufferExecCompleted();
}

static void WallyHandleIntroSlide(void)
{
    HandleIntroSlide(gBattleBufferA[gActiveBattler][1]);
    gIntroSlideFlags |= 1;
    WallyBufferExecCompleted();
}

static void WallyHandleIntroTrainerBallThrow(void)
{
    u8 paletteNum;
    u8 taskId;

    SetSpritePrimaryCoordsFromSecondaryCoords(&gSprites[gBattlerSpriteIds[gActiveBattler]]);

    gSprites[gBattlerSpriteIds[gActiveBattler]].data[0] = 50;
    gSprites[gBattlerSpriteIds[gActiveBattler]].data[2] = -40;
    gSprites[gBattlerSpriteIds[gActiveBattler]].data[4] = gSprites[gBattlerSpriteIds[gActiveBattler]].y;
    gSprites[gBattlerSpriteIds[gActiveBattler]].callback = StartAnimLinearTranslation;
    gSprites[gBattlerSpriteIds[gActiveBattler]].data[5] = gActiveBattler;

    StoreSpriteCallbackInData6(&gSprites[gBattlerSpriteIds[gActiveBattler]], SpriteCB_FreePlayerSpriteLoadMonSprite);
    StartSpriteAnim(&gSprites[gBattlerSpriteIds[gActiveBattler]], 1);

    paletteNum = AllocSpritePalette(0xD6F8);
    LoadCompressedPalette(gTrainerBackPicPaletteTable[TRAINER_BACK_PIC_WALLY].data, OBJ_PLTT_ID(paletteNum), PLTT_SIZE_4BPP);
    gSprites[gBattlerSpriteIds[gActiveBattler]].oam.paletteNum = paletteNum;

    taskId = CreateTask(Task_StartSendOutAnim, 5);
    gTasks[taskId].data[0] = gActiveBattler;

    if (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusSummaryShown)
        gTasks[gBattlerStatusSummaryTaskId[gActiveBattler]].func = Task_HidePartyStatusSummary;

    gBattleSpritesDataPtr->animationData->introAnimActive = TRUE;
    gBattlerControllerFuncs[gActiveBattler] = BattleControllerDummy;
}

static void StartSendOutAnim(u8 battlerId)
{
    u16 species;

    gBattleSpritesDataPtr->battlerData[battlerId].transformSpecies = 0;
    gBattlerPartyIndexes[battlerId] = gBattleBufferA[battlerId][1];
    species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
    gBattleControllerData[battlerId] = CreateInvisibleSpriteWithCallback(SpriteCB_WaitForBattlerBallReleaseAnim);
    SetMultiuseSpriteTemplateToPokemon(species, GetBattlerPosition(battlerId));
    gBattlerSpriteIds[battlerId] = CreateSprite(&gMultiuseSpriteTemplate,
                                        GetBattlerSpriteCoord(battlerId, BATTLER_COORD_X_2),
                                        GetBattlerSpriteDefault_Y(battlerId),
                                        GetBattlerSpriteSubpriority(battlerId));

    gSprites[gBattleControllerData[battlerId]].data[1] = gBattlerSpriteIds[battlerId];
    gSprites[gBattleControllerData[battlerId]].data[2] = battlerId;

    gSprites[gBattlerSpriteIds[battlerId]].data[0] = battlerId;
    gSprites[gBattlerSpriteIds[battlerId]].data[2] = species;
    gSprites[gBattlerSpriteIds[battlerId]].oam.paletteNum = battlerId;

    StartSpriteAnim(&gSprites[gBattlerSpriteIds[battlerId]], gBattleMonForms[battlerId]);
    gSprites[gBattlerSpriteIds[battlerId]].invisible = TRUE;
    gSprites[gBattlerSpriteIds[battlerId]].callback = SpriteCallbackDummy;
    gSprites[gBattleControllerData[battlerId]].data[0] = DoPokeballSendOutAnimation(0, POKEBALL_PLAYER_SENDOUT);
}

static void Task_StartSendOutAnim(u8 taskId)
{
    if (gTasks[taskId].data[1] < 31)
    {
        gTasks[taskId].data[1]++;
    }
    else
    {
        u8 savedActiveBank = gActiveBattler;

        gActiveBattler = gTasks[taskId].data[0];
        gBattleBufferA[gActiveBattler][1] = gBattlerPartyIndexes[gActiveBattler];
        StartSendOutAnim(gActiveBattler);
        gBattlerControllerFuncs[gActiveBattler] = Intro_TryShinyAnimShowHealthbox;
        gActiveBattler = savedActiveBank;
        DestroyTask(taskId);
    }
}

static void WallyHandleDrawPartyStatusSummary(void)
{
    if (gBattleBufferA[gActiveBattler][1] != 0 && GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
    {
        WallyBufferExecCompleted();
    }
    else
    {
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusSummaryShown = 1;
        gBattlerStatusSummaryTaskId[gActiveBattler] = CreatePartyStatusSummarySprites(gActiveBattler, (struct HpAndStatus *)&gBattleBufferA[gActiveBattler][4], gBattleBufferA[gActiveBattler][1], gBattleBufferA[gActiveBattler][2]);
        WallyBufferExecCompleted();
    }
}

static void WallyHandleHidePartyStatusSummary(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleEndBounceEffect(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleSpriteInvisibility(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleBattleAnimation(void)
{
    u8 animationId = gBattleBufferA[gActiveBattler][1];
    u16 argument = gBattleBufferA[gActiveBattler][2] | (gBattleBufferA[gActiveBattler][3] << 8);

    if (TryHandleLaunchBattleTableAnimation(gActiveBattler, gActiveBattler, gActiveBattler, animationId, argument))
        WallyBufferExecCompleted();
    else
        gBattlerControllerFuncs[gActiveBattler] = CompleteOnFinishedBattleAnimation;
}

static void WallyHandleLinkStandbyMsg(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleResetActionMoveSelection(void)
{
    WallyBufferExecCompleted();
}

static void WallyHandleEndLinkBattle(void)
{
    gBattleOutcome = gBattleBufferA[gActiveBattler][1];
    FadeOutMapMusic(5);
    BeginFastPaletteFade(3);
    WallyBufferExecCompleted();

    if (!(gBattleTypeFlags & BATTLE_TYPE_IS_MASTER) && gBattleTypeFlags & BATTLE_TYPE_LINK)
        gBattlerControllerFuncs[gActiveBattler] = SetBattleEndCallbacks;
}

static void WallyCmdEnd(void)
{
}

