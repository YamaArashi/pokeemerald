#include "global.h"
#include "malloc.h"
#include "battle_anim.h"
#include "battle_interface.h"
#include "bg.h"
#include "cable_club.h"
#include "data.h"
#include "daycare.h"
#include "decompress.h"
#include "event_data.h"
#include "evolution_scene.h"
#include "field_screen_effect.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "librfu.h"
#include "link.h"
#include "link_rfu.h"
#include "load_save.h"
#include "mail.h"
#include "main.h"
#include "mystery_gift.h"
#include "mystery_gift_menu.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pokeball.h"
#include "pokedex.h"
#include "pokemon_icon.h"
#include "pokemon_summary_screen.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "save.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trainer_card.h"
#include "trade.h"
#include "union_room.h"
#include "util.h"
#include "window.h"
#include "constants/contest.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/region_map_sections.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/union_room.h"

// IDs for RunTradeMenuCallback
enum {
    CB_MAIN_MENU,
    CB_SELECTED_MON,
    CB_SHOW_MON_SUMMARY,
    CB_CONFIRM_TRADE_PROMPT,
    CB_CANCEL_TRADE_PROMPT,
    CB_READY_WAIT, // Unused in Emerald, equivalent to CB_IDLE
    CB_SET_SELECTED_MONS,
    CB_PRINT_IS_THIS_OKAY,
    CB_HANDLE_TRADE_CANCELED,
    CB_FADE_TO_START_TRADE,
    CB_WAIT_TO_START_TRADE,
    CB_INIT_EXIT_CANCELED_TRADE,
    CB_EXIT_CANCELED_TRADE,
    CB_START_LINK_TRADE,
    CB_INIT_CONFIRM_TRADE_PROMPT,
    CB_UNUSED_CLOSE_MSG,
    CB_WAIT_TO_START_RFU_TRADE,
    CB_PARTNER_MON_INVALID,
    CB_IDLE = 100,
};

// Indexes for sActionTexts
enum {
    TEXT_CANCEL,
    TEXT_CHOOSE_MON,
    TEXT_SUMMARY,
    TEXT_TRADE,
    TEXT_CANCEL_TRADE,
    TEXT_JP_QUIT,
};

// Indexes for sMessages
enum {
    MSG_STANDBY,
    MSG_CANCELED,
    MSG_ONLY_MON1,
    MSG_ONLY_MON2,
    MSG_WAITING_FOR_FRIEND,
    MSG_FRIEND_WANTS_TO_TRADE,
    MSG_MON_CANT_BE_TRADED,
    MSG_EGG_CANT_BE_TRADED,
    MSG_FRIENDS_MON_CANT_BE_TRADED,
};

// IDs for QueueAction
enum {
    QUEUE_SEND_DATA,
    QUEUE_STANDBY,
    QUEUE_ONLY_MON1,
    QUEUE_ONLY_MON2,
    QUEUE_UNUSED1, // Presumably intended for MSG_WAITING_FOR_FRIEND
    QUEUE_UNUSED2, // Presumably intended for MSG_FRIEND_WANTS_TO_TRADE
    QUEUE_MON_CANT_BE_TRADED,
    QUEUE_EGG_CANT_BE_TRADED,
    QUEUE_FRIENDS_MON_CANT_BE_TRADED,
};

#define QUEUE_DELAY_MSG   3
#define QUEUE_DELAY_DATA  5

// The following tags are offsets from GFXTAG_MENU_TEXT
// They're looped over in CB2_CreateTradeMenu and CB2_ReturnToTradeMenu
// and used as indexes into sMenuTextTileBuffers
enum {
    GFXTAG_PLAYER_NAME_L,
    GFXTAG_PLAYER_NAME_M,
    GFXTAG_PLAYER_NAME_R,
    GFXTAG_PARTNER_NAME_L,
    GFXTAG_PARTNER_NAME_M,
    GFXTAG_PARTNER_NAME_R,
    GFXTAG_CANCEL_L,
    GFXTAG_CANCEL_R,
    GFXTAG_CHOOSE_PKMN_L,
    GFXTAG_CHOOSE_PKMN_M,
    GFXTAG_CHOOSE_PKMN_R,
    GFXTAG_CHOOSE_PKMN_EMPTY_1, // 6 sprites to cover the full bottom bar, but only first 3 are needed
    GFXTAG_CHOOSE_PKMN_EMPTY_2,
    GFXTAG_CHOOSE_PKMN_EMPTY_3,
    NUM_MENU_TEXT_SPRITES
};
#define NUM_PLAYER_NAME_SPRITES (1 + GFXTAG_PLAYER_NAME_R - GFXTAG_PLAYER_NAME_L)
#define NUM_PARTNER_NAME_SPRITES (1 + GFXTAG_PARTNER_NAME_R - GFXTAG_PARTNER_NAME_L)
#define NUM_CHOOSE_PKMN_SPRITES (1 + GFXTAG_CHOOSE_PKMN_EMPTY_3 - GFXTAG_CHOOSE_PKMN_L)

// Values for signaling to/from the link partner
enum {
    STATUS_NONE,
    STATUS_READY,
    STATUS_CANCEL,
};

// Checked to confirm DrawSelectedMonScreen has reached final state
#define DRAW_SELECTED_FINISH 5

struct InGameTrade {
    u8 nickname[POKEMON_NAME_LENGTH + 1];
    u16 species;
    u8 ivs[NUM_STATS];
    u8 abilityNum;
    u32 otId;
    u8 conditions[CONTEST_CATEGORIES_COUNT];
    u32 personality;
    u16 heldItem;
    u8 mailNum;
    u8 otName[TRAINER_NAME_LENGTH + 1];
    u8 otGender;
    u8 sheen;
    u16 requestedSpecies;
};

static EWRAM_DATA u8 *sMenuTextTileBuffer = NULL;

// Bytes 0-2 are used for the player's name text
// Bytes 3-5 are used for the partner's name text
// Bytes 6-7 are used for the Cancel text
// Bytes 8-13 are used for the Choose a Pokémon text
// See the corresponding GFXTAGs in src/data/trade.h
static EWRAM_DATA u8 *sMenuTextTileBuffers[NUM_MENU_TEXT_SPRITES] = {NULL};

EWRAM_DATA struct Mail gTradeMail[PARTY_SIZE] = {0};
EWRAM_DATA u8 gSelectedTradeMonPositions[2] = {0};
static EWRAM_DATA struct {
    u8 bg2hofs;
    u8 bg3hofs;
    u8 filler_2[38];
    u8 partySpriteIds[2][PARTY_SIZE];
    u8 cursorSpriteId;
    u8 cursorPosition;
    u8 partyCounts[2];
    bool8 optionsActive[PARTY_SIZE * 2 + 1];
    bool8 isLiveMon[2][PARTY_SIZE];
    bool8 isEgg[2][PARTY_SIZE];
    u8 hpBarLevels[2][PARTY_SIZE];
    u8 bufferPartyState;
    u8 filler_6A[5];
    u8 callbackId;
    u8 neverRead_70;
    u16 bottomTextTileStart;
    u8 drawSelectedMonState[2];
    u8 selectedMonIdx[2];
    u8 playerSelectStatus;
    u8 partnerSelectStatus;
    u8 playerConfirmStatus;
    u8 partnerConfirmStatus;
    u8 filler_7C[2];
    u8 partnerCursorPosition;
    u16 linkData[20];
    u8 timer;
    u8 giftRibbons[GIFT_RIBBONS_COUNT];
    u8 filler_B4[0x81C];
    struct {
        bool8 active;
        u16 delay;
        u8 actionId;
    } queuedActions[4];
    u16 tilemapBuffer[BG_SCREEN_SIZE / 2];
} *sTradeMenu = NULL;

static EWRAM_DATA struct {
    struct Pokemon tempMon; // Used as a temp variable when swapping Pokémon
    u32 timer;
    u32 monPersonalities[2];
    u8 filler_70[2];
    u8 playerFinishStatus;
    u8 partnerFinishStatus;
    u16 linkData[10];
    u8 linkTimeoutZero1;
    u8 linkTimeoutZero2;
    u16 linkTimeoutTimer;
    u16 neverRead_8C;
    u8 monSpriteIds[2];
    u8 connectionSpriteId1; // Multi-purpose sprite ids used during the transfer sequence
    u8 connectionSpriteId2;
    u8 cableEndSpriteId;
    u8 scheduleLinkTransfer;
    u16 state;
    u8 filler_96[0x3C];
    u8 releasePokeballSpriteId;
    u8 bouncingPokeballSpriteId;
    u16 texX;
    u16 texY;
    u16 neverRead_D8;
    u16 neverRead_DA;
    u16 scrX;
    u16 scrY;
    s16 bg1vofs;
    s16 bg1hofs;
    s16 bg2vofs;
    s16 bg2hofs;
    u16 sXY;
    u16 gbaScale;
    u16 alpha;
    bool8 isLinkTrade;
    u16 monSpecies[2];
    u16 cachedMapMusic;
    u8 textColors[3];
    u8 filler_F9;
    bool8 isCableTrade;
    u8 wirelessWinLeft;
    u8 wirelessWinTop;
    u8 wirelessWinRight;
    u8 wirelessWinBottom;
} *sTradeAnim = NULL;

static bool32 IsWirelessTrade(void);
static void CB2_CreateTradeMenu(void);
static void VBlankCB_TradeMenu(void);
static void CB2_TradeMenu(void);
static void LoadTradeBgGfx(u8);
static void SetActiveMenuOptions(void);
static bool8 BufferTradeParties(void);
static void CB1_UpdateLink(void);
static void RunTradeMenuCallback(void);
static void SetSelectedMon(u8);
static void DrawSelectedMonScreen(u8);
static u8 GetMonNicknameWidth(u8 *, u8, u8);
static void BufferMovesString(u8 *, u8, u8);
static void PrintPartyNicknames(u8);
static void PrintLevelAndGender(u8, u8, u8, u8, u8, u8);
static void PrintPartyLevelsAndGenders(u8);
static void PrintTradePartnerPartyNicknames(void);
static void RedrawPartyWindow(u8);
static void Task_DrawSelectionSummary(u8);
static void Task_DrawSelectionTrade(u8);
static void QueueAction(u16, u8);
static u32 GetNumQueuedActions(void);
static void DoQueuedActions(void);
static void PrintTradeMessage(u8);
static bool8 LoadUISpriteGfx(void);
static void DrawBottomRowText(const u8 *, u8 *, u8);
static void ComputePartyTradeableFlags(u8);
static void ComputePartyHPBarLevels(u8);
static void SetTradePartyHPBarSprites(void);
static void SaveTradeGiftRibbons(void);
static u32 CanTradeSelectedMon(struct Pokemon *, int, int);
static void SpriteCB_LinkMonGlow(struct Sprite *);
static void SpriteCB_LinkMonShadow(struct Sprite *);
static void SpriteCB_CableEndSending(struct Sprite *);
static void SpriteCB_CableEndReceiving(struct Sprite *);
static void SpriteCB_GbaScreen(struct Sprite *);
static void TradeAnimInit_LoadGfx(void);
static void CB2_InGameTrade(void);
static void SetTradeSequenceBgGpuRegs(u8);
static void LoadTradeSequenceSpriteSheetsAndPalettes(void);
static void BufferTradeSceneStrings(void);
static bool8 DoTradeAnim(void);
static bool8 DoTradeAnim_Cable(void);
static bool8 DoTradeAnim_Wireless(void);
static void SpriteCB_BouncingPokeball(struct Sprite *);
static void SpriteCB_BouncingPokeballDepart(struct Sprite *);
static void SpriteCB_BouncingPokeballDepartEnd(struct Sprite *);
static void SpriteCB_BouncingPokeballArrive(struct Sprite *);
static void BufferInGameTradeMonName(void);
static void GetInGameTradeMail(struct Mail *, const struct InGameTrade *);
static void CB2_UpdateLinkTrade(void);
static void CB2_WaitTradeComplete(void);
static void CB2_SaveAndEndTrade(void);
static void CB2_FreeTradeAnim(void);
static void Task_InGameTrade(u8);
static void CheckPartnersMonForRibbons(void);
static void Task_AnimateWirelessSignal(u8);
static void Task_OpenCenterWhiteColumn(u8);
static void Task_CloseCenterWhiteColumn(u8);
static void CB2_SaveAndEndWirelessTrade(void);

#include "data/trade.h"

static bool8 SendLinkData(const void *linkData, u32 size)
{
    if (gPlayerCurrActivity == ACTIVITY_29)
    {
        rfu_NI_setSendData(lman.acceptSlot_flag, 84, linkData, size);
        return TRUE;
    }
    else
    {
        return SendBlock(0, linkData, size);
    }
}

static void RequestLinkData(u8 type)
{
    SendBlockRequest(type);
}

static bool32 IsLinkTradeTaskFinished(void)
{
    if (gPlayerCurrActivity == ACTIVITY_29)
    {
        if (gRfuSlotStatusNI[Rfu_GetIndexOfNewestChild(lman.acceptSlot_flag)]->send.state == 0)
            return TRUE;
        else
            return FALSE;
    }
    else
    {
        return IsLinkTaskFinished();
    }
}

static u32 _GetBlockReceivedStatus(void)
{
    return GetBlockReceivedStatus();
}

static void TradeResetReceivedFlags(void)
{
    if (IsWirelessTrade())
        rfu_clearSlot(12, lman.acceptSlot_flag);
    else
        ResetBlockReceivedFlags();
}

static void TradeResetReceivedFlag(u32 who)
{
    if (IsWirelessTrade())
        rfu_clearSlot(12, lman.acceptSlot_flag);
    else
        ResetBlockReceivedFlag(who);
}

static bool32 IsWirelessTrade(void)
{
    if (gWirelessCommType && gPlayerCurrActivity == ACTIVITY_29)
        return TRUE;
    else
        return FALSE;
}

static void SetTradeLinkStandbyCallback(u8 unused)
{
    SetLinkStandbyCallback();
}

static bool32 _IsLinkTaskFinished(void)
{
    return IsLinkTaskFinished();
}

static void InitTradeMenu(void)
{
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetTasks();
    ResetPaletteFade();

    gPaletteFade.bufferTransferDisabled = TRUE;

    SetVBlankCallback(VBlankCB_TradeMenu);
    LoadPalette(gStandardMenuPalette, BG_PLTT_ID(15), PLTT_SIZEOF(10));
    LoadPalette(gStandardMenuPalette, BG_PLTT_ID(13), PLTT_SIZEOF(10));
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    SetBgTilemapBuffer(1, sTradeMenu->tilemapBuffer);

    if (InitWindows(sWindowTemplates))
    {
        u32 i;

        DeactivateAllTextPrinters();

        for (i = 0; i < ARRAY_COUNT(sWindowTemplates) - 1; i++)
        {
            ClearWindowTilemap(i);
            FillWindowPixelBuffer(i, PIXEL_FILL(0));
        }

        FillBgTilemapBufferRect(0, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT, 15);
        LoadUserWindowBorderGfx_(0, 20, BG_PLTT_ID(12));
        LoadUserWindowBorderGfx(2, 1, BG_PLTT_ID(14));
        LoadMonIconPalettes();
        sTradeMenu->bufferPartyState = 0;
        sTradeMenu->callbackId = CB_MAIN_MENU;
        sTradeMenu->neverRead_70 = 0;
        sTradeMenu->drawSelectedMonState[TRADE_PLAYER] = 0;
        sTradeMenu->drawSelectedMonState[TRADE_PARTNER] = 0;
        sTradeMenu->playerConfirmStatus = STATUS_NONE;
        sTradeMenu->partnerConfirmStatus = STATUS_NONE;
        sTradeMenu->timer = 0;
    }
}

void CB2_StartCreateTradeMenu(void)
{
    SetMainCallback2(CB2_CreateTradeMenu);
    gMain.callback1 = NULL;
    gEnemyPartyCount = 0;
}

