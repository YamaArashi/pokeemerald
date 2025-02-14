#include "global.h"
#include "item_use.h"
#include "battle.h"
#include "battle_pyramid.h"
#include "battle_pyramid_bag.h"
#include "berry.h"
#include "berry_powder.h"
#include "bike.h"
#include "coins.h"
#include "data.h"
#include "event_data.h"
#include "event_object_lock.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "fieldmap.h"
#include "field_effect.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_weather.h"
#include "item.h"
#include "item_menu.h"
#include "item_use.h"
#include "mail.h"
#include "main.h"
#include "menu.h"
#include "menu_helpers.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pokeblock.h"
#include "pokemon.h"
#include "script.h"
#include "sound.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "constants/event_bg.h"
#include "constants/event_objects.h"
#include "constants/item_effects.h"
#include "constants/items.h"
#include "constants/songs.h"

static void SetUpItemUseCallback(u8);
static void FieldCB_UseItemOnField(void);
static void Task_CallItemUseOnFieldCallback(u8);
static void Task_UseItemfinder(u8);
static void Task_CloseItemfinderMessage(u8);
static void Task_HiddenItemNearby(u8);
static void Task_StandingOnHiddenItem(u8);
static bool8 ItemfinderCheckForHiddenItems(const struct MapEvents *, u8);
static u8 GetDirectionToHiddenItem(s16, s16);
static void PlayerFaceHiddenItem(u8);
static void CheckForHiddenItemsInMapConnection(u8);
static void Task_OpenRegisteredPokeblockCase(u8);
static void ItemUseOnFieldCB_Bike(u8);
static void ItemUseOnFieldCB_Rod(u8);
static void ItemUseOnFieldCB_Itemfinder(u8);
static void ItemUseOnFieldCB_Berry(u8);
static void ItemUseOnFieldCB_WailmerPailBerry(u8);
static void ItemUseOnFieldCB_WailmerPailSudowoodo(u8);
static bool8 TryToWaterSudowoodo(void);
static void BootUpSoundTMHM(u8);
static void Task_ShowTMHMContainedMessage(u8);
static void UseTMHMYesNo(u8);
static void UseTMHM(u8);
static void Task_StartUseRepel(u8);
static void Task_UseRepel(u8);
static void Task_CloseCantUseKeyItemMessage(u8);
static void SetDistanceOfClosestHiddenItem(u8, s16, s16);
static void CB2_OpenPokeblockFromBag(void);

// EWRAM variables
EWRAM_DATA static void(*sItemUseOnFieldCB)(u8 taskId) = NULL;

// Below is set TRUE by UseRegisteredKeyItemOnField
#define tUsingRegisteredKeyItem  data[3]

// UB here if an item with type ITEM_USE_MAIL or ITEM_USE_BAG_MENU uses SetUpItemUseCallback
// Never occurs in vanilla, but can occur with improperly created items
static const MainCallback sItemUseCallbacks[] =
{
    [ITEM_USE_PARTY_MENU - 1]  = CB2_ShowPartyMenuForItemUse,
    [ITEM_USE_FIELD - 1]       = CB2_ReturnToField,
    [ITEM_USE_PBLOCK_CASE - 1] = NULL,
};

static const u8 sClockwiseDirections[] = {DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST};

static const struct YesNoFuncTable sUseTMHMYesNoFuncTable =
{
    .yesFunc = UseTMHM,
    .noFunc = CloseItemMessage,
};

#define tEnigmaBerryType data[4]
static void SetUpItemUseCallback(u8 taskId)
{
    u8 type;
    if (gSpecialVar_ItemId == ITEM_ENIGMA_BERRY)
        type = gTasks[taskId].tEnigmaBerryType - 1;
    else
        type = ItemId_GetType(gSpecialVar_ItemId) - 1;
    if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
    {
        gBagMenu->newScreenCallback = sItemUseCallbacks[type];
        Task_FadeAndCloseBagMenu(taskId);
    }
    else
    {
        gPyramidBagMenu->newScreenCallback = sItemUseCallbacks[type];
        CloseBattlePyramidBag(taskId);
    }
}

static void SetUpItemUseOnFieldCallback(u8 taskId)
{
    if (gTasks[taskId].tUsingRegisteredKeyItem != TRUE)
    {
        gFieldCallback = FieldCB_UseItemOnField;
        SetUpItemUseCallback(taskId);
    }
    else
    {
        sItemUseOnFieldCB(taskId);
    }
}

static void FieldCB_UseItemOnField(void)
{
    FadeInFromBlack();
    CreateTask(Task_CallItemUseOnFieldCallback, 8);
}

static void Task_CallItemUseOnFieldCallback(u8 taskId)
{
    if (IsWeatherNotFadingIn() == 1)
        sItemUseOnFieldCB(taskId);
}

static void DisplayCannotUseItemMessage(u8 taskId, bool8 isUsingRegisteredKeyItemOnField, const u8 *str)
{
    StringExpandPlaceholders(gStringVar4, str);
    if (!isUsingRegisteredKeyItemOnField)
    {
        if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
            DisplayItemMessage(taskId, FONT_NORMAL, gStringVar4, CloseItemMessage);
        else
            DisplayItemMessageInBattlePyramid(taskId, gText_DadsAdvice, Task_CloseBattlePyramidBagMessage);
    }
    else
    {
        DisplayItemMessageOnField(taskId, gStringVar4, Task_CloseCantUseKeyItemMessage);
    }
}

