#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_ai_script_commands.h"
#include "battle_arena.h"
#include "battle_controllers.h"
#include "battle_interface.h"
#include "battle_main.h"
#include "battle_message.h"
#include "battle_pyramid.h"
#include "battle_scripts.h"
#include "battle_setup.h"
#include "battle_tower.h"
#include "battle_util.h"
#include "berry.h"
#include "bg.h"
#include "data.h"
#include "decompress.h"
#include "dma3.h"
#include "event_data.h"
#include "evolution_scene.h"
#include "graphics.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "item.h"
#include "link.h"
#include "link_rfu.h"
#include "load_save.h"
#include "main.h"
#include "malloc.h"
#include "m4a.h"
#include "palette.h"
#include "party_menu.h"
#include "pokeball.h"
#include "pokedex.h"
#include "pokemon.h"
#include "random.h"
#include "recorded_battle.h"
#include "roamer.h"
#include "safari_zone.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "trig.h"
#include "tv.h"
#include "util.h"
#include "window.h"
#include "constants/abilities.h"
#include "constants/battle_move_effects.h"
#include "constants/battle_string_ids.h"
#include "constants/hold_effects.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/party_menu.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/trainers.h"
#include "cable_club.h"

extern const struct BgTemplate gBattleBgTemplates[];
extern const struct WindowTemplate *const gBattleWindowTemplates[];

static void CB2_InitBattleInternal(void);
static void CB2_PreInitMultiBattle(void);
static void CB2_PreInitIngamePlayerPartnerBattle(void);
static void CB2_HandleStartMultiPartnerBattle(void);
static void CB2_HandleStartMultiBattle(void);
static void CB2_HandleStartBattle(void);
static void TryCorrectShedinjaLanguage(struct Pokemon *mon);
static u8 CreateNPCTrainerParty(struct Pokemon *party, u16 trainerNum, bool8 firstTrainer);
static void BattleMainCB1(void);
static void CB2_EndLinkBattle(void);
static void EndLinkBattleInSteps(void);
static void CB2_InitAskRecordBattle(void);
static void CB2_AskRecordBattle(void);
static void AskRecordBattle(void);
static void SpriteCB_MoveWildMonToRight(struct Sprite *sprite);
static void SpriteCB_WildMonShowHealthbox(struct Sprite *sprite);
static void SpriteCB_WildMonAnimate(struct Sprite *sprite);
static void SpriteCB_Flicker(struct Sprite *sprite);
static void SpriteCB_AnimFaintOpponent(struct Sprite *sprite);
static void SpriteCB_BlinkVisible(struct Sprite *sprite);
static void SpriteCB_Idle(struct Sprite *sprite);
static void SpriteCB_BattleSpriteSlideLeft(struct Sprite *sprite);
static void TurnValuesCleanUp(bool8 var0);
static void SpriteCB_BounceEffect(struct Sprite *sprite);
static void BattleStartClearSetData(void);
static void BattleIntroGetMonsData(void);
static void BattleIntroPrepareBackgroundSlide(void);
static void BattleIntroDrawTrainersOrMonsSprites(void);
static void BattleIntroDrawPartySummaryScreens(void);
static void BattleIntroPrintTrainerWantsToBattle(void);
static void BattleIntroPrintWildMonAttacked(void);
static void BattleIntroPrintOpponentSendsOut(void);
static void BattleIntroPrintPlayerSendsOut(void);
static void BattleIntroOpponent1SendsOutMonAnimation(void);
static void BattleIntroOpponent2SendsOutMonAnimation(void);
static void BattleIntroRecordMonsToDex(void);
static void BattleIntroPlayer1SendsOutMonAnimation(void);
static void TryDoEventsBeforeFirstTurn(void);
static void HandleTurnActionSelectionState(void);
static void RunTurnActionsFunctions(void);
static void SetActionsAndBattlersTurnOrder(void);
static void UpdateBattlerPartyOrdersOnSwitch(void);
static bool8 AllAtActionConfirmed(void);
static void CheckFocusPunch_ClearVarsBeforeTurnStarts(void);
static void FreeResetData_ReturnToOvOrDoEvolutions(void);
static void ReturnFromBattleToOverworld(void);
static void TryEvolvePokemon(void);
static void WaitForEvoSceneToFinish(void);
static void HandleEndTurn_ContinueBattle(void);
static void HandleEndTurn_BattleWon(void);
static void HandleEndTurn_BattleLost(void);
static void HandleEndTurn_RanFromBattle(void);
static void HandleEndTurn_MonFled(void);
static void HandleEndTurn_FinishBattle(void);
static void SpriteCB_UnusedBattleInit(struct Sprite *sprite);
static void SpriteCB_UnusedBattleInit_Main(struct Sprite *sprite);