static void CB2_CreateTradeMenu(void)
{
    int i;
    struct SpriteTemplate temp;
    u8 id;
    u32 xPos;

    switch (gMain.state)
    {
    case 0:
        sTradeMenu = AllocZeroed(sizeof(*sTradeMenu));
        InitTradeMenu();
        sMenuTextTileBuffer = AllocZeroed(NUM_MENU_TEXT_SPRITES * 256);

        for (i = 0; i < NUM_MENU_TEXT_SPRITES; i++)
            sMenuTextTileBuffers[i] = &sMenuTextTileBuffer[i * 256];

        gMain.state++;
        break;
    case 1:
        gPaletteFade.bufferTransferDisabled = FALSE;

        for (i = 0; i < PARTY_SIZE; i++)
            CreateMon(&gEnemyParty[i], SPECIES_NONE, 0, USE_RANDOM_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);

        PrintTradeMessage(MSG_STANDBY);
        ShowBg(0);

        if (!gReceivedRemoteLinkPlayers)
        {
            gLinkType = LINKTYPE_TRADE_CONNECTING;
            sTradeMenu->timer = 0;

            if (gWirelessCommType)
            {
                SetWirelessCommType1();
                OpenLink();
                CreateTask_RfuIdle();
            }
            else
            {
                OpenLink();
                gMain.state++;
                CreateTask(Task_WaitForLinkPlayerConnection, 1);
            }
        }
        else
        {
            gMain.state = 4;
        }
        break;
    case 2:
        sTradeMenu->timer++;
        if (sTradeMenu->timer > 11)
        {
            sTradeMenu->timer = 0;
            gMain.state++;
        }
        break;
    case 3:
        if (GetLinkPlayerCount_2() >= GetSavedPlayerCount())
        {
            if (IsLinkMaster())
            {
                if (++sTradeMenu->timer > 30)
                {
                    CheckShouldAdvanceLinkState();
                    gMain.state++;
                }
            }
            else
            {
                gMain.state++;
            }
        }
        break;
    case 4:
        if (gReceivedRemoteLinkPlayers == TRUE && IsLinkPlayerDataExchangeComplete() == TRUE)
        {
            DestroyTask_RfuIdle();
            CalculatePlayerPartyCount();
            gMain.state++;
            sTradeMenu->timer = 0;
            if (gWirelessCommType)
            {
                Rfu_SetLinkRecovery(TRUE);
                SetLinkStandbyCallback();
            }
        }
        break;
    case 5:
        if (gWirelessCommType)
        {
            if (IsLinkRfuTaskFinished())
            {
                gMain.state++;
                LoadWirelessStatusIndicatorSpriteGfx();
                CreateWirelessStatusIndicatorSprite(0, 0);
            }
        }
        else
        {
            gMain.state++;
        }
        break;
    case 6:
        if (BufferTradeParties())
        {
            SaveTradeGiftRibbons();
            gMain.state++;
        }
        break;
    case 7:
        CalculateEnemyPartyCount();
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        sTradeMenu->partyCounts[TRADE_PLAYER] = gPlayerPartyCount;
        sTradeMenu->partyCounts[TRADE_PARTNER] = gEnemyPartyCount;

        for (i = 0; i < sTradeMenu->partyCounts[TRADE_PLAYER]; i++)
        {
            struct Pokemon *mon = &gPlayerParty[i];
            sTradeMenu->partySpriteIds[TRADE_PLAYER][i] = CreateMonIcon(GetMonData(mon, MON_DATA_SPECIES_OR_EGG),
                                                         SpriteCB_MonIcon,
                                                         (sTradeMonSpriteCoords[i][0] * 8) + 14,
                                                         (sTradeMonSpriteCoords[i][1] * 8) - 12,
                                                         1,
                                                         GetMonData(mon, MON_DATA_PERSONALITY),
                                                         TRUE);
        }

        for (i = 0; i < sTradeMenu->partyCounts[TRADE_PARTNER]; i++)
        {
            struct Pokemon *mon = &gEnemyParty[i];
            sTradeMenu->partySpriteIds[TRADE_PARTNER][i] = CreateMonIcon(GetMonData(mon, MON_DATA_SPECIES_OR_EGG, NULL),
                                                         SpriteCB_MonIcon,
                                                         (sTradeMonSpriteCoords[i + PARTY_SIZE][0] * 8) + 14,
                                                         (sTradeMonSpriteCoords[i + PARTY_SIZE][1] * 8) - 12,
                                                         1,
                                                         GetMonData(mon, MON_DATA_PERSONALITY),
                                                         FALSE);
        }
        gMain.state++;
        break;
    case 8:
        LoadHeldItemIcons();
        DrawHeldItemIconsForTrade(&sTradeMenu->partyCounts[0], sTradeMenu->partySpriteIds[0], TRADE_PLAYER);
        gMain.state++;
        break;
    case 9:
        DrawHeldItemIconsForTrade(&sTradeMenu->partyCounts[0], sTradeMenu->partySpriteIds[0], TRADE_PARTNER);
        gMain.state++;
        break;
    case 10:
        DrawTextWindowAndBufferTiles(gSaveBlock2Ptr->playerName, sMenuTextTileBuffers[GFXTAG_PLAYER_NAME_L], 0, 0, 3);
        id = GetMultiplayerId();
        DrawTextWindowAndBufferTiles(gLinkPlayers[id ^ 1].name, sMenuTextTileBuffers[GFXTAG_PARTNER_NAME_L], 0, 0, 3);
        DrawTextWindowAndBufferTiles(sActionTexts[TEXT_CANCEL], sMenuTextTileBuffers[GFXTAG_CANCEL_L], 0, 0, 2);
        DrawBottomRowText(sActionTexts[TEXT_CHOOSE_MON], sMenuTextTileBuffers[GFXTAG_CHOOSE_PKMN_L], 24);
        gMain.state++;
        sTradeMenu->timer = 0;
        break;
    case 11:
        if (LoadUISpriteGfx())
            gMain.state++;
        break;
    case 12:
        // Create player's name text sprites
        xPos = GetStringCenterAlignXOffset(FONT_NORMAL, gSaveBlock2Ptr->playerName, 120);
        for (i = 0; i < NUM_PLAYER_NAME_SPRITES; i++)
        {
            temp = sSpriteTemplate_MenuText;
            temp.tileTag += i + GFXTAG_PLAYER_NAME_L;
            CreateSprite(&temp, xPos + (i * 32) + 16, 10, 1);
        }

        // Create partner's name text sprites
        xPos = GetStringCenterAlignXOffset(FONT_NORMAL, gLinkPlayers[GetMultiplayerId() ^ 1].name, 120);
        for (i = 0; i < NUM_PARTNER_NAME_SPRITES; i++)
        {
            temp = sSpriteTemplate_MenuText;
            temp.tileTag += i + GFXTAG_PARTNER_NAME_L;
            CreateSprite(&temp, xPos + (i * 32) + 136, 10, 1);
        }
        gMain.state++;
        break;
    case 13:
        // Create Cancel text sprites
        temp = sSpriteTemplate_MenuText;
        temp.tileTag += GFXTAG_CANCEL_L;
        CreateSprite(&temp, 215, 152, 1);
        temp = sSpriteTemplate_MenuText;
        temp.tileTag += GFXTAG_CANCEL_R;
        CreateSprite(&temp, 215 + 32, 152, 1);

        // Create Choose a Pokémon text sprites (only 3 are needed, other 3 are empty)
        for (i = 0; i < NUM_CHOOSE_PKMN_SPRITES; i++)
        {
            temp = sSpriteTemplate_MenuText;
            temp.tileTag += i + GFXTAG_CHOOSE_PKMN_L;
            CreateSprite(&temp, (i * 32) + 24, 150, 1);
        }

        sTradeMenu->cursorSpriteId = CreateSprite(&sSpriteTemplate_Cursor, sTradeMonSpriteCoords[0][0] * 8 + 32, sTradeMonSpriteCoords[0][1] * 8, 2);
        sTradeMenu->cursorPosition = 0;
        gMain.state++;
        rbox_fill_rectangle(0);
        break;
    case 14:
        ComputePartyTradeableFlags(TRADE_PLAYER);
        PrintPartyNicknames(TRADE_PLAYER);
        sTradeMenu->bg2hofs = 0;
        sTradeMenu->bg3hofs = 0;
        SetActiveMenuOptions();
        gMain.state++;
        PlayBGM(MUS_SCHOOL);
        break;
    case 15:
        ComputePartyTradeableFlags(TRADE_PARTNER);
        PrintPartyNicknames(TRADE_PARTNER);
        gMain.state++;
        // fallthrough
    case 16:
        LoadTradeBgGfx(0);
        gMain.state++;
        break;
    case 17:
        LoadTradeBgGfx(1);
        gMain.state++;
        break;
    case 18:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    case 19:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
        LoadTradeBgGfx(2);
        gMain.state++;
        break;
    case 20:
        ComputePartyHPBarLevels(TRADE_PLAYER);
        gMain.state++;
        break;
    case 21:
        ComputePartyHPBarLevels(TRADE_PARTNER);
        SetTradePartyHPBarSprites();
        gMain.state++;
        break;
    case 22:
        if (!gPaletteFade.active)
        {
            gMain.callback1 = CB1_UpdateLink;
            SetMainCallback2(CB2_TradeMenu);
        }
        break;
    }

    RunTextPrinters();
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void CB2_ReturnToTradeMenu(void)
{
    int i;
    struct SpriteTemplate temp;
    u8 id;
    u32 xPos;

    switch (gMain.state)
    {
    case 0:
        InitTradeMenu();
        gMain.state++;
        break;
    case 1:
        gMain.state++;
        sTradeMenu->timer = 0;
        break;
    case 2:
        gMain.state++;
        break;
    case 3:
        gMain.state++;
        break;
    case 4:
        CalculatePlayerPartyCount();
        gMain.state++;
        break;
    case 5:
        if (gWirelessCommType)
        {
            LoadWirelessStatusIndicatorSpriteGfx();
            CreateWirelessStatusIndicatorSprite(0, 0);
        }
        gMain.state++;
        break;
    case 6:
        gMain.state++;
        break;
    case 7:
        CalculateEnemyPartyCount();
        sTradeMenu->partyCounts[TRADE_PLAYER] = gPlayerPartyCount;
        sTradeMenu->partyCounts[TRADE_PARTNER] = gEnemyPartyCount;
        ClearWindowTilemap(0);
        PrintPartyNicknames(TRADE_PLAYER);
        PrintPartyNicknames(TRADE_PARTNER);

        for (i = 0; i < sTradeMenu->partyCounts[TRADE_PLAYER]; i++)
        {
            struct Pokemon *mon = &gPlayerParty[i];
            sTradeMenu->partySpriteIds[TRADE_PLAYER][i] = CreateMonIcon(GetMonData(mon, MON_DATA_SPECIES_OR_EGG, NULL),
                                                         SpriteCB_MonIcon,
                                                         (sTradeMonSpriteCoords[i][0] * 8) + 14,
                                                         (sTradeMonSpriteCoords[i][1] * 8) - 12,
                                                         1,
                                                         GetMonData(mon, MON_DATA_PERSONALITY),
                                                         TRUE);
        }

        for (i = 0; i < sTradeMenu->partyCounts[TRADE_PARTNER]; i++)
        {
            struct Pokemon *mon = &gEnemyParty[i];
            sTradeMenu->partySpriteIds[TRADE_PARTNER][i] = CreateMonIcon(GetMonData(mon, MON_DATA_SPECIES_OR_EGG, NULL),
                                                         SpriteCB_MonIcon,
                                                         (sTradeMonSpriteCoords[i + PARTY_SIZE][0] * 8) + 14,
                                                         (sTradeMonSpriteCoords[i + PARTY_SIZE][1] * 8) - 12,
                                                         1,
                                                         GetMonData(mon, MON_DATA_PERSONALITY),
                                                         FALSE);
        }
        gMain.state++;
        break;
    case 8:
        LoadHeldItemIcons();
        DrawHeldItemIconsForTrade(&sTradeMenu->partyCounts[0], sTradeMenu->partySpriteIds[0], TRADE_PLAYER);
        gMain.state++;
        break;
    case 9:
        DrawHeldItemIconsForTrade(&sTradeMenu->partyCounts[0], sTradeMenu->partySpriteIds[0], TRADE_PARTNER);
        gMain.state++;
        break;
    case 10:
        DrawTextWindowAndBufferTiles(gSaveBlock2Ptr->playerName, sMenuTextTileBuffers[GFXTAG_PLAYER_NAME_L], 0, 0, 3);
        id = GetMultiplayerId();
        DrawTextWindowAndBufferTiles(gLinkPlayers[id ^ 1].name, sMenuTextTileBuffers[GFXTAG_PARTNER_NAME_L], 0, 0, 3);
        DrawTextWindowAndBufferTiles(sActionTexts[TEXT_CANCEL], sMenuTextTileBuffers[GFXTAG_CANCEL_L], 0, 0, 2);
        DrawBottomRowText(sActionTexts[TEXT_CHOOSE_MON], sMenuTextTileBuffers[GFXTAG_CHOOSE_PKMN_L], 24);
        gMain.state++;
        sTradeMenu->timer = 0;
        break;
    case 11:
        if (LoadUISpriteGfx())
            gMain.state++;
        break;
    case 12:
        // Create player's name text sprites
        xPos = GetStringCenterAlignXOffset(FONT_NORMAL, gSaveBlock2Ptr->playerName, 120);
        for (i = 0; i < NUM_PLAYER_NAME_SPRITES; i++)
        {
            temp = sSpriteTemplate_MenuText;
            temp.tileTag += i + GFXTAG_PLAYER_NAME_L;
            CreateSprite(&temp, xPos + (i * 32) + 16, 10, 1);
        }

        // Create partner's name text sprites
        xPos = GetStringCenterAlignXOffset(FONT_NORMAL, gLinkPlayers[GetMultiplayerId() ^ 1].name, 120);
        for (i = 0; i < NUM_PARTNER_NAME_SPRITES; i++)
        {
            temp = sSpriteTemplate_MenuText;
            temp.tileTag += i + GFXTAG_PARTNER_NAME_L;
            CreateSprite(&temp, xPos + (i * 32) + 136, 10, 1);
        }
        gMain.state++;
        break;
    case 13:
        // Create Cancel text sprites
        temp = sSpriteTemplate_MenuText;
        temp.tileTag += GFXTAG_CANCEL_L;
        CreateSprite(&temp, 215, 152, 1);
        temp = sSpriteTemplate_MenuText;
        temp.tileTag += GFXTAG_CANCEL_R;
        CreateSprite(&temp, 215 + 32, 152, 1);

        // Create Choose a Pokémon text sprites
        for (i = 0; i < NUM_CHOOSE_PKMN_SPRITES; i++)
        {
            temp = sSpriteTemplate_MenuText;
            temp.tileTag += i + GFXTAG_CHOOSE_PKMN_L;
            CreateSprite(&temp, (i * 32) + 24, 150, 1);
        }

        if (sTradeMenu->cursorPosition < PARTY_SIZE)
            sTradeMenu->cursorPosition = gLastViewedMonIndex;
        else
            sTradeMenu->cursorPosition = gLastViewedMonIndex + PARTY_SIZE;

        sTradeMenu->cursorSpriteId = CreateSprite(&sSpriteTemplate_Cursor,
                                                     sTradeMonSpriteCoords[sTradeMenu->cursorPosition][0] * 8 + 32,
                                                     sTradeMonSpriteCoords[sTradeMenu->cursorPosition][1] * 8, 2);
        gMain.state = 16;
        break;
    case 16:
        LoadTradeBgGfx(0);
        gMain.state++;
        break;
    case 17:
        LoadTradeBgGfx(1);
        sTradeMenu->bg2hofs = 0;
        sTradeMenu->bg3hofs = 0;
        SetActiveMenuOptions();
        gMain.state++;
        break;
    case 18:
        gPaletteFade.bufferTransferDisabled = FALSE;
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    case 19:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
        LoadTradeBgGfx(2);
        gMain.state++;
        break;
    case 20:
        gMain.state++;
        break;
    case 21:
        SetTradePartyHPBarSprites();
        gMain.state++;
        break;
    case 22:
        if (!gPaletteFade.active)
            SetMainCallback2(CB2_TradeMenu);
        break;
    }

    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_TradeMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB_FadeToStartTrade(void)
{
    if (++sTradeMenu->timer > 15)
    {
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sTradeMenu->callbackId = CB_WAIT_TO_START_TRADE;
    }
}

static void CB_WaitToStartTrade(void)
{
    if (!gPaletteFade.active)
    {
        gSelectedTradeMonPositions[TRADE_PLAYER] = sTradeMenu->cursorPosition;
        gSelectedTradeMonPositions[TRADE_PARTNER] = sTradeMenu->partnerCursorPosition;

        if (gWirelessCommType)
        {
            sTradeMenu->callbackId = CB_WAIT_TO_START_RFU_TRADE;
        }
        else
        {
            SetCloseLinkCallbackAndType(32);
            sTradeMenu->callbackId = CB_START_LINK_TRADE;
        }
    }
}

static void CB_StartLinkTrade(void)
{
    gMain.savedCallback = CB2_StartCreateTradeMenu;

    if (gWirelessCommType)
    {
        // Wireless Link Trade
        if (IsLinkRfuTaskFinished())
        {
            Free(sMenuTextTileBuffer);
            FreeAllWindowBuffers();
            Free(sTradeMenu);
            gMain.callback1 = NULL;
            DestroyWirelessStatusIndicatorSprite();
            SetMainCallback2(CB2_LinkTrade);
        }
    }
    else
    {
        // Cable Link Trade
        if (!gReceivedRemoteLinkPlayers)
        {
            Free(sMenuTextTileBuffer);
            FreeAllWindowBuffers();
            Free(sTradeMenu);
            gMain.callback1 = NULL;
            SetMainCallback2(CB2_LinkTrade);
        }
    }
}

static void CB2_TradeMenu(void)
{
    RunTradeMenuCallback();
    DoQueuedActions();

    // As long as drawSelectedMonState is 0, these do nothing
    DrawSelectedMonScreen(TRADE_PLAYER);
    DrawSelectedMonScreen(TRADE_PARTNER);

    SetGpuReg(REG_OFFSET_BG2HOFS, sTradeMenu->bg2hofs++);
    SetGpuReg(REG_OFFSET_BG3HOFS, sTradeMenu->bg3hofs--);

    RunTextPrintersAndIsPrinter0Active();
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void LoadTradeBgGfx(u8 state)
{
    int i;

    switch (state)
    {
    case 0:
        LoadPalette(gTradeMenu_Pal, BG_PLTT_ID(0), 3 * PLTT_SIZE_4BPP);
        LoadBgTiles(1, gTradeMenu_Gfx, 0x1280, 0);
        CopyToBgTilemapBufferRect_ChangePalette(1, gTradeMenu_Tilemap, 0, 0, 32, 20, 0);
        LoadBgTilemap(2, sTradeStripesBG2Tilemap, 0x800, 0);
        break;
    case 1:
        LoadBgTilemap(3, sTradeStripesBG3Tilemap, 0x800, 0);
        PrintPartyLevelsAndGenders(TRADE_PLAYER);
        PrintPartyLevelsAndGenders(TRADE_PARTNER);
        CopyBgTilemapBufferToVram(1);
        break;
    case 2:
        for (i = 0; i < 4; i++)
            SetGpuReg(REG_OFFSET_BG0HOFS + (i * 2), 0);
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        ShowBg(3);
        break;
    }
}

// Determine (based on party counts) where the main menu cursor can go
static void SetActiveMenuOptions(void)
{
    int i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (i < sTradeMenu->partyCounts[TRADE_PLAYER])
        {
            // Present player Pokémon
            gSprites[sTradeMenu->partySpriteIds[TRADE_PLAYER][i]].invisible = FALSE;
            sTradeMenu->optionsActive[i] = TRUE;
        }
        else
        {
            // Absent player Pokémon
            sTradeMenu->optionsActive[i] = FALSE;
        }

        if (i < sTradeMenu->partyCounts[TRADE_PARTNER])
        {
            // Present partner Pokémon
            gSprites[sTradeMenu->partySpriteIds[TRADE_PARTNER][i]].invisible = FALSE;
            sTradeMenu->optionsActive[i + PARTY_SIZE] = TRUE;
        }
        else
        {
            // Absent partner Pokémon
            sTradeMenu->optionsActive[i + PARTY_SIZE] = FALSE;
        }
    }

    // Cancel is always active
    sTradeMenu->optionsActive[PARTY_SIZE * 2] = TRUE;
}

// why not just use memcpy?
static void Trade_Memcpy(void *dest, const void *src, u32 size)
{
    u8 *_dest = dest;
    const u8 *_src = src;
    u32 i;
    for (i = 0; i < size; i++)
        _dest[i] = _src[i];
}

static bool8 BufferTradeParties(void)
{
    u8 id = GetMultiplayerId();
    int i;
    struct Pokemon *mon;

    switch (sTradeMenu->bufferPartyState)
    {
    case 0:
        // The parties are sent in pairs rather than all at once
        Trade_Memcpy(gBlockSendBuffer, &gPlayerParty[0], 2 * sizeof(struct Pokemon));
        sTradeMenu->bufferPartyState++;
        sTradeMenu->timer = 0;
        break;
    case 1:
        if (IsLinkTradeTaskFinished())
        {
            if (_GetBlockReceivedStatus() == 0)
            {
                sTradeMenu->bufferPartyState++;
            }
            else
            {
                TradeResetReceivedFlags();
                sTradeMenu->bufferPartyState++;
            }
        }
        break;
    case 3:
        if (id == 0)
            RequestLinkData(BLOCK_REQ_SIZE_200);
        sTradeMenu->bufferPartyState++;
        break;
    case 4:
        if (_GetBlockReceivedStatus() == 3)
        {
            Trade_Memcpy(&gEnemyParty[0], gBlockRecvBuffer[id ^ 1], 2 * sizeof(struct Pokemon));
            TradeResetReceivedFlags();
            sTradeMenu->bufferPartyState++;
        }
        break;
    case 5:
        Trade_Memcpy(gBlockSendBuffer, &gPlayerParty[2], 2 * sizeof(struct Pokemon));
        sTradeMenu->bufferPartyState++;
        break;
    case 7:
        if (id == 0)
            RequestLinkData(BLOCK_REQ_SIZE_200);
        sTradeMenu->bufferPartyState++;
        break;
    case 8:
        if (_GetBlockReceivedStatus() == 3)
        {
            Trade_Memcpy(&gEnemyParty[2], gBlockRecvBuffer[id ^ 1],  2 * sizeof(struct Pokemon));
            TradeResetReceivedFlags();
            sTradeMenu->bufferPartyState++;
        }
        break;
    case 9:
        Trade_Memcpy(gBlockSendBuffer, &gPlayerParty[4], 2 * sizeof(struct Pokemon));
        sTradeMenu->bufferPartyState++;
        break;
    case 11:
        if (id == 0)
            RequestLinkData(BLOCK_REQ_SIZE_200);
        sTradeMenu->bufferPartyState++;
        break;
    case 12:
        if (_GetBlockReceivedStatus() == 3)
        {
            Trade_Memcpy(&gEnemyParty[4], gBlockRecvBuffer[id ^ 1], 2 * sizeof(struct Pokemon));
            TradeResetReceivedFlags();
            sTradeMenu->bufferPartyState++;
        }
        break;
    case 13:
        Trade_Memcpy(gBlockSendBuffer, gSaveBlock1Ptr->mail, PARTY_SIZE * sizeof(struct Mail) + 4);
        sTradeMenu->bufferPartyState++;
        break;
    case 15:
        if (id == 0)
            RequestLinkData(BLOCK_REQ_SIZE_220);
        sTradeMenu->bufferPartyState++;
        break;
    case 16:
        if (_GetBlockReceivedStatus() == 3)
        {
            Trade_Memcpy(gTradeMail, gBlockRecvBuffer[id ^ 1], PARTY_SIZE * sizeof(struct Mail));
            TradeResetReceivedFlags();
            sTradeMenu->bufferPartyState++;
        }
        break;
    case 17:
        Trade_Memcpy(gBlockSendBuffer, gSaveBlock1Ptr->giftRibbons, sizeof(sTradeMenu->giftRibbons));
        sTradeMenu->bufferPartyState++;
        break;
    case 19:
        if (id == 0)
            RequestLinkData(BLOCK_REQ_SIZE_40);
        sTradeMenu->bufferPartyState++;
        break;
    case 20:
        if (_GetBlockReceivedStatus() == 3)
        {
            Trade_Memcpy(sTradeMenu->giftRibbons, gBlockRecvBuffer[id ^ 1], sizeof(sTradeMenu->giftRibbons));
            TradeResetReceivedFlags();
            sTradeMenu->bufferPartyState++;
        }
        break;
    case 21:
        for (i = 0, mon = gEnemyParty; i < PARTY_SIZE; mon++, i++)
        {
            u8 name[POKEMON_NAME_LENGTH + 1];
            u16 species = GetMonData(mon, MON_DATA_SPECIES);

            if (species != SPECIES_NONE)
            {
                if (species == SPECIES_SHEDINJA && GetMonData(mon, MON_DATA_LANGUAGE) != LANGUAGE_JAPANESE)
                {
                    GetMonData(mon, MON_DATA_NICKNAME, name);

                    if (!StringCompareWithoutExtCtrlCodes(name, sText_ShedinjaJP))
                        SetMonData(mon, MON_DATA_NICKNAME, gSpeciesNames[SPECIES_SHEDINJA]);
                }
            }
        }
        return TRUE;
    // Delay until next state
    case 2:
    case 6:
    case 10:
    case 14:
    case 18:
        sTradeMenu->timer++;
        if (sTradeMenu->timer > 10)
        {
            sTradeMenu->timer = 0;
            sTradeMenu->bufferPartyState++;
        }
        break;
    }
    return FALSE;
}

static void PrintIsThisTradeOkay(void)
{
    DrawBottomRowText(sText_IsThisTradeOkay, (void *)(OBJ_VRAM0 + (sTradeMenu->bottomTextTileStart * 32)), 24);
}

static void Leader_ReadLinkBuffer(u8 mpId, u8 status)
{
    if (status & 1)
    {
        switch (gBlockRecvBuffer[0][0])
        {
        case LINKCMD_REQUEST_CANCEL:
            sTradeMenu->playerSelectStatus = STATUS_CANCEL;
            break;
        case LINKCMD_READY_TO_TRADE:
            sTradeMenu->playerSelectStatus = STATUS_READY;
            break;
        case LINKCMD_INIT_BLOCK:
            sTradeMenu->playerConfirmStatus = STATUS_READY;
            break;
        case LINKCMD_READY_CANCEL_TRADE:
            sTradeMenu->playerConfirmStatus = STATUS_CANCEL;
            break;
        }
        TradeResetReceivedFlag(0);
    }

    if (status & 2)
    {
        switch (gBlockRecvBuffer[1][0])
        {
        case LINKCMD_REQUEST_CANCEL:
            sTradeMenu->partnerSelectStatus = STATUS_CANCEL;
            break;
        case LINKCMD_READY_TO_TRADE:
            sTradeMenu->partnerCursorPosition = gBlockRecvBuffer[1][1] + PARTY_SIZE;
            sTradeMenu->partnerSelectStatus = STATUS_READY;
            break;
        case LINKCMD_INIT_BLOCK:
            sTradeMenu->partnerConfirmStatus = STATUS_READY;
            break;
        case LINKCMD_READY_CANCEL_TRADE:
            sTradeMenu->partnerConfirmStatus = STATUS_CANCEL;
            break;
        }
        TradeResetReceivedFlag(1);
    }
}

static void Follower_ReadLinkBuffer(u8 mpId, u8 status)
{
    if (status & 1)
    {
        switch (gBlockRecvBuffer[0][0])
        {
        case LINKCMD_BOTH_CANCEL_TRADE:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            PrintTradeMessage(MSG_WAITING_FOR_FRIEND);
            sTradeMenu->callbackId = CB_INIT_EXIT_CANCELED_TRADE;
            break;
        case LINKCMD_PARTNER_CANCEL_TRADE:
            PrintTradeMessage(MSG_FRIEND_WANTS_TO_TRADE);
            sTradeMenu->callbackId = CB_HANDLE_TRADE_CANCELED;
            break;
        case LINKCMD_SET_MONS_TO_TRADE:
            sTradeMenu->partnerCursorPosition = gBlockRecvBuffer[0][1] + PARTY_SIZE;
            rbox_fill_rectangle(0);
            SetSelectedMon(sTradeMenu->cursorPosition);
            SetSelectedMon(sTradeMenu->partnerCursorPosition);
            sTradeMenu->callbackId = CB_PRINT_IS_THIS_OKAY;
            break;
        case LINKCMD_START_TRADE:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            sTradeMenu->callbackId = CB_WAIT_TO_START_TRADE;
            break;
        case LINKCMD_PLAYER_CANCEL_TRADE:
            PrintTradeMessage(MSG_CANCELED);
            sTradeMenu->callbackId = CB_HANDLE_TRADE_CANCELED;
        }
        TradeResetReceivedFlag(0);
    }

    if (status & 2)
        TradeResetReceivedFlag(1);
}

static void Leader_HandleCommunication(void)
{
    if (sTradeMenu->playerSelectStatus != STATUS_NONE
     && sTradeMenu->partnerSelectStatus != STATUS_NONE)
    {
        if (sTradeMenu->playerSelectStatus == STATUS_READY
         && sTradeMenu->partnerSelectStatus == STATUS_READY)
        {
            // Both players have selected a Pokémon to trade
            sTradeMenu->callbackId = CB_SET_SELECTED_MONS;
            sTradeMenu->linkData[0] = LINKCMD_SET_MONS_TO_TRADE;
            sTradeMenu->linkData[1] = sTradeMenu->cursorPosition;
            QueueAction(QUEUE_DELAY_DATA, QUEUE_SEND_DATA);
            sTradeMenu->playerSelectStatus = sTradeMenu->partnerSelectStatus = STATUS_NONE;
        }
        else if (sTradeMenu->playerSelectStatus == STATUS_READY
              && sTradeMenu->partnerSelectStatus == STATUS_CANCEL)
        {
            // The player has selected a Pokémon to trade,
            // but the partner has selected Cancel
            PrintTradeMessage(MSG_CANCELED);
            sTradeMenu->linkData[0] = LINKCMD_PARTNER_CANCEL_TRADE;
            sTradeMenu->linkData[1] = 0;
            QueueAction(QUEUE_DELAY_DATA, QUEUE_SEND_DATA);
            sTradeMenu->playerConfirmStatus = sTradeMenu->partnerConfirmStatus = STATUS_NONE;
            sTradeMenu->playerSelectStatus = sTradeMenu->partnerSelectStatus = STATUS_NONE;
            sTradeMenu->callbackId = CB_HANDLE_TRADE_CANCELED;
        }
        else if (sTradeMenu->playerSelectStatus == STATUS_CANCEL
              && sTradeMenu->partnerSelectStatus == STATUS_READY)
        {
            // The partner has selected a Pokémon to trade,
            // but the player has selected cancel
            PrintTradeMessage(MSG_FRIEND_WANTS_TO_TRADE);
            sTradeMenu->linkData[0] = LINKCMD_PLAYER_CANCEL_TRADE;
            sTradeMenu->linkData[1] = 0;
            QueueAction(QUEUE_DELAY_DATA, QUEUE_SEND_DATA);
            sTradeMenu->playerConfirmStatus = sTradeMenu->partnerConfirmStatus = STATUS_NONE;
            sTradeMenu->playerSelectStatus = sTradeMenu->partnerSelectStatus = STATUS_NONE;
            sTradeMenu->callbackId = CB_HANDLE_TRADE_CANCELED;
        }
        else if (sTradeMenu->playerSelectStatus == STATUS_CANCEL
              && sTradeMenu->partnerSelectStatus == STATUS_CANCEL)
        {
            // Both players have selected Cancel
            sTradeMenu->linkData[0] = LINKCMD_BOTH_CANCEL_TRADE;
            sTradeMenu->linkData[1] = 0;
            QueueAction(QUEUE_DELAY_DATA, QUEUE_SEND_DATA);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            sTradeMenu->playerSelectStatus = sTradeMenu->partnerSelectStatus = STATUS_NONE;
            sTradeMenu->callbackId = CB_INIT_EXIT_CANCELED_TRADE;
        }
    }

    if (sTradeMenu->playerConfirmStatus != STATUS_NONE
     && sTradeMenu->partnerConfirmStatus != STATUS_NONE)
    {
        if (sTradeMenu->playerConfirmStatus == STATUS_READY
         && sTradeMenu->partnerConfirmStatus == STATUS_READY)
        {
            // Both players have confirmed trade
            sTradeMenu->linkData[0] = LINKCMD_START_TRADE;
            sTradeMenu->linkData[1] = 0;
            QueueAction(QUEUE_DELAY_DATA, QUEUE_SEND_DATA);
            sTradeMenu->playerConfirmStatus = STATUS_NONE;
            sTradeMenu->partnerConfirmStatus = STATUS_NONE;
            sTradeMenu->callbackId = CB_FADE_TO_START_TRADE;
        }

        if (sTradeMenu->playerConfirmStatus == STATUS_CANCEL
         || sTradeMenu->partnerConfirmStatus == STATUS_CANCEL)
        {
            // One of the players has decided not to confirm the trade,
            // or the trade was not allowed.
            PrintTradeMessage(MSG_CANCELED);
            sTradeMenu->linkData[0] = LINKCMD_PLAYER_CANCEL_TRADE;
            sTradeMenu->linkData[1] = 0;
            QueueAction(QUEUE_DELAY_DATA, QUEUE_SEND_DATA);
            sTradeMenu->playerConfirmStatus = STATUS_NONE;
            sTradeMenu->partnerConfirmStatus = STATUS_NONE;
            sTradeMenu->callbackId = CB_HANDLE_TRADE_CANCELED;
        }
    }
}

static void _SetLinkData(u16 *linkData, u16 linkCmd, u16 cursorPosition)
{
    linkData[0] = linkCmd;
    linkData[1] = cursorPosition;
    QueueAction(QUEUE_DELAY_DATA, QUEUE_SEND_DATA);
}

static void SetLinkData(u16 linkCmd, u16 cursorPosition)
{
    _SetLinkData(sTradeMenu->linkData, linkCmd, cursorPosition);
}

static void CB1_UpdateLink(void)
{
    u8 mpId = GetMultiplayerId();
    u8 status;

    if ((status = _GetBlockReceivedStatus()))
    {
        if (mpId == 0)
            Leader_ReadLinkBuffer(mpId, status);
        else
            Follower_ReadLinkBuffer(mpId, status);
    }

    if (mpId == 0)
        Leader_HandleCommunication();
}

static u8 GetNewCursorPosition(u8 oldPosition, u8 direction)
{
    int i;
    u8 newPosition = 0;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (sTradeMenu->optionsActive[sCursorMoveDestinations[oldPosition][direction][i]] == TRUE)
        {
            newPosition = sCursorMoveDestinations[oldPosition][direction][i];
            break;
        }
    }

    return newPosition;
}

static void TradeMenuMoveCursor(u8 *cursorPosition, u8 direction)
{
    u8 newPosition = GetNewCursorPosition(*cursorPosition, direction);

    if (newPosition == (PARTY_SIZE * 2)) // CANCEL
    {
        StartSpriteAnim(&gSprites[sTradeMenu->cursorSpriteId], CURSOR_ANIM_ON_CANCEL);
        gSprites[sTradeMenu->cursorSpriteId].x = DISPLAY_WIDTH - 16;
        gSprites[sTradeMenu->cursorSpriteId].y = DISPLAY_HEIGHT;
    }
    else
    {
        StartSpriteAnim(&gSprites[sTradeMenu->cursorSpriteId], CURSOR_ANIM_NORMAL);
        gSprites[sTradeMenu->cursorSpriteId].x = sTradeMonSpriteCoords[newPosition][0] * 8 + 32;
        gSprites[sTradeMenu->cursorSpriteId].y = sTradeMonSpriteCoords[newPosition][1] * 8;
    }

    if (*cursorPosition != newPosition)
        PlaySE(SE_SELECT);

    *cursorPosition = newPosition;
}

static void SetReadyToTrade(void)
{
    PrintTradeMessage(MSG_STANDBY);
    sTradeMenu->callbackId = CB_IDLE;

    if (GetMultiplayerId() == 1)
    {
        // Communicate to the link leader that we're ready to trade
        SetLinkData(LINKCMD_READY_TO_TRADE, sTradeMenu->cursorPosition);
    }
    else
    {
        // We are the link leader, no communication necessary
        sTradeMenu->playerSelectStatus = STATUS_READY;
    }
}

static void CB_ProcessMenuInput(void)
{
    if (JOY_REPEAT(DPAD_UP))
        TradeMenuMoveCursor(&sTradeMenu->cursorPosition, 0);
    else if (JOY_REPEAT(DPAD_DOWN))
        TradeMenuMoveCursor(&sTradeMenu->cursorPosition, 1);
    else if (JOY_REPEAT(DPAD_LEFT))
        TradeMenuMoveCursor(&sTradeMenu->cursorPosition, 2);
    else if (JOY_REPEAT(DPAD_RIGHT))
        TradeMenuMoveCursor(&sTradeMenu->cursorPosition, 3);

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);

        if (sTradeMenu->cursorPosition < PARTY_SIZE)
        {
            // Selected Pokémon in player's party
            DrawTextBorderOuter(1, 1, 14);
            FillWindowPixelBuffer(1, PIXEL_FILL(1));
            PrintMenuTable(1, ARRAY_COUNT(sSelectTradeMonActions), sSelectTradeMonActions);
            InitMenuInUpperLeftCornerNormal(1, ARRAY_COUNT(sSelectTradeMonActions), 0);
            PutWindowTilemap(1);
            CopyWindowToVram(1, COPYWIN_FULL);
            sTradeMenu->callbackId = CB_SELECTED_MON;
        }
        else if (sTradeMenu->cursorPosition < PARTY_SIZE * 2)
        {
            // Selected Pokémon in partner's party
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            sTradeMenu->callbackId = CB_SHOW_MON_SUMMARY;
        }
        else if (sTradeMenu->cursorPosition == PARTY_SIZE * 2)
        {
            // Selected Cancel
            CreateYesNoMenu(&sTradeYesNoWindowTemplate, 1, 14, 0);
            sTradeMenu->callbackId = CB_CANCEL_TRADE_PROMPT;
            DrawBottomRowText(sActionTexts[TEXT_CANCEL_TRADE], (void *)(OBJ_VRAM0 + sTradeMenu->bottomTextTileStart * 32), 24);
        }
    }
}