static void DisplayDadsAdviceCannotUseItemMessage(u8 taskId, bool8 isUsingRegisteredKeyItemOnField)
{
    DisplayCannotUseItemMessage(taskId, isUsingRegisteredKeyItemOnField, gText_DadsAdvice);
}

static void DisplayCannotDismountBikeMessage(u8 taskId, bool8 isUsingRegisteredKeyItemOnField)
{
    DisplayCannotUseItemMessage(taskId, isUsingRegisteredKeyItemOnField, gText_CantDismountBike);
}

static void Task_CloseCantUseKeyItemMessage(u8 taskId)
{
    ClearDialogWindowAndFrame(0, TRUE);
    DestroyTask(taskId);
    ScriptUnfreezeObjectEvents();
    UnlockPlayerFieldControls();
}

u8 CheckIfItemIsTMHMOrEvolutionStone(u16 itemId)
{
    if (ItemId_GetFieldFunc(itemId) == ItemUseOutOfBattle_TMHM)
        return 1;
    else if (ItemId_GetFieldFunc(itemId) == ItemUseOutOfBattle_EvolutionStone)
        return 2;
    else
        return 0;
}

// Mail in the bag menu can't have a message but it can be checked (view the mail background, no message)
static void CB2_CheckMail(void)
{
    struct Mail mail;
    mail.itemId = gSpecialVar_ItemId;
    ReadMail(&mail, CB2_ReturnToBagMenuPocket, FALSE);
}

void ItemUseOutOfBattle_Mail(u8 taskId)
{
    gBagMenu->newScreenCallback = CB2_CheckMail;
    Task_FadeAndCloseBagMenu(taskId);
}

void ItemUseOutOfBattle_Bike(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    s16 coordsY;
    s16 coordsX;
    u8 behavior;
    PlayerGetDestCoords(&coordsX, &coordsY);
    behavior = MapGridGetMetatileBehaviorAt(coordsX, coordsY);
    if (FlagGet(FLAG_SYS_CYCLING_ROAD) == TRUE || MetatileBehavior_IsVerticalRail(behavior) == TRUE || MetatileBehavior_IsHorizontalRail(behavior) == TRUE || MetatileBehavior_IsIsolatedVerticalRail(behavior) == TRUE || MetatileBehavior_IsIsolatedHorizontalRail(behavior) == TRUE)
    {
        DisplayCannotDismountBikeMessage(taskId, tUsingRegisteredKeyItem);
    }
    else
    {
        if (Overworld_IsBikingAllowed() == TRUE && IsBikingDisallowedByPlayer() == 0)
        {
            sItemUseOnFieldCB = ItemUseOnFieldCB_Bike;
            SetUpItemUseOnFieldCallback(taskId);
        }
        else
        {
            DisplayDadsAdviceCannotUseItemMessage(taskId, tUsingRegisteredKeyItem);
        }
    }
}

static void ItemUseOnFieldCB_Bike(u8 taskId)
{
    if (ItemId_GetSecondaryId(gSpecialVar_ItemId) == MACH_BIKE)
        GetOnOffBike(PLAYER_AVATAR_FLAG_MACH_BIKE);
    else // ACRO_BIKE
        GetOnOffBike(PLAYER_AVATAR_FLAG_ACRO_BIKE);
    ScriptUnfreezeObjectEvents();
    UnlockPlayerFieldControls();
    DestroyTask(taskId);
}

static bool32 CanFish(void)
{
    s16 x, y;
    u16 tileBehavior;

    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    tileBehavior = MapGridGetMetatileBehaviorAt(x, y);

    if (MetatileBehavior_IsWaterfall(tileBehavior))
        return FALSE;

    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_UNDERWATER))
        return FALSE;

    if (!TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
    {
        if (IsPlayerFacingSurfableFishableWater())
            return TRUE;
    }
    else
    {
        if (MetatileBehavior_IsSurfableWaterOrUnderwater(tileBehavior) && MapGridGetCollisionAt(x, y) == 0)
            return TRUE;
        if (MetatileBehavior_IsBridgeOverWaterNoEdge(tileBehavior) == TRUE)
            return TRUE;
    }

    return FALSE;
}

void ItemUseOutOfBattle_Rod(u8 taskId)
{
    if (CanFish() == TRUE)
    {
        sItemUseOnFieldCB = ItemUseOnFieldCB_Rod;
        SetUpItemUseOnFieldCallback(taskId);
    }
    else
    {
        DisplayDadsAdviceCannotUseItemMessage(taskId, gTasks[taskId].tUsingRegisteredKeyItem);
    }
}

static void ItemUseOnFieldCB_Rod(u8 taskId)
{
    StartFishing(ItemId_GetSecondaryId(gSpecialVar_ItemId));
    DestroyTask(taskId);
}

void ItemUseOutOfBattle_Itemfinder(u8 var)
{
    IncrementGameStat(GAME_STAT_USED_ITEMFINDER);
    sItemUseOnFieldCB = ItemUseOnFieldCB_Itemfinder;
    SetUpItemUseOnFieldCallback(var);
}

