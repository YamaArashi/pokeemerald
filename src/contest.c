#include "global.h"
#include "gpu_regs.h"
#include "bg.h"
#include "malloc.h"
#include "battle.h"
#include "battle_anim.h"
#include "contest.h"
#include "contest_link.h"
#include "data.h"
#include "decompress.h"
#include "graphics.h"
#include "link.h"
#include "m4a.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "random.h"
#include "new_game.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "tv.h"
#include "scanline_effect.h"
#include "util.h"
#include "contest_util.h"
#include "dma3.h"
#include "battle_message.h"
#include "event_scripts.h"
#include "event_data.h"
#include "strings.h"
#include "contest_effect.h"
#include "contest_link.h"
#include "international_string_util.h"
#include "data.h"
#include "contest_ai.h"
#include "constants/event_objects.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// This file's functions.
static void LoadContestPalettes(void);
static void Task_StartContestWaitFade(u8 taskId);
static void Task_TryStartLinkContest(u8 taskId);
static void Task_CommunicateMonIdxs(u8 taskId);
static void Task_EndCommunicateMonIdxs(u8 taskId);
static void Task_ReadyStartLinkContest(u8 taskId);
static bool8 SetupContestGraphics(u8 *stateVar);
static void Task_WaitToRaiseCurtainAtStart(u8 taskId);
static void Task_RaiseCurtainAtStart(u8 taskId);
static void VBlankCB_Contest(void);
static void CB2_ContestMain(void);
static void Task_DisplayAppealNumberText(u8 taskId);
static void Task_TryShowMoveSelectScreen(u8 taskId);
static void Task_ShowMoveSelectScreen(u8 taskId);
static void Task_HandleMoveSelectInput(u8 taskId);
static void DrawMoveSelectArrow(s8);
static void EraseMoveSelectArrow(s8);
static void Task_SelectedMove(u8 taskId);
static void Task_EndCommunicateMoveSelections(u8 taskId);
static void Task_HideMoveSelectScreen(u8 taskId);
static void Task_HideApplauseMeterForAppealStart(u8 taskId);
static void Task_WaitHideApplauseMeterForAppealStart(u8 taskId);
static void Task_AppealSetup(u8 taskId);
static void Task_DoAppeals(u8 taskId);
static void Task_EndWaitForLink(u8);
static void SpriteCB_MonSlideIn(struct Sprite *);
static void SpriteCB_MonSlideOut(struct Sprite *);
static void Task_FinishRoundOfAppeals(u8);
static void Task_ReadyUpdateHeartSliders(u8);
static void Task_UpdateHeartSliders(u8);
static void Task_WaitForHeartSliders(u8);
static void Task_RestorePlttBufferUnfaded(u8);
static void Task_WaitPrintRoundResult(u8);
static void Task_PrintRoundResultText(u8);
static void Task_ReUpdateHeartSliders(u8);
static void Task_WaitForHeartSlidersAgain(u8);
static void Task_DropCurtainAtRoundEnd(u8);
static void Task_TryStartNextRoundOfAppeals(u8);
static void Task_StartNewRoundOfAppeals(u8);
static void Task_EndAppeals(u8);
static void Task_WaitForOutOfTimeMsg(u8);
static void Task_DropCurtainAtAppealsEnd(u8);
static void Task_TryCommunicateFinalStandings(u8);
static void Task_CommunicateFinalStandings(u8);
static void Task_EndCommunicateFinalStandings(u8);
static void Task_ContestReturnToField(u8);
static void FieldCB_ContestReturnToField(void);
static bool8 IsPlayerLinkLeader(void);
static void PrintContestantTrainerName(u8);
static void PrintContestantTrainerNameWithColor(u8, u8);
static void PrintContestantMonName(u8);
static void PrintContestantMonNameWithColor(u8, u8);
static u8 CreateJudgeSprite(void);
static u8 CreateJudgeSpeechBubbleSprite(void);
static u8 CreateContestantSprite(u16, u32, u32, u32);
static void PrintContestMoveDescription(u16);
static u16 SanitizeSpecies(u16);
static void ContestClearGeneralTextWindow(void);
static u16 GetChosenMove(u8);
static void GetAllChosenMoves(void);
static void ContestPrintLinkStandby(void);
static void FillContestantWindowBgs(void);
static void CreateSliderHeartSprites(void);
static void SetBottomSliderHeartsInvisibility(bool8);
static void CreateNextTurnSprites(void);
static void CreateApplauseMeterSprite(void);
static void CreateJudgeAttentionEyeTask(void);
static void CreateUnusedBlendTask(void);
static void ContestDebugDoPrint(void);
static void DrawContestantWindows(void);
static void ApplyNextTurnOrder(void);
static void SlideApplauseMeterIn(void);
static void SlideApplauseMeterOut(void);
static void SetBgForCurtainDrop(void);
static void UpdateContestantBoxOrder(void);
static void Task_StartDropCurtainAtRoundEnd(u8);
static void AnimateSliderHearts(u8);
static void CreateInvisibleBattleTargetSprite(void);
static void Contest_StartTextPrinter(const u8 *, u32);
static void ContestBG_FillBoxWithIncrementingTile(u8, u16, u8, u8, u8, u8, u8, s16);
static bool32 Contest_RunTextPrinters(void);
static void Contest_SetBgCopyFlags(u32 flagIndex);
static void CalculateFinalScores(void);
static void CalculateAppealMoveImpact(u8);
static void SetMoveAnimAttackerData(u8);
static void BlinkContestantBox(u8, u8);
static u8 CreateContestantBoxBlinkSprites(u8);
static u16 SanitizeMove(u16);
static void SetMoveSpecificAnimData(u8);
static void SetMoveTargetPosition(u16);
static void ClearMoveAnimData(u8);
static void StopFlashJudgeAttentionEye(u8);
static void DrawUnnervedSymbols(void);
static void PrintAppealMoveResultText(u8, u8);
static void DoJudgeSpeechBubble(u8);
static void ShowHideNextTurnGfx(bool8);
static u8 UpdateAppealHearts(s16, s16, u8);
static bool8 UpdateConditionStars(u8, u8);
static bool8 DrawStatusSymbol(u8);
static void DrawStatusSymbols(void);
static void StartStopFlashJudgeAttentionEye(u8);
static void BlendAudienceBackground(s8, s8);
static void ShowAndUpdateApplauseMeter(s8 unused);
static void AnimateAudience(void);
static void UpdateApplauseMeter(void);
static void RankContestants(void);
static void SetAttentionLevels(void);
static void UpdateHeartSliders(void);
static bool8 SlidersDoneUpdating(void);
static void ContestBG_FillBoxWithTile(u8, u16, u8, u8, u8, u8, u8);
static void Contest_PrintTextToBg0WindowStd(u32, const u8 *);
static s16 GetContestantRound2Points(u8);
static void DetermineFinalStandings(void);
static bool8 DidContestantPlaceHigher(s32, s32, struct ContestFinalStandings *);
static void Task_UpdateAppealHearts(u8);
static void SpriteCB_UpdateHeartSlider(struct Sprite *);
static void Task_FlashJudgeAttentionEye(u8);
static void Task_StopFlashJudgeAttentionEye(u8);
static void Task_UnusedBlend(u8);
static void InitUnusedBlendTaskData(u8);
static void UpdateBlendTaskContestantData(u8);
static void SpriteCB_BlinkContestantBox(struct Sprite *);
static void SpriteCB_EndBlinkContestantBox(struct Sprite *sprite);
static u8 StartApplauseOverflowAnimation(void);
static void Task_ApplauseOverflowAnimation(u8);
static void Task_SlideApplauseMeterIn(u8);
static void Task_SlideApplauseMeterOut(u8);
static void Task_ShowAndUpdateApplauseMeter(u8);
static void Task_AnimateAudience(u8);
static void Task_BlendAudienceBackground(u8);
static const u8 *GetTurnOrderNumberGfx(u8);
static void Task_UpdateCurtainDropAtRoundEnd(u8);
static void Task_ResetForNextRound(u8);
static void Task_WaitRaiseCurtainAtRoundEnd(u8);
static void Task_StartRaiseCurtainAtRoundEnd(u8);
static void Task_WaitForSliderHeartAnim(u8);
static void SetBattleTargetSpritePosition(void);
static void CalculateContestLiveUpdateData(void);
static void SetConestLiveUpdateTVData(void);
static void SetContestLiveUpdateFlags(u8);
static void ContestDebugPrintBitStrings(void);
static void StripPlayerNameForLinkContest(u8 *);
static void StripMonNameForLinkContest(u8 *, s32);
static void SwapMoveDescAndContestTilemaps(void);

// An index into a palette where the text color for each contestant is stored.
// Contestant 0 will use palette color 10, contestant 1 will use color 11, etc.
#define CONTESTANT_TEXT_COLOR_START 10

enum {
// The "{Pokémon Name} / {Trainer Name}" windows.
    WIN_CONTESTANT0_NAME,
    WIN_CONTESTANT1_NAME,
    WIN_CONTESTANT2_NAME,
    WIN_CONTESTANT3_NAME,
    WIN_GENERAL_TEXT,
    // The available moves, from top to bottom
    WIN_MOVE0,
    WIN_MOVE1,
    WIN_MOVE2,
    WIN_MOVE3,
    // The small "/" character between the move category and the
    // appeal/jam display
    WIN_SLASH,
    WIN_MOVE_DESCRIPTION
};

enum {
    JUDGE_SYMBOL_SWIRL,
    JUDGE_SYMBOL_SWIRL_UNUSED,
    JUDGE_SYMBOL_ONE_EXCLAMATION,
    JUDGE_SYMBOL_TWO_EXCLAMATIONS,
    JUDGE_SYMBOL_NUMBER_ONE_UNUSED,
    JUDGE_SYMBOL_NUMBER_ONE,
    JUDGE_SYMBOL_NUMBER_FOUR,
    JUDGE_SYMBOL_QUESTION_MARK,
    JUDGE_SYMBOL_STAR,
};

enum {
    STAT_SYMBOL_CIRCLE,
    STAT_SYMBOL_WAVE,
    STAT_SYMBOL_X,
    STAT_SYMBOL_SWIRL,
    STAT_SYMBOL_SQUARE,
};

enum {
    CONTEST_DEBUG_MODE_OFF,
    CONTEST_DEBUG_MODE_PRINT_POINT_TOTAL,
    CONTEST_DEBUG_MODE_PRINT_WINNER_FLAGS,
    CONTEST_DEBUG_MODE_PRINT_LOSER_FLAGS
};

#define MOVE_WINDOWS_START WIN_MOVE0

#define TAG_CONTEST_SYMBOLS_PAL 0xABE0
#define TAG_JUDGE_SYMBOLS_GFX   0xABE0
#define TAG_FACES_GFX           0xABE1
#define TAG_APPLAUSE_METER      0xABE2

#define TAG_SLIDER_HEART    0x4E20
#define TAG_JUDGE           0x4E21
#define TAG_NEXT_TURN_PAL   0x4E22
#define TAG_NEXT_TURN_1_GFX 0x4E22
#define TAG_NEXT_TURN_2_GFX 0x4E23
#define TAG_NEXT_TURN_3_GFX 0x4E24
#define TAG_NEXT_TURN_4_GFX 0x4E25

#define TAG_BLINK_EFFECT_CONTESTANT0 0x80E8
#define TAG_BLINK_EFFECT_CONTESTANT1 0x80E9
#define TAG_BLINK_EFFECT_CONTESTANT2 0x80EA
#define TAG_BLINK_EFFECT_CONTESTANT3 0x80EB

#define TILE_FILLED_APPEAL_HEART 0x5012
#define TILE_FILLED_JAM_HEART    0x5014
#define TILE_EMPTY_APPEAL_HEART  0x5035
#define TILE_EMPTY_JAM_HEART     0x5036

enum {
    SLIDER_HEART_ANIM_NORMAL,
    SLIDER_HEART_ANIM_DISAPPEAR,
    SLIDER_HEART_ANIM_APPEAR,
};

// States for Task_DoAppeals
enum {
    APPEALSTATE_START_TURN,
    APPEALSTATE_WAIT_LINK,
    APPEALSTATE_CHECK_SKIP_TURN,
    APPEALSTATE_SLIDE_MON_IN,
    APPEALSTATE_WAIT_SLIDE_MON,
    APPEALSTATE_PRINT_USED_MOVE_MSG,
    APPEALSTATE_WAIT_USED_MOVE_MSG,
    APPEALSTATE_MOVE_ANIM,
    APPEALSTATE_WAIT_MOVE_ANIM,
    APPEALSTATE_MOVE_ANIM_MULTITURN,
    APPEALSTATE_SLIDE_MON_OUT,
    APPEALSTATE_FREE_MON_SPRITE,
    APPEALSTATE_UPDATE_MOVE_USERS_HEARTS,
    APPEALSTATE_WAIT_MOVE_USERS_HEARTS,
    APPEALSTATE_PRINT_COMBO_MSG,
    APPEALSTATE_TRY_UPDATE_HEARTS_FROM_COMBO,
    APPEALSTATE_WAIT_HEARTS_FROM_COMBO,
    APPEALSTATE_CHECK_REPEATED_MOVE,
    APPEALSTATE_WAIT_HEARTS_FROM_REPEAT,
    APPEALSTATE_UPDATE_HEARTS_FROM_REPEAT,
    APPEALSTATE_START_TURN_END_DELAY,
    APPEALSTATE_TURN_END_DELAY,
    APPEALSTATE_START_NEXT_TURN,
    APPEALSTATE_TRY_PRINT_MOVE_RESULT,
    APPEALSTATE_WAIT_MOVE_RESULT_MSG,
    APPEALSTATE_UPDATE_OPPONENTS,
    APPEALSTATE_UPDATE_OPPONENT,
    APPEALSTATE_WAIT_OPPONENT_RESPONSE_MSG,
    APPEALSTATE_UPDATE_OPPONENT_HEARTS,
    APPEALSTATE_WAIT_OPPONENT_HEARTS,
    APPEALSTATE_UPDATE_OPPONENT_STATUS,
    APPEALSTATE_PRINT_SKIP_TURN_MSG,
    APPEALSTATE_WAIT_SKIP_TURN_MSG,
    APPEALSTATE_PRINT_TOO_NERVOUS_MSG,
    APPEALSTATE_WAIT_TOO_NERVOUS_MSG,
    APPEALSTATE_TRY_JUDGE_STAR,
    APPEALSTATE_WAIT_JUDGE_STAR,
    APPEALSTATE_UPDATE_MOVE_USERS_STARS,
    APPEALSTATE_WAIT_MOVE_USERS_STARS,
    APPEALSTATE_UPDATE_OPPONENT_STARS,
    APPEALSTATE_WAIT_OPPONENT_STARS,
    APPEALSTATE_UPDATE_CROWD,
    APPEALSTATE_42, // Unused state
    APPEALSTATE_WAIT_EXCITEMENT_HEARTS,
    APPEALSTATE_44, // Unused state
    APPEALSTATE_WAIT_JUDGE_COMBO,
    APPEALSTATE_WAIT_JUDGE_REPEATED_MOVE,
    APPEALSTATE_TRY_SHOW_NEXT_TURN_GFX,
    APPEALSTATE_CHECK_TURN_ORDER_MOD,
    APPEALSTATE_WAIT_JUDGE_TURN_ORDER,
    APPEALSTATE_UPDATE_MOVE_USERS_STATUS,
    APPEALSTATE_TRY_PRINT_SKIP_NEXT_TURN_MSG,
    APPEALSTATE_WAIT_SKIP_NEXT_TURN_MSG,
    APPEALSTATE_DO_CROWD_UNEXCITED,
    APPEALSTATE_DO_CROWD_EXCITED,
    APPEALSTATE_SLIDE_APPLAUSE_OUT,
    APPEALSTATE_WAIT_SLIDE_APPLAUSE,
    APPEALSTATE_PRINT_CROWD_WATCHES_MSG,
    APPEALSTATE_PRINT_MON_MOVE_IGNORED_MSG,
    APPEALSTATE_WAIT_MON_MOVE_IGNORED_MSG,
};

// EWRAM vars.
EWRAM_DATA struct ContestPokemon gContestMons[CONTESTANT_COUNT] = {0};
EWRAM_DATA s16 gContestMonRound1Points[CONTESTANT_COUNT] = {0}; // "Round 1" points are based on condition
EWRAM_DATA s16 gContestMonTotalPoints[CONTESTANT_COUNT] = {0}; // Round 1 points + Round 2 points
EWRAM_DATA s16 gContestMonAppealPointTotals[CONTESTANT_COUNT] = {0};
EWRAM_DATA s16 gContestMonRound2Points[CONTESTANT_COUNT] = {0}; // "Round 2" points are just appeal points * 2
EWRAM_DATA u8 gContestFinalStandings[CONTESTANT_COUNT] = {0};
EWRAM_DATA u8 gContestMonPartyIndex = 0;
EWRAM_DATA u8 gContestPlayerMonIndex = 0;
EWRAM_DATA u8 gContestantTurnOrder[CONTESTANT_COUNT] = {0};
EWRAM_DATA u8 gLinkContestFlags = 0;
// Bit 0: Is a link contest
// Bit 1: Link contest uses wireless adapter
EWRAM_DATA u8 gContestLinkLeaderIndex = 0;
EWRAM_DATA u16 gSpecialVar_ContestCategory = 0;
EWRAM_DATA u16 gSpecialVar_ContestRank = 0;
EWRAM_DATA u8 gNumLinkContestPlayers = 0;
EWRAM_DATA u8 gHighestRibbonRank = 0;
EWRAM_DATA struct ContestResources *gContestResources = NULL;
static EWRAM_DATA u8 sContestBgCopyFlags = 0;
EWRAM_DATA struct ContestWinner gCurContestWinner = {0};
EWRAM_DATA bool8 gCurContestWinnerIsForArtist = 0;
EWRAM_DATA u8 gCurContestWinnerSaveIdx = 0;

// IWRAM common vars.
COMMON_DATA u32 gContestRngValue = 0;

extern const u8 gText_LinkStandby4[];
extern const u8 gText_BDot[];
extern const u8 gText_CDot[];
extern void (*const gContestEffectFuncs[])(void);

static const u8 sSliderHeartYPositions[CONTESTANT_COUNT] =
{
    36, 76, 116, 156
};

// The "Next Turn" sprites cover up the slider, so the y positions are the same as above
static const u8 sNextTurnSpriteYPositions[CONTESTANT_COUNT] =
{
    36, 76, 116, 156
};

static const struct SpriteSheet sSpriteSheet_SliderHeart =
{
    .data = gContestSliderHeart_Gfx,
    .size = 0x20,
    .tag = TAG_SLIDER_HEART
};

static const struct OamData sOam_SliderHeart =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0
};

static const union AffineAnimCmd sAffineAnim_SliderHeart_Normal[] =
{
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_SliderHeart_SpinDisappear[] =
{
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    AFFINEANIMCMD_FRAME(-10, -10, -20, 20),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_SliderHeart_SpinAppear[] =
{
    AFFINEANIMCMD_FRAME(56, 56, 0, 0),
    AFFINEANIMCMD_FRAME(10, 10, 20, 20),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd* const sAffineAnims_SliderHeart[] =
{
    [SLIDER_HEART_ANIM_NORMAL]    = sAffineAnim_SliderHeart_Normal,
    [SLIDER_HEART_ANIM_DISAPPEAR] = sAffineAnim_SliderHeart_SpinDisappear,
    [SLIDER_HEART_ANIM_APPEAR]    = sAffineAnim_SliderHeart_SpinAppear
};

static const struct SpriteTemplate sSpriteTemplate_SliderHeart =
{
    .tileTag = TAG_SLIDER_HEART,
    .paletteTag = TAG_CONTEST_SYMBOLS_PAL,
    .oam = &sOam_SliderHeart,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_SliderHeart,
    .callback = SpriteCallbackDummy
};

static const struct CompressedSpriteSheet sSpriteSheet_NextTurn[CONTESTANT_COUNT] =
{
    {
        .data = gContestNextTurnGfx,
        .size = 0x100,
        .tag = TAG_NEXT_TURN_1_GFX
    },
    {
        .data = gContestNextTurnGfx,
        .size = 0x100,
        .tag = TAG_NEXT_TURN_2_GFX
    },
    {
        .data = gContestNextTurnGfx,
        .size = 0x100,
        .tag = TAG_NEXT_TURN_3_GFX
    },
    {
        .data = gContestNextTurnGfx,
        .size = 0x100,
        .tag = TAG_NEXT_TURN_4_GFX
    }
};

static const struct SpritePalette sSpritePalette_NextTurn =
{
    .data = gContestPal,
    .tag = TAG_NEXT_TURN_PAL
};

static const struct OamData sOam_NextTurn =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0
};

static const struct SpriteTemplate sSpriteTemplates_NextTurn[CONTESTANT_COUNT] =
{
    {
        .tileTag = TAG_NEXT_TURN_1_GFX,
        .paletteTag = TAG_NEXT_TURN_PAL,
        .oam = &sOam_NextTurn,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    },
    {
        .tileTag = TAG_NEXT_TURN_2_GFX,
        .paletteTag = TAG_NEXT_TURN_PAL,
        .oam = &sOam_NextTurn,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    },
    {
        .tileTag = TAG_NEXT_TURN_3_GFX,
        .paletteTag = TAG_NEXT_TURN_PAL,
        .oam = &sOam_NextTurn,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    },
    {
        .tileTag = TAG_NEXT_TURN_4_GFX,
        .paletteTag = TAG_NEXT_TURN_PAL,
        .oam = &sOam_NextTurn,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    }
};

/*    v-- Origin
[0    +][1     ]   64x8
*/
static const struct Subsprite sSubsprites_NextTurn[] =
{
    {
        .x = -28,
        .y = -4,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 0,
        .priority = 0
    },
    {
        .x = 4,
        .y = -4,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 4,
        .priority = 0
    }
};

static const struct SubspriteTable sSubspriteTable_NextTurn[] =
{
    {
        .subspriteCount = ARRAY_COUNT(sSubsprites_NextTurn),
        .subsprites = sSubsprites_NextTurn
    }
};

// Unused
static const struct CompressedSpriteSheet sSpriteSheet_Faces =
{
    .data = gContestFaces_Gfx,
    .size = 0x180,
    .tag = TAG_FACES_GFX
};

static const struct OamData sOam_Faces =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
};

// Unused
static const struct SpriteTemplate sSpriteTemplate_Faces =
{
    .tileTag = TAG_FACES_GFX,
    .paletteTag = TAG_CONTEST_SYMBOLS_PAL,
    .oam = &sOam_Faces,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct CompressedSpriteSheet sSpriteSheet_ApplauseMeter =
{
    .data = gContestApplauseGfx,
    .size = 0x400,
    .tag = TAG_APPLAUSE_METER
};

static const struct SpritePalette sSpritePalette_ApplauseMeter =
{
    .data = gContestPal,
    .tag = TAG_APPLAUSE_METER
};

static const struct OamData sOam_ApplauseMeter =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .x = 0,
    .size = SPRITE_SIZE(64x32),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
};

static const struct SpriteTemplate sSpriteTemplate_ApplauseMeter =
{
    .tileTag = TAG_APPLAUSE_METER,
    .paletteTag = TAG_APPLAUSE_METER,
    .oam = &sOam_ApplauseMeter,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct OamData sOam_Judge =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 3,
    .paletteNum = 2,
};

static const struct SpriteTemplate sSpriteTemplate_Judge =
{
    .tileTag = TAG_JUDGE,
    .paletteTag = TAG_JUDGE,
    .oam = &sOam_Judge,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct CompressedSpriteSheet sSpriteSheet_Judge =
{
    .data = gContestJudgeGfx,
    .size = 0x800,
    .tag = TAG_JUDGE
};

static const struct CompressedSpriteSheet sSpriteSheet_JudgeSymbols =
{
    .data = gContestJudgeSymbolsGfx,
    .size = 0x380,
    .tag = TAG_JUDGE_SYMBOLS_GFX
};

static const struct CompressedSpritePalette sSpritePalette_JudgeSymbols =
{
    .data = gContestJudgeSymbolsPal,
    .tag = TAG_CONTEST_SYMBOLS_PAL
};

static const struct SpriteTemplate sSpriteTemplate_JudgeSpeechBubble =
{
    .tileTag = TAG_JUDGE_SYMBOLS_GFX,
    .paletteTag = TAG_CONTEST_SYMBOLS_PAL,
    .oam = &gOamData_AffineOff_ObjNormal_16x16,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const u16 sText_Pal[] = INCBIN_U16("graphics/contest/text.gbapal");

#include "data/contest_text_tables.h"

static const struct BgTemplate sContestBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 0x18,
        .screenSize = 2,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 2,
        .mapBaseIndex = 0x1E,
        .screenSize = 2,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 0x1C,
        .screenSize = 2,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 0x1A,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    }
};

static const struct WindowTemplate sContestWindowTemplates[] =
{
    [WIN_CONTESTANT0_NAME] = {
        .bg = 0,
        .tilemapLeft = 18,
        .tilemapTop = 0,
        .width = 12,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x200
    },
    [WIN_CONTESTANT1_NAME] = {
        .bg = 0,
        .tilemapLeft = 18,
        .tilemapTop = 5,
        .width = 12,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x218
    },
    [WIN_CONTESTANT2_NAME] = {
        .bg = 0,
        .tilemapLeft = 18,
        .tilemapTop = 10,
        .width = 12,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x230
    },
    [WIN_CONTESTANT3_NAME] = {
        .bg = 0,
        .tilemapLeft = 18,
        .tilemapTop = 15,
        .width = 12,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x248
    },
    [WIN_GENERAL_TEXT] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 15,
        .width = 17,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x260
    },
    [WIN_MOVE0] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 31,
        .width = 9,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x2A4
    },
    [WIN_MOVE1] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 33,
        .width = 9,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x2B6
    },
    [WIN_MOVE2] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 35,
        .width = 9,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x2C8
    },
    [WIN_MOVE3] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 37,
        .width = 9,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x2DA
    },
    [WIN_SLASH] = {
        .bg = 0,
        .tilemapLeft = 16,
        .tilemapTop = 31,
        .width = 1,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x2EC
    },
    [WIN_MOVE_DESCRIPTION] = {
        .bg = 0,
        .tilemapLeft = 11,
        .tilemapTop = 35,
        .width = 18,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x2EE
    },
    DUMMY_WIN_TEMPLATE
};

#include "data/contest_opponents.h"

static const struct CompressedSpriteSheet sSpriteSheets_ContestantsTurnBlinkEffect[CONTESTANT_COUNT] =
{
    {
        .data = gBlankGfxCompressed,
        .size = 0x1000,
        .tag = TAG_BLINK_EFFECT_CONTESTANT0
    },
    {
        .data = gBlankGfxCompressed,
        .size = 0x1000,
        .tag = TAG_BLINK_EFFECT_CONTESTANT1
    },
    {
        .data = gBlankGfxCompressed,
        .size = 0x1000,
        .tag = TAG_BLINK_EFFECT_CONTESTANT2
    },
    {
        .data = gBlankGfxCompressed,
        .size = 0x1000,
        .tag = TAG_BLINK_EFFECT_CONTESTANT3
    }
};

static const struct SpritePalette sSpritePalettes_ContestantsTurnBlinkEffect[CONTESTANT_COUNT] =
{
    {
        .data = eContestTempSave.cachedWindowPalettes[5],
        .tag = TAG_BLINK_EFFECT_CONTESTANT0
    },
    {
        .data = eContestTempSave.cachedWindowPalettes[6],
        .tag = TAG_BLINK_EFFECT_CONTESTANT1
    },
    {
        .data = eContestTempSave.cachedWindowPalettes[7],
        .tag = TAG_BLINK_EFFECT_CONTESTANT2
    },
    {
        .data = eContestTempSave.cachedWindowPalettes[8],
        .tag = TAG_BLINK_EFFECT_CONTESTANT3
    }
};

