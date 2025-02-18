#include "global.h"
#include "mail.h"
#include "constants/items.h"
#include "main.h"
#include "overworld.h"
#include "task.h"
#include "scanline_effect.h"
#include "palette.h"
#include "text.h"
#include "menu.h"
#include "menu_helpers.h"
#include "text_window.h"
#include "string_util.h"
#include "international_string_util.h"
#include "strings.h"
#include "gpu_regs.h"
#include "bg.h"
#include "pokemon_icon.h"
#include "malloc.h"
#include "easy_chat.h"
#include "graphics.h"
#include "constants/rgb.h"

// Bead and Dream mail feature an icon of the Pokémon holding it.
enum {
    ICON_TYPE_NONE,
    ICON_TYPE_BEAD,
    ICON_TYPE_DREAM,
};

struct MailLineLayout
{
    u8 numEasyChatWords:2;
    u8 xOffset:6;
    u8 height;
};

struct MailLayout
{
    u8 numLines;
    u8 signatureYPos;
    u8 signatureWidth;
    u8 wordsYPos;
    u8 wordsXPos;
    const struct MailLineLayout *lines;
};

struct MailGraphics
{
    const u16 *palette;
    const u32 *tiles;
    const u32 *tileMap;
    u32 unused;
    u16 textColor;
    u16 textShadow;
};

struct MailRead
{
    /*0x0000*/ u8 message[8][64];
    /*0x0200*/ u8 playerName[12];
    /*0x020C*/ MainCallback exitCallback;
    /*0x0210*/ MainCallback callback;
    /*0x0214*/ struct Mail *mail;
    /*0x0218*/ bool8 hasText;
    /*0x0219*/ u8 signatureWidth;
    /*0x021a*/ u8 mailType;
    /*0x021b*/ u8 iconType;
    /*0x021c*/ u8 monIconSpriteId;
    /*0x021d*/ u8 language;
    /*0x021e*/ bool8 international;
    /*0x0220*/ u8 * (*parserSingle)(u8 *dest, u16 word);
    /*0x0224*/ u8 * (*parserMultiple)(u8 *dest, const u16 *src, u16 length1, u16 length2);
    /*0x0228*/ const struct MailLayout *layout;
    /*0x022c*/ u8 bg1TilemapBuffer[0x1000];
    /*0x122c*/ u8 bg2TilemapBuffer[0x1000];
};

static EWRAM_DATA struct MailRead *sMailRead = NULL;

static void CB2_InitMailRead(void);
static void BufferMailText(void);
static void PrintMailText(void);
static void VBlankCB_MailRead(void);
static void CB2_MailRead(void);
static void CB2_WaitForPaletteExitOnKeyPress(void);
static void CB2_ExitOnKeyPress(void);
static void CB2_ExitMailReadFreeVars(void);

static const struct BgTemplate sBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .priority = 0
    }, {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .priority = 1
    }, {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 29,
        .priority = 2
    }
};

static const struct WindowTemplate sWindowTemplates[] = {
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 3,
        .width = 26,
        .height = 15,
        .paletteNum = 15,
        .baseBlock = 1
    },
    DUMMY_WIN_TEMPLATE
};

static const u8 sTextColors[] = {
    TEXT_COLOR_TRANSPARENT,
    TEXT_DYNAMIC_COLOR_1,
    TEXT_DYNAMIC_COLOR_2
};

// Background is alternating bars of a dark/light color.
// Either blue or red depending on player's gender
static const u16 sBgColors[GENDER_COUNT][2] = {
    [MALE]   = { RGB(13, 22, 26), RGB(5, 13, 20) },
    [FEMALE] = { RGB(28, 15, 17), RGB(20, 6, 14) }
};