static void ItemUseOnFieldCB_Itemfinder(u8 taskId)
{
    if (ItemfinderCheckForHiddenItems(gMapHeader.events, taskId) == TRUE)
        gTasks[taskId].func = Task_UseItemfinder;
    else
        DisplayItemMessageOnField(taskId, gText_ItemFinderNothing, Task_CloseItemfinderMessage);
}

// Define itemfinder task data
#define tItemDistanceX    data[0]
#define tItemDistanceY    data[1]
#define tItemFound        data[2]
#define tCounter          data[3] // Used to count delay between beeps and rotations during player spin
#define tItemfinderBeeps  data[4]
#define tFacingDir        data[5]

static void Task_UseItemfinder(u8 taskId)
{
    u8 playerDir;
    u8 playerDirToItem;
    u8 i;
    s16 *data = gTasks[taskId].data;
    if (tCounter == 0)
    {
        if (tItemfinderBeeps == 4)
        {
            playerDirToItem = GetDirectionToHiddenItem(tItemDistanceX, tItemDistanceY);
            if (playerDirToItem != DIR_NONE)
            {
                PlayerFaceHiddenItem(sClockwiseDirections[playerDirToItem - 1]);
                gTasks[taskId].func = Task_HiddenItemNearby;
            }
            else
            {
                // Player is standing on hidden item
                playerDir = GetPlayerFacingDirection();
                for (i = 0; i < ARRAY_COUNT(sClockwiseDirections); i++)
                {
                    if (playerDir == sClockwiseDirections[i])
                        tFacingDir = (i + 1) & 3;
                }
                gTasks[taskId].func = Task_StandingOnHiddenItem;
                tCounter = 0;
                tItemFound = 0;
            }
            return;
        }
        PlaySE(SE_ITEMFINDER);
        tItemfinderBeeps++;
    }
    tCounter = (tCounter + 1) & 0x1F;
}

static void Task_CloseItemfinderMessage(u8 taskId)
{
    ClearDialogWindowAndFrame(0, TRUE);
    ScriptUnfreezeObjectEvents();
    UnlockPlayerFieldControls();
    DestroyTask(taskId);
}

static bool8 ItemfinderCheckForHiddenItems(const struct MapEvents *events, u8 taskId)
{
    int itemX, itemY;
    s16 playerX, playerY, i, distanceX, distanceY;
    PlayerGetDestCoords(&playerX, &playerY);
    gTasks[taskId].tItemFound = FALSE;

    for (i = 0; i < events->bgEventCount; i++)
    {
        // Check if there are any hidden items on the current map that haven't been picked up
        if (events->bgEvents[i].kind == BG_EVENT_HIDDEN_ITEM && !FlagGet(events->bgEvents[i].bgUnion.hiddenItem.hiddenItemId + FLAG_HIDDEN_ITEMS_START))
        {
            itemX = (u16)events->bgEvents[i].x + MAP_OFFSET;
            distanceX = itemX - playerX;
            itemY = (u16)events->bgEvents[i].y + MAP_OFFSET;
            distanceY = itemY - playerY;

            // Player can see 7 metatiles on either side horizontally
            // and 5 metatiles on either side vertically
            if (distanceX >= -7 && distanceX <= 7 && distanceY >= -5 && distanceY <= 5)
                SetDistanceOfClosestHiddenItem(taskId, distanceX, distanceY);
        }
    }

    CheckForHiddenItemsInMapConnection(taskId);
    if (gTasks[taskId].tItemFound == TRUE)
        return TRUE;
    else
        return FALSE;
}

static bool8 IsHiddenItemPresentAtCoords(const struct MapEvents *events, s16 x, s16 y)
{
    u8 bgEventCount = events->bgEventCount;
    const struct BgEvent *bgEvent = events->bgEvents;
    int i;

    for (i = 0; i < bgEventCount; i++)
    {
        if (bgEvent[i].kind == BG_EVENT_HIDDEN_ITEM && x == (u16)bgEvent[i].x && y == (u16)bgEvent[i].y) // hidden item and coordinates matches x and y passed?
        {
            if (!FlagGet(bgEvent[i].bgUnion.hiddenItem.hiddenItemId + FLAG_HIDDEN_ITEMS_START))
                return TRUE;
            else
                return FALSE;
        }
    }
    return FALSE;
}

static bool8 IsHiddenItemPresentInConnection(const struct MapConnection *connection, int x, int y)
{
    s16 connectionX, connectionY;
    struct MapHeader const *const connectionHeader = GetMapHeaderFromConnection(connection);

// To convert our x/y into coordinates that are relative to the connected map, we must:
//  - Subtract the virtual offset used for the border buffer (MAP_OFFSET).
//  - Subtract the horizontal offset between North/South connections, or the vertical offset for East/West
//  - Account for map size. (0,0) is in the NW corner of our map, so when looking North/West we have to add the height/width of the connected map,
//     and when looking South/East we have to subtract the height/width of our current map.
#define localX (x - MAP_OFFSET)
#define localY (y - MAP_OFFSET)
    switch (connection->direction)
    {
    case CONNECTION_NORTH:
        connectionX = localX - connection->offset;
        connectionY = connectionHeader->mapLayout->height + localY;
        break;
    case CONNECTION_SOUTH:
        connectionX = localX - connection->offset;
        connectionY = localY - gMapHeader.mapLayout->height;
        break;
    case CONNECTION_WEST:
        connectionX = connectionHeader->mapLayout->width + localX;
        connectionY = localY - connection->offset;
        break;
    case CONNECTION_EAST:
        connectionX = localX - gMapHeader.mapLayout->width;
        connectionY = localY - connection->offset;
        break;
    default:
        return FALSE;
    }
    return IsHiddenItemPresentAtCoords(connectionHeader->events, connectionX, connectionY);
}