static void RedrawChooseAPokemonWindow(void)
{
    PrintTradePartnerPartyNicknames();
    sTradeMenu->callbackId = CB_MAIN_MENU;
    gSprites[sTradeMenu->cursorSpriteId].invisible = FALSE;
    DrawBottomRowText(sActionTexts[TEXT_CHOOSE_MON], (void *)(OBJ_VRAM0 + sTradeMenu->bottomTextTileStart * 32), 24);
}

static void CB_ProcessSelectedMonInput(void)
{
    switch (Menu_ProcessInputNoWrap())
    {
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        RedrawChooseAPokemonWindow();
        break;
    case MENU_NOTHING_CHOSEN:
        break;
    case 0: // Summary
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sTradeMenu->callbackId = CB_SHOW_MON_SUMMARY;
        break;
    case 1: // Trade
        switch (CanTradeSelectedMon(gPlayerParty, gPlayerPartyCount, sTradeMenu->cursorPosition))
        {
        case CAN_TRADE_MON:
            SetReadyToTrade();
            gSprites[sTradeMenu->cursorSpriteId].invisible = TRUE;
            break;
        case CANT_TRADE_LAST_MON:
            QueueAction(QUEUE_DELAY_MSG, QUEUE_ONLY_MON2);
            sTradeMenu->callbackId = CB_HANDLE_TRADE_CANCELED;
            break;
        case CANT_TRADE_NATIONAL:
        case CANT_TRADE_INVALID_MON:
            QueueAction(QUEUE_DELAY_MSG, QUEUE_MON_CANT_BE_TRADED);
            sTradeMenu->callbackId = CB_HANDLE_TRADE_CANCELED;
            break;
        case CANT_TRADE_EGG_YET:
        case CANT_TRADE_PARTNER_EGG_YET:
            QueueAction(QUEUE_DELAY_MSG, QUEUE_EGG_CANT_BE_TRADED);
            sTradeMenu->callbackId = CB_HANDLE_TRADE_CANCELED;
            break;
        }
        break;
    }
}

static void CB_ChooseMonAfterButtonPress(void)
{
    if (JOY_NEW(A_BUTTON) || JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        RedrawChooseAPokemonWindow();
    }
}

static void CB_ShowTradeMonSummaryScreen(void)
{
    if (!gPaletteFade.active)
    {
        if (sTradeMenu->cursorPosition < PARTY_SIZE)
            ShowPokemonSummaryScreen(SUMMARY_MODE_LOCK_MOVES, gPlayerParty, sTradeMenu->cursorPosition, sTradeMenu->partyCounts[TRADE_PLAYER] - 1, CB2_ReturnToTradeMenu);
        else
            ShowPokemonSummaryScreen(SUMMARY_MODE_LOCK_MOVES, gEnemyParty, sTradeMenu->cursorPosition - PARTY_SIZE, sTradeMenu->partyCounts[TRADE_PARTNER] - 1, CB2_ReturnToTradeMenu);
        FreeAllWindowBuffers();
    }
}

static u8 CheckValidityOfTradeMons(u8 *aliveMons, u8 playerPartyCount, u8 playerMonIdx, u8 partnerMonIdx)
{
    int i;
    u16 partnerSpecies;
    u8 hasLiveMon = 0;

    // Make sure mon to be traded isn't player's last alive mon
    for (i = 0; i < playerPartyCount; i++)
    {
        if (playerMonIdx != i)
            hasLiveMon += aliveMons[i];
    }
    partnerMonIdx %= PARTY_SIZE;
    partnerSpecies = GetMonData(&gEnemyParty[partnerMonIdx], MON_DATA_SPECIES);

    // Partner cant trade illegitimate Deoxys or Mew
    if (partnerSpecies == SPECIES_DEOXYS || partnerSpecies == SPECIES_MEW)
    {
        if (!GetMonData(&gEnemyParty[partnerMonIdx], MON_DATA_MODERN_FATEFUL_ENCOUNTER))
            return PARTNER_MON_INVALID;
    }

    // Partner cant trade Egg or non-Hoenn mon if player doesn't have National Dex
    if (!IsNationalPokedexEnabled())
    {
        if (sTradeMenu->isEgg[TRADE_PARTNER][partnerMonIdx] || !IsSpeciesInHoennDex(partnerSpecies))
            return PARTNER_MON_INVALID;
    }

    if (hasLiveMon)
        hasLiveMon = BOTH_MONS_VALID;

    return hasLiveMon; //PLAYER_MON_INVALID or BOTH_MONS_VALID
}

// Returns TRUE if the partner's selected mon is invalid, FALSE otherwise
static bool32 CheckMonsBeforeTrade(void)
{
    int i;
    u8 aliveMons[PARTY_SIZE * 2];

    for (i = 0; i < sTradeMenu->partyCounts[TRADE_PLAYER]; i++)
        aliveMons[i] = sTradeMenu->isLiveMon[TRADE_PLAYER][i];

    switch (CheckValidityOfTradeMons(aliveMons, sTradeMenu->partyCounts[TRADE_PLAYER],
                                                sTradeMenu->cursorPosition,
                                                sTradeMenu->partnerCursorPosition))
    {
    case PLAYER_MON_INVALID:
        QueueAction(QUEUE_DELAY_MSG, QUEUE_ONLY_MON2);
        SetLinkData(LINKCMD_READY_CANCEL_TRADE, 0);
        break;
    case BOTH_MONS_VALID:
        QueueAction(QUEUE_DELAY_MSG, QUEUE_STANDBY);
        SetLinkData(LINKCMD_INIT_BLOCK, 0);
        break;
    case PARTNER_MON_INVALID:
        QueueAction(QUEUE_DELAY_MSG, QUEUE_FRIENDS_MON_CANT_BE_TRADED);
        return TRUE;
    }

    return FALSE;
}

static void CB_ProcessConfirmTradeInput(void)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // YES, Confirm selection
        if (!CheckMonsBeforeTrade())
            sTradeMenu->callbackId = CB_IDLE;
        else
            sTradeMenu->callbackId = CB_PARTNER_MON_INVALID;

        PutWindowTilemap(17);
        break;
    case 1: // NO, Cancel Trade
    case MENU_B_PRESSED:
        QueueAction(QUEUE_DELAY_MSG, QUEUE_STANDBY);
        if (IsLinkTradeTaskFinished())
            SetLinkData(LINKCMD_READY_CANCEL_TRADE, 0);
        sTradeMenu->callbackId = CB_IDLE;
        PutWindowTilemap(17);
        break;
    }
}

// Only when choosing Yes to cancel, when No is chosen all are redrawn anyway
static void RestoreNicknamesCoveredByYesNo(void)
{
    int i;

    for (i = 0; i < sTradeMenu->partyCounts[1] - 4; i++)
    {
        PutWindowTilemap(i + PARTY_SIZE * 2);
        CopyWindowToVram(i + PARTY_SIZE * 2, COPYWIN_MAP);
    }
}

static void CB_ProcessCancelTradeInput(void)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // YES, Cancel
        PrintTradeMessage(MSG_WAITING_FOR_FRIEND);
        SetLinkData(LINKCMD_REQUEST_CANCEL, 0);
        gSprites[sTradeMenu->cursorSpriteId].invisible = TRUE;
        sTradeMenu->callbackId = CB_IDLE;
        RestoreNicknamesCoveredByYesNo();
        break;
    case 1: // NO, Continue
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        RedrawChooseAPokemonWindow();
        break;
    }
}

static void CB_SetSelectedMons(void)
{
    if (GetMultiplayerId() == 0)
    {
        rbox_fill_rectangle(0);
        SetSelectedMon(sTradeMenu->cursorPosition);
        SetSelectedMon(sTradeMenu->partnerCursorPosition);
    }
    sTradeMenu->callbackId = CB_PRINT_IS_THIS_OKAY;
}

static void CB_PrintIsThisTradeOkay(void)
{
    if (sTradeMenu->drawSelectedMonState[TRADE_PLAYER] == DRAW_SELECTED_FINISH
     && sTradeMenu->drawSelectedMonState[TRADE_PARTNER] == DRAW_SELECTED_FINISH)
    {
        PrintIsThisTradeOkay();
        sTradeMenu->callbackId = CB_INIT_CONFIRM_TRADE_PROMPT;
    }
}

static void CB_InitConfirmTradePrompt(void)
{
    sTradeMenu->timer++;
    if (sTradeMenu->timer > 120)
    {
        CreateYesNoMenu(&sTradeYesNoWindowTemplate, 1, 14, 0);
        sTradeMenu->timer = 0;
        sTradeMenu->callbackId = CB_CONFIRM_TRADE_PROMPT;
    }
}

static void CB_HandleTradeCanceled(void)
{
    int i;

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        rbox_fill_rectangle(0);
        rbox_fill_rectangle(1);

        for (i = 0; i < 4; i++)
        {
            FillWindowPixelBuffer(i + 14, PIXEL_FILL(0));
            rbox_fill_rectangle(i + 14);
        }

        RedrawPartyWindow(TRADE_PLAYER);
        RedrawPartyWindow(TRADE_PARTNER);
        sTradeMenu->callbackId = CB_MAIN_MENU;
        gSprites[sTradeMenu->cursorSpriteId].invisible = FALSE;
    }
}

static void CB_InitExitCanceledTrade(void)
{
    if (!gPaletteFade.active)
    {
        if (gWirelessCommType)
            SetLinkStandbyCallback();
        else
            SetCloseLinkCallbackAndType(12);

        sTradeMenu->callbackId = CB_EXIT_CANCELED_TRADE;
    }
}

static void CB_ExitCanceledTrade(void)
{
    if (gWirelessCommType)
    {
        if (IsLinkTradeTaskFinished() && GetNumQueuedActions() == 0)
        {
            Free(sMenuTextTileBuffer);
            Free(sTradeMenu);
            FreeAllWindowBuffers();
            DestroyWirelessStatusIndicatorSprite();
            SetMainCallback2(CB2_ReturnToFieldFromMultiplayer);
        }
    }
    else
    {
        if (!gReceivedRemoteLinkPlayers)
        {
            Free(sMenuTextTileBuffer);
            Free(sTradeMenu);
            FreeAllWindowBuffers();
            SetMainCallback2(CB2_ReturnToFieldFromMultiplayer);
        }
    }
}

static void CB_WaitToStartRfuTrade(void)
{
    if (!Rfu_SetLinkRecovery(FALSE) && GetNumQueuedActions() == 0)
    {
        SetLinkStandbyCallback();
        sTradeMenu->callbackId = CB_START_LINK_TRADE;
    }
}

static void CB_PartnersMonWasInvalid(void)
{
    if (JOY_NEW(A_BUTTON))
    {
        SetLinkData(LINKCMD_READY_CANCEL_TRADE, 0);
        sTradeMenu->callbackId = CB_IDLE;
    }
}

static void RunTradeMenuCallback(void)
{
    switch (sTradeMenu->callbackId)
    {
    case CB_MAIN_MENU:
        CB_ProcessMenuInput();
        break;
    case CB_SELECTED_MON:
        CB_ProcessSelectedMonInput();
        break;
    case CB_SHOW_MON_SUMMARY:
        CB_ShowTradeMonSummaryScreen();
        break;
    case CB_CONFIRM_TRADE_PROMPT:
        CB_ProcessConfirmTradeInput();
        break;
    case CB_CANCEL_TRADE_PROMPT:
        CB_ProcessCancelTradeInput();
        break;
    case CB_SET_SELECTED_MONS:
        CB_SetSelectedMons();
        break;
    case CB_PRINT_IS_THIS_OKAY:
        CB_PrintIsThisTradeOkay();
        break;
    case CB_HANDLE_TRADE_CANCELED:
        CB_HandleTradeCanceled();
        break;
    case CB_FADE_TO_START_TRADE:
        CB_FadeToStartTrade();
        break;
    case CB_WAIT_TO_START_TRADE:
        CB_WaitToStartTrade();
        break;
    case CB_INIT_EXIT_CANCELED_TRADE:
        CB_InitExitCanceledTrade();
        break;
    case CB_EXIT_CANCELED_TRADE:
        CB_ExitCanceledTrade();
        break;
    case CB_START_LINK_TRADE:
        CB_StartLinkTrade();
        break;
    case CB_INIT_CONFIRM_TRADE_PROMPT:
        CB_InitConfirmTradePrompt();
        break;
    case CB_UNUSED_CLOSE_MSG:
        CB_ChooseMonAfterButtonPress();
        break;
    case CB_WAIT_TO_START_RFU_TRADE:
        CB_WaitToStartRfuTrade();
        break;
    case CB_PARTNER_MON_INVALID:
        CB_PartnersMonWasInvalid();
        break;
  //case CB_IDLE: is nop
    }
}

static void SetSelectedMon(u8 cursorPosition)
{
    //cursorPosition 0-5 are the player's mons, 6-11 are the partner's
    u8 whichParty = cursorPosition / PARTY_SIZE;

    if (sTradeMenu->drawSelectedMonState[whichParty] == 0)
    {
        // Start the animation to display just the selected
        // Pokémon in the middle of the screen
        sTradeMenu->drawSelectedMonState[whichParty] = 1;
        sTradeMenu->selectedMonIdx[whichParty] = cursorPosition;
    }
}

static void DrawSelectedMonScreen(u8 whichParty)
{
    s8 nameStringWidth;
    u8 nickname[POKEMON_NAME_BUFFER_SIZE];
    u8 movesString[56];
    u8 i;
    u8 partyIdx;
    u8 selectedMonParty;
    u8 selectedMonIdx = sTradeMenu->selectedMonIdx[whichParty];

    selectedMonParty = TRADE_PARTNER;
    if (sTradeMenu->selectedMonIdx[whichParty] < PARTY_SIZE)
        selectedMonParty = TRADE_PLAYER;
    partyIdx = selectedMonIdx % PARTY_SIZE;
    nameStringWidth = 0;

    switch (sTradeMenu->drawSelectedMonState[whichParty])
    {
    default:
        // Idle while state is 0, and once it reaches the final state (DRAW_SELECTED_FINISH)
        break;
    case 1:
        // Erase the rest of the party
        for (i = 0; i < sTradeMenu->partyCounts[whichParty]; i++)
            gSprites[sTradeMenu->partySpriteIds[selectedMonParty][i]].invisible = TRUE;
        for (i = 0; i < PARTY_SIZE; i++)
            ClearWindowTilemap(i + (whichParty * PARTY_SIZE + 2));

        // Re-display the selected Pokémon
        gSprites[sTradeMenu->partySpriteIds[selectedMonParty][partyIdx]].invisible = FALSE;

        // Move the selected Pokémon to the center
        gSprites[sTradeMenu->partySpriteIds[selectedMonParty][partyIdx]].data[0] = 20;
        gSprites[sTradeMenu->partySpriteIds[selectedMonParty][partyIdx]].data[2] = (sTradeMonSpriteCoords[selectedMonParty * PARTY_SIZE][0]
                                                                                  + sTradeMonSpriteCoords[selectedMonParty * PARTY_SIZE + 1][0]) / 2 * 8 + 14;
        gSprites[sTradeMenu->partySpriteIds[selectedMonParty][partyIdx]].data[4] = (sTradeMonSpriteCoords[selectedMonParty * PARTY_SIZE][1] * 8) - 12;
        StoreSpriteCallbackInData6(&gSprites[sTradeMenu->partySpriteIds[selectedMonParty][partyIdx]], SpriteCB_MonIcon);
        sTradeMenu->drawSelectedMonState[whichParty]++;
        Trade_MoveSelectedMonToTarget(&gSprites[sTradeMenu->partySpriteIds[selectedMonParty][partyIdx]]);

        CopyToBgTilemapBufferRect_ChangePalette(1, sTradePartyBoxTilemap, whichParty * 15, 0, 15, 17, 0);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(0);

        if (selectedMonParty == TRADE_PLAYER)
            PrintTradePartnerPartyNicknames();
        break;
    case 2:
        // Wait for the selected pokemon's sprite to move to the correct position
        if (gSprites[sTradeMenu->partySpriteIds[selectedMonParty][partyIdx]].callback == SpriteCB_MonIcon)
            sTradeMenu->drawSelectedMonState[whichParty] = 3;
        break;
    case 3:
        // Redisplay the bg box
        CopyToBgTilemapBufferRect_ChangePalette(1, sTradeMovesBoxTilemap, selectedMonParty * 15, 0, 15, 17, 0);
        CopyBgTilemapBufferToVram(1);

        // Finalize the selected pokemon's position
        gSprites[sTradeMenu->partySpriteIds[selectedMonParty][partyIdx]].x = (sTradeMonSpriteCoords[selectedMonParty * PARTY_SIZE][0]
                                                                            + sTradeMonSpriteCoords[selectedMonParty * PARTY_SIZE + 1][0]) / 2 * 8 + 14;
        gSprites[sTradeMenu->partySpriteIds[selectedMonParty][partyIdx]].y = (sTradeMonSpriteCoords[selectedMonParty * PARTY_SIZE][1] * 8) - 12;
        gSprites[sTradeMenu->partySpriteIds[selectedMonParty][partyIdx]].x2 = 0;
        gSprites[sTradeMenu->partySpriteIds[selectedMonParty][partyIdx]].y2 = 0;

        // Print selected pokemon's name and moves
        nameStringWidth = GetMonNicknameWidth(nickname, selectedMonParty, partyIdx);
        AddTextPrinterParameterized3((whichParty * 2) + 14, FONT_SMALL, (80 - nameStringWidth) / 2, 4, sTradeTextColors, 0, nickname);
        BufferMovesString(movesString, selectedMonParty, partyIdx);
        AddTextPrinterParameterized4((whichParty * 2) + 15, FONT_NORMAL, 0, 0, 0, 0, sTradeTextColors, 0, movesString);
        PutWindowTilemap((whichParty * 2) + 14);
        CopyWindowToVram((whichParty * 2) + 14, COPYWIN_FULL);
        PutWindowTilemap((whichParty * 2) + 15);
        CopyWindowToVram((whichParty * 2) + 15, COPYWIN_FULL);

        sTradeMenu->drawSelectedMonState[whichParty]++;
        break;
    case 4:
        PrintLevelAndGender(whichParty, partyIdx,
            sSelectedMonLevelGenderCoords[whichParty][0] + 4,
            sSelectedMonLevelGenderCoords[whichParty][1] + 1,
            sSelectedMonLevelGenderCoords[whichParty][0],
            sSelectedMonLevelGenderCoords[whichParty][1]);
        sTradeMenu->drawSelectedMonState[whichParty]++;
        break;
    }
}

static u8 GetMonNicknameWidth(u8 *str, u8 whichParty, u8 partyIdx)
{
    u8 nickname[POKEMON_NAME_LENGTH + 1];

    if (whichParty == TRADE_PLAYER)
        GetMonData(&gPlayerParty[partyIdx], MON_DATA_NICKNAME, nickname);
    else
        GetMonData(&gEnemyParty[partyIdx], MON_DATA_NICKNAME, nickname);

    StringCopy_Nickname(str, nickname);
    return GetStringWidth(FONT_SMALL, str, GetFontAttribute(FONT_SMALL, FONTATTR_LETTER_SPACING));
}