static const struct OamData sOam_ContestantsTurnBlinkEffect =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_DOUBLE,
    .objMode = ST_OAM_OBJ_BLEND,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AffineAnimCmd sAffineAnim_ContestantsTurnBlinkEffect_0[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 0, 0),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_ContestantsTurnBlinkEffect_1[] =
{
    AFFINEANIMCMD_FRAME(3, 3, 0, 15),
    AFFINEANIMCMD_FRAME(-3, -3, 0, 15),
    AFFINEANIMCMD_FRAME(3, 3, 0, 15),
    AFFINEANIMCMD_FRAME(-3, -3, 0, 15),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd *const sAffineAnims_ContestantsTurnBlinkEffect[] =
{
    sAffineAnim_ContestantsTurnBlinkEffect_0,
    sAffineAnim_ContestantsTurnBlinkEffect_1
};

static const struct SpriteTemplate sSpriteTemplates_ContestantsTurnBlinkEffect[CONTESTANT_COUNT] =
{
    {
        .tileTag = TAG_BLINK_EFFECT_CONTESTANT0,
        .paletteTag = TAG_BLINK_EFFECT_CONTESTANT0,
        .oam = &sOam_ContestantsTurnBlinkEffect,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = sAffineAnims_ContestantsTurnBlinkEffect,
        .callback = SpriteCallbackDummy
    },
    {
        .tileTag = TAG_BLINK_EFFECT_CONTESTANT1,
        .paletteTag = TAG_BLINK_EFFECT_CONTESTANT1,
        .oam = &sOam_ContestantsTurnBlinkEffect,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = sAffineAnims_ContestantsTurnBlinkEffect,
        .callback = SpriteCallbackDummy
    },
    {
        .tileTag = TAG_BLINK_EFFECT_CONTESTANT2,
        .paletteTag = TAG_BLINK_EFFECT_CONTESTANT2,
        .oam = &sOam_ContestantsTurnBlinkEffect,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = sAffineAnims_ContestantsTurnBlinkEffect,
        .callback = SpriteCallbackDummy
    },
    {
        .tileTag = TAG_BLINK_EFFECT_CONTESTANT3,
        .paletteTag = TAG_BLINK_EFFECT_CONTESTANT3,
        .oam = &sOam_ContestantsTurnBlinkEffect,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = sAffineAnims_ContestantsTurnBlinkEffect,
        .callback = SpriteCallbackDummy
    }
};

static const s8 sContestExcitementTable[CONTEST_CATEGORIES_COUNT][CONTEST_CATEGORIES_COUNT] =
{
    [CONTEST_CATEGORY_COOL] = {
        [CONTEST_CATEGORY_COOL]   = +1,
        [CONTEST_CATEGORY_BEAUTY] =  0,
        [CONTEST_CATEGORY_CUTE]   = -1,
        [CONTEST_CATEGORY_SMART]  = -1,
        [CONTEST_CATEGORY_TOUGH]  =  0
    },
    [CONTEST_CATEGORY_BEAUTY] = {
        [CONTEST_CATEGORY_COOL]   =  0,
        [CONTEST_CATEGORY_BEAUTY] = +1,
        [CONTEST_CATEGORY_CUTE]   =  0,
        [CONTEST_CATEGORY_SMART]  = -1,
        [CONTEST_CATEGORY_TOUGH]  = -1
    },
    [CONTEST_CATEGORY_CUTE] = {
        [CONTEST_CATEGORY_COOL]   = -1,
        [CONTEST_CATEGORY_BEAUTY] =  0,
        [CONTEST_CATEGORY_CUTE]   = +1,
        [CONTEST_CATEGORY_SMART]  =  0,
        [CONTEST_CATEGORY_TOUGH]  = -1
    },
    [CONTEST_CATEGORY_SMART] = {
        [CONTEST_CATEGORY_COOL]   = -1,
        [CONTEST_CATEGORY_BEAUTY] = -1,
        [CONTEST_CATEGORY_CUTE]   =  0,
        [CONTEST_CATEGORY_SMART]  = +1,
        [CONTEST_CATEGORY_TOUGH]  =  0
    },
    [CONTEST_CATEGORY_TOUGH] = {
        [CONTEST_CATEGORY_COOL]   =  0,
        [CONTEST_CATEGORY_BEAUTY] = -1,
        [CONTEST_CATEGORY_CUTE]   = -1,
        [CONTEST_CATEGORY_SMART]  =  0,
        [CONTEST_CATEGORY_TOUGH]  = +1
    }
};

static void TaskDummy1(u8 taskId)
{
}

void ResetLinkContestBoolean(void)
{
    gLinkContestFlags = 0;
}

static void SetupContestGpuRegs(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG2 | WININ_WIN0_BG3 | WININ_WIN0_OBJ | WININ_WIN0_CLR | WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG2 | WININ_WIN1_BG3 | WININ_WIN1_OBJ | WININ_WIN1_CLR);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_BG2 | WINOUT_WIN01_BG3 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR | WINOUT_WINOBJ_BG0 | WINOUT_WINOBJ_BG1 | WINOUT_WINOBJ_BG2 | WINOUT_WINOBJ_BG3 | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR);
    SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_BG0_ON | DISPCNT_BG1_ON | DISPCNT_BG2_ON | DISPCNT_BG3_ON | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);

    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    gBattle_BG1_X = 0;
    gBattle_BG1_Y = 0;
    gBattle_BG2_X = 0;
    gBattle_BG2_Y = 0;
    gBattle_BG3_X = 0;
    gBattle_BG3_Y = 0;
    gBattle_WIN0H = 0;
    gBattle_WIN0V = 0;
    gBattle_WIN1H = 0;
    gBattle_WIN1V = 0;
}

void LoadContestBgAfterMoveAnim(void)
{
    s32 i;

    LZDecompressVram(gContestInterfaceGfx, (void *)VRAM);
    LZDecompressVram(gContestAudienceGfx, (void *)(BG_SCREEN_ADDR(4)));
    CopyToBgTilemapBuffer(3, gContestAudienceTilemap, 0, 0);
    CopyBgTilemapBufferToVram(3);
    LoadCompressedPalette(gContestInterfaceAudiencePalette, BG_PLTT_OFFSET, BG_PLTT_SIZE);
    LoadContestPalettes();
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        u32 contestantWindowId = 5 + i;

        LoadPalette(eContestTempSave.cachedWindowPalettes[contestantWindowId], BG_PLTT_ID(5 + gContestantTurnOrder[i]), sizeof((eContestTempSave.cachedWindowPalettes[contestantWindowId])));
    }
}

static void InitContestInfoBgs(void)
{
    s32 i;

    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sContestBgTemplates, ARRAY_COUNT(sContestBgTemplates));
    SetBgAttribute(3, BG_ATTR_WRAPAROUND, 1);
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        SetBgTilemapBuffer(i, gContestResources->contestBgTilemaps[i]);
    }
}

static void InitContestWindows(void)
{
    InitWindows(sContestWindowTemplates);
    DeactivateAllTextPrinters();
    if (gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK)
    {
        gTextFlags.canABSpeedUpPrint = FALSE;
    }
    else
    {
        gTextFlags.canABSpeedUpPrint = TRUE;
    }
}

static void LoadContestPalettes(void)
{
    s32 i;

    LoadPalette(sText_Pal, BG_PLTT_ID(15), sizeof(sText_Pal));
    SetBackdropFromColor(RGB_BLACK);
    for (i = 10; i < 14; i++)
        LoadPalette(&gPlttBufferUnfaded[BG_PLTT_ID(15) + 1], BG_PLTT_ID(15) + i, PLTT_SIZEOF(1));
    FillPalette(RGB(31, 17, 31), BG_PLTT_ID(15) + 3, PLTT_SIZEOF(1));
}

static void InitContestResources(void)
{
    s32 i;

    eContest = (struct Contest){};
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        eContest.unk[i] = 0xFF;
    }
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        eContestantStatus[i] = (struct ContestantStatus){};
    }
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        eContestantStatus[i].ranking = 0;
        eContestantStatus[i].effectStringId = CONTEST_STRING_NONE;
        eContestantStatus[i].effectStringId2 = CONTEST_STRING_NONE;
    }
    eContestAppealResults = (struct ContestAppealMoveResults){};
    eContestAI = (struct ContestAIInfo){};
    *gContestResources->excitement = (struct ContestExcitement){};
    memset(eContestGfxState, 0, CONTESTANT_COUNT * sizeof(struct ContestGraphicsState));

    if (!(gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK))
        SortContestants(FALSE);

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        eContestantStatus[i].nextTurnOrder = CONTESTANT_NONE;
        eContest.prevTurnOrder[i] = gContestantTurnOrder[i];
    }
    // Calling this here while all the nextTurnOrder values are CONTESTANT_NONE will actually
    // just reverse the turn order.
    ApplyNextTurnOrder();
    memset(gContestResources->tv, 0, sizeof(*gContestResources->tv) * CONTESTANT_COUNT);
}

static void AllocContestResources(void)
{
    gContestResources = AllocZeroed(sizeof(struct ContestResources));
    gContestResources->contest = AllocZeroed(sizeof(struct Contest));
    gContestResources->status = AllocZeroed(sizeof(struct ContestantStatus) * CONTESTANT_COUNT);
    gContestResources->appealResults = AllocZeroed(sizeof(struct ContestAppealMoveResults));
    gContestResources->aiData = AllocZeroed(sizeof(struct ContestAIInfo));
    gContestResources->excitement = AllocZeroed(sizeof(struct ContestExcitement) * CONTESTANT_COUNT);
    gContestResources->gfxState = AllocZeroed(sizeof(struct ContestGraphicsState) * CONTESTANT_COUNT);
    gContestResources->moveAnim = AllocZeroed(sizeof(struct ContestMoveAnimData));
    gContestResources->tv = AllocZeroed(sizeof(struct ContestTV) * CONTESTANT_COUNT);
    gContestResources->unused = AllocZeroed(sizeof(struct ContestUnused));
    gContestResources->contestBgTilemaps[0] = AllocZeroed(0x1000);
    gContestResources->contestBgTilemaps[1] = AllocZeroed(0x1000);
    gContestResources->contestBgTilemaps[2] = AllocZeroed(0x1000);
    gContestResources->contestBgTilemaps[3] = AllocZeroed(0x1000);
    gContestResources->boxBlinkTiles1 = AllocZeroed(0x800);
    gContestResources->boxBlinkTiles2 = AllocZeroed(0x800);
    gContestResources->animBgTileBuffer = AllocZeroed(0x2000);
    gBattleAnimBgTileBuffer = gContestResources->animBgTileBuffer;
    gBattleAnimBgTilemapBuffer = gContestResources->contestBgTilemaps[1];
}

static void FreeContestResources(void)
{
    FREE_AND_SET_NULL(gContestResources->contest);
    FREE_AND_SET_NULL(gContestResources->status);
    FREE_AND_SET_NULL(gContestResources->appealResults);
    FREE_AND_SET_NULL(gContestResources->aiData);
    FREE_AND_SET_NULL(gContestResources->excitement);
    FREE_AND_SET_NULL(gContestResources->gfxState);
    FREE_AND_SET_NULL(gContestResources->moveAnim);
    FREE_AND_SET_NULL(gContestResources->tv);
    FREE_AND_SET_NULL(gContestResources->unused);
    FREE_AND_SET_NULL(gContestResources->contestBgTilemaps[0]);
    FREE_AND_SET_NULL(gContestResources->contestBgTilemaps[1]);
    FREE_AND_SET_NULL(gContestResources->contestBgTilemaps[2]);
    FREE_AND_SET_NULL(gContestResources->contestBgTilemaps[3]);
    FREE_AND_SET_NULL(gContestResources->boxBlinkTiles1);
    FREE_AND_SET_NULL(gContestResources->boxBlinkTiles2);
    FREE_AND_SET_NULL(gContestResources->animBgTileBuffer);
    FREE_AND_SET_NULL(gContestResources);
    gBattleAnimBgTileBuffer = NULL;
    gBattleAnimBgTilemapBuffer = NULL;
}

void CB2_StartContest(void)
{
    switch (gMain.state)
    {
    case 0:
        sContestBgCopyFlags = 0;
        AllocContestResources();
        AllocateMonSpritesGfx();
        FREE_AND_SET_NULL(gMonSpritesGfxPtr->firstDecompressed);
        gMonSpritesGfxPtr->firstDecompressed = AllocZeroed(0x4000);
        SetVBlankCallback(NULL);
        InitContestInfoBgs();
        InitContestWindows();
        SetupContestGpuRegs();
        ScanlineEffect_Clear();
        ResetPaletteFade();
        gPaletteFade.bufferTransferDisabled = TRUE;
        ResetSpriteData();
        ResetTasks();
        FreeAllSpritePalettes();
        gReservedSpritePaletteCount = 4;
        eContestDebugMode = CONTEST_DEBUG_MODE_OFF;
        ClearBattleMonForms();
        InitContestResources();
        gMain.state++;
        break;
    case 1:
        gMain.state++;
        break;
    case 2:
        if (SetupContestGraphics(&eContest.contestSetupState))
        {
            eContest.contestSetupState = 0;
            gMain.state++;
        }
        break;
    case 3:
        SetBgForCurtainDrop();
        gBattle_BG1_X = 0;
        gBattle_BG1_Y = 0;
        BeginFastPaletteFade(2);
        gPaletteFade.bufferTransferDisabled = FALSE;
        SetVBlankCallback(VBlankCB_Contest);
        eContest.mainTaskId = CreateTask(Task_StartContestWaitFade, 10);
        SetMainCallback2(CB2_ContestMain);
        if (gLinkContestFlags & LINK_CONTEST_FLAG_IS_WIRELESS)
        {
            LoadWirelessStatusIndicatorSpriteGfx();
            CreateWirelessStatusIndicatorSprite(8, 8);
        }
        break;
    }
}

static void Task_StartContestWaitFade(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        gTasks[taskId].data[0] = 0;
        gTasks[taskId].func = Task_TryStartLinkContest;
    }
}

// If this is a link contest try to start appeals communication
// Otherwise skip ahead
static void Task_TryStartLinkContest(u8 taskId)
{
    if (gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK)
    {
        if (gLinkContestFlags & LINK_CONTEST_FLAG_IS_WIRELESS)
        {
            switch (gTasks[taskId].data[0])
            {
            case 0:
                ContestPrintLinkStandby();
                gTasks[taskId].data[0]++;
                // fallthrough
            case 1:
                if (IsLinkTaskFinished())
                {
                    SetLinkStandbyCallback();
                    gTasks[taskId].data[0]++;
                }
                return;
            case 2:
                if (IsLinkTaskFinished() != TRUE)
                    return;
                gTasks[taskId].data[0]++;
                break;
            }
        }

        if (!gPaletteFade.active)
        {
            gPaletteFade.bufferTransferDisabled = FALSE;
            if (!(gLinkContestFlags & LINK_CONTEST_FLAG_IS_WIRELESS))
                ContestPrintLinkStandby();
            CreateTask(Task_CommunicateMonIdxs, 0);
            gTasks[taskId].data[0] = 0;
            gTasks[taskId].func = TaskDummy1;
        }
    }
    else
    {
        gTasks[taskId].func = Task_WaitToRaiseCurtainAtStart;
    }
}

static void Task_CommunicateMonIdxs(u8 taskId)
{
    SetTaskFuncWithFollowupFunc(taskId, Task_LinkContest_CommunicateMonIdxs, Task_EndCommunicateMonIdxs);
}

static void Task_EndCommunicateMonIdxs(u8 taskId)
{
    gTasks[taskId].data[0] = 1;
    gTasks[taskId].func = Task_ReadyStartLinkContest;
}

static void Task_ReadyStartLinkContest(u8 taskId)
{
    // data[0] always 1 here
    gTasks[taskId].data[0]--;
    if (gTasks[taskId].data[0] <= 0)
    {
        GetMultiplayerId();  // unused return value
        DestroyTask(taskId);
        gTasks[eContest.mainTaskId].func = Task_WaitToRaiseCurtainAtStart;
        gRngValue = gContestRngValue;
    }
}

static bool8 SetupContestGraphics(u8 *stateVar)
{
    u16 ALIGNED(4) tempPalette1[16];
    u16 ALIGNED(4) tempPalette2[16];

    switch (*stateVar)
    {
    case 0:
        gPaletteFade.bufferTransferDisabled = TRUE;
        RequestDma3Fill(0, (void *)VRAM, 0x8000, 1);
        RequestDma3Fill(0, (void *)VRAM + 0x8000, 0x8000, 1);
        RequestDma3Fill(0, (void *)VRAM + 0x10000, 0x8000, 1);
        break;
    case 1:
        LZDecompressVram(gContestInterfaceGfx, (void *)VRAM);
        break;
    case 2:
        LZDecompressVram(gContestAudienceGfx, (void *)(BG_SCREEN_ADDR(4)));
        DmaCopyLarge32(3, (void *)(BG_SCREEN_ADDR(4)), eUnzippedContestAudience_Gfx, 0x2000, 0x1000);
        break;
    case 3:
        CopyToBgTilemapBuffer(3, gContestAudienceTilemap, 0, 0);
        CopyBgTilemapBufferToVram(3);
        break;
    case 4:
        CopyToBgTilemapBuffer(2, gContestInterfaceTilemap, 0, 0);
        CopyBgTilemapBufferToVram(2);
        // This is a bug, and copies random junk. savedJunk is never read.
        DmaCopy32Defvars(3, gContestResources->contestBgTilemaps[2], eContestTempSave.savedJunk, sizeof(eContestTempSave.savedJunk));
        break;
    case 5:
        LoadCompressedPalette(gContestInterfaceAudiencePalette, BG_PLTT_OFFSET, BG_PLTT_SIZE);
        CpuCopy32(&gPlttBufferUnfaded[BG_PLTT_ID(8)], tempPalette1, PLTT_SIZE_4BPP);
        CpuCopy32(&gPlttBufferUnfaded[BG_PLTT_ID(5 + gContestPlayerMonIndex)], tempPalette2, PLTT_SIZE_4BPP);
        CpuCopy32(tempPalette2, &gPlttBufferUnfaded[BG_PLTT_ID(8)], PLTT_SIZE_4BPP);
        CpuCopy32(tempPalette1, &gPlttBufferUnfaded[BG_PLTT_ID(5 + gContestPlayerMonIndex)], PLTT_SIZE_4BPP);
        DmaCopy32Defvars(3, gPlttBufferUnfaded, eContestTempSave.cachedWindowPalettes, sizeof(eContestTempSave.cachedWindowPalettes));
        LoadContestPalettes();
        break;
    case 6:
        DrawContestantWindows();
        FillContestantWindowBgs();
        SwapMoveDescAndContestTilemaps();
        eContest.judgeSpeechBubbleSpriteId = CreateJudgeSpeechBubbleSprite();
        CreateSliderHeartSprites();
        CreateNextTurnSprites();
        CreateApplauseMeterSprite();
        CreateJudgeAttentionEyeTask();
        CreateUnusedBlendTask();
        gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;
        gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;
        gBattlerPositions[2] = B_POSITION_OPPONENT_RIGHT;
        gBattlerPositions[3] = B_POSITION_PLAYER_RIGHT;
        gBattleTypeFlags = 0;
        gBattlerAttacker = B_POSITION_PLAYER_RIGHT;
        gBattlerTarget = B_POSITION_OPPONENT_RIGHT;
        // Unclear why judge sprite is assigned here
        // Overwritten in APPEALSTATE_SLIDE_MON_IN with the attacking contest mon
        gBattlerSpriteIds[gBattlerAttacker] = CreateJudgeSprite();
        CreateInvisibleBattleTargetSprite();
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(1);
        ShowBg(3);
        ShowBg(2);
        ShowBg(0);
        ShowBg(1);
        break;
    default:
        *stateVar = 0;
        return TRUE;
    }

    (*stateVar)++;
    return FALSE;
}

static void Task_WaitToRaiseCurtainAtStart(u8 taskId)
{
    gPaletteFade.bufferTransferDisabled = FALSE;
    if (!gPaletteFade.active)
    {
        gTasks[taskId].data[0] = 0;
        gTasks[taskId].data[1] = 0;
        gTasks[taskId].func = Task_RaiseCurtainAtStart;
    }
}

static void Task_RaiseCurtainAtStart(u8 taskId)
{
    switch (gTasks[taskId].data[0])
    {
    case 0:
        if (gTasks[taskId].data[1]++ <= 60)
            break;
        gTasks[taskId].data[1] = 0;
        PlaySE12WithPanning(SE_CONTEST_CURTAIN_RISE, 0);
        gTasks[taskId].data[0]++;
        break;
    case 1:
        *(s16 *)&gBattle_BG1_Y += 7;
        if ((s16)gBattle_BG1_Y <= DISPLAY_HEIGHT)
            break;
        gTasks[taskId].data[0]++;
        break;
    case 2:
        UpdateContestantBoxOrder();
        gTasks[taskId].data[0]++;
        break;
    case 3:
    {
        u16 bg0Cnt = GetGpuReg(REG_OFFSET_BG0CNT);
        u16 bg2Cnt = GetGpuReg(REG_OFFSET_BG2CNT);
        ((struct BgCnt *)&bg0Cnt)->priority = 0;
        ((struct BgCnt *)&bg2Cnt)->priority = 0;
        SetGpuReg(REG_OFFSET_BG0CNT, bg0Cnt);
        SetGpuReg(REG_OFFSET_BG2CNT, bg2Cnt);
        SlideApplauseMeterIn();
        gTasks[taskId].data[0]++;
        break;
    }
    case 4:
    default:
        if (eContest.applauseMeterIsMoving)
            break;
        gTasks[taskId].data[0] = 0;
        gTasks[taskId].data[1] = 0;
        gTasks[taskId].func = Task_DisplayAppealNumberText;
        break;
    }
}

static void CB2_ContestMain(void)
{
    s32 i;

    AnimateSprites();
    RunTasks();
    BuildOamBuffer();
    UpdatePaletteFade();

    for (i = 0; i < 4; i++)
    {
        if ((sContestBgCopyFlags >> i) & 1)
            CopyBgTilemapBufferToVram(i);
    }
    sContestBgCopyFlags = 0;
}

static void VBlankCB_Contest(void)
{
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
    TransferPlttBuffer();
    LoadOam();
    ProcessSpriteCopyRequests();
    ScanlineEffect_InitHBlankDmaTransfer();
}

static void Task_DisplayAppealNumberText(u8 taskId)
{
    if (gTasks[taskId].data[0] == 0)
    {
        gBattle_BG0_Y = 0;
        gBattle_BG2_Y = 0;
        ContestDebugDoPrint();
        DmaCopy32Defvars(3, gPlttBufferUnfaded, eContestTempSave.cachedPlttBufferUnfaded, PLTT_SIZE);
        ConvertIntToDecimalStringN(gStringVar1, eContest.appealNumber + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
        if (!Contest_IsMonsTurnDisabled(gContestPlayerMonIndex))
            StringCopy(gDisplayedStringBattle, gText_AppealNumWhichMoveWillBePlayed);
        else
            StringCopy(gDisplayedStringBattle, gText_AppealNumButItCantParticipate);
        ContestClearGeneralTextWindow();
        StringExpandPlaceholders(gStringVarBuffer, gDisplayedStringBattle);
        Contest_StartTextPrinter(gStringVarBuffer, TRUE);
        gTasks[taskId].data[0]++;
    }
    else
    {
        if (!Contest_RunTextPrinters())
        {
            gTasks[taskId].data[0] = 0;
            gTasks[taskId].func = Task_TryShowMoveSelectScreen;
        }
    }
}

static void Task_TryShowMoveSelectScreen(u8 taskId)
{
    // Wait for button press to show move select screen
    if ((JOY_NEW(A_BUTTON)) || (gMain.newKeys == B_BUTTON))
    {
        PlaySE(SE_SELECT);
        if (!Contest_IsMonsTurnDisabled(gContestPlayerMonIndex))
        {
            SetBottomSliderHeartsInvisibility(TRUE);
            gTasks[taskId].func = Task_ShowMoveSelectScreen;
        }
        else
        {
            // Skip move selection (selected move will be MOVE_NONE)
            gTasks[taskId].func = Task_SelectedMove;
        }
    }
}

static void Task_ShowMoveSelectScreen(u8 taskId)
{
    u8 i;
    u8 moveName[32];

    gBattle_BG0_Y = DISPLAY_HEIGHT;
    gBattle_BG2_Y = DISPLAY_HEIGHT;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 move = gContestMons[gContestPlayerMonIndex].moves[i];
        u8 *moveNameBuffer = moveName;

        if (eContestantStatus[gContestPlayerMonIndex].prevMove != MOVE_NONE
            && IsContestantAllowedToCombo(gContestPlayerMonIndex)
            && AreMovesContestCombo(eContestantStatus[gContestPlayerMonIndex].prevMove, move)
            && eContestantStatus[gContestPlayerMonIndex].hasJudgesAttention)
        {
            // Highlight the text because it's a combo move
            moveNameBuffer = StringCopy(moveName, gText_ColorLightShadowDarkGray);
        }
        else if (move != MOVE_NONE
                 && eContestantStatus[gContestPlayerMonIndex].prevMove == move
                 && gContestMoves[move].effect != CONTEST_EFFECT_REPETITION_NOT_BORING)
        {
            // Gray the text because it's a repeated move
            moveNameBuffer = StringCopy(moveName, gText_ColorBlue);
        }
        moveNameBuffer = StringCopy(moveNameBuffer, gMoveNames[move]);

        FillWindowPixelBuffer(i + MOVE_WINDOWS_START, PIXEL_FILL(0));
        Contest_PrintTextToBg0WindowAt(i + MOVE_WINDOWS_START, moveName, 5, 1, FONT_NARROW);
    }

    DrawMoveSelectArrow(eContest.playerMoveChoice);
    PrintContestMoveDescription(gContestMons[gContestPlayerMonIndex].moves[eContest.playerMoveChoice]);
    gTasks[taskId].func = Task_HandleMoveSelectInput;
}

static void Task_HandleMoveSelectInput(u8 taskId)
{
    u8 numMoves = 0;
    s32 i;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (gContestMons[gContestPlayerMonIndex].moves[i] != MOVE_NONE)
            numMoves++;
    }

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].func = Task_SelectedMove;
    }
    else
    {
        switch (gMain.newAndRepeatedKeys)
        {
        case B_BUTTON:
            // Cancel move selection
            PlaySE(SE_SELECT);
            SetBottomSliderHeartsInvisibility(FALSE);
            ConvertIntToDecimalStringN(gStringVar1, eContest.appealNumber + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
            if (!Contest_IsMonsTurnDisabled(gContestPlayerMonIndex))
                StringCopy(gDisplayedStringBattle, gText_AppealNumWhichMoveWillBePlayed);
            else
                StringCopy(gDisplayedStringBattle, gText_AppealNumButItCantParticipate);
            ContestClearGeneralTextWindow();
            StringExpandPlaceholders(gStringVarBuffer, gDisplayedStringBattle);
            Contest_StartTextPrinter(gStringVarBuffer, FALSE);
            gBattle_BG0_Y = 0;
            gBattle_BG2_Y = 0;
            gTasks[taskId].func = Task_TryShowMoveSelectScreen;
            break;
        case DPAD_LEFT:
        case DPAD_RIGHT:
            break;
        case DPAD_UP:
            EraseMoveSelectArrow(eContest.playerMoveChoice);
            if (eContest.playerMoveChoice == 0)
                eContest.playerMoveChoice = numMoves - 1;
            else
                eContest.playerMoveChoice--;
            DrawMoveSelectArrow(eContest.playerMoveChoice);
            PrintContestMoveDescription(gContestMons[gContestPlayerMonIndex].moves[eContest.playerMoveChoice]);
            if (numMoves > 1)
                PlaySE(SE_SELECT);
            break;
        case DPAD_DOWN:
            EraseMoveSelectArrow(eContest.playerMoveChoice);
            if (eContest.playerMoveChoice == numMoves - 1)
                eContest.playerMoveChoice = 0;
            else
                eContest.playerMoveChoice++;
            DrawMoveSelectArrow(eContest.playerMoveChoice);
            PrintContestMoveDescription(gContestMons[gContestPlayerMonIndex].moves[eContest.playerMoveChoice]);
            if (numMoves > 1)
                PlaySE(SE_SELECT);
            break;
        }
    }
}

