#include "global.h"
#include "malloc.h"
#include "battle.h"
#include "battle_gfx_sfx_util.h"
#include "bg.h"
#include "contest.h"
#include "contest_painting.h"
#include "data.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "image_processing_effects.h"
#include "international_string_util.h"
#include "main.h"
#include "lilycove_lady.h"
#include "palette.h"
#include "random.h"
#include "scanline_effect.h"
#include "string_util.h"
#include "strings.h"
#include "text.h"
#include "window.h"
#include "constants/rgb.h"

COMMON_DATA u16 (*gContestMonPixels)[][32] = {0};
COMMON_DATA struct ImageProcessingContext gImageProcessingContext = {0};
COMMON_DATA struct ContestWinner *gContestPaintingWinner = {0};
COMMON_DATA u16 *gContestPaintingMonPalette = NULL;

static u8 sHoldState;
static u16 sMosaicVal;
static u16 sFadeCounter;
static bool8 sVarsInitialized;
static u8 sWindowId;

static void ShowContestPainting(void);
static void HoldContestPainting(void);
static void InitContestPaintingWindow(void);
static void InitContestPaintingBg(void);
static void InitContestPaintingVars(bool8);
static void CreateContestPaintingPicture(u8, u8);
static void PrintContestPaintingCaption(u8, u8);
static void VBlankCB_ContestPainting(void);
static void _InitContestMonPixels(u8 *spriteGfx, u16 *palette, u16 (*destPixels)[64][64]);

extern const u8 gContestHallPaintingCaption[];
extern const u8 gContestCoolness[];
extern const u8 gContestBeauty[];
extern const u8 gContestCuteness[];
extern const u8 gContestSmartness[];
extern const u8 gContestToughness[];
extern const u8 gContestRankNormal[];
extern const u8 gContestRankSuper[];
extern const u8 gContestRankHyper[];
extern const u8 gContestRankMaster[];
extern const u8 gContestLink[];
extern const u8 gContestPaintingCool1[];
extern const u8 gContestPaintingCool2[];
extern const u8 gContestPaintingCool3[];
extern const u8 gContestPaintingBeauty1[];
extern const u8 gContestPaintingBeauty2[];
extern const u8 gContestPaintingBeauty3[];
extern const u8 gContestPaintingCute1[];
extern const u8 gContestPaintingCute2[];
extern const u8 gContestPaintingCute3[];
extern const u8 gContestPaintingSmart1[];
extern const u8 gContestPaintingSmart2[];
extern const u8 gContestPaintingSmart3[];
extern const u8 gContestPaintingTough1[];
extern const u8 gContestPaintingTough2[];
extern const u8 gContestPaintingTough3[];

static const u16 sPictureFramePalettes[]         = INCBIN_U16("graphics/picture_frame/bg.gbapal");
static const u8 sPictureFrameTiles_Cool[]        = INCBIN_U8("graphics/picture_frame/cool.4bpp.rl");
static const u8 sPictureFrameTiles_Beauty[]      = INCBIN_U8("graphics/picture_frame/beauty.4bpp.rl");
static const u8 sPictureFrameTiles_Cute[]        = INCBIN_U8("graphics/picture_frame/cute.4bpp.rl");
static const u8 sPictureFrameTiles_Smart[]       = INCBIN_U8("graphics/picture_frame/smart.4bpp.rl");
static const u8 sPictureFrameTiles_Tough[]       = INCBIN_U8("graphics/picture_frame/tough.4bpp.rl");
static const u8 sPictureFrameTiles_HallLobby[]   = INCBIN_U8("graphics/picture_frame/lobby.4bpp.rl");
static const u8 sPictureFrameTilemap_Cool[]      = INCBIN_U8("graphics/picture_frame/cool_map.bin.rl");
static const u8 sPictureFrameTilemap_Beauty[]    = INCBIN_U8("graphics/picture_frame/beauty_map.bin.rl");
static const u8 sPictureFrameTilemap_Cute[]      = INCBIN_U8("graphics/picture_frame/cute_map.bin.rl");
static const u8 sPictureFrameTilemap_Smart[]     = INCBIN_U8("graphics/picture_frame/smart_map.bin.rl");
static const u8 sPictureFrameTilemap_Tough[]     = INCBIN_U8("graphics/picture_frame/tough_map.bin.rl");
static const u8 sPictureFrameTilemap_HallLobby[] = INCBIN_U8("graphics/picture_frame/lobby_map.bin.rl");