EWRAM_DATA u16 gBattle_BG0_X = 0;
EWRAM_DATA u16 gBattle_BG0_Y = 0;
EWRAM_DATA u16 gBattle_BG1_X = 0;
EWRAM_DATA u16 gBattle_BG1_Y = 0;
EWRAM_DATA u16 gBattle_BG2_X = 0;
EWRAM_DATA u16 gBattle_BG2_Y = 0;
EWRAM_DATA u16 gBattle_BG3_X = 0;
EWRAM_DATA u16 gBattle_BG3_Y = 0;
EWRAM_DATA u16 gBattle_WIN0H = 0;
EWRAM_DATA u16 gBattle_WIN0V = 0;
EWRAM_DATA u16 gBattle_WIN1H = 0;
EWRAM_DATA u16 gBattle_WIN1V = 0;
EWRAM_DATA u8 gDisplayedStringBattle[300] = {0};
EWRAM_DATA u8 gBattleTextBuff1[TEXT_BUFF_ARRAY_COUNT] = {0};
EWRAM_DATA u8 gBattleTextBuff2[TEXT_BUFF_ARRAY_COUNT] = {0};
EWRAM_DATA u8 gBattleTextBuff3[TEXT_BUFF_ARRAY_COUNT] = {0};
// The below array is never intentionally used. However, Juan's
// defeat text (SootopolisCity_Gym_1F_Text_JuanDefeat) is too long
// for gDisplayedStringBattle and overflows into this array. If it
// is removed (and none of the buffers above are increased in size)
// it will instead overflow into useful data.
EWRAM_DATA static u32 sFlickerArray[25] = {0};
EWRAM_DATA u32 gBattleTypeFlags = 0;
EWRAM_DATA u8 gBattleEnvironment = 0;
EWRAM_DATA u32 gUnusedFirstBattleVar1 = 0; // Never read
EWRAM_DATA struct MultiPartnerMenuPokemon gMultiPartnerParty[MULTI_PARTY_SIZE] = {0};
EWRAM_DATA static struct MultiPartnerMenuPokemon* sMultiPartnerPartyBuffer = NULL;
EWRAM_DATA u8 *gBattleAnimBgTileBuffer = NULL;
EWRAM_DATA u8 *gBattleAnimBgTilemapBuffer = NULL;
EWRAM_DATA u8 gBattleBufferA[MAX_BATTLERS_COUNT][0x200] = {0};
EWRAM_DATA u8 gBattleBufferB[MAX_BATTLERS_COUNT][0x200] = {0};
EWRAM_DATA u8 gActiveBattler = 0;
EWRAM_DATA u32 gBattleControllerExecFlags = 0;
EWRAM_DATA u8 gBattlersCount = 0;
EWRAM_DATA u16 gBattlerPartyIndexes[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gBattlerPositions[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gActionsByTurnOrder[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gBattlerByTurnOrder[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gCurrentTurnActionNumber = 0;
EWRAM_DATA u8 gCurrentActionFuncId = 0;
EWRAM_DATA struct BattlePokemon gBattleMons[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gBattlerSpriteIds[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gCurrMovePos = 0;
EWRAM_DATA u8 gChosenMovePos = 0;
EWRAM_DATA u16 gCurrentMove = 0;
EWRAM_DATA u16 gChosenMove = 0;
EWRAM_DATA u16 gCalledMove = 0;
EWRAM_DATA s32 gBattleMoveDamage = 0;
EWRAM_DATA s32 gHpDealt = 0;
EWRAM_DATA s32 gBideDmg[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u16 gLastUsedItem = 0;
EWRAM_DATA u8 gLastUsedAbility = 0;
EWRAM_DATA u8 gBattlerAttacker = 0;
EWRAM_DATA u8 gBattlerTarget = 0;
EWRAM_DATA u8 gBattlerFainted = 0;
EWRAM_DATA u8 gEffectBattler = 0;
EWRAM_DATA u8 gPotentialItemEffectBattler = 0;
EWRAM_DATA u8 gAbsentBattlerFlags = 0;
EWRAM_DATA u8 gCritMultiplier = 0;
EWRAM_DATA u8 gMultiHitCounter = 0;
EWRAM_DATA const u8 *gBattlescriptCurrInstr = NULL;
EWRAM_DATA u32 gUnusedBattleMainVar = 0;
EWRAM_DATA u8 gChosenActionByBattler[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA const u8 *gSelectionBattleScripts[MAX_BATTLERS_COUNT] = {NULL};
EWRAM_DATA const u8 *gPalaceSelectionBattleScripts[MAX_BATTLERS_COUNT] = {NULL};
EWRAM_DATA u16 gLastPrintedMoves[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u16 gLastMoves[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u16 gLastLandedMoves[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u16 gLastHitByType[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u16 gLastResultingMoves[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u16 gLockedMoves[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gLastHitBy[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u16 gChosenMoveByBattler[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gMoveResultFlags = 0;
EWRAM_DATA u32 gHitMarker = 0;
EWRAM_DATA static u8 sUnusedBattlersArray[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gBideTarget[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gUnusedFirstBattleVar2 = 0; // Never read
EWRAM_DATA u16 gSideStatuses[NUM_BATTLE_SIDES] = {0};
EWRAM_DATA struct SideTimer gSideTimers[NUM_BATTLE_SIDES] = {0};
EWRAM_DATA u32 gStatuses3[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA struct DisableStruct gDisableStructs[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u16 gPauseCounterBattle = 0;
EWRAM_DATA u16 gPaydayMoney = 0;
EWRAM_DATA u16 gRandomTurnNumber = 0;
EWRAM_DATA u8 gBattleCommunication[BATTLE_COMMUNICATION_ENTRIES_COUNT] = {0};
EWRAM_DATA u8 gBattleOutcome = 0;
EWRAM_DATA struct ProtectStruct gProtectStructs[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA struct SpecialStatus gSpecialStatuses[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u16 gBattleWeather = 0;
EWRAM_DATA struct WishFutureKnock gWishFutureKnock = {0};
EWRAM_DATA u16 gIntroSlideFlags = 0;
EWRAM_DATA u8 gSentPokesToOpponent[2] = {0};
EWRAM_DATA u16 gDynamicBasePower = 0;
EWRAM_DATA u16 gExpShareExp = 0;
EWRAM_DATA struct BattleEnigmaBerry gEnigmaBerries[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA struct BattleScripting gBattleScripting = {0};
EWRAM_DATA struct BattleStruct *gBattleStruct = NULL;
EWRAM_DATA u8 *gLinkBattleSendBuffer = NULL;
EWRAM_DATA u8 *gLinkBattleRecvBuffer = NULL;
EWRAM_DATA struct BattleResources *gBattleResources = NULL;
EWRAM_DATA u8 gActionSelectionCursor[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gMoveSelectionCursor[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gBattlerStatusSummaryTaskId[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gBattlerInMenuId = 0;
EWRAM_DATA bool8 gDoingBattleAnim = FALSE;
EWRAM_DATA u32 gTransformedPersonalities[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u8 gPlayerDpadHoldFrames = 0;
EWRAM_DATA struct BattleSpriteData *gBattleSpritesDataPtr = NULL;
EWRAM_DATA struct MonSpritesGfx *gMonSpritesGfxPtr = NULL;
EWRAM_DATA struct BattleHealthboxInfo *gBattleControllerOpponentHealthboxData = NULL; // Never read
EWRAM_DATA struct BattleHealthboxInfo *gBattleControllerOpponentFlankHealthboxData = NULL; // Never read
EWRAM_DATA u16 gBattleMovePower = 0;
EWRAM_DATA u16 gMoveToLearn = 0;
EWRAM_DATA u8 gBattleMonForms[MAX_BATTLERS_COUNT] = {0};

COMMON_DATA void (*gPreBattleCallback1)(void) = NULL;
COMMON_DATA void (*gBattleMainFunc)(void) = NULL;
COMMON_DATA struct BattleResults gBattleResults = {0};
COMMON_DATA u8 gLeveledUpInBattle = 0;
COMMON_DATA void (*gBattlerControllerFuncs[MAX_BATTLERS_COUNT])(void) = {0};
COMMON_DATA u8 gHealthboxSpriteIds[MAX_BATTLERS_COUNT] = {0};
COMMON_DATA u8 gMultiUsePlayerCursor = 0;
COMMON_DATA u8 gNumberOfMovesToChoose = 0;
COMMON_DATA u8 gBattleControllerData[MAX_BATTLERS_COUNT] = {0}; // Used by the battle controllers to store misc sprite/task IDs for each battler

static const struct ScanlineEffectParams sIntroScanlineParams16Bit =
{
    &REG_BG3HOFS, SCANLINE_EFFECT_DMACNT_16BIT, 1
};

// unused
static const struct ScanlineEffectParams sIntroScanlineParams32Bit =
{
    &REG_BG3HOFS, SCANLINE_EFFECT_DMACNT_32BIT, 1
};

const struct SpriteTemplate gUnusedBattleInitSprite =
{
    .tileTag = 0,
    .paletteTag = 0,
    .oam = &gDummyOamData,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_UnusedBattleInit,
};

static const u8 sText_ShedinjaJpnName[] = _("ヌケニン"); // Nukenin

const struct OamData gOamData_BattleSpriteOpponentSide =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 2,
    .paletteNum = 0,
    .affineParam = 0,
};

const struct OamData gOamData_BattleSpritePlayerSide =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 2,
    .paletteNum = 2,
    .affineParam = 0,
};

static const union AnimCmd sAnim_Unused[] =
{
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd *const sAnims_Unused[] =
{
    sAnim_Unused,
};

static const union AffineAnimCmd sAffineAnim_Unused[] =
{
    AFFINEANIMCMD_FRAME(-0x10, 0x0, 0, 4),
    AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 0x3C),
    AFFINEANIMCMD_JUMP(1),
};

static const union AffineAnimCmd *const sAffineAnims_Unused[] =
{
    sAffineAnim_Unused,
};

static const s8 sCenterToCornerVecXs[8] ={-32, -16, -16, -32, -32};

// format: attacking type, defending type, damage multiplier
// the multiplier is a (decimal) fixed-point number:
// 20 is ×2.0 TYPE_MUL_SUPER_EFFECTIVE
// 10 is ×1.0 TYPE_MUL_NORMAL
// 05 is ×0.5 TYPE_MUL_NOT_EFFECTIVE
// 00 is ×0.0 TYPE_MUL_NO_EFFECT
const u8 gTypeEffectiveness[336] =
{
    TYPE_NORMAL, TYPE_ROCK, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_NORMAL, TYPE_STEEL, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_FIRE, TYPE_FIRE, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_FIRE, TYPE_WATER, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_FIRE, TYPE_GRASS, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_FIRE, TYPE_ICE, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_FIRE, TYPE_BUG, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_FIRE, TYPE_ROCK, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_FIRE, TYPE_DRAGON, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_FIRE, TYPE_STEEL, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_WATER, TYPE_FIRE, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_WATER, TYPE_WATER, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_WATER, TYPE_GRASS, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_WATER, TYPE_GROUND, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_WATER, TYPE_ROCK, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_WATER, TYPE_DRAGON, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_ELECTRIC, TYPE_WATER, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_ELECTRIC, TYPE_ELECTRIC, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_ELECTRIC, TYPE_GRASS, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_ELECTRIC, TYPE_GROUND, TYPE_MUL_NO_EFFECT,
    TYPE_ELECTRIC, TYPE_FLYING, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_ELECTRIC, TYPE_DRAGON, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_GRASS, TYPE_FIRE, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_GRASS, TYPE_WATER, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_GRASS, TYPE_GRASS, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_GRASS, TYPE_POISON, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_GRASS, TYPE_GROUND, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_GRASS, TYPE_FLYING, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_GRASS, TYPE_BUG, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_GRASS, TYPE_ROCK, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_GRASS, TYPE_DRAGON, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_GRASS, TYPE_STEEL, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_ICE, TYPE_WATER, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_ICE, TYPE_GRASS, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_ICE, TYPE_ICE, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_ICE, TYPE_GROUND, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_ICE, TYPE_FLYING, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_ICE, TYPE_DRAGON, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_ICE, TYPE_STEEL, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_ICE, TYPE_FIRE, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_FIGHTING, TYPE_NORMAL, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_FIGHTING, TYPE_ICE, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_FIGHTING, TYPE_POISON, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_FIGHTING, TYPE_FLYING, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_FIGHTING, TYPE_PSYCHIC, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_FIGHTING, TYPE_BUG, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_FIGHTING, TYPE_ROCK, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_FIGHTING, TYPE_DARK, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_FIGHTING, TYPE_STEEL, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_POISON, TYPE_GRASS, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_POISON, TYPE_POISON, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_POISON, TYPE_GROUND, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_POISON, TYPE_ROCK, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_POISON, TYPE_GHOST, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_POISON, TYPE_STEEL, TYPE_MUL_NO_EFFECT,
    TYPE_GROUND, TYPE_FIRE, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_GROUND, TYPE_ELECTRIC, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_GROUND, TYPE_GRASS, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_GROUND, TYPE_POISON, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_GROUND, TYPE_FLYING, TYPE_MUL_NO_EFFECT,
    TYPE_GROUND, TYPE_BUG, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_GROUND, TYPE_ROCK, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_GROUND, TYPE_STEEL, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_FLYING, TYPE_ELECTRIC, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_FLYING, TYPE_GRASS, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_FLYING, TYPE_FIGHTING, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_FLYING, TYPE_BUG, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_FLYING, TYPE_ROCK, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_FLYING, TYPE_STEEL, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_PSYCHIC, TYPE_FIGHTING, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_PSYCHIC, TYPE_POISON, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_PSYCHIC, TYPE_PSYCHIC, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_PSYCHIC, TYPE_DARK, TYPE_MUL_NO_EFFECT,
    TYPE_PSYCHIC, TYPE_STEEL, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_BUG, TYPE_FIRE, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_BUG, TYPE_GRASS, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_BUG, TYPE_FIGHTING, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_BUG, TYPE_POISON, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_BUG, TYPE_FLYING, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_BUG, TYPE_PSYCHIC, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_BUG, TYPE_GHOST, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_BUG, TYPE_DARK, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_BUG, TYPE_STEEL, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_ROCK, TYPE_FIRE, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_ROCK, TYPE_ICE, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_ROCK, TYPE_FIGHTING, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_ROCK, TYPE_GROUND, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_ROCK, TYPE_FLYING, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_ROCK, TYPE_BUG, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_ROCK, TYPE_STEEL, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_GHOST, TYPE_NORMAL, TYPE_MUL_NO_EFFECT,
    TYPE_GHOST, TYPE_PSYCHIC, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_GHOST, TYPE_DARK, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_GHOST, TYPE_STEEL, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_GHOST, TYPE_GHOST, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_DRAGON, TYPE_DRAGON, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_DRAGON, TYPE_STEEL, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_DARK, TYPE_FIGHTING, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_DARK, TYPE_PSYCHIC, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_DARK, TYPE_GHOST, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_DARK, TYPE_DARK, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_DARK, TYPE_STEEL, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_STEEL, TYPE_FIRE, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_STEEL, TYPE_WATER, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_STEEL, TYPE_ELECTRIC, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_STEEL, TYPE_ICE, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_STEEL, TYPE_ROCK, TYPE_MUL_SUPER_EFFECTIVE,
    TYPE_STEEL, TYPE_STEEL, TYPE_MUL_NOT_EFFECTIVE,
    TYPE_FORESIGHT, TYPE_FORESIGHT, TYPE_MUL_NO_EFFECT,
    TYPE_NORMAL, TYPE_GHOST, TYPE_MUL_NO_EFFECT,
    TYPE_FIGHTING, TYPE_GHOST, TYPE_MUL_NO_EFFECT,
    TYPE_ENDTABLE, TYPE_ENDTABLE, TYPE_MUL_NO_EFFECT
};

const u8 gTypeNames[NUMBER_OF_MON_TYPES][TYPE_NAME_LENGTH + 1] =
{
    [TYPE_NORMAL] = _("NORMAL"),
    [TYPE_FIGHTING] = _("FIGHT"),
    [TYPE_FLYING] = _("FLYING"),
    [TYPE_POISON] = _("POISON"),
    [TYPE_GROUND] = _("GROUND"),
    [TYPE_ROCK] = _("ROCK"),
    [TYPE_BUG] = _("BUG"),
    [TYPE_GHOST] = _("GHOST"),
    [TYPE_STEEL] = _("STEEL"),
    [TYPE_MYSTERY] = _("???"),
    [TYPE_FIRE] = _("FIRE"),
    [TYPE_WATER] = _("WATER"),
    [TYPE_GRASS] = _("GRASS"),
    [TYPE_ELECTRIC] = _("ELECTR"),
    [TYPE_PSYCHIC] = _("PSYCHC"),
    [TYPE_ICE] = _("ICE"),
    [TYPE_DRAGON] = _("DRAGON"),
    [TYPE_DARK] = _("DARK"),
};

// This is a factor in how much money you get for beating a trainer.
const struct TrainerMoney gTrainerMoneyTable[] =
{
    {TRAINER_CLASS_TEAM_AQUA, 5},
    {TRAINER_CLASS_AQUA_ADMIN, 10},
    {TRAINER_CLASS_AQUA_LEADER, 20},
    {TRAINER_CLASS_AROMA_LADY, 10},
    {TRAINER_CLASS_RUIN_MANIAC, 15},
    {TRAINER_CLASS_INTERVIEWER, 12},
    {TRAINER_CLASS_TUBER_F, 1},
    {TRAINER_CLASS_TUBER_M, 1},
    {TRAINER_CLASS_SIS_AND_BRO, 3},
    {TRAINER_CLASS_COOLTRAINER, 12},
    {TRAINER_CLASS_HEX_MANIAC, 6},
    {TRAINER_CLASS_LADY, 50},
    {TRAINER_CLASS_BEAUTY, 20},
    {TRAINER_CLASS_RICH_BOY, 50},
    {TRAINER_CLASS_POKEMANIAC, 15},
    {TRAINER_CLASS_SWIMMER_M, 2},
    {TRAINER_CLASS_BLACK_BELT, 8},
    {TRAINER_CLASS_GUITARIST, 8},
    {TRAINER_CLASS_KINDLER, 8},
    {TRAINER_CLASS_CAMPER, 4},
    {TRAINER_CLASS_OLD_COUPLE, 10},
    {TRAINER_CLASS_BUG_MANIAC, 15},
    {TRAINER_CLASS_PSYCHIC, 6},
    {TRAINER_CLASS_GENTLEMAN, 20},
    {TRAINER_CLASS_ELITE_FOUR, 25},
    {TRAINER_CLASS_LEADER, 25},
    {TRAINER_CLASS_SCHOOL_KID, 5},
    {TRAINER_CLASS_SR_AND_JR, 4},
    {TRAINER_CLASS_POKEFAN, 20},
    {TRAINER_CLASS_EXPERT, 10},
    {TRAINER_CLASS_YOUNGSTER, 4},
    {TRAINER_CLASS_CHAMPION, 50},
    {TRAINER_CLASS_FISHERMAN, 10},
    {TRAINER_CLASS_TRIATHLETE, 10},
    {TRAINER_CLASS_DRAGON_TAMER, 12},
    {TRAINER_CLASS_BIRD_KEEPER, 8},
    {TRAINER_CLASS_NINJA_BOY, 3},
    {TRAINER_CLASS_BATTLE_GIRL, 6},
    {TRAINER_CLASS_PARASOL_LADY, 10},
    {TRAINER_CLASS_SWIMMER_F, 2},
    {TRAINER_CLASS_PICNICKER, 4},
    {TRAINER_CLASS_TWINS, 3},
    {TRAINER_CLASS_SAILOR, 8},
    {TRAINER_CLASS_COLLECTOR, 15},
    {TRAINER_CLASS_RIVAL, 15},
    {TRAINER_CLASS_PKMN_BREEDER, 10},
    {TRAINER_CLASS_PKMN_RANGER, 12},
    {TRAINER_CLASS_TEAM_MAGMA, 5},
    {TRAINER_CLASS_MAGMA_ADMIN, 10},
    {TRAINER_CLASS_MAGMA_LEADER, 20},
    {TRAINER_CLASS_LASS, 4},
    {TRAINER_CLASS_BUG_CATCHER, 4},
    {TRAINER_CLASS_HIKER, 10},
    {TRAINER_CLASS_YOUNG_COUPLE, 8},
    {TRAINER_CLASS_WINSTRATE, 10},
    {0xFF, 5}, // Any trainer class not listed above uses this
};

#include "data/text/abilities.h"

static void (* const sTurnActionsFuncsTable[])(void) =
{
    [B_ACTION_USE_MOVE]               = HandleAction_UseMove,
    [B_ACTION_USE_ITEM]               = HandleAction_UseItem,
    [B_ACTION_SWITCH]                 = HandleAction_Switch,
    [B_ACTION_RUN]                    = HandleAction_Run,
    [B_ACTION_SAFARI_WATCH_CAREFULLY] = HandleAction_WatchesCarefully,
    [B_ACTION_SAFARI_BALL]            = HandleAction_SafariZoneBallThrow,
    [B_ACTION_SAFARI_POKEBLOCK]       = HandleAction_ThrowPokeblock,
    [B_ACTION_SAFARI_GO_NEAR]         = HandleAction_GoNear,
    [B_ACTION_SAFARI_RUN]             = HandleAction_SafariZoneRun,
    [B_ACTION_WALLY_THROW]            = HandleAction_WallyBallThrow,
    [B_ACTION_EXEC_SCRIPT]            = HandleAction_RunBattleScript,
    [B_ACTION_TRY_FINISH]             = HandleAction_TryFinish,
    [B_ACTION_FINISHED]               = HandleAction_ActionFinished,
    [B_ACTION_NOTHING_FAINTED]        = HandleAction_NothingIsFainted,
};

static void (* const sEndTurnFuncsTable[])(void) =
{
    [0]                           = HandleEndTurn_ContinueBattle,
    [B_OUTCOME_WON]               = HandleEndTurn_BattleWon,
    [B_OUTCOME_LOST]              = HandleEndTurn_BattleLost,
    [B_OUTCOME_DREW]              = HandleEndTurn_BattleLost,
    [B_OUTCOME_RAN]               = HandleEndTurn_RanFromBattle,
    [B_OUTCOME_PLAYER_TELEPORTED] = HandleEndTurn_FinishBattle,
    [B_OUTCOME_MON_FLED]          = HandleEndTurn_MonFled,
    [B_OUTCOME_CAUGHT]            = HandleEndTurn_FinishBattle,
    [B_OUTCOME_NO_SAFARI_BALLS]   = HandleEndTurn_FinishBattle,
    [B_OUTCOME_FORFEITED]         = HandleEndTurn_FinishBattle,
    [B_OUTCOME_MON_TELEPORTED]    = HandleEndTurn_FinishBattle,
};

const u8 gStatusConditionString_PoisonJpn[] = _("どく$$$$$");
const u8 gStatusConditionString_SleepJpn[] = _("ねむり$$$$");
const u8 gStatusConditionString_ParalysisJpn[] = _("まひ$$$$$");
const u8 gStatusConditionString_BurnJpn[] = _("やけど$$$$");
const u8 gStatusConditionString_IceJpn[] = _("こおり$$$$");
const u8 gStatusConditionString_ConfusionJpn[] = _("こんらん$$$");
const u8 gStatusConditionString_LoveJpn[] = _("メロメロ$$$");

const u8 * const gStatusConditionStringsTable[][2] =
{
    {gStatusConditionString_PoisonJpn, gText_Poison},
    {gStatusConditionString_SleepJpn, gText_Sleep},
    {gStatusConditionString_ParalysisJpn, gText_Paralysis},
    {gStatusConditionString_BurnJpn, gText_Burn},
    {gStatusConditionString_IceJpn, gText_Ice},
    {gStatusConditionString_ConfusionJpn, gText_Confusion},
    {gStatusConditionString_LoveJpn, gText_Love}
};

void CB2_InitBattle(void)
{
    MoveSaveBlocks_ResetHeap();
    AllocateBattleResources();
    AllocateBattleSpritesData();
    AllocateMonSpritesGfx();
    RecordedBattle_ClearFrontierPassFlag();

    if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
        {
            CB2_InitBattleInternal();
        }
        else if (!(gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER))
        {
            HandleLinkBattleSetup();
            SetMainCallback2(CB2_PreInitMultiBattle);
        }
        else
        {
            SetMainCallback2(CB2_PreInitIngamePlayerPartnerBattle);
        }
        gBattleCommunication[MULTIUSE_STATE] = 0;
    }
    else
    {
        CB2_InitBattleInternal();
    }
}

static void CB2_InitBattleInternal(void)
{
    s32 i;

    SetHBlankCallback(NULL);
    SetVBlankCallback(NULL);

    CpuFill32(0, (void *)(VRAM), VRAM_SIZE);

    SetGpuReg(REG_OFFSET_MOSAIC, 0);
    SetGpuReg(REG_OFFSET_WIN0H, DISPLAY_WIDTH);
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(DISPLAY_HEIGHT / 2, DISPLAY_HEIGHT / 2 + 1));
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);

    gBattle_WIN0H = DISPLAY_WIDTH;

    if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER && gPartnerTrainerId != TRAINER_STEVEN_PARTNER)
    {
        gBattle_WIN0V = DISPLAY_HEIGHT - 1;
        gBattle_WIN1H = DISPLAY_WIDTH;
        gBattle_WIN1V = 32;
    }
    else
    {
        gBattle_WIN0V = WIN_RANGE(DISPLAY_HEIGHT / 2, DISPLAY_HEIGHT / 2 + 1);
        ScanlineEffect_Clear();

        for (i = 0; i < DISPLAY_HEIGHT / 2; i++)
        {
            gScanlineEffectRegBuffers[0][i] = 0xF0;
            gScanlineEffectRegBuffers[1][i] = 0xF0;
        }

        for (; i < DISPLAY_HEIGHT; i++)
        {
            gScanlineEffectRegBuffers[0][i] = 0xFF10;
            gScanlineEffectRegBuffers[1][i] = 0xFF10;
        }

        ScanlineEffect_SetParams(sIntroScanlineParams16Bit);
    }

    ResetPaletteFade();
    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    gBattle_BG1_X = 0;
    gBattle_BG1_Y = 0;
    gBattle_BG2_X = 0;
    gBattle_BG2_Y = 0;
    gBattle_BG3_X = 0;
    gBattle_BG3_Y = 0;

    gBattleEnvironment = BattleSetup_GetEnvironmentId();
    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
        gBattleEnvironment = BATTLE_ENVIRONMENT_BUILDING;

    InitBattleBgsVideo();
    LoadBattleTextboxAndBackground();
    ResetSpriteData();
    ResetTasks();
    DrawBattleEntryBackground();
    FreeAllSpritePalettes();
    gReservedSpritePaletteCount = MAX_BATTLERS_COUNT;
    SetVBlankCallback(VBlankCB_Battle);
    SetUpBattleVarsAndBirchZigzagoon();

    if (gBattleTypeFlags & BATTLE_TYPE_MULTI && gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER)
        SetMainCallback2(CB2_HandleStartMultiPartnerBattle);
    else if (gBattleTypeFlags & BATTLE_TYPE_MULTI && gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
        SetMainCallback2(CB2_HandleStartMultiPartnerBattle);
    else if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
        SetMainCallback2(CB2_HandleStartMultiBattle);
    else
        SetMainCallback2(CB2_HandleStartBattle);

    if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED)))
    {
        CreateNPCTrainerParty(&gEnemyParty[0], gTrainerBattleOpponent_A, TRUE);
        if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
            CreateNPCTrainerParty(&gEnemyParty[PARTY_SIZE / 2], gTrainerBattleOpponent_B, FALSE);
        SetWildMonHeldItem();
    }

    gMain.inBattle = TRUE;
    gSaveBlock2Ptr->frontier.disableRecordBattle = FALSE;

    for (i = 0; i < PARTY_SIZE; i++)
        AdjustFriendship(&gPlayerParty[i], FRIENDSHIP_EVENT_LEAGUE_BATTLE);

    gBattleCommunication[MULTIUSE_STATE] = 0;
}

#define BUFFER_PARTY_VS_SCREEN_STATUS(party, flags, i)                      \
    for ((i) = 0; (i) < PARTY_SIZE; (i)++)                                  \
    {                                                                       \
        u16 species = GetMonData(&(party)[(i)], MON_DATA_SPECIES_OR_EGG);   \
        u16 hp = GetMonData(&(party)[(i)], MON_DATA_HP);                    \
        u32 status = GetMonData(&(party)[(i)], MON_DATA_STATUS);            \
                                                                            \
        if (species == SPECIES_NONE)                                        \
            continue;                                                       \
                                                                            \
        /* Is healthy mon? */                                               \
        if (species != SPECIES_EGG && hp != 0 && status == 0)               \
            (flags) |= 1 << (i) * 2;                                        \
                                                                            \
        if (species == SPECIES_NONE) /* Redundant */                        \
            continue;                                                       \
                                                                            \
        /* Is Egg or statused? */                                           \
        if (hp != 0 && (species == SPECIES_EGG || status != 0))             \
            (flags) |= 2 << (i) * 2;                                        \
                                                                            \
        if (species == SPECIES_NONE) /* Redundant */                        \
            continue;                                                       \
                                                                            \
        /* Is fainted? */                                                   \
        if (species != SPECIES_EGG && hp == 0)                              \
            (flags) |= 3 << (i) * 2;                                        \
    }

// For Vs Screen at link battle start
static void BufferPartyVsScreenHealth_AtStart(void)
{
    u16 flags = 0;
    s32 i;

    BUFFER_PARTY_VS_SCREEN_STATUS(gPlayerParty, flags, i);
    gBattleStruct->multiBuffer.linkBattlerHeader.vsScreenHealthFlagsLo = flags;
    *(&gBattleStruct->multiBuffer.linkBattlerHeader.vsScreenHealthFlagsHi) = flags >> 8;
    gBattleStruct->multiBuffer.linkBattlerHeader.vsScreenHealthFlagsHi |= FlagGet(FLAG_SYS_FRONTIER_PASS) << 7;
}

static void SetPlayerBerryDataInBattleStruct(void)
{
    s32 i;
    struct BattleStruct *battleStruct = gBattleStruct;
    struct BattleEnigmaBerry *battleBerry = &battleStruct->multiBuffer.linkBattlerHeader.battleEnigmaBerry;

    if (IsEnigmaBerryValid() == TRUE)
    {
        for (i = 0; i < BERRY_NAME_LENGTH; i++)
            battleBerry->name[i] = gSaveBlock1Ptr->enigmaBerry.berry.name[i];
        battleBerry->name[i] = EOS;

        for (i = 0; i < BERRY_ITEM_EFFECT_COUNT; i++)
            battleBerry->itemEffect[i] = gSaveBlock1Ptr->enigmaBerry.itemEffect[i];

        battleBerry->holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
        battleBerry->holdEffectParam = gSaveBlock1Ptr->enigmaBerry.holdEffectParam;
    }
    else
    {
        const struct Berry *berryData = GetBerryInfo(ItemIdToBerryType(ITEM_ENIGMA_BERRY));

        for (i = 0; i < BERRY_NAME_LENGTH; i++)
            battleBerry->name[i] = berryData->name[i];
        battleBerry->name[i] = EOS;

        for (i = 0; i < BERRY_ITEM_EFFECT_COUNT; i++)
            battleBerry->itemEffect[i] = 0;

        battleBerry->holdEffect = HOLD_EFFECT_NONE;
        battleBerry->holdEffectParam = 0;
    }
}

static void SetAllPlayersBerryData(void)
{
    s32 i, j;

    if (!(gBattleTypeFlags & BATTLE_TYPE_LINK))
    {
        if (IsEnigmaBerryValid() == TRUE)
        {
            for (i = 0; i < BERRY_NAME_LENGTH; i++)
            {
                gEnigmaBerries[0].name[i] = gSaveBlock1Ptr->enigmaBerry.berry.name[i];
                gEnigmaBerries[2].name[i] = gSaveBlock1Ptr->enigmaBerry.berry.name[i];
            }
            gEnigmaBerries[0].name[i] = EOS;
            gEnigmaBerries[2].name[i] = EOS;

            for (i = 0; i < BERRY_ITEM_EFFECT_COUNT; i++)
            {
                gEnigmaBerries[0].itemEffect[i] = gSaveBlock1Ptr->enigmaBerry.itemEffect[i];
                gEnigmaBerries[2].itemEffect[i] = gSaveBlock1Ptr->enigmaBerry.itemEffect[i];
            }

            gEnigmaBerries[0].holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
            gEnigmaBerries[2].holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
            gEnigmaBerries[0].holdEffectParam = gSaveBlock1Ptr->enigmaBerry.holdEffectParam;
            gEnigmaBerries[2].holdEffectParam = gSaveBlock1Ptr->enigmaBerry.holdEffectParam;
        }
        else
        {
            const struct Berry *berryData = GetBerryInfo(ItemIdToBerryType(ITEM_ENIGMA_BERRY));

            for (i = 0; i < BERRY_NAME_LENGTH; i++)
            {
                gEnigmaBerries[0].name[i] = berryData->name[i];
                gEnigmaBerries[2].name[i] = berryData->name[i];
            }
            gEnigmaBerries[0].name[i] = EOS;
            gEnigmaBerries[2].name[i] = EOS;

            for (i = 0; i < BERRY_ITEM_EFFECT_COUNT; i++)
            {
                gEnigmaBerries[0].itemEffect[i] = 0;
                gEnigmaBerries[2].itemEffect[i] = 0;
            }

            gEnigmaBerries[0].holdEffect = HOLD_EFFECT_NONE;
            gEnigmaBerries[2].holdEffect = HOLD_EFFECT_NONE;
            gEnigmaBerries[0].holdEffectParam = 0;
            gEnigmaBerries[2].holdEffectParam = 0;
        }
    }
    else
    {
        s32 numPlayers;
        struct BattleEnigmaBerry *src;
        u8 battlerId;

        if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
        {
            if (gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER)
                numPlayers = 2;
            else
                numPlayers = 4;

            for (i = 0; i < numPlayers; i++)
            {
                src = (struct BattleEnigmaBerry *)(gBlockRecvBuffer[i] + 2);
                battlerId = gLinkPlayers[i].id;

                for (j = 0; j < BERRY_NAME_LENGTH; j++)
                    gEnigmaBerries[battlerId].name[j] = src->name[j];
                gEnigmaBerries[battlerId].name[j] = EOS;

                for (j = 0; j < BERRY_ITEM_EFFECT_COUNT; j++)
                    gEnigmaBerries[battlerId].itemEffect[j] = src->itemEffect[j];

                gEnigmaBerries[battlerId].holdEffect = src->holdEffect;
                gEnigmaBerries[battlerId].holdEffectParam = src->holdEffectParam;
            }
        }
        else
        {
            for (i = 0; i < 2; i++)
            {
                src = (struct BattleEnigmaBerry *)(gBlockRecvBuffer[i] + 2);

                for (j = 0; j < BERRY_NAME_LENGTH; j++)
                {
                    gEnigmaBerries[i].name[j] = src->name[j];
                    gEnigmaBerries[i + 2].name[j] = src->name[j];
                }
                gEnigmaBerries[i].name[j] = EOS;
                gEnigmaBerries[i + 2].name[j] = EOS;

                for (j = 0; j < BERRY_ITEM_EFFECT_COUNT; j++)
                {
                    gEnigmaBerries[i].itemEffect[j] = src->itemEffect[j];
                    gEnigmaBerries[i + 2].itemEffect[j] = src->itemEffect[j];
                }

                gEnigmaBerries[i].holdEffect = src->holdEffect;
                gEnigmaBerries[i + 2].holdEffect = src->holdEffect;
                gEnigmaBerries[i].holdEffectParam = src->holdEffectParam;
                gEnigmaBerries[i + 2].holdEffectParam = src->holdEffectParam;
            }
        }
    }
}

// This was inlined in Ruby/Sapphire
static void FindLinkBattleMaster(u8 numPlayers, u8 multiPlayerId)
{
    u8 found = 0;

    // If player 1 is playing the minimum version, player 1 is master.
    if (gBlockRecvBuffer[0][0] == 0x100)
    {
        if (multiPlayerId == 0)
            gBattleTypeFlags |= BATTLE_TYPE_IS_MASTER | BATTLE_TYPE_TRAINER;
        else
            gBattleTypeFlags |= BATTLE_TYPE_TRAINER;
        found++;
    }

    if (found == 0)
    {
        // If multiple different versions are being used, player 1 is master.
        s32 i;

        for (i = 0; i < numPlayers; i++)
        {
            if (gBlockRecvBuffer[0][0] != gBlockRecvBuffer[i][0])
                break;
        }

        if (i == numPlayers)
        {
            if (multiPlayerId == 0)
                gBattleTypeFlags |= BATTLE_TYPE_IS_MASTER | BATTLE_TYPE_TRAINER;
            else
                gBattleTypeFlags |= BATTLE_TYPE_TRAINER;
            found++;
        }

        if (found == 0)
        {
            // Lowest index player with the highest game version is master.
            for (i = 0; i < numPlayers; i++)
            {
                if (gBlockRecvBuffer[i][0] == 0x300 && i != multiPlayerId)
                {
                    if (i < multiPlayerId)
                        break;
                }
                if (gBlockRecvBuffer[i][0] > 0x300 && i != multiPlayerId)
                    break;
            }

            if (i == numPlayers)
                gBattleTypeFlags |= BATTLE_TYPE_IS_MASTER | BATTLE_TYPE_TRAINER;
            else
                gBattleTypeFlags |= BATTLE_TYPE_TRAINER;
        }
    }
}

static void CB2_HandleStartBattle(void)
{
    u8 playerMultiplayerId;
    u8 enemyMultiplayerId;

    RunTasks();
    AnimateSprites();
    BuildOamBuffer();

    playerMultiplayerId = GetMultiplayerId();
    gBattleScripting.multiplayerId = playerMultiplayerId;
    enemyMultiplayerId = playerMultiplayerId ^ BIT_SIDE;

    switch (gBattleCommunication[MULTIUSE_STATE])
    {
    case 0:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            ShowBg(0);
            ShowBg(1);
            ShowBg(2);
            ShowBg(3);
            FillAroundBattleWindows();
            gBattleCommunication[MULTIUSE_STATE] = 1;
        }
        if (gWirelessCommType)
            LoadWirelessStatusIndicatorSpriteGfx();
        break;
    case 1:
        if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        {
            if (gReceivedRemoteLinkPlayers)
            {
                if (IsLinkTaskFinished())
                {
                    // 0x300
                    *(&gBattleStruct->multiBuffer.linkBattlerHeader.versionSignatureLo) = 0;
                    *(&gBattleStruct->multiBuffer.linkBattlerHeader.versionSignatureHi) = 3;
                    BufferPartyVsScreenHealth_AtStart();
                    SetPlayerBerryDataInBattleStruct();

                    if (gTrainerBattleOpponent_A == TRAINER_UNION_ROOM)
                    {
                        gLinkPlayers[0].id = 0;
                        gLinkPlayers[1].id = 1;
                    }

                    SendBlock(BitmaskAllOtherLinkPlayers(), &gBattleStruct->multiBuffer.linkBattlerHeader, sizeof(gBattleStruct->multiBuffer.linkBattlerHeader));
                    gBattleCommunication[MULTIUSE_STATE] = 2;
                }
                if (gWirelessCommType)
                    CreateWirelessStatusIndicatorSprite(0, 0);
            }
        }
        else
        {
            if (!(gBattleTypeFlags & BATTLE_TYPE_RECORDED))
                gBattleTypeFlags |= BATTLE_TYPE_IS_MASTER;
            gBattleCommunication[MULTIUSE_STATE] = 15;
            SetAllPlayersBerryData();
        }
        break;
    case 2:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            u8 taskId;

            ResetBlockReceivedFlags();
            FindLinkBattleMaster(2, playerMultiplayerId);
            SetAllPlayersBerryData();
            taskId = CreateTask(InitLinkBattleVsScreen, 0);
            gTasks[taskId].data[1] = 0x10E;
            gTasks[taskId].data[2] = 0x5A;
            gTasks[taskId].data[5] = 0;
            gTasks[taskId].data[3] = gBattleStruct->multiBuffer.linkBattlerHeader.vsScreenHealthFlagsLo | (gBattleStruct->multiBuffer.linkBattlerHeader.vsScreenHealthFlagsHi << 8);
            gTasks[taskId].data[4] = gBlockRecvBuffer[enemyMultiplayerId][1];
            RecordedBattle_SetFrontierPassFlagFromHword(gBlockRecvBuffer[playerMultiplayerId][1]);
            RecordedBattle_SetFrontierPassFlagFromHword(gBlockRecvBuffer[enemyMultiplayerId][1]);
            SetDeoxysStats();
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 3:
        // Link battle, send/receive party Pokémon 2 at a time
        if (IsLinkTaskFinished())
        {
            // Send Pokémon 1-2
            SendBlock(BitmaskAllOtherLinkPlayers(), gPlayerParty, sizeof(struct Pokemon) * 2);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 4:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            // Recv Pokémon 1-2
            ResetBlockReceivedFlags();
            memcpy(gEnemyParty, gBlockRecvBuffer[enemyMultiplayerId], sizeof(struct Pokemon) * 2);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 7:
        if (IsLinkTaskFinished())
        {
            // Send Pokémon 3-4
            SendBlock(BitmaskAllOtherLinkPlayers(), &gPlayerParty[2], sizeof(struct Pokemon) * 2);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 8:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            // Recv Pokémon 3-4
            ResetBlockReceivedFlags();
            memcpy(&gEnemyParty[2], gBlockRecvBuffer[enemyMultiplayerId], sizeof(struct Pokemon) * 2);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 11:
        if (IsLinkTaskFinished())
        {
            // Send Pokémon 5-6
            SendBlock(BitmaskAllOtherLinkPlayers(), &gPlayerParty[4], sizeof(struct Pokemon) * 2);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 12:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            // Recv Pokémon 5-6
            ResetBlockReceivedFlags();
            memcpy(&gEnemyParty[4], gBlockRecvBuffer[enemyMultiplayerId], sizeof(struct Pokemon) * 2);

            TryCorrectShedinjaLanguage(&gEnemyParty[0]);
            TryCorrectShedinjaLanguage(&gEnemyParty[1]);
            TryCorrectShedinjaLanguage(&gEnemyParty[2]);
            TryCorrectShedinjaLanguage(&gEnemyParty[3]);
            TryCorrectShedinjaLanguage(&gEnemyParty[4]);
            TryCorrectShedinjaLanguage(&gEnemyParty[5]);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 15:
        InitBattleControllers();
        RecordedBattle_SetTrainerInfo();
        gBattleCommunication[SPRITES_INIT_STATE1] = 0;
        gBattleCommunication[SPRITES_INIT_STATE2] = 0;
        if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        {
            // Check if both players are using Emerald
            // to determine if the recorded battle rng
            // seed needs to be sent
            s32 i;
            for (i = 0; i < 2 && (gLinkPlayers[i].version & 0xFF) == VERSION_EMERALD; i++);

            if (i == 2)
                gBattleCommunication[MULTIUSE_STATE] = 16;
            else
                gBattleCommunication[MULTIUSE_STATE] = 18;
        }
        else
        {
            gBattleCommunication[MULTIUSE_STATE] = 18;
        }
        break;
    case 16:
        // Both players are using Emerald, send rng seed for recorded battle
        if (IsLinkTaskFinished())
        {
            SendBlock(BitmaskAllOtherLinkPlayers(), &gRecordedBattleRngSeed, sizeof(gRecordedBattleRngSeed));
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 17:
        // Receive rng seed for recorded battle (only read it if partner is the link master)
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            ResetBlockReceivedFlags();
            if (!(gBattleTypeFlags & BATTLE_TYPE_IS_MASTER))
                memcpy(&gRecordedBattleRngSeed, gBlockRecvBuffer[enemyMultiplayerId], sizeof(gRecordedBattleRngSeed));
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 18:
        // Finish, start battle
        if (BattleInitAllSprites(&gBattleCommunication[SPRITES_INIT_STATE1], &gBattleCommunication[SPRITES_INIT_STATE2]))
        {
            gPreBattleCallback1 = gMain.callback1;
            gMain.callback1 = BattleMainCB1;
            SetMainCallback2(BattleMainCB2);
            if (gBattleTypeFlags & BATTLE_TYPE_LINK)
                gBattleTypeFlags |= BATTLE_TYPE_LINK_IN_BATTLE;
        }
        break;
    // Introduce short delays between sending party Pokémon for link
    case 5:
    case 9:
    case 13:
        gBattleCommunication[MULTIUSE_STATE]++;
        gBattleCommunication[1] = 1;
    case 6:
    case 10:
    case 14:
        if (--gBattleCommunication[1] == 0)
            gBattleCommunication[MULTIUSE_STATE]++;
        break;
    }
}

static void CB2_HandleStartMultiPartnerBattle(void)
{
    u8 playerMultiplayerId;
    u8 partnerMultiplayerId;

    RunTasks();
    AnimateSprites();
    BuildOamBuffer();

    playerMultiplayerId = GetMultiplayerId();
    gBattleScripting.multiplayerId = playerMultiplayerId;
    partnerMultiplayerId = playerMultiplayerId ^ BIT_SIDE;

    switch (gBattleCommunication[MULTIUSE_STATE])
    {
    case 0:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            ShowBg(0);
            ShowBg(1);
            ShowBg(2);
            ShowBg(3);
            FillAroundBattleWindows();
            gBattleCommunication[MULTIUSE_STATE] = 1;
        }
        if (gWirelessCommType)
            LoadWirelessStatusIndicatorSpriteGfx();
        // fall through
    case 1:
        if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        {
            if (gReceivedRemoteLinkPlayers)
            {
                u8 language;

                gLinkPlayers[0].id = 0;
                gLinkPlayers[1].id = 2;
                gLinkPlayers[2].id = 1;
                gLinkPlayers[3].id = 3;
                GetFrontierTrainerName(gLinkPlayers[2].name, gTrainerBattleOpponent_A);
                GetFrontierTrainerName(gLinkPlayers[3].name, gTrainerBattleOpponent_B);
                GetBattleTowerTrainerLanguage(&language, gTrainerBattleOpponent_A);
                gLinkPlayers[2].language = language;
                GetBattleTowerTrainerLanguage(&language, gTrainerBattleOpponent_B);
                gLinkPlayers[3].language = language;

                if (IsLinkTaskFinished())
                {
                    // 0x300
                    *(&gBattleStruct->multiBuffer.linkBattlerHeader.versionSignatureLo) = 0;
                    *(&gBattleStruct->multiBuffer.linkBattlerHeader.versionSignatureHi) = 3;
                    BufferPartyVsScreenHealth_AtStart();
                    SetPlayerBerryDataInBattleStruct();
                    SendBlock(BitmaskAllOtherLinkPlayers(), &gBattleStruct->multiBuffer.linkBattlerHeader, sizeof(gBattleStruct->multiBuffer.linkBattlerHeader));
                    gBattleCommunication[MULTIUSE_STATE] = 2;
                }

                if (gWirelessCommType)
                    CreateWirelessStatusIndicatorSprite(0, 0);
            }
        }
        else
        {
            if (!(gBattleTypeFlags & BATTLE_TYPE_RECORDED))
                gBattleTypeFlags |= BATTLE_TYPE_IS_MASTER;
            gBattleCommunication[MULTIUSE_STATE] = 13;
            SetAllPlayersBerryData();
        }
        break;
    case 2:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            u8 taskId;

            ResetBlockReceivedFlags();
            FindLinkBattleMaster(2, playerMultiplayerId);
            SetAllPlayersBerryData();
            taskId = CreateTask(InitLinkBattleVsScreen, 0);
            gTasks[taskId].data[1] = 0x10E;
            gTasks[taskId].data[2] = 0x5A;
            gTasks[taskId].data[5] = 0;
            gTasks[taskId].data[3] = 0x145;
            gTasks[taskId].data[4] = 0x145;
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 3:
        // Link battle, send/receive party Pokémon in groups
        if (IsLinkTaskFinished())
        {
            // Send Pokémon 1-2
            SendBlock(BitmaskAllOtherLinkPlayers(), gPlayerParty, sizeof(struct Pokemon) * 2);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 4:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            // Recv partner's Pokémon 1-2, and copy partner's and own Pokémon into party positions
            ResetBlockReceivedFlags();
            if (gLinkPlayers[playerMultiplayerId].id != 0)
            {
                memcpy(gPlayerParty, gBlockRecvBuffer[partnerMultiplayerId], sizeof(struct Pokemon) * 2);
                memcpy(&gPlayerParty[MULTI_PARTY_SIZE], gBlockRecvBuffer[playerMultiplayerId], sizeof(struct Pokemon) * 2);
            }
            else
            {
                memcpy(gPlayerParty, gBlockRecvBuffer[playerMultiplayerId], sizeof(struct Pokemon) * 2);
                memcpy(&gPlayerParty[MULTI_PARTY_SIZE], gBlockRecvBuffer[partnerMultiplayerId], sizeof(struct Pokemon) * 2);
            }
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 5:
        if (IsLinkTaskFinished())
        {
            // Send Pokémon 3
            SendBlock(BitmaskAllOtherLinkPlayers(), &gPlayerParty[2], sizeof(struct Pokemon));
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 6:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            // Recv partner's Pokémon 3, and copy partner's and own Pokémon into party positions
            ResetBlockReceivedFlags();
            if (gLinkPlayers[playerMultiplayerId].id != 0)
            {
                memcpy(&gPlayerParty[2], gBlockRecvBuffer[partnerMultiplayerId], sizeof(struct Pokemon));
                memcpy(&gPlayerParty[2 + MULTI_PARTY_SIZE], gBlockRecvBuffer[playerMultiplayerId], sizeof(struct Pokemon));
            }
            else
            {
                memcpy(&gPlayerParty[2], gBlockRecvBuffer[playerMultiplayerId], sizeof(struct Pokemon));
                memcpy(&gPlayerParty[2 + MULTI_PARTY_SIZE], gBlockRecvBuffer[partnerMultiplayerId], sizeof(struct Pokemon));
            }
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 7:
        if (IsLinkTaskFinished())
        {
            // Send enemy Pokémon 1-2 to partner
            SendBlock(BitmaskAllOtherLinkPlayers(), gEnemyParty, sizeof(struct Pokemon) * 2);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 8:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            // Recv enemy Pokémon 1-2 (if not master)
            ResetBlockReceivedFlags();
            if (GetMultiplayerId() != 0)
                memcpy(gEnemyParty, gBlockRecvBuffer[0], sizeof(struct Pokemon) * 2);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 9:
        if (IsLinkTaskFinished())
        {
            // Send enemy Pokémon 3-4 to partner
            SendBlock(BitmaskAllOtherLinkPlayers(), &gEnemyParty[2], sizeof(struct Pokemon) * 2);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 10:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            // Recv enemy Pokémon 3-4 (if not master)
            ResetBlockReceivedFlags();
            if (GetMultiplayerId() != 0)
                memcpy(&gEnemyParty[2], gBlockRecvBuffer[0], sizeof(struct Pokemon) * 2);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 11:
        if (IsLinkTaskFinished())
        {
            // Send enemy Pokémon 5-6 to partner
            SendBlock(BitmaskAllOtherLinkPlayers(), &gEnemyParty[4], sizeof(struct Pokemon) * 2);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 12:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            // Recv enemy Pokémon 5-6 (if not master)
            ResetBlockReceivedFlags();
            if (GetMultiplayerId() != 0)
                memcpy(&gEnemyParty[4], gBlockRecvBuffer[0], sizeof(struct Pokemon) * 2);

            TryCorrectShedinjaLanguage(&gPlayerParty[0]);
            TryCorrectShedinjaLanguage(&gPlayerParty[1]);
            TryCorrectShedinjaLanguage(&gPlayerParty[2]);
            TryCorrectShedinjaLanguage(&gPlayerParty[3]);
            TryCorrectShedinjaLanguage(&gPlayerParty[4]);
            TryCorrectShedinjaLanguage(&gPlayerParty[5]);
            TryCorrectShedinjaLanguage(&gEnemyParty[0]);
            TryCorrectShedinjaLanguage(&gEnemyParty[1]);
            TryCorrectShedinjaLanguage(&gEnemyParty[2]);
            TryCorrectShedinjaLanguage(&gEnemyParty[3]);
            TryCorrectShedinjaLanguage(&gEnemyParty[4]);
            TryCorrectShedinjaLanguage(&gEnemyParty[5]);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 13:
        InitBattleControllers();
        RecordedBattle_SetTrainerInfo();
        gBattleCommunication[SPRITES_INIT_STATE1] = 0;
        gBattleCommunication[SPRITES_INIT_STATE2] = 0;
        if (gBattleTypeFlags & BATTLE_TYPE_LINK)
            gBattleCommunication[MULTIUSE_STATE] = 14;
        else
            gBattleCommunication[MULTIUSE_STATE] = 16;
        break;
    case 14:
        // Send rng seed for recorded battle
        if (IsLinkTaskFinished())
        {
            SendBlock(BitmaskAllOtherLinkPlayers(), &gRecordedBattleRngSeed, sizeof(gRecordedBattleRngSeed));
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 15:
        // Receive rng seed for recorded battle (only read it if partner is the link master)
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            ResetBlockReceivedFlags();
            if (!(gBattleTypeFlags & BATTLE_TYPE_IS_MASTER))
                memcpy(&gRecordedBattleRngSeed, gBlockRecvBuffer[partnerMultiplayerId], sizeof(gRecordedBattleRngSeed));
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 16:
        // Finish, start battle
        if (BattleInitAllSprites(&gBattleCommunication[SPRITES_INIT_STATE1], &gBattleCommunication[SPRITES_INIT_STATE2]))
        {
            TrySetLinkBattleTowerEnemyPartyLevel();
            gPreBattleCallback1 = gMain.callback1;
            gMain.callback1 = BattleMainCB1;
            SetMainCallback2(BattleMainCB2);
            if (gBattleTypeFlags & BATTLE_TYPE_LINK)
                gBattleTypeFlags |= BATTLE_TYPE_LINK_IN_BATTLE;
        }
        break;
    }
}

static void SetMultiPartnerMenuParty(u8 offset)
{
    s32 i;

    for (i = 0; i < MULTI_PARTY_SIZE; i++)
    {
        gMultiPartnerParty[i].species     = GetMonData(&gPlayerParty[offset + i], MON_DATA_SPECIES);
        gMultiPartnerParty[i].heldItem    = GetMonData(&gPlayerParty[offset + i], MON_DATA_HELD_ITEM);
        GetMonData(&gPlayerParty[offset + i], MON_DATA_NICKNAME, gMultiPartnerParty[i].nickname);
        gMultiPartnerParty[i].level       = GetMonData(&gPlayerParty[offset + i], MON_DATA_LEVEL);
        gMultiPartnerParty[i].hp          = GetMonData(&gPlayerParty[offset + i], MON_DATA_HP);
        gMultiPartnerParty[i].maxhp       = GetMonData(&gPlayerParty[offset + i], MON_DATA_MAX_HP);
        gMultiPartnerParty[i].status      = GetMonData(&gPlayerParty[offset + i], MON_DATA_STATUS);
        gMultiPartnerParty[i].personality = GetMonData(&gPlayerParty[offset + i], MON_DATA_PERSONALITY);
        gMultiPartnerParty[i].gender      = GetMonGender(&gPlayerParty[offset + i]);
        StripExtCtrlCodes(gMultiPartnerParty[i].nickname);
        if (GetMonData(&gPlayerParty[offset + i], MON_DATA_LANGUAGE) != LANGUAGE_JAPANESE)
            PadNameString(gMultiPartnerParty[i].nickname, CHAR_SPACE);
    }
    memcpy(sMultiPartnerPartyBuffer, gMultiPartnerParty, sizeof(gMultiPartnerParty));
}

static void CB2_PreInitMultiBattle(void)
{
    s32 i;
    u8 playerMultiplierId;
    s32 numPlayers = MAX_BATTLERS_COUNT;
    u8 blockMask = 0xF;
    u32 *savedBattleTypeFlags;
    void (**savedCallback)(void);

    if (gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER)
    {
        numPlayers = 2;
        blockMask = 3;
    }

    playerMultiplierId = GetMultiplayerId();
    gBattleScripting.multiplayerId = playerMultiplierId;
    savedCallback = &gBattleStruct->savedCallback;
    savedBattleTypeFlags = &gBattleStruct->savedBattleTypeFlags;

    RunTasks();
    AnimateSprites();
    BuildOamBuffer();

    switch (gBattleCommunication[MULTIUSE_STATE])
    {
    case 0:
        if (gReceivedRemoteLinkPlayers && IsLinkTaskFinished())
        {
            sMultiPartnerPartyBuffer = Alloc(sizeof(gMultiPartnerParty));
            SetMultiPartnerMenuParty(0);
            SendBlock(BitmaskAllOtherLinkPlayers(), sMultiPartnerPartyBuffer, sizeof(gMultiPartnerParty));
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 1:
        if ((GetBlockReceivedStatus() & blockMask) == blockMask)
        {
            ResetBlockReceivedFlags();
            for (i = 0; i < numPlayers; i++)
            {
                if (i == playerMultiplierId)
                    continue;

                if (numPlayers == MAX_LINK_PLAYERS)
                {
                    if ((!(gLinkPlayers[i].id & 1) && !(gLinkPlayers[playerMultiplierId].id & 1))
                        || (gLinkPlayers[i].id & 1 && gLinkPlayers[playerMultiplierId].id & 1))
                    {
                        memcpy(gMultiPartnerParty, gBlockRecvBuffer[i], sizeof(gMultiPartnerParty));
                    }
                }
                else
                {
                    memcpy(gMultiPartnerParty, gBlockRecvBuffer[i], sizeof(gMultiPartnerParty));
                }
            }
            gBattleCommunication[MULTIUSE_STATE]++;
            *savedCallback = gMain.savedCallback;
            *savedBattleTypeFlags = gBattleTypeFlags;
            gMain.savedCallback = CB2_PreInitMultiBattle;
            ShowPartyMenuToShowcaseMultiBattleParty();
        }
        break;
    case 2:
        if (IsLinkTaskFinished() && !gPaletteFade.active)
        {
            gBattleCommunication[MULTIUSE_STATE]++;
            if (gWirelessCommType)
                SetLinkStandbyCallback();
            else
                SetCloseLinkCallback();
        }
        break;
    case 3:
        if (gWirelessCommType)
        {
            if (IsLinkRfuTaskFinished())
            {
                gBattleTypeFlags = *savedBattleTypeFlags;
                gMain.savedCallback = *savedCallback;
                SetMainCallback2(CB2_InitBattleInternal);
                FREE_AND_SET_NULL(sMultiPartnerPartyBuffer);
            }
        }
        else if (gReceivedRemoteLinkPlayers == 0)
        {
            gBattleTypeFlags = *savedBattleTypeFlags;
            gMain.savedCallback = *savedCallback;
            SetMainCallback2(CB2_InitBattleInternal);
            FREE_AND_SET_NULL(sMultiPartnerPartyBuffer);
        }
        break;
    }
}

static void CB2_PreInitIngamePlayerPartnerBattle(void)
{
    u32 *savedBattleTypeFlags;
    void (**savedCallback)(void);

    savedCallback = &gBattleStruct->savedCallback;
    savedBattleTypeFlags = &gBattleStruct->savedBattleTypeFlags;

    RunTasks();
    AnimateSprites();
    BuildOamBuffer();

    switch (gBattleCommunication[MULTIUSE_STATE])
    {
    case 0:
        sMultiPartnerPartyBuffer = Alloc(sizeof(gMultiPartnerParty));
        SetMultiPartnerMenuParty(MULTI_PARTY_SIZE);
        gBattleCommunication[MULTIUSE_STATE]++;
        *savedCallback = gMain.savedCallback;
        *savedBattleTypeFlags = gBattleTypeFlags;
        gMain.savedCallback = CB2_PreInitIngamePlayerPartnerBattle;
        ShowPartyMenuToShowcaseMultiBattleParty();
        break;
    case 1:
        if (!gPaletteFade.active)
        {
            gBattleCommunication[MULTIUSE_STATE] = 2;
            gBattleTypeFlags = *savedBattleTypeFlags;
            gMain.savedCallback = *savedCallback;
            SetMainCallback2(CB2_InitBattleInternal);
            FREE_AND_SET_NULL(sMultiPartnerPartyBuffer);
        }
        break;
    }
}

static void CB2_HandleStartMultiBattle(void)
{
    u8 playerMultiplayerId;
    s32 id;
    u8 var;

    playerMultiplayerId = GetMultiplayerId();
    gBattleScripting.multiplayerId = playerMultiplayerId;

    RunTasks();
    AnimateSprites();
    BuildOamBuffer();

    switch (gBattleCommunication[MULTIUSE_STATE])
    {
    case 0:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            ShowBg(0);
            ShowBg(1);
            ShowBg(2);
            ShowBg(3);
            FillAroundBattleWindows();
            gBattleCommunication[MULTIUSE_STATE] = 1;
        }
        if (gWirelessCommType)
            LoadWirelessStatusIndicatorSpriteGfx();
        break;
    case 1:
        if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        {
            if (gReceivedRemoteLinkPlayers)
            {
                if (IsLinkTaskFinished())
                {
                    // 0x300
                    *(&gBattleStruct->multiBuffer.linkBattlerHeader.versionSignatureLo) = 0;
                    *(&gBattleStruct->multiBuffer.linkBattlerHeader.versionSignatureHi) = 3;
                    BufferPartyVsScreenHealth_AtStart();
                    SetPlayerBerryDataInBattleStruct();

                    SendBlock(BitmaskAllOtherLinkPlayers(), &gBattleStruct->multiBuffer.linkBattlerHeader, sizeof(gBattleStruct->multiBuffer.linkBattlerHeader));
                    gBattleCommunication[MULTIUSE_STATE]++;
                }
                if (gWirelessCommType)
                    CreateWirelessStatusIndicatorSprite(0, 0);
            }
        }
        else
        {
            if (!(gBattleTypeFlags & BATTLE_TYPE_RECORDED))
                gBattleTypeFlags |= BATTLE_TYPE_IS_MASTER;
            gBattleCommunication[MULTIUSE_STATE] = 7;
            SetAllPlayersBerryData();
        }
        break;
    case 2:
        if ((GetBlockReceivedStatus() & 0xF) == 0xF)
        {
            ResetBlockReceivedFlags();
            FindLinkBattleMaster(4, playerMultiplayerId);
            SetAllPlayersBerryData();
            SetDeoxysStats();
            var = CreateTask(InitLinkBattleVsScreen, 0);
            gTasks[var].data[1] = 0x10E;
            gTasks[var].data[2] = 0x5A;
            gTasks[var].data[5] = 0;
            gTasks[var].data[3] = 0;
            gTasks[var].data[4] = 0;

            for (id = 0; id < MAX_LINK_PLAYERS; id++)
            {
                RecordedBattle_SetFrontierPassFlagFromHword(gBlockRecvBuffer[id][1]);
                switch (gLinkPlayers[id].id)
                {
                case 0:
                    gTasks[var].data[3] |= gBlockRecvBuffer[id][1] & 0x3F;
                    break;
                case 1:
                    gTasks[var].data[4] |= gBlockRecvBuffer[id][1] & 0x3F;
                    break;
                case 2:
                    gTasks[var].data[3] |= (gBlockRecvBuffer[id][1] & 0x3F) << 6;
                    break;
                case 3:
                    gTasks[var].data[4] |= (gBlockRecvBuffer[id][1] & 0x3F) << 6;
                    break;
                }
            }
            ZeroEnemyPartyMons();
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        else
        {
            break;
        }
        // fall through
    case 3:
        if (IsLinkTaskFinished())
        {
            SendBlock(BitmaskAllOtherLinkPlayers(), gPlayerParty, sizeof(struct Pokemon) * 2);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 4:
        if ((GetBlockReceivedStatus() & 0xF) == 0xF)
        {
            ResetBlockReceivedFlags();
            for (id = 0; id < MAX_LINK_PLAYERS; id++)
            {
                if (id == playerMultiplayerId)
                {
                    switch (gLinkPlayers[id].id)
                    {
                    case 0:
                    case 3:
                        memcpy(gPlayerParty, gBlockRecvBuffer[id], sizeof(struct Pokemon) * 2);
                        break;
                    case 1:
                    case 2:
                        memcpy(gPlayerParty + MULTI_PARTY_SIZE, gBlockRecvBuffer[id], sizeof(struct Pokemon) * 2);
                        break;
                    }
                }
                else
                {
                    if ((!(gLinkPlayers[id].id & 1) && !(gLinkPlayers[playerMultiplayerId].id & 1))
                     || ((gLinkPlayers[id].id & 1) && (gLinkPlayers[playerMultiplayerId].id & 1)))
                    {
                        switch (gLinkPlayers[id].id)
                        {
                        case 0:
                        case 3:
                            memcpy(gPlayerParty, gBlockRecvBuffer[id], sizeof(struct Pokemon) * 2);
                            break;
                        case 1:
                        case 2:
                            memcpy(gPlayerParty + MULTI_PARTY_SIZE, gBlockRecvBuffer[id], sizeof(struct Pokemon) * 2);
                            break;
                        }
                    }
                    else
                    {
                        switch (gLinkPlayers[id].id)
                        {
                        case 0:
                        case 3:
                            memcpy(gEnemyParty, gBlockRecvBuffer[id], sizeof(struct Pokemon) * 2);
                            break;
                        case 1:
                        case 2:
                            memcpy(gEnemyParty + MULTI_PARTY_SIZE, gBlockRecvBuffer[id], sizeof(struct Pokemon) * 2);
                            break;
                        }
                    }
                }
            }
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 5:
        if (IsLinkTaskFinished())
        {
            SendBlock(BitmaskAllOtherLinkPlayers(), gPlayerParty + 2, sizeof(struct Pokemon));
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 6:
        if ((GetBlockReceivedStatus() & 0xF) == 0xF)
        {
            ResetBlockReceivedFlags();
            for (id = 0; id < MAX_LINK_PLAYERS; id++)
            {
                if (id == playerMultiplayerId)
                {
                    switch (gLinkPlayers[id].id)
                    {
                    case 0:
                    case 3:
                        memcpy(gPlayerParty + 2, gBlockRecvBuffer[id], sizeof(struct Pokemon));
                        break;
                    case 1:
                    case 2:
                        memcpy(gPlayerParty + 5, gBlockRecvBuffer[id], sizeof(struct Pokemon));
                        break;
                    }
                }
                else
                {
                    if ((!(gLinkPlayers[id].id & 1) && !(gLinkPlayers[playerMultiplayerId].id & 1))
                     || ((gLinkPlayers[id].id & 1) && (gLinkPlayers[playerMultiplayerId].id & 1)))
                    {
                        switch (gLinkPlayers[id].id)
                        {
                        case 0:
                        case 3:
                            memcpy(gPlayerParty + 2, gBlockRecvBuffer[id], sizeof(struct Pokemon));
                            break;
                        case 1:
                        case 2:
                            memcpy(gPlayerParty + 5, gBlockRecvBuffer[id], sizeof(struct Pokemon));
                            break;
                        }
                    }
                    else
                    {
                        switch (gLinkPlayers[id].id)
                        {
                        case 0:
                        case 3:
                            memcpy(gEnemyParty + 2, gBlockRecvBuffer[id], sizeof(struct Pokemon));
                            break;
                        case 1:
                        case 2:
                            memcpy(gEnemyParty + 5, gBlockRecvBuffer[id], sizeof(struct Pokemon));
                            break;
                        }
                    }
                }
            }
            TryCorrectShedinjaLanguage(&gPlayerParty[0]);
            TryCorrectShedinjaLanguage(&gPlayerParty[1]);
            TryCorrectShedinjaLanguage(&gPlayerParty[2]);
            TryCorrectShedinjaLanguage(&gPlayerParty[3]);
            TryCorrectShedinjaLanguage(&gPlayerParty[4]);
            TryCorrectShedinjaLanguage(&gPlayerParty[5]);

            TryCorrectShedinjaLanguage(&gEnemyParty[0]);
            TryCorrectShedinjaLanguage(&gEnemyParty[1]);
            TryCorrectShedinjaLanguage(&gEnemyParty[2]);
            TryCorrectShedinjaLanguage(&gEnemyParty[3]);
            TryCorrectShedinjaLanguage(&gEnemyParty[4]);
            TryCorrectShedinjaLanguage(&gEnemyParty[5]);

            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 7:
        InitBattleControllers();
        RecordedBattle_SetTrainerInfo();
        gBattleCommunication[SPRITES_INIT_STATE1] = 0;
        gBattleCommunication[SPRITES_INIT_STATE2] = 0;
        if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        {
            for (id = 0; id < MAX_LINK_PLAYERS && (gLinkPlayers[id].version & 0xFF) == VERSION_EMERALD; id++);

            if (id == MAX_LINK_PLAYERS)
                gBattleCommunication[MULTIUSE_STATE] = 8;
            else
                gBattleCommunication[MULTIUSE_STATE] = 10;
        }
        else
        {
            gBattleCommunication[MULTIUSE_STATE] = 10;
        }
        break;
    case 8:
        if (IsLinkTaskFinished())
        {
            u32 *ptr = gBattleStruct->multiBuffer.battleVideo;
            ptr[0] = gBattleTypeFlags;
            ptr[1] = gRecordedBattleRngSeed; // UB: overwrites berry data
            SendBlock(BitmaskAllOtherLinkPlayers(), ptr, sizeof(gBattleStruct->multiBuffer.battleVideo));
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 9:
        if ((GetBlockReceivedStatus() & 0xF) == 0xF)
        {
            ResetBlockReceivedFlags();
            for (var = 0; var < 4; var++)
            {
                u32 blockValue = gBlockRecvBuffer[var][0];
                if (blockValue & 4)
                {
                    memcpy(&gRecordedBattleRngSeed, &gBlockRecvBuffer[var][2], sizeof(gRecordedBattleRngSeed));
                    break;
                }
            }

            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 10:
        if (BattleInitAllSprites(&gBattleCommunication[SPRITES_INIT_STATE1], &gBattleCommunication[SPRITES_INIT_STATE2]))
        {
            gPreBattleCallback1 = gMain.callback1;
            gMain.callback1 = BattleMainCB1;
            SetMainCallback2(BattleMainCB2);
            if (gBattleTypeFlags & BATTLE_TYPE_LINK)
            {
                gTrainerBattleOpponent_A = TRAINER_LINK_OPPONENT;
                gBattleTypeFlags |= BATTLE_TYPE_LINK_IN_BATTLE;
            }
        }
        break;
    }
}

void BattleMainCB2(void)
{
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
    RunTasks();

    if (JOY_HELD(B_BUTTON) && gBattleTypeFlags & BATTLE_TYPE_RECORDED && RecordedBattle_CanStopPlayback())
    {
        // Player pressed B during recorded battle playback, end battle
        gSpecialVar_Result = gBattleOutcome = B_OUTCOME_PLAYER_TELEPORTED;
        ResetPaletteFadeControl();
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        SetMainCallback2(CB2_QuitRecordedBattle);
    }
}

static void FreeRestoreBattleData(void)
{
    gMain.callback1 = gPreBattleCallback1;
    gScanlineEffect.state = 3;
    gMain.inBattle = FALSE;
    ZeroEnemyPartyMons();
    m4aSongNumStop(SE_LOW_HEALTH);
    FreeMonSpritesGfx();
    FreeBattleSpritesData();
    FreeBattleResources();
}

void CB2_QuitRecordedBattle(void)
{
    UpdatePaletteFade();
    if (!gPaletteFade.active)
    {
        m4aMPlayStop(&gMPlayInfo_SE1);
        m4aMPlayStop(&gMPlayInfo_SE2);
        FreeRestoreBattleData();
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
    }
}

#define sState data[0]
#define sDelay data[4]

static void SpriteCB_UnusedBattleInit(struct Sprite *sprite)
{
    sprite->sState = 0;
    sprite->callback = SpriteCB_UnusedBattleInit_Main;
}

static void SpriteCB_UnusedBattleInit_Main(struct Sprite *sprite)
{
    u16 *arr = (u16 *)gDecompressionBuffer;

    switch (sprite->sState)
    {
    case 0:
        sprite->sState++;
        sprite->data[1] = 0;
        sprite->data[2] = 0x281;
        sprite->data[3] = 0;
        sprite->sDelay = 1;
        // fall through
    case 1:
        sprite->sDelay--;
        if (sprite->sDelay == 0)
        {
            s32 i;
            s32 r2;
            s32 r0;

            sprite->sDelay = 2;
            r2 = sprite->data[1] + sprite->data[3] * 32;
            r0 = sprite->data[2] - sprite->data[3] * 32;
            for (i = 0; i < 29; i += 2)
            {
                arr[r2 + i] = 0x3D;
                arr[r0 + i] = 0x3D;
            }
            sprite->data[3]++;
            if (sprite->data[3] == 21)
            {
                sprite->sState++;
                sprite->data[1] = 32;
            }
        }
        break;
    case 2:
        sprite->data[1]--;
        if (sprite->data[1] == 20)
            SetMainCallback2(CB2_InitBattle);
        break;
    }
}

static u8 CreateNPCTrainerParty(struct Pokemon *party, u16 trainerNum, bool8 firstTrainer)
{
    u32 nameHash = 0;
    u32 personalityValue;
    u8 fixedIV;
    s32 i, j;
    u8 monsCount;

    if (trainerNum == TRAINER_SECRET_BASE)
        return 0;

    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER && !(gBattleTypeFlags & (BATTLE_TYPE_FRONTIER
                                                                        | BATTLE_TYPE_EREADER_TRAINER
                                                                        | BATTLE_TYPE_TRAINER_HILL)))
    {
        if (firstTrainer == TRUE)
            ZeroEnemyPartyMons();

        if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
        {
            if (gTrainers[trainerNum].partySize > PARTY_SIZE / 2)
                monsCount = PARTY_SIZE / 2;
            else
                monsCount = gTrainers[trainerNum].partySize;
        }
        else
        {
            monsCount = gTrainers[trainerNum].partySize;
        }

        for (i = 0; i < monsCount; i++)
        {

            if (gTrainers[trainerNum].doubleBattle == TRUE)
                personalityValue = 0x80;
            else if (gTrainers[trainerNum].encounterMusic_gender & F_TRAINER_FEMALE)
                personalityValue = 0x78; // Use personality more likely to result in a female Pokémon
            else
                personalityValue = 0x88; // Use personality more likely to result in a male Pokémon

            for (j = 0; gTrainers[trainerNum].trainerName[j] != EOS; j++)
                nameHash += gTrainers[trainerNum].trainerName[j];

            switch (gTrainers[trainerNum].partyFlags)
            {
            case 0:
            {
                const struct TrainerMonNoItemDefaultMoves *partyData = gTrainers[trainerNum].party.NoItemDefaultMoves;

                for (j = 0; gSpeciesNames[partyData[i].species][j] != EOS; j++)
                    nameHash += gSpeciesNames[partyData[i].species][j];

                personalityValue += nameHash << 8;
                fixedIV = partyData[i].iv * MAX_PER_STAT_IVS / 255;
                CreateMon(&party[i], partyData[i].species, partyData[i].lvl, fixedIV, TRUE, personalityValue, OT_ID_RANDOM_NO_SHINY, 0);
                break;
            }
            case F_TRAINER_PARTY_CUSTOM_MOVESET:
            {
                const struct TrainerMonNoItemCustomMoves *partyData = gTrainers[trainerNum].party.NoItemCustomMoves;

                for (j = 0; gSpeciesNames[partyData[i].species][j] != EOS; j++)
                    nameHash += gSpeciesNames[partyData[i].species][j];

                personalityValue += nameHash << 8;
                fixedIV = partyData[i].iv * MAX_PER_STAT_IVS / 255;
                CreateMon(&party[i], partyData[i].species, partyData[i].lvl, fixedIV, TRUE, personalityValue, OT_ID_RANDOM_NO_SHINY, 0);

                for (j = 0; j < MAX_MON_MOVES; j++)
                {
                    SetMonData(&party[i], MON_DATA_MOVE1 + j, &partyData[i].moves[j]);
                    SetMonData(&party[i], MON_DATA_PP1 + j, &gBattleMoves[partyData[i].moves[j]].pp);
                }
                break;
            }
            case F_TRAINER_PARTY_HELD_ITEM:
            {
                const struct TrainerMonItemDefaultMoves *partyData = gTrainers[trainerNum].party.ItemDefaultMoves;

                for (j = 0; gSpeciesNames[partyData[i].species][j] != EOS; j++)
                    nameHash += gSpeciesNames[partyData[i].species][j];

                personalityValue += nameHash << 8;
                fixedIV = partyData[i].iv * MAX_PER_STAT_IVS / 255;
                CreateMon(&party[i], partyData[i].species, partyData[i].lvl, fixedIV, TRUE, personalityValue, OT_ID_RANDOM_NO_SHINY, 0);

                SetMonData(&party[i], MON_DATA_HELD_ITEM, &partyData[i].heldItem);
                break;
            }
            case F_TRAINER_PARTY_CUSTOM_MOVESET | F_TRAINER_PARTY_HELD_ITEM:
            {
                const struct TrainerMonItemCustomMoves *partyData = gTrainers[trainerNum].party.ItemCustomMoves;

                for (j = 0; gSpeciesNames[partyData[i].species][j] != EOS; j++)
                    nameHash += gSpeciesNames[partyData[i].species][j];

                personalityValue += nameHash << 8;
                fixedIV = partyData[i].iv * MAX_PER_STAT_IVS / 255;
                CreateMon(&party[i], partyData[i].species, partyData[i].lvl, fixedIV, TRUE, personalityValue, OT_ID_RANDOM_NO_SHINY, 0);

                SetMonData(&party[i], MON_DATA_HELD_ITEM, &partyData[i].heldItem);

                for (j = 0; j < MAX_MON_MOVES; j++)
                {
                    SetMonData(&party[i], MON_DATA_MOVE1 + j, &partyData[i].moves[j]);
                    SetMonData(&party[i], MON_DATA_PP1 + j, &gBattleMoves[partyData[i].moves[j]].pp);
                }
                break;
            }
            }
        }

        gBattleTypeFlags |= gTrainers[trainerNum].doubleBattle;
    }

    return gTrainers[trainerNum].partySize;
}

static void UNUSED HBlankCB_Battle(void)
{
    if (REG_VCOUNT < DISPLAY_HEIGHT && REG_VCOUNT >= 111)
        SetGpuReg(REG_OFFSET_BG0CNT, BGCNT_SCREENBASE(24) | BGCNT_TXT256x512);
}

void VBlankCB_Battle(void)
{
    // Change gRngSeed every vblank unless the battle could be recorded.
    if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_RECORDED)))
        Random();

    SetGpuReg(REG_OFFSET_BG0HOFS, gBattle_BG0_X);
    SetGpuReg(REG_OFFSET_BG0VOFS, gBattle_BG0_Y);
    SetGpuReg(REG_OFFSET_BG1HOFS, gBattle_BG1_X);
    SetGpuReg(REG_OFFSET_BG1VOFS, gBattle_BG1_Y);
    SetGpuReg(REG_OFFSET_BG2HOFS, gBattle_BG2_X);
    SetGpuReg(REG_OFFSET_BG2VOFS, gBattle_BG2_Y);
    SetGpuReg(REG_OFFSET_BG3HOFS, gBattle_BG3_X);
    SetGpuReg(REG_OFFSET_BG3VOFS, gBattle_BG3_Y);
    SetGpuReg(REG_OFFSET_WIN0H, gBattle_WIN0H);
    SetGpuReg(REG_OFFSET_WIN0V, gBattle_WIN0V);
    SetGpuReg(REG_OFFSET_WIN1H, gBattle_WIN1H);
    SetGpuReg(REG_OFFSET_WIN1V, gBattle_WIN1V);
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    ScanlineEffect_InitHBlankDmaTransfer();
}

void SpriteCB_VsLetterDummy(struct Sprite *sprite)
{

}

static void SpriteCB_VsLetter(struct Sprite *sprite)
{
    if (sprite->data[0] != 0)
        sprite->x = sprite->data[1] + ((sprite->data[2] & 0xFF00) >> 8);
    else
        sprite->x = sprite->data[1] - ((sprite->data[2] & 0xFF00) >> 8);

    sprite->data[2] += 0x180;

    if (sprite->affineAnimEnded)
    {
        FreeSpriteTilesByTag(ANIM_SPRITES_START);
        FreeSpritePaletteByTag(ANIM_SPRITES_START);
        FreeSpriteOamMatrix(sprite);
        DestroySprite(sprite);
    }
}

void SpriteCB_VsLetterInit(struct Sprite *sprite)
{
    StartSpriteAffineAnim(sprite, 1);
    sprite->callback = SpriteCB_VsLetter;
    PlaySE(SE_MUGSHOT);
}

static void BufferPartyVsScreenHealth_AtEnd(u8 taskId)
{
    struct Pokemon *party1 = NULL;
    struct Pokemon *party2 = NULL;
    u8 multiplayerId = gBattleScripting.multiplayerId;
    u32 flags;
    s32 i;

    if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
    {
        switch (gLinkPlayers[multiplayerId].id)
        {
        case 0:
        case 2:
            party1 = gPlayerParty;
            party2 = gEnemyParty;
            break;
        case 1:
        case 3:
            party1 = gEnemyParty;
            party2 = gPlayerParty;
            break;
        }
    }
    else
    {
        party1 = gPlayerParty;
        party2 = gEnemyParty;
    }

    flags = 0;
    BUFFER_PARTY_VS_SCREEN_STATUS(party1, flags, i);
    gTasks[taskId].data[3] = flags;

    flags = 0;
    BUFFER_PARTY_VS_SCREEN_STATUS(party2, flags, i);
    gTasks[taskId].data[4] = flags;
}

void CB2_InitEndLinkBattle(void)
{
    s32 i;
    u8 taskId;

    SetHBlankCallback(NULL);
    SetVBlankCallback(NULL);
    gBattleTypeFlags &= ~BATTLE_TYPE_LINK_IN_BATTLE;

    if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
    {
        SetMainCallback2(gMain.savedCallback);
        FreeBattleResources();
        FreeBattleSpritesData();
        FreeMonSpritesGfx();
    }
    else
    {
        CpuFill32(0, (void *)(VRAM), VRAM_SIZE);
        SetGpuReg(REG_OFFSET_MOSAIC, 0);
        SetGpuReg(REG_OFFSET_WIN0H, DISPLAY_WIDTH);
        SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(DISPLAY_HEIGHT / 2, DISPLAY_HEIGHT / 2 + 1));
        SetGpuReg(REG_OFFSET_WININ, 0);
        SetGpuReg(REG_OFFSET_WINOUT, 0);
        gBattle_WIN0H = DISPLAY_WIDTH;
        gBattle_WIN0V = WIN_RANGE(DISPLAY_HEIGHT / 2, DISPLAY_HEIGHT / 2 + 1);
        ScanlineEffect_Clear();

        i = 0;
        while (i < 80)
        {
            gScanlineEffectRegBuffers[0][i] = 0xF0;
            gScanlineEffectRegBuffers[1][i] = 0xF0;
            i++;
        }

        while (i < 160)
        {
            gScanlineEffectRegBuffers[0][i] = 0xFF10;
            gScanlineEffectRegBuffers[1][i] = 0xFF10;
            i++;
        }

        ResetPaletteFade();

        gBattle_BG0_X = 0;
        gBattle_BG0_Y = 0;
        gBattle_BG1_X = 0;
        gBattle_BG1_Y = 0;
        gBattle_BG2_X = 0;
        gBattle_BG2_Y = 0;
        gBattle_BG3_X = 0;
        gBattle_BG3_Y = 0;

        InitBattleBgsVideo();
        LoadCompressedPalette(gBattleTextboxPalette, BG_PLTT_ID(0), 2 * PLTT_SIZE_4BPP);
        LoadBattleMenuWindowGfx();
        ResetSpriteData();
        ResetTasks();
        DrawBattleEntryBackground();
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_BG2 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
        FreeAllSpritePalettes();
        gReservedSpritePaletteCount = MAX_BATTLERS_COUNT;
        SetVBlankCallback(VBlankCB_Battle);

        // Show end Vs screen with battle results
        taskId = CreateTask(InitLinkBattleVsScreen, 0);
        gTasks[taskId].data[1] = 0x10E;
        gTasks[taskId].data[2] = 0x5A;
        gTasks[taskId].data[5] = 1;
        BufferPartyVsScreenHealth_AtEnd(taskId);

        SetMainCallback2(CB2_EndLinkBattle);
        gBattleCommunication[MULTIUSE_STATE] = 0;
    }
}

static void CB2_EndLinkBattle(void)
{
    EndLinkBattleInSteps();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
    RunTasks();
}

static void EndLinkBattleInSteps(void)
{
    s32 i;

    switch (gBattleCommunication[MULTIUSE_STATE])
    {
    case 0:
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        gBattleCommunication[1] = 0xFF;
        gBattleCommunication[MULTIUSE_STATE]++;
        break;
    case 1:
        if (--gBattleCommunication[1] == 0)
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 2:
        if (!gPaletteFade.active)
        {
            u8 battlerCount;

            gMain.anyLinkBattlerHasFrontierPass = RecordedBattle_GetFrontierPassFlag();

            if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
                battlerCount = 4;
            else
                battlerCount = 2;

            for (i = 0; i < battlerCount && (gLinkPlayers[i].version & 0xFF) == VERSION_EMERALD; i++);

            if (!gSaveBlock2Ptr->frontier.disableRecordBattle && i == battlerCount)
            {
                if (FlagGet(FLAG_SYS_FRONTIER_PASS))
                {
                    // Ask player if they want to record the battle
                    FreeAllWindowBuffers();
                    SetMainCallback2(CB2_InitAskRecordBattle);
                }
                else if (!gMain.anyLinkBattlerHasFrontierPass)
                {
                    // No players can record this battle, end
                    SetMainCallback2(gMain.savedCallback);
                    FreeBattleResources();
                    FreeBattleSpritesData();
                    FreeMonSpritesGfx();
                }
                else if (gReceivedRemoteLinkPlayers == 0)
                {
                    // Player can't record battle but
                    // another player can, reconnect with them
                    CreateTask(Task_ReconnectWithLinkPlayers, 5);
                    gBattleCommunication[MULTIUSE_STATE]++;
                }
                else
                {
                    gBattleCommunication[MULTIUSE_STATE]++;
                }
            }
            else
            {
                SetMainCallback2(gMain.savedCallback);
                FreeBattleResources();
                FreeBattleSpritesData();
                FreeMonSpritesGfx();
            }
        }
        break;
    case 3:
        CpuFill32(0, (void *)VRAM, VRAM_SIZE);

        for (i = 0; i < 2; i++)
            LoadChosenBattleElement(i);

        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        gBattleCommunication[MULTIUSE_STATE]++;
        break;
    case 4:
        if (!gPaletteFade.active)
            gBattleCommunication[MULTIUSE_STATE]++;
        break;
    case 5:
        if (!FuncIsActiveTask(Task_ReconnectWithLinkPlayers))
            gBattleCommunication[MULTIUSE_STATE]++;
        break;
    case 6:
        if (IsLinkTaskFinished() == TRUE)
        {
            SetLinkStandbyCallback();
            BattlePutTextOnWindow(gText_LinkStandby3, B_WIN_MSG);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 7:
        if (!IsTextPrinterActive(B_WIN_MSG))
        {
            if (IsLinkTaskFinished() == TRUE)
                gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 8:
        if (!gWirelessCommType)
            SetCloseLinkCallback();
        gBattleCommunication[MULTIUSE_STATE]++;
        break;
    case 9:
        if (!gMain.anyLinkBattlerHasFrontierPass || gWirelessCommType || gReceivedRemoteLinkPlayers != 1)
        {
            gMain.anyLinkBattlerHasFrontierPass = FALSE;
            SetMainCallback2(gMain.savedCallback);
            FreeBattleResources();
            FreeBattleSpritesData();
            FreeMonSpritesGfx();
        }
        break;
    }
}

u32 GetBattleBgTemplateData(u8 arrayId, u8 caseId)
{
    u32 ret = 0;

    switch (caseId)
    {
    case 0:
        ret = gBattleBgTemplates[arrayId].bg;
        break;
    case 1:
        ret = gBattleBgTemplates[arrayId].charBaseIndex;
        break;
    case 2:
        ret = gBattleBgTemplates[arrayId].mapBaseIndex;
        break;
    case 3:
        ret = gBattleBgTemplates[arrayId].screenSize;
        break;
    case 4:
        ret = gBattleBgTemplates[arrayId].paletteMode;
        break;
    case 5: // Only this case is used
        ret = gBattleBgTemplates[arrayId].priority;
        break;
    case 6:
        ret = gBattleBgTemplates[arrayId].baseTile;
        break;
    }

    return ret;
}

static void CB2_InitAskRecordBattle(void)
{
    s32 i;

    SetHBlankCallback(NULL);
    SetVBlankCallback(NULL);
    CpuFill32(0, (void *)(VRAM), VRAM_SIZE);
    ResetPaletteFade();
    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    gBattle_BG1_X = 0;
    gBattle_BG1_Y = 0;
    gBattle_BG2_X = 0;
    gBattle_BG2_Y = 0;
    gBattle_BG3_X = 0;
    gBattle_BG3_Y = 0;
    InitBattleBgsVideo();
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    LoadBattleMenuWindowGfx();

    for (i = 0; i < 2; i++)
        LoadChosenBattleElement(i);

    ResetSpriteData();
    ResetTasks();
    FreeAllSpritePalettes();
    gReservedSpritePaletteCount = MAX_BATTLERS_COUNT;
    SetVBlankCallback(VBlankCB_Battle);
    SetMainCallback2(CB2_AskRecordBattle);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
    gBattleCommunication[MULTIUSE_STATE] = 0;
}

static void CB2_AskRecordBattle(void)
{
    AskRecordBattle();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
    RunTasks();
}


// States for AskRecordBattle
#define STATE_INIT             0
#define STATE_LINK             1
#define STATE_WAIT_LINK        2
#define STATE_ASK_RECORD       3
#define STATE_PRINT_YES_NO     4
#define STATE_HANDLE_YES_NO    5
#define STATE_RECORD_NO        6
#define STATE_END_RECORD_NO    7
#define STATE_WAIT_END         8
#define STATE_END              9
#define STATE_RECORD_YES      10
#define STATE_RECORD_WAIT     11
#define STATE_END_RECORD_YES  12

static void AskRecordBattle(void)
{
    switch (gBattleCommunication[MULTIUSE_STATE])
    {
    case STATE_INIT:
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        gBattleCommunication[MULTIUSE_STATE]++;
        break;
    case STATE_LINK:
        if (gMain.anyLinkBattlerHasFrontierPass && gReceivedRemoteLinkPlayers == 0)
            CreateTask(Task_ReconnectWithLinkPlayers, 5);
        gBattleCommunication[MULTIUSE_STATE]++;
        break;
    case STATE_WAIT_LINK:
        if (!FuncIsActiveTask(Task_ReconnectWithLinkPlayers))
            gBattleCommunication[MULTIUSE_STATE]++;
        break;
    case STATE_ASK_RECORD:
        if (!gPaletteFade.active)
        {
            // "Would you like to record your battle on your FRONTIER PASS?"
            BattlePutTextOnWindow(gText_RecordBattleToPass, B_WIN_MSG);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case STATE_PRINT_YES_NO:
        if (!IsTextPrinterActive(B_WIN_MSG))
        {
            HandleBattleWindow(YESNOBOX_X_Y, 0);
            BattlePutTextOnWindow(gText_BattleYesNoChoice, B_WIN_YESNO);
            gBattleCommunication[CURSOR_POSITION] = 1;
            BattleCreateYesNoCursorAt(1);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case STATE_HANDLE_YES_NO:
        if (JOY_NEW(DPAD_UP))
        {
            if (gBattleCommunication[CURSOR_POSITION] != 0)
            {
                // Moved cursor onto Yes
                PlaySE(SE_SELECT);
                BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
                gBattleCommunication[CURSOR_POSITION] = 0;
                BattleCreateYesNoCursorAt(0);
            }
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            if (gBattleCommunication[CURSOR_POSITION] == 0)
            {
                // Moved cursor onto No
                PlaySE(SE_SELECT);
                BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
                gBattleCommunication[CURSOR_POSITION] = 1;
                BattleCreateYesNoCursorAt(1);
            }
        }
        else if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            if (gBattleCommunication[CURSOR_POSITION] == 0)
            {
                // Selected Yes
                HandleBattleWindow(YESNOBOX_X_Y, WINDOW_CLEAR);
                gBattleCommunication[1] = MoveRecordedBattleToSaveData();
                gBattleCommunication[MULTIUSE_STATE] = STATE_RECORD_YES;
            }
            else
            {
                // Selected No
                gBattleCommunication[MULTIUSE_STATE]++;
            }
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case STATE_RECORD_NO:
        if (IsLinkTaskFinished() == TRUE)
        {
            HandleBattleWindow(YESNOBOX_X_Y, WINDOW_CLEAR);
            if (gMain.anyLinkBattlerHasFrontierPass)
            {
                // Other battlers may be recording, wait for them
                SetLinkStandbyCallback();
                BattlePutTextOnWindow(gText_LinkStandby3, B_WIN_MSG);
            }
            gBattleCommunication[MULTIUSE_STATE]++; // STATE_END_RECORD_NO
        }
        break;
    case STATE_WAIT_END:
        if (--gBattleCommunication[1] == 0)
        {
            if (gMain.anyLinkBattlerHasFrontierPass && !gWirelessCommType)
                SetCloseLinkCallback();
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case STATE_END:
        if (!gMain.anyLinkBattlerHasFrontierPass || gWirelessCommType || gReceivedRemoteLinkPlayers != 1)
        {
            gMain.anyLinkBattlerHasFrontierPass = FALSE;
            if (!gPaletteFade.active)
            {
                SetMainCallback2(gMain.savedCallback);
                FreeBattleResources();
                FreeBattleSpritesData();
                FreeMonSpritesGfx();
            }
        }
        break;
    case STATE_RECORD_YES:
        if (gBattleCommunication[1] == 1)
        {
            PlaySE(SE_SAVE);
            BattleStringExpandPlaceholdersToDisplayedString(gText_BattleRecordedOnPass);
            BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
            gBattleCommunication[1] = 128; // Delay
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        else
        {
            BattleStringExpandPlaceholdersToDisplayedString(BattleFrontier_BattleTowerBattleRoom_Text_RecordCouldntBeSaved);
            BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
            gBattleCommunication[1] = 128; // Delay
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case STATE_RECORD_WAIT:
        if (IsLinkTaskFinished() == TRUE && !IsTextPrinterActive(B_WIN_MSG) && --gBattleCommunication[1] == 0)
        {
            if (gMain.anyLinkBattlerHasFrontierPass)
            {
                SetLinkStandbyCallback();
                BattlePutTextOnWindow(gText_LinkStandby3, B_WIN_MSG);
            }
            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case STATE_END_RECORD_YES:
    case STATE_END_RECORD_NO:
        if (!IsTextPrinterActive(B_WIN_MSG))
        {
            if (gMain.anyLinkBattlerHasFrontierPass)
            {
                if (IsLinkTaskFinished() == TRUE)
                {
                    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
                    gBattleCommunication[1] = 32; // Delay
                    gBattleCommunication[MULTIUSE_STATE] = STATE_WAIT_END;
                }

            }
            else
            {
                BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
                gBattleCommunication[1] = 32; // Delay
                gBattleCommunication[MULTIUSE_STATE] = STATE_WAIT_END;
            }
        }
        break;
    }
}

static void TryCorrectShedinjaLanguage(struct Pokemon *mon)
{
    u8 nickname[POKEMON_NAME_LENGTH + 1];
    u8 language = LANGUAGE_JAPANESE;

    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_SHEDINJA
     && GetMonData(mon, MON_DATA_LANGUAGE) != language)
    {
        GetMonData(mon, MON_DATA_NICKNAME, nickname);
        if (StringCompareWithoutExtCtrlCodes(nickname, sText_ShedinjaJpnName) == 0)
            SetMonData(mon, MON_DATA_LANGUAGE, &language);
    }
}

u32 GetBattleWindowTemplatePixelWidth(u32 windowsType, u32 tableId)
{
    return gBattleWindowTemplates[windowsType][tableId].width * 8;
}

#define sBattler            data[0]
#define sSpeciesId          data[2]

void SpriteCB_WildMon(struct Sprite *sprite)
{
    sprite->callback = SpriteCB_MoveWildMonToRight;
    StartSpriteAnimIfDifferent(sprite, 0);
    BeginNormalPaletteFade(0x20000, 0, 10, 10, RGB(8, 8, 8));
}

static void SpriteCB_MoveWildMonToRight(struct Sprite *sprite)
{
    if ((gIntroSlideFlags & 1) == 0)
    {
        sprite->x2 += 2;
        if (sprite->x2 == 0)
        {
            sprite->callback = SpriteCB_WildMonShowHealthbox;
        }
    }
}

static void SpriteCB_WildMonShowHealthbox(struct Sprite *sprite)
{
    if (sprite->animEnded)
    {
        StartHealthboxSlideIn(sprite->sBattler);
        SetHealthboxSpriteVisible(gHealthboxSpriteIds[sprite->sBattler]);
        sprite->callback = SpriteCB_WildMonAnimate;
        StartSpriteAnimIfDifferent(sprite, 0);
        BeginNormalPaletteFade(0x20000, 0, 10, 0, RGB(8, 8, 8));
    }
}

static void SpriteCB_WildMonAnimate(struct Sprite *sprite)
{
    if (!gPaletteFade.active)
    {
        BattleAnimateFrontSprite(sprite, sprite->sSpeciesId, FALSE, 1);
    }
}

void SpriteCallbackDummy_2(struct Sprite *sprite)
{

}

#define sNumFlickers data[3]
#define sDelay       data[4]

static void UNUSED SpriteCB_InitFlicker(struct Sprite *sprite)
{
    sprite->sNumFlickers = 6;
    sprite->sDelay = 1;
    sprite->callback = SpriteCB_Flicker;
}

static void SpriteCB_Flicker(struct Sprite *sprite)
{
    sprite->sDelay--;
    if (sprite->sDelay == 0)
    {
        sprite->sDelay = 8;
        sprite->invisible ^= 1;
        sprite->sNumFlickers--;
        if (sprite->sNumFlickers == 0)
        {
            sprite->invisible = FALSE;
            sprite->callback = SpriteCallbackDummy_2;
            sFlickerArray[0] = 0;
        }
    }
}

#undef sNumFlickers
#undef sDelay

extern const struct MonCoords gMonFrontPicCoords[];
extern const struct MonCoords gCastformFrontSpriteCoords[];

void SpriteCB_FaintOpponentMon(struct Sprite *sprite)
{
    u8 battler = sprite->sBattler;
    u16 species;
    u8 yOffset;

    if (gBattleSpritesDataPtr->battlerData[battler].transformSpecies != 0)
        species = gBattleSpritesDataPtr->battlerData[battler].transformSpecies;
    else
        species = sprite->sSpeciesId;

    GetMonData(&gEnemyParty[gBattlerPartyIndexes[battler]], MON_DATA_PERSONALITY);  // Unused return value.

    if (species == SPECIES_UNOWN)
    {
        u32 personalityValue = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battler]], MON_DATA_PERSONALITY);
        u16 unownForm = GET_UNOWN_LETTER(personalityValue);
        u16 unownSpecies;

        if (unownForm == 0)
            unownSpecies = SPECIES_UNOWN;  // Use the A Unown form.
        else
            unownSpecies = NUM_SPECIES + unownForm;  // Use one of the other Unown letters.

        yOffset = gMonFrontPicCoords[unownSpecies].y_offset;
    }
    else if (species == SPECIES_CASTFORM)
    {
        yOffset = gCastformFrontSpriteCoords[gBattleMonForms[battler]].y_offset;
    }
    else if (species > NUM_SPECIES)
    {
        yOffset = gMonFrontPicCoords[SPECIES_NONE].y_offset;
    }
    else
    {
        yOffset = gMonFrontPicCoords[species].y_offset;
    }

    sprite->data[3] = 8 - yOffset / 8;
    sprite->data[4] = 1;
    sprite->callback = SpriteCB_AnimFaintOpponent;
}

static void SpriteCB_AnimFaintOpponent(struct Sprite *sprite)
{
    s32 i;

    if (--sprite->data[4] == 0)
    {
        sprite->data[4] = 2;
        sprite->y2 += 8; // Move the sprite down.
        if (--sprite->data[3] < 0)
        {
            FreeSpriteOamMatrix(sprite);
            DestroySprite(sprite);
        }
        else // Erase bottom part of the sprite to create a smooth illusion of mon falling down.
        {
            u8 *dst = gMonSpritesGfxPtr->sprites.byte[GetBattlerPosition(sprite->sBattler)] + (gBattleMonForms[sprite->sBattler] << 11) + (sprite->data[3] << 8);

            for (i = 0; i < 0x100; i++)
                *(dst++) = 0;

            StartSpriteAnim(sprite, gBattleMonForms[sprite->sBattler]);
        }
    }
}

// Used when selecting a move, which can hit multiple targets, in double battles.
void SpriteCB_ShowAsMoveTarget(struct Sprite *sprite)
{
    sprite->data[3] = 8;
    sprite->data[4] = sprite->invisible;
    sprite->callback = SpriteCB_BlinkVisible;
}

static void SpriteCB_BlinkVisible(struct Sprite *sprite)
{
    if (--sprite->data[3] == 0)
    {
        sprite->invisible ^= 1;
        sprite->data[3] = 8;
    }
}

void SpriteCB_HideAsMoveTarget(struct Sprite *sprite)
{
    sprite->invisible = sprite->data[4];
    sprite->data[4] = FALSE;
    sprite->callback = SpriteCallbackDummy_2;
}

void SpriteCB_OpponentMonFromBall(struct Sprite *sprite)
{
    if (sprite->affineAnimEnded)
    {
        if (!(gHitMarker & HITMARKER_NO_ANIMATIONS) || gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
        {
            if (HasTwoFramesAnimation(sprite->sSpeciesId))
                StartSpriteAnim(sprite, 1);
        }
        BattleAnimateFrontSprite(sprite, sprite->sSpeciesId, TRUE, 1);
    }
}

// This callback is frequently overwritten by SpriteCB_TrainerSlideIn
void SpriteCB_BattleSpriteStartSlideLeft(struct Sprite *sprite)
{
    sprite->callback = SpriteCB_BattleSpriteSlideLeft;
}

static void SpriteCB_BattleSpriteSlideLeft(struct Sprite *sprite)
{
    if (!(gIntroSlideFlags & 1))
    {
        sprite->x2 -= 2;
        if (sprite->x2 == 0)
        {
            sprite->callback = SpriteCB_Idle;
            sprite->data[1] = 0;
        }
    }
}

static void UNUSED SetIdleSpriteCallback(struct Sprite *sprite)
{
    sprite->callback = SpriteCB_Idle;
}

static void SpriteCB_Idle(struct Sprite *sprite)
{
}

#define sSpeedX data[1]
#define sSpeedY data[2]

void SpriteCB_FaintSlideAnim(struct Sprite *sprite)
{
    if (!(gIntroSlideFlags & 1))
    {
        sprite->x2 += sprite->sSpeedX;
        sprite->y2 += sprite->sSpeedY;
    }
}

#undef sSpeedX
#undef sSpeedY

#define sSinIndex           data[0]
#define sDelta              data[1]
#define sAmplitude          data[2]
#define sBouncerSpriteId    data[3]
#define sWhich              data[4]

void DoBounceEffect(u8 battler, u8 which, s8 delta, s8 amplitude)
{
    u8 invisibleSpriteId;
    u8 bouncerSpriteId;

    switch (which)
    {
    case BOUNCE_HEALTHBOX:
    default:
        if (gBattleSpritesDataPtr->healthBoxesData[battler].healthboxIsBouncing)
            return;
        break;
    case BOUNCE_MON:
        if (gBattleSpritesDataPtr->healthBoxesData[battler].battlerIsBouncing)
            return;
        break;
    }

    invisibleSpriteId = CreateInvisibleSpriteWithCallback(SpriteCB_BounceEffect);
    if (which == BOUNCE_HEALTHBOX)
    {
        bouncerSpriteId = gHealthboxSpriteIds[battler];
        gBattleSpritesDataPtr->healthBoxesData[battler].healthboxBounceSpriteId = invisibleSpriteId;
        gBattleSpritesDataPtr->healthBoxesData[battler].healthboxIsBouncing = 1;
        gSprites[invisibleSpriteId].sSinIndex = 128; // 0
    }
    else
    {
        bouncerSpriteId = gBattlerSpriteIds[battler];
        gBattleSpritesDataPtr->healthBoxesData[battler].battlerBounceSpriteId = invisibleSpriteId;
        gBattleSpritesDataPtr->healthBoxesData[battler].battlerIsBouncing = 1;
        gSprites[invisibleSpriteId].sSinIndex = 192; // -1
    }
    gSprites[invisibleSpriteId].sDelta = delta;
    gSprites[invisibleSpriteId].sAmplitude = amplitude;
    gSprites[invisibleSpriteId].sBouncerSpriteId = bouncerSpriteId;
    gSprites[invisibleSpriteId].sWhich = which;
    gSprites[bouncerSpriteId].x2 = 0;
    gSprites[bouncerSpriteId].y2 = 0;
}

void EndBounceEffect(u8 battler, u8 which)
{
    u8 bouncerSpriteId;

    if (which == BOUNCE_HEALTHBOX)
    {
        if (!gBattleSpritesDataPtr->healthBoxesData[battler].healthboxIsBouncing)
            return;

        bouncerSpriteId = gSprites[gBattleSpritesDataPtr->healthBoxesData[battler].healthboxBounceSpriteId].sBouncerSpriteId;
        DestroySprite(&gSprites[gBattleSpritesDataPtr->healthBoxesData[battler].healthboxBounceSpriteId]);
        gBattleSpritesDataPtr->healthBoxesData[battler].healthboxIsBouncing = 0;
    }
    else
    {
        if (!gBattleSpritesDataPtr->healthBoxesData[battler].battlerIsBouncing)
            return;

        bouncerSpriteId = gSprites[gBattleSpritesDataPtr->healthBoxesData[battler].battlerBounceSpriteId].sBouncerSpriteId;
        DestroySprite(&gSprites[gBattleSpritesDataPtr->healthBoxesData[battler].battlerBounceSpriteId]);
        gBattleSpritesDataPtr->healthBoxesData[battler].battlerIsBouncing = 0;
    }

    gSprites[bouncerSpriteId].x2 = 0;
    gSprites[bouncerSpriteId].y2 = 0;
}

static void SpriteCB_BounceEffect(struct Sprite *sprite)
{
    u8 bouncerSpriteId = sprite->sBouncerSpriteId;
    s32 index;

    if (sprite->sWhich == BOUNCE_HEALTHBOX)
        index = sprite->sSinIndex;
    else
        index = sprite->sSinIndex;

    gSprites[bouncerSpriteId].y2 = Sin(index, sprite->sAmplitude) + sprite->sAmplitude;
    sprite->sSinIndex = (sprite->sSinIndex + sprite->sDelta) & 0xFF;
}

#undef sSinIndex
#undef sDelta
#undef sAmplitude
#undef sBouncerSpriteId
#undef sWhich

void SpriteCB_PlayerMonFromBall(struct Sprite *sprite)
{
    if (sprite->affineAnimEnded)
        BattleAnimateBackSprite(sprite, sprite->sSpeciesId);
}

static void SpriteCB_TrainerThrowObject_Main(struct Sprite *sprite)
{
    AnimSetCenterToCornerVecX(sprite);
    if (sprite->animEnded)
        sprite->callback = SpriteCB_Idle;
}

// Sprite callback for a trainer back pic to throw an object
// (Wally throwing a ball, throwing Pokéblocks/balls in the Safari Zone)
void SpriteCB_TrainerThrowObject(struct Sprite *sprite)
{
    StartSpriteAnim(sprite, 1);
    sprite->callback = SpriteCB_TrainerThrowObject_Main;
}

void AnimSetCenterToCornerVecX(struct Sprite *sprite)
{
    if (sprite->animDelayCounter == 0)
        sprite->centerToCornerVecX = sCenterToCornerVecXs[sprite->animCmdIndex];
}

void BeginBattleIntroDummy(void)
{

}

void BeginBattleIntro(void)
{
    BattleStartClearSetData();
    gBattleCommunication[1] = 0;
    gBattleMainFunc = BattleIntroGetMonsData;
}

static void BattleMainCB1(void)
{
    gBattleMainFunc();

    for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
        gBattlerControllerFuncs[gActiveBattler]();
}

static void BattleStartClearSetData(void)
{
    s32 i;
    u32 j;
    u8 *dataPtr;

    TurnValuesCleanUp(FALSE);
    SpecialStatusesClear();

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        gStatuses3[i] = 0;

        dataPtr = (u8 *)&gDisableStructs[i];
        for (j = 0; j < sizeof(struct DisableStruct); j++)
            dataPtr[j] = 0;

        gDisableStructs[i].isFirstTurn = 2;
        sUnusedBattlersArray[i] = 0;
        gLastMoves[i] = MOVE_NONE;
        gLastLandedMoves[i] = MOVE_NONE;
        gLastHitByType[i] = 0;
        gLastResultingMoves[i] = MOVE_NONE;
        gLastHitBy[i] = 0xFF;
        gLockedMoves[i] = MOVE_NONE;
        gLastPrintedMoves[i] = MOVE_NONE;
        gBattleResources->flags->flags[i] = 0;
        gPalaceSelectionBattleScripts[i] = 0;
    }

    for (i = 0; i < 2; i++)
    {
        gSideStatuses[i] = 0;

        dataPtr = (u8 *)&gSideTimers[i];
        for (j = 0; j < sizeof(struct SideTimer); j++)
            dataPtr[j] = 0;
    }

    gBattlerAttacker = 0;
    gBattlerTarget = 0;
    gBattleWeather = 0;

    dataPtr = (u8 *)&gWishFutureKnock;
    for (i = 0; i < sizeof(struct WishFutureKnock); i++)
        dataPtr[i] = 0;

    gHitMarker = 0;

    if (!(gBattleTypeFlags & BATTLE_TYPE_RECORDED))
    {
        if (!(gBattleTypeFlags & BATTLE_TYPE_LINK) && gSaveBlock2Ptr->optionsBattleSceneOff == TRUE)
            gHitMarker |= HITMARKER_NO_ANIMATIONS;
    }
    else if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)) && GetBattleSceneInRecordedBattle())
    {
        gHitMarker |= HITMARKER_NO_ANIMATIONS;
    }

    gBattleScripting.battleStyle = gSaveBlock2Ptr->optionsBattleStyle;

    gMultiHitCounter = 0;
    gBattleOutcome = 0;
    gBattleControllerExecFlags = 0;
    gPaydayMoney = 0;
    gBattleResources->battleScriptsStack->size = 0;
    gBattleResources->battleCallbackStack->size = 0;

    for (i = 0; i < BATTLE_COMMUNICATION_ENTRIES_COUNT; i++)
        gBattleCommunication[i] = 0;

    gPauseCounterBattle = 0;
    gBattleMoveDamage = 0;
    gIntroSlideFlags = 0;
    gBattleScripting.animTurn = 0;
    gBattleScripting.animTargetsHit = 0;
    gLeveledUpInBattle = 0;
    gAbsentBattlerFlags = 0;
    gBattleStruct->runTries = 0;
    gBattleStruct->safariGoNearCounter = 0;
    gBattleStruct->safariPkblThrowCounter = 0;
    *(&gBattleStruct->safariCatchFactor) = gSpeciesInfo[GetMonData(&gEnemyParty[0], MON_DATA_SPECIES)].catchRate * 100 / 1275;
    gBattleStruct->safariEscapeFactor = 3;
    gBattleStruct->wildVictorySong = 0;
    gBattleStruct->moneyMultiplier = 1;

    for (i = 0; i < 8; i++)
    {
        *((u8 *)gBattleStruct->lastTakenMove + i) = MOVE_NONE;
        *((u8 *)gBattleStruct->usedHeldItems + i) = ITEM_NONE;
        *((u8 *)gBattleStruct->choicedMove + i) = MOVE_NONE;
        *((u8 *)gBattleStruct->changedItems + i) = ITEM_NONE;
        *(i + 0 * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
        *(i + 1 * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
        *(i + 2 * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
        *(i + 3 * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
    }

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        *(gBattleStruct->AI_monToSwitchIntoId + i) = PARTY_SIZE;
    }

    gBattleStruct->givenExpMons = 0;
    gBattleStruct->palaceFlags = 0;

    gRandomTurnNumber = Random();

    dataPtr = (u8 *)(&gBattleResults);
    for (i = 0; i < sizeof(struct BattleResults); i++)
        dataPtr[i] = 0;

    gBattleResults.shinyWildMon = IsMonShiny(&gEnemyParty[0]);

    gBattleStruct->arenaLostPlayerMons = 0;
    gBattleStruct->arenaLostOpponentMons = 0;
}

void SwitchInClearSetData(void)
{
    struct DisableStruct disableStructCopy = gDisableStructs[gActiveBattler];
    s32 i;
    u8 *ptr;

    if (gBattleMoves[gCurrentMove].effect != EFFECT_BATON_PASS)
    {
        for (i = 0; i < NUM_BATTLE_STATS; i++)
            gBattleMons[gActiveBattler].statStages[i] = DEFAULT_STAT_STAGE;
        for (i = 0; i < gBattlersCount; i++)
        {
            if ((gBattleMons[i].status2 & STATUS2_ESCAPE_PREVENTION) && gDisableStructs[i].battlerPreventingEscape == gActiveBattler)
                gBattleMons[i].status2 &= ~STATUS2_ESCAPE_PREVENTION;
            if ((gStatuses3[i] & STATUS3_ALWAYS_HITS) && gDisableStructs[i].battlerWithSureHit == gActiveBattler)
            {
                gStatuses3[i] &= ~STATUS3_ALWAYS_HITS;
                gDisableStructs[i].battlerWithSureHit = 0;
            }
        }
    }
    if (gBattleMoves[gCurrentMove].effect == EFFECT_BATON_PASS)
    {
        gBattleMons[gActiveBattler].status2 &= (STATUS2_CONFUSION | STATUS2_FOCUS_ENERGY | STATUS2_SUBSTITUTE | STATUS2_ESCAPE_PREVENTION | STATUS2_CURSED);
        gStatuses3[gActiveBattler] &= (STATUS3_LEECHSEED_BATTLER | STATUS3_LEECHSEED | STATUS3_ALWAYS_HITS | STATUS3_PERISH_SONG | STATUS3_ROOTED | STATUS3_MUDSPORT | STATUS3_WATERSPORT);

        for (i = 0; i < gBattlersCount; i++)
        {
            if (GetBattlerSide(gActiveBattler) != GetBattlerSide(i)
             && (gStatuses3[i] & STATUS3_ALWAYS_HITS) != 0
             && (gDisableStructs[i].battlerWithSureHit == gActiveBattler))
            {
                gStatuses3[i] &= ~STATUS3_ALWAYS_HITS;
                gStatuses3[i] |= STATUS3_ALWAYS_HITS_TURN(2);
            }
        }
    }
    else
    {
        gBattleMons[gActiveBattler].status2 = 0;
        gStatuses3[gActiveBattler] = 0;
    }

    for (i = 0; i < gBattlersCount; i++)
    {
        if (gBattleMons[i].status2 & STATUS2_INFATUATED_WITH(gActiveBattler))
            gBattleMons[i].status2 &= ~STATUS2_INFATUATED_WITH(gActiveBattler);
        if ((gBattleMons[i].status2 & STATUS2_WRAPPED) && *(gBattleStruct->wrappedBy + i) == gActiveBattler)
            gBattleMons[i].status2 &= ~STATUS2_WRAPPED;
    }

    gActionSelectionCursor[gActiveBattler] = 0;
    gMoveSelectionCursor[gActiveBattler] = 0;

    ptr = (u8 *)&gDisableStructs[gActiveBattler];
    for (i = 0; i < sizeof(struct DisableStruct); i++)
        ptr[i] = 0;

    if (gBattleMoves[gCurrentMove].effect == EFFECT_BATON_PASS)
    {
        gDisableStructs[gActiveBattler].substituteHP = disableStructCopy.substituteHP;
        gDisableStructs[gActiveBattler].battlerWithSureHit = disableStructCopy.battlerWithSureHit;
        gDisableStructs[gActiveBattler].perishSongTimer = disableStructCopy.perishSongTimer;
        gDisableStructs[gActiveBattler].perishSongTimerStartValue = disableStructCopy.perishSongTimerStartValue;
        gDisableStructs[gActiveBattler].battlerPreventingEscape = disableStructCopy.battlerPreventingEscape;
    }

    gMoveResultFlags = 0;
    gDisableStructs[gActiveBattler].isFirstTurn = 2;
    gDisableStructs[gActiveBattler].truantSwitchInHack = disableStructCopy.truantSwitchInHack;
    gLastMoves[gActiveBattler] = MOVE_NONE;
    gLastLandedMoves[gActiveBattler] = MOVE_NONE;
    gLastHitByType[gActiveBattler] = 0;
    gLastResultingMoves[gActiveBattler] = MOVE_NONE;
    gLastPrintedMoves[gActiveBattler] = MOVE_NONE;
    gLastHitBy[gActiveBattler] = 0xFF;

    *(gBattleStruct->lastTakenMove + gActiveBattler * 2 + 0) = MOVE_NONE;
    *(gBattleStruct->lastTakenMove + gActiveBattler * 2 + 1) = MOVE_NONE;
    *(0 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
    *(0 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 1) = 0;
    *(1 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
    *(1 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 1) = 0;
    *(2 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
    *(2 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 1) = 0;
    *(3 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
    *(3 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 1) = 0;

    gBattleStruct->palaceFlags &= ~(gBitTable[gActiveBattler]);

    for (i = 0; i < gBattlersCount; i++)
    {
        if (i != gActiveBattler && GetBattlerSide(i) != GetBattlerSide(gActiveBattler))
        {
            *(gBattleStruct->lastTakenMove + i * 2 + 0) = MOVE_NONE;
            *(gBattleStruct->lastTakenMove + i * 2 + 1) = MOVE_NONE;
        }
        *(i * 8 + gActiveBattler * 2 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
        *(i * 8 + gActiveBattler * 2 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 1) = 0;
    }

    *(u8 *)((u8 *)(&gBattleStruct->choicedMove[gActiveBattler]) + 0) = MOVE_NONE;
    *(u8 *)((u8 *)(&gBattleStruct->choicedMove[gActiveBattler]) + 1) = MOVE_NONE;

    gBattleResources->flags->flags[gActiveBattler] = 0;
    gCurrentMove = MOVE_NONE;
    gBattleStruct->arenaTurnCounter = 0xFF;

    ClearBattlerMoveHistory(gActiveBattler);
    ClearBattlerAbilityHistory(gActiveBattler);
}

void FaintClearSetData(void)
{
    s32 i;
    u8 *ptr;

    for (i = 0; i < NUM_BATTLE_STATS; i++)
        gBattleMons[gActiveBattler].statStages[i] = DEFAULT_STAT_STAGE;

    gBattleMons[gActiveBattler].status2 = 0;
    gStatuses3[gActiveBattler] = 0;

    for (i = 0; i < gBattlersCount; i++)
    {
        if ((gBattleMons[i].status2 & STATUS2_ESCAPE_PREVENTION) && gDisableStructs[i].battlerPreventingEscape == gActiveBattler)
            gBattleMons[i].status2 &= ~STATUS2_ESCAPE_PREVENTION;
        if (gBattleMons[i].status2 & STATUS2_INFATUATED_WITH(gActiveBattler))
            gBattleMons[i].status2 &= ~STATUS2_INFATUATED_WITH(gActiveBattler);
        if ((gBattleMons[i].status2 & STATUS2_WRAPPED) && *(gBattleStruct->wrappedBy + i) == gActiveBattler)
            gBattleMons[i].status2 &= ~STATUS2_WRAPPED;
    }

    gActionSelectionCursor[gActiveBattler] = 0;
    gMoveSelectionCursor[gActiveBattler] = 0;

    ptr = (u8 *)&gDisableStructs[gActiveBattler];
    for (i = 0; i < sizeof(struct DisableStruct); i++)
        ptr[i] = 0;

    gProtectStructs[gActiveBattler].protected = FALSE;
    gProtectStructs[gActiveBattler].endured = FALSE;
    gProtectStructs[gActiveBattler].noValidMoves = FALSE;
    gProtectStructs[gActiveBattler].helpingHand = FALSE;
    gProtectStructs[gActiveBattler].bounceMove = FALSE;
    gProtectStructs[gActiveBattler].stealMove = FALSE;
    gProtectStructs[gActiveBattler].flag0Unknown = FALSE;
    gProtectStructs[gActiveBattler].prlzImmobility = FALSE;
    gProtectStructs[gActiveBattler].confusionSelfDmg = FALSE;
    gProtectStructs[gActiveBattler].targetNotAffected = FALSE;
    gProtectStructs[gActiveBattler].chargingTurn = FALSE;
    gProtectStructs[gActiveBattler].fleeType = 0;
    gProtectStructs[gActiveBattler].usedImprisonedMove = FALSE;
    gProtectStructs[gActiveBattler].loveImmobility = FALSE;
    gProtectStructs[gActiveBattler].usedDisabledMove = FALSE;
    gProtectStructs[gActiveBattler].usedTauntedMove = FALSE;
    gProtectStructs[gActiveBattler].flag2Unknown = FALSE;
    gProtectStructs[gActiveBattler].flinchImmobility = FALSE;
    gProtectStructs[gActiveBattler].notFirstStrike = FALSE;

    gDisableStructs[gActiveBattler].isFirstTurn = 2;

    gLastMoves[gActiveBattler] = MOVE_NONE;
    gLastLandedMoves[gActiveBattler] = MOVE_NONE;
    gLastHitByType[gActiveBattler] = 0;
    gLastResultingMoves[gActiveBattler] = MOVE_NONE;
    gLastPrintedMoves[gActiveBattler] = MOVE_NONE;
    gLastHitBy[gActiveBattler] = 0xFF;

    *(u8 *)((u8 *)(&gBattleStruct->choicedMove[gActiveBattler]) + 0) = MOVE_NONE;
    *(u8 *)((u8 *)(&gBattleStruct->choicedMove[gActiveBattler]) + 1) = MOVE_NONE;

    *(gBattleStruct->lastTakenMove + gActiveBattler * 2 + 0) = MOVE_NONE;
    *(gBattleStruct->lastTakenMove + gActiveBattler * 2 + 1) = MOVE_NONE;
    *(0 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
    *(0 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 1) = 0;
    *(1 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
    *(1 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 1) = 0;
    *(2 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
    *(2 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 1) = 0;
    *(3 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
    *(3 * 2 + gActiveBattler * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 1) = 0;

    gBattleStruct->palaceFlags &= ~(gBitTable[gActiveBattler]);

    for (i = 0; i < gBattlersCount; i++)
    {
        if (i != gActiveBattler && GetBattlerSide(i) != GetBattlerSide(gActiveBattler))
        {
            *(gBattleStruct->lastTakenMove + i * 2 + 0) = MOVE_NONE;
            *(gBattleStruct->lastTakenMove + i * 2 + 1) = MOVE_NONE;
        }
        *(i * 8 + gActiveBattler * 2 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = 0;
        *(i * 8 + gActiveBattler * 2 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 1) = 0;
    }

    gBattleResources->flags->flags[gActiveBattler] = 0;

    gBattleMons[gActiveBattler].types[0] = gSpeciesInfo[gBattleMons[gActiveBattler].species].types[0];
    gBattleMons[gActiveBattler].types[1] = gSpeciesInfo[gBattleMons[gActiveBattler].species].types[1];

    ClearBattlerMoveHistory(gActiveBattler);
    ClearBattlerAbilityHistory(gActiveBattler);
}

static void BattleIntroGetMonsData(void)
{
    switch (gBattleCommunication[MULTIUSE_STATE])
    {
    case 0:
        gActiveBattler = gBattleCommunication[1];
        BtlController_EmitGetMonData(BUFFER_A, REQUEST_ALL_BATTLE, 0);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattleCommunication[MULTIUSE_STATE]++;
        break;
    case 1:
        if (gBattleControllerExecFlags == 0)
        {
            gBattleCommunication[1]++;
            if (gBattleCommunication[1] == gBattlersCount)
                gBattleMainFunc = BattleIntroPrepareBackgroundSlide;
            else
                gBattleCommunication[MULTIUSE_STATE] = 0;
        }
        break;
    }
}

static void BattleIntroPrepareBackgroundSlide(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        gActiveBattler = GetBattlerAtPosition(0);
        BtlController_EmitIntroSlide(BUFFER_A, gBattleEnvironment);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattleMainFunc = BattleIntroDrawTrainersOrMonsSprites;
        gBattleCommunication[MULTIUSE_STATE] = 0;
        gBattleCommunication[SPRITES_INIT_STATE1] = 0;
    }
}

static void BattleIntroDrawTrainersOrMonsSprites(void)
{
    u8 *ptr;
    s32 i;

    if (gBattleControllerExecFlags)
        return;

    for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
    {
        if ((gBattleTypeFlags & BATTLE_TYPE_SAFARI)
            && GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
        {
            ptr = (u8 *)&gBattleMons[gActiveBattler];
            for (i = 0; i < sizeof(struct BattlePokemon); i++)
                ptr[i] = 0;
        }
        else
        {
            u16 *hpOnSwitchout;

            ptr = (u8 *)&gBattleMons[gActiveBattler];
            for (i = 0; i < sizeof(struct BattlePokemon); i++)
                ptr[i] = gBattleBufferB[gActiveBattler][4 + i];

            gBattleMons[gActiveBattler].types[0] = gSpeciesInfo[gBattleMons[gActiveBattler].species].types[0];
            gBattleMons[gActiveBattler].types[1] = gSpeciesInfo[gBattleMons[gActiveBattler].species].types[1];
            gBattleMons[gActiveBattler].ability = GetAbilityBySpecies(gBattleMons[gActiveBattler].species, gBattleMons[gActiveBattler].abilityNum);
            hpOnSwitchout = &gBattleStruct->hpOnSwitchout[GetBattlerSide(gActiveBattler)];
            *hpOnSwitchout = gBattleMons[gActiveBattler].hp;
            for (i = 0; i < NUM_BATTLE_STATS; i++)
                gBattleMons[gActiveBattler].statStages[i] = DEFAULT_STAT_STAGE;
            gBattleMons[gActiveBattler].status2 = 0;
        }

        if (GetBattlerPosition(gActiveBattler) == B_POSITION_PLAYER_LEFT)
        {
            BtlController_EmitDrawTrainerPic(BUFFER_A);
            MarkBattlerForControllerExec(gActiveBattler);
        }

        if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        {
            if (GetBattlerPosition(gActiveBattler) == B_POSITION_OPPONENT_LEFT)
            {
                BtlController_EmitDrawTrainerPic(BUFFER_A);
                MarkBattlerForControllerExec(gActiveBattler);
            }
            if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT
             && !(gBattleTypeFlags & (BATTLE_TYPE_EREADER_TRAINER
                                      | BATTLE_TYPE_FRONTIER
                                      | BATTLE_TYPE_LINK
                                      | BATTLE_TYPE_RECORDED_LINK
                                      | BATTLE_TYPE_TRAINER_HILL)))
            {
                HandleSetPokedexFlag(SpeciesToNationalPokedexNum(gBattleMons[gActiveBattler].species), FLAG_SET_SEEN, gBattleMons[gActiveBattler].personality);
            }
        }
        else
        {
            if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT)
            {
                if (!(gBattleTypeFlags & (BATTLE_TYPE_EREADER_TRAINER
                                      | BATTLE_TYPE_FRONTIER
                                      | BATTLE_TYPE_LINK
                                      | BATTLE_TYPE_RECORDED_LINK
                                      | BATTLE_TYPE_TRAINER_HILL)))
                {
                    HandleSetPokedexFlag(SpeciesToNationalPokedexNum(gBattleMons[gActiveBattler].species), FLAG_SET_SEEN, gBattleMons[gActiveBattler].personality);
                }
                BtlController_EmitLoadMonSprite(BUFFER_A);
                MarkBattlerForControllerExec(gActiveBattler);
                gBattleResults.lastOpponentSpecies = GetMonData(&gEnemyParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_SPECIES, NULL);
            }
        }

        if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
        {
            if (GetBattlerPosition(gActiveBattler) == B_POSITION_PLAYER_RIGHT
             || GetBattlerPosition(gActiveBattler) == B_POSITION_OPPONENT_RIGHT)
            {
                BtlController_EmitDrawTrainerPic(BUFFER_A);
                MarkBattlerForControllerExec(gActiveBattler);
            }
        }

        if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS && GetBattlerPosition(gActiveBattler) == B_POSITION_OPPONENT_RIGHT)
        {
            BtlController_EmitDrawTrainerPic(BUFFER_A);
            MarkBattlerForControllerExec(gActiveBattler);
        }

        if (gBattleTypeFlags & BATTLE_TYPE_ARENA)
            BattleArena_InitPoints();
    }
    gBattleMainFunc = BattleIntroDrawPartySummaryScreens;
}

static void BattleIntroDrawPartySummaryScreens(void)
{
    s32 i;
    struct HpAndStatus hpStatus[PARTY_SIZE];

    if (gBattleControllerExecFlags)
        return;

    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_NONE
             || GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_EGG)
            {
                hpStatus[i].hp = HP_EMPTY_SLOT;
                hpStatus[i].status = 0;
            }
            else
            {
                hpStatus[i].hp = GetMonData(&gEnemyParty[i], MON_DATA_HP);
                hpStatus[i].status = GetMonData(&gEnemyParty[i], MON_DATA_STATUS);
            }
        }
        gActiveBattler = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        BtlController_EmitDrawPartyStatusSummary(BUFFER_A, hpStatus, PARTY_SUMM_SKIP_DRAW_DELAY);
        MarkBattlerForControllerExec(gActiveBattler);

        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_NONE
             || GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_EGG)
            {
                hpStatus[i].hp = HP_EMPTY_SLOT;
                hpStatus[i].status = 0;
            }
            else
            {
                hpStatus[i].hp = GetMonData(&gPlayerParty[i], MON_DATA_HP);
                hpStatus[i].status = GetMonData(&gPlayerParty[i], MON_DATA_STATUS);
            }
        }
        gActiveBattler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        BtlController_EmitDrawPartyStatusSummary(BUFFER_A, hpStatus, PARTY_SUMM_SKIP_DRAW_DELAY);
        MarkBattlerForControllerExec(gActiveBattler);

        gBattleMainFunc = BattleIntroPrintTrainerWantsToBattle;
    }
    else
    {
        // The struct gets set here, but nothing is ever done with it since
        // wild battles don't show the party summary.
        // Still, there's no point in having dead code.

        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_NONE
             || GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_EGG)
            {
                hpStatus[i].hp = HP_EMPTY_SLOT;
                hpStatus[i].status = 0;
            }
            else
            {
                hpStatus[i].hp = GetMonData(&gPlayerParty[i], MON_DATA_HP);
                hpStatus[i].status = GetMonData(&gPlayerParty[i], MON_DATA_STATUS);
            }
        }

        gBattleMainFunc = BattleIntroPrintWildMonAttacked;
    }
}

static void BattleIntroPrintTrainerWantsToBattle(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        gActiveBattler = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        PrepareStringBattle(STRINGID_INTROMSG, gActiveBattler);
        gBattleMainFunc = BattleIntroPrintOpponentSendsOut;
    }
}

static void BattleIntroPrintWildMonAttacked(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        gBattleMainFunc = BattleIntroPrintPlayerSendsOut;
        PrepareStringBattle(STRINGID_INTROMSG, 0);
    }
}

static void BattleIntroPrintOpponentSendsOut(void)
{
    u32 position;

    if (gBattleControllerExecFlags)
        return;

    if (!(gBattleTypeFlags & BATTLE_TYPE_RECORDED))
    {
        position = B_POSITION_OPPONENT_LEFT;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_IS_MASTER)
            position = B_POSITION_OPPONENT_LEFT;
        else
            position = B_POSITION_PLAYER_LEFT;
    }
    else
    {
        position = B_POSITION_OPPONENT_LEFT;
    }

    PrepareStringBattle(STRINGID_INTROSENDOUT, GetBattlerAtPosition(position));
    gBattleMainFunc = BattleIntroOpponent1SendsOutMonAnimation;
}

static void BattleIntroOpponent2SendsOutMonAnimation(void)
{
    u32 position;

    if (!(gBattleTypeFlags & BATTLE_TYPE_RECORDED))
    {
        position = B_POSITION_OPPONENT_RIGHT;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_IS_MASTER)
            position = B_POSITION_OPPONENT_RIGHT;
        else
            position = B_POSITION_PLAYER_RIGHT;
    }
    else
    {
        position = B_POSITION_OPPONENT_RIGHT;
    }

    for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
    {
        if (GetBattlerPosition(gActiveBattler) == position)
        {
            BtlController_EmitIntroTrainerBallThrow(BUFFER_A);
            MarkBattlerForControllerExec(gActiveBattler);
        }
    }

    gBattleMainFunc = BattleIntroRecordMonsToDex;
}

static void BattleIntroOpponent1SendsOutMonAnimation(void)
{
    u32 position;

    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
        {
            if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_IS_MASTER)
                position = B_POSITION_OPPONENT_LEFT;
            else
                position = B_POSITION_PLAYER_LEFT;
        }
        else
        {
            position = B_POSITION_OPPONENT_LEFT;
        }
    }
    else
    {
        position = B_POSITION_OPPONENT_LEFT;
    }

    if (gBattleControllerExecFlags)
        return;

    for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
    {
        if (GetBattlerPosition(gActiveBattler) == position)
        {
            BtlController_EmitIntroTrainerBallThrow(BUFFER_A);
            MarkBattlerForControllerExec(gActiveBattler);
            if (gBattleTypeFlags & (BATTLE_TYPE_MULTI | BATTLE_TYPE_TWO_OPPONENTS))
            {
                gBattleMainFunc = BattleIntroOpponent2SendsOutMonAnimation;
                return;
            }
        }
    }

    gBattleMainFunc = BattleIntroRecordMonsToDex;
}

static void BattleIntroRecordMonsToDex(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
        {
            if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT
             && !(gBattleTypeFlags & (BATTLE_TYPE_EREADER_TRAINER
                                      | BATTLE_TYPE_FRONTIER
                                      | BATTLE_TYPE_LINK
                                      | BATTLE_TYPE_RECORDED_LINK
                                      | BATTLE_TYPE_TRAINER_HILL)))
            {
                HandleSetPokedexFlag(SpeciesToNationalPokedexNum(gBattleMons[gActiveBattler].species), FLAG_SET_SEEN, gBattleMons[gActiveBattler].personality);
            }
        }
        gBattleMainFunc = BattleIntroPrintPlayerSendsOut;
    }
}

static void UNUSED BattleIntroSkipRecordMonsToDex(void)
{
    if (gBattleControllerExecFlags == 0)
        gBattleMainFunc = BattleIntroPrintPlayerSendsOut;
}

static void BattleIntroPrintPlayerSendsOut(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        u8 position;

        if (!(gBattleTypeFlags & BATTLE_TYPE_RECORDED))
        {
            position = B_POSITION_PLAYER_LEFT;
        }
        else if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
        {
            if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_IS_MASTER)
                position = B_POSITION_PLAYER_LEFT;
            else
                position = B_POSITION_OPPONENT_LEFT;
        }
        else
        {
            position = B_POSITION_PLAYER_LEFT;
        }

        if (!(gBattleTypeFlags & BATTLE_TYPE_SAFARI))
            PrepareStringBattle(STRINGID_INTROSENDOUT, GetBattlerAtPosition(position));

        gBattleMainFunc = BattleIntroPlayer1SendsOutMonAnimation;
    }
}

static void BattleIntroPlayer2SendsOutMonAnimation(void)
{
    u32 position;

    if (!(gBattleTypeFlags & BATTLE_TYPE_RECORDED))
    {
        position = B_POSITION_PLAYER_RIGHT;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_IS_MASTER)
            position = B_POSITION_PLAYER_RIGHT;
        else
            position = B_POSITION_OPPONENT_RIGHT;
    }
    else
    {
        position = B_POSITION_PLAYER_RIGHT;
    }

    for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
    {
        if (GetBattlerPosition(gActiveBattler) == position)
        {
            BtlController_EmitIntroTrainerBallThrow(BUFFER_A);
            MarkBattlerForControllerExec(gActiveBattler);
        }
    }

    gBattleStruct->switchInAbilitiesCounter = 0;
    gBattleStruct->switchInItemsCounter = 0;
    gBattleStruct->overworldWeatherDone = FALSE;

    gBattleMainFunc = TryDoEventsBeforeFirstTurn;
}

static void BattleIntroPlayer1SendsOutMonAnimation(void)
{
    u32 position;

    if (!(gBattleTypeFlags & BATTLE_TYPE_RECORDED))
    {
        position = B_POSITION_PLAYER_LEFT;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_IS_MASTER)
            position = B_POSITION_PLAYER_LEFT;
        else
            position = B_POSITION_OPPONENT_LEFT;
    }
    else
    {
        position = B_POSITION_PLAYER_LEFT;
    }

    if (gBattleControllerExecFlags)
        return;

    for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
    {
        if (GetBattlerPosition(gActiveBattler) == position)
        {
            BtlController_EmitIntroTrainerBallThrow(BUFFER_A);
            MarkBattlerForControllerExec(gActiveBattler);
            if (gBattleTypeFlags & (BATTLE_TYPE_MULTI))
            {
                gBattleMainFunc = BattleIntroPlayer2SendsOutMonAnimation;
                return;
            }
        }
    }

    gBattleStruct->switchInAbilitiesCounter = 0;
    gBattleStruct->switchInItemsCounter = 0;
    gBattleStruct->overworldWeatherDone = FALSE;

    gBattleMainFunc = TryDoEventsBeforeFirstTurn;
}

static void UNUSED BattleIntroSwitchInPlayerMons(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
        {
            if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
            {
                BtlController_EmitSwitchInAnim(BUFFER_A, gBattlerPartyIndexes[gActiveBattler], FALSE);
                MarkBattlerForControllerExec(gActiveBattler);
            }
        }

        gBattleStruct->switchInAbilitiesCounter = 0;
        gBattleStruct->switchInItemsCounter = 0;
        gBattleStruct->overworldWeatherDone = FALSE;

        gBattleMainFunc = TryDoEventsBeforeFirstTurn;
    }
}

static void TryDoEventsBeforeFirstTurn(void)
{
    s32 i;
    s32 j;
    u8 effect = 0;

    if (gBattleControllerExecFlags)
        return;

    if (gBattleStruct->switchInAbilitiesCounter == 0)
    {
        for (i = 0; i < gBattlersCount; i++)
            gBattlerByTurnOrder[i] = i;
        for (i = 0; i < gBattlersCount - 1; i++)
        {
            for (j = i + 1; j < gBattlersCount; j++)
            {
                if (GetWhoStrikesFirst(gBattlerByTurnOrder[i], gBattlerByTurnOrder[j], TRUE) != 0)
                    SwapTurnOrder(i, j);
            }
        }
    }
    if (!gBattleStruct->overworldWeatherDone
        && AbilityBattleEffects(0, 0, 0, ABILITYEFFECT_SWITCH_IN_WEATHER, 0) != 0)
    {
        gBattleStruct->overworldWeatherDone = TRUE;
        return;
    }
    // Check all switch in abilities happening from the fastest mon to slowest.
    while (gBattleStruct->switchInAbilitiesCounter < gBattlersCount)
    {
        if (AbilityBattleEffects(ABILITYEFFECT_ON_SWITCHIN, gBattlerByTurnOrder[gBattleStruct->switchInAbilitiesCounter], 0, 0, 0) != 0)
            effect++;

        gBattleStruct->switchInAbilitiesCounter++;

        if (effect != 0)
            return;
    }
    if (AbilityBattleEffects(ABILITYEFFECT_INTIMIDATE1, 0, 0, 0, 0) != 0)
        return;
    if (AbilityBattleEffects(ABILITYEFFECT_TRACE, 0, 0, 0, 0) != 0)
        return;
    // Check all switch in items having effect from the fastest mon to slowest.
    while (gBattleStruct->switchInItemsCounter < gBattlersCount)
    {
        if (ItemBattleEffects(ITEMEFFECT_ON_SWITCH_IN, gBattlerByTurnOrder[gBattleStruct->switchInItemsCounter], FALSE))
            effect++;

        gBattleStruct->switchInItemsCounter++;

        if (effect != 0)
            return;
    }
    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        *(gBattleStruct->monToSwitchIntoId + i) = PARTY_SIZE;
        gChosenActionByBattler[i] = B_ACTION_NONE;
        gChosenMoveByBattler[i] = MOVE_NONE;
    }
    TurnValuesCleanUp(FALSE);
    SpecialStatusesClear();
    *(&gBattleStruct->absentBattlerFlags) = gAbsentBattlerFlags;
    BattlePutTextOnWindow(gText_EmptyString3, B_WIN_MSG);
    gBattleMainFunc = HandleTurnActionSelectionState;
    ResetSentPokesToOpponentValue();

    for (i = 0; i < BATTLE_COMMUNICATION_ENTRIES_COUNT; i++)
        gBattleCommunication[i] = 0;

    for (i = 0; i < gBattlersCount; i++)
        gBattleMons[i].status2 &= ~STATUS2_FLINCHED;

    *(&gBattleStruct->turnEffectsTracker) = 0;
    *(&gBattleStruct->turnEffectsBattlerId) = 0;
    *(&gBattleStruct->wishPerishSongState) = 0;
    *(&gBattleStruct->wishPerishSongBattlerId) = 0;
    gBattleScripting.moveendState = 0;
    gBattleStruct->faintedActionsState = 0;
    gBattleStruct->turnCountersTracker = 0;
    gMoveResultFlags = 0;

    gRandomTurnNumber = Random();

    if (gBattleTypeFlags & BATTLE_TYPE_ARENA)
    {
        StopCryAndClearCrySongs();
        BattleScriptExecute(BattleScript_ArenaTurnBeginning);
    }
}

static void HandleEndTurn_ContinueBattle(void)
{
    s32 i;

    if (gBattleControllerExecFlags == 0)
    {
        gBattleMainFunc = BattleTurnPassed;
        for (i = 0; i < BATTLE_COMMUNICATION_ENTRIES_COUNT; i++)
            gBattleCommunication[i] = 0;
        for (i = 0; i < gBattlersCount; i++)
        {
            gBattleMons[i].status2 &= ~STATUS2_FLINCHED;
            if ((gBattleMons[i].status1 & STATUS1_SLEEP) && (gBattleMons[i].status2 & STATUS2_MULTIPLETURNS))
                CancelMultiTurnMoves(i);
        }
        gBattleStruct->turnEffectsTracker = 0;
        gBattleStruct->turnEffectsBattlerId = 0;
        gBattleStruct->wishPerishSongState = 0;
        gBattleStruct->wishPerishSongBattlerId = 0;
        gBattleStruct->turnCountersTracker = 0;
        gMoveResultFlags = 0;
    }
}

void BattleTurnPassed(void)
{
    s32 i;

    TurnValuesCleanUp(TRUE);
    if (gBattleOutcome == 0)
    {
        if (DoFieldEndTurnEffects())
            return;
        if (DoBattlerEndTurnEffects())
            return;
    }
    if (HandleFaintedMonActions())
        return;
    gBattleStruct->faintedActionsState = 0;
    if (HandleWishPerishSongOnTurnEnd())
        return;

    TurnValuesCleanUp(FALSE);
    gHitMarker &= ~HITMARKER_NO_ATTACKSTRING;
    gHitMarker &= ~HITMARKER_UNABLE_TO_USE_MOVE;
    gHitMarker &= ~HITMARKER_PLAYER_FAINTED;
    gHitMarker &= ~HITMARKER_PASSIVE_DAMAGE;
    gBattleScripting.animTurn = 0;
    gBattleScripting.animTargetsHit = 0;
    gBattleScripting.moveendState = 0;
    gBattleMoveDamage = 0;
    gMoveResultFlags = 0;

    for (i = 0; i < 5; i++)
        gBattleCommunication[i] = 0;

    if (gBattleOutcome != 0)
    {
        gCurrentActionFuncId = B_ACTION_FINISHED;
        gBattleMainFunc = RunTurnActionsFunctions;
        return;
    }

    if (gBattleResults.battleTurnCounter < 0xFF)
    {
        gBattleResults.battleTurnCounter++;
        gBattleStruct->arenaTurnCounter++;
    }

    for (i = 0; i < gBattlersCount; i++)
    {
        gChosenActionByBattler[i] = B_ACTION_NONE;
        gChosenMoveByBattler[i] = MOVE_NONE;
    }

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
        *(gBattleStruct->monToSwitchIntoId + i) = PARTY_SIZE;

    *(&gBattleStruct->absentBattlerFlags) = gAbsentBattlerFlags;
    BattlePutTextOnWindow(gText_EmptyString3, B_WIN_MSG);
    gBattleMainFunc = HandleTurnActionSelectionState;
    gRandomTurnNumber = Random();

    if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        BattleScriptExecute(BattleScript_PalacePrintFlavorText);
    else if (gBattleTypeFlags & BATTLE_TYPE_ARENA && gBattleStruct->arenaTurnCounter == 0)
        BattleScriptExecute(BattleScript_ArenaTurnBeginning);
}

u8 IsRunningFromBattleImpossible(void)
{
    u8 holdEffect;
    u8 side;
    s32 i;

    if (gBattleMons[gActiveBattler].item == ITEM_ENIGMA_BERRY)
        holdEffect = gEnigmaBerries[gActiveBattler].holdEffect;
    else
        holdEffect = ItemId_GetHoldEffect(gBattleMons[gActiveBattler].item);

    gPotentialItemEffectBattler = gActiveBattler;

    if (holdEffect == HOLD_EFFECT_CAN_ALWAYS_RUN)
        return BATTLE_RUN_SUCCESS;
    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        return BATTLE_RUN_SUCCESS;
    if (gBattleMons[gActiveBattler].ability == ABILITY_RUN_AWAY)
        return BATTLE_RUN_SUCCESS;

    side = GetBattlerSide(gActiveBattler);

    for (i = 0; i < gBattlersCount; i++)
    {
        if (side != GetBattlerSide(i)
         && gBattleMons[i].ability == ABILITY_SHADOW_TAG)
        {
            gBattleScripting.battler = i;
            gLastUsedAbility = gBattleMons[i].ability;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PREVENTS_ESCAPE;
            return BATTLE_RUN_FAILURE;
        }
        if (side != GetBattlerSide(i)
         && gBattleMons[gActiveBattler].ability != ABILITY_LEVITATE
         && !IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_FLYING)
         && gBattleMons[i].ability == ABILITY_ARENA_TRAP)
        {
            gBattleScripting.battler = i;
            gLastUsedAbility = gBattleMons[i].ability;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PREVENTS_ESCAPE;
            return BATTLE_RUN_FAILURE;
        }
    }
    i = AbilityBattleEffects(ABILITYEFFECT_CHECK_FIELD_EXCEPT_BATTLER, gActiveBattler, ABILITY_MAGNET_PULL, 0, 0);
    if (i != 0 && IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_STEEL))
    {
        gBattleScripting.battler = i - 1;
        gLastUsedAbility = gBattleMons[i - 1].ability;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PREVENTS_ESCAPE;
        return BATTLE_RUN_FAILURE;
    }
    if ((gBattleMons[gActiveBattler].status2 & (STATUS2_ESCAPE_PREVENTION | STATUS2_WRAPPED))
        || (gStatuses3[gActiveBattler] & STATUS3_ROOTED))
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CANT_ESCAPE;
        return BATTLE_RUN_FORBIDDEN;
    }
    if (gBattleTypeFlags & BATTLE_TYPE_FIRST_BATTLE)
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_DONT_LEAVE_BIRCH;
        return BATTLE_RUN_FORBIDDEN;
    }
    return BATTLE_RUN_SUCCESS;
}

void SwitchPartyOrder(u8 battler)
{
    s32 i;
    u8 partyId1;
    u8 partyId2;

    for (i = 0; i < (int)ARRAY_COUNT(gBattlePartyCurrentOrder); i++)
        gBattlePartyCurrentOrder[i] = *(battler * 3 + i + (u8 *)(gBattleStruct->battlerPartyOrders));

    partyId1 = GetPartyIdFromBattlePartyId(gBattlerPartyIndexes[battler]);
    partyId2 = GetPartyIdFromBattlePartyId(*(gBattleStruct->monToSwitchIntoId + battler));
    SwitchPartyMonSlots(partyId1, partyId2);

    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
    {
        for (i = 0; i < (int)ARRAY_COUNT(gBattlePartyCurrentOrder); i++)
        {
            *(battler * 3 + i + (u8 *)(gBattleStruct->battlerPartyOrders)) = gBattlePartyCurrentOrder[i];
            *(BATTLE_PARTNER(battler) * 3 + i + (u8 *)(gBattleStruct->battlerPartyOrders)) = gBattlePartyCurrentOrder[i];
        }
    }
    else
    {
        for (i = 0; i < (int)ARRAY_COUNT(gBattlePartyCurrentOrder); i++)
        {
            *(battler * 3 + i + (u8 *)(gBattleStruct->battlerPartyOrders)) = gBattlePartyCurrentOrder[i];
        }
    }
}

enum
{
    STATE_TURN_START_RECORD,
    STATE_BEFORE_ACTION_CHOSEN,
    STATE_WAIT_ACTION_CHOSEN,
    STATE_WAIT_ACTION_CASE_CHOSEN,
    STATE_WAIT_ACTION_CONFIRMED_STANDBY,
    STATE_WAIT_ACTION_CONFIRMED,
    STATE_SELECTION_SCRIPT,
    STATE_WAIT_SET_BEFORE_ACTION,
    STATE_SELECTION_SCRIPT_MAY_RUN
};

static void HandleTurnActionSelectionState(void)
{
    s32 i;

    gBattleCommunication[ACTIONS_CONFIRMED_COUNT] = 0;
    for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
    {
        u8 position = GetBattlerPosition(gActiveBattler);
        switch (gBattleCommunication[gActiveBattler])
        {
        case STATE_TURN_START_RECORD: // Recorded battle related action on start of every turn.
            RecordedBattle_CopyBattlerMoves();
            gBattleCommunication[gActiveBattler] = STATE_BEFORE_ACTION_CHOSEN;
            break;
        case STATE_BEFORE_ACTION_CHOSEN: // Choose an action.
            *(gBattleStruct->monToSwitchIntoId + gActiveBattler) = PARTY_SIZE;
            if (gBattleTypeFlags & BATTLE_TYPE_MULTI
                || (position & BIT_FLANK) == B_FLANK_LEFT
                || gBattleStruct->absentBattlerFlags & gBitTable[GetBattlerAtPosition(BATTLE_PARTNER(position))]
                || gBattleCommunication[GetBattlerAtPosition(BATTLE_PARTNER(position))] == STATE_WAIT_ACTION_CONFIRMED)
            {
                if (gBattleStruct->absentBattlerFlags & gBitTable[gActiveBattler])
                {
                    gChosenActionByBattler[gActiveBattler] = B_ACTION_NOTHING_FAINTED;
                    if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI))
                        gBattleCommunication[gActiveBattler] = STATE_WAIT_ACTION_CONFIRMED;
                    else
                        gBattleCommunication[gActiveBattler] = STATE_WAIT_ACTION_CONFIRMED_STANDBY;
                }
                else
                {
                    if (gBattleMons[gActiveBattler].status2 & STATUS2_MULTIPLETURNS
                        || gBattleMons[gActiveBattler].status2 & STATUS2_RECHARGE)
                    {
                        gChosenActionByBattler[gActiveBattler] = B_ACTION_USE_MOVE;
                        gBattleCommunication[gActiveBattler] = STATE_WAIT_ACTION_CONFIRMED_STANDBY;
                    }
                    else
                    {
                        BtlController_EmitChooseAction(BUFFER_A, gChosenActionByBattler[0], gBattleBufferB[0][1] | (gBattleBufferB[0][2] << 8));
                        MarkBattlerForControllerExec(gActiveBattler);
                        gBattleCommunication[gActiveBattler]++;
                    }
                }
            }
            break;
        case STATE_WAIT_ACTION_CHOSEN: // Try to perform an action.
            if (!(gBattleControllerExecFlags & ((gBitTable[gActiveBattler]) | (0xF << 28) | (gBitTable[gActiveBattler] << 4) | (gBitTable[gActiveBattler] << 8) | (gBitTable[gActiveBattler] << 12))))
            {
                RecordedBattle_SetBattlerAction(gActiveBattler, gBattleBufferB[gActiveBattler][1]);
                gChosenActionByBattler[gActiveBattler] = gBattleBufferB[gActiveBattler][1];

                switch (gBattleBufferB[gActiveBattler][1])
                {
                case B_ACTION_USE_MOVE:
                    if (AreAllMovesUnusable())
                    {
                        gBattleCommunication[gActiveBattler] = STATE_SELECTION_SCRIPT;
                        *(gBattleStruct->selectionScriptFinished + gActiveBattler) = FALSE;
                        *(gBattleStruct->stateIdAfterSelScript + gActiveBattler) = STATE_WAIT_ACTION_CONFIRMED_STANDBY;
                        *(gBattleStruct->moveTarget + gActiveBattler) = gBattleBufferB[gActiveBattler][3];
                        return;
                    }
                    else if (gDisableStructs[gActiveBattler].encoredMove != 0)
                    {
                        gChosenMoveByBattler[gActiveBattler] = gDisableStructs[gActiveBattler].encoredMove;
                        *(gBattleStruct->chosenMovePositions + gActiveBattler) = gDisableStructs[gActiveBattler].encoredMovePos;
                        gBattleCommunication[gActiveBattler] = STATE_WAIT_ACTION_CONFIRMED_STANDBY;
                        return;
                    }
                    else
                    {
                        struct ChooseMoveStruct moveInfo;

                        moveInfo.species = gBattleMons[gActiveBattler].species;
                        moveInfo.monTypes[0] = gBattleMons[gActiveBattler].types[0];
                        moveInfo.monTypes[1] = gBattleMons[gActiveBattler].types[1];

                        for (i = 0; i < MAX_MON_MOVES; i++)
                        {
                            moveInfo.moves[i] = gBattleMons[gActiveBattler].moves[i];
                            moveInfo.currentPp[i] = gBattleMons[gActiveBattler].pp[i];
                            moveInfo.maxPp[i] = CalculatePPWithBonus(
                                                            gBattleMons[gActiveBattler].moves[i],
                                                            gBattleMons[gActiveBattler].ppBonuses,
                                                            i);
                        }

                        BtlController_EmitChooseMove(BUFFER_A, (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) != 0, FALSE, &moveInfo);
                        MarkBattlerForControllerExec(gActiveBattler);
                    }
                    break;
                case B_ACTION_USE_ITEM:
                    if (gBattleTypeFlags & (BATTLE_TYPE_LINK
                                            | BATTLE_TYPE_FRONTIER_NO_PYRAMID
                                            | BATTLE_TYPE_EREADER_TRAINER
                                            | BATTLE_TYPE_RECORDED_LINK))
                    {
                        RecordedBattle_ClearBattlerAction(gActiveBattler, 1);
                        gSelectionBattleScripts[gActiveBattler] = BattleScript_ActionSelectionItemsCantBeUsed;
                        gBattleCommunication[gActiveBattler] = STATE_SELECTION_SCRIPT;
                        *(gBattleStruct->selectionScriptFinished + gActiveBattler) = FALSE;
                        *(gBattleStruct->stateIdAfterSelScript + gActiveBattler) = STATE_BEFORE_ACTION_CHOSEN;
                        return;
                    }
                    else
                    {
                        BtlController_EmitChooseItem(BUFFER_A, gBattleStruct->battlerPartyOrders[gActiveBattler]);
                        MarkBattlerForControllerExec(gActiveBattler);
                    }
                    break;
                case B_ACTION_SWITCH:
                    *(gBattleStruct->battlerPartyIndexes + gActiveBattler) = gBattlerPartyIndexes[gActiveBattler];
                    if (gBattleMons[gActiveBattler].status2 & (STATUS2_WRAPPED | STATUS2_ESCAPE_PREVENTION)
                        || gBattleTypeFlags & BATTLE_TYPE_ARENA
                        || gStatuses3[gActiveBattler] & STATUS3_ROOTED)
                    {
                        BtlController_EmitChoosePokemon(BUFFER_A, PARTY_ACTION_CANT_SWITCH, PARTY_SIZE, ABILITY_NONE, gBattleStruct->battlerPartyOrders[gActiveBattler]);
                    }
                    else if ((i = ABILITY_ON_OPPOSING_FIELD(gActiveBattler, ABILITY_SHADOW_TAG))
                             || ((i = ABILITY_ON_OPPOSING_FIELD(gActiveBattler, ABILITY_ARENA_TRAP))
                                 && !IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_FLYING)
                                 && gBattleMons[gActiveBattler].ability != ABILITY_LEVITATE)
                             || ((i = AbilityBattleEffects(ABILITYEFFECT_CHECK_FIELD_EXCEPT_BATTLER, gActiveBattler, ABILITY_MAGNET_PULL, 0, 0))
                                 && IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_STEEL)))
                    {
                        BtlController_EmitChoosePokemon(BUFFER_A, ((i - 1) << 4) | PARTY_ACTION_ABILITY_PREVENTS, PARTY_SIZE, gLastUsedAbility, gBattleStruct->battlerPartyOrders[gActiveBattler]);
                    }
                    else
                    {
                        if (gActiveBattler == 2 && gChosenActionByBattler[0] == B_ACTION_SWITCH)
                            BtlController_EmitChoosePokemon(BUFFER_A, PARTY_ACTION_CHOOSE_MON, *(gBattleStruct->monToSwitchIntoId + 0), ABILITY_NONE, gBattleStruct->battlerPartyOrders[gActiveBattler]);
                        else if (gActiveBattler == 3 && gChosenActionByBattler[1] == B_ACTION_SWITCH)
                            BtlController_EmitChoosePokemon(BUFFER_A, PARTY_ACTION_CHOOSE_MON, *(gBattleStruct->monToSwitchIntoId + 1), ABILITY_NONE, gBattleStruct->battlerPartyOrders[gActiveBattler]);
                        else
                            BtlController_EmitChoosePokemon(BUFFER_A, PARTY_ACTION_CHOOSE_MON, PARTY_SIZE, ABILITY_NONE, gBattleStruct->battlerPartyOrders[gActiveBattler]);
                    }
                    MarkBattlerForControllerExec(gActiveBattler);
                    break;
                case B_ACTION_SAFARI_BALL:
                    if (IsPlayerPartyAndPokemonStorageFull())
                    {
                        gSelectionBattleScripts[gActiveBattler] = BattleScript_PrintFullBox;
                        gBattleCommunication[gActiveBattler] = STATE_SELECTION_SCRIPT;
                        *(gBattleStruct->selectionScriptFinished + gActiveBattler) = FALSE;
                        *(gBattleStruct->stateIdAfterSelScript + gActiveBattler) = STATE_BEFORE_ACTION_CHOSEN;
                        return;
                    }
                    break;
                case B_ACTION_SAFARI_POKEBLOCK:
                    BtlController_EmitChooseItem(BUFFER_A, gBattleStruct->battlerPartyOrders[gActiveBattler]);
                    MarkBattlerForControllerExec(gActiveBattler);
                    break;
                case B_ACTION_CANCEL_PARTNER:
                    gBattleCommunication[gActiveBattler] = STATE_WAIT_SET_BEFORE_ACTION;
                    gBattleCommunication[GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler)))] = STATE_BEFORE_ACTION_CHOSEN;
                    RecordedBattle_ClearBattlerAction(gActiveBattler, 1);
                    if (gBattleMons[GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler)))].status2 & STATUS2_MULTIPLETURNS
                        || gBattleMons[GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler)))].status2 & STATUS2_RECHARGE)
                    {
                        BtlController_EmitEndBounceEffect(BUFFER_A);
                        MarkBattlerForControllerExec(gActiveBattler);
                        return;
                    }
                    else if (gChosenActionByBattler[GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler)))] == B_ACTION_SWITCH)
                    {
                        RecordedBattle_ClearBattlerAction(GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler))), 2);
                    }
                    else if (gChosenActionByBattler[GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler)))] == B_ACTION_RUN)
                    {
                        RecordedBattle_ClearBattlerAction(GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler))), 1);
                    }
                    else if (gChosenActionByBattler[GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler)))] == B_ACTION_USE_MOVE
                             && (gProtectStructs[GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler)))].noValidMoves
                                || gDisableStructs[GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler)))].encoredMove))
                    {
                        RecordedBattle_ClearBattlerAction(GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler))), 1);
                    }
                    else if (gBattleTypeFlags & BATTLE_TYPE_PALACE
                             && gChosenActionByBattler[GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler)))] == B_ACTION_USE_MOVE)
                    {
                        gRngValue = gBattlePalaceMoveSelectionRngValue;
                        RecordedBattle_ClearBattlerAction(GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler))), 1);
                    }
                    else
                    {
                        RecordedBattle_ClearBattlerAction(GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gActiveBattler))), 3);
                    }
                    BtlController_EmitEndBounceEffect(BUFFER_A);
                    MarkBattlerForControllerExec(gActiveBattler);
                    return;
                }

                if (gBattleTypeFlags & BATTLE_TYPE_TRAINER
                    && gBattleTypeFlags & (BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL)
                    && gBattleBufferB[gActiveBattler][1] == B_ACTION_RUN)
                {
                    gSelectionBattleScripts[gActiveBattler] = BattleScript_AskIfWantsToForfeitMatch;
                    gBattleCommunication[gActiveBattler] = STATE_SELECTION_SCRIPT_MAY_RUN;
                    *(gBattleStruct->selectionScriptFinished + gActiveBattler) = FALSE;
                    *(gBattleStruct->stateIdAfterSelScript + gActiveBattler) = STATE_BEFORE_ACTION_CHOSEN;
                    return;
                }
                else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER
                         && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
                         && gBattleBufferB[gActiveBattler][1] == B_ACTION_RUN)
                {
                    BattleScriptExecute(BattleScript_PrintCantRunFromTrainer);
                    gBattleCommunication[gActiveBattler] = STATE_BEFORE_ACTION_CHOSEN;
                }
                else if (IsRunningFromBattleImpossible() != BATTLE_RUN_SUCCESS
                         && gBattleBufferB[gActiveBattler][1] == B_ACTION_RUN)
                {
                    gSelectionBattleScripts[gActiveBattler] = BattleScript_PrintCantEscapeFromBattle;
                    gBattleCommunication[gActiveBattler] = STATE_SELECTION_SCRIPT;
                    *(gBattleStruct->selectionScriptFinished + gActiveBattler) = FALSE;
                    *(gBattleStruct->stateIdAfterSelScript + gActiveBattler) = STATE_BEFORE_ACTION_CHOSEN;
                    return;
                }
                else
                {
                    gBattleCommunication[gActiveBattler]++;
                }
            }
            break;
        case STATE_WAIT_ACTION_CASE_CHOSEN:
            if (!(gBattleControllerExecFlags & ((gBitTable[gActiveBattler]) | (0xF << 28) | (gBitTable[gActiveBattler] << 4) | (gBitTable[gActiveBattler] << 8) | (gBitTable[gActiveBattler] << 12))))
            {
                switch (gChosenActionByBattler[gActiveBattler])
                {
                case B_ACTION_USE_MOVE:
                    switch (gBattleBufferB[gActiveBattler][1])
                    {
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                    case 9:
                        gChosenActionByBattler[gActiveBattler] = gBattleBufferB[gActiveBattler][1];
                        return;
                    case 15:
                        gChosenActionByBattler[gActiveBattler] = B_ACTION_SWITCH;
                        UpdateBattlerPartyOrdersOnSwitch();
                        return;
                    default:
                        RecordedBattle_CheckMovesetChanges(B_RECORD_MODE_PLAYBACK);
                        if ((gBattleBufferB[gActiveBattler][2] | (gBattleBufferB[gActiveBattler][3] << 8)) == 0xFFFF)
                        {
                            gBattleCommunication[gActiveBattler] = STATE_BEFORE_ACTION_CHOSEN;
                            RecordedBattle_ClearBattlerAction(gActiveBattler, 1);
                        }
                        else if (TrySetCantSelectMoveBattleScript())
                        {
                            RecordedBattle_ClearBattlerAction(gActiveBattler, 1);
                            gBattleCommunication[gActiveBattler] = STATE_SELECTION_SCRIPT;
                            *(gBattleStruct->selectionScriptFinished + gActiveBattler) = FALSE;
                            gBattleBufferB[gActiveBattler][1] = B_ACTION_USE_MOVE;
                            *(gBattleStruct->stateIdAfterSelScript + gActiveBattler) = STATE_WAIT_ACTION_CHOSEN;
                            return;
                        }
                        else
                        {
                            if (!(gBattleTypeFlags & BATTLE_TYPE_PALACE))
                            {
                                RecordedBattle_SetBattlerAction(gActiveBattler, gBattleBufferB[gActiveBattler][2]);
                                RecordedBattle_SetBattlerAction(gActiveBattler, gBattleBufferB[gActiveBattler][3]);
                            }
                            *(gBattleStruct->chosenMovePositions + gActiveBattler) = gBattleBufferB[gActiveBattler][2];
                            gChosenMoveByBattler[gActiveBattler] = gBattleMons[gActiveBattler].moves[*(gBattleStruct->chosenMovePositions + gActiveBattler)];
                            *(gBattleStruct->moveTarget + gActiveBattler) = gBattleBufferB[gActiveBattler][3];
                            gBattleCommunication[gActiveBattler]++;
                        }
                        break;
                    }
                    break;
                case B_ACTION_USE_ITEM:
                    if ((gBattleBufferB[gActiveBattler][1] | (gBattleBufferB[gActiveBattler][2] << 8)) == 0)
                    {
                        gBattleCommunication[gActiveBattler] = STATE_BEFORE_ACTION_CHOSEN;
                    }
                    else
                    {
                        gLastUsedItem = (gBattleBufferB[gActiveBattler][1] | (gBattleBufferB[gActiveBattler][2] << 8));
                        gBattleCommunication[gActiveBattler]++;
                    }
                    break;
                case B_ACTION_SWITCH:
                    if (gBattleBufferB[gActiveBattler][1] == PARTY_SIZE)
                    {
                        gBattleCommunication[gActiveBattler] = STATE_BEFORE_ACTION_CHOSEN;
                        RecordedBattle_ClearBattlerAction(gActiveBattler, 1);
                    }
                    else
                    {
                        UpdateBattlerPartyOrdersOnSwitch();
                        gBattleCommunication[gActiveBattler]++;
                    }
                    break;
                case B_ACTION_RUN:
                    gHitMarker |= HITMARKER_RUN;
                    gBattleCommunication[gActiveBattler]++;
                    break;
                case B_ACTION_SAFARI_WATCH_CAREFULLY:
                    gBattleCommunication[gActiveBattler]++;
                    break;
                case B_ACTION_SAFARI_BALL:
                    gBattleCommunication[gActiveBattler]++;
                    break;
                case B_ACTION_SAFARI_POKEBLOCK:
                    if ((gBattleBufferB[gActiveBattler][1] | (gBattleBufferB[gActiveBattler][2] << 8)) != 0)
                        gBattleCommunication[gActiveBattler]++;
                    else
                        gBattleCommunication[gActiveBattler] = STATE_BEFORE_ACTION_CHOSEN;
                    break;
                case B_ACTION_SAFARI_GO_NEAR:
                    gBattleCommunication[gActiveBattler]++;
                    break;
                case B_ACTION_SAFARI_RUN:
                    gHitMarker |= HITMARKER_RUN;
                    gBattleCommunication[gActiveBattler]++;
                    break;
                case B_ACTION_WALLY_THROW:
                    gBattleCommunication[gActiveBattler]++;
                    break;
                }
            }
            break;
        case STATE_WAIT_ACTION_CONFIRMED_STANDBY:
            if (!(gBattleControllerExecFlags & ((gBitTable[gActiveBattler])
                                                | (0xF << 28)
                                                | (gBitTable[gActiveBattler] << 4)
                                                | (gBitTable[gActiveBattler] << 8)
                                                | (gBitTable[gActiveBattler] << 12))))
            {
                if (AllAtActionConfirmed())
                    i = TRUE;
                else
                    i = FALSE;

                if (((gBattleTypeFlags & BATTLE_TYPE_MULTI) || !(gBattleTypeFlags &  BATTLE_TYPE_DOUBLE))
                    || (position & BIT_FLANK) != B_FLANK_LEFT
                    || (*(&gBattleStruct->absentBattlerFlags) & gBitTable[GetBattlerAtPosition(BATTLE_PARTNER(position))]))
                {
                    BtlController_EmitLinkStandbyMsg(BUFFER_A, LINK_STANDBY_MSG_STOP_BOUNCE, i);
                }
                else
                {
                    BtlController_EmitLinkStandbyMsg(BUFFER_A, LINK_STANDBY_STOP_BOUNCE_ONLY, i);
                }
                MarkBattlerForControllerExec(gActiveBattler);
                gBattleCommunication[gActiveBattler]++;
            }
            break;
        case STATE_WAIT_ACTION_CONFIRMED:
            if (!(gBattleControllerExecFlags & ((gBitTable[gActiveBattler]) | (0xF << 28) | (gBitTable[gActiveBattler] << 4) | (gBitTable[gActiveBattler] << 8) | (gBitTable[gActiveBattler] << 12))))
            {
                gBattleCommunication[ACTIONS_CONFIRMED_COUNT]++;
            }
            break;
        case STATE_SELECTION_SCRIPT:
            if (*(gBattleStruct->selectionScriptFinished + gActiveBattler))
            {
                gBattleCommunication[gActiveBattler] = *(gBattleStruct->stateIdAfterSelScript + gActiveBattler);
            }
            else
            {
                gBattlerAttacker = gActiveBattler;
                gBattlescriptCurrInstr = gSelectionBattleScripts[gActiveBattler];
                if (!(gBattleControllerExecFlags & ((gBitTable[gActiveBattler]) | (0xF << 28) | (gBitTable[gActiveBattler] << 4) | (gBitTable[gActiveBattler] << 8) | (gBitTable[gActiveBattler] << 12))))
                {
                    gBattleScriptingCommandsTable[gBattlescriptCurrInstr[0]]();
                }
                gSelectionBattleScripts[gActiveBattler] = gBattlescriptCurrInstr;
            }
            break;
        case STATE_WAIT_SET_BEFORE_ACTION:
            if (!(gBattleControllerExecFlags & ((gBitTable[gActiveBattler]) | (0xF << 28) | (gBitTable[gActiveBattler] << 4) | (gBitTable[gActiveBattler] << 8) | (gBitTable[gActiveBattler] << 12))))
            {
                gBattleCommunication[gActiveBattler] = STATE_BEFORE_ACTION_CHOSEN;
            }
            break;
        case STATE_SELECTION_SCRIPT_MAY_RUN:
            if (*(gBattleStruct->selectionScriptFinished + gActiveBattler))
            {
                if (gBattleBufferB[gActiveBattler][1] == B_ACTION_NOTHING_FAINTED)
                {
                    gHitMarker |= HITMARKER_RUN;
                    gChosenActionByBattler[gActiveBattler] = B_ACTION_RUN;
                    gBattleCommunication[gActiveBattler] = STATE_WAIT_ACTION_CONFIRMED_STANDBY;
                }
                else
                {
                    RecordedBattle_ClearBattlerAction(gActiveBattler, 1);
                    gBattleCommunication[gActiveBattler] = *(gBattleStruct->stateIdAfterSelScript + gActiveBattler);
                }
            }
            else
            {
                gBattlerAttacker = gActiveBattler;
                gBattlescriptCurrInstr = gSelectionBattleScripts[gActiveBattler];
                if (!(gBattleControllerExecFlags & ((gBitTable[gActiveBattler]) | (0xF << 28) | (gBitTable[gActiveBattler] << 4) | (gBitTable[gActiveBattler] << 8) | (gBitTable[gActiveBattler] << 12))))
                {
                    gBattleScriptingCommandsTable[gBattlescriptCurrInstr[0]]();
                }
                gSelectionBattleScripts[gActiveBattler] = gBattlescriptCurrInstr;
            }
            break;
        }
    }

    // Check if everyone chose actions.
    if (gBattleCommunication[ACTIONS_CONFIRMED_COUNT] == gBattlersCount)
    {
        RecordedBattle_CheckMovesetChanges(B_RECORD_MODE_RECORDING);
        gBattleMainFunc = SetActionsAndBattlersTurnOrder;

        if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
        {
            for (i = 0; i < gBattlersCount; i++)
            {
                if (gChosenActionByBattler[i] == B_ACTION_SWITCH)
                    SwitchPartyOrderInGameMulti(i, *(gBattleStruct->monToSwitchIntoId + i));
            }
        }
    }
}