static void DrawMoveSelectArrow(s8 moveIndex)
{
    ContestBG_FillBoxWithIncrementingTile(2, 55, 0, 31 + moveIndex * 2, 2, 2, 17, 1);
}

static void EraseMoveSelectArrow(s8 moveIndex)
{
    ContestBG_FillBoxWithIncrementingTile(2, 11, 0, 31 + moveIndex * 2, 2, 1, 17, 1);
    ContestBG_FillBoxWithIncrementingTile(2, 11, 0, 32 + moveIndex * 2, 2, 1, 17, 1);
}

static void Task_SelectedMove(u8 taskId)
{
    if (gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK)
    {
        u16 move = GetChosenMove(gContestPlayerMonIndex);
        u8 taskId2;

        eContestantStatus[gContestPlayerMonIndex].currMove = move;
        taskId2 = CreateTask(Task_LinkContest_CommunicateMoveSelections, 0);
        SetTaskFuncWithFollowupFunc(taskId2, Task_LinkContest_CommunicateMoveSelections, Task_EndCommunicateMoveSelections);
        gTasks[taskId].func = TaskDummy1;
        ContestPrintLinkStandby();
        SetBottomSliderHeartsInvisibility(FALSE);
    }
    else
    {
        GetAllChosenMoves();
        gTasks[taskId].func = Task_HideMoveSelectScreen;
    }
}

static void Task_EndCommunicateMoveSelections(u8 taskId)
{
    DestroyTask(taskId);
    gTasks[eContest.mainTaskId].func = Task_HideMoveSelectScreen;
}

static void Task_HideMoveSelectScreen(u8 taskId)
{
    s32 i;

    ContestClearGeneralTextWindow();
    gBattle_BG0_Y = 0;
    gBattle_BG2_Y = 0;
    SetBottomSliderHeartsInvisibility(FALSE);

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        FillWindowPixelBuffer(MOVE_WINDOWS_START + i, PIXEL_FILL(0));
        PutWindowTilemap(MOVE_WINDOWS_START + i);
        CopyWindowToVram(MOVE_WINDOWS_START + i, COPYWIN_GFX);
    }
    Contest_SetBgCopyFlags(0);
    // This seems to be a bug; it should have just copied PLTT_BUFFER_SIZE.
    DmaCopy32Defvars(3, gPlttBufferFaded, eContestTempSave.cachedPlttBufferFaded, PLTT_SIZE);
    LoadPalette(eContestTempSave.cachedPlttBufferUnfaded, 0, PLTT_SIZE);
    gTasks[taskId].data[0] = 0;
    gTasks[taskId].data[1] = 0;
    gTasks[taskId].func = Task_HideApplauseMeterForAppealStart;
}

static void Task_HideApplauseMeterForAppealStart(u8 taskId)
{
    if (++gTasks[taskId].data[0] > 2)
    {
        gTasks[taskId].data[0] = 0;
        if (++gTasks[taskId].data[1] == 2)
        {
            SlideApplauseMeterOut();
            AnimateSliderHearts(SLIDER_HEART_ANIM_DISAPPEAR);
            gTasks[taskId].func = Task_WaitHideApplauseMeterForAppealStart;
        }
    }
}

static void Task_WaitHideApplauseMeterForAppealStart(u8 taskId)
{
    if (!eContest.applauseMeterIsMoving && !eContest.sliderHeartsAnimating)
        gTasks[taskId].func = Task_AppealSetup;
}

#define tState data[0]
#define tMonSpriteId data[2]
#define tCounter data[10]

static void Task_AppealSetup(u8 taskId)
{
    if (++gTasks[taskId].data[0] > 19)
    {
        eContest.turnNumber = 0;
        eContest.unusedRng = gRngValue;
        if ((gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK) && IsPlayerLinkLeader())
        {
            s32 i;

            for (i = 0; i + gNumLinkContestPlayers < CONTESTANT_COUNT; i++)
            {
                eContestantStatus[gNumLinkContestPlayers + i].currMove = GetChosenMove(gNumLinkContestPlayers + i);
            }
        }
        gTasks[taskId].tState = APPEALSTATE_START_TURN;
        gTasks[taskId].func = Task_DoAppeals;
    }
}

static void Task_DoAppeals(u8 taskId)
{
    u8 spriteId;
    s32 i;
    u8 contestant = eContest.currentContestant;
    s8 r3;

    switch (gTasks[taskId].tState)
    {
    case APPEALSTATE_START_TURN:
        ContestDebugDoPrint();
        for (i = 0; eContest.turnNumber != eContestAppealResults.turnOrder[i]; i++)
            ;
        eContest.currentContestant = i;
        contestant = eContest.currentContestant;
        if (gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK)
        {
            u8 taskId2;

            eContest.waitForLink = TRUE;
            if (IsPlayerLinkLeader())
                CalculateAppealMoveImpact(eContest.currentContestant);
            taskId2 = CreateTask(Task_LinkContest_CommunicateAppealsState, 0);
            SetTaskFuncWithFollowupFunc(taskId2, Task_LinkContest_CommunicateAppealsState, Task_EndWaitForLink);
            ContestPrintLinkStandby();
            gTasks[taskId].tState = APPEALSTATE_WAIT_LINK;
        }
        else
        {
            CalculateAppealMoveImpact(eContest.currentContestant);
            gTasks[taskId].tState = APPEALSTATE_CHECK_SKIP_TURN;
        }
        return;
    case APPEALSTATE_WAIT_LINK:
        if (!eContest.waitForLink)
            gTasks[taskId].tState = APPEALSTATE_CHECK_SKIP_TURN;
        return;
    case APPEALSTATE_CHECK_SKIP_TURN:
        SetContestLiveUpdateFlags(contestant);
        ContestDebugPrintBitStrings();
        if (eContestantStatus[contestant].numTurnsSkipped != 0
            || eContestantStatus[contestant].noMoreTurns)
        {
            gTasks[taskId].tState = APPEALSTATE_PRINT_SKIP_TURN_MSG;
        }
        else
        {
            ContestClearGeneralTextWindow();
            gTasks[taskId].tCounter = 0;
            gTasks[taskId].tState = APPEALSTATE_SLIDE_MON_IN;
        }
        return;
    case APPEALSTATE_SLIDE_MON_IN:
        for (i = 0; i < CONTESTANT_COUNT; i++)
            gBattleMonForms[i] = 0;
        memset(gContestResources->moveAnim, 0, sizeof(*gContestResources->moveAnim));
        SetMoveAnimAttackerData(eContest.currentContestant);
        spriteId = CreateContestantSprite(
            gContestMons[eContest.currentContestant].species,
            gContestMons[eContest.currentContestant].otId,
            gContestMons[eContest.currentContestant].personality,
            eContest.currentContestant);
        gSprites[spriteId].x2 = 120;
        gSprites[spriteId].callback = SpriteCB_MonSlideIn;
        gTasks[taskId].tMonSpriteId = spriteId;
        gBattlerSpriteIds[gBattlerAttacker] = spriteId;
        BlinkContestantBox(CreateContestantBoxBlinkSprites(eContest.currentContestant), FALSE);
        gTasks[taskId].tState = APPEALSTATE_WAIT_SLIDE_MON;
        return;
    case APPEALSTATE_WAIT_SLIDE_MON:
        spriteId = gTasks[taskId].tMonSpriteId;
        if (gSprites[spriteId].callback == SpriteCallbackDummy)
        {
            // Once mon has slid in, also wait for box to finish blinking
            if (!eContestGfxState[contestant].boxBlinking)
                gTasks[taskId].tState = APPEALSTATE_PRINT_USED_MOVE_MSG;
        }
        return;
    case APPEALSTATE_PRINT_USED_MOVE_MSG:
        if (eContestantStatus[contestant].nervous)
        {
            gTasks[taskId].tState = APPEALSTATE_PRINT_TOO_NERVOUS_MSG;
        }
        else
        {
            ContestClearGeneralTextWindow();
            StringCopy(gStringVar1, gContestMons[contestant].nickname);
            if (eContestantStatus[contestant].currMove < MOVES_COUNT)
                StringCopy(gStringVar2, gMoveNames[eContestantStatus[contestant].currMove]);
            else
                StringCopy(gStringVar2, sInvalidContestMoveNames[eContestantStatus[contestant].moveCategory]);
            StringExpandPlaceholders(gStringVarBuffer, gText_MonAppealedWithMove);
            Contest_StartTextPrinter(gStringVarBuffer, TRUE);
            gTasks[taskId].tState = APPEALSTATE_WAIT_USED_MOVE_MSG;
        }
        return;
    case APPEALSTATE_WAIT_USED_MOVE_MSG:
        if (!Contest_RunTextPrinters())
        {
            eContest.moveAnimTurnCount = 0;
            gTasks[taskId].tState = APPEALSTATE_MOVE_ANIM;
        }
        return;
    case APPEALSTATE_MOVE_ANIM:
        {
            u16 move = SanitizeMove(eContestantStatus[eContest.currentContestant].currMove);
            SetMoveSpecificAnimData(eContest.currentContestant);
            SetMoveAnimAttackerData(eContest.currentContestant);
            SetMoveTargetPosition(move);
            DoMoveAnim(move);
            gTasks[taskId].tState = APPEALSTATE_WAIT_MOVE_ANIM;
        }
        return;
    case APPEALSTATE_WAIT_MOVE_ANIM:
        gAnimScriptCallback();
        if (!gAnimScriptActive)
        {
            ClearMoveAnimData(contestant);
            if (eContest.moveAnimTurnCount != 0)
            {
                gTasks[taskId].tCounter = 0;
                gTasks[taskId].tState = APPEALSTATE_MOVE_ANIM_MULTITURN;
            }
            else
            {
                if (!eContestantStatus[contestant].hasJudgesAttention)
                    StopFlashJudgeAttentionEye(contestant);
                DrawUnnervedSymbols();
                gTasks[taskId].tState = APPEALSTATE_TRY_PRINT_MOVE_RESULT;
            }
        }
        return;
    case APPEALSTATE_MOVE_ANIM_MULTITURN:
        if (gTasks[taskId].tCounter++ > 30)
        {
            gTasks[taskId].tCounter = 0;
            gTasks[taskId].tState = APPEALSTATE_MOVE_ANIM;
        }
        return;
    case APPEALSTATE_TRY_PRINT_MOVE_RESULT:
        gTasks[taskId].data[1] = 0;
        if (eContestantStatus[contestant].effectStringId != CONTEST_STRING_NONE)
        {
            PrintAppealMoveResultText(contestant, eContestantStatus[contestant].effectStringId);
            eContestantStatus[contestant].effectStringId = CONTEST_STRING_NONE;
            gTasks[taskId].tState = APPEALSTATE_WAIT_MOVE_RESULT_MSG;
        }
        else
        {
            if (eContestantStatus[contestant].effectStringId2 != CONTEST_STRING_NONE)
            {
                for (i = 0; i < CONTESTANT_COUNT; i++)
                {
                    if (i != contestant && eContestantStatus[i].effectStringId != CONTEST_STRING_NONE)
                        break;
                }
                if (i == CONTESTANT_COUNT)
                {
                    PrintAppealMoveResultText(contestant, eContestantStatus[contestant].effectStringId2);
                    eContestantStatus[contestant].effectStringId2 = CONTEST_STRING_NONE;
                    gTasks[taskId].tState = APPEALSTATE_WAIT_MOVE_RESULT_MSG;
                }
                else
                {
                    gTasks[taskId].tState = APPEALSTATE_CHECK_TURN_ORDER_MOD;
                }
            }
            else
            {
                gTasks[taskId].tState = APPEALSTATE_CHECK_TURN_ORDER_MOD;
            }
        }
        return;
    case APPEALSTATE_WAIT_MOVE_RESULT_MSG:
        if (!Contest_RunTextPrinters())
            gTasks[taskId].tState = APPEALSTATE_TRY_PRINT_MOVE_RESULT;
        return;
    case APPEALSTATE_CHECK_TURN_ORDER_MOD:
        if (eContestantStatus[contestant].turnOrderModAction == 1)
        {
            DoJudgeSpeechBubble(JUDGE_SYMBOL_NUMBER_ONE);
        }
        else if (eContestantStatus[contestant].turnOrderModAction == 2)
        {
            DoJudgeSpeechBubble(JUDGE_SYMBOL_NUMBER_FOUR);
        }
        else if (eContestantStatus[contestant].turnOrderModAction == 3)
        {
            DoJudgeSpeechBubble(JUDGE_SYMBOL_QUESTION_MARK);
        }
        else
        {
            gTasks[taskId].tState = APPEALSTATE_TRY_SHOW_NEXT_TURN_GFX;
            return;
        }
        gTasks[taskId].tState = APPEALSTATE_WAIT_JUDGE_TURN_ORDER;
        return;
    case APPEALSTATE_WAIT_JUDGE_TURN_ORDER:
        if (!eContest.waitForJudgeSpeechBubble)
            gTasks[taskId].tState = APPEALSTATE_TRY_SHOW_NEXT_TURN_GFX;
        return;
    case APPEALSTATE_TRY_SHOW_NEXT_TURN_GFX:
        ShowHideNextTurnGfx(TRUE);
        gTasks[taskId].tState = APPEALSTATE_UPDATE_MOVE_USERS_HEARTS;
        return;
    case APPEALSTATE_UPDATE_MOVE_USERS_HEARTS:
        UpdateAppealHearts(0, eContestantStatus[contestant].appeal, contestant);
        gTasks[taskId].tState = APPEALSTATE_WAIT_MOVE_USERS_HEARTS;
        return;
    case APPEALSTATE_WAIT_MOVE_USERS_HEARTS:
        if (!eContestGfxState[eContest.currentContestant].updatingAppealHearts)
            gTasks[taskId].tState = APPEALSTATE_TRY_JUDGE_STAR;
        return;
    case APPEALSTATE_TRY_JUDGE_STAR:
        if (eContestantStatus[contestant].conditionMod == CONDITION_GAIN)
            DoJudgeSpeechBubble(JUDGE_SYMBOL_STAR);
        gTasks[taskId].tState = APPEALSTATE_WAIT_JUDGE_STAR;
        return;
    case APPEALSTATE_WAIT_JUDGE_STAR:
        if (!eContest.waitForJudgeSpeechBubble)
            gTasks[taskId].tState = APPEALSTATE_UPDATE_MOVE_USERS_STARS;
        return;
    case APPEALSTATE_UPDATE_MOVE_USERS_STARS:
        if (UpdateConditionStars(contestant, TRUE))
        {
            gTasks[taskId].tCounter = 0;
            gTasks[taskId].tState = APPEALSTATE_WAIT_MOVE_USERS_STARS;
        }
        else
        {
            gTasks[taskId].tState = APPEALSTATE_UPDATE_MOVE_USERS_STATUS;
        }
        return;
    case APPEALSTATE_WAIT_MOVE_USERS_STARS:
        if (++gTasks[taskId].tCounter > 20)
        {
            gTasks[taskId].tCounter = 0;
            gTasks[taskId].tState = APPEALSTATE_UPDATE_MOVE_USERS_STATUS;
        }
        return;
    case APPEALSTATE_UPDATE_MOVE_USERS_STATUS:
        if (DrawStatusSymbol(contestant))
            PlaySE(SE_CONTEST_ICON_CHANGE);
        gTasks[taskId].tState = APPEALSTATE_UPDATE_OPPONENTS;
        return;
    case APPEALSTATE_UPDATE_OPPONENTS:
        gTasks[taskId].data[1] = 0;
        gTasks[taskId].tState = APPEALSTATE_UPDATE_OPPONENT;
        return;
    case APPEALSTATE_UPDATE_OPPONENT:
        {
            // Update each other contestant as a result of this move appeal
            s32 j = 0;

            r3 = FALSE; // Can't get this to use local variable. Should be "needsUpdate"
            for (i = gTasks[taskId].data[1]; i < CONTESTANT_COUNT; i++)
            {
                r3 = FALSE;
                for (j = 0; j < CONTESTANT_COUNT; j++)
                {
                    if (j != contestant && gContestantTurnOrder[j] == i
                        && eContestantStatus[j].effectStringId != CONTEST_STRING_NONE)
                    {
                        r3 = TRUE;
                        break;
                    }
                }
                if (r3)
                    break;
            }
            if (r3)
            {
                // Update contestant
                gTasks[taskId].data[1] = gContestantTurnOrder[j];
                PrintAppealMoveResultText(j, eContestantStatus[j].effectStringId);
                eContestantStatus[j].effectStringId = CONTEST_STRING_NONE;
                gTasks[taskId].tState = APPEALSTATE_WAIT_OPPONENT_RESPONSE_MSG;
            }
            else
            {
                // Done updating contestants
                gTasks[taskId].data[1] = 0;
                gTasks[taskId].tCounter = 0;
                gTasks[taskId].tState = APPEALSTATE_TRY_PRINT_SKIP_NEXT_TURN_MSG;
                DrawStatusSymbols();
            }
        }
        return;
    case APPEALSTATE_WAIT_OPPONENT_RESPONSE_MSG:
        // Wait for contestants response to current appeal
        // i.e. "Contestant managed to avert its gaze"
        if (!Contest_RunTextPrinters())
            gTasks[taskId].tState = APPEALSTATE_UPDATE_OPPONENT_HEARTS;
        return;
    case APPEALSTATE_UPDATE_OPPONENT_HEARTS:
        for (i = 0; gTasks[taskId].data[1] != gContestantTurnOrder[i]; i++)
            ;
        UpdateAppealHearts(eContestantStatus[i].appeal + eContestantStatus[i].jam, -eContestantStatus[i].jam, i);
        gTasks[taskId].tState = APPEALSTATE_WAIT_OPPONENT_HEARTS;
        return;
    case APPEALSTATE_WAIT_OPPONENT_HEARTS:
        for (i = 0; gTasks[taskId].data[1] != gContestantTurnOrder[i]; i++)
            ;
        if (!eContestGfxState[i].updatingAppealHearts)
            gTasks[taskId].tState = APPEALSTATE_UPDATE_OPPONENT_STARS;
        return;
    case APPEALSTATE_UPDATE_OPPONENT_STARS:
        for (i = 0; gTasks[taskId].data[1] != gContestantTurnOrder[i]; i++)
            ;
        if (UpdateConditionStars(i, TRUE))
        {
            gTasks[taskId].tCounter = 0;
            gTasks[taskId].tState = APPEALSTATE_WAIT_OPPONENT_STARS;
        }
        else
        {
            gTasks[taskId].tState = APPEALSTATE_UPDATE_OPPONENT_STATUS;
        }
        return;
    case APPEALSTATE_WAIT_OPPONENT_STARS:
        if (++gTasks[taskId].tCounter > 20)
        {
            gTasks[taskId].tCounter = 0;
            gTasks[taskId].tState = APPEALSTATE_UPDATE_OPPONENT_STATUS;
        }
        return;
    case APPEALSTATE_UPDATE_OPPONENT_STATUS:
        for (i = 0; i < CONTESTANT_COUNT; i++)
        {
            if (gContestantTurnOrder[i] == gTasks[taskId].data[1])
                break;
        }
        if (DrawStatusSymbol(i))
            PlaySE(SE_CONTEST_ICON_CHANGE);
        else
            PlaySE(SE_CONTEST_ICON_CLEAR);
        if (eContestantStatus[i].judgesAttentionWasRemoved)
        {
            StopFlashJudgeAttentionEye(i);
            eContestantStatus[i].judgesAttentionWasRemoved = FALSE;
        }
        gTasks[taskId].data[1]++;
        gTasks[taskId].tState = APPEALSTATE_UPDATE_OPPONENT;
        return;
    case APPEALSTATE_TRY_PRINT_SKIP_NEXT_TURN_MSG:
        if (gTasks[taskId].tCounter++ > 9)
        {
            gTasks[taskId].tCounter = 0;
            if (eContestantStatus[contestant].numTurnsSkipped != 0
                || eContestantStatus[contestant].turnSkipped)
            {
                ContestClearGeneralTextWindow();
                StringCopy(gStringVar1, gContestMons[contestant].nickname);
                StringExpandPlaceholders(gStringVarBuffer, gText_MonCantAppealNextTurn);
                Contest_StartTextPrinter(gStringVarBuffer, TRUE);
            }
            gTasks[taskId].tState = APPEALSTATE_WAIT_SKIP_NEXT_TURN_MSG;
        }
        return;
    case APPEALSTATE_WAIT_SKIP_NEXT_TURN_MSG:
        if (!Contest_RunTextPrinters())
        {
            if (!eContestantStatus[contestant].usedComboMove)
                gTasks[taskId].tState = APPEALSTATE_CHECK_REPEATED_MOVE;
            else
                gTasks[taskId].tState = APPEALSTATE_PRINT_COMBO_MSG;
        }
        return;
    case APPEALSTATE_PRINT_COMBO_MSG:
    {
        s8 completedCombo = eContestantStatus[contestant].completedCombo;
        if (eContestantStatus[contestant].completedCombo)
        {
            // Finished combo

            // Looks like there were originally meant to be move combos of
            // different effectivenesses. completedCombo however is only ever 0 or 1,
            // so in here only "Went over well" will ever be used
            ContestClearGeneralTextWindow();
            if (completedCombo == 1)
                Contest_StartTextPrinter(gText_AppealComboWentOverWell, TRUE);
            else if (completedCombo == 2)
                Contest_StartTextPrinter(gText_AppealComboWentOverVeryWell, TRUE);
            else
                Contest_StartTextPrinter(gText_AppealComboWentOverExcellently, TRUE);

            DoJudgeSpeechBubble(JUDGE_SYMBOL_TWO_EXCLAMATIONS);
            gTasks[taskId].tCounter = 0;
            gTasks[taskId].tState = APPEALSTATE_WAIT_JUDGE_COMBO;
        }
        else
        {
            // Started combo
            ContestClearGeneralTextWindow();
            StringCopy(gStringVar1, gContestMons[contestant].nickname);
            StringExpandPlaceholders(gStringVarBuffer, gText_JudgeLookedAtMonExpectantly);
            Contest_StartTextPrinter(gStringVarBuffer, TRUE);
            DoJudgeSpeechBubble(JUDGE_SYMBOL_ONE_EXCLAMATION);
            gTasks[taskId].tCounter = 0;
            gTasks[taskId].tState = APPEALSTATE_WAIT_JUDGE_COMBO;
        }
        return;
    }
    case APPEALSTATE_WAIT_JUDGE_COMBO:
        if (!eContest.waitForJudgeSpeechBubble)
        {
            StartStopFlashJudgeAttentionEye(eContest.currentContestant);
            gTasks[taskId].tState = APPEALSTATE_TRY_UPDATE_HEARTS_FROM_COMBO;
        }
        return;
    case APPEALSTATE_TRY_UPDATE_HEARTS_FROM_COMBO:
        if (!Contest_RunTextPrinters())
        {
            if (++gTasks[taskId].tCounter > 50)
            {
                if (!eContestantStatus[contestant].hasJudgesAttention)
                {
                    UpdateAppealHearts(
                        eContestantStatus[contestant].appeal,
                        eContestantStatus[contestant].comboAppealBonus,
                        contestant);
                    eContestantStatus[contestant].appeal += eContestantStatus[contestant].comboAppealBonus;
                }
                gTasks[taskId].tState = APPEALSTATE_WAIT_HEARTS_FROM_COMBO;
            }
        }
        return;
    case APPEALSTATE_WAIT_HEARTS_FROM_COMBO:
        if (!eContestGfxState[contestant].updatingAppealHearts)
        {
            gTasks[taskId].tCounter = 0;
            gTasks[taskId].tState = APPEALSTATE_CHECK_REPEATED_MOVE;
        }
        return;
    case APPEALSTATE_CHECK_REPEATED_MOVE:
        if (eContestantStatus[contestant].repeatedMove)
        {
            ContestClearGeneralTextWindow();
            StringCopy(gStringVar1, gContestMons[contestant].nickname);
            StringExpandPlaceholders(gStringVarBuffer, gText_RepeatedAppeal);
            Contest_StartTextPrinter(gStringVarBuffer, TRUE);
            gTasks[taskId].tCounter = 0;
            DoJudgeSpeechBubble(JUDGE_SYMBOL_SWIRL);
            gTasks[taskId].tState = APPEALSTATE_WAIT_JUDGE_REPEATED_MOVE;
        }
        else
        {
            gTasks[taskId].tState = APPEALSTATE_UPDATE_CROWD;
        }
        return;
    case APPEALSTATE_WAIT_JUDGE_REPEATED_MOVE:
        if (!eContest.waitForJudgeSpeechBubble)
            gTasks[taskId].tState = APPEALSTATE_UPDATE_HEARTS_FROM_REPEAT;
        return;
    case APPEALSTATE_UPDATE_HEARTS_FROM_REPEAT:
        if (!Contest_RunTextPrinters())
        {
            UpdateAppealHearts(eContestantStatus[contestant].appeal, -eContestantStatus[contestant].repeatJam, contestant);
            eContestantStatus[contestant].appeal -= eContestantStatus[contestant].repeatJam;
            gTasks[taskId].tState = APPEALSTATE_WAIT_HEARTS_FROM_REPEAT;
        }
        return;
    case APPEALSTATE_WAIT_HEARTS_FROM_REPEAT:
        ContestDebugDoPrint();
        if (!eContestGfxState[contestant].updatingAppealHearts)
        {
            gTasks[taskId].tCounter = 0;
            ContestClearGeneralTextWindow();
            gTasks[taskId].tState = APPEALSTATE_UPDATE_CROWD;
        }
        return;
    case APPEALSTATE_UPDATE_CROWD:
        if (eContestExcitement.frozen && contestant != eContestExcitement.freezer)
        {
            gTasks[taskId].tState = APPEALSTATE_PRINT_CROWD_WATCHES_MSG;
        }
        else
        {
            r3 = eContestExcitement.moveExcitement; // Can't get this to use local variable. Should be "moveExcitement"
            if (eContestantStatus[contestant].overrideCategoryExcitementMod)
            {
                r3 = 1;
                StringCopy(gStringVar3, gMoveNames[eContestantStatus[contestant].currMove]);
            }
            else
            {
                StringCopy(gStringVar3, sContestConditions[gContestMoves[eContestantStatus[contestant].currMove].contestCategory]);
            }

            if (r3 > 0 && eContestantStatus[contestant].repeatedMove)
                r3 = 0;

            ContestClearGeneralTextWindow();
            StringCopy(gStringVar1, gContestMons[contestant].nickname);
            eContest.applauseLevel += r3;
            if (eContest.applauseLevel < 0)
                eContest.applauseLevel = 0;
            if (r3 == 0)
            {
                gTasks[taskId].tState = APPEALSTATE_SLIDE_APPLAUSE_OUT;
            }
            else
            {
                if (r3 < 0)
                    StringExpandPlaceholders(gStringVarBuffer, gText_MonsXDidntGoOverWell);
                else if (r3 > 0 && eContest.applauseLevel <= 4)
                    StringExpandPlaceholders(gStringVarBuffer, gText_MonsXWentOverGreat);
                else
                    StringExpandPlaceholders(gStringVarBuffer, gText_MonsXGotTheCrowdGoing);
                Contest_StartTextPrinter(gStringVarBuffer, TRUE);
                gTasks[taskId].tCounter = 0;
                gTasks[taskId].data[11] = 0;
                if (r3 < 0)
                    gTasks[taskId].tState = APPEALSTATE_DO_CROWD_UNEXCITED;
                else
                    gTasks[taskId].tState = APPEALSTATE_DO_CROWD_EXCITED;
            }
        }
        return;
    case APPEALSTATE_DO_CROWD_UNEXCITED:
        switch (gTasks[taskId].tCounter)
        {
        case 0:
            BlendAudienceBackground(-1, 1);
            PlayFanfare(MUS_TOO_BAD);
            gTasks[taskId].tCounter++;
            break;
        case 1:
            if (!eContest.waitForAudienceBlend && !Contest_RunTextPrinters())
            {
                ShowAndUpdateApplauseMeter(-1);
                gTasks[taskId].tCounter++;
            }
            break;
        case 2:
            if (!eContest.isShowingApplauseMeter)
            {
                if (gTasks[taskId].data[11]++ > 29)
                {
                    gTasks[taskId].data[11] = 0;
                    BlendAudienceBackground(-1, -1);
                    gTasks[taskId].tCounter++;
                }
            }
            break;
        case 3:
            if (!gPaletteFade.active)
            {
                gTasks[taskId].tCounter = 0;
                gTasks[taskId].data[11] = 0;
                gTasks[taskId].tState = APPEALSTATE_WAIT_EXCITEMENT_HEARTS;
            }
            break;
        }
        return;
    case APPEALSTATE_DO_CROWD_EXCITED:
        switch (gTasks[taskId].tCounter)
        {
        case 0:
            if (!Contest_RunTextPrinters())
            {
                BlendAudienceBackground(1, 1);
                gTasks[taskId].tCounter++;
            }
            break;
        case 1:
            if (!eContest.waitForAudienceBlend)
            {
                AnimateAudience();
                PlaySE(SE_M_ENCORE2);
                ShowAndUpdateApplauseMeter(1);
                gTasks[taskId].tCounter++;
            }
            break;
        case 2:
            if (!eContest.isShowingApplauseMeter)
            {
                if (gTasks[taskId].data[11]++ > 29)
                {
                    gTasks[taskId].data[11] = 0;
                    UpdateAppealHearts(eContestantStatus[contestant].appeal, eContestExcitement.excitementAppealBonus, contestant);
                    eContestantStatus[contestant].appeal += eContestExcitement.excitementAppealBonus;
                    gTasks[taskId].tCounter++;
                }
            }
            break;
        case 3:
            if (!eContestGfxState[contestant].updatingAppealHearts)
            {
                if (!eContest.animatingAudience)
                {
                    BlendAudienceBackground(1, -1);
                    gTasks[taskId].tCounter++;
                }
            }
            break;
        case 4:
            if (!gPaletteFade.active)
            {
                gTasks[taskId].tCounter = 0;
                gTasks[taskId].data[11] = 0;
                gTasks[taskId].tState = APPEALSTATE_WAIT_EXCITEMENT_HEARTS;
            }
            break;
        }
        return;
    case APPEALSTATE_WAIT_EXCITEMENT_HEARTS:
        if (!eContestGfxState[contestant].updatingAppealHearts)
        {
            ContestClearGeneralTextWindow();
            gTasks[taskId].tState = APPEALSTATE_SLIDE_APPLAUSE_OUT;
        }
        return;
    case APPEALSTATE_PRINT_CROWD_WATCHES_MSG:
        ContestClearGeneralTextWindow();
        StringCopy(gStringVar3, gContestMons[eContestExcitement.freezer].nickname);
        StringCopy(gStringVar1, gContestMons[contestant].nickname);
        StringCopy(gStringVar2, gMoveNames[eContestantStatus[contestant].currMove]);
        StringExpandPlaceholders(gStringVarBuffer, gText_CrowdContinuesToWatchMon);
        Contest_StartTextPrinter(gStringVarBuffer, TRUE);
        gTasks[taskId].tState = APPEALSTATE_PRINT_MON_MOVE_IGNORED_MSG;
        return;
    case APPEALSTATE_PRINT_MON_MOVE_IGNORED_MSG:
        if (!Contest_RunTextPrinters())
        {
            ContestClearGeneralTextWindow();
            StringExpandPlaceholders(gStringVarBuffer, gText_MonsMoveIsIgnored);
            Contest_StartTextPrinter(gStringVarBuffer, TRUE);
            gTasks[taskId].tState = APPEALSTATE_WAIT_MON_MOVE_IGNORED_MSG;
        }
        return;
    case APPEALSTATE_WAIT_MON_MOVE_IGNORED_MSG:
        if (!Contest_RunTextPrinters())
        {
            ContestClearGeneralTextWindow();
            gTasks[taskId].tState = APPEALSTATE_SLIDE_APPLAUSE_OUT;
        }
        return;
    case APPEALSTATE_PRINT_TOO_NERVOUS_MSG:
        if (eContestantStatus[contestant].hasJudgesAttention)
            eContestantStatus[contestant].hasJudgesAttention = FALSE;
        StartStopFlashJudgeAttentionEye(contestant);
        StringCopy(gStringVar1, gContestMons[contestant].nickname);
        StringCopy(gStringVar2, gMoveNames[eContestantStatus[contestant].currMove]);
        StringExpandPlaceholders(gStringVarBuffer, gText_MonWasTooNervousToMove);
        Contest_StartTextPrinter(gStringVarBuffer, TRUE);
        gTasks[taskId].tState = APPEALSTATE_WAIT_TOO_NERVOUS_MSG;
        return;
    case APPEALSTATE_WAIT_TOO_NERVOUS_MSG:
        if (!Contest_RunTextPrinters())
            gTasks[taskId].tState = APPEALSTATE_SLIDE_APPLAUSE_OUT;
        return;
    case APPEALSTATE_SLIDE_APPLAUSE_OUT:
        SlideApplauseMeterOut();
        gTasks[taskId].tState = APPEALSTATE_WAIT_SLIDE_APPLAUSE;
        return;
    case APPEALSTATE_WAIT_SLIDE_APPLAUSE:
        if (!eContest.applauseMeterIsMoving)
        {
            if (eContest.applauseLevel > 4)
            {
                eContest.applauseLevel = 0;
                UpdateApplauseMeter();
            }
            gTasks[taskId].tState = APPEALSTATE_SLIDE_MON_OUT;
        }
        return;
    case APPEALSTATE_SLIDE_MON_OUT:
        spriteId = gTasks[taskId].tMonSpriteId;
        gSprites[spriteId].callback = SpriteCB_MonSlideOut;
        gTasks[taskId].tState = APPEALSTATE_FREE_MON_SPRITE;
        return;
    case APPEALSTATE_FREE_MON_SPRITE:
        spriteId = gTasks[taskId].tMonSpriteId;
        if (gSprites[spriteId].invisible)
        {
            FreeSpriteOamMatrix(&gSprites[spriteId]);
            DestroySprite(&gSprites[spriteId]);
            gTasks[taskId].tState = APPEALSTATE_START_TURN_END_DELAY;
        }
        return;
    case APPEALSTATE_START_TURN_END_DELAY:
        gTasks[taskId].tCounter = 0;
        gTasks[taskId].tState = APPEALSTATE_TURN_END_DELAY;
        return;
    case APPEALSTATE_PRINT_SKIP_TURN_MSG:
        ContestClearGeneralTextWindow();
        StringCopy(gStringVar1, gContestMons[contestant].nickname);
        StringExpandPlaceholders(gStringVarBuffer, gText_MonWasWatchingOthers);
        Contest_StartTextPrinter(gStringVarBuffer, TRUE);
        gTasks[taskId].tState = APPEALSTATE_WAIT_SKIP_TURN_MSG;
        return;
    case APPEALSTATE_WAIT_SKIP_TURN_MSG:
        if (!Contest_RunTextPrinters())
            gTasks[taskId].tState = APPEALSTATE_TURN_END_DELAY;
        return;
    case APPEALSTATE_TURN_END_DELAY:
        if (++gTasks[taskId].tCounter > 29)
        {
            gTasks[taskId].tCounter = 0;
            gTasks[taskId].tState = APPEALSTATE_START_NEXT_TURN;
        }
        return;
    case APPEALSTATE_START_NEXT_TURN:
        if (++eContest.turnNumber == CONTESTANT_COUNT)
        {
            gTasks[taskId].tState = 0;
            gTasks[taskId].data[1] = 0;
            gTasks[taskId].tMonSpriteId = 0;
            gTasks[taskId].func = Task_FinishRoundOfAppeals;
        }
        else
        {
            gTasks[taskId].tState = APPEALSTATE_START_TURN;
        }
        return;
    }
}