static const u8 *const sContestCategoryNames_Unused[] =
{
    [CONTEST_CATEGORY_COOL]   = gContestCoolness,
    [CONTEST_CATEGORY_BEAUTY] = gContestBeauty,
    [CONTEST_CATEGORY_CUTE]   = gContestCuteness,
    [CONTEST_CATEGORY_SMART]  = gContestSmartness,
    [CONTEST_CATEGORY_TOUGH]  = gContestToughness,
};

static const u8 *const sContestRankNames[] =
{
    [CONTEST_RANK_NORMAL] = gContestRankNormal,
    [CONTEST_RANK_SUPER]  = gContestRankSuper,
    [CONTEST_RANK_HYPER]  = gContestRankHyper,
    [CONTEST_RANK_MASTER] = gContestRankMaster,
    [CONTEST_RANK_LINK]   = gContestLink,
};

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 10,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
};

static const struct WindowTemplate sWindowTemplate =
{
    .bg = 1,
    .tilemapLeft = 2,
    .tilemapTop = 14,
    .width = 26,
    .height = 4,
    .paletteNum = 15,
    .baseBlock = 1,
};

static const u8 *const sMuseumCaptions[NUM_PAINTING_CAPTIONS * CONTEST_CATEGORIES_COUNT] =
{
    [0 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_COOL]   = gContestPaintingCool1,
    [1 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_COOL]   = gContestPaintingCool2,
    [2 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_COOL]   = gContestPaintingCool3,
    [0 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_BEAUTY] = gContestPaintingBeauty1,
    [1 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_BEAUTY] = gContestPaintingBeauty2,
    [2 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_BEAUTY] = gContestPaintingBeauty3,
    [0 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_CUTE]   = gContestPaintingCute1,
    [1 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_CUTE]   = gContestPaintingCute2,
    [2 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_CUTE]   = gContestPaintingCute3,
    [0 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_SMART]  = gContestPaintingSmart1,
    [1 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_SMART]  = gContestPaintingSmart2,
    [2 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_SMART]  = gContestPaintingSmart3,
    [0 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_TOUGH]  = gContestPaintingTough1,
    [1 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_TOUGH]  = gContestPaintingTough2,
    [2 + NUM_PAINTING_CAPTIONS * CONTEST_CATEGORY_TOUGH]  = gContestPaintingTough3,
};

static const struct OamData sContestPaintingMonOamData =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = TRUE,
    .bpp = ST_OAM_8BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
};

static const u16 sBgPalette[] = {RGB_BLACK, RGB_BLACK};

void SetContestWinnerForPainting(int contestWinnerId)
{
    u8 *saveIdx = &gCurContestWinnerSaveIdx;
    u8 *isForArtist = &gCurContestWinnerIsForArtist;
    gCurContestWinner = gSaveBlock1Ptr->contestWinners[contestWinnerId - 1];
    *saveIdx = contestWinnerId - 1;
    *isForArtist = FALSE;
}

void CB2_ContestPainting(void)
{
    ShowContestPainting();
}

static void CB2_HoldContestPainting(void)
{
    HoldContestPainting();
    RunTextPrinters();
    UpdatePaletteFade();
}

static void CB2_QuitContestPainting(void)
{
    SetMainCallback2(gMain.savedCallback);
    FREE_AND_SET_NULL(gContestPaintingMonPalette);
    FREE_AND_SET_NULL(gContestMonPixels);
    RemoveWindow(sWindowId);
    Free(GetBgTilemapBuffer(1));
    FreeMonSpritesGfx();
}

