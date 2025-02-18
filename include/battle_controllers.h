#ifndef GUARD_BATTLE_CONTROLLERS_H
#define GUARD_BATTLE_CONTROLLERS_H

enum {
    REQUEST_ALL_BATTLE,
    REQUEST_SPECIES_BATTLE,
    REQUEST_HELDITEM_BATTLE,
    REQUEST_MOVES_PP_BATTLE,
    REQUEST_MOVE1_BATTLE,
    REQUEST_MOVE2_BATTLE,
    REQUEST_MOVE3_BATTLE,
    REQUEST_MOVE4_BATTLE,
    REQUEST_PP_DATA_BATTLE,
    REQUEST_PPMOVE1_BATTLE,
    REQUEST_PPMOVE2_BATTLE,
    REQUEST_PPMOVE3_BATTLE,
    REQUEST_PPMOVE4_BATTLE,
    REQUEST_UNUSED_13_BATTLE,
    REQUEST_UNUSED_14_BATTLE,
    REQUEST_UNUSED_15_BATTLE,
    REQUEST_UNUSED_16_BATTLE,
    REQUEST_OTID_BATTLE,
    REQUEST_EXP_BATTLE,
    REQUEST_HP_EV_BATTLE,
    REQUEST_ATK_EV_BATTLE,
    REQUEST_DEF_EV_BATTLE,
    REQUEST_SPEED_EV_BATTLE,
    REQUEST_SPATK_EV_BATTLE,
    REQUEST_SPDEF_EV_BATTLE,
    REQUEST_FRIENDSHIP_BATTLE,
    REQUEST_POKERUS_BATTLE,
    REQUEST_MET_LOCATION_BATTLE,
    REQUEST_MET_LEVEL_BATTLE,
    REQUEST_MET_GAME_BATTLE,
    REQUEST_POKEBALL_BATTLE,
    REQUEST_ALL_IVS_BATTLE,
    REQUEST_HP_IV_BATTLE,
    REQUEST_ATK_IV_BATTLE,
    REQUEST_DEF_IV_BATTLE,
    REQUEST_SPEED_IV_BATTLE,
    REQUEST_SPATK_IV_BATTLE,
    REQUEST_SPDEF_IV_BATTLE,
    REQUEST_PERSONALITY_BATTLE,
    REQUEST_CHECKSUM_BATTLE,
    REQUEST_STATUS_BATTLE,
    REQUEST_LEVEL_BATTLE,
    REQUEST_HP_BATTLE,
    REQUEST_MAX_HP_BATTLE,
    REQUEST_ATK_BATTLE,
    REQUEST_DEF_BATTLE,
    REQUEST_SPEED_BATTLE,
    REQUEST_SPATK_BATTLE,
    REQUEST_SPDEF_BATTLE,
    REQUEST_COOL_BATTLE,
    REQUEST_BEAUTY_BATTLE,
    REQUEST_CUTE_BATTLE,
    REQUEST_SMART_BATTLE,
    REQUEST_TOUGH_BATTLE,
    REQUEST_SHEEN_BATTLE,
    REQUEST_COOL_RIBBON_BATTLE,
    REQUEST_BEAUTY_RIBBON_BATTLE,
    REQUEST_CUTE_RIBBON_BATTLE,
    REQUEST_SMART_RIBBON_BATTLE,
    REQUEST_TOUGH_RIBBON_BATTLE,
};

// Special arguments for Battle Controller functions.

enum { // Values given to the emit functions to choose gBattleBufferA or gBattleBufferB
    BUFFER_A,
    BUFFER_B
};

enum {
    RESET_ACTION_MOVE_SELECTION,
    RESET_ACTION_SELECTION,
    RESET_MOVE_SELECTION,
};

enum {
    BALL_NO_SHAKES,
    BALL_1_SHAKE,
    BALL_2_SHAKES,
    BALL_3_SHAKES_FAIL,
    BALL_3_SHAKES_SUCCESS,
    BALL_TRAINER_BLOCK,
};

enum {
    LINK_STANDBY_MSG_STOP_BOUNCE,
    LINK_STANDBY_STOP_BOUNCE_ONLY,
    LINK_STANDBY_MSG_ONLY,
};

#define INSTANT_HP_BAR_DROP     0x7FFF

#define PARTY_SUMM_SKIP_DRAW_DELAY (1 << 7)

// Special return values in gBattleBufferB from Battle Controller functions.
#define RET_VALUE_LEVELED_UP   11

struct UnusedControllerStruct
{
    u8 unk:7;
    u8 flag:1;
};

struct HpAndStatus
{
    u16 hp;
    u32 status;
};