static void Task_EndWaitForLink(u8 taskId)
{
    eContest.waitForLink = FALSE;
    DestroyTask(taskId);
}

static void SpriteCB_MonSlideIn(struct Sprite *sprite)
{
    if (sprite->x2 != 0)
    {
        sprite->x2 -= 2;
    }
    else
    {
        if (++sprite->data[0] == 31)
        {
            sprite->data[0] = 0;
            sprite->callback = SpriteCallbackDummy;
        }
    }
}

static void SpriteCB_MonSlideOut(struct Sprite *sprite)
{
    sprite->x2 -= 6;
    if (sprite->x + sprite->x2 < -32)
    {
        sprite->callback = SpriteCallbackDummy;
        sprite->invisible = TRUE;
    }
}

static void Task_FinishRoundOfAppeals(u8 taskId)
{
    switch (gTasks[taskId].data[0])
    {
    case 0:
        if (gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK)
        {
            u8 taskId2;

            eContest.waitForLink = TRUE;
            if (IsPlayerLinkLeader())
            {
                RankContestants();
                SetAttentionLevels();
            }
            taskId2 = CreateTask(Task_LinkContest_CommunicateAppealsState, 0);
            SetTaskFuncWithFollowupFunc(taskId2, Task_LinkContest_CommunicateAppealsState, Task_EndWaitForLink);
            ContestPrintLinkStandby();
            gTasks[taskId].data[0] = 1;
        }
        else
        {
            RankContestants();
            SetAttentionLevels();
            gTasks[taskId].data[0] = 2;
        }
        break;
    case 1:
        if (!eContest.waitForLink)
            gTasks[taskId].data[0] = 2;
        break;
    case 2:
        gTasks[taskId].data[0] = 0;
        gTasks[taskId].func = Task_ReadyUpdateHeartSliders;
        break;
    }
}

static void Task_ReadyUpdateHeartSliders(u8 taskId)
{
    ShowHideNextTurnGfx(FALSE);
    gTasks[taskId].data[0] = 0;
    gTasks[taskId].data[1] = 0;
    gTasks[taskId].func = Task_UpdateHeartSliders;
}

static void Task_UpdateHeartSliders(u8 taskId)
{
    switch (gTasks[taskId].data[0])
    {
    case 0:
        if (++gTasks[taskId].data[1] > 20)
        {
            AnimateSliderHearts(SLIDER_HEART_ANIM_APPEAR);
            gTasks[taskId].data[1] = 0;
            gTasks[taskId].data[0]++;
        }
        break;
    case 1:
        if (!eContest.sliderHeartsAnimating)
        {
            if (++gTasks[taskId].data[1] > 20)
            {
                gTasks[taskId].data[1] = 0;
                gTasks[taskId].data[0]++;
            }
        }
        break;
    case 2:
        UpdateHeartSliders();
        gTasks[taskId].data[0] = 0;
        gTasks[taskId].data[1] = 0;
        gTasks[taskId].func = Task_WaitForHeartSliders;
        break;
    }
}

static void Task_WaitForHeartSliders(u8 taskId)
{
    if (SlidersDoneUpdating())
        gTasks[taskId].func = Task_RestorePlttBufferUnfaded;
}

static void Task_RestorePlttBufferUnfaded(u8 taskId)
{
    DmaCopy32Defvars(3, eContestTempSave.cachedPlttBufferUnfaded, gPlttBufferUnfaded, PLTT_SIZE);
    gTasks[taskId].data[0] = 0;
    gTasks[taskId].data[1] = 2;
    gTasks[taskId].func = Task_WaitPrintRoundResult;
}

static void Task_WaitPrintRoundResult(u8 taskId)
{
    if (++gTasks[taskId].data[0] > 2)
    {
        gTasks[taskId].data[0] = 0;
        if (--gTasks[taskId].data[1] == 0)
            gTasks[taskId].func = Task_PrintRoundResultText;
    }
}

static void Task_PrintRoundResultText(u8 taskId)
{
    if (gTasks[taskId].data[0] == 0)
    {
        u8 attention = eContestantStatus[gContestPlayerMonIndex].attentionLevel;

        ContestClearGeneralTextWindow();
        StringCopy(gStringVar1, gContestMons[gContestPlayerMonIndex].nickname);
        StringExpandPlaceholders(gStringVarBuffer, sRoundResultTexts[attention]);
        Contest_StartTextPrinter(gStringVarBuffer, TRUE);
        gTasks[taskId].data[0]++;
    }
    else
    {
        if (!Contest_RunTextPrinters())
        {
            gTasks[taskId].data[0] = 0;
            gTasks[taskId].func = Task_ReUpdateHeartSliders;
            ContestDebugDoPrint();
        }
    }
}

static void Task_ReUpdateHeartSliders(u8 taskId)
{
    if (gTasks[taskId].data[0]++ > 29)
    {
        gTasks[taskId].data[0] = 0;
        UpdateHeartSliders(); // ? Sliders have already been updated
        gTasks[taskId].func = Task_WaitForHeartSlidersAgain;
    }
}

static void Task_WaitForHeartSlidersAgain(u8 taskId)
{
    if (SlidersDoneUpdating())
    {
        gTasks[taskId].data[0] = 0;
        gTasks[taskId].func = Task_DropCurtainAtRoundEnd;
    }
}

static void Task_DropCurtainAtRoundEnd(u8 taskId)
{
    SetBgForCurtainDrop();
    gTasks[taskId].func = Task_StartDropCurtainAtRoundEnd;
}

static void Task_UpdateContestantBoxOrder(u8 taskId)
{
    UpdateContestantBoxOrder();
    gTasks[taskId].func = Task_TryStartNextRoundOfAppeals;
}

static void Task_TryStartNextRoundOfAppeals(u8 taskId)
{
    vu16 sp0 = GetGpuReg(REG_OFFSET_BG0CNT);
    vu16 sp2 = GetGpuReg(REG_OFFSET_BG2CNT);
    ((vBgCnt *)&sp0)->priority = 0;
    ((vBgCnt *)&sp2)->priority = 0;
    SetGpuReg(REG_OFFSET_BG0CNT, sp0);
    SetGpuReg(REG_OFFSET_BG2CNT, sp2);
    eContest.appealNumber++;
    if (eContest.appealNumber == CONTEST_NUM_APPEALS)
    {
        gTasks[taskId].func = Task_EndAppeals;
    }
    else
    {
        SlideApplauseMeterIn();
        gTasks[taskId].func = Task_StartNewRoundOfAppeals;
    }
}

static void Task_StartNewRoundOfAppeals(u8 taskId)
{
    if (!eContest.applauseMeterIsMoving)
        gTasks[taskId].func = Task_DisplayAppealNumberText;
}

static void Task_EndAppeals(u8 taskId)
{
    s32 i;

    gBattle_BG0_Y = 0;
    gBattle_BG2_Y = 0;
    for (i = 0; i < CONTESTANT_COUNT; i++)
        gContestMonAppealPointTotals[i] = eContestantStatus[i].pointTotal;
    CalculateFinalScores();
    ContestClearGeneralTextWindow();
    if (!(gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK))
    {
        BravoTrainerPokemonProfile_BeforeInterview1(eContestantStatus[gContestPlayerMonIndex].prevMove);
    }
    else
    {
        CalculateContestLiveUpdateData();
        SetConestLiveUpdateTVData();
        ContestDebugPrintBitStrings();
    }
    gContestRngValue = gRngValue;
    StringExpandPlaceholders(gStringVarBuffer, gText_AllOutOfAppealTime);
    Contest_StartTextPrinter(gStringVarBuffer, TRUE);
    gTasks[taskId].data[2] = 0;
    gTasks[taskId].func = Task_WaitForOutOfTimeMsg;
}

static void Task_WaitForOutOfTimeMsg(u8 taskId)
{
    if (!Contest_RunTextPrinters())
    {
        SetBgForCurtainDrop();
        gBattle_BG1_X = 0;
        gBattle_BG1_Y = DISPLAY_HEIGHT;
        PlaySE12WithPanning(SE_CONTEST_CURTAIN_FALL, 0);
        gTasks[taskId].data[0] = 0;
        gTasks[taskId].func = Task_DropCurtainAtAppealsEnd;
    }
}

static void Task_DropCurtainAtAppealsEnd(u8 taskId)
{
    gBattle_BG1_Y -= 7;
    if ((s16)gBattle_BG1_Y < 0)
        gBattle_BG1_Y = 0;
    if (gBattle_BG1_Y == 0)
    {
        gTasks[taskId].func = Task_TryCommunicateFinalStandings;
        gTasks[taskId].data[0] = 0;
    }
}

static void Task_TryCommunicateFinalStandings(u8 taskId)
{
    if (gTasks[taskId].data[0]++ >= 50)
    {
        gTasks[taskId].data[0] = 0;
        if (gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK)
        {
            gTasks[taskId].func = Task_CommunicateFinalStandings;
        }
        else
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            gTasks[taskId].func = Task_ContestReturnToField;
        }
    }
}

static void Task_CommunicateFinalStandings(u8 taskId)
{
    u8 taskId2 = CreateTask(Task_LinkContest_CommunicateFinalStandings, 0);

    SetTaskFuncWithFollowupFunc(taskId2, Task_LinkContest_CommunicateFinalStandings, Task_EndCommunicateFinalStandings);
    gTasks[taskId].func = TaskDummy1;
    ContestPrintLinkStandby();
    SetBottomSliderHeartsInvisibility(FALSE);
}

static void Task_EndCommunicateFinalStandings(u8 taskId)
{
    DestroyTask(taskId);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[eContest.mainTaskId].func = Task_ContestReturnToField;
}

static void Task_ContestReturnToField(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        gFieldCallback = FieldCB_ContestReturnToField;
        FreeAllWindowBuffers();
        FreeContestResources();
        FreeMonSpritesGfx();
        SetMainCallback2(CB2_ReturnToField);
    }
}

static void FieldCB_ContestReturnToField(void)
{
    UnlockPlayerFieldControls();
    ScriptContext_Enable();
}

static void TryPutPlayerLast(void)
{
    if (!(gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK))
        gContestPlayerMonIndex = CONTESTANT_COUNT - 1;
}

static bool8 IsPlayerLinkLeader(void)
{
    if (gContestPlayerMonIndex == gContestLinkLeaderIndex)
        return TRUE;
    return FALSE;
}

void CreateContestMonFromParty(u8 partyIndex)
{
    u8 name[max(PLAYER_NAME_LENGTH + 1, POKEMON_NAME_BUFFER_SIZE)];
    u16 heldItem;
    s16 cool;
    s16 beauty;
    s16 cute;
    s16 smart;
    s16 tough;

    StringCopy(name, gSaveBlock2Ptr->playerName);
    if (gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK)
        StripPlayerNameForLinkContest(name);
    memcpy(gContestMons[gContestPlayerMonIndex].trainerName, name, PLAYER_NAME_LENGTH + 1);
    if (gSaveBlock2Ptr->playerGender == MALE)
        gContestMons[gContestPlayerMonIndex].trainerGfxId = OBJ_EVENT_GFX_LINK_BRENDAN;
    else
        gContestMons[gContestPlayerMonIndex].trainerGfxId = OBJ_EVENT_GFX_LINK_MAY;
    gContestMons[gContestPlayerMonIndex].aiFlags = 0;
    gContestMons[gContestPlayerMonIndex].highestRank = 0;
    gContestMons[gContestPlayerMonIndex].species = GetMonData(&gPlayerParty[partyIndex], MON_DATA_SPECIES);
    GetMonData(&gPlayerParty[partyIndex], MON_DATA_NICKNAME, name);
    StringGet_Nickname(name);
    if (gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK)
    {
        StripMonNameForLinkContest(name, GetMonData(&gPlayerParty[partyIndex], MON_DATA_LANGUAGE));
    }
    memcpy(gContestMons[gContestPlayerMonIndex].nickname, name, POKEMON_NAME_LENGTH + 1);
    StringCopy(gContestMons[gContestPlayerMonIndex].nickname, name);
    gContestMons[gContestPlayerMonIndex].cool = GetMonData(&gPlayerParty[partyIndex], MON_DATA_COOL);
    gContestMons[gContestPlayerMonIndex].beauty = GetMonData(&gPlayerParty[partyIndex], MON_DATA_BEAUTY);
    gContestMons[gContestPlayerMonIndex].cute = GetMonData(&gPlayerParty[partyIndex], MON_DATA_CUTE);
    gContestMons[gContestPlayerMonIndex].smart = GetMonData(&gPlayerParty[partyIndex], MON_DATA_SMART);
    gContestMons[gContestPlayerMonIndex].tough = GetMonData(&gPlayerParty[partyIndex], MON_DATA_TOUGH);
    gContestMons[gContestPlayerMonIndex].sheen = GetMonData(&gPlayerParty[partyIndex], MON_DATA_SHEEN);
    gContestMons[gContestPlayerMonIndex].moves[0] = GetMonData(&gPlayerParty[partyIndex], MON_DATA_MOVE1);
    gContestMons[gContestPlayerMonIndex].moves[1] = GetMonData(&gPlayerParty[partyIndex], MON_DATA_MOVE2);
    gContestMons[gContestPlayerMonIndex].moves[2] = GetMonData(&gPlayerParty[partyIndex], MON_DATA_MOVE3);
    gContestMons[gContestPlayerMonIndex].moves[3] = GetMonData(&gPlayerParty[partyIndex], MON_DATA_MOVE4);
    gContestMons[gContestPlayerMonIndex].personality = GetMonData(&gPlayerParty[partyIndex], MON_DATA_PERSONALITY);
    gContestMons[gContestPlayerMonIndex].otId = GetMonData(&gPlayerParty[partyIndex], MON_DATA_OT_ID);

    heldItem = GetMonData(&gPlayerParty[partyIndex], MON_DATA_HELD_ITEM);
    cool   = gContestMons[gContestPlayerMonIndex].cool;
    beauty = gContestMons[gContestPlayerMonIndex].beauty;
    cute   = gContestMons[gContestPlayerMonIndex].cute;
    smart  = gContestMons[gContestPlayerMonIndex].smart;
    tough  = gContestMons[gContestPlayerMonIndex].tough;
    if      (heldItem == ITEM_RED_SCARF)
        cool += 20;
    else if (heldItem == ITEM_BLUE_SCARF)
        beauty += 20;
    else if (heldItem == ITEM_PINK_SCARF)
        cute += 20;
    else if (heldItem == ITEM_GREEN_SCARF)
        smart += 20;
    else if (heldItem == ITEM_YELLOW_SCARF)
        tough += 20;
    if (cool > 255)
        cool = 255;
    if (beauty > 255)
        beauty = 255;
    if (cute > 255)
        cute = 255;
    if (smart > 255)
        smart = 255;
    if (tough > 255)
        tough = 255;
    gContestMons[gContestPlayerMonIndex].cool = cool;
    gContestMons[gContestPlayerMonIndex].beauty = beauty;
    gContestMons[gContestPlayerMonIndex].cute = cute;
    gContestMons[gContestPlayerMonIndex].smart = smart;
    gContestMons[gContestPlayerMonIndex].tough = tough;
}

void SetContestants(u8 contestType, u8 rank)
{
    s32 i;
    u8 opponentsCount = 0;
    u8 opponents[100];
    bool8 allowPostgameContestants = FALSE;
    const u8 * filter;

    TryPutPlayerLast();

    if (FlagGet(FLAG_SYS_GAME_CLEAR) && !(gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK))
        allowPostgameContestants = TRUE;

    // Find all suitable opponents
    filter = gPostgameContestOpponentFilter;
    for (i = 0; i < ARRAY_COUNT(gContestOpponents); i++)
    {
        if (rank == gContestOpponents[i].whichRank)
        {
            if (allowPostgameContestants == TRUE)
            {
                if (filter[i] == CONTEST_FILTER_NO_POSTGAME)
                    continue;
            }
            else
            {
                if (filter[i] == CONTEST_FILTER_ONLY_POSTGAME)
                    continue;
            }
            if      (contestType == CONTEST_CATEGORY_COOL && gContestOpponents[i].aiPool_Cool)
                opponents[opponentsCount++] = i;
            else if (contestType == CONTEST_CATEGORY_BEAUTY && gContestOpponents[i].aiPool_Beauty)
                opponents[opponentsCount++] = i;
            else if (contestType == CONTEST_CATEGORY_CUTE && gContestOpponents[i].aiPool_Cute)
                opponents[opponentsCount++] = i;
            else if (contestType == CONTEST_CATEGORY_SMART && gContestOpponents[i].aiPool_Smart)
                opponents[opponentsCount++] = i;
            else if (contestType == CONTEST_CATEGORY_TOUGH && gContestOpponents[i].aiPool_Tough)
                opponents[opponentsCount++] = i;
        }
    }
    opponents[opponentsCount] = CONTESTANT_NONE;

    // Choose three random opponents from the list
    for (i = 0; i < CONTESTANT_COUNT - 1; i++)
    {
        u16 rnd = Random() % opponentsCount;
        s32 j;

        gContestMons[i] = gContestOpponents[opponents[rnd]];
        for (j = rnd; opponents[j] != CONTESTANT_NONE; j++)
            opponents[j] = opponents[j + 1];
        opponentsCount--;
    }

    CreateContestMonFromParty(gContestMonPartyIndex);
}

void SetLinkAIContestants(u8 contestType, u8 rank, bool32 isPostgame)
{
    s32 i, j;
    u8 opponentsCount = 0;
    u8 opponents[100];

    if (gNumLinkContestPlayers == CONTESTANT_COUNT)
        return;

    // Find all suitable AI opponents
    for (i = 0; i < ARRAY_COUNT(gContestOpponents); i++)
    {
        if (rank != gContestOpponents[i].whichRank)
            continue;

        if (isPostgame == TRUE)
        {
            if (gPostgameContestOpponentFilter[i] == CONTEST_FILTER_NO_POSTGAME)
                continue;
        }
        else
        {
            if (gPostgameContestOpponentFilter[i] == CONTEST_FILTER_ONLY_POSTGAME)
                continue;
        }
        if ((contestType == CONTEST_CATEGORY_COOL && gContestOpponents[i].aiPool_Cool)
            || (contestType == CONTEST_CATEGORY_BEAUTY && gContestOpponents[i].aiPool_Beauty)
            || (contestType == CONTEST_CATEGORY_CUTE && gContestOpponents[i].aiPool_Cute)
            || (contestType == CONTEST_CATEGORY_SMART && gContestOpponents[i].aiPool_Smart)
            || (contestType == CONTEST_CATEGORY_TOUGH && gContestOpponents[i].aiPool_Tough))
            opponents[opponentsCount++] = i;
    }
    opponents[opponentsCount] = CONTESTANT_NONE;

    // Fill remaining contestant slots with random AI opponents from the list
    for (i = 0; i < CONTESTANT_COUNT - gNumLinkContestPlayers; i++)
    {
        u16 rnd = GetContestRand() % opponentsCount;

        gContestMons[gNumLinkContestPlayers + i] = gContestOpponents[opponents[rnd]];
        StripPlayerNameForLinkContest(gContestMons[gNumLinkContestPlayers + i].trainerName);
        StripMonNameForLinkContest(gContestMons[gNumLinkContestPlayers + i].nickname, GAME_LANGUAGE);
        for (j = rnd; opponents[j] != CONTESTANT_NONE; j++)
            opponents[j] = opponents[j + 1];
        opponentsCount--;
    }
}

u8 GetContestEntryEligibility(struct Pokemon *pkmn)
{
    u8 ribbon;
    u8 eligibility;

    if (GetMonData(pkmn, MON_DATA_IS_EGG))
        return CANT_ENTER_CONTEST_EGG;
    if (GetMonData(pkmn, MON_DATA_HP) == 0)
        return CANT_ENTER_CONTEST_FAINTED;
    switch (gSpecialVar_ContestCategory)
    {
    case CONTEST_CATEGORY_COOL:
        ribbon = GetMonData(pkmn, MON_DATA_COOL_RIBBON);
        break;
    case CONTEST_CATEGORY_BEAUTY:
        ribbon = GetMonData(pkmn, MON_DATA_BEAUTY_RIBBON);
        break;
    case CONTEST_CATEGORY_CUTE:
        ribbon = GetMonData(pkmn, MON_DATA_CUTE_RIBBON);
        break;
    case CONTEST_CATEGORY_SMART:
        ribbon = GetMonData(pkmn, MON_DATA_SMART_RIBBON);
        break;
    case CONTEST_CATEGORY_TOUGH:
        ribbon = GetMonData(pkmn, MON_DATA_TOUGH_RIBBON);
        break;
    default:
        return CANT_ENTER_CONTEST;
    }

    // Couldn't get this to match any other way.
    // Returns 2, 1, or 0 respectively if ribbon's rank is above, equal, or below
    // the current contest rank.
    if (ribbon > gSpecialVar_ContestRank)
        eligibility = CAN_ENTER_CONTEST_HIGH_RANK;
    else if (ribbon >= gSpecialVar_ContestRank)
        eligibility = CAN_ENTER_CONTEST_EQUAL_RANK;
    else
        eligibility = CANT_ENTER_CONTEST;
    return eligibility;
}

