#include "global.h"
#include "battle_records.h"
#include "bg.h"
#include "window.h"
#include "link.h"
#include "battle.h"
#include "overworld.h"
#include "text.h"
#include "text_window.h"
#include "strings.h"
#include "string_util.h"
#include "trainer_card.h"
#include "menu.h"
#include "menu_helpers.h"
#include "palette.h"
#include "main.h"
#include "scanline_effect.h"
#include "international_string_util.h"
#include "sound.h"
#include "constants/songs.h"
#include "malloc.h"
#include "gpu_regs.h"
#include "constants/game_stat.h"
#include "trainer_hill.h"
#include "constants/rgb.h"

static void Task_CloseTrainerHillRecordsOnButton(u8 taskId);
static void Task_BeginPaletteFade(u8 taskId);
static void Task_ExitTrainerHillRecords(u8 taskId);
static void RemoveTrainerHillRecordsWindow(u8 windowId);
static void CB2_ShowTrainerHillRecords(void);

EWRAM_DATA u8 gRecordsWindowId = 0;
EWRAM_DATA static u8 *sTilemapBuffer = NULL;

static const u32 sTrainerHillWindowTileset[] = INCBIN_U32("graphics/trainer_hill/records_window.4bpp");
static const u16 sTrainerHillWindowPalette[] = INCBIN_U16("graphics/trainer_hill/records_window.gbapal");
static const u32 sTrainerHillWindowTilemap[] = INCBIN_U32("graphics/trainer_hill/records_window.bin");

static const struct BgTemplate sTrainerHillRecordsBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 1,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    }
};