static bool8 AllAtActionConfirmed(void)
{
    s32 i, count;

    for (count = 0, i = 0; i < gBattlersCount; i++)
    {
        if (gBattleCommunication[i] == STATE_WAIT_ACTION_CONFIRMED)
            count++;
    }

    if (count + 1 == gBattlersCount)
        return TRUE;
    else
        return FALSE;
}

static void UpdateBattlerPartyOrdersOnSwitch(void)
{
    *(gBattleStruct->monToSwitchIntoId + gActiveBattler) = gBattleBufferB[gActiveBattler][1];
    RecordedBattle_SetBattlerAction(gActiveBattler, gBattleBufferB[gActiveBattler][1]);

    if (gBattleTypeFlags & BATTLE_TYPE_LINK && gBattleTypeFlags & BATTLE_TYPE_MULTI)
    {
        *(gActiveBattler * 3 + (u8 *)(gBattleStruct->battlerPartyOrders) + 0) &= 0xF;
        *(gActiveBattler * 3 + (u8 *)(gBattleStruct->battlerPartyOrders) + 0) |= (gBattleBufferB[gActiveBattler][2] & 0xF0);
        *(gActiveBattler * 3 + (u8 *)(gBattleStruct->battlerPartyOrders) + 1) = gBattleBufferB[gActiveBattler][3];

        *((BATTLE_PARTNER(gActiveBattler)) * 3 + (u8 *)(gBattleStruct->battlerPartyOrders) + 0) &= (0xF0);
        *((BATTLE_PARTNER(gActiveBattler)) * 3 + (u8 *)(gBattleStruct->battlerPartyOrders) + 0) |= (gBattleBufferB[gActiveBattler][2] & 0xF0) >> 4;
        *((BATTLE_PARTNER(gActiveBattler)) * 3 + (u8 *)(gBattleStruct->battlerPartyOrders) + 2) = gBattleBufferB[gActiveBattler][3];
    }
}