static void DrawContestantWindowText(void)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        FillWindowPixelBuffer(gContestantTurnOrder[i], PIXEL_FILL(0));
        PrintContestantTrainerName(i);
        PrintContestantMonName(i);
    }
}

static u8 *Contest_CopyStringWithColor(const u8 *string, u8 color)
{
    u8 * ptr = StringCopy(gDisplayedStringBattle, gText_ColorTransparent);
    ptr[-1] = color; // Overwrites the "{COLOR TRANSPARENT}" part of the string.
    ptr = StringCopy(ptr, string);

    return ptr;
}

static void PrintContestantTrainerName(u8 contestant)
{
    PrintContestantTrainerNameWithColor(contestant, contestant + CONTESTANT_TEXT_COLOR_START);
}

static void PrintContestantTrainerNameWithColor(u8 contestant, u8 color)
{
    u8 buffer[32];
    s32 offset;

    StringCopy(buffer, gText_Slash);
    StringAppend(buffer, gContestMons[contestant].trainerName);
    Contest_CopyStringWithColor(buffer, color);
    offset = GetStringRightAlignXOffset(FONT_NARROW, gDisplayedStringBattle, 0x60);
    if (offset > 55)
        offset = 55;
    Contest_PrintTextToBg0WindowAt(gContestantTurnOrder[contestant], gDisplayedStringBattle, offset, 1, FONT_NARROW);
}

static void PrintContestantMonName(u8 contestant)
{
    PrintContestantMonNameWithColor(contestant, contestant + CONTESTANT_TEXT_COLOR_START);
}

static void PrintContestantMonNameWithColor(u8 contestant, u8 color)
{
    Contest_CopyStringWithColor(gContestMons[contestant].nickname, color);
    Contest_PrintTextToBg0WindowAt(gContestantTurnOrder[contestant], gDisplayedStringBattle, 5, 1, FONT_NARROW);
}

static u16 CalculateContestantRound1Points(u8 who, u8 contestCategory)
{
    u8 statMain;
    u8 statSub1;
    u8 statSub2;

    switch (contestCategory)
    {
    case CONTEST_CATEGORY_COOL:
        statMain = gContestMons[who].cool;
        statSub1 = gContestMons[who].tough;
        statSub2 = gContestMons[who].beauty;
        break;
    case CONTEST_CATEGORY_BEAUTY:
        statMain = gContestMons[who].beauty;
        statSub1 = gContestMons[who].cool;
        statSub2 = gContestMons[who].cute;
        break;
    case CONTEST_CATEGORY_CUTE:
        statMain = gContestMons[who].cute;
        statSub1 = gContestMons[who].beauty;
        statSub2 = gContestMons[who].smart;
        break;
    case CONTEST_CATEGORY_SMART:
        statMain = gContestMons[who].smart;
        statSub1 = gContestMons[who].cute;
        statSub2 = gContestMons[who].tough;
        break;
    case CONTEST_CATEGORY_TOUGH:
    default:
        statMain = gContestMons[who].tough;
        statSub1 = gContestMons[who].smart;
        statSub2 = gContestMons[who].cool;
        break;
    }
    return statMain + (statSub1 + statSub2 + gContestMons[who].sheen) / 2;
}

void CalculateRound1Points(u8 contestCategory)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
        gContestMonRound1Points[i] = CalculateContestantRound1Points(i, contestCategory);
}

static u8 CreateJudgeSprite(void)
{
    u8 spriteId;

    LoadCompressedSpriteSheet(&sSpriteSheet_Judge);
    LoadCompressedPalette(gContest2Pal, OBJ_PLTT_ID(1), PLTT_SIZE_4BPP);
    spriteId = CreateSprite(&sSpriteTemplate_Judge, 112, 36, 30);
    gSprites[spriteId].oam.paletteNum = 1;
    gSprites[spriteId].callback = SpriteCallbackDummy;
    return spriteId;
}

static u8 CreateJudgeSpeechBubbleSprite(void)
{
    u8 spriteId;

    LoadCompressedSpriteSheet(&sSpriteSheet_JudgeSymbols);
    LoadCompressedSpritePalette(&sSpritePalette_JudgeSymbols);
    spriteId = CreateSprite(&sSpriteTemplate_JudgeSpeechBubble, 96, 10, 29);
    gSprites[spriteId].invisible = TRUE;
    gSprites[spriteId].data[0] = gSprites[spriteId].oam.tileNum;
    return spriteId;
}

static u8 CreateContestantSprite(u16 species, u32 otId, u32 personality, u32 index)
{
    u8 spriteId;
    species = SanitizeSpecies(species);

    if (index == gContestPlayerMonIndex)
        HandleLoadSpecialPokePic_2(&gMonBackPicTable[species], gMonSpritesGfxPtr->sprites.ptr[B_POSITION_PLAYER_LEFT], species, personality);
    else
        HandleLoadSpecialPokePic_DontHandleDeoxys(&gMonBackPicTable[species], gMonSpritesGfxPtr->sprites.ptr[B_POSITION_PLAYER_LEFT], species, personality);

    LoadCompressedPalette(GetMonSpritePalFromSpeciesAndPersonality(species, otId, personality), OBJ_PLTT_ID(2), PLTT_SIZE_4BPP);
    SetMultiuseSpriteTemplateToPokemon(species, B_POSITION_PLAYER_LEFT);

    spriteId = CreateSprite(&gMultiuseSpriteTemplate, 0x70, GetBattlerSpriteFinal_Y(2, species, FALSE), 30);
    gSprites[spriteId].oam.paletteNum = 2;
    gSprites[spriteId].oam.priority = 2;
    gSprites[spriteId].subpriority = GetBattlerSpriteSubpriority(2);
    gSprites[spriteId].callback = SpriteCallbackDummy;
    gSprites[spriteId].data[0] = gSprites[spriteId].oam.paletteNum;
    gSprites[spriteId].data[2] = species;
    if (IsSpeciesNotUnown(species))
        gSprites[spriteId].affineAnims = gAffineAnims_BattleSpriteContest;
    else
        gSprites[spriteId].affineAnims = gAffineAnims_BattleSpriteOpponentSide;
    StartSpriteAffineAnim(&gSprites[spriteId], BATTLER_AFFINE_NORMAL);

    return spriteId;
}

bool8 IsSpeciesNotUnown(u16 species)
{
    if (species == SPECIES_UNOWN)
        return FALSE;
    else
        return TRUE;
}

// The contestant info windows and general-purpose text box are drawn on one half, while
// the moves and move description windows are drawn on another screen. Only the first 32 * 20
// tiles are actually drawn on screen.
static void SwapMoveDescAndContestTilemaps(void)
{
    CpuCopy16(gContestResources->contestBgTilemaps[0], gContestResources->contestBgTilemaps[0] + 0x500, 32 * 20);
    CpuCopy16(gContestResources->contestBgTilemaps[2], gContestResources->contestBgTilemaps[2] + 0x500, 32 * 20);
}

// Functionally unused
static u16 GetMoveEffectSymbolTileOffset(u16 move, u8 contestant)
{
    u16 offset;

    switch (gContestEffects[gContestMoves[move].effect].effectType)
    {
    case CONTEST_EFFECT_TYPE_APPEAL:
    case CONTEST_EFFECT_TYPE_AVOID_STARTLE:
    case CONTEST_EFFECT_TYPE_UNKNOWN:
        offset = 0x9082;
        break;
    case CONTEST_EFFECT_TYPE_STARTLE_MON:
    case CONTEST_EFFECT_TYPE_STARTLE_MONS:
        offset = 0x9088;
        break;
    default:
    //case CONTEST_EFFECT_TYPE_WORSEN:
    //case CONTEST_EFFECT_TYPE_SPECIAL_APPEAL:
    //case CONTEST_EFFECT_TYPE_TURN_ORDER:
        offset = 0x9086;
        break;
    }
    offset += 0x9000 + (contestant << 12);
    return offset;
}

static void PrintContestMoveDescription(u16 move)
{
    u8 category;
    u16 categoryTile;
    u8 numHearts;

    // The contest category icon is implemented as a 5x2 group of tiles.
    category = gContestMoves[move].contestCategory;
    if      (category == CONTEST_CATEGORY_COOL)
        categoryTile = 0x4040;
    else if (category == CONTEST_CATEGORY_BEAUTY)
        categoryTile = 0x4045;
    else if (category == CONTEST_CATEGORY_CUTE)
        categoryTile = 0x404A;
    else if (category == CONTEST_CATEGORY_SMART)
        categoryTile = 0x406A;
    else
        categoryTile = 0x408A;

    ContestBG_FillBoxWithIncrementingTile(0, categoryTile,        0x0b, 0x1f, 0x05, 0x01, 0x11, 0x01);
    ContestBG_FillBoxWithIncrementingTile(0, categoryTile + 0x10, 0x0b, 0x20, 0x05, 0x01, 0x11, 0x01);

    // Appeal hearts
    if (gContestEffects[gContestMoves[move].effect].appeal == 0xFF)
        numHearts = 0;
    else
        numHearts = gContestEffects[gContestMoves[move].effect].appeal / 10;
    if (numHearts > MAX_CONTEST_MOVE_HEARTS)
        numHearts = MAX_CONTEST_MOVE_HEARTS;
    ContestBG_FillBoxWithTile(0, TILE_EMPTY_APPEAL_HEART, 0x15, 0x1f, MAX_CONTEST_MOVE_HEARTS, 0x01, 0x11);
    ContestBG_FillBoxWithTile(0, TILE_FILLED_APPEAL_HEART, 0x15, 0x1f, numHearts, 0x01, 0x11);

    // Jam hearts
    if (gContestEffects[gContestMoves[move].effect].jam == 0xFF)
        numHearts = 0;
    else
        numHearts = gContestEffects[gContestMoves[move].effect].jam / 10;
    if (numHearts > MAX_CONTEST_MOVE_HEARTS)
        numHearts = MAX_CONTEST_MOVE_HEARTS;
    ContestBG_FillBoxWithTile(0, TILE_EMPTY_JAM_HEART, 0x15, 0x20, MAX_CONTEST_MOVE_HEARTS, 0x01, 0x11);
    ContestBG_FillBoxWithTile(0, TILE_FILLED_JAM_HEART, 0x15, 0x20, numHearts, 0x01, 0x11);

    FillWindowPixelBuffer(WIN_MOVE_DESCRIPTION, PIXEL_FILL(0));
    Contest_PrintTextToBg0WindowStd(WIN_MOVE_DESCRIPTION, gContestEffectDescriptionPointers[gContestMoves[move].effect]);
    Contest_PrintTextToBg0WindowStd(WIN_SLASH, gText_Slash);
}

static void DrawMoveEffectSymbol(u16 move, u8 contestant)
{
    u8 contestantOffset = gContestantTurnOrder[contestant] * 5 + 2;

    if (!Contest_IsMonsTurnDisabled(contestant) && move != MOVE_NONE)
    {
        u16 tile = GetMoveEffectSymbolTileOffset(move, contestant);

        ContestBG_FillBoxWithIncrementingTile(0, tile,      20, contestantOffset,     2, 1, 17, 1);
        ContestBG_FillBoxWithIncrementingTile(0, tile + 16, 20, contestantOffset + 1, 2, 1, 17, 1);
    }
    else
    {
        ContestBG_FillBoxWithTile(0, 0, 20, contestantOffset, 2, 2, 17);
    }
}

static void UNUSED DrawMoveEffectSymbols(void)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
        DrawMoveEffectSymbol(eContestantStatus[i].currMove, i);
}

static u16 GetStarTileOffset(void)
{
    return 0x2034;
}

static bool8 UpdateConditionStars(u8 contestantIdx, bool8 resetMod)
{
    u8 contestantOffset;
    s32 numStars;

    if (eContestantStatus[contestantIdx].conditionMod == CONDITION_NO_CHANGE)
        return FALSE;
    contestantOffset = gContestantTurnOrder[contestantIdx] * 5 + 2;
    numStars = eContestantStatus[contestantIdx].condition / 10;
    if (eContestantStatus[contestantIdx].conditionMod == CONDITION_GAIN)
    {
        ContestBG_FillBoxWithTile(0, GetStarTileOffset(), 19, contestantOffset, 1, numStars, 17);
        if (resetMod)
        {
            PlaySE(SE_EXP_MAX);
            eContestantStatus[contestantIdx].conditionMod = CONDITION_NO_CHANGE;
        }
    }
    else // CONDITION_LOSE
    {
        ContestBG_FillBoxWithTile(0, 0, 19, contestantOffset + numStars, 1, 3 - numStars, 17);
        if (resetMod)
        {
            PlaySE(SE_CONTEST_CONDITION_LOSE);
            eContestantStatus[contestantIdx].conditionMod = CONDITION_NO_CHANGE;
        }
    }
    return TRUE;
}

static void DrawConditionStars(void)
{
    s32 i;
    s32 numStars;

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        u8 contestantOffset = gContestantTurnOrder[i] * 5 + 2;
        u16 starOffset = GetStarTileOffset();

        numStars = eContestantStatus[i].condition / 10;
        ContestBG_FillBoxWithTile(0, starOffset, 19, contestantOffset, 1, numStars, 17);
        ContestBG_FillBoxWithTile(0,  0, 19, contestantOffset + numStars, 1, 3 - numStars, 17);
    }
}

static u16 GetStatusSymbolTileOffset(u8 status)
{
    u16 offset = 0;

    switch (status)
    {
    case STAT_SYMBOL_CIRCLE: // For resistant
        offset = 0x80;
        break;
    case STAT_SYMBOL_WAVE: // For nervous
        offset = 0x84;
        break;
    case STAT_SYMBOL_X: // For turn skipped
        offset = 0x86;
        break;
    case STAT_SYMBOL_SWIRL: // For jammed/unnerved
        offset = 0x88;
        break;
    case STAT_SYMBOL_SQUARE: // Never used
        offset = 0x82;
        break;
    }
    offset += 0x9000;
    return offset;
}

static bool8 DrawStatusSymbol(u8 contestant)
{
    bool8 statused = TRUE;
    u16 symbolOffset = 0;
    u8 contestantOffset = gContestantTurnOrder[contestant] * 5 + 2;

    if (eContestantStatus[contestant].resistant
     || eContestantStatus[contestant].immune
     || eContestantStatus[contestant].jamSafetyCount != 0
     || eContestantStatus[contestant].jamReduction != 0)
        symbolOffset = GetStatusSymbolTileOffset(STAT_SYMBOL_CIRCLE);
    else if (eContestantStatus[contestant].nervous)
        symbolOffset = GetStatusSymbolTileOffset(STAT_SYMBOL_WAVE);
    else if (eContestantStatus[contestant].numTurnsSkipped != 0 || eContestantStatus[contestant].noMoreTurns)
        symbolOffset = GetStatusSymbolTileOffset(STAT_SYMBOL_X);
    else
        statused = FALSE;

    if (statused)
    {
        ContestBG_FillBoxWithIncrementingTile(0, symbolOffset,      20, contestantOffset,     2, 1, 17, 1);
        ContestBG_FillBoxWithIncrementingTile(0, symbolOffset + 16, 20, contestantOffset + 1, 2, 1, 17, 1);
    }
    else
    {
        ContestBG_FillBoxWithTile(0, 0, 20, contestantOffset, 2, 2, 17);
    }
    return statused;
}

static void DrawStatusSymbols(void)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
        DrawStatusSymbol(i);
}

static void ContestClearGeneralTextWindow(void)
{
    FillWindowPixelBuffer(WIN_GENERAL_TEXT, PIXEL_FILL(0));
    CopyWindowToVram(WIN_GENERAL_TEXT, COPYWIN_GFX);
    Contest_SetBgCopyFlags(0);
}

static u16 GetChosenMove(u8 contestant)
{
    if (Contest_IsMonsTurnDisabled(contestant))
        return MOVE_NONE;
    if (contestant == gContestPlayerMonIndex)
    {
        return gContestMons[contestant].moves[eContest.playerMoveChoice];
    }
    else
    {
        u8 moveChoice;

        ContestAI_ResetAI(contestant);
        moveChoice = ContestAI_GetActionToUse();
        return gContestMons[contestant].moves[moveChoice];
    }
}

static void GetAllChosenMoves(void)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
        eContestantStatus[i].currMove = GetChosenMove(i);
}

static void RankContestants(void)
{
    s32 i;
    s32 j;
    s16 arr[CONTESTANT_COUNT];

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        eContestantStatus[i].pointTotal += eContestantStatus[i].appeal;
        arr[i] = eContestantStatus[i].pointTotal;
    }

    // Sort the point totals using bubble-sort.
    for (i = 0; i < CONTESTANT_COUNT - 1; i++)
    {
        for (j = CONTESTANT_COUNT - 1; j > i; j--)
        {
            if (arr[j - 1] < arr[j])
            {
                u16 temp;
                SWAP(arr[j], arr[j - 1], temp);
            }
        }
    }

    // For each contestant, find the best rank with their point total.
    // Normally, each point total is different, and this will output the
    // rankings as expected. However, if two Pokémon are tied, then they
    // both get the best rank for that point total.
    //
    // For example if the point totals are [100, 80, 80, 50], the ranks will
    // be [1, 2, 2, 4]. The Pokémon with a point total of 80 stop looking
    // when they see the first 80 in the array, so they both share the '2'
    // rank.
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        for (j = 0; j < CONTESTANT_COUNT; j++)
        {
            if (eContestantStatus[i].pointTotal == arr[j])
            {
                eContestantStatus[i].ranking = j;
                break;
            }
        }
    }
    SortContestants(TRUE);
    ApplyNextTurnOrder();
}

static void SetAttentionLevels(void)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        u8 attentionLevel;

        if (eContestantStatus[i].currMove == MOVE_NONE)
            attentionLevel = 5;
        else if (eContestantStatus[i].appeal <= 0)
            attentionLevel = 0;
        else if (eContestantStatus[i].appeal < 30)
            attentionLevel = 1;
        else if (eContestantStatus[i].appeal < 60)
            attentionLevel = 2;
        else if (eContestantStatus[i].appeal < 80)
            attentionLevel = 3;
        else
            attentionLevel = 4;

        eContestantStatus[i].attentionLevel = attentionLevel;
    }
}

static bool8 ContestantCanUseTurn(u8 contestant)
{
    if (eContestantStatus[contestant].numTurnsSkipped != 0 || eContestantStatus[contestant].noMoreTurns)
        return FALSE;
    else
        return TRUE;
}

static void SetContestantStatusesForNextRound(void)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        eContestantStatus[i].appeal = 0;
        eContestantStatus[i].baseAppeal = 0;
        eContestantStatus[i].jamSafetyCount = 0;
        if (eContestantStatus[i].numTurnsSkipped > 0)
            eContestantStatus[i].numTurnsSkipped--;
        eContestantStatus[i].jam = 0;
        eContestantStatus[i].resistant = 0;
        eContestantStatus[i].jamReduction = 0;
        eContestantStatus[i].immune = 0;
        eContestantStatus[i].moreEasilyStartled = 0;
        eContestantStatus[i].usedRepeatableMove = 0;
        eContestantStatus[i].nervous = FALSE;
        eContestantStatus[i].effectStringId = CONTEST_STRING_NONE;
        eContestantStatus[i].effectStringId2 = CONTEST_STRING_NONE;
        eContestantStatus[i].conditionMod = CONDITION_NO_CHANGE;
        eContestantStatus[i].repeatedPrevMove = eContestantStatus[i].repeatedMove;
        eContestantStatus[i].repeatedMove = FALSE;
        eContestantStatus[i].turnOrderModAction = 0;
        eContestantStatus[i].appealTripleCondition = 0;
        if (eContestantStatus[i].turnSkipped)
        {
            eContestantStatus[i].numTurnsSkipped = 1;
            eContestantStatus[i].turnSkipped = 0;
        }
        if (eContestantStatus[i].exploded)
        {
            eContestantStatus[i].noMoreTurns = TRUE;
            eContestantStatus[i].exploded = FALSE;
        }
        eContestantStatus[i].overrideCategoryExcitementMod = 0;
    }
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        eContestantStatus[i].prevMove = eContestantStatus[i].currMove;
        eContest.moveHistory[eContest.appealNumber][i] = eContestantStatus[i].currMove;
        eContest.excitementHistory[eContest.appealNumber][i] = Contest_GetMoveExcitement(eContestantStatus[i].currMove);
        eContestantStatus[i].currMove = MOVE_NONE;
    }
    eContestExcitement.frozen = FALSE;
}

bool8 Contest_IsMonsTurnDisabled(u8 contestant)
{
    if (eContestantStatus[contestant].numTurnsSkipped != 0 || eContestantStatus[contestant].noMoreTurns)
        return TRUE;
    else
        return FALSE;
}

static void CalculateTotalPointsForContestant(u8 contestant)
{
    gContestMonRound2Points[contestant] = GetContestantRound2Points(contestant);
    gContestMonTotalPoints[contestant] = gContestMonRound1Points[contestant] + gContestMonRound2Points[contestant];
}

static void CalculateFinalScores(void)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
        CalculateTotalPointsForContestant(i);
    DetermineFinalStandings();
}

static s16 GetContestantRound2Points(u8 contestant)
{
    return gContestMonAppealPointTotals[contestant] * 2;
}

static void DetermineFinalStandings(void)
{
    u16 randomOrdering[CONTESTANT_COUNT] = {0};
    struct ContestFinalStandings standings[CONTESTANT_COUNT];
    s32 i;

    // Seed random order in case of ties
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        s32 j;
        randomOrdering[i] = Random();
        for (j = 0; j < i; j++)
        {
            if (randomOrdering[i] == randomOrdering[j])
            {
                i--;
                break;
            }
        }
    }

    // Init data for ranking contestants
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        standings[i].totalPoints = gContestMonTotalPoints[i];
        standings[i].round1Points = gContestMonRound1Points[i];
        standings[i].random = randomOrdering[i];
        standings[i].contestant = i;
    }

    // Rank contestants
    for (i = 0; i < CONTESTANT_COUNT - 1; i++)
    {
        s32 j;
        for (j = CONTESTANT_COUNT - 1; j > i; j--)
        {
            if (DidContestantPlaceHigher(j - 1, j, standings))
            {
                // Swap contestants in array
                struct ContestFinalStandings temp;

                temp.totalPoints  = standings[j - 1].totalPoints;
                temp.round1Points = standings[j - 1].round1Points;
                temp.random       = standings[j - 1].random;
                temp.contestant   = standings[j - 1].contestant;

                standings[j - 1].totalPoints  = standings[j].totalPoints;
                standings[j - 1].round1Points = standings[j].round1Points;
                standings[j - 1].random       = standings[j].random;
                standings[j - 1].contestant   = standings[j].contestant;

                standings[j].totalPoints  = temp.totalPoints;
                standings[j].round1Points = temp.round1Points;
                standings[j].random       = temp.random;
                standings[j].contestant   = temp.contestant;
            }
        }
    }

    // Assign placements. i is the placing (0 is 1st, 1 is 2nd...)
    for (i = 0; i < CONTESTANT_COUNT; i++)
        gContestFinalStandings[standings[i].contestant] = i;
}

void SaveLinkContestResults(void)
{
    if ((gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK))
    {
        gSaveBlock2Ptr->contestLinkResults[gSpecialVar_ContestCategory][gContestFinalStandings[gContestPlayerMonIndex]] =
        ((gSaveBlock2Ptr->contestLinkResults[gSpecialVar_ContestCategory][gContestFinalStandings[gContestPlayerMonIndex]] + 1) > 9999) ? 9999 :
        (gSaveBlock2Ptr->contestLinkResults[gSpecialVar_ContestCategory][gContestFinalStandings[gContestPlayerMonIndex]] + 1);

    }
}

static bool8 DidContestantPlaceHigher(s32 a, s32 b, struct ContestFinalStandings *standings)
{
    bool8 retVal;

    // Rank contestants first based on total points
    if (standings[a].totalPoints < standings[b].totalPoints)
        retVal = TRUE;
    else if (standings[a].totalPoints > standings[b].totalPoints)
        retVal = FALSE;
    // If tied, rank on round 1 points
    else if (standings[a].round1Points < standings[b].round1Points)
        retVal = TRUE;
    else if (standings[a].round1Points > standings[b].round1Points)
        retVal = FALSE;
    // If tied again, choose randomly
    else if (standings[a].random < standings[b].random)
        retVal = TRUE;
    else
        retVal = FALSE;
    return retVal;
}

static void ContestPrintLinkStandby(void)
{
    gBattle_BG0_Y = 0;
    gBattle_BG2_Y = 0;
    ContestClearGeneralTextWindow();
    Contest_StartTextPrinter(gText_LinkStandby4, FALSE);
}

static void FillContestantWindowBgs(void)
{
    int i;

    for(i = 0; i < CONTESTANT_COUNT; i++)
        ContestBG_FillBoxWithTile(0, 0, 0x16, 2 + i * 5, 8, 2, 0x11);
}

static u16 GetAppealHeartTileOffset(u8 contestant)
{
    u16 offset;

    if (contestant == 0)
        offset = 0x5011;
    else if (contestant == 1)
        offset = 0x6011;
    else if (contestant == 2)
        offset = 0x7011;
    else
        offset = 0x8011;
    return offset + 1;
}

static s8 GetNumHeartsFromAppealPoints(s16 appeal)
{
    s8 hearts = appeal / 10;

    if (hearts > 16)
        hearts = 16;
    else if (hearts < -16)
        hearts = -16;
    return hearts;
}

#define tNumHearts   data[0]
#define tHeartsDelta data[1]
#define tHeartsSign  data[2]
#define tContestant  data[3]
#define tDelayTimer  data[10]

static u8 UpdateAppealHearts(s16 startAppeal, s16 appealDelta, u8 contestant)
{
    u8 taskId;
    s8 startHearts;
    s8 heartsDelta;

    eContestGfxState[contestant].updatingAppealHearts = TRUE;
    taskId = CreateTask(Task_UpdateAppealHearts, 20);
    startHearts = GetNumHeartsFromAppealPoints(startAppeal);
    heartsDelta = GetNumHeartsFromAppealPoints(startAppeal + appealDelta) - startHearts;
    GetAppealHeartTileOffset(contestant);  // unused return value
    gTasks[taskId].tNumHearts = abs(startHearts);
    gTasks[taskId].tHeartsDelta = heartsDelta;
    if (startHearts > 0 || (startHearts == 0 && heartsDelta > 0))
        gTasks[taskId].tHeartsSign = 1;
    else
        gTasks[taskId].tHeartsSign = -1;
    gTasks[taskId].tContestant = contestant;
    return taskId;
}