#undef localX
#undef localY

static void CheckForHiddenItemsInMapConnection(u8 taskId)
{
    s16 playerX, playerY;
    s16 x, y;
    s16 width = gMapHeader.mapLayout->width + MAP_OFFSET;
    s16 height = gMapHeader.mapLayout->height + MAP_OFFSET;

    s16 var1 = MAP_OFFSET;
    s16 var2 = MAP_OFFSET;

    PlayerGetDestCoords(&playerX, &playerY);

    // Player can see 7 metatiles on either side horizontally
    // and 5 metatiles on either side vertically
    for (x = playerX - 7; x <= playerX + 7; x++)
    {
        for (y = playerY - 5; y <= playerY + 5; y++)
        {
            if (var1 > x
             || x >= width
             || var2 > y
             || y >= height)
            {
                const struct MapConnection *conn = GetMapConnectionAtPos(x, y);
                if (conn && IsHiddenItemPresentInConnection(conn, x, y) == TRUE)
                    SetDistanceOfClosestHiddenItem(taskId, x - playerX, y - playerY);
            }
        }
    }
}

static void SetDistanceOfClosestHiddenItem(u8 taskId, s16 itemDistanceX, s16 itemDistanceY)
{
    s16 *data = gTasks[taskId].data;
    s16 oldItemAbsX, oldItemAbsY, newItemAbsX, newItemAbsY;

    if (tItemFound == FALSE)
    {
        // No other items found yet, set this one
        tItemDistanceX = itemDistanceX;
        tItemDistanceY = itemDistanceY;
        tItemFound = TRUE;
    }
    else
    {
        // Other items have been found, check if this one is closer

        // Get absolute x distance of the already-found item
        if (tItemDistanceX < 0)
            oldItemAbsX = tItemDistanceX * -1; // WEST
        else
            oldItemAbsX = tItemDistanceX;      // EAST

        // Get absolute y distance of the already-found item
        if (tItemDistanceY < 0)
            oldItemAbsY = tItemDistanceY * -1; // NORTH
        else
            oldItemAbsY = tItemDistanceY;      // SOUTH

        // Get absolute x distance of the newly-found item
        if (itemDistanceX < 0)
            newItemAbsX = itemDistanceX * -1;
        else
            newItemAbsX = itemDistanceX;

        // Get absolute y distance of the newly-found item
        if (itemDistanceY < 0)
            newItemAbsY = itemDistanceY * -1;
        else
            newItemAbsY = itemDistanceY;


        if (oldItemAbsX + oldItemAbsY > newItemAbsX + newItemAbsY)
        {
            // New item is closer
            tItemDistanceX = itemDistanceX;
            tItemDistanceY = itemDistanceY;
        }
        else
        {
            if (oldItemAbsX + oldItemAbsY == newItemAbsX + newItemAbsY
            && (oldItemAbsY > newItemAbsY || (oldItemAbsY == newItemAbsY && tItemDistanceY < itemDistanceY)))
            {
                // If items are equal distance, use whichever is closer on the Y axis or further south
                tItemDistanceX = itemDistanceX;
                tItemDistanceY = itemDistanceY;
            }
        }
    }
}

static u8 GetDirectionToHiddenItem(s16 itemDistanceX, s16 itemDistanceY)
{
    s16 absX, absY;

    if (itemDistanceX == 0 && itemDistanceY == 0)
        return DIR_NONE; // player is standing on the item.

    // Get absolute X distance.
    if (itemDistanceX < 0)
        absX = itemDistanceX * -1;
    else
        absX = itemDistanceX;

    // Get absolute Y distance.
    if (itemDistanceY < 0)
        absY = itemDistanceY * -1;
    else
        absY = itemDistanceY;

    if (absX > absY)
    {
        if (itemDistanceX < 0)
            return DIR_EAST;
        else
            return DIR_NORTH;
    }
    else
    {
        if (absX < absY)
        {
            if (itemDistanceY < 0)
                return DIR_SOUTH;
            else
                return DIR_WEST;
        }
        if (absX == absY)
        {
            if (itemDistanceY < 0)
                return DIR_SOUTH;
            else
                return DIR_WEST;
        }
        return DIR_NONE; // Unreachable
    }
}

static void PlayerFaceHiddenItem(u8 direction)
{
    ObjectEventClearHeldMovementIfFinished(&gObjectEvents[GetObjectEventIdByLocalIdAndMap(OBJ_EVENT_ID_PLAYER, 0, 0)]);
    ObjectEventClearHeldMovement(&gObjectEvents[GetObjectEventIdByLocalIdAndMap(OBJ_EVENT_ID_PLAYER, 0, 0)]);
    UnfreezeObjectEvent(&gObjectEvents[GetObjectEventIdByLocalIdAndMap(OBJ_EVENT_ID_PLAYER, 0, 0)]);
    PlayerTurnInPlace(direction);
}