static void BufferMovesString(u8 *str, u8 whichParty, u8 partyIdx)
{
    u16 moves[MAX_MON_MOVES];
    u16 i;

    if (!sTradeMenu->isEgg[whichParty][partyIdx])
    {
        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            if (whichParty == TRADE_PLAYER)
                moves[i] = GetMonData(&gPlayerParty[partyIdx], i + MON_DATA_MOVE1, NULL);
            else
                moves[i] = GetMonData(&gEnemyParty[partyIdx], i + MON_DATA_MOVE1, NULL);
        }

        StringCopy(str, sText_EmptyString);

        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            if (moves[i] != MOVE_NONE)
                StringAppend(str, gMoveNames[moves[i]]);

            StringAppend(str, sText_NewLine);
        }
    }
    else
    {
        StringCopy(str, sText_EmptyString);
        StringAppend(str, sText_FourQuestionMarks);
    }
}

static void PrintPartyMonNickname(u8 whichParty, u8 windowId, u8 *nickname)
{
    u8 xPos;
    windowId += (whichParty * PARTY_SIZE) + 2;
    xPos = GetStringCenterAlignXOffset(FONT_SMALL, nickname, 64);
    AddTextPrinterParameterized3(windowId, FONT_SMALL, xPos, 4, sTradeTextColors, 0, nickname);
    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void PrintPartyNicknames(u8 whichParty)
{
    u8 i;
    u8 nickname[POKEMON_NAME_BUFFER_SIZE];
    u8 str[max(32, POKEMON_NAME_BUFFER_SIZE)];
    struct Pokemon *party = (whichParty == TRADE_PLAYER) ? gPlayerParty : gEnemyParty;

    for (i = 0; i < sTradeMenu->partyCounts[whichParty]; i++)
    {
        GetMonData(&party[i], MON_DATA_NICKNAME, nickname);
        StringCopy_Nickname(str, nickname);
        PrintPartyMonNickname(whichParty, i, str);
    }
}

static void PrintLevelAndGender(u8 whichParty, u8 monIdx, u8 x, u8 y, u8 width, u8 height)
{
    u8 level;
    u32 symbolTile;
    u8 gender;
    u8 nickname[POKEMON_NAME_LENGTH + 1];

    CopyToBgTilemapBufferRect_ChangePalette(1, gTradeMenuMonBox_Tilemap, width, height, 6, 3, 0);
    CopyBgTilemapBufferToVram(1);

    if (whichParty == TRADE_PLAYER)
        level = GetMonData(&gPlayerParty[monIdx], MON_DATA_LEVEL, NULL);
    else
        level = GetMonData(&gEnemyParty[monIdx], MON_DATA_LEVEL, NULL);

    if (!sTradeMenu->isEgg[whichParty][monIdx])
    {
        if (level / 10 != 0)
            sTradeMenu->tilemapBuffer[x + (y * 32)] = (level / 10) + 0x60;

        sTradeMenu->tilemapBuffer[x + (y * 32) + 1] = (level % 10) + 0x70;
    }
    else
    {
        sTradeMenu->tilemapBuffer[x + (y * 32) - 32] = sTradeMenu->tilemapBuffer[x + (y * 32) - 33];
        sTradeMenu->tilemapBuffer[x + (y * 32) - 31] = sTradeMenu->tilemapBuffer[x + (y * 32) - 36] | 0x400;
    }

    if (sTradeMenu->isEgg[whichParty][monIdx])
    {
        symbolTile = 0x480;
    }
    else
    {
        if (whichParty == TRADE_PLAYER)
        {
            gender = GetMonGender(&gPlayerParty[monIdx]);
            GetMonData(&gPlayerParty[monIdx], MON_DATA_NICKNAME, nickname);
        }
        else
        {
            gender = GetMonGender(&gEnemyParty[monIdx]);
            GetMonData(&gEnemyParty[monIdx], MON_DATA_NICKNAME, nickname);
        }

        switch (gender)
        {
        case MON_MALE:
            symbolTile = !NameHasGenderSymbol(nickname, MON_MALE) ? 0x84 : 0x83;
            break;
        case MON_FEMALE:
            symbolTile = !NameHasGenderSymbol(nickname, MON_FEMALE) ? 0x85 : 0x83;
            break;
        default:
            symbolTile = 0x83;
            break;
        }
    }
    sTradeMenu->tilemapBuffer[(y - 1) * 32 + x + 1] = symbolTile;
}

static void PrintPartyLevelsAndGenders(u8 whichParty)
{
    s32 i;
    for (i = 0; i < sTradeMenu->partyCounts[whichParty]; i++)
    {
        s32 j = i + PARTY_SIZE * whichParty;
        PrintLevelAndGender(
            whichParty,
            i,
            sTradeMonLevelCoords[j][0],
            sTradeMonLevelCoords[j][1],
            sTradeMonBoxCoords[j][0],
            sTradeMonBoxCoords[j][1]
        );
    }
}

static void ShowTradePartyMonIcons(u8 whichParty)
{
    int i;

    for (i = 0; i < sTradeMenu->partyCounts[whichParty]; i++)
    {
        gSprites[sTradeMenu->partySpriteIds[whichParty][i]].invisible = FALSE;
        gSprites[sTradeMenu->partySpriteIds[whichParty][i]].x = sTradeMonSpriteCoords[(whichParty * PARTY_SIZE) + i][0] * 8 + 14;
        gSprites[sTradeMenu->partySpriteIds[whichParty][i]].y = sTradeMonSpriteCoords[(whichParty * PARTY_SIZE) + i][1] * 8 - 12;
        gSprites[sTradeMenu->partySpriteIds[whichParty][i]].x2 = 0;
        gSprites[sTradeMenu->partySpriteIds[whichParty][i]].y2 = 0;
    }
}

static void PrintTradePartnerPartyNicknames(void)
{
    rbox_fill_rectangle(1);
  //PrintPartyNicknames(TRADE_PLAYER); ?
    PrintPartyNicknames(TRADE_PARTNER);
}

// Returning to the party selection screen from the "is this trade ok?" screen
static void RedrawPartyWindow(u8 whichParty)
{
    CopyToBgTilemapBufferRect_ChangePalette(1, sTradePartyBoxTilemap, whichParty * 15, 0, 15, 17, 0);
    CopyBgTilemapBufferToVram(1);
    PrintPartyLevelsAndGenders(whichParty);
    PrintPartyNicknames(whichParty);
    ShowTradePartyMonIcons(whichParty);
    DrawBottomRowText(sActionTexts[TEXT_CHOOSE_MON], (void *)(OBJ_VRAM0 + (sTradeMenu->bottomTextTileStart * 32)), 24);
    sTradeMenu->drawSelectedMonState[whichParty] = 0;
}

static void Task_DrawSelectionSummary(u8 taskId)
{
    FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
    CopyBgTilemapBufferToVram(0);
}

static void Task_DrawSelectionTrade(u8 taskId)
{
    FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT);
    CopyBgTilemapBufferToVram(0);
}

static void QueueAction(u16 delay, u8 actionId)
{
    int i;

    for (i = 0; i < (int)ARRAY_COUNT(sTradeMenu->queuedActions); i++)
    {
        // Find first available spot
        if (!sTradeMenu->queuedActions[i].active)
        {
            sTradeMenu->queuedActions[i].delay = delay;
            sTradeMenu->queuedActions[i].actionId = actionId;
            sTradeMenu->queuedActions[i].active = TRUE;
            break;
        }
    }
}

static u32 GetNumQueuedActions(void)
{
    u32 numActions = 0;
    int i;

    for (i = 0; i < (int)ARRAY_COUNT(sTradeMenu->queuedActions); i++)
    {
        numActions += sTradeMenu->queuedActions[i].active;
    }

    return numActions;
}

static void DoQueuedActions(void)
{
    int i;

    for (i = 0; i < (int)ARRAY_COUNT(sTradeMenu->queuedActions); i++)
    {
        if (sTradeMenu->queuedActions[i].active)
        {
            if (sTradeMenu->queuedActions[i].delay != 0)
            {
                sTradeMenu->queuedActions[i].delay--;
            }
            else
            {
                switch (sTradeMenu->queuedActions[i].actionId)
                {
                case QUEUE_SEND_DATA:
                    SendLinkData(sTradeMenu->linkData, 20);
                    break;
                case QUEUE_STANDBY:
                    PrintTradeMessage(MSG_STANDBY);
                    break;
                case QUEUE_ONLY_MON1:
                    PrintTradeMessage(MSG_ONLY_MON1);
                    break;
                case QUEUE_ONLY_MON2:
                case QUEUE_UNUSED1:
                case QUEUE_UNUSED2:
                    PrintTradeMessage(MSG_ONLY_MON2);
                    break;
                case QUEUE_MON_CANT_BE_TRADED:
                    PrintTradeMessage(MSG_MON_CANT_BE_TRADED);
                    break;
                case QUEUE_EGG_CANT_BE_TRADED:
                    PrintTradeMessage(MSG_EGG_CANT_BE_TRADED);
                    break;
                case QUEUE_FRIENDS_MON_CANT_BE_TRADED:
                    PrintTradeMessage(MSG_FRIENDS_MON_CANT_BE_TRADED);
                    break;
                }
                sTradeMenu->queuedActions[i].active = FALSE;
            }
        }
    }
}

static void PrintTradeMessage(u8 messageId)
{
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    AddTextPrinterParameterized(0, FONT_NORMAL, sMessages[messageId], 0, 1, TEXT_SKIP_DRAW, NULL);
    DrawTextBorderOuter(0, 20, 12);
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);
}

static bool8 LoadUISpriteGfx(void)
{
    struct SpriteSheet sheet;

    if (sTradeMenu->timer < NUM_MENU_TEXT_SPRITES)
    {
        sheet.data = sMenuTextTileBuffers[sTradeMenu->timer];
        sheet.size = 0x100;
        sheet.tag = GFXTAG_MENU_TEXT + sTradeMenu->timer;
    }

    switch (sTradeMenu->timer)
    {
    case GFXTAG_PLAYER_NAME_L:
    case GFXTAG_PLAYER_NAME_M:
    case GFXTAG_PLAYER_NAME_R:
    case GFXTAG_PARTNER_NAME_L:
    case GFXTAG_PARTNER_NAME_M:
    case GFXTAG_PARTNER_NAME_R:
    case GFXTAG_CANCEL_L:
    case GFXTAG_CANCEL_R:
        LoadSpriteSheet(&sheet);
        sTradeMenu->timer++;
        break;
    case GFXTAG_CHOOSE_PKMN_L:
        sTradeMenu->bottomTextTileStart = LoadSpriteSheet(&sheet);
        sTradeMenu->timer++;
        break;
    case GFXTAG_CHOOSE_PKMN_M:
    case GFXTAG_CHOOSE_PKMN_R:
    case GFXTAG_CHOOSE_PKMN_EMPTY_1:
    case GFXTAG_CHOOSE_PKMN_EMPTY_2:
    case GFXTAG_CHOOSE_PKMN_EMPTY_3:
        LoadSpriteSheet(&sheet);
        sTradeMenu->timer++;
        break;
    case NUM_MENU_TEXT_SPRITES:
        LoadSpritePalette(&sSpritePalette_MenuText);
        sTradeMenu->timer++;
        break;
    case NUM_MENU_TEXT_SPRITES + 1:
        LoadSpritePalette(&sCursor_SpritePalette);
        sTradeMenu->timer++;
        break;
    case NUM_MENU_TEXT_SPRITES + 2:
        LoadSpriteSheet(&sCursor_SpriteSheet);
        sTradeMenu->timer++;
        break;
    case NUM_MENU_TEXT_SPRITES + 3:
        sTradeMenu->timer = 0;
        return TRUE;
    }

    return FALSE;
}

static void DrawBottomRowText(const u8 *str, u8 *dest, u8 unused)
{
    DrawTextWindowAndBufferTiles(str, dest, 0, 0, 6);
}

static void ComputePartyTradeableFlags(u8 whichParty)
{
    int i;

    switch (whichParty)
    {
    case TRADE_PLAYER:
        for (i = 0; i < sTradeMenu->partyCounts[whichParty]; i++)
        {
            if (GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG) == TRUE)
            {
                sTradeMenu->isLiveMon[whichParty][i] = FALSE;
                sTradeMenu->isEgg[whichParty][i] = TRUE;
            }
            else if (GetMonData(&gPlayerParty[i], MON_DATA_HP) == 0)
            {
                sTradeMenu->isLiveMon[whichParty][i] = FALSE;
                sTradeMenu->isEgg[whichParty][i] = FALSE;
            }
            else
            {
                sTradeMenu->isLiveMon[whichParty][i] = TRUE;
                sTradeMenu->isEgg[whichParty][i] = FALSE;
            }
        }
        break;
    case TRADE_PARTNER:
        for (i = 0; i < sTradeMenu->partyCounts[whichParty]; i++)
        {
            if (GetMonData(&gEnemyParty[i], MON_DATA_IS_EGG) == TRUE)
            {
                sTradeMenu->isLiveMon[whichParty][i] = FALSE;
                sTradeMenu->isEgg[whichParty][i] = TRUE;
            }
            else if (GetMonData(&gEnemyParty[i], MON_DATA_HP) == 0)
            {
                sTradeMenu->isLiveMon[whichParty][i] = FALSE;
                sTradeMenu->isEgg[whichParty][i] = FALSE;
            }
            else
            {
                sTradeMenu->isLiveMon[whichParty][i] = TRUE;
                sTradeMenu->isEgg[whichParty][i] = FALSE;
            }
        }
        break;
    }
}

static void ComputePartyHPBarLevels(u8 whichParty)
{
    u16 i, curHp, maxHp;

    switch (whichParty)
    {
    case TRADE_PLAYER:
        for (i = 0; i < sTradeMenu->partyCounts[TRADE_PLAYER]; i++)
        {
            curHp = GetMonData(&gPlayerParty[i], MON_DATA_HP);
            maxHp = GetMonData(&gPlayerParty[i], MON_DATA_MAX_HP);
            sTradeMenu->hpBarLevels[TRADE_PLAYER][i] = GetHPBarLevel(curHp, maxHp);
        }
        break;
    case TRADE_PARTNER:
        for (i = 0; i < sTradeMenu->partyCounts[TRADE_PARTNER]; i++)
        {
            curHp = GetMonData(&gEnemyParty[i], MON_DATA_HP);
            maxHp = GetMonData(&gEnemyParty[i], MON_DATA_MAX_HP);
            sTradeMenu->hpBarLevels[TRADE_PARTNER][i] = GetHPBarLevel(curHp, maxHp);
        }
        break;
    }
}

static void SetTradePartyHPBarSprites(void)
{
    int i, j;

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < sTradeMenu->partyCounts[i]; j++)
            SetPartyHPBarSprite(&gSprites[sTradeMenu->partySpriteIds[i][j]], 4 - sTradeMenu->hpBarLevels[i][j]);
    }
}

static void SaveTradeGiftRibbons(void)
{
    int i;

    for (i = 0; i < (int)ARRAY_COUNT(sTradeMenu->giftRibbons); i++)
    {
        if (gSaveBlock1Ptr->giftRibbons[i] == 0 && sTradeMenu->giftRibbons[i] != 0)
        {
            if (sTradeMenu->giftRibbons[i] < MAX_GIFT_RIBBON)
                gSaveBlock1Ptr->giftRibbons[i] = sTradeMenu->giftRibbons[i];
        }
    }
}

static u32 CanTradeSelectedMon(struct Pokemon *playerParty, int partyCount, int monIdx)
{
    int i, numMonsLeft;
    struct LinkPlayer *partner;
    u32 species[PARTY_SIZE];
    u32 species2[PARTY_SIZE];

    for (i = 0; i < partyCount; i++)
    {
        species2[i] = GetMonData(&playerParty[i], MON_DATA_SPECIES_OR_EGG);
        species[i] = GetMonData(&playerParty[i], MON_DATA_SPECIES);
    }

    // Cant trade Eggs or non-Hoenn mons if player doesn't have National Dex
    if (!IsNationalPokedexEnabled())
    {
        if (species2[monIdx] == SPECIES_EGG)
            return CANT_TRADE_EGG_YET;

        if (!IsSpeciesInHoennDex(species2[monIdx]))
            return CANT_TRADE_NATIONAL;
    }

    partner = &gLinkPlayers[GetMultiplayerId() ^ 1];
    if ((partner->version & 0xFF) != VERSION_RUBY &&
        (partner->version & 0xFF) != VERSION_SAPPHIRE)
    {
        // Does partner not have National Dex
        if (!(partner->progressFlagsCopy & 0xF))
        {
            if (species2[monIdx] == SPECIES_EGG)
                return CANT_TRADE_PARTNER_EGG_YET;

            if (!IsSpeciesInHoennDex(species2[monIdx]))
                return CANT_TRADE_INVALID_MON;
        }
    }

    if (species[monIdx] == SPECIES_DEOXYS || species[monIdx] == SPECIES_MEW)
    {
        if (!GetMonData(&playerParty[monIdx], MON_DATA_MODERN_FATEFUL_ENCOUNTER))
            return CANT_TRADE_INVALID_MON;
    }

    // Make Eggs not count for numMonsLeft
    for (i = 0; i < partyCount; i++)
    {
        if (species2[i] == SPECIES_EGG)
            species2[i] = SPECIES_NONE;
    }

    // Count alive mons in party, excluding selected trade mon
    for (numMonsLeft = 0, i = 0; i < partyCount; i++)
    {
        if (i != monIdx)
            numMonsLeft += species2[i];
    }

    if (numMonsLeft != 0)
        return CAN_TRADE_MON;
    else
        return CANT_TRADE_LAST_MON;
}

s32 GetGameProgressForLinkTrade(void)
{
    // The usage of this value is a little unusual given it's treated as a bool,
    // but it's the result of its usage in FRLG, where 0 is FRLG, 1 is RS, and 2 is Emerald.
    s32 versionId; // 0: RSE, 2: FRLG
    u16 version;

    if (gReceivedRemoteLinkPlayers)
    {
        versionId = 0;
        version = (gLinkPlayers[GetMultiplayerId() ^ 1].version & 0xFF);

        if (version == VERSION_RUBY || version == VERSION_SAPPHIRE || version == VERSION_EMERALD)
            versionId = 0;
        else if (version == VERSION_FIRE_RED || version == VERSION_LEAF_GREEN)
            versionId = 2;

        // If trading with FRLG, both players must have progessed the story enough
        if (versionId > 0)
        {
            // Is player champion
            if (gLinkPlayers[GetMultiplayerId()].progressFlagsCopy & 0xF0)
            {
                if (versionId == 2) // Check is only relevant in FRLG, this will always be true
                {
                    // Has FRLG partner finished the Sevii Islands
                    if (gLinkPlayers[GetMultiplayerId() ^ 1].progressFlagsCopy & 0xF0)
                        return TRADE_BOTH_PLAYERS_READY;
                    else
                        return TRADE_PARTNER_NOT_READY;
                }
            }
            else
            {
                return TRADE_PLAYER_NOT_READY;
            }
        }
    }
    return TRADE_BOTH_PLAYERS_READY;
}

static bool32 IsDeoxysOrMewUntradable(u16 species, bool8 isModernFatefulEncounter)
{
    if (species == SPECIES_DEOXYS || species == SPECIES_MEW)
    {
        if (!isModernFatefulEncounter)
            return TRUE;
    }
    return FALSE;
}

int GetUnionRoomTradeMessageId(struct RfuGameCompatibilityData player, struct RfuGameCompatibilityData partner, u16 playerSpecies2, u16 partnerSpecies, u8 requestedType, u16 playerSpecies, bool8 isModernFatefulEncounter)
{
    bool8 playerHasNationalDex = player.hasNationalDex;
    bool8 playerCanLinkNationally = player.canLinkNationally;
    bool8 partnerHasNationalDex = partner.hasNationalDex;
    bool8 partnerCanLinkNationally = partner.canLinkNationally;
    u8 partnerVersion = partner.version;

    // If partner is not using Emerald, both players must have progressed the story
    // to a certain point (becoming champion in RSE, finishing the Sevii islands in FRLG)
    if (partnerVersion != VERSION_EMERALD)
    {
        if (!playerCanLinkNationally)
            return UR_TRADE_MSG_CANT_TRADE_WITH_PARTNER_1;
        else if (!partnerCanLinkNationally)
            return UR_TRADE_MSG_CANT_TRADE_WITH_PARTNER_2;
    }

    // Cannot trade illegitimate Deoxys/Mew
    if (IsDeoxysOrMewUntradable(playerSpecies, isModernFatefulEncounter))
        return UR_TRADE_MSG_MON_CANT_BE_TRADED_2;

    if (partnerSpecies == SPECIES_EGG)
    {
        // If partner is trading an Egg then the player must also be trading an Egg
        if (playerSpecies2 != partnerSpecies)
            return UR_TRADE_MSG_NOT_EGG;
    }
    else
    {
        // Player's Pokémon must be of the type the partner requested
        if (gSpeciesInfo[playerSpecies2].types[0] != requestedType
         && gSpeciesInfo[playerSpecies2].types[1] != requestedType)
            return UR_TRADE_MSG_NOT_MON_PARTNER_WANTS;
    }

    // If the player is trading an Egg then the partner must also be trading an Egg
    // Odd that this wasn't checked earlier, as by this point we know either the partner doesn't have an Egg or that both do.
    if (playerSpecies2 == SPECIES_EGG && playerSpecies2 != partnerSpecies)
        return UR_TRADE_MSG_MON_CANT_BE_TRADED_1;

    // If the player doesn't have the National Dex then Eggs and non-Hoenn Pokémon can't be traded
    if (!playerHasNationalDex)
    {
        if (playerSpecies2 == SPECIES_EGG)
            return UR_TRADE_MSG_EGG_CANT_BE_TRADED;

        if (!IsSpeciesInHoennDex(playerSpecies2))
            return UR_TRADE_MSG_MON_CANT_BE_TRADED_2;

        if (!IsSpeciesInHoennDex(partnerSpecies))
            return UR_TRADE_MSG_PARTNERS_MON_CANT_BE_TRADED;
    }

    // If the partner doesn't have the National Dex then the player's offer has to be a Hoenn Pokémon
    if (!partnerHasNationalDex && !IsSpeciesInHoennDex(playerSpecies2))
        return UR_TRADE_MSG_PARTNER_CANT_ACCEPT_MON;

    // Trade is allowed
    return UR_TRADE_MSG_NONE;
}

int CanRegisterMonForTradingBoard(struct RfuGameCompatibilityData player, u16 species2, u16 species, bool8 isModernFatefulEncounter)
{
    bool8 hasNationalDex = player.hasNationalDex;

    if (IsDeoxysOrMewUntradable(species, isModernFatefulEncounter))
        return CANT_REGISTER_MON;

    if (hasNationalDex)
        return CAN_REGISTER_MON;

    // Eggs can only be traded if the player has the National Dex
    if (species2 == SPECIES_EGG)
        return CANT_REGISTER_EGG;

    if (IsSpeciesInHoennDex(species2))
        return CAN_REGISTER_MON;

    return CANT_REGISTER_MON;
}

// Spin Trade wasnt fully implemented, but this checks if a mon would be valid to Spin Trade
// Unlike later generations, this version of Spin Trade isnt only for Eggs
int CanSpinTradeMon(struct Pokemon *mon, u16 monIdx)
{
    int i, version, versions, canTradeAnyMon, numMonsLeft;
    int speciesArray[PARTY_SIZE];

    // Make Eggs not count for numMonsLeft
    for (i = 0; i < gPlayerPartyCount; i++)
    {
        speciesArray[i] = GetMonData(&mon[i], MON_DATA_SPECIES_OR_EGG);
        if (speciesArray[i] == SPECIES_EGG)
            speciesArray[i] = SPECIES_NONE;
    }

    versions = 0;
    canTradeAnyMon = TRUE;
    for (i = 0; i < GetLinkPlayerCount(); i++)
    {
        version = gLinkPlayers[i].version & 0xFF;
        if (version == VERSION_FIRE_RED ||
            version == VERSION_LEAF_GREEN)
            versions = 0;
        else
            versions |= 1;
    }

    for (i = 0; i < GetLinkPlayerCount(); i++)
    {
        struct LinkPlayer *player = &gLinkPlayers[i];

        // Does player not have National Dex
        do
        {
            if (!(player->progressFlags & 0xF))
                canTradeAnyMon = FALSE;

            if (versions && (player->progressFlags / 16))
                canTradeAnyMon = FALSE;
        } while (0);
    }

    if (canTradeAnyMon == FALSE)
    {
        if (!IsSpeciesInHoennDex(speciesArray[monIdx]))
            return CANT_TRADE_NATIONAL;

        if (speciesArray[monIdx] == SPECIES_NONE)
            return CANT_TRADE_EGG_YET;
    }

    numMonsLeft = 0;
    for (i = 0; i < gPlayerPartyCount; i++)
    {
        if (monIdx != i)
            numMonsLeft += speciesArray[i];
    }

    if (!numMonsLeft)
        return CANT_TRADE_LAST_MON;
    else
        return CAN_TRADE_MON;
}