static void Task_UpdateAppealHearts(u8 taskId)
{
    u8 contestant = gTasks[taskId].tContestant;
    s16 startHearts = gTasks[taskId].tNumHearts;
    s16 heartsDelta = gTasks[taskId].tHeartsDelta;

    if (++gTasks[taskId].tDelayTimer > 14)
    {
        u16 heartOffset;
        u8 newNumHearts;
        u8 pitchMod;
        bool8 onSecondLine;

        gTasks[taskId].tDelayTimer = 0;
        if (gTasks[taskId].tHeartsDelta == 0)
        {
            // No more hearts to add/remove, end
            DestroyTask(taskId);
            eContestGfxState[contestant].updatingAppealHearts = FALSE;
            return;
        }
        else if (startHearts == 0)
        {
            if (heartsDelta < 0)
            {
                // Losing hearts, get black heart offset
                heartOffset = GetAppealHeartTileOffset(contestant) + 2;
                gTasks[taskId].tHeartsDelta++;
            }
            else
            {
                // Gaining hearts, get red heart offset
                heartOffset = GetAppealHeartTileOffset(contestant);
                gTasks[taskId].tHeartsDelta--;
            }
            newNumHearts = gTasks[taskId].tNumHearts++;
        }
        else
        {
            if (gTasks[taskId].tHeartsSign < 0)
            {
                // Hearts currently black (negative)
                if (heartsDelta < 0)
                {
                    // Losing points, add black heart
                    newNumHearts = gTasks[taskId].tNumHearts++;
                    gTasks[taskId].tHeartsDelta++;
                    heartOffset = GetAppealHeartTileOffset(contestant) + 2;
                }
                else
                {
                    // Gaining points, remove black heart
                    newNumHearts = --gTasks[taskId].tNumHearts;
                    heartOffset = 0;
                    gTasks[taskId].tHeartsDelta--;
                }
            }
            else
            {
                // Hearts currently red (positive)
                if (heartsDelta < 0)
                {
                    // Losing points, remove red heart
                    newNumHearts = --gTasks[taskId].tNumHearts;
                    heartOffset = 0;
                    gTasks[taskId].tHeartsDelta++;
                }
                else
                {
                    // Gaining points, add red heart
                    newNumHearts = gTasks[taskId].tNumHearts++;
                    gTasks[taskId].tHeartsDelta--;
                    heartOffset = GetAppealHeartTileOffset(contestant);
                }
            }
        }
        pitchMod = newNumHearts;
        onSecondLine = FALSE;

        // Check if wrapping to second line of hearts
        if (newNumHearts > 7)
        {
            onSecondLine = TRUE;
            newNumHearts -= 8;
        }
        ContestBG_FillBoxWithTile(0, heartOffset, newNumHearts + 22, gContestantTurnOrder[contestant] * 5 + 2 + onSecondLine,  1, 1, 17);
        if (heartsDelta > 0)
        {
            PlaySE(SE_CONTEST_HEART);
            m4aMPlayImmInit(&gMPlayInfo_SE1);
            m4aMPlayPitchControl(&gMPlayInfo_SE1, TRACKS_ALL, pitchMod * 256);
        }
        else
        {
            PlaySE(SE_BOO);
        }

        if (!onSecondLine && newNumHearts == 0 && heartOffset == 0)
            gTasks[taskId].tHeartsSign = -gTasks[taskId].tHeartsSign;
    }
}

static void CreateSliderHeartSprites(void)
{
    s32 i;

    LoadSpriteSheet(&sSpriteSheet_SliderHeart);
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        u8 y = sSliderHeartYPositions[gContestantTurnOrder[i]];

        eContestGfxState[i].sliderHeartSpriteId = CreateSprite(&sSpriteTemplate_SliderHeart, 180, y, 1);
    }
}

#define sContestant data[0]
#define sTargetX    data[1]
#define sMoveX      data[2]

static void UpdateHeartSlider(u8 contestant)
{
    u8 spriteId;
    s16 slideTarget;

    eContestGfxState[contestant].sliderUpdating = TRUE;
    spriteId = eContestGfxState[contestant].sliderHeartSpriteId;
    slideTarget = eContestantStatus[contestant].pointTotal / 10 * 2;
    if (slideTarget > 56)
        slideTarget = 56;
    else if (slideTarget < 0)
        slideTarget = 0;
    gSprites[spriteId].invisible = FALSE;
    gSprites[spriteId].sContestant = contestant;
    gSprites[spriteId].sTargetX = slideTarget;
    if (gSprites[spriteId].sTargetX > gSprites[spriteId].x2)
        gSprites[spriteId].sMoveX = 1;
    else
        gSprites[spriteId].sMoveX = -1;
    gSprites[spriteId].callback = SpriteCB_UpdateHeartSlider;
}

static void UpdateHeartSliders(void)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
        UpdateHeartSlider(i);
}

static bool8 SlidersDoneUpdating(void)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        if (eContestGfxState[i].sliderUpdating)
            break;
    }
    if (i == CONTESTANT_COUNT)
        return TRUE;
    else
        return FALSE;
}

static void SpriteCB_UpdateHeartSlider(struct Sprite *sprite)
{
    if (sprite->x2 == sprite->sTargetX)
    {
        eContestGfxState[sprite->sContestant].sliderUpdating = FALSE;
        sprite->callback = SpriteCallbackDummy;
    }
    else
    {
        sprite->x2 += sprite->sMoveX;
    }
}

#undef sContestant
#undef sTargetX
#undef sMoveX

// Y positions change as the contestants change order
static void UpdateSliderHeartSpriteYPositions(void)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
        gSprites[eContestGfxState[i].sliderHeartSpriteId].y = sSliderHeartYPositions[gContestantTurnOrder[i]];
}

// Used to hide (or subsequently reshow) the bottom two slider hearts that get hidden by text windows by moving them offscreen
static void SetBottomSliderHeartsInvisibility(bool8 invisible)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        // Skip the top two contestants sliders
        if (gContestantTurnOrder[i] > 1)
        {
            if (!invisible)
                gSprites[eContestGfxState[i].sliderHeartSpriteId].x = 180;
            else
                gSprites[eContestGfxState[i].sliderHeartSpriteId].x = 256;
        }
    }
}

static void CreateNextTurnSprites(void)
{
    s32 i;

    LoadSpritePalette(&sSpritePalette_NextTurn);
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        LoadCompressedSpriteSheet(&sSpriteSheet_NextTurn[i]);
        eContestGfxState[i].nextTurnSpriteId = CreateSprite(&sSpriteTemplates_NextTurn[i],
                                                           204,
                                                           sNextTurnSpriteYPositions[gContestantTurnOrder[i]],
                                                           0);
        SetSubspriteTables(&gSprites[eContestGfxState[i].nextTurnSpriteId], sSubspriteTable_NextTurn);
        gSprites[eContestGfxState[i].nextTurnSpriteId].invisible = TRUE;
    }
}

static void CreateApplauseMeterSprite(void)
{
    u8 spriteId;

    LoadCompressedSpriteSheet(&sSpriteSheet_ApplauseMeter);
    LoadSpritePalette(&sSpritePalette_ApplauseMeter);
    spriteId = CreateSprite(&sSpriteTemplate_ApplauseMeter, 30, 44, 1);
    gSprites[spriteId].invisible = TRUE;
    eContest.applauseMeterSpriteId = spriteId;
}

static void CreateJudgeAttentionEyeTask(void)
{
    u8 i;
    u8 taskId = CreateTask(Task_FlashJudgeAttentionEye, 30);

    eContest.judgeAttentionTaskId = taskId;
    for (i = 0; i < CONTESTANT_COUNT; i++)
        gTasks[taskId].data[i * 4] = 0xFF;
}

static void StartFlashJudgeAttentionEye(u8 contestant)
{
    gTasks[eContest.judgeAttentionTaskId].data[contestant * 4 + 0] = 0;
    gTasks[eContest.judgeAttentionTaskId].data[contestant * 4 + 1] = 0;
}

static void StopFlashJudgeAttentionEye(u8 contestant)
{
    u8 taskId = CreateTask(Task_StopFlashJudgeAttentionEye, 31);
    gTasks[taskId].data[0] = contestant;
}

static void Task_StopFlashJudgeAttentionEye(u8 taskId)
{
    u8 contestant = gTasks[taskId].data[0];

    if (gTasks[eContest.judgeAttentionTaskId].data[contestant * 4 + 0] == 0
     || gTasks[eContest.judgeAttentionTaskId].data[contestant * 4 + 0] == 0xFF)
    {
        gTasks[eContest.judgeAttentionTaskId].data[contestant * 4 + 0] = 0xFF;
        gTasks[eContest.judgeAttentionTaskId].data[contestant * 4 + 1] = 0;
        BlendPalette(BG_PLTT_ID(5 + eContest.prevTurnOrder[contestant]) + 6, 2, 0, RGB(31, 31, 18));
        DestroyTask(taskId);
    }
}

static void Task_FlashJudgeAttentionEye(u8 taskId)
{
    u8 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        u8 offset = i * 4;

        if (gTasks[taskId].data[offset + 0] != 0xFF)
        {
            if (gTasks[taskId].data[offset + 1] == 0)
                gTasks[taskId].data[offset + 0]++;
            else
                gTasks[taskId].data[offset + 0]--;

            if (gTasks[taskId].data[offset + 0] == 16
             || gTasks[taskId].data[offset + 0] == 0)
                gTasks[taskId].data[offset + 1] ^= 1;

            BlendPalette(BG_PLTT_ID(5 + eContest.prevTurnOrder[i]) + 6, 2, gTasks[taskId].data[offset + 0], RGB(31, 31, 18));
        }
    }
}

// Note: While the below task is run for the entire Appeals portion of the contest,
//       because data[i * 4] is always 0xFF it never does anything
//       If turned on by setting that data between 0 and 16, it blends
//       an odd selection of palette colors (e.g. the text box, the appeal hearts
//       for only one contestant, the heart outlines in the move selection box, etc)
//       Given the similarities, it's possible this was an incorrect attempt
//       at something similar to what CreateJudgeAttentionEyeTask does
static void CreateUnusedBlendTask(void)
{
    s32 i;

    eContest.blendTaskId = CreateTask(Task_UnusedBlend, 30);
    for (i = 0; i < CONTESTANT_COUNT; i++)
        InitUnusedBlendTaskData(i);
}

static void InitUnusedBlendTaskData(u8 contestant)
{
    gTasks[eContest.blendTaskId].data[contestant * 4] = 0xFF;
    gTasks[eContest.blendTaskId].data[contestant * 4 + 1] = 0;
}

static void UpdateBlendTaskContestantsData(void)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
        UpdateBlendTaskContestantData(i);
}

static void UpdateBlendTaskContestantData(u8 contestant)
{
    u32 palOffset1;
    u32 palOffset2;

    InitUnusedBlendTaskData(contestant);

    palOffset1 = contestant + 5;
    DmaCopy16Defvars(3,
                     &gPlttBufferUnfaded[PLTT_ID(palOffset1) + 10],
                     &gPlttBufferFaded[PLTT_ID(palOffset1) + 10],
                     PLTT_SIZEOF(1));
    palOffset2 = PLTT_ID(contestant + 5) + 12 + contestant;
    DmaCopy16Defvars(3,
                     &gPlttBufferUnfaded[palOffset2],
                     &gPlttBufferFaded[palOffset2],
                     PLTT_SIZEOF(1));
}

// See comments on CreateUnusedBlendTask
static void Task_UnusedBlend(u8 taskId)
{
    u8 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        u8 idx = i * 4;

        // Below is never true
        if (gTasks[taskId].data[idx] != 0xFF)
        {
            if (++gTasks[taskId].data[idx + 2] > 2)
            {
                gTasks[taskId].data[idx + 2] = 0;

                if (gTasks[taskId].data[idx + 1] == 0)
                    gTasks[taskId].data[idx]++;
                else
                    gTasks[taskId].data[idx]--;

                if (gTasks[taskId].data[idx] == 16
                 || gTasks[taskId].data[idx] == 0)
                    gTasks[taskId].data[idx + 1] ^= 1;

                BlendPalette(BG_PLTT_ID(5 + i) + 10,     1, gTasks[taskId].data[idx + 0], RGB(31, 31, 18));
                BlendPalette(BG_PLTT_ID(5 + i) + 12 + i, 1, gTasks[taskId].data[idx + 0], RGB(31, 31, 18));
            }
        }
    }
}

static void StartStopFlashJudgeAttentionEye(u8 contestant)
{
    if (eContestantStatus[contestant].hasJudgesAttention)
        StartFlashJudgeAttentionEye(contestant);
    else
        StopFlashJudgeAttentionEye(contestant);
}

static u8 CreateContestantBoxBlinkSprites(u8 contestant)
{
    u8 spriteId1, spriteId2;
    u8 x = gContestantTurnOrder[contestant] * 40 + 32;

    LoadCompressedSpriteSheet(&sSpriteSheets_ContestantsTurnBlinkEffect[contestant]);
    LoadSpritePalette(&sSpritePalettes_ContestantsTurnBlinkEffect[contestant]);
    spriteId1 = CreateSprite(&sSpriteTemplates_ContestantsTurnBlinkEffect[contestant], 184, x, 29);
    spriteId2 = CreateSprite(&sSpriteTemplates_ContestantsTurnBlinkEffect[contestant], 248, x, 29);
    gSprites[spriteId2].oam.tileNum += 64;

    CopySpriteTiles(0,
                    3,
                    (void *)VRAM,
                    (u16 *)(BG_SCREEN_ADDR(28) + gContestantTurnOrder[contestant] * 5 * 64 + 0x26),
                    gContestResources->boxBlinkTiles1);

    CopySpriteTiles(0,
                    3, (void *)VRAM,
                    (u16 *)(BG_SCREEN_ADDR(28) + gContestantTurnOrder[contestant] * 5 * 64 + 0x36),
                    gContestResources->boxBlinkTiles2);

    CpuFill32(0, gContestResources->boxBlinkTiles1 + 0x500, 0x300);
    CpuFill32(0, gContestResources->boxBlinkTiles2 + 0x500, 0x300);

    RequestDma3Copy(gContestResources->boxBlinkTiles1,
                    (u8 *)(OBJ_VRAM0 + gSprites[spriteId1].oam.tileNum * 32),
                    0x800,
                    1);

    RequestDma3Copy(gContestResources->boxBlinkTiles2,
                    (u8 *)(OBJ_VRAM0 + gSprites[spriteId2].oam.tileNum * 32),
                    0x800,
                    1);

    gSprites[spriteId1].data[0] = spriteId2;
    gSprites[spriteId2].data[0] = spriteId1;

    gSprites[spriteId1].data[1] = contestant;
    gSprites[spriteId2].data[1] = contestant;

    return spriteId1;
}

static void DestroyContestantBoxBlinkSprites(u8 spriteId)
{
    u8 spriteId2 = gSprites[spriteId].data[0];

    FreeSpriteOamMatrix(&gSprites[spriteId2]);
    DestroySprite(&gSprites[spriteId2]);
    DestroySpriteAndFreeResources(&gSprites[spriteId]);
}

static void SetBlendForContestantBoxBlink(void)
{
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(7, 9));
}

static void ResetBlendForContestantBoxBlink(void)
{
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
}

// To indicate whose turn is up
static void BlinkContestantBox(u8 spriteId, bool8 b)
{
    u8 spriteId2;

    SetBlendForContestantBoxBlink();
    eContestGfxState[gSprites[spriteId].data[1]].boxBlinking = TRUE;
    spriteId2 = gSprites[spriteId].data[0];
    StartSpriteAffineAnim(&gSprites[spriteId], 1);
    StartSpriteAffineAnim(&gSprites[spriteId2], 1);
    gSprites[spriteId].callback = SpriteCB_BlinkContestantBox;
    gSprites[spriteId2].callback = SpriteCallbackDummy;
    if (b == FALSE)
        PlaySE(SE_CONTEST_MONS_TURN);
    else
        PlaySE(SE_PC_LOGIN);
}

static void SpriteCB_BlinkContestantBox(struct Sprite *sprite)
{
    if (sprite->affineAnimEnded)
    {
        u8 spriteId2 = sprite->data[0];

        if (gSprites[spriteId2].affineAnimEnded)
        {
            sprite->invisible = TRUE;
            gSprites[spriteId2].invisible = TRUE;
            sprite->callback = SpriteCB_EndBlinkContestantBox;
        }
    }
}

static void SpriteCB_EndBlinkContestantBox(struct Sprite *sprite)
{
    eContestGfxState[sprite->data[1]].boxBlinking = FALSE;
    DestroyContestantBoxBlinkSprites(sprite->data[0]);
    ResetBlendForContestantBoxBlink();
}

static void UNUSED ContestDebugTogglePointTotal(void)
{
    if(eContestDebugMode == CONTEST_DEBUG_MODE_PRINT_POINT_TOTAL)
        eContestDebugMode = CONTEST_DEBUG_MODE_OFF;
    else
        eContestDebugMode = CONTEST_DEBUG_MODE_PRINT_POINT_TOTAL;

    if(eContestDebugMode == CONTEST_DEBUG_MODE_OFF)
    {
        DrawContestantWindowText();
        SwapMoveDescAndContestTilemaps();
    }
    else
    {
        ContestDebugDoPrint();
    }
}

static void ContestDebugDoPrint(void)
{
    u8 i;
    s16 value;
    u8 *txtPtr;
    u8 text[8];

    if (!gEnableContestDebugging)
        return;

    switch (eContestDebugMode)
    {
    case CONTEST_DEBUG_MODE_OFF:
        break;
    case CONTEST_DEBUG_MODE_PRINT_WINNER_FLAGS:
    case CONTEST_DEBUG_MODE_PRINT_LOSER_FLAGS:
        ContestDebugPrintBitStrings();
        break;
    // The only other possible value is 1, which is only set by ContestDebugTogglePointTotal.
    //
    // case CONTEST_DEBUG_MODE_PRINT_POINT_TOTAL:
    default:
        for (i = 0; i < CONTESTANT_COUNT; i++)
            FillWindowPixelBuffer(i, PIXEL_FILL(0));
        for (i = 0; i < CONTESTANT_COUNT; i++)
        {
            value = eContestantStatus[i].pointTotal;
            txtPtr = text;
            if (eContestantStatus[i].pointTotal < 0)
            {
                value *= -1;
                txtPtr = StringCopy(txtPtr, gText_OneDash);
            }
            ConvertIntToDecimalStringN(txtPtr, value, STR_CONV_MODE_LEFT_ALIGN, 4);
            Contest_PrintTextToBg0WindowAt(gContestantTurnOrder[i], text, 55, 1, FONT_NARROW);
        }
        for (i = 0; i < CONTESTANT_COUNT; i++)
        {
            value = eContestantStatus[i].appeal;
            txtPtr = text;
            if (eContestantStatus[i].appeal < 0)
            {
                value *= -1;
                txtPtr = StringCopy(txtPtr, gText_OneDash);
            }
            ConvertIntToDecimalStringN(txtPtr, value, STR_CONV_MODE_LEFT_ALIGN, 4);
            Contest_PrintTextToBg0WindowAt(gContestantTurnOrder[i], text, 5, 1, FONT_NARROW);
        }
        SwapMoveDescAndContestTilemaps();
        break;
    }
}

void SortContestants(bool8 useRanking)
{
    u8 scratch[CONTESTANT_COUNT];
    u16 randomOrdering[CONTESTANT_COUNT] = {0};
    s32 i;
    s32 v3;

    // Generate a unique random number for each contestant.
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        s32 j;
        randomOrdering[i] = Random();

        // Loop through all the numbers generated so far.
        for (j = 0; j < i; j++)
        {
            if (randomOrdering[i] == randomOrdering[j])
            {
                // This number isn't unique; try generating again.
                i--;
                break;
            }
        }
    }

    if (!useRanking)
    {
        // Order based on the results of the Conditions round using Insertion Sort.
        // Use the randomOrdering to break ties.
        for (i = 0; i < CONTESTANT_COUNT; i++)
        {
            // Append this contestant to the list.
            gContestantTurnOrder[i] = i;

            // Determine where the contestant should be ordered.
            for (v3 = 0; v3 < i; v3++)
            {
                if (gContestMonRound1Points[gContestantTurnOrder[v3]] < gContestMonRound1Points[i]
                 || (gContestMonRound1Points[gContestantTurnOrder[v3]] == gContestMonRound1Points[i] && randomOrdering[gContestantTurnOrder[v3]] < randomOrdering[i]))
                {
                    // Shift everything larger up to make room.
                    s32 j;
                    for (j = i; j > v3; j--)
                        gContestantTurnOrder[j] = gContestantTurnOrder[j - 1];

                    // Insert into the new spot.
                    gContestantTurnOrder[v3] = i;
                    break;
                }
            }

            // This is redundant.
            // Perhaps GF switched from true insertion sort to in-place insertion sort, and forgot to
            // remove this check?
            if (v3 == i)
                gContestantTurnOrder[i] = i;
        }

        // Invert gContestantTurnOrder; above, it was a list of contestant IDs. Now it's a list of turn orderings.
        //
        // For example, if contestant 3 had the first turn, then `gContestantTurnOrder[1] = 3`. The turn is the index,
        // the contestant is the data. After inverting the list, `gContestantTurnOrder[3] = 1`. The contestant is the index,
        // and the turn is the data.
        memcpy(scratch, gContestantTurnOrder, sizeof(scratch));
        for (i = 0; i < CONTESTANT_COUNT; i++)
            gContestantTurnOrder[scratch[i]] = i;
    }
    else
    {
        // Order contestants based on their ranking.
        // If contestants have tied ranking, fill in the next available slot.
        //
        // Note that ranking is calculated so that shared places still take up a ranking
        // space. A ranking like [1, 2, 2, 3] is not possible; it would be [1, 2, 2, 4]
        // instead.
        memset(scratch, CONTESTANT_NONE, sizeof(scratch));
        for (i = 0; i < CONTESTANT_COUNT; i++)
        {
            u8 j = eContestantStatus[i].ranking;

            while (1)
            {
                u8 *ptr = &scratch[j];
                if (*ptr == CONTESTANT_NONE)
                {
                    *ptr = i;
                    gContestantTurnOrder[i] = j;
                    break;
                }
                j++;
            }
        }

        // Randomize the order of contestants with tied rankings using Selection Sort.
        //
        // Look through the array for tied ranks, and use randomOrdering to break the tie.
        // This ensures that contestants with the same rank will be randomly ordered. This
        // uses an in-place slection sort, which involves a lot of extra swapping.
        for (i = 0; i < CONTESTANT_COUNT - 1; i++)
        {
            for (v3 = CONTESTANT_COUNT - 1; v3 > i; v3--)
            {
                if (eContestantStatus[v3 - 1].ranking == eContestantStatus[v3].ranking
                 && gContestantTurnOrder[v3 - 1] < gContestantTurnOrder[v3]
                 && randomOrdering[v3 - 1] < randomOrdering[v3])
                {
                    u8 temp = gContestantTurnOrder[v3];
                    gContestantTurnOrder[v3] = gContestantTurnOrder[v3 - 1];
                    gContestantTurnOrder[v3 - 1] = temp;
                }
            }
        }
    }
}

static void DrawContestantWindows(void)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        s32 windowId = i + 5;
        LoadPalette(eContestTempSave.cachedWindowPalettes[windowId], BG_PLTT_ID(5 + gContestantTurnOrder[i]), sizeof(eContestTempSave.cachedWindowPalettes[0]));
    }
    DrawContestantWindowText();
}

static void CalculateAppealMoveImpact(u8 contestant)
{
    u16 move;
    u8 effect;
    u8 rnd;
    s32 i;

    eContestantStatus[contestant].appeal = 0;
    eContestantStatus[contestant].baseAppeal = 0;
    if (!ContestantCanUseTurn(contestant))
        return;

    move = eContestantStatus[contestant].currMove;
    effect = gContestMoves[move].effect;

    eContestantStatus[contestant].moveCategory = gContestMoves[eContestantStatus[contestant].currMove].contestCategory;
    if (eContestantStatus[contestant].currMove == eContestantStatus[contestant].prevMove && eContestantStatus[contestant].currMove != MOVE_NONE)
    {
        eContestantStatus[contestant].repeatedMove = TRUE;
        eContestantStatus[contestant].moveRepeatCount++;
    }
    else
    {
        eContestantStatus[contestant].moveRepeatCount = 0;
    }
    eContestantStatus[contestant].baseAppeal = gContestEffects[effect].appeal;
    eContestantStatus[contestant].appeal = eContestantStatus[contestant].baseAppeal;
    eContestAppealResults.jam = gContestEffects[effect].jam;
    eContestAppealResults.jam2 = eContestAppealResults.jam;

    eContestAppealResults.contestant = contestant;
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        eContestantStatus[i].jam = 0;
        eContestAppealResults.unnervedPokes[i] = 0;
    }

    if (eContestantStatus[contestant].hasJudgesAttention
        && !AreMovesContestCombo(eContestantStatus[contestant].prevMove, eContestantStatus[contestant].currMove))
        eContestantStatus[contestant].hasJudgesAttention = FALSE;

    gContestEffectFuncs[effect]();

    if (eContestantStatus[contestant].conditionMod == CONDITION_GAIN)
        eContestantStatus[contestant].appeal += eContestantStatus[contestant].condition - 10;
    else if (eContestantStatus[contestant].appealTripleCondition)
        eContestantStatus[contestant].appeal += eContestantStatus[contestant].condition * 3;
    else
        eContestantStatus[contestant].appeal += eContestantStatus[contestant].condition;

    eContestantStatus[contestant].completedCombo = FALSE;
    eContestantStatus[contestant].usedComboMove = FALSE;
    if (IsContestantAllowedToCombo(contestant))
    {
        bool8 completedCombo = AreMovesContestCombo(eContestantStatus[contestant].prevMove, eContestantStatus[contestant].currMove);

        if (completedCombo && eContestantStatus[contestant].hasJudgesAttention)
        {
            eContestantStatus[contestant].completedCombo = completedCombo;
            eContestantStatus[contestant].usedComboMove = TRUE;
            eContestantStatus[contestant].hasJudgesAttention = FALSE;
            eContestantStatus[contestant].comboAppealBonus = eContestantStatus[contestant].baseAppeal * eContestantStatus[contestant].completedCombo;
            eContestantStatus[contestant].completedComboFlag = TRUE; // Redundant with completedCombo, used by AI
        }
        else
        {
            if (gContestMoves[eContestantStatus[contestant].currMove].comboStarterId != 0)
            {
                eContestantStatus[contestant].hasJudgesAttention = TRUE;
                eContestantStatus[contestant].usedComboMove = TRUE;
            }
            else
            {
                eContestantStatus[contestant].hasJudgesAttention = FALSE;
            }
        }
    }
    if (eContestantStatus[contestant].repeatedMove)
        eContestantStatus[contestant].repeatJam = (eContestantStatus[contestant].moveRepeatCount + 1) * 10;

    if (eContestantStatus[contestant].nervous)
    {
        eContestantStatus[contestant].hasJudgesAttention = FALSE;
        eContestantStatus[contestant].appeal = 0;
        eContestantStatus[contestant].baseAppeal = 0;
    }
    eContestExcitement.moveExcitement = Contest_GetMoveExcitement(eContestantStatus[contestant].currMove);
    if (eContestantStatus[contestant].overrideCategoryExcitementMod)
        eContestExcitement.moveExcitement = 1;

    if (eContestExcitement.moveExcitement > 0)
    {
        if (eContest.applauseLevel + eContestExcitement.moveExcitement > 4)
            eContestExcitement.excitementAppealBonus = 60;
        else
            eContestExcitement.excitementAppealBonus = 10;
    }
    else
    {
        eContestExcitement.excitementAppealBonus = 0;
    }

    // Transform and Role Play require a visible target mon
    // so randomly choose a contestant to be the "target"
    rnd = Random() % (CONTESTANT_COUNT - 1);
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        // Target can't be the attacker
        if (i != contestant)
        {
            if (rnd == 0)
                break;
            rnd--;
        }
    }
    eContestantStatus[contestant].contestantAnimTarget = i;
}

void SetContestantEffectStringID(u8 contestant, u8 effectStringId)
{
    eContestantStatus[contestant].effectStringId = effectStringId;
}

void SetContestantEffectStringID2(u8 contestant, u8 effectStringId)
{
    eContestantStatus[contestant].effectStringId2 = effectStringId;
}

void SetStartledString(u8 contestant, u8 jam)
{
    if      (jam >= 60)
        SetContestantEffectStringID(contestant, CONTEST_STRING_TRIPPED_OVER);
    else if (jam >= 40)
        SetContestantEffectStringID(contestant, CONTEST_STRING_LEAPT_UP);
    else if (jam >= 30)
        SetContestantEffectStringID(contestant, CONTEST_STRING_UTTER_CRY);
    else if (jam >= 20)
        SetContestantEffectStringID(contestant, CONTEST_STRING_TURNED_BACK);
    else if (jam >= 10)
        SetContestantEffectStringID(contestant, CONTEST_STRING_LOOKED_DOWN);
}