static void ShowContestPainting(void)
{
    switch (gMain.state)
    {
    case 0:
        ScanlineEffect_Stop();
        SetVBlankCallback(NULL);
        AllocateMonSpritesGfx();
        gContestPaintingWinner = &gCurContestWinner;
        InitContestPaintingVars(TRUE);
        InitContestPaintingBg();
        gMain.state++;
        break;
    case 1:
        ResetPaletteFade();
        DmaFillLarge32(3, 0, (void *)VRAM, VRAM_SIZE, 0x1000);
        ResetSpriteData();
        gMain.state++;
        break;
    case 2:
        SeedRng(gMain.vblankCounter1);
        InitKeys();
        InitContestPaintingWindow();
        gMain.state++;
        break;
    case 3:
        CreateContestPaintingPicture(gCurContestWinnerSaveIdx, gCurContestWinnerIsForArtist);
        gMain.state++;
        break;
    case 4:
        PrintContestPaintingCaption(gCurContestWinnerSaveIdx, gCurContestWinnerIsForArtist);
        SetBackdropFromPalette(sBgPalette);
        DmaClear32(3, PLTT, PLTT_SIZE);
        BeginFastPaletteFade(2);
        SetVBlankCallback(VBlankCB_ContestPainting);
        sHoldState = 0;
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG0_ON | DISPCNT_BG1_ON | DISPCNT_OBJ_ON);
        SetMainCallback2(CB2_HoldContestPainting);
        break;
    }
}

static void HoldContestPainting(void)
{
    switch (sHoldState)
    {
    case 0:
        if (!gPaletteFade.active)
            sHoldState = 1;
        if (sVarsInitialized && sFadeCounter)
            sFadeCounter--;
        break;
    case 1:
        if ((JOY_NEW(A_BUTTON)) || (JOY_NEW(B_BUTTON)))
        {
            sHoldState++;
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        }

        if (sVarsInitialized)
            sFadeCounter = 0;
        break;
    case 2:
        if (!gPaletteFade.active)
            SetMainCallback2(CB2_QuitContestPainting);
        if (sVarsInitialized && sFadeCounter < 30)
            sFadeCounter++;
        break;
    }
}

static void InitContestPaintingWindow(void)
{
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
    SetBgTilemapBuffer(1, AllocZeroed(BG_SCREEN_SIZE));
    sWindowId = AddWindow(&sWindowTemplate);
    DeactivateAllTextPrinters();
    FillWindowPixelBuffer(sWindowId, PIXEL_FILL(0));
    PutWindowTilemap(sWindowId);
    CopyWindowToVram(sWindowId, COPYWIN_FULL);
    ShowBg(1);
}

static void PrintContestPaintingCaption(u8 contestType, bool8 isForArtist)
{
    int x;
    u8 category;

    // Artist's painting has no caption
    if (isForArtist == TRUE)
        return;

    category = gContestPaintingWinner->contestCategory;
    if (contestType < MUSEUM_CONTEST_WINNERS_START)
    {
        // Contest Hall caption
        BufferContestName(gStringVar1, category);
        StringAppend(gStringVar1, gText_Space);
        StringAppend(gStringVar1, sContestRankNames[gContestPaintingWinner->contestRank]);
        StringCopy(gStringVar2, gContestPaintingWinner->trainerName);
        ConvertInternationalContestantName(gStringVar2);
        StringCopy(gStringVar3, gContestPaintingWinner->monName);
        StringExpandPlaceholders(gStringVarBuffer, gContestHallPaintingCaption);
    }
    else
    {
        // Museum caption
        StringCopy(gStringVar1, gContestPaintingWinner->monName);
        StringExpandPlaceholders(gStringVarBuffer, sMuseumCaptions[category]);
    }

    x = GetStringCenterAlignXOffset(FONT_NORMAL, gStringVarBuffer, 208);
    AddTextPrinterParameterized(sWindowId, FONT_NORMAL, gStringVarBuffer, x, 1, 0, 0);
    CopyBgTilemapBufferToVram(1);
}

