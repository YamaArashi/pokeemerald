#include "global.h"
#include "constants/songs.h"
#include "bg.h"
#include "decoration.h"
#include "event_scripts.h"
#include "event_object_movement.h"
#include "field_screen_effect.h"
#include "field_weather.h"
#include "international_string_util.h"
#include "item.h"
#include "item_icon.h"
#include "item_menu.h"
#include "constants/items.h"
#include "list_menu.h"
#include "mail.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "player_pc.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "window.h"
#include "menu_specialized.h"

// Top level PC menu options
enum {
    MENU_ITEMSTORAGE,
    MENU_MAILBOX,
    MENU_DECORATION,
    MENU_TURNOFF
};

// Item storage menu options
enum {
    MENU_WITHDRAW,
    MENU_DEPOSIT,
    MENU_TOSS,
    MENU_EXIT
};

// Windows for the main menus (top level menu, and item storage menu)
enum {
    WIN_MAIN_MENU,
    WIN_MAIN_MENU_BEDROOM,
    WIN_ITEM_STORAGE_MENU,
};

// Windows for item storage (while viewing the PC's item inventory)
enum {
    ITEMPC_WIN_LIST,
    ITEMPC_WIN_MESSAGE,
    ITEMPC_WIN_ICON,
    ITEMPC_WIN_TITLE,
    ITEMPC_WIN_QUANTITY,
    ITEMPC_WIN_YESNO,
    ITEMPC_WIN_COUNT
};
 // When showing the main list, the first window to this window are drawn
#define ITEMPC_WIN_LIST_END ITEMPC_WIN_TITLE

// Message IDs for Item Storage
enum {
    MSG_SWITCH_WHICH_ITEM = 0xFFF7,
    MSG_OKAY_TO_THROW_AWAY,
    MSG_TOO_IMPORTANT,
    MSG_NO_MORE_ROOM,
    MSG_THREW_AWAY_ITEM,
    MSG_HOW_MANY_TO_TOSS,
    MSG_WITHDREW_ITEM,
    MSG_HOW_MANY_TO_WITHDRAW,
    MSG_GO_BACK_TO_PREV
};

#define TAG_ITEM_ICON    5110
#define TAG_SCROLL_ARROW 5112

// Item list ID for toSwapPos to indicate an item is not currently being swapped
#define NOT_SWAPPING 0xFF

#define SWAP_LINE_LENGTH 7

struct ItemStorageMenu
{
    struct ListMenuItem listItems[PC_ITEMS_COUNT + 1];
    u8 itemNames[PC_ITEMS_COUNT + 1][ITEM_NAME_LENGTH + 10];
    u8 windowIds[ITEMPC_WIN_COUNT];
    u8 toSwapPos;
    u8 spriteId;
    u8 swapLineSpriteIds[SWAP_LINE_LENGTH];
};

static void InitPlayerPCMenu(u8);
static void PlayerPCProcessMenuInput(u8);
static void InitItemStorageMenu(u8, u8);

static u8 GetMailboxMailCount(void);
static void Mailbox_CompactMailList(void);
static void Mailbox_DrawMailboxMenu(u8);
static void Mailbox_ProcessInput(u8);
static void Mailbox_PrintWhatToDoWithPlayerMailText(u8);
static void Mailbox_ReturnToPlayerPC(u8);
static void Mailbox_PrintMailOptions(u8);
static void Mailbox_MailOptionsProcessInput(u8);

static void PlayerPC_ItemStorage(u8);
static void PlayerPC_Mailbox(u8);
static void PlayerPC_Decoration(u8);
static void PlayerPC_TurnOff(u8);

static void Mailbox_DoMailMoveToBag(u8);
static void Mailbox_DoMailRead(u8);
static void Mailbox_MoveToBag(u8);
static void Mailbox_Give(u8);
static void Mailbox_Cancel(u8);

static void Mailbox_CancelMoveToBag(u8);
static void Mailbox_HandleConfirmMoveToBag(u8);
static void Mailbox_AskConfirmMoveToBag(u8);
static void Mailbox_DoGiveMailPokeMenu(u8);
static void Mailbox_NoPokemonForMail(u8);

static void Mailbox_FadeAndReadMail(u8);
static void Mailbox_ReturnToFieldFromReadMail(void);
static void Mailbox_ReshowAfterMail(void);
static void Mailbox_HandleReturnToProcessInput(u8);
static void Mailbox_UpdateMailListAfterDeposit(void);

static void ItemStorage_Withdraw(u8);
static void ItemStorage_Deposit(u8);
static void ItemStorage_Toss(u8);
static void ItemStorage_Exit(u8);
static void ItemStorage_TossItemYes(u8);
static void ItemStorage_TossItemNo(u8);

static void ItemStorageMenuPrint(const u8 *);
static void ItemStorageMenuProcessInput(u8);
static void SetPlayerPCListCount(u8);
static void ItemStorage_HandleReturnToProcessInput(u8);

static void ItemStorage_Enter(u8, bool8);
static void ItemStorage_CreateListMenu(u8);
static void ItemStorage_ProcessInput(u8);
static void Task_ItemStorage_Deposit(u8);
static void ItemStorage_ReshowAfterBagMenu(void);
static void ItemStorage_DoItemWithdraw(u8);
static void ItemStorage_DoItemToss(u8);
static void ItemStorage_HandleQuantityRolling(u8);
static void ItemStorage_ExitItemList(u8);
static void ItemStorage_StartItemSwap(u8);
static void ItemStorage_DoItemAction(u8);
static void ItemStorage_FinishItemSwap(u8, bool8);
static void ItemStorage_HandleRemoveItem(u8);
static void ItemStorage_HandleErrorMessageInput(u8);
static void ItemStorage_ReturnToListInput(u8);

static const u8 *ItemStorage_GetMessage(u16);
static void CopyItemName_PlayerPC(u8 *, u16);

static void ItemStorage_Init(void);
static void ItemStorage_DrawSwapArrow(u8, u8, u8);
static void ItemStorage_RemoveWindow(u8);
static void ItemStorage_UpdateSwapLinePos(u8);
static void ItemStorage_ProcessItemSwapInput(u8);
static void ItemStorage_EraseItemIcon(void);
static void ItemStorage_DrawItemIcon(u16);
static void ItemStorage_PrintDescription(s32);
static void ItemStorage_EraseMainMenu(u8);
static void ItemStorage_MoveCursor(s32, bool8, struct ListMenu *);
static void ItemStorage_PrintMenuItem(u8, u32, u8);

static EWRAM_DATA const u8 *sTopMenuOptionOrder = NULL;
static EWRAM_DATA u8 sTopMenuNumOptions = 0;
EWRAM_DATA struct PlayerPCItemPageStruct gPlayerPCItemPageInfo = {};
static EWRAM_DATA struct ItemStorageMenu *sItemStorageMenu = NULL;

static const u8 *const sItemStorage_OptionDescriptions[] =
{
    [MENU_WITHDRAW] = gText_TakeOutItemsFromPC,
    [MENU_DEPOSIT]  = gText_StoreItemsInPC,
    [MENU_TOSS]     = gText_ThrowAwayItemsInPC,
    [MENU_EXIT]     = gText_GoBackPrevMenu,
};