static void PrintAppealMoveResultText(u8 contestant, u8 stringId)
{
    StringCopy(gStringVar1, gContestMons[contestant].nickname);
    StringCopy(gStringVar2, gMoveNames[eContestantStatus[contestant].currMove]);
    if      (gContestMoves[eContestantStatus[eContestAppealResults.contestant].currMove].contestCategory == CONTEST_CATEGORY_COOL)
        StringCopy(gStringVar3, gText_Contest_Shyness);
    else if (gContestMoves[eContestantStatus[eContestAppealResults.contestant].currMove].contestCategory == CONTEST_CATEGORY_BEAUTY)
        StringCopy(gStringVar3, gText_Contest_Anxiety);
    else if (gContestMoves[eContestantStatus[eContestAppealResults.contestant].currMove].contestCategory == CONTEST_CATEGORY_CUTE)
        StringCopy(gStringVar3, gText_Contest_Laziness);
    else if (gContestMoves[eContestantStatus[eContestAppealResults.contestant].currMove].contestCategory == CONTEST_CATEGORY_SMART)
        StringCopy(gStringVar3, gText_Contest_Hesitancy);
    else
        StringCopy(gStringVar3, gText_Contest_Fear);
    StringExpandPlaceholders(gStringVarBuffer, sAppealResultTexts[stringId]);
    ContestClearGeneralTextWindow();
    Contest_StartTextPrinter(gStringVarBuffer, TRUE);
}

void MakeContestantNervous(u8 p)
{
    eContestantStatus[p].nervous = TRUE;
    eContestantStatus[p].currMove = MOVE_NONE;
}

// This function calculates the new turn order for the next round. The
// algorithm first checks for explicit turn assignments in the
// ContestantStatus::nextTurnOrder field of each contestant. The remaining
// turns are assigned such that the turn order will reverse.
//
// For example, if no Pokémon have a defined nextTurnOrder, then the 4th
// will become 1st, the 3rd will become 2nd, etc.
//
// Note: This function assumes that multiple Pokémon cannot have the same
// nextTurnOrder value.
static void ApplyNextTurnOrder(void)
{
    u8 nextContestant = 0;
    s32 i;
    s32 j;
    u8 newTurnOrder[CONTESTANT_COUNT];
    bool8 isContestantOrdered[CONTESTANT_COUNT];

    // Copy the current turn order.
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        newTurnOrder[i] = gContestantTurnOrder[i];
        isContestantOrdered[i] = FALSE;
    }

    // For each turn, assign a contestant to that turn.
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        // Look for explicit turn assignments.
        for (j = 0; j < CONTESTANT_COUNT; j++)
        {
            if (eContestantStatus[j].nextTurnOrder == i)
            {
                newTurnOrder[j] = i;
                isContestantOrdered[j] = TRUE;
                break;
            }
        }

        if (j == CONTESTANT_COUNT)
        {
            // No contestant was assigned to this turn. Look for the unassigned contestant
            // with the highest turn order.
            //
            // First, look for the first unassigned contestant.
            for (j = 0; j < CONTESTANT_COUNT; j++)
            {
                if (!isContestantOrdered[j] && eContestantStatus[j].nextTurnOrder == CONTESTANT_NONE)
                {
                    nextContestant = j;
                    j++;
                    break;
                }
            }

            // Then, look for a better candidate, with a higher turn order.
            for (; j < CONTESTANT_COUNT; j++)
            {
                if (!isContestantOrdered[j] && eContestantStatus[j].nextTurnOrder == CONTESTANT_NONE
                 && gContestantTurnOrder[nextContestant] > gContestantTurnOrder[j])
                    nextContestant = j;
            }

            // Assign the contestant to this turn.
            newTurnOrder[nextContestant] = i;
            isContestantOrdered[nextContestant] = TRUE;
        }
    }

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        eContestAppealResults.turnOrder[i] = newTurnOrder[i];
        eContestantStatus[i].nextTurnOrder = CONTESTANT_NONE;
        eContestantStatus[i].turnOrderMod = 0;
        gContestantTurnOrder[i] = newTurnOrder[i];
    }
}

static void SpriteCB_JudgeSpeechBubble(struct Sprite *sprite)
{
    if (sprite->data[1]++ > 84)
    {
        sprite->data[1] = 0;
        sprite->invisible = TRUE;
        sprite->callback = SpriteCallbackDummy;
        eContest.waitForJudgeSpeechBubble = FALSE;
    }
}

static void DoJudgeSpeechBubble(u8 symbolId)
{
    u8 spriteId = eContest.judgeSpeechBubbleSpriteId;

    switch (symbolId)
    {
    case JUDGE_SYMBOL_SWIRL:
    case JUDGE_SYMBOL_SWIRL_UNUSED:
        gSprites[spriteId].oam.tileNum = gSprites[spriteId].data[0];
        PlaySE(SE_FAILURE);
        break;
    case JUDGE_SYMBOL_ONE_EXCLAMATION:
        gSprites[spriteId].oam.tileNum = gSprites[spriteId].data[0] + 4;
        PlaySE(SE_SUCCESS);
        break;
    case JUDGE_SYMBOL_TWO_EXCLAMATIONS:
        gSprites[spriteId].oam.tileNum = gSprites[spriteId].data[0] + 8;
        PlaySE(SE_SUCCESS);
        break;
    case JUDGE_SYMBOL_NUMBER_ONE_UNUSED: // Identical to JUDGE_SYMBOL_NUMBER_ONE
        gSprites[spriteId].oam.tileNum = gSprites[spriteId].data[0] + 12;
        PlaySE(SE_WARP_IN);
        break;
    case JUDGE_SYMBOL_NUMBER_ONE:
        gSprites[spriteId].oam.tileNum = gSprites[spriteId].data[0] + 12;
        PlaySE(SE_WARP_IN);
        break;
    case JUDGE_SYMBOL_NUMBER_FOUR:
        gSprites[spriteId].oam.tileNum = gSprites[spriteId].data[0] + 16;
        PlaySE(SE_WARP_IN);
        break;
    case JUDGE_SYMBOL_STAR:
        gSprites[spriteId].oam.tileNum = gSprites[spriteId].data[0] + 24;
        PlaySE(SE_M_HEAL_BELL);
        break;
    case JUDGE_SYMBOL_QUESTION_MARK:
    default:
        gSprites[spriteId].oam.tileNum = gSprites[spriteId].data[0] + 20;
        PlaySE(SE_WARP_IN);
        break;
    }
    gSprites[spriteId].data[1] = 0;
    gSprites[spriteId].invisible = FALSE;
    gSprites[spriteId].callback = SpriteCB_JudgeSpeechBubble;
    eContest.waitForJudgeSpeechBubble = TRUE;
}

static void UpdateApplauseMeter(void)
{
    s32 i;

    for (i = 0; i < APPLAUSE_METER_SIZE; i++)
    {
        const u8 *src;

        if (i < eContest.applauseLevel)
            src = &gContestApplauseMeterGfx[64];
        else
            src = gContestApplauseMeterGfx;
        CpuCopy32(src, (void *)(OBJ_VRAM0 + (gSprites[eContest.applauseMeterSpriteId].oam.tileNum + 17 + i) * 32), 32);
        CpuCopy32(src + 32, (void *)(OBJ_VRAM0 + (gSprites[eContest.applauseMeterSpriteId].oam.tileNum + 25 + i) * 32), 32);

        if (eContest.applauseLevel > 4)
            StartApplauseOverflowAnimation();
    }
}

s8 Contest_GetMoveExcitement(u16 move)
{
    return sContestExcitementTable[gSpecialVar_ContestCategory][gContestMoves[move].contestCategory];
}

static u8 StartApplauseOverflowAnimation(void)
{
    u8 taskId = CreateTask(Task_ApplauseOverflowAnimation, 10);

    gTasks[taskId].data[1] = 1;
    gTasks[taskId].data[2] = IndexOfSpritePaletteTag(TAG_APPLAUSE_METER);
    return taskId;
}

static void Task_ApplauseOverflowAnimation(u8 taskId)
{
    // Skip every other frame.
    if (++gTasks[taskId].data[0] == 1)
    {
        gTasks[taskId].data[0] = 0;

        // Alternate between normal colors and white.
        if (gTasks[taskId].data[3] == 0)
            gTasks[taskId].data[4]++;
        else
            gTasks[taskId].data[4]--;

        BlendPalette(OBJ_PLTT_ID(gTasks[taskId].data[2]) + 8, 1, gTasks[taskId].data[4], RGB_WHITE);

        // At the maximum or minimum blending, switch directions.
        if (gTasks[taskId].data[4] == 0 || gTasks[taskId].data[4] == 16)
        {
            gTasks[taskId].data[3] ^= 1;

            // Continue the animation until the applause meter is cleared.
            if (eContest.applauseLevel < 5)
            {
                BlendPalette(OBJ_PLTT_ID(gTasks[taskId].data[2]) + 8, 1, 0, RGB_RED);
                DestroyTask(taskId);
            }
        }
    }
}

static void SlideApplauseMeterIn(void)
{
    CreateTask(Task_SlideApplauseMeterIn, 10);
    gSprites[eContest.applauseMeterSpriteId].x2 = -70;
    gSprites[eContest.applauseMeterSpriteId].invisible = FALSE;
    eContest.applauseMeterIsMoving = TRUE;
}

static void Task_SlideApplauseMeterIn(u8 taskId)
{
    struct Sprite *sprite = &gSprites[eContest.applauseMeterSpriteId];

    gTasks[taskId].data[10] += 1664;
    sprite->x2 += gTasks[taskId].data[10] >> 8;
    gTasks[taskId].data[10] = gTasks[taskId].data[10] & 0xFF;
    if (sprite->x2 > 0)
        sprite->x2 = 0;
    if (sprite->x2 == 0)
    {
        eContest.applauseMeterIsMoving = FALSE;
        DestroyTask(taskId);
    }
}

static void SlideApplauseMeterOut(void)
{
    if (gSprites[eContest.applauseMeterSpriteId].invisible == TRUE)
    {
        eContest.applauseMeterIsMoving = FALSE;
    }
    else
    {
        CreateTask(Task_SlideApplauseMeterOut, 10);
        gSprites[eContest.applauseMeterSpriteId].x2 = 0;
        eContest.applauseMeterIsMoving = TRUE;
    }
}

static void Task_SlideApplauseMeterOut(u8 taskId)
{
    struct Sprite *sprite = &gSprites[eContest.applauseMeterSpriteId];

    gTasks[taskId].data[10] += 1664;
    sprite->x2 -= gTasks[taskId].data[10] >> 8;
    gTasks[taskId].data[10] = gTasks[taskId].data[10] & 0xFF;
    if (sprite->x2 < -70)
        sprite->x2 = -70;
    if (sprite->x2 == -70)
    {
        sprite->invisible = TRUE;
        eContest.applauseMeterIsMoving = FALSE;
        DestroyTask(taskId);
    }
}

static void ShowAndUpdateApplauseMeter(s8 unused)
{
    u8 taskId = CreateTask(Task_ShowAndUpdateApplauseMeter, 5);

    gTasks[taskId].data[0] = unused;
    eContest.isShowingApplauseMeter = TRUE;
}

static void Task_ShowAndUpdateApplauseMeter(u8 taskId)
{
    switch (gTasks[taskId].data[10])
    {
    case 0:
        SlideApplauseMeterIn();
        gTasks[taskId].data[10]++;
        break;
    case 1:
        if (!eContest.applauseMeterIsMoving)
        {
            gTasks[taskId].data[10]++;
        }
        break;
    case 2:
        if (gTasks[taskId].data[11]++ > 20)
        {
            gTasks[taskId].data[11] = 0;
            UpdateApplauseMeter();
            eContest.isShowingApplauseMeter = FALSE;
            DestroyTask(taskId);
        }
        break;
    }
}

static void UNUSED HideApplauseMeterNoAnim(void)
{
    gSprites[eContest.applauseMeterSpriteId].x2 = 0;
    gSprites[eContest.applauseMeterSpriteId].invisible = FALSE;
}

static void UNUSED ShowApplauseMeterNoAnim(void)
{
    gSprites[eContest.applauseMeterSpriteId].invisible = TRUE;
}

#define tDelay  data[10]
#define tFrame  data[11]
#define tCycles data[12]

static void AnimateAudience(void)
{
    CreateTask(Task_AnimateAudience, 15);
    eContest.animatingAudience = TRUE;
}

static void Task_AnimateAudience(u8 taskId)
{
    if (gTasks[taskId].tDelay++ > 6)
    {
        gTasks[taskId].tDelay = 0;
        if (gTasks[taskId].tFrame == 0)
        {
            RequestDma3Copy(eContestAudienceFrame2_Gfx, (void *)(BG_SCREEN_ADDR(4)), 0x1000, 1);
        }
        else
        {
            RequestDma3Copy(eUnzippedContestAudience_Gfx, (void *)(BG_SCREEN_ADDR(4)), 0x1000, 1);
            gTasks[taskId].tCycles++;
        }

        gTasks[taskId].tFrame ^= 1;

        if (gTasks[taskId].tCycles == 9)
        {
            eContest.animatingAudience = FALSE;
            DestroyTask(taskId);
        }
    }
}

#undef tDelay
#undef tFrame
#undef tCycles

#define tBlendColor       data[0]
#define tBlendCoeff       data[1]
#define tBlendDir         data[2]
#define tTargetBlendCoeff data[3]
#define tBlendDelay       data[10]

static void BlendAudienceBackground(s8 excitementDir, s8 blendDir)
{
    u8 taskId = CreateTask(Task_BlendAudienceBackground, 10);
    u16 blendColor;
    u8 blendCoeff;
    u8 targetBlendCoeff;

    if (excitementDir > 0)
    {
        blendColor = RGB(30, 27, 8);
        if (blendDir > 0)
        {
            // Blend to yellow (amount depends on applause meter)
            blendCoeff = 0;
            targetBlendCoeff = eContest.applauseLevel * 3;
        }
        else
        {
            // Blend back to original
            blendCoeff = eContest.applauseLevel * 3;
            targetBlendCoeff = 0;
        }
    }
    else
    {
        blendColor = RGB_BLACK;
        if (blendDir > 0)
        {
            // Blend to black
            blendCoeff = 0;
            targetBlendCoeff = 12;
        }
        else
        {
            // Black back to original
            blendCoeff = 12;
            targetBlendCoeff = 0;
        }
    }
    gTasks[taskId].tBlendColor = blendColor;
    gTasks[taskId].tBlendCoeff = blendCoeff;
    gTasks[taskId].tBlendDir = blendDir;
    gTasks[taskId].tTargetBlendCoeff = targetBlendCoeff;
    // Because this isn't set to TRUE here, the main task doesn't wait for the color blend
    // Unclear if this was intentional or not (perhaps waiting added too much delay). In any case it does nothing now
    eContest.waitForAudienceBlend = FALSE;

}

static void Task_BlendAudienceBackground(u8 taskId)
{
    if (gTasks[taskId].tBlendDelay++ >= 0)
    {
        gTasks[taskId].tBlendDelay = 0;
        if (gTasks[taskId].tBlendDir > 0)
            gTasks[taskId].tBlendCoeff++;
        else
            gTasks[taskId].tBlendCoeff--;

        BlendPalette(BG_PLTT_ID(1) + 1,  1, gTasks[taskId].tBlendCoeff, gTasks[taskId].tBlendColor);
        BlendPalette(BG_PLTT_ID(1) + 10, 1, gTasks[taskId].tBlendCoeff, gTasks[taskId].tBlendColor);

        if (gTasks[taskId].tBlendCoeff == gTasks[taskId].tTargetBlendCoeff)
        {
            DestroyTask(taskId);
            eContest.waitForAudienceBlend = FALSE;
        }
    }
}

#undef tBlendColor
#undef tBlendCoeff
#undef tTargetBlendCoeff
#undef tBlendDelay

static void ShowHideNextTurnGfx(bool8 show)
{
    s32 i;

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        if (eContestantStatus[i].turnOrderMod != 0 && show)
        {
            CpuCopy32(GetTurnOrderNumberGfx(i), (void *)(OBJ_VRAM0 + (gSprites[eContestGfxState[i].nextTurnSpriteId].oam.tileNum + 6) * 32), 32);
            gSprites[eContestGfxState[i].nextTurnSpriteId].y = sNextTurnSpriteYPositions[gContestantTurnOrder[i]];
            gSprites[eContestGfxState[i].nextTurnSpriteId].invisible = FALSE;
        }
        else
        {
            gSprites[eContestGfxState[i].nextTurnSpriteId].invisible = TRUE;
        }
    }
}

static const u8 *GetTurnOrderNumberGfx(u8 contestant)
{
    if (eContestantStatus[contestant].turnOrderMod != 1)
        return gContestNextTurnRandomGfx;
    else
        return gContestNextTurnNumbersGfx + eContestantStatus[contestant].nextTurnOrder * 32;
}

static void DrawUnnervedSymbols(void)
{
    s32 i = 0;
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        if (eContestAppealResults.unnervedPokes[i] != 0 && !Contest_IsMonsTurnDisabled(i))
        {
            u32 contestantOffset = gContestantTurnOrder[i] * 5 + 2;
            u16 symbolOffset = GetStatusSymbolTileOffset(STAT_SYMBOL_SWIRL);

            ContestBG_FillBoxWithIncrementingTile(0, symbolOffset, 20, contestantOffset, 2, 1, 17, 1);
            symbolOffset += 16;
            ContestBG_FillBoxWithIncrementingTile(0, symbolOffset, 20, contestantOffset + 1, 2, 1, 17, 1);
            PlaySE(SE_CONTEST_ICON_CHANGE);
        }
    }
}

bool8 IsContestantAllowedToCombo(u8 contestant)
{
    if (eContestantStatus[contestant].repeatedMove || eContestantStatus[contestant].nervous)
        return FALSE;
    else
        return TRUE;
}

static void SetBgForCurtainDrop(void)
{
    s32 i;
    u16 bg0Cnt, bg1Cnt, bg2Cnt;

    bg1Cnt = GetGpuReg(REG_OFFSET_BG1CNT);
    ((vBgCnt *)&bg1Cnt)->priority = 0;
    ((vBgCnt *)&bg1Cnt)->screenSize = 2;
    ((vBgCnt *)&bg1Cnt)->areaOverflowMode = 0;
    ((vBgCnt *)&bg1Cnt)->charBaseBlock = 0;

    SetGpuReg(REG_OFFSET_BG1CNT, bg1Cnt);

    bg0Cnt = GetGpuReg(REG_OFFSET_BG0CNT);
    bg2Cnt = GetGpuReg(REG_OFFSET_BG2CNT);
    ((vBgCnt *)&bg0Cnt)->priority = 1;
    ((vBgCnt *)&bg2Cnt)->priority = 1;

    SetGpuReg(REG_OFFSET_BG0CNT, bg0Cnt);
    SetGpuReg(REG_OFFSET_BG2CNT, bg2Cnt);

    gBattle_BG1_X = DISPLAY_WIDTH;
    gBattle_BG1_Y = DISPLAY_HEIGHT;
    SetGpuReg(REG_OFFSET_BG1HOFS, gBattle_BG1_X);
    SetGpuReg(REG_OFFSET_BG1VOFS, gBattle_BG1_Y);

    CpuFill32(0, gContestResources->contestBgTilemaps[1], 0x1000);

    CopyToBgTilemapBuffer(1, gContestCurtainTilemap, 0, 0);
    Contest_SetBgCopyFlags(1);

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        gSprites[eContestGfxState[i].sliderHeartSpriteId].oam.priority = 1;
        gSprites[eContestGfxState[i].nextTurnSpriteId].oam.priority = 1;
    }
}

static void UpdateContestantBoxOrder(void)
{
    s32 i;
    u16 bg1Cnt;

    RequestDma3Fill(0,(void *)(BG_CHAR_ADDR(2)), 0x2000, 1);
    CpuFill32(0, gContestResources->contestBgTilemaps[1], 0x1000);
    Contest_SetBgCopyFlags(1);
    bg1Cnt = GetGpuReg(REG_OFFSET_BG1CNT);
    ((vBgCnt *) &bg1Cnt)->priority = 1;
    ((vBgCnt *) &bg1Cnt)->screenSize = 0;
    ((vBgCnt *) &bg1Cnt)->areaOverflowMode = 0;
    ((vBgCnt *) &bg1Cnt)->charBaseBlock = 2;

    SetGpuReg(REG_OFFSET_BG1CNT, bg1Cnt);

    gBattle_BG1_X = 0;
    gBattle_BG1_Y = 0;

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        gSprites[eContestGfxState[i].sliderHeartSpriteId].oam.priority = 0;
        gSprites[eContestGfxState[i].nextTurnSpriteId].oam.priority = 0;
    }
}

static void Task_StartDropCurtainAtRoundEnd(u8 taskId)
{
    gBattle_BG1_X = 0;
    gBattle_BG1_Y = DISPLAY_HEIGHT;
    PlaySE12WithPanning(SE_CONTEST_CURTAIN_FALL, 0);
    gTasks[taskId].func = Task_UpdateCurtainDropAtRoundEnd;
}

static void Task_UpdateCurtainDropAtRoundEnd(u8 taskId)
{
    if ((s16)(gBattle_BG1_Y -= 7) < 0)
        gBattle_BG1_Y = 0;
    if (gBattle_BG1_Y == 0)
    {
        gTasks[taskId].data[0] = 0;
        gTasks[taskId].data[1] = 0;
        gTasks[taskId].data[2] = 0;
        gTasks[taskId].func = Task_ResetForNextRound;
    }
}

static void Task_ResetForNextRound(u8 taskId)
{
    s32 i;

    switch (gTasks[taskId].data[0])
    {
    case 0:
        for (i = 0; i < CONTESTANT_COUNT; i++)
            eContest.prevTurnOrder[i] = gContestantTurnOrder[i];
        FillContestantWindowBgs();
        UpdateBlendTaskContestantsData();
        DrawConditionStars();
        DrawContestantWindows();
        ShowHideNextTurnGfx(TRUE);
        UpdateSliderHeartSpriteYPositions();
        gTasks[taskId].data[0] = 1;
        break;
    case 1:
        if (gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK)
        {
            u8 taskId2;

            eContest.waitForLink = TRUE;
            if (IsPlayerLinkLeader())
                SetContestantStatusesForNextRound();
            taskId2 = CreateTask(Task_LinkContest_CommunicateAppealsState, 0);
            SetTaskFuncWithFollowupFunc(taskId2, Task_LinkContest_CommunicateAppealsState, Task_EndWaitForLink);
            ContestPrintLinkStandby();
            gTasks[taskId].data[0] = 2;
        }
        else
        {
            SetContestantStatusesForNextRound();
            gTasks[taskId].data[0] = 3;
        }
        break;
    case 2:
        if (!eContest.waitForLink)
            gTasks[taskId].data[0] = 3;
        break;
    case 3:
        DrawStatusSymbols();
        SwapMoveDescAndContestTilemaps();
        gTasks[taskId].data[0] = 0;
        gTasks[taskId].func = Task_WaitRaiseCurtainAtRoundEnd;
        break;
    }
}

static void Task_UpdateRaiseCurtainAtRoundEnd(u8 taskId)
{
    if ((s16)(gBattle_BG1_Y += 7) > DISPLAY_HEIGHT)
        gTasks[taskId].func = Task_UpdateContestantBoxOrder;
}

static void Task_WaitRaiseCurtainAtRoundEnd(u8 taskId)
{
    if (gTasks[taskId].data[2] < 10)
    {
        gTasks[taskId].data[2]++;
    }
    else
    {
        if (gTasks[taskId].data[1] == 0)
        {
            if (gTasks[taskId].data[0] == 16)
                gTasks[taskId].data[1]++;
            else
                gTasks[taskId].data[0]++;
        }
        else
        {
            if (gTasks[taskId].data[0] == 0)
            {
                gTasks[taskId].data[1] = 0;
                gTasks[taskId].data[2] = 0;
                gTasks[taskId].func = Task_StartRaiseCurtainAtRoundEnd;
            }
            else
            {
                gTasks[taskId].data[0]--;
            }
        }
    }
}

static void Task_StartRaiseCurtainAtRoundEnd(u8 taskId)
{
    if (gTasks[taskId].data[2] < 10)
    {
        gTasks[taskId].data[2]++;
    }
    else
    {
        gTasks[taskId].data[2] = 0;
        PlaySE12WithPanning(SE_CONTEST_CURTAIN_RISE, 0);
        gTasks[taskId].func = Task_UpdateRaiseCurtainAtRoundEnd;
    }
}

#define tAnimId data[0]

static void AnimateSliderHearts(u8 animId)
{
    s32 i;
    u8 taskId;

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        gSprites[eContestGfxState[i].sliderHeartSpriteId].oam.matrixNum = AllocOamMatrix();
        gSprites[eContestGfxState[i].sliderHeartSpriteId].oam.affineMode = ST_OAM_AFFINE_NORMAL;
        StartSpriteAffineAnim(&gSprites[eContestGfxState[i].sliderHeartSpriteId], animId);
        if (animId == SLIDER_HEART_ANIM_APPEAR)
        {
            AnimateSprite(&gSprites[eContestGfxState[i].sliderHeartSpriteId]);
            gSprites[eContestGfxState[i].sliderHeartSpriteId].invisible = FALSE;
        }
    }
    taskId = CreateTask(Task_WaitForSliderHeartAnim, 5);
    gTasks[taskId].tAnimId = animId;
    eContest.sliderHeartsAnimating = TRUE;
}

static void Task_WaitForSliderHeartAnim(u8 taskId)
{
    s32 i;

    if (gSprites[eContestGfxState[0].sliderHeartSpriteId].affineAnimEnded)
    {
        if ((u8)gTasks[taskId].tAnimId == SLIDER_HEART_ANIM_DISAPPEAR)
        {
            for (i = 0; i < CONTESTANT_COUNT; i++)
                gSprites[eContestGfxState[i].sliderHeartSpriteId].invisible = TRUE;
        }
        for (i = 0; i < CONTESTANT_COUNT; i++)
            FreeSpriteOamMatrix(&gSprites[eContestGfxState[i].sliderHeartSpriteId]);
        eContest.sliderHeartsAnimating = FALSE;
        DestroyTask(taskId);
    }
}

#undef tAnimId

static u16 SanitizeMove(u16 move)
{
    if (move >= MOVES_COUNT)
        move = MOVE_POUND;
    return move;
}

static u16 SanitizeSpecies(u16 species)
{
    if (species >= NUM_SPECIES)
        species = SPECIES_NONE;
    return species;
}

static void SetMoveSpecificAnimData(u8 contestant)
{
    s32 i;
    u16 move = SanitizeMove(eContestantStatus[contestant].currMove);
    u16 species = SanitizeSpecies(gContestMons[contestant].species);
    u8 targetContestant;

    memset(&gContestResources->moveAnim->species, 0, 20);
    ClearBattleAnimationVars();
    for (i = 0; i < CONTESTANT_COUNT; i++)
        gBattleMonForms[i] = 0;
    switch (move)
    {
    case MOVE_CURSE:
        if (gSpeciesInfo[species].types[0] == TYPE_GHOST || gSpeciesInfo[species].types[1] == TYPE_GHOST)
            gAnimMoveTurn = 0;
        else
            gAnimMoveTurn = 1;
        break;
    case MOVE_TRANSFORM:
    case MOVE_ROLE_PLAY:
        targetContestant = eContestantStatus[contestant].contestantAnimTarget;
        gContestResources->moveAnim->targetSpecies = SanitizeSpecies(gContestMons[targetContestant].species);
        gContestResources->moveAnim->targetPersonality = gContestMons[targetContestant].personality;
        gContestResources->moveAnim->hasTargetAnim = TRUE;
        break;
    case MOVE_RETURN:
        gAnimFriendship = MAX_FRIENDSHIP;
        break;
    case MOVE_FRUSTRATION:
        gAnimFriendship = 0;
        break;
    case MOVE_SOLAR_BEAM:
    case MOVE_RAZOR_WIND:
    case MOVE_SKULL_BASH:
    case MOVE_SKY_ATTACK:
        if (eContest.moveAnimTurnCount == 0)
        {
            eContest.moveAnimTurnCount = 2;
            gAnimMoveTurn = 0;
        }
        else
        {
            gAnimMoveTurn = 1;
        }
        break;
    }
    SetBattleTargetSpritePosition();
}