void SwapTurnOrder(u8 id1, u8 id2)
{
    u32 temp;

    SWAP(gActionsByTurnOrder[id1], gActionsByTurnOrder[id2], temp);
    SWAP(gBattlerByTurnOrder[id1], gBattlerByTurnOrder[id2], temp);
}

u8 GetWhoStrikesFirst(u8 battler1, u8 battler2, bool8 ignoreChosenMoves)
{
    u8 strikesFirst = 0;
    u8 speedMultiplierBattler1 = 0, speedMultiplierBattler2 = 0;
    u32 speedBattler1 = 0, speedBattler2 = 0;
    u8 holdEffect = 0;
    u8 holdEffectParam = 0;
    u16 moveBattler1 = 0, moveBattler2 = 0;

    if (WEATHER_HAS_EFFECT)
    {
        if ((gBattleMons[battler1].ability == ABILITY_SWIFT_SWIM && gBattleWeather & B_WEATHER_RAIN)
            || (gBattleMons[battler1].ability == ABILITY_CHLOROPHYLL && gBattleWeather & B_WEATHER_SUN))
            speedMultiplierBattler1 = 2;
        else
            speedMultiplierBattler1 = 1;

        if ((gBattleMons[battler2].ability == ABILITY_SWIFT_SWIM && gBattleWeather & B_WEATHER_RAIN)
            || (gBattleMons[battler2].ability == ABILITY_CHLOROPHYLL && gBattleWeather & B_WEATHER_SUN))
            speedMultiplierBattler2 = 2;
        else
            speedMultiplierBattler2 = 1;
    }
    else
    {
        speedMultiplierBattler1 = 1;
        speedMultiplierBattler2 = 1;
    }

    speedBattler1 = (gBattleMons[battler1].speed * speedMultiplierBattler1)
                * (gStatStageRatios[gBattleMons[battler1].statStages[STAT_SPEED]][0])
                / (gStatStageRatios[gBattleMons[battler1].statStages[STAT_SPEED]][1]);

    if (gBattleMons[battler1].item == ITEM_ENIGMA_BERRY)
    {
        holdEffect = gEnigmaBerries[battler1].holdEffect;
        holdEffectParam = gEnigmaBerries[battler1].holdEffectParam;
    }
    else
    {
        holdEffect = ItemId_GetHoldEffect(gBattleMons[battler1].item);
        holdEffectParam = ItemId_GetHoldEffectParam(gBattleMons[battler1].item);
    }

    // badge boost
    if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK | BATTLE_TYPE_FRONTIER))
        && FlagGet(FLAG_BADGE03_GET)
        && GetBattlerSide(battler1) == B_SIDE_PLAYER)
    {
        speedBattler1 = (speedBattler1 * 110) / 100;
    }

    if (holdEffect == HOLD_EFFECT_MACHO_BRACE)
        speedBattler1 /= 2;

    if (gBattleMons[battler1].status1 & STATUS1_PARALYSIS)
        speedBattler1 /= 4;

    if (holdEffect == HOLD_EFFECT_QUICK_CLAW && gRandomTurnNumber < (0xFFFF * holdEffectParam) / 100)
        speedBattler1 = UINT_MAX;

    // check second battlerId's speed

    speedBattler2 = (gBattleMons[battler2].speed * speedMultiplierBattler2)
                * (gStatStageRatios[gBattleMons[battler2].statStages[STAT_SPEED]][0])
                / (gStatStageRatios[gBattleMons[battler2].statStages[STAT_SPEED]][1]);

    if (gBattleMons[battler2].item == ITEM_ENIGMA_BERRY)
    {
        holdEffect = gEnigmaBerries[battler2].holdEffect;
        holdEffectParam = gEnigmaBerries[battler2].holdEffectParam;
    }
    else
    {
        holdEffect = ItemId_GetHoldEffect(gBattleMons[battler2].item);
        holdEffectParam = ItemId_GetHoldEffectParam(gBattleMons[battler2].item);
    }

    // badge boost
    if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK | BATTLE_TYPE_FRONTIER))
        && FlagGet(FLAG_BADGE03_GET)
        && GetBattlerSide(battler2) == B_SIDE_PLAYER)
    {
        speedBattler2 = (speedBattler2 * 110) / 100;
    }

    if (holdEffect == HOLD_EFFECT_MACHO_BRACE)
        speedBattler2 /= 2;

    if (gBattleMons[battler2].status1 & STATUS1_PARALYSIS)
        speedBattler2 /= 4;

    if (holdEffect == HOLD_EFFECT_QUICK_CLAW && gRandomTurnNumber < (0xFFFF * holdEffectParam) / 100)
        speedBattler2 = UINT_MAX;

    if (ignoreChosenMoves)
    {
        moveBattler1 = MOVE_NONE;
        moveBattler2 = MOVE_NONE;
    }
    else
    {
        if (gChosenActionByBattler[battler1] == B_ACTION_USE_MOVE)
        {
            if (gProtectStructs[battler1].noValidMoves)
                moveBattler1 = MOVE_STRUGGLE;
            else
                moveBattler1 = gBattleMons[battler1].moves[*(gBattleStruct->chosenMovePositions + battler1)];
        }
        else
        {
            moveBattler1 = MOVE_NONE;
        }

        if (gChosenActionByBattler[battler2] == B_ACTION_USE_MOVE)
        {
            if (gProtectStructs[battler2].noValidMoves)
                moveBattler2 = MOVE_STRUGGLE;
            else
                moveBattler2 = gBattleMons[battler2].moves[*(gBattleStruct->chosenMovePositions + battler2)];
        }
        else
        {
            moveBattler2 = MOVE_NONE;
        }
    }

    // both move priorities are different than 0
    if (gBattleMoves[moveBattler1].priority != 0 || gBattleMoves[moveBattler2].priority != 0)
    {
        // both priorities are the same
        if (gBattleMoves[moveBattler1].priority == gBattleMoves[moveBattler2].priority)
        {
            if (speedBattler1 == speedBattler2 && Random() & 1)
                strikesFirst = 2; // same speeds, same priorities
            else if (speedBattler1 < speedBattler2)
                strikesFirst = 1; // battler2 has more speed

            // else battler1 has more speed
        }
        else if (gBattleMoves[moveBattler1].priority < gBattleMoves[moveBattler2].priority)
        {
            strikesFirst = 1; // battler2's move has greater priority
        }

        // else battler1's move has greater priority
    }
    // both priorities are equal to 0
    else
    {
        if (speedBattler1 == speedBattler2 && Random() & 1)
            strikesFirst = 2; // same speeds, same priorities
        else if (speedBattler1 < speedBattler2)
            strikesFirst = 1; // battler2 has more speed

        // else battler1 has more speed
    }

    return strikesFirst;
}

