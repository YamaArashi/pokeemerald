#include "global.h"
#include "overworld.h"
#include "berry_blender.h"
#include "bg.h"
#include "window.h"
#include "task.h"
#include "sprite.h"
#include "sound.h"
#include "m4a.h"
#include "bg.h"
#include "palette.h"
#include "decompress.h"
#include "malloc.h"
#include "gpu_regs.h"
#include "text.h"
#include "text_window.h"
#include "event_data.h"
#include "main.h"
#include "link.h"
#include "link_rfu.h"
#include "item_menu_icons.h"
#include "berry.h"
#include "item.h"
#include "string_util.h"
#include "international_string_util.h"
#include "random.h"
#include "menu.h"
#include "pokeblock.h"
#include "trig.h"
#include "tv.h"
#include "item_menu.h"
#include "battle_records.h"
#include "graphics.h"
#include "new_game.h"
#include "save.h"
#include "strings.h"
#include "constants/game_stat.h"
#include "constants/items.h"
#include "constants/rgb.h"
#include "constants/songs.h"

enum {
    SCORE_BEST,
    SCORE_GOOD,
    SCORE_MISS,
    NUM_SCORE_TYPES,
};

// Redundant with the above. Reversed
enum {
    PROXIMITY_MISS,
    PROXIMITY_GOOD,
    PROXIMITY_BEST,
};

enum {
    SCOREANIM_GOOD,
    SCOREANIM_MISS,
    SCOREANIM_BEST_FLASH,
    SCOREANIM_BEST_STATIC,
};

enum {
    PLAY_AGAIN_YES,
    PLAY_AGAIN_NO,
    CANT_PLAY_NO_BERRIES,
    CANT_PLAY_NO_PKBLCK_SPACE
};

enum {
    BLENDER_MISTER,
    BLENDER_LADDIE,
    BLENDER_LASSIE,
    BLENDER_MASTER,
    BLENDER_DUDE,
    BLENDER_MISS
};

#define BLENDER_MAX_PLAYERS MAX_LINK_PLAYERS

#define NO_PLAYER 0xFF

#define MAX_PROGRESS_BAR 1000

#define MAX_ARROW_POS 0x10000 // By virtue of being u16
#define MIN_ARROW_SPEED 0x80
#define ARROW_FALL_ROTATION 0x5800 // The amount the arrow spins as it falls in at the start

// Tile offsets
#define PROGRESS_BAR_FILLED_TOP    0x80E9
#define PROGRESS_BAR_FILLED_BOTTOM 0x80F9
#define PROGRESS_BAR_EMPTY_TOP     0x80E1
#define PROGRESS_BAR_EMPTY_BOTTOM  0x80F1
#define RPM_DIGIT 0x8072

// Tile and palette tags
#define GFXTAG_COUNTDOWN_NUMBERS 12345
#define GFXTAG_START             12346
#define GFXTAG_PARTICLES         23456
#define GFXTAG_PLAYER_ARROW      46545
#define GFXTAG_SCORE_SYMBOLS     48888

#define PALTAG_PLAYER_ARROW      12312
#define PALTAG_MISC              46546

// Last berry that an NPC can put in
#define NUM_NPC_BERRIES ITEM_TO_BERRY(ITEM_ASPEAR_BERRY)

enum {
    // Windows 0-3 are used implicitly in several loops over BLENDER_MAX_PLAYERS
    // i.e. window 0 is for player 1, window 1 for player 2, etc.
    WIN_MSG = BLENDER_MAX_PLAYERS,
    WIN_RESULTS,
};

struct BlenderBerry
{
    u16 itemId;
    u8 name[BERRY_NAME_LENGTH + 1];
    u8 flavors[FLAVOR_COUNT + 1]; // 5 flavors, + 1 for feel
};

struct TimeAndRPM
{
    u32 time;
    u16 maxRPM;
};

struct BlenderGameBlock
{
    struct TimeAndRPM timeRPM;
    u16 scores[BLENDER_MAX_PLAYERS][NUM_SCORE_TYPES];
};

struct TvBlenderStruct
{
    u8 name[11];
    u8 pokeblockFlavor;
    u8 pokeblockColor;
    u8 pokeblockSheen;
};

struct BerryBlender
{
    u8 mainState;
    u8 loadGfxState;
    u8 unused0[66];
    u16 unk0; // never read
    u8 scoreIconIds[NUM_SCORE_TYPES];
    u16 arrowPos;
    s16 speed;
    u16 maxRPM;
    u8 playerArrowSpriteIds[BLENDER_MAX_PLAYERS];
    u8 playerArrowSpriteIds2[BLENDER_MAX_PLAYERS];
    u8 unused1[11];
    u8 gameEndState;
    u16 playerContinueResponses[BLENDER_MAX_PLAYERS];
    u16 canceledPlayerCmd;
    u16 canceledPlayerId;
    u16 playAgainState;
    u8 slowdownTimer;
    u16 chosenItemId[BLENDER_MAX_PLAYERS];
    u8 numPlayers;
    u8 unused2[16];
    u16 arrowIdToPlayerId[BLENDER_MAX_PLAYERS];
    u16 playerIdToArrowId[BLENDER_MAX_PLAYERS];
    u8 yesNoAnswer;
    u8 stringVar[100];
    u32 gameFrameTime;
    s32 framesToWait;
    u32 unk1; // never read
    u8 unused3[4];
    u8 playerToThrowBerry;
    u16 progressBarValue;
    u16 maxProgressBarValue;
    u16 centerScale;
    u16 bg_X;
    u16 bg_Y;
    u8 opponentTaskIds[BLENDER_MAX_PLAYERS - 1];
    u8 perfectOpponents; // for debugging, NPCs will always hit Best
    u16 scores[BLENDER_MAX_PLAYERS][NUM_SCORE_TYPES];
    u8 playerPlaces[BLENDER_MAX_PLAYERS];
    struct BgAffineSrcData bgAffineSrc;
    u16 savedMusic;
    struct BlenderBerry blendedBerries[BLENDER_MAX_PLAYERS];
    struct TimeAndRPM smallBlock;
    u32 linkPlayAgainState;
    u8 ownRanking;
    struct TvBlenderStruct tvBlender;
    u8 tilemapBuffers[2][BG_SCREEN_SIZE];
    s16 textState;
    void *tilesBuffer;
    struct BlenderGameBlock gameBlock;
};

static void SetBgPos(void);
static void Task_HandleOpponent1(u8);
static void Task_HandleOpponent2(u8);
static void Task_HandleOpponent3(u8);
static void Task_HandleBerryMaster(u8);
static void Task_PlayPokeblockFanfare(u8);
static void SpriteCB_PlayerArrow(struct Sprite *);
static void SpriteCB_ScoreSymbol(struct Sprite *);
static void SpriteCB_CountdownNumber(struct Sprite *);
static void SpriteCB_Start(struct Sprite *);
static void SpriteCB_ScoreSymbolBest(struct Sprite *);
static void InitLocalPlayers(u8);
static void CB2_LoadBerryBlender(void);
static void UpdateBlenderCenter(void);
static bool32 PrintMessage(s16 *, const u8 *, s32 );
static void StartBlender(void);
static void CB2_StartBlenderLink(void);
static void CB2_StartBlenderLocal(void);
static void Blender_DummiedOutFunc(s16, s16);
static void CB2_PlayBlender(void);
static void DrawBlenderCenter(struct BgAffineSrcData *);
static bool8 UpdateBlenderLandScreenShake(void);
static void SetPlayerIdMaps(void);
static void PrintPlayerNames(void);
static void InitBlenderBgs(void);
static void SetPlayerBerryData(u8, u16);
static void Blender_AddTextPrinter(u8, const u8 *, u8, u8, s32, s32);
static void ResetLinkCmds(void);
static void CreateParticleSprites(void);
static void ShakeBgCoordForHit(s16 *, u16);
static void TryUpdateProgressBar(u16, u16);
static void UpdateRPM(u16);
static void RestoreBgCoords(void);
static void ProcessLinkPlayerCmds(void);
static void CB2_EndBlenderGame(void);
static bool8 PrintBlendingRanking(void);
static bool8 PrintBlendingResults(void);
static void CB2_CheckPlayAgainLocal(void);
static void CB2_CheckPlayAgainLink(void);
static void UpdateProgressBar(u16, u16);
static void PrintMadePokeblockString(struct Pokeblock *, u8 *);
static bool32 TryAddContestLinkTvShow(struct Pokeblock *, struct TvBlenderStruct *);

EWRAM_DATA static struct BerryBlender *sBerryBlender = NULL;
EWRAM_DATA static s32 sDebug_PokeblockFactorFlavors[FLAVOR_COUNT] = {0};
EWRAM_DATA static s32 sDebug_PokeblockFactorFlavorsAfterRPM[FLAVOR_COUNT] = {0};
EWRAM_DATA static u32 sDebug_PokeblockFactorRPM = 0;

static s16 sPokeblockFlavors[FLAVOR_COUNT + 1]; // + 1 for feel
static s16 sPokeblockPresentFlavors[FLAVOR_COUNT + 1];
static s16 sDebug_MaxRPMStage;
static s16 sDebug_GameTimeStage;

COMMON_DATA u8 gInGameOpponentsNo = 0;

static const u16 sBlenderCenter_Pal[] = INCBIN_U16("graphics/berry_blender/center.gbapal");
static const u8 sBlenderCenter_Tilemap[] = INCBIN_U8("graphics/berry_blender/center_map.bin");
static const u16 sBlenderOuter_Pal[] = INCBIN_U16("graphics/berry_blender/outer.gbapal");

static const u16 sUnused_Pal[] = INCBIN_U16("graphics/berry_blender/unused.gbapal");
static const u16 sEmpty_Pal[16 * 14] = {0};

// unused text
static const u8 sUnusedText_YesNo[] = _("YES\nNO");
static const u8 sUnusedText_2[] = _("▶");
static const u8 sUnusedText_Space[] = _(" ");
static const u8 sUnusedText_Terminating[] = _("Terminating.");
static const u8 sUnusedText_LinkPartnerNotFound[] = _("Link partner(s) not found.\nPlease try again.\p");

static const u8 sText_BerryBlenderStart[] = _("Starting up the BERRY BLENDER.\pPlease select a BERRY from your BAG\nto put in the BERRY BLENDER.\p");
static const u8 sText_NewParagraph[] = _("\p");
static const u8 sText_WasMade[] = _(" was made!");
static const u8 sText_Mister[] = _("MISTER");
static const u8 sText_Laddie[] = _("LADDIE");
static const u8 sText_Lassie[] = _("LASSIE");
static const u8 sText_Master[] = _("MASTER");
static const u8 sText_Dude[] = _("DUDE");
static const u8 sText_Miss[] = _("MISS");

static const u8 *const sBlenderOpponentsNames[] =
{
    [BLENDER_MISTER] = sText_Mister,
    [BLENDER_LADDIE] = sText_Laddie,
    [BLENDER_LASSIE] = sText_Lassie,
    [BLENDER_MASTER] = sText_Master,
    [BLENDER_DUDE]   = sText_Dude,
    [BLENDER_MISS]   = sText_Miss
};

static const u8 sText_PressAToStart[] = _("Press the A Button to start.");
static const u8 sText_PleaseWaitAWhile[] = _("Please wait a while.");
static const u8 sText_CommunicationStandby[] = _("Communication standby…");
static const u8 sText_WouldLikeToBlendAnotherBerry[] = _("Would you like to blend another BERRY?");
static const u8 sText_RunOutOfBerriesForBlending[] = _("You've run out of BERRIES for\nblending in the BERRY BLENDER.\p");
static const u8 sText_YourPokeblockCaseIsFull[] = _("Your {POKEBLOCK} CASE is full.\p");
static const u8 sText_HasNoBerriesToPut[] = _(" has no BERRIES to put in\nthe BERRY BLENDER.");
static const u8 sText_ApostropheSPokeblockCaseIsFull[] = _("'s {POKEBLOCK} CASE is full.\p");
static const u8 sText_BlendingResults[] = _("RESULTS OF BLENDING");
static const u8 sText_BerryUsed[] = _("BERRY USED");
static const u8 sText_SpaceBerry[] = _(" BERRY");
static const u8 sText_Time[] = _("Time:");
static const u8 sText_Min[] = _(" min. ");
static const u8 sText_Sec[] = _(" sec.");
static const u8 sText_MaximumSpeed[] = _("MAXIMUM SPEED");
static const u8 sText_RPM[] = _(" RPM");
static const u8 sText_Dot[] = _(".");
static const u8 sText_NewLine[] = _("\n");
static const u8 sText_Space[] = _(" ");
static const u8 sText_Ranking[] = _("RANKING");
static const u8 sText_TheLevelIs[] = _("The level is ");
static const u8 sText_TheFeelIs[] = _(", and the feel is ");
static const u8 sText_Dot2[] = _(".");

static const struct BgTemplate sBgTemplates[3] =
{
    {
        .bg = 0,
        .charBaseIndex = 3,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    },
    {
        .bg = 1,
        .charBaseIndex = 2,
        .mapBaseIndex = 12,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 8,
        .screenSize = 1,
        .paletteMode = 1,
        .priority = 0,
        .baseTile = 0,
    }
};

