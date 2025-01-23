#include "global.h"
#include "pokemon.h"
#include "egg_hatch.h"
#include "pokedex.h"
#include "constants/items.h"
#include "script.h"
#include "decompress.h"
#include "task.h"
#include "palette.h"
#include "main.h"
#include "event_data.h"
#include "sound.h"
#include "constants/songs.h"
#include "text.h"
#include "text_window.h"
#include "string_util.h"
#include "menu.h"
#include "trig.h"
#include "random.h"
#include "malloc.h"
#include "dma3.h"
#include "gpu_regs.h"
#include "bg.h"
#include "m4a.h"
#include "window.h"
#include "graphics.h"
#include "constants/abilities.h"
#include "daycare.h"
#include "overworld.h"
#include "scanline_effect.h"
#include "field_weather.h"
#include "international_string_util.h"
#include "naming_screen.h"
#include "pokemon_storage_system.h"
#include "field_screen_effect.h"
#include "trade.h"
#include "data.h"
#include "battle.h" // to get rid of later
#include "constants/rgb.h"

#define GFXTAG_EGG       12345
#define GFXTAG_EGG_SHARD 23456

#define PALTAG_EGG       54321

#define EGG_X (DISPLAY_WIDTH / 2)
#define EGG_Y (DISPLAY_HEIGHT / 2 - 5)

struct EggHatchData
{
    u8 eggSpriteId;
    u8 monSpriteId;
    u8 state;
    u8 delayTimer;
    u8 eggPartyId;
    u8 unused_5;
    u8 unused_6;
    u8 eggShardVelocityId;
    u8 windowId;
    u8 unused_9;
    u8 unused_A;
    u16 species;
    u8 textColor[3];
};

extern const u8 gText_HatchedFromEgg[];
extern const u8 gText_NicknameHatchPrompt[];

static void Task_EggHatch(u8);
static void CB2_LoadEggHatch(void);
static void CB2_EggHatch(void);
static void SpriteCB_Egg_Shake1(struct Sprite *);
static void SpriteCB_Egg_Shake2(struct Sprite *);
static void SpriteCB_Egg_Shake3(struct Sprite *);
static void SpriteCB_Egg_WaitHatch(struct Sprite *);
static void SpriteCB_Egg_Hatch(struct Sprite *);
static void SpriteCB_Egg_Reveal(struct Sprite *);
static void SpriteCB_EggShard(struct Sprite *);
static void EggHatchPrintMessage(u8, u8 *, u8, u8, u8);
static void CreateRandomEggShardSprite(void);
static void CreateEggShardSprite(u8, u8, s16, s16, s16, u8);

static struct EggHatchData *sEggHatchData;

static const u16 sEggPalette[]  = INCBIN_U16("graphics/pokemon/egg/normal.gbapal");
static const u8 sEggHatchTiles[] = INCBIN_U8("graphics/pokemon/egg/hatch.4bpp");
static const u8 sEggShardTiles[] = INCBIN_U8("graphics/pokemon/egg/shard.4bpp");

static const struct OamData sOamData_Egg =
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