static const struct MailGraphics sMailGraphics[] = {
    [ITEM_TO_MAIL(ITEM_ORANGE_MAIL)] = {
        .palette = gMailPalette_Orange,
        .tiles = gMailTiles_Orange,
        .tileMap = gMailTilemap_Orange,
        .unused = 0x2C0,
        .textColor = RGB(10, 10, 10),
        .textShadow = RGB(25, 25, 25),
    },
    [ITEM_TO_MAIL(ITEM_HARBOR_MAIL)] = {
        .palette = gMailPalette_Harbor,
        .tiles = gMailTiles_Harbor,
        .tileMap = gMailTilemap_Harbor,
        .unused = 0x2E0,
        .textColor = RGB_WHITE,
        .textShadow = RGB(17, 17, 17),
    },
    [ITEM_TO_MAIL(ITEM_GLITTER_MAIL)] = {
        .palette = gMailPalette_Glitter,
        .tiles = gMailTiles_Glitter,
        .tileMap = gMailTilemap_Glitter,
        .unused = 0x400,
        .textColor = RGB(10, 10, 10),
        .textShadow = RGB(25, 25, 25),
    },
    [ITEM_TO_MAIL(ITEM_MECH_MAIL)] = {
        .palette = gMailPalette_Mech,
        .tiles = gMailTiles_Mech,
        .tileMap = gMailTilemap_Mech,
        .unused = 0x1E0,
        .textColor = RGB_WHITE,
        .textShadow = RGB(17, 17, 17),
    },
    [ITEM_TO_MAIL(ITEM_WOOD_MAIL)] = {
        .palette = gMailPalette_Wood,
        .tiles = gMailTiles_Wood,
        .tileMap = gMailTilemap_Wood,
        .unused = 0x2E0,
        .textColor = RGB_WHITE,
        .textShadow = RGB(17, 17, 17),
    },
    [ITEM_TO_MAIL(ITEM_WAVE_MAIL)] = {
        .palette = gMailPalette_Wave,
        .tiles = gMailTiles_Wave,
        .tileMap = gMailTilemap_Wave,
        .unused = 0x300,
        .textColor = RGB(10, 10, 10),
        .textShadow = RGB(25, 25, 25),
    },
    [ITEM_TO_MAIL(ITEM_BEAD_MAIL)] = {
        .palette = gMailPalette_Bead,
        .tiles = gMailTiles_Bead,
        .tileMap = gMailTilemap_Bead,
        .unused = 0x140,
        .textColor = RGB_WHITE,
        .textShadow = RGB(17, 17, 17),
    },
    [ITEM_TO_MAIL(ITEM_SHADOW_MAIL)] = {
        .palette = gMailPalette_Shadow,
        .tiles = gMailTiles_Shadow,
        .tileMap = gMailTilemap_Shadow,
        .unused = 0x300,
        .textColor = RGB_WHITE,
        .textShadow = RGB(17, 17, 17),
    },
    [ITEM_TO_MAIL(ITEM_TROPIC_MAIL)] = {
        .palette = gMailPalette_Tropic,
        .tiles = gMailTiles_Tropic,
        .tileMap = gMailTilemap_Tropic,
        .unused = 0x220,
        .textColor = RGB(10, 10, 10),
        .textShadow = RGB(25, 25, 25),
    },
    [ITEM_TO_MAIL(ITEM_DREAM_MAIL)] = {
        .palette = gMailPalette_Dream,
        .tiles = gMailTiles_Dream,
        .tileMap = gMailTilemap_Dream,
        .unused = 0x340,
        .textColor = RGB(10, 10, 10),
        .textShadow = RGB(25, 25, 25),
    },
    [ITEM_TO_MAIL(ITEM_FAB_MAIL)] = {
        .palette = gMailPalette_Fab,
        .tiles = gMailTiles_Fab,
        .tileMap = gMailTilemap_Fab,
        .unused = 0x2a0,
        .textColor = RGB(10, 10, 10),
        .textShadow = RGB(25, 25, 25),
    },
    [ITEM_TO_MAIL(ITEM_RETRO_MAIL)] = {
        .palette = gMailPalette_Retro,
        .tiles = gMailTiles_Retro,
        .tileMap = gMailTilemap_Retro,
        .unused = 0x520,
        .textColor = RGB(10, 10, 10),
        .textShadow = RGB(25, 25, 25),
    }
};

static const struct MailLineLayout sLineLayouts_Wide[] = {
    { .numEasyChatWords = 3, .xOffset = 0, .height = 16 },
    { .numEasyChatWords = 3, .xOffset = 0, .height = 16 },
    { .numEasyChatWords = 3, .xOffset = 0, .height = 16 }
};