static const struct MenuAction sPlayerPCMenuActions[] =
{
    [MENU_ITEMSTORAGE] = { gText_ItemStorage, {PlayerPC_ItemStorage} },
    [MENU_MAILBOX]     = { gText_Mailbox,     {PlayerPC_Mailbox} },
    [MENU_DECORATION]  = { gText_Decoration,  {PlayerPC_Decoration} },
    [MENU_TURNOFF]     = { gText_TurnOff,     {PlayerPC_TurnOff} }
};

static const u8 sBedroomPC_OptionOrder[] =
{
    MENU_ITEMSTORAGE,
    MENU_MAILBOX,
    MENU_DECORATION,
    MENU_TURNOFF
};
#define NUM_BEDROOM_PC_OPTIONS ARRAY_COUNT(sBedroomPC_OptionOrder)

static const u8 sPlayerPC_OptionOrder[] =
{
    MENU_ITEMSTORAGE,
    MENU_MAILBOX,
    MENU_TURNOFF
};
#define NUM_PLAYER_PC_OPTIONS ARRAY_COUNT(sPlayerPC_OptionOrder)

static const struct MenuAction sItemStorage_MenuActions[] =
{
    [MENU_WITHDRAW] = { gText_WithdrawItem, {ItemStorage_Withdraw} },
    [MENU_DEPOSIT]  = { gText_DepositItem,  {ItemStorage_Deposit} },
    [MENU_TOSS]     = { gText_TossItem,     {ItemStorage_Toss} },
    [MENU_EXIT]     = { gText_Cancel,       {ItemStorage_Exit} }
};

static const u16 sNewGamePCItems[][2] =
{
    { ITEM_POTION, 1 },
    { ITEM_NONE, 0 }
};

const struct MenuAction gMailboxMailOptions[] =
{
    { gText_Read,      {Mailbox_DoMailRead} },
    { gText_MoveToBag, {Mailbox_MoveToBag} },
    { gText_Give2,     {Mailbox_Give} },
    { gText_Cancel2,   {Mailbox_Cancel} }
};

static const struct WindowTemplate sWindowTemplates_MainMenus[] =
{
    [WIN_MAIN_MENU] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 1,
        .width = 9,
        .height = 6,
        .paletteNum = 15,
        .baseBlock = 1
    },
    [WIN_MAIN_MENU_BEDROOM] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 1,
        .width = 9,
        .height = 8,
        .paletteNum = 15,
        .baseBlock = 1
    },
    [WIN_ITEM_STORAGE_MENU] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 1,
        .width = 10,
        .height = 8,
        .paletteNum = 15,
        .baseBlock = 1
    }
};

static const struct YesNoFuncTable ItemTossYesNoFuncs =
{
    ItemStorage_TossItemYes,
    ItemStorage_TossItemNo
};

static const struct ListMenuTemplate sListMenuTemplate_ItemStorage =
{
    .items = NULL,
    .moveCursorFunc = ItemStorage_MoveCursor,
    .itemPrintFunc = ItemStorage_PrintMenuItem,
    .totalItems = 0,
    .maxShowed = 0,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 9,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = FALSE,
    .itemVerticalPadding = 0,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = FONT_NARROW,
    .cursorKind = CURSOR_BLACK_ARROW,
};

static const struct WindowTemplate sWindowTemplates_ItemStorage[ITEMPC_WIN_COUNT] =
{
    [ITEMPC_WIN_LIST] = {
        .bg = 0,
        .tilemapLeft = 16,
        .tilemapTop = 1,
        .width = 13,
        .height = 18,
        .paletteNum = 15,
        .baseBlock = 0x0001
    },
    [ITEMPC_WIN_MESSAGE] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 13,
        .width = 13,
        .height = 6,
        .paletteNum = 15,
        .baseBlock = 0x00EB
    },
    [ITEMPC_WIN_ICON] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 8,
        .width = 3,
        .height = 3,
        .paletteNum = 15,
        .baseBlock = 0x0153
    },
    [ITEMPC_WIN_TITLE] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 1,
        .width = 13,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x0139
    },
    [ITEMPC_WIN_QUANTITY] = {
        .bg = 0,
        .tilemapLeft = 8,
        .tilemapTop = 9,
        .width = 6,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x015C
    },
    [ITEMPC_WIN_YESNO] = {
        .bg = 0,
        .tilemapLeft = 9,
        .tilemapTop = 7,
        .width = 5,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x0168
    }
};

static const u8 sSwapArrowTextColors[] = {TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY, TEXT_COLOR_DARK_GRAY};

void NewGameInitPCItems(void)
{
    u8 i = 0;
    ClearItemSlots(gSaveBlock1Ptr->pcItems, PC_ITEMS_COUNT);

    while (TRUE)
    {
        if (sNewGamePCItems[i][0] == ITEM_NONE || sNewGamePCItems[i][1] == 0)
            break;
        if (AddPCItem(sNewGamePCItems[i][0], sNewGamePCItems[i][1]) != TRUE)
            break;
        i++;
    }
}

void BedroomPC(void)
{
    sTopMenuOptionOrder = sBedroomPC_OptionOrder;
    sTopMenuNumOptions = NUM_BEDROOM_PC_OPTIONS;
    DisplayItemMessageOnField(CreateTask(TaskDummy, 0), gText_WhatWouldYouLike, InitPlayerPCMenu);
}

void PlayerPC(void)
{
    sTopMenuOptionOrder = sPlayerPC_OptionOrder;
    sTopMenuNumOptions = NUM_PLAYER_PC_OPTIONS;
    DisplayItemMessageOnField(CreateTask(TaskDummy, 0), gText_WhatWouldYouLike, InitPlayerPCMenu);
}

#define tUsedSlots  data[1]
#define tQuantity   data[2]
#define tInTossMenu data[3]
#define tWindowId   data[4]
#define tListTaskId data[5]

static void InitPlayerPCMenu(u8 taskId)
{
    u16 *data;
    struct WindowTemplate windowTemplate;
    data = gTasks[taskId].data;

    if (sTopMenuNumOptions == NUM_PLAYER_PC_OPTIONS)
        windowTemplate = sWindowTemplates_MainMenus[WIN_MAIN_MENU];
    else // Bedroom PC
        windowTemplate = sWindowTemplates_MainMenus[WIN_MAIN_MENU_BEDROOM];

    windowTemplate.width = GetMaxWidthInSubsetOfMenuTable(sPlayerPCMenuActions, sTopMenuOptionOrder, sTopMenuNumOptions);
    tWindowId = AddWindow(&windowTemplate);
    SetStandardWindowBorderStyle(tWindowId, FALSE);
    PrintMenuActionTextsInUpperLeftCorner(tWindowId, sTopMenuNumOptions, sPlayerPCMenuActions, sTopMenuOptionOrder);
    InitMenuInUpperLeftCornerNormal(tWindowId, sTopMenuNumOptions, 0);
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = PlayerPCProcessMenuInput;
}

static void PlayerPCProcessMenuInput(u8 taskId)
{
    u16 *data;
    s8 inputOptionId;

    data = gTasks[taskId].data;
    if (sTopMenuNumOptions > 3)
        inputOptionId = Menu_ProcessInput();
    else
        inputOptionId = Menu_ProcessInputNoWrap();

    switch (inputOptionId)
    {
    case MENU_NOTHING_CHOSEN:
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        ClearStdWindowAndFrameToTransparent(tWindowId, FALSE);
        ClearWindowTilemap(tWindowId);
        RemoveWindow(tWindowId);
        ScheduleBgCopyTilemapToVram(0);
        gTasks[taskId].func = PlayerPC_TurnOff;
        break;
    default:
        ClearStdWindowAndFrameToTransparent(tWindowId, FALSE);
        ClearWindowTilemap(tWindowId);
        RemoveWindow(tWindowId);
        ScheduleBgCopyTilemapToVram(0);
        gTasks[taskId].func = sPlayerPCMenuActions[sTopMenuOptionOrder[inputOptionId]].func.void_u8;
        break;
    }
}