static const struct WindowTemplate sTrainerHillRecordsWindowTemplates[] =
{
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 1,
        .width = 26,
        .height = 18,
        .paletteNum = 15,
        .baseBlock = 20
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sLinkBattleRecordsWindow =
{
    .bg = 0,
    .tilemapLeft = 2,
    .tilemapTop = 1,
    .width = 26,
    .height = 17,
    .paletteNum = 15,
    .baseBlock = 1
};

static const u8 sText_DashesNoPlayer[] = _("-------");
static const u8 sText_DashesNoScore[] = _("----");

// code
static void ClearLinkBattleRecord(struct LinkBattleRecord *record)
{
    CpuFill16(0, record, sizeof(struct LinkBattleRecord));
    record->name[0] = EOS;
    record->trainerId = 0;
    record->wins = 0;
    record->losses = 0;
    record->draws = 0;
}

static void ClearLinkBattleRecords(struct LinkBattleRecord *records)
{
    s32 i;
    for (i = 0; i < LINK_B_RECORDS_COUNT; i++)
    {
        ClearLinkBattleRecord(records + i);
    }
    SetGameStat(GAME_STAT_LINK_BATTLE_WINS, 0);
    SetGameStat(GAME_STAT_LINK_BATTLE_LOSSES, 0);
    SetGameStat(GAME_STAT_LINK_BATTLE_DRAWS, 0);
}

static s32 GetLinkBattleRecordTotalBattles(struct LinkBattleRecord *record)
{
    return record->wins + record->losses + record->draws;
}

static s32 FindLinkBattleRecord(struct LinkBattleRecord *records, const u8 *name, u16 trainerId)
{
    s32 i;

    for (i = 0; i < LINK_B_RECORDS_COUNT; i++)
    {
        if (!StringCompareN(records[i].name, name, PLAYER_NAME_LENGTH) && records[i].trainerId == trainerId)
            return i;
    }

    return LINK_B_RECORDS_COUNT;
}

static void SortLinkBattleRecords(struct LinkBattleRecords *records)
{
    s32 i, j;

    for (i = LINK_B_RECORDS_COUNT - 1; i > 0; i--)
    {
        for (j = i - 1; j >= 0; j--)
        {
            s32 totalBattlesI = GetLinkBattleRecordTotalBattles(&records->entries[i]);
            s32 totalBattlesJ = GetLinkBattleRecordTotalBattles(&records->entries[j]);

            if (totalBattlesI > totalBattlesJ)
            {
                struct LinkBattleRecord temp1;
                u8 temp2;

                temp1 = records->entries[i];
                records->entries[i] = records->entries[j];
                records->entries[j] = temp1;

                temp2 = records->languages[i];
                records->languages[i] = records->languages[j];
                records->languages[j] = temp2;
            }
        }
    }
}

static void UpdateLinkBattleRecord(struct LinkBattleRecord *record, s32 battleOutcome)
{
    switch (battleOutcome)
    {
    case B_OUTCOME_WON:
        record->wins++;
        if (record->wins > 9999)
            record->wins = 9999;
        break;
    case B_OUTCOME_LOST:
        record->losses++;
        if (record->losses > 9999)
            record->losses = 9999;
        break;
    case B_OUTCOME_DREW:
        record->draws++;
        if (record->draws > 9999)
            record->draws = 9999;
        break;
    }
}

static void UpdateLinkBattleGameStats(s32 battleOutcome)
{
    u8 stat;

    switch (battleOutcome)
    {
    case B_OUTCOME_WON:
        stat = GAME_STAT_LINK_BATTLE_WINS;
        break;
    case B_OUTCOME_LOST:
        stat = GAME_STAT_LINK_BATTLE_LOSSES;
        break;
    case B_OUTCOME_DREW:
        stat = GAME_STAT_LINK_BATTLE_DRAWS;
        break;
    default:
        return;
    }

    if (GetGameStat(stat) < 9999)
        IncrementGameStat(stat);
}

static void UpdateLinkBattleRecords(struct LinkBattleRecords *records, const u8 *name, u16 trainerId, s32 battleOutcome, u8 battlerId)
{
    s32 index;

    UpdateLinkBattleGameStats(battleOutcome);
    SortLinkBattleRecords(records);
    index = FindLinkBattleRecord(records->entries, name, trainerId);
    if (index == LINK_B_RECORDS_COUNT)
    {
        index = LINK_B_RECORDS_COUNT - 1;
        ClearLinkBattleRecord(&records->entries[index]);
        StringCopyN(records->entries[index].name, name, PLAYER_NAME_LENGTH);
        records->entries[index].trainerId = trainerId;
        records->languages[index] = gLinkPlayers[battlerId].language;
    }
    UpdateLinkBattleRecord(&records->entries[index], battleOutcome);
    SortLinkBattleRecords(records);
}

void ClearPlayerLinkBattleRecords(void)
{
    ClearLinkBattleRecords(gSaveBlock1Ptr->linkBattleRecords.entries);
}

static void IncTrainerCardWins(s32 battlerId)
{
    u16 *wins = &gTrainerCards[battlerId].linkBattleWins;
    (*wins)++;
    if (*wins > 9999)
        *wins = 9999;
}

static void IncTrainerCardLosses(s32 battlerId)
{
    u16 *losses = &gTrainerCards[battlerId].linkBattleLosses;
    (*losses)++;
    if (*losses > 9999)
        *losses = 9999;
}

static void UpdateTrainerCardWinsLosses(s32 battlerId)
{
    switch (gBattleOutcome)
    {
    case B_OUTCOME_WON:
        IncTrainerCardWins(BATTLE_OPPOSITE(battlerId));
        IncTrainerCardLosses(battlerId);
        break;
    case B_OUTCOME_LOST:
        IncTrainerCardLosses(BATTLE_OPPOSITE(battlerId));
        IncTrainerCardWins(battlerId);
        break;
    }
}

void UpdatePlayerLinkBattleRecords(s32 battlerId)
{
    if (InUnionRoom() != TRUE)
    {
        UpdateTrainerCardWinsLosses(battlerId);
        UpdateLinkBattleRecords(
            &gSaveBlock1Ptr->linkBattleRecords,
            gTrainerCards[battlerId].playerName,
            gTrainerCards[battlerId].trainerId,
            gBattleOutcome,
            battlerId);
    }
}

static void PrintLinkBattleWinsLossesDraws(struct LinkBattleRecord *records)
{
    s32 x;

    ConvertIntToDecimalStringN(gStringVar1, GetGameStat(GAME_STAT_LINK_BATTLE_WINS), STR_CONV_MODE_LEFT_ALIGN, 4);
    ConvertIntToDecimalStringN(gStringVar2, GetGameStat(GAME_STAT_LINK_BATTLE_LOSSES), STR_CONV_MODE_LEFT_ALIGN, 4);
    ConvertIntToDecimalStringN(gStringVar3, GetGameStat(GAME_STAT_LINK_BATTLE_DRAWS), STR_CONV_MODE_LEFT_ALIGN, 4);
    StringExpandPlaceholders(gStringVarBuffer, gText_TotalRecordWLD);

    x = GetStringCenterAlignXOffset(FONT_NORMAL, gStringVarBuffer, 0xD0);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, x, 0x11, 0, NULL);
}