static const struct MailLayout sMailLayouts_Wide[] = {
    [ITEM_TO_MAIL(ITEM_ORANGE_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Wide),
        .signatureYPos = 0,
        .signatureWidth = 0,
        .wordsYPos = 2,
        .wordsXPos = 4,
        .lines = sLineLayouts_Wide,
    },
    [ITEM_TO_MAIL(ITEM_HARBOR_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Wide),
        .signatureYPos = 0,
        .signatureWidth = 0,
        .wordsYPos = 2,
        .wordsXPos = 4,
        .lines = sLineLayouts_Wide,
    },
    [ITEM_TO_MAIL(ITEM_GLITTER_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Wide),
        .signatureYPos = 0,
        .signatureWidth = 0,
        .wordsYPos = 2,
        .wordsXPos = 4,
        .lines = sLineLayouts_Wide,
    },
    [ITEM_TO_MAIL(ITEM_MECH_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Wide),
        .signatureYPos = 0,
        .signatureWidth = 0,
        .wordsYPos = 2,
        .wordsXPos = 4,
        .lines = sLineLayouts_Wide,
    },
    [ITEM_TO_MAIL(ITEM_WOOD_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Wide),
        .signatureYPos = 0,
        .signatureWidth = 0,
        .wordsYPos = 2,
        .wordsXPos = 4,
        .lines = sLineLayouts_Wide,
    },
    [ITEM_TO_MAIL(ITEM_WAVE_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Wide),
        .signatureYPos = 0,
        .signatureWidth = 0,
        .wordsYPos = 2,
        .wordsXPos = 4,
        .lines = sLineLayouts_Wide,
    },
    [ITEM_TO_MAIL(ITEM_BEAD_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Wide),
        .signatureYPos = 0,
        .signatureWidth = 0,
        .wordsYPos = 2,
        .wordsXPos = 4,
        .lines = sLineLayouts_Wide,
    },
    [ITEM_TO_MAIL(ITEM_SHADOW_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Wide),
        .signatureYPos = 0,
        .signatureWidth = 0,
        .wordsYPos = 2,
        .wordsXPos = 4,
        .lines = sLineLayouts_Wide,
    },
    [ITEM_TO_MAIL(ITEM_TROPIC_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Wide),
        .signatureYPos = 0,
        .signatureWidth = 0,
        .wordsYPos = 2,
        .wordsXPos = 4,
        .lines = sLineLayouts_Wide,
    },
    [ITEM_TO_MAIL(ITEM_DREAM_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Wide),
        .signatureYPos = 0,
        .signatureWidth = 0,
        .wordsYPos = 2,
        .wordsXPos = 4,
        .lines = sLineLayouts_Wide,
    },
    [ITEM_TO_MAIL(ITEM_FAB_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Wide),
        .signatureYPos = 8,
        .signatureWidth = 0,
        .wordsYPos = 2,
        .wordsXPos = 4,
        .lines = sLineLayouts_Wide,
    },
    [ITEM_TO_MAIL(ITEM_RETRO_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Wide),
        .signatureYPos = 0,
        .signatureWidth = 0,
        .wordsYPos = 2,
        .wordsXPos = 0,
        .lines = sLineLayouts_Wide,
    },
};

static const struct MailLineLayout sLineLayouts_Tall[] = {
    { .numEasyChatWords = 2, .xOffset = 0, .height = 16 },
    { .numEasyChatWords = 2, .xOffset = 0, .height = 16 },
    { .numEasyChatWords = 2, .xOffset = 0, .height = 16 },
    { .numEasyChatWords = 2, .xOffset = 0, .height = 16 },
    { .numEasyChatWords = 1, .xOffset = 0, .height = 16 }
};

