#include "global.h"
#include "gpu_regs.h"
#include "main.h"
#include "trainer_card.h"
#include "battle_anim.h"
#include "event_data.h"
#include "recorded_battle.h"
#include "malloc.h"
#include "sprite.h"
#include "scanline_effect.h"
#include "text_window.h"
#include "task.h"
#include "graphics.h"
#include "strings.h"
#include "frontier_pass.h"
#include "international_string_util.h"
#include "palette.h"
#include "window.h"
#include "decompress.h"
#include "menu_helpers.h"
#include "menu.h"
#include "bg.h"
#include "sound.h"
#include "string_util.h"
#include "battle_pyramid.h"
#include "overworld.h"
#include "math_util.h"
#include "constants/battle_frontier.h"
#include "constants/rgb.h"
#include "constants/region_map_sections.h"
#include "constants/songs.h"

// gFrontierPassBg_Pal has 8*16 colors, but they attempt to load 13*16 colors.
// As a result it goes out of bounds and interprets 160 bytes of whatever comes
// after gFrontierPassBg_Pal (by default, gFrontierPassBg_Gfx) as a palette.
// Nothing uses these colors (except the Trainer Card, which correctly writes them)
// so in practice this bug has no effect on the game.
#ifdef BUGFIX
#define NUM_BG_PAL_SLOTS 8
#else
#define NUM_BG_PAL_SLOTS 13
#endif

// All windows displayed in the frontier pass.
enum
{
    WINDOW_EARNED_SYMBOLS,
    WINDOW_BATTLE_RECORD,
    WINDOW_BATTLE_POINTS,
    WINDOW_DESCRIPTION,
    WINDOW_DUMMY,
    WINDOW_COUNT
};

// Windows displayed in the facilities map view.
enum
{
    MAP_WINDOW_UNUSED, // Overlaps the "Battle Frontier" title area of the map
    MAP_WINDOW_NAME,
    MAP_WINDOW_DESCRIPTION,
    MAP_WINDOW_COUNT
};

enum
{
    CURSOR_AREA_NOTHING,
    CURSOR_AREA_MAP,
    CURSOR_AREA_CARD,
    CURSOR_AREA_RECORD,
    CURSOR_AREA_CANCEL,
    CURSOR_AREA_POINTS,
    CURSOR_AREA_EARNED_SYMBOLS, // The window containing the symbols
    CURSOR_AREA_SYMBOL_TOWER,
    CURSOR_AREA_SYMBOL_DOME,
    CURSOR_AREA_SYMBOL_PALACE,
    CURSOR_AREA_SYMBOL_ARENA,
    CURSOR_AREA_SYMBOL_FACTORY,
    CURSOR_AREA_SYMBOL_PIKE,
    CURSOR_AREA_SYMBOL_PYRAMID,
    CURSOR_AREA_COUNT
};

// Start of symbol cursor areas
#define CURSOR_AREA_SYMBOL CURSOR_AREA_SYMBOL_TOWER

enum {
    MAP_INDICATOR_RECTANGLE,
    MAP_INDICATOR_SQUARE,
};

enum {
    TAG_CURSOR,
    TAG_MAP_INDICATOR,
    TAG_MEDAL_SILVER,
    TAG_MEDAL_GOLD,
    TAG_HEAD_MALE,
    TAG_HEAD_FEMALE,
};

// Error return codes. Never read
enum {
    SUCCESS,
    ERR_ALREADY_DONE,
    ERR_ALLOC_FAILED,
};

struct FrontierPassData
{
    void (*callback)(void);
    u16 state;
    u16 battlePoints;
    s16 cursorX;
    s16 cursorY;
    u8 cursorArea;
    u8 previousCursorArea;
    bool8 hasBattleRecord:1;
    u8 areaToShow:3;
    u8 trainerStars:4;
    u8 facilitySymbols[NUM_FRONTIER_FACILITIES]; // 0: no symbol, 1: silver, 2: gold
};

struct FrontierPassGfx
{
    struct Sprite *cursorSprite;
    struct Sprite *symbolSprites[NUM_FRONTIER_FACILITIES];
    // These 3 tilemaps are used to overwrite the respective area when highlighted
    u8 *mapAndCardZoomTilemap;
    u8 *mapAndCardTilemap;
    u8 *battleRecordTilemap;
    bool8 zooming;
    s16 scaleX;
    s16 scaleY;
    u8 tilemapBuff1[BG_SCREEN_SIZE * 2];
    u8 tilemapBuff2[BG_SCREEN_SIZE * 2];
    u8 tilemapBuff3[BG_SCREEN_SIZE / 2];
};

struct FrontierPassSaved
{
    void (*callback)(void);
    s16 cursorX;
    s16 cursorY;
};

struct FrontierMapData
{
    void (*callback)(void);
    struct Sprite *cursorSprite;
    struct Sprite *playerHeadSprite;
    struct Sprite *mapIndicatorSprite;
    u8 cursorPos;
    u8 unused;
    u8 tilemapBuff0[BG_SCREEN_SIZE * 2];
    u8 tilemapBuff1[BG_SCREEN_SIZE * 2];
    u8 tilemapBuff2[BG_SCREEN_SIZE * 2];
};

static EWRAM_DATA struct FrontierPassData *sPassData = NULL;
static EWRAM_DATA struct FrontierPassGfx *sPassGfx = NULL;
static EWRAM_DATA struct FrontierMapData *sMapData = NULL;
static EWRAM_DATA struct FrontierPassSaved sSavedPassData = {0};

static u32 AllocateFrontierPassData(void (*callback)(void));
static void ShowFrontierMap(void (*callback)(void));
static void CB2_InitFrontierPass(void);
static void DrawFrontierPassBg(void);
static void FreeCursorAndSymbolSprites(void);
static void LoadCursorAndSymbolSprites(void);
static u32 FreeFrontierPassData(void);
static bool32 InitFrontierPass(void);
static bool32 HideFrontierPass(void);
static void Task_HandleFrontierPassInput(u8);
static void Task_PassAreaZoom(u8);
static void UpdateAreaHighlight(u8, u8);
static void PrintAreaDescription(u8);
static void ShowHideZoomingArea(bool8, bool8);
static void SpriteCB_PlayerHead(struct Sprite *);

static const u16 sMaleHead_Pal[]                 = INCBIN_U16("graphics/frontier_pass/map_heads.gbapal");
static const u16 sFemaleHead_Pal[]               = INCBIN_U16("graphics/frontier_pass/map_heads_female.gbapal");
static const u32 sMapScreen_Gfx[]                = INCBIN_U32("graphics/frontier_pass/map_screen.4bpp.lz");
static const u32 sCursor_Gfx[]                   = INCBIN_U32("graphics/frontier_pass/cursor.4bpp.lz");
static const u32 sHeads_Gfx[]                    = INCBIN_U32("graphics/frontier_pass/map_heads.4bpp.lz");
static const u32 sMapCursor_Gfx[]                = INCBIN_U32("graphics/frontier_pass/map_cursor.4bpp.lz");
static const u32 sMapScreen_Tilemap[]            = INCBIN_U32("graphics/frontier_pass/map_screen.bin.lz");
static const u32 sMapAndCard_ZoomedOut_Tilemap[] = INCBIN_U32("graphics/frontier_pass/small_map_and_card.bin.lz");
static const u32 sCardBall_Filled_Tilemap[]      = INCBIN_U32("graphics/frontier_pass/card_ball_filled.bin"); // Unused
static const u32 sBattleRecord_Tilemap[]         = INCBIN_U32("graphics/frontier_pass/record_frame.bin.lz");
static const u32 sMapAndCard_Zooming_Tilemap[]   = INCBIN_U32("graphics/frontier_pass/small_map_and_card_affine.bin.lz");