void ReshowPlayerPC(u8 var)
{
    DisplayItemMessageOnField(var, gText_WhatWouldYouLike, InitPlayerPCMenu);
}

static void PlayerPC_ItemStorage(u8 taskId)
{
    InitItemStorageMenu(taskId, MENU_WITHDRAW);
    gTasks[taskId].func = ItemStorageMenuProcessInput;
}

static void PlayerPC_Mailbox(u8 taskId)
{
    gPlayerPCItemPageInfo.count = GetMailboxMailCount();

    if (gPlayerPCItemPageInfo.count == 0)
    {
        // Mailbox cannot be opened if no mail is in PC
        DisplayItemMessageOnField(taskId, gText_NoMailHere, ReshowPlayerPC);
    }
    else
    {
        gPlayerPCItemPageInfo.cursorPos = 0;
        gPlayerPCItemPageInfo.itemsAbove = 0;
        gPlayerPCItemPageInfo.scrollIndicatorTaskId = TASK_NONE;
        Mailbox_CompactMailList();
        SetPlayerPCListCount(taskId);
        if (MailboxMenu_Alloc(gPlayerPCItemPageInfo.count) == TRUE)
        {
            ClearDialogWindowAndFrame(0, FALSE);
            Mailbox_DrawMailboxMenu(taskId);
            gTasks[taskId].func = Mailbox_ProcessInput;
        }
        else
        {
            // Alloc failed, exit Mailbox
            DisplayItemMessageOnField(taskId, gText_NoMailHere, ReshowPlayerPC);
        }
    }
}

static void PlayerPC_Decoration(u8 taskId)
{
    DoPlayerRoomDecorationMenu(taskId);
}

static void PlayerPC_TurnOff(u8 taskId)
{
    if (sTopMenuNumOptions == NUM_BEDROOM_PC_OPTIONS) // Flimsy way to determine if Bedroom PC is in use
    {
        if (gSaveBlock2Ptr->playerGender == MALE)
            ScriptContext_SetupScript(LittlerootTown_BrendansHouse_2F_EventScript_TurnOffPlayerPC);
        else
            ScriptContext_SetupScript(LittlerootTown_MaysHouse_2F_EventScript_TurnOffPlayerPC);
    }
    else
    {
        ScriptContext_Enable();
    }
    DestroyTask(taskId);
}

static void InitItemStorageMenu(u8 taskId, u8 var)
{
    u16 *data;
    struct WindowTemplate windowTemplate;

    data = gTasks[taskId].data;
    windowTemplate = sWindowTemplates_MainMenus[WIN_ITEM_STORAGE_MENU];
    windowTemplate.width = GetMaxWidthInMenuTable(sItemStorage_MenuActions, ARRAY_COUNT(sItemStorage_MenuActions));
    tWindowId = AddWindow(&windowTemplate);
    SetStandardWindowBorderStyle(tWindowId, FALSE);
    PrintMenuTable(tWindowId, ARRAY_COUNT(sItemStorage_MenuActions), sItemStorage_MenuActions);
    InitMenuInUpperLeftCornerNormal(tWindowId, ARRAY_COUNT(sItemStorage_MenuActions), var);
    ScheduleBgCopyTilemapToVram(0);
    ItemStorageMenuPrint(sItemStorage_OptionDescriptions[var]);
}

static void ItemStorageMenuPrint(const u8 *textPtr)
{
    DrawDialogueFrame(0, FALSE);
    AddTextPrinterParameterized(0, FONT_NORMAL, textPtr, 0, 1, 0, 0);
}

static void ItemStorageMenuProcessInput(u8 taskId)
{
    s8 oldPos, newPos;
    s8 inputOptionId;

    oldPos = Menu_GetCursorPos();
    inputOptionId = Menu_ProcessInput();
    newPos = Menu_GetCursorPos();
    switch (inputOptionId)
    {
    case MENU_NOTHING_CHOSEN:
        if (oldPos != newPos)
            ItemStorageMenuPrint(sItemStorage_OptionDescriptions[newPos]);
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        ItemStorage_Exit(taskId);
        break;
    default:
        PlaySE(SE_SELECT);
        sItemStorage_MenuActions[inputOptionId].func.void_u8(taskId);
        break;
    }
}

static void ItemStorage_Deposit(u8 taskId)
{
    gTasks[taskId].func = Task_ItemStorage_Deposit;
    FadeScreen(FADE_TO_BLACK, 0);
}

static void Task_ItemStorage_Deposit(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        CB2_GoToItemDepositMenu();
        DestroyTask(taskId);
    }
}

void CB2_PlayerPCExitBagMenu(void)
{
    gFieldCallback = ItemStorage_ReshowAfterBagMenu;
    SetMainCallback2(CB2_ReturnToField);
}

static void ItemStorage_ReshowAfterBagMenu(void)
{
    LoadMessageBoxAndBorderGfx();
    DrawDialogueFrame(0, TRUE);
    InitItemStorageMenu(CreateTask(ItemStorage_HandleReturnToProcessInput, 0), 1);
    FadeInFromBlack();
}

static void ItemStorage_HandleReturnToProcessInput(u8 taskId)
{
    if (IsWeatherNotFadingIn() == TRUE)
        gTasks[taskId].func = ItemStorageMenuProcessInput;
}

static void ItemStorage_Withdraw(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    tUsedSlots = CountUsedPCItemSlots();
    if (tUsedSlots != 0)
    {
        ItemStorage_Enter(taskId, FALSE);
    }
    else
    {
        // Can't withdraw, no items in PC
        ItemStorage_EraseMainMenu(taskId);
        DisplayItemMessageOnField(taskId, gText_NoItems, PlayerPC_ItemStorage);
    }

}

static void ItemStorage_Toss(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    tUsedSlots = CountUsedPCItemSlots();
    if (tUsedSlots != 0)
    {
        ItemStorage_Enter(taskId, TRUE);
    }
    else
    {
        // Can't toss, no items in PC
        ItemStorage_EraseMainMenu(taskId);
        DisplayItemMessageOnField(taskId, gText_NoItems, PlayerPC_ItemStorage);
    }
}

static void ItemStorage_Enter(u8 taskId, bool8 toss)
{
    u16 *data = gTasks[taskId].data;

    tInTossMenu = toss;
    ItemStorage_EraseMainMenu(taskId);
    gPlayerPCItemPageInfo.cursorPos = 0;
    gPlayerPCItemPageInfo.itemsAbove = 0;
    gPlayerPCItemPageInfo.scrollIndicatorTaskId = TASK_NONE;
    SetPlayerPCListCount(taskId);
    ItemStorage_Init();
    FreeAndReserveObjectSpritePalettes();
    LoadListMenuSwapLineGfx();
    CreateSwapLineSprites(sItemStorageMenu->swapLineSpriteIds, SWAP_LINE_LENGTH);
    ClearDialogWindowAndFrame(0, FALSE);
    gTasks[taskId].func = ItemStorage_CreateListMenu;
}