static const struct WindowTemplate sWindowTemplates[] =
{
    { // Player 1
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 6,
        .width = 7,
        .height = 2,
        .paletteNum = 14,
        .baseBlock = 0x28,
    },
    { // Player 2
        .bg = 0,
        .tilemapLeft = 22,
        .tilemapTop = 6,
        .width = 7,
        .height = 2,
        .paletteNum = 14,
        .baseBlock = 0x36,
    },
    { // Player 3
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 12,
        .width = 7,
        .height = 2,
        .paletteNum = 14,
        .baseBlock = 0x44,
    },
    { // Player 4
        .bg = 0,
        .tilemapLeft = 22,
        .tilemapTop = 12,
        .width = 7,
        .height = 2,
        .paletteNum = 14,
        .baseBlock = 0x52,
    },
    [WIN_MSG] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 27,
        .height = 4,
        .paletteNum = 14,
        .baseBlock = 0x60,
    },
    [WIN_RESULTS] = {
        .bg = 0,
        .tilemapLeft = 5,
        .tilemapTop = 3,
        .width = 21,
        .height = 14,
        .paletteNum = 14,
        .baseBlock = 0x60,
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sYesNoWindowTemplate_ContinuePlaying =
{
    .bg = 0,
    .tilemapLeft = 21,
    .tilemapTop = 9,
    .width = 5,
    .height = 4,
    .paletteNum = 14,
    .baseBlock = 0xCC
};

static const s8 sPlayerArrowQuadrant[BLENDER_MAX_PLAYERS][2] =
{
    {-1, -1},
    { 1, -1},
    {-1,  1},
    { 1,  1}
};

static const u8 sPlayerArrowPos[BLENDER_MAX_PLAYERS][2] =
{
    { 72,  32},
    {168,  32},
    { 72, 128},
    {168, 128}
};

static const u8 sPlayerIdMap[BLENDER_MAX_PLAYERS - 1][BLENDER_MAX_PLAYERS] =
{
    {NO_PLAYER, 0, 1, NO_PLAYER}, // 2 Players
    {NO_PLAYER, 0, 1,         2}, // 3 Players
    {        0, 1, 2,         3}  // 4 Players
};


// Blender arrow positions:
//
//           0x0000 (limit 0x10000)
//            .  .
//         .        .
// 0x4000 .          . 0xC000
//        .          .
//         .        .
//            .  .
//           0x8000
//
static const u16 sArrowStartPos[] = {
    0,
    MAX_ARROW_POS / 4 * 3, // 0xC000
    MAX_ARROW_POS / 4,     // 0x4000
    MAX_ARROW_POS / 4 * 2  // 0x8000
};
static const u8 sArrowStartPosIds[BLENDER_MAX_PLAYERS - 1] = {1, 1, 0};
static const u8 sArrowHitRangeStart[BLENDER_MAX_PLAYERS] = {32, 224, 96, 160};

static const TaskFunc sLocalOpponentTasks[] =
{
    Task_HandleOpponent1,
    Task_HandleOpponent2,
    Task_HandleOpponent3
};

static const struct OamData sOam_PlayerArrow =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sAnim_PlayerArrow_TopLeft[] =
{
    ANIMCMD_FRAME(16, 5, .vFlip = TRUE, .hFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PlayerArrow_TopRight[] =
{
    ANIMCMD_FRAME(16, 5, .vFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PlayerArrow_BottomLeft[] =
{
    ANIMCMD_FRAME(16, 5, .hFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PlayerArrow_BottomRight[] =
{
    ANIMCMD_FRAME(16, 5, 0, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PlayerArrow_TopLeft_Flash[] =
{
    ANIMCMD_FRAME(48, 2, .vFlip = TRUE, .hFlip = TRUE),
    ANIMCMD_FRAME(32, 5, .vFlip = TRUE, .hFlip = TRUE),
    ANIMCMD_FRAME(48, 3, .vFlip = TRUE, .hFlip = TRUE),
    ANIMCMD_FRAME(16, 5, .vFlip = TRUE, .hFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PlayerArrow_TopRight_Flash[] =
{
    ANIMCMD_FRAME(48, 2, .vFlip = TRUE),
    ANIMCMD_FRAME(32, 5, .vFlip = TRUE),
    ANIMCMD_FRAME(48, 3, .vFlip = TRUE),
    ANIMCMD_FRAME(16, 5, .vFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PlayerArrow_BottomLeft_Flash[] =
{
    ANIMCMD_FRAME(48, 2, .hFlip = TRUE),
    ANIMCMD_FRAME(32, 5, .hFlip = TRUE),
    ANIMCMD_FRAME(48, 3, .hFlip = TRUE),
    ANIMCMD_FRAME(16, 5, .hFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PlayerArrow_BottomRight_Flash[] =
{
    ANIMCMD_FRAME(48, 2, 0, 0),
    ANIMCMD_FRAME(32, 5, 0, 0),
    ANIMCMD_FRAME(48, 3, 0, 0),
    ANIMCMD_FRAME(16, 5, 0, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PlayerArrow_TopLeft_Off[] =
{
    ANIMCMD_FRAME(0, 5, .vFlip = TRUE, .hFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PlayerArrow_TopRight_Off[] =
{
    ANIMCMD_FRAME(0, 5, .vFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PlayerArrow_BottomLeft_Off[] =
{
    ANIMCMD_FRAME(0, 5, .hFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PlayerArrow_BottomRight_Off[] =
{
    ANIMCMD_FRAME(0, 5, 0, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_PlayerArrow[] =
{
    sAnim_PlayerArrow_TopLeft,
    sAnim_PlayerArrow_TopRight,
    sAnim_PlayerArrow_BottomLeft,
    sAnim_PlayerArrow_BottomRight,
    sAnim_PlayerArrow_TopLeft_Flash,
    sAnim_PlayerArrow_TopRight_Flash,
    sAnim_PlayerArrow_BottomLeft_Flash,
    sAnim_PlayerArrow_BottomRight_Flash,
    sAnim_PlayerArrow_TopLeft_Off,
    sAnim_PlayerArrow_TopRight_Off,
    sAnim_PlayerArrow_BottomLeft_Off,
    sAnim_PlayerArrow_BottomRight_Off
};

static const struct SpriteSheet sSpriteSheet_PlayerArrow =
{
    gBerryBlenderPlayerArrow_Gfx, 0x800, GFXTAG_PLAYER_ARROW
};

static const struct SpritePalette sSpritePal_BlenderMisc =
{
    gBerryBlenderMiscPalette, PALTAG_MISC
};

static const struct SpritePalette sSpritePal_PlayerArrow =
{
    gBerryBlenderArrowPalette, PALTAG_PLAYER_ARROW
};

static const struct SpriteTemplate sSpriteTemplate_PlayerArrow =
{
    .tileTag = GFXTAG_PLAYER_ARROW,
    .paletteTag = PALTAG_PLAYER_ARROW,
    .oam = &sOam_PlayerArrow,
    .anims = sAnims_PlayerArrow,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_PlayerArrow
};

static const struct OamData sOam_ScoreSymbols =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sAnim_ScoreSymbols_Good[] =
{
    ANIMCMD_FRAME(0, 20),
    ANIMCMD_END
};

static const union AnimCmd sAnim_ScoreSymbols_Miss[] =
{
    ANIMCMD_FRAME(4, 20, 1, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_ScoreSymbols_BestFlash[] =
{
    ANIMCMD_FRAME(8, 4),
    ANIMCMD_FRAME(12, 4),
    ANIMCMD_FRAME(8, 4),
    ANIMCMD_FRAME(12, 4),
    ANIMCMD_FRAME(8, 4),
    ANIMCMD_END
};

static const union AnimCmd sAnim_ScoreSymbols_BestStatic[] =
{
    ANIMCMD_FRAME(8, 4),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_ScoreSymbols[] =
{
    [SCOREANIM_GOOD]        = sAnim_ScoreSymbols_Good,
    [SCOREANIM_MISS]        = sAnim_ScoreSymbols_Miss,
    [SCOREANIM_BEST_FLASH]  = sAnim_ScoreSymbols_BestFlash,
    [SCOREANIM_BEST_STATIC] = sAnim_ScoreSymbols_BestStatic,
};

static const struct SpriteSheet sSpriteSheet_ScoreSymbols =
{
    gBerryBlenderScoreSymbols_Gfx, 0x200, GFXTAG_SCORE_SYMBOLS
};

static const struct SpriteTemplate sSpriteTemplate_ScoreSymbols =
{
    .tileTag = GFXTAG_SCORE_SYMBOLS,
    .paletteTag = PALTAG_MISC,
    .oam = &sOam_ScoreSymbols,
    .anims = sAnims_ScoreSymbols,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_ScoreSymbol
};

static const struct OamData sOam_Particles =
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
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sAnim_SparkleCrossToX[] =
{
    ANIMCMD_FRAME(0, 3),
    ANIMCMD_FRAME(1, 4),
    ANIMCMD_FRAME(3, 5),
    ANIMCMD_FRAME(1, 4),
    ANIMCMD_FRAME(0, 3),
    ANIMCMD_END
};

static const union AnimCmd sAnim_SparkleXToCross[] =
{
    ANIMCMD_FRAME(0, 3),
    ANIMCMD_FRAME(2, 4),
    ANIMCMD_FRAME(4, 5),
    ANIMCMD_FRAME(2, 4),
    ANIMCMD_FRAME(0, 3),
    ANIMCMD_END
};

static const union AnimCmd sAnim_SparkleFull[] =
{
    ANIMCMD_FRAME(0, 2),
    ANIMCMD_FRAME(1, 2),
    ANIMCMD_FRAME(2, 2),
    ANIMCMD_FRAME(4, 4),
    ANIMCMD_FRAME(3, 3),
    ANIMCMD_FRAME(2, 2),
    ANIMCMD_FRAME(1, 2),
    ANIMCMD_FRAME(0, 2),
    ANIMCMD_END
};

static const union AnimCmd sAnim_GreenArrow[] =
{
    ANIMCMD_FRAME(5, 5, 1, 1),
    ANIMCMD_END
};

static const union AnimCmd sAnim_GreenDot[] =
{
    ANIMCMD_FRAME(6, 5, 1, 1),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_Particles[] =
{
    sAnim_SparkleCrossToX, // Only this effect is ever used, rest go unused
    sAnim_SparkleXToCross,
    sAnim_SparkleFull,
    sAnim_GreenArrow,
    sAnim_GreenDot,
};

static const struct SpriteSheet sSpriteSheet_Particles =
{
    gBerryBlenderParticles_Gfx, 0xE0, GFXTAG_PARTICLES
};

static const struct SpriteTemplate sSpriteTemplate_Particles =
{
    .tileTag = GFXTAG_PARTICLES,
    .paletteTag = PALTAG_MISC,
    .oam = &sOam_Particles,
    .anims = sAnims_Particles,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct OamData sOam_CountdownNumbers =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sAnim_CountdownNumbers_3[] =
{
    ANIMCMD_FRAME(32, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_CountdownNumbers_2[] =
{
    ANIMCMD_FRAME(16, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_CountdownNumbers_1[] =
{
    ANIMCMD_FRAME(0, 30),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_CountdownNumbers[] =
{
    sAnim_CountdownNumbers_3,
    sAnim_CountdownNumbers_2,
    sAnim_CountdownNumbers_1,
};

static const struct SpriteSheet sSpriteSheet_CountdownNumbers =
{
    gBerryBlenderCountdownNumbers_Gfx, 0x600, GFXTAG_COUNTDOWN_NUMBERS
};

static const struct SpriteTemplate sSpriteTemplate_CountdownNumbers =
{
    .tileTag = GFXTAG_COUNTDOWN_NUMBERS,
    .paletteTag = PALTAG_MISC,
    .oam = &sOam_CountdownNumbers,
    .anims = sAnims_CountdownNumbers,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_CountdownNumber
};

static const struct OamData sOam_Start =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sAnim_Start[] =
{
    ANIMCMD_FRAME(0, 30),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_Start[] =
{
    sAnim_Start,
};

static const struct SpriteSheet sSpriteSheet_Start =
{
    gBerryBlenderStart_Gfx, 0x400, GFXTAG_START
};

static const struct SpriteTemplate sSpriteTemplate_Start =
{
    .tileTag = GFXTAG_START,
    .paletteTag = PALTAG_MISC,
    .oam = &sOam_Start,
    .anims = sAnims_Start,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Start
};

// Data for throwing the berries in at the start
// x, y, bounce speed, x speed, y speed
static const s16 sBerrySpriteData[][5] =
{
    {-10,  20,  10,   2,   1},
    {250,  20,  10,  -2,   1},
    {-10, 140,  10,   2,  -1},
    {250, 140,  10,  -2,  -1},
};

// There are only 5 different berries the NPCs will ever use
// Each of these sets represents 3 berries chosen to be used by the NPCs
// If the player's berry is one of the 5 possible berries, a set is chosen that excludes it
static const u8 sOpponentBerrySets[NUM_NPC_BERRIES * 2][3] =
{
    // These sets are used if the player chose one of the 5 NPC berries
    {ITEM_TO_BERRY(ITEM_ASPEAR_BERRY) - 1, ITEM_TO_BERRY(ITEM_RAWST_BERRY) - 1,  ITEM_TO_BERRY(ITEM_PECHA_BERRY) - 1},   // player chose Cheri Berry
    {ITEM_TO_BERRY(ITEM_CHERI_BERRY) - 1,  ITEM_TO_BERRY(ITEM_ASPEAR_BERRY) - 1, ITEM_TO_BERRY(ITEM_RAWST_BERRY) - 1},   // player chose Chesto Berry
    {ITEM_TO_BERRY(ITEM_CHESTO_BERRY) - 1, ITEM_TO_BERRY(ITEM_CHERI_BERRY) - 1,  ITEM_TO_BERRY(ITEM_ASPEAR_BERRY) - 1},  // player chose Pecha Berry
    {ITEM_TO_BERRY(ITEM_PECHA_BERRY) - 1,  ITEM_TO_BERRY(ITEM_CHESTO_BERRY) - 1, ITEM_TO_BERRY(ITEM_CHERI_BERRY) - 1},   // player chose Rawst Berry
    {ITEM_TO_BERRY(ITEM_RAWST_BERRY) - 1,  ITEM_TO_BERRY(ITEM_PECHA_BERRY) - 1,  ITEM_TO_BERRY(ITEM_CHESTO_BERRY) - 1},  // player chose Aspear Berry

    // These sets are used if the player chose a different berry (set is selected by player's berry % 5)
    {ITEM_TO_BERRY(ITEM_CHERI_BERRY) - 1,  ITEM_TO_BERRY(ITEM_PECHA_BERRY) - 1,  ITEM_TO_BERRY(ITEM_RAWST_BERRY) - 1},   // player chose Leppa, Figy, ...
    {ITEM_TO_BERRY(ITEM_CHESTO_BERRY) - 1, ITEM_TO_BERRY(ITEM_RAWST_BERRY) - 1,  ITEM_TO_BERRY(ITEM_ASPEAR_BERRY) - 1},  // player chose Oran, Wiki, ...
    {ITEM_TO_BERRY(ITEM_PECHA_BERRY) - 1,  ITEM_TO_BERRY(ITEM_ASPEAR_BERRY) - 1, ITEM_TO_BERRY(ITEM_CHERI_BERRY) - 1},   // player chose Persim, Mago, ...
    {ITEM_TO_BERRY(ITEM_RAWST_BERRY) - 1,  ITEM_TO_BERRY(ITEM_CHERI_BERRY) - 1,  ITEM_TO_BERRY(ITEM_CHESTO_BERRY) - 1},  // player chose Lum, Aguav, ...
    {ITEM_TO_BERRY(ITEM_ASPEAR_BERRY) - 1, ITEM_TO_BERRY(ITEM_CHESTO_BERRY) - 1, ITEM_TO_BERRY(ITEM_PECHA_BERRY) - 1},   // player chose Sitrus, Iapapa, ...
};

// Berry master's berries follow the same rules as above, but instead of explicitly listing
// the alternate sets if the player chooses one of these berries, it implicitly uses these berries - 5, i.e. Tamato - Nomel
static const u8 sBerryMasterBerries[] = {
    ITEM_TO_BERRY(ITEM_SPELON_BERRY) - 1,
    ITEM_TO_BERRY(ITEM_PAMTRE_BERRY) - 1,
    ITEM_TO_BERRY(ITEM_WATMEL_BERRY) - 1,
    ITEM_TO_BERRY(ITEM_DURIN_BERRY) - 1,
    ITEM_TO_BERRY(ITEM_BELUE_BERRY) - 1
};

// "0 players" is link
static const u8 sNumPlayersToSpeedDivisor[] = {1, 1, 2, 3, 4};

// Black pokeblocks will use one of these random combinations of flavors
static const u8 sBlackPokeblockFlavorFlags[] = {
    (1 << FLAVOR_SOUR)   | (1 << FLAVOR_BITTER) | (1 << FLAVOR_SWEET),
    (1 << FLAVOR_SOUR)   | (1 << FLAVOR_SWEET)  | (1 << FLAVOR_DRY),
    (1 << FLAVOR_SOUR)   | (1 << FLAVOR_DRY)    | (1 << FLAVOR_SPICY),
    (1 << FLAVOR_SOUR)   | (1 << FLAVOR_BITTER) | (1 << FLAVOR_DRY),
    (1 << FLAVOR_SOUR)   | (1 << FLAVOR_BITTER) | (1 << FLAVOR_SPICY),
    (1 << FLAVOR_BITTER) | (1 << FLAVOR_SWEET)  | (1 << FLAVOR_DRY),
    (1 << FLAVOR_BITTER) | (1 << FLAVOR_SWEET)  | (1 << FLAVOR_SPICY),
    (1 << FLAVOR_BITTER) | (1 << FLAVOR_DRY)    | (1 << FLAVOR_SPICY),
    (1 << FLAVOR_SWEET)  | (1 << FLAVOR_DRY)    | (1 << FLAVOR_SPICY),
    (1 << FLAVOR_SOUR)   | (1 << FLAVOR_SWEET)  | (1 << FLAVOR_SPICY),
};

static const u8 sJPText_GoodTvReady[] = _("\nいいTVができました "); // Unused
static const u8 sJPText_BadTvReady[] = _("\nダメTVができました "); // Unused
static const u8 sJPText_Flavors[][5] = {_("からい"), _("しぶい"), _("あまい"), _("にがい"), _("すっぱい")}; // Unused

static const u8 sUnused[] = {
    6, 6, 6, 6, 5,
    3, 3, 3, 2, 2,
    3, 3, 3, 3, 2
};

static const struct WindowTemplate sBlenderRecordWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 6,
    .tilemapTop = 4,
    .width = 18,
    .height = 11,
    .paletteNum = 15,
    .baseBlock = 8
};

static void UpdateHitPitch(void)
{
    m4aMPlayPitchControl(&gMPlayInfo_SE2, TRACKS_ALL, 2 * (sBerryBlender->speed - MIN_ARROW_SPEED));
}

static void VBlankCB_BerryBlender(void)
{
    SetBgPos();
    SetBgAffine(2, sBerryBlender->bgAffineSrc.texX, sBerryBlender->bgAffineSrc.texY,
                sBerryBlender->bgAffineSrc.scrX, sBerryBlender->bgAffineSrc.scrY,
                sBerryBlender->bgAffineSrc.sx, sBerryBlender->bgAffineSrc.sy,
                sBerryBlender->bgAffineSrc.alpha);
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static bool8 LoadBerryBlenderGfx(void)
{
    switch (sBerryBlender->loadGfxState)
    {
    case 0:
        sBerryBlender->tilesBuffer = AllocZeroed(GetDecompressedDataSize(gBerryBlenderCenter_Gfx) + 100);
        LZDecompressWram(gBerryBlenderCenter_Gfx, sBerryBlender->tilesBuffer);
        sBerryBlender->loadGfxState++;
        break;
    case 1:
        CopyToBgTilemapBuffer(2, sBlenderCenter_Tilemap, 0x400, 0);
        CopyBgTilemapBufferToVram(2);
        LoadPalette(sBlenderCenter_Pal, BG_PLTT_ID(0), 8 * PLTT_SIZE_4BPP);
        sBerryBlender->loadGfxState++;
        break;
    case 2:
        LoadBgTiles(2, sBerryBlender->tilesBuffer, GetDecompressedDataSize(gBerryBlenderCenter_Gfx), 0);
        sBerryBlender->loadGfxState++;
        break;
    case 3:
        LZDecompressWram(gBerryBlenderOuter_Gfx, sBerryBlender->tilesBuffer);
        sBerryBlender->loadGfxState++;
        break;
    case 4:
        LoadBgTiles(1, sBerryBlender->tilesBuffer, GetDecompressedDataSize(gBerryBlenderOuter_Gfx), 0);
        sBerryBlender->loadGfxState++;
        break;
    case 5:
        LZDecompressWram(gBerryBlenderOuter_Tilemap, sBerryBlender->tilesBuffer);
        sBerryBlender->loadGfxState++;
        break;
    case 6:
        CopyToBgTilemapBuffer(1, sBerryBlender->tilesBuffer, GetDecompressedDataSize(gBerryBlenderOuter_Tilemap), 0);
        CopyBgTilemapBufferToVram(1);
        sBerryBlender->loadGfxState++;
        break;
    case 7:
        LoadPalette(sBlenderOuter_Pal, BG_PLTT_ID(8), PLTT_SIZE_4BPP);
        sBerryBlender->loadGfxState++;
        break;
    case 8:
        LoadSpriteSheet(&sSpriteSheet_PlayerArrow);
        LoadSpriteSheet(&sSpriteSheet_Particles);
        LoadSpriteSheet(&sSpriteSheet_ScoreSymbols);
        sBerryBlender->loadGfxState++;
        break;
    case 9:
        LoadSpriteSheet(&sSpriteSheet_CountdownNumbers);
        LoadSpriteSheet(&sSpriteSheet_Start);
        LoadSpritePalette(&sSpritePal_PlayerArrow);
        LoadSpritePalette(&sSpritePal_BlenderMisc);
        Free(sBerryBlender->tilesBuffer);
        sBerryBlender->loadGfxState = 0;
        return TRUE;
    }

    return FALSE;
}

static void DrawBlenderBg(void)
{
    FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
    CopyBgTilemapBufferToVram(0);
    ShowBg(0);
    ShowBg(1);
    SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
}

static void InitBerryBlenderWindows(void)
{
    if (InitWindows(sWindowTemplates))
    {
        s32 i;

        DeactivateAllTextPrinters();
        // Initialize only the main text windows (player names and message box; excludes results screen)
        for (i = 0; i < WIN_RESULTS; i++)
            FillWindowPixelBuffer(i, PIXEL_FILL(0));

        FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
        Menu_LoadStdPalAt(BG_PLTT_ID(14));
    }
}

// gSpecialVar_0x8004 is the number of NPC opponents
// Set to 0 indicates it's a link blender
void DoBerryBlending(void)
{
    if (sBerryBlender == NULL)
        sBerryBlender = AllocZeroed(sizeof(*sBerryBlender));

    sBerryBlender->gameEndState = 0;
    sBerryBlender->mainState = 0;
    sBerryBlender->gameEndState = 0;

    InitLocalPlayers(gSpecialVar_0x8004);
    SetMainCallback2(CB2_LoadBerryBlender);
}

// Show the blender screen initially and prompt to choose a berry
static void CB2_LoadBerryBlender(void)
{
    s32 i;

    switch (sBerryBlender->mainState)
    {
    case 0:
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetSpriteData();
        FreeAllSpritePalettes();
        SetVBlankCallback(NULL);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(1, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        SetBgTilemapBuffer(1, sBerryBlender->tilemapBuffers[0]);
        SetBgTilemapBuffer(2, sBerryBlender->tilemapBuffers[1]);
        LoadUserWindowBorderGfx(0, 1, BG_PLTT_ID(13));
        LoadMessageBoxGfx(0, 0x14, BG_PLTT_ID(15));
        InitBerryBlenderWindows();

        sBerryBlender->mainState++;
        sBerryBlender->maxProgressBarValue = 0;
        sBerryBlender->progressBarValue = 0;
        sBerryBlender->centerScale = 80;
        sBerryBlender->bg_X = 0;
        sBerryBlender->bg_Y = 0;
        sBerryBlender->loadGfxState = 0;

        UpdateBlenderCenter();
        break;
    case 1:
        if (LoadBerryBlenderGfx())
        {
            for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
            {
                sBerryBlender->playerArrowSpriteIds[i] = CreateSprite(&sSpriteTemplate_PlayerArrow, sPlayerArrowPos[i][0], sPlayerArrowPos[i][1], 1);
                StartSpriteAnim(&gSprites[sBerryBlender->playerArrowSpriteIds[i]], i + 8);
            }
            if (gReceivedRemoteLinkPlayers && gWirelessCommType)
            {
                LoadWirelessStatusIndicatorSpriteGfx();
                CreateWirelessStatusIndicatorSprite(0, 0);
            }
            SetVBlankCallback(VBlankCB_BerryBlender);
            sBerryBlender->mainState++;
        }
        break;
    case 2:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        UpdateBlenderCenter();
        sBerryBlender->mainState++;
        break;
    case 3:
        DrawBlenderBg();
        if (!gPaletteFade.active)
            sBerryBlender->mainState++;
        break;
    case 4:
        if (PrintMessage(&sBerryBlender->textState, sText_BerryBlenderStart, GetPlayerTextSpeedDelay()))
            sBerryBlender->mainState++;
        break;
    case 5:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
        sBerryBlender->mainState++;
        break;
    case 6:
        if (!gPaletteFade.active)
        {
            // Go to bag menu to choose berry, set callback to StartBlender
            FreeAllWindowBuffers();
            UnsetBgTilemapBuffer(2);
            UnsetBgTilemapBuffer(1);
            SetVBlankCallback(NULL);
            ChooseBerryForMachine(StartBlender);

            sBerryBlender->mainState = 0;
        }
        break;
    }

    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
}

#define sTargetY       data[0]
#define sX             data[1]
#define sY             data[2]
#define sBounceSpeed   data[3]
#define sYUpSpeed      data[4]
#define sBounces       data[5]
#define sXSpeed        data[6]
#define sYDownSpeed    data[7]

// For throwing berries into the machine
static void SpriteCB_Berry(struct Sprite *sprite)
{
    sprite->sX += sprite->sXSpeed;
    sprite->sY -= sprite->sYUpSpeed;
    sprite->sY += sprite->sYDownSpeed;
    sprite->sTargetY += sprite->sYDownSpeed;
    sprite->sYUpSpeed--;

    if (sprite->sTargetY < sprite->sY)
    {
        sprite->sBounceSpeed = sprite->sYUpSpeed = sprite->sBounceSpeed - 1;

        if (++sprite->sBounces > 3)
            DestroySprite(sprite);
        else
            PlaySE(SE_BALL_TRAY_EXIT);
    }
    sprite->x = sprite->sX;
    sprite->y = sprite->sY;
}

static void SetBerrySpriteData(struct Sprite *sprite, s16 x, s16 y, s16 bounceSpeed, s16 xSpeed, s16 ySpeed)
{
    sprite->sTargetY = y;
    sprite->sX = x;
    sprite->sY = y;
    sprite->sBounceSpeed = bounceSpeed;
    sprite->sYUpSpeed = 10;
    sprite->sBounces = 0;
    sprite->sXSpeed = xSpeed;
    sprite->sYDownSpeed = ySpeed;
    sprite->callback = SpriteCB_Berry;
}

#undef sTargetY
#undef sX
#undef sY
#undef sBounceSpeed
#undef sYUpSpeed
#undef sBounces
#undef sXSpeed
#undef sYDownSpeed

static void CreateBerrySprite(u16 itemId, u8 playerId)
{
    u8 spriteId = CreateSpinningBerrySprite(ITEM_TO_BERRY(itemId) - 1, 0, 80, playerId & 1);
    SetBerrySpriteData(&gSprites[spriteId],
                        sBerrySpriteData[playerId][0],
                        sBerrySpriteData[playerId][1],
                        sBerrySpriteData[playerId][2],
                        sBerrySpriteData[playerId][3],
                        sBerrySpriteData[playerId][4]);
}

static void ConvertItemToBlenderBerry(struct BlenderBerry* berry, u16 itemId)
{
    const struct Berry *berryInfo = GetBerryInfo(ITEM_TO_BERRY(itemId));

    berry->itemId = itemId;
    StringCopy(berry->name, berryInfo->name);
    berry->flavors[FLAVOR_SPICY] = berryInfo->spicy;
    berry->flavors[FLAVOR_DRY] = berryInfo->dry;
    berry->flavors[FLAVOR_SWEET] = berryInfo->sweet;
    berry->flavors[FLAVOR_BITTER] = berryInfo->bitter;
    berry->flavors[FLAVOR_SOUR] = berryInfo->sour;
    berry->flavors[FLAVOR_COUNT] = berryInfo->smoothness;
}

static void InitLocalPlayers(u8 opponentsNum)
{
    switch (opponentsNum)
    {
    case 0: // Link games have 0 in-game opponents
        gInGameOpponentsNo = 0;
        break;
    case 1:
        gInGameOpponentsNo = 1;
        sBerryBlender->numPlayers = 2;
        StringCopy(gLinkPlayers[0].name, gSaveBlock2Ptr->playerName);

        if (!FlagGet(FLAG_HIDE_LILYCOVE_CONTEST_HALL_BLEND_MASTER))
            StringCopy(gLinkPlayers[1].name, sBlenderOpponentsNames[BLENDER_MASTER]);
        else
            StringCopy(gLinkPlayers[1].name, sBlenderOpponentsNames[BLENDER_MISTER]);

        gLinkPlayers[0].language = GAME_LANGUAGE;
        gLinkPlayers[1].language = GAME_LANGUAGE;
        break;
    case 2:
        gInGameOpponentsNo = 2;
        sBerryBlender->numPlayers = 3;
        StringCopy(gLinkPlayers[0].name, gSaveBlock2Ptr->playerName);
        StringCopy(gLinkPlayers[1].name, sBlenderOpponentsNames[BLENDER_DUDE]);
        StringCopy(gLinkPlayers[2].name, sBlenderOpponentsNames[BLENDER_LASSIE]);

        gLinkPlayers[0].language = GAME_LANGUAGE;
        gLinkPlayers[1].language = GAME_LANGUAGE;
        gLinkPlayers[2].language = GAME_LANGUAGE;
        break;
    case 3:
        gInGameOpponentsNo = 3;
        sBerryBlender->numPlayers = 4;
        StringCopy(gLinkPlayers[0].name, gSaveBlock2Ptr->playerName);
        StringCopy(gLinkPlayers[1].name, sBlenderOpponentsNames[BLENDER_MISS]);
        StringCopy(gLinkPlayers[2].name, sBlenderOpponentsNames[BLENDER_LADDIE]);
        StringCopy(gLinkPlayers[3].name, sBlenderOpponentsNames[BLENDER_LASSIE]);

        gLinkPlayers[0].language = GAME_LANGUAGE;
        gLinkPlayers[1].language = GAME_LANGUAGE;
        gLinkPlayers[2].language = GAME_LANGUAGE;
        gLinkPlayers[3].language = GAME_LANGUAGE;
        break;
    }
}

static void StartBlender(void)
{
    s32 i;

    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    if (sBerryBlender == NULL)
        sBerryBlender = AllocZeroed(sizeof(*sBerryBlender));

    sBerryBlender->mainState = 0;
    sBerryBlender->unk1 = 0;

    for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
        sBerryBlender->chosenItemId[i] = ITEM_NONE;

    InitLocalPlayers(gSpecialVar_0x8004);

    if (gSpecialVar_0x8004 == 0)
        SetMainCallback2(CB2_StartBlenderLink);
    else
        SetMainCallback2(CB2_StartBlenderLocal);
}

static void CB2_StartBlenderLink(void)
{
    s32 i, j;

    switch (sBerryBlender->mainState)
    {
    case 0:
        InitBlenderBgs();
        gLinkType = LINKTYPE_BERRY_BLENDER;
        sBerryBlender->slowdownTimer = 0;
        for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
        {
            sBerryBlender->playerContinueResponses[i] = 0;
            for (j = 0; j < NUM_SCORE_TYPES; j++)
            {
                sBerryBlender->scores[i][j] = 0;
            }
        }
        sBerryBlender->playAgainState = 0;
        sBerryBlender->maxRPM = 0;
        sBerryBlender->loadGfxState = 0;
        sBerryBlender->mainState++;
        break;
    case 1:
        if (LoadBerryBlenderGfx())
        {
            sBerryBlender->mainState++;
            UpdateBlenderCenter();
        }
        break;
    case 2:
        for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
        {
            sBerryBlender->playerArrowSpriteIds2[i] = CreateSprite(&sSpriteTemplate_PlayerArrow, sPlayerArrowPos[i][0], sPlayerArrowPos[i][1], 1);
            StartSpriteAnim(&gSprites[sBerryBlender->playerArrowSpriteIds2[i]], i + 8);
        }
        if (gReceivedRemoteLinkPlayers && gWirelessCommType)
        {
            LoadWirelessStatusIndicatorSpriteGfx();
            CreateWirelessStatusIndicatorSprite(0, 0);
        }
        sBerryBlender->mainState++;
        break;
    case 3:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        sBerryBlender->mainState++;
        break;
    case 4:
        DrawBlenderBg();
        if (!gPaletteFade.active)
        {
            sBerryBlender->mainState++;
        }
        break;
    case 5:
        PrintMessage(&sBerryBlender->textState, sText_CommunicationStandby, 0);
        sBerryBlender->mainState = 8;
        sBerryBlender->framesToWait = 0;
        break;
    case 8:
        // Send berry choice to link partners
        sBerryBlender->mainState++;
        sBerryBlender->playerToThrowBerry = 0;
        ConvertItemToBlenderBerry(&sBerryBlender->blendedBerries[0], gSpecialVar_ItemId);
        memcpy(gBlockSendBuffer, &sBerryBlender->blendedBerries[0], sizeof(struct BlenderBerry));
        SetLinkStandbyCallback();
        sBerryBlender->framesToWait = 0;
        break;
    case 9:
        if (IsLinkTaskFinished())
        {
            ResetBlockReceivedFlags();
            if (GetMultiplayerId() == 0)
                SendBlockRequest(BLOCK_REQ_SIZE_40);
            sBerryBlender->mainState++;
        }
        break;
    case 10:
        if (++sBerryBlender->framesToWait > 20)
        {
            // Wait for partners' berries
            ClearDialogWindowAndFrameToTransparent(WIN_MSG, TRUE);
            if (GetBlockReceivedStatus() == GetLinkPlayerCountAsBitFlags())
            {
                for (i = 0; i < GetLinkPlayerCount(); i++)
                {
                    memcpy(&sBerryBlender->blendedBerries[i], &gBlockRecvBuffer[i][0], sizeof(struct BlenderBerry));
                    sBerryBlender->chosenItemId[i] = sBerryBlender->blendedBerries[i].itemId;
                }

                ResetBlockReceivedFlags();
                sBerryBlender->mainState++;
            }
        }
        break;
    case 11:
        sBerryBlender->numPlayers = GetLinkPlayerCount();

        // Throw 1 player's berry in
        for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
        {
            if (sBerryBlender->playerToThrowBerry == sPlayerIdMap[sBerryBlender->numPlayers - 2][i])
            {
                CreateBerrySprite(sBerryBlender->chosenItemId[sBerryBlender->playerToThrowBerry], i);
                break;
            }
        }

        sBerryBlender->framesToWait = 0;
        sBerryBlender->mainState++;
        sBerryBlender->playerToThrowBerry++;
        break;
    case 12:
        if (++sBerryBlender->framesToWait > 60)
        {
            if (sBerryBlender->playerToThrowBerry >= sBerryBlender->numPlayers)
            {
                // Finished throwing berries in
                sBerryBlender->mainState++;
                sBerryBlender->arrowPos = sArrowStartPos[sArrowStartPosIds[sBerryBlender->numPlayers - 2]] - ARROW_FALL_ROTATION;
            }
            else
            {
                // Haven't finished throwing berries in, go back to prev step
                sBerryBlender->mainState--;
            }
            sBerryBlender->framesToWait = 0;
        }
        break;
    case 13:
        if (IsLinkTaskFinished())
        {
            sBerryBlender->mainState++;
            DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
            PlaySE(SE_FALL);
            ShowBg(2);
        }
        break;
    case 14:
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_BG2_ON);
        sBerryBlender->arrowPos += 0x200;
        sBerryBlender->centerScale += 4;
        if (sBerryBlender->centerScale > 255)
        {
            SetGpuRegBits(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(2));
            sBerryBlender->mainState++;
            sBerryBlender->centerScale = 256;
            sBerryBlender->arrowPos = sArrowStartPos[sArrowStartPosIds[sBerryBlender->numPlayers - 2]];
            sBerryBlender->framesToWait = 0;
            PlaySE(SE_TRUCK_DOOR);
            SetPlayerIdMaps();
            PrintPlayerNames();
        }
        DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
        break;
    case 15:
        if (UpdateBlenderLandScreenShake())
        {
            sBerryBlender->framesToWait = 0;
            sBerryBlender->mainState++;
        }
        DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
        break;
    case 16:
        CreateSprite(&sSpriteTemplate_CountdownNumbers, 120, -16, 3);
        sBerryBlender->mainState++;
        break;
    case 17:
        // Wait here for the countdown
        // State is progressed in SpriteCB_Start
        break;
    case 18:
        sBerryBlender->mainState++;
        break;
    case 19:
        SetLinkStandbyCallback();
        sBerryBlender->mainState++;
        break;
    case 20:
        if (IsLinkTaskFinished())
        {
            SetBerryBlenderLinkCallback();
            sBerryBlender->mainState++;
        }
        break;
    case 21:
        sBerryBlender->speed = MIN_ARROW_SPEED;
        sBerryBlender->gameFrameTime = 0;
        SetMainCallback2(CB2_PlayBlender);
        if (GetCurrentMapMusic() != MUS_CYCLING)
        {
            sBerryBlender->savedMusic = GetCurrentMapMusic();
        }
        PlayBGM(MUS_CYCLING);
        break;
    }

    Blender_DummiedOutFunc(sBerryBlender->bg_X, sBerryBlender->bg_Y);
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
}

static void InitBlenderBgs(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT, 0);

    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetTasks();

    SetVBlankCallback(VBlankCB_BerryBlender);

    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(1, sBgTemplates, ARRAY_COUNT(sBgTemplates));

    SetBgTilemapBuffer(1, sBerryBlender->tilemapBuffers[0]);
    SetBgTilemapBuffer(2, sBerryBlender->tilemapBuffers[1]);
    LoadUserWindowBorderGfx(0, 1, BG_PLTT_ID(13));
    LoadMessageBoxGfx(0, 0x14, BG_PLTT_ID(15));
    InitBerryBlenderWindows();

    sBerryBlender->unk0 = 0;
    sBerryBlender->speed = 0;
    sBerryBlender->arrowPos = 0;
    sBerryBlender->maxRPM = 0;
    sBerryBlender->bg_X = 0;
    sBerryBlender->bg_Y = 0;
}

static u8 GetArrowProximity(u16 arrowPos, u8 playerId)
{
    u32 pos = (arrowPos / 256) + 24;
    u8 arrowId = sBerryBlender->playerIdToArrowId[playerId];
    u32 hitRangeStart = sArrowHitRangeStart[arrowId];

    if (pos >= hitRangeStart && pos < hitRangeStart + 48)
    {
        if (pos >= hitRangeStart + 20 && pos < hitRangeStart + 28)
            return PROXIMITY_BEST;
        else
            return PROXIMITY_GOOD;
    }

    return PROXIMITY_MISS;
}

static void SetOpponentsBerryData(u16 playerBerryItemId, u8 playersNum, struct BlenderBerry* playerBerry)
{
    u16 opponentSetId = 0;
    u16 opponentBerryId;
    u16 berryMasterDiff;
    u16 i;

    if (playerBerryItemId == ITEM_ENIGMA_BERRY)
    {
        for (i = 0; i < FLAVOR_COUNT; i++)
        {
            if (playerBerry->flavors[opponentSetId] > playerBerry->flavors[i])
                opponentSetId = i;
        }
        opponentSetId += NUM_NPC_BERRIES;
    }
    else
    {
        opponentSetId = ITEM_TO_BERRY(playerBerryItemId) - 1;
        if (opponentSetId >= NUM_NPC_BERRIES)
            opponentSetId = (opponentSetId % NUM_NPC_BERRIES) + NUM_NPC_BERRIES;
    }
    for (i = 0; i < playersNum - 1; i++)
    {
        opponentBerryId = sOpponentBerrySets[opponentSetId][i];
        berryMasterDiff = ITEM_TO_BERRY(playerBerryItemId) - ITEM_TO_BERRY(ITEM_SPELON_BERRY);
        if (!FlagGet(FLAG_HIDE_LILYCOVE_CONTEST_HALL_BLEND_MASTER) && gSpecialVar_0x8004 == 1)
        {
            opponentSetId %= ARRAY_COUNT(sBerryMasterBerries);
            opponentBerryId = sBerryMasterBerries[opponentSetId];

            // If the player's berry is any of the Berry Master's berries,
            // then use the next lower set of berries
            if (berryMasterDiff < ARRAY_COUNT(sBerryMasterBerries))
                opponentBerryId -= ARRAY_COUNT(sBerryMasterBerries);
        }
        SetPlayerBerryData(i + 1, opponentBerryId + FIRST_BERRY_INDEX);
    }
}

static void SetPlayerIdMaps(void)
{
    s32 i, j;

    for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
    {
        sBerryBlender->playerIdToArrowId[i] = NO_PLAYER;
        sBerryBlender->arrowIdToPlayerId[i] = sPlayerIdMap[sBerryBlender->numPlayers - 2][i];
    }
    for (j = 0; j < BLENDER_MAX_PLAYERS; j++)
    {
        for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
        {
            if (sBerryBlender->arrowIdToPlayerId[i] == j)
                sBerryBlender->playerIdToArrowId[j] = i;
        }
    }
}

static void PrintPlayerNames(void)
{
    s32 i, xPos;
    u32 playerId = 0;
    u8 text[20];

    if (gReceivedRemoteLinkPlayers)
        playerId = GetMultiplayerId();

    for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
    {
        if (sBerryBlender->arrowIdToPlayerId[i] != NO_PLAYER)
        {
            sBerryBlender->playerArrowSpriteIds[sBerryBlender->arrowIdToPlayerId[i]] = sBerryBlender->playerArrowSpriteIds2[i];
            StartSpriteAnim(&gSprites[sBerryBlender->playerArrowSpriteIds[sBerryBlender->arrowIdToPlayerId[i]]], i);

            text[0] = EOS;
            StringCopy(text, gLinkPlayers[sBerryBlender->arrowIdToPlayerId[i]].name);
            xPos = GetStringCenterAlignXOffset(FONT_NORMAL, text, 0x38);

            if (playerId == sBerryBlender->arrowIdToPlayerId[i])
                Blender_AddTextPrinter(i, text, xPos, 1, 0, 2); // Highlight player's name in red
            else
                Blender_AddTextPrinter(i, text, xPos, 1, 0, 1);

            PutWindowTilemap(i);
            CopyWindowToVram(i, COPYWIN_FULL);
        }
    }
}

static void CB2_StartBlenderLocal(void)
{
    s32 i, j;

    switch (sBerryBlender->mainState)
    {
    case 0:
        SetWirelessCommType0();
        InitBlenderBgs();
        SetPlayerBerryData(0, gSpecialVar_ItemId);
        ConvertItemToBlenderBerry(&sBerryBlender->blendedBerries[0], gSpecialVar_ItemId);
        SetOpponentsBerryData(gSpecialVar_ItemId, sBerryBlender->numPlayers, &sBerryBlender->blendedBerries[0]);

        for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
        {
            sBerryBlender->playerContinueResponses[i] = 0;
            for (j = 0; j < NUM_SCORE_TYPES; j++)
            {
                sBerryBlender->scores[i][j] = 0;
            }
        }

        sBerryBlender->playAgainState = 0;
        sBerryBlender->loadGfxState = 0;
        gLinkType = LINKTYPE_BERRY_BLENDER;
        sBerryBlender->mainState++;
        break;
    case 1:
        if (LoadBerryBlenderGfx())
        {
            sBerryBlender->mainState++;
            UpdateBlenderCenter();
        }
        break;
    case 2:
        for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
        {
            sBerryBlender->playerArrowSpriteIds2[i] = CreateSprite(&sSpriteTemplate_PlayerArrow, sPlayerArrowPos[i][0], sPlayerArrowPos[i][1], 1);
            StartSpriteAnim(&gSprites[sBerryBlender->playerArrowSpriteIds2[i]], i + 8);
        }
        sBerryBlender->mainState++;
        break;
    case 3:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        sBerryBlender->mainState++;
        sBerryBlender->framesToWait = 0;
        break;
    case 4:
        if (++sBerryBlender->framesToWait == 2)
            DrawBlenderBg();
        if (!gPaletteFade.active)
            sBerryBlender->mainState = 8;
        break;
    case 8:
        sBerryBlender->mainState = 11;
        sBerryBlender->playerToThrowBerry = 0;
        break;
    case 11:
        for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
        {
            // Throw 1 player's berry in
            u32 playerId = sPlayerIdMap[sBerryBlender->numPlayers - 2][i];
            if (sBerryBlender->playerToThrowBerry == playerId)
            {
                CreateBerrySprite(sBerryBlender->chosenItemId[sBerryBlender->playerToThrowBerry], i);
                break;
            }
        }
        sBerryBlender->framesToWait = 0;
        sBerryBlender->mainState++;
        sBerryBlender->playerToThrowBerry++;
        break;
    case 12:
        if (++sBerryBlender->framesToWait > 60)
        {
            if (sBerryBlender->playerToThrowBerry >= sBerryBlender->numPlayers)
            {
                // Finished throwing berries in
                sBerryBlender->arrowPos = sArrowStartPos[sArrowStartPosIds[sBerryBlender->numPlayers - 2]] - ARROW_FALL_ROTATION;
                sBerryBlender->mainState++;
            }
            else
            {
                // Haven't finished throwing berries in, go back to prev step
                sBerryBlender->mainState--;
            }
            sBerryBlender->framesToWait = 0;
        }
        break;
    case 13:
        sBerryBlender->mainState++;
        SetPlayerIdMaps();
        PlaySE(SE_FALL);
        DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
        ShowBg(2);
        break;
    case 14:
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_BG2_ON);
        sBerryBlender->arrowPos += 0x200;
        sBerryBlender->centerScale += 4;
        if (sBerryBlender->centerScale > 255)
        {
            sBerryBlender->mainState++;
            sBerryBlender->centerScale = 256;
            sBerryBlender->arrowPos = sArrowStartPos[sArrowStartPosIds[sBerryBlender->numPlayers - 2]];
            SetGpuRegBits(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(2));
            sBerryBlender->framesToWait = 0;
            PlaySE(SE_TRUCK_DOOR);
            PrintPlayerNames();
        }
        DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
        break;
    case 15:
        if (UpdateBlenderLandScreenShake())
        {
            sBerryBlender->mainState++;
        }
        DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
        break;
    case 16:
        CreateSprite(&sSpriteTemplate_CountdownNumbers, 120, -16, 3);
        sBerryBlender->mainState++;
        break;
    case 17:
        // Wait here for the countdown
        // State is progressed in SpriteCB_Start
        break;
    case 18:
        sBerryBlender->mainState++;
        break;
    case 19:
        sBerryBlender->mainState++;
        break;
    case 20:
        sBerryBlender->mainState++;
        break;
    case 21:
        ResetLinkCmds();
        sBerryBlender->speed = MIN_ARROW_SPEED;
        sBerryBlender->gameFrameTime = 0;
        sBerryBlender->perfectOpponents = FALSE;
        sBerryBlender->slowdownTimer = 0;
        SetMainCallback2(CB2_PlayBlender);

        if (gSpecialVar_0x8004 == 1)
        {
            if (!FlagGet(FLAG_HIDE_LILYCOVE_CONTEST_HALL_BLEND_MASTER))
                sBerryBlender->opponentTaskIds[0] = CreateTask(Task_HandleBerryMaster, 10);
            else
                sBerryBlender->opponentTaskIds[0] = CreateTask(sLocalOpponentTasks[0], 10);
        }

        if (gSpecialVar_0x8004 > 1)
        {
            for (i = 0; i < gSpecialVar_0x8004; i++)
                sBerryBlender->opponentTaskIds[i] = CreateTask(sLocalOpponentTasks[i], 10 + i);
        }

        if (GetCurrentMapMusic() != MUS_CYCLING)
            sBerryBlender->savedMusic = GetCurrentMapMusic();

        PlayBGM(MUS_CYCLING);
        PlaySE(SE_BERRY_BLENDER);
        UpdateHitPitch();
        break;
    }

    Blender_DummiedOutFunc(sBerryBlender->bg_X, sBerryBlender->bg_Y);
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
}

static void ResetLinkCmds(void)
{
    s32 i;
    for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
    {
        gSendCmd[BLENDER_COMM_INPUT_STATE] = 0;
        gSendCmd[BLENDER_COMM_SCORE] = 0;
        gRecvCmds[i][BLENDER_COMM_INPUT_STATE] = 0;
        gRecvCmds[i][BLENDER_COMM_SCORE] = 0;
    }
}

#define tTimer    data[0]
#define tDelay    data[1]
#define tPlayerId data[2]

static void Task_OpponentMiss(u8 taskId)
{
   if(++gTasks[taskId].tTimer > gTasks[taskId].tDelay)
   {
        gRecvCmds[gTasks[taskId].tPlayerId][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_MISS;
        DestroyTask(taskId);
   }
}

static void CreateOpponentMissTask(u8 playerId, u8 delay)
{
    u8 taskId = CreateTask(Task_OpponentMiss, 80);
    gTasks[taskId].tDelay = delay;
    gTasks[taskId].tPlayerId = playerId;
}

#undef tTimer
#undef tDelay
#undef tPlayerId

#define tDidInput data[0]

static void Task_HandleOpponent1(u8 taskId)
{
    if (GetArrowProximity(sBerryBlender->arrowPos, 1) == PROXIMITY_BEST)
    {
        if (!gTasks[taskId].tDidInput)
        {
            if (!sBerryBlender->perfectOpponents)
            {
                u8 rand = Random() / 655;
                if (sBerryBlender->speed < 500)
                {
                    if (rand > 75)
                        gRecvCmds[1][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_BEST;
                    else
                        gRecvCmds[1][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_GOOD;

                    // BUG: Overrwrote above assignment. Opponent 1 can't get Best at low speed
                    #ifndef BUGFIX
                    gRecvCmds[1][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_GOOD;
                    #endif
                }
                else if (sBerryBlender->speed < 1500)
                {
                    if (rand > 80)
                    {
                        gRecvCmds[1][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_BEST;
                    }
                    else
                    {
                        u8 value = rand - 21;
                        if (value < 60)
                            gRecvCmds[1][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_GOOD;
                        else if (rand < 10)
                            CreateOpponentMissTask(1, 5);
                    }
                }
                else if (rand <= 90)
                {
                    u8 value = rand - 71;
                    if (value < 20)
                        gRecvCmds[1][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_GOOD;
                    else if (rand < 30)
                        CreateOpponentMissTask(1, 5);
                }
                else
                {
                    gRecvCmds[1][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_BEST;
                }
            }
            else
            {
                gRecvCmds[1][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_BEST;
            }

            gTasks[taskId].tDidInput = TRUE;
        }
    }
    else
    {
        gTasks[taskId].tDidInput = FALSE;
    }
}

static void Task_HandleOpponent2(u8 taskId)
{
    u32 var1 = (sBerryBlender->arrowPos + 0x1800) & 0xFFFF;
    u8 arrowId = sBerryBlender->playerIdToArrowId[2];
    if ((var1 >> 8) > sArrowHitRangeStart[arrowId] + 20 && (var1 >> 8) < sArrowHitRangeStart[arrowId] + 40)
    {
        if (!gTasks[taskId].tDidInput)
        {
            if (!sBerryBlender->perfectOpponents)
            {
                u8 rand = Random() / 655;
                if (sBerryBlender->speed < 500)
                {
                    if (rand > 66)
                        gRecvCmds[2][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_BEST;
                    else
                        gRecvCmds[2][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_GOOD;
                }
                else
                {
                    if (rand > 65)
                        gRecvCmds[2][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_BEST;
                    if (rand > 40 && rand <= 65)
                        gRecvCmds[2][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_GOOD;
                    if (rand < 10)
                        CreateOpponentMissTask(2, 5);
                }

                gTasks[taskId].tDidInput = TRUE;
            }
            else
            {
                gRecvCmds[2][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_BEST;
                gTasks[taskId].tDidInput = TRUE;
            }
        }
    }
    else
    {
        gTasks[taskId].tDidInput = FALSE;
    }
}

static void Task_HandleOpponent3(u8 taskId)
{
    u32 var1 = (sBerryBlender->arrowPos + 0x1800) & 0xFFFF;
    u8 arrowId = sBerryBlender->playerIdToArrowId[3];
    if ((var1 >> 8) > sArrowHitRangeStart[arrowId] + 20 && (var1 >> 8) < sArrowHitRangeStart[arrowId] + 40)
    {
        if (gTasks[taskId].data[0] == 0)
        {
            if (!sBerryBlender->perfectOpponents)
            {
                u8 rand = (Random() / 655);
                if (sBerryBlender->speed < 500)
                {
                    if (rand > 88)
                        gRecvCmds[3][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_BEST;
                    else
                        gRecvCmds[3][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_GOOD;
                }
                else
                {
                    if (rand > 60)
                        gRecvCmds[3][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_BEST;
                    else if (rand > 55 && rand <= 60)
                        gRecvCmds[3][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_GOOD;
                    if (rand < 5)
                        CreateOpponentMissTask(3, 5);
                }
                gTasks[taskId].tDidInput = TRUE;
            }
            else
            {
                gRecvCmds[3][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_BEST;
                gTasks[taskId].tDidInput = TRUE;
            }
        }
    }
    else
    {
        gTasks[taskId].tDidInput = FALSE;
    }
}

static void Task_HandleBerryMaster(u8 taskId)
{
    if (GetArrowProximity(sBerryBlender->arrowPos, 1) == PROXIMITY_BEST)
    {
        if (!gTasks[taskId].tDidInput)
        {
            gRecvCmds[1][BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_BEST;
            gTasks[taskId].tDidInput = TRUE;
        }
    }
    else
    {
        gTasks[taskId].tDidInput = FALSE;
    }
}

#undef tDidInput

static void CreateScoreSymbolSprite(u16 cmd, u8 arrowId)
{
    u8 spriteId;

    spriteId = CreateSprite(&sSpriteTemplate_ScoreSymbols,
                            sPlayerArrowPos[arrowId][0] - (10 * sPlayerArrowQuadrant[arrowId][0]),
                            sPlayerArrowPos[arrowId][1] - (10 * sPlayerArrowQuadrant[arrowId][1]),
                            1);
    if (cmd == LINKCMD_BLENDER_SCORE_BEST)
    {
        StartSpriteAnim(&gSprites[spriteId], SCOREANIM_BEST_FLASH);
        gSprites[spriteId].callback = SpriteCB_ScoreSymbolBest;
        PlaySE(SE_ICE_STAIRS);
    }
    else if (cmd == LINKCMD_BLENDER_SCORE_GOOD)
    {
        StartSpriteAnim(&gSprites[spriteId], SCOREANIM_GOOD);
        PlaySE(SE_SUCCESS);
    }
    else if (cmd == LINKCMD_BLENDER_SCORE_MISS)
    {
        StartSpriteAnim(&gSprites[spriteId], SCOREANIM_MISS);
        PlaySE(SE_FAILURE);
    }
    CreateParticleSprites();
}

static void UpdateSpeedFromHit(u16 cmd)
{
    UpdateHitPitch();
    switch (cmd)
    {
    case LINKCMD_BLENDER_SCORE_BEST:
        if (sBerryBlender->speed < 1500)
        {
            sBerryBlender->speed += (384 / sNumPlayersToSpeedDivisor[sBerryBlender->numPlayers]);
        }
        else
        {
            sBerryBlender->speed += (128 / sNumPlayersToSpeedDivisor[sBerryBlender->numPlayers]);
            ShakeBgCoordForHit(&sBerryBlender->bg_X, (sBerryBlender->speed / 100) - 10);
            ShakeBgCoordForHit(&sBerryBlender->bg_Y, (sBerryBlender->speed / 100) - 10);
        }
        break;
    case LINKCMD_BLENDER_SCORE_GOOD:
        if (sBerryBlender->speed < 1500)
            sBerryBlender->speed += (256 / sNumPlayersToSpeedDivisor[sBerryBlender->numPlayers]);
        break;
    case LINKCMD_BLENDER_SCORE_MISS:
        sBerryBlender->speed -= (256 / sNumPlayersToSpeedDivisor[sBerryBlender->numPlayers]);
        if (sBerryBlender->speed < MIN_ARROW_SPEED)
            sBerryBlender->speed = MIN_ARROW_SPEED;
        break;
    }
}

// Return TRUE if the received command matches the corresponding Link or RFU command
static bool32 CheckRecvCmdMatches(u16 recvCmd, u16 linkCmd, u16 rfuCmd)
{
    if (gReceivedRemoteLinkPlayers && gWirelessCommType)
    {
        if ((recvCmd & RFUCMD_MASK) == rfuCmd)
            return TRUE;
    }
    else
    {
        if (recvCmd == linkCmd)
            return TRUE;
    }

    return FALSE;
}

static void UpdateOpponentScores(void)
{
    s32 i;

    if (gSpecialVar_0x8004 != 0)
    {
        // Local game, "send" players score to itself
        if (gSendCmd[BLENDER_COMM_SCORE] != 0)
        {
            gRecvCmds[0][BLENDER_COMM_SCORE] = gSendCmd[BLENDER_COMM_SCORE];
            gRecvCmds[0][BLENDER_COMM_INPUT_STATE] = LINKCMD_BLENDER_SEND_KEYS;
            gSendCmd[BLENDER_COMM_SCORE] = 0;
        }

        // Local game, simulate NPCs sending keys
        // Their actual inputs are handled by Task_HandleOpponent
        for (i = 1; i < BLENDER_MAX_PLAYERS; i++)
        {
            if (gRecvCmds[i][BLENDER_COMM_SCORE] != 0)
                gRecvCmds[i][BLENDER_COMM_INPUT_STATE] = LINKCMD_BLENDER_SEND_KEYS;
        }
    }
    for (i = 0; i < sBerryBlender->numPlayers; i++)
    {
        if (CheckRecvCmdMatches(gRecvCmds[i][BLENDER_COMM_INPUT_STATE], LINKCMD_BLENDER_SEND_KEYS, RFUCMD_BLENDER_SEND_KEYS))
        {
            u32 arrowId = sBerryBlender->playerIdToArrowId[i];
            if (gRecvCmds[i][BLENDER_COMM_SCORE] == LINKCMD_BLENDER_SCORE_BEST)
            {
                UpdateSpeedFromHit(LINKCMD_BLENDER_SCORE_BEST);
                sBerryBlender->progressBarValue += (sBerryBlender->speed / 55);
                if (sBerryBlender->progressBarValue >= MAX_PROGRESS_BAR)
                    sBerryBlender->progressBarValue = MAX_PROGRESS_BAR;
                CreateScoreSymbolSprite(LINKCMD_BLENDER_SCORE_BEST, arrowId);
                sBerryBlender->scores[i][SCORE_BEST]++;
            }
            else if (gRecvCmds[i][BLENDER_COMM_SCORE] == LINKCMD_BLENDER_SCORE_GOOD)
            {
                UpdateSpeedFromHit(LINKCMD_BLENDER_SCORE_GOOD);
                sBerryBlender->progressBarValue += (sBerryBlender->speed / 70);
                CreateScoreSymbolSprite(LINKCMD_BLENDER_SCORE_GOOD, arrowId);
                sBerryBlender->scores[i][SCORE_GOOD]++;
            }
            else if (gRecvCmds[i][BLENDER_COMM_SCORE] == LINKCMD_BLENDER_SCORE_MISS)
            {
                CreateScoreSymbolSprite(LINKCMD_BLENDER_SCORE_MISS, arrowId);
                UpdateSpeedFromHit(LINKCMD_BLENDER_SCORE_MISS);
                if (sBerryBlender->scores[i][SCORE_MISS] < 999)
                    sBerryBlender->scores[i][SCORE_MISS]++;
            }

            // BUG: Should be [i][BLENDER_COMM_SCORE] below, not [BLENDER_COMM_SCORE][i]
            // As a result the music tempo updates if any player misses, but only if 1 specific player hits
            #ifdef BUGFIX
            if (gRecvCmds[i][BLENDER_COMM_SCORE] == LINKCMD_BLENDER_SCORE_MISS
             || gRecvCmds[i][BLENDER_COMM_SCORE] == LINKCMD_BLENDER_SCORE_BEST
             || gRecvCmds[i][BLENDER_COMM_SCORE] == LINKCMD_BLENDER_SCORE_GOOD)
            #else
            if (gRecvCmds[i][BLENDER_COMM_SCORE] == LINKCMD_BLENDER_SCORE_MISS
             || gRecvCmds[BLENDER_COMM_SCORE][i] == LINKCMD_BLENDER_SCORE_BEST
             || gRecvCmds[BLENDER_COMM_SCORE][i] == LINKCMD_BLENDER_SCORE_GOOD)
            #endif
            {
                if (sBerryBlender->speed > 1500)
                    m4aMPlayTempoControl(&gMPlayInfo_BGM, ((sBerryBlender->speed - 750) / 20) + 256);
                else
                    m4aMPlayTempoControl(&gMPlayInfo_BGM, 256);
            }
        }
    }
    if (gSpecialVar_0x8004 != 0)
    {
        for (i = 0; i < sBerryBlender->numPlayers; i++)
        {
            gRecvCmds[i][BLENDER_COMM_INPUT_STATE] = 0;
            gRecvCmds[i][BLENDER_COMM_SCORE] = 0;
        }
    }
}

static void HandlePlayerInput(void)
{
    u8 arrowId;
    bool8 pressedA = FALSE;
    u8 playerId = 0;

    if (gReceivedRemoteLinkPlayers)
        playerId = GetMultiplayerId();

    arrowId = sBerryBlender->playerIdToArrowId[playerId];

    if (sBerryBlender->gameEndState == 0)
    {
        if (gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A && JOY_NEW(A_BUTTON))
        {
            if (JOY_HELD_RAW(A_BUTTON | L_BUTTON) != (A_BUTTON | L_BUTTON))
                pressedA = TRUE;
        }
        else if (JOY_NEW(A_BUTTON))
        {
            pressedA = TRUE;
        }

        if (pressedA)
        {
            u8 proximity;
            StartSpriteAnim(&gSprites[sBerryBlender->playerArrowSpriteIds[sBerryBlender->arrowIdToPlayerId[arrowId]]], arrowId + 4);
            proximity = GetArrowProximity(sBerryBlender->arrowPos, playerId);

            if (proximity == PROXIMITY_BEST)
                gSendCmd[BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_BEST;
            else if (proximity == PROXIMITY_GOOD)
                gSendCmd[BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_GOOD;
            else
                gSendCmd[BLENDER_COMM_SCORE] = LINKCMD_BLENDER_SCORE_MISS;
        }
    }
    if (++sBerryBlender->slowdownTimer > 5)
    {
        if (sBerryBlender->speed > MIN_ARROW_SPEED)
            sBerryBlender->speed--;
        sBerryBlender->slowdownTimer = 0;
    }

    if (gEnableContestDebugging && JOY_NEW(L_BUTTON))
        sBerryBlender->perfectOpponents ^= 1;
}

static void CB2_PlayBlender(void)
{
    UpdateBlenderCenter();

    if (sBerryBlender->gameFrameTime < (99 * 60 * 60) + (59 * 60)) // game time can't be longer than 99 minutes and 59 seconds, can't print 3 digits
        sBerryBlender->gameFrameTime++;

    HandlePlayerInput();
    SetLinkDebugValues((u16)(sBerryBlender->speed), sBerryBlender->progressBarValue);
    UpdateOpponentScores();
    TryUpdateProgressBar(sBerryBlender->progressBarValue, MAX_PROGRESS_BAR);
    UpdateRPM(sBerryBlender->speed);
    RestoreBgCoords();
    ProcessLinkPlayerCmds();
    if (sBerryBlender->gameEndState == 0 && sBerryBlender->maxProgressBarValue >= MAX_PROGRESS_BAR)
    {
        sBerryBlender->progressBarValue = MAX_PROGRESS_BAR;
        sBerryBlender->gameEndState = 1;
        SetMainCallback2(CB2_EndBlenderGame);
    }

    Blender_DummiedOutFunc(sBerryBlender->bg_X, sBerryBlender->bg_Y);
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
}

static void Blender_DummiedOutFunc(s16 bgX, s16 bgY)
{

}

static bool8 AreBlenderBerriesSame(struct BlenderBerry* berries, u8 a, u8 b)
{
    // First check to itemId is pointless (and wrong anyway?), always false when this is called
    // Only used to determine if two enigma berries are equivalent
    if (berries[a].itemId != berries[b].itemId
     || (StringCompare(berries[a].name, berries[b].name) == 0
      && (berries[a].flavors[FLAVOR_SPICY] == berries[b].flavors[FLAVOR_SPICY]
       && berries[a].flavors[FLAVOR_DRY] == berries[b].flavors[FLAVOR_DRY]
       && berries[a].flavors[FLAVOR_SWEET] == berries[b].flavors[FLAVOR_SWEET]
       && berries[a].flavors[FLAVOR_BITTER] == berries[b].flavors[FLAVOR_BITTER]
       && berries[a].flavors[FLAVOR_SOUR] == berries[b].flavors[FLAVOR_SOUR]
       && berries[a].flavors[FLAVOR_COUNT] == berries[b].flavors[FLAVOR_COUNT])))
        return TRUE;
    else
        return FALSE;
}

static u32 CalculatePokeblockColor(struct BlenderBerry* berries, s16 *_flavors, u8 numPlayers, u8 negativeFlavors)
{
    s16 flavors[FLAVOR_COUNT + 1];
    s32 i, j;
    u8 numFlavors;

    for (i = 0; i < FLAVOR_COUNT + 1; i++)
        flavors[i] = _flavors[i];

    j = 0;
    for (i = 0; i < FLAVOR_COUNT; i++)
    {
        if (flavors[i] == 0)
            j++;
    }

    // If all 5 flavors are 0, or if 4-5 flavors were negative,
    // or if players used the same berry, color is black
    if (j == FLAVOR_COUNT || negativeFlavors > 3)
        return PBLOCK_CLR_BLACK;

    for (i = 0; i < numPlayers; i++)
    {
        for (j = 0; j < numPlayers; j++)
        {
            if (berries[i].itemId == berries[j].itemId && i != j
                && (berries[i].itemId != ITEM_ENIGMA_BERRY || AreBlenderBerriesSame(berries, i, j)))
                    return PBLOCK_CLR_BLACK;
        }
    }

    numFlavors = 0;
    for (numFlavors = 0, i = 0; i < FLAVOR_COUNT; i++)
    {
        if (flavors[i] > 0)
            numFlavors++;
    }

    // Check for special colors (White/Gray/Gold)
    if (numFlavors > 3)
        return PBLOCK_CLR_WHITE;

    if (numFlavors == 3)
        return PBLOCK_CLR_GRAY;

    for (i = 0; i < FLAVOR_COUNT; i++)
    {
        if (flavors[i] > 50)
            return PBLOCK_CLR_GOLD;
    }

    // Only 1 flavor present, return corresponding color
    if (numFlavors == 1 && flavors[FLAVOR_SPICY] > 0)
        return PBLOCK_CLR_RED;
    if (numFlavors == 1 && flavors[FLAVOR_DRY] > 0)
        return PBLOCK_CLR_BLUE;
    if (numFlavors == 1 && flavors[FLAVOR_SWEET] > 0)
        return PBLOCK_CLR_PINK;
    if (numFlavors == 1 && flavors[FLAVOR_BITTER] > 0)
        return PBLOCK_CLR_GREEN;
    if (numFlavors == 1 && flavors[FLAVOR_SOUR] > 0)
        return PBLOCK_CLR_YELLOW;

    if (numFlavors == 2)
    {
        // Determine which 2 flavors are present
        s32 idx = 0;
        for (i = 0; i < FLAVOR_COUNT; i++)
        {
            if (flavors[i] > 0)
                sPokeblockPresentFlavors[idx++] = i;
        }
        // Use the stronger flavor to determine color
        // The weaker flavor is returned in the upper 16 bits, but this is ignored in the color assignment
        if (flavors[sPokeblockPresentFlavors[0]] >= flavors[sPokeblockPresentFlavors[1]])
        {
            if (sPokeblockPresentFlavors[0] == FLAVOR_SPICY)
                return (sPokeblockPresentFlavors[1] << 16) | PBLOCK_CLR_PURPLE;
            if (sPokeblockPresentFlavors[0] == FLAVOR_DRY)
                return (sPokeblockPresentFlavors[1] << 16) | PBLOCK_CLR_INDIGO;
            if (sPokeblockPresentFlavors[0] == FLAVOR_SWEET)
                return (sPokeblockPresentFlavors[1] << 16) | PBLOCK_CLR_BROWN;
            if (sPokeblockPresentFlavors[0] == FLAVOR_BITTER)
                return (sPokeblockPresentFlavors[1] << 16) | PBLOCK_CLR_LITE_BLUE;
            if (sPokeblockPresentFlavors[0] == FLAVOR_SOUR)
                return (sPokeblockPresentFlavors[1] << 16) | PBLOCK_CLR_OLIVE;
        }
        else
        {
            if (sPokeblockPresentFlavors[1] == FLAVOR_SPICY)
                return (sPokeblockPresentFlavors[0] << 16) | PBLOCK_CLR_PURPLE;
            if (sPokeblockPresentFlavors[1] == FLAVOR_DRY)
                return (sPokeblockPresentFlavors[0] << 16) | PBLOCK_CLR_INDIGO;
            if (sPokeblockPresentFlavors[1] == FLAVOR_SWEET)
                return (sPokeblockPresentFlavors[0] << 16) | PBLOCK_CLR_BROWN;
            if (sPokeblockPresentFlavors[1] == FLAVOR_BITTER)
                return (sPokeblockPresentFlavors[0] << 16) | PBLOCK_CLR_LITE_BLUE;
            if (sPokeblockPresentFlavors[1] == FLAVOR_SOUR)
                return (sPokeblockPresentFlavors[0] << 16) | PBLOCK_CLR_OLIVE;
        }
    }
    return PBLOCK_CLR_NONE;
}

static void Debug_SetMaxRPMStage(s16 value)
{
    sDebug_MaxRPMStage = value;
}

static s16 UNUSED Debug_GetMaxRPMStage(void)
{
    return sDebug_MaxRPMStage;
}

static void Debug_SetGameTimeStage(s16 value)
{
    sDebug_GameTimeStage = value;
}

static s16 UNUSED Debug_GetGameTimeStage(void)
{
    return sDebug_GameTimeStage;
}

static void CalculatePokeblock(struct BlenderBerry *berries, struct Pokeblock *pokeblock, u8 numPlayers, u8 *flavors, u16 maxRPM)
{
    s32 i, j;
    s32 multiuseVar;
    u8 numNegatives;

    for (i = 0; i < FLAVOR_COUNT + 1; i++)
        sPokeblockFlavors[i] = 0;

    // Add up the flavor + feel of each players berry
    for (i = 0; i < numPlayers; i++)
    {
        for (j = 0; j < FLAVOR_COUNT + 1; j++)
            sPokeblockFlavors[j] += berries[i].flavors[j];
    }

    // Subtract each flavor total from the prev one
    // The idea is to focus on only the flavors with the highest totals
    // Bad way to do it though (order matters here)
    multiuseVar = sPokeblockFlavors[0];
    sPokeblockFlavors[FLAVOR_SPICY]  -= sPokeblockFlavors[FLAVOR_DRY];
    sPokeblockFlavors[FLAVOR_DRY]    -= sPokeblockFlavors[FLAVOR_SWEET];
    sPokeblockFlavors[FLAVOR_SWEET]  -= sPokeblockFlavors[FLAVOR_BITTER];
    sPokeblockFlavors[FLAVOR_BITTER] -= sPokeblockFlavors[FLAVOR_SOUR];
    sPokeblockFlavors[FLAVOR_SOUR]   -= multiuseVar;

    // Count (and reset) the resulting negative flavors
    multiuseVar = 0;
    for (i = 0; i < FLAVOR_COUNT; i++)
    {
        if (sPokeblockFlavors[i] < 0)
        {
            sPokeblockFlavors[i] = 0;
            multiuseVar++;
        }
    }
    numNegatives = multiuseVar;

    // Subtract the number of negative flavor totals from each positive total (without going below 0)
    for (i = 0; i < FLAVOR_COUNT; i++)
    {
        if (sPokeblockFlavors[i] > 0)
        {
            if (sPokeblockFlavors[i] < multiuseVar)
                sPokeblockFlavors[i] = 0;
            else
                sPokeblockFlavors[i] -= multiuseVar;
        }
    }

    for (i = 0; i < FLAVOR_COUNT; i++)
        sDebug_PokeblockFactorFlavors[i] = sPokeblockFlavors[i];

    // Factor in max RPM and round
    sDebug_PokeblockFactorRPM = multiuseVar = maxRPM / 333 + 100;
    for (i = 0; i < FLAVOR_COUNT; i++)
    {
        s32 remainder;
        s32 flavor = sPokeblockFlavors[i];
        flavor = (flavor * multiuseVar) / 10;
        remainder = flavor % 10;
        flavor /= 10;
        if (remainder > 4)
            flavor++;
        sPokeblockFlavors[i] = flavor;
    }

    for (i = 0; i < FLAVOR_COUNT; i++)
        sDebug_PokeblockFactorFlavorsAfterRPM[i] = sPokeblockFlavors[i];

    // Calculate color and feel of pokeblock
    pokeblock->color = CalculatePokeblockColor(berries, &sPokeblockFlavors[0], numPlayers, numNegatives);
    sPokeblockFlavors[FLAVOR_COUNT] = (sPokeblockFlavors[FLAVOR_COUNT] / numPlayers) - numPlayers;

    if (sPokeblockFlavors[FLAVOR_COUNT] < 0)
        sPokeblockFlavors[FLAVOR_COUNT] = 0;

    if (pokeblock->color == PBLOCK_CLR_BLACK)
    {
        // Black pokeblocks get their flavors randomly reassigned
        multiuseVar = Random() % ARRAY_COUNT(sBlackPokeblockFlavorFlags);
        for (i = 0; i < FLAVOR_COUNT; i++)
        {
            if ((sBlackPokeblockFlavorFlags[multiuseVar] >> i) & 1)
                sPokeblockFlavors[i] = 2;
            else
                sPokeblockFlavors[i] = 0;
        }
    }

    for (i = 0; i < FLAVOR_COUNT + 1; i++)
    {
        if (sPokeblockFlavors[i] > 255)
            sPokeblockFlavors[i] = 255;
    }

    pokeblock->spicy  = sPokeblockFlavors[FLAVOR_SPICY];
    pokeblock->dry    = sPokeblockFlavors[FLAVOR_DRY];
    pokeblock->sweet  = sPokeblockFlavors[FLAVOR_SWEET];
    pokeblock->bitter = sPokeblockFlavors[FLAVOR_BITTER];
    pokeblock->sour   = sPokeblockFlavors[FLAVOR_SOUR];
    pokeblock->feel   = sPokeblockFlavors[FLAVOR_COUNT];

    for (i = 0; i < FLAVOR_COUNT + 1; i++)
        flavors[i] = sPokeblockFlavors[i];
}

static void UNUSED Debug_CalculatePokeblock(struct BlenderBerry* berries, struct Pokeblock* pokeblock, u8 numPlayers, u8 *flavors, u16 maxRPM)
{
    CalculatePokeblock(berries, pokeblock, numPlayers, flavors, maxRPM);
}

static void Debug_SetStageVars(void)
{
    u32 frames = (u16)(sBerryBlender->gameFrameTime);
    u16 maxRPM = sBerryBlender->maxRPM;
    s16 stage = 0;

    if (frames < 900)
        stage = 5;
    else if ((u16)(frames - 900) < 600)
        stage = 4;
    else if ((u16)(frames - 1500) < 600)
        stage = 3;
    else if ((u16)(frames - 2100) < 900)
        stage = 2;
    else if ((u16)(frames - 3300) < 300)
        stage = 1;

    Debug_SetGameTimeStage(stage);

    stage = 0;
    if (maxRPM <= 64)
    {
        if (maxRPM >= 50 && maxRPM < 100)
            stage = -1;
        else if (maxRPM >= 100 && maxRPM < 150)
            stage = -2;
        else if (maxRPM >= 150 && maxRPM < 200)
            stage = -3;
        else if (maxRPM >= 200 && maxRPM < 250)
            stage = -4;
        else if (maxRPM >= 250 && maxRPM < 300)
            stage = -5;
        else if (maxRPM >= 350 && maxRPM < 400)
            stage = -6;
        else if (maxRPM >= 400 && maxRPM < 450)
            stage = -7;
        else if (maxRPM >= 500 && maxRPM < 550)
            stage = -8;
        else if (maxRPM >= 550 && maxRPM < 600)
            stage = -9;
        else if (maxRPM >= 600)
            stage = -10;
    }

    Debug_SetMaxRPMStage(stage);
}

static void SendContinuePromptResponse(u16 *cmd)
{
    if (gReceivedRemoteLinkPlayers && gWirelessCommType)
        *cmd = RFUCMD_SEND_PACKET;
    else
        *cmd = LINKCMD_SEND_PACKET;
}

static void CB2_EndBlenderGame(void)
{
    u8 i, j;

    if (sBerryBlender->gameEndState < 3)
        UpdateBlenderCenter();

    GetMultiplayerId(); // unused return value

    switch (sBerryBlender->gameEndState)
    {
    case 1:
        m4aMPlayTempoControl(&gMPlayInfo_BGM, 256);
        for (i = 0; i < gSpecialVar_0x8004; i++)
        {
            DestroyTask(sBerryBlender->opponentTaskIds[i]);
        }
        sBerryBlender->gameEndState++;
        break;
    case 2:
        sBerryBlender->speed -= 32;
        if (sBerryBlender->speed <= 0)
        {
            ClearLinkCallback();
            sBerryBlender->speed = 0;

            if (gReceivedRemoteLinkPlayers)
                sBerryBlender->gameEndState++;
            else
                sBerryBlender->gameEndState = 5;

            sBerryBlender->mainState = 0;
            m4aMPlayStop(&gMPlayInfo_SE2);
        }
        UpdateHitPitch();
        break;
    case 3:
        if (GetMultiplayerId() != 0)
        {
            sBerryBlender->gameEndState++;
        }
        else if (IsLinkTaskFinished())
        {
            if (gReceivedRemoteLinkPlayers && gWirelessCommType)
            {
                sBerryBlender->gameBlock.timeRPM.time = sBerryBlender->gameFrameTime;
                sBerryBlender->gameBlock.timeRPM.maxRPM = sBerryBlender->maxRPM;

                for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
                {
                    for (j = 0; j < NUM_SCORE_TYPES; j++)
                        sBerryBlender->gameBlock.scores[i][j] = sBerryBlender->scores[i][j];
                }

                if (SendBlock(0, &sBerryBlender->gameBlock, sizeof(sBerryBlender->gameBlock)))
                    sBerryBlender->gameEndState++;
            }
            else
            {
                sBerryBlender->smallBlock.time = sBerryBlender->gameFrameTime;
                sBerryBlender->smallBlock.maxRPM = sBerryBlender->maxRPM;
                if (SendBlock(0, &sBerryBlender->smallBlock, sizeof(sBerryBlender->smallBlock) + 32))
                    sBerryBlender->gameEndState++;
            }
        }
        break;
    case 4:
        if (GetBlockReceivedStatus())
        {
            ResetBlockReceivedFlags();
            sBerryBlender->gameEndState++;

            if (gReceivedRemoteLinkPlayers && gWirelessCommType)
            {
                struct BlenderGameBlock *receivedBlock = (struct BlenderGameBlock *)(&gBlockRecvBuffer);

                sBerryBlender->maxRPM = receivedBlock->timeRPM.maxRPM;
                sBerryBlender->gameFrameTime = receivedBlock->timeRPM.time;

                for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
                {
                    for (j = 0; j < NUM_SCORE_TYPES; j++)
                        sBerryBlender->scores[i][j] = receivedBlock->scores[i][j];
                }
            }
            else
            {
                struct TimeAndRPM *receivedBlock = (struct TimeAndRPM *)(&gBlockRecvBuffer);

                sBerryBlender->maxRPM = receivedBlock->maxRPM;
                sBerryBlender->gameFrameTime = receivedBlock->time;
            }
        }
        break;
    case 5:
        if (PrintBlendingRanking())
            sBerryBlender->gameEndState++;
        break;
    case 6:
        if (PrintBlendingResults())
        {
            if (gInGameOpponentsNo == 0)
                IncrementGameStat(GAME_STAT_POKEBLOCKS_WITH_FRIENDS);
            else
                IncrementGameStat(GAME_STAT_POKEBLOCKS);

            sBerryBlender->gameEndState++;
        }
        break;
    case 7:
        if (PrintMessage(&sBerryBlender->textState, sText_WouldLikeToBlendAnotherBerry, GetPlayerTextSpeedDelay()))
            sBerryBlender->gameEndState++;
        break;
    case 9:
        sBerryBlender->yesNoAnswer = 0;
        CreateYesNoMenu(&sYesNoWindowTemplate_ContinuePlaying, 1, 0xD, 0);
        sBerryBlender->gameEndState++;
        break;
    case 10:
        switch (Menu_ProcessInputNoWrapClearOnChoose())
        {
        case 1:
        case MENU_B_PRESSED:
            sBerryBlender->yesNoAnswer = 1;
            sBerryBlender->gameEndState++;
            for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
            {
                if (sBerryBlender->arrowIdToPlayerId[i] != NO_PLAYER)
                {
                    PutWindowTilemap(i);
                    CopyWindowToVram(i, COPYWIN_FULL);
                }
            }
            break;
        case 0:
            sBerryBlender->yesNoAnswer = 0;
            sBerryBlender->gameEndState++;
            for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
            {
                if (sBerryBlender->arrowIdToPlayerId[i] != NO_PLAYER)
                {
                    PutWindowTilemap(i);
                    CopyWindowToVram(i, COPYWIN_FULL);
                }
            }
            break;
        }
        break;
    case 11:
        SendContinuePromptResponse(&gSendCmd[BLENDER_COMM_INPUT_STATE]);
        if (sBerryBlender->yesNoAnswer == 0)
        {
            if (IsBagPocketNonEmpty(POCKET_BERRIES) == FALSE)
            {
                // No berries
                sBerryBlender->playAgainState = CANT_PLAY_NO_BERRIES;
                gSendCmd[BLENDER_COMM_RESP] = LINKCMD_BLENDER_NO_BERRIES;
            }
            else if (GetFirstFreePokeblockSlot() == -1)
            {
                // No space for pokeblocks
                sBerryBlender->playAgainState = CANT_PLAY_NO_PKBLCK_SPACE;
                gSendCmd[BLENDER_COMM_RESP] = LINKCMD_BLENDER_NO_PBLOCK_SPACE;
            }
            else
            {
                sBerryBlender->playAgainState = PLAY_AGAIN_YES;
                gSendCmd[BLENDER_COMM_RESP] = LINKCMD_BLENDER_PLAY_AGAIN;
            }
            sBerryBlender->gameEndState++;
        }
        else
        {
            sBerryBlender->playAgainState = PLAY_AGAIN_NO;
            gSendCmd[BLENDER_COMM_RESP] = LINKCMD_CONT_BLOCK;
            sBerryBlender->gameEndState++;
        }
        break;
    case 12:
        if (gInGameOpponentsNo)
        {
            SetMainCallback2(CB2_CheckPlayAgainLocal);
            sBerryBlender->gameEndState = 0;
            sBerryBlender->mainState = 0;
        }
        else
        {
            sBerryBlender->gameEndState++;
        }
        break;
    case 8:
        sBerryBlender->gameEndState++;
        break;
    case 13:
        if (PrintMessage(&sBerryBlender->textState, sText_CommunicationStandby, GetPlayerTextSpeedDelay()))
        {
            SetMainCallback2(CB2_CheckPlayAgainLink);
            sBerryBlender->gameEndState = 0;
            sBerryBlender->mainState = 0;
        }
        break;
    }

    RestoreBgCoords();
    UpdateRPM(sBerryBlender->speed);
    ProcessLinkPlayerCmds();
    Blender_DummiedOutFunc(sBerryBlender->bg_X, sBerryBlender->bg_Y);
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
}

static bool8 LinkPlayAgainHandleSaving(void)
{
    switch (sBerryBlender->linkPlayAgainState)
    {
    case 0:
        SetLinkStandbyCallback();
        sBerryBlender->linkPlayAgainState = 1;
        sBerryBlender->framesToWait = 0;
        break;
    case 1:
        if (IsLinkTaskFinished())
        {
            sBerryBlender->linkPlayAgainState++;
            gSoftResetDisabled = TRUE;
        }
        break;
    case 2:
        WriteSaveBlock2();
        sBerryBlender->linkPlayAgainState++;
        sBerryBlender->framesToWait = 0;
        break;
    case 3:
        if (++sBerryBlender->framesToWait == 10)
        {
            SetLinkStandbyCallback();
            sBerryBlender->linkPlayAgainState++;
        }
        break;
    case 4:
        if (IsLinkTaskFinished())
        {
            if (WriteSaveBlock1Sector())
            {
                sBerryBlender->linkPlayAgainState = 5;
            }
            else
            {
                sBerryBlender->framesToWait = 0;
                sBerryBlender->linkPlayAgainState = 3;
            }
        }
        break;
    case 5:
        sBerryBlender->linkPlayAgainState++;
        sBerryBlender->framesToWait = 0;
        break;
    case 6:
        if (++sBerryBlender->framesToWait > 5)
        {
            gSoftResetDisabled = FALSE;
            return TRUE;
        }
        break;
    }

    return FALSE;
}

static void CB2_CheckPlayAgainLink(void)
{
    switch (sBerryBlender->gameEndState)
    {
    case 0:
        if (sBerryBlender->playerContinueResponses[0] == LINKCMD_SEND_LINK_TYPE)
        {
            // Link leader says game will continue
            sBerryBlender->gameEndState = 5;
        }
        else if (sBerryBlender->playerContinueResponses[0] == LINKCMD_BLENDER_STOP)
        {
            // Link leader says game will stop, if necessary print why
            if (sBerryBlender->canceledPlayerCmd == LINKCMD_BLENDER_NO_BERRIES)
                sBerryBlender->gameEndState = 2;
            else if (sBerryBlender->canceledPlayerCmd == LINKCMD_BLENDER_NO_PBLOCK_SPACE)
                sBerryBlender->gameEndState = 1;
            else
                sBerryBlender->gameEndState = 5;
        }
        break;
    case 1:
        sBerryBlender->gameEndState = 3;
        StringCopy(gStringVarBuffer, gLinkPlayers[sBerryBlender->canceledPlayerId].name);
        StringAppend(gStringVarBuffer, sText_ApostropheSPokeblockCaseIsFull);
        break;
    case 2:
        sBerryBlender->gameEndState++;
        StringCopy(gStringVarBuffer, gLinkPlayers[sBerryBlender->canceledPlayerId].name);
        StringAppend(gStringVarBuffer, sText_HasNoBerriesToPut);
        break;
    case 3:
        if (PrintMessage(&sBerryBlender->textState, gStringVarBuffer, GetPlayerTextSpeedDelay()))
        {
            sBerryBlender->framesToWait = 0;
            sBerryBlender->gameEndState++;
        }
        break;
    case 4:
        if (++sBerryBlender->framesToWait > 60)
            sBerryBlender->gameEndState = 5;
        break;
    case 5:
        PrintMessage(&sBerryBlender->textState, gText_SavingDontTurnOff2, 0);
        SetLinkStandbyCallback();
        sBerryBlender->gameEndState++;
        break;
    case 6:
        if (IsLinkTaskFinished())
        {
            sBerryBlender->framesToWait = 0;
            sBerryBlender->gameEndState++;
            sBerryBlender->linkPlayAgainState = 0;
        }
        break;
    case 7:
        if (LinkPlayAgainHandleSaving())
        {
            PlaySE(SE_SAVE);
            sBerryBlender->gameEndState++;
        }
        break;
    case 8:
        sBerryBlender->gameEndState++;
        SetLinkStandbyCallback();
        break;
    case 9:
        if (IsLinkTaskFinished())
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            sBerryBlender->gameEndState++;
        }
        break;
    case 10:
        if (!gPaletteFade.active)
        {
            if (sBerryBlender->playerContinueResponses[0] == LINKCMD_SEND_LINK_TYPE)
            {
                FreeAllWindowBuffers();
                UnsetBgTilemapBuffer(2);
                UnsetBgTilemapBuffer(1);
                FREE_AND_SET_NULL(sBerryBlender);
                SetMainCallback2(DoBerryBlending);
            }
            else
            {
                sBerryBlender->framesToWait = 0;
                sBerryBlender->gameEndState++;
            }
        }
        break;
    case 11:
        if (++sBerryBlender->framesToWait > 30)
        {
            SetCloseLinkCallback();
            sBerryBlender->gameEndState++;
        }
        break;
    case 12:
        if (!gReceivedRemoteLinkPlayers)
        {
            FREE_AND_SET_NULL(sBerryBlender);
            SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        }
        break;
    }

    ProcessLinkPlayerCmds();
    Blender_DummiedOutFunc(sBerryBlender->bg_X, sBerryBlender->bg_Y);
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
}

static void CB2_CheckPlayAgainLocal(void)
{
    switch (sBerryBlender->gameEndState)
    {
    case 0:
        if (sBerryBlender->playAgainState == PLAY_AGAIN_YES || sBerryBlender->playAgainState == PLAY_AGAIN_NO)
            sBerryBlender->gameEndState = 9;
        if (sBerryBlender->playAgainState == CANT_PLAY_NO_BERRIES)
            sBerryBlender->gameEndState = 2;
        if (sBerryBlender->playAgainState == CANT_PLAY_NO_PKBLCK_SPACE)
            sBerryBlender->gameEndState = 1;
        break;
    case 1:
        sBerryBlender->gameEndState = 3;
        sBerryBlender->textState = 0;
        StringCopy(gStringVarBuffer, sText_YourPokeblockCaseIsFull);
        break;
    case 2:
        sBerryBlender->gameEndState++;
        sBerryBlender->textState = 0;
        StringCopy(gStringVarBuffer, sText_RunOutOfBerriesForBlending);
        break;
    case 3:
        if (PrintMessage(&sBerryBlender->textState, gStringVarBuffer, GetPlayerTextSpeedDelay()))
            sBerryBlender->gameEndState = 9;
        break;
    case 9:
        BeginFastPaletteFade(3);
        sBerryBlender->gameEndState++;
        break;
    case 10:
        if (!gPaletteFade.active)
        {
            if (sBerryBlender->playAgainState == PLAY_AGAIN_YES)
                SetMainCallback2(DoBerryBlending);
            else
                SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);

            FreeAllWindowBuffers();
            UnsetBgTilemapBuffer(2);
            UnsetBgTilemapBuffer(1);
            FREE_AND_SET_NULL(sBerryBlender);
        }
        break;
    }

    ProcessLinkPlayerCmds();
    Blender_DummiedOutFunc(sBerryBlender->bg_X, sBerryBlender->bg_Y);
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
}

static void ProcessLinkPlayerCmds(void)
{
    if (gReceivedRemoteLinkPlayers)
    {
        if (CheckRecvCmdMatches(gRecvCmds[0][BLENDER_COMM_INPUT_STATE], LINKCMD_SEND_PACKET, RFUCMD_SEND_PACKET))
        {
            if (gRecvCmds[0][BLENDER_COMM_RESP] == LINKCMD_BLENDER_STOP)
            {
                // Link leader has indicated play is stopping, read signal to determine why
                switch (gRecvCmds[0][BLENDER_COMM_STOP_TYPE])
                {
                case LINKCMD_CONT_BLOCK: // Someone selected "No" to continue playing
                    sBerryBlender->canceledPlayerCmd = LINKCMD_CONT_BLOCK;
                    sBerryBlender->canceledPlayerId = gRecvCmds[0][BLENDER_COMM_PLAYER_ID];
                    break;
                case LINKCMD_BLENDER_NO_BERRIES:
                    sBerryBlender->canceledPlayerCmd = LINKCMD_BLENDER_NO_BERRIES;
                    sBerryBlender->canceledPlayerId = gRecvCmds[0][BLENDER_COMM_PLAYER_ID];
                    break;
                case LINKCMD_BLENDER_NO_PBLOCK_SPACE:
                    sBerryBlender->canceledPlayerCmd = LINKCMD_BLENDER_NO_PBLOCK_SPACE;
                    sBerryBlender->canceledPlayerId = gRecvCmds[0][BLENDER_COMM_PLAYER_ID];
                    break;
                }

                sBerryBlender->playerContinueResponses[0] = LINKCMD_BLENDER_STOP;
            }
            else if (gRecvCmds[0][BLENDER_COMM_RESP] == LINKCMD_SEND_LINK_TYPE)
            {
                // Link leader has indicated play will continue
                sBerryBlender->playerContinueResponses[0] = LINKCMD_SEND_LINK_TYPE;
            }
        }

        // If player is link leader, check for responses to the "Continue playing" prompt (even if it's not up yet)
        if (GetMultiplayerId() == 0
            && sBerryBlender->playerContinueResponses[0] != LINKCMD_BLENDER_STOP
            && sBerryBlender->playerContinueResponses[0] != LINKCMD_SEND_LINK_TYPE)
        {
            u8 i;

            // Try to gather responses
            for (i = 0; i < GetLinkPlayerCount(); i++)
            {
                if (CheckRecvCmdMatches(gRecvCmds[i][BLENDER_COMM_INPUT_STATE], LINKCMD_SEND_PACKET, RFUCMD_SEND_PACKET))
                {
                    switch (gRecvCmds[i][BLENDER_COMM_RESP])
                    {
                    case LINKCMD_CONT_BLOCK: // Selected "No"
                        sBerryBlender->playerContinueResponses[i] = LINKCMD_CONT_BLOCK;
                        break;
                    case LINKCMD_BLENDER_PLAY_AGAIN: // Selected "Yes"
                        sBerryBlender->playerContinueResponses[i] = LINKCMD_BLENDER_PLAY_AGAIN;
                        break;
                    case LINKCMD_BLENDER_NO_BERRIES:
                        sBerryBlender->playerContinueResponses[i] = LINKCMD_BLENDER_NO_BERRIES;
                        break;
                    case LINKCMD_BLENDER_NO_PBLOCK_SPACE:
                        sBerryBlender->playerContinueResponses[i] = LINKCMD_BLENDER_NO_PBLOCK_SPACE;
                        break;
                    }
                }
            }

            // Count players that have responded, stopping at first non-response
            for (i = 0; i < GetLinkPlayerCount(); i++)
            {
                if (sBerryBlender->playerContinueResponses[i] == 0)
                    break;
            }

            // If all players responded, handle response
            if (i == GetLinkPlayerCount())
            {
                // Count players that decided to play again, stopping at first negative response
                for (i = 0; i < GetLinkPlayerCount(); i++)
                {
                    if (sBerryBlender->playerContinueResponses[i] != LINKCMD_BLENDER_PLAY_AGAIN)
                        break;
                }

                // Schedule signal to other players about whether or not play will continue
                SendContinuePromptResponse(&gSendCmd[BLENDER_COMM_INPUT_STATE]);
                if (i == GetLinkPlayerCount())
                {
                    // All players chose to continue playing
                    gSendCmd[BLENDER_COMM_RESP] = LINKCMD_SEND_LINK_TYPE;
                }
                else
                {
                    // At least 1 player decided to stop playing, or can't continue playing
                    gSendCmd[BLENDER_COMM_RESP] = LINKCMD_BLENDER_STOP;
                    gSendCmd[BLENDER_COMM_STOP_TYPE] = sBerryBlender->playerContinueResponses[i];
                    gSendCmd[BLENDER_COMM_PLAYER_ID] = i;
                }
            }
        }
    }
}

static void DrawBlenderCenter(struct BgAffineSrcData *dest)
{
    struct BgAffineSrcData affineSrc;

    affineSrc.texX = (DISPLAY_WIDTH / 2) << 8;
    affineSrc.texY = (DISPLAY_HEIGHT / 2) << 8;
    affineSrc.scrX = DISPLAY_WIDTH / 2 - sBerryBlender->bg_X;
    affineSrc.scrY = DISPLAY_HEIGHT / 2 - sBerryBlender->bg_Y;
    affineSrc.sx = sBerryBlender->centerScale;
    affineSrc.sy = sBerryBlender->centerScale;
    affineSrc.alpha = sBerryBlender->arrowPos;

    *dest = affineSrc;
}

u16 GetBlenderArrowPosition(void)
{
    return sBerryBlender->arrowPos;
}

static void UpdateBlenderCenter(void)
{
    u8 playerId = 0;

    if (gReceivedRemoteLinkPlayers)
        playerId = GetMultiplayerId();

    if (gWirelessCommType && gReceivedRemoteLinkPlayers)
    {
        if (playerId == 0)
        {
            sBerryBlender->arrowPos += sBerryBlender->speed;
            gSendCmd[BLENDER_COMM_PROGRESS_BAR] = sBerryBlender->progressBarValue;
            gSendCmd[BLENDER_COMM_ARROW_POS] = sBerryBlender->arrowPos;
            DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
        }
        else
        {
            if ((gRecvCmds[0][BLENDER_COMM_INPUT_STATE] & RFUCMD_MASK) == RFUCMD_BLENDER_SEND_KEYS)
            {
                sBerryBlender->progressBarValue = gRecvCmds[0][BLENDER_COMM_PROGRESS_BAR];
                sBerryBlender->arrowPos = gRecvCmds[0][BLENDER_COMM_ARROW_POS];
                DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
            }
        }
    }
    else
    {
        sBerryBlender->arrowPos += sBerryBlender->speed;
        DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
    }
}

static void SetBgPos(void)
{
    SetGpuReg(REG_OFFSET_BG1HOFS, sBerryBlender->bg_X);
    SetGpuReg(REG_OFFSET_BG1VOFS, sBerryBlender->bg_Y);

    SetGpuReg(REG_OFFSET_BG0HOFS, sBerryBlender->bg_X);
    SetGpuReg(REG_OFFSET_BG0VOFS, sBerryBlender->bg_Y);
}

static void SpriteCB_Particle(struct Sprite *sprite)
{
    sprite->data[2] += sprite->data[0];
    sprite->data[3] += sprite->data[1];
    sprite->x2 = sprite->data[2] / 8;
    sprite->y2 = sprite->data[3] / 8;

    if (sprite->animEnded)
        DestroySprite(sprite);
}

static void CreateParticleSprites(void)
{
    s32 limit = (Random() % 2) + 1;
    s32 i;

    for (i = 0; i < limit; i++)
    {
        u16 rand;
        s32 x, y;
        u8 spriteId;

        rand = sBerryBlender->arrowPos + (Random() % 20);

        x = gSineTable[(rand & 0xFF) + 64] / 4;
        y = gSineTable[(rand & 0xFF)] / 4;

        spriteId = CreateSprite(&sSpriteTemplate_Particles, x + 120, y + 80, 1);
        gSprites[spriteId].data[0] = 16 - (Random() % 32);
        gSprites[spriteId].data[1] = 16 - (Random() % 32);

        gSprites[spriteId].callback = SpriteCB_Particle;
    }
}

static void SpriteCB_ScoreSymbol(struct Sprite *sprite)
{
    sprite->data[0]++;
    sprite->y2 = -(sprite->data[0] / 3);

    if (sprite->animEnded)
        DestroySprite(sprite);
}

static void SpriteCB_ScoreSymbolBest(struct Sprite *sprite)
{
    sprite->data[0]++;
    sprite->y2 = -(sprite->data[0] * 2);

    if (sprite->y2 < -12)
        sprite->y2 = -12;
    if (sprite->animEnded)
        DestroySprite(sprite);
}

static void SetPlayerBerryData(u8 playerId, u16 itemId)
{
    sBerryBlender->chosenItemId[playerId] = itemId;
    ConvertItemToBlenderBerry(&sBerryBlender->blendedBerries[playerId], itemId);
}

#define sState  data[0]
#define sYPos   data[1]
#define sDelay  data[2]
#define sAnimId data[3]

static void SpriteCB_CountdownNumber(struct Sprite *sprite)
{
    switch (sprite->sState)
    {
    case 0:
        sprite->sYPos += 8;
        if (sprite->sYPos > DISPLAY_HEIGHT / 2 + 8)
        {
            sprite->sYPos = DISPLAY_HEIGHT / 2 + 8;
            sprite->sState++;
            PlaySE(SE_BALL_BOUNCE_1);
        }
        break;
    case 1:
        if (++sprite->sDelay > 20)
        {
            sprite->sState++;
            sprite->sDelay = 0;
        }
        break;
    case 2:
        sprite->sYPos += 4;
        if (sprite->sYPos > DISPLAY_HEIGHT + 16)
        {
            if (++sprite->sAnimId == 3)
            {
                DestroySprite(sprite);
                CreateSprite(&sSpriteTemplate_Start, 120, -20, 2);
            }
            else
            {
                sprite->sState = 0;
                sprite->sYPos = -16;
                StartSpriteAnim(sprite, sprite->sAnimId);
            }
        }
        break;
    }

    sprite->y2 = sprite->sYPos;
}

#undef sState
#undef sYPos
#undef sDelay
#undef sAnimId

static void SpriteCB_Start(struct Sprite *sprite)
{
    switch (sprite->data[0])
    {
    case 0:
        sprite->data[1] += 8;
        if (sprite->data[1] > 92)
        {
            sprite->data[1] = 92;
            sprite->data[0]++;
            PlaySE(SE_PIN);
        }
        break;
    case 1:
        sprite->data[2] += 1;
        if (sprite->data[2] > 20)
            sprite->data[0]++;
        break;
    case 2:
        sprite->data[1] += 4;
        if (sprite->data[1] > DISPLAY_HEIGHT + 16)
        {
            sBerryBlender->mainState++;
            DestroySprite(sprite);
        }
        break;
    }

    sprite->y2 = sprite->data[1];
}

static void TryUpdateProgressBar(u16 current, u16 limit)
{
    // Progress bar doesn't move unless it's going up
    if (sBerryBlender->maxProgressBarValue < current)
    {
        sBerryBlender->maxProgressBarValue += 2;
        UpdateProgressBar(sBerryBlender->maxProgressBarValue, limit);
    }
}

static void UpdateProgressBar(u16 value, u16 limit)
{
    s32 amountFilled, maxFilledSegment, subSegmentsFilled, i;
    u16 *vram;

    vram = (u16 *)(BG_SCREEN_ADDR(12));
    amountFilled = (value * 64) / limit;
    maxFilledSegment = amountFilled / 8;

    // Set filled progress bar tiles in full segments
    for (i = 0; i < maxFilledSegment; i++)
    {
        vram[11 + i] = PROGRESS_BAR_FILLED_TOP;
        vram[43 + i] = PROGRESS_BAR_FILLED_BOTTOM;
    }

    // If progress bar between segments, fill with the corresponding partial segment tiles
    subSegmentsFilled = amountFilled % 8;
    if (subSegmentsFilled != 0)
    {
        vram[11 + i] = subSegmentsFilled + PROGRESS_BAR_EMPTY_TOP;
        vram[43 + i] = subSegmentsFilled + PROGRESS_BAR_EMPTY_BOTTOM;
        i++;
    }

    // Fill the remaining full segments with empty progress tiles
    // Essentially unnecessary, given that it starts empty and progress only goes up
    for (; i < 8; i++)
    {
        vram[11 + i] = PROGRESS_BAR_EMPTY_TOP;
        vram[43 + i] = PROGRESS_BAR_EMPTY_BOTTOM;
    }
}

static u32 ArrowSpeedToRPM(u16 speed)
{
    return 60 * 60 * 100 * speed / MAX_ARROW_POS;
}

static void UpdateRPM(u16 speed)
{
    u8 i;
    u8 digits[5];

    // Check if new max RPM has been reached
    u32 currentRPM = ArrowSpeedToRPM(speed);
    if (sBerryBlender->maxRPM < currentRPM)
        sBerryBlender->maxRPM = currentRPM;

    // Draw the current RPM number at the bottom of the screen
    for (i = 0; i < 5; i++)
    {
        digits[i] = currentRPM % 10;
        currentRPM /= 10;
    }
    *((u16 *)(BG_SCREEN_ADDR(12) + 0x458)) = digits[4] + RPM_DIGIT;
    *((u16 *)(BG_SCREEN_ADDR(12) + 0x45A)) = digits[3] + RPM_DIGIT;
    *((u16 *)(BG_SCREEN_ADDR(12) + 0x45C)) = digits[2] + RPM_DIGIT;
    *((u16 *)(BG_SCREEN_ADDR(12) + 0x460)) = digits[1] + RPM_DIGIT;
    *((u16 *)(BG_SCREEN_ADDR(12) + 0x462)) = digits[0] + RPM_DIGIT;
}

// Passed a pointer to the bg x/y
// Used when hitting a Best at high RPM
static void ShakeBgCoordForHit(s16 *coord, u16 speed)
{
    if (*coord == 0)
        *coord = (Random() % speed) - (speed / 2);
}

static void RestoreBgCoord(s16 *coord)
{
    if (*coord < 0)
        (*coord)++;
    if (*coord > 0)
        (*coord)--;
}

// For "unshaking" the screen after ShakeBgCoordForHit is called
static void RestoreBgCoords(void)
{
    RestoreBgCoord(&sBerryBlender->bg_X);
    RestoreBgCoord(&sBerryBlender->bg_Y);
}

static void BlenderLandShakeBgCoord(s16 *coord, u16 timer)
{
    s32 strength;

    if (timer < 10)
        strength = 16;
    else
        strength = 8;

    if (*coord == 0)
    {
        *coord = (Random() % strength) - (strength / 2);
    }
    else
    {
        if (*coord < 0)
            (*coord)++;
        if (*coord > 0)
            (*coord)--;
    }
}

// For shaking the screen when the blender lands after falling in at the start
static bool8 UpdateBlenderLandScreenShake(void)
{
    if (sBerryBlender->framesToWait == 0)
    {
        sBerryBlender->bg_X = 0;
        sBerryBlender->bg_Y = 0;
    }

    sBerryBlender->framesToWait++;
    BlenderLandShakeBgCoord(&sBerryBlender->bg_X, sBerryBlender->framesToWait);
    BlenderLandShakeBgCoord(&sBerryBlender->bg_Y, sBerryBlender->framesToWait);

    if (sBerryBlender->framesToWait == 20)
    {
        sBerryBlender->bg_X = 0;
        sBerryBlender->bg_Y = 0;
        return TRUE;
    }

    return FALSE;
}

static void SpriteCB_PlayerArrow(struct Sprite *sprite)
{
   sprite->x2 = -(sBerryBlender->bg_X);
   sprite->y2 = -(sBerryBlender->bg_Y);
}

static void TryUpdateBerryBlenderRecord(void)
{
    if (gSaveBlock1Ptr->berryBlenderRecords[sBerryBlender->numPlayers - 2] < sBerryBlender->maxRPM)
        gSaveBlock1Ptr->berryBlenderRecords[sBerryBlender->numPlayers - 2] = sBerryBlender->maxRPM;
}

static bool8 PrintBlendingResults(void)
{
    u16 i;
    s32 xPos, yPos;

    struct Pokeblock pokeblock;
    u8 flavors[FLAVOR_COUNT + 1];
    u8 text[40];
    u16 UNUSED berryIds[4];

    switch (sBerryBlender->mainState)
    {
    case 0:
        sBerryBlender->mainState++;
        sBerryBlender->framesToWait = 17;
        break;
    case 1:
        sBerryBlender->framesToWait -= 10;
        if (sBerryBlender->framesToWait < 0)
        {
            sBerryBlender->framesToWait = 0;
            sBerryBlender->mainState++;
        }
        break;
    case 2:
        if (++sBerryBlender->framesToWait > 20)
        {
            for (i = 0; i < NUM_SCORE_TYPES; i++)
                DestroySprite(&gSprites[sBerryBlender->scoreIconIds[i]]);

            sBerryBlender->framesToWait = 0;
            sBerryBlender->mainState++;
        }
        break;
    case 3:
        {
            u16 minutes, seconds;
            u8 *txtPtr;

            xPos = GetStringCenterAlignXOffset(FONT_NORMAL, sText_BlendingResults, 0xA8);
            Blender_AddTextPrinter(WIN_RESULTS, sText_BlendingResults, xPos, 1, TEXT_SKIP_DRAW, 0);

            if (sBerryBlender->numPlayers == BLENDER_MAX_PLAYERS)
                yPos = 17;
            else
                yPos = 21;

            for (i = 0; i < sBerryBlender->numPlayers; yPos += 16, i++)
            {
                u8 place = sBerryBlender->playerPlaces[i];

                ConvertIntToDecimalStringN(sBerryBlender->stringVar, i + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
                StringAppend(sBerryBlender->stringVar, sText_Dot);
                StringAppend(sBerryBlender->stringVar, gText_Space);
                StringAppend(sBerryBlender->stringVar, gLinkPlayers[place].name);
                Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, 8, yPos, TEXT_SKIP_DRAW, 3);

                StringCopy(sBerryBlender->stringVar, sBerryBlender->blendedBerries[place].name);
                ConvertInternationalString(sBerryBlender->stringVar, gLinkPlayers[place].language);
                StringAppend(sBerryBlender->stringVar, sText_SpaceBerry);
                Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, 0x54, yPos, TEXT_SKIP_DRAW, 3);
            }

            Blender_AddTextPrinter(WIN_RESULTS, sText_MaximumSpeed, 0, 0x51, TEXT_SKIP_DRAW, 3);
            ConvertIntToDecimalStringN(sBerryBlender->stringVar, sBerryBlender->maxRPM / 100, STR_CONV_MODE_RIGHT_ALIGN, 3);
            StringAppend(sBerryBlender->stringVar, sText_Dot);

            ConvertIntToDecimalStringN(text, sBerryBlender->maxRPM % 100, STR_CONV_MODE_LEADING_ZEROS, 2);
            StringAppend(sBerryBlender->stringVar, text);
            StringAppend(sBerryBlender->stringVar, sText_RPM);

            xPos = GetStringRightAlignXOffset(FONT_NORMAL, sBerryBlender->stringVar, 0xA8);
            Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, xPos, 0x51, TEXT_SKIP_DRAW, 3);
            Blender_AddTextPrinter(WIN_RESULTS, sText_Time, 0, 0x61, TEXT_SKIP_DRAW, 3);

            seconds = (sBerryBlender->gameFrameTime / 60) % 60;
            minutes = (sBerryBlender->gameFrameTime / (60 * 60));

            ConvertIntToDecimalStringN(sBerryBlender->stringVar, minutes, STR_CONV_MODE_LEADING_ZEROS, 2);
            txtPtr = StringAppend(sBerryBlender->stringVar, sText_Min);

            ConvertIntToDecimalStringN(txtPtr, seconds, STR_CONV_MODE_LEADING_ZEROS, 2);
            StringAppend(sBerryBlender->stringVar, sText_Sec);

            xPos = GetStringRightAlignXOffset(FONT_NORMAL, sBerryBlender->stringVar, 0xA8);
            Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, xPos, 0x61, TEXT_SKIP_DRAW, 3);

            sBerryBlender->framesToWait = 0;
            sBerryBlender->mainState++;

            CopyWindowToVram(WIN_RESULTS, COPYWIN_GFX);
        }
        break;
    case 4:
        if (JOY_NEW(A_BUTTON))
            sBerryBlender->mainState++;
        break;
    case 5:
        ClearStdWindowAndFrameToTransparent(WIN_RESULTS, TRUE);

        for (i = 0; i < BLENDER_MAX_PLAYERS; i++)
        {
            if (sBerryBlender->chosenItemId[i] != 0)
                berryIds[i] = sBerryBlender->chosenItemId[i] - FIRST_BERRY_INDEX;
            if (sBerryBlender->arrowIdToPlayerId[i] != NO_PLAYER)
            {
                PutWindowTilemap(i);
                CopyWindowToVram(i, COPYWIN_FULL);
            }
        }

        Debug_SetStageVars();
        CalculatePokeblock(sBerryBlender->blendedBerries, &pokeblock, sBerryBlender->numPlayers, flavors, sBerryBlender->maxRPM);
        PrintMadePokeblockString(&pokeblock, sBerryBlender->stringVar);
        TryAddContestLinkTvShow(&pokeblock, &sBerryBlender->tvBlender);

        CreateTask(Task_PlayPokeblockFanfare, 6);
        IncrementDailyBerryBlender();

        RemoveBagItem(gSpecialVar_ItemId, 1);
        AddPokeblock(&pokeblock);

        sBerryBlender->textState = 0;
        sBerryBlender->mainState++;
        break;
    case 6:
        if (PrintMessage(&sBerryBlender->textState, sBerryBlender->stringVar, GetPlayerTextSpeedDelay()))
        {
            TryUpdateBerryBlenderRecord();
            return TRUE;
        }
        break;
    }

    return FALSE;
}

static void PrintMadePokeblockString(struct Pokeblock *pokeblock, u8 *dst)
{
    u8 text[12];
    u8 flavorLvl, feel;

    dst[0] = EOS;
    StringCopy(dst, gPokeblockNames[pokeblock->color]);
    StringAppend(dst, sText_WasMade);
    StringAppend(dst, sText_NewLine);

    flavorLvl = GetHighestPokeblocksFlavorLevel(pokeblock);
    feel = GetPokeblocksFeel(pokeblock);

    StringAppend(dst, sText_TheLevelIs);
    ConvertIntToDecimalStringN(text, flavorLvl, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringAppend(dst, text);

    StringAppend(dst, sText_TheFeelIs);
    ConvertIntToDecimalStringN(text, feel, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringAppend(dst, text);

    StringAppend(dst, sText_Dot2);
    StringAppend(dst, sText_NewParagraph);
}

static void SortBasedOnPoints(u8 *places, u8 playersNum, u32 *scores)
{
    s32 i, j;

    for (i = 0; i < playersNum; i++)
    {
        for (j = 0; j < playersNum; j++)
        {
            if (scores[places[i]] > scores[places[j]])
            {
                u8 temp;
                SWAP(places[i], places[j], temp);
            }
        }
    }
}

static void SortScores(void)
{
    u8 playerId;
    u8 i;
    u8 places[BLENDER_MAX_PLAYERS];
    u32 points[BLENDER_MAX_PLAYERS];

    for (i = 0; i < sBerryBlender->numPlayers; i++)
        places[i] = i;
    for (i = 0; i < sBerryBlender->numPlayers; i++)
    {
        points[i] = 1000000 * sBerryBlender->scores[i][SCORE_BEST];
        points[i] += 1000 * sBerryBlender->scores[i][SCORE_GOOD];
        points[i] += 1000 - sBerryBlender->scores[i][SCORE_MISS];
    }
    SortBasedOnPoints(places, sBerryBlender->numPlayers, points);
    for (i = 0; i < sBerryBlender->numPlayers; i++)
        sBerryBlender->playerPlaces[i] = places[i];

    if (!gReceivedRemoteLinkPlayers)
        playerId = 0;
    else
        playerId = GetMultiplayerId();

    for (i = 0; i < sBerryBlender->numPlayers; i++)
    {
        if (sBerryBlender->playerPlaces[i] == playerId)
            sBerryBlender->ownRanking = i;
    }
}

static bool8 PrintBlendingRanking(void)
{
    u16 i;
    s32 xPos, yPos;

    switch (sBerryBlender->mainState)
    {
    case 0:
        sBerryBlender->mainState++;
        sBerryBlender->framesToWait = 255;
        break;
    case 1:
        sBerryBlender->framesToWait -= 10;
        if (sBerryBlender->framesToWait < 0)
        {
            sBerryBlender->framesToWait = 0;
            sBerryBlender->mainState++;
        }
        break;
    case 2:
        if (++sBerryBlender->framesToWait > 20)
        {
            sBerryBlender->framesToWait = 0;
            sBerryBlender->mainState++;
        }
        break;
    case 3:
        DrawStdFrameWithCustomTileAndPalette(WIN_RESULTS, FALSE, 1, 0xD);
        xPos = GetStringCenterAlignXOffset(FONT_NORMAL, sText_Ranking, 168);
        Blender_AddTextPrinter(WIN_RESULTS, sText_Ranking, xPos, 1, TEXT_SKIP_DRAW, 0);

        sBerryBlender->scoreIconIds[SCORE_BEST] = CreateSprite(&sSpriteTemplate_ScoreSymbols, 128, 52, 0);
        StartSpriteAnim(&gSprites[sBerryBlender->scoreIconIds[SCORE_BEST]], SCOREANIM_BEST_STATIC);
        gSprites[sBerryBlender->scoreIconIds[SCORE_BEST]].callback = SpriteCallbackDummy;

        sBerryBlender->scoreIconIds[SCORE_GOOD] = CreateSprite(&sSpriteTemplate_ScoreSymbols, 160, 52, 0);
        // implicitly uses SCOREANIM_GOOD, no need to assign
        gSprites[sBerryBlender->scoreIconIds[SCORE_GOOD]].callback = SpriteCallbackDummy;

        sBerryBlender->scoreIconIds[SCORE_MISS] = CreateSprite(&sSpriteTemplate_ScoreSymbols, 192, 52, 0);
        StartSpriteAnim(&gSprites[sBerryBlender->scoreIconIds[SCORE_MISS]], SCOREANIM_MISS);
        gSprites[sBerryBlender->scoreIconIds[SCORE_MISS]].callback = SpriteCallbackDummy;

        SortScores();

        for (yPos = 41, i = 0; i < sBerryBlender->numPlayers; yPos += 16, i++)
        {
            u8 place = sBerryBlender->playerPlaces[i];

            ConvertIntToDecimalStringN(sBerryBlender->stringVar, i + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
            StringAppend(sBerryBlender->stringVar, sText_Dot);
            StringAppend(sBerryBlender->stringVar, gText_Space);
            StringAppend(sBerryBlender->stringVar, gLinkPlayers[place].name);
            Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, 0, yPos, TEXT_SKIP_DRAW, 3);

            ConvertIntToDecimalStringN(sBerryBlender->stringVar, sBerryBlender->scores[place][SCORE_BEST], STR_CONV_MODE_RIGHT_ALIGN, 3);
            Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, 78, yPos, TEXT_SKIP_DRAW, 3);

            ConvertIntToDecimalStringN(sBerryBlender->stringVar, sBerryBlender->scores[place][SCORE_GOOD], STR_CONV_MODE_RIGHT_ALIGN, 3);
            Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, 78 + 32, yPos, TEXT_SKIP_DRAW, 3);

            ConvertIntToDecimalStringN(sBerryBlender->stringVar, sBerryBlender->scores[place][SCORE_MISS], STR_CONV_MODE_RIGHT_ALIGN, 3);
            Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, 78 + 64, yPos, TEXT_SKIP_DRAW, 3);
        }

        PutWindowTilemap(WIN_RESULTS);
        CopyWindowToVram(WIN_RESULTS, COPYWIN_FULL);

        sBerryBlender->framesToWait = 0;
        sBerryBlender->mainState++;
        break;
    case 4:
        if (++sBerryBlender->framesToWait > 20)
            sBerryBlender->mainState++;
        break;
    case 5:
        if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            sBerryBlender->mainState++;
        }
        break;
    case 6:
        sBerryBlender->mainState = 0;
        return TRUE;
    }

    return FALSE;
}

void ShowBerryBlenderRecordWindow(void)
{
    s32 i;
    s32 xPos, yPos;
    struct WindowTemplate winTemplate;
    u8 text[32];

    winTemplate = sBlenderRecordWindowTemplate;
    gRecordsWindowId = AddWindow(&winTemplate);
    DrawStdWindowFrame(gRecordsWindowId, FALSE);
    FillWindowPixelBuffer(gRecordsWindowId, PIXEL_FILL(1));

    xPos = GetStringCenterAlignXOffset(FONT_NORMAL, gText_BlenderMaxSpeedRecord, 144);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_BlenderMaxSpeedRecord, xPos, 1, 0, NULL);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gText_234Players, 4, 41, 0, NULL);

    for (i = 0, yPos = 41; i < NUM_SCORE_TYPES; i++)
    {
        u8 *txtPtr;
        u32 record;

        record = gSaveBlock1Ptr->berryBlenderRecords[i];

        txtPtr = ConvertIntToDecimalStringN(text, record / 100, STR_CONV_MODE_RIGHT_ALIGN, 3);
        txtPtr = StringAppend(txtPtr, sText_Dot);
        txtPtr = ConvertIntToDecimalStringN(txtPtr, record % 100, STR_CONV_MODE_LEADING_ZEROS, 2);
        txtPtr = StringAppend(txtPtr, sText_RPM);

        xPos = GetStringRightAlignXOffset(FONT_NORMAL, text, 140);
        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, text, xPos, yPos + (i * 16), 0, NULL);
    }

    PutWindowTilemap(gRecordsWindowId);
    CopyWindowToVram(gRecordsWindowId, COPYWIN_FULL);
}

static void Task_PlayPokeblockFanfare(u8 taskId)
{
    if (gTasks[taskId].data[0] == 0)
    {
        PlayFanfare(MUS_LEVEL_UP);
        gTasks[taskId].data[0]++;
    }
    if (IsFanfareTaskInactive())
    {
        PlayBGM(sBerryBlender->savedMusic);
        DestroyTask(taskId);
    }
}

static bool32 TryAddContestLinkTvShow(struct Pokeblock *pokeblock, struct TvBlenderStruct *tvBlender)
{
    u8 flavorLevel = GetHighestPokeblocksFlavorLevel(pokeblock);
    u16 sheen = (flavorLevel * 10) / GetPokeblocksFeel(pokeblock);

    tvBlender->pokeblockSheen = sheen;
    tvBlender->pokeblockColor = pokeblock->color;
    tvBlender->name[0] = EOS;

    if (gReceivedRemoteLinkPlayers)
    {
        if (sBerryBlender->ownRanking == 0 && sheen > 20)
        {
            // Player came first, try to put on air
            StringCopy(tvBlender->name, gLinkPlayers[sBerryBlender->playerPlaces[sBerryBlender->numPlayers - 1]].name);
            tvBlender->pokeblockFlavor = GetPokeblocksFlavor(pokeblock);
            if (Put3CheersForPokeblocksOnTheAir(tvBlender->name, tvBlender->pokeblockFlavor,
                                            tvBlender->pokeblockColor, tvBlender->pokeblockSheen,
                                            gLinkPlayers[sBerryBlender->playerPlaces[sBerryBlender->numPlayers - 1]].language))
            {
                return TRUE;
            }

            return FALSE;
        }
        else if (sBerryBlender->ownRanking == sBerryBlender->numPlayers - 1 && sheen <= 20)
        {
            // Player came last, try to put on air
            StringCopy(tvBlender->name, gLinkPlayers[sBerryBlender->playerPlaces[0]].name);
            tvBlender->pokeblockFlavor = GetPokeblocksFlavor(pokeblock);
            if (Put3CheersForPokeblocksOnTheAir(tvBlender->name, tvBlender->pokeblockFlavor,
                                            tvBlender->pokeblockColor, tvBlender->pokeblockSheen,
                                            gLinkPlayers[sBerryBlender->playerPlaces[0]].language))
            {
                return TRUE;
            }

            return FALSE;
        }
    }

    return FALSE;
}

static void Blender_AddTextPrinter(u8 windowId, const u8 *string, u8 x, u8 y, s32 speed, s32 caseId)
{
    u8 txtColor[3];
    u32 letterSpacing = 0;

    switch (caseId)
    {
    case 0:
    case 3:
#ifdef UBFIX
    default:
#endif
        txtColor[0] = TEXT_COLOR_WHITE;
        txtColor[1] = TEXT_COLOR_DARK_GRAY;
        txtColor[2] = TEXT_COLOR_LIGHT_GRAY;
        break;
    case 1:
        txtColor[0] = TEXT_COLOR_TRANSPARENT;
        txtColor[1] = TEXT_COLOR_DARK_GRAY;
        txtColor[2] = TEXT_COLOR_LIGHT_GRAY;
        break;
    case 2:
        txtColor[0] = TEXT_COLOR_TRANSPARENT;
        txtColor[1] = TEXT_COLOR_RED;
        txtColor[2] = TEXT_COLOR_LIGHT_RED;
        break;
    }

    if (caseId != 3)
        FillWindowPixelBuffer(windowId, PIXEL_FILL(txtColor[0]));

    AddTextPrinterParameterized4(windowId, FONT_NORMAL, x, y, letterSpacing, 1, txtColor, speed, string);
}

static bool32 PrintMessage(s16 *textState, const u8 *string, s32 textSpeed)
{
    switch (*textState)
    {
    case 0:
        DrawDialogFrameWithCustomTileAndPalette(WIN_MSG, FALSE, 0x14, 0xF);
        Blender_AddTextPrinter(WIN_MSG, string, 0, 1, textSpeed, 0);
        PutWindowTilemap(WIN_MSG);
        CopyWindowToVram(WIN_MSG, COPYWIN_FULL);
        (*textState)++;
        break;
    case 1:
        if (!IsTextPrinterActive(WIN_MSG))
        {
            *textState = 0;
            return TRUE;
        }
        break;
    }

    return FALSE;
}