static void SetActionsAndBattlersTurnOrder(void)
{
    s32 turnOrderId = 0;
    s32 i, j;

    if (gBattleTypeFlags & BATTLE_TYPE_SAFARI)
    {
        for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
        {
            gActionsByTurnOrder[turnOrderId] = gChosenActionByBattler[gActiveBattler];
            gBattlerByTurnOrder[turnOrderId] = gActiveBattler;
            turnOrderId++;
        }
    }
    else
    {
        if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        {
            for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
            {
                if (gChosenActionByBattler[gActiveBattler] == B_ACTION_RUN)
                {
                    turnOrderId = 5;
                    break;
                }
            }
        }
        else
        {
            if (gChosenActionByBattler[0] == B_ACTION_RUN)
            {
                gActiveBattler = 0;
                turnOrderId = 5;
            }
            if (gChosenActionByBattler[2] == B_ACTION_RUN)
            {
                gActiveBattler = 2;
                turnOrderId = 5;
            }
        }

        if (turnOrderId == 5) // One of battlers wants to run.
        {
            gActionsByTurnOrder[0] = gChosenActionByBattler[gActiveBattler];
            gBattlerByTurnOrder[0] = gActiveBattler;
            turnOrderId = 1;
            for (i = 0; i < gBattlersCount; i++)
            {
                if (i != gActiveBattler)
                {
                    gActionsByTurnOrder[turnOrderId] = gChosenActionByBattler[i];
                    gBattlerByTurnOrder[turnOrderId] = i;
                    turnOrderId++;
                }
            }
            gBattleMainFunc = CheckFocusPunch_ClearVarsBeforeTurnStarts;
            gBattleStruct->focusPunchBattlerId = 0;
            return;
        }
        else
        {
            for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
            {
                if (gChosenActionByBattler[gActiveBattler] == B_ACTION_USE_ITEM || gChosenActionByBattler[gActiveBattler] == B_ACTION_SWITCH)
                {
                    gActionsByTurnOrder[turnOrderId] = gChosenActionByBattler[gActiveBattler];
                    gBattlerByTurnOrder[turnOrderId] = gActiveBattler;
                    turnOrderId++;
                }
            }
            for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
            {
                if (gChosenActionByBattler[gActiveBattler] != B_ACTION_USE_ITEM && gChosenActionByBattler[gActiveBattler] != B_ACTION_SWITCH)
                {
                    gActionsByTurnOrder[turnOrderId] = gChosenActionByBattler[gActiveBattler];
                    gBattlerByTurnOrder[turnOrderId] = gActiveBattler;
                    turnOrderId++;
                }
            }
            for (i = 0; i < gBattlersCount - 1; i++)
            {
                for (j = i + 1; j < gBattlersCount; j++)
                {
                    u8 battler1 = gBattlerByTurnOrder[i];
                    u8 battler2 = gBattlerByTurnOrder[j];
                    if (gActionsByTurnOrder[i] != B_ACTION_USE_ITEM
                        && gActionsByTurnOrder[j] != B_ACTION_USE_ITEM
                        && gActionsByTurnOrder[i] != B_ACTION_SWITCH
                        && gActionsByTurnOrder[j] != B_ACTION_SWITCH)
                    {
                        if (GetWhoStrikesFirst(battler1, battler2, FALSE))
                            SwapTurnOrder(i, j);
                    }
                }
            }
        }
    }
    gBattleMainFunc = CheckFocusPunch_ClearVarsBeforeTurnStarts;
    gBattleStruct->focusPunchBattlerId = 0;
}