static void SpriteCB_LinkMonGlow(struct Sprite *sprite)
{
    if (++sprite->data[0] == 10)
    {
        PlaySE(SE_BALL);
        sprite->data[0] = 0;
    }
}

static void SpriteCB_LinkMonGlowWireless(struct Sprite *sprite)
{
    if (!sprite->invisible && ++sprite->data[0] == 10)
    {
        PlaySE(SE_M_SWAGGER2);
        sprite->data[0] = 0;
    }
}

// Palette flash for trade glow core
static void SpriteCB_LinkMonShadow(struct Sprite *sprite)
{
    if (!sprite->data[1])
    {
        if (++sprite->data[0] == 12)
            sprite->data[0] = 0;

        LoadPalette(&sLinkMonShadow_Pal[sprite->data[0]], OBJ_PLTT_ID2(sprite->oam.paletteNum) + 4, PLTT_SIZEOF(1));
    }
}

// Move cable down offscreen
static void SpriteCB_CableEndSending(struct Sprite *sprite)
{
    sprite->data[0]++;
    sprite->y2++;

    if (sprite->data[0] == 10)
        DestroySprite(sprite);
}

// Move cable up onscreen
static void SpriteCB_CableEndReceiving(struct Sprite *sprite)
{
    sprite->data[0]++;
    sprite->y2--;

    if (sprite->data[0] == 10)
        DestroySprite(sprite);
}

static void SpriteCB_GbaScreen(struct Sprite *sprite)
{
    if (++sprite->data[0] == 15)
    {
        PlaySE(SE_M_MINIMIZE);
        sprite->data[0] = 0;
    }
}

static void SetTradeBGAffine(void)
{
    struct BgAffineDstData affine;

    DoBgAffineSet(&affine, sTradeAnim->texX * 0x100, sTradeAnim->texY * 0x100, sTradeAnim->scrX, sTradeAnim->scrY, sTradeAnim->sXY, sTradeAnim->sXY, sTradeAnim->alpha);
    SetGpuReg(REG_OFFSET_BG2PA, affine.pa);
    SetGpuReg(REG_OFFSET_BG2PB, affine.pb);
    SetGpuReg(REG_OFFSET_BG2PC, affine.pc);
    SetGpuReg(REG_OFFSET_BG2PD, affine.pd);
    SetGpuReg(REG_OFFSET_BG2X_L, affine.dx);
    SetGpuReg(REG_OFFSET_BG2X_H, affine.dx >> 16);
    SetGpuReg(REG_OFFSET_BG2Y_L, affine.dy);
    SetGpuReg(REG_OFFSET_BG2Y_H, affine.dy >> 16);
}

static void SetTradeGpuRegs(void)
{
    u16 dispcnt;

    SetGpuReg(REG_OFFSET_BG1VOFS, sTradeAnim->bg1vofs);
    SetGpuReg(REG_OFFSET_BG1HOFS, sTradeAnim->bg1hofs);

    dispcnt = GetGpuReg(REG_OFFSET_DISPCNT);
    if ((dispcnt & 7) == DISPCNT_MODE_0)
    {
        SetGpuReg(REG_OFFSET_BG2VOFS, sTradeAnim->bg2vofs);
        SetGpuReg(REG_OFFSET_BG2HOFS, sTradeAnim->bg2hofs);
    }
    else
    {
        SetTradeBGAffine();
    }
}

static void VBlankCB_TradeAnim(void)
{
    SetTradeGpuRegs();
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void ClearLinkTimeoutTimer(void)
{
    sTradeAnim->linkTimeoutTimer = 0;
    sTradeAnim->linkTimeoutZero1 = 0;
    sTradeAnim->linkTimeoutZero2 = 0;
}

static void CheckForLinkTimeout(void)
{
    if (sTradeAnim->linkTimeoutZero1 == sTradeAnim->linkTimeoutZero2)
        sTradeAnim->linkTimeoutTimer++;
    else
        sTradeAnim->linkTimeoutTimer = 0;

    if (sTradeAnim->linkTimeoutTimer > 300)
    {
        CloseLink();
        SetMainCallback2(CB2_LinkError);
        sTradeAnim->linkTimeoutTimer = 0;
        sTradeAnim->linkTimeoutZero2 = 0;
        sTradeAnim->linkTimeoutZero1 = 0;
    }

    sTradeAnim->linkTimeoutZero2 = sTradeAnim->linkTimeoutZero1;
}

static u32 TradeGetMultiplayerId(void)
{
    if (gReceivedRemoteLinkPlayers)
        return GetMultiplayerId();
    return 0;
}

static void LoadTradeMonPic(u8 whichParty, u8 state)
{
    int pos = 0;
    struct Pokemon *mon = NULL;
    u16 species;
    u32 personality;

    if (whichParty == TRADE_PLAYER)
    {
        mon = &gPlayerParty[gSelectedTradeMonPositions[TRADE_PLAYER]];
        pos = B_POSITION_OPPONENT_LEFT;
    }

    if (whichParty == TRADE_PARTNER)
    {
        mon = &gEnemyParty[gSelectedTradeMonPositions[TRADE_PARTNER] % PARTY_SIZE];
        pos = B_POSITION_OPPONENT_RIGHT;
    }

    switch (state)
    {
    case 0:
        species = GetMonData(mon, MON_DATA_SPECIES_OR_EGG);
        personality = GetMonData(mon, MON_DATA_PERSONALITY);

        if (whichParty == TRADE_PLAYER)
            HandleLoadSpecialPokePic_2(&gMonFrontPicTable[species], gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_LEFT], species, personality);
        else
            HandleLoadSpecialPokePic_DontHandleDeoxys(&gMonFrontPicTable[species], gMonSpritesGfxPtr->sprites.ptr[whichParty * 2 + B_POSITION_OPPONENT_LEFT], species, personality);

        LoadCompressedSpritePalette(GetMonSpritePalStruct(mon));
        sTradeAnim->monSpecies[whichParty] = species;
        sTradeAnim->monPersonalities[whichParty] = personality;
        break;
    case 1:
        SetMultiuseSpriteTemplateToPokemon(GetMonSpritePalStruct(mon)->tag, pos);
        sTradeAnim->monSpriteIds[whichParty] = CreateSprite(&gMultiuseSpriteTemplate, 120, 60, 6);
        gSprites[sTradeAnim->monSpriteIds[whichParty]].invisible = TRUE;
        gSprites[sTradeAnim->monSpriteIds[whichParty]].callback = SpriteCallbackDummy;
        break;
    }
}

void CB2_LinkTrade(void)
{
    switch (gMain.state)
    {
    case 0:
        if (!gReceivedRemoteLinkPlayers)
        {
            gLinkType = LINKTYPE_TRADE_DISCONNECTED;
            CloseLink();
        }
        sTradeAnim = AllocZeroed(sizeof(*sTradeAnim));
        AllocateMonSpritesGfx();
        ResetTasks();
        ResetSpriteData();
        FreeAllSpritePalettes();
        SetVBlankCallback(VBlankCB_TradeAnim);
        TradeAnimInit_LoadGfx();
        ClearLinkTimeoutTimer();
        gMain.state++;
        sTradeAnim->neverRead_8C = 0;
        sTradeAnim->state = 0;
        sTradeAnim->isLinkTrade = TRUE;
        sTradeAnim->texX = 64;
        sTradeAnim->texY = 64;
        sTradeAnim->neverRead_D8 = 0;
        sTradeAnim->neverRead_DA = 0;
        sTradeAnim->scrX = DISPLAY_WIDTH / 2;
        sTradeAnim->scrY = DISPLAY_HEIGHT / 2;
        sTradeAnim->sXY = 256;
        sTradeAnim->alpha = 0;
        break;
    case 1:
        if (!gReceivedRemoteLinkPlayers)
        {
            sTradeAnim->isCableTrade = TRUE;
            OpenLink();
            gMain.state++;
            sTradeAnim->timer = 0;
        }
        else
        {
            gMain.state = 4;
        }
        break;
    case 2:
        if (++sTradeAnim->timer > 60)
        {
            sTradeAnim->timer = 0;
            gMain.state++;
        }
        break;
    case 3:
        if (IsLinkMaster())
        {
            if (GetLinkPlayerCount_2() >= GetSavedPlayerCount())
            {
                if (++sTradeAnim->timer > 30)
                {
                    CheckShouldAdvanceLinkState();
                    gMain.state++;
                }
            }
            else
            {
                CheckForLinkTimeout();
            }
        }
        else
        {
            gMain.state++;
        }
        break;
    case 4:
        CheckForLinkTimeout();
        if (gReceivedRemoteLinkPlayers == TRUE && IsLinkPlayerDataExchangeComplete() == TRUE)
            gMain.state++;
        break;
    case 5:
        sTradeAnim->playerFinishStatus = 0;
        sTradeAnim->partnerFinishStatus = 0;
        sTradeAnim->scheduleLinkTransfer = 0;
        LoadTradeMonPic(TRADE_PLAYER, 0);
        gMain.state++;
        break;
    case 6:
        LoadTradeMonPic(TRADE_PLAYER, 1);
        gMain.state++;
        break;
    case 7:
        LoadTradeMonPic(TRADE_PARTNER, 0);
        gMain.state++;
        break;
    case 8:
        LoadTradeMonPic(TRADE_PARTNER, 1);
        LinkTradeDrawWindow();
        gMain.state++;
        break;
    case 9:
        LoadTradeSequenceSpriteSheetsAndPalettes();
        LoadSpriteSheet(&sPokeBallSpriteSheet);
        LoadSpritePalette(&sPokeBallSpritePalette);
        gMain.state++;
        break;
    case 10:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        ShowBg(0);
        gMain.state++;
        break;
    case 11:
        InitTradeSequenceBgGpuRegs();
        BufferTradeSceneStrings();
        gMain.state++;
        break;
    case 12:
        if (!gPaletteFade.active)
        {
            if (gWirelessCommType)
            {
                LoadWirelessStatusIndicatorSpriteGfx();
                CreateWirelessStatusIndicatorSprite(0, 0);
            }
            SetMainCallback2(CB2_UpdateLinkTrade);
        }
        break;
    }
    RunTasks();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

void InitTradeSequenceBgGpuRegs(void)
{
    SetTradeSequenceBgGpuRegs(5);
    SetTradeSequenceBgGpuRegs(0);
}

void LinkTradeDrawWindow(void)
{
    FillWindowPixelBuffer(0, PIXEL_FILL(15));
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);
}

static void TradeAnimInit_LoadGfx(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sTradeSequenceBgTemplates, ARRAY_COUNT(sTradeSequenceBgTemplates));
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    SetBgTilemapBuffer(0, Alloc(BG_SCREEN_SIZE));
    SetBgTilemapBuffer(1, Alloc(BG_SCREEN_SIZE));
    SetBgTilemapBuffer(3, Alloc(BG_SCREEN_SIZE));
    DeactivateAllTextPrinters();
    // Doing the graphics load...
    DecompressAndLoadBgGfxUsingHeap(0, gBattleTextboxTiles, 0, 0, 0);
    LZDecompressWram(gBattleTextboxTilemap, gDecompressionBuffer);
    CopyToBgTilemapBuffer(0, gDecompressionBuffer, BG_SCREEN_SIZE, 0);
    LoadCompressedPalette(gBattleTextboxPalette, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
    InitWindows(sTradeSequenceWindowTemplates);
    // ... and doing the same load again
    DecompressAndLoadBgGfxUsingHeap(0, gBattleTextboxTiles, 0, 0, 0);
    LZDecompressWram(gBattleTextboxTilemap, gDecompressionBuffer);
    CopyToBgTilemapBuffer(0, gDecompressionBuffer, BG_SCREEN_SIZE, 0);
    LoadCompressedPalette(gBattleTextboxPalette, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
}

static void CB2_InitInGameTrade(void)
{
    u8 otName[11];

    switch (gMain.state)
    {
    case 0:
        gSelectedTradeMonPositions[TRADE_PLAYER] = gSpecialVar_0x8005;
        gSelectedTradeMonPositions[TRADE_PARTNER] = PARTY_SIZE;
        StringCopy(gLinkPlayers[0].name, gSaveBlock2Ptr->playerName);
        GetMonData(&gEnemyParty[0], MON_DATA_OT_NAME, otName);
        StringCopy(gLinkPlayers[1].name, otName);
        gLinkPlayers[0].language = GAME_LANGUAGE;
        gLinkPlayers[1].language = GetMonData(&gEnemyParty[0], MON_DATA_LANGUAGE);
        sTradeAnim = AllocZeroed(sizeof(*sTradeAnim));
        AllocateMonSpritesGfx();
        ResetTasks();
        ResetSpriteData();
        FreeAllSpritePalettes();
        SetVBlankCallback(VBlankCB_TradeAnim);
        TradeAnimInit_LoadGfx();
        sTradeAnim->isLinkTrade = FALSE;
        sTradeAnim->neverRead_8C = 0;
        sTradeAnim->state = 0;
        sTradeAnim->texX = 64;
        sTradeAnim->texY = 64;
        sTradeAnim->neverRead_D8 = 0;
        sTradeAnim->neverRead_DA = 0;
        sTradeAnim->scrX = DISPLAY_WIDTH / 2;
        sTradeAnim->scrY = DISPLAY_HEIGHT / 2;
        sTradeAnim->sXY = 256;
        sTradeAnim->alpha = 0;
        sTradeAnim->timer = 0;
        gMain.state = 5;
        break;
    case 5:
        LoadTradeMonPic(TRADE_PLAYER, 0);
        gMain.state++;
        break;
    case 6:
        LoadTradeMonPic(TRADE_PLAYER, 1);
        gMain.state++;
        break;
    case 7:
        LoadTradeMonPic(TRADE_PARTNER, 0);
        ShowBg(0);
        gMain.state++;
        break;
    case 8:
        LoadTradeMonPic(TRADE_PARTNER, 1);
        FillWindowPixelBuffer(0, PIXEL_FILL(15));
        PutWindowTilemap(0);
        CopyWindowToVram(0, COPYWIN_FULL);
        gMain.state++;
        break;
    case 9:
        LoadTradeSequenceSpriteSheetsAndPalettes();
        LoadSpriteSheet(&sPokeBallSpriteSheet);
        LoadSpritePalette(&sPokeBallSpritePalette);
        gMain.state++;
        break;
    case 10:
        ShowBg(0);
        gMain.state++;
        break;
    case 11:
        SetTradeSequenceBgGpuRegs(5);
        SetTradeSequenceBgGpuRegs(0);
        BufferTradeSceneStrings();
        gMain.state++;
        break;
    case 12:
        SetMainCallback2(CB2_InGameTrade);
        break;
    }

    RunTasks();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void UpdatePokedexForReceivedMon(u8 partyIdx)
{
    struct Pokemon *mon = &gPlayerParty[partyIdx];

    if (!GetMonData(mon, MON_DATA_IS_EGG))
    {
        u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
        u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
        species = SpeciesToNationalPokedexNum(species);
        GetSetPokedexFlag(species, FLAG_SET_SEEN);
        HandleSetPokedexFlag(species, FLAG_SET_CAUGHT, personality);
    }
}

// Functionally nop after commented code
static void TryEnableNationalDexFromLinkPartner(void)
{
    u8 UNUSED mpId = GetMultiplayerId();
    // Originally in Ruby but commented out
    /*if (gLinkPlayers[mpId ^ 1].lp_field_2 == 0x8000)
        EnableNationalPokedex();*/
}

static void TradeMons(u8 playerPartyIdx, u8 partnerPartyIdx)
{
    u8 friendship;

    struct Pokemon *playerMon = &gPlayerParty[playerPartyIdx];
    u16 playerMail = GetMonData(playerMon, MON_DATA_MAIL);

    struct Pokemon *partnerMon = &gEnemyParty[partnerPartyIdx];
    u16 partnerMail = GetMonData(partnerMon, MON_DATA_MAIL);

    // The mail attached to the sent Pokémon no longer exists in your file.
    if (playerMail != MAIL_NONE)
        ClearMail(&gSaveBlock1Ptr->mail[playerMail]);

    SWAP(*playerMon, *partnerMon, sTradeAnim->tempMon);

    // By default, a Pokémon received from a trade will have 70 Friendship.
    // Eggs use Friendship to track egg cycles, so don't set this on Eggs.
    friendship = 70;
    if (!GetMonData(playerMon, MON_DATA_IS_EGG))
        SetMonData(playerMon, MON_DATA_FRIENDSHIP, &friendship);

    if (partnerMail != MAIL_NONE)
        GiveMailToMon(playerMon, &gTradeMail[partnerMail]);

    UpdatePokedexForReceivedMon(playerPartyIdx);
    if (gReceivedRemoteLinkPlayers)
        TryEnableNationalDexFromLinkPartner();
}

static void HandleLinkDataSend(void)
{
    switch (sTradeAnim->scheduleLinkTransfer)
    {
    case 1:
        if (IsLinkTaskFinished())
        {
            SendBlock(BitmaskAllOtherLinkPlayers(), sTradeAnim->linkData, sizeof(sTradeAnim->linkData));
            sTradeAnim->scheduleLinkTransfer++;
        }
        // fallthrough
    case 2:
        sTradeAnim->scheduleLinkTransfer = 0;
        break;
    }
}

static void CB2_InGameTrade(void)
{
    DoTradeAnim();
    RunTasks();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void SetTradeSequenceBgGpuRegs(u8 state)
{
    switch (state)
    {
    case 0:
        sTradeAnim->bg2vofs = 0;
        sTradeAnim->bg2hofs = 180;
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG0_ON |
                                      DISPCNT_BG2_ON |
                                      DISPCNT_OBJ_ON);
        SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(2) |
                                     BGCNT_CHARBASE(1) |
                                     BGCNT_16COLOR |
                                     BGCNT_SCREENBASE(18) |
                                     BGCNT_TXT512x256);
        LoadPalette(gTradeGba2_Pal, BG_PLTT_ID(1), 3 * PLTT_SIZE_4BPP);
        DmaCopyLarge16(3, gTradeGba_Gfx, (void *) BG_CHAR_ADDR(1), 0x1420, 0x1000);
        DmaCopy16Defvars(3, gTradePlatform_Tilemap, (void *) BG_SCREEN_ADDR(18), 0x1000);
        break;
    case 1:
        sTradeAnim->bg1hofs = 0;
        sTradeAnim->bg1vofs = 348;
        SetGpuReg(REG_OFFSET_BG1VOFS, 348);
        SetGpuReg(REG_OFFSET_BG1CNT, BGCNT_PRIORITY(2) |
                                     BGCNT_CHARBASE(0) |
                                     BGCNT_16COLOR |
                                     BGCNT_SCREENBASE(5) |
                                     BGCNT_TXT256x512);
        SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(2) |
                                     BGCNT_CHARBASE(1) |
                                     BGCNT_16COLOR |
                                     BGCNT_SCREENBASE(18) |
                                     BGCNT_TXT256x512);

        if (sTradeAnim->isCableTrade)
        {
            DmaCopy16Defvars(3, sGbaMapCable, (void *) BG_SCREEN_ADDR(5), 0x1000);
        }
        else
        {
            DmaCopy16Defvars(3, sGbaMapWireless, (void *) BG_SCREEN_ADDR(5), 0x1000);
        }

        DmaCopyLarge16(3, gTradeGba_Gfx, (void *) BG_CHAR_ADDR(0), 0x1420, 0x1000);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG1_ON |
                                      DISPCNT_OBJ_ON);
        break;
    case 2:
        sTradeAnim->bg1vofs = 0;
        sTradeAnim->bg1hofs = 0;
        if (!sTradeAnim->isCableTrade)
        {
            SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_1 |
                                          DISPCNT_OBJ_1D_MAP |
                                          DISPCNT_BG1_ON |
                                          DISPCNT_OBJ_ON);
            LZ77UnCompVram(sWirelessCloseup_Map, (void *) BG_SCREEN_ADDR(5));
            BlendPalettes(0x8, 16, RGB_BLACK);
        }
        else
        {
            SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_1 |
                                          DISPCNT_OBJ_1D_MAP |
                                          DISPCNT_BG1_ON |
                                          DISPCNT_OBJ_ON);
            DmaCopy16Defvars(3, sCableCloseup_Map, (void *) BG_SCREEN_ADDR(5), 0x800);
            BlendPalettes(0x1, 16, RGB_BLACK);
        }
        break;
    case 3:
        LoadPalette(sWirelessSignalNone_Pal, BG_PLTT_ID(3), PLTT_SIZE_4BPP);
        LZ77UnCompVram(sWirelessSignal_Gfx, (void *) BG_CHAR_ADDR(1));
        LZ77UnCompVram(sWirelessSignal_Tilemap, (void *) BG_SCREEN_ADDR(18));
        sTradeAnim->bg2vofs = 80;
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG1_ON |
                                      DISPCNT_BG2_ON |
                                      DISPCNT_OBJ_ON);
        break;
    case 4:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_1 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG2_ON |
                                      DISPCNT_OBJ_ON);
        SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(3) |
                                     BGCNT_CHARBASE(1) |
                                     BGCNT_256COLOR |
                                     BGCNT_SCREENBASE(18) |
                                     BGCNT_AFF128x128);
        sTradeAnim->texX = 64;
        sTradeAnim->texY = 92;
        sTradeAnim->sXY = 32;
        sTradeAnim->gbaScale = 1024;
        sTradeAnim->alpha = 0;

        DmaCopyLarge16(3, sGbaAffine_Gfx, (void *) BG_CHAR_ADDR(1), 0x2840, 0x1000);

        if (sTradeAnim->isCableTrade)
        {
            DmaCopy16Defvars(3, sGbaAffineMapCable, (void *) BG_SCREEN_ADDR(18), 0x100);
        }
        else
        {
            DmaCopy16Defvars(3, sGbaAffineMapWireless, (void *) BG_SCREEN_ADDR(18), 0x100);
        }
        break;
    case 5:
        sTradeAnim->bg1vofs = 0;
        sTradeAnim->bg1hofs = 0;
        break;
    case 6:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_1 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG2_ON |
                                      DISPCNT_OBJ_ON);
        SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(3) |
                                     BGCNT_CHARBASE(1) |
                                     BGCNT_256COLOR |
                                     BGCNT_SCREENBASE(18) |
                                     BGCNT_AFF128x128);
        sTradeAnim->texX = 64;
        sTradeAnim->texY = 92;
        sTradeAnim->sXY = 256;
        sTradeAnim->gbaScale = 128;
        sTradeAnim->scrX = 120;
        sTradeAnim->scrY = 80;
        sTradeAnim->alpha = 0;

        DmaCopyLarge16(3, sGbaAffine_Gfx, (void *) BG_CHAR_ADDR(1), 0x2840, 0x1000);

        if (sTradeAnim->isCableTrade)
        {
            DmaCopy16Defvars(3, sGbaAffineMapCable, (void *) BG_SCREEN_ADDR(18), 0x100);
        }
        else
        {
            DmaCopy16Defvars(3, sGbaAffineMapWireless, (void *) BG_SCREEN_ADDR(18), 0x100);
        }
        break;
    case 7:
        sTradeAnim->bg2vofs = 0;
        sTradeAnim->bg2hofs = 0;
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(2) |
                                     BGCNT_CHARBASE(1) |
                                     BGCNT_16COLOR |
                                     BGCNT_SCREENBASE(18) |
                                     BGCNT_TXT512x256);
        LoadPalette(gTradeGba2_Pal, BG_PLTT_ID(1), 3 * PLTT_SIZE_4BPP);
        DmaCopyLarge16(3, gTradeGba_Gfx, (void *) BG_CHAR_ADDR(1), 0x1420, 0x1000);
        DmaCopy16Defvars(3, gTradePlatform_Tilemap, (void *) BG_SCREEN_ADDR(18), 0x1000);
        break;
    }
}

static void LoadTradeSequenceSpriteSheetsAndPalettes(void)
{
    LoadSpriteSheet(&sSpriteSheet_LinkMonGlow);
    LoadSpriteSheet(&sSpriteSheet_LinkMonShadow);
    LoadSpriteSheet(&sSpriteSheet_CableEnd);
    LoadSpriteSheet(&sSpriteSheet_GbaScreen);
    LoadSpritePalette(&sSpritePalette_LinkMon);
    LoadSpritePalette(&sSpritePalette_Gba);
}