static const s16 sBgAffineCoords[][2] =
{
    [CURSOR_AREA_MAP - 1]  = {216,  32},
    [CURSOR_AREA_CARD - 1] = {216, 128}
};

static const struct BgTemplate sPassBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 1,
        .mapBaseIndex = 29,
        .screenSize = 1,
        .paletteMode = 1,
        .priority = 0,
        .baseTile = 0
    },
};

static const struct BgTemplate sMapBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
};

static const struct WindowTemplate sPassWindowTemplates[WINDOW_COUNT] =
{
    [WINDOW_EARNED_SYMBOLS] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 3,
        .width = 12,
        .height = 3,
        .paletteNum = 15,
        .baseBlock = 0x1,
    },
    [WINDOW_BATTLE_RECORD] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 10,
        .width = 12,
        .height = 3,
        .paletteNum = 15,
        .baseBlock = 0x26,
    },
    [WINDOW_BATTLE_POINTS] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 13,
        .width = 12,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x4B,
    },
    [WINDOW_DESCRIPTION] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 18,
        .width = 30,
        .height = 3,
        .paletteNum = 15,
        .baseBlock = 0x7C,
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sMapWindowTemplates[] =
{
    [MAP_WINDOW_UNUSED] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 1,
        .width = 15,
        .height = 5,
        .paletteNum = 15,
        .baseBlock = 0x1,
    },
    [MAP_WINDOW_NAME] = {
        .bg = 0,
        .tilemapLeft = 20,
        .tilemapTop = 1,
        .width = 10,
        .height = 14,
        .paletteNum = 15,
        .baseBlock = 0x4D,
    },
    [MAP_WINDOW_DESCRIPTION] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 16,
        .width = 26,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0xDA,
    },
    DUMMY_WIN_TEMPLATE
};

static const u8 sTextColors[][3] =
{
    {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY},
    {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_BLUE},
    {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_RED, TEXT_COLOR_LIGHT_RED},
};

struct
{
    s16 yStart;
    s16 yEnd;
    s16 xStart;
    s16 xEnd;
}
static const sPassAreasLayout[CURSOR_AREA_COUNT - 1] =
{
    [CURSOR_AREA_MAP - 1]            = { 28,  76, 132, 220},
    [CURSOR_AREA_CARD - 1]           = { 84, 132, 132, 220},
    [CURSOR_AREA_RECORD - 1]         = { 80, 102,  20, 108},
    [CURSOR_AREA_CANCEL - 1]         = {  0,  16, 152, 240},
    [CURSOR_AREA_POINTS - 1]         = {108, 134,  20, 108},
    [CURSOR_AREA_EARNED_SYMBOLS - 1] = { 24,  48,  20, 108},
    [CURSOR_AREA_SYMBOL_TOWER - 1]   = { 50,  66,  20,  36},
    [CURSOR_AREA_SYMBOL_DOME - 1]    = { 66,  82,  32,  48},
    [CURSOR_AREA_SYMBOL_PALACE - 1]  = { 50,  66,  44,  60},
    [CURSOR_AREA_SYMBOL_ARENA - 1]   = { 66,  82,  56,  72},
    [CURSOR_AREA_SYMBOL_FACTORY - 1] = { 50,  66,  68,  84},
    [CURSOR_AREA_SYMBOL_PIKE - 1]    = { 66,  82,  80,  96},
    [CURSOR_AREA_SYMBOL_PYRAMID - 1] = { 50,  66,  92, 108},
};

static const struct CompressedSpriteSheet sCursorSpriteSheets[] =
{
    {sCursor_Gfx, 0x100, TAG_CURSOR},
    {sMapCursor_Gfx, 0x400, TAG_MAP_INDICATOR},
    {gFrontierPassMedals_Gfx, 0x380, TAG_MEDAL_SILVER},
};

static const struct CompressedSpriteSheet sHeadsSpriteSheet[] =
{
    {sHeads_Gfx, 0x100, TAG_HEAD_MALE},
    {}
};

static const struct SpritePalette sSpritePalettes[] =
{
    {gFrontierPassCursor_Pal,       TAG_CURSOR},
    {gFrontierPassMapCursor_Pal,    TAG_MAP_INDICATOR},
    {gFrontierPassMedalsSilver_Pal, TAG_MEDAL_SILVER},
    {gFrontierPassMedalsGold_Pal,   TAG_MEDAL_GOLD},
    {sMaleHead_Pal,                 TAG_HEAD_MALE},
    {sFemaleHead_Pal,               TAG_HEAD_FEMALE},
    {}
};