static void TurnValuesCleanUp(bool8 var0)
{
    s32 i;
    u8 *dataPtr;

    for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
    {
        if (var0)
        {
            gProtectStructs[gActiveBattler].protected = 0;
            gProtectStructs[gActiveBattler].endured = 0;
        }
        else
        {
            dataPtr = (u8 *)(&gProtectStructs[gActiveBattler]);
            for (i = 0; i < sizeof(struct ProtectStruct); i++)
                dataPtr[i] = 0;

            if (gDisableStructs[gActiveBattler].isFirstTurn)
                gDisableStructs[gActiveBattler].isFirstTurn--;

            if (gDisableStructs[gActiveBattler].rechargeTimer)
            {
                gDisableStructs[gActiveBattler].rechargeTimer--;
                if (gDisableStructs[gActiveBattler].rechargeTimer == 0)
                    gBattleMons[gActiveBattler].status2 &= ~STATUS2_RECHARGE;
            }
        }

        if (gDisableStructs[gActiveBattler].substituteHP == 0)
            gBattleMons[gActiveBattler].status2 &= ~STATUS2_SUBSTITUTE;
    }

    gSideTimers[0].followmeTimer = 0;
    gSideTimers[1].followmeTimer = 0;
}

void SpecialStatusesClear(void)
{
    for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
    {
        s32 i;
        u8 *dataPtr = (u8 *)(&gSpecialStatuses[gActiveBattler]);

        for (i = 0; i < sizeof(struct SpecialStatus); i++)
            dataPtr[i] = 0;
    }
}