// Buffers "[Pokemon] will be sent to [Trainer]" strings
static void BufferTradeSceneStrings(void)
{
    u8 mpId;
    u8 name[POKEMON_NAME_BUFFER_SIZE];
    const struct InGameTrade *ingameTrade;

    if (sTradeAnim->isLinkTrade)
    {
        mpId = GetMultiplayerId();
        StringCopy(gStringVar1, gLinkPlayers[mpId ^ 1].name);
        GetMonData(&gEnemyParty[gSelectedTradeMonPositions[TRADE_PARTNER] % PARTY_SIZE], MON_DATA_NICKNAME, name);
        StringCopy_Nickname(gStringVar3, name);
        GetMonData(&gPlayerParty[gSelectedTradeMonPositions[TRADE_PLAYER]], MON_DATA_NICKNAME, name);
        StringCopy_Nickname(gStringVar2, name);
    }
    else
    {
        ingameTrade = &sIngameTrades[gSpecialVar_0x8004];
        StringCopy(gStringVar1, ingameTrade->otName);
        StringCopy_Nickname(gStringVar3, ingameTrade->nickname);
        GetMonData(&gPlayerParty[gSpecialVar_0x8005], MON_DATA_NICKNAME, name);
        StringCopy_Nickname(gStringVar2, name);
    }
}

// returns TRUE if it finished a link trade, FALSE if it finished an in-game trade or if sequence is still going
static bool8 DoTradeAnim(void)
{
    if (sTradeAnim->isCableTrade)
        return DoTradeAnim_Cable();
    else
        return DoTradeAnim_Wireless();
}

// Below are the states for the main switch in DoTradeAnim_Cable and DoTradeAnim_Wireless
// When DoTradeAnim_Wireless has a unique version of a state used by DoTradeAnim_Cable, it adds the below modifier
#define STATE_WIRELESS 100
enum {
    STATE_START,
    STATE_MON_SLIDE_IN,
    // 2-9 unused
    STATE_SEND_MSG = 10,
    STATE_BYE_BYE,
    STATE_POKEBALL_DEPART,
    STATE_POKEBALL_DEPART_WAIT,
    STATE_FADE_OUT_TO_GBA_SEND,
    // 15-19 unused
    STATE_WAIT_FADE_OUT_TO_GBA_SEND = 20,
    STATE_FADE_IN_TO_GBA_SEND,
    STATE_WAIT_FADE_IN_TO_GBA_SEND,
    STATE_GBA_ZOOM_OUT,
    STATE_GBA_FLASH_SEND,
    STATE_GBA_STOP_FLASH_SEND,
    STATE_PAN_AWAY_GBA,
    STATE_CREATE_LINK_MON_LEAVING,
    STATE_LINK_MON_TRAVEL_OUT,
    STATE_FADE_OUT_TO_CROSSING,
    STATE_WAIT_FADE_OUT_TO_CROSSING,
    STATE_FADE_IN_TO_CROSSING,
    STATE_WAIT_FADE_IN_TO_CROSSING,
    STATE_CROSSING_LINK_MONS_ENTER,
    STATE_CROSSING_BLEND_WHITE_1,
    STATE_CROSSING_BLEND_WHITE_2,
    STATE_CROSSING_BLEND_WHITE_3,
    STATE_CROSSING_CREATE_MON_PICS,
    STATE_CROSSING_MON_PICS_MOVE,
    STATE_CROSSING_LINK_MONS_EXIT,
    STATE_CREATE_LINK_MON_ARRIVING,
    STATE_FADE_OUT_TO_GBA_RECV,
    STATE_WAIT_FADE_OUT_TO_GBA_RECV,
    STATE_LINK_MON_TRAVEL_IN,
    STATE_PAN_TO_GBA,
    STATE_DESTROY_LINK_MON,
    STATE_LINK_MON_ARRIVED_DELAY,
    STATE_MOVE_GBA_TO_CENTER,
    STATE_GBA_FLASH_RECV,
    STATE_UNUSED,
    STATE_GBA_STOP_FLASH_RECV,
    STATE_GBA_ZOOM_IN,
    STATE_FADE_OUT_TO_NEW_MON,
    // 53-59 unused
    STATE_WAIT_FADE_OUT_TO_NEW_MON = 60,
    STATE_FADE_IN_TO_NEW_MON,
    STATE_WAIT_FADE_IN_TO_NEW_MON,
    STATE_POKEBALL_ARRIVE,
    STATE_FADE_POKEBALL_TO_NORMAL,
    STATE_POKEBALL_ARRIVE_WAIT,
    STATE_SHOW_NEW_MON,
    STATE_NEW_MON_MSG,
    STATE_TAKE_CARE_OF_MON,
    STATE_AFTER_NEW_MON_DELAY,
    STATE_CHECK_RIBBONS,
    STATE_END_LINK_TRADE,
    STATE_TRY_EVOLUTION,
    STATE_FADE_OUT_END,
    STATE_WAIT_FADE_OUT_END,
    // Special states
    STATE_GBA_FLASH_SEND_WIRELESS = STATE_GBA_FLASH_SEND + STATE_WIRELESS,
    STATE_GBA_STOP_FLASH_SEND_WIRELESS,
    STATE_WAIT_WIRELESS_SIGNAL_SEND,
    STATE_PAN_TO_GBA_WIRELESS = STATE_PAN_TO_GBA + STATE_WIRELESS,
    STATE_DESTROY_LINK_MON_WIRELESS,
    STATE_WAIT_WIRELESS_SIGNAL_RECV,
    STATE_DELAY_FOR_MON_ANIM = 167,
    STATE_LINK_MON_TRAVEL_OFFSCREEN = 200,
    STATE_WAIT_FOR_MON_CRY = 267,
};

static bool8 DoTradeAnim_Cable(void)
{
    u16 evoTarget;

    switch (sTradeAnim->state)
    {
    case STATE_START:
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].invisible = FALSE;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].x2 = -180;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].y2 = gMonFrontPicCoords[sTradeAnim->monSpecies[TRADE_PLAYER]].y_offset;
        sTradeAnim->state++;
        sTradeAnim->cachedMapMusic = GetCurrentMapMusic();
        PlayNewMapMusic(MUS_EVOLUTION);
        break;
    case STATE_MON_SLIDE_IN:
        if (sTradeAnim->bg2hofs > 0)
        {
            // Sliding
            gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].x2 += 3;
            sTradeAnim->bg2hofs -= 3;
        }
        else
        {
            // Pokémon has arrived onscreen
            gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].x2 = 0;
            sTradeAnim->bg2hofs = 0;
            sTradeAnim->state = STATE_SEND_MSG;
        }
        break;
    case STATE_SEND_MSG:
        StringExpandPlaceholders(gStringVarBuffer, gText_XWillBeSentToY);
        DrawTextOnTradeWindow(0, gStringVarBuffer, 0);

        if (sTradeAnim->monSpecies[TRADE_PLAYER] != SPECIES_EGG)
            PlayCry_Normal(sTradeAnim->monSpecies[TRADE_PLAYER], 0);

        sTradeAnim->state = STATE_BYE_BYE;
        sTradeAnim->timer = 0;
        break;
    case STATE_BYE_BYE:
        if (++sTradeAnim->timer == 80)
        {
            sTradeAnim->releasePokeballSpriteId = CreateTradePokeballSprite(sTradeAnim->monSpriteIds[TRADE_PLAYER], gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].oam.paletteNum, 120, 32, 2, 1, 0x14, 0xfffff);
            sTradeAnim->state++;
            StringExpandPlaceholders(gStringVarBuffer, gText_ByeByeVar1);
            DrawTextOnTradeWindow(0, gStringVarBuffer, 0);
        }
        break;
    case STATE_POKEBALL_DEPART:
        if (gSprites[sTradeAnim->releasePokeballSpriteId].callback == SpriteCallbackDummy)
        {
            sTradeAnim->bouncingPokeballSpriteId = CreateSprite(&sSpriteTemplate_Pokeball, 120, 32, 0);
            gSprites[sTradeAnim->bouncingPokeballSpriteId].callback = SpriteCB_BouncingPokeballDepart;
            DestroySprite(&gSprites[sTradeAnim->releasePokeballSpriteId]);
            sTradeAnim->state++;
        }
        break;
    case STATE_POKEBALL_DEPART_WAIT:
        // The game waits here for the sprite to finish its animation sequence.
        break;
    case STATE_FADE_OUT_TO_GBA_SEND:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sTradeAnim->state = STATE_WAIT_FADE_OUT_TO_GBA_SEND;
        break;
    case STATE_WAIT_FADE_OUT_TO_GBA_SEND:
        if (!gPaletteFade.active)
        {
            SetTradeSequenceBgGpuRegs(4);
            FillWindowPixelBuffer(0, PIXEL_FILL(15));
            CopyWindowToVram(0, COPYWIN_FULL);
            sTradeAnim->state++;
        }
        break;
    case STATE_FADE_IN_TO_GBA_SEND:
        BeginNormalPaletteFade(PALETTES_ALL, -1, 16, 0, RGB_BLACK);
        sTradeAnim->state++;
        break;
    case STATE_WAIT_FADE_IN_TO_GBA_SEND:
        if (!gPaletteFade.active)
            sTradeAnim->state = STATE_GBA_ZOOM_OUT;
        break;
    case STATE_GBA_ZOOM_OUT:
        if (sTradeAnim->gbaScale > 0x100)
        {
            sTradeAnim->gbaScale -= 0x34;
        }
        else
        {
            SetTradeSequenceBgGpuRegs(1);
            sTradeAnim->gbaScale = 0x80;
            sTradeAnim->state++;
            sTradeAnim->timer = 0;
        }
        sTradeAnim->sXY = 0x8000 / sTradeAnim->gbaScale;
        break;
    case STATE_GBA_FLASH_SEND:
        if (++sTradeAnim->timer > 20)
        {
            SetTradeBGAffine();
            sTradeAnim->connectionSpriteId2 = CreateSprite(&sSpriteTemplate_GbaScreenFlash_Long, 120, 80, 0);
            sTradeAnim->state++;
        }
        break;
    case STATE_GBA_STOP_FLASH_SEND:
        if (gSprites[sTradeAnim->connectionSpriteId2].animEnded)
        {
            DestroySprite(&gSprites[sTradeAnim->connectionSpriteId2]);
            SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_BLEND |
                                         BLDCNT_TGT2_BG1 |
                                         BLDCNT_TGT2_BG2);
            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(12, 4));
            sTradeAnim->state++;
        }
        break;
    case STATE_PAN_AWAY_GBA:
        if (--sTradeAnim->bg1vofs == 316)
            sTradeAnim->state++;

        if (sTradeAnim->bg1vofs == 328)
            sTradeAnim->cableEndSpriteId = CreateSprite(&sSpriteTemplate_CableEnd, 128, 65, 0);
        break;
    case STATE_CREATE_LINK_MON_LEAVING:
        sTradeAnim->connectionSpriteId1 = CreateSprite(&sSpriteTemplate_LinkMonGlow, 128, 80, 3);
        sTradeAnim->connectionSpriteId2 = CreateSprite(&sSpriteTemplate_LinkMonShadow, 128, 80, 0);
        StartSpriteAnim(&gSprites[sTradeAnim->connectionSpriteId2], ANIM_LINKMON_SMALL);
        sTradeAnim->state++;
        break;
    case STATE_LINK_MON_TRAVEL_OUT:
        if ((sTradeAnim->bg1vofs -= 2) == 166)
            sTradeAnim->state = STATE_LINK_MON_TRAVEL_OFFSCREEN;

        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_1 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG1_ON |
                                      DISPCNT_OBJ_ON);
        break;
    case STATE_LINK_MON_TRAVEL_OFFSCREEN:
        gSprites[sTradeAnim->connectionSpriteId1].y -= 2;
        gSprites[sTradeAnim->connectionSpriteId2].y -= 2;
        if (gSprites[sTradeAnim->connectionSpriteId1].y < -8)
            sTradeAnim->state = STATE_FADE_OUT_TO_CROSSING;
        break;
    case STATE_FADE_OUT_TO_CROSSING:
        BeginNormalPaletteFade(PALETTES_ALL, -1, 0, 16, RGB_BLACK);
        sTradeAnim->state = STATE_WAIT_FADE_OUT_TO_CROSSING;
        break;
    case STATE_WAIT_FADE_OUT_TO_CROSSING:
        if (!gPaletteFade.active)
        {
            DestroySprite(&gSprites[sTradeAnim->connectionSpriteId1]);
            DestroySprite(&gSprites[sTradeAnim->connectionSpriteId2]);
            SetTradeSequenceBgGpuRegs(2);
            sTradeAnim->state++;
        }
        break;
    case STATE_FADE_IN_TO_CROSSING:
        BeginNormalPaletteFade(PALETTES_ALL, -1, 16, 0, RGB_BLACK);
        sTradeAnim->connectionSpriteId1 = CreateSprite(&sSpriteTemplate_LinkMonShadow, 111, 170, 0);
        sTradeAnim->connectionSpriteId2 = CreateSprite(&sSpriteTemplate_LinkMonShadow, 129, -10, 0);
        sTradeAnim->state++;
        break;
    case STATE_WAIT_FADE_IN_TO_CROSSING:
        if (!gPaletteFade.active)
        {
            PlaySE(SE_WARP_OUT);
            sTradeAnim->state++;
        }
        gSprites[sTradeAnim->connectionSpriteId1].y2 -= 3;
        gSprites[sTradeAnim->connectionSpriteId2].y2 += 3;
        break;
    case STATE_CROSSING_LINK_MONS_ENTER:
        gSprites[sTradeAnim->connectionSpriteId1].y2 -= 3;
        gSprites[sTradeAnim->connectionSpriteId2].y2 += 3;
        if (gSprites[sTradeAnim->connectionSpriteId1].y2 <= -90)
        {
            gSprites[sTradeAnim->connectionSpriteId1].data[1] = 1;
            gSprites[sTradeAnim->connectionSpriteId2].data[1] = 1;
            sTradeAnim->state++;
        }
        break;
    case STATE_CROSSING_BLEND_WHITE_1:
        BlendPalettes(0x1, 16, RGB_WHITEALPHA);
        sTradeAnim->state++;
        break;
    case STATE_CROSSING_BLEND_WHITE_2:
        BlendPalettes(0x1, 0, RGB_WHITEALPHA);
        sTradeAnim->state++;
        break;
    case STATE_CROSSING_BLEND_WHITE_3:
        BlendPalettes(0x1, 16, RGB_WHITEALPHA);
        sTradeAnim->state++;
        break;
    case STATE_CROSSING_CREATE_MON_PICS:
        if (!IsMonSpriteNotFlipped(sTradeAnim->monSpecies[TRADE_PLAYER]))
        {
            gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].affineAnims = sAffineAnims_CrossingMonPics;
            gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].oam.affineMode = ST_OAM_AFFINE_DOUBLE;
            CalcCenterToCornerVec(&gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]], SPRITE_SHAPE(64x64), SPRITE_SIZE(64x64), ST_OAM_AFFINE_DOUBLE);
            StartSpriteAffineAnim(&gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]], 0);
        }
        else
        {
            StartSpriteAffineAnim(&gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]], 0);
        }
        StartSpriteAffineAnim(&gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]], 0);
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].x = 60;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].x = 180;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].y = 192;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].y = -32;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].invisible = FALSE;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].invisible = FALSE;
        sTradeAnim->state++;
        break;
    case STATE_CROSSING_MON_PICS_MOVE:
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].y2 -= 3;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].y2 += 3;
        if (gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].y2 < -DISPLAY_HEIGHT
         && gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].y2 >= -DISPLAY_HEIGHT - 3)
        {
            PlaySE(SE_WARP_IN);
        }
        if (gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].y2 < -222)
        {
            gSprites[sTradeAnim->connectionSpriteId1].data[1] = 0;
            gSprites[sTradeAnim->connectionSpriteId2].data[1] = 0;
            sTradeAnim->state++;
            gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].invisible = TRUE;
            gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].invisible = TRUE;
            BlendPalettes(0x1, 0, RGB_WHITEALPHA);
        }
        break;
    case STATE_CROSSING_LINK_MONS_EXIT:
        gSprites[sTradeAnim->connectionSpriteId1].y2 -= 3;
        gSprites[sTradeAnim->connectionSpriteId2].y2 += 3;
        if (gSprites[sTradeAnim->connectionSpriteId1].y2 <= -222)
        {
            BeginNormalPaletteFade(PALETTES_ALL, -1, 0, 16, RGB_BLACK);
            sTradeAnim->state++;
            DestroySprite(&gSprites[sTradeAnim->connectionSpriteId1]);
            DestroySprite(&gSprites[sTradeAnim->connectionSpriteId2]);
        }
        break;
    case STATE_CREATE_LINK_MON_ARRIVING:
        if (!gPaletteFade.active)
        {
            sTradeAnim->state++;
            SetTradeSequenceBgGpuRegs(1);
            sTradeAnim->bg1vofs = 166;
            sTradeAnim->connectionSpriteId1 = CreateSprite(&sSpriteTemplate_LinkMonGlow, 128, -20, 3);
            sTradeAnim->connectionSpriteId2 = CreateSprite(&sSpriteTemplate_LinkMonShadow, 128, -20, 0);
            StartSpriteAnim(&gSprites[sTradeAnim->connectionSpriteId2], ANIM_LINKMON_SMALL);
        }
        break;
    case STATE_FADE_OUT_TO_GBA_RECV:
        BeginNormalPaletteFade(PALETTES_ALL, -1, 16, 0, RGB_BLACK);
        sTradeAnim->state++;
        break;
    case STATE_WAIT_FADE_OUT_TO_GBA_RECV:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG1_ON |
                                      DISPCNT_OBJ_ON);
        if (!gPaletteFade.active)
            sTradeAnim->state++;
        break;
    case STATE_LINK_MON_TRAVEL_IN:
        gSprites[sTradeAnim->connectionSpriteId1].y2 += 3;
        gSprites[sTradeAnim->connectionSpriteId2].y2 += 3;
        if (gSprites[sTradeAnim->connectionSpriteId1].y2 + gSprites[sTradeAnim->connectionSpriteId1].y == 64)
            sTradeAnim->state++;
        break;
    case STATE_PAN_TO_GBA:
        if ((sTradeAnim->bg1vofs += 2) > 316)
        {
            sTradeAnim->bg1vofs = 316;
            sTradeAnim->state++;
        }
        break;
    case STATE_DESTROY_LINK_MON:
        DestroySprite(&gSprites[sTradeAnim->connectionSpriteId1]);
        DestroySprite(&gSprites[sTradeAnim->connectionSpriteId2]);
        sTradeAnim->state++;
        sTradeAnim->timer = 0;
        break;
    case STATE_LINK_MON_ARRIVED_DELAY:
        if (++sTradeAnim->timer == 10)
            sTradeAnim->state++;
        break;
    case STATE_MOVE_GBA_TO_CENTER:
        if (++sTradeAnim->bg1vofs > 348)
        {
            sTradeAnim->bg1vofs = 348;
            sTradeAnim->state++;
        }
        if (sTradeAnim->bg1vofs == 328 && sTradeAnim->isCableTrade)
        {
            sTradeAnim->cableEndSpriteId = CreateSprite(&sSpriteTemplate_CableEnd, 128, 65, 0);
            gSprites[sTradeAnim->cableEndSpriteId].callback = SpriteCB_CableEndReceiving;
        }
        break;
    case STATE_GBA_FLASH_RECV:
        sTradeAnim->connectionSpriteId2 = CreateSprite(&sSpriteTemplate_GbaScreenFlash_Long, 120, 80, 0);
        sTradeAnim->state = STATE_GBA_STOP_FLASH_RECV;
        break;
    case STATE_GBA_STOP_FLASH_RECV:
        if (gSprites[sTradeAnim->connectionSpriteId2].animEnded)
        {
            DestroySprite(&gSprites[sTradeAnim->connectionSpriteId2]);
            SetTradeSequenceBgGpuRegs(6);
            sTradeAnim->state++;
            PlaySE(SE_M_SAND_ATTACK);
        }
        break;
    case STATE_GBA_ZOOM_IN:
        if (sTradeAnim->gbaScale < 0x400)
        {
            sTradeAnim->gbaScale += 0x34;
        }
        else
        {
            sTradeAnim->gbaScale = 0x400;
            sTradeAnim->state++;
        }
        sTradeAnim->sXY = 0x8000 / sTradeAnim->gbaScale;
        break;
    case STATE_FADE_OUT_TO_NEW_MON:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sTradeAnim->state = STATE_WAIT_FADE_OUT_TO_NEW_MON;
        break;

    case STATE_WAIT_FADE_OUT_TO_NEW_MON:
        if (!gPaletteFade.active)
        {
            SetTradeSequenceBgGpuRegs(5);
            SetTradeSequenceBgGpuRegs(7);
            gPaletteFade.bufferTransferDisabled = TRUE;
            sTradeAnim->state++;
        }
        break;
    case STATE_FADE_IN_TO_NEW_MON:
        gPaletteFade.bufferTransferDisabled = FALSE;
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        sTradeAnim->state++;
        break;
    case STATE_WAIT_FADE_IN_TO_NEW_MON:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG2_ON |
                                      DISPCNT_OBJ_ON);
        if (!gPaletteFade.active)
            sTradeAnim->state++;
        break;
    case STATE_POKEBALL_ARRIVE:
        sTradeAnim->bouncingPokeballSpriteId = CreateSprite(&sSpriteTemplate_Pokeball, 120, -8, 0);
        gSprites[sTradeAnim->bouncingPokeballSpriteId].data[3] = 74;
        gSprites[sTradeAnim->bouncingPokeballSpriteId].callback = SpriteCB_BouncingPokeballArrive;
        StartSpriteAnim(&gSprites[sTradeAnim->bouncingPokeballSpriteId], 1);
        StartSpriteAffineAnim(&gSprites[sTradeAnim->bouncingPokeballSpriteId], 2);
        BlendPalettes(1 << (16 + gSprites[sTradeAnim->bouncingPokeballSpriteId].oam.paletteNum), 16, RGB_WHITEALPHA);
        sTradeAnim->state++;
        sTradeAnim->timer = 0;
        break;
    case STATE_FADE_POKEBALL_TO_NORMAL:
        BeginNormalPaletteFade(1 << (16 + gSprites[sTradeAnim->bouncingPokeballSpriteId].oam.paletteNum), 1, 16, 0, RGB_WHITEALPHA);
        sTradeAnim->state++;
        break;
    case STATE_POKEBALL_ARRIVE_WAIT:
        if (gSprites[sTradeAnim->bouncingPokeballSpriteId].callback == SpriteCallbackDummy)
        {
            HandleLoadSpecialPokePic_2(&gMonFrontPicTable[sTradeAnim->monSpecies[TRADE_PARTNER]],
                                        gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_RIGHT],
                                        sTradeAnim->monSpecies[TRADE_PARTNER],
                                        sTradeAnim->monPersonalities[TRADE_PARTNER]);
            sTradeAnim->state++;
        }
        break;
    case STATE_SHOW_NEW_MON:
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].x = 120;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].y = gMonFrontPicCoords[sTradeAnim->monSpecies[TRADE_PARTNER]].y_offset + 60;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].x2 = 0;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].y2 = 0;
        StartSpriteAnim(&gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]], 0);
        CreatePokeballSpriteToReleaseMon(sTradeAnim->monSpriteIds[TRADE_PARTNER], gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].oam.paletteNum, 120, 84, 2, 1, 20, PALETTES_BG | (0xF << 16), sTradeAnim->monSpecies[TRADE_PARTNER]);
        FreeSpriteOamMatrix(&gSprites[sTradeAnim->bouncingPokeballSpriteId]);
        DestroySprite(&gSprites[sTradeAnim->bouncingPokeballSpriteId]);
        sTradeAnim->state++;
        break;
    case STATE_NEW_MON_MSG:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG0_ON |
                                      DISPCNT_BG2_ON |
                                      DISPCNT_OBJ_ON);
        StringExpandPlaceholders(gStringVarBuffer, gText_XSentOverY);
        DrawTextOnTradeWindow(0, gStringVarBuffer, 0);
        sTradeAnim->state = STATE_DELAY_FOR_MON_ANIM;
        sTradeAnim->timer = 0;
        break;
    case STATE_DELAY_FOR_MON_ANIM:
        if (++sTradeAnim->timer > 60)
        {
            sTradeAnim->state = STATE_WAIT_FOR_MON_CRY;
            sTradeAnim->timer = 0;
        }
        break;
    case STATE_WAIT_FOR_MON_CRY:
        if (IsCryFinished())
            sTradeAnim->state = STATE_TAKE_CARE_OF_MON;
        break;
    case STATE_TAKE_CARE_OF_MON:
        if (++sTradeAnim->timer == 10)
            PlayFanfare(MUS_EVOLVED);

        if (sTradeAnim->timer == 250)
        {
            sTradeAnim->state++;
            StringExpandPlaceholders(gStringVarBuffer, gText_TakeGoodCareOfX);
            DrawTextOnTradeWindow(0, gStringVarBuffer, 0);
            sTradeAnim->timer = 0;
        }
        break;
    case STATE_AFTER_NEW_MON_DELAY:
        if (++sTradeAnim->timer == 60)
            sTradeAnim->state++;
        break;
    case STATE_CHECK_RIBBONS:
        CheckPartnersMonForRibbons();
        sTradeAnim->state++;
        break;
    case STATE_END_LINK_TRADE:
        if (sTradeAnim->isLinkTrade)
            return TRUE;
        else if (JOY_NEW(A_BUTTON))
            sTradeAnim->state++;
        break;
    case STATE_TRY_EVOLUTION: // Only if in-game trade, link trades use CB2_TryLinkTradeEvolution
        TradeMons(gSpecialVar_0x8005, 0);
        gCB2_AfterEvolution = CB2_InGameTrade;
        evoTarget = GetEvolutionTargetSpecies(&gPlayerParty[gSelectedTradeMonPositions[TRADE_PLAYER]], EVO_MODE_TRADE, ITEM_NONE);
        if (evoTarget != SPECIES_NONE)
            TradeEvolutionScene(&gPlayerParty[gSelectedTradeMonPositions[TRADE_PLAYER]], evoTarget, sTradeAnim->monSpriteIds[TRADE_PARTNER], gSelectedTradeMonPositions[TRADE_PLAYER]);
        sTradeAnim->state++;
        break;
    case STATE_FADE_OUT_END:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sTradeAnim->state++;
        break;
    case STATE_WAIT_FADE_OUT_END:
        if (!gPaletteFade.active)
        {
            PlayNewMapMusic(sTradeAnim->cachedMapMusic);
            if (sTradeAnim)
            {
                FreeAllWindowBuffers();
                Free(GetBgTilemapBuffer(3));
                Free(GetBgTilemapBuffer(1));
                Free(GetBgTilemapBuffer(0));
                FreeMonSpritesGfx();
                FREE_AND_SET_NULL(sTradeAnim);
            }
            SetMainCallback2(CB2_ReturnToField);
            BufferInGameTradeMonName();
        }
        break;
    }
    return FALSE;
}

