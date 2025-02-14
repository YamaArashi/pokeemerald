#include "global.h"
#include "hall_of_fame.h"
#include "task.h"
#include "palette.h"
#include "sprite.h"
#include "pokemon.h"
#include "text.h"
#include "text_window.h"
#include "malloc.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "main.h"
#include "sound.h"
#include "constants/songs.h"
#include "decompress.h"
#include "save.h"
#include "strings.h"
#include "window.h"
#include "credits.h"
#include "bg.h"
#include "constants/game_stat.h"
#include "util.h"
#include "string_util.h"
#include "m4a.h"
#include "international_string_util.h"
#include "scanline_effect.h"
#include "trig.h"
#include "random.h"
#include "event_data.h"
#include "overworld.h"
#include "menu.h"
#include "fldeff_misc.h"
#include "trainer_pokemon_sprites.h"
#include "data.h"
#include "confetti_util.h"
#include "constants/rgb.h"

#define HALL_OF_FAME_MAX_TEAMS 50
#define TAG_CONFETTI 1001

struct HallofFameMon
{
    u32 tid;
    u32 personality;
    u16 species:9;
    u16 lvl:7;
    u8 nickname[POKEMON_NAME_LENGTH];
};

struct HallofFameTeam
{
    struct HallofFameMon mon[PARTY_SIZE];
};

STATIC_ASSERT(sizeof(struct HallofFameTeam) * HALL_OF_FAME_MAX_TEAMS <= SECTOR_DATA_SIZE * NUM_HOF_SECTORS, HallOfFameFreeSpace);

struct HofGfx
{
    u16 state;
    u8 field_2[16];
    u8 tilemap1[0x1000];
    u8 tilemap2[0x1000];
};

static EWRAM_DATA u32 sHofFadePalettes = 0;
static EWRAM_DATA struct HallofFameTeam *sHofMonPtr = NULL;
static EWRAM_DATA struct HofGfx *sHofGfxPtr = NULL;

static void ClearVramOamPltt_LoadHofPal(void);
static void LoadHofGfx(void);
static void InitHofBgs(void);
static bool8 CreateHofConfettiSprite(void);
static void StartCredits(void);
static bool8 LoadHofBgs(void);
static void Task_Hof_InitMonData(u8 taskId);
static void Task_Hof_InitTeamSaveData(u8 taskId);
static void Task_Hof_SetMonDisplayTask(u8 taskId);
static void Task_Hof_TrySaveData(u8 taskId);
static void Task_Hof_WaitToDisplayMon(u8 taskId);
static void Task_Hof_DisplayMon(u8 taskId);
static void Task_Hof_PrintMonInfoAfterAnimating(u8 taskId);
static void Task_Hof_TryDisplayAnotherMon(u8 taskId);
static void Task_Hof_PaletteFadeAndPrintWelcomeText(u8 taskId);
static void Task_Hof_DoConfetti(u8 taskId);
static void Task_Hof_WaitToDisplayPlayer(u8 taskId);
static void Task_Hof_DisplayPlayer(u8 taskId);
static void Task_Hof_WaitAndPrintPlayerInfo(u8 taskId);
static void Task_Hof_ExitOnKeyPressed(u8 taskId);
static void Task_Hof_HandlePaletteOnExit(u8 taskId);
static void Task_Hof_HandleExit(u8 taskId);
static void Task_HofPC_CopySaveData(u8 taskId);
static void Task_HofPC_PrintDataIsCorrupted(u8 taskId);
static void Task_HofPC_DrawSpritesPrintText(u8 taskId);
static void Task_HofPC_PrintMonInfo(u8 taskId);
static void Task_HofPC_HandleInput(u8 taskId);
static void Task_HofPC_HandlePaletteOnExit(u8 taskId);
static void Task_HofPC_HandleExit(u8 taskId);
static void Task_HofPC_ExitOnButtonPress(u8 taskId);
static void SpriteCB_GetOnScreenAndAnimate(struct Sprite *sprite);
static void HallOfFame_PrintMonInfo(struct HallofFameMon* currMon, u8 unused1, u8 unused2);
static void HallOfFame_PrintWelcomeText(u8 unusedPossiblyWindowId, u8 unused2);
static void HallOfFame_PrintPlayerInfo(u8 unused1, u8 unused2);
static void Task_DoDomeConfetti(u8 taskId);
static void SpriteCB_HofConfetti(struct Sprite *sprite);

static const struct BgTemplate sHof_BgTemplates[] =
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
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    },
};

static const struct WindowTemplate sHof_WindowTemplate = {
    .bg = 0,
    .tilemapLeft = 2,
    .tilemapTop = 2,
    .width = 14,
    .height = 6,
    .paletteNum = 14,
    .baseBlock = 1
};

static const u8 sMonInfoTextColors[4] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY};
static const u8 sPlayerInfoTextColors[4] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};
static const u8 sUnusedTextColors[4] = {TEXT_COLOR_RED, TEXT_COLOR_LIGHT_RED, TEXT_COLOR_TRANSPARENT};

static const struct CompressedSpriteSheet sSpriteSheet_Confetti[] =
{
    {.data = gConfetti_Gfx, .size = 0x220, .tag = TAG_CONFETTI},
    {},
};

static const struct CompressedSpritePalette sSpritePalette_Confetti[] =
{
    {.data = gConfetti_Pal, .tag = TAG_CONFETTI},
    {},
};

static const s16 sHallOfFame_MonFullTeamPositions[PARTY_SIZE][4] =
{
    {120,   210,    120,    40},
    {326,   220,    56,     40},
    {-86,   220,    184,    40},
    {120,   -62,    120,    88},
    {-70,   -92,    200,    88},
    {310,   -92,    40,     88}
};

static const s16 sHallOfFame_MonHalfTeamPositions[PARTY_SIZE / 2][4] =
{
    {120,   234,    120,    64},
    {326,   244,    56,     64},
    {-86,   244,    184,    64}
};

static const struct OamData sOamData_Confetti =
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
    .affineParam = 0,
};