static void Task_HiddenItemNearby(u8 taskId)
{
    if (ObjectEventCheckHeldMovementStatus(&gObjectEvents[GetObjectEventIdByLocalIdAndMap(OBJ_EVENT_ID_PLAYER, 0, 0)]) == TRUE)
        DisplayItemMessageOnField(taskId, gText_ItemFinderNearby, Task_CloseItemfinderMessage);
}

static void Task_StandingOnHiddenItem(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (ObjectEventCheckHeldMovementStatus(&gObjectEvents[GetObjectEventIdByLocalIdAndMap(OBJ_EVENT_ID_PLAYER, 0, 0)]) == TRUE
    || tItemFound == FALSE)
    {
        // Spin player around on item
        PlayerFaceHiddenItem(sClockwiseDirections[tFacingDir]);
        tItemFound = TRUE;
        tFacingDir = (tFacingDir + 1) & 3;
        tCounter++;

        if (tCounter == 4)
            DisplayItemMessageOnField(taskId, gText_ItemFinderOnTop, Task_CloseItemfinderMessage);
    }
}

// Undefine itemfinder task data
#undef tItemDistanceX
#undef tItemDistanceY
#undef tItemFound
#undef tCounter
#undef tItemfinderBeeps
#undef tFacingDir

void ItemUseOutOfBattle_PokeblockCase(u8 taskId)
{
    if (MenuHelpers_IsLinkActive() == TRUE)
    {
        DisplayDadsAdviceCannotUseItemMessage(taskId, gTasks[taskId].tUsingRegisteredKeyItem);
    }
    else if (gTasks[taskId].tUsingRegisteredKeyItem != TRUE)
    {
        gBagMenu->newScreenCallback = CB2_OpenPokeblockFromBag;
        Task_FadeAndCloseBagMenu(taskId);
    }
    else
    {
        gFieldCallback = FieldCB_ReturnToFieldNoScript;
        FadeScreen(FADE_TO_BLACK, 0);
        gTasks[taskId].func = Task_OpenRegisteredPokeblockCase;
    }
}

static void CB2_OpenPokeblockFromBag(void)
{
    OpenPokeblockCase(PBLOCK_CASE_FIELD, CB2_ReturnToBagMenuPocket);
}

static void Task_OpenRegisteredPokeblockCase(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        OpenPokeblockCase(PBLOCK_CASE_FIELD, CB2_ReturnToField);
        DestroyTask(taskId);
    }
}

void ItemUseOutOfBattle_CoinCase(u8 taskId)
{
    ConvertIntToDecimalStringN(gStringVar1, GetCoins(), STR_CONV_MODE_LEFT_ALIGN, 4);
    StringExpandPlaceholders(gStringVar4, gText_CoinCase);

    if (!gTasks[taskId].tUsingRegisteredKeyItem)
    {
        DisplayItemMessage(taskId, FONT_NORMAL, gStringVar4, CloseItemMessage);
    }
    else
    {
        DisplayItemMessageOnField(taskId, gStringVar4, Task_CloseCantUseKeyItemMessage);
    }
}

void ItemUseOutOfBattle_PowderJar(u8 taskId)
{
    ConvertIntToDecimalStringN(gStringVar1, GetBerryPowder(), STR_CONV_MODE_LEFT_ALIGN, 5);
    StringExpandPlaceholders(gStringVar4, gText_PowderQty);

    if (!gTasks[taskId].tUsingRegisteredKeyItem)
    {
        DisplayItemMessage(taskId, FONT_NORMAL, gStringVar4, CloseItemMessage);
    }
    else
    {
        DisplayItemMessageOnField(taskId, gStringVar4, Task_CloseCantUseKeyItemMessage);
    }
}

void ItemUseOutOfBattle_Berry(u8 taskId)
{
    if (IsPlayerFacingEmptyBerryTreePatch() == TRUE)
    {
        sItemUseOnFieldCB = ItemUseOnFieldCB_Berry;
        gFieldCallback = FieldCB_UseItemOnField;
        gBagMenu->newScreenCallback = CB2_ReturnToField;
        Task_FadeAndCloseBagMenu(taskId);
    }
    else
    {
        ItemId_GetFieldFunc(gSpecialVar_ItemId)(taskId);
    }
}

static void ItemUseOnFieldCB_Berry(u8 taskId)
{
    RemoveBagItem(gSpecialVar_ItemId, 1);
    LockPlayerFieldControls();
    ScriptContext_SetupScript(BerryTree_EventScript_ItemUsePlantBerry);
    DestroyTask(taskId);
}

void ItemUseOutOfBattle_WailmerPail(u8 taskId)
{
    if (TryToWaterSudowoodo() == TRUE)
    {
        sItemUseOnFieldCB = ItemUseOnFieldCB_WailmerPailSudowoodo;
        SetUpItemUseOnFieldCallback(taskId);
    }
    else if (TryToWaterBerryTree() == TRUE)
    {
        sItemUseOnFieldCB = ItemUseOnFieldCB_WailmerPailBerry;
        SetUpItemUseOnFieldCallback(taskId);
    }
    else
    {
        DisplayDadsAdviceCannotUseItemMessage(taskId, gTasks[taskId].tUsingRegisteredKeyItem);
    }
}