static const union AnimCmd sAnim_Frame1_Unused[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Frame1[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Frame2[] =
{
    ANIMCMD_FRAME(4, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Frame3[] =
{
    ANIMCMD_FRAME(8, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Frame4[] =
{
    ANIMCMD_FRAME(12, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Frame5[] =
{
    ANIMCMD_FRAME(16, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Frame6[] =
{
    ANIMCMD_FRAME(20, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Frame7[] =
{
    ANIMCMD_FRAME(24, 0),
    ANIMCMD_END
};

static const union AnimCmd sAnim_MapIndicatorCursor_Rectangle[] =
{
    ANIMCMD_FRAME(0, 45),
    ANIMCMD_FRAME(8, 45),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd sAnim_MapIndicatorCursor_Square[] =
{
    ANIMCMD_FRAME(16, 45),
    ANIMCMD_FRAME(24, 45),
    ANIMCMD_JUMP(0)
};

// Used both by the cursor and the map head icons
static const union AnimCmd *const sAnims_TwoFrame[] =
{
    sAnim_Frame1,
    sAnim_Frame2
};

static const union AnimCmd *const sAnims_Medal[] =
{
    [CURSOR_AREA_SYMBOL_TOWER   - CURSOR_AREA_SYMBOL] = sAnim_Frame1,
    [CURSOR_AREA_SYMBOL_DOME    - CURSOR_AREA_SYMBOL] = sAnim_Frame2,
    [CURSOR_AREA_SYMBOL_PALACE  - CURSOR_AREA_SYMBOL] = sAnim_Frame3,
    [CURSOR_AREA_SYMBOL_ARENA   - CURSOR_AREA_SYMBOL] = sAnim_Frame4,
    [CURSOR_AREA_SYMBOL_FACTORY - CURSOR_AREA_SYMBOL] = sAnim_Frame5,
    [CURSOR_AREA_SYMBOL_PIKE    - CURSOR_AREA_SYMBOL] = sAnim_Frame6,
    [CURSOR_AREA_SYMBOL_PYRAMID - CURSOR_AREA_SYMBOL] = sAnim_Frame7
};

static const union AnimCmd *const sAnims_MapIndicatorCursor[] =
{
    [MAP_INDICATOR_RECTANGLE] = sAnim_MapIndicatorCursor_Rectangle,
    [MAP_INDICATOR_SQUARE]    = sAnim_MapIndicatorCursor_Square
};

static const union AffineAnimCmd sAffineAnim_Unused[] =
{
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd *const sAffineAnims_Unused[] =
{
    sAffineAnim_Unused
};

static const struct SpriteTemplate sSpriteTemplates_Cursors[] =
{
    // Triangular cursor
    {
        .tileTag = TAG_CURSOR,
        .paletteTag = TAG_CURSOR,
        .oam = &gOamData_AffineOff_ObjNormal_16x16,
        .anims = sAnims_TwoFrame,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    },
    // Map indicator cursor
    {
        .tileTag = TAG_MAP_INDICATOR,
        .paletteTag = TAG_MAP_INDICATOR,
        .oam = &gOamData_AffineOff_ObjNormal_32x16,
        .anims = sAnims_MapIndicatorCursor,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    },
};

static const struct SpriteTemplate sSpriteTemplate_Medal =
{
    .tileTag = TAG_MEDAL_SILVER,
    .paletteTag = TAG_MEDAL_SILVER,
    .oam = &gOamData_AffineOff_ObjNormal_16x16,
    .anims = sAnims_Medal,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sSpriteTemplate_PlayerHead =
{
    .tileTag = TAG_HEAD_MALE,
    .paletteTag = TAG_HEAD_MALE,
    .oam = &gOamData_AffineOff_ObjNormal_16x16,
    .anims = sAnims_TwoFrame,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_PlayerHead,
};

static const u8 *const sPassAreaDescriptions[CURSOR_AREA_COUNT + 1] =
{
    [CURSOR_AREA_NOTHING]        = gText_ThereIsNoBattleRecord, // NOTHING is re-used for CURSOR_AREA_RECORD when no Record is present
    [CURSOR_AREA_MAP]            = gText_CheckFrontierMap,
    [CURSOR_AREA_CARD]           = gText_CheckTrainerCard,
    [CURSOR_AREA_RECORD]         = gText_ViewRecordedBattle,
    [CURSOR_AREA_CANCEL]         = gText_PutAwayFrontierPass,
    [CURSOR_AREA_POINTS]         = gText_CurrentBattlePoints,
    [CURSOR_AREA_EARNED_SYMBOLS] = gText_CollectedSymbols,
    [CURSOR_AREA_SYMBOL_TOWER]   = gText_BattleTowerAbilitySymbol,
    [CURSOR_AREA_SYMBOL_DOME]    = gText_BattleDomeTacticsSymbol,
    [CURSOR_AREA_SYMBOL_PALACE]  = gText_BattlePalaceSpiritsSymbol,
    [CURSOR_AREA_SYMBOL_ARENA]   = gText_BattleArenaGutsSymbol,
    [CURSOR_AREA_SYMBOL_FACTORY] = gText_BattleFactoryKnowledgeSymbol,
    [CURSOR_AREA_SYMBOL_PIKE]    = gText_BattlePikeLuckSymbol,
    [CURSOR_AREA_SYMBOL_PYRAMID] = gText_BattlePyramidBraveSymbol,
    [CURSOR_AREA_COUNT]          = gText_EmptyString7,
};

struct
{
    const u8 *name;
    const u8 *description;
    s16 x;
    s16 y;
    u8 animNum;
} static const sMapLandmarks[NUM_FRONTIER_FACILITIES] =
{
    [FRONTIER_FACILITY_TOWER]   = {gText_BattleTower3,   gText_BattleTowerDesc,    89,  40, MAP_INDICATOR_SQUARE},
    [FRONTIER_FACILITY_DOME]    = {gText_BattleDome2,    gText_BattleDomeDesc,     33,  42, MAP_INDICATOR_SQUARE},
    [FRONTIER_FACILITY_PALACE]  = {gText_BattlePalace2,  gText_BattlePalaceDesc,  120,  86, MAP_INDICATOR_RECTANGLE},
    [FRONTIER_FACILITY_ARENA]   = {gText_BattleArena2,   gText_BattleArenaDesc,   114,  59, MAP_INDICATOR_RECTANGLE},
    [FRONTIER_FACILITY_FACTORY] = {gText_BattleFactory2, gText_BattleFactoryDesc,  25,  67, MAP_INDICATOR_RECTANGLE},
    [FRONTIER_FACILITY_PIKE]    = {gText_BattlePike2,    gText_BattlePikeDesc,     57,  57, MAP_INDICATOR_SQUARE},
    [FRONTIER_FACILITY_PYRAMID] = {gText_BattlePyramid2, gText_BattlePyramidDesc, 134,  41, MAP_INDICATOR_SQUARE},
};

static void ResetGpuRegsAndBgs(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG3CNT, 0);
    SetGpuReg(REG_OFFSET_BG2CNT, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
    ChangeBgX(2, 0, BG_COORD_SET);
    ChangeBgY(2, 0, BG_COORD_SET);
    ChangeBgX(3, 0, BG_COORD_SET);
    ChangeBgY(3, 0, BG_COORD_SET);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_WIN1H, 0);
    SetGpuReg(REG_OFFSET_WIN1V, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    CpuFill16(0, (void *)VRAM, VRAM_SIZE);
    CpuFill32(0, (void *)OAM, OAM_SIZE);
}

void ShowFrontierPass(void (*callback)(void))
{
    AllocateFrontierPassData(callback);
    SetMainCallback2(CB2_InitFrontierPass);
}

static void LeaveFrontierPass(void)
{
    SetMainCallback2(sPassData->callback);
    FreeFrontierPassData();
}

static u32 AllocateFrontierPassData(void (*callback)(void))
{
    u8 i;

    if (sPassData != NULL)
        return ERR_ALREADY_DONE;

    sPassData = AllocZeroed(sizeof(*sPassData));
    if (sPassData == NULL)
        return ERR_ALLOC_FAILED;

    sPassData->callback = callback;
    i = GetCurrentRegionMapSectionId();
    if (i != MAPSEC_BATTLE_FRONTIER && i != MAPSEC_ARTISAN_CAVE)
    {
        // Player is not in the frontier, set
        // cursor position to the Trainer Card
        sPassData->cursorX = 176;
        sPassData->cursorY = 104;
    }
    else
    {
        // Player is in the frontier, set
        // cursor position to the frontier map
        sPassData->cursorX = 176;
        sPassData->cursorY = 48;
    }

    sPassData->battlePoints = gSaveBlock2Ptr->frontier.battlePoints;
    sPassData->hasBattleRecord = CanCopyRecordedBattleSaveData();
    sPassData->areaToShow = CURSOR_AREA_NOTHING;
    sPassData->trainerStars = CountPlayerTrainerStars();
    for (i = 0; i < NUM_FRONTIER_FACILITIES; i++)
    {
        if (FlagGet(FLAG_SYS_TOWER_SILVER + i * 2))
            sPassData->facilitySymbols[i]++;
        if (FlagGet(FLAG_SYS_TOWER_GOLD + i * 2))
            sPassData->facilitySymbols[i]++;
    }

    return SUCCESS;
}

static u32 FreeFrontierPassData(void)
{
    if (sPassData == NULL)
        return ERR_ALREADY_DONE;

    memset(sPassData, 0, sizeof(*sPassData)); // Why clear data, if it's going to be freed anyway?
    FREE_AND_SET_NULL(sPassData);
    return SUCCESS;
}

static u32 AllocateFrontierPassGfx(void)
{
    if (sPassGfx != NULL)
        return ERR_ALREADY_DONE;

    sPassGfx = AllocZeroed(sizeof(*sPassGfx));
    if (sPassGfx == NULL)
        return ERR_ALLOC_FAILED;

    return SUCCESS;
}

static u32 FreeFrontierPassGfx(void)
{
    FreeAllWindowBuffers();
    if (sPassGfx == NULL)
        return ERR_ALREADY_DONE;

    TRY_FREE_AND_SET_NULL(sPassGfx->battleRecordTilemap);
    TRY_FREE_AND_SET_NULL(sPassGfx->mapAndCardTilemap);
    TRY_FREE_AND_SET_NULL(sPassGfx->mapAndCardZoomTilemap);

    memset(sPassGfx, 0, sizeof(*sPassGfx)); // Why clear data, if it's going to be freed anyway?
    FREE_AND_SET_NULL(sPassGfx);
    return SUCCESS;
}

static void VBlankCB_FrontierPass(void)
{
    if (sPassGfx->zooming)
    {
        SetBgAffine(2,
                    sBgAffineCoords[sPassData->areaToShow - 1][0] << 8,
                    sBgAffineCoords[sPassData->areaToShow - 1][1] << 8,
                    sBgAffineCoords[sPassData->areaToShow - 1][0],
                    sBgAffineCoords[sPassData->areaToShow - 1][1],
                    sPassGfx->scaleX,
                    sPassGfx->scaleY,
                    0);
    }
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_FrontierPass(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
}

static void CB2_InitFrontierPass(void)
{
    if (InitFrontierPass())
    {
        CreateTask(Task_HandleFrontierPassInput, 0);
        SetMainCallback2(CB2_FrontierPass);
    }
}

static void CB2_HideFrontierPass(void)
{
    if (HideFrontierPass())
        LeaveFrontierPass();
}

static bool32 InitFrontierPass(void)
{
    u32 sizeOut = 0;

    switch (sPassData->state)
    {
    case 0:
        SetVBlankCallback(NULL);
        ScanlineEffect_Stop();
        SetVBlankHBlankCallbacksToNull();
        DisableInterrupts(INTR_FLAG_HBLANK);
        break;
    case 1:
        ResetGpuRegsAndBgs();
        break;
    case 2:
        ResetTasks();
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetPaletteFade();
        ResetTempTileDataBuffers();
        break;
    case 3:
        AllocateFrontierPassGfx();
        break;
    case 4:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(1, sPassBgTemplates, ARRAY_COUNT(sPassBgTemplates));
        SetBgTilemapBuffer(1, sPassGfx->tilemapBuff1);
        SetBgTilemapBuffer(2, sPassGfx->tilemapBuff2);
        SetBgTilemapBuffer(3, sPassGfx->tilemapBuff3);
        SetBgAttribute(2, BG_ATTR_WRAPAROUND, 1);
        break;
    case 5:
        InitWindows(sPassWindowTemplates);
        DeactivateAllTextPrinters();
        break;
    case 6:
        sPassGfx->mapAndCardZoomTilemap = malloc_and_decompress(sMapAndCard_Zooming_Tilemap, &sizeOut);
        sPassGfx->mapAndCardTilemap = malloc_and_decompress(sMapAndCard_ZoomedOut_Tilemap, &sizeOut);
        sPassGfx->battleRecordTilemap = malloc_and_decompress(sBattleRecord_Tilemap, &sizeOut);
        DecompressAndCopyTileDataToVram(1, gFrontierPassBg_Gfx, 0, 0, 0);
        DecompressAndCopyTileDataToVram(2, gFrontierPassMapAndCard_Gfx, 0, 0, 0);
        break;
    case 7:
        if (FreeTempTileDataBuffersIfPossible())
            return FALSE;
        FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
        FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
        FillBgTilemapBufferRect_Palette0(2, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
        CopyBgTilemapBufferToVram(0);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(2);
        break;
    case 8:
        LoadPalette(gFrontierPassBg_Pal, 0, NUM_BG_PAL_SLOTS * PLTT_SIZE_4BPP);
        LoadPalette(gFrontierPassBg_Pal[1 + sPassData->trainerStars], BG_PLTT_ID(1), PLTT_SIZE_4BPP);
        LoadPalette(GetTextWindowPalette(0), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
        DrawFrontierPassBg();
        UpdateAreaHighlight(sPassData->cursorArea, sPassData->previousCursorArea);
        if (sPassData->areaToShow == CURSOR_AREA_MAP || sPassData->areaToShow == CURSOR_AREA_CARD)
        {
            sPassData->state = 0;
            return TRUE;
        }
        break;
    case 9:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        LoadCursorAndSymbolSprites();
        SetVBlankCallback(VBlankCB_FrontierPass);
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        break;
    case 10:
        AnimateSprites();
        BuildOamBuffer();
        if (UpdatePaletteFade())
            return FALSE;

        sPassData->state = 0;
        return TRUE;
    }

    sPassData->state++;
    return FALSE;
}

static bool32 HideFrontierPass(void)
{
    switch (sPassData->state)
    {
    case 0:
        if (sPassData->areaToShow != CURSOR_AREA_MAP && sPassData->areaToShow != CURSOR_AREA_CARD)
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        }
        else
        {
            sPassData->state = 2;
            return FALSE;
        }
        break;
    case 1:
        if (UpdatePaletteFade())
            return FALSE;
        break;
    case 2:
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        HideBg(0);
        HideBg(1);
        HideBg(2);
        SetVBlankCallback(NULL);
        ScanlineEffect_Stop();
        SetVBlankHBlankCallbacksToNull();
        break;
    case 3:
        FreeCursorAndSymbolSprites();
        break;
    case 4:
        ResetGpuRegsAndBgs();
        ResetTasks();
        ResetSpriteData();
        FreeAllSpritePalettes();
        break;
    case 5:
        UnsetBgTilemapBuffer(0);
        UnsetBgTilemapBuffer(1);
        UnsetBgTilemapBuffer(2);
        FreeFrontierPassGfx();
        sPassData->state = 0;
        return TRUE;
    }

    sPassData->state++;
    return FALSE;
}

static u8 GetCursorAreaFromCoords(s16 x, s16 y)
{
    u8 i;

    // Minus/Plus 1, because the table doesn't take CURSOR_AREA_NOTHING into account.
    for (i = 0; i < CURSOR_AREA_COUNT - 1; i++)
    {
        if (sPassAreasLayout[i].yStart <= y && sPassAreasLayout[i].yEnd >= y
         && sPassAreasLayout[i].xStart <= x && sPassAreasLayout[i].xEnd >= x)
        {
            if (i >= CURSOR_AREA_SYMBOL - 1 && sPassData->facilitySymbols[i - CURSOR_AREA_SYMBOL + 1] == 0)
                break;

            return i + 1;
        }
    }

    return CURSOR_AREA_NOTHING;
}

// For Task_PassAreaZoom
#define tZoomOut data[0]

void CB2_ReshowFrontierPass(void)
{
    u8 taskId;

    if (!InitFrontierPass())
        return;

    switch (sPassData->areaToShow)
    {
    case CURSOR_AREA_MAP:
    case CURSOR_AREA_CARD:
        taskId = CreateTask(Task_PassAreaZoom, 0);
        gTasks[taskId].tZoomOut = TRUE;
        break;
    case CURSOR_AREA_RECORD:
    default:
        sPassData->areaToShow = CURSOR_AREA_NOTHING;
        taskId = CreateTask(Task_HandleFrontierPassInput, 0);
        break;
    }

    SetMainCallback2(CB2_FrontierPass);
}

static void CB2_ReturnFromRecord(void)
{
    AllocateFrontierPassData(sSavedPassData.callback);
    sPassData->cursorX = sSavedPassData.cursorX;
    sPassData->cursorY = sSavedPassData.cursorY;
    memset(&sSavedPassData, 0, sizeof(sSavedPassData));
    switch (CurrentBattlePyramidLocation())
    {
    case PYRAMID_LOCATION_FLOOR:
        PlayBGM(MUS_B_PYRAMID);
        break;
    case PYRAMID_LOCATION_TOP:
        PlayBGM(MUS_B_PYRAMID_TOP);
        break;
    default:
        Overworld_PlaySpecialMapMusic();
        break;
    }

    SetMainCallback2(CB2_ReshowFrontierPass);
}

static void CB2_ShowFrontierPassFeature(void)
{
    if (!HideFrontierPass())
        return;

    switch (sPassData->areaToShow)
    {
    case CURSOR_AREA_MAP:
        ShowFrontierMap(CB2_ReshowFrontierPass);
        break;
    case CURSOR_AREA_RECORD:
        sSavedPassData.callback = sPassData->callback;
        sSavedPassData.cursorX = sPassData->cursorX;
        sSavedPassData.cursorY = sPassData->cursorY;
        FreeFrontierPassData();
        PlayRecordedBattle(CB2_ReturnFromRecord);
        break;
    case CURSOR_AREA_CARD:
        ShowPlayerTrainerCard(CB2_ReshowFrontierPass);
        break;
    }
}

static bool32 TryCallPassAreaFunction(u8 taskId, u8 cursorArea)
{
    switch (cursorArea)
    {
    case CURSOR_AREA_RECORD:
        if (!sPassData->hasBattleRecord)
            return FALSE;
        sPassData->areaToShow = CURSOR_AREA_RECORD;
        DestroyTask(taskId);
        SetMainCallback2(CB2_ShowFrontierPassFeature);
        break;
    case CURSOR_AREA_MAP:
    case CURSOR_AREA_CARD:
        sPassData->areaToShow = cursorArea;
        gTasks[taskId].func = Task_PassAreaZoom;
        gTasks[taskId].tZoomOut = FALSE;
        break;
    default:
        return FALSE;
    }

    sPassData->cursorX = sPassGfx->cursorSprite->x;
    sPassData->cursorY = sPassGfx->cursorSprite->y;
    return TRUE;
}

static void Task_HandleFrontierPassInput(u8 taskId)
{
    u8 var = FALSE; // Reused, first informs whether the cursor moves, then used as the new cursor area.

    if (JOY_HELD(DPAD_UP) && sPassGfx->cursorSprite->y >= 9)
    {
        sPassGfx->cursorSprite->y -= 2;
        if (sPassGfx->cursorSprite->y <= 7)
            sPassGfx->cursorSprite->y = 2;
        var = TRUE;
    }
    if (JOY_HELD(DPAD_DOWN) && sPassGfx->cursorSprite->y <= 135)
    {
        sPassGfx->cursorSprite->y += 2;
        if (sPassGfx->cursorSprite->y >= 137)
            sPassGfx->cursorSprite->y = 136;
        var = TRUE;
    }

    if (JOY_HELD(DPAD_LEFT) && sPassGfx->cursorSprite->x >= 6)
    {
        sPassGfx->cursorSprite->x -= 2;
        if (sPassGfx->cursorSprite->x <= 4)
            sPassGfx->cursorSprite->x = 5;
        var = TRUE;
    }
    if (JOY_HELD(DPAD_RIGHT) && sPassGfx->cursorSprite->x <= 231)
    {
        sPassGfx->cursorSprite->x += 2;
        if (sPassGfx->cursorSprite->x >= 233)
            sPassGfx->cursorSprite->x = 232;
        var = TRUE;
    }

    if (!var) // Cursor did not change.
    {
        if (sPassData->cursorArea != CURSOR_AREA_NOTHING && JOY_NEW(A_BUTTON))
        {
            if (sPassData->cursorArea <= CURSOR_AREA_RECORD) // Map, Card, Record
            {
                PlaySE(SE_SELECT);
                if (TryCallPassAreaFunction(taskId, sPassData->cursorArea))
                    return;
            }
            else if (sPassData->cursorArea == CURSOR_AREA_CANCEL)
            {
                PlaySE(SE_PC_OFF);
                SetMainCallback2(CB2_HideFrontierPass);
                DestroyTask(taskId);
                // BUG. The function should return here. Otherwise, it can play the same sound twice and destroy the same task twice.
                #ifdef BUGFIX
                return;
                #endif
            }
        }

        if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_PC_OFF);
            SetMainCallback2(CB2_HideFrontierPass);
            DestroyTask(taskId);
        }
    }
    else
    {
        var = GetCursorAreaFromCoords(sPassGfx->cursorSprite->x - 5, sPassGfx->cursorSprite->y + 5);
        if (sPassData->cursorArea != var)
        {
            PrintAreaDescription(var);
            sPassData->previousCursorArea = sPassData->cursorArea;
            sPassData->cursorArea = var;
            UpdateAreaHighlight(sPassData->cursorArea, sPassData->previousCursorArea);
        }
    }
}

#define tScaleX      data[1]
#define tScaleY      data[2]
#define tScaleSpeedX data[3]
#define tScaleSpeedY data[4]

// Zoom in/out for the Frontier map or the trainer card
static void Task_PassAreaZoom(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (sPassData->state)
    {
    case 0:
        // Initialize the zoom, start fading in/out
        if (!tZoomOut)
        {
            // Zooming in to map/card screen
            ShowHideZoomingArea(TRUE, FALSE);
            tScaleX = Q_8_8(1);
            tScaleY = Q_8_8(1);
            tScaleSpeedX = 0x15;
            tScaleSpeedY = 0x15;
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_WHITE);
        }
        else
        {
            // Zooming out of map/card screen back to frontier pass
            tScaleX = Q_8_8(1.984375); // 1 and 63/64
            tScaleY = Q_8_8(1.984375);
            tScaleSpeedX = -0x15;
            tScaleSpeedY = -0x15;
            SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
            ShowBg(0);
            ShowBg(1);
            ShowBg(2);
            LoadCursorAndSymbolSprites();
            SetVBlankCallback(VBlankCB_FrontierPass);
            BlendPalettes(PALETTES_ALL, 16, RGB_WHITE);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_WHITE);
        }
        sPassGfx->zooming = TRUE;
        sPassGfx->scaleX = MathUtil_Inv16(tScaleX);
        sPassGfx->scaleY = MathUtil_Inv16(tScaleY);
        break;
    case 1:
        // Update the fade and zoom
        UpdatePaletteFade();
        tScaleX += tScaleSpeedX;
        tScaleY += tScaleSpeedY;
        sPassGfx->scaleX = MathUtil_Inv16(tScaleX);
        sPassGfx->scaleY = MathUtil_Inv16(tScaleY);

        // Check if zoom hasn't reached target
        if (!tZoomOut)
        {
            if (tScaleX <= Q_8_8(1.984375))
                return;
        }
        else
        {
            if (tScaleX != Q_8_8(1))
                return;
        }
        break;
    case 2:
        if (sPassGfx->zooming)
            sPassGfx->zooming = FALSE;
        if (UpdatePaletteFade())
            return;

        if (!tZoomOut)
        {
            // Zoomed in and faded out, switch to map or trainer card
            DestroyTask(taskId);
            SetMainCallback2(CB2_ShowFrontierPassFeature);
        }
        else
        {
            // Zoomed out and faded in, return to frontier pass
            ShowHideZoomingArea(FALSE, FALSE);
            sPassData->areaToShow = CURSOR_AREA_NOTHING;
            gTasks[taskId].func = Task_HandleFrontierPassInput;
        }
        SetBgAttribute(2, BG_ATTR_WRAPAROUND, 0);
        sPassData->state = 0;
        return;
    }

    sPassData->state++;
}

static void ShowAndPrintWindows(void)
{
    s32 x;
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++)
    {
        PutWindowTilemap(i);
        FillWindowPixelBuffer(i, PIXEL_FILL(0));
    }

    x = GetStringCenterAlignXOffset(FONT_NORMAL, gText_SymbolsEarned, 96);
    AddTextPrinterParameterized3(WINDOW_EARNED_SYMBOLS, FONT_NORMAL, x, 5, sTextColors[0], 0, gText_SymbolsEarned);

    x = GetStringCenterAlignXOffset(FONT_NORMAL, gText_BattleRecord, 96);
    AddTextPrinterParameterized3(WINDOW_BATTLE_RECORD, FONT_NORMAL, x, 5, sTextColors[0], 0, gText_BattleRecord);

    AddTextPrinterParameterized3(WINDOW_BATTLE_POINTS, FONT_SMALL_NARROW, 5, 4, sTextColors[0], 0, gText_BattlePoints);
    ConvertIntToDecimalStringN(gStringVar4, sPassData->battlePoints, STR_CONV_MODE_LEFT_ALIGN, 5);
    x = GetStringRightAlignXOffset(FONT_SMALL_NARROW, gStringVar4, 91);
    AddTextPrinterParameterized3(WINDOW_BATTLE_POINTS, FONT_SMALL_NARROW, x, 16, sTextColors[0], 0, gStringVar4);

    sPassData->cursorArea = GetCursorAreaFromCoords(sPassData->cursorX - 5, sPassData->cursorY + 5);
    sPassData->previousCursorArea = CURSOR_AREA_NOTHING;
    PrintAreaDescription(sPassData->cursorArea);

    for (i = 0; i < WINDOW_COUNT; i++)
        CopyWindowToVram(i, COPYWIN_FULL);

    CopyBgTilemapBufferToVram(0);
}

static void PrintAreaDescription(u8 cursorArea)
{
    FillWindowPixelBuffer(WINDOW_DESCRIPTION, PIXEL_FILL(0));

    if (cursorArea == CURSOR_AREA_RECORD && !sPassData->hasBattleRecord)
        AddTextPrinterParameterized3(WINDOW_DESCRIPTION, FONT_NORMAL, 2, 0, sTextColors[1], 0, sPassAreaDescriptions[CURSOR_AREA_NOTHING]);
    else if (cursorArea != CURSOR_AREA_NOTHING)
        AddTextPrinterParameterized3(WINDOW_DESCRIPTION, FONT_NORMAL, 2, 0, sTextColors[1], 0, sPassAreaDescriptions[cursorArea]);

    CopyWindowToVram(WINDOW_DESCRIPTION, COPYWIN_FULL);
    CopyBgTilemapBufferToVram(0);
}

static void ShowHideZoomingArea(bool8 show, bool8 zoomedIn)
{
    switch (sPassData->areaToShow)
    {
    case CURSOR_AREA_MAP:
        if (show)
            CopyToBgTilemapBufferRect_ChangePalette(2, sPassGfx->mapAndCardZoomTilemap, 16, 3, 12, 7, 16);
        else
            FillBgTilemapBufferRect(2, 0, 16, 3, 12, 7, 16);
        break;
    case CURSOR_AREA_CARD:
        if (show)
            CopyToBgTilemapBufferRect_ChangePalette(2, sPassGfx->mapAndCardZoomTilemap + 84, 16, 10, 12, 7, 16);
        else
            FillBgTilemapBufferRect(2, 0, 16, 10, 12, 7, 16);
        break;
    default:
        return;
    }

    CopyBgTilemapBufferToVram(2);
    if (zoomedIn)
    {
        SetBgAffine(2,
                    sBgAffineCoords[sPassData->areaToShow - 1][0] << 8,
                    sBgAffineCoords[sPassData->areaToShow - 1][1] << 8,
                    sBgAffineCoords[sPassData->areaToShow - 1][0],
                    sBgAffineCoords[sPassData->areaToShow - 1][1],
                    MathUtil_Inv16(Q_8_8(1.984375)), // 1 and 63/64
                    MathUtil_Inv16(Q_8_8(1.984375)),
                    0);
    }
    else
    {
        SetBgAffine(2,
                    sBgAffineCoords[sPassData->areaToShow - 1][0] << 8,
                    sBgAffineCoords[sPassData->areaToShow - 1][1] << 8,
                    sBgAffineCoords[sPassData->areaToShow - 1][0],
                    sBgAffineCoords[sPassData->areaToShow - 1][1],
                    MathUtil_Inv16(Q_8_8(1)),
                    MathUtil_Inv16(Q_8_8(1)),
                    0);
    }
}

static void UpdateAreaHighlight(u8 cursorArea, u8 previousCursorArea)
{
    #define NON_HIGHLIGHT_AREA(area) ((area) == CURSOR_AREA_NOTHING || (area) > CURSOR_AREA_CANCEL)

    // If moving off highlightable area, unhighlight it
    switch (previousCursorArea)
    {
    case CURSOR_AREA_MAP:
        CopyToBgTilemapBufferRect_ChangePalette(1, sPassGfx->mapAndCardTilemap, 16, 3, 12, 7, 17);
        break;
    case CURSOR_AREA_CARD:
        CopyToBgTilemapBufferRect_ChangePalette(1, sPassGfx->mapAndCardTilemap + 336, 16, 10, 12, 7, 17);
        break;
    case CURSOR_AREA_RECORD:
        if (sPassData->hasBattleRecord)
            CopyToBgTilemapBufferRect_ChangePalette(1, sPassGfx->battleRecordTilemap, 2, 10, 12, 3, 17);
        else if (NON_HIGHLIGHT_AREA(cursorArea))
            return;
        break;
    case CURSOR_AREA_CANCEL:
        CopyToBgTilemapBufferRect_ChangePalette(1, gFrontierPassCancelButton_Tilemap, 21, 0, 9, 2, 17);
        break;
    default:
        if (NON_HIGHLIGHT_AREA(cursorArea))
            return;
        break;
    }

    // If moving on to highlightable area, highlight it
    switch (cursorArea)
    {
    case CURSOR_AREA_MAP:
        CopyToBgTilemapBufferRect_ChangePalette(1, sPassGfx->mapAndCardTilemap + 168, 16, 3, 12, 7, 17);
        break;
    case CURSOR_AREA_CARD:
        CopyToBgTilemapBufferRect_ChangePalette(1, sPassGfx->mapAndCardTilemap + 504, 16, 10, 12, 7, 17);
        break;
    case CURSOR_AREA_RECORD:
        if (sPassData->hasBattleRecord)
            CopyToBgTilemapBufferRect_ChangePalette(1, sPassGfx->battleRecordTilemap + 72, 2, 10, 12, 3, 17);
        else
            return;
        break;
    case CURSOR_AREA_CANCEL:
        CopyToBgTilemapBufferRect_ChangePalette(1, gFrontierPassCancelButtonHighlighted_Tilemap, 21, 0, 9, 2, 17);
        break;
    default:
        if (NON_HIGHLIGHT_AREA(previousCursorArea))
            return;
    }

    CopyBgTilemapBufferToVram(1);
}

static void DrawFrontierPassBg(void)
{
    CopyToBgTilemapBuffer(1, gFrontierPassBg_Tilemap, 0, 0);
    UpdateAreaHighlight(sPassData->cursorArea, sPassData->previousCursorArea);
    ShowHideZoomingArea(TRUE, sPassData->areaToShow); // If returning to frontier pass from map/card (areaToShow will be != 0)
    ShowAndPrintWindows();
    CopyBgTilemapBufferToVram(1);
}

static void LoadCursorAndSymbolSprites(void)
{
    u8 spriteId;
    u8 i = 0;

    FreeAllSpritePalettes();
    ResetAffineAnimData();
    LoadSpritePalettes(sSpritePalettes);
    LoadCompressedSpriteSheet(&sCursorSpriteSheets[0]);
    LoadCompressedSpriteSheet(&sCursorSpriteSheets[2]);
    spriteId = CreateSprite(&sSpriteTemplates_Cursors[0], sPassData->cursorX, sPassData->cursorY, 0);
    sPassGfx->cursorSprite = &gSprites[spriteId];
    sPassGfx->cursorSprite->oam.priority = 0;

    for (i = 0; i < NUM_FRONTIER_FACILITIES; i++)
    {
        if (sPassData->facilitySymbols[i] != 0)
        {
            struct SpriteTemplate sprite = sSpriteTemplate_Medal;

            sprite.paletteTag += sPassData->facilitySymbols[i] - 1; // Adds 1 if gold for TAG_MEDAL_GOLD
            spriteId = CreateSprite(&sprite, sPassAreasLayout[i + CURSOR_AREA_SYMBOL - 1].xStart + 8, sPassAreasLayout[i + CURSOR_AREA_SYMBOL - 1].yStart + 6, i + 1);
            sPassGfx->symbolSprites[i] = &gSprites[spriteId];
            sPassGfx->symbolSprites[i]->oam.priority = 2;
            StartSpriteAnim(sPassGfx->symbolSprites[i], i);
        }
    }
}

static void FreeCursorAndSymbolSprites(void)
{
    u8 i = 0;

    DestroySprite(sPassGfx->cursorSprite);
    sPassGfx->cursorSprite = NULL;
    for (i = 0; i < NUM_FRONTIER_FACILITIES; i++)
    {
        if (sPassGfx->symbolSprites[i] != NULL)
        {
            DestroySprite(sPassGfx->symbolSprites[i]);
            sPassGfx->symbolSprites[i] = NULL;
        }
    }
    FreeAllSpritePalettes();
    FreeSpriteTilesByTag(TAG_MEDAL_SILVER);
    FreeSpriteTilesByTag(TAG_CURSOR);
}

static void SpriteCB_PlayerHead(struct Sprite *sprite)
{

}

// Frontier Map code.

// Forward declarations.
static void Task_HandleFrontierMap(u8 taskId);
static void PrintOnFrontierMap(void);
static void InitFrontierMapSprites(void);
static void HandleFrontierMapCursorMove(u8 direction);

static void ShowFrontierMap(void (*callback)(void))
{
    if (sMapData != NULL)
        SetMainCallback2(callback); // This line doesn't make sense at all, since it gets overwritten later anyway.

    sMapData = AllocZeroed(sizeof(*sMapData));
    sMapData->callback = callback;
    ResetTasks();
    CreateTask(Task_HandleFrontierMap, 0);
    SetMainCallback2(CB2_FrontierPass);
}

static void FreeFrontierMap(void)
{
    ResetTasks();
    SetMainCallback2(sMapData->callback);
    memset(sMapData, 0, sizeof(*sMapData)); // Pointless memory clear.
    FREE_AND_SET_NULL(sMapData);
}

static bool32 InitFrontierMap(void)
{
    switch (sPassData->state)
    {
    case 0:
        SetVBlankCallback(NULL);
        ScanlineEffect_Stop();
        SetVBlankHBlankCallbacksToNull();
        break;
    case 1:
        ResetGpuRegsAndBgs();
        break;
    case 2:
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetPaletteFade();
        ResetTempTileDataBuffers();
        break;
    case 3:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sMapBgTemplates, ARRAY_COUNT(sMapBgTemplates));
        SetBgTilemapBuffer(0, sMapData->tilemapBuff0);
        SetBgTilemapBuffer(1, sMapData->tilemapBuff1);
        SetBgTilemapBuffer(2, sMapData->tilemapBuff2);
        FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
        FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
        FillBgTilemapBufferRect_Palette0(2, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
        CopyBgTilemapBufferToVram(0);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(2);
        break;
    case 4:
        InitWindows(sMapWindowTemplates);
        DeactivateAllTextPrinters();
        PrintOnFrontierMap();
        DecompressAndCopyTileDataToVram(1, sMapScreen_Gfx, 0, 0, 0);
        break;
    case 5:
        if (FreeTempTileDataBuffersIfPossible())
            return FALSE;
        LoadPalette(gFrontierPassBg_Pal, BG_PLTT_ID(0), NUM_BG_PAL_SLOTS * PLTT_SIZE_4BPP);
        LoadPalette(GetTextWindowPalette(0), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
        CopyToBgTilemapBuffer(2, sMapScreen_Tilemap, 0, 0);
        CopyBgTilemapBufferToVram(2);
        break;
    case 6:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        InitFrontierMapSprites();
        SetVBlankCallback(VBlankCB_FrontierPass);
        BlendPalettes(PALETTES_ALL, 16, RGB_WHITE);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_WHITE);
        break;
    case 7:
        if (UpdatePaletteFade())
            return FALSE;
        sPassData->state = 0;
        return TRUE;
    }

    sPassData->state++;
    return FALSE;
}

static bool32 ExitFrontierMap(void)
{
    switch (sPassData->state)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_WHITE);
        break;
    case 1:
        if (UpdatePaletteFade())
            return FALSE;
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        HideBg(0);
        HideBg(1);
        HideBg(2);
        break;
    case 2:
        SetVBlankCallback(NULL);
        ScanlineEffect_Stop();
        SetVBlankHBlankCallbacksToNull();
        break;
    case 3:
        if (sMapData->cursorSprite != NULL)
        {
            DestroySprite(sMapData->cursorSprite);
            FreeSpriteTilesByTag(TAG_CURSOR);
        }
        if (sMapData->mapIndicatorSprite != NULL)
        {
            DestroySprite(sMapData->mapIndicatorSprite);
            FreeSpriteTilesByTag(TAG_MAP_INDICATOR);
        }
        if (sMapData->playerHeadSprite != NULL)
        {
            DestroySprite(sMapData->playerHeadSprite);
            FreeSpriteTilesByTag(TAG_HEAD_MALE);
        }
        FreeAllWindowBuffers();
        break;
    case 4:
        ResetGpuRegsAndBgs();
        ResetSpriteData();
        FreeAllSpritePalettes();
        break;
    case 5:
        UnsetBgTilemapBuffer(0);
        UnsetBgTilemapBuffer(1);
        UnsetBgTilemapBuffer(2);
        sPassData->state = 0;
        return TRUE;
    }

    sPassData->state++;
    return FALSE;
}

#define tState     data[0]
#define tMoveSteps data[1]

static void Task_HandleFrontierMap(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        if (InitFrontierMap())
            break;
        return;
    case 1:
        if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_PC_OFF);
            tState = 4;
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            if (sMapData->cursorPos >= NUM_FRONTIER_FACILITIES - 1)
                HandleFrontierMapCursorMove(0);
            else
                tState = 2;
        }
        else if (JOY_NEW(DPAD_UP))
        {
            if (sMapData->cursorPos == 0)
                HandleFrontierMapCursorMove(1);
            else
                tState = 3;
        }
        return;
    case 2:
        if (tMoveSteps > 3)
        {
            HandleFrontierMapCursorMove(0);
            tMoveSteps = 0;
            tState = 1;
        }
        else
        {
            sMapData->cursorSprite->y += 4;
            tMoveSteps++;
        }
        return;
    case 3:
        if (tMoveSteps > 3)
        {
            HandleFrontierMapCursorMove(1);
            tMoveSteps = 0;
            tState = 1;
        }
        else
        {
            sMapData->cursorSprite->y -= 4;
            tMoveSteps++;
        }
        return;
    case 4:
        if (ExitFrontierMap())
            break;
        return;
    case 5:
        DestroyTask(taskId);
        FreeFrontierMap();
        return;
    }

    tState++;
}

static u8 MapNumToFrontierFacilityId(u16 mapNum) // id + 1, zero means not a frontier map number
{
    // In Battle Tower
    if ((mapNum >= MAP_NUM(BATTLE_FRONTIER_BATTLE_TOWER_LOBBY) && mapNum <= MAP_NUM(BATTLE_FRONTIER_BATTLE_TOWER_BATTLE_ROOM))
     || (mapNum >= MAP_NUM(BATTLE_FRONTIER_BATTLE_TOWER_MULTI_PARTNER_ROOM) && mapNum <= MAP_NUM(BATTLE_FRONTIER_BATTLE_TOWER_MULTI_BATTLE_ROOM)))
        return FRONTIER_FACILITY_TOWER + 1;

    // In Battle Dome
    else if (mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_DOME_LOBBY)
             || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_DOME_CORRIDOR)
             || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_DOME_PRE_BATTLE_ROOM)
             || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_DOME_BATTLE_ROOM))
        return FRONTIER_FACILITY_DOME + 1;

    // In Battle Palace
    else if (mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_PALACE_LOBBY)
        || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_PALACE_CORRIDOR)
        || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_PALACE_BATTLE_ROOM))
        return FRONTIER_FACILITY_PALACE + 1;

    // In Battle Arena
    else if (mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_ARENA_LOBBY)
        || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_ARENA_CORRIDOR)
        || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_ARENA_BATTLE_ROOM))
        return FRONTIER_FACILITY_ARENA + 1;

    // In Battle Factory
    else if (mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_FACTORY_LOBBY)
        || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_FACTORY_PRE_BATTLE_ROOM)
        || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_FACTORY_BATTLE_ROOM))
        return FRONTIER_FACILITY_FACTORY + 1;

    // In Battle Pike
    else if (mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_PIKE_LOBBY)
             || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_PIKE_CORRIDOR)
             || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_PIKE_THREE_PATH_ROOM)
             || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_PIKE_ROOM_NORMAL)
             || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_PIKE_ROOM_FINAL)
             || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_PIKE_ROOM_WILD_MONS))
        return FRONTIER_FACILITY_PIKE + 1;

    // In Battle Pyramid
    else if (mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_PYRAMID_LOBBY)
        || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR)
        || mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_PYRAMID_TOP))
        return FRONTIER_FACILITY_PYRAMID + 1;

    else
        return 0;
}