static void ItemStorage_Exit(u8 taskId)
{
    ItemStorage_EraseMainMenu(taskId);
    ReshowPlayerPC(taskId);
}

// Used by Item Storage and the Mailbox
static void SetPlayerPCListCount(u8 taskId)
{
    if (gPlayerPCItemPageInfo.count > 7)
        gPlayerPCItemPageInfo.pageItems = 8;
    else
        gPlayerPCItemPageInfo.pageItems = gPlayerPCItemPageInfo.count + 1;
}

static void ItemStorage_EraseMainMenu(u8 taskId)
{
    u16 *data = gTasks[taskId].data;
    ClearStdWindowAndFrameToTransparent(tWindowId, FALSE);
    ClearWindowTilemap(tWindowId);
    RemoveWindow(tWindowId);
    ScheduleBgCopyTilemapToVram(0);
}

static u8 GetMailboxMailCount(void)
{
    u8 mailInPC, i;

    // Count mail in PC (by first skipping over mail in party)
    for (mailInPC = 0, i = PARTY_SIZE; i < MAIL_COUNT; i++)
        if (gSaveBlock1Ptr->mail[i].itemId != ITEM_NONE)
            mailInPC++;

    return mailInPC;
}

static void Mailbox_CompactMailList(void)
{
    struct Mail temp;
    u8 i, j;

    for (i = PARTY_SIZE; i < MAIL_COUNT - 1; i++)
    {
        for (j = i + 1; j < MAIL_COUNT; j++)
        {
            if (gSaveBlock1Ptr->mail[i].itemId == ITEM_NONE)
                SWAP(gSaveBlock1Ptr->mail[i], gSaveBlock1Ptr->mail[j], temp);
        }
    }
}

static void Mailbox_DrawMailboxMenu(u8 taskId)
{
    u8 windowId = MailboxMenu_AddWindow(MAILBOXWIN_TITLE);
    MailboxMenu_AddWindow(MAILBOXWIN_LIST);
    AddTextPrinterParameterized(windowId, FONT_NORMAL, gText_Mailbox, GetStringCenterAlignXOffset(FONT_NORMAL, gText_Mailbox, 0x40), 1, 0, NULL);
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].tListTaskId = MailboxMenu_CreateList(&gPlayerPCItemPageInfo);
    MailboxMenu_AddScrollArrows(&gPlayerPCItemPageInfo);
}

static void Mailbox_ProcessInput(u8 taskId)
{
    u16 *data = gTasks[taskId].data;

    if (!gPaletteFade.active)
    {
        s32 inputOptionId = ListMenu_ProcessInput(tListTaskId);
        ListMenuGetScrollAndRow(tListTaskId, &gPlayerPCItemPageInfo.itemsAbove, &gPlayerPCItemPageInfo.cursorPos);

        switch (inputOptionId)
        {
        case LIST_NOTHING_CHOSEN:
            break;
        case LIST_CANCEL:
            PlaySE(SE_SELECT);
            RemoveScrollIndicatorArrowPair(gPlayerPCItemPageInfo.scrollIndicatorTaskId);
            Mailbox_ReturnToPlayerPC(taskId);
            break;
        default:
            // Selected mail, ask what to do with it
            PlaySE(SE_SELECT);
            MailboxMenu_RemoveWindow(MAILBOXWIN_TITLE);
            MailboxMenu_RemoveWindow(MAILBOXWIN_LIST);
            DestroyListMenuTask(tListTaskId, &gPlayerPCItemPageInfo.itemsAbove, &gPlayerPCItemPageInfo.cursorPos);
            ScheduleBgCopyTilemapToVram(0);
            RemoveScrollIndicatorArrowPair(gPlayerPCItemPageInfo.scrollIndicatorTaskId);
            gTasks[taskId].func = Mailbox_PrintWhatToDoWithPlayerMailText;
            break;
        }
    }
}

static void Mailbox_PrintWhatToDoWithPlayerMailText(u8 taskId)
{
    StringCopy(gStringVar1, gSaveBlock1Ptr->mail[gPlayerPCItemPageInfo.itemsAbove + PARTY_SIZE + gPlayerPCItemPageInfo.cursorPos].playerName);
    ConvertInternationalPlayerNameStripChar(gStringVar1, CHAR_SPACE);
    StringExpandPlaceholders(gStringVarBuffer, gText_WhatToDoWithVar1sMail);
    DisplayItemMessageOnField(taskId, gStringVarBuffer, Mailbox_PrintMailOptions);
}

static void Mailbox_ReturnToPlayerPC(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    MailboxMenu_RemoveWindow(MAILBOXWIN_TITLE);
    MailboxMenu_RemoveWindow(MAILBOXWIN_LIST);
    DestroyListMenuTask(tListTaskId, NULL, NULL);
    ScheduleBgCopyTilemapToVram(0);
    MailboxMenu_Free();
    ReshowPlayerPC(taskId);
}

static void Mailbox_PrintMailOptions(u8 taskId)
{
    u8 windowId = MailboxMenu_AddWindow(MAILBOXWIN_OPTIONS);
    PrintMenuTable(windowId, ARRAY_COUNT(gMailboxMailOptions), gMailboxMailOptions);
    InitMenuInUpperLeftCornerNormal(windowId, ARRAY_COUNT(gMailboxMailOptions), 0);
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = Mailbox_MailOptionsProcessInput;
}

static void Mailbox_MailOptionsProcessInput(u8 taskId)
{
    s8 inputOptionId = ProcessMenuInput_other();

    switch (inputOptionId)
    {
    case MENU_NOTHING_CHOSEN:
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        Mailbox_Cancel(taskId);
        break;
    default:
        PlaySE(SE_SELECT);
        gMailboxMailOptions[inputOptionId].func.void_u8(taskId);
        break;
    }
}

static void Mailbox_DoMailRead(u8 taskId)
{
    FadeScreen(FADE_TO_BLACK, 0);
    gTasks[taskId].func = Mailbox_FadeAndReadMail;
}

static void Mailbox_FadeAndReadMail(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        MailboxMenu_Free();
        CleanupOverworldWindowsAndTilemaps();
        ReadMail(&gSaveBlock1Ptr->mail[gPlayerPCItemPageInfo.itemsAbove + PARTY_SIZE + gPlayerPCItemPageInfo.cursorPos], Mailbox_ReturnToFieldFromReadMail, TRUE);
        DestroyTask(taskId);
    }
}

static void Mailbox_ReturnToFieldFromReadMail(void)
{
    gFieldCallback = Mailbox_ReshowAfterMail;
    SetMainCallback2(CB2_ReturnToField);
}

static void Mailbox_ReshowAfterMail(void)
{
    u8 taskId;

    LoadMessageBoxAndBorderGfx();
    taskId = CreateTask(Mailbox_HandleReturnToProcessInput, 0);
    if (MailboxMenu_Alloc(gPlayerPCItemPageInfo.count) == TRUE)
        Mailbox_DrawMailboxMenu(taskId);
    else
        DestroyTask(taskId);
    FadeInFromBlack();
}

static void Mailbox_HandleReturnToProcessInput(u8 taskId)
{
    if (IsWeatherNotFadingIn() == TRUE)
        gTasks[taskId].func = Mailbox_ProcessInput;
}