static void InitContestPaintingBg(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    REG_IE |= INTR_FLAG_VBLANK;
    SetGpuReg(REG_OFFSET_BG0CNT, BGCNT_PRIORITY(2) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(12) | BGCNT_MOSAIC | BGCNT_16COLOR | BGCNT_TXT256x256);
    SetGpuReg(REG_OFFSET_BG1CNT, BGCNT_PRIORITY(1) | BGCNT_CHARBASE(1) | BGCNT_SCREENBASE(10) | BGCNT_MOSAIC | BGCNT_16COLOR | BGCNT_TXT256x256);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
}

static void InitContestPaintingVars(bool8 reset)
{
    if (reset == FALSE)
    {
        // Never reached
        sVarsInitialized = FALSE;
        sMosaicVal = 0;
        sFadeCounter = 0;
    }
    else
    {
        sVarsInitialized = TRUE;
        sMosaicVal = 15;
        sFadeCounter = 30;
    }
}

static void UpdateContestPaintingMosaicEffect(void)
{
    if (!sVarsInitialized)
    {
        SetGpuReg(REG_OFFSET_MOSAIC, 0);
    }
    else
    {
        SetGpuReg(REG_OFFSET_BG1CNT, BGCNT_PRIORITY(1) | BGCNT_CHARBASE(1) | BGCNT_SCREENBASE(10) | BGCNT_MOSAIC | BGCNT_16COLOR | BGCNT_TXT256x256);
        sMosaicVal = sFadeCounter / 2;
        SetGpuReg(REG_OFFSET_MOSAIC, (sMosaicVal << 12) | (sMosaicVal << 8) | (sMosaicVal << 4) | (sMosaicVal << 0));
    }
}

static void VBlankCB_ContestPainting(void)
{
    UpdateContestPaintingMosaicEffect();
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void InitContestMonPixels(u16 species, bool8 backPic)
{
    const void *pal = GetMonSpritePalFromSpeciesAndPersonality(species, gContestPaintingWinner->trainerId, gContestPaintingWinner->personality);
    LZDecompressVram(pal, gContestPaintingMonPalette);
    if (!backPic)
    {
        HandleLoadSpecialPokePic_DontHandleDeoxys(
            &gMonFrontPicTable[species],
            gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_LEFT],
            species,
            gContestPaintingWinner->personality);
        _InitContestMonPixels(gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_LEFT], gContestPaintingMonPalette, (void *)gContestMonPixels);
    }
    else
    {
        HandleLoadSpecialPokePic_DontHandleDeoxys(
            &gMonBackPicTable[species],
            gMonSpritesGfxPtr->sprites.ptr[B_POSITION_PLAYER_LEFT],
            species,
            gContestPaintingWinner->personality);
        _InitContestMonPixels(gMonSpritesGfxPtr->sprites.ptr[B_POSITION_PLAYER_LEFT], gContestPaintingMonPalette, (void *)gContestMonPixels);
    }
}

static void _InitContestMonPixels(u8 *spriteGfx, u16 *palette, u16 (*destPixels)[64][64])
{
    u16 tileY, tileX, pixelY, pixelX;
    u8 colorIndex;

    for (tileY = 0; tileY < 8; tileY++)
    {
        for (tileX = 0; tileX < 8; tileX++)
        {
            for (pixelY = 0; pixelY < 8; pixelY++)
            {
                for (pixelX = 0; pixelX < 8; pixelX++)
                {
                    colorIndex = spriteGfx[32 * (tileY * 8 + tileX) + (pixelY << 2) + (pixelX >> 1)];
                    if (pixelX & 1)
                        colorIndex >>= 4;
                    else
                        colorIndex &= 0xF;

                    if (colorIndex == 0) // transparent pixel
                        (*destPixels)[8 * tileY + pixelY][tileX * 8 + pixelX] = 0x8000;
                    else
                        (*destPixels)[8 * tileY + pixelY][tileX * 8 + pixelX] = palette[colorIndex];
                }
            }
        }
    }
}

#define VRAM_PICTURE_DATA(x, y) (((u16 *)(BG_SCREEN_ADDR(12)))[(y) * 32 + (x)])