static void ClearMoveAnimData(u8 contestant)
{
    memset(gContestResources->moveAnim, 0, sizeof(struct ContestMoveAnimData));
    if (eContest.moveAnimTurnCount != 0)
        eContest.moveAnimTurnCount--;
}

static void SetMoveAnimAttackerData(u8 contestant)
{
    gContestResources->moveAnim->contestant = contestant;
    gContestResources->moveAnim->species = SanitizeSpecies(gContestMons[contestant].species);
    gContestResources->moveAnim->personality = gContestMons[contestant].personality;
    gContestResources->moveAnim->otId = gContestMons[contestant].otId;
}

static void CreateInvisibleBattleTargetSprite(void)
{
    gBattlerSpriteIds[B_POSITION_OPPONENT_RIGHT] = CreateInvisibleSpriteWithCallback(SpriteCallbackDummy);
    InitSpriteAffineAnim(&gSprites[gBattlerSpriteIds[gBattlerTarget]]);
    SetBattleTargetSpritePosition();
}

static void SetBattleTargetSpritePosition(void)
{
    struct Sprite *sprite = &gSprites[gBattlerSpriteIds[B_POSITION_OPPONENT_RIGHT]];

    sprite->x2 = 0;
    sprite->y2 = 0;
    sprite->x = GetBattlerSpriteCoord(B_POSITION_OPPONENT_RIGHT, BATTLER_COORD_X);
    sprite->y = GetBattlerSpriteCoord(B_POSITION_OPPONENT_RIGHT, BATTLER_COORD_Y);
    sprite->invisible = TRUE;
}

static void SetMoveTargetPosition(u16 move)
{
    switch (gBattleMoves[move].target)
    {
    case MOVE_TARGET_USER_OR_SELECTED:
    case MOVE_TARGET_USER:
        gBattlerTarget = B_POSITION_PLAYER_RIGHT;
        break;
    case MOVE_TARGET_SELECTED:
    case MOVE_TARGET_RANDOM:
    case MOVE_TARGET_BOTH:
    case MOVE_TARGET_FOES_AND_ALLY:
    default:
        gBattlerTarget = B_POSITION_OPPONENT_RIGHT;
        break;
    }
}

static void Contest_PrintTextToBg0WindowStd(u32 windowId, const u8 *b)
{
    struct TextPrinterTemplate printerTemplate;

    printerTemplate.currentChar = b;
    printerTemplate.windowId = windowId;
    printerTemplate.fontId = FONT_NORMAL;
    printerTemplate.x = 0;
    printerTemplate.y = 1;
    printerTemplate.currentX = 0;
    printerTemplate.currentY = 1;
    printerTemplate.letterSpacing = 0;
    printerTemplate.lineSpacing = 0;
    printerTemplate.unk = 0;
    printerTemplate.fgColor = 15;
    printerTemplate.bgColor = 0;
    printerTemplate.shadowColor = 8;

    AddTextPrinter(&printerTemplate, 0, 0);
    PutWindowTilemap(windowId);
    Contest_SetBgCopyFlags(0);
}

void Contest_PrintTextToBg0WindowAt(u32 windowId, u8 *currChar, s32 x, s32 y, s32 fontId)
{
    struct TextPrinterTemplate printerTemplate;

    printerTemplate.currentChar = currChar;
    printerTemplate.windowId = windowId;
    printerTemplate.fontId = fontId;
    printerTemplate.x = x;
    printerTemplate.y = y;
    printerTemplate.currentX = x;
    printerTemplate.currentY = y;
    printerTemplate.letterSpacing = 0;
    printerTemplate.lineSpacing = 0;
    printerTemplate.unk = 0;
    printerTemplate.fgColor = 15;
    printerTemplate.bgColor = 0;
    printerTemplate.shadowColor = 8;

    AddTextPrinter(&printerTemplate, 0, 0);
    PutWindowTilemap(windowId);
    Contest_SetBgCopyFlags(0);
}

static void Contest_StartTextPrinter(const u8 *currChar, bool32 b)
{
    struct TextPrinterTemplate printerTemplate;
    u8 speed;

    printerTemplate.currentChar = currChar;
    printerTemplate.windowId = WIN_GENERAL_TEXT;
    printerTemplate.fontId = FONT_NORMAL;
    printerTemplate.x = 0;
    printerTemplate.y = 1;
    printerTemplate.currentX = 0;
    printerTemplate.currentY = 1;
    printerTemplate.letterSpacing = 0;
    printerTemplate.lineSpacing = 0;
    printerTemplate.unk = 0;
    printerTemplate.fgColor = 1;
    printerTemplate.bgColor = 0;
    printerTemplate.shadowColor = 8;

    if (!b)
    {
        AddTextPrinter(&printerTemplate, 0, 0);
    }
    else
    {
        if (gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK)
            speed = 4;
        else
            speed = GetPlayerTextSpeedDelay();
        AddTextPrinter(&printerTemplate, speed, 0);
    }

    PutWindowTilemap(WIN_GENERAL_TEXT);
    Contest_SetBgCopyFlags(0);
}

static void ContestBG_FillBoxWithIncrementingTile(u8 bg, u16 firstTileNum, u8 x, u8 y, u8 width, u8 height, u8 paletteSlot, s16 tileNumData)
{
    WriteSequenceToBgTilemapBuffer(bg, firstTileNum, x, y, width, height, paletteSlot, tileNumData);
    Contest_SetBgCopyFlags(bg);
}

static void ContestBG_FillBoxWithTile(u8 bg, u16 firstTileNum, u8 x, u8 y, u8 width, u8 height, u8 paletteSlot)
{
    ContestBG_FillBoxWithIncrementingTile(bg, firstTileNum, x, y, width, height, paletteSlot, 0);
}

static bool32 Contest_RunTextPrinters(void)
{
    RunTextPrinters();
    return IsTextPrinterActive(WIN_GENERAL_TEXT);
}

static void Contest_SetBgCopyFlags(u32 flagIndex)
{
    sContestBgCopyFlags |= 1 << flagIndex;
}

void ResetContestLinkResults(void)
{
    s32 i;
    s32 j;

    for(i = 0; i < CONTEST_CATEGORIES_COUNT; i++)
        for(j = 0; j < CONTESTANT_COUNT; j++)
            gSaveBlock2Ptr->contestLinkResults[i][j] = 0;
}

bool8 SaveContestWinner(u8 rank)
{
    s32 i;
    u8 captionId = Random() % NUM_PAINTING_CAPTIONS;

    // Get the index of the winner among the contestants
    for (i = 0; i < CONTESTANT_COUNT - 1; i++)
        if (gContestFinalStandings[i] == 0)
            break;

    // Exit if attempting to save a Pokémon other than the player's to the museum
    if (rank == CONTEST_SAVE_FOR_MUSEUM && i != gContestPlayerMonIndex)
        return FALSE;

    // Adjust the random painting caption depending on the category
    switch (gSpecialVar_ContestCategory)
    {
    case CONTEST_CATEGORY_COOL:
        captionId += NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_COOL;
        break;
    case CONTEST_CATEGORY_BEAUTY:
        captionId += NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_BEAUTY;
        break;
    case CONTEST_CATEGORY_CUTE:
        captionId += NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_CUTE;
        break;
    case CONTEST_CATEGORY_SMART:
        captionId += NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_SMART;
        break;
    case CONTEST_CATEGORY_TOUGH:
        captionId += NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_TOUGH;
        break;
    }

    if (rank != CONTEST_SAVE_FOR_ARTIST)
    {
        // Save winner in the saveblock
        // Used to save any winner for the Contest Hall or the Museum
        // but excludes the temporary save used by the artist
        u8 id = GetContestWinnerSaveIdx(rank, TRUE);
        gSaveBlock1Ptr->contestWinners[id].personality = gContestMons[i].personality;
        gSaveBlock1Ptr->contestWinners[id].species = gContestMons[i].species;
        gSaveBlock1Ptr->contestWinners[id].trainerId = gContestMons[i].otId;
        StringCopy(gSaveBlock1Ptr->contestWinners[id].monName, gContestMons[i].nickname);
        StringCopy(gSaveBlock1Ptr->contestWinners[id].trainerName, gContestMons[i].trainerName);
        if(gLinkContestFlags & LINK_CONTEST_FLAG_IS_LINK)
            gSaveBlock1Ptr->contestWinners[id].contestRank = CONTEST_RANK_LINK;
        else
            gSaveBlock1Ptr->contestWinners[id].contestRank = gSpecialVar_ContestRank;

        if (rank != CONTEST_SAVE_FOR_MUSEUM)
            gSaveBlock1Ptr->contestWinners[id].contestCategory = gSpecialVar_ContestCategory;
        else
            gSaveBlock1Ptr->contestWinners[id].contestCategory = captionId;
    }
    else
    {
        // Set the most recent winner so the artist can show the player their painting
        gCurContestWinner.personality = gContestMons[i].personality;
        gCurContestWinner.trainerId = gContestMons[i].otId;
        gCurContestWinner.species = gContestMons[i].species;
        StringCopy(gCurContestWinner.monName, gContestMons[i].nickname);
        StringCopy(gCurContestWinner.trainerName, gContestMons[i].trainerName);
        gCurContestWinner.contestCategory = captionId;
    }
    return TRUE;
}

// Rank is either a regular contest rank (for saving winners to show in the Contest Hall)
// Or one of two special IDs listed below (for saving winners to show in Museum, or from the artist)
// If just retrieving the index where the winner *would* go, shift is FALSE
// If actually preparing to insert the winner into the saveblock, shift is TRUE
u8 GetContestWinnerSaveIdx(u8 rank, bool8 shift)
{
    s32 i;

    switch (rank)
    {
    case CONTEST_RANK_NORMAL:
    case CONTEST_RANK_SUPER:
    case CONTEST_RANK_HYPER:
    case CONTEST_RANK_MASTER:
        if (shift)
        {
            for (i = NUM_CONTEST_HALL_WINNERS - 1; i > 0; i--)
                memcpy(&gSaveBlock1Ptr->contestWinners[i], &gSaveBlock1Ptr->contestWinners[i - 1], sizeof(struct ContestWinner));
        }
        return CONTEST_WINNER_HALL_1 - 1;
    default:
//  case CONTEST_SAVE_FOR_MUSEUM:
//  case CONTEST_SAVE_FOR_ARTIST:
        switch (gSpecialVar_ContestCategory)
        {
        case CONTEST_CATEGORY_COOL:
            return CONTEST_WINNER_MUSEUM_COOL - 1;
        case CONTEST_CATEGORY_BEAUTY:
            return CONTEST_WINNER_MUSEUM_BEAUTY - 1;
        case CONTEST_CATEGORY_CUTE:
            return CONTEST_WINNER_MUSEUM_CUTE - 1;
        case CONTEST_CATEGORY_SMART:
            return CONTEST_WINNER_MUSEUM_SMART - 1;
        case CONTEST_CATEGORY_TOUGH:
        default:
            return CONTEST_WINNER_MUSEUM_TOUGH - 1;
        }
    }
}

void ClearContestWinnerPicsInContestHall(void)
{
    s32 i;

    for (i = 0; i < MUSEUM_CONTEST_WINNERS_START; i++)
        gSaveBlock1Ptr->contestWinners[i] = gDefaultContestWinners[i];
}

static void SetContestLiveUpdateFlags(u8 contestant)
{
    s32 i;

    if (!eContestExcitement.frozen
        && eContestExcitement.moveExcitement > 0
        && !eContestantStatus[contestant].repeatedMove)
    {
        gContestResources->tv[contestant].winnerFlags |= CONTESTLIVE_FLAG_EXCITING_APPEAL;
        gContestResources->tv[contestant].madeExcitingAppeal = TRUE;
    }

    if (eContestantStatus[contestant].nervous)
        gContestResources->tv[contestant].winnerFlags |= CONTESTLIVE_FLAG_GOT_NERVOUS;

    if (!eContestExcitement.frozen
        && eContestExcitement.moveExcitement != 0
        && eContestExcitement.excitementAppealBonus == 60)
    {
        gContestResources->tv[contestant].winnerFlags |= CONTESTLIVE_FLAG_MAXED_EXCITEMENT;
    }

    if (eContestantStatus[contestant].usedComboMove
        && eContestantStatus[contestant].completedCombo)
    {
        gContestResources->tv[contestant].winnerFlags |= CONTESTLIVE_FLAG_USED_COMBO;
    }

    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        if (i != contestant && eContestantStatus[i].jam != 0)
        {
            gContestResources->tv[contestant].winnerFlags |= CONTESTLIVE_FLAG_STARTLED_OTHER;
            gContestResources->tv[i].winnerFlags |= CONTESTLIVE_FLAG_GOT_STARTLED;
        }
    }

    if (eContestantStatus[contestant].numTurnsSkipped != 0
        || eContestantStatus[contestant].noMoreTurns)
    {
        gContestResources->tv[contestant].winnerFlags |= CONTESTLIVE_FLAG_SKIPPED_TURN;
    }
    else if (!eContestantStatus[contestant].nervous)
    {
        gContestResources->tv[contestant].winnerFlags |= CONTESTLIVE_FLAG_MADE_APPEAL;
        gContestResources->tv[contestant].madeAppeal = TRUE;
        gContestResources->tv[contestant].appeals[eContest.appealNumber] = eContestantStatus[contestant].currMove;
    }

    if (eContestantStatus[contestant].repeatedMove)
        gContestResources->tv[contestant].loserFlags |= CONTESTLIVE_FLAG_REPEATED_MOVE;

    if (eContest.applauseLevel == 4
        && !eContestExcitement.frozen
        && eContestExcitement.moveExcitement < 0)
    {
        gContestResources->tv[contestant].loserFlags |= CONTESTLIVE_FLAG_MISSED_EXCITEMENT;
    }
}

#define APPEAL_MOVES_END 0xFFFF

static void CalculateContestLiveUpdateData(void)
{
    u8 loser;
    s32 i, j;
    bool32 notLastInRound1, notLastInRound2;
    u16 appealMoves[CONTEST_NUM_APPEALS + 1];
    u8 numMoveUses[CONTEST_NUM_APPEALS + 1];
    u16 moveCandidates[CONTEST_NUM_APPEALS];
    u8 winner;
    u8 mostUses;
    u8 numMoveCandidates;

    loser = 0;
    winner = 0;

    // Get loser/winner ids
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        if (gContestFinalStandings[i] == 0)
            winner = i;
        else if (gContestFinalStandings[i] == CONTESTANT_COUNT - 1)
            loser = i;
    }

    // Set flags for commenting on loser
    gContestResources->tv[loser].loserFlags |= CONTESTLIVE_FLAG_LOST;
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        if (i != winner && gContestMonTotalPoints[winner] - gContestMonTotalPoints[i] <= 50)
            gContestResources->tv[i].loserFlags |= CONTESTLIVE_FLAG_LOST_SMALL_MARGIN;

        if (!gContestResources->tv[i].madeExcitingAppeal)
            gContestResources->tv[i].loserFlags |= CONTESTLIVE_FLAG_NO_EXCITEMENT;

        for (j = 0; j < CONTESTANT_COUNT; j++)
        {
            if (gContestMonRound1Points[i] < gContestMonRound1Points[j])
                break;
        }
        if (j == CONTESTANT_COUNT && gContestFinalStandings[i] != 0)
            gContestResources->tv[i].loserFlags |= CONTESTLIVE_FLAG_BLEW_LEAD;

        notLastInRound1 = FALSE;
        notLastInRound2 = FALSE;
        for (j = 0; j < CONTESTANT_COUNT; j++)
        {
            if (gContestMonRound1Points[i] > gContestMonRound1Points[j])
                notLastInRound1 = TRUE;
            if (gContestMonRound2Points[i] > gContestMonRound2Points[j])
                notLastInRound2 = TRUE;
        }
        if (!notLastInRound1 && !notLastInRound2)
            gContestResources->tv[i].loserFlags |= CONTESTLIVE_FLAG_LAST_BOTH_ROUNDS;

        if (!gContestResources->tv[i].madeAppeal)
            gContestResources->tv[i].loserFlags |= CONTESTLIVE_FLAG_NO_APPEALS;
    }

    // Get what moves the winner used and how many times they used them
    for (i = 0; i < CONTEST_NUM_APPEALS; i++)
    {
        appealMoves[i] = MOVE_NONE;
        numMoveUses[i] = 0;
    }
    appealMoves[CONTEST_NUM_APPEALS] = APPEAL_MOVES_END;
    numMoveUses[CONTEST_NUM_APPEALS] = 0;

    for (i = 0; i < CONTEST_NUM_APPEALS; i++)
    {
        if (gContestResources->tv[winner].appeals[i] != MOVE_NONE)
        {
            for (j = 0; j < CONTEST_NUM_APPEALS; j++)
            {
                if (gContestResources->tv[winner].appeals[i] != appealMoves[j])
                {
                    if (appealMoves[j] == MOVE_NONE)
                    {
                        appealMoves[j] = gContestResources->tv[winner].appeals[i];
                        numMoveUses[j]++;
                    }
                }
                else
                {
                    numMoveUses[j]++;
                }
            }
        }
    }

    // Choose an appeal move to comment on for the winner (most commonly used)
    moveCandidates[0] = appealMoves[0];
    mostUses = numMoveUses[0];
    numMoveCandidates = 0;
    for (i = 1; appealMoves[i] != APPEAL_MOVES_END; i++)
    {
        if (mostUses < numMoveUses[i])
        {
            moveCandidates[0] = appealMoves[i];
            mostUses = numMoveUses[i];
            numMoveCandidates = 1;
        }
        else if (mostUses == numMoveUses[i])
        {
            moveCandidates[numMoveCandidates] = appealMoves[i];
            numMoveCandidates++;
        }
    }

    gContestResources->tv[winner].move = moveCandidates[Random() % numMoveCandidates];
}

static void SetConestLiveUpdateTVData(void)
{
    s32 i;
    u32 flags;
    u8 winner;
    u8 round1Placing, round2Placing;
    u8 count;
    u8 randAction;
    u8 numLoserCandidates;
    u8 flagId;
    u16 winnerFlag;
    u8 loserFlag;
    u8 loser;
    u8 loserCandidates[CONTESTANT_COUNT - 1];

    // Players mon didn't win, don't generate show
    if (gContestFinalStandings[gContestPlayerMonIndex] != 0)
        return;

    // Get winner id (unnecessary, we now know it's gContestPlayerMonIndex)
    winner = 0;
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        if (gContestFinalStandings[i] == 0)
            winner = i;
    }

    // Get winner's placement in Round 1 and 2
    round1Placing = 0;
    round2Placing = 0;
    for (i = 0; i < CONTESTANT_COUNT; i++)
    {
        if (gContestMonRound1Points[winner] < gContestMonRound1Points[i])
            round1Placing++;
        if (gContestMonRound2Points[winner] < gContestMonRound2Points[i])
            round2Placing++;
    }

    // Count how many TV comment-worthy actions the winner took
    flags = gContestResources->tv[winner].winnerFlags;
    count = 0;
    for (i = 0; i < 8; flags >>= 1, i++)
    {
        if (flags & 1)
            count++;
    }

    // Randomly choose one of these actions to comment on
    randAction = Random() % count;
    flags = gContestResources->tv[winner].winnerFlags;
    count = 0;
    flagId = 0;
    for (i = 0; i < 8; flags >>= 1, flagId++, i++)
    {
        if (!(flags & 1))
            continue;
        if (randAction == count)
            break;
        count++;
    }
    winnerFlag = 1 << flagId;

    // Pick a losing player with the highest severity of bad actions to comment on
    if (winner == 0)
    {
        loserCandidates[0] = 1;
        loserFlag = gContestResources->tv[1].loserFlags;
        i = 2;
    }
    else
    {
        loserCandidates[0] = 0;
        loserFlag = gContestResources->tv[0].loserFlags;
        i = 1;
    }

    numLoserCandidates = 1;
    for (; i < CONTESTANT_COUNT; i++)
    {
        if (i != winner)
        {
            if (loserFlag < gContestResources->tv[i].loserFlags)
            {
                // Losing player currently has the worst (highest) set of flags, only candidate
                loserCandidates[0] = i;
                loserFlag = gContestResources->tv[i].loserFlags;
                numLoserCandidates = 1;
            }
            else if (loserFlag == gContestResources->tv[i].loserFlags)
            {
                // Tie, increment number of loser candidates
                loserCandidates[numLoserCandidates] = i;
                numLoserCandidates++;
            }
        }
    }
    loser = loserCandidates[Random() % numLoserCandidates];

    // Choose the "worst" action to comment on (flag with highest value)
    flagId = CONTESTLIVE_FLAG_NO_APPEALS;
    for (i = 0; i < 8; flagId >>= 1, i++)
    {
        loserFlag = gContestResources->tv[loser].loserFlags & flagId;
        if (loserFlag)
            break;
    }

    ContestLiveUpdates_Init(round1Placing);
    ContestLiveUpdates_SetRound2Placing(round2Placing);
    ContestLiveUpdates_SetWinnerAppealFlag(winnerFlag);
    ContestLiveUpdates_SetWinnerMoveUsed(gContestResources->tv[winner].move);
    ContestLiveUpdates_SetLoserData(loserFlag, loser);
}

// Unused
void ContestDebugToggleBitfields(bool8 loserFlags)
{
    if (eContestDebugMode == CONTEST_DEBUG_MODE_OFF)
    {
        if (!loserFlags)
            eContestDebugMode = CONTEST_DEBUG_MODE_PRINT_WINNER_FLAGS;
        else
            eContestDebugMode = CONTEST_DEBUG_MODE_PRINT_LOSER_FLAGS;
    }
    else
    {
        eContestDebugMode = CONTEST_DEBUG_MODE_OFF;
    }

    if (eContestDebugMode == CONTEST_DEBUG_MODE_OFF)
    {
        DrawContestantWindowText();
        SwapMoveDescAndContestTilemaps();
    }
    else
    {
        ContestDebugPrintBitStrings();
    }
}

static void ContestDebugPrintBitStrings(void)
{
    u8 i;
    s8 j;
    u8 text1[20];
    u8 text2[20];
    u8 *txtPtr;
    u32 bits;

    if (!gEnableContestDebugging)
        return;

    if (eContestDebugMode != CONTEST_DEBUG_MODE_PRINT_WINNER_FLAGS && eContestDebugMode != CONTEST_DEBUG_MODE_PRINT_LOSER_FLAGS)
        return;

    for (i = 0; i < CONTESTANT_COUNT; i++)
        FillWindowPixelBuffer(i, PIXEL_FILL(0));

    if (eContestDebugMode == CONTEST_DEBUG_MODE_PRINT_WINNER_FLAGS)
    {
        for (i = 0; i < CONTESTANT_COUNT; i++)
        {
            txtPtr = StringCopy(text1, gText_CDot);
            Contest_PrintTextToBg0WindowAt(gContestantTurnOrder[i], text1, 5, 1, FONT_NARROW);
            bits = gContestResources->tv[i].winnerFlags;
            for (j = 7; j > -1; j--) // Weird loop.
            {
                txtPtr = ConvertIntToDecimalStringN(txtPtr, bits & 1, STR_CONV_MODE_LEFT_ALIGN, 1);
                bits >>= 1;
            }

            for (j = 0; j < 5; j++)
                text2[j] = text1[j];

            text2[j] = EOS;
            Contest_PrintTextToBg0WindowAt(gContestantTurnOrder[i], text2, 5, 1, 7);
            Contest_PrintTextToBg0WindowAt(gContestantTurnOrder[i], text1 + j, 55, 1, FONT_NARROW);
        }
    }
    else // CONTEST_DEBUG_MODE_PRINT_LOSER_FLAGS
    {
        for (i = 0; i < CONTESTANT_COUNT; i++)
        {
            StringCopy(text1, gText_BDot);
            bits = gContestResources->tv[i].loserFlags;
            txtPtr = &text1[2];
            for (j = 7; j > -1; j--) // Weird loop.
            {
                txtPtr = ConvertIntToDecimalStringN(txtPtr, bits & 1, STR_CONV_MODE_LEFT_ALIGN, 1);
                bits >>= 1;
            }

            for (j = 0; j < 5; j++)
                text2[j] = text1[j];

            text2[j] = EOS;
            Contest_PrintTextToBg0WindowAt(gContestantTurnOrder[i], text2, 5, 1, FONT_NARROW);
            Contest_PrintTextToBg0WindowAt(gContestantTurnOrder[i], text1 + j, 55, 1, FONT_NARROW);
        }
    }
    SwapMoveDescAndContestTilemaps();
}

static u8 GetMonNicknameLanguage(u8 *nickname)
{
    u8 ret = GAME_LANGUAGE;

    if (nickname[0] == EXT_CTRL_CODE_BEGIN && nickname[1] == EXT_CTRL_CODE_JPN)
        return GAME_LANGUAGE;

    if (StringLength(nickname) <= 5)
    {
        // Name is short enough that it might be Japanese.
        // Make sure  all the character values are valid latin name characters.
        while (*nickname != EOS)
        {
            if ((*nickname >= CHAR_A && *nickname <= CHAR_z)
                || (*nickname >= CHAR_0 && *nickname <= CHAR_9)
                || *nickname == CHAR_SPACE
                || *nickname == CHAR_PERIOD
                || *nickname == CHAR_COMMA
                || *nickname == CHAR_EXCL_MARK
                || *nickname == CHAR_QUESTION_MARK
                || *nickname == CHAR_MALE
                || *nickname == CHAR_FEMALE
                || *nickname == CHAR_SLASH
                || *nickname == CHAR_HYPHEN
                || *nickname == CHAR_ELLIPSIS
                || *nickname == CHAR_DBL_QUOTE_LEFT
                || *nickname == CHAR_DBL_QUOTE_RIGHT
                || *nickname == CHAR_SGL_QUOTE_LEFT
#ifdef BUGFIX
                || *nickname == CHAR_SGL_QUOTE_RIGHT
#else
                || *nickname == CHAR_DBL_QUOTE_LEFT // Most likely a typo, CHAR_SGL_QUOTE_RIGHT should be here instead.
#endif
                )
            {
                nickname++;
            }
            else
            {
                // Invalid latin name character, assume the name was Japanese.
                ret = LANGUAGE_JAPANESE;
                break;
            }
        }
    }

    return ret;
}

static void StripPlayerNameForLinkContest(u8 *playerName)
{
    u8 chr = playerName[5];

    playerName[5] = EOS;
    playerName[PLAYER_NAME_LENGTH] = chr;
}

static void StripMonNameForLinkContest(u8 *monName, s32 language)
{
    u8 chr;

    StripExtCtrlCodes(monName);
    if (language == LANGUAGE_JAPANESE)
    {
        monName[5] = EOS;
        monName[POKEMON_NAME_LENGTH] = EXT_CTRL_CODE_BEGIN;
    }
    else
    {
        chr = monName[5];
        monName[5] = EOS;
        monName[POKEMON_NAME_LENGTH] = chr;
    }
}

void StripPlayerAndMonNamesForLinkContest(struct ContestPokemon *mon, s32 language)
{
    u8 *name = mon->nickname;

    if (language == LANGUAGE_JAPANESE)
    {
        ConvertInternationalString(name, GetMonNicknameLanguage(name));
    }
    else if (name[POKEMON_NAME_LENGTH] == EXT_CTRL_CODE_BEGIN)
    {
        ConvertInternationalString(name, LANGUAGE_JAPANESE);
    }
    else
    {
        name[5] = name[POKEMON_NAME_LENGTH];
        name[POKEMON_NAME_LENGTH] = EOS;
    }

    name = mon->trainerName;
    if (language == LANGUAGE_JAPANESE)
    {
        name[7] = EOS;
        name[6] = name[4];
        name[5] = name[3];
        name[4] = name[2];
        name[3] = name[1];
        name[2] = mon->trainerName[0];
        name[1] = EXT_CTRL_CODE_JPN;
        name[0] = EXT_CTRL_CODE_BEGIN;
    }
    else
    {
        name[5] = name[PLAYER_NAME_LENGTH];
        name[PLAYER_NAME_LENGTH] = EOS;
    }
}