struct MovePpInfo
{
    u16 moves[MAX_MON_MOVES];
    u8 pp[MAX_MON_MOVES];
    u8 ppBonuses;
};

struct ChooseMoveStruct
{
    u16 moves[MAX_MON_MOVES];
    u8 currentPp[MAX_MON_MOVES];
    u8 maxPp[MAX_MON_MOVES];
    u16 species;
    u8 monTypes[2];
};

enum
{
    CONTROLLER_GETMONDATA,
    CONTROLLER_GETRAWMONDATA,
    CONTROLLER_SETMONDATA,
    CONTROLLER_SETRAWMONDATA,
    CONTROLLER_LOADMONSPRITE,
    CONTROLLER_SWITCHINANIM,
    CONTROLLER_RETURNMONTOBALL,
    CONTROLLER_DRAWTRAINERPIC,
    CONTROLLER_TRAINERSLIDE,
    CONTROLLER_TRAINERSLIDEBACK,
    CONTROLLER_FAINTANIMATION,
    CONTROLLER_PALETTEFADE,
    CONTROLLER_SUCCESSBALLTHROWANIM,
    CONTROLLER_BALLTHROWANIM,
    CONTROLLER_PAUSE,
    CONTROLLER_MOVEANIMATION,
    CONTROLLER_PRINTSTRING,
    CONTROLLER_PRINTSTRINGPLAYERONLY,
    CONTROLLER_CHOOSEACTION,
    CONTROLLER_YESNOBOX,
    CONTROLLER_CHOOSEMOVE,
    CONTROLLER_OPENBAG,
    CONTROLLER_CHOOSEPOKEMON,
    CONTROLLER_23,
    CONTROLLER_HEALTHBARUPDATE,
    CONTROLLER_EXPUPDATE,
    CONTROLLER_STATUSICONUPDATE,
    CONTROLLER_STATUSANIMATION,
    CONTROLLER_STATUSXOR,
    CONTROLLER_DATATRANSFER,
    CONTROLLER_DMA3TRANSFER,
    CONTROLLER_PLAYBGM,
    CONTROLLER_32,
    CONTROLLER_TWORETURNVALUES,
    CONTROLLER_CHOSENMONRETURNVALUE,
    CONTROLLER_ONERETURNVALUE,
    CONTROLLER_ONERETURNVALUE_DUPLICATE,
    CONTROLLER_CLEARUNKVAR,
    CONTROLLER_SETUNKVAR,
    CONTROLLER_CLEARUNKFLAG,
    CONTROLLER_TOGGLEUNKFLAG,
    CONTROLLER_HITANIMATION,
    CONTROLLER_CANTSWITCH,
    CONTROLLER_PLAYSE,
    CONTROLLER_PLAYFANFAREORBGM,
    CONTROLLER_FAINTINGCRY,
    CONTROLLER_INTROSLIDE,
    CONTROLLER_INTROTRAINERBALLTHROW,
    CONTROLLER_DRAWPARTYSTATUSSUMMARY,
    CONTROLLER_HIDEPARTYSTATUSSUMMARY,
    CONTROLLER_ENDBOUNCE,
    CONTROLLER_SPRITEINVISIBILITY,
    CONTROLLER_BATTLEANIMATION,
    CONTROLLER_LINKSTANDBYMSG,
    CONTROLLER_RESETACTIONMOVESELECTION,
    CONTROLLER_ENDLINKBATTLE,
    /*new controllers should go here*/
    CONTROLLER_TERMINATOR_NOP,
    CONTROLLER_CMDS_COUNT
};

extern struct UnusedControllerStruct gUnusedControllerStruct;

// general functions
void HandleLinkBattleSetup(void);
void SetUpBattleVarsAndBirchZigzagoon(void);
void InitBattleControllers(void);
void TryReceiveLinkBattleData(void);
void PrepareBufferDataTransferLink(u8 bufferId, u16 size, u8 *data);