static void CheckFocusPunch_ClearVarsBeforeTurnStarts(void)
{
    if (!(gHitMarker & HITMARKER_RUN))
    {
        while (gBattleStruct->focusPunchBattlerId < gBattlersCount)
        {
            gActiveBattler = gBattlerAttacker = gBattleStruct->focusPunchBattlerId;
            gBattleStruct->focusPunchBattlerId++;
            if (gChosenMoveByBattler[gActiveBattler] == MOVE_FOCUS_PUNCH
                && !(gBattleMons[gActiveBattler].status1 & STATUS1_SLEEP)
                && !(gDisableStructs[gBattlerAttacker].truantCounter)
                && !(gProtectStructs[gActiveBattler].noValidMoves))
            {
                BattleScriptExecute(BattleScript_FocusPunchSetUp);
                return;
            }
        }
    }

    TryClearRageStatuses();
    gCurrentTurnActionNumber = 0;
    gCurrentActionFuncId = gActionsByTurnOrder[gCurrentTurnActionNumber];
    gDynamicBasePower = 0;
    gBattleStruct->dynamicMoveType = 0;
    gBattleMainFunc = RunTurnActionsFunctions;
    gBattleCommunication[3] = 0;
    gBattleCommunication[4] = 0;
    gBattleScripting.multihitMoveEffect = 0;
    gBattleResources->battleScriptsStack->size = 0;
}