// Task data for Task_AnimateWirelessSignal
#define tIdx               data[0]
#define tCounter           data[1]
#define tSignalComingBack  data[2]

static bool8 DoTradeAnim_Wireless(void)
{
    u16 evoTarget;

    switch (sTradeAnim->state)
    {
    case STATE_START:
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].invisible = FALSE;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].x2 = -180;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].y2 = gMonFrontPicCoords[sTradeAnim->monSpecies[TRADE_PLAYER]].y_offset;
        sTradeAnim->state++;
        sTradeAnim->cachedMapMusic = GetCurrentMapMusic();
        PlayNewMapMusic(MUS_EVOLUTION);
        break;
    case STATE_MON_SLIDE_IN:
        if (sTradeAnim->bg2hofs > 0)
        {
            gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].x2 += 3;
            sTradeAnim->bg2hofs -= 3;
        }
        else
        {
            gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].x2 = 0;
            sTradeAnim->bg2hofs = 0;
            sTradeAnim->state = STATE_SEND_MSG;
        }
        break;
    case STATE_SEND_MSG:
        StringExpandPlaceholders(gStringVarBuffer, gText_XWillBeSentToY);
        DrawTextOnTradeWindow(0, gStringVarBuffer, 0);

        if (sTradeAnim->monSpecies[TRADE_PLAYER] != SPECIES_EGG)
            PlayCry_Normal(sTradeAnim->monSpecies[TRADE_PLAYER], 0);

        sTradeAnim->state = STATE_BYE_BYE;
        sTradeAnim->timer = 0;
        break;
    case STATE_BYE_BYE:
        if (++sTradeAnim->timer == 80)
        {
            sTradeAnim->releasePokeballSpriteId = CreateTradePokeballSprite(sTradeAnim->monSpriteIds[TRADE_PLAYER], gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].oam.paletteNum, 120, 32, 2, 1, 0x14, 0xfffff);
            sTradeAnim->state++;
            StringExpandPlaceholders(gStringVarBuffer, gText_ByeByeVar1);
            DrawTextOnTradeWindow(0, gStringVarBuffer, 0);
        }
        break;
    case STATE_POKEBALL_DEPART:
        if (gSprites[sTradeAnim->releasePokeballSpriteId].callback == SpriteCallbackDummy)
        {
            sTradeAnim->bouncingPokeballSpriteId = CreateSprite(&sSpriteTemplate_Pokeball, 120, 32, 0);
            gSprites[sTradeAnim->bouncingPokeballSpriteId].callback = SpriteCB_BouncingPokeballDepart;
            DestroySprite(&gSprites[sTradeAnim->releasePokeballSpriteId]);
            sTradeAnim->state++;
        }
        break;
    case STATE_POKEBALL_DEPART_WAIT:
        // The game waits here for the sprite to finish its animation sequence.
        break;
    case STATE_FADE_OUT_TO_GBA_SEND:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sTradeAnim->state = STATE_WAIT_FADE_OUT_TO_GBA_SEND;
        break;
    case STATE_WAIT_FADE_OUT_TO_GBA_SEND:
        if (!gPaletteFade.active)
        {
            SetTradeSequenceBgGpuRegs(4);
            FillWindowPixelBuffer(0, PIXEL_FILL(15));
            CopyWindowToVram(0, COPYWIN_FULL);
            sTradeAnim->state++;
        }
        break;
    case STATE_FADE_IN_TO_GBA_SEND:
        BeginNormalPaletteFade(PALETTES_ALL, -1, 16, 0, RGB_BLACK);
        sTradeAnim->state++;
        break;
    case STATE_WAIT_FADE_IN_TO_GBA_SEND:
        if (!gPaletteFade.active)
            sTradeAnim->state = STATE_GBA_ZOOM_OUT;
        break;
    case STATE_GBA_ZOOM_OUT:
        if (sTradeAnim->gbaScale > 0x100)
        {
            sTradeAnim->gbaScale -= 0x34;
        }
        else
        {
            SetTradeSequenceBgGpuRegs(1);
            sTradeAnim->gbaScale = 0x80;
            sTradeAnim->state = STATE_GBA_FLASH_SEND_WIRELESS;
            sTradeAnim->timer = 0;
        }
        sTradeAnim->sXY = 0x8000 / sTradeAnim->gbaScale;
        break;
    case STATE_GBA_FLASH_SEND_WIRELESS:
        if (++sTradeAnim->timer > 20)
        {
            SetTradeSequenceBgGpuRegs(3);
            sTradeAnim->connectionSpriteId2 = CreateSprite(&sSpriteTemplate_GbaScreenFlash_Short, 120, 80, 0);
            sTradeAnim->state++;
        }
        break;
    case STATE_GBA_STOP_FLASH_SEND_WIRELESS:
        if (gSprites[sTradeAnim->connectionSpriteId2].animEnded)
        {
            DestroySprite(&gSprites[sTradeAnim->connectionSpriteId2]);
            SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 |
                                         BLDCNT_TGT1_OBJ |
                                         BLDCNT_EFFECT_BLEND |
                                         BLDCNT_TGT2_BG2);
            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(16, 4));

            // Start wireless signal effect
            CreateTask(Task_AnimateWirelessSignal, 5);
            sTradeAnim->state++;
        }
        break;
    case STATE_WAIT_WIRELESS_SIGNAL_SEND:
        if (!FuncIsActiveTask(Task_AnimateWirelessSignal))
            sTradeAnim->state = STATE_PAN_AWAY_GBA;
        break;
    case STATE_PAN_AWAY_GBA:
        if (--sTradeAnim->bg1vofs == 316)
            sTradeAnim->state++;
        break;
    case STATE_CREATE_LINK_MON_LEAVING:
        sTradeAnim->connectionSpriteId1 = CreateSprite(&sSpriteTemplate_LinkMonGlow, 120, 80, 3);
        gSprites[sTradeAnim->connectionSpriteId1].callback = SpriteCB_LinkMonGlowWireless;
        sTradeAnim->connectionSpriteId2 = CreateSprite(&sSpriteTemplate_LinkMonShadow, 120, 80, 0);
        StartSpriteAnim(&gSprites[sTradeAnim->connectionSpriteId2], ANIM_LINKMON_SMALL);
        sTradeAnim->state++;
        break;
    case STATE_LINK_MON_TRAVEL_OUT:
        if ((sTradeAnim->bg1vofs -= 3) == 166)
            sTradeAnim->state = STATE_LINK_MON_TRAVEL_OFFSCREEN;

        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_1 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG1_ON |
                                      DISPCNT_OBJ_ON);
        break;
    case STATE_LINK_MON_TRAVEL_OFFSCREEN:
        gSprites[sTradeAnim->connectionSpriteId1].y -= 2;
        gSprites[sTradeAnim->connectionSpriteId2].y -= 2;
        if (gSprites[sTradeAnim->connectionSpriteId1].y < -8)
            sTradeAnim->state = STATE_FADE_OUT_TO_CROSSING;
        break;
    case STATE_FADE_OUT_TO_CROSSING:
        BeginNormalPaletteFade(PALETTES_ALL, -1, 0, 16, RGB_BLACK);
        sTradeAnim->state = STATE_WAIT_FADE_OUT_TO_CROSSING;
        break;
    case STATE_WAIT_FADE_OUT_TO_CROSSING:
        if (!gPaletteFade.active)
        {
            DestroySprite(&gSprites[sTradeAnim->connectionSpriteId1]);
            DestroySprite(&gSprites[sTradeAnim->connectionSpriteId2]);
            SetTradeSequenceBgGpuRegs(2);
            sTradeAnim->state++;
        }
        break;
    case STATE_FADE_IN_TO_CROSSING:
        BeginNormalPaletteFade(PALETTES_ALL, -1, 16, 0, RGB_BLACK);
        sTradeAnim->connectionSpriteId1 = CreateSprite(&sSpriteTemplate_LinkMonShadow, 111, 170, 0);
        sTradeAnim->connectionSpriteId2 = CreateSprite(&sSpriteTemplate_LinkMonShadow, 129, -10, 0);
        sTradeAnim->state++;
        break;
    case STATE_WAIT_FADE_IN_TO_CROSSING:
        if (!gPaletteFade.active)
        {
            PlaySE(SE_WARP_OUT);
            sTradeAnim->state++;
        }
        gSprites[sTradeAnim->connectionSpriteId1].y2 -= 3;
        gSprites[sTradeAnim->connectionSpriteId2].y2 += 3;
        break;
    case STATE_CROSSING_LINK_MONS_ENTER:
        gSprites[sTradeAnim->connectionSpriteId1].y2 -= 3;
        gSprites[sTradeAnim->connectionSpriteId2].y2 += 3;
        if (gSprites[sTradeAnim->connectionSpriteId1].y2 <= -90)
        {
            gSprites[sTradeAnim->connectionSpriteId1].data[1] = 1;
            gSprites[sTradeAnim->connectionSpriteId2].data[1] = 1;
            sTradeAnim->state++;
            CreateTask(Task_OpenCenterWhiteColumn, 5);
        }
        break;
    case STATE_CROSSING_BLEND_WHITE_1:
        BlendPalettes(0x8, 16, RGB_WHITEALPHA);
        sTradeAnim->state++;
        break;
    case STATE_CROSSING_BLEND_WHITE_2:
        BlendPalettes(0x8, 16, RGB_WHITEALPHA);
        sTradeAnim->state++;
        break;
    case STATE_CROSSING_BLEND_WHITE_3:
        BlendPalettes(0x8, 16, RGB_WHITEALPHA);
        sTradeAnim->state++;
        break;
    case STATE_CROSSING_CREATE_MON_PICS:
        if (!IsMonSpriteNotFlipped(sTradeAnim->monSpecies[TRADE_PLAYER]))
        {
            gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].affineAnims = sAffineAnims_CrossingMonPics;
            gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].oam.affineMode = ST_OAM_AFFINE_DOUBLE;
            CalcCenterToCornerVec(&gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]], SPRITE_SHAPE(64x64), SPRITE_SIZE(64x64), ST_OAM_AFFINE_DOUBLE);
            StartSpriteAffineAnim(&gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]], 0);
        }
        else
        {
            StartSpriteAffineAnim(&gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]], 0);
        }
        StartSpriteAffineAnim(&gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]], 0);
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].x = 40;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].x = 200;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].y = 192;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].y = -32;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].invisible = FALSE;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].invisible = FALSE;
        sTradeAnim->state++;
        break;
    case STATE_CROSSING_MON_PICS_MOVE:
        gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].y2 -= 3;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].y2 += 3;
        if (gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].y2 < -DISPLAY_HEIGHT
         && gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].y2 >= -DISPLAY_HEIGHT - 3)
        {
            PlaySE(SE_WARP_IN);
        }
        if (gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].y2 < -222)
        {
            gSprites[sTradeAnim->connectionSpriteId1].data[1] = 0;
            gSprites[sTradeAnim->connectionSpriteId2].data[1] = 0;
            sTradeAnim->state++;
            gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]].invisible = TRUE;
            gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].invisible = TRUE;
            CreateTask(Task_CloseCenterWhiteColumn, 5);
        }
        break;
    case STATE_CROSSING_LINK_MONS_EXIT:
        gSprites[sTradeAnim->connectionSpriteId1].y2 -= 3;
        gSprites[sTradeAnim->connectionSpriteId2].y2 += 3;
        if (gSprites[sTradeAnim->connectionSpriteId1].y2 <= -222)
        {
            BeginNormalPaletteFade(PALETTES_ALL, -1, 0, 16, RGB_BLACK);
            sTradeAnim->state++;
            DestroySprite(&gSprites[sTradeAnim->connectionSpriteId1]);
            DestroySprite(&gSprites[sTradeAnim->connectionSpriteId2]);
        }
        break;
    case STATE_CREATE_LINK_MON_ARRIVING:
        if (!gPaletteFade.active)
        {
            sTradeAnim->state++;
            SetTradeSequenceBgGpuRegs(1);
            sTradeAnim->bg1vofs = 166;
            SetTradeSequenceBgGpuRegs(3);
            sTradeAnim->bg2vofs = 412;
            sTradeAnim->connectionSpriteId1 = CreateSprite(&sSpriteTemplate_LinkMonGlow, 120, -20, 3);
            gSprites[sTradeAnim->connectionSpriteId1].callback = SpriteCB_LinkMonGlowWireless;
            sTradeAnim->connectionSpriteId2 = CreateSprite(&sSpriteTemplate_LinkMonShadow, 120, -20, 0);
            StartSpriteAnim(&gSprites[sTradeAnim->connectionSpriteId2], ANIM_LINKMON_SMALL);
        }
        break;
    case STATE_FADE_OUT_TO_GBA_RECV:
        BeginNormalPaletteFade(PALETTES_ALL, -1, 16, 0, RGB_BLACK);
        sTradeAnim->state++;
        break;
    case STATE_WAIT_FADE_OUT_TO_GBA_RECV:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG1_ON |
                                      DISPCNT_OBJ_ON);
        if (!gPaletteFade.active)
            sTradeAnim->state++;
        break;
    case STATE_LINK_MON_TRAVEL_IN:
        gSprites[sTradeAnim->connectionSpriteId1].y2 += 4;
        gSprites[sTradeAnim->connectionSpriteId2].y2 += 4;
        if (gSprites[sTradeAnim->connectionSpriteId1].y2 + gSprites[sTradeAnim->connectionSpriteId1].y == 64)
        {
            sTradeAnim->state = STATE_PAN_TO_GBA_WIRELESS;
            sTradeAnim->timer = 0;
        }
        break;
    case STATE_PAN_TO_GBA_WIRELESS:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG1_ON |
                                      DISPCNT_BG2_ON |
                                      DISPCNT_OBJ_ON);
        sTradeAnim->bg1vofs += 3;
        sTradeAnim->bg2vofs += 3;
        if (++sTradeAnim->timer == 10)
        {
            u8 taskId = CreateTask(Task_AnimateWirelessSignal, 5);
            gTasks[taskId].tSignalComingBack = TRUE;
        }
        if (sTradeAnim->bg1vofs > 316)
        {
            sTradeAnim->bg1vofs = 316;
            sTradeAnim->state++;
        }
        break;
    case STATE_DESTROY_LINK_MON_WIRELESS:
        DestroySprite(&gSprites[sTradeAnim->connectionSpriteId1]);
        DestroySprite(&gSprites[sTradeAnim->connectionSpriteId2]);
        sTradeAnim->state++;
        sTradeAnim->timer = 0;
        break;
    case STATE_WAIT_WIRELESS_SIGNAL_RECV:
        if (!FuncIsActiveTask(Task_AnimateWirelessSignal))
        {
            sTradeAnim->state = STATE_LINK_MON_ARRIVED_DELAY;
            sTradeAnim->timer = 0;
        }
        break;
    case STATE_LINK_MON_ARRIVED_DELAY:
        if (++sTradeAnim->timer == 10)
            sTradeAnim->state++;
        break;
    case STATE_MOVE_GBA_TO_CENTER:
        if (++sTradeAnim->bg1vofs > 348)
        {
            sTradeAnim->bg1vofs = 348;
            sTradeAnim->state++;
        }
        break;
    case STATE_GBA_FLASH_RECV:
        sTradeAnim->connectionSpriteId2 = CreateSprite(&sSpriteTemplate_GbaScreenFlash_Long, 120, 80, 0);
        sTradeAnim->state = STATE_GBA_STOP_FLASH_RECV;
        break;
    case STATE_GBA_STOP_FLASH_RECV:
        if (gSprites[sTradeAnim->connectionSpriteId2].animEnded)
        {
            DestroySprite(&gSprites[sTradeAnim->connectionSpriteId2]);
            SetTradeSequenceBgGpuRegs(6);
            sTradeAnim->state++;
            PlaySE(SE_M_SAND_ATTACK);
        }
        break;
    case STATE_GBA_ZOOM_IN:
        if (sTradeAnim->gbaScale < 0x400)
        {
            sTradeAnim->gbaScale += 0x34;
        }
        else
        {
            sTradeAnim->gbaScale = 0x400;
            sTradeAnim->state++;
        }
        sTradeAnim->sXY = 0x8000 / sTradeAnim->gbaScale;
        break;
    case STATE_FADE_OUT_TO_NEW_MON:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sTradeAnim->state = STATE_WAIT_FADE_OUT_TO_NEW_MON;
        break;
    case STATE_WAIT_FADE_OUT_TO_NEW_MON:
        if (!gPaletteFade.active)
        {
            SetTradeSequenceBgGpuRegs(5);
            SetTradeSequenceBgGpuRegs(7);
            gPaletteFade.bufferTransferDisabled = TRUE;
            sTradeAnim->state++;
        }
        break;
    case STATE_FADE_IN_TO_NEW_MON:
        gPaletteFade.bufferTransferDisabled = FALSE;
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        sTradeAnim->state++;
        break;
    case STATE_WAIT_FADE_IN_TO_NEW_MON:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG2_ON |
                                      DISPCNT_OBJ_ON);
        if (!gPaletteFade.active)
            sTradeAnim->state++;
        break;
    case STATE_POKEBALL_ARRIVE:
        sTradeAnim->bouncingPokeballSpriteId = CreateSprite(&sSpriteTemplate_Pokeball, 120, -8, 0);
        gSprites[sTradeAnim->bouncingPokeballSpriteId].data[3] = 74;
        gSprites[sTradeAnim->bouncingPokeballSpriteId].callback = SpriteCB_BouncingPokeballArrive;
        StartSpriteAnim(&gSprites[sTradeAnim->bouncingPokeballSpriteId], 1);
        StartSpriteAffineAnim(&gSprites[sTradeAnim->bouncingPokeballSpriteId], 2);
        BlendPalettes(1 << (16 + gSprites[sTradeAnim->bouncingPokeballSpriteId].oam.paletteNum), 16, RGB_WHITEALPHA);
        sTradeAnim->state++;
        sTradeAnim->timer = 0;
        break;
    case STATE_FADE_POKEBALL_TO_NORMAL:
        BeginNormalPaletteFade(1 << (16 + gSprites[sTradeAnim->bouncingPokeballSpriteId].oam.paletteNum), 1, 16, 0, RGB_WHITEALPHA);
        sTradeAnim->state++;
        break;
    case STATE_POKEBALL_ARRIVE_WAIT:
        if (gSprites[sTradeAnim->bouncingPokeballSpriteId].callback == SpriteCallbackDummy)
        {
            HandleLoadSpecialPokePic_2(&gMonFrontPicTable[sTradeAnim->monSpecies[TRADE_PARTNER]],
                                        gMonSpritesGfxPtr->sprites.ptr[B_POSITION_OPPONENT_RIGHT],
                                        sTradeAnim->monSpecies[TRADE_PARTNER],
                                        sTradeAnim->monPersonalities[TRADE_PARTNER]);
            sTradeAnim->state++;
        }
        break;
    case STATE_SHOW_NEW_MON:
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].x = 120;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].y = gMonFrontPicCoords[sTradeAnim->monSpecies[TRADE_PARTNER]].y_offset + 60;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].x2 = 0;
        gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].y2 = 0;
        StartSpriteAnim(&gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]], 0);
        CreatePokeballSpriteToReleaseMon(sTradeAnim->monSpriteIds[TRADE_PARTNER], gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]].oam.paletteNum, 120, 84, 2, 1, 20, PALETTES_BG | (0xF << 16), sTradeAnim->monSpecies[TRADE_PARTNER]);
        FreeSpriteOamMatrix(&gSprites[sTradeAnim->bouncingPokeballSpriteId]);
        DestroySprite(&gSprites[sTradeAnim->bouncingPokeballSpriteId]);
        sTradeAnim->state++;
        break;
    case STATE_NEW_MON_MSG:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 |
                                      DISPCNT_OBJ_1D_MAP |
                                      DISPCNT_BG0_ON |
                                      DISPCNT_BG2_ON |
                                      DISPCNT_OBJ_ON);
        StringExpandPlaceholders(gStringVarBuffer, gText_XSentOverY);
        DrawTextOnTradeWindow(0, gStringVarBuffer, 0);
        sTradeAnim->state = STATE_DELAY_FOR_MON_ANIM;
        sTradeAnim->timer = 0;
        break;
    case STATE_DELAY_FOR_MON_ANIM:
        if (++sTradeAnim->timer > 60)
        {
            sTradeAnim->state = STATE_WAIT_FOR_MON_CRY;
            sTradeAnim->timer = 0;
        }
        break;
    case STATE_WAIT_FOR_MON_CRY:
        if (IsCryFinished())
            sTradeAnim->state = STATE_TAKE_CARE_OF_MON;
        break;
    case STATE_TAKE_CARE_OF_MON:
        if (++sTradeAnim->timer == 10)
            PlayFanfare(MUS_EVOLVED);

        if (sTradeAnim->timer == 250)
        {
            sTradeAnim->state++;
            StringExpandPlaceholders(gStringVarBuffer, gText_TakeGoodCareOfX);
            DrawTextOnTradeWindow(0, gStringVarBuffer, 0);
            sTradeAnim->timer = 0;
        }
        break;
    case STATE_AFTER_NEW_MON_DELAY:
        if (++sTradeAnim->timer == 60)
            sTradeAnim->state++;
        break;
    case STATE_CHECK_RIBBONS:
        CheckPartnersMonForRibbons();
        sTradeAnim->state++;
        break;
    case STATE_END_LINK_TRADE:
        if (sTradeAnim->isLinkTrade)
            return TRUE;
        else if (JOY_NEW(A_BUTTON))
            sTradeAnim->state++;
        break;
    case STATE_TRY_EVOLUTION: // Only if in-game trade, link trades use CB2_TryLinkTradeEvolution
        TradeMons(gSpecialVar_0x8005, 0);
        gCB2_AfterEvolution = CB2_InGameTrade;
        evoTarget = GetEvolutionTargetSpecies(&gPlayerParty[gSelectedTradeMonPositions[TRADE_PLAYER]], EVO_MODE_TRADE, ITEM_NONE);
        if (evoTarget != SPECIES_NONE)
            TradeEvolutionScene(&gPlayerParty[gSelectedTradeMonPositions[TRADE_PLAYER]], evoTarget, sTradeAnim->monSpriteIds[TRADE_PARTNER], gSelectedTradeMonPositions[TRADE_PLAYER]);
        sTradeAnim->state++;
        break;
    case STATE_FADE_OUT_END:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sTradeAnim->state++;
        break;
    case STATE_WAIT_FADE_OUT_END:
        if (!gPaletteFade.active)
        {
            PlayNewMapMusic(sTradeAnim->cachedMapMusic);
            if (sTradeAnim)
            {
                FreeAllWindowBuffers();
                Free(GetBgTilemapBuffer(3));
                Free(GetBgTilemapBuffer(1));
                Free(GetBgTilemapBuffer(0));
                FreeMonSpritesGfx();
                FREE_AND_SET_NULL(sTradeAnim);
            }
            SetMainCallback2(CB2_ReturnToField);
            BufferInGameTradeMonName();
        }
        break;
    }
    return FALSE;
}