// emitters
void BtlController_EmitGetMonData(u8 bufferId, u8 requestId, u8 monToCheck);
void BtlController_EmitSetMonData(u8 bufferId, u8 requestId, u8 monToCheck, u8 bytes, void *data);
void BtlController_EmitLoadMonSprite(u8 bufferId);
void BtlController_EmitSwitchInAnim(u8 bufferId, u8 partyId, bool8 dontClearSubstituteBit);
void BtlController_EmitReturnMonToBall(u8 bufferId, bool8 skipAnim);
void BtlController_EmitDrawTrainerPic(u8 bufferId);
void BtlController_EmitTrainerSlide(u8 bufferId);
void BtlController_EmitTrainerSlideBack(u8 bufferId);
void BtlController_EmitFaintAnimation(u8 bufferId);
void BtlController_EmitBallThrowAnim(u8 bufferId, u8 caseId);
void BtlController_EmitMoveAnimation(u8 bufferId, u16 move, u8 turnOfMove, u16 movePower, s32 dmg, u8 friendship, struct DisableStruct *disableStructPtr, u8 multihit);
void BtlController_EmitPrintString(u8 bufferId, u16 stringId);
void BtlController_EmitPrintSelectionString(u8 bufferId, u16 stringId);
void BtlController_EmitChooseAction(u8 bufferId, u8 action, u16 itemId);
void BtlController_EmitYesNoBox(u8 bufferId);
void BtlController_EmitChooseMove(u8 bufferId, bool8 isDoubleBattle, bool8 NoPpNumber, struct ChooseMoveStruct *movePpData);
void BtlController_EmitChooseItem(u8 bufferId, u8 *battlePartyOrder);
void BtlController_EmitChoosePokemon(u8 bufferId, u8 caseId, u8 slotId, u8 abilityId, u8 *data);
void BtlController_EmitHealthBarUpdate(u8 bufferId, u16 hpValue);
void BtlController_EmitExpUpdate(u8 bufferId, u8 partyId, u16 expPoints);
void BtlController_EmitStatusIconUpdate(u8 bufferId, u32 status1, u32 status2);
void BtlController_EmitStatusAnimation(u8 bufferId, bool8 status2, u32 status);
void BtlController_EmitDataTransfer(u8 bufferId, u16 size, void *data);
void BtlController_EmitTwoReturnValues(u8 bufferId, u8 ret8, u16 ret16);
void BtlController_EmitChosenMonReturnValue(u8 bufferId, u8 partyId, u8 *battlePartyOrder);
void BtlController_EmitOneReturnValue(u8 bufferId, u16 ret);
void BtlController_EmitOneReturnValue_Duplicate(u8 bufferId, u16 ret);
void BtlController_EmitHitAnimation(u8 bufferId);
void BtlController_EmitCantSwitch(u8 bufferId);
void BtlController_EmitPlaySE(u8 bufferId, u16 songId);
void BtlController_EmitPlayFanfareOrBGM(u8 bufferId, u16 songId, bool8 playBGM);
void BtlController_EmitFaintingCry(u8 bufferId);
void BtlController_EmitIntroSlide(u8 bufferId, u8 environmentId);
void BtlController_EmitIntroTrainerBallThrow(u8 bufferId);
void BtlController_EmitDrawPartyStatusSummary(u8 bufferId, struct HpAndStatus* hpAndStatus, u8 flags);
void BtlController_EmitHidePartyStatusSummary(u8 bufferId);
void BtlController_EmitEndBounceEffect(u8 bufferId);
void BtlController_EmitSpriteInvisibility(u8 bufferId, bool8 isInvisible);
void BtlController_EmitBattleAnimation(u8 bufferId, u8 animationId, u16 argument);
void BtlController_EmitLinkStandbyMsg(u8 bufferId, u8 mode, bool32 record);
void BtlController_EmitResetActionMoveSelection(u8 bufferId, u8 caseId);
void BtlController_EmitEndLinkBattle(u8 bufferId, u8 battleOutcome);

// player controller
void SetControllerToPlayer(void);
void BattleControllerDummy(void);
void PlayerHandleGetRawMonData(void);
void SetBattleEndCallbacks(void);
void SpriteCB_FreePlayerSpriteLoadMonSprite(struct Sprite *sprite);
void CB2_SetUpReshowBattleScreenAfterMenu(void);
void CB2_SetUpReshowBattleScreenAfterMenu2(void);
void Task_PlayerController_RestoreBgmAfterCry(u8 taskId);
void ActionSelectionCreateCursorAt(u8 cursorPos, u8 unused);
void ActionSelectionDestroyCursorAt(u8 cursorPos);
void InitMoveSelectionsVarsAndStrings(void);

// recorded player controller
void SetControllerToRecordedPlayer(void);

// opponent controller
void SetControllerToOpponent(void);

// player partner controller
void SetControllerToPlayerPartner(void);

// safari controller
void SetControllerToSafari(void);

// wally controller
void SetControllerToWally(void);

// recorded opponent controller
void SetControllerToRecordedOpponent(void);

// link opponent
void SetControllerToLinkOpponent(void);

// link partner
void SetControllerToLinkPartner(void);

#endif // GUARD_BATTLE_CONTROLLERS_H