static void PrintLinkBattleRecord(struct LinkBattleRecord *record, u8 y, s32 language)
{
    if (record->wins == 0 && record->losses == 0 && record->draws == 0)
    {
        // empty slot
        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, sText_DashesNoPlayer,   8, (y * 8) + 1, 0, NULL);
        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, sText_DashesNoScore,  80, (y * 8) + 1, 0, NULL);
        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, sText_DashesNoScore, 128, (y * 8) + 1, 0, NULL);
        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, sText_DashesNoScore, 176, (y * 8) + 1, 0, NULL);
    }
    else
    {
        StringFillWithTerminator(gStringVar1, 8);
        StringCopyN(gStringVar1, record->name, 7);
        ConvertInternationalString(gStringVar1, language);

        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVar1, 8, (y * 8) + 1, 0, NULL);

        ConvertIntToDecimalStringN(gStringVar1, record->wins, STR_CONV_MODE_RIGHT_ALIGN, 4);
        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVar1,  80, (y * 8) + 1, 0, NULL);

        ConvertIntToDecimalStringN(gStringVar1, record->losses, STR_CONV_MODE_RIGHT_ALIGN, 4);
        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVar1, 128, (y * 8) + 1, 0, NULL);

        ConvertIntToDecimalStringN(gStringVar1, record->draws, STR_CONV_MODE_RIGHT_ALIGN, 4);
        AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVar1, 176, (y * 8) + 1, 0, NULL);
    }
}

void ShowLinkBattleRecords(void)
{
    s32 i, x;

    gRecordsWindowId = AddWindow(&sLinkBattleRecordsWindow);
    DrawStdWindowFrame(gRecordsWindowId, FALSE);
    FillWindowPixelBuffer(gRecordsWindowId, PIXEL_FILL(1));
    StringExpandPlaceholders(gStringVarBuffer, gText_PlayersBattleResults);

    x = GetStringCenterAlignXOffset(FONT_NORMAL, gStringVarBuffer, 208);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, x, 1, 0, NULL);
    PrintLinkBattleWinsLossesDraws(gSaveBlock1Ptr->linkBattleRecords.entries);

    StringExpandPlaceholders(gStringVarBuffer, gText_WinLoseDraw);
    AddTextPrinterParameterized(gRecordsWindowId, FONT_NORMAL, gStringVarBuffer, 0, 41, 0, NULL);

    for (i = 0; i < LINK_B_RECORDS_COUNT; i++)
    {
        PrintLinkBattleRecord(&gSaveBlock1Ptr->linkBattleRecords.entries[i], 7 + (i * 2), gSaveBlock1Ptr->linkBattleRecords.languages[i]);
    }

    PutWindowTilemap(gRecordsWindowId);
    CopyWindowToVram(gRecordsWindowId, COPYWIN_FULL);
}

void RemoveRecordsWindow(void)
{
    ClearStdWindowAndFrame(gRecordsWindowId, FALSE);
    RemoveWindow(gRecordsWindowId);
}