static const struct MailLayout sMailLayouts_Tall[] = {
    [ITEM_TO_MAIL(ITEM_ORANGE_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Tall),
        .signatureYPos = 7,
        .signatureWidth = 88,
        .wordsYPos = 11,
        .wordsXPos = 30,
        .lines = sLineLayouts_Tall,
    },
    [ITEM_TO_MAIL(ITEM_HARBOR_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Tall),
        .signatureYPos = 10,
        .signatureWidth = 96,
        .wordsYPos = 9,
        .wordsXPos = 30,
        .lines = sLineLayouts_Tall,
    },
    [ITEM_TO_MAIL(ITEM_GLITTER_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Tall),
        .signatureYPos = 12,
        .signatureWidth = 104,
        .wordsYPos = 5,
        .wordsXPos = 30,
        .lines = sLineLayouts_Tall,
    },
    [ITEM_TO_MAIL(ITEM_MECH_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Tall),
        .signatureYPos = 5,
        .signatureWidth = 96,
        .wordsYPos = 8,
        .wordsXPos = 30,
        .lines = sLineLayouts_Tall,
    },
    [ITEM_TO_MAIL(ITEM_WOOD_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Tall),
        .signatureYPos = 10,
        .signatureWidth = 96,
        .wordsYPos = 9,
        .wordsXPos = 30,
        .lines = sLineLayouts_Tall,
    },
    [ITEM_TO_MAIL(ITEM_WAVE_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Tall),
        .signatureYPos = 9,
        .signatureWidth = 112,
        .wordsYPos = 5,
        .wordsXPos = 30,
        .lines = sLineLayouts_Tall,
    },
    [ITEM_TO_MAIL(ITEM_BEAD_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Tall),
        .signatureYPos = 12,
        .signatureWidth = 104,
        .wordsYPos = 9,
        .wordsXPos = 30,
        .lines = sLineLayouts_Tall,
    },
    [ITEM_TO_MAIL(ITEM_SHADOW_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Tall),
        .signatureYPos = 13,
        .signatureWidth = 104,
        .wordsYPos = 13,
        .wordsXPos = 30,
        .lines = sLineLayouts_Tall,
    },
    [ITEM_TO_MAIL(ITEM_TROPIC_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Tall),
        .signatureYPos = 9,
        .signatureWidth = 96,
        .wordsYPos = 9,
        .wordsXPos = 30,
        .lines = sLineLayouts_Tall,
    },
    [ITEM_TO_MAIL(ITEM_DREAM_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Tall),
        .signatureYPos = 9,
        .signatureWidth = 96,
        .wordsYPos = 9,
        .wordsXPos = 30,
        .lines = sLineLayouts_Tall,
    },
    [ITEM_TO_MAIL(ITEM_FAB_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Tall),
        .signatureYPos = 17,
        .signatureWidth = 104,
        .wordsYPos = 15,
        .wordsXPos = 30,
        .lines = sLineLayouts_Tall,
    },
    [ITEM_TO_MAIL(ITEM_RETRO_MAIL)] = {
        .numLines = ARRAY_COUNT(sLineLayouts_Tall),
        .signatureYPos = 9,
        .signatureWidth = 96,
        .wordsYPos = 5,
        .wordsXPos = 30,
        .lines = sLineLayouts_Tall,
    },
};

void ReadMail(struct Mail *mail, void (*exitCallback)(void), bool8 hasText)
{
    u16 buffer[2];
    u16 species;

    sMailRead = AllocZeroed(sizeof(*sMailRead));
    sMailRead->language = GAME_LANGUAGE;
    sMailRead->international = TRUE;
    sMailRead->parserSingle = CopyEasyChatWord;
    sMailRead->parserMultiple = ConvertEasyChatWordsToString;
    if (IS_ITEM_MAIL(mail->itemId))
    {
        sMailRead->mailType = ITEM_TO_MAIL(mail->itemId);
    }
    else
    {
        sMailRead->mailType = ITEM_TO_MAIL(FIRST_MAIL_INDEX);
        hasText = FALSE;
    }
    switch (sMailRead->international)
    {
    case FALSE:
    default:
        // Never reached. JP only?
        sMailRead->layout = &sMailLayouts_Wide[sMailRead->mailType];
        break;
    case TRUE:
        sMailRead->layout = &sMailLayouts_Tall[sMailRead->mailType];
        break;
    }
    species = MailSpeciesToSpecies(mail->species, buffer);
    if (species > SPECIES_NONE && species < NUM_SPECIES)
    {
        switch (sMailRead->mailType)
        {
        default:
            sMailRead->iconType = ICON_TYPE_NONE;
            break;
        case ITEM_TO_MAIL(ITEM_BEAD_MAIL):
            sMailRead->iconType = ICON_TYPE_BEAD;
            break;
        case ITEM_TO_MAIL(ITEM_DREAM_MAIL):
            sMailRead->iconType = ICON_TYPE_DREAM;
            break;
        }
    }
    else
    {
        sMailRead->iconType = ICON_TYPE_NONE;
    }
    sMailRead->mail = mail;
    sMailRead->exitCallback = exitCallback;
    sMailRead->hasText = hasText;
    SetMainCallback2(CB2_InitMailRead);
}