static void Mailbox_MoveToBag(u8 taskId)
{
    DisplayItemMessageOnField(taskId, gText_MessageWillBeLost, Mailbox_AskConfirmMoveToBag);
}

static void Mailbox_AskConfirmMoveToBag(u8 taskId)
{
    DisplayYesNoMenuDefaultYes();
    gTasks[taskId].func = Mailbox_HandleConfirmMoveToBag;
}

static void Mailbox_HandleConfirmMoveToBag(u8 taskId)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // Yes
        Mailbox_DoMailMoveToBag(taskId);
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
    case 1: // No
        Mailbox_CancelMoveToBag(taskId);
        break;
    case MENU_NOTHING_CHOSEN:
    default:
        break;
    }
}

static void Mailbox_DoMailMoveToBag(u8 taskId)
{
    struct Mail *mail = &gSaveBlock1Ptr->mail[gPlayerPCItemPageInfo.itemsAbove + PARTY_SIZE + gPlayerPCItemPageInfo.cursorPos];
    if (!AddBagItem(mail->itemId, 1))
    {
        DisplayItemMessageOnField(taskId, gText_BagIsFull, Mailbox_Cancel);
    }
    else
    {
        DisplayItemMessageOnField(taskId, gText_MailToBagMessageErased, Mailbox_Cancel);
        ClearMail(mail);
        Mailbox_CompactMailList();
        gPlayerPCItemPageInfo.count--;
        if (gPlayerPCItemPageInfo.count < (gPlayerPCItemPageInfo.pageItems + gPlayerPCItemPageInfo.itemsAbove) && gPlayerPCItemPageInfo.itemsAbove != 0)
            gPlayerPCItemPageInfo.itemsAbove--;
        SetPlayerPCListCount(taskId);
    }
}

static void Mailbox_CancelMoveToBag(u8 taskId)
{
    Mailbox_Cancel(taskId);
}

static void Mailbox_Give(u8 taskId)
{
    if (CalculatePlayerPartyCount() == 0)
    {
        Mailbox_NoPokemonForMail(taskId);
    }
    else
    {
        FadeScreen(FADE_TO_BLACK, 0);
        gTasks[taskId].func = Mailbox_DoGiveMailPokeMenu;
    }
}

static void Mailbox_DoGiveMailPokeMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        MailboxMenu_Free();
        CleanupOverworldWindowsAndTilemaps();
        ChooseMonToGiveMailFromMailbox();
        DestroyTask(taskId);
    }
}

void Mailbox_ReturnToMailListAfterDeposit(void)
{
    gFieldCallback = Mailbox_UpdateMailListAfterDeposit;
    SetMainCallback2(CB2_ReturnToField);
}

static void Mailbox_UpdateMailListAfterDeposit(void)
{
    u8 taskId;
    u8 prevCount;
    taskId = CreateTask(Mailbox_HandleReturnToProcessInput, 0);
    prevCount = gPlayerPCItemPageInfo.count;
    gPlayerPCItemPageInfo.count = GetMailboxMailCount();
    Mailbox_CompactMailList();
    if (prevCount != gPlayerPCItemPageInfo.count && (gPlayerPCItemPageInfo.count < (gPlayerPCItemPageInfo.pageItems + gPlayerPCItemPageInfo.itemsAbove))
       && gPlayerPCItemPageInfo.itemsAbove != 0)
        gPlayerPCItemPageInfo.itemsAbove--;
    SetPlayerPCListCount(taskId);
    LoadMessageBoxAndBorderGfx();
    if (MailboxMenu_Alloc(gPlayerPCItemPageInfo.count) == TRUE)
        Mailbox_DrawMailboxMenu(taskId);
    else
        DestroyTask(taskId);
    FadeInFromBlack();
}

static void Mailbox_NoPokemonForMail(u8 taskId)
{
    DisplayItemMessageOnField(taskId, gText_NoPokemon, Mailbox_Cancel);
}

static void Mailbox_Cancel(u8 taskId)
{
    MailboxMenu_RemoveWindow(MAILBOXWIN_OPTIONS);
    ClearDialogWindowAndFrame(0, FALSE);
    Mailbox_DrawMailboxMenu(taskId);
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = Mailbox_ProcessInput;
}

static void ItemStorage_Init(void)
{
    sItemStorageMenu = AllocZeroed(sizeof(*sItemStorageMenu));
    memset(sItemStorageMenu->windowIds, WINDOW_NONE, ITEMPC_WIN_COUNT);
    sItemStorageMenu->toSwapPos = NOT_SWAPPING;
    sItemStorageMenu->spriteId = SPRITE_NONE;
}

static void ItemStorage_Free(void)
{
    u32 i;
    for (i = 0; i < ITEMPC_WIN_COUNT; i++)
        ItemStorage_RemoveWindow(i);
    Free(sItemStorageMenu);
}

static u8 ItemStorage_AddWindow(u8 i)
{
    u8 *windowIdLoc = &sItemStorageMenu->windowIds[i];
    if (*windowIdLoc == WINDOW_NONE)
    {
        *windowIdLoc = AddWindow(&sWindowTemplates_ItemStorage[i]);
        DrawStdFrameWithCustomTileAndPalette(*windowIdLoc, FALSE, 0x214, 0xE);
        ScheduleBgCopyTilemapToVram(0);
    }
    return *windowIdLoc;
}

static void ItemStorage_RemoveWindow(u8 i)
{
    u8 *windowIdLoc = &sItemStorageMenu->windowIds[i];
    if (*windowIdLoc != WINDOW_NONE)
    {
        ClearStdWindowAndFrameToTransparent(*windowIdLoc, FALSE);
        ClearWindowTilemap(*windowIdLoc);
        ScheduleBgCopyTilemapToVram(0);
        RemoveWindow(*windowIdLoc);
        *windowIdLoc = WINDOW_NONE;
    }
}

void ItemStorage_RefreshListMenu(void)
{
    u16 i;

    // Copy item names for all entries but the last (which is Cancel)
    for(i = 0; i < gPlayerPCItemPageInfo.count - 1; i++)
    {
        CopyItemName_PlayerPC(&sItemStorageMenu->itemNames[i][0], gSaveBlock1Ptr->pcItems[i].itemId);
        sItemStorageMenu->listItems[i].name = &sItemStorageMenu->itemNames[i][0];
        sItemStorageMenu->listItems[i].id = i;
    }

    // Set up Cancel entry
    StringCopy(&sItemStorageMenu->itemNames[i][0], gText_Cancel2);
    sItemStorageMenu->listItems[i].name = &sItemStorageMenu->itemNames[i][0];
    sItemStorageMenu->listItems[i].id = LIST_CANCEL;

    // Set list menu data
    gMultiuseListMenuTemplate = sListMenuTemplate_ItemStorage;
    gMultiuseListMenuTemplate.windowId = ItemStorage_AddWindow(ITEMPC_WIN_LIST);
    gMultiuseListMenuTemplate.totalItems = gPlayerPCItemPageInfo.count;
    gMultiuseListMenuTemplate.items = sItemStorageMenu->listItems;
    gMultiuseListMenuTemplate.maxShowed = gPlayerPCItemPageInfo.pageItems;
}

void CopyItemName_PlayerPC(u8 *string, u16 itemId)
{
    CopyItemName(itemId, string);
}