static void ItemUseOnFieldCB_WailmerPailBerry(u8 taskId)
{
    LockPlayerFieldControls();
    ScriptContext_SetupScript(BerryTree_EventScript_ItemUseWailmerPail);
    DestroyTask(taskId);
}

static bool8 TryToWaterSudowoodo(void)
{
    s16 x, y;
    u8 elevation;
    u8 objId;
    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    elevation = PlayerGetElevation();
    objId = GetObjectEventIdByPosition(x, y, elevation);
    if (objId == OBJECT_EVENTS_COUNT || gObjectEvents[objId].graphicsId != OBJ_EVENT_GFX_SUDOWOODO)
        return FALSE;
    else
        return TRUE;
}

static void ItemUseOnFieldCB_WailmerPailSudowoodo(u8 taskId)
{
    LockPlayerFieldControls();
    ScriptContext_SetupScript(BattleFrontier_OutsideEast_EventScript_WaterSudowoodo);
    DestroyTask(taskId);
}

void ItemUseOutOfBattle_Medicine(u8 taskId)
{
    gItemUseCB = ItemUseCB_Medicine;
    SetUpItemUseCallback(taskId);
}

void ItemUseOutOfBattle_ReduceEV(u8 taskId)
{
    gItemUseCB = ItemUseCB_ReduceEV;
    SetUpItemUseCallback(taskId);
}

void ItemUseOutOfBattle_SacredAsh(u8 taskId)
{
    gItemUseCB = ItemUseCB_SacredAsh;
    SetUpItemUseCallback(taskId);
}

void ItemUseOutOfBattle_PPRecovery(u8 taskId)
{
    gItemUseCB = ItemUseCB_PPRecovery;
    SetUpItemUseCallback(taskId);
}

void ItemUseOutOfBattle_PPUp(u8 taskId)
{
    gItemUseCB = ItemUseCB_PPUp;
    SetUpItemUseCallback(taskId);
}

void ItemUseOutOfBattle_RareCandy(u8 taskId)
{
    gItemUseCB = ItemUseCB_RareCandy;
    SetUpItemUseCallback(taskId);
}

void ItemUseOutOfBattle_TMHM(u8 taskId)
{
    if (gSpecialVar_ItemId >= ITEM_HM01)
        DisplayItemMessage(taskId, FONT_NORMAL, gText_BootedUpHM, BootUpSoundTMHM); // HM
    else
        DisplayItemMessage(taskId, FONT_NORMAL, gText_BootedUpTM, BootUpSoundTMHM); // TM
}

static void BootUpSoundTMHM(u8 taskId)
{
    PlaySE(SE_PC_LOGIN);
    gTasks[taskId].func = Task_ShowTMHMContainedMessage;
}

static void Task_ShowTMHMContainedMessage(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        StringCopy(gStringVar1, gMoveNames[ItemIdToBattleMoveId(gSpecialVar_ItemId)]);
        StringExpandPlaceholders(gStringVar4, gText_TMHMContainedVar1);
        DisplayItemMessage(taskId, FONT_NORMAL, gStringVar4, UseTMHMYesNo);
    }
}

static void UseTMHMYesNo(u8 taskId)
{
    BagMenu_YesNo(taskId, ITEMWIN_YESNO_HIGH, &sUseTMHMYesNoFuncTable);
}

static void UseTMHM(u8 taskId)
{
    gItemUseCB = ItemUseCB_TMHM;
    SetUpItemUseCallback(taskId);
}

static void RemoveUsedItem(void)
{
    RemoveBagItem(gSpecialVar_ItemId, 1);
    CopyItemName(gSpecialVar_ItemId, gStringVar2);
    StringExpandPlaceholders(gStringVar4, gText_PlayerUsedVar2);
    if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
    {
        UpdatePocketItemList(ItemId_GetPocket(gSpecialVar_ItemId));
        UpdatePocketListPosition(ItemId_GetPocket(gSpecialVar_ItemId));
    }
    else
    {
        UpdatePyramidBagList();
        UpdatePyramidBagCursorPos();
    }
}

void ItemUseOutOfBattle_Repel(u8 taskId)
{
    if (VarGet(VAR_REPEL_STEP_COUNT) == 0)
        gTasks[taskId].func = Task_StartUseRepel;
    else if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
        DisplayItemMessage(taskId, FONT_NORMAL, gText_RepelEffectsLingered, CloseItemMessage);
    else
        DisplayItemMessageInBattlePyramid(taskId, gText_RepelEffectsLingered, Task_CloseBattlePyramidBagMessage);
}

static void Task_StartUseRepel(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (++data[8] > 7)
    {
        data[8] = 0;
        PlaySE(SE_REPEL);
        gTasks[taskId].func = Task_UseRepel;
    }
}

static void Task_UseRepel(u8 taskId)
{
    if (!IsSEPlaying())
    {
        VarSet(VAR_REPEL_STEP_COUNT, ItemId_GetHoldEffectParam(gSpecialVar_ItemId));
        RemoveUsedItem();
        if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
            DisplayItemMessage(taskId, FONT_NORMAL, gStringVar4, CloseItemMessage);
        else
            DisplayItemMessageInBattlePyramid(taskId, gStringVar4, Task_CloseBattlePyramidBagMessage);
    }
}