static void InitFrontierMapSprites(void)
{
    struct SpriteTemplate sprite;
    u8 spriteId;
    u8 id;
    s16 x = 0, y;

    FreeAllSpritePalettes();
    LoadSpritePalettes(sSpritePalettes);

    LoadCompressedSpriteSheet(&sCursorSpriteSheets[0]);
    spriteId = CreateSprite(&sSpriteTemplates_Cursors[0], 155, (sMapData->cursorPos * 16) + 8, 2);
    sMapData->cursorSprite = &gSprites[spriteId];
    sMapData->cursorSprite->oam.priority = 0;
    sMapData->cursorSprite->hFlip = TRUE;
    StartSpriteAnim(sMapData->cursorSprite, 1);

    LoadCompressedSpriteSheet(&sCursorSpriteSheets[1]);
    spriteId = CreateSprite(&sSpriteTemplates_Cursors[1], sMapLandmarks[sMapData->cursorPos].x, sMapLandmarks[sMapData->cursorPos].y, 1);
    sMapData->mapIndicatorSprite = &gSprites[spriteId];
    sMapData->mapIndicatorSprite->oam.priority = 0;
    StartSpriteAnim(sMapData->mapIndicatorSprite, sMapLandmarks[sMapData->cursorPos].animNum);

    // Create player indicator head sprite only if it's in vicinity of battle frontier.
    id = GetCurrentRegionMapSectionId();
    if (id == MAPSEC_BATTLE_FRONTIER || id == MAPSEC_ARTISAN_CAVE)
    {
        s8 mapNum = gSaveBlock1Ptr->location.mapNum;

        if (mapNum == MAP_NUM(BATTLE_FRONTIER_OUTSIDE_WEST)
            || (mapNum == MAP_NUM(BATTLE_FRONTIER_OUTSIDE_EAST) && (x = 55)))
        {
            x += gSaveBlock1Ptr->pos.x;
            y = gSaveBlock1Ptr->pos.y;

            x /= 8;
            y /= 8;

            id = 0;
        }
        else
        {
            id = MapNumToFrontierFacilityId(mapNum);
            if (id != 0)
            {
                x = sMapLandmarks[id - 1].x;
                y = sMapLandmarks[id - 1].y;
            }
            else
            {
                // Handle Artisan Cave.
                if (gSaveBlock1Ptr->escapeWarp.mapNum == MAP_NUM(BATTLE_FRONTIER_OUTSIDE_EAST))
                    x = gSaveBlock1Ptr->escapeWarp.x + 55;
                else
                    x = gSaveBlock1Ptr->escapeWarp.x;

                y = gSaveBlock1Ptr->escapeWarp.y;

                x /= 8;
                y /= 8;
            }
        }

        LoadCompressedSpriteSheet(sHeadsSpriteSheet);
        sprite = sSpriteTemplate_PlayerHead;
        sprite.paletteTag = gSaveBlock2Ptr->playerGender + TAG_HEAD_MALE; // TAG_HEAD_FEMALE if gender is FEMALE
        if (id != 0)
        {
            spriteId = CreateSprite(&sprite, x, y, 0);
        }
        else
        {
            x *= 8;
            y *= 8;
            spriteId = CreateSprite(&sprite, x + 20, y + 36, 0);
        }

        sMapData->playerHeadSprite = &gSprites[spriteId];
        sMapData->playerHeadSprite->oam.priority = 0;
        if (gSaveBlock2Ptr->playerGender != MALE)
            StartSpriteAnim(sMapData->playerHeadSprite, 1);
    }
}