static void LoadContestPaintingFrame(u8 contestWinnerId, bool8 isForArtist)
{
    u8 x, y;

    LoadPalette(sPictureFramePalettes, BG_PLTT_ID(0), 8 * PLTT_SIZE_4BPP);
    if (isForArtist == TRUE)
    {
        // Load Artist's frame
        switch (gContestPaintingWinner->contestCategory / NUM_PAINTING_CAPTIONS)
        {
        case CONTEST_CATEGORY_COOL:
            RLUnCompVram(sPictureFrameTiles_Cool, (void *)VRAM);
            RLUnCompWram(sPictureFrameTilemap_Cool, gContestMonPixels);
            break;
        case CONTEST_CATEGORY_BEAUTY:
            RLUnCompVram(sPictureFrameTiles_Beauty, (void *)VRAM);
            RLUnCompWram(sPictureFrameTilemap_Beauty, gContestMonPixels);
            break;
        case CONTEST_CATEGORY_CUTE:
            RLUnCompVram(sPictureFrameTiles_Cute, (void *)VRAM);
            RLUnCompWram(sPictureFrameTilemap_Cute, gContestMonPixels);
            break;
        case CONTEST_CATEGORY_SMART:
            RLUnCompVram(sPictureFrameTiles_Smart, (void *)VRAM);
            RLUnCompWram(sPictureFrameTilemap_Smart, gContestMonPixels);
            break;
        case CONTEST_CATEGORY_TOUGH:
            RLUnCompVram(sPictureFrameTiles_Tough, (void *)VRAM);
            RLUnCompWram(sPictureFrameTilemap_Tough, gContestMonPixels);
            break;
        }

        // Set the background
        for (y = 0; y < 20; y++)
        {
            for (x = 0; x < 32; x++)
                VRAM_PICTURE_DATA(x, y) = 0x1015;
        }

        // Copy the image frame
        for (y = 0; y < 10; y++)
        {
            for (x = 0; x < 18; x++)
                VRAM_PICTURE_DATA(x + 6, y + 2) = (*gContestMonPixels)[y + 2][x + 6];
        }

        // Re-set the entire top row to the first top frame part
        for (x = 0; x < 16; x++)
            VRAM_PICTURE_DATA(x + 7, 2) = (*gContestMonPixels)[2][7];
    }
    else if (contestWinnerId < MUSEUM_CONTEST_WINNERS_START)
    {
        // Load Contest Hall lobby frame
        RLUnCompVram(sPictureFrameTiles_HallLobby, (void *)VRAM);
        RLUnCompVram(sPictureFrameTilemap_HallLobby, (void *)(BG_SCREEN_ADDR(12)));
    }
    else
    {
        // Load Museum frame
        switch (gContestPaintingWinner->contestCategory / NUM_PAINTING_CAPTIONS)
        {
        case CONTEST_CATEGORY_COOL:
            RLUnCompVram(sPictureFrameTiles_Cool, (void *)VRAM);
            RLUnCompVram(sPictureFrameTilemap_Cool, (void *)(BG_SCREEN_ADDR(12)));
            break;
        case CONTEST_CATEGORY_BEAUTY:
            RLUnCompVram(sPictureFrameTiles_Beauty, (void *)VRAM);
            RLUnCompVram(sPictureFrameTilemap_Beauty, (void *)(BG_SCREEN_ADDR(12)));
            break;
        case CONTEST_CATEGORY_CUTE:
            RLUnCompVram(sPictureFrameTiles_Cute, (void *)VRAM);
            RLUnCompVram(sPictureFrameTilemap_Cute, (void *)(BG_SCREEN_ADDR(12)));
            break;
        case CONTEST_CATEGORY_SMART:
            RLUnCompVram(sPictureFrameTiles_Smart, (void *)VRAM);
            RLUnCompVram(sPictureFrameTilemap_Smart, (void *)(BG_SCREEN_ADDR(12)));
            break;
        case CONTEST_CATEGORY_TOUGH:
            RLUnCompVram(sPictureFrameTiles_Tough, (void *)VRAM);
            RLUnCompVram(sPictureFrameTilemap_Tough, (void *)(BG_SCREEN_ADDR(12)));
            break;
        }
    }
}