static void RunTurnActionsFunctions(void)
{
    if (gBattleOutcome != 0)
        gCurrentActionFuncId = B_ACTION_FINISHED;

    *(&gBattleStruct->savedTurnActionNumber) = gCurrentTurnActionNumber;
    sTurnActionsFuncsTable[gCurrentActionFuncId]();

    if (gCurrentTurnActionNumber >= gBattlersCount) // everyone did their actions, turn finished
    {
        gHitMarker &= ~HITMARKER_PASSIVE_DAMAGE;
        gBattleMainFunc = sEndTurnFuncsTable[gBattleOutcome & 0x7F];
    }
    else
    {
        if (gBattleStruct->savedTurnActionNumber != gCurrentTurnActionNumber) // action turn has been done, clear hitmarker bits for another battlerId
        {
            gHitMarker &= ~HITMARKER_NO_ATTACKSTRING;
            gHitMarker &= ~HITMARKER_UNABLE_TO_USE_MOVE;
        }
    }
}

static void HandleEndTurn_BattleWon(void)
{
    gCurrentActionFuncId = 0;

    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
    {
        gSpecialVar_Result = gBattleOutcome;
        gBattleTextBuff1[0] = gBattleOutcome;
        gBattlerAttacker = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        gBattlescriptCurrInstr = BattleScript_LinkBattleWonOrLost;
        gBattleOutcome &= ~B_OUTCOME_LINK_BATTLE_RAN;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER
            && gBattleTypeFlags & (BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_EREADER_TRAINER))
    {
        BattleStopLowHpSound();
        gBattlescriptCurrInstr = BattleScript_FrontierTrainerBattleWon;

        if (gTrainerBattleOpponent_A == TRAINER_FRONTIER_BRAIN)
            PlayBGM(MUS_VICTORY_GYM_LEADER);
        else
            PlayBGM(MUS_VICTORY_TRAINER);
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER && !(gBattleTypeFlags & BATTLE_TYPE_LINK))
    {
        BattleStopLowHpSound();
        gBattlescriptCurrInstr = BattleScript_LocalTrainerBattleWon;

        switch (gTrainers[gTrainerBattleOpponent_A].trainerClass)
        {
        case TRAINER_CLASS_ELITE_FOUR:
        case TRAINER_CLASS_CHAMPION:
            PlayBGM(MUS_VICTORY_LEAGUE);
            break;
        case TRAINER_CLASS_TEAM_AQUA:
        case TRAINER_CLASS_TEAM_MAGMA:
        case TRAINER_CLASS_AQUA_ADMIN:
        case TRAINER_CLASS_AQUA_LEADER:
        case TRAINER_CLASS_MAGMA_ADMIN:
        case TRAINER_CLASS_MAGMA_LEADER:
            PlayBGM(MUS_VICTORY_AQUA_MAGMA);
            break;
        case TRAINER_CLASS_LEADER:
            PlayBGM(MUS_VICTORY_GYM_LEADER);
            break;
        default:
            PlayBGM(MUS_VICTORY_TRAINER);
            break;
        }
    }
    else
    {
        gBattlescriptCurrInstr = BattleScript_PayDayMoneyAndPickUpItems;
    }

    gBattleMainFunc = HandleEndTurn_FinishBattle;
}

static void HandleEndTurn_BattleLost(void)
{
    gCurrentActionFuncId = 0;

    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
    {
        if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
        {
            if (gBattleOutcome & B_OUTCOME_LINK_BATTLE_RAN)
            {
                gBattlescriptCurrInstr = BattleScript_PrintPlayerForfeitedLinkBattle;
                gBattleOutcome &= ~B_OUTCOME_LINK_BATTLE_RAN;
                gSaveBlock2Ptr->frontier.disableRecordBattle = TRUE;
            }
            else
            {
                gBattlescriptCurrInstr = BattleScript_FrontierLinkBattleLost;
                gBattleOutcome &= ~B_OUTCOME_LINK_BATTLE_RAN;
            }
        }
        else
        {
            gBattleTextBuff1[0] = gBattleOutcome;
            gBattlerAttacker = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
            gBattlescriptCurrInstr = BattleScript_LinkBattleWonOrLost;
            gBattleOutcome &= ~B_OUTCOME_LINK_BATTLE_RAN;
        }
    }
    else
    {
        gBattlescriptCurrInstr = BattleScript_LocalBattleLost;
    }

    gBattleMainFunc = HandleEndTurn_FinishBattle;
}

static void HandleEndTurn_RanFromBattle(void)
{
    gCurrentActionFuncId = 0;

    if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER && gBattleTypeFlags & BATTLE_TYPE_TRAINER)
    {
        gBattlescriptCurrInstr = BattleScript_PrintPlayerForfeited;
        gBattleOutcome = B_OUTCOME_FORFEITED;
        gSaveBlock2Ptr->frontier.disableRecordBattle = TRUE;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_HILL)
    {
        gBattlescriptCurrInstr = BattleScript_PrintPlayerForfeited;
        gBattleOutcome = B_OUTCOME_FORFEITED;
    }
    else
    {
        switch (gProtectStructs[gBattlerAttacker].fleeType)
        {
        default:
            gBattlescriptCurrInstr = BattleScript_GotAwaySafely;
            break;
        case FLEE_ITEM:
            gBattlescriptCurrInstr = BattleScript_SmokeBallEscape;
            break;
        case FLEE_ABILITY:
            gBattlescriptCurrInstr = BattleScript_RanAwayUsingMonAbility;
            break;
        }
    }

    gBattleMainFunc = HandleEndTurn_FinishBattle;
}

static void HandleEndTurn_MonFled(void)
{
    gCurrentActionFuncId = 0;

    PREPARE_MON_NICK_BUFFER(gBattleTextBuff1, gBattlerAttacker, gBattlerPartyIndexes[gBattlerAttacker]);
    gBattlescriptCurrInstr = BattleScript_WildMonFled;

    gBattleMainFunc = HandleEndTurn_FinishBattle;
}

static void HandleEndTurn_FinishBattle(void)
{
    if (gCurrentActionFuncId == B_ACTION_TRY_FINISH || gCurrentActionFuncId == B_ACTION_FINISHED)
    {
        if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK
                                  | BATTLE_TYPE_RECORDED_LINK
                                  | BATTLE_TYPE_FIRST_BATTLE
                                  | BATTLE_TYPE_SAFARI
                                  | BATTLE_TYPE_EREADER_TRAINER
                                  | BATTLE_TYPE_WALLY_TUTORIAL
                                  | BATTLE_TYPE_FRONTIER)))
        {
            for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
            {
                if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
                {
                    if (gBattleResults.playerMon1Species == SPECIES_NONE)
                    {
                        gBattleResults.playerMon1Species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_SPECIES, NULL);
                        GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_NICKNAME, gBattleResults.playerMon1Name);
                    }
                    else
                    {
                        gBattleResults.playerMon2Species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_SPECIES, NULL);
                        GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_NICKNAME, gBattleResults.playerMon2Name);
                    }
                }
            }
            TryPutPokemonTodayOnAir();
        }

        if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK
                                  | BATTLE_TYPE_RECORDED_LINK
                                  | BATTLE_TYPE_TRAINER
                                  | BATTLE_TYPE_FIRST_BATTLE
                                  | BATTLE_TYPE_SAFARI
                                  | BATTLE_TYPE_FRONTIER
                                  | BATTLE_TYPE_EREADER_TRAINER
                                  | BATTLE_TYPE_WALLY_TUTORIAL))
            && gBattleResults.shinyWildMon)
        {
            TryPutBreakingNewsOnAir();
        }

        RecordedBattle_SetPlaybackFinished();
        BeginFastPaletteFade(3);
        FadeOutMapMusic(5);
        gBattleMainFunc = FreeResetData_ReturnToOvOrDoEvolutions;
        gCB2_AfterEvolution = BattleMainCB2;
    }
    else
    {
        if (gBattleControllerExecFlags == 0)
            gBattleScriptingCommandsTable[gBattlescriptCurrInstr[0]]();
    }
}

static void FreeResetData_ReturnToOvOrDoEvolutions(void)
{
    if (!gPaletteFade.active)
    {
        ResetSpriteData();
        if (gLeveledUpInBattle == 0 || gBattleOutcome != B_OUTCOME_WON)
        {
            gBattleMainFunc = ReturnFromBattleToOverworld;
            return;
        }
        else
        {
            gBattleMainFunc = TryEvolvePokemon;
        }
    }

    FreeAllWindowBuffers();
    if (!(gBattleTypeFlags & BATTLE_TYPE_LINK))
    {
        FreeMonSpritesGfx();
        FreeBattleResources();
        FreeBattleSpritesData();
    }
}

static void TryEvolvePokemon(void)
{
    s32 i;

    while (gLeveledUpInBattle != 0)
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (gLeveledUpInBattle & gBitTable[i])
            {
                u16 species;
                u8 levelUpBits = gLeveledUpInBattle;

                levelUpBits &= ~(gBitTable[i]);
                gLeveledUpInBattle = levelUpBits;

                species = GetEvolutionTargetSpecies(&gPlayerParty[i], EVO_MODE_NORMAL, levelUpBits);
                if (species != SPECIES_NONE)
                {
                    FreeAllWindowBuffers();
                    gBattleMainFunc = WaitForEvoSceneToFinish;
                    EvolutionScene(&gPlayerParty[i], species, TRUE, i);
                    return;
                }
            }
        }
    }

    gBattleMainFunc = ReturnFromBattleToOverworld;
}

static void WaitForEvoSceneToFinish(void)
{
    if (gMain.callback2 == BattleMainCB2)
        gBattleMainFunc = TryEvolvePokemon;
}

static void ReturnFromBattleToOverworld(void)
{
    if (!(gBattleTypeFlags & BATTLE_TYPE_LINK))
    {
        RandomlyGivePartyPokerus(gPlayerParty);
        PartySpreadPokerus(gPlayerParty);
    }

    if (gBattleTypeFlags & BATTLE_TYPE_LINK && gReceivedRemoteLinkPlayers)
        return;

    gSpecialVar_Result = gBattleOutcome;
    gMain.inBattle = FALSE;
    gMain.callback1 = gPreBattleCallback1;

    if (gBattleTypeFlags & BATTLE_TYPE_ROAMER)
    {
        UpdateRoamerHPStatus(&gEnemyParty[0]);

#ifndef BUGFIX
        if ((gBattleOutcome & B_OUTCOME_WON) || gBattleOutcome == B_OUTCOME_CAUGHT)
#else
        if ((gBattleOutcome == B_OUTCOME_WON) || gBattleOutcome == B_OUTCOME_CAUGHT) // Bug: When Roar is used by roamer, gBattleOutcome is B_OUTCOME_PLAYER_TELEPORTED (5).
#endif                                                                               // & with B_OUTCOME_WON (1) will return TRUE and deactivates the roamer.
            SetRoamerInactive();
    }

    m4aSongNumStop(SE_LOW_HEALTH);
    SetMainCallback2(gMain.savedCallback);
}

void RunBattleScriptCommands_PopCallbacksStack(void)
{
    if (gCurrentActionFuncId == B_ACTION_TRY_FINISH || gCurrentActionFuncId == B_ACTION_FINISHED)
    {
        if (gBattleResources->battleCallbackStack->size != 0)
            gBattleResources->battleCallbackStack->size--;
        gBattleMainFunc = gBattleResources->battleCallbackStack->function[gBattleResources->battleCallbackStack->size];
    }
    else
    {
        if (gBattleControllerExecFlags == 0)
            gBattleScriptingCommandsTable[gBattlescriptCurrInstr[0]]();
    }
}

void RunBattleScriptCommands(void)
{
    if (gBattleControllerExecFlags == 0)
        gBattleScriptingCommandsTable[gBattlescriptCurrInstr[0]]();
}