static const union AnimCmd sAnim_PinkConfettiA[] =
{
    ANIMCMD_FRAME(0, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_RedConfettiA[] =
{
    ANIMCMD_FRAME(1, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_BlueConfettiA[] =
{
    ANIMCMD_FRAME(2, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_RedConfettiB[] =
{
    ANIMCMD_FRAME(3, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_BlueConfettiB[] =
{
    ANIMCMD_FRAME(4, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_YellowConfettiA[] =
{
    ANIMCMD_FRAME(5, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_WhiteConfettiA[] =
{
    ANIMCMD_FRAME(6, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_GreenConfettiA[] =
{
    ANIMCMD_FRAME(7, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PinkConfettiB[] =
{
    ANIMCMD_FRAME(8, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_BlueConfettiC[] =
{
    ANIMCMD_FRAME(9, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_YellowConfettiB[] =
{
    ANIMCMD_FRAME(10, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_WhiteConfettiB[] =
{
    ANIMCMD_FRAME(11, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_GreenConfettiB[] =
{
    ANIMCMD_FRAME(12, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_PinkConfettiC[] =
{
    ANIMCMD_FRAME(13, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_RedConfettiC[] =
{
    ANIMCMD_FRAME(14, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_YellowConfettiC[] =
{
    ANIMCMD_FRAME(15, 30),
    ANIMCMD_END
};

static const union AnimCmd sAnim_WhiteConfettiC[] =
{
    ANIMCMD_FRAME(16, 30),
    ANIMCMD_END
};

static const union AnimCmd * const sAnims_Confetti[] =
{
    sAnim_PinkConfettiA,
    sAnim_RedConfettiA,
    sAnim_BlueConfettiA,
    sAnim_RedConfettiB,
    sAnim_BlueConfettiB,
    sAnim_YellowConfettiA,
    sAnim_WhiteConfettiA,
    sAnim_GreenConfettiA,
    sAnim_PinkConfettiB,
    sAnim_BlueConfettiC,
    sAnim_YellowConfettiB,
    sAnim_WhiteConfettiB,
    sAnim_GreenConfettiB,
    sAnim_PinkConfettiC,
    sAnim_RedConfettiC,
    sAnim_YellowConfettiC,
    sAnim_WhiteConfettiC
};

static const struct SpriteTemplate sSpriteTemplate_HofConfetti =
{
    .tileTag = TAG_CONFETTI,
    .paletteTag = TAG_CONFETTI,
    .oam = &sOamData_Confetti,
    .anims = sAnims_Confetti,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_HofConfetti
};

static const u16 sHallOfFame_Pal[] = INCBIN_U16("graphics/misc/japanese_hof.gbapal");

static const u32 sHallOfFame_Gfx[] = INCBIN_U32("graphics/misc/japanese_hof.4bpp.lz");

static const struct HallofFameMon sDummyFameMon =
{
    .tid = 0x3EA03EA,
    .personality = 0,
    .species = SPECIES_NONE,
    .lvl = 0,
    .nickname = {0}
};

// Unused, order of party slots on Hall of Fame screen
static const u8 sHallOfFame_SlotOrder[] = {
    2, 1, 3,
    6, 4, 5,
};

// code
static void VBlankCB_HallOfFame(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_HallOfFame(void)
{
    RunTasks();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static bool8 InitHallOfFameScreen(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        ClearVramOamPltt_LoadHofPal();
        sHofGfxPtr = AllocZeroed(sizeof(*sHofGfxPtr));
        gMain.state = 1;
        break;
    case 1:
        LoadHofGfx();
        gMain.state++;
        break;
    case 2:
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_ALL);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(16, 7));
        SetGpuReg(REG_OFFSET_BLDY, 0);
        InitHofBgs();
        sHofGfxPtr->state = 0;
        gMain.state++;
        break;
    case 3:
        if (!LoadHofBgs())
        {
            SetVBlankCallback(VBlankCB_HallOfFame);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
            gMain.state++;
        }
        break;
    case 4:
        UpdatePaletteFade();
        if (!gPaletteFade.active)
        {
            SetMainCallback2(CB2_HallOfFame);
            PlayBGM(MUS_HALL_OF_FAME);
            return FALSE;
        }
        break;
    }
    return TRUE;
}

#define tDontSaveData       data[0]
#define tDisplayedMonId     data[1]
#define tMonNumber          data[2]
#define tFrameCount         data[3]
#define tPlayerSpriteID     data[4]
#define tMonSpriteId(i)     data[i + 5]

void CB2_DoHallOfFameScreen(void)
{
    if (!InitHallOfFameScreen())
    {
        u8 taskId = CreateTask(Task_Hof_InitMonData, 0);
        gTasks[taskId].tDontSaveData = FALSE;
        sHofMonPtr = AllocZeroed(sizeof(*sHofMonPtr));
    }
}

void CB2_DoHallOfFameScreenDontSaveData(void)
{
    if (!InitHallOfFameScreen())
    {
        u8 taskId = CreateTask(Task_Hof_InitMonData, 0);
        gTasks[taskId].tDontSaveData = TRUE;
        sHofMonPtr = AllocZeroed(sizeof(*sHofMonPtr));
    }
}

static void Task_Hof_InitMonData(u8 taskId)
{
    u16 i, j;

    gTasks[taskId].tMonNumber = 0; // valid pokes

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u8 nickname[POKEMON_NAME_LENGTH + 1];
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES))
        {
            sHofMonPtr->mon[i].species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG);
            sHofMonPtr->mon[i].tid = GetMonData(&gPlayerParty[i], MON_DATA_OT_ID);
            sHofMonPtr->mon[i].personality = GetMonData(&gPlayerParty[i], MON_DATA_PERSONALITY);
            sHofMonPtr->mon[i].lvl = GetMonData(&gPlayerParty[i], MON_DATA_LEVEL);
            GetMonData(&gPlayerParty[i], MON_DATA_NICKNAME, nickname);
            for (j = 0; j < POKEMON_NAME_LENGTH; j++)
                sHofMonPtr->mon[i].nickname[j] = nickname[j];
            gTasks[taskId].tMonNumber++;
        }
        else
        {
            sHofMonPtr->mon[i].species = SPECIES_NONE;
            sHofMonPtr->mon[i].tid = 0;
            sHofMonPtr->mon[i].personality = 0;
            sHofMonPtr->mon[i].lvl = 0;
            sHofMonPtr->mon[i].nickname[0] = EOS;
        }
    }

    sHofFadePalettes = 0;
    gTasks[taskId].tDisplayedMonId = 0;
    gTasks[taskId].tPlayerSpriteID = SPRITE_NONE;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        gTasks[taskId].tMonSpriteId(i) = SPRITE_NONE;
    }

    if (gTasks[taskId].tDontSaveData)
        gTasks[taskId].func = Task_Hof_SetMonDisplayTask;
    else
        gTasks[taskId].func = Task_Hof_InitTeamSaveData;
}

static void Task_Hof_InitTeamSaveData(u8 taskId)
{
    u16 i;
    struct HallofFameTeam *lastSavedTeam = (struct HallofFameTeam *)(gDecompressionBuffer);

    if (!gHasHallOfFameRecords)
    {
        memset(gDecompressionBuffer, 0, SECTOR_SIZE * NUM_HOF_SECTORS);
    }
    else
    {
        if (LoadGameSave(SAVE_HALL_OF_FAME) != SAVE_STATUS_OK)
            memset(gDecompressionBuffer, 0, SECTOR_SIZE * NUM_HOF_SECTORS);
    }

    for (i = 0; i < HALL_OF_FAME_MAX_TEAMS; i++, lastSavedTeam++)
    {
        if (lastSavedTeam->mon[0].species == SPECIES_NONE)
            break;
    }
    if (i >= HALL_OF_FAME_MAX_TEAMS)
    {
        struct HallofFameTeam *afterTeam = (struct HallofFameTeam *)(gDecompressionBuffer);
        struct HallofFameTeam *beforeTeam = (struct HallofFameTeam *)(gDecompressionBuffer);
        afterTeam++;
        for (i = 0; i < HALL_OF_FAME_MAX_TEAMS - 1; i++, beforeTeam++, afterTeam++)
        {
            *beforeTeam = *afterTeam;
        }
        lastSavedTeam--;
    }
    *lastSavedTeam = *sHofMonPtr;

    DrawDialogueFrame(0, FALSE);
    AddTextPrinterParameterized2(0, FONT_NORMAL, gText_SavingDontTurnOffPower, 0, NULL, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
    CopyWindowToVram(0, COPYWIN_FULL);
    gTasks[taskId].func = Task_Hof_TrySaveData;
}

static void Task_Hof_TrySaveData(u8 taskId)
{
    gGameContinueCallback = CB2_DoHallOfFameScreenDontSaveData;
    if (TrySavingData(SAVE_HALL_OF_FAME) == SAVE_STATUS_ERROR && gDamagedSaveSectors != 0)
    {
        UnsetBgTilemapBuffer(1);
        UnsetBgTilemapBuffer(3);
        FreeAllWindowBuffers();

        TRY_FREE_AND_SET_NULL(sHofGfxPtr);
        TRY_FREE_AND_SET_NULL(sHofMonPtr);

        DestroyTask(taskId);
    }
    else
    {
        PlaySE(SE_SAVE);
        gTasks[taskId].func = Task_Hof_WaitToDisplayMon;
        gTasks[taskId].tFrameCount = 32;
    }
}

static void Task_Hof_WaitToDisplayMon(u8 taskId)
{
    if (gTasks[taskId].tFrameCount)
        gTasks[taskId].tFrameCount--;
    else
        gTasks[taskId].func = Task_Hof_SetMonDisplayTask;
}

static void Task_Hof_SetMonDisplayTask(u8 taskId)
{
    gTasks[taskId].func = Task_Hof_DisplayMon;
}

#define tDestinationX  data[1]
#define tDestinationY  data[2]
#define tSpecies       data[7]

static void Task_Hof_DisplayMon(u8 taskId)
{
    u8 spriteId;
    s16 startX, startY, destX, destY;

    u16 currMonId = gTasks[taskId].tDisplayedMonId;
    struct HallofFameMon* currMon = &sHofMonPtr->mon[currMonId];

    if (gTasks[taskId].tMonNumber > PARTY_SIZE / 2)
    {
        startX = sHallOfFame_MonFullTeamPositions[currMonId][0];
        startY = sHallOfFame_MonFullTeamPositions[currMonId][1];
        destX = sHallOfFame_MonFullTeamPositions[currMonId][2];
        destY = sHallOfFame_MonFullTeamPositions[currMonId][3];
    }
    else
    {
        startX = sHallOfFame_MonHalfTeamPositions[currMonId][0];
        startY = sHallOfFame_MonHalfTeamPositions[currMonId][1];
        destX = sHallOfFame_MonHalfTeamPositions[currMonId][2];
        destY = sHallOfFame_MonHalfTeamPositions[currMonId][3];
    }

    if (currMon->species == SPECIES_EGG)
        destY += 10;

    spriteId = CreateMonPicSprite_Affine(currMon->species, currMon->tid, currMon->personality, MON_PIC_AFFINE_FRONT, startX, startY, currMonId, TAG_NONE);
    gSprites[spriteId].tDestinationX = destX;
    gSprites[spriteId].tDestinationY = destY;
    gSprites[spriteId].data[0] = 0;
    gSprites[spriteId].tSpecies = currMon->species;
    gSprites[spriteId].callback = SpriteCB_GetOnScreenAndAnimate;
    gTasks[taskId].tMonSpriteId(currMonId) = spriteId;
    ClearDialogWindowAndFrame(0, TRUE);
    gTasks[taskId].func = Task_Hof_PrintMonInfoAfterAnimating;
}

static void Task_Hof_PrintMonInfoAfterAnimating(u8 taskId)
{
    u16 currMonId = gTasks[taskId].tDisplayedMonId;
    struct HallofFameMon* currMon = &sHofMonPtr->mon[currMonId];
    struct Sprite *monSprite = &gSprites[gTasks[taskId].tMonSpriteId(currMonId)];

    if (monSprite->callback == SpriteCallbackDummy)
    {
        monSprite->oam.affineMode = ST_OAM_AFFINE_OFF;
        HallOfFame_PrintMonInfo(currMon, 0, 14);
        gTasks[taskId].tFrameCount = 120;
        gTasks[taskId].func = Task_Hof_TryDisplayAnotherMon;
    }
}

static void Task_Hof_TryDisplayAnotherMon(u8 taskId)
{
    u16 currPokeID = gTasks[taskId].tDisplayedMonId;
    struct HallofFameMon* currMon = &sHofMonPtr->mon[currPokeID];

    if (gTasks[taskId].tFrameCount != 0)
    {
        gTasks[taskId].tFrameCount--;
    }
    else
    {
        sHofFadePalettes |= (0x10000 << gSprites[gTasks[taskId].tMonSpriteId(currPokeID)].oam.paletteNum);
        if (gTasks[taskId].tDisplayedMonId < PARTY_SIZE - 1 && currMon[1].species != SPECIES_NONE) // there is another Pokémon to display
        {
            gTasks[taskId].tDisplayedMonId++;
            BeginNormalPaletteFade(sHofFadePalettes, 0, 12, 12, RGB(16, 29, 24));
            gSprites[gTasks[taskId].tMonSpriteId(currPokeID)].oam.priority = 1;
            gTasks[taskId].func = Task_Hof_DisplayMon;
        }
        else
        {
            gTasks[taskId].func = Task_Hof_PaletteFadeAndPrintWelcomeText;
        }
    }
}

static void Task_Hof_PaletteFadeAndPrintWelcomeText(u8 taskId)
{
    u16 i;

    BeginNormalPaletteFade(PALETTES_OBJECTS, 0, 0, 0, RGB_BLACK);
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (gTasks[taskId].tMonSpriteId(i) != SPRITE_NONE)
            gSprites[gTasks[taskId].tMonSpriteId(i)].oam.priority = 0;
    }

    HallOfFame_PrintWelcomeText(0, 15);
    PlaySE(SE_APPLAUSE);
    gTasks[taskId].tFrameCount = 400;
    gTasks[taskId].func = Task_Hof_DoConfetti;
}

static void Task_Hof_DoConfetti(u8 taskId)
{
    if (gTasks[taskId].tFrameCount != 0)
    {
        gTasks[taskId].tFrameCount--;

        // Create new confetti every 4th frame for the first 290 frames
        // For the last 110 frames wait for the existing confetti to fall offscreen
        if ((gTasks[taskId].tFrameCount & 3) == 0 && gTasks[taskId].tFrameCount > 110)
            CreateHofConfettiSprite();
    }
    else
    {
        u16 i;
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (gTasks[taskId].tMonSpriteId(i) != SPRITE_NONE)
                gSprites[gTasks[taskId].tMonSpriteId(i)].oam.priority = 1;
        }
        BeginNormalPaletteFade(sHofFadePalettes, 0, 12, 12, RGB(16, 29, 24));
        FillWindowPixelBuffer(0, PIXEL_FILL(0));
        CopyWindowToVram(0, COPYWIN_FULL);
        gTasks[taskId].tFrameCount = 7;
        gTasks[taskId].func = Task_Hof_WaitToDisplayPlayer;
    }
}

static void Task_Hof_WaitToDisplayPlayer(u8 taskId)
{
    if (gTasks[taskId].tFrameCount >= 16)
    {
        gTasks[taskId].func = Task_Hof_DisplayPlayer;
    }
    else
    {
        gTasks[taskId].tFrameCount++;
        SetGpuReg(REG_OFFSET_BLDALPHA, gTasks[taskId].tFrameCount * 256);
    }
}

static void Task_Hof_DisplayPlayer(u8 taskId)
{
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    ShowBg(0);
    ShowBg(1);
    ShowBg(3);
    gTasks[taskId].tPlayerSpriteID = CreateTrainerPicSprite(PlayerGenderToFrontTrainerPicId_Debug(gSaveBlock2Ptr->playerGender, TRUE), TRUE, 120, 72, 6, TAG_NONE);
    AddWindow(&sHof_WindowTemplate);
    LoadWindowGfx(1, gSaveBlock2Ptr->optionsWindowFrameType, 0x21D, BG_PLTT_ID(13));
    LoadPalette(GetTextWindowPalette(1), BG_PLTT_ID(14), PLTT_SIZE_4BPP);
    gTasks[taskId].tFrameCount = 120;
    gTasks[taskId].func = Task_Hof_WaitAndPrintPlayerInfo;
}

static void Task_Hof_WaitAndPrintPlayerInfo(u8 taskId)
{
    if (gTasks[taskId].tFrameCount != 0)
    {
        gTasks[taskId].tFrameCount--;
    }
    else if (gSprites[gTasks[taskId].tPlayerSpriteID].x != 192)
    {
        gSprites[gTasks[taskId].tPlayerSpriteID].x++;
    }
    else
    {
        FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 0x20, 0x20);
        HallOfFame_PrintPlayerInfo(1, 2);
        DrawDialogueFrame(0, FALSE);
        AddTextPrinterParameterized2(0, FONT_NORMAL, gText_LeagueChamp, 0, NULL, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
        CopyWindowToVram(0, COPYWIN_FULL);
        gTasks[taskId].func = Task_Hof_ExitOnKeyPressed;
    }
}

static void Task_Hof_ExitOnKeyPressed(u8 taskId)
{
    if (JOY_NEW(A_BUTTON))
    {
        FadeOutBGM(4);
        gTasks[taskId].func = Task_Hof_HandlePaletteOnExit;
    }
}

static void Task_Hof_HandlePaletteOnExit(u8 taskId)
{
    CpuCopy16(gPlttBufferFaded, gPlttBufferUnfaded, PLTT_SIZE);
    BeginNormalPaletteFade(PALETTES_ALL, 8, 0, 0x10, RGB_BLACK);
    gTasks[taskId].func = Task_Hof_HandleExit;
}

static void Task_Hof_HandleExit(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        s32 i;

        for (i = 0; i < PARTY_SIZE; i++)
        {
            u8 spriteId = gTasks[taskId].tMonSpriteId(i);
            if (spriteId != SPRITE_NONE)
            {
                FreeOamMatrix(gSprites[spriteId].oam.matrixNum);
                FreeAndDestroyMonPicSprite(spriteId);
            }
        }

        FreeAndDestroyTrainerPicSprite(gTasks[taskId].tPlayerSpriteID);
        HideBg(0);
        HideBg(1);
        HideBg(3);
        FreeAllWindowBuffers();
        UnsetBgTilemapBuffer(1);
        UnsetBgTilemapBuffer(3);
        ResetBgsAndClearDma3BusyFlags(0);
        DestroyTask(taskId);

        TRY_FREE_AND_SET_NULL(sHofGfxPtr);
        TRY_FREE_AND_SET_NULL(sHofMonPtr);

        StartCredits();
    }
}

static void StartCredits(void)
{
    SetMainCallback2(CB2_StartCreditsSequence);
}

#undef tDontSaveData
#undef tDisplayedPoke
#undef tMonNumber
#undef tFrameCount
#undef tPlayerSpriteID
#undef tMonSpriteId

#define tCurrTeamNo     data[0]
#define tCurrPageNo     data[1]
#define tCurrMonId      data[2]
#define tMonNo          data[4]
#define tMonSpriteId(i) data[i + 5]

void CB2_DoHallOfFamePC(void)
{
    switch (gMain.state)
    {
    case 0:
    default:
        SetVBlankCallback(NULL);
        ClearVramOamPltt_LoadHofPal();
        sHofGfxPtr = AllocZeroed(sizeof(*sHofGfxPtr));
        gMain.state = 1;
        break;
    case 1:
        LoadHofGfx();
        gMain.state++;
        break;
    case 2:
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        InitHofBgs();
        gMain.state++;
        break;
    case 3:
        if (!LoadHofBgs())
        {
            struct HallofFameTeam *fameTeam = (struct HallofFameTeam *)(gDecompressionBuffer);
            fameTeam->mon[0] = sDummyFameMon;
            ComputerScreenOpenEffect(0, 0, 0);
            SetVBlankCallback(VBlankCB_HallOfFame);
            gMain.state++;
        }
        break;
    case 4:
        RunTasks();
        AnimateSprites();
        BuildOamBuffer();
        UpdatePaletteFade();
        if (!IsComputerScreenOpenEffectActive())
            gMain.state++;
        break;
    case 5:
        {
            u8 taskId, i;

            SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_ALL);
            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(16, 7));
            SetGpuReg(REG_OFFSET_BLDY, 0);
            taskId = CreateTask(Task_HofPC_CopySaveData, 0);

            for (i = 0; i < PARTY_SIZE; i++)
            {
                gTasks[taskId].tMonSpriteId(i) = SPRITE_NONE;
            }

            sHofMonPtr = AllocZeroed(SECTOR_SIZE * NUM_HOF_SECTORS);
            SetMainCallback2(CB2_HallOfFame);
        }
        break;
    }
}

static void Task_HofPC_CopySaveData(u8 taskId)
{
    HofPCTopBar_AddWindow(0, 30, 0, 12, 0x226);
    if (LoadGameSave(SAVE_HALL_OF_FAME) != SAVE_STATUS_OK)
    {
        gTasks[taskId].func = Task_HofPC_PrintDataIsCorrupted;
    }
    else
    {
        u16 i;
        struct HallofFameTeam *savedTeams;

        CpuCopy16(gDecompressionBuffer, sHofMonPtr, SECTOR_SIZE * NUM_HOF_SECTORS);
        savedTeams = sHofMonPtr;
        for (i = 0; i < HALL_OF_FAME_MAX_TEAMS; i++, savedTeams++)
        {
            if (savedTeams->mon[0].species == SPECIES_NONE)
                break;
        }

        if (i < HALL_OF_FAME_MAX_TEAMS)
            gTasks[taskId].tCurrTeamNo = i - 1;
        else
            gTasks[taskId].tCurrTeamNo = HALL_OF_FAME_MAX_TEAMS - 1;

        gTasks[taskId].tCurrPageNo = GetGameStat(GAME_STAT_ENTERED_HOF);

        gTasks[taskId].func = Task_HofPC_DrawSpritesPrintText;
    }
}

static void Task_HofPC_DrawSpritesPrintText(u8 taskId)
{
    struct HallofFameTeam *savedTeams = sHofMonPtr;
    struct HallofFameMon* currMon;
    u16 i;

    for (i = 0; i < gTasks[taskId].tCurrTeamNo; i++)
        savedTeams++;

    currMon = &savedTeams->mon[0];
    sHofFadePalettes = 0;
    gTasks[taskId].tCurrMonId = 0;
    gTasks[taskId].tMonNo = 0;

    for (i = 0; i < PARTY_SIZE; i++, currMon++)
    {
        if (currMon->species != 0)
            gTasks[taskId].tMonNo++;
    }

    currMon = &savedTeams->mon[0];

    for (i = 0; i < PARTY_SIZE; i++, currMon++)
    {
        if (currMon->species != 0)
        {
            u16 spriteId;
            s16 posX, posY;

            if (gTasks[taskId].tMonNo > PARTY_SIZE / 2)
            {
                posX = sHallOfFame_MonFullTeamPositions[i][2];
                posY = sHallOfFame_MonFullTeamPositions[i][3];
            }
            else
            {
                posX = sHallOfFame_MonHalfTeamPositions[i][2];
                posY = sHallOfFame_MonHalfTeamPositions[i][3];
            }

            if (currMon->species == SPECIES_EGG)
                posY += 10;

            spriteId = CreateMonPicSprite_HandleDeoxys(currMon->species, currMon->tid, currMon->personality, TRUE, posX, posY, i, TAG_NONE);
            gSprites[spriteId].oam.priority = 1;
            gTasks[taskId].tMonSpriteId(i) = spriteId;
        }
        else
        {
            gTasks[taskId].tMonSpriteId(i) = SPRITE_NONE;
        }
    }

    BlendPalettes(PALETTES_OBJECTS, 0xC, RGB(16, 29, 24));

    ConvertIntToDecimalStringN(gStringVar1, gTasks[taskId].tCurrPageNo, STR_CONV_MODE_RIGHT_ALIGN, 3);
    StringExpandPlaceholders(gStringVarBuffer, gText_HOFNumber);

    if (gTasks[taskId].tCurrTeamNo <= 0)
        HofPCTopBar_PrintPair(gStringVarBuffer, gText_PickCancel, FALSE, 0, TRUE);
    else
        HofPCTopBar_PrintPair(gStringVarBuffer, gText_PickNextCancel, FALSE, 0, TRUE);

    gTasks[taskId].func = Task_HofPC_PrintMonInfo;
}

static void Task_HofPC_PrintMonInfo(u8 taskId)
{
    struct HallofFameTeam *savedTeams = sHofMonPtr;
    struct HallofFameMon* currMon;
    u16 i;
    u16 currMonID;

    for (i = 0; i < gTasks[taskId].tCurrTeamNo; i++)
        savedTeams++;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u16 spriteId = gTasks[taskId].tMonSpriteId(i);
        if (spriteId != SPRITE_NONE)
            gSprites[spriteId].oam.priority = 1;
    }

    currMonID = gTasks[taskId].tMonSpriteId(gTasks[taskId].tCurrMonId);
    gSprites[currMonID].oam.priority = 0;
    sHofFadePalettes = (0x10000 << gSprites[currMonID].oam.paletteNum) ^ PALETTES_OBJECTS;
    BlendPalettesUnfaded(sHofFadePalettes, 0xC, RGB(16, 29, 24));

    currMon = &savedTeams->mon[gTasks[taskId].tCurrMonId];
    if (currMon->species != SPECIES_EGG)
    {
        StopCryAndClearCrySongs();
        PlayCry_Normal(currMon->species, 0);
    }
    HallOfFame_PrintMonInfo(currMon, 0, 14);

    gTasks[taskId].func = Task_HofPC_HandleInput;
}

static void Task_HofPC_HandleInput(u8 taskId)
{
    u16 i;

    if (JOY_NEW(A_BUTTON))
    {
        if (gTasks[taskId].tCurrTeamNo != 0) // prepare another team to view
        {
            gTasks[taskId].tCurrTeamNo--;
            for (i = 0; i < PARTY_SIZE; i++)
            {
                u8 spriteId = gTasks[taskId].tMonSpriteId(i);
                if (spriteId != SPRITE_NONE)
                {
                    FreeAndDestroyMonPicSprite(spriteId);
                    gTasks[taskId].tMonSpriteId(i) = SPRITE_NONE;
                }
            }
            if (gTasks[taskId].tCurrPageNo != 0)
                gTasks[taskId].tCurrPageNo--;
            gTasks[taskId].func = Task_HofPC_DrawSpritesPrintText;
        }
        else // no more teams to view, turn off hall of fame PC
        {
            if (IsCryPlayingOrClearCrySongs())
            {
                StopCryAndClearCrySongs();
                m4aMPlayVolumeControl(&gMPlayInfo_BGM, TRACKS_ALL, 0x100);
            }
            gTasks[taskId].func = Task_HofPC_HandlePaletteOnExit;
        }
    }
    else if (JOY_NEW(B_BUTTON)) // turn off hall of fame PC
    {
        if (IsCryPlayingOrClearCrySongs())
        {
            StopCryAndClearCrySongs();
            m4aMPlayVolumeControl(&gMPlayInfo_BGM, TRACKS_ALL, 0x100);
        }
        gTasks[taskId].func = Task_HofPC_HandlePaletteOnExit;
    }
    else if (JOY_NEW(DPAD_UP) && gTasks[taskId].tCurrMonId != 0) // change mon -1
    {
        gTasks[taskId].tCurrMonId--;
        gTasks[taskId].func = Task_HofPC_PrintMonInfo;
    }
    else if (JOY_NEW(DPAD_DOWN) && gTasks[taskId].tCurrMonId < gTasks[taskId].tMonNo - 1) // change mon +1
    {
        gTasks[taskId].tCurrMonId++;
        gTasks[taskId].func = Task_HofPC_PrintMonInfo;
    }
}

static void Task_HofPC_HandlePaletteOnExit(u8 taskId)
{
    struct HallofFameTeam *fameTeam;

    CpuCopy16(gPlttBufferFaded, gPlttBufferUnfaded, PLTT_SIZE);
    fameTeam = (struct HallofFameTeam *)(gDecompressionBuffer);
    fameTeam->mon[0] = sDummyFameMon;
    ComputerScreenCloseEffect(0, 0, 0);
    gTasks[taskId].func = Task_HofPC_HandleExit;
}

static void Task_HofPC_HandleExit(u8 taskId)
{
    if (!IsComputerScreenCloseEffectActive())
    {
        u8 i;

        for (i = 0; i < PARTY_SIZE; i++)
        {
            u16 spriteId = gTasks[taskId].tMonSpriteId(i);
            if (spriteId != SPRITE_NONE)
            {
                FreeAndDestroyMonPicSprite(spriteId);
                gTasks[taskId].tMonSpriteId(i) = SPRITE_NONE;
            }
        }

        HideBg(0);
        HideBg(1);
        HideBg(3);
        HofPCTopBar_RemoveWindow();
        FreeAllWindowBuffers();
        UnsetBgTilemapBuffer(1);
        UnsetBgTilemapBuffer(3);
        ResetBgsAndClearDma3BusyFlags(0);
        DestroyTask(taskId);

        TRY_FREE_AND_SET_NULL(sHofGfxPtr);
        TRY_FREE_AND_SET_NULL(sHofMonPtr);

        ReturnFromHallOfFamePC();
    }
}

static void Task_HofPC_PrintDataIsCorrupted(u8 taskId)
{
    HofPCTopBar_Print(gText_AButtonExit, 8, TRUE);
    DrawDialogueFrame(0, FALSE);
    AddTextPrinterParameterized2(0, FONT_NORMAL, gText_HOFCorrupted, 0, NULL, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
    CopyWindowToVram(0, COPYWIN_FULL);
    gTasks[taskId].func = Task_HofPC_ExitOnButtonPress;
}

static void Task_HofPC_ExitOnButtonPress(u8 taskId)
{
    if (JOY_NEW(A_BUTTON))
        gTasks[taskId].func = Task_HofPC_HandlePaletteOnExit;
}

#undef tCurrTeamNo
#undef tCurrPageNo
#undef tCurrMonId
#undef tMonNo
#undef tMonSpriteId

static void HallOfFame_PrintWelcomeText(u8 unusedPossiblyWindowId, u8 unused2)
{
    FillWindowPixelBuffer(0, PIXEL_FILL(0));
    PutWindowTilemap(0);
    AddTextPrinterParameterized3(0, FONT_NORMAL, GetStringCenterAlignXOffset(FONT_NORMAL, gText_WelcomeToHOF, 0xD0), 1, sMonInfoTextColors, 0, gText_WelcomeToHOF);
    CopyWindowToVram(0, COPYWIN_FULL);
}

static void HallOfFame_PrintMonInfo(struct HallofFameMon* currMon, u8 unused1, u8 unused2)
{
    u8 text[max(32, POKEMON_NAME_LENGTH + 1)];
    u8 *stringPtr;
    s32 dexNumber;
    s32 width;

    FillWindowPixelBuffer(0, PIXEL_FILL(0));
    PutWindowTilemap(0);

    // dex number
    if (currMon->species != SPECIES_EGG)
    {
        stringPtr = StringCopy(text, gText_Number);
        dexNumber = SpeciesToPokedexNum(currMon->species);
        if (dexNumber != 0xFFFF)
        {
            stringPtr[0] = (dexNumber / 100) + CHAR_0;
            stringPtr++;
            dexNumber %= 100;
            stringPtr[0] = (dexNumber / 10) + CHAR_0;
            stringPtr++;
            stringPtr[0] = (dexNumber % 10) + CHAR_0;
            stringPtr++;
        }
        else
        {
            *(stringPtr)++ = CHAR_QUESTION_MARK;
            *(stringPtr)++ = CHAR_QUESTION_MARK;
            *(stringPtr)++ = CHAR_QUESTION_MARK;
        }
        stringPtr[0] = EOS;
        AddTextPrinterParameterized3(0, FONT_NORMAL, 0x10, 1, sMonInfoTextColors, TEXT_SKIP_DRAW, text);
    }

    // nickname, species names, gender and level
    memcpy(text, currMon->nickname, POKEMON_NAME_LENGTH);
    text[POKEMON_NAME_LENGTH] = EOS;
    if (currMon->species == SPECIES_EGG)
    {
        width = GetStringCenterAlignXOffset(FONT_NORMAL, text, 0xD0);
        AddTextPrinterParameterized3(0, FONT_NORMAL, width, 1, sMonInfoTextColors, TEXT_SKIP_DRAW, text);
        CopyWindowToVram(0, COPYWIN_FULL);
    }
    else
    {
        width = GetStringRightAlignXOffset(FONT_NORMAL, text, 0x80);
        AddTextPrinterParameterized3(0, FONT_NORMAL, width, 1, sMonInfoTextColors, TEXT_SKIP_DRAW, text);

        text[0] = CHAR_SLASH;
        stringPtr = StringCopy(text + 1, gSpeciesNames[currMon->species]);

        if (currMon->species != SPECIES_NIDORAN_M && currMon->species != SPECIES_NIDORAN_F)
        {
            switch (GetGenderFromSpeciesAndPersonality(currMon->species, currMon->personality))
            {
            case MON_MALE:
                stringPtr[0] = CHAR_MALE;
                stringPtr++;
                break;
            case MON_FEMALE:
                stringPtr[0] = CHAR_FEMALE;
                stringPtr++;
                break;
            }
        }

        stringPtr[0] = EOS;
        AddTextPrinterParameterized3(0, FONT_NORMAL, 0x80, 1, sMonInfoTextColors, TEXT_SKIP_DRAW, text);

        stringPtr = StringCopy(text, gText_Level);
        ConvertIntToDecimalStringN(stringPtr, currMon->lvl, STR_CONV_MODE_LEFT_ALIGN, 3);
        AddTextPrinterParameterized3(0, FONT_NORMAL, 0x24, 0x11, sMonInfoTextColors, TEXT_SKIP_DRAW, text);

        stringPtr = StringCopy(text, gText_IDNumber);
        ConvertIntToDecimalStringN(stringPtr, (u16)(currMon->tid), STR_CONV_MODE_LEADING_ZEROS, 5);
        AddTextPrinterParameterized3(0, FONT_NORMAL, 0x68, 0x11, sMonInfoTextColors, TEXT_SKIP_DRAW, text);

        CopyWindowToVram(0, COPYWIN_FULL);
    }
}

static void HallOfFame_PrintPlayerInfo(u8 unused1, u8 unused2)
{
    u8 text[20];
    u32 width;
    u16 trainerId;

    FillWindowPixelBuffer(1, PIXEL_FILL(1));
    PutWindowTilemap(1);
    DrawStdFrameWithCustomTileAndPalette(1, FALSE, 0x21D, 0xD);
    AddTextPrinterParameterized3(1, FONT_NORMAL, 0, 1, sPlayerInfoTextColors, TEXT_SKIP_DRAW, gText_Name);

    width = GetStringRightAlignXOffset(FONT_NORMAL, gSaveBlock2Ptr->playerName, 0x70);
    AddTextPrinterParameterized3(1, FONT_NORMAL, width, 1, sPlayerInfoTextColors, TEXT_SKIP_DRAW, gSaveBlock2Ptr->playerName);

    trainerId = (gSaveBlock2Ptr->playerTrainerId[0]) | (gSaveBlock2Ptr->playerTrainerId[1] << 8);
    AddTextPrinterParameterized3(1, FONT_NORMAL, 0, 0x11, sPlayerInfoTextColors, 0, gText_IDNumber);
    text[0] = (trainerId % 100000) / 10000 + CHAR_0;
    text[1] = (trainerId % 10000) / 1000 + CHAR_0;
    text[2] = (trainerId % 1000) / 100 + CHAR_0;
    text[3] = (trainerId % 100) / 10 + CHAR_0;
    text[4] = (trainerId % 10) / 1 + CHAR_0;
    text[5] = EOS;
    width = GetStringRightAlignXOffset(FONT_NORMAL, text, 0x70);
    AddTextPrinterParameterized3(1, FONT_NORMAL, width, 0x11, sPlayerInfoTextColors, TEXT_SKIP_DRAW, text);

    AddTextPrinterParameterized3(1, FONT_NORMAL, 0, 0x21, sPlayerInfoTextColors, TEXT_SKIP_DRAW, gText_Time);
    text[0] = (gSaveBlock2Ptr->playTimeHours / 100) + CHAR_0;
    text[1] = (gSaveBlock2Ptr->playTimeHours % 100) / 10 + CHAR_0;
    text[2] = (gSaveBlock2Ptr->playTimeHours % 10) + CHAR_0;

    if (text[0] == CHAR_0)
        text[0] = CHAR_SPACE;
    if (text[0] == CHAR_SPACE && text[1] == CHAR_0)
        text[8] = CHAR_SPACE;

    text[3] = CHAR_COLON;
    text[4] = (gSaveBlock2Ptr->playTimeMinutes % 100) / 10 + CHAR_0;
    text[5] = (gSaveBlock2Ptr->playTimeMinutes % 10) + CHAR_0;
    text[6] = EOS;

    width = GetStringRightAlignXOffset(FONT_NORMAL, text, 0x70);
    AddTextPrinterParameterized3(1, FONT_NORMAL, width, 0x21, sPlayerInfoTextColors, TEXT_SKIP_DRAW, text);

    CopyWindowToVram(1, COPYWIN_FULL);
}

static void ClearVramOamPltt_LoadHofPal(void)
{
    u32 vramOffset, oamOffset, plttOffset;
    u32 vramSize, oamSize, plttSize;

    vramOffset = (VRAM);
    vramSize = VRAM_SIZE;
    while (TRUE)
    {
        DmaFill16(3, 0, vramOffset, 0x1000);
        vramOffset += 0x1000;
        vramSize -= 0x1000;
        if (vramSize <= 0x1000)
        {
            DmaFill16(3, 0, vramOffset, vramSize);
            break;
        }
    }

    oamOffset = OAM;
    oamSize = OAM_SIZE;
    DmaFill32(3, 0, oamOffset, oamSize);

    plttOffset = PLTT;
    plttSize = PLTT_SIZE;
    DmaFill16(3, 0, plttOffset, plttSize);

    ResetPaletteFade();
    LoadPalette(sHallOfFame_Pal, BG_PLTT_ID(0), sizeof(sHallOfFame_Pal));
}

static void LoadHofGfx(void)
{
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    ResetTempTileDataBuffers();
    ResetAllPicSprites();
    FreeAllSpritePalettes();
    gReservedSpritePaletteCount = 8;
    LoadCompressedSpriteSheet(sSpriteSheet_Confetti);
    LoadCompressedSpritePalette(sSpritePalette_Confetti);
}

static void InitHofBgs(void)
{
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sHof_BgTemplates, ARRAY_COUNT(sHof_BgTemplates));
    SetBgTilemapBuffer(1, sHofGfxPtr->tilemap1);
    SetBgTilemapBuffer(3, sHofGfxPtr->tilemap2);
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
    ChangeBgX(3, 0, BG_COORD_SET);
    ChangeBgY(3, 0, BG_COORD_SET);
}

static bool8 LoadHofBgs(void)
{
    switch (sHofGfxPtr->state)
    {
    case 0:
        DecompressAndCopyTileDataToVram(1, sHallOfFame_Gfx, 0, 0, 0);
        break;
    case 1:
        if (FreeTempTileDataBuffersIfPossible())
            return TRUE;
        break;
    case 2:
        FillBgTilemapBufferRect_Palette0(1, 1, 0, 0, 0x20, 2);
        FillBgTilemapBufferRect_Palette0(1, 0, 0, 3, 0x20, 0xB);
        FillBgTilemapBufferRect_Palette0(1, 1, 0, 0xE, 0x20, 6);
        FillBgTilemapBufferRect_Palette0(3, 2, 0, 0, 0x20, 0x20);

        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(3);
        break;
    case 3:
        InitStandardTextBoxWindows();
        InitTextBoxGfxAndPrinters();
        break;
    case 4:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
        ShowBg(0);
        ShowBg(1);
        ShowBg(3);
        sHofGfxPtr->state = 0;
        return FALSE;
    }

    sHofGfxPtr->state++;
    return TRUE;
}

static void SpriteCB_GetOnScreenAndAnimate(struct Sprite *sprite)
{
    if (sprite->x != sprite->tDestinationX
        || sprite->y != sprite->tDestinationY)
    {
        if (sprite->x < sprite->tDestinationX)
            sprite->x += 15;
        if (sprite->x > sprite->tDestinationX)
            sprite->x -= 15;

        if (sprite->y < sprite->tDestinationY)
            sprite->y += 10;
        if (sprite->y > sprite->tDestinationY)
            sprite->y -= 10;
    }
    else
    {
        s16 species = sprite->tSpecies;

        if (species == SPECIES_EGG)
            DoMonFrontSpriteAnimation(sprite, species, TRUE, 3);
        else
            DoMonFrontSpriteAnimation(sprite, species, FALSE, 3);
    }
}

#undef tDestinationX
#undef tDestinationY
#undef tSpecies

#define sSineIdx data[0]
#define sExtraY  data[1]

static void SpriteCB_HofConfetti(struct Sprite *sprite)
{
    if (sprite->y2 > 120)
    {
        DestroySprite(sprite);
    }
    else
    {
        u16 rand;
        u8 sineIdx;

        sprite->y2++;
        sprite->y2 += sprite->sExtraY;

        sineIdx = sprite->sSineIdx;
        rand = (Random() % 4) + 8;
        sprite->x2 = rand * gSineTable[sineIdx] / 256;

        sprite->sSineIdx += 4;
    }
}

static bool8 CreateHofConfettiSprite(void)
{
    u8 spriteID;
    struct Sprite *sprite;

    s16 posX = Random() % DISPLAY_WIDTH;
    s16 posY = -(Random() % 8);

    spriteID = CreateSprite(&sSpriteTemplate_HofConfetti, posX, posY, 0);
    sprite = &gSprites[spriteID];

    StartSpriteAnim(sprite, Random() % ARRAY_COUNT(sAnims_Confetti));

    // 1/4 confetti sprites move an extra Y coord each frame
    if (Random() & 3)
        sprite->sExtraY = 0;
    else
        sprite->sExtraY = 1;

    return FALSE;
}

// The below confetti functions are used when a Battle Dome tourney is won
// For the Hall of Fame confetti see Task_Hof_DoConfetti
// The end result is essentially the same, just a very different way of handling it
#define tState         data[0]
#define tTimer         data[1]
#define tConfettiCount data[15]

// Indexes into the data array of the struct ConfettiUtil
#define CONFETTI_SINE_IDX 0
#define CONFETTI_EXTRA_Y  1
#define CONFETTI_TASK_ID  7

void DoDomeConfetti(void)
{
    u8 taskId;

    gSpecialVar_0x8004 = 180;
    taskId = CreateTask(Task_DoDomeConfetti, 0);
    if (taskId != TASK_NONE)
    {
        gTasks[taskId].tTimer = gSpecialVar_0x8004;
        gSpecialVar_0x8005 = taskId;
    }
}

static void StopDomeConfetti(void)
{
    u8 taskId;

    if ((taskId = FindTaskIdByFunc(Task_DoDomeConfetti)) != TASK_NONE)
        DestroyTask(taskId);

    ConfettiUtil_Free();
    FreeSpriteTilesByTag(TAG_CONFETTI);
    FreeSpritePaletteByTag(TAG_CONFETTI);
}

static void UpdateDomeConfetti(struct ConfettiUtil *util)
{
    if (util->yDelta > 110)
    {
        // Destroy confetti after it falls far enough
        gTasks[util->data[CONFETTI_TASK_ID]].tConfettiCount--;
        ConfettiUtil_Remove(util->id);
    }
    else
    {
        // Move confetti down
        u8 sineIdx;
        s32 rand;

        util->yDelta++;
        util->yDelta += util->data[CONFETTI_EXTRA_Y];

        sineIdx = util->data[CONFETTI_SINE_IDX];
        rand = Random();
        rand &= 3;
        rand += 8;
        util->xDelta = (rand) * ((gSineTable[sineIdx])) / 256;

        util->data[CONFETTI_SINE_IDX] += 4;
    }
}

static void Task_DoDomeConfetti(u8 taskId)
{
    u32 id = 0;
    u16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        if (!ConfettiUtil_Init(64))
        {
            // Init failed
            DestroyTask(taskId);
            gSpecialVar_0x8004 = 0;
            gSpecialVar_0x8005 = 0xFFFF;
        }
        LoadCompressedSpriteSheet(sSpriteSheet_Confetti);
        LoadCompressedSpritePalette(sSpritePalette_Confetti);
        tState++;
        break;
    case 1:
        if (tTimer != 0 && tTimer % 3 == 0)
        {
            // Create new confetti every 3 frames
            id = ConfettiUtil_AddNew(&sOamData_Confetti,
                              TAG_CONFETTI,
                              TAG_CONFETTI,
                              Random() % DISPLAY_WIDTH,
                              -(Random() % 8),
                              Random() % ARRAY_COUNT(sAnims_Confetti),
                              id);
            if (id != 0xFF)
            {
                ConfettiUtil_SetCallback(id, UpdateDomeConfetti);

                // 1/4 of the confetti move an extra y coord every frame
                if ((Random() % 4) == 0)
                    ConfettiUtil_SetData(id, CONFETTI_EXTRA_Y, 1);

                ConfettiUtil_SetData(id, CONFETTI_TASK_ID, taskId);
                tConfettiCount++;
            }
        }

        ConfettiUtil_Update();
        if (tTimer != 0)
            tTimer--;
        else if (tConfettiCount == 0)
            tState = 0xFF;
        break;
    case 0xFF:
        StopDomeConfetti();
        gSpecialVar_0x8004 = 0;
        gSpecialVar_0x8005 = 0xFFFF;
        break;
    }
}