static void Task_UsedBlackWhiteFlute(u8 taskId)
{
    if(++gTasks[taskId].data[8] > 7)
    {
        PlaySE(SE_GLASS_FLUTE);
        if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
            DisplayItemMessage(taskId, FONT_NORMAL, gStringVar4, CloseItemMessage);
        else
            DisplayItemMessageInBattlePyramid(taskId, gStringVar4, Task_CloseBattlePyramidBagMessage);
    }
}

void ItemUseOutOfBattle_BlackWhiteFlute(u8 taskId)
{
    CopyItemName(gSpecialVar_ItemId, gStringVar2);
    if (gSpecialVar_ItemId == ITEM_WHITE_FLUTE)
    {
        FlagSet(FLAG_SYS_ENC_UP_ITEM);
        FlagClear(FLAG_SYS_ENC_DOWN_ITEM);
        StringExpandPlaceholders(gStringVar4, gText_UsedVar2WildLured);
    }
    else
    {
        FlagSet(FLAG_SYS_ENC_DOWN_ITEM);
        FlagClear(FLAG_SYS_ENC_UP_ITEM);
        StringExpandPlaceholders(gStringVar4, gText_UsedVar2WildRepelled);
    }
    gTasks[taskId].data[8] = 0;
    gTasks[taskId].func = Task_UsedBlackWhiteFlute;
}

void Task_UseDigEscapeRopeOnField(u8 taskId)
{
    ResetInitialPlayerAvatarState();
    StartEscapeRopeFieldEffect();
    DestroyTask(taskId);
}

static void ItemUseOnFieldCB_EscapeRope(u8 taskId)
{
    Overworld_ResetStateAfterDigEscRope();
    RemoveUsedItem();
    gTasks[taskId].data[0] = 0;
    DisplayItemMessageOnField(taskId, gStringVar4, Task_UseDigEscapeRopeOnField);
}

bool8 CanUseDigOrEscapeRopeOnCurMap(void)
{
    if (gMapHeader.allowEscaping)
        return TRUE;
    else
        return FALSE;
}

void ItemUseOutOfBattle_EscapeRope(u8 taskId)
{
    if (CanUseDigOrEscapeRopeOnCurMap() == TRUE)
    {
        sItemUseOnFieldCB = ItemUseOnFieldCB_EscapeRope;
        SetUpItemUseOnFieldCallback(taskId);
    }
    else
    {
        DisplayDadsAdviceCannotUseItemMessage(taskId, gTasks[taskId].tUsingRegisteredKeyItem);
    }
}

void ItemUseOutOfBattle_EvolutionStone(u8 taskId)
{
    gItemUseCB = ItemUseCB_EvolutionStone;
    SetUpItemUseCallback(taskId);
}

void ItemUseInBattle_PokeBall(u8 taskId)
{
    if (IsPlayerPartyAndPokemonStorageFull() == FALSE) // have room for mon?
    {
        RemoveBagItem(gSpecialVar_ItemId, 1);
        if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
            Task_FadeAndCloseBagMenu(taskId);
        else
            CloseBattlePyramidBag(taskId);
    }
    else if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
    {
        DisplayItemMessage(taskId, FONT_NORMAL, gText_BoxFull, CloseItemMessage);
    }
    else
    {
        DisplayItemMessageInBattlePyramid(taskId, gText_BoxFull, Task_CloseBattlePyramidBagMessage);
    }
}

static void Task_CloseStatIncreaseMessage(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
            Task_FadeAndCloseBagMenu(taskId);
        else
            CloseBattlePyramidBag(taskId);
    }
}

static void Task_UseStatIncreaseItem(u8 taskId)
{
    if(++gTasks[taskId].data[8] > 7)
    {
        PlaySE(SE_USE_ITEM);
        RemoveBagItem(gSpecialVar_ItemId, 1);
        if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
            DisplayItemMessage(taskId, FONT_NORMAL, UseStatIncreaseItem(gSpecialVar_ItemId), Task_CloseStatIncreaseMessage);
        else
            DisplayItemMessageInBattlePyramid(taskId, UseStatIncreaseItem(gSpecialVar_ItemId), Task_CloseStatIncreaseMessage);
    }
}

// e.g. X Attack, Guard Spec
void ItemUseInBattle_StatIncrease(u8 taskId)
{
    u16 partyId = gBattlerPartyIndexes[gBattlerInMenuId];

    if (ExecuteTableBasedItemEffect(&gPlayerParty[partyId], gSpecialVar_ItemId, partyId, 0) != FALSE)
    {
        if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
            DisplayItemMessage(taskId, FONT_NORMAL, gText_WontHaveEffect, CloseItemMessage);
        else
            DisplayItemMessageInBattlePyramid(taskId, gText_WontHaveEffect, Task_CloseBattlePyramidBagMessage);
    }
    else
    {
        gTasks[taskId].func = Task_UseStatIncreaseItem;
        gTasks[taskId].data[8] = 0;
    }
}