static void Task_TrainerHillWaitForPaletteFade(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_CloseTrainerHillRecordsOnButton;
}

static void Task_CloseTrainerHillRecordsOnButton(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    if (JOY_NEW(A_BUTTON) || JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        task->func = Task_BeginPaletteFade;
    }
}

static void Task_BeginPaletteFade(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
    gTasks[taskId].func = Task_ExitTrainerHillRecords;
}

static void Task_ExitTrainerHillRecords(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        Free(sTilemapBuffer);
        RemoveTrainerHillRecordsWindow(0);
        FreeAllWindowBuffers();
        DestroyTask(taskId);
    }
}

static void RemoveTrainerHillRecordsWindow(u8 windowId)
{
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    ClearWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_GFX);
    RemoveWindow(windowId);
}

static void ClearVramOamPlttRegs(void)
{
    DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
    DmaClear32(3, OAM, OAM_SIZE);
    DmaClear16(3, PLTT, PLTT_SIZE);

    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2CNT, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG3CNT, 0);
    SetGpuReg(REG_OFFSET_BG3HOFS, 0);
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
}

static void ClearTasksAndGraphicalStructs(void)
{
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
}

static void ResetBgCoordinates(void)
{
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
    ChangeBgX(2, 0, BG_COORD_SET);
    ChangeBgY(2, 0, BG_COORD_SET);
    ChangeBgX(3, 0, BG_COORD_SET);
    ChangeBgY(3, 0, BG_COORD_SET);
}

static void SetDispcntReg(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_BG0_ON | DISPCNT_BG3_ON | DISPCNT_OBJ_1D_MAP);
}

static void LoadTrainerHillRecordsWindowGfx(u8 bgId)
{
    LoadBgTiles(bgId, sTrainerHillWindowTileset, sizeof(sTrainerHillWindowTileset), 0);
    CopyToBgTilemapBufferRect(bgId, sTrainerHillWindowTilemap, 0, 0, 0x20, 0x20);
    LoadPalette(sTrainerHillWindowPalette, BG_PLTT_ID(0), sizeof(sTrainerHillWindowPalette));
}

static void VblankCB_TrainerHillRecords(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void MainCB2_TrainerHillRecords(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

void ShowTrainerHillRecords(void)
{
    SetVBlankCallback(NULL);
    SetMainCallback2(CB2_ShowTrainerHillRecords);
}

static void CB2_ShowTrainerHillRecords(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        ClearVramOamPlttRegs();
        gMain.state++;
        break;
    case 1:
        ClearTasksAndGraphicalStructs();
        gMain.state++;
        break;
    case 2:
        sTilemapBuffer = AllocZeroed(BG_SCREEN_SIZE);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sTrainerHillRecordsBgTemplates, ARRAY_COUNT(sTrainerHillRecordsBgTemplates));
        SetBgTilemapBuffer(3, sTilemapBuffer);
        ResetBgCoordinates();
        gMain.state++;
        break;
    case 3:
        LoadTrainerHillRecordsWindowGfx(3);
        LoadPalette(GetTextWindowPalette(0), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 4:
        if (IsDma3ManagerBusyWithBgCopy() != TRUE)
        {
            ShowBg(0);
            ShowBg(3);
            CopyBgTilemapBufferToVram(3);
            gMain.state++;
        }
        break;
    case 5:
        InitWindows(sTrainerHillRecordsWindowTemplates);
        DeactivateAllTextPrinters();
        gMain.state++;
        break;
    case 6:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        gMain.state++;
        break;
    case 7:
        SetDispcntReg();
        SetVBlankCallback(VblankCB_TrainerHillRecords);
        PrintOnTrainerHillRecordsWindow();
        CreateTask(Task_TrainerHillWaitForPaletteFade, 8);
        SetMainCallback2(MainCB2_TrainerHillRecords);
        gMain.state = 0;
        break;
    }
}