#undef VRAM_PICTURE_DATA

static void InitPaintingMonOamData(u8 contestWinnerId)
{
    gMain.oamBuffer[0] = sContestPaintingMonOamData;
    gMain.oamBuffer[0].tileNum = 0;

    if (contestWinnerId > 1)
    {
        gMain.oamBuffer[0].x = 88;
        gMain.oamBuffer[0].y = 24;
    }
    else
    {
        gMain.oamBuffer[0].x = 88; // Duplicated code
        gMain.oamBuffer[0].y = 24;
    }
}

static u8 GetImageEffectForContestWinner(u8 contestWinnerId)
{
    u8 contestCategory;

    if (contestWinnerId < MUSEUM_CONTEST_WINNERS_START)
        contestCategory = gContestPaintingWinner->contestCategory;
    else
        contestCategory = gContestPaintingWinner->contestCategory / NUM_PAINTING_CAPTIONS;

    switch (contestCategory)
    {
    case CONTEST_CATEGORY_COOL:
        return IMAGE_EFFECT_OUTLINE_COLORED;
    case CONTEST_CATEGORY_BEAUTY:
        return IMAGE_EFFECT_SHIMMER;
    case CONTEST_CATEGORY_CUTE:
        return IMAGE_EFFECT_POINTILLISM;
    case CONTEST_CATEGORY_SMART:
        return IMAGE_EFFECT_CHARCOAL;
    case CONTEST_CATEGORY_TOUGH:
        return IMAGE_EFFECT_GRAYSCALE_LIGHT;
    }

    return contestCategory;
}

static void AllocPaintingResources(void)
{
    gContestPaintingMonPalette = AllocZeroed(OBJ_PLTT_SIZE);
    gContestMonPixels = AllocZeroed(0x2000);
}

static void DoContestPaintingImageProcessing(u8 imageEffect)
{
    gImageProcessingContext.canvasPixels = gContestMonPixels;
    gImageProcessingContext.canvasPalette = gContestPaintingMonPalette;
    gImageProcessingContext.paletteStart = 0;
    gImageProcessingContext.personality = gContestPaintingWinner->personality % 256;
    gImageProcessingContext.columnStart = 0;
    gImageProcessingContext.rowStart = 0;
    gImageProcessingContext.columnEnd = 64;
    gImageProcessingContext.rowEnd = 64;
    gImageProcessingContext.canvasWidth = 64;
    gImageProcessingContext.canvasHeight = 64;

    switch (imageEffect)
    {
    case IMAGE_EFFECT_CHARCOAL:
    case IMAGE_EFFECT_GRAYSCALE_LIGHT:
        gImageProcessingContext.quantizeEffect = QUANTIZE_EFFECT_GRAYSCALE;
        break;
    case IMAGE_EFFECT_OUTLINE_COLORED:
    case IMAGE_EFFECT_SHIMMER:
    case IMAGE_EFFECT_POINTILLISM:
    default:
        gImageProcessingContext.quantizeEffect = QUANTIZE_EFFECT_STANDARD_LIMITED_COLORS;
        break;
    }

    gImageProcessingContext.var_16 = 2;
    gImageProcessingContext.effect = imageEffect;
    gImageProcessingContext.dest = (void *)OBJ_VRAM0;

    ApplyImageProcessingEffects(&gImageProcessingContext);
    ApplyImageProcessingQuantization(&gImageProcessingContext);
    ConvertImageProcessingToGBA(&gImageProcessingContext);
    LoadPalette(gContestPaintingMonPalette, OBJ_PLTT_ID(0), 16 * PLTT_SIZE_4BPP);
}

static void CreateContestPaintingPicture(u8 contestWinnerId, bool8 isForArtist)
{
    AllocPaintingResources();
    InitContestMonPixels(gContestPaintingWinner->species, FALSE);
    DoContestPaintingImageProcessing(GetImageEffectForContestWinner(contestWinnerId));
    InitPaintingMonOamData(contestWinnerId);
    LoadContestPaintingFrame(contestWinnerId, isForArtist);
}