static void ItemStorage_MoveCursor(s32 id, bool8 onInit, struct ListMenu *list)
{
    if (onInit != TRUE)
        PlaySE(SE_SELECT);
    if (sItemStorageMenu->toSwapPos == NOT_SWAPPING)
    {
        ItemStorage_EraseItemIcon();
        if (id != LIST_CANCEL)
            ItemStorage_DrawItemIcon(gSaveBlock1Ptr->pcItems[id].itemId);
        else
            ItemStorage_DrawItemIcon(ITEM_LIST_END);
        ItemStorage_PrintDescription(id);
    }
}

static void ItemStorage_PrintMenuItem(u8 windowId, u32 id, u8 yOffset)
{
    if (id != LIST_CANCEL)
    {
        if (sItemStorageMenu->toSwapPos != NOT_SWAPPING)
        {
            if (sItemStorageMenu->toSwapPos == (u8)id)
                ItemStorage_DrawSwapArrow(yOffset, 0, TEXT_SKIP_DRAW);
            else
                ItemStorage_DrawSwapArrow(yOffset, 0xFF, TEXT_SKIP_DRAW);
        }
        ConvertIntToDecimalStringN(gStringVar1, gSaveBlock1Ptr->pcItems[id].quantity, STR_CONV_MODE_RIGHT_ALIGN, 3);
        StringExpandPlaceholders(gStringVarBuffer, gText_xVar1);
        AddTextPrinterParameterized(windowId, FONT_NARROW, gStringVarBuffer, GetStringRightAlignXOffset(FONT_NARROW, gStringVarBuffer, 104), yOffset, TEXT_SKIP_DRAW, NULL);
    }
}

static void ItemStorage_PrintDescription(s32 id)
{
    const u8 *description;
    u8 windowId = sItemStorageMenu->windowIds[ITEMPC_WIN_MESSAGE];

    // Get item description (or Cancel text)
    if (id != LIST_CANCEL)
        description = (u8 *)ItemId_GetDescription(gSaveBlock1Ptr->pcItems[id].itemId);
    else
        description = ItemStorage_GetMessage(MSG_GO_BACK_TO_PREV);

    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    AddTextPrinterParameterized(windowId, FONT_NORMAL, description, 0, 1, 0, NULL);
}

static void ItemStorage_AddScrollIndicator(void)
{
    if (gPlayerPCItemPageInfo.scrollIndicatorTaskId == TASK_NONE)
        gPlayerPCItemPageInfo.scrollIndicatorTaskId = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_UP, 176, 12, 148,
                                                                                                gPlayerPCItemPageInfo.count - gPlayerPCItemPageInfo.pageItems,
                                                                                                TAG_SCROLL_ARROW,
                                                                                                TAG_SCROLL_ARROW,
                                                                                                &gPlayerPCItemPageInfo.itemsAbove);
}

static void ItemStorage_RemoveScrollIndicator(void)
{
    if (gPlayerPCItemPageInfo.scrollIndicatorTaskId != TASK_NONE)
    {
        RemoveScrollIndicatorArrowPair(gPlayerPCItemPageInfo.scrollIndicatorTaskId);
        gPlayerPCItemPageInfo.scrollIndicatorTaskId = TASK_NONE;
    }
}

static void ItemStorage_SetSwapArrow(u8 listTaskId, u8 b, u8 speed)
{
    ItemStorage_DrawSwapArrow(ListMenuGetYCoordForPrintingArrowCursor(listTaskId), b, speed);
}

static void ItemStorage_DrawSwapArrow(u8 y, u8 b, u8 speed)
{
    u8 windowId = sItemStorageMenu->windowIds[ITEMPC_WIN_LIST];
    if (b == 0xFF)
        FillWindowPixelRect(windowId, PIXEL_FILL(1), 0, y, GetMenuCursorDimensionByFont(FONT_NORMAL, 0), GetMenuCursorDimensionByFont(FONT_NORMAL, 1));
    else
        AddTextPrinterParameterized4(windowId, FONT_NORMAL, 0, y, 0, 0, sSwapArrowTextColors, speed, gText_SelectorArrow2);
}

static void ItemStorage_DrawItemIcon(u16 itemId)
{
    u8 spriteId;
    u8 *spriteIdLoc = &sItemStorageMenu->spriteId;

    if (*spriteIdLoc == SPRITE_NONE)
    {
        FreeSpriteTilesByTag(TAG_ITEM_ICON);
        FreeSpritePaletteByTag(TAG_ITEM_ICON);
        spriteId = AddItemIconSprite(TAG_ITEM_ICON, TAG_ITEM_ICON, itemId);
        if (spriteId != MAX_SPRITES)
        {
            *spriteIdLoc = spriteId;
            gSprites[spriteId].oam.priority = 0;
            gSprites[spriteId].x2 = 24;
            gSprites[spriteId].y2 = 80;
        }
    }
}

static void ItemStorage_EraseItemIcon(void)
{
    u8 *spriteIdLoc = &sItemStorageMenu->spriteId;
    if (*spriteIdLoc != SPRITE_NONE)
    {
        FreeSpriteTilesByTag(TAG_ITEM_ICON);
        FreeSpritePaletteByTag(TAG_ITEM_ICON);
        DestroySprite(&gSprites[*spriteIdLoc]);
        *spriteIdLoc = SPRITE_NONE;
    }
}

static void ItemStorage_CompactList(void)
{
    CompactPCItems();
    SetItemListPerPageCount(gSaveBlock1Ptr->pcItems, PC_ITEMS_COUNT, &gPlayerPCItemPageInfo.pageItems, &gPlayerPCItemPageInfo.count, 8);
}

static void ItemStorage_CompactCursor(void)
{
    SetCursorWithinListBounds(&gPlayerPCItemPageInfo.itemsAbove, &gPlayerPCItemPageInfo.cursorPos, gPlayerPCItemPageInfo.pageItems, gPlayerPCItemPageInfo.count);
}

static void ItemStorage_CreateListMenu(u8 taskId)
{
    s16 *data;
    bool32 toss;
    u32 i, x;
    const u8 *text;

    data = gTasks[taskId].data;
    for (i = 0; i <= ITEMPC_WIN_LIST_END; i++)
        ItemStorage_AddWindow(i);
    toss = tInTossMenu;
    text = gText_TossItem;
    if (!toss)
        text = gText_WithdrawItem;
    x = GetStringCenterAlignXOffset(FONT_NORMAL, text, 104);
    AddTextPrinterParameterized(sItemStorageMenu->windowIds[ITEMPC_WIN_TITLE], FONT_NORMAL, text, x, 1, 0, NULL);
    CopyWindowToVram(sItemStorageMenu->windowIds[ITEMPC_WIN_ICON], COPYWIN_GFX);
    ItemStorage_CompactList();
    ItemStorage_CompactCursor();
    ItemStorage_RefreshListMenu();
    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, gPlayerPCItemPageInfo.itemsAbove, gPlayerPCItemPageInfo.cursorPos);
    ItemStorage_AddScrollIndicator();
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = ItemStorage_ProcessInput;
}