static void PrintOnFrontierMap(void)
{
    u8 i;

    for (i = 0; i < MAP_WINDOW_COUNT; i++)
    {
        PutWindowTilemap(i);
        FillWindowPixelBuffer(i, PIXEL_FILL(0));
    }

    for (i = 0; i < NUM_FRONTIER_FACILITIES; i++)
    {
        if (i == sMapData->cursorPos)
            AddTextPrinterParameterized3(MAP_WINDOW_NAME, FONT_NARROW, 4, (i * 16) + 1, sTextColors[2], 0, sMapLandmarks[i].name);
        else
            AddTextPrinterParameterized3(MAP_WINDOW_NAME, FONT_NARROW, 4, (i * 16) + 1, sTextColors[1], 0, sMapLandmarks[i].name);
    }

    AddTextPrinterParameterized3(MAP_WINDOW_DESCRIPTION, FONT_NORMAL, 4, 0, sTextColors[0], 0, sMapLandmarks[sMapData->cursorPos].description);

    for (i = 0; i < MAP_WINDOW_COUNT; i++)
        CopyWindowToVram(i, COPYWIN_FULL);

    CopyBgTilemapBufferToVram(0);
}

static void HandleFrontierMapCursorMove(u8 direction)
{
    u8 oldCursorPos, i;

    if (direction)
    {
        oldCursorPos = sMapData->cursorPos;
        sMapData->cursorPos = (oldCursorPos + 6) % NUM_FRONTIER_FACILITIES;
    }
    else
    {
        oldCursorPos = sMapData->cursorPos;
        sMapData->cursorPos = (oldCursorPos + 1) % NUM_FRONTIER_FACILITIES;
    }

    AddTextPrinterParameterized3(MAP_WINDOW_NAME, FONT_NARROW, 4, (oldCursorPos * 16) + 1, sTextColors[1], 0, sMapLandmarks[oldCursorPos].name);
    AddTextPrinterParameterized3(MAP_WINDOW_NAME, FONT_NARROW, 4, (sMapData->cursorPos * 16) + 1, sTextColors[2], 0, sMapLandmarks[sMapData->cursorPos].name);

    sMapData->cursorSprite->y = (sMapData->cursorPos * 16) + 8;

    StartSpriteAnim(sMapData->mapIndicatorSprite, sMapLandmarks[sMapData->cursorPos].animNum);
    sMapData->mapIndicatorSprite->x = sMapLandmarks[sMapData->cursorPos].x;
    sMapData->mapIndicatorSprite->y = sMapLandmarks[sMapData->cursorPos].y;
    FillWindowPixelBuffer(MAP_WINDOW_DESCRIPTION, PIXEL_FILL(0));
    AddTextPrinterParameterized3(MAP_WINDOW_DESCRIPTION, FONT_NORMAL, 4, 0, sTextColors[0], 0, sMapLandmarks[sMapData->cursorPos].description);

    for (i = 0; i < MAP_WINDOW_COUNT; i++)
        CopyWindowToVram(i, COPYWIN_FULL);

    CopyBgTilemapBufferToVram(0);
    PlaySE(SE_DEX_SCROLL);
}