static void ItemUseInBattle_ShowPartyMenu(u8 taskId)
{
    if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
    {
        gBagMenu->newScreenCallback = ChooseMonForInBattleItem;
        Task_FadeAndCloseBagMenu(taskId);
    }
    else
    {
        gPyramidBagMenu->newScreenCallback = ChooseMonForInBattleItem;
        CloseBattlePyramidBag(taskId);
    }
}

void ItemUseInBattle_Medicine(u8 taskId)
{
    gItemUseCB = ItemUseCB_Medicine;
    ItemUseInBattle_ShowPartyMenu(taskId);
}

// Unused. Sacred Ash cannot be used in battle
void ItemUseInBattle_SacredAsh(u8 taskId)
{
    gItemUseCB = ItemUseCB_SacredAsh;
    ItemUseInBattle_ShowPartyMenu(taskId);
}

void ItemUseInBattle_PPRecovery(u8 taskId)
{
    gItemUseCB = ItemUseCB_PPRecovery;
    ItemUseInBattle_ShowPartyMenu(taskId);
}

// Fluffy Tail / Poke Doll
void ItemUseInBattle_Escape(u8 taskId)
{

    if((gBattleTypeFlags & BATTLE_TYPE_TRAINER) == FALSE)
    {
        RemoveUsedItem();
        if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
            DisplayItemMessage(taskId, FONT_NORMAL, gStringVar4, Task_FadeAndCloseBagMenu);
        else
            DisplayItemMessageInBattlePyramid(taskId, gStringVar4, CloseBattlePyramidBag);
    }
    else
    {
        DisplayDadsAdviceCannotUseItemMessage(taskId, gTasks[taskId].tUsingRegisteredKeyItem);
    }
}

void ItemUseOutOfBattle_EnigmaBerry(u8 taskId)
{
    switch (GetItemEffectType(gSpecialVar_ItemId))
    {
    case ITEM_EFFECT_HEAL_HP:
    case ITEM_EFFECT_CURE_POISON:
    case ITEM_EFFECT_CURE_SLEEP:
    case ITEM_EFFECT_CURE_BURN:
    case ITEM_EFFECT_CURE_FREEZE:
    case ITEM_EFFECT_CURE_PARALYSIS:
    case ITEM_EFFECT_CURE_ALL_STATUS:
    case ITEM_EFFECT_ATK_EV:
    case ITEM_EFFECT_HP_EV:
    case ITEM_EFFECT_SPATK_EV:
    case ITEM_EFFECT_SPDEF_EV:
    case ITEM_EFFECT_SPEED_EV:
    case ITEM_EFFECT_DEF_EV:
        gTasks[taskId].tEnigmaBerryType = ITEM_USE_PARTY_MENU;
        ItemUseOutOfBattle_Medicine(taskId);
        break;
    case ITEM_EFFECT_SACRED_ASH:
        gTasks[taskId].tEnigmaBerryType = ITEM_USE_PARTY_MENU;
        ItemUseOutOfBattle_SacredAsh(taskId);
        break;
    case ITEM_EFFECT_RAISE_LEVEL:
        gTasks[taskId].tEnigmaBerryType = ITEM_USE_PARTY_MENU;
        ItemUseOutOfBattle_RareCandy(taskId);
        break;
    case ITEM_EFFECT_PP_UP:
    case ITEM_EFFECT_PP_MAX:
        gTasks[taskId].tEnigmaBerryType = ITEM_USE_PARTY_MENU;
        ItemUseOutOfBattle_PPUp(taskId);
        break;
    case ITEM_EFFECT_HEAL_PP:
        gTasks[taskId].tEnigmaBerryType = ITEM_USE_PARTY_MENU;
        ItemUseOutOfBattle_PPRecovery(taskId);
        break;
    default:
        gTasks[taskId].tEnigmaBerryType = ITEM_USE_BAG_MENU;
        ItemUseOutOfBattle_CannotUse(taskId);
        break;
    }
}

void ItemUseInBattle_EnigmaBerry(u8 taskId)
{
    switch (GetItemEffectType(gSpecialVar_ItemId))
    {
    case ITEM_EFFECT_X_ITEM:
        ItemUseInBattle_StatIncrease(taskId);
        break;
    case ITEM_EFFECT_HEAL_HP:
    case ITEM_EFFECT_CURE_POISON:
    case ITEM_EFFECT_CURE_SLEEP:
    case ITEM_EFFECT_CURE_BURN:
    case ITEM_EFFECT_CURE_FREEZE:
    case ITEM_EFFECT_CURE_PARALYSIS:
    case ITEM_EFFECT_CURE_ALL_STATUS:
    case ITEM_EFFECT_CURE_CONFUSION:
    case ITEM_EFFECT_CURE_INFATUATION:
        ItemUseInBattle_Medicine(taskId);
        break;
    case ITEM_EFFECT_HEAL_PP:
        ItemUseInBattle_PPRecovery(taskId);
        break;
    default:
        ItemUseOutOfBattle_CannotUse(taskId);
        break;
    }
}

void ItemUseOutOfBattle_CannotUse(u8 taskId)
{
    DisplayDadsAdviceCannotUseItemMessage(taskId, gTasks[taskId].tUsingRegisteredKeyItem);
}

#undef tUsingRegisteredKeyItem