static const u8 *ItemStorage_GetMessage(u16 itemId)
{
    const u8 *string;

    switch(itemId)
    {
    case MSG_GO_BACK_TO_PREV:
        string = gText_GoBackPrevMenu;
        break;
    case MSG_HOW_MANY_TO_WITHDRAW:
        string = gText_WithdrawHowManyItems;
        break;
    case MSG_WITHDREW_ITEM:
        string = gText_WithdrawXItems;
        break;
    case MSG_HOW_MANY_TO_TOSS:
        string = gText_TossHowManyVar1s;
        break;
    case MSG_THREW_AWAY_ITEM:
        string = gText_ThrewAwayVar2Var1s;
        break;
    case MSG_NO_MORE_ROOM:
        string = gText_NoRoomInBag;
        break;
    case MSG_TOO_IMPORTANT:
        string = gText_TooImportantToToss;
        break;
    case MSG_OKAY_TO_THROW_AWAY:
        string = gText_ConfirmTossItems;
        break;
    case MSG_SWITCH_WHICH_ITEM:
        string = gText_MoveVar1Where;
        break;
    default:
        string = ItemId_GetDescription(itemId);
        break;
    }
    return string;
}

static void ItemStorage_PrintMessage(const u8 *string)
{
    u8 windowId = sItemStorageMenu->windowIds[ITEMPC_WIN_MESSAGE];
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    StringExpandPlaceholders(gStringVarBuffer, string);
    AddTextPrinterParameterized(windowId, FONT_NORMAL, gStringVarBuffer, 0, 1, 0, NULL);
}

// Process input while on the item storage's item list
static void ItemStorage_ProcessInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (JOY_NEW(SELECT_BUTTON))
    {
        // 'Select' starts input for swapping items if not on Cancel
        ListMenuGetScrollAndRow(tListTaskId, &gPlayerPCItemPageInfo.itemsAbove, &gPlayerPCItemPageInfo.cursorPos);
        if (gPlayerPCItemPageInfo.itemsAbove + gPlayerPCItemPageInfo.cursorPos != gPlayerPCItemPageInfo.count - 1)
        {
            PlaySE(SE_SELECT);
            ItemStorage_StartItemSwap(taskId);
        }
    }
    else
    {
        s32 id = ListMenu_ProcessInput(tListTaskId);
        ListMenuGetScrollAndRow(tListTaskId, &gPlayerPCItemPageInfo.itemsAbove, &gPlayerPCItemPageInfo.cursorPos);
        switch (id)
        {
        case LIST_NOTHING_CHOSEN:
            break;
        case LIST_CANCEL:
            PlaySE(SE_SELECT);
            ItemStorage_ExitItemList(taskId);
            break;
        default:
            PlaySE(SE_SELECT);
            ItemStorage_DoItemAction(taskId);
            break;
        }
    }
}

static void ItemStorage_ReturnToMenuSelect(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (!IsDma3ManagerBusyWithBgCopy())
    {
        DrawDialogueFrame(0, FALSE);

        // Select Withdraw/Toss by default depending on which was just exited
        if (!tInTossMenu)
            InitItemStorageMenu(taskId, MENU_WITHDRAW);
        else
            InitItemStorageMenu(taskId, MENU_TOSS);
        gTasks[taskId].func = ItemStorageMenuProcessInput;
    }
}

static void ItemStorage_ExitItemList(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    ItemStorage_EraseItemIcon();
    ItemStorage_RemoveScrollIndicator();
    DestroyListMenuTask(tListTaskId, NULL, NULL);
    DestroySwapLineSprites(sItemStorageMenu->swapLineSpriteIds, SWAP_LINE_LENGTH);
    ItemStorage_Free();
    gTasks[taskId].func = ItemStorage_ReturnToMenuSelect;
}

static void ItemStorage_StartItemSwap(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    ListMenuSetUnkIndicatorsStructField(tListTaskId, 16, 1);
    sItemStorageMenu->toSwapPos = gPlayerPCItemPageInfo.itemsAbove + gPlayerPCItemPageInfo.cursorPos;
    ItemStorage_SetSwapArrow(tListTaskId, 0, 0);
    ItemStorage_UpdateSwapLinePos(sItemStorageMenu->toSwapPos);
    CopyItemName(gSaveBlock1Ptr->pcItems[sItemStorageMenu->toSwapPos].itemId, gStringVar1);
    ItemStorage_PrintMessage(ItemStorage_GetMessage(MSG_SWITCH_WHICH_ITEM));
    gTasks[taskId].func = ItemStorage_ProcessItemSwapInput;
}

static void ItemStorage_ProcessItemSwapInput(u8 taskId)
{
    s16 *data;
    s32 id;

    data = gTasks[taskId].data;
    if (JOY_NEW(SELECT_BUTTON))
    {
        ListMenuGetScrollAndRow(tListTaskId, &gPlayerPCItemPageInfo.itemsAbove, &gPlayerPCItemPageInfo.cursorPos);
        ItemStorage_FinishItemSwap(taskId, FALSE);
        return;
    }
    id = ListMenu_ProcessInput(tListTaskId);
    ListMenuGetScrollAndRow(tListTaskId, &gPlayerPCItemPageInfo.itemsAbove, &gPlayerPCItemPageInfo.cursorPos);
    SetSwapLineSpritesInvisibility(sItemStorageMenu->swapLineSpriteIds, SWAP_LINE_LENGTH, FALSE);
    ItemStorage_UpdateSwapLinePos(gPlayerPCItemPageInfo.cursorPos);
    switch (id)
    {
    case LIST_NOTHING_CHOSEN:
        break;
    case LIST_CANCEL:
        if (JOY_NEW(A_BUTTON))
            ItemStorage_FinishItemSwap(taskId, FALSE);
        else
            ItemStorage_FinishItemSwap(taskId, TRUE);
        break;
    default:
        ItemStorage_FinishItemSwap(taskId, FALSE);
        break;
    }
}

static void ItemStorage_FinishItemSwap(u8 taskId, bool8 canceled)
{
    s16 *data = gTasks[taskId].data;
    u16 newPos = gPlayerPCItemPageInfo.itemsAbove + gPlayerPCItemPageInfo.cursorPos;
    PlaySE(SE_SELECT);
    DestroyListMenuTask(tListTaskId, &gPlayerPCItemPageInfo.itemsAbove, &gPlayerPCItemPageInfo.cursorPos);

    if (!canceled && sItemStorageMenu->toSwapPos != newPos && sItemStorageMenu->toSwapPos != newPos - 1)
    {
        MoveItemSlotInList(gSaveBlock1Ptr->pcItems, sItemStorageMenu->toSwapPos, newPos);
        ItemStorage_RefreshListMenu();
    }
    if (sItemStorageMenu->toSwapPos < newPos)
        gPlayerPCItemPageInfo.cursorPos--;

    SetSwapLineSpritesInvisibility(sItemStorageMenu->swapLineSpriteIds, SWAP_LINE_LENGTH, TRUE);
    sItemStorageMenu->toSwapPos = NOT_SWAPPING;
    tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, gPlayerPCItemPageInfo.itemsAbove, gPlayerPCItemPageInfo.cursorPos);
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = ItemStorage_ProcessInput;
}

static void ItemStorage_UpdateSwapLinePos(u8 y)
{
    UpdateSwapLineSpritesPos(sItemStorageMenu->swapLineSpriteIds, SWAP_LINE_LENGTH, 128, (y+1) * 16);
}

static void ItemStorage_PrintItemQuantity(u8 windowId, u16 value, u32 mode, u8 x, u8 y, u8 n)
{
    ConvertIntToDecimalStringN(gStringVar1, value, mode, n);
    StringExpandPlaceholders(gStringVarBuffer, gText_xVar1);
    AddTextPrinterParameterized(windowId, FONT_NORMAL, gStringVarBuffer, GetStringCenterAlignXOffset(FONT_NORMAL, gStringVarBuffer, 48), y, 0, NULL);
}