static bool8 MailReadBuildGraphics(void)
{
    u16 icon;

    switch (gMain.state)
    {
        case 0:
            SetVBlankCallback(NULL);
            ScanlineEffect_Stop();
            SetGpuReg(REG_OFFSET_DISPCNT, 0);
            break;
        case 1:
            CpuFill16(0, (void *)OAM, OAM_SIZE);
            break;
        case 2:
            ResetPaletteFade();
            break;
        case 3:
            ResetTasks();
            break;
        case 4:
            ResetSpriteData();
            break;
        case 5:
            FreeAllSpritePalettes();
            ResetTempTileDataBuffers();
            SetGpuReg(REG_OFFSET_BG0HOFS, 0);
            SetGpuReg(REG_OFFSET_BG0VOFS, 0);
            SetGpuReg(REG_OFFSET_BG1HOFS, 0);
            SetGpuReg(REG_OFFSET_BG1VOFS, 0);
            SetGpuReg(REG_OFFSET_BG2VOFS, 0);
            SetGpuReg(REG_OFFSET_BG2HOFS, 0);
            SetGpuReg(REG_OFFSET_BG3HOFS, 0);
            SetGpuReg(REG_OFFSET_BG3VOFS, 0);
            SetGpuReg(REG_OFFSET_BLDCNT,  0);
            SetGpuReg(REG_OFFSET_BLDALPHA, 0);
            break;
        case 6:
            ResetBgsAndClearDma3BusyFlags(0);
            InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
            SetBgTilemapBuffer(1, sMailRead->bg1TilemapBuffer);
            SetBgTilemapBuffer(2, sMailRead->bg2TilemapBuffer);
            break;
        case 7:
            InitWindows(sWindowTemplates);
            DeactivateAllTextPrinters();
            break;
        case 8:
            DecompressAndCopyTileDataToVram(1, sMailGraphics[sMailRead->mailType].tiles, 0, 0, 0);
            break;
        case 9:
            if (FreeTempTileDataBuffersIfPossible())
                return FALSE;
            break;
        case 10:
            FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
            FillBgTilemapBufferRect_Palette0(2, 1, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
            CopyToBgTilemapBuffer(1, sMailGraphics[sMailRead->mailType].tileMap, 0, 0);
            break;
        case 11:
            CopyBgTilemapBufferToVram(0);
            CopyBgTilemapBufferToVram(1);
            CopyBgTilemapBufferToVram(2);
            break;
        case 12:
            LoadPalette(GetOverworldTextboxPalettePtr(), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
            gPlttBufferUnfaded[BG_PLTT_ID(15) + 10] = sMailGraphics[sMailRead->mailType].textColor;
            gPlttBufferFaded[BG_PLTT_ID(15) + 10] = sMailGraphics[sMailRead->mailType].textColor;
            gPlttBufferUnfaded[BG_PLTT_ID(15) + 11] = sMailGraphics[sMailRead->mailType].textShadow;
            gPlttBufferFaded[BG_PLTT_ID(15) + 11] = sMailGraphics[sMailRead->mailType].textShadow;

            LoadPalette(sMailGraphics[sMailRead->mailType].palette, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
            gPlttBufferUnfaded[BG_PLTT_ID(0) + 10] = sBgColors[gSaveBlock2Ptr->playerGender][0];
            gPlttBufferFaded[BG_PLTT_ID(0) + 10] = sBgColors[gSaveBlock2Ptr->playerGender][0];
            gPlttBufferUnfaded[BG_PLTT_ID(0) + 11] = sBgColors[gSaveBlock2Ptr->playerGender][1];
            gPlttBufferFaded[BG_PLTT_ID(0) + 11] = sBgColors[gSaveBlock2Ptr->playerGender][1];
            break;
        case 13:
            if (sMailRead->hasText)
                BufferMailText();
            break;
        case 14:
            if (sMailRead->hasText)
            {
                PrintMailText();
                RunTextPrinters();
            }
            break;
        case 15:
            if (Overworld_IsRecvQueueAtMax() == TRUE)
                return FALSE;
            break;
        case 16:
            SetVBlankCallback(VBlankCB_MailRead);
            gPaletteFade.bufferTransferDisabled = TRUE;
            break;
        case 17:
            icon = GetIconSpeciesNoPersonality(sMailRead->mail->species);
            switch (sMailRead->iconType)
            {
            case ICON_TYPE_BEAD:
                LoadMonIconPalette(icon);
                sMailRead->monIconSpriteId = CreateMonIconNoPersonality(icon, SpriteCallbackDummy, 96, 128, 0, FALSE);
                break;
            case ICON_TYPE_DREAM:
                LoadMonIconPalette(icon);
                sMailRead->monIconSpriteId = CreateMonIconNoPersonality(icon, SpriteCallbackDummy, 40, 128, 0, FALSE);
                break;
            }
            break;
        case 18:
            SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
            ShowBg(0);
            ShowBg(1);
            ShowBg(2);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
            gPaletteFade.bufferTransferDisabled = FALSE;
            sMailRead->callback = CB2_WaitForPaletteExitOnKeyPress;
            return TRUE;
        default:
            return FALSE;
    }
    gMain.state++;
    return FALSE;
}

static void CB2_InitMailRead(void)
{
    do
    {
        if (MailReadBuildGraphics() == TRUE)
        {
            SetMainCallback2(CB2_MailRead);
            break;
        }
    } while (MenuHelpers_IsLinkActive() != TRUE);
}

static void BufferMailText(void)
{
    u16 i;
    u8 numWords;
    u8 *ptr;

    // Convert the easy chat words to strings line by line and buffer them to message
    numWords = 0;
    for (i = 0; i < sMailRead->layout->numLines; i ++)
    {
        ConvertEasyChatWordsToString(sMailRead->message[i], &sMailRead->mail->words[numWords], sMailRead->layout->lines[i].numEasyChatWords, 1);
        numWords += sMailRead->layout->lines[i].numEasyChatWords;
    }

    // Buffer the signature
    ptr = StringCopy(sMailRead->playerName, sMailRead->mail->playerName);
    if (!sMailRead->international)
    {
        // Never reached
        StringCopy(ptr, gText_FromSpace); // Odd, "From" text is already printed in PrintMailText
        sMailRead->signatureWidth = sMailRead->layout->signatureWidth - (StringLength(sMailRead->playerName) * 8 - 96);
    }
    else
    {
        ConvertInternationalPlayerName(sMailRead->playerName);
        sMailRead->signatureWidth = sMailRead->layout->signatureWidth;
    }
}

static void PrintMailText(void)
{
    u16 i;
    u8 signature[32];
    u8 y;
    u8 *bufptr;
    s32 box_x;
    s32 box_y;

    y = 0;
    PutWindowTilemap(0);
    PutWindowTilemap(1);
    FillWindowPixelBuffer(0, PIXEL_FILL(0));
    FillWindowPixelBuffer(1, PIXEL_FILL(0));
    for (i = 0; i < sMailRead->layout->numLines; i ++)
    {
        if (sMailRead->message[i][0] == EOS || sMailRead->message[i][0] == CHAR_SPACE)
            continue;

        AddTextPrinterParameterized3(0, FONT_NORMAL, sMailRead->layout->lines[i].xOffset + sMailRead->layout->wordsXPos, y + sMailRead->layout->wordsYPos, sTextColors, 0, sMailRead->message[i]);
        y += sMailRead->layout->lines[i].height;
    }
    bufptr = StringCopy(signature, gText_FromSpace);
    StringCopy(bufptr, sMailRead->playerName);
    box_x = GetStringCenterAlignXOffset(FONT_NORMAL, signature, sMailRead->signatureWidth) + 104;
    box_y = sMailRead->layout->signatureYPos + 88;
    AddTextPrinterParameterized3(0, FONT_NORMAL, box_x, box_y, sTextColors, 0, signature);
    CopyWindowToVram(0, COPYWIN_FULL);
    CopyWindowToVram(1, COPYWIN_FULL);
}

static void VBlankCB_MailRead(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_MailRead(void)
{
    if (sMailRead->iconType != ICON_TYPE_NONE)
    {
        AnimateSprites();
        BuildOamBuffer();
    }
    sMailRead->callback();
}

static void CB2_WaitForPaletteExitOnKeyPress(void)
{
    if (!UpdatePaletteFade())
    {
        sMailRead->callback = CB2_ExitOnKeyPress;
    }
}

static void CB2_ExitOnKeyPress(void)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sMailRead->callback = CB2_ExitMailReadFreeVars;
    }
}

static void CB2_ExitMailReadFreeVars(void)
{
    if (!UpdatePaletteFade())
    {
        SetMainCallback2(sMailRead->exitCallback);
        switch (sMailRead->iconType)
        {
        case ICON_TYPE_BEAD:
        case ICON_TYPE_DREAM:
            FreeMonIconPalette(GetIconSpeciesNoPersonality(sMailRead->mail->species));
            FreeAndDestroyMonIconSprite(&gSprites[sMailRead->monIconSpriteId]);
        }
        memset(sMailRead, 0, sizeof(*sMailRead));
        ResetPaletteFade();
        UnsetBgTilemapBuffer(0);
        UnsetBgTilemapBuffer(1);
        ResetBgsAndClearDma3BusyFlags(0);
        FreeAllWindowBuffers();
        FREE_AND_SET_NULL(sMailRead);
    }
}