static const union AnimCmd sSpriteAnim_Egg_Normal[] =
{
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Egg_Cracked1[] =
{
    ANIMCMD_FRAME(16, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Egg_Cracked2[] =
{
    ANIMCMD_FRAME(32, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Egg_Cracked3[] =
{
    ANIMCMD_FRAME(48, 5),
    ANIMCMD_END
};

enum {
    EGG_ANIM_NORMAL,
    EGG_ANIM_CRACKED_1,
    EGG_ANIM_CRACKED_2,
    EGG_ANIM_CRACKED_3,
};

static const union AnimCmd *const sSpriteAnimTable_Egg[] =
{
    [EGG_ANIM_NORMAL]    = sSpriteAnim_Egg_Normal,
    [EGG_ANIM_CRACKED_1] = sSpriteAnim_Egg_Cracked1,
    [EGG_ANIM_CRACKED_2] = sSpriteAnim_Egg_Cracked2,
    [EGG_ANIM_CRACKED_3] = sSpriteAnim_Egg_Cracked3,
};

static const struct SpriteSheet sEggHatch_Sheet =
{
    .data = sEggHatchTiles,
    .size = sizeof(sEggHatchTiles),
    .tag = GFXTAG_EGG,
};

static const struct SpriteSheet sEggShards_Sheet =
{
    .data = sEggShardTiles,
    .size = sizeof(sEggShardTiles),
    .tag = GFXTAG_EGG_SHARD,
};

static const struct SpritePalette sEgg_SpritePalette =
{
    .data = sEggPalette,
    .tag = PALTAG_EGG
};

static const struct SpriteTemplate sSpriteTemplate_Egg =
{
    .tileTag = GFXTAG_EGG,
    .paletteTag = PALTAG_EGG,
    .oam = &sOamData_Egg,
    .anims = sSpriteAnimTable_Egg,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct OamData sOamData_EggShard =
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
    .priority = 2,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sSpriteAnim_EggShard0[] =
{
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_EggShard1[] =
{
    ANIMCMD_FRAME(1, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_EggShard2[] =
{
    ANIMCMD_FRAME(2, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_EggShard3[] =
{
    ANIMCMD_FRAME(3, 5),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_EggShard[] =
{
    sSpriteAnim_EggShard0,
    sSpriteAnim_EggShard1,
    sSpriteAnim_EggShard2,
    sSpriteAnim_EggShard3,
};

static const struct SpriteTemplate sSpriteTemplate_EggShard =
{
    .tileTag = GFXTAG_EGG_SHARD,
    .paletteTag = PALTAG_EGG,
    .oam = &sOamData_EggShard,
    .anims = sSpriteAnimTable_EggShard,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_EggShard
};

static const struct BgTemplate sBgTemplates_EggHatch[] =
{
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 24,
        .screenSize = 3,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },

    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 8,
        .screenSize = 1,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
};

static const struct WindowTemplate sWinTemplates_EggHatch[] =
{
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 26,
        .height = 4,
        .paletteNum = 0,
        .baseBlock = 64
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sYesNoWinTemplate =
{
    .bg = 0,
    .tilemapLeft = 21,
    .tilemapTop = 9,
    .width = 5,
    .height = 4,
    .paletteNum = 15,
    .baseBlock = 424
};

static const s16 sEggShardVelocities[][2] =
{
    // First shake
    {Q_8_8(-1.5),       Q_8_8(-3.75)},

    // Third shake
    {Q_8_8(-5),         Q_8_8(-3)},
    {Q_8_8(3.5),        Q_8_8(-3)},

    // Hatching
    {Q_8_8(-4),         Q_8_8(-3.75)},
    {Q_8_8(2),          Q_8_8(-1.5)},
    {Q_8_8(-0.5),       Q_8_8(-6.75)},
    {Q_8_8(5),          Q_8_8(-2.25)},
    {Q_8_8(-1.5),       Q_8_8(-3.75)},
    {Q_8_8(4.5),        Q_8_8(-1.5)},
    {Q_8_8(-1),         Q_8_8(-6.75)},
    {Q_8_8(4),          Q_8_8(-2.25)},
    {Q_8_8(-3.5),       Q_8_8(-3.75)},
    {Q_8_8(1),          Q_8_8(-1.5)},
    {Q_8_8(-3.515625),  Q_8_8(-6.75)},
    {Q_8_8(4.5),        Q_8_8(-2.25)},
    {Q_8_8(-0.5),       Q_8_8(-7.5)},
    {Q_8_8(1),          Q_8_8(-4.5)},
    {Q_8_8(-2.5),       Q_8_8(-2.25)},
    {Q_8_8(2.5),        Q_8_8(-7.5)},
};

static void CreateHatchedMon(struct Pokemon *egg, struct Pokemon *temp)
{
    u16 species;
    u32 personality, pokerus;
    u8 i, friendship, language, gameMet, markings, isModernFatefulEncounter;
    u16 moves[MAX_MON_MOVES];
    u32 ivs[NUM_STATS];

    species = GetMonData(egg, MON_DATA_SPECIES);

    for (i = 0; i < MAX_MON_MOVES; i++)
        moves[i] = GetMonData(egg, MON_DATA_MOVE1 + i);

    personality = GetMonData(egg, MON_DATA_PERSONALITY);

    for (i = 0; i < NUM_STATS; i++)
        ivs[i] = GetMonData(egg, MON_DATA_HP_IV + i);

    // The language is initially read from the Egg but is later overwritten below
    language = GetMonData(egg, MON_DATA_LANGUAGE);
    gameMet = GetMonData(egg, MON_DATA_MET_GAME);
    markings = GetMonData(egg, MON_DATA_MARKINGS);
    pokerus = GetMonData(egg, MON_DATA_POKERUS);
    isModernFatefulEncounter = GetMonData(egg, MON_DATA_MODERN_FATEFUL_ENCOUNTER);

    CreateMon(temp, species, EGG_HATCH_LEVEL, USE_RANDOM_IVS, TRUE, personality, OT_ID_PLAYER_ID, 0);

    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonData(temp, MON_DATA_MOVE1 + i,  &moves[i]);

    for (i = 0; i < NUM_STATS; i++)
        SetMonData(temp, MON_DATA_HP_IV + i,  &ivs[i]);

    language = GAME_LANGUAGE;
    SetMonData(temp, MON_DATA_LANGUAGE, &language);
    SetMonData(temp, MON_DATA_MET_GAME, &gameMet);
    SetMonData(temp, MON_DATA_MARKINGS, &markings);

    friendship = 120;
    SetMonData(temp, MON_DATA_FRIENDSHIP, &friendship);
    SetMonData(temp, MON_DATA_POKERUS, &pokerus);
    SetMonData(temp, MON_DATA_MODERN_FATEFUL_ENCOUNTER, &isModernFatefulEncounter);

    *egg = *temp;
}

static void AddHatchedMonToParty(u8 id)
{
    u8 isEgg = 0x46; // ?
    u16 species;
    u8 name[POKEMON_NAME_LENGTH + 1];
    u16 ball;
    u16 metLevel;
    u8 metLocation;
    struct Pokemon *mon = &gPlayerParty[id];

    CreateHatchedMon(mon, &gEnemyParty[0]);
    SetMonData(mon, MON_DATA_IS_EGG, &isEgg);

    species = GetMonData(mon, MON_DATA_SPECIES);
    GetSpeciesName(name, species);
    SetMonData(mon, MON_DATA_NICKNAME, name);

    species = SpeciesToNationalPokedexNum(species);
    GetSetPokedexFlag(species, FLAG_SET_SEEN);
    GetSetPokedexFlag(species, FLAG_SET_CAUGHT);

    GetMonNickname2(mon, gStringVar1);

    ball = ITEM_POKE_BALL;
    SetMonData(mon, MON_DATA_POKEBALL, &ball);

    // A met level of 0 is interpreted on the summary screen as "hatched at"
    metLevel = 0;
    SetMonData(mon, MON_DATA_MET_LEVEL, &metLevel);

    metLocation = GetCurrentRegionMapSectionId();
    SetMonData(mon, MON_DATA_MET_LOCATION, &metLocation);

    MonRestorePP(mon);
    CalculateMonStats(mon);
}

void ScriptHatchMon(void)
{
    AddHatchedMonToParty(gSpecialVar_0x8004);
}

static bool8 _CheckDaycareMonReceivedMail(struct DayCare *daycare, u8 daycareId)
{
    u8 nickname[max(32, POKEMON_NAME_BUFFER_SIZE)];
    struct DaycareMon *daycareMon = &daycare->mons[daycareId];

    GetBoxMonNickname(&daycareMon->mon, nickname);
    if (daycareMon->mail.message.itemId != ITEM_NONE
        && (StringCompareWithoutExtCtrlCodes(nickname, daycareMon->mail.monName) != 0
         || StringCompareWithoutExtCtrlCodes(gSaveBlock2Ptr->playerName, daycareMon->mail.otName) != 0))
    {
        StringCopy(gStringVar1, nickname);
        TVShowConvertInternationalString(gStringVar2, daycareMon->mail.otName, daycareMon->mail.gameLanguage);
        TVShowConvertInternationalString(gStringVar3, daycareMon->mail.monName, daycareMon->mail.monLanguage);
        return TRUE;
    }
    return FALSE;
}

bool8 CheckDaycareMonReceivedMail(void)
{
    return _CheckDaycareMonReceivedMail(&gSaveBlock1Ptr->daycare, gSpecialVar_0x8004);
}

static u8 EggHatchCreateMonSprite(u8 useAlt, u8 state, u8 partyId, u16 *speciesLoc)
{
    u8 position = 0;
    u8 spriteId = 0;
    struct Pokemon *mon = NULL;

    if (useAlt == FALSE)
    {
        mon = &gPlayerParty[partyId];
        position = B_POSITION_OPPONENT_LEFT;
    }
    if (useAlt == TRUE)
    {
        // Alternate sprite allocation position. Never reached.
        mon = &gPlayerParty[partyId];
        position = B_POSITION_OPPONENT_RIGHT;
    }
    switch (state)
    {
    case 0:
        // Load mon sprite gfx
        {
            u16 species = GetMonData(mon, MON_DATA_SPECIES);
            u32 pid = GetMonData(mon, MON_DATA_PERSONALITY);
            HandleLoadSpecialPokePic_DontHandleDeoxys(&gMonFrontPicTable[species],
                                                      gMonSpritesGfxPtr->sprites.ptr[(useAlt * 2) + B_POSITION_OPPONENT_LEFT],
                                                      species, pid);
            LoadCompressedSpritePalette(GetMonSpritePalStruct(mon));
            *speciesLoc = species;
        }
        break;
    case 1:
        // Create mon sprite
        SetMultiuseSpriteTemplateToPokemon(GetMonSpritePalStruct(mon)->tag, position);
        spriteId = CreateSprite(&gMultiuseSpriteTemplate, EGG_X, EGG_Y, 6);
        gSprites[spriteId].invisible = TRUE;
        gSprites[spriteId].callback = SpriteCallbackDummy;
        break;
    }
    return spriteId;
}

static void VBlankCB_EggHatch(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void EggHatch(void)
{
    LockPlayerFieldControls();
    CreateTask(Task_EggHatch, 10);
    FadeScreen(FADE_TO_BLACK, 0);
}

static void Task_EggHatch(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_LoadEggHatch);
        gFieldCallback = FieldCB_ContinueScriptHandleMusic;
        DestroyTask(taskId);
    }
}

static void CB2_LoadEggHatch(void)
{
    switch (gMain.state)
    {
    case 0:
        SetGpuReg(REG_OFFSET_DISPCNT, 0);

        sEggHatchData = Alloc(sizeof(*sEggHatchData));
        AllocateMonSpritesGfx();
        sEggHatchData->eggPartyId = gSpecialVar_0x8004;
        sEggHatchData->eggShardVelocityId = 0;

        SetVBlankCallback(VBlankCB_EggHatch);
        gSpecialVar_0x8005 = GetCurrentMapMusic();

        ResetTempTileDataBuffers();
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates_EggHatch, ARRAY_COUNT(sBgTemplates_EggHatch));

        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);

        SetBgAttribute(1, BG_ATTR_PRIORITY, 2);
        SetBgTilemapBuffer(1, Alloc(0x1000));
        SetBgTilemapBuffer(0, Alloc(0x2000));

        DeactivateAllTextPrinters();
        ResetPaletteFade();
        FreeAllSpritePalettes();
        ResetSpriteData();
        ResetTasks();
        ScanlineEffect_Stop();
        m4aSoundVSyncOn();
        gMain.state++;
        break;
    case 1:
        InitWindows(sWinTemplates_EggHatch);
        sEggHatchData->windowId = 0;
        gMain.state++;
        break;
    case 2:
        DecompressAndLoadBgGfxUsingHeap(0, gBattleTextboxTiles, 0, 0, 0);
        CopyToBgTilemapBuffer(0, gBattleTextboxTilemap, 0, 0);
        LoadCompressedPalette(gBattleTextboxPalette, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 3:
        LoadSpriteSheet(&sEggHatch_Sheet);
        LoadSpriteSheet(&sEggShards_Sheet);
        LoadSpritePalette(&sEgg_SpritePalette);
        gMain.state++;
        break;
    case 4:
        CopyBgTilemapBufferToVram(0);
        AddHatchedMonToParty(sEggHatchData->eggPartyId);
        gMain.state++;
        break;
    case 5:
        EggHatchCreateMonSprite(FALSE, 0, sEggHatchData->eggPartyId, &sEggHatchData->species);
        gMain.state++;
        break;
    case 6:
        sEggHatchData->monSpriteId = EggHatchCreateMonSprite(FALSE, 1, sEggHatchData->eggPartyId, &sEggHatchData->species);
        gMain.state++;
        break;
    case 7:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        LoadPalette(gTradeGba2_Pal, BG_PLTT_ID(1), 5 * PLTT_SIZE_4BPP);
        LoadBgTiles(1, gTradeGba_Gfx, 0x1420, 0);
        CopyToBgTilemapBuffer(1, gTradePlatform_Tilemap, 0x1000, 0);
        CopyBgTilemapBufferToVram(1);
        gMain.state++;
        break;
    case 8:
        SetMainCallback2(CB2_EggHatch);
        sEggHatchData->state = 0;
        break;
    }
    RunTasks();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void EggHatchSetMonNickname(void)
{
    SetMonData(&gPlayerParty[gSpecialVar_0x8004], MON_DATA_NICKNAME, gStringVar3);
    FreeMonSpritesGfx();
    Free(sEggHatchData);
    SetMainCallback2(CB2_ReturnToField);
}

#define tTimer data[0]

static void Task_EggHatchPlayBGM(u8 taskId)
{
    if (gTasks[taskId].tTimer == 0)
    {
        StopMapMusic();
        PlayRainStoppingSoundEffect();
    }

    if (gTasks[taskId].tTimer == 1)
        PlayBGM(MUS_EVOLUTION_INTRO);

    if (gTasks[taskId].tTimer > 60)
    {
        PlayBGM(MUS_EVOLUTION);
        DestroyTask(taskId);
    }
    gTasks[taskId].tTimer++;
}

static void CB2_EggHatch(void)
{
    u16 species;
    u8 gender;
    u32 personality;

    switch (sEggHatchData->state)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        sEggHatchData->eggSpriteId = CreateSprite(&sSpriteTemplate_Egg, EGG_X, EGG_Y, 5);
        ShowBg(0);
        ShowBg(1);
        sEggHatchData->state++;
        CreateTask(Task_EggHatchPlayBGM, 5);
        break;
    case 1:
        if (!gPaletteFade.active)
        {
            FillWindowPixelBuffer(sEggHatchData->windowId, PIXEL_FILL(0));
            sEggHatchData->delayTimer = 0;
            sEggHatchData->state++;
        }
        break;
    case 2:
        if (++sEggHatchData->delayTimer > 30)
        {
            // Start hatching animation
            sEggHatchData->state++;
            gSprites[sEggHatchData->eggSpriteId].callback = SpriteCB_Egg_Shake1;
        }
        break;
    case 3:
        // Wait for hatching animation to finish
        if (gSprites[sEggHatchData->eggSpriteId].callback == SpriteCallbackDummy)
        {
            species = GetMonData(&gPlayerParty[sEggHatchData->eggPartyId], MON_DATA_SPECIES);
            DoMonFrontSpriteAnimation(&gSprites[sEggHatchData->monSpriteId], species, FALSE, 1);
            sEggHatchData->state++;
        }
        break;
    case 4:
        // Wait for Pokémon's front sprite animation
        if (gSprites[sEggHatchData->monSpriteId].callback == SpriteCallbackDummy)
            sEggHatchData->state++;
        break;
    case 5:
        // "{mon} hatched from egg" message/fanfare
        GetMonNickname2(&gPlayerParty[sEggHatchData->eggPartyId], gStringVar1);
        StringExpandPlaceholders(gStringVar4, gText_HatchedFromEgg);
        EggHatchPrintMessage(sEggHatchData->windowId, gStringVar4, 0, 3, TEXT_SKIP_DRAW);
        PlayFanfare(MUS_EVOLVED);
        sEggHatchData->state++;
        PutWindowTilemap(sEggHatchData->windowId);
        CopyWindowToVram(sEggHatchData->windowId, COPYWIN_FULL);
        break;
    case 6:
        if (IsFanfareTaskInactive())
            sEggHatchData->state++;
        break;
    case 7: // Twice?
        if (IsFanfareTaskInactive())
            sEggHatchData->state++;
        break;
    case 8:
        // Ready the nickname prompt
        GetMonNickname2(&gPlayerParty[sEggHatchData->eggPartyId], gStringVar1);
        StringExpandPlaceholders(gStringVar4, gText_NicknameHatchPrompt);
        EggHatchPrintMessage(sEggHatchData->windowId, gStringVar4, 0, 2, 1);
        sEggHatchData->state++;
        break;
    case 9:
        // Print the nickname prompt
        if (!IsTextPrinterActive(sEggHatchData->windowId))
        {
            LoadUserWindowBorderGfx(sEggHatchData->windowId, 0x140, BG_PLTT_ID(14));
            CreateYesNoMenu(&sYesNoWinTemplate, 0x140, 0xE, 0);
            sEggHatchData->state++;
        }
        break;
    case 10:
        // Handle the nickname prompt input
        switch (Menu_ProcessInputNoWrapClearOnChoose())
        {
        case 0: // Yes
            GetMonNickname2(&gPlayerParty[sEggHatchData->eggPartyId], gStringVar3);
            species = GetMonData(&gPlayerParty[sEggHatchData->eggPartyId], MON_DATA_SPECIES);
            gender = GetMonGender(&gPlayerParty[sEggHatchData->eggPartyId]);
            personality = GetMonData(&gPlayerParty[sEggHatchData->eggPartyId], MON_DATA_PERSONALITY, 0);
            DoNamingScreen(NAMING_SCREEN_NICKNAME, gStringVar3, species, gender, personality, EggHatchSetMonNickname);
            break;
        case 1: // No
        case MENU_B_PRESSED:
            sEggHatchData->state++;
            break;
        }
        break;
    case 11:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sEggHatchData->state++;
        break;
    case 12:
        if (!gPaletteFade.active)
        {
            FreeMonSpritesGfx();
            RemoveWindow(sEggHatchData->windowId);
            UnsetBgTilemapBuffer(0);
            UnsetBgTilemapBuffer(1);
            Free(sEggHatchData);
            SetMainCallback2(CB2_ReturnToField);
        }
        break;
    }

    RunTasks();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

#define sTimer      data[0]
#define sSinIdx     data[1]
#define sDelayTimer data[2]

static void SpriteCB_Egg_Shake1(struct Sprite *sprite)
{
    if (++sprite->sTimer > 20)
    {
        sprite->callback = SpriteCB_Egg_Shake2;
        sprite->sTimer = 0;
    }
    else
    {
        // Shake egg
        sprite->sSinIdx = (sprite->sSinIdx + 20) & 0xFF;
        sprite->x2 = Sin(sprite->sSinIdx, 1);
        if (sprite->sTimer == 15)
        {
            // First egg crack
            PlaySE(SE_BALL);
            StartSpriteAnim(sprite, EGG_ANIM_CRACKED_1);
            CreateRandomEggShardSprite();
        }
    }
}

static void SpriteCB_Egg_Shake2(struct Sprite *sprite)
{
    if (++sprite->sDelayTimer > 30)
    {
        if (++sprite->sTimer > 20)
        {
            sprite->callback = SpriteCB_Egg_Shake3;
            sprite->sTimer = 0;
            sprite->sDelayTimer = 0;
        }
        else
        {
            // Shake egg
            sprite->sSinIdx = (sprite->sSinIdx + 20) & 0xFF;
            sprite->x2 = Sin(sprite->sSinIdx, 2);
            if (sprite->sTimer == 15)
            {
                // Second egg crack
                PlaySE(SE_BALL);
                StartSpriteAnim(sprite, EGG_ANIM_CRACKED_2);
            }
        }
    }
}

static void SpriteCB_Egg_Shake3(struct Sprite *sprite)
{
    if (++sprite->sDelayTimer > 30)
    {
        if (++sprite->sTimer > 38)
        {
            u16 UNUSED species;
            sprite->callback = SpriteCB_Egg_WaitHatch;
            sprite->sTimer = 0;
            species = GetMonData(&gPlayerParty[sEggHatchData->eggPartyId], MON_DATA_SPECIES);
            gSprites[sEggHatchData->monSpriteId].x2 = 0;
            gSprites[sEggHatchData->monSpriteId].y2 = 0;
        }
        else
        {
            // Shake egg
            sprite->sSinIdx = (sprite->sSinIdx + 20) & 0xFF;
            sprite->x2 = Sin(sprite->sSinIdx, 2);
            if (sprite->sTimer == 15)
            {
                // Third egg crack
                // This ineffectually sets the animation to the frame it's already using.
                // They likely meant to use the 3rd and final cracked frame of the egg, which goes unused as a result.
                PlaySE(SE_BALL);
            #ifdef BUGFIX
                StartSpriteAnim(sprite, EGG_ANIM_CRACKED_3);
            #else
                StartSpriteAnim(sprite, EGG_ANIM_CRACKED_2);
            #endif
                CreateRandomEggShardSprite();
                CreateRandomEggShardSprite();
            }
            if (sprite->sTimer == 30)
                PlaySE(SE_BALL);
        }
    }
}

static void SpriteCB_Egg_WaitHatch(struct Sprite *sprite)
{
    if (++sprite->sTimer > 50)
    {
        sprite->callback = SpriteCB_Egg_Hatch;
        sprite->sTimer = 0;
    }
}

static void SpriteCB_Egg_Hatch(struct Sprite *sprite)
{
    s16 i;

    // Fade to white to hide transition from egg to Pokémon
    if (sprite->sTimer == 0)
        BeginNormalPaletteFade(PALETTES_ALL, -1, 0, 16, RGB_WHITEALPHA);

    // Create a shower of 16 egg shards in 4 groups of 4
    if ((u32)sprite->sTimer < 4)
    {
        for (i = 0; i < 4; i++)
            CreateRandomEggShardSprite();
    }

    sprite->sTimer++;

    if (!gPaletteFade.active)
    {
        // Screen is hidden by the fade to white, hide egg
        PlaySE(SE_EGG_HATCH);
        sprite->invisible = TRUE;
        sprite->callback = SpriteCB_Egg_Reveal;
        sprite->sTimer = 0;
    }
}

static void SpriteCB_Egg_Reveal(struct Sprite *sprite)
{
    if (sprite->sTimer == 0)
    {
        // Reveal hatched Pokémon
        gSprites[sEggHatchData->monSpriteId].invisible = FALSE;
        StartSpriteAffineAnim(&gSprites[sEggHatchData->monSpriteId], BATTLER_AFFINE_EMERGE);
    }

    // Fade back from white for reveal
    if (sprite->sTimer == 8)
        BeginNormalPaletteFade(PALETTES_ALL, -1, 16, 0, RGB_WHITEALPHA);

    if (sprite->sTimer <= 9)
        gSprites[sEggHatchData->monSpriteId].y--;

    if (sprite->sTimer > 40)
        sprite->callback = SpriteCallbackDummy; // Finished

    sprite->sTimer++;
}

#define sVelocX data[1]
#define sVelocY data[2]
#define sAccelY data[3]
#define sDeltaX data[4]
#define sDeltaY data[5]

static void SpriteCB_EggShard(struct Sprite *sprite)
{
    sprite->sDeltaX += sprite->sVelocX;
    sprite->sDeltaY += sprite->sVelocY;

    sprite->x2 = sprite->sDeltaX / 256;
    sprite->y2 = sprite->sDeltaY / 256;

    sprite->sVelocY += sprite->sAccelY;

    if (sprite->y + sprite->y2 > sprite->y + 20 && sprite->sVelocY > 0)
        DestroySprite(sprite);
}

static void CreateRandomEggShardSprite(void)
{
    u16 spriteAnimIndex;

    s16 velocityX = sEggShardVelocities[sEggHatchData->eggShardVelocityId][0];
    s16 velocityY = sEggShardVelocities[sEggHatchData->eggShardVelocityId][1];
    sEggHatchData->eggShardVelocityId++;

    // Randomly choose one of the 4 shard images
    spriteAnimIndex = Random() % ARRAY_COUNT(sSpriteAnimTable_EggShard);

    CreateEggShardSprite(EGG_X, EGG_Y - 15, velocityX, velocityY, 100, spriteAnimIndex);
}

static void CreateEggShardSprite(u8 x, u8 y, s16 velocityX, s16 velocityY, s16 acceleration, u8 spriteAnimIndex)
{
    u8 spriteId = CreateSprite(&sSpriteTemplate_EggShard, x, y, 4);
    gSprites[spriteId].sVelocX = velocityX;
    gSprites[spriteId].sVelocY = velocityY;
    gSprites[spriteId].sAccelY = acceleration;
    StartSpriteAnim(&gSprites[spriteId], spriteAnimIndex);
}

static void EggHatchPrintMessage(u8 windowId, u8 *string, u8 x, u8 y, u8 speed)
{
    FillWindowPixelBuffer(windowId, PIXEL_FILL(15));
    sEggHatchData->textColor[0] = 0;
    sEggHatchData->textColor[1] = 5;
    sEggHatchData->textColor[2] = 6;
    AddTextPrinterParameterized4(windowId, FONT_NORMAL, x, y, 0, 0, sEggHatchData->textColor, speed, string);
}

u8 GetEggCyclesToSubtract(void)
{
    u8 count, i;
    for (count = CalculatePlayerPartyCount(), i = 0; i < count; i++)
    {
        if (!GetMonData(&gPlayerParty[i], MON_DATA_SANITY_IS_EGG))
        {
            u8 ability = GetMonAbility(&gPlayerParty[i]);
            if (ability == ABILITY_MAGMA_ARMOR || ability == ABILITY_FLAME_BODY)
                return 2;
        }
    }
    return 1;
}

u16 CountPartyAliveNonEggMons(void)
{
    u16 aliveNonEggMonsCount = CountStorageNonEggMons();
    aliveNonEggMonsCount += CountPartyAliveNonEggMonsExcept(PARTY_SIZE);
    return aliveNonEggMonsCount;
}