// Try to evolve a Pokémon received in a link trade
// In-game trades resolve evolution during the trade sequence, in STATE_TRY_EVOLUTION
static void CB2_TryLinkTradeEvolution(void)
{
    u16 evoTarget;
    switch (gMain.state)
    {
    case 0:
        gMain.state = 4;
        gSoftResetDisabled = TRUE;
        break;
    case 4:
        gCB2_AfterEvolution = CB2_SaveAndEndTrade;
        evoTarget = GetEvolutionTargetSpecies(&gPlayerParty[gSelectedTradeMonPositions[TRADE_PLAYER]], EVO_MODE_TRADE, ITEM_NONE);
        if (evoTarget != SPECIES_NONE)
            TradeEvolutionScene(&gPlayerParty[gSelectedTradeMonPositions[TRADE_PLAYER]], evoTarget, sTradeAnim->monSpriteIds[TRADE_PARTNER], gSelectedTradeMonPositions[TRADE_PLAYER]);
        else if (IsWirelessTrade())
            SetMainCallback2(CB2_SaveAndEndWirelessTrade);
        else
            SetMainCallback2(CB2_SaveAndEndTrade);
        gSelectedTradeMonPositions[TRADE_PLAYER] = 255;
        break;
    }
    if (!HasLinkErrorOccurred())
        RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void HandleLinkDataReceive(void)
{
    u8 recvStatus;
    TradeGetMultiplayerId(); // no effect call, ret val ignored
    recvStatus = GetBlockReceivedStatus();
    if (recvStatus & (1 << 0))
    {
        if (gBlockRecvBuffer[0][0] == LINKCMD_CONFIRM_FINISH_TRADE)
            SetMainCallback2(CB2_TryLinkTradeEvolution);

        if (gBlockRecvBuffer[0][0] == LINKCMD_READY_FINISH_TRADE)
            sTradeAnim->playerFinishStatus = STATUS_READY;

        ResetBlockReceivedFlag(0);
    }
    if (recvStatus & (1 << 1))
    {
        if (gBlockRecvBuffer[1][0] == LINKCMD_READY_FINISH_TRADE)
            sTradeAnim->partnerFinishStatus = STATUS_READY;

        ResetBlockReceivedFlag(1);
    }
}

static void SpriteCB_BouncingPokeball(struct Sprite *sprite)
{
    sprite->y += sprite->data[0] / 10;
    sprite->data[5] += sprite->data[1];
    sprite->x = sprite->data[5] / 10;
    if (sprite->y > 0x4c)
    {
        sprite->y = 0x4c;
        sprite->data[0] = -(sprite->data[0] * sprite->data[2]) / 100;
        sprite->data[3] ++;
    }
    if (sprite->x == 0x78)
        sprite->data[1] = 0;
    sprite->data[0] += sprite->data[4];
    if (sprite->data[3] == 4)
    {
        sprite->data[7] = 1;
        sprite->callback = SpriteCallbackDummy;
    }
}

static void SpriteCB_BouncingPokeballDepart(struct Sprite *sprite)
{
    sprite->y2 += sTradeBallVerticalVelocityTable[sprite->data[0]];
    if (sprite->data[0] == 22)
        PlaySE(SE_BALL_BOUNCE_1);
    if (++ sprite->data[0] == 44)
    {
        PlaySE(SE_M_MEGA_KICK);
        sprite->callback = SpriteCB_BouncingPokeballDepartEnd;
        sprite->data[0] = 0;
        BeginNormalPaletteFade(1 << (16 + sprite->oam.paletteNum), -1, 0, 16, RGB_WHITEALPHA);
    }
}

static void SpriteCB_BouncingPokeballDepartEnd(struct Sprite *sprite)
{
    if (sprite->data[1] == 20)
        StartSpriteAffineAnim(sprite, 1);
    if (++ sprite->data[1] > 20)
    {
        sprite->y2 -= sTradeBallVerticalVelocityTable[sprite->data[0]];
        if (++ sprite->data[0] == 23)
        {
            DestroySprite(sprite);
            sTradeAnim->state = STATE_FADE_OUT_TO_GBA_SEND; // Resume the master trade animation
        }
    }
}

static void SpriteCB_BouncingPokeballArrive(struct Sprite *sprite)
{
    if (sprite->data[2] == 0)
    {
        if ((sprite->y += 4) > sprite->data[3])
        {
            sprite->data[2] ++;
            sprite->data[0] = 0x16;
            PlaySE(SE_BALL_BOUNCE_1);
        }
    }
    else
    {
        if (sprite->data[0] == 0x42)
            PlaySE(SE_BALL_BOUNCE_2);
        if (sprite->data[0] == 0x5c)
            PlaySE(SE_BALL_BOUNCE_3);
        if (sprite->data[0] == 0x6b)
            PlaySE(SE_BALL_BOUNCE_4);
        sprite->y2 += sTradeBallVerticalVelocityTable[sprite->data[0]];
        if (++sprite->data[0] == 0x6c)
            sprite->callback = SpriteCallbackDummy;
    }
}

u16 GetInGameTradeSpeciesInfo(void)
{
    const struct InGameTrade *inGameTrade = &sIngameTrades[gSpecialVar_0x8004];
    StringCopy(gStringVar1, gSpeciesNames[inGameTrade->requestedSpecies]);
    StringCopy(gStringVar2, gSpeciesNames[inGameTrade->species]);
    return inGameTrade->requestedSpecies;
}

static void BufferInGameTradeMonName(void)
{
    u8 nickname[max(32, POKEMON_NAME_BUFFER_SIZE)];
    const struct InGameTrade *inGameTrade = &sIngameTrades[gSpecialVar_0x8004];
    GetMonData(&gPlayerParty[gSpecialVar_0x8005], MON_DATA_NICKNAME, nickname);
    StringCopy_Nickname(gStringVar1, nickname);
    StringCopy(gStringVar2, gSpeciesNames[inGameTrade->species]);
}

static void CreateInGameTradePokemonInternal(u8 whichPlayerMon, u8 whichInGameTrade)
{
    const struct InGameTrade *inGameTrade = &sIngameTrades[whichInGameTrade];
    u8 level = GetMonData(&gPlayerParty[whichPlayerMon], MON_DATA_LEVEL);

    struct Mail mail;
    u8 metLocation = METLOC_IN_GAME_TRADE;
    u8 mailNum;
    struct Pokemon *pokemon = &gEnemyParty[0];

    CreateMon(pokemon, inGameTrade->species, level, USE_RANDOM_IVS, TRUE, inGameTrade->personality, OT_ID_PRESET, inGameTrade->otId);

    SetMonData(pokemon, MON_DATA_HP_IV, &inGameTrade->ivs[0]);
    SetMonData(pokemon, MON_DATA_ATK_IV, &inGameTrade->ivs[1]);
    SetMonData(pokemon, MON_DATA_DEF_IV, &inGameTrade->ivs[2]);
    SetMonData(pokemon, MON_DATA_SPEED_IV, &inGameTrade->ivs[3]);
    SetMonData(pokemon, MON_DATA_SPATK_IV, &inGameTrade->ivs[4]);
    SetMonData(pokemon, MON_DATA_SPDEF_IV, &inGameTrade->ivs[5]);
    SetMonData(pokemon, MON_DATA_NICKNAME, inGameTrade->nickname);
    SetMonData(pokemon, MON_DATA_OT_NAME, inGameTrade->otName);
    SetMonData(pokemon, MON_DATA_OT_GENDER, &inGameTrade->otGender);
    SetMonData(pokemon, MON_DATA_ABILITY_NUM, &inGameTrade->abilityNum);
    SetMonData(pokemon, MON_DATA_BEAUTY, &inGameTrade->conditions[1]);
    SetMonData(pokemon, MON_DATA_CUTE, &inGameTrade->conditions[2]);
    SetMonData(pokemon, MON_DATA_COOL, &inGameTrade->conditions[0]);
    SetMonData(pokemon, MON_DATA_SMART, &inGameTrade->conditions[3]);
    SetMonData(pokemon, MON_DATA_TOUGH, &inGameTrade->conditions[4]);
    SetMonData(pokemon, MON_DATA_SHEEN, &inGameTrade->sheen);
    SetMonData(pokemon, MON_DATA_MET_LOCATION, &metLocation);

    mailNum = 0;
    if (inGameTrade->heldItem != ITEM_NONE)
    {
        if (ItemIsMail(inGameTrade->heldItem))
        {
            GetInGameTradeMail(&mail, inGameTrade);
            gTradeMail[0] = mail;
            SetMonData(pokemon, MON_DATA_MAIL, &mailNum);
            SetMonData(pokemon, MON_DATA_HELD_ITEM, &inGameTrade->heldItem);
        }
        else
        {
            SetMonData(pokemon, MON_DATA_HELD_ITEM, &inGameTrade->heldItem);
        }
    }
    CalculateMonStats(&gEnemyParty[0]);
}

static void GetInGameTradeMail(struct Mail *mail, const struct InGameTrade *trade)
{
    s32 i;

    for (i = 0; i < MAIL_WORDS_COUNT; i++)
        mail->words[i] = sIngameTradeMail[trade->mailNum][i];

    StringCopy(mail->playerName, trade->otName);
    PadNameString(mail->playerName, CHAR_SPACE);

    mail->trainerId[0] = trade->otId >> 24;
    mail->trainerId[1] = trade->otId >> 16;
    mail->trainerId[2] = trade->otId >> 8;
    mail->trainerId[3] = trade->otId;
    mail->species = trade->species;
    mail->itemId = trade->heldItem;
}

u16 GetTradeSpecies(void)
{
    if (GetMonData(&gPlayerParty[gSpecialVar_0x8005], MON_DATA_IS_EGG))
        return SPECIES_NONE;
    return GetMonData(&gPlayerParty[gSpecialVar_0x8005], MON_DATA_SPECIES);
}

void CreateInGameTradePokemon(void)
{
    CreateInGameTradePokemonInternal(gSpecialVar_0x8005, gSpecialVar_0x8004);
}

static void CB2_UpdateLinkTrade(void)
{
    if (DoTradeAnim() == TRUE)
    {
        DestroySprite(&gSprites[sTradeAnim->monSpriteIds[TRADE_PLAYER]]);
        FreeSpriteOamMatrix(&gSprites[sTradeAnim->monSpriteIds[TRADE_PARTNER]]);
        TradeMons(gSelectedTradeMonPositions[TRADE_PLAYER], gSelectedTradeMonPositions[TRADE_PARTNER] % PARTY_SIZE);
        if (!IsWirelessTrade())
        {
            sTradeAnim->linkData[0] = LINKCMD_READY_FINISH_TRADE;
            sTradeAnim->scheduleLinkTransfer = 1;
        }
        SetMainCallback2(CB2_WaitTradeComplete);
    }
    HandleLinkDataSend();
    HandleLinkDataReceive();
    RunTasks();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void CB2_WaitTradeComplete(void)
{
    u8 mpId = TradeGetMultiplayerId();
    if (IsWirelessTrade())
    {
        SetMainCallback2(CB2_TryLinkTradeEvolution);
    }
    else
    {
        HandleLinkDataReceive();
        if (mpId == 0
            && sTradeAnim->playerFinishStatus == STATUS_READY
            && sTradeAnim->partnerFinishStatus == STATUS_READY)
        {
            sTradeAnim->linkData[0] = LINKCMD_CONFIRM_FINISH_TRADE;
            SendBlock(BitmaskAllOtherLinkPlayers(), sTradeAnim->linkData, sizeof(sTradeAnim->linkData));
            sTradeAnim->playerFinishStatus = STATUS_CANCEL;
            sTradeAnim->partnerFinishStatus = STATUS_CANCEL;
        }
    }
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void CB2_SaveAndEndTrade(void)
{
    switch (gMain.state)
    {
    case 0:
        gMain.state++;
        StringExpandPlaceholders(gStringVarBuffer, gText_CommunicationStandby5);
        DrawTextOnTradeWindow(0, gStringVarBuffer, 0);
        break;
    case 1:
        SetTradeLinkStandbyCallback(0);
        gMain.state = 100;
        sTradeAnim->timer = 0;
        break;
    case 100:
        if (++sTradeAnim->timer > 180)
        {
            gMain.state = 101;
            sTradeAnim->timer = 0;
        }
        if (_IsLinkTaskFinished())
            gMain.state = 2;
        break;
    case 101:
        if (_IsLinkTaskFinished())
            gMain.state = 2;
        break;
    case 2:
        gMain.state = 50;
        StringExpandPlaceholders(gStringVarBuffer, gText_SavingDontTurnOffPower);
        DrawTextOnTradeWindow(0, gStringVarBuffer, 0);
        break;
    case 50:
        if (!InUnionRoom())
            IncrementGameStat(GAME_STAT_POKEMON_TRADES);
        if (gWirelessCommType)
            MysteryGift_TryIncrementStat(CARD_STAT_NUM_TRADES, gLinkPlayers[GetMultiplayerId() ^ 1].trainerId);

        SetContinueGameWarpStatusToDynamicWarp();
        LinkFullSave_Init();
        gMain.state++;
        sTradeAnim->timer = 0;
        break;
    case 51:
        if (++sTradeAnim->timer == 5)
            gMain.state++;
        break;
    case 52:
        if (LinkFullSave_WriteSector())
        {
            ClearContinueGameWarpStatus2();
            gMain.state = 4;
        }
        else
        {
            // Save isn't finished, delay again
            sTradeAnim->timer = 0;
            gMain.state = 51;
        }
        break;
    case 4:
        LinkFullSave_ReplaceLastSector();
        gMain.state = 40;
        sTradeAnim->timer = 0;
        break;
    case 40:
        if (++sTradeAnim->timer > 50)
        {
            if (GetMultiplayerId() == 0)
                sTradeAnim->timer = Random() % 30;
            else
                sTradeAnim->timer = 0;
            gMain.state = 41;
        }
        break;
    case 41:
        if (sTradeAnim->timer == 0)
        {
            SetTradeLinkStandbyCallback(1);
            gMain.state = 42;
        }
        else
        {
            sTradeAnim->timer--;
        }
        break;
    case 42:
        if (_IsLinkTaskFinished())
        {
            LinkFullSave_SetLastSectorSignature();
            gMain.state = 5;
        }
        break;
    case 5:
        if (++sTradeAnim->timer > 60)
        {
            gMain.state++;
            SetTradeLinkStandbyCallback(2);
        }
        break;
    case 6:
        if (_IsLinkTaskFinished())
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            gMain.state ++;
        }
        break;
    case 7:
        if (!gPaletteFade.active)
        {
            FadeOutBGM(3);
            gMain.state++;
        }
        break;
    case 8:
        if (IsBGMStopped() == TRUE)
        {
            if (gWirelessCommType && gMain.savedCallback == CB2_StartCreateTradeMenu)
                SetTradeLinkStandbyCallback(3);
            else
                SetCloseLinkCallback();
            gMain.state++;
        }
        break;
    case 9:
        if (gWirelessCommType && gMain.savedCallback == CB2_StartCreateTradeMenu)
        {
            if (_IsLinkTaskFinished())
            {
                gSoftResetDisabled = FALSE;
                SetMainCallback2(CB2_FreeTradeAnim);
            }
        }
        else if (!gReceivedRemoteLinkPlayers)
        {
            gSoftResetDisabled = FALSE;
            SetMainCallback2(CB2_FreeTradeAnim);
        }
        break;
    }
    if (!HasLinkErrorOccurred())
    {
        RunTasks();
    }
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void CB2_FreeTradeAnim(void)
{
    if (!gPaletteFade.active)
    {
        FreeAllWindowBuffers();
        Free(GetBgTilemapBuffer(3));
        Free(GetBgTilemapBuffer(1));
        Free(GetBgTilemapBuffer(0));
        FreeMonSpritesGfx();
        FREE_AND_SET_NULL(sTradeAnim);
        if (gWirelessCommType)
            DestroyWirelessStatusIndicatorSprite();
        SetMainCallback2(gMain.savedCallback);
    }
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

void DoInGameTradeScene(void)
{
    LockPlayerFieldControls();
    CreateTask(Task_InGameTrade, 10);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
}

static void Task_InGameTrade(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetMainCallback2(CB2_InitInGameTrade);
        gFieldCallback = FieldCB_ContinueScriptHandleMusic;
        DestroyTask(taskId);
    }
}

static void CheckPartnersMonForRibbons(void)
{
    u8 i;
    u8 numRibbons = 0;
    for (i = 0; i < (MON_DATA_UNUSED_RIBBONS - MON_DATA_CHAMPION_RIBBON); i++)
        numRibbons += GetMonData(&gEnemyParty[gSelectedTradeMonPositions[TRADE_PARTNER] % PARTY_SIZE], MON_DATA_CHAMPION_RIBBON + i);

    if (numRibbons != 0)
        FlagSet(FLAG_SYS_RIBBON_GET);
}

void LoadTradeAnimGfx(void)
{
    TradeAnimInit_LoadGfx();
}

void DrawTextOnTradeWindow(u8 windowId, const u8 *str, u8 speed)
{
    FillWindowPixelBuffer(windowId, PIXEL_FILL(15));
    sTradeAnim->textColors[0] = TEXT_DYNAMIC_COLOR_6;
    sTradeAnim->textColors[1] = TEXT_COLOR_WHITE;
    sTradeAnim->textColors[2] = TEXT_COLOR_GREEN;
    AddTextPrinterParameterized4(windowId, FONT_NORMAL, 0, 2, 0, 0, sTradeAnim->textColors, speed, str);
    CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void Task_AnimateWirelessSignal(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    u16 paletteIdx = sWirelessSignalAnimParams[tIdx][0] * 16;

    if (!tSignalComingBack)
    {
        if (paletteIdx == 256)
            LoadPalette(sWirelessSignalNone_Pal, BG_PLTT_ID(3), PLTT_SIZE_4BPP);
        else
            LoadPalette(&sWirelessSignalSend_Pal[paletteIdx], BG_PLTT_ID(3), PLTT_SIZE_4BPP);
    }
    else
    {
        if (paletteIdx == 256)
            LoadPalette(sWirelessSignalNone_Pal, BG_PLTT_ID(3), PLTT_SIZE_4BPP);
        else
            LoadPalette(&sWirelessSignalRecv_Pal[paletteIdx], BG_PLTT_ID(3), PLTT_SIZE_4BPP);
    }

    if (sWirelessSignalAnimParams[tIdx][0] == 0 && tCounter == 0)
        PlaySE(SE_M_HEAL_BELL);

    if (tCounter == sWirelessSignalAnimParams[tIdx][1])
    {
        tIdx++;
        tCounter = 0;
        if (sWirelessSignalAnimParams[tIdx][1] == 0xFF)
        {
            DestroyTask(taskId);
        }
    }
    else
    {
        tCounter++;
    }
}

#undef tIdx
#undef tCounter
#undef tSignalComingBack

static void Task_OpenCenterWhiteColumn(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (data[0] == 0)
    {
        sTradeAnim->wirelessWinLeft = sTradeAnim->wirelessWinRight = DISPLAY_WIDTH / 2;
        sTradeAnim->wirelessWinTop = 0;
        sTradeAnim->wirelessWinBottom = DISPLAY_HEIGHT;
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_OBJ);
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0 |
                                    WININ_WIN0_BG1 |
                                    WININ_WIN0_OBJ);
    }

    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE2(sTradeAnim->wirelessWinLeft, sTradeAnim->wirelessWinRight));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE2(sTradeAnim->wirelessWinTop, sTradeAnim->wirelessWinBottom));

    data[0]++;
    sTradeAnim->wirelessWinLeft -= 5;
    sTradeAnim->wirelessWinRight += 5;

    if (sTradeAnim->wirelessWinLeft < 80)
        DestroyTask(taskId);
}

static void Task_CloseCenterWhiteColumn(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (data[0] == 0)
    {
        sTradeAnim->wirelessWinLeft = 80;
        sTradeAnim->wirelessWinRight = DISPLAY_WIDTH - 80;
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_OBJ);
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0 |
                                    WININ_WIN0_BG1 |
                                    WININ_WIN0_OBJ);
    }

    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE2(sTradeAnim->wirelessWinLeft, sTradeAnim->wirelessWinRight));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE2(sTradeAnim->wirelessWinTop, sTradeAnim->wirelessWinBottom));

    if (sTradeAnim->wirelessWinLeft != DISPLAY_WIDTH / 2)
    {
        data[0]++;
        sTradeAnim->wirelessWinLeft += 5;
        sTradeAnim->wirelessWinRight -= 5;

        if (sTradeAnim->wirelessWinLeft > DISPLAY_WIDTH / 2 - 5)
            BlendPalettes(0x8, 0, RGB_WHITEALPHA);
    }
    else
    {
        ClearGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON);
        DestroyTask(taskId);
    }
}

static void CB2_SaveAndEndWirelessTrade(void)
{
    switch (gMain.state)
    {
    case 0:
        gMain.state = 1;
        StringExpandPlaceholders(gStringVarBuffer, gText_CommunicationStandby5);
        DrawTextOnTradeWindow(0, gStringVarBuffer, 0);
        break;
    case 1:
        SetTradeLinkStandbyCallback(0);
        gMain.state = 2;
        sTradeAnim->timer = 0;
        break;
    case 2:
        if (_IsLinkTaskFinished())
        {
            gMain.state = 3;
            StringExpandPlaceholders(gStringVarBuffer, gText_SavingDontTurnOffPower);
            DrawTextOnTradeWindow(0, gStringVarBuffer, 0);
            IncrementGameStat(GAME_STAT_POKEMON_TRADES);
            LinkFullSave_Init();
            sTradeAnim->timer = 0;
        }
        break;
    case 3:
        if (++sTradeAnim->timer == 5)
            gMain.state = 4;
        break;
    case 4:
        if (LinkFullSave_WriteSector())
        {
            gMain.state = 5;
        }
        else
        {
            sTradeAnim->timer = 0;
            gMain.state = 3;
        }
        break;
    case 5:
        LinkFullSave_ReplaceLastSector();
        gMain.state = 6;
        sTradeAnim->timer = 0;
        break;
    case 6:
        if (++sTradeAnim->timer > 10)
        {
            if (GetMultiplayerId() == 0)
                sTradeAnim->timer = Random() % 30;
            else
                sTradeAnim->timer = 0;
            gMain.state = 7;
        }
        break;
    case 7:
        if (sTradeAnim->timer == 0)
        {
            SetTradeLinkStandbyCallback(1);
            gMain.state = 8;
        }
        else
        {
            sTradeAnim->timer--;
        }
        break;
    case 8:
        if (_IsLinkTaskFinished())
        {
            LinkFullSave_SetLastSectorSignature();
            gMain.state = 9;
        }
        break;
    case 9:
        if (++sTradeAnim->timer > 60)
        {
            gMain.state++;
            SetTradeLinkStandbyCallback(2);
        }
        break;
    case 10:
        if (_IsLinkTaskFinished())
        {
            FadeOutBGM(3);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            gMain.state = 11;
        }
        break;
    case 11:
        if (!gPaletteFade.active && IsBGMStopped() == TRUE)
        {
            SetTradeLinkStandbyCallback(3);
            gMain.state = 12;
        }
        break;
    case 12:
        if (_IsLinkTaskFinished())
        {
            gSoftResetDisabled = FALSE;
            SetMainCallback2(CB2_FreeTradeAnim);
        }
        break;
    }

    if (!HasLinkErrorOccurred())
        RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}