// Start an item Withdraw/Toss
static void ItemStorage_DoItemAction(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 pos = gPlayerPCItemPageInfo.cursorPos + gPlayerPCItemPageInfo.itemsAbove;
    ItemStorage_RemoveScrollIndicator();
    tQuantity = 1;

    if (!tInTossMenu)
    {
        if (gSaveBlock1Ptr->pcItems[pos].quantity == 1)
        {
            // Withdrawing 1 item, do it automatically
            ItemStorage_DoItemWithdraw(taskId);
            return;
        }

        // Withdrawing multiple items, show "how many" message
        CopyItemName(gSaveBlock1Ptr->pcItems[pos].itemId, gStringVar1);
        ItemStorage_PrintMessage(ItemStorage_GetMessage(MSG_HOW_MANY_TO_WITHDRAW));
    }
    else
    {
        if (gSaveBlock1Ptr->pcItems[pos].quantity == 1)
        {
            // Tossing 1 item, do it automatically
            ItemStorage_DoItemToss(taskId);
            return;
        }

        // Tossing multiple items, show "how many" message
        CopyItemName(gSaveBlock1Ptr->pcItems[pos].itemId, gStringVar1);
        ItemStorage_PrintMessage(ItemStorage_GetMessage(MSG_HOW_MANY_TO_TOSS));
    }

    // Set up "how many" prompt
    ItemStorage_PrintItemQuantity(ItemStorage_AddWindow(ITEMPC_WIN_QUANTITY), tQuantity, STR_CONV_MODE_LEADING_ZEROS, 8, 1, 3);
    gTasks[taskId].func = ItemStorage_HandleQuantityRolling;
}

static void ItemStorage_HandleQuantityRolling(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 pos = gPlayerPCItemPageInfo.cursorPos + gPlayerPCItemPageInfo.itemsAbove;

    if (AdjustQuantityAccordingToDPadInput(&tQuantity, gSaveBlock1Ptr->pcItems[pos].quantity) == TRUE)
    {
        ItemStorage_PrintItemQuantity(ItemStorage_AddWindow(ITEMPC_WIN_QUANTITY), tQuantity, STR_CONV_MODE_LEADING_ZEROS, 8, 1, 3);
    }
    else
    {
        if (JOY_NEW(A_BUTTON))
        {
            // Quantity confirmed, perform action
            PlaySE(SE_SELECT);
            ItemStorage_RemoveWindow(ITEMPC_WIN_QUANTITY);
            if (!tInTossMenu)
                ItemStorage_DoItemWithdraw(taskId);
            else
                ItemStorage_DoItemToss(taskId);
        }
        else if (JOY_NEW(B_BUTTON))
        {
            // Canceled action
            PlaySE(SE_SELECT);
            ItemStorage_RemoveWindow(ITEMPC_WIN_QUANTITY);
            ItemStorage_PrintMessage(ItemStorage_GetMessage(gSaveBlock1Ptr->pcItems[pos].itemId));
            ItemStorage_ReturnToListInput(taskId);
        }
    }
}

static void ItemStorage_DoItemWithdraw(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 pos = gPlayerPCItemPageInfo.cursorPos + gPlayerPCItemPageInfo.itemsAbove;

    if (AddBagItem(gSaveBlock1Ptr->pcItems[pos].itemId, tQuantity) == TRUE)
    {
        // Item withdrawn
        CopyItemName(gSaveBlock1Ptr->pcItems[pos].itemId, gStringVar1);
        ConvertIntToDecimalStringN(gStringVar2, tQuantity, STR_CONV_MODE_LEFT_ALIGN, 3);
        ItemStorage_PrintMessage(ItemStorage_GetMessage(MSG_WITHDREW_ITEM));
        gTasks[taskId].func = ItemStorage_HandleRemoveItem;
    }
    else
    {
        // No room to withdraw items
        tQuantity = 0;
        ItemStorage_PrintMessage(ItemStorage_GetMessage(MSG_NO_MORE_ROOM));
        gTasks[taskId].func = ItemStorage_HandleErrorMessageInput;
    }
}

static void ItemStorage_DoItemToss(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 pos = gPlayerPCItemPageInfo.cursorPos + gPlayerPCItemPageInfo.itemsAbove;

    if (!ItemId_GetImportance(gSaveBlock1Ptr->pcItems[pos].itemId))
    {
        // Show toss confirmation prompt
        CopyItemName(gSaveBlock1Ptr->pcItems[pos].itemId, gStringVar1);
        ConvertIntToDecimalStringN(gStringVar2, tQuantity, STR_CONV_MODE_LEFT_ALIGN, 3);
        ItemStorage_PrintMessage(ItemStorage_GetMessage(MSG_OKAY_TO_THROW_AWAY));
        CreateYesNoMenuWithCallbacks(taskId, &sWindowTemplates_ItemStorage[ITEMPC_WIN_YESNO], 1, 0, 1, 0x214, 0xE, &ItemTossYesNoFuncs);
    }
    else
    {
        // Can't toss important items
        tQuantity = 0;
        ItemStorage_PrintMessage(ItemStorage_GetMessage(MSG_TOO_IMPORTANT));
        gTasks[taskId].func = ItemStorage_HandleErrorMessageInput;
    }
}

static void ItemStorage_TossItemYes(u8 taskId)
{
    ItemStorage_PrintMessage(ItemStorage_GetMessage(MSG_THREW_AWAY_ITEM));
    gTasks[taskId].func = ItemStorage_HandleRemoveItem;
}

static void ItemStorage_TossItemNo(u8 taskId)
{
    ItemStorage_PrintMessage(ItemStorage_GetMessage(gSaveBlock1Ptr->pcItems[gPlayerPCItemPageInfo.itemsAbove + gPlayerPCItemPageInfo.cursorPos].itemId));
    ItemStorage_ReturnToListInput(taskId);
}

// Remove item from PC (was either Tossed or Withdrawn)
static void ItemStorage_HandleRemoveItem(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        RemovePCItem(gPlayerPCItemPageInfo.cursorPos + gPlayerPCItemPageInfo.itemsAbove, tQuantity);
        DestroyListMenuTask(tListTaskId, &gPlayerPCItemPageInfo.itemsAbove, &gPlayerPCItemPageInfo.cursorPos);
        ItemStorage_CompactList();
        ItemStorage_CompactCursor();
        ItemStorage_RefreshListMenu();
        tListTaskId = ListMenuInit(&gMultiuseListMenuTemplate, gPlayerPCItemPageInfo.itemsAbove, gPlayerPCItemPageInfo.cursorPos);
        ItemStorage_ReturnToListInput(taskId);
    }
}

static void ItemStorage_HandleErrorMessageInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        ItemStorage_PrintMessage(ItemStorage_GetMessage(gSaveBlock1Ptr->pcItems[gPlayerPCItemPageInfo.itemsAbove + gPlayerPCItemPageInfo.cursorPos].itemId));
        ItemStorage_ReturnToListInput(taskId);
    }
}

static void ItemStorage_ReturnToListInput(u8 taskId)
{
    ItemStorage_AddScrollIndicator();
    gTasks[taskId].func = ItemStorage_ProcessInput;
}
