#include "global.h"
#include "malloc.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_gfx_sfx_util.h"
#include "battle_interface.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "battle_pyramid_bag.h"
#include "bg.h"
#include "contest.h"
#include "data.h"
#include "decompress.h"
#include "easy_chat.h"
#include "event_data.h"
#include "evolution_scene.h"
#include "field_control_avatar.h"
#include "field_effect.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_specials.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "fldeff.h"
#include "fldeff_misc.h"
#include "frontier_util.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "item.h"
#include "item_menu.h"
#include "item_use.h"
#include "link.h"
#include "link_rfu.h"
#include "mail.h"
#include "main.h"
#include "menu.h"
#include "menu_helpers.h"
#include "menu_specialized.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "player_pc.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "pokemon_jump.h"
#include "pokemon_storage_system.h"
#include "pokemon_summary_screen.h"
#include "region_map.h"
#include "reshow_battle_screen.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "start_menu.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trade.h"
#include "union_room.h"
#include "window.h"
#include "constants/battle.h"
#include "constants/battle_frontier.h"
#include "constants/field_effects.h"
#include "constants/item_effects.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/party_menu.h"
#include "constants/rgb.h"
#include "constants/songs.h"

enum {
    MENU_SUMMARY,
    MENU_SWITCH,
    MENU_CANCEL1,
    MENU_ITEM,
    MENU_GIVE,
    MENU_TAKE_ITEM,
    MENU_MAIL,
    MENU_TAKE_MAIL,
    MENU_READ,
    MENU_CANCEL2,
    MENU_SHIFT,
    MENU_SEND_OUT,
    MENU_ENTER,
    MENU_NO_ENTRY,
    MENU_STORE,
    MENU_REGISTER,
    MENU_TRADE1,
    MENU_TRADE2,
    MENU_TOSS,
    MENU_FIELD_MOVES
};

// IDs for the action lists that appear when a party mon is selected
enum {
    ACTIONS_NONE,
    ACTIONS_SWITCH,
    ACTIONS_SHIFT,
    ACTIONS_SEND_OUT,
    ACTIONS_ENTER,
    ACTIONS_NO_ENTRY,
    ACTIONS_STORE,
    ACTIONS_SUMMARY_ONLY,
    ACTIONS_ITEM,
    ACTIONS_MAIL,
    ACTIONS_REGISTER,
    ACTIONS_TRADE,
    ACTIONS_SPIN_TRADE,
    ACTIONS_TAKEITEM_TOSS,
};

// In CursorCb_FieldMove, field moves <= FIELD_MOVE_WATERFALL are assumed to line up with the badge flags.
// Badge flag names are commented here for people searching for references to remove the badge requirement.
enum {
    FIELD_MOVE_CUT,         // FLAG_BADGE01_GET
    FIELD_MOVE_FLASH,       // FLAG_BADGE02_GET
    FIELD_MOVE_ROCK_SMASH,  // FLAG_BADGE03_GET
    FIELD_MOVE_STRENGTH,    // FLAG_BADGE04_GET
    FIELD_MOVE_SURF,        // FLAG_BADGE05_GET
    FIELD_MOVE_FLY,         // FLAG_BADGE06_GET
    FIELD_MOVE_DIVE,        // FLAG_BADGE07_GET
    FIELD_MOVE_WATERFALL,   // FLAG_BADGE08_GET
    FIELD_MOVE_TELEPORT,
    FIELD_MOVE_DIG,
    FIELD_MOVE_SECRET_POWER,
    FIELD_MOVE_MILK_DRINK,
    FIELD_MOVE_SOFT_BOILED,
    FIELD_MOVE_SWEET_SCENT,
    FIELD_MOVES_COUNT
};

enum {
    PARTY_BOX_LEFT_COLUMN,
    PARTY_BOX_RIGHT_COLUMN,
};

enum {
    TAG_POKEBALL = 1200,
    TAG_POKEBALL_SMALL,
    TAG_STATUS_ICONS,
};

#define TAG_HELD_ITEM 55120

#define PARTY_PAL_SELECTED     (1 << 0)
#define PARTY_PAL_FAINTED      (1 << 1)
#define PARTY_PAL_TO_SWITCH    (1 << 2)
#define PARTY_PAL_MULTI_ALT    (1 << 3)
#define PARTY_PAL_SWITCHING    (1 << 4)
#define PARTY_PAL_TO_SOFTBOIL  (1 << 5)
#define PARTY_PAL_NO_MON       (1 << 6)
#define PARTY_PAL_UNUSED       (1 << 7)

#define MENU_DIR_DOWN     1
#define MENU_DIR_UP      -1
#define MENU_DIR_RIGHT    2
#define MENU_DIR_LEFT    -2

enum {
    CAN_LEARN_MOVE,
    CANNOT_LEARN_MOVE,
    ALREADY_KNOWS_MOVE,
    CANNOT_LEARN_MOVE_IS_EGG
};

enum {
    // Window ids 0-5 are implicitly assigned to each party Pokémon in InitPartyMenuBoxes
    WIN_MSG = PARTY_SIZE,
};

struct PartyMenuBoxInfoRects
{
    void (*blitFunc)(u8, u8, u8, u8, u8, bool8);
    u8 dimensions[24];
    u8 descTextLeft;
    u8 descTextTop;
    u8 descTextWidth;
    u8 descTextHeight;
};

struct PartyMenuInternal
{
    TaskFunc task;
    MainCallback exitCallback;
    u32 chooseHalf:1;
    u32 lastSelectedSlot:3;  // Used to return to same slot when going left/right bewtween columns
    u32 spriteIdConfirmPokeball:7;
    u32 spriteIdCancelPokeball:7;
    u32 messageId:14;
    u8 windowId[3];
    u8 actions[8];
    u8 numActions;
    // In vanilla Emerald, only the first 0xB0 hwords (0x160 bytes) are actually used.
    // However, a full 0x100 hwords (0x200 bytes) are allocated.
    // It is likely that the 0x160 value used below is a constant defined by
    // bin2c, the utility used to encode the compressed palette data.
    u16 palBuffer[BG_PLTT_SIZE / sizeof(u16)];
    s16 data[16];
};

struct PartyMenuBox
{
    const struct PartyMenuBoxInfoRects *infoRects;
    const u8 *spriteCoords;
    u8 windowId;
    u8 monSpriteId;
    u8 itemSpriteId;
    u8 pokeballSpriteId;
    u8 statusSpriteId;
};

// EWRAM vars
static EWRAM_DATA struct PartyMenuInternal *sPartyMenuInternal = NULL;
EWRAM_DATA struct PartyMenu gPartyMenu = {0};
static EWRAM_DATA struct PartyMenuBox *sPartyMenuBoxes = NULL;
static EWRAM_DATA u8 *sPartyBgGfxTilemap = NULL;
static EWRAM_DATA u8 *sPartyBgTilemapBuffer = NULL;
EWRAM_DATA bool8 gPartyMenuUseExitCallback = 0;
EWRAM_DATA u8 gSelectedMonPartyId = 0;
EWRAM_DATA MainCallback gPostMenuFieldCallback = NULL;
static EWRAM_DATA u16 *sSlot1TilemapBuffer = 0; // for switching party slots
static EWRAM_DATA u16 *sSlot2TilemapBuffer = 0; //
EWRAM_DATA u8 gSelectedOrderFromParty[MAX_FRONTIER_PARTY_SIZE] = {0};
static EWRAM_DATA u16 sPartyMenuItemId = 0;
static EWRAM_DATA u16 sUnused = 0;
EWRAM_DATA u8 gBattlePartyCurrentOrder[PARTY_SIZE / 2] = {0}; // bits 0-3 are the current pos of Slot 1, 4-7 are Slot 2, and so on

// IWRAM common
COMMON_DATA void (*gItemUseCB)(u8, TaskFunc) = NULL;

static void ResetPartyMenu(void);
static void CB2_InitPartyMenu(void);
static bool8 ShowPartyMenu(void);
static void SetPartyMonsAllowedInMinigame(void);
static void ExitPartyMenu(void);
static bool8 AllocPartyMenuBg(void);
static bool8 AllocPartyMenuBgGfx(void);
static void InitPartyMenuWindows(u8);
static void InitPartyMenuBoxes(u8);
static void LoadPartyMenuPokeballGfx(void);
static void LoadPartyMenuAilmentGfx(void);
static bool8 CreatePartyMonSpritesLoop(void);
static bool8 RenderPartyMenuBoxes(void);
static void CreateCancelConfirmPokeballSprites(void);
static void CreateCancelConfirmWindows(u8);
static void Task_ExitPartyMenu(u8);
static void FreePartyPointers(void);
static void PartyPaletteBufferCopy(u8);
static void DisplayPartyPokemonDataForMultiBattle(u8);
static void LoadPartyBoxPalette(struct PartyMenuBox *, u8);
static void DrawEmptySlot(u8 windowId);
static void DisplayPartyPokemonDataForRelearner(u8);
static void DisplayPartyPokemonDataForContest(u8);
static void DisplayPartyPokemonDataForChooseHalf(u8);
static void DisplayPartyPokemonDataForWirelessMinigame(u8);
static void DisplayPartyPokemonDataForBattlePyramidHeldItem(u8);
static bool8 DisplayPartyPokemonDataForMoveTutorOrEvolutionItem(u8);
static void DisplayPartyPokemonData(u8);
static void DisplayPartyPokemonNickname(struct Pokemon *, struct PartyMenuBox *, u8);
static void DisplayPartyPokemonLevelCheck(struct Pokemon *, struct PartyMenuBox *, u8);
static void DisplayPartyPokemonGenderNidoranCheck(struct Pokemon *, struct PartyMenuBox *, u8);
static void DisplayPartyPokemonHPCheck(struct Pokemon *, struct PartyMenuBox *, u8);
static void DisplayPartyPokemonMaxHPCheck(struct Pokemon *, struct PartyMenuBox *, u8);
static void DisplayPartyPokemonHPBarCheck(struct Pokemon *, struct PartyMenuBox *);
static void DisplayPartyPokemonDescriptionText(u8, struct PartyMenuBox *, u8);
static bool8 IsMonAllowedInMinigame(u8);
static void DisplayPartyPokemonDataToTeachMove(u8, u16, u8);
static u8 CanMonLearnTMTutor(struct Pokemon *, u16, u8);
static void DisplayPartyPokemonBarDetail(u8, const u8 *, u8, const u8 *);
static void DisplayPartyPokemonLevel(u8, struct PartyMenuBox *);
static void DisplayPartyPokemonGender(u8, u16, u8 *, struct PartyMenuBox *);
static void DisplayPartyPokemonHP(u16, struct PartyMenuBox *);
static void DisplayPartyPokemonMaxHP(u16, struct PartyMenuBox *);
static void DisplayPartyPokemonHPBar(u16, u16, struct PartyMenuBox *);
static void CreatePartyMonIconSpriteParameterized(u16, u32, struct PartyMenuBox *, u8, u32);
static void CreatePartyMonHeldItemSpriteParameterized(u16, u16, struct PartyMenuBox *);
static void CreatePartyMonPokeballSpriteParameterized(u16, struct PartyMenuBox *);
static void CreatePartyMonStatusSpriteParameterized(u16, u8, struct PartyMenuBox *);
// These next 4 functions are essentially redundant with the above 4
// The only difference is that rather than receive the data directly they retrieve it from the mon struct
static void CreatePartyMonHeldItemSprite(struct Pokemon *, struct PartyMenuBox *);
static void CreatePartyMonPokeballSprite(struct Pokemon *, struct PartyMenuBox *);
static void CreatePartyMonIconSprite(struct Pokemon *, struct PartyMenuBox *, u32);
static void CreatePartyMonStatusSprite(struct Pokemon *, struct PartyMenuBox *);
static u8 CreateSmallPokeballButtonSprite(u8, u8);
static void DrawCancelConfirmButtons(void);
static u8 CreatePokeballButtonSprite(u8, u8);
static void AnimateSelectedPartyIcon(u8, u8);
static void PartyMenuStartSpriteAnim(u8, u8);
static u8 GetPartyBoxPaletteFlags(u8, u8);
static bool8 PartyBoxPal_ParnterOrDisqualifiedInArena(u8);
static u8 GetPartyIdFromBattleSlot(u8);
static void Task_ClosePartyMenuAndSetCB2(u8);
static void UpdatePartyToFieldOrder(void);
static void MoveCursorToConfirm(void);
static void HandleChooseMonCancel(u8, s8 *);
static void HandleChooseMonSelection(u8, s8 *);
static u16 PartyMenuButtonHandler(s8 *);
static s8 *GetCurrentPartySlotPtr(void);
static bool8 IsSelectedMonNotEgg(u8 *);
static void PartyMenuRemoveWindow(u8 *);
static void CB2_SetUpExitToBattleScreen(void);
static void Task_ClosePartyMenuAfterText(u8);
static void TryTutorSelectedMon(u8);
static void TryGiveMailToSelectedMon(u8);
static void TryGiveItemOrMailToSelectedMon(u8);
static void SwitchSelectedMons(u8);
static void TryEnterMonForMinigame(u8, u8);
static void Task_TryCreateSelectionWindow(u8);
static void FinishTwoMonAction(u8);
static void CancelParticipationPrompt(u8);
static bool8 DisplayCancelChooseMonYesNo(u8);
static const u8 *GetFacilityCancelString(void);
static void Task_CancelChooseMonYesNo(u8);
static void PartyMenuDisplayYesNoMenu(void);
static void Task_HandleCancelChooseMonYesNoInput(u8);
static void Task_ReturnToChooseMonAfterText(u8);
static void UpdateCurrentPartySelection(s8 *, s8);
static void UpdatePartySelectionSingleLayout(s8 *, s8);
static void UpdatePartySelectionDoubleLayout(s8 *, s8);
static s8 GetNewSlotDoubleLayout(s8, s8);
static void PrintMessage(const u8 *);
static void Task_PrintAndWaitForText(u8);
static bool16 IsMonAllowedInPokemonJump(struct Pokemon *);
static bool16 IsMonAllowedInDodrioBerryPicking(struct Pokemon *);
static void Task_CancelParticipationYesNo(u8);
static void Task_HandleCancelParticipationYesNoInput(u8);
static bool8 CanLearnTutorMove(u16, u8);
static u16 GetTutorMove(u8);
static bool8 ShouldUseChooseMonText(void);
static void SetPartyMonFieldSelectionActions(struct Pokemon *, u8);
static u8 GetPartyMenuActionsTypeInBattle(struct Pokemon *);
static u8 GetPartySlotEntryStatus(s8);
static void Task_UpdateHeldItemSprite(u8);
static void Task_HandleSelectionMenuInput(u8);
static void CB2_ShowPokemonSummaryScreen(void);
static void UpdatePartyToBattleOrder(void);
static void CB2_ReturnToPartyMenuFromSummaryScreen(void);
static void SlidePartyMenuBoxOneStep(u8);
static void Task_SlideSelectedSlotsOffscreen(u8);
static void SwitchPartyMon(void);
static void Task_SlideSelectedSlotsOnscreen(u8);
static void CB2_SelectBagItemToGive(void);
static void CB2_GiveHoldItem(void);
static void CB2_WriteMailToGiveMon(void);
static void Task_SwitchHoldItemsPrompt(u8);
static void Task_GiveHoldItem(u8);
static void Task_SwitchItemsYesNo(u8);
static void Task_HandleSwitchItemsYesNoInput(u8);
static void Task_WriteMailToGiveMonAfterText(u8);
static void CB2_ReturnToPartyMenuFromWritingMail(void);
static void Task_DisplayGaveMailFromPartyMessage(u8);
static void UpdatePartyMonHeldItemSprite(struct Pokemon *, struct PartyMenuBox *);
static void Task_TossHeldItemYesNo(u8 taskId);
static void Task_HandleTossHeldItemYesNoInput(u8);
static void Task_TossHeldItem(u8);
static void CB2_ReadHeldMail(void);
static void CB2_ReturnToPartyMenuFromReadingMail(void);
static void Task_SendMailToPCYesNo(u8);
static void Task_HandleSendMailToPCYesNoInput(u8);
static void Task_LoseMailMessageYesNo(u8);
static void Task_HandleLoseMailMessageYesNoInput(u8);
static bool8 TrySwitchInPokemon(void);
static void Task_SpinTradeYesNo(u8);
static void Task_HandleSpinTradeYesNoInput(u8);
static void Task_CancelAfterAorBPress(u8);
static void DisplayFieldMoveExitAreaMessage(u8);
static void DisplayCantUseFlashMessage(void);
static void DisplayCantUseSurfMessage(void);
static void Task_FieldMoveExitAreaYesNo(u8);
static void Task_HandleFieldMoveExitAreaYesNoInput(u8);
static void Task_FieldMoveWaitForFade(u8);
static u16 GetFieldMoveMonSpecies(void);
static void UpdatePartyMonHPBar(u8, struct Pokemon *);
static void SpriteCB_UpdatePartyMonIcon(struct Sprite *);
static void SpriteCB_BouncePartyMonIcon(struct Sprite *);
static void ShowOrHideHeldItemSprite(u16, struct PartyMenuBox *);
static void CreateHeldItemSpriteForTrade(u8, bool8);
static void SpriteCB_HeldItem(struct Sprite *);
static void SetPartyMonAilmentGfx(struct Pokemon *, struct PartyMenuBox *);
static void UpdatePartyMonAilmentGfx(u8, struct PartyMenuBox *);
static u8 GetPartyLayoutFromBattleType(void);
static void Task_SetSacredAshCB(u8);
static void CB2_ReturnToBagMenu(void);
static void Task_DisplayHPRestoredMessage(u8);
static u16 ItemEffectToMonEv(struct Pokemon *, u8);
static void ItemEffectToStatString(u8, u8 *);
static void ReturnToUseOnWhichMon(u8);
static void SetSelectedMoveForPPItem(u8);
static void TryUsePPItem(u8);
static void Task_LearnedMove(u8);
static void Task_ReplaceMoveYesNo(u8);
static void Task_DoLearnedMoveFanfareAfterText(u8);
static void Task_LearnNextMoveOrClosePartyMenu(u8);
static void Task_TryLearningNextMove(u8);
static void Task_HandleReplaceMoveYesNoInput(u8);
static void Task_ShowSummaryScreenToForgetMove(u8);
static void StopLearningMovePrompt(u8);
static void CB2_ShowSummaryScreenToForgetMove(void);
static void CB2_ReturnToPartyMenuWhileLearningMove(void);
static void Task_ReturnToPartyMenuWhileLearningMove(u8);
static void DisplayPartyMenuForgotMoveMessage(u8);
static void Task_PartyMenuReplaceMove(u8);
static void Task_StopLearningMoveYesNo(u8);
static void Task_HandleStopLearningMoveYesNoInput(u8);
static void Task_TryLearningNextMoveAfterText(u8);
static void BufferMonStatsToTaskData(struct Pokemon *, s16 *);
static void UpdateMonDisplayInfoAfterRareCandy(u8, struct Pokemon *);
static void Task_DisplayLevelUpStatsPg1(u8);
static void DisplayLevelUpStatsPg1(u8);
static void Task_DisplayLevelUpStatsPg2(u8);
static void DisplayLevelUpStatsPg2(u8);
static void Task_TryLearnNewMoves(u8);
static void PartyMenuTryEvolution(u8);
static void DisplayMonNeedsToReplaceMove(u8);
static void DisplayMonLearnedMove(u8, u16);
static void UseSacredAsh(u8);
static void Task_SacredAshLoop(u8);
static void Task_SacredAshDisplayHPRestored(u8);
static void GiveItemOrMailToSelectedMon(u8);
static void DisplayItemMustBeRemovedFirstMessage(u8);
static void Task_SwitchItemsFromBagYesNo(u8);
static void RemoveItemToGiveFromBag(u16);
static void CB2_WriteMailToGiveMonFromBag(void);
static void GiveItemToSelectedMon(u8);
static void Task_UpdateHeldItemSpriteAndClosePartyMenu(u8);
static void CB2_ReturnToPartyOrBagMenuFromWritingMail(void);
static bool8 ReturnGiveItemToBagOrPC(u16);
static void Task_DisplayGaveMailFromBagMessage(u8);
static void Task_HandleSwitchItemsFromBagYesNoInput(u8);
static void Task_ValidateChosenHalfParty(u8);
static bool8 GetBattleEntryEligibility(struct Pokemon *);
static bool8 HasPartySlotAlreadyBeenSelected(u8);
static u8 GetBattleEntryLevelCap(void);
static u8 GetMaxBattleEntries(void);
static u8 GetMinBattleEntries(void);
static void Task_ContinueChoosingHalfParty(u8);
static void BufferBattlePartyOrder(u8 *, bool8);
static void BufferBattlePartyOrderBySide(u8 *, u8, u8);
static void Task_InitMultiPartnerPartySlideIn(u8);
static void Task_MultiPartnerPartySlideIn(u8);
static void SlideMultiPartyMenuBoxSpritesOneStep(u8);
static void Task_WaitAfterMultiPartnerPartySlideIn(u8);
static void BufferMonSelection(void);
static void Task_PartyMenuWaitForFade(u8 taskId);
static void Task_ChooseContestMon(u8 taskId);
static void CB2_ChooseContestMon(void);
static void Task_ChoosePartyMon(u8 taskId);
static void Task_ChooseMonForMoveRelearner(u8);
static void CB2_ChooseMonForMoveRelearner(void);
static void Task_BattlePyramidChooseMonHeldItems(u8);
static void ShiftMoveSlot(struct Pokemon *, u8, u8);
static void BlitBitmapToPartyWindow_LeftColumn(u8, u8, u8, u8, u8, bool8);
static void BlitBitmapToPartyWindow_RightColumn(u8, u8, u8, u8, u8, bool8);
static void CursorCb_Summary(u8);
static void CursorCb_Switch(u8);
static void CursorCb_Cancel1(u8);
static void CursorCb_Item(u8);
static void CursorCb_Give(u8);
static void CursorCb_TakeItem(u8);
static void CursorCb_Mail(u8);
static void CursorCb_Read(u8);
static void CursorCb_TakeMail(u8);
static void CursorCb_Cancel2(u8);
static void CursorCb_SendMon(u8);
static void CursorCb_Enter(u8);
static void CursorCb_NoEntry(u8);
static void CursorCb_Store(u8);
static void CursorCb_Register(u8);
static void CursorCb_Trade1(u8);
static void CursorCb_Trade2(u8);
static void CursorCb_Toss(u8);
static void CursorCb_FieldMove(u8);
static bool8 SetUpFieldMove_Surf(void);
static bool8 SetUpFieldMove_Fly(void);
static bool8 SetUpFieldMove_Waterfall(void);
static bool8 SetUpFieldMove_Dive(void);

// static const data
#include "data/pokemon/tutor_learnsets.h"
#include "data/party_menu.h"

// code
static void InitPartyMenu(u8 menuType, u8 layout, u8 partyAction, bool8 keepCursorPos, u8 messageId, TaskFunc task, MainCallback callback)
{
    u16 i;

    ResetPartyMenu();
    sPartyMenuInternal = Alloc(sizeof(struct PartyMenuInternal));
    if (sPartyMenuInternal == NULL)
    {
        SetMainCallback2(callback);
    }
    else
    {
        gPartyMenu.menuType = menuType;
        gPartyMenu.exitCallback = callback;
        gPartyMenu.action = partyAction;
        sPartyMenuInternal->messageId = messageId;
        sPartyMenuInternal->task = task;
        sPartyMenuInternal->exitCallback = NULL;
        sPartyMenuInternal->lastSelectedSlot = 0;
        sPartyMenuInternal->spriteIdConfirmPokeball = 0x7F;
        sPartyMenuInternal->spriteIdCancelPokeball = 0x7F;

        if (menuType == PARTY_MENU_TYPE_CHOOSE_HALF)
            sPartyMenuInternal->chooseHalf = TRUE;
        else
            sPartyMenuInternal->chooseHalf = FALSE;

        if (layout != KEEP_PARTY_LAYOUT)
            gPartyMenu.layout = layout;

        for (i = 0; i < ARRAY_COUNT(sPartyMenuInternal->data); i++)
            sPartyMenuInternal->data[i] = 0;
        for (i = 0; i < ARRAY_COUNT(sPartyMenuInternal->windowId); i++)
            sPartyMenuInternal->windowId[i] = WINDOW_NONE;

        if (!keepCursorPos)
            gPartyMenu.slotId = 0;
        else if (gPartyMenu.slotId > PARTY_SIZE - 1 || GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_SPECIES) == SPECIES_NONE)
            gPartyMenu.slotId = 0;

        gTextFlags.autoScroll = 0;
        CalculatePlayerPartyCount();
        SetMainCallback2(CB2_InitPartyMenu);
    }
}

static void CB2_UpdatePartyMenu(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void VBlankCB_PartyMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_InitPartyMenu(void)
{
    while (TRUE)
    {
        if (MenuHelpers_ShouldWaitForLinkRecv() == TRUE || ShowPartyMenu() == TRUE || MenuHelpers_IsLinkActive() == TRUE)
            break;
    }
}

static bool8 ShowPartyMenu(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankHBlankCallbacksToNull();
        ResetVramOamAndBgCntRegs();
        ClearScheduledBgCopiesToVram();
        gMain.state++;
        break;
    case 1:
        ScanlineEffect_Stop();
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        gPaletteFade.bufferTransferDisabled = TRUE;
        gMain.state++;
        break;
    case 3:
        ResetSpriteData();
        gMain.state++;
        break;
    case 4:
        FreeAllSpritePalettes();
        gMain.state++;
        break;
    case 5:
        if (!MenuHelpers_IsLinkActive())
            ResetTasks();
        gMain.state++;
        break;
    case 6:
        SetPartyMonsAllowedInMinigame();
        gMain.state++;
        break;
    case 7:
        if (!AllocPartyMenuBg())
        {
            ExitPartyMenu();
            return TRUE;
        }
        else
        {
            sPartyMenuInternal->data[0] = 0;
            gMain.state++;
        }
        break;
    case 8:
        if (AllocPartyMenuBgGfx())
            gMain.state++;
        break;
    case 9:
        InitPartyMenuWindows(gPartyMenu.layout);
        gMain.state++;
        break;
    case 10:
        InitPartyMenuBoxes(gPartyMenu.layout);
        sPartyMenuInternal->data[0] = 0;
        gMain.state++;
        break;
    case 11:
        LoadHeldItemIcons();
        gMain.state++;
        break;
    case 12:
        LoadPartyMenuPokeballGfx();
        gMain.state++;
        break;
    case 13:
        LoadPartyMenuAilmentGfx();
        gMain.state++;
        break;
    case 14:
        LoadMonIconPalettes();
        gMain.state++;
        break;
    case 15:
        if (CreatePartyMonSpritesLoop())
        {
            sPartyMenuInternal->data[0] = 0;
            gMain.state++;
        }
        break;
    case 16:
        if (RenderPartyMenuBoxes())
        {
            sPartyMenuInternal->data[0] = 0;
            gMain.state++;
        }
        break;
    case 17:
        CreateCancelConfirmPokeballSprites();
        gMain.state++;
        break;
    case 18:
        CreateCancelConfirmWindows(sPartyMenuInternal->chooseHalf);
        gMain.state++;
        break;
    case 19:
        gMain.state++;
        break;
    case 20:
        CreateTask(sPartyMenuInternal->task, 0);
        DisplayPartyMenuStdMessage(sPartyMenuInternal->messageId);
        gMain.state++;
        break;
    case 21:
        BlendPalettes(PALETTES_ALL, 16, 0);
        gPaletteFade.bufferTransferDisabled = FALSE;
        gMain.state++;
        break;
    case 22:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    default:
        SetVBlankCallback(VBlankCB_PartyMenu);
        SetMainCallback2(CB2_UpdatePartyMenu);
        return TRUE;
    }
    return FALSE;
}

static void ExitPartyMenu(void)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    CreateTask(Task_ExitPartyMenu, 0);
    SetVBlankCallback(VBlankCB_PartyMenu);
    SetMainCallback2(CB2_UpdatePartyMenu);
}

static void Task_ExitPartyMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetMainCallback2(gPartyMenu.exitCallback);
        FreePartyPointers();
        DestroyTask(taskId);
    }
}

static void ResetPartyMenu(void)
{
    sPartyMenuInternal = NULL;
    sPartyBgTilemapBuffer = NULL;
    sPartyMenuBoxes = NULL;
    sPartyBgGfxTilemap = NULL;
}

static bool8 AllocPartyMenuBg(void)
{
    sPartyBgTilemapBuffer = Alloc(0x800);
    if (sPartyBgTilemapBuffer == NULL)
        return FALSE;

    memset(sPartyBgTilemapBuffer, 0, 0x800);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sPartyMenuBgTemplates, ARRAY_COUNT(sPartyMenuBgTemplates));
    SetBgTilemapBuffer(1, sPartyBgTilemapBuffer);
    ResetAllBgsCoordinates();
    ScheduleBgCopyTilemapToVram(1);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    return TRUE;
}

static bool8 AllocPartyMenuBgGfx(void)
{
    u32 sizeout;

    switch (sPartyMenuInternal->data[0])
    {
    case 0:
        sPartyBgGfxTilemap = malloc_and_decompress(gPartyMenuBg_Gfx, &sizeout);
        LoadBgTiles(1, sPartyBgGfxTilemap, sizeout, 0);
        sPartyMenuInternal->data[0]++;
        break;
    case 1:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            LZDecompressWram(gPartyMenuBg_Tilemap, sPartyBgTilemapBuffer);
            sPartyMenuInternal->data[0]++;
        }
        break;
    case 2:
        LoadCompressedPalette(gPartyMenuBg_Pal, BG_PLTT_ID(0), 11 * PLTT_SIZE_4BPP);
        CpuCopy16(gPlttBufferUnfaded, sPartyMenuInternal->palBuffer, 11 * PLTT_SIZE_4BPP);
        sPartyMenuInternal->data[0]++;
        break;
    case 3:
        PartyPaletteBufferCopy(4);
        sPartyMenuInternal->data[0]++;
        break;
    case 4:
        PartyPaletteBufferCopy(5);
        sPartyMenuInternal->data[0]++;
        break;
    case 5:
        PartyPaletteBufferCopy(6);
        sPartyMenuInternal->data[0]++;
        break;
    case 6:
        PartyPaletteBufferCopy(7);
        sPartyMenuInternal->data[0]++;
        break;
    case 7:
        PartyPaletteBufferCopy(8);
        sPartyMenuInternal->data[0]++;
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static void PartyPaletteBufferCopy(u8 palNum)
{
    u8 offset = PLTT_ID(palNum);
    CpuCopy16(&gPlttBufferUnfaded[BG_PLTT_ID(3)], &gPlttBufferUnfaded[offset], PLTT_SIZE_4BPP);
    CpuCopy16(&gPlttBufferUnfaded[BG_PLTT_ID(3)], &gPlttBufferFaded[offset], PLTT_SIZE_4BPP);
}

static void FreePartyPointers(void)
{
    if (sPartyMenuInternal)
        Free(sPartyMenuInternal);
    if (sPartyBgTilemapBuffer)
        Free(sPartyBgTilemapBuffer);
    if (sPartyBgGfxTilemap)
        Free(sPartyBgGfxTilemap);
    if (sPartyMenuBoxes)
        Free(sPartyMenuBoxes);
    FreeAllWindowBuffers();
}

static void InitPartyMenuBoxes(u8 layout)
{
    u8 i;

    sPartyMenuBoxes = Alloc(sizeof(struct PartyMenuBox[PARTY_SIZE]));

    for (i = 0; i < PARTY_SIZE; i++)
    {
        sPartyMenuBoxes[i].infoRects = &sPartyBoxInfoRects[PARTY_BOX_RIGHT_COLUMN];
        sPartyMenuBoxes[i].spriteCoords = sPartyMenuSpriteCoords[layout][i];
        sPartyMenuBoxes[i].windowId = i;
        sPartyMenuBoxes[i].monSpriteId = SPRITE_NONE;
        sPartyMenuBoxes[i].itemSpriteId = SPRITE_NONE;
        sPartyMenuBoxes[i].pokeballSpriteId = SPRITE_NONE;
        sPartyMenuBoxes[i].statusSpriteId = SPRITE_NONE;
    }
    // The first party mon goes in the left column
    sPartyMenuBoxes[0].infoRects = &sPartyBoxInfoRects[PARTY_BOX_LEFT_COLUMN];

    if (layout == PARTY_LAYOUT_MULTI_SHOWCASE)
        sPartyMenuBoxes[3].infoRects = &sPartyBoxInfoRects[PARTY_BOX_LEFT_COLUMN];
    else if (layout != PARTY_LAYOUT_SINGLE)
        sPartyMenuBoxes[1].infoRects = &sPartyBoxInfoRects[PARTY_BOX_LEFT_COLUMN];
}

static void RenderPartyMenuBox(u8 slot)
{
    if (gPartyMenu.menuType == PARTY_MENU_TYPE_MULTI_SHOWCASE && slot >= MULTI_PARTY_SIZE)
    {
        DisplayPartyPokemonDataForMultiBattle(slot);
        if (gMultiPartnerParty[slot - MULTI_PARTY_SIZE].species == SPECIES_NONE)
            LoadPartyBoxPalette(&sPartyMenuBoxes[slot], PARTY_PAL_NO_MON);
        else
            LoadPartyBoxPalette(&sPartyMenuBoxes[slot], PARTY_PAL_MULTI_ALT);
        CopyWindowToVram(sPartyMenuBoxes[slot].windowId, COPYWIN_GFX);
        PutWindowTilemap(sPartyMenuBoxes[slot].windowId);
        ScheduleBgCopyTilemapToVram(2);
    }
    else
    {
        if (GetMonData(&gPlayerParty[slot], MON_DATA_SPECIES) == SPECIES_NONE)
        {
            DrawEmptySlot(sPartyMenuBoxes[slot].windowId);
            LoadPartyBoxPalette(&sPartyMenuBoxes[slot], PARTY_PAL_NO_MON);
            CopyWindowToVram(sPartyMenuBoxes[slot].windowId, COPYWIN_GFX);
        }
        else
        {
            if (gPartyMenu.menuType == PARTY_MENU_TYPE_MOVE_RELEARNER)
                DisplayPartyPokemonDataForRelearner(slot);
            else if (gPartyMenu.menuType == PARTY_MENU_TYPE_CONTEST)
                DisplayPartyPokemonDataForContest(slot);
            else if (gPartyMenu.menuType == PARTY_MENU_TYPE_CHOOSE_HALF)
                DisplayPartyPokemonDataForChooseHalf(slot);
            else if (gPartyMenu.menuType == PARTY_MENU_TYPE_MINIGAME)
                DisplayPartyPokemonDataForWirelessMinigame(slot);
            else if (gPartyMenu.menuType == PARTY_MENU_TYPE_STORE_PYRAMID_HELD_ITEMS)
                DisplayPartyPokemonDataForBattlePyramidHeldItem(slot);
            else if (!DisplayPartyPokemonDataForMoveTutorOrEvolutionItem(slot))
                DisplayPartyPokemonData(slot);

            if (gPartyMenu.menuType == PARTY_MENU_TYPE_MULTI_SHOWCASE)
                AnimatePartySlot(slot, 0);
            else if (gPartyMenu.slotId == slot)
                AnimatePartySlot(slot, 1);
            else
                AnimatePartySlot(slot, 0);
        }
        PutWindowTilemap(sPartyMenuBoxes[slot].windowId);
        ScheduleBgCopyTilemapToVram(0);
    }
}

static void DisplayPartyPokemonData(u8 slot)
{
    if (GetMonData(&gPlayerParty[slot], MON_DATA_IS_EGG))
    {
        sPartyMenuBoxes[slot].infoRects->blitFunc(sPartyMenuBoxes[slot].windowId, 0, 0, 0, 0, TRUE);
        DisplayPartyPokemonNickname(&gPlayerParty[slot], &sPartyMenuBoxes[slot], 0);
    }
    else
    {
        sPartyMenuBoxes[slot].infoRects->blitFunc(sPartyMenuBoxes[slot].windowId, 0, 0, 0, 0, FALSE);
        DisplayPartyPokemonNickname(&gPlayerParty[slot], &sPartyMenuBoxes[slot], 0);
        DisplayPartyPokemonLevelCheck(&gPlayerParty[slot], &sPartyMenuBoxes[slot], 0);
        DisplayPartyPokemonGenderNidoranCheck(&gPlayerParty[slot], &sPartyMenuBoxes[slot], 0);
        DisplayPartyPokemonHPCheck(&gPlayerParty[slot], &sPartyMenuBoxes[slot], 0);
        DisplayPartyPokemonMaxHPCheck(&gPlayerParty[slot], &sPartyMenuBoxes[slot], 0);
        DisplayPartyPokemonHPBarCheck(&gPlayerParty[slot], &sPartyMenuBoxes[slot]);
    }
}

static void DisplayPartyPokemonDescriptionData(u8 slot, u8 stringID)
{
    struct Pokemon *mon = &gPlayerParty[slot];

    sPartyMenuBoxes[slot].infoRects->blitFunc(sPartyMenuBoxes[slot].windowId, 0, 0, 0, 0, TRUE);
    DisplayPartyPokemonNickname(mon, &sPartyMenuBoxes[slot], 0);
    if (!GetMonData(mon, MON_DATA_IS_EGG))
    {
        DisplayPartyPokemonLevelCheck(mon, &sPartyMenuBoxes[slot], 0);
        DisplayPartyPokemonGenderNidoranCheck(mon, &sPartyMenuBoxes[slot], 0);
    }
    DisplayPartyPokemonDescriptionText(stringID, &sPartyMenuBoxes[slot], 0);
}

static void DisplayPartyPokemonDataForChooseHalf(u8 slot)
{
    u8 i;
    struct Pokemon *mon = &gPlayerParty[slot];
    u8 *order = gSelectedOrderFromParty;

    if (!GetBattleEntryEligibility(mon))
    {
        DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_NOT_ABLE);
        return;
    }
    else
    {
        for (i = 0; i < GetMaxBattleEntries(); i++)
        {
            if (order[i] != 0 && (order[i] - 1) == slot)
            {
                DisplayPartyPokemonDescriptionData(slot, i + PARTYBOX_DESC_FIRST);
                return;
            }
        }
        DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_ABLE_3);
    }
}

static void DisplayPartyPokemonDataForContest(u8 slot)
{
    switch (GetContestEntryEligibility(&gPlayerParty[slot]))
    {
    case CANT_ENTER_CONTEST:
    case CANT_ENTER_CONTEST_EGG:
    case CANT_ENTER_CONTEST_FAINTED:
        DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_NOT_ABLE);
        break;
    case CAN_ENTER_CONTEST_EQUAL_RANK:
    case CAN_ENTER_CONTEST_HIGH_RANK:
        DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_ABLE);
        break;
    }
}

static void DisplayPartyPokemonDataForRelearner(u8 slot)
{
    if (GetNumberOfRelearnableMoves(&gPlayerParty[slot]) == 0)
        DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_NOT_ABLE_2);
    else
        DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_ABLE_2);
}

static void DisplayPartyPokemonDataForWirelessMinigame(u8 slot)
{
    if (IsMonAllowedInMinigame(slot) == TRUE)
        DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_ABLE);
    else
        DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_NOT_ABLE);
}

static void DisplayPartyPokemonDataForBattlePyramidHeldItem(u8 slot)
{
    if (GetMonData(&gPlayerParty[slot], MON_DATA_HELD_ITEM))
        DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_HAVE);
    else
        DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_DONT_HAVE);
}

// Returns TRUE if teaching move or cant evolve with item (i.e. description data is shown), FALSE otherwise
static bool8 DisplayPartyPokemonDataForMoveTutorOrEvolutionItem(u8 slot)
{
    struct Pokemon *currentPokemon = &gPlayerParty[slot];
    u16 item = gSpecialVar_ItemId;

    if (gPartyMenu.action == PARTY_ACTION_MOVE_TUTOR)
    {
        gSpecialVar_Result = FALSE;
        DisplayPartyPokemonDataToTeachMove(slot, 0, gSpecialVar_0x8005);
    }
    else
    {
        if (gPartyMenu.action != PARTY_ACTION_USE_ITEM)
            return FALSE;

        switch (CheckIfItemIsTMHMOrEvolutionStone(item))
        {
        default:
            return FALSE;
        case 1: // TM/HM
            DisplayPartyPokemonDataToTeachMove(slot, item, 0);
            break;
        case 2: // Evolution stone
            if (!GetMonData(currentPokemon, MON_DATA_IS_EGG) && GetEvolutionTargetSpecies(currentPokemon, EVO_MODE_ITEM_CHECK, item) != SPECIES_NONE)
                return FALSE;
            DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_NO_USE);
            break;
        }
    }
    return TRUE;
}

static void DisplayPartyPokemonDataToTeachMove(u8 slot, u16 item, u8 tutor)
{
    switch (CanMonLearnTMTutor(&gPlayerParty[slot], item, tutor))
    {
    case CANNOT_LEARN_MOVE:
    case CANNOT_LEARN_MOVE_IS_EGG:
        DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_NOT_ABLE_2);
        break;
    case ALREADY_KNOWS_MOVE:
        DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_LEARNED);
        break;
    default:
        DisplayPartyPokemonDescriptionData(slot, PARTYBOX_DESC_ABLE_2);
        break;
    }
}

static void DisplayPartyPokemonDataForMultiBattle(u8 slot)
{
    struct PartyMenuBox *menuBox = &sPartyMenuBoxes[slot];
    u8 actualSlot = slot - MULTI_PARTY_SIZE;

    if (gMultiPartnerParty[actualSlot].species == SPECIES_NONE)
    {
        DrawEmptySlot(menuBox->windowId);
    }
    else
    {
        menuBox->infoRects->blitFunc(menuBox->windowId, 0, 0, 0, 0, FALSE);
        StringCopy(gStringVar1, gMultiPartnerParty[actualSlot].nickname);
        StringGet_Nickname(gStringVar1);
        ConvertInternationalPlayerName(gStringVar1);
        DisplayPartyPokemonBarDetail(menuBox->windowId, gStringVar1, 0, menuBox->infoRects->dimensions);
        DisplayPartyPokemonLevel(gMultiPartnerParty[actualSlot].level, menuBox);
        DisplayPartyPokemonGender(gMultiPartnerParty[actualSlot].gender, gMultiPartnerParty[actualSlot].species, gMultiPartnerParty[actualSlot].nickname, menuBox);
        DisplayPartyPokemonHP(gMultiPartnerParty[actualSlot].hp, menuBox);
        DisplayPartyPokemonMaxHP(gMultiPartnerParty[actualSlot].maxhp, menuBox);
        DisplayPartyPokemonHPBar(gMultiPartnerParty[actualSlot].hp, gMultiPartnerParty[actualSlot].maxhp, menuBox);
    }
}

static bool8 RenderPartyMenuBoxes(void)
{
    RenderPartyMenuBox(sPartyMenuInternal->data[0]);
    if (++sPartyMenuInternal->data[0] == PARTY_SIZE)
        return TRUE;
    else
        return FALSE;
}

static u8 *GetPartyMenuBgTile(u16 tileId)
{
    return &sPartyBgGfxTilemap[tileId << 5];
}

static void CreatePartyMonSprites(u8 slot)
{
    u8 actualSlot;

    if (gPartyMenu.menuType == PARTY_MENU_TYPE_MULTI_SHOWCASE && slot >= MULTI_PARTY_SIZE)
    {
        u8 status;
        actualSlot = slot - MULTI_PARTY_SIZE;

        if (gMultiPartnerParty[actualSlot].species != SPECIES_NONE)
        {
            CreatePartyMonIconSpriteParameterized(gMultiPartnerParty[actualSlot].species, gMultiPartnerParty[actualSlot].personality, &sPartyMenuBoxes[slot], 0, FALSE);
            CreatePartyMonHeldItemSpriteParameterized(gMultiPartnerParty[actualSlot].species, gMultiPartnerParty[actualSlot].heldItem, &sPartyMenuBoxes[slot]);
            CreatePartyMonPokeballSpriteParameterized(gMultiPartnerParty[actualSlot].species, &sPartyMenuBoxes[slot]);
            if (gMultiPartnerParty[actualSlot].hp == 0)
                status = AILMENT_FNT;
            else
                status = GetAilmentFromStatus(gMultiPartnerParty[actualSlot].status);
            CreatePartyMonStatusSpriteParameterized(gMultiPartnerParty[actualSlot].species, status, &sPartyMenuBoxes[slot]);
        }
    }
    else if (GetMonData(&gPlayerParty[slot], MON_DATA_SPECIES) != SPECIES_NONE)
    {
        CreatePartyMonIconSprite(&gPlayerParty[slot], &sPartyMenuBoxes[slot], slot);
        CreatePartyMonHeldItemSprite(&gPlayerParty[slot], &sPartyMenuBoxes[slot]);
        CreatePartyMonPokeballSprite(&gPlayerParty[slot], &sPartyMenuBoxes[slot]);
        CreatePartyMonStatusSprite(&gPlayerParty[slot], &sPartyMenuBoxes[slot]);
    }
}

static bool8 CreatePartyMonSpritesLoop(void)
{
    CreatePartyMonSprites(sPartyMenuInternal->data[0]);
    if (++sPartyMenuInternal->data[0] == PARTY_SIZE)
        return TRUE;
    else
        return FALSE;
}

static void CreateCancelConfirmPokeballSprites(void)
{
    if (gPartyMenu.menuType == PARTY_MENU_TYPE_MULTI_SHOWCASE)
    {
        // The showcase has no Cancel/Confirm buttons
        FillBgTilemapBufferRect(1, 14, 23, 17, 7, 2, 1);
    }
    else
    {
        if (sPartyMenuInternal->chooseHalf)
        {
            sPartyMenuInternal->spriteIdConfirmPokeball = CreateSmallPokeballButtonSprite(0xBF, 0x88);
            DrawCancelConfirmButtons();
            sPartyMenuInternal->spriteIdCancelPokeball = CreateSmallPokeballButtonSprite(0xBF, 0x98);
        }
        else
        {
            sPartyMenuInternal->spriteIdCancelPokeball = CreatePokeballButtonSprite(198, 148);
        }
        AnimatePartySlot(gPartyMenu.slotId, 1);
    }
}

void AnimatePartySlot(u8 slot, u8 animNum)
{
    u8 spriteId;

    switch (slot)
    {
    default:
        if (GetMonData(&gPlayerParty[slot], MON_DATA_SPECIES) != SPECIES_NONE)
        {
            LoadPartyBoxPalette(&sPartyMenuBoxes[slot], GetPartyBoxPaletteFlags(slot, animNum));
            AnimateSelectedPartyIcon(sPartyMenuBoxes[slot].monSpriteId, animNum);
            PartyMenuStartSpriteAnim(sPartyMenuBoxes[slot].pokeballSpriteId, animNum);
        }
        return;
    case PARTY_SIZE: // Confirm
        if (animNum == 0)
            SetBgTilemapPalette(1, 23, 16, 7, 2, 1);
        else
            SetBgTilemapPalette(1, 23, 16, 7, 2, 2);
        spriteId = sPartyMenuInternal->spriteIdConfirmPokeball;
        break;
    case PARTY_SIZE + 1: // Cancel
        // The position of the Cancel button changes if Confirm is present
        if (!sPartyMenuInternal->chooseHalf)
        {
            if (animNum == 0)
                SetBgTilemapPalette(1, 23, 17, 7, 2, 1);
            else
                SetBgTilemapPalette(1, 23, 17, 7, 2, 2);
        }
        else if (animNum == 0)
        {
            SetBgTilemapPalette(1, 23, 18, 7, 2, 1);
        }
        else
        {
            SetBgTilemapPalette(1, 23, 18, 7, 2, 2);
        }
        spriteId = sPartyMenuInternal->spriteIdCancelPokeball;
        break;
    }
    PartyMenuStartSpriteAnim(spriteId, animNum);
    ScheduleBgCopyTilemapToVram(1);
}

static u8 GetPartyBoxPaletteFlags(u8 slot, u8 animNum)
{
    u8 palFlags = 0;

    if (animNum == 1)
        palFlags |= PARTY_PAL_SELECTED;
    if (GetMonData(&gPlayerParty[slot], MON_DATA_HP) == 0)
        palFlags |= PARTY_PAL_FAINTED;
    if (PartyBoxPal_ParnterOrDisqualifiedInArena(slot) == TRUE)
        palFlags |= PARTY_PAL_MULTI_ALT;
    if (gPartyMenu.action == PARTY_ACTION_SWITCHING)
        palFlags |= PARTY_PAL_SWITCHING;
    if (gPartyMenu.action == PARTY_ACTION_SWITCH)
    {
        if (slot == gPartyMenu.slotId || slot == gPartyMenu.slotId2)
            palFlags |= PARTY_PAL_TO_SWITCH;
    }
    if (gPartyMenu.action == PARTY_ACTION_SOFTBOILED && slot == gPartyMenu.slotId )
        palFlags |= PARTY_PAL_TO_SOFTBOIL;

    return palFlags;
}

static bool8 PartyBoxPal_ParnterOrDisqualifiedInArena(u8 slot)
{
    if (gPartyMenu.layout == PARTY_LAYOUT_MULTI && (slot == 1 || slot == 4 || slot == 5))
        return TRUE;

    if (slot < MULTI_PARTY_SIZE && (gBattleTypeFlags & BATTLE_TYPE_ARENA) && gMain.inBattle && (gBattleStruct->arenaLostPlayerMons >> GetPartyIdFromBattleSlot(slot) & 1))
        return TRUE;

    return FALSE;
}

static void DrawCancelConfirmButtons(void)
{
    CopyToBgTilemapBufferRect_ChangePalette(1, sConfirmButton_Tilemap, 23, 16, 7, 2, 17);
    CopyToBgTilemapBufferRect_ChangePalette(1, sCancelButton_Tilemap, 23, 18, 7, 2, 17);
    ScheduleBgCopyTilemapToVram(1);
}

bool8 IsMultiBattle(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_MULTI && gBattleTypeFlags & BATTLE_TYPE_DOUBLE && gBattleTypeFlags & BATTLE_TYPE_TRAINER && gMain.inBattle)
        return TRUE;
    else
        return FALSE;
}

static void SwapPartyPokemon(struct Pokemon *mon1, struct Pokemon *mon2)
{
    struct Pokemon *temp = Alloc(sizeof(struct Pokemon));

    *temp = *mon1;
    *mon1 = *mon2;
    *mon2 = *temp;

    Free(temp);
}

static void Task_ClosePartyMenu(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_ClosePartyMenuAndSetCB2;
}

static void Task_ClosePartyMenuAndSetCB2(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        if (gPartyMenu.menuType == PARTY_MENU_TYPE_IN_BATTLE)
            UpdatePartyToFieldOrder();

        if (sPartyMenuInternal->exitCallback != NULL)
            SetMainCallback2(sPartyMenuInternal->exitCallback);
        else
            SetMainCallback2(gPartyMenu.exitCallback);

        ResetSpriteData();
        FreePartyPointers();
        DestroyTask(taskId);
    }
}

u8 GetCursorSelectionMonId(void)
{
    return gPartyMenu.slotId;
}

u8 GetPartyMenuType(void)
{
    return gPartyMenu.menuType;
}

void Task_HandleChooseMonInput(u8 taskId)
{
    if (!gPaletteFade.active && MenuHelpers_ShouldWaitForLinkRecv() != TRUE)
    {
        s8 *slotPtr = GetCurrentPartySlotPtr();

        switch (PartyMenuButtonHandler(slotPtr))
        {
        case A_BUTTON: // Selected mon
            HandleChooseMonSelection(taskId, slotPtr);
            break;
        case B_BUTTON: // Selected Cancel / pressed B
            HandleChooseMonCancel(taskId, slotPtr);
            break;
        case START_BUTTON:
            if (sPartyMenuInternal->chooseHalf)
            {
                PlaySE(SE_SELECT);
                MoveCursorToConfirm();
            }
            break;
        }
    }
}

static s8 *GetCurrentPartySlotPtr(void)
{
    if (gPartyMenu.action == PARTY_ACTION_SWITCH || gPartyMenu.action == PARTY_ACTION_SOFTBOILED)
        return &gPartyMenu.slotId2;
    else
        return &gPartyMenu.slotId;
}

static void HandleChooseMonSelection(u8 taskId, s8 *slotPtr)
{
    if (*slotPtr == PARTY_SIZE)
    {
        gPartyMenu.task(taskId);
    }
    else
    {
        switch (gPartyMenu.action)
        {
        case PARTY_ACTION_SOFTBOILED:
            if (IsSelectedMonNotEgg((u8 *)slotPtr))
            {
                PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
                Task_TryUseSoftboiledOnPartyMon(taskId);
            }
            break;
        case PARTY_ACTION_USE_ITEM:
            if (IsSelectedMonNotEgg((u8 *)slotPtr))
            {
                if (gPartyMenu.menuType == PARTY_MENU_TYPE_IN_BATTLE)
                    sPartyMenuInternal->exitCallback = CB2_SetUpExitToBattleScreen;

                PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
                gItemUseCB(taskId, Task_ClosePartyMenuAfterText);
            }
            break;
        case PARTY_ACTION_MOVE_TUTOR:
            if (IsSelectedMonNotEgg((u8 *)slotPtr))
            {
                PlaySE(SE_SELECT);
                PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
                TryTutorSelectedMon(taskId);
            }
            break;
        case PARTY_ACTION_GIVE_MAILBOX_MAIL:
            if (IsSelectedMonNotEgg((u8 *)slotPtr))
            {
                PlaySE(SE_SELECT);
                PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
                TryGiveMailToSelectedMon(taskId);
            }
            break;
        case PARTY_ACTION_GIVE_ITEM:
        case PARTY_ACTION_GIVE_PC_ITEM:
            if (IsSelectedMonNotEgg((u8 *)slotPtr))
            {
                PlaySE(SE_SELECT);
                PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
                TryGiveItemOrMailToSelectedMon(taskId);
            }
            break;
        case PARTY_ACTION_SWITCH:
            PlaySE(SE_SELECT);
            SwitchSelectedMons(taskId);
            break;
        case PARTY_ACTION_CHOOSE_AND_CLOSE:
            PlaySE(SE_SELECT);
            Task_ClosePartyMenu(taskId);
            break;
        case PARTY_ACTION_MINIGAME:
            if (IsSelectedMonNotEgg((u8 *)slotPtr))
            {
                TryEnterMonForMinigame(taskId, (u8)*slotPtr);
            }
            break;
        default:
        case PARTY_ACTION_ABILITY_PREVENTS:
        case PARTY_ACTION_SWITCHING:
            PlaySE(SE_SELECT);
            Task_TryCreateSelectionWindow(taskId);
            break;
        }
    }
}

static bool8 IsSelectedMonNotEgg(u8 *slotPtr)
{
    if (GetMonData(&gPlayerParty[*slotPtr], MON_DATA_IS_EGG) == TRUE)
    {
        PlaySE(SE_FAILURE);
        return FALSE;
    }
    return TRUE;
}

static void HandleChooseMonCancel(u8 taskId, s8 *slotPtr)
{
    switch (gPartyMenu.action)
    {
    case PARTY_ACTION_SEND_OUT:
        PlaySE(SE_FAILURE);
        break;
    case PARTY_ACTION_SWITCH:
    case PARTY_ACTION_SOFTBOILED:
        PlaySE(SE_SELECT);
        FinishTwoMonAction(taskId);
        break;
    case PARTY_ACTION_MINIGAME:
        PlaySE(SE_SELECT);
        CancelParticipationPrompt(taskId);
        break;
    default:
        PlaySE(SE_SELECT);
        if (DisplayCancelChooseMonYesNo(taskId) != TRUE)
        {
            if (!MenuHelpers_IsLinkActive())
                gSpecialVar_0x8004 = PARTY_SIZE + 1;
            gPartyMenuUseExitCallback = FALSE;
            *slotPtr = PARTY_SIZE + 1;
            Task_ClosePartyMenu(taskId);
        }
        break;
    }
}

static bool8 DisplayCancelChooseMonYesNo(u8 taskId)
{
    const u8 *stringPtr = NULL;

    if (gPartyMenu.menuType == PARTY_MENU_TYPE_CONTEST)
        stringPtr = gText_CancelParticipation;
    else if (gPartyMenu.menuType == PARTY_MENU_TYPE_CHOOSE_HALF)
        stringPtr = GetFacilityCancelString();

    if (stringPtr == NULL)
        return FALSE;

    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    StringExpandPlaceholders(gStringVar4, stringPtr);
    DisplayPartyMenuMessage(gStringVar4, TRUE);
    gTasks[taskId].func = Task_CancelChooseMonYesNo;
    return TRUE;
}

static void Task_CancelChooseMonYesNo(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        PartyMenuDisplayYesNoMenu();
        gTasks[taskId].func = Task_HandleCancelChooseMonYesNoInput;
    }
}

static void Task_HandleCancelChooseMonYesNoInput(u8 taskId)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0:
        gPartyMenuUseExitCallback = FALSE;
        gPartyMenu.slotId = PARTY_SIZE + 1;
        ClearSelectedPartyOrder();
        Task_ClosePartyMenu(taskId);
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        // fallthrough
    case 1:
        Task_ReturnToChooseMonAfterText(taskId);
        break;
    }
}

static u16 PartyMenuButtonHandler(s8 *slotPtr)
{
    s8 movementDir;

    switch (gMain.newAndRepeatedKeys)
    {
    case DPAD_UP:
        movementDir = MENU_DIR_UP;
        break;
    case DPAD_DOWN:
        movementDir = MENU_DIR_DOWN;
        break;
    case DPAD_LEFT:
        movementDir = MENU_DIR_LEFT;
        break;
    case DPAD_RIGHT:
        movementDir = MENU_DIR_RIGHT;
        break;
    default:
        switch (GetLRKeysPressedAndHeld())
        {
        case MENU_L_PRESSED:
            movementDir = MENU_DIR_UP;
            break;
        case MENU_R_PRESSED:
            movementDir = MENU_DIR_DOWN;
            break;
        default:
            movementDir = 0;
            break;
        }
        break;
    }

    if (JOY_NEW(START_BUTTON))
        return START_BUTTON;

    if (movementDir)
    {
        UpdateCurrentPartySelection(slotPtr, movementDir);
        return 0;
    }

    // Pressed Cancel
    if (JOY_NEW(A_BUTTON) && *slotPtr == PARTY_SIZE + 1)
        return B_BUTTON;

    return JOY_NEW(A_BUTTON | B_BUTTON);
}

static void UpdateCurrentPartySelection(s8 *slotPtr, s8 movementDir)
{
    s8 newSlotId = *slotPtr;
    u8 layout = gPartyMenu.layout;

    if (layout == PARTY_LAYOUT_SINGLE)
        UpdatePartySelectionSingleLayout(slotPtr, movementDir);
    else
        UpdatePartySelectionDoubleLayout(slotPtr, movementDir);

    if (*slotPtr != newSlotId)
    {
        PlaySE(SE_SELECT);
        AnimatePartySlot(newSlotId, 0);
        AnimatePartySlot(*slotPtr, 1);
    }
}

static void UpdatePartySelectionSingleLayout(s8 *slotPtr, s8 movementDir)
{
    // PARTY_SIZE + 1 is Cancel, PARTY_SIZE is Confirm
    switch (movementDir)
    {
    case MENU_DIR_UP:
        if (*slotPtr == 0)
        {
            *slotPtr = PARTY_SIZE + 1;
        }
        else if (*slotPtr == PARTY_SIZE)
        {
            *slotPtr = gPlayerPartyCount - 1;
        }
        else if (*slotPtr == PARTY_SIZE + 1)
        {
            if (sPartyMenuInternal->chooseHalf)
                *slotPtr = PARTY_SIZE;
            else
                *slotPtr = gPlayerPartyCount - 1;
        }
        else
        {
            (*slotPtr)--;
        }
        break;
    case MENU_DIR_DOWN:
        if (*slotPtr == PARTY_SIZE + 1)
        {
            *slotPtr = 0;
        }
        else
        {
            if (*slotPtr == gPlayerPartyCount - 1)
            {
                if (sPartyMenuInternal->chooseHalf)
                    *slotPtr = PARTY_SIZE;
                else
                    *slotPtr = PARTY_SIZE + 1;
            }
            else
            {
                (*slotPtr)++;
            }
        }
        break;
    case MENU_DIR_RIGHT:
        if (gPlayerPartyCount != 1 && *slotPtr == 0)
        {
            if (sPartyMenuInternal->lastSelectedSlot == 0)
                *slotPtr = 1;
            else
                *slotPtr = sPartyMenuInternal->lastSelectedSlot;
        }
        break;
    case MENU_DIR_LEFT:
        if (*slotPtr != 0 && *slotPtr != PARTY_SIZE && *slotPtr != PARTY_SIZE + 1)
        {
            sPartyMenuInternal->lastSelectedSlot = *slotPtr;
            *slotPtr = 0;
        }
        break;
    }
}

static void UpdatePartySelectionDoubleLayout(s8 *slotPtr, s8 movementDir)
{
    // PARTY_SIZE + 1 is Cancel, PARTY_SIZE is Confirm
    // newSlot is used temporarily as a movement direction during its later assignment
    s8 newSlot = movementDir;

    switch (movementDir)
    {
    case MENU_DIR_UP:
        if (*slotPtr == 0)
        {
            *slotPtr = PARTY_SIZE + 1;
            break;
        }
        else if (*slotPtr == PARTY_SIZE)
        {
            *slotPtr = gPlayerPartyCount - 1;
            break;
        }
        else if (*slotPtr == PARTY_SIZE + 1)
        {
            if (sPartyMenuInternal->chooseHalf)
            {
                *slotPtr = PARTY_SIZE;
                break;
            }
            (*slotPtr)--;
        }
        newSlot = GetNewSlotDoubleLayout(*slotPtr, newSlot);
        if (newSlot != -1)
            *slotPtr = newSlot;
        break;
    case MENU_DIR_DOWN:
        if (*slotPtr == PARTY_SIZE)
        {
            *slotPtr = PARTY_SIZE + 1;
        }
        else if (*slotPtr == PARTY_SIZE + 1)
        {
            *slotPtr = 0;
        }
        else
        {
            newSlot = GetNewSlotDoubleLayout(*slotPtr, MENU_DIR_DOWN);
            if (newSlot == -1)
            {
                if (sPartyMenuInternal->chooseHalf)
                    *slotPtr = PARTY_SIZE;
                else
                    *slotPtr = PARTY_SIZE + 1;
            }
            else
            {
                *slotPtr = newSlot;
            }
        }
        break;
    case MENU_DIR_RIGHT:
        if (*slotPtr == 0)
        {
            if (sPartyMenuInternal->lastSelectedSlot == 3)
            {
                if (GetMonData(&gPlayerParty[3], MON_DATA_SPECIES) != SPECIES_NONE)
                    *slotPtr = 3;
            }
            else if (GetMonData(&gPlayerParty[2], MON_DATA_SPECIES) != SPECIES_NONE)
            {
                *slotPtr = 2;
            }
        }
        else if (*slotPtr == 1)
        {
            if (sPartyMenuInternal->lastSelectedSlot == 5)
            {
                if (GetMonData(&gPlayerParty[5], MON_DATA_SPECIES) != SPECIES_NONE)
                    *slotPtr = 5;
            }
            else if (GetMonData(&gPlayerParty[4], MON_DATA_SPECIES) != SPECIES_NONE)
            {
                *slotPtr = 4;
            }
        }
        break;
    case MENU_DIR_LEFT:
        if (*slotPtr == 2 || *slotPtr == 3)
        {
            sPartyMenuInternal->lastSelectedSlot = *slotPtr;
            *slotPtr = 0;
        }
        else if (*slotPtr == 4 || *slotPtr == 5)
        {
            sPartyMenuInternal->lastSelectedSlot = *slotPtr;
            *slotPtr = 1;
        }
        break;
    }
}

static s8 GetNewSlotDoubleLayout(s8 slotId, s8 movementDir)
{
    while (TRUE)
    {
        slotId += movementDir;
        if ((u8)slotId >= PARTY_SIZE)
            return -1;
        if (GetMonData(&gPlayerParty[slotId], MON_DATA_SPECIES) != SPECIES_NONE)
            return slotId;
    }
}

u8 *GetMonNickname(struct Pokemon *mon, u8 *dest)
{
    GetMonData(mon, MON_DATA_NICKNAME, dest);
    return StringGet_Nickname(dest);
}

#define tKeepOpen  data[0]

u8 DisplayPartyMenuMessage(const u8 *str, bool8 keepOpen)
{
    u8 taskId;

    PrintMessage(str);
    taskId = CreateTask(Task_PrintAndWaitForText, 1);
    gTasks[taskId].tKeepOpen = keepOpen;
    return taskId;
}

static void Task_PrintAndWaitForText(u8 taskId)
{
    if (RunTextPrintersRetIsActive(WIN_MSG) != TRUE)
    {
        if (gTasks[taskId].tKeepOpen == FALSE)
        {
            ClearStdWindowAndFrameToTransparent(WIN_MSG, FALSE);
            ClearWindowTilemap(WIN_MSG);
        }
        DestroyTask(taskId);
    }
}

#undef tKeepOpen

bool8 IsPartyMenuTextPrinterActive(void)
{
    return FuncIsActiveTask(Task_PrintAndWaitForText);
}

static void Task_WaitForLinkAndReturnToChooseMon(u8 taskId)
{
    if (MenuHelpers_ShouldWaitForLinkRecv() != TRUE)
    {
        DisplayPartyMenuStdMessage(PARTY_MSG_CHOOSE_MON);
        gTasks[taskId].func = Task_HandleChooseMonInput;
    }
}

static void Task_ReturnToChooseMonAfterText(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        ClearStdWindowAndFrameToTransparent(WIN_MSG, FALSE);
        ClearWindowTilemap(WIN_MSG);
        if (MenuHelpers_IsLinkActive() == TRUE)
        {
            gTasks[taskId].func = Task_WaitForLinkAndReturnToChooseMon;
        }
        else
        {
            DisplayPartyMenuStdMessage(PARTY_MSG_CHOOSE_MON);
            gTasks[taskId].func = Task_HandleChooseMonInput;
        }
    }
}

static void DisplayGaveHeldItemMessage(struct Pokemon *mon, u16 item, bool8 keepOpen, u8 unused)
{
    GetMonNickname(mon, gStringVar1);
    CopyItemName(item, gStringVar2);
    StringExpandPlaceholders(gStringVar4, gText_PkmnWasGivenItem);
    DisplayPartyMenuMessage(gStringVar4, keepOpen);
    ScheduleBgCopyTilemapToVram(2);
}

static void DisplayTookHeldItemMessage(struct Pokemon *mon, u16 item, bool8 keepOpen)
{
    GetMonNickname(mon, gStringVar1);
    CopyItemName(item, gStringVar2);
    StringExpandPlaceholders(gStringVar4, gText_ReceivedItemFromPkmn);
    DisplayPartyMenuMessage(gStringVar4, keepOpen);
    ScheduleBgCopyTilemapToVram(2);
}

static void DisplayAlreadyHoldingItemSwitchMessage(struct Pokemon *mon, u16 item, bool8 keepOpen)
{
    GetMonNickname(mon, gStringVar1);
    CopyItemName(item, gStringVar2);
    StringExpandPlaceholders(gStringVar4, gText_PkmnAlreadyHoldingItemSwitch);
    DisplayPartyMenuMessage(gStringVar4, keepOpen);
    ScheduleBgCopyTilemapToVram(2);
}

static void DisplaySwitchedHeldItemMessage(u16 item, u16 item2, bool8 keepOpen)
{
    CopyItemName(item, gStringVar1);
    CopyItemName(item2, gStringVar2);
    StringExpandPlaceholders(gStringVar4, gText_SwitchedPkmnItem);
    DisplayPartyMenuMessage(gStringVar4, keepOpen);
    ScheduleBgCopyTilemapToVram(2);
}

static void GiveItemToMon(struct Pokemon *mon, u16 item)
{
    u8 itemBytes[2];

    if (ItemIsMail(item) == TRUE)
    {
        if (GiveMailToMonByItemId(mon, item) == MAIL_NONE)
            return;
    }
    itemBytes[0] = item;
    itemBytes[1] = item >> 8;
    SetMonData(mon, MON_DATA_HELD_ITEM, itemBytes);
}

static u8 TryTakeMonItem(struct Pokemon *mon)
{
    u16 item = GetMonData(mon, MON_DATA_HELD_ITEM);

    if (item == ITEM_NONE)
        return 0;
    if (AddBagItem(item, 1) == FALSE)
        return 1;

    item = ITEM_NONE;
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
    return 2;
}

static void BufferBagFullCantTakeItemMessage(u16 itemUnused)
{
    StringExpandPlaceholders(gStringVar4, gText_BagFullCouldNotRemoveItem);
}

#define tHP           data[0]
#define tMaxHP        data[1]
#define tHPIncrement  data[2]
#define tHPToAdd      data[3]
#define tPartyId      data[4]
#define tStartHP      data[5]

static void Task_PartyMenuModifyHP(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    tHP += tHPIncrement;
    tHPToAdd--;
    SetMonData(&gPlayerParty[tPartyId], MON_DATA_HP, &tHP);
    DisplayPartyPokemonHPCheck(&gPlayerParty[tPartyId], &sPartyMenuBoxes[tPartyId], 1);
    DisplayPartyPokemonHPBarCheck(&gPlayerParty[tPartyId], &sPartyMenuBoxes[tPartyId]);
    if (tHPToAdd == 0 || tHP == 0 || tHP == tMaxHP)
    {
        // If HP was recovered, buffer the amount recovered
        if (tHP > tStartHP)
            ConvertIntToDecimalStringN(gStringVar2, tHP - tStartHP, STR_CONV_MODE_LEFT_ALIGN, 3);

        SwitchTaskToFollowupFunc(taskId);
    }
}

void PartyMenuModifyHP(u8 taskId, u8 slot, s8 hpIncrement, s16 hpDifference, TaskFunc task)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    s16 *data = gTasks[taskId].data;

    tHP = GetMonData(mon, MON_DATA_HP);
    tMaxHP = GetMonData(mon, MON_DATA_MAX_HP);
    tHPIncrement = hpIncrement;
    tHPToAdd = hpDifference;
    tPartyId = slot;
    tStartHP = tHP;
    SetTaskFuncWithFollowupFunc(taskId, Task_PartyMenuModifyHP, task);
}

// The usage of hp in this function is mostly nonsense
// Because caseId is always passed 0, none of the other cases ever occur
static void ResetHPTaskData(u8 taskId, u8 caseId, u32 hp)
{
    s16 *data = gTasks[taskId].data;

    switch (caseId) // always zero
    {
        case 0:
            tHP = hp;
            tStartHP = hp;
            break;
        case 1:
            tMaxHP = hp;
            break;
        case 2:
            tHPIncrement = hp;
            break;
        case 3:
            tHPToAdd = hp;
            break;
        case 4:
            tPartyId = hp;
            break;
        case 5:
            SetTaskFuncWithFollowupFunc(taskId, Task_PartyMenuModifyHP, (TaskFunc)hp); // >casting hp as a taskfunc
            break;
    }
}

#undef tHP
#undef tMaxHP
#undef tHPIncrement
#undef tHPToAdd
#undef tPartyId
#undef tStartHP

u8 GetAilmentFromStatus(u32 status)
{
    if (status & STATUS1_PSN_ANY)
        return AILMENT_PSN;
    if (status & STATUS1_PARALYSIS)
        return AILMENT_PRZ;
    if (status & STATUS1_SLEEP)
        return AILMENT_SLP;
    if (status & STATUS1_FREEZE)
        return AILMENT_FRZ;
    if (status & STATUS1_BURN)
        return AILMENT_BRN;
    return AILMENT_NONE;
}

u8 GetMonAilment(struct Pokemon *mon)
{
    u8 ailment;

    if (GetMonData(mon, MON_DATA_HP) == 0)
        return AILMENT_FNT;
    ailment = GetAilmentFromStatus(GetMonData(mon, MON_DATA_STATUS));
    if (ailment != AILMENT_NONE)
        return ailment;
    if (CheckPartyPokerus(mon, 0))
        return AILMENT_PKRS;
    return AILMENT_NONE;
}

static void SetPartyMonsAllowedInMinigame(void)
{
    s16 *ptr;

    if (gPartyMenu.menuType == PARTY_MENU_TYPE_MINIGAME)
    {
        u8 i;

        ptr = &gPartyMenu.data1;
        gPartyMenu.data1 = 0;
        if (gSpecialVar_0x8005 == 0)
        {
            for (i = 0; i < gPlayerPartyCount; i++)
                *ptr += IsMonAllowedInPokemonJump(&gPlayerParty[i]) << i;
        }
        else
        {
            for (i = 0; i < gPlayerPartyCount; i++)
                *ptr += IsMonAllowedInDodrioBerryPicking(&gPlayerParty[i]) << i;
        }
    }
}

static bool16 IsMonAllowedInPokemonJump(struct Pokemon *mon)
{
    if (GetMonData(mon, MON_DATA_IS_EGG) != TRUE && IsSpeciesAllowedInPokemonJump(GetMonData(mon, MON_DATA_SPECIES)))
        return TRUE;
    return FALSE;
}


static bool16 IsMonAllowedInDodrioBerryPicking(struct Pokemon *mon)
{
    if (GetMonData(mon, MON_DATA_IS_EGG) != TRUE && GetMonData(mon, MON_DATA_SPECIES) == SPECIES_DODRIO)
        return TRUE;
    return FALSE;
}

static bool8 IsMonAllowedInMinigame(u8 slot)
{
    if (!((gPartyMenu.data1 >> slot) & 1))
        return FALSE;
    return TRUE;
}

static void TryEnterMonForMinigame(u8 taskId, u8 slot)
{
    if (IsMonAllowedInMinigame(slot) == TRUE)
    {
        PlaySE(SE_SELECT);
        gSpecialVar_0x8004 = slot;
        Task_ClosePartyMenu(taskId);
    }
    else
    {
        PlaySE(SE_FAILURE);
        DisplayPartyMenuMessage(gText_PkmnCantParticipate, FALSE);
        ScheduleBgCopyTilemapToVram(2);
        gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
    }
}

static void CancelParticipationPrompt(u8 taskId)
{
    DisplayPartyMenuMessage(gText_CancelParticipation, TRUE);
    ScheduleBgCopyTilemapToVram(2);
    gTasks[taskId].func = Task_CancelParticipationYesNo;
}

static void Task_CancelParticipationYesNo(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        PartyMenuDisplayYesNoMenu();
        gTasks[taskId].func = Task_HandleCancelParticipationYesNoInput;
    }
}

static void Task_HandleCancelParticipationYesNoInput(u8 taskId)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0:
        gSpecialVar_0x8004 = PARTY_SIZE + 1;
        Task_ClosePartyMenu(taskId);
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        // fallthrough
    case 1:
        gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
        break;
    }
}

static u8 CanMonLearnTMTutor(struct Pokemon *mon, u16 item, u8 tutor)
{
    u16 move;

    if (GetMonData(mon, MON_DATA_IS_EGG))
        return CANNOT_LEARN_MOVE_IS_EGG;

    if (item >= ITEM_TM01)
    {
        if (!CanMonLearnTMHM(mon, item - ITEM_TM01))
            return CANNOT_LEARN_MOVE;
        else
            move = ItemIdToBattleMoveId(item);
    }
    else
    {
        if (!CanLearnTutorMove(GetMonData(mon, MON_DATA_SPECIES), tutor))
            return CANNOT_LEARN_MOVE;
        else
            move = GetTutorMove(tutor);
    }

    if (MonKnowsMove(mon, move) == TRUE)
        return ALREADY_KNOWS_MOVE;
    else
        return CAN_LEARN_MOVE;
}

static u16 GetTutorMove(u8 tutor)
{
    return gTutorMoves[tutor];
}

static bool8 CanLearnTutorMove(u16 species, u8 tutor)
{
    if (sTutorLearnsets[species] & (1 << tutor))
        return TRUE;
    else
        return FALSE;
}

static void InitPartyMenuWindows(u8 layout)
{
    u8 i;

    switch (layout)
    {
    case PARTY_LAYOUT_SINGLE:
        InitWindows(sSinglePartyMenuWindowTemplate);
        break;
    case PARTY_LAYOUT_DOUBLE:
        InitWindows(sDoublePartyMenuWindowTemplate);
        break;
    case PARTY_LAYOUT_MULTI:
        InitWindows(sMultiPartyMenuWindowTemplate);
        break;
    default: // PARTY_LAYOUT_MULTI_SHOWCASE
        InitWindows(sShowcaseMultiPartyMenuWindowTemplate);
        break;
    }
    DeactivateAllTextPrinters();
    for (i = 0; i < PARTY_SIZE; i++)
        FillWindowPixelBuffer(i, PIXEL_FILL(0));
    LoadUserWindowBorderGfx(0, 0x4F, BG_PLTT_ID(13));
    LoadPalette(GetOverworldTextboxPalettePtr(), BG_PLTT_ID(14), PLTT_SIZE_4BPP);
    LoadPalette(gStandardMenuPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
}

static void CreateCancelConfirmWindows(bool8 chooseHalf)
{
    u8 confirmWindowId;
    u8 cancelWindowId;
    u8 offset;
    u8 mainOffset;

    if (gPartyMenu.menuType != PARTY_MENU_TYPE_MULTI_SHOWCASE)
    {
        if (chooseHalf == TRUE)
        {
            confirmWindowId = AddWindow(&sConfirmButtonWindowTemplate);
            FillWindowPixelBuffer(confirmWindowId, PIXEL_FILL(0));
            mainOffset = GetStringCenterAlignXOffset(FONT_SMALL, gMenuText_Confirm, 48);
            AddTextPrinterParameterized4(confirmWindowId, FONT_SMALL, mainOffset, 1, 0, 0, sFontColorTable[0], TEXT_SKIP_DRAW, gMenuText_Confirm);
            PutWindowTilemap(confirmWindowId);
            CopyWindowToVram(confirmWindowId, COPYWIN_GFX);
            cancelWindowId = AddWindow(&sMultiCancelButtonWindowTemplate);
            offset = 0;
        }
        else
        {
            cancelWindowId = AddWindow(&sCancelButtonWindowTemplate);
            offset = 3;
        }
        FillWindowPixelBuffer(cancelWindowId, PIXEL_FILL(0));

        // Branches are functionally identical. Second branch is never reached, Spin Trade wasnt fully implemented
        if (gPartyMenu.menuType != PARTY_MENU_TYPE_SPIN_TRADE)
        {
            mainOffset = GetStringCenterAlignXOffset(FONT_SMALL, gText_Cancel, 48);
            AddTextPrinterParameterized3(cancelWindowId, FONT_SMALL, mainOffset + offset, 1, sFontColorTable[0], TEXT_SKIP_DRAW, gText_Cancel);
        }
        else
        {
            mainOffset = GetStringCenterAlignXOffset(FONT_SMALL, gText_Cancel2, 48);
            AddTextPrinterParameterized3(cancelWindowId, FONT_SMALL, mainOffset + offset, 1, sFontColorTable[0], TEXT_SKIP_DRAW, gText_Cancel2);
        }
        PutWindowTilemap(cancelWindowId);
        CopyWindowToVram(cancelWindowId, COPYWIN_GFX);
        ScheduleBgCopyTilemapToVram(0);
    }
}

static u16 *GetPartyMenuPalBufferPtr(u8 paletteId)
{
    return &sPartyMenuInternal->palBuffer[paletteId];
}

static void BlitBitmapToPartyWindow(u8 windowId, const u8 *b, u8 c, u8 x, u8 y, u8 width, u8 height)
{
    u8 *pixels = AllocZeroed(height * width * 32);
    u8 i, j;

    if (pixels != NULL)
    {
        for (i = 0; i < height; i++)
        {
            for (j = 0; j < width; j++)
                CpuCopy16(GetPartyMenuBgTile(b[x + j + ((y + i) * c)]), &pixels[(i * width + j) * 32], 32);
        }
        BlitBitmapToWindow(windowId, pixels, x * 8, y * 8, width * 8, height * 8);
        Free(pixels);
    }
}

static void BlitBitmapToPartyWindow_LeftColumn(u8 windowId, u8 x, u8 y, u8 width, u8 height, bool8 hideHP)
{
    if (width == 0 && height == 0)
    {
        width = 10;
        height = 7;
    }
    if (hideHP == FALSE)
        BlitBitmapToPartyWindow(windowId, sSlotTilemap_Main, 10, x, y, width, height);
    else
        BlitBitmapToPartyWindow(windowId, sSlotTilemap_MainNoHP, 10, x, y, width, height);
}

static void BlitBitmapToPartyWindow_RightColumn(u8 windowId, u8 x, u8 y, u8 width, u8 height, bool8 hideHP)
{
    if (width == 0 && height == 0)
    {
        width = 18;
        height = 3;
    }
    if (hideHP == FALSE)
        BlitBitmapToPartyWindow(windowId, sSlotTilemap_Wide, 18, x, y, width, height);
    else
        BlitBitmapToPartyWindow(windowId, sSlotTilemap_WideNoHP, 18, x, y, width, height);
}

static void DrawEmptySlot(u8 windowId)
{
    BlitBitmapToPartyWindow(windowId, sSlotTilemap_WideEmpty, 18, 0, 0, 18, 3);
}

#define LOAD_PARTY_BOX_PAL(paletteIds, paletteOffsets)                                                    \
{                                                                                                         \
    LoadPalette(GetPartyMenuPalBufferPtr(paletteIds[0]), paletteOffsets[0] + palOffset, PLTT_SIZEOF(1));  \
    LoadPalette(GetPartyMenuPalBufferPtr(paletteIds[1]), paletteOffsets[1] + palOffset, PLTT_SIZEOF(1));  \
    LoadPalette(GetPartyMenuPalBufferPtr(paletteIds[2]), paletteOffsets[2] + palOffset, PLTT_SIZEOF(1));  \
}

static void LoadPartyBoxPalette(struct PartyMenuBox *menuBox, u8 palFlags)
{
    u8 palOffset = BG_PLTT_ID(GetWindowAttribute(menuBox->windowId, WINDOW_PALETTE_NUM));

    if (palFlags & PARTY_PAL_NO_MON)
    {
        LOAD_PARTY_BOX_PAL(sPartyBoxNoMonPalIds, sPartyBoxNoMonPalOffsets);
    }
    else if (palFlags & PARTY_PAL_TO_SOFTBOIL)
    {
        if (palFlags & PARTY_PAL_SELECTED)
        {
            LOAD_PARTY_BOX_PAL(sPartyBoxSelectedForActionPalIds1, sPartyBoxPalOffsets1);
            LOAD_PARTY_BOX_PAL(sPartyBoxCurrSelectionPalIds2, sPartyBoxPalOffsets2);
        }
        else
        {
            LOAD_PARTY_BOX_PAL(sPartyBoxSelectedForActionPalIds1, sPartyBoxPalOffsets1);
            LOAD_PARTY_BOX_PAL(sPartyBoxSelectedForActionPalIds2, sPartyBoxPalOffsets2);
        }
    }
    else if (palFlags & PARTY_PAL_SWITCHING)
    {
        LOAD_PARTY_BOX_PAL(sPartyBoxSelectedForActionPalIds1, sPartyBoxPalOffsets1);
        LOAD_PARTY_BOX_PAL(sPartyBoxSelectedForActionPalIds2, sPartyBoxPalOffsets2);
    }
    else if (palFlags & PARTY_PAL_TO_SWITCH)
    {
        if (palFlags & PARTY_PAL_SELECTED)
        {
            LOAD_PARTY_BOX_PAL(sPartyBoxSelectedForActionPalIds1, sPartyBoxPalOffsets1);
            LOAD_PARTY_BOX_PAL(sPartyBoxCurrSelectionPalIds2, sPartyBoxPalOffsets2);
        }
        else
        {
            LOAD_PARTY_BOX_PAL(sPartyBoxSelectedForActionPalIds1, sPartyBoxPalOffsets1);
            LOAD_PARTY_BOX_PAL(sPartyBoxSelectedForActionPalIds2, sPartyBoxPalOffsets2);
        }
    }
    else if (palFlags & PARTY_PAL_FAINTED)
    {
        if (palFlags & PARTY_PAL_SELECTED)
        {
            LOAD_PARTY_BOX_PAL(sPartyBoxCurrSelectionFaintedPalIds, sPartyBoxPalOffsets1);
            LOAD_PARTY_BOX_PAL(sPartyBoxCurrSelectionPalIds2, sPartyBoxPalOffsets2);
        }
        else
        {
            LOAD_PARTY_BOX_PAL(sPartyBoxFaintedPalIds1, sPartyBoxPalOffsets1);
            LOAD_PARTY_BOX_PAL(sPartyBoxFaintedPalIds2, sPartyBoxPalOffsets2);
        }
    }
    else if (palFlags & PARTY_PAL_MULTI_ALT)
    {
        if (palFlags & PARTY_PAL_SELECTED)
        {
            LOAD_PARTY_BOX_PAL(sPartyBoxCurrSelectionMultiPalIds, sPartyBoxPalOffsets1);
            LOAD_PARTY_BOX_PAL(sPartyBoxCurrSelectionPalIds2, sPartyBoxPalOffsets2);
        }
        else
        {
            LOAD_PARTY_BOX_PAL(sPartyBoxMultiPalIds1, sPartyBoxPalOffsets1);
            LOAD_PARTY_BOX_PAL(sPartyBoxMultiPalIds2, sPartyBoxPalOffsets2);
        }
    }
    else if (palFlags & PARTY_PAL_SELECTED)
    {
        LOAD_PARTY_BOX_PAL(sPartyBoxCurrSelectionPalIds1, sPartyBoxPalOffsets1);
        LOAD_PARTY_BOX_PAL(sPartyBoxCurrSelectionPalIds2, sPartyBoxPalOffsets2);
    }
    else
    {
        LOAD_PARTY_BOX_PAL(sPartyBoxEmptySlotPalIds1, sPartyBoxPalOffsets1);
        LOAD_PARTY_BOX_PAL(sPartyBoxEmptySlotPalIds2, sPartyBoxPalOffsets2);
    }
}

static void DisplayPartyPokemonBarDetail(u8 windowId, const u8 *str, u8 color, const u8 *align)
{
    AddTextPrinterParameterized3(windowId, FONT_SMALL, align[0], align[1], sFontColorTable[color], 0, str);
}

static void DisplayPartyPokemonNickname(struct Pokemon *mon, struct PartyMenuBox *menuBox, u8 c)
{
    u8 nickname[POKEMON_NAME_LENGTH + 1];

    if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE)
    {
        if (c == 1)
            menuBox->infoRects->blitFunc(menuBox->windowId, menuBox->infoRects->dimensions[0] >> 3, menuBox->infoRects->dimensions[1] >> 3, menuBox->infoRects->dimensions[2] >> 3, menuBox->infoRects->dimensions[3] >> 3, FALSE);
        GetMonNickname(mon, nickname);
        DisplayPartyPokemonBarDetail(menuBox->windowId, nickname, 0, menuBox->infoRects->dimensions);
    }
}

static void DisplayPartyPokemonLevelCheck(struct Pokemon *mon, struct PartyMenuBox *menuBox, u8 c)
{
    if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE)
    {
        u8 ailment = GetMonAilment(mon);
        if (ailment == AILMENT_NONE || ailment == AILMENT_PKRS)
        {
            if (c != 0)
                menuBox->infoRects->blitFunc(menuBox->windowId, menuBox->infoRects->dimensions[4] >> 3, (menuBox->infoRects->dimensions[5] >> 3) + 1, menuBox->infoRects->dimensions[6] >> 3, menuBox->infoRects->dimensions[7] >> 3, FALSE);
            if (c != 2)
                DisplayPartyPokemonLevel(GetMonData(mon, MON_DATA_LEVEL), menuBox);
        }
    }
}

static void DisplayPartyPokemonLevel(u8 level, struct PartyMenuBox *menuBox)
{
    ConvertIntToDecimalStringN(gStringVar2, level, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringCopy(gStringVar1, gText_LevelSymbol);
    StringAppend(gStringVar1, gStringVar2);
    DisplayPartyPokemonBarDetail(menuBox->windowId, gStringVar1, 0, &menuBox->infoRects->dimensions[4]);
}

static void DisplayPartyPokemonGenderNidoranCheck(struct Pokemon *mon, struct PartyMenuBox *menuBox, u8 c)
{
    u8 nickname[POKEMON_NAME_LENGTH + 1];

    if (c == 1)
        menuBox->infoRects->blitFunc(menuBox->windowId, menuBox->infoRects->dimensions[8] >> 3, (menuBox->infoRects->dimensions[9] >> 3) + 1, menuBox->infoRects->dimensions[10] >> 3, menuBox->infoRects->dimensions[11] >> 3, FALSE);
    GetMonNickname(mon, nickname);
    DisplayPartyPokemonGender(GetMonGender(mon), GetMonData(mon, MON_DATA_SPECIES), nickname, menuBox);
}

static void DisplayPartyPokemonGender(u8 gender, u16 species, u8 *nickname, struct PartyMenuBox *menuBox)
{
    u8 palOffset = BG_PLTT_ID(GetWindowAttribute(menuBox->windowId, WINDOW_PALETTE_NUM));

    if (species == SPECIES_NONE)
        return;
    if ((species == SPECIES_NIDORAN_M || species == SPECIES_NIDORAN_F) && StringCompare(nickname, gSpeciesNames[species]) == 0)
        return;
    switch (gender)
    {
    case MON_MALE:
        LoadPalette(GetPartyMenuPalBufferPtr(sGenderMalePalIds[0]), sGenderPalOffsets[0] + palOffset, PLTT_SIZEOF(1));
        LoadPalette(GetPartyMenuPalBufferPtr(sGenderMalePalIds[1]), sGenderPalOffsets[1] + palOffset, PLTT_SIZEOF(1));
        DisplayPartyPokemonBarDetail(menuBox->windowId, gText_MaleSymbol, 2, &menuBox->infoRects->dimensions[8]);
        break;
    case MON_FEMALE:
        LoadPalette(GetPartyMenuPalBufferPtr(sGenderFemalePalIds[0]), sGenderPalOffsets[0] + palOffset, PLTT_SIZEOF(1));
        LoadPalette(GetPartyMenuPalBufferPtr(sGenderFemalePalIds[1]), sGenderPalOffsets[1] + palOffset, PLTT_SIZEOF(1));
        DisplayPartyPokemonBarDetail(menuBox->windowId, gText_FemaleSymbol, 2, &menuBox->infoRects->dimensions[8]);
        break;
    }
}

static void DisplayPartyPokemonHPCheck(struct Pokemon *mon, struct PartyMenuBox *menuBox, u8 c)
{
    if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE)
    {
        if (c != 0)
            menuBox->infoRects->blitFunc(menuBox->windowId, menuBox->infoRects->dimensions[12] >> 3, (menuBox->infoRects->dimensions[13] >> 3) + 1, menuBox->infoRects->dimensions[14] >> 3, menuBox->infoRects->dimensions[15] >> 3, FALSE);
        if (c != 2)
            DisplayPartyPokemonHP(GetMonData(mon, MON_DATA_HP), menuBox);
    }
}

static void DisplayPartyPokemonHP(u16 hp, struct PartyMenuBox *menuBox)
{
    u8 *strOut = ConvertIntToDecimalStringN(gStringVar1, hp, STR_CONV_MODE_RIGHT_ALIGN, 3);

    strOut[0] = CHAR_SLASH;
    strOut[1] = EOS;

    DisplayPartyPokemonBarDetail(menuBox->windowId, gStringVar1, 0, &menuBox->infoRects->dimensions[12]);
}

static void DisplayPartyPokemonMaxHPCheck(struct Pokemon *mon, struct PartyMenuBox *menuBox, u8 c)
{
    if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE)
    {
        if (c != 0)
            menuBox->infoRects->blitFunc(menuBox->windowId, (menuBox->infoRects->dimensions[16] >> 3) + 1, (menuBox->infoRects->dimensions[17] >> 3) + 1, menuBox->infoRects->dimensions[18] >> 3, menuBox->infoRects->dimensions[19] >> 3, FALSE);
        if (c != 2)
            DisplayPartyPokemonMaxHP(GetMonData(mon, MON_DATA_MAX_HP), menuBox);
    }
}

static void DisplayPartyPokemonMaxHP(u16 maxhp, struct PartyMenuBox *menuBox)
{
    ConvertIntToDecimalStringN(gStringVar2, maxhp, STR_CONV_MODE_RIGHT_ALIGN, 3);
    StringCopy(gStringVar1, gText_Slash);
    StringAppend(gStringVar1, gStringVar2);
    DisplayPartyPokemonBarDetail(menuBox->windowId, gStringVar1, 0, &menuBox->infoRects->dimensions[16]);
}

static void DisplayPartyPokemonHPBarCheck(struct Pokemon *mon, struct PartyMenuBox *menuBox)
{
    if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE)
        DisplayPartyPokemonHPBar(GetMonData(mon, MON_DATA_HP), GetMonData(mon, MON_DATA_MAX_HP), menuBox);
}

static void DisplayPartyPokemonHPBar(u16 hp, u16 maxhp, struct PartyMenuBox *menuBox)
{
    u8 palOffset = BG_PLTT_ID(GetWindowAttribute(menuBox->windowId, WINDOW_PALETTE_NUM));
    u8 hpFraction;

    switch (GetHPBarLevel(hp, maxhp))
    {
    case HP_BAR_GREEN:
    case HP_BAR_FULL:
        LoadPalette(GetPartyMenuPalBufferPtr(sHPBarGreenPalIds[0]), sHPBarPalOffsets[0] + palOffset, PLTT_SIZEOF(1));
        LoadPalette(GetPartyMenuPalBufferPtr(sHPBarGreenPalIds[1]), sHPBarPalOffsets[1] + palOffset, PLTT_SIZEOF(1));
        break;
    case HP_BAR_YELLOW:
        LoadPalette(GetPartyMenuPalBufferPtr(sHPBarYellowPalIds[0]), sHPBarPalOffsets[0] + palOffset, PLTT_SIZEOF(1));
        LoadPalette(GetPartyMenuPalBufferPtr(sHPBarYellowPalIds[1]), sHPBarPalOffsets[1] + palOffset, PLTT_SIZEOF(1));
        break;
    default:
        LoadPalette(GetPartyMenuPalBufferPtr(sHPBarRedPalIds[0]), sHPBarPalOffsets[0] + palOffset, PLTT_SIZEOF(1));
        LoadPalette(GetPartyMenuPalBufferPtr(sHPBarRedPalIds[1]), sHPBarPalOffsets[1] + palOffset, PLTT_SIZEOF(1));
        break;
    }

    hpFraction = GetScaledHPFraction(hp, maxhp, menuBox->infoRects->dimensions[22]);
    FillWindowPixelRect(menuBox->windowId, sHPBarPalOffsets[1], menuBox->infoRects->dimensions[20], menuBox->infoRects->dimensions[21], hpFraction, 1);
    FillWindowPixelRect(menuBox->windowId, sHPBarPalOffsets[0], menuBox->infoRects->dimensions[20], menuBox->infoRects->dimensions[21] + 1, hpFraction, 2);
    if (hpFraction != menuBox->infoRects->dimensions[22])
    {
        // This appears to be an alternating fill
        FillWindowPixelRect(menuBox->windowId, 0x0D, menuBox->infoRects->dimensions[20] + hpFraction, menuBox->infoRects->dimensions[21], menuBox->infoRects->dimensions[22] - hpFraction, 1);
        FillWindowPixelRect(menuBox->windowId, 0x02, menuBox->infoRects->dimensions[20] + hpFraction, menuBox->infoRects->dimensions[21] + 1, menuBox->infoRects->dimensions[22] - hpFraction, 2);
    }
    CopyWindowToVram(menuBox->windowId, COPYWIN_GFX);
}

static void DisplayPartyPokemonDescriptionText(u8 stringID, struct PartyMenuBox *menuBox, u8 c)
{
    if (c)
    {
        int width = ((menuBox->infoRects->descTextLeft % 8) + menuBox->infoRects->descTextWidth + 7) / 8;
        int height = ((menuBox->infoRects->descTextTop % 8) + menuBox->infoRects->descTextHeight + 7) / 8;
        menuBox->infoRects->blitFunc(menuBox->windowId, menuBox->infoRects->descTextLeft >> 3, menuBox->infoRects->descTextTop >> 3, width, height, TRUE);
    }
    if (c != 2)
        AddTextPrinterParameterized3(menuBox->windowId, FONT_NORMAL, menuBox->infoRects->descTextLeft, menuBox->infoRects->descTextTop, sFontColorTable[0], 0, sDescriptionStringTable[stringID]);
}

static void PartyMenuRemoveWindow(u8 *ptr)
{
    if (*ptr != WINDOW_NONE)
    {
        ClearStdWindowAndFrameToTransparent(*ptr, FALSE);
        RemoveWindow(*ptr);
        *ptr = WINDOW_NONE;
        ScheduleBgCopyTilemapToVram(2);
    }
}

void DisplayPartyMenuStdMessage(u32 stringId)
{
    u8 *windowPtr = &sPartyMenuInternal->windowId[1];

    if (*windowPtr != WINDOW_NONE)
        PartyMenuRemoveWindow(windowPtr);

    if (stringId != PARTY_MSG_NONE)
    {
        switch (stringId)
        {
        case PARTY_MSG_DO_WHAT_WITH_MON:
            *windowPtr = AddWindow(&sDoWhatWithMonMsgWindowTemplate);
            break;
        case PARTY_MSG_DO_WHAT_WITH_ITEM:
            *windowPtr = AddWindow(&sDoWhatWithItemMsgWindowTemplate);
            break;
        case PARTY_MSG_DO_WHAT_WITH_MAIL:
            *windowPtr = AddWindow(&sDoWhatWithMailMsgWindowTemplate);
            break;
        case PARTY_MSG_RESTORE_WHICH_MOVE:
        case PARTY_MSG_BOOST_PP_WHICH_MOVE:
            *windowPtr = AddWindow(&sWhichMoveMsgWindowTemplate);
            break;
        case PARTY_MSG_ALREADY_HOLDING_ONE:
            *windowPtr = AddWindow(&sAlreadyHoldingOneMsgWindowTemplate);
            break;
        default:
            *windowPtr = AddWindow(&sDefaultPartyMsgWindowTemplate);
            break;
        }

        if (stringId == PARTY_MSG_CHOOSE_MON)
        {
            if (sPartyMenuInternal->chooseHalf)
                stringId = PARTY_MSG_CHOOSE_MON_AND_CONFIRM;
            else if (!ShouldUseChooseMonText())
                stringId = PARTY_MSG_CHOOSE_MON_OR_CANCEL;
        }
        DrawStdFrameWithCustomTileAndPalette(*windowPtr, FALSE, 0x4F, 13);
        StringExpandPlaceholders(gStringVar4, sActionStringTable[stringId]);
        AddTextPrinterParameterized(*windowPtr, FONT_NORMAL, gStringVar4, 0, 1, 0, 0);
        ScheduleBgCopyTilemapToVram(2);
    }
}

static bool8 ShouldUseChooseMonText(void)
{
    struct Pokemon *party = gPlayerParty;
    u8 i;
    u8 numAliveMons = 0;

    if (gPartyMenu.action == PARTY_ACTION_SEND_OUT)
        return TRUE;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE && (GetMonData(&party[i], MON_DATA_HP) != 0 || GetMonData(&party[i], MON_DATA_IS_EGG)))
            numAliveMons++;
        if (numAliveMons > 1)
            return TRUE;
    }
    return FALSE;
}

static u8 DisplaySelectionWindow(u8 windowType)
{
    struct WindowTemplate window;
    u8 cursorDimension;
    u8 letterSpacing;
    u8 i;

    switch (windowType)
    {
    case SELECTWINDOW_ACTIONS:
        SetWindowTemplateFields(&window, 2, 19, 19 - (sPartyMenuInternal->numActions * 2), 10, sPartyMenuInternal->numActions * 2, 14, 0x2E9);
        break;
    case SELECTWINDOW_ITEM:
        window = sItemGiveTakeWindowTemplate;
        break;
    case SELECTWINDOW_MAIL:
        window = sMailReadTakeWindowTemplate;
        break;
    default: // SELECTWINDOW_MOVES
        window = sMoveSelectWindowTemplate;
        break;
    }

    sPartyMenuInternal->windowId[0] = AddWindow(&window);
    DrawStdFrameWithCustomTileAndPalette(sPartyMenuInternal->windowId[0], FALSE, 0x4F, 13);
    if (windowType == SELECTWINDOW_MOVES)
        return sPartyMenuInternal->windowId[0];
    cursorDimension = GetMenuCursorDimensionByFont(FONT_NORMAL, 0);
    letterSpacing = GetFontAttribute(FONT_NORMAL, FONTATTR_LETTER_SPACING);

    for (i = 0; i < sPartyMenuInternal->numActions; i++)
    {
        u8 fontColorsId = (sPartyMenuInternal->actions[i] >= MENU_FIELD_MOVES) ? 4 : 3;
        AddTextPrinterParameterized4(sPartyMenuInternal->windowId[0], FONT_NORMAL, cursorDimension, (i * 16) + 1, letterSpacing, 0, sFontColorTable[fontColorsId], 0, sCursorOptions[sPartyMenuInternal->actions[i]].text);
    }

    InitMenuInUpperLeftCorner(sPartyMenuInternal->windowId[0], sPartyMenuInternal->numActions, 0, TRUE);
    ScheduleBgCopyTilemapToVram(2);

    return sPartyMenuInternal->windowId[0];
}

static void PrintMessage(const u8 *text)
{
    DrawStdFrameWithCustomTileAndPalette(WIN_MSG, FALSE, 0x4F, 13);
    gTextFlags.canABSpeedUpPrint = TRUE;
    AddTextPrinterParameterized2(WIN_MSG, FONT_NORMAL, text, GetPlayerTextSpeedDelay(), 0, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY);
}

static void PartyMenuDisplayYesNoMenu(void)
{
    CreateYesNoMenu(&sPartyMenuYesNoWindowTemplate, 0x4F, 13, 0);
}

static u8 CreateLevelUpStatsWindow(void)
{
    sPartyMenuInternal->windowId[0] = AddWindow(&sLevelUpStatsWindowTemplate);
    DrawStdFrameWithCustomTileAndPalette(sPartyMenuInternal->windowId[0], FALSE, 0x4F, 13);
    return sPartyMenuInternal->windowId[0];
}

static void RemoveLevelUpStatsWindow(void)
{
    ClearWindowTilemap(sPartyMenuInternal->windowId[0]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
}

static void SetPartyMonSelectionActions(struct Pokemon *mons, u8 slotId, u8 action)
{
    u8 i;

    if (action == ACTIONS_NONE)
    {
        SetPartyMonFieldSelectionActions(mons, slotId);
    }
    else
    {
        sPartyMenuInternal->numActions = sPartyMenuActionCounts[action];
        for (i = 0; i < sPartyMenuInternal->numActions; i++)
            sPartyMenuInternal->actions[i] = sPartyMenuActions[action][i];
    }
}

static void SetPartyMonFieldSelectionActions(struct Pokemon *mons, u8 slotId)
{
    u8 i, j;

    sPartyMenuInternal->numActions = 0;
    AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, MENU_SUMMARY);

    // Add field moves to action list
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        for (j = 0; sFieldMoves[j] != FIELD_MOVES_COUNT; j++)
        {
            if (GetMonData(&mons[slotId], i + MON_DATA_MOVE1) == sFieldMoves[j])
            {
                AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, j + MENU_FIELD_MOVES);
                break;
            }
        }
    }

    if (!InBattlePike())
    {
        if (GetMonData(&mons[1], MON_DATA_SPECIES) != SPECIES_NONE)
            AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, MENU_SWITCH);
        if (ItemIsMail(GetMonData(&mons[slotId], MON_DATA_HELD_ITEM)))
            AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, MENU_MAIL);
        else
            AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, MENU_ITEM);
    }
    AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, MENU_CANCEL1);
}

static u8 GetPartyMenuActionsType(struct Pokemon *mon)
{
    u32 actionType;

    switch (gPartyMenu.menuType)
    {
    case PARTY_MENU_TYPE_FIELD:
        if (InMultiPartnerRoom() == TRUE || GetMonData(mon, MON_DATA_IS_EGG))
            actionType = ACTIONS_SWITCH;
        else
            actionType = ACTIONS_NONE; // actions populated by SetPartyMonFieldSelectionActions
        break;
    case PARTY_MENU_TYPE_IN_BATTLE:
        actionType = GetPartyMenuActionsTypeInBattle(mon);
        break;
    case PARTY_MENU_TYPE_CHOOSE_HALF:
        switch (GetPartySlotEntryStatus(gPartyMenu.slotId))
        {
        default: // Not eligible
            actionType = ACTIONS_SUMMARY_ONLY;
            break;
        case 0: // Eligible
            actionType = ACTIONS_ENTER;
            break;
        case 1: // Already selected
            actionType = ACTIONS_NO_ENTRY;
            break;
        }
        break;
    case PARTY_MENU_TYPE_DAYCARE:
        actionType = (GetMonData(mon, MON_DATA_IS_EGG)) ? ACTIONS_SUMMARY_ONLY : ACTIONS_STORE;
        break;
    case PARTY_MENU_TYPE_UNION_ROOM_REGISTER:
        actionType = ACTIONS_REGISTER;
        break;
    case PARTY_MENU_TYPE_UNION_ROOM_TRADE:
        actionType = ACTIONS_TRADE;
        break;
    case PARTY_MENU_TYPE_SPIN_TRADE:
        actionType = ACTIONS_SPIN_TRADE;
        break;
    case PARTY_MENU_TYPE_STORE_PYRAMID_HELD_ITEMS:
        actionType = ACTIONS_TAKEITEM_TOSS;
        break;
    // The following have no selection actions (i.e. they exit immediately upon selection)
    // PARTY_MENU_TYPE_CONTEST
    // PARTY_MENU_TYPE_CHOOSE_MON
    // PARTY_MENU_TYPE_MULTI_SHOWCASE
    // PARTY_MENU_TYPE_MOVE_RELEARNER
    // PARTY_MENU_TYPE_MINIGAME
    default:
        actionType = ACTIONS_NONE;
        break;
    }
    return actionType;
}

static bool8 CreateSelectionWindow(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    u16 item;

    GetMonNickname(mon, gStringVar1);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    if (gPartyMenu.menuType != PARTY_MENU_TYPE_STORE_PYRAMID_HELD_ITEMS)
    {
        SetPartyMonSelectionActions(gPlayerParty, gPartyMenu.slotId, GetPartyMenuActionsType(mon));
        DisplaySelectionWindow(SELECTWINDOW_ACTIONS);
        DisplayPartyMenuStdMessage(PARTY_MSG_DO_WHAT_WITH_MON);
    }
    else
    {
        item = GetMonData(mon, MON_DATA_HELD_ITEM);
        if (item != ITEM_NONE)
        {
            SetPartyMonSelectionActions(gPlayerParty, gPartyMenu.slotId, GetPartyMenuActionsType(mon));
            DisplaySelectionWindow(SELECTWINDOW_ITEM);
            CopyItemName(item, gStringVar2);
            DisplayPartyMenuStdMessage(PARTY_MSG_ALREADY_HOLDING_ONE);
        }
        else
        {
            StringExpandPlaceholders(gStringVar4, gText_PkmnNotHolding);
            DisplayPartyMenuMessage(gStringVar4, TRUE);
            ScheduleBgCopyTilemapToVram(2);
            gTasks[taskId].func = Task_UpdateHeldItemSprite;
            return FALSE;
        }
    }
    return TRUE;
}

static void Task_TryCreateSelectionWindow(u8 taskId)
{
    if (CreateSelectionWindow(taskId))
    {
        gTasks[taskId].data[0] = 0xFF;
        gTasks[taskId].func = Task_HandleSelectionMenuInput;
    }
}

static void Task_HandleSelectionMenuInput(u8 taskId)
{
    if (!gPaletteFade.active && MenuHelpers_ShouldWaitForLinkRecv() != TRUE)
    {
        s8 input;
        s16 *data = gTasks[taskId].data;

        if (sPartyMenuInternal->numActions <= 3)
            input = Menu_ProcessInputNoWrapAround_other();
        else
            input = ProcessMenuInput_other();

        data[0] = Menu_GetCursorPos();
        switch (input)
        {
        case MENU_NOTHING_CHOSEN:
            break;
        case MENU_B_PRESSED:
            PlaySE(SE_SELECT);
            PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[2]);
            sCursorOptions[sPartyMenuInternal->actions[sPartyMenuInternal->numActions - 1]].func(taskId);
            break;
        default:
            PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[2]);
            sCursorOptions[sPartyMenuInternal->actions[input]].func(taskId);
            break;
        }
    }
}

static void CursorCb_Summary(u8 taskId)
{
    PlaySE(SE_SELECT);
    sPartyMenuInternal->exitCallback = CB2_ShowPokemonSummaryScreen;
    Task_ClosePartyMenu(taskId);
}

static void CB2_ShowPokemonSummaryScreen(void)
{
    if (gPartyMenu.menuType == PARTY_MENU_TYPE_IN_BATTLE)
    {
        UpdatePartyToBattleOrder();
        ShowPokemonSummaryScreen(SUMMARY_MODE_LOCK_MOVES, gPlayerParty, gPartyMenu.slotId, gPlayerPartyCount - 1, CB2_ReturnToPartyMenuFromSummaryScreen);
    }
    else
    {
        ShowPokemonSummaryScreen(SUMMARY_MODE_NORMAL, gPlayerParty, gPartyMenu.slotId, gPlayerPartyCount - 1, CB2_ReturnToPartyMenuFromSummaryScreen);
    }
}

static void CB2_ReturnToPartyMenuFromSummaryScreen(void)
{
    gPaletteFade.bufferTransferDisabled = TRUE;
    gPartyMenu.slotId = gLastViewedMonIndex;
    InitPartyMenu(gPartyMenu.menuType, KEEP_PARTY_LAYOUT, gPartyMenu.action, TRUE, PARTY_MSG_DO_WHAT_WITH_MON, Task_TryCreateSelectionWindow, gPartyMenu.exitCallback);
}

static void CursorCb_Switch(u8 taskId)
{
    PlaySE(SE_SELECT);
    gPartyMenu.action = PARTY_ACTION_SWITCH;
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    DisplayPartyMenuStdMessage(PARTY_MSG_MOVE_TO_WHERE);
    AnimatePartySlot(gPartyMenu.slotId, 1);
    gPartyMenu.slotId2 = gPartyMenu.slotId;
    gTasks[taskId].func = Task_HandleChooseMonInput;
}

#define tSlot1Left     data[0]
#define tSlot1Top      data[1]
#define tSlot1Width    data[2]
#define tSlot1Height   data[3]
#define tSlot2Left     data[4]
#define tSlot2Top      data[5]
#define tSlot2Width    data[6]
#define tSlot2Height   data[7]
#define tSlot1Offset   data[8]
#define tSlot2Offset   data[9]
#define tSlot1SlideDir data[10]
#define tSlot2SlideDir data[11]

static void SwitchSelectedMons(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u8 windowIds[2];

    if (gPartyMenu.slotId2 == gPartyMenu.slotId)
    {
        FinishTwoMonAction(taskId);
    }
    else
    {
        // Initialize switching party mons slide animation
        windowIds[0] = sPartyMenuBoxes[gPartyMenu.slotId].windowId;
        tSlot1Left = GetWindowAttribute(windowIds[0], WINDOW_TILEMAP_LEFT);
        tSlot1Top = GetWindowAttribute(windowIds[0], WINDOW_TILEMAP_TOP);
        tSlot1Width = GetWindowAttribute(windowIds[0], WINDOW_WIDTH);
        tSlot1Height = GetWindowAttribute(windowIds[0], WINDOW_HEIGHT);
        tSlot1Offset = 0;
        if (tSlot1Width == 10)
            tSlot1SlideDir = -1;
        else
            tSlot1SlideDir = 1;
        windowIds[1] = sPartyMenuBoxes[gPartyMenu.slotId2].windowId;
        tSlot2Left = GetWindowAttribute(windowIds[1], WINDOW_TILEMAP_LEFT);
        tSlot2Top = GetWindowAttribute(windowIds[1], WINDOW_TILEMAP_TOP);
        tSlot2Width = GetWindowAttribute(windowIds[1], WINDOW_WIDTH);
        tSlot2Height = GetWindowAttribute(windowIds[1], WINDOW_HEIGHT);
        tSlot2Offset = 0;
        if (tSlot2Width == 10)
            tSlot2SlideDir = -1;
        else
            tSlot2SlideDir = 1;
        sSlot1TilemapBuffer = Alloc(tSlot1Width * (tSlot1Height << 1));
        sSlot2TilemapBuffer = Alloc(tSlot2Width * (tSlot2Height << 1));
        CopyToBufferFromBgTilemap(0, sSlot1TilemapBuffer, tSlot1Left, tSlot1Top, tSlot1Width, tSlot1Height);
        CopyToBufferFromBgTilemap(0, sSlot2TilemapBuffer, tSlot2Left, tSlot2Top, tSlot2Width, tSlot2Height);
        ClearWindowTilemap(windowIds[0]);
        ClearWindowTilemap(windowIds[1]);
        gPartyMenu.action = PARTY_ACTION_SWITCHING;
        AnimatePartySlot(gPartyMenu.slotId, 1);
        AnimatePartySlot(gPartyMenu.slotId2, 1);
        SlidePartyMenuBoxOneStep(taskId);
        gTasks[taskId].func = Task_SlideSelectedSlotsOffscreen;
    }
}

// returns FALSE if the slot has slid fully offscreen / back onscreen
static bool8 TryMovePartySlot(s16 x, s16 width, u8 *leftMove, u8 *newX, u8 *newWidth)
{
    if (x + width < 0)
        return FALSE;
    if (x >= 32)
        return FALSE;

    if (x < 0)
    {
        *leftMove = x * -1;
        *newX = 0;
        *newWidth = width + x;
    }
    else
    {
        *leftMove = 0;
        *newX = x;
        if (x + width >= 32)
            *newWidth = 32 - x;
        else
            *newWidth = width;

    }
    return TRUE;
}

static void MoveAndBufferPartySlot(const void *rectSrc, s16 x, s16 y, s16 width, s16 height, s16 dir)
{
    u8 srcX, newX, newWidth;

    if (TryMovePartySlot(x, width, &srcX, &newX, &newWidth))
    {
        FillBgTilemapBufferRect_Palette0(0, 0, newX, y, newWidth, height);
        if (TryMovePartySlot(x + dir, width, &srcX, &newX, &newWidth))
            CopyRectToBgTilemapBufferRect(0, rectSrc, srcX, 0, width, height, newX, y, newWidth, height, 17, 0, 0);
    }
}

static void MovePartyMenuBoxSprites(struct PartyMenuBox *menuBox, s16 offset)
{
    gSprites[menuBox->pokeballSpriteId].x2 += offset * 8;
    gSprites[menuBox->itemSpriteId].x2 += offset * 8;
    gSprites[menuBox->monSpriteId].x2 += offset * 8;
    gSprites[menuBox->statusSpriteId].x2 += offset * 8;
}

static void SlidePartyMenuBoxSpritesOneStep(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (tSlot1SlideDir != 0)
        MovePartyMenuBoxSprites(&sPartyMenuBoxes[gPartyMenu.slotId], tSlot1SlideDir);
    if (tSlot2SlideDir != 0)
        MovePartyMenuBoxSprites(&sPartyMenuBoxes[gPartyMenu.slotId2], tSlot2SlideDir);
}

static void SlidePartyMenuBoxOneStep(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (tSlot1SlideDir != 0)
        MoveAndBufferPartySlot(sSlot1TilemapBuffer, tSlot1Left + tSlot1Offset, tSlot1Top, tSlot1Width, tSlot1Height, tSlot1SlideDir);
    if (tSlot2SlideDir != 0)
        MoveAndBufferPartySlot(sSlot2TilemapBuffer, tSlot2Left + tSlot2Offset, tSlot2Top, tSlot2Width, tSlot2Height, tSlot2SlideDir);
    ScheduleBgCopyTilemapToVram(0);
}

static void Task_SlideSelectedSlotsOffscreen(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 slidingSlotPositions[2];

    SlidePartyMenuBoxOneStep(taskId);
    SlidePartyMenuBoxSpritesOneStep(taskId);
    tSlot1Offset += tSlot1SlideDir;
    tSlot2Offset += tSlot2SlideDir;
    slidingSlotPositions[0] = tSlot1Left + tSlot1Offset;
    slidingSlotPositions[1] = tSlot2Left + tSlot2Offset;

    // Both slots have slid offscreen
    if (slidingSlotPositions[0] > 33 && slidingSlotPositions[1] > 33)
    {
        tSlot1SlideDir *= -1;
        tSlot2SlideDir *= -1;
        SwitchPartyMon();
        DisplayPartyPokemonData(gPartyMenu.slotId);
        DisplayPartyPokemonData(gPartyMenu.slotId2);
        PutWindowTilemap(sPartyMenuBoxes[gPartyMenu.slotId].windowId);
        PutWindowTilemap(sPartyMenuBoxes[gPartyMenu.slotId2].windowId);
        CopyToBufferFromBgTilemap(0, sSlot1TilemapBuffer, tSlot1Left, tSlot1Top, tSlot1Width, tSlot1Height);
        CopyToBufferFromBgTilemap(0, sSlot2TilemapBuffer, tSlot2Left, tSlot2Top, tSlot2Width, tSlot2Height);
        ClearWindowTilemap(sPartyMenuBoxes[gPartyMenu.slotId].windowId);
        ClearWindowTilemap(sPartyMenuBoxes[gPartyMenu.slotId2].windowId);
        gTasks[taskId].func = Task_SlideSelectedSlotsOnscreen;
    }
}

static void Task_SlideSelectedSlotsOnscreen(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    SlidePartyMenuBoxOneStep(taskId);
    SlidePartyMenuBoxSpritesOneStep(taskId);

    // Both slots have slid back onscreen
    if (tSlot1SlideDir == 0 && tSlot2SlideDir == 0)
    {
        PutWindowTilemap(sPartyMenuBoxes[gPartyMenu.slotId].windowId);
        PutWindowTilemap(sPartyMenuBoxes[gPartyMenu.slotId2].windowId);
        ScheduleBgCopyTilemapToVram(0);
        Free(sSlot1TilemapBuffer);
        Free(sSlot2TilemapBuffer);
        FinishTwoMonAction(taskId);
    }
    // Continue sliding
    else
    {
        tSlot1Offset += tSlot1SlideDir;
        tSlot2Offset += tSlot2SlideDir;
        if (tSlot1Offset == 0)
            tSlot1SlideDir = 0;
        if (tSlot2Offset == 0)
            tSlot2SlideDir = 0;
    }
}

static void SwitchMenuBoxSprites(u8 *spriteIdPtr1, u8 *spriteIdPtr2)
{
    u8 spriteIdBuffer = *spriteIdPtr1;
    u16 xBuffer1, yBuffer1, xBuffer2, yBuffer2;

    *spriteIdPtr1 = *spriteIdPtr2;
    *spriteIdPtr2 = spriteIdBuffer;
    xBuffer1 = gSprites[*spriteIdPtr1].x;
    yBuffer1 = gSprites[*spriteIdPtr1].y;
    xBuffer2 = gSprites[*spriteIdPtr1].x2;
    yBuffer2 = gSprites[*spriteIdPtr1].y2;
    gSprites[*spriteIdPtr1].x = gSprites[*spriteIdPtr2].x;
    gSprites[*spriteIdPtr1].y = gSprites[*spriteIdPtr2].y;
    gSprites[*spriteIdPtr1].x2 = gSprites[*spriteIdPtr2].x2;
    gSprites[*spriteIdPtr1].y2 = gSprites[*spriteIdPtr2].y2;
    gSprites[*spriteIdPtr2].x = xBuffer1;
    gSprites[*spriteIdPtr2].y = yBuffer1;
    gSprites[*spriteIdPtr2].x2 = xBuffer2;
    gSprites[*spriteIdPtr2].y2 = yBuffer2;
}

static void SwitchPartyMon(void)
{
    struct PartyMenuBox *menuBoxes[2];
    struct Pokemon *mon1, *mon2;
    struct Pokemon *monBuffer;

    menuBoxes[0] = &sPartyMenuBoxes[gPartyMenu.slotId];
    menuBoxes[1] = &sPartyMenuBoxes[gPartyMenu.slotId2];
    mon1 = &gPlayerParty[gPartyMenu.slotId];
    mon2 = &gPlayerParty[gPartyMenu.slotId2];
    monBuffer = Alloc(sizeof(struct Pokemon));
    *monBuffer = *mon1;
    *mon1 = *mon2;
    *mon2 = *monBuffer;
    Free(monBuffer);
    SwitchMenuBoxSprites(&menuBoxes[0]->pokeballSpriteId, &menuBoxes[1]->pokeballSpriteId);
    SwitchMenuBoxSprites(&menuBoxes[0]->itemSpriteId, &menuBoxes[1]->itemSpriteId);
    SwitchMenuBoxSprites(&menuBoxes[0]->monSpriteId, &menuBoxes[1]->monSpriteId);
    SwitchMenuBoxSprites(&menuBoxes[0]->statusSpriteId, &menuBoxes[1]->statusSpriteId);
}

// Finish switching mons or using Softboiled
static void FinishTwoMonAction(u8 taskId)
{
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    gPartyMenu.action = PARTY_ACTION_CHOOSE_MON;
    AnimatePartySlot(gPartyMenu.slotId, 0);
    gPartyMenu.slotId = gPartyMenu.slotId2;
    AnimatePartySlot(gPartyMenu.slotId2, 1);
    DisplayPartyMenuStdMessage(PARTY_MSG_CHOOSE_MON);
    gTasks[taskId].func = Task_HandleChooseMonInput;
}

#undef tSlot1Left
#undef tSlot1Top
#undef tSlot1Width
#undef tSlot1Height
#undef tSlot2Left
#undef tSlot2Top
#undef tSlot2Width
#undef tSlot2Height
#undef tSlot1Offset
#undef tSlot2Offset
#undef tSlot1SlideDir
#undef tSlot2SlideDir

static void CursorCb_Cancel1(u8 taskId)
{
    PlaySE(SE_SELECT);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    if (gPartyMenu.menuType == PARTY_MENU_TYPE_DAYCARE)
        DisplayPartyMenuStdMessage(PARTY_MSG_CHOOSE_MON_2);
    else
        DisplayPartyMenuStdMessage(PARTY_MSG_CHOOSE_MON);
    gTasks[taskId].func = Task_HandleChooseMonInput;
}

static void CursorCb_Item(u8 taskId)
{
    PlaySE(SE_SELECT);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    SetPartyMonSelectionActions(gPlayerParty, gPartyMenu.slotId, ACTIONS_ITEM);
    DisplaySelectionWindow(SELECTWINDOW_ITEM);
    DisplayPartyMenuStdMessage(PARTY_MSG_DO_WHAT_WITH_ITEM);
    gTasks[taskId].data[0] = 0xFF;
    gTasks[taskId].func = Task_HandleSelectionMenuInput;
}

static void CursorCb_Give(u8 taskId)
{
    PlaySE(SE_SELECT);
    sPartyMenuInternal->exitCallback = CB2_SelectBagItemToGive;
    Task_ClosePartyMenu(taskId);
}

static void CB2_SelectBagItemToGive(void)
{
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE == FALSE)
        GoToBagMenu(ITEMMENULOCATION_PARTY, POCKETS_COUNT, CB2_GiveHoldItem);
    else
        GoToBattlePyramidBagMenu(PYRAMIDBAG_LOC_PARTY, CB2_GiveHoldItem);
}

static void CB2_GiveHoldItem(void)
{
    if (gSpecialVar_ItemId == ITEM_NONE)
    {
        InitPartyMenu(gPartyMenu.menuType, KEEP_PARTY_LAYOUT, gPartyMenu.action, TRUE, PARTY_MSG_NONE, Task_TryCreateSelectionWindow, gPartyMenu.exitCallback);
    }
    else
    {
        sPartyMenuItemId = GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_HELD_ITEM);

        // Already holding item
        if (sPartyMenuItemId != ITEM_NONE)
        {
            InitPartyMenu(gPartyMenu.menuType, KEEP_PARTY_LAYOUT, gPartyMenu.action, TRUE, PARTY_MSG_NONE, Task_SwitchHoldItemsPrompt, gPartyMenu.exitCallback);
        }
        // Give mail
        else if (ItemIsMail(gSpecialVar_ItemId))
        {
            RemoveBagItem(gSpecialVar_ItemId, 1);
            GiveItemToMon(&gPlayerParty[gPartyMenu.slotId], gSpecialVar_ItemId);
            CB2_WriteMailToGiveMon();
        }
        // Give item
        else
        {
            InitPartyMenu(gPartyMenu.menuType, KEEP_PARTY_LAYOUT, gPartyMenu.action, TRUE, PARTY_MSG_NONE, Task_GiveHoldItem, gPartyMenu.exitCallback);
        }
    }
}

static void Task_GiveHoldItem(u8 taskId)
{
    u16 item;

    if (!gPaletteFade.active)
    {
        item = gSpecialVar_ItemId;
        DisplayGaveHeldItemMessage(&gPlayerParty[gPartyMenu.slotId], item, FALSE, 0);
        GiveItemToMon(&gPlayerParty[gPartyMenu.slotId], item);
        RemoveBagItem(item, 1);
        gTasks[taskId].func = Task_UpdateHeldItemSprite;
    }
}

static void Task_SwitchHoldItemsPrompt(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DisplayAlreadyHoldingItemSwitchMessage(&gPlayerParty[gPartyMenu.slotId], sPartyMenuItemId, TRUE);
        gTasks[taskId].func = Task_SwitchItemsYesNo;
    }
}

static void Task_SwitchItemsYesNo(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        PartyMenuDisplayYesNoMenu();
        gTasks[taskId].func = Task_HandleSwitchItemsYesNoInput;
    }
}

static void Task_HandleSwitchItemsYesNoInput(u8 taskId)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // Yes, switch items
        RemoveBagItem(gSpecialVar_ItemId, 1);

        // No room to return held item to bag
        if (AddBagItem(sPartyMenuItemId, 1) == FALSE)
        {
            AddBagItem(gSpecialVar_ItemId, 1);
            BufferBagFullCantTakeItemMessage(sPartyMenuItemId);
            DisplayPartyMenuMessage(gStringVar4, FALSE);
            gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
        }
        // Giving mail
        else if (ItemIsMail(gSpecialVar_ItemId))
        {
            GiveItemToMon(&gPlayerParty[gPartyMenu.slotId], gSpecialVar_ItemId);
            gTasks[taskId].func = Task_WriteMailToGiveMonAfterText;
        }
        // Giving item
        else
        {
            GiveItemToMon(&gPlayerParty[gPartyMenu.slotId], gSpecialVar_ItemId);
            DisplaySwitchedHeldItemMessage(gSpecialVar_ItemId, sPartyMenuItemId, TRUE);
            gTasks[taskId].func = Task_UpdateHeldItemSprite;
        }
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        // fallthrough
    case 1: // No
        gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
        break;
    }
}

static void Task_WriteMailToGiveMonAfterText(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        sPartyMenuInternal->exitCallback = CB2_WriteMailToGiveMon;
        Task_ClosePartyMenu(taskId);
    }
}

static void CB2_WriteMailToGiveMon(void)
{
    u8 mail = GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_MAIL);

    DoEasyChatScreen(
        EASY_CHAT_TYPE_MAIL,
        gSaveBlock1Ptr->mail[mail].words,
        CB2_ReturnToPartyMenuFromWritingMail,
        EASY_CHAT_PERSON_DISPLAY_NONE);
}

static void CB2_ReturnToPartyMenuFromWritingMail(void)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    u16 item = GetMonData(mon, MON_DATA_HELD_ITEM);

    // Canceled writing mail
    if (gSpecialVar_Result == FALSE)
    {
        TakeMailFromMon(mon);
        SetMonData(mon, MON_DATA_HELD_ITEM, &sPartyMenuItemId);
        RemoveBagItem(sPartyMenuItemId, 1);
        AddBagItem(item, 1);
        InitPartyMenu(gPartyMenu.menuType, KEEP_PARTY_LAYOUT, gPartyMenu.action, TRUE, PARTY_MSG_CHOOSE_MON, Task_TryCreateSelectionWindow, gPartyMenu.exitCallback);
    }
    // Wrote mail
    else
    {
        InitPartyMenu(gPartyMenu.menuType, KEEP_PARTY_LAYOUT, gPartyMenu.action, TRUE, PARTY_MSG_NONE, Task_DisplayGaveMailFromPartyMessage, gPartyMenu.exitCallback);
    }
}

// Nearly redundant with Task_DisplayGaveMailFromBagMessgae
static void Task_DisplayGaveMailFromPartyMessage(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        if (sPartyMenuItemId == ITEM_NONE)
            DisplayGaveHeldItemMessage(&gPlayerParty[gPartyMenu.slotId], gSpecialVar_ItemId, FALSE, 0);
        else
            DisplaySwitchedHeldItemMessage(gSpecialVar_ItemId, sPartyMenuItemId, FALSE);
        gTasks[taskId].func = Task_UpdateHeldItemSprite;
    }
}

static void Task_UpdateHeldItemSprite(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];

    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        UpdatePartyMonHeldItemSprite(mon, &sPartyMenuBoxes[gPartyMenu.slotId]);
        if (gPartyMenu.menuType == PARTY_MENU_TYPE_STORE_PYRAMID_HELD_ITEMS)
        {
            if (GetMonData(mon, MON_DATA_HELD_ITEM) != ITEM_NONE)
                DisplayPartyPokemonDescriptionText(PARTYBOX_DESC_HAVE, &sPartyMenuBoxes[gPartyMenu.slotId], 1);
            else
                DisplayPartyPokemonDescriptionText(PARTYBOX_DESC_DONT_HAVE, &sPartyMenuBoxes[gPartyMenu.slotId], 1);
        }
        Task_ReturnToChooseMonAfterText(taskId);
    }
}

static void CursorCb_TakeItem(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    u16 item = GetMonData(mon, MON_DATA_HELD_ITEM);

    PlaySE(SE_SELECT);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    switch (TryTakeMonItem(mon))
    {
    case 0: // Not holding item
        GetMonNickname(mon, gStringVar1);
        StringExpandPlaceholders(gStringVar4, gText_PkmnNotHolding);
        DisplayPartyMenuMessage(gStringVar4, TRUE);
        break;
    case 1: // No room to take item
        BufferBagFullCantTakeItemMessage(item);
        DisplayPartyMenuMessage(gStringVar4, TRUE);
        break;
    default: // Took item
        DisplayTookHeldItemMessage(mon, item, TRUE);
        break;
    }
    ScheduleBgCopyTilemapToVram(2);
    gTasks[taskId].func = Task_UpdateHeldItemSprite;
}

static void CursorCb_Toss(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    u16 item = GetMonData(mon, MON_DATA_HELD_ITEM);

    PlaySE(SE_SELECT);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    if (item == ITEM_NONE)
    {
        GetMonNickname(mon, gStringVar1);
        StringExpandPlaceholders(gStringVar4, gText_PkmnNotHolding);
        DisplayPartyMenuMessage(gStringVar4, TRUE);
        gTasks[taskId].func = Task_UpdateHeldItemSprite;
    }
    else
    {
        CopyItemName(item, gStringVar1);
        StringExpandPlaceholders(gStringVar4, gText_ThrowAwayItem);
        DisplayPartyMenuMessage(gStringVar4, TRUE);
        gTasks[taskId].func = Task_TossHeldItemYesNo;
    }
}

static void Task_TossHeldItemYesNo(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        PartyMenuDisplayYesNoMenu();
        gTasks[taskId].func = Task_HandleTossHeldItemYesNoInput;
    }
}

static void Task_HandleTossHeldItemYesNoInput(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];

    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0:
        CopyItemName(GetMonData(mon, MON_DATA_HELD_ITEM), gStringVar1);
        StringExpandPlaceholders(gStringVar4, gText_ItemThrownAway);
        DisplayPartyMenuMessage(gStringVar4, FALSE);
        gTasks[taskId].func = Task_TossHeldItem;
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        // fallthrough
    case 1:
        gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
        break;
    }
}

static void Task_TossHeldItem(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];

    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        u16 item = ITEM_NONE;

        SetMonData(mon, MON_DATA_HELD_ITEM, &item);
        UpdatePartyMonHeldItemSprite(mon, &sPartyMenuBoxes[gPartyMenu.slotId]);
        DisplayPartyPokemonDescriptionText(PARTYBOX_DESC_DONT_HAVE, &sPartyMenuBoxes[gPartyMenu.slotId], 1);
        gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
    }
}

static void CursorCb_Mail(u8 taskId)
{
    PlaySE(SE_SELECT);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    SetPartyMonSelectionActions(gPlayerParty, gPartyMenu.slotId, ACTIONS_MAIL);
    DisplaySelectionWindow(SELECTWINDOW_MAIL);
    DisplayPartyMenuStdMessage(PARTY_MSG_DO_WHAT_WITH_MAIL);
    gTasks[taskId].data[0] = 0xFF;
    gTasks[taskId].func = Task_HandleSelectionMenuInput;
}

static void CursorCb_Read(u8 taskId)
{
    PlaySE(SE_SELECT);
    sPartyMenuInternal->exitCallback = CB2_ReadHeldMail;
    Task_ClosePartyMenu(taskId);
}

static void CB2_ReadHeldMail(void)
{
    ReadMail(&gSaveBlock1Ptr->mail[GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_MAIL)], CB2_ReturnToPartyMenuFromReadingMail, TRUE);
}

static void CB2_ReturnToPartyMenuFromReadingMail(void)
{
    gPaletteFade.bufferTransferDisabled = TRUE;
    InitPartyMenu(gPartyMenu.menuType, KEEP_PARTY_LAYOUT, gPartyMenu.action, TRUE, PARTY_MSG_DO_WHAT_WITH_MON, Task_TryCreateSelectionWindow, gPartyMenu.exitCallback);
}

static void CursorCb_TakeMail(u8 taskId)
{
    PlaySE(SE_SELECT);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    DisplayPartyMenuMessage(gText_SendMailToPC, TRUE);
    gTasks[taskId].func = Task_SendMailToPCYesNo;
}

static void Task_SendMailToPCYesNo(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        PartyMenuDisplayYesNoMenu();
        gTasks[taskId].func = Task_HandleSendMailToPCYesNoInput;
    }
}

static void Task_HandleSendMailToPCYesNoInput(u8 taskId)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // Yes, send to PC
        if (TakeMailFromMonAndSave(&gPlayerParty[gPartyMenu.slotId]) != MAIL_NONE)
        {
            DisplayPartyMenuMessage(gText_MailSentToPC, FALSE);
            gTasks[taskId].func = Task_UpdateHeldItemSprite;
        }
        else
        {
            DisplayPartyMenuMessage(gText_PCMailboxFull, FALSE);
            gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
        }
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        // fallthrough
    case 1:
        DisplayPartyMenuMessage(gText_MailMessageWillBeLost, TRUE);
        gTasks[taskId].func = Task_LoseMailMessageYesNo;
        break;
    }
}

static void Task_LoseMailMessageYesNo(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        PartyMenuDisplayYesNoMenu();
        gTasks[taskId].func = Task_HandleLoseMailMessageYesNoInput;
    }
}

static void Task_HandleLoseMailMessageYesNoInput(u8 taskId)
{
    u16 item;

    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // Yes, lose mail message
        item = GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_HELD_ITEM);
        if (AddBagItem(item, 1) == TRUE)
        {
            TakeMailFromMon(&gPlayerParty[gPartyMenu.slotId]);
            DisplayPartyMenuMessage(gText_MailTakenFromPkmn, FALSE);
            gTasks[taskId].func = Task_UpdateHeldItemSprite;
        }
        else
        {
            BufferBagFullCantTakeItemMessage(item);
            DisplayPartyMenuMessage(gStringVar4, FALSE);
            gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
        }
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        // fallthrough
    case 1:
        gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
        break;
    }
}

static void CursorCb_Cancel2(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];

    PlaySE(SE_SELECT);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    SetPartyMonSelectionActions(gPlayerParty, gPartyMenu.slotId, GetPartyMenuActionsType(mon));
    if (gPartyMenu.menuType != PARTY_MENU_TYPE_STORE_PYRAMID_HELD_ITEMS)
    {
        DisplaySelectionWindow(SELECTWINDOW_ACTIONS);
        DisplayPartyMenuStdMessage(PARTY_MSG_DO_WHAT_WITH_MON);
    }
    else
    {
        DisplaySelectionWindow(SELECTWINDOW_ITEM);
        CopyItemName(GetMonData(mon, MON_DATA_HELD_ITEM), gStringVar2);
        DisplayPartyMenuStdMessage(PARTY_MSG_ALREADY_HOLDING_ONE);
    }
    gTasks[taskId].data[0] = 0xFF;
    gTasks[taskId].func = Task_HandleSelectionMenuInput;
}

static void CursorCb_SendMon(u8 taskId)
{
    PlaySE(SE_SELECT);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    if (TrySwitchInPokemon() == TRUE)
    {
        Task_ClosePartyMenu(taskId);
    }
    else
    {
        // gStringVar4 below is the error message buffered by TrySwitchInPokemon
        PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
        DisplayPartyMenuMessage(gStringVar4, TRUE);
        gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
    }
}

static void CursorCb_Enter(u8 taskId)
{
    u8 maxBattlers;
    u8 i;

    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    maxBattlers = GetMaxBattleEntries();
    for (i = 0; i < maxBattlers; i++)
    {
        if (gSelectedOrderFromParty[i] == 0)
        {
            PlaySE(SE_SELECT);
            gSelectedOrderFromParty[i] = gPartyMenu.slotId + 1;
            DisplayPartyPokemonDescriptionText(i + PARTYBOX_DESC_FIRST, &sPartyMenuBoxes[gPartyMenu.slotId], 1);
            if (i == (maxBattlers - 1))
                MoveCursorToConfirm();
            DisplayPartyMenuStdMessage(PARTY_MSG_CHOOSE_MON);
            gTasks[taskId].func = Task_HandleChooseMonInput;
            return;
        }
    }
    ConvertIntToDecimalStringN(gStringVar1, maxBattlers, STR_CONV_MODE_LEFT_ALIGN, 1);
    StringExpandPlaceholders(gStringVar4, gText_NoMoreThanVar1Pkmn);
    PlaySE(SE_FAILURE);
    DisplayPartyMenuMessage(gStringVar4, TRUE);
    gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
}

static void MoveCursorToConfirm(void)
{
    AnimatePartySlot(gPartyMenu.slotId, 0);
    gPartyMenu.slotId = PARTY_SIZE;
    AnimatePartySlot(gPartyMenu.slotId, 1);
}

static void CursorCb_NoEntry(u8 taskId)
{
    u8 maxBattlers;
    u8 i, j;

    PlaySE(SE_SELECT);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    maxBattlers = GetMaxBattleEntries();
    for (i = 0; i < maxBattlers; i++)
    {
        if (gSelectedOrderFromParty[i] == (gPartyMenu.slotId + 1))
        {
            for (j = i; j < (maxBattlers - 1); j++)
                gSelectedOrderFromParty[j] = gSelectedOrderFromParty[j + 1];
            gSelectedOrderFromParty[j] = 0;
            break;
        }
    }
    DisplayPartyPokemonDescriptionText(PARTYBOX_DESC_ABLE_3, &sPartyMenuBoxes[gPartyMenu.slotId], 1);
    for (i = 0; i < (maxBattlers - 1); i++)
    {
        if (gSelectedOrderFromParty[i] != 0)
            DisplayPartyPokemonDescriptionText(i + PARTYBOX_DESC_FIRST, &sPartyMenuBoxes[gSelectedOrderFromParty[i] - 1], 1);
    }
    DisplayPartyMenuStdMessage(PARTY_MSG_CHOOSE_MON);
    gTasks[taskId].func = Task_HandleChooseMonInput;
}

static void CursorCb_Store(u8 taskId)
{
    PlaySE(SE_SELECT);
    Task_ClosePartyMenu(taskId);
}

// Register mon for the Trading Board in Union Room
static void CursorCb_Register(u8 taskId)
{
    u16 species2 = GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_SPECIES_OR_EGG);
    u16 species = GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_SPECIES);
    u8 isModernFatefulEncounter = GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_MODERN_FATEFUL_ENCOUNTER);

    switch (CanRegisterMonForTradingBoard(*(struct RfuGameCompatibilityData *)GetHostRfuGameData(), species2, species, isModernFatefulEncounter))
    {
    case CANT_REGISTER_MON:
        StringExpandPlaceholders(gStringVar4, gText_PkmnCantBeTradedNow);
        break;
    case CANT_REGISTER_EGG:
        StringExpandPlaceholders(gStringVar4, gText_EggCantBeTradedNow);
        break;
    default:
        PlaySE(SE_SELECT);
        Task_ClosePartyMenu(taskId);
        return;
    }
    PlaySE(SE_FAILURE);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    StringAppend(gStringVar4, gText_PauseUntilPress);
    DisplayPartyMenuMessage(gStringVar4, TRUE);
    gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
}

static void CursorCb_Trade1(u8 taskId)
{
    u16 species2 = GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_SPECIES_OR_EGG);
    u16 species = GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_SPECIES);
    u8 isModernFatefulEncounter = GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_MODERN_FATEFUL_ENCOUNTER);
    u32 stringId = GetUnionRoomTradeMessageId(*(struct RfuGameCompatibilityData *)GetHostRfuGameData(), gRfuPartnerCompatibilityData, species2, gUnionRoomOfferedSpecies, gUnionRoomRequestedMonType, species, isModernFatefulEncounter);

    if (stringId != UR_TRADE_MSG_NONE)
    {
        StringExpandPlaceholders(gStringVar4, sUnionRoomTradeMessages[stringId - 1]);
        PlaySE(SE_FAILURE);
        PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
        PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
        StringAppend(gStringVar4, gText_PauseUntilPress);
        DisplayPartyMenuMessage(gStringVar4, TRUE);
        gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
    }
    else
    {
        PlaySE(SE_SELECT);
        Task_ClosePartyMenu(taskId);
    }
}

// Spin Trade (based on the translation of the Japanese trade prompt)
// Not fully implemented, and normally unreachable because PARTY_MENU_TYPE_SPIN_TRADE is never used
static void CursorCb_Trade2(u8 taskId)
{
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    switch (CanSpinTradeMon(gPlayerParty, gPartyMenu.slotId))
    {
    case CANT_TRADE_LAST_MON:
        StringExpandPlaceholders(gStringVar4, gText_OnlyPkmnForBattle);
        break;
    case CANT_TRADE_NATIONAL:
        StringExpandPlaceholders(gStringVar4, gText_PkmnCantBeTradedNow);
        break;
    case CANT_TRADE_EGG_YET:
        StringExpandPlaceholders(gStringVar4, gText_EggCantBeTradedNow);
        break;
    default: // CAN_TRADE_MON
        PlaySE(SE_SELECT);
        GetMonNickname(&gPlayerParty[gPartyMenu.slotId], gStringVar1);
        StringExpandPlaceholders(gStringVar4, gJPText_AreYouSureYouWantToSpinTradeMon);
        DisplayPartyMenuMessage(gStringVar4, TRUE);
        gTasks[taskId].func = Task_SpinTradeYesNo;
        return;
    }
    PlaySE(SE_FAILURE);
    StringAppend(gStringVar4, gText_PauseUntilPress);
    DisplayPartyMenuMessage(gStringVar4, TRUE);
    gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
}

static void Task_SpinTradeYesNo(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        PartyMenuDisplayYesNoMenu();
        gTasks[taskId].func = Task_HandleSpinTradeYesNoInput;
    }
}

// See comment on CursorCb_Trade2. Because no callback is set, selecting YES (0) to spin trade just closes the party menu
static void Task_HandleSpinTradeYesNoInput(u8 taskId)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0:
        Task_ClosePartyMenu(taskId);
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        // fallthrough
    case 1:
        Task_ReturnToChooseMonAfterText(taskId);
        break;
    }
}

static void CursorCb_FieldMove(u8 taskId)
{
    u8 fieldMove = sPartyMenuInternal->actions[Menu_GetCursorPos()] - MENU_FIELD_MOVES;
    const struct MapHeader *mapHeader;

    PlaySE(SE_SELECT);
    if (sFieldMoveCursorCallbacks[fieldMove].fieldMoveFunc == NULL)
        return;

    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
    if (MenuHelpers_IsLinkActive() == TRUE || InUnionRoom() == TRUE)
    {
        if (fieldMove == FIELD_MOVE_MILK_DRINK || fieldMove == FIELD_MOVE_SOFT_BOILED)
            DisplayPartyMenuStdMessage(PARTY_MSG_CANT_USE_HERE);
        else
            DisplayPartyMenuStdMessage(sFieldMoveCursorCallbacks[fieldMove].msgId);

        gTasks[taskId].func = Task_CancelAfterAorBPress;
    }
    else
    {
        // All field moves before WATERFALL are HMs.
        if (fieldMove <= FIELD_MOVE_WATERFALL && FlagGet(FLAG_BADGE01_GET + fieldMove) != TRUE)
        {
            DisplayPartyMenuMessage(gText_CantUseUntilNewBadge, TRUE);
            gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
        }
        else if (sFieldMoveCursorCallbacks[fieldMove].fieldMoveFunc() == TRUE)
        {
            switch (fieldMove)
            {
            case FIELD_MOVE_MILK_DRINK:
            case FIELD_MOVE_SOFT_BOILED:
                ChooseMonForSoftboiled(taskId);
                break;
            case FIELD_MOVE_TELEPORT:
                mapHeader = Overworld_GetMapHeaderByGroupAndId(gSaveBlock1Ptr->lastHealLocation.mapGroup, gSaveBlock1Ptr->lastHealLocation.mapNum);
                GetMapNameGeneric(gStringVar1, mapHeader->regionMapSectionId);
                StringExpandPlaceholders(gStringVar4, gText_ReturnToHealingSpot);
                DisplayFieldMoveExitAreaMessage(taskId);
                sPartyMenuInternal->data[0] = fieldMove;
                break;
            case FIELD_MOVE_DIG:
                mapHeader = Overworld_GetMapHeaderByGroupAndId(gSaveBlock1Ptr->escapeWarp.mapGroup, gSaveBlock1Ptr->escapeWarp.mapNum);
                GetMapNameGeneric(gStringVar1, mapHeader->regionMapSectionId);
                StringExpandPlaceholders(gStringVar4, gText_EscapeFromHere);
                DisplayFieldMoveExitAreaMessage(taskId);
                sPartyMenuInternal->data[0] = fieldMove;
                break;
            case FIELD_MOVE_FLY:
                gPartyMenu.exitCallback = CB2_OpenFlyMap;
                Task_ClosePartyMenu(taskId);
                break;
            default:
                gPartyMenu.exitCallback = CB2_ReturnToField;
                Task_ClosePartyMenu(taskId);
                break;
            }
        }
        // Cant use Field Move
        else
        {
            switch (fieldMove)
            {
            case FIELD_MOVE_SURF:
                DisplayCantUseSurfMessage();
                break;
            case FIELD_MOVE_FLASH:
                DisplayCantUseFlashMessage();
                break;
            default:
                DisplayPartyMenuStdMessage(sFieldMoveCursorCallbacks[fieldMove].msgId);
                break;
            }
            gTasks[taskId].func = Task_CancelAfterAorBPress;
        }
    }
}

static void DisplayFieldMoveExitAreaMessage(u8 taskId)
{
    DisplayPartyMenuMessage(gStringVar4, TRUE);
    gTasks[taskId].func = Task_FieldMoveExitAreaYesNo;
}

static void Task_FieldMoveExitAreaYesNo(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        PartyMenuDisplayYesNoMenu();
        gTasks[taskId].func = Task_HandleFieldMoveExitAreaYesNoInput;
    }
}

static void Task_HandleFieldMoveExitAreaYesNoInput(u8 taskId)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0:
        gPartyMenu.exitCallback = CB2_ReturnToField;
        Task_ClosePartyMenu(taskId);
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        // fallthrough
    case 1:
        gFieldCallback2 = NULL;
        gPostMenuFieldCallback = NULL;
        Task_ReturnToChooseMonAfterText(taskId);
        break;
    }
}

bool8 FieldCallback_PrepareFadeInFromMenu(void)
{
    FadeInFromBlack();
    CreateTask(Task_FieldMoveWaitForFade, 8);
    return TRUE;
}

static void Task_FieldMoveWaitForFade(u8 taskId)
{
    if (IsWeatherNotFadingIn() == TRUE)
    {
        gFieldEffectArguments[0] = GetFieldMoveMonSpecies();
        gPostMenuFieldCallback();
        DestroyTask(taskId);
    }
}

static u16 GetFieldMoveMonSpecies(void)
{
    return GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_SPECIES);
}

static void Task_CancelAfterAorBPress(u8 taskId)
{
    if ((JOY_NEW(A_BUTTON)) || (JOY_NEW(B_BUTTON)))
        CursorCb_Cancel1(taskId);
}

static void DisplayCantUseFlashMessage(void)
{
    if (FlagGet(FLAG_SYS_USE_FLASH) == TRUE)
        DisplayPartyMenuStdMessage(PARTY_MSG_ALREADY_IN_USE);
    else
        DisplayPartyMenuStdMessage(PARTY_MSG_CANT_USE_HERE);
}

static void FieldCallback_Surf(void)
{
    gFieldEffectArguments[0] = GetCursorSelectionMonId();
    FieldEffectStart(FLDEFF_USE_SURF);
}

static bool8 SetUpFieldMove_Surf(void)
{
    if (PartyHasMonWithSurf() == TRUE && IsPlayerFacingSurfableFishableWater() == TRUE)
    {
        gFieldCallback2 = FieldCallback_PrepareFadeInFromMenu;
        gPostMenuFieldCallback = FieldCallback_Surf;
        return TRUE;
    }
    return FALSE;
}

static void DisplayCantUseSurfMessage(void)
{
    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
        DisplayPartyMenuStdMessage(PARTY_MSG_ALREADY_SURFING);
    else
        DisplayPartyMenuStdMessage(PARTY_MSG_CANT_SURF_HERE);
}

static bool8 SetUpFieldMove_Fly(void)
{
    if (Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType) == TRUE)
        return TRUE;
    else
        return FALSE;
}

void CB2_ReturnToPartyMenuFromFlyMap(void)
{
    InitPartyMenu(PARTY_MENU_TYPE_FIELD, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_MON, TRUE, PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput, CB2_ReturnToFieldWithOpenMenu);
}

static void FieldCallback_Waterfall(void)
{
    gFieldEffectArguments[0] = GetCursorSelectionMonId();
    FieldEffectStart(FLDEFF_USE_WATERFALL);
}

static bool8 SetUpFieldMove_Waterfall(void)
{
    s16 x, y;

    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    if (MetatileBehavior_IsWaterfall(MapGridGetMetatileBehaviorAt(x, y)) == TRUE && IsPlayerSurfingNorth() == TRUE)
    {
        gFieldCallback2 = FieldCallback_PrepareFadeInFromMenu;
        gPostMenuFieldCallback = FieldCallback_Waterfall;
        return TRUE;
    }
    return FALSE;
}

static void FieldCallback_Dive(void)
{
    gFieldEffectArguments[0] = GetCursorSelectionMonId();
    FieldEffectStart(FLDEFF_USE_DIVE);
}

static bool8 SetUpFieldMove_Dive(void)
{
    gFieldEffectArguments[1] = TrySetDiveWarp();
    if (gFieldEffectArguments[1] != 0)
    {
        gFieldCallback2 = FieldCallback_PrepareFadeInFromMenu;
        gPostMenuFieldCallback = FieldCallback_Dive;
        return TRUE;
    }
    return FALSE;
}

static void CreatePartyMonIconSprite(struct Pokemon *mon, struct PartyMenuBox *menuBox, u32 slot)
{
    bool32 handleDeoxys = TRUE;
    u16 species2;

    // If in a multi battle, show partners Deoxys icon as Normal forme
    if (IsMultiBattle() == TRUE && gMain.inBattle)
        handleDeoxys = (sMultiBattlePartnersPartyMask[slot] ^ handleDeoxys) ? TRUE : FALSE;

    species2 = GetMonData(mon, MON_DATA_SPECIES_OR_EGG);
    CreatePartyMonIconSpriteParameterized(species2, GetMonData(mon, MON_DATA_PERSONALITY), menuBox, 1, handleDeoxys);
    UpdatePartyMonHPBar(menuBox->monSpriteId, mon);
}

static void CreatePartyMonIconSpriteParameterized(u16 species, u32 pid, struct PartyMenuBox *menuBox, u8 priority, bool32 handleDeoxys)
{
    if (species != SPECIES_NONE)
    {
        menuBox->monSpriteId = CreateMonIcon(species, SpriteCB_MonIcon, menuBox->spriteCoords[0], menuBox->spriteCoords[1], 4, pid, handleDeoxys);
        gSprites[menuBox->monSpriteId].oam.priority = priority;
    }
}

static void UpdateHPBar(u8 spriteId, u16 hp, u16 maxhp)
{
    switch (GetHPBarLevel(hp, maxhp))
    {
    case HP_BAR_FULL:
        SetPartyHPBarSprite(&gSprites[spriteId], 0);
        break;
    case HP_BAR_GREEN:
        SetPartyHPBarSprite(&gSprites[spriteId], 1);
        break;
    case HP_BAR_YELLOW:
        SetPartyHPBarSprite(&gSprites[spriteId], 2);
        break;
    case HP_BAR_RED:
        SetPartyHPBarSprite(&gSprites[spriteId], 3);
        break;
    default:
        SetPartyHPBarSprite(&gSprites[spriteId], 4);
        break;
    }
}

static void UpdatePartyMonHPBar(u8 spriteId, struct Pokemon *mon)
{
    UpdateHPBar(spriteId, GetMonData(mon, MON_DATA_HP), GetMonData(mon, MON_DATA_MAX_HP));
}

static void AnimateSelectedPartyIcon(u8 spriteId, u8 animNum)
{
    gSprites[spriteId].data[0] = 0;
    if (animNum == 0)
    {
        if (gSprites[spriteId].x == 16)
        {
            gSprites[spriteId].x2 = 0;
            gSprites[spriteId].y2 = -4;
        }
        else
        {
            gSprites[spriteId].x2 = -4;
            gSprites[spriteId].y2 = 0;
        }
        gSprites[spriteId].callback = SpriteCB_UpdatePartyMonIcon;
    }
    else
    {
        gSprites[spriteId].x2 = 0;
        gSprites[spriteId].y2 = 0;
        gSprites[spriteId].callback = SpriteCB_BouncePartyMonIcon;
    }
}

static void SpriteCB_BouncePartyMonIcon(struct Sprite *sprite)
{
    u8 animCmd = UpdateMonIconFrame(sprite);

    if (animCmd != 0)
    {
        if (animCmd & 1) // % 2 also matches
            sprite->y2 = -3;
        else
            sprite->y2 = 1;
    }
}

static void SpriteCB_UpdatePartyMonIcon(struct Sprite *sprite)
{
    UpdateMonIconFrame(sprite);
}

static void CreatePartyMonHeldItemSprite(struct Pokemon *mon, struct PartyMenuBox *menuBox)
{
    if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE)
    {
        menuBox->itemSpriteId = CreateSprite(&sSpriteTemplate_HeldItem, menuBox->spriteCoords[2], menuBox->spriteCoords[3], 0);
        UpdatePartyMonHeldItemSprite(mon, menuBox);
    }
}

static void CreatePartyMonHeldItemSpriteParameterized(u16 species, u16 item, struct PartyMenuBox *menuBox)
{
    if (species != SPECIES_NONE)
    {
        menuBox->itemSpriteId = CreateSprite(&sSpriteTemplate_HeldItem, menuBox->spriteCoords[2], menuBox->spriteCoords[3], 0);
        gSprites[menuBox->itemSpriteId].oam.priority = 0;
        ShowOrHideHeldItemSprite(item, menuBox);
    }
}

static void UpdatePartyMonHeldItemSprite(struct Pokemon *mon, struct PartyMenuBox *menuBox)
{
    ShowOrHideHeldItemSprite(GetMonData(mon, MON_DATA_HELD_ITEM), menuBox);
}

static void ShowOrHideHeldItemSprite(u16 item, struct PartyMenuBox *menuBox)
{
    if (item == ITEM_NONE)
    {
        gSprites[menuBox->itemSpriteId].invisible = TRUE;
    }
    else
    {
        if (ItemIsMail(item))
            StartSpriteAnim(&gSprites[menuBox->itemSpriteId], 1);
        else
            StartSpriteAnim(&gSprites[menuBox->itemSpriteId], 0);
        gSprites[menuBox->itemSpriteId].invisible = FALSE;
    }
}

void LoadHeldItemIcons(void)
{
    LoadSpriteSheet(&sSpriteSheet_HeldItem);
    LoadSpritePalette(&sSpritePalette_HeldItem);
}

void DrawHeldItemIconsForTrade(u8 *partyCounts, u8 *partySpriteIds, u8 whichParty)
{
    u16 i;
    u16 item;

    switch (whichParty)
    {
    case TRADE_PLAYER:
        for (i = 0; i < partyCounts[TRADE_PLAYER]; i++)
        {
            item = GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM);
            if (item != ITEM_NONE)
                CreateHeldItemSpriteForTrade(partySpriteIds[i], ItemIsMail(item));
        }
        break;
    case TRADE_PARTNER:
        for (i = 0; i < partyCounts[TRADE_PARTNER]; i++)
        {
            item = GetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM);
            if (item != ITEM_NONE)
                CreateHeldItemSpriteForTrade(partySpriteIds[i + PARTY_SIZE], ItemIsMail(item));
        }
        break;
    }
}

static void CreateHeldItemSpriteForTrade(u8 spriteId, bool8 isMail)
{
    u8 subpriority = gSprites[spriteId].subpriority;
    u8 newSpriteId = CreateSprite(&sSpriteTemplate_HeldItem, 250, 170, subpriority - 1);

    gSprites[newSpriteId].x2 = 4;
    gSprites[newSpriteId].y2 = 10;
    gSprites[newSpriteId].callback = SpriteCB_HeldItem;
    gSprites[newSpriteId].data[7] = spriteId;
    StartSpriteAnim(&gSprites[newSpriteId], isMail);
    gSprites[newSpriteId].callback(&gSprites[newSpriteId]);
}

static void SpriteCB_HeldItem(struct Sprite *sprite)
{
    u8 otherSpriteId = sprite->data[7];

    if (gSprites[otherSpriteId].invisible)
    {
        sprite->invisible = TRUE;
    }
    else
    {
        sprite->invisible = FALSE;
        sprite->x = gSprites[otherSpriteId].x + gSprites[otherSpriteId].x2;
        sprite->y = gSprites[otherSpriteId].y + gSprites[otherSpriteId].y2;
    }
}

static void CreatePartyMonPokeballSprite(struct Pokemon *mon, struct PartyMenuBox *menuBox)
{
    if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE)
        menuBox->pokeballSpriteId = CreateSprite(&sSpriteTemplate_MenuPokeball, menuBox->spriteCoords[6], menuBox->spriteCoords[7], 8);
}

static void CreatePartyMonPokeballSpriteParameterized(u16 species, struct PartyMenuBox *menuBox)
{
    if (species != SPECIES_NONE)
    {
        menuBox->pokeballSpriteId = CreateSprite(&sSpriteTemplate_MenuPokeball, menuBox->spriteCoords[6], menuBox->spriteCoords[7], 8);
        gSprites[menuBox->pokeballSpriteId].oam.priority = 0;
    }
}

// For Cancel when Confirm isnt present
static u8 CreatePokeballButtonSprite(u8 x, u8 y)
{
    u8 spriteId = CreateSprite(&sSpriteTemplate_MenuPokeball, x, y, 8);

    gSprites[spriteId].oam.priority = 2;
    return spriteId;
}

// For Confirm and Cancel when both are present
static u8 CreateSmallPokeballButtonSprite(u8 x, u8 y)
{
    return CreateSprite(&sSpriteTemplate_MenuPokeballSmall, x, y, 8);
}

static void PartyMenuStartSpriteAnim(u8 spriteId, u8 animNum)
{
    StartSpriteAnim(&gSprites[spriteId], animNum);
}

// Might explain the large blank section in gPartyMenuPokeballSmall_Gfx
// At the very least this is how the unused anim cmds for sSpriteAnimTable_MenuPokeballSmall were meant to be accessed
static void UNUSED SpriteCB_BounceConfirmCancelButton(u8 spriteId, u8 spriteId2, u8 animNum)
{
    if (animNum == 0)
    {
        StartSpriteAnim(&gSprites[spriteId], 2);
        StartSpriteAnim(&gSprites[spriteId2], 4);
        gSprites[spriteId].y2 = 0;
        gSprites[spriteId2].y2 = 0;
    }
    else
    {
        StartSpriteAnim(&gSprites[spriteId], 3);
        StartSpriteAnim(&gSprites[spriteId2], 5);
        gSprites[spriteId].y2 = -4;
        gSprites[spriteId2].y2 = 4;
    }
}

static void LoadPartyMenuPokeballGfx(void)
{
    LoadCompressedSpriteSheet(&sSpriteSheet_MenuPokeball);
    LoadCompressedSpriteSheet(&sSpriteSheet_MenuPokeballSmall);
    LoadCompressedSpritePalette(&sSpritePalette_MenuPokeball);
}

static void CreatePartyMonStatusSprite(struct Pokemon *mon, struct PartyMenuBox *menuBox)
{
    if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE)
    {
        menuBox->statusSpriteId = CreateSprite(&sSpriteTemplate_StatusIcons, menuBox->spriteCoords[4], menuBox->spriteCoords[5], 0);
        SetPartyMonAilmentGfx(mon, menuBox);
    }
}

static void CreatePartyMonStatusSpriteParameterized(u16 species, u8 status, struct PartyMenuBox *menuBox)
{
    if (species != SPECIES_NONE)
    {
        menuBox->statusSpriteId = CreateSprite(&sSpriteTemplate_StatusIcons, menuBox->spriteCoords[4], menuBox->spriteCoords[5], 0);
        UpdatePartyMonAilmentGfx(status, menuBox);
        gSprites[menuBox->statusSpriteId].oam.priority = 0;
    }
}

static void SetPartyMonAilmentGfx(struct Pokemon *mon, struct PartyMenuBox *menuBox)
{
    UpdatePartyMonAilmentGfx(GetMonAilment(mon), menuBox);
}

static void UpdatePartyMonAilmentGfx(u8 status, struct PartyMenuBox *menuBox)
{
    switch (status)
    {
    case AILMENT_NONE:
    case AILMENT_PKRS:
        gSprites[menuBox->statusSpriteId].invisible = TRUE;
        break;
    default:
        StartSpriteAnim(&gSprites[menuBox->statusSpriteId], status - 1);
        gSprites[menuBox->statusSpriteId].invisible = FALSE;
        break;
    }
}

static void LoadPartyMenuAilmentGfx(void)
{
    LoadCompressedSpriteSheet(&sSpriteSheet_StatusIcons);
    LoadCompressedSpritePalette(&sSpritePalette_StatusIcons);
}

void CB2_ShowPartyMenuForItemUse(void)
{
    MainCallback callback = CB2_ReturnToBagMenu;
    u8 partyLayout;
    u8 menuType;
    u8 i;
    u8 msgId;
    TaskFunc task;

    if (gMain.inBattle)
    {
        menuType = PARTY_MENU_TYPE_IN_BATTLE;
        partyLayout = GetPartyLayoutFromBattleType();
    }
    else
    {
        menuType = PARTY_MENU_TYPE_FIELD;
        partyLayout = PARTY_LAYOUT_SINGLE;
    }

    if (GetItemEffectType(gSpecialVar_ItemId) == ITEM_EFFECT_SACRED_ASH)
    {
        gPartyMenu.slotId = 0;
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) != SPECIES_NONE && GetMonData(&gPlayerParty[i], MON_DATA_HP) == 0)
            {
                gPartyMenu.slotId = i;
                break;
            }
        }
        task = Task_SetSacredAshCB;
        msgId = PARTY_MSG_NONE;
    }
    else
    {
        if (GetPocketByItemId(gSpecialVar_ItemId) == POCKET_TM_HM)
            msgId = PARTY_MSG_TEACH_WHICH_MON;
        else
            msgId = PARTY_MSG_USE_ON_WHICH_MON;

        task = Task_HandleChooseMonInput;
    }

    InitPartyMenu(menuType, partyLayout, PARTY_ACTION_USE_ITEM, TRUE, msgId, task, callback);
}

static void CB2_ReturnToBagMenu(void)
{
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE == FALSE)
        GoToBagMenu(ITEMMENULOCATION_LAST, POCKETS_COUNT, NULL);
    else
        GoToBattlePyramidBagMenu(PYRAMIDBAG_LOC_PREV, gPyramidBagMenuState.exitCallback);
}

static void Task_SetSacredAshCB(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        if (gPartyMenu.menuType == PARTY_MENU_TYPE_IN_BATTLE)
            sPartyMenuInternal->exitCallback = CB2_SetUpExitToBattleScreen;
        gItemUseCB(taskId, Task_ClosePartyMenuAfterText); // ItemUseCB_SacredAsh in this case
    }
}

static bool8 IsHPRecoveryItem(u16 item)
{
    const u8 *effect;

    if (item == ITEM_ENIGMA_BERRY)
        effect = gSaveBlock1Ptr->enigmaBerry.itemEffect;
    else
        effect = gItemEffectTable[item - ITEM_POTION];

    if (effect[4] & ITEM4_HEAL_HP)
        return TRUE;
    else
        return FALSE;
}

static void GetMedicineItemEffectMessage(u16 item)
{
    switch (GetItemEffectType(item))
    {
    case ITEM_EFFECT_CURE_POISON:
        StringExpandPlaceholders(gStringVar4, gText_PkmnCuredOfPoison);
        break;
    case ITEM_EFFECT_CURE_SLEEP:
        StringExpandPlaceholders(gStringVar4, gText_PkmnWokeUp2);
        break;
    case ITEM_EFFECT_CURE_BURN:
        StringExpandPlaceholders(gStringVar4, gText_PkmnBurnHealed);
        break;
    case ITEM_EFFECT_CURE_FREEZE:
        StringExpandPlaceholders(gStringVar4, gText_PkmnThawedOut);
        break;
    case ITEM_EFFECT_CURE_PARALYSIS:
        StringExpandPlaceholders(gStringVar4, gText_PkmnCuredOfParalysis);
        break;
    case ITEM_EFFECT_CURE_CONFUSION:
        StringExpandPlaceholders(gStringVar4, gText_PkmnSnappedOutOfConfusion);
        break;
    case ITEM_EFFECT_CURE_INFATUATION:
        StringExpandPlaceholders(gStringVar4, gText_PkmnGotOverInfatuation);
        break;
    case ITEM_EFFECT_CURE_ALL_STATUS:
        StringExpandPlaceholders(gStringVar4, gText_PkmnBecameHealthy);
        break;
    case ITEM_EFFECT_HP_EV:
        StringCopy(gStringVar2, gText_HP3);
        StringExpandPlaceholders(gStringVar4, gText_PkmnBaseVar2StatIncreased);
        break;
    case ITEM_EFFECT_ATK_EV:
        StringCopy(gStringVar2, gText_Attack3);
        StringExpandPlaceholders(gStringVar4, gText_PkmnBaseVar2StatIncreased);
        break;
    case ITEM_EFFECT_DEF_EV:
        StringCopy(gStringVar2, gText_Defense3);
        StringExpandPlaceholders(gStringVar4, gText_PkmnBaseVar2StatIncreased);
        break;
    case ITEM_EFFECT_SPEED_EV:
        StringCopy(gStringVar2, gText_Speed2);
        StringExpandPlaceholders(gStringVar4, gText_PkmnBaseVar2StatIncreased);
        break;
    case ITEM_EFFECT_SPATK_EV:
        StringCopy(gStringVar2, gText_SpAtk3);
        StringExpandPlaceholders(gStringVar4, gText_PkmnBaseVar2StatIncreased);
        break;
    case ITEM_EFFECT_SPDEF_EV:
        StringCopy(gStringVar2, gText_SpDef3);
        StringExpandPlaceholders(gStringVar4, gText_PkmnBaseVar2StatIncreased);
        break;
    case ITEM_EFFECT_PP_UP:
    case ITEM_EFFECT_PP_MAX:
        StringExpandPlaceholders(gStringVar4, gText_MovesPPIncreased);
        break;
    case ITEM_EFFECT_HEAL_PP:
        StringExpandPlaceholders(gStringVar4, gText_PPWasRestored);
        break;
    default:
        StringExpandPlaceholders(gStringVar4, gText_WontHaveEffect);
        break;
    }
}

static bool8 NotUsingHPEVItemOnShedinja(struct Pokemon *mon, u16 item)
{
    if (GetItemEffectType(item) == ITEM_EFFECT_HP_EV && GetMonData(mon, MON_DATA_SPECIES) == SPECIES_SHEDINJA)
        return FALSE;
    return TRUE;
}

static bool8 IsItemFlute(u16 item)
{
    if (item == ITEM_BLUE_FLUTE || item == ITEM_RED_FLUTE || item == ITEM_YELLOW_FLUTE)
        return TRUE;
    return FALSE;
}

static bool8 ExecuteTableBasedItemEffect_(u8 partyMonIndex, u16 item, u8 monMoveIndex)
{
    if (gMain.inBattle)
        return ExecuteTableBasedItemEffect(&gPlayerParty[partyMonIndex], item, GetPartyIdFromBattleSlot(partyMonIndex), monMoveIndex);
    else
        return ExecuteTableBasedItemEffect(&gPlayerParty[partyMonIndex], item, partyMonIndex, monMoveIndex);
}

void ItemUseCB_Medicine(u8 taskId, TaskFunc task)
{
    u16 hp = 0;
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    u16 item = gSpecialVar_ItemId;
    bool8 canHeal, cannotUse;

    if (NotUsingHPEVItemOnShedinja(mon, item) == FALSE)
    {
        cannotUse = TRUE;
    }
    else
    {
        canHeal = IsHPRecoveryItem(item);
        if (canHeal == TRUE)
        {
            hp = GetMonData(mon, MON_DATA_HP);
            if (hp == GetMonData(mon, MON_DATA_MAX_HP))
                canHeal = FALSE;
        }
        cannotUse = ExecuteTableBasedItemEffect_(gPartyMenu.slotId, item, 0);
    }

    if (cannotUse != FALSE)
    {
        gPartyMenuUseExitCallback = FALSE;
        PlaySE(SE_SELECT);
        DisplayPartyMenuMessage(gText_WontHaveEffect, TRUE);
        ScheduleBgCopyTilemapToVram(2);
        gTasks[taskId].func = task;
    }
    else
    {
        gPartyMenuUseExitCallback = TRUE;
        if (!IsItemFlute(item))
        {
            PlaySE(SE_USE_ITEM);
            if (gPartyMenu.action != PARTY_ACTION_REUSABLE_ITEM)
                RemoveBagItem(item, 1);
        }
        else
        {
            PlaySE(SE_GLASS_FLUTE);
        }
        SetPartyMonAilmentGfx(mon, &sPartyMenuBoxes[gPartyMenu.slotId]);
        if (gSprites[sPartyMenuBoxes[gPartyMenu.slotId].statusSpriteId].invisible)
            DisplayPartyPokemonLevelCheck(mon, &sPartyMenuBoxes[gPartyMenu.slotId], 1);
        if (canHeal == TRUE)
        {
            if (hp == 0)
                AnimatePartySlot(gPartyMenu.slotId, 1);
            PartyMenuModifyHP(taskId, gPartyMenu.slotId, 1, GetMonData(mon, MON_DATA_HP) - hp, Task_DisplayHPRestoredMessage);
            ResetHPTaskData(taskId, 0, hp);
            return;
        }
        else
        {
            GetMonNickname(mon, gStringVar1);
            GetMedicineItemEffectMessage(item);
            DisplayPartyMenuMessage(gStringVar4, TRUE);
            ScheduleBgCopyTilemapToVram(2);
            gTasks[taskId].func = task;
        }
    }
}

static void Task_DisplayHPRestoredMessage(u8 taskId)
{
    GetMonNickname(&gPlayerParty[gPartyMenu.slotId], gStringVar1);
    StringExpandPlaceholders(gStringVar4, gText_PkmnHPRestoredByVar2);
    DisplayPartyMenuMessage(gStringVar4, FALSE);
    ScheduleBgCopyTilemapToVram(2);
    HandleBattleLowHpMusicChange();
    gTasks[taskId].func = Task_ClosePartyMenuAfterText;
}

static void Task_ClosePartyMenuAfterText(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        if (gPartyMenuUseExitCallback == FALSE)
            sPartyMenuInternal->exitCallback = NULL;
        Task_ClosePartyMenu(taskId);
    }
}

void ItemUseCB_ReduceEV(u8 taskId, TaskFunc task)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    u16 item = gSpecialVar_ItemId;
    u8 effectType = GetItemEffectType(item);
    u16 friendship = GetMonData(mon, MON_DATA_FRIENDSHIP);
    u16 ev = ItemEffectToMonEv(mon, effectType);
    bool8 cannotUseEffect = ExecuteTableBasedItemEffect_(gPartyMenu.slotId, item, 0);
    u16 newFriendship = GetMonData(mon, MON_DATA_FRIENDSHIP);
    u16 newEv = ItemEffectToMonEv(mon, effectType);

    if (cannotUseEffect || (friendship == newFriendship && ev == newEv))
    {
        gPartyMenuUseExitCallback = FALSE;
        PlaySE(SE_SELECT);
        DisplayPartyMenuMessage(gText_WontHaveEffect, TRUE);
        ScheduleBgCopyTilemapToVram(2);
        gTasks[taskId].func = task;
    }
    else
    {
        gPartyMenuUseExitCallback = TRUE;
        PlaySE(SE_USE_ITEM);
        RemoveBagItem(item, 1);
        GetMonNickname(mon, gStringVar1);
        ItemEffectToStatString(effectType, gStringVar2);
        if (friendship != newFriendship)
        {
            if (ev != newEv)
                StringExpandPlaceholders(gStringVar4, gText_PkmnFriendlyBaseVar2Fell);
            else
                StringExpandPlaceholders(gStringVar4, gText_PkmnFriendlyBaseVar2CantFall);
        }
        else
        {
            StringExpandPlaceholders(gStringVar4, gText_PkmnAdoresBaseVar2Fell);
        }
        DisplayPartyMenuMessage(gStringVar4, TRUE);
        ScheduleBgCopyTilemapToVram(2);
        gTasks[taskId].func = task;
    }
}

static u16 ItemEffectToMonEv(struct Pokemon *mon, u8 effectType)
{
    switch (effectType)
    {
    case ITEM_EFFECT_HP_EV:
        if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_SHEDINJA)
            return GetMonData(mon, MON_DATA_HP_EV);
        break;
    case ITEM_EFFECT_ATK_EV:
        return GetMonData(mon, MON_DATA_ATK_EV);
    case ITEM_EFFECT_DEF_EV:
        return GetMonData(mon, MON_DATA_DEF_EV);
    case ITEM_EFFECT_SPEED_EV:
        return GetMonData(mon, MON_DATA_SPEED_EV);
    case ITEM_EFFECT_SPATK_EV:
        return GetMonData(mon, MON_DATA_SPATK_EV);
    case ITEM_EFFECT_SPDEF_EV:
        return GetMonData(mon, MON_DATA_SPDEF_EV);
    }
    return 0;
}

static void ItemEffectToStatString(u8 effectType, u8 *dest)
{
    switch (effectType)
    {
    case ITEM_EFFECT_HP_EV:
        StringCopy(dest, gText_HP3);
        break;
    case ITEM_EFFECT_ATK_EV:
        StringCopy(dest, gText_Attack3);
        break;
    case ITEM_EFFECT_DEF_EV:
        StringCopy(dest, gText_Defense3);
        break;
    case ITEM_EFFECT_SPEED_EV:
        StringCopy(dest, gText_Speed2);
        break;
    case ITEM_EFFECT_SPATK_EV:
        StringCopy(dest, gText_SpAtk3);
        break;
    case ITEM_EFFECT_SPDEF_EV:
        StringCopy(dest, gText_SpDef3);
        break;
    }
}

static void ShowMoveSelectWindow(u8 slot)
{
    u8 i;
    u8 moveCount = 0;
    u8 fontId = FONT_NORMAL;
    u8 windowId = DisplaySelectionWindow(SELECTWINDOW_MOVES);
    u16 move;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        move = GetMonData(&gPlayerParty[slot], MON_DATA_MOVE1 + i);
        AddTextPrinterParameterized(windowId, fontId, gMoveNames[move], 8, (i * 16) + 1, TEXT_SKIP_DRAW, NULL);
        if (move != MOVE_NONE)
            moveCount++;
    }
    InitMenuInUpperLeftCornerNormal(windowId, moveCount, 0);
    ScheduleBgCopyTilemapToVram(2);
}

static void Task_HandleWhichMoveInput(u8 taskId)
{
    s8 input = Menu_ProcessInput();

    if (input != MENU_NOTHING_CHOSEN)
    {
        if (input == MENU_B_PRESSED)
        {
            PlaySE(SE_SELECT);
            ReturnToUseOnWhichMon(taskId);
        }
        else
        {
            PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
            SetSelectedMoveForPPItem(taskId);
        }
    }
}

void ItemUseCB_PPRecovery(u8 taskId, TaskFunc task)
{
    const u8 *effect;
    u16 item = gSpecialVar_ItemId;

    if (item == ITEM_ENIGMA_BERRY)
        effect = gSaveBlock1Ptr->enigmaBerry.itemEffect;
    else
        effect = gItemEffectTable[item - ITEM_POTION];

    if (!(effect[4] & ITEM4_HEAL_PP_ONE))
    {
        gPartyMenu.data1 = 0;
        TryUsePPItem(taskId);
    }
    else
    {
        PlaySE(SE_SELECT);
        DisplayPartyMenuStdMessage(PARTY_MSG_RESTORE_WHICH_MOVE);
        ShowMoveSelectWindow(gPartyMenu.slotId);
        gTasks[taskId].func = Task_HandleWhichMoveInput;
    }
}

static void SetSelectedMoveForPPItem(u8 taskId)
{
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    gPartyMenu.data1 = Menu_GetCursorPos();
    TryUsePPItem(taskId);
}

static void ReturnToUseOnWhichMon(u8 taskId)
{
    gTasks[taskId].func = Task_HandleChooseMonInput;
    sPartyMenuInternal->exitCallback = NULL;
    PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
    DisplayPartyMenuStdMessage(PARTY_MSG_USE_ON_WHICH_MON);
}

static void TryUsePPItem(u8 taskId)
{
    u16 move = MOVE_NONE;
    s16 *moveSlot = &gPartyMenu.data1;
    u16 item = gSpecialVar_ItemId;
    struct PartyMenu *ptr = &gPartyMenu;
    struct Pokemon *mon;

    if (ExecuteTableBasedItemEffect_(ptr->slotId, item, *moveSlot))
    {
        gPartyMenuUseExitCallback = FALSE;
        PlaySE(SE_SELECT);
        DisplayPartyMenuMessage(gText_WontHaveEffect, TRUE);
        ScheduleBgCopyTilemapToVram(2);
        gTasks[taskId].func = Task_ClosePartyMenuAfterText;
    }
    else
    {
        gPartyMenuUseExitCallback = TRUE;
        mon = &gPlayerParty[ptr->slotId];
        PlaySE(SE_USE_ITEM);
        RemoveBagItem(item, 1);
        move = GetMonData(mon, MON_DATA_MOVE1 + *moveSlot);
        StringCopy(gStringVar1, gMoveNames[move]);
        GetMedicineItemEffectMessage(item);
        DisplayPartyMenuMessage(gStringVar4, TRUE);
        ScheduleBgCopyTilemapToVram(2);
        gTasks[taskId].func = Task_ClosePartyMenuAfterText;
    }
}

void ItemUseCB_PPUp(u8 taskId, TaskFunc task)
{
    PlaySE(SE_SELECT);
    DisplayPartyMenuStdMessage(PARTY_MSG_BOOST_PP_WHICH_MOVE);
    ShowMoveSelectWindow(gPartyMenu.slotId);
    gTasks[taskId].func = Task_HandleWhichMoveInput;
}

u16 ItemIdToBattleMoveId(u16 item)
{
    u16 tmNumber = item - ITEM_TM01;
    return sTMHMMoves[tmNumber];
}

bool8 IsMoveHm(u16 move)
{
    u8 i;

    for (i = 0; i < NUM_HIDDEN_MACHINES; i++)
    {
        if (sTMHMMoves[i + NUM_TECHNICAL_MACHINES] == move)
            return TRUE;
    }
    return FALSE;
}

bool8 MonKnowsMove(struct Pokemon *mon, u16 move)
{
    u8 i;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (GetMonData(mon, MON_DATA_MOVE1 + i) == move)
            return TRUE;
    }
    return FALSE;
}

static void DisplayLearnMoveMessage(const u8 *str)
{
    StringExpandPlaceholders(gStringVar4, str);
    DisplayPartyMenuMessage(gStringVar4, TRUE);
    ScheduleBgCopyTilemapToVram(2);
}

static void DisplayLearnMoveMessageAndClose(u8 taskId, const u8 *str)
{
    DisplayLearnMoveMessage(str);
    gTasks[taskId].func = Task_ClosePartyMenuAfterText;
}

// move[1] doesn't use constants cause I don't know if it's actually a move ID storage

void ItemUseCB_TMHM(u8 taskId, TaskFunc task)
{
    struct Pokemon *mon;
    s16 *move;
    u16 item;

    PlaySE(SE_SELECT);
    mon = &gPlayerParty[gPartyMenu.slotId];
    move = &gPartyMenu.data1;
    item = gSpecialVar_ItemId;
    GetMonNickname(mon, gStringVar1);
    move[0] = ItemIdToBattleMoveId(item);
    StringCopy(gStringVar2, gMoveNames[move[0]]);
    move[1] = 0;

    switch (CanMonLearnTMTutor(mon, item, 0))
    {
    case CANNOT_LEARN_MOVE:
        DisplayLearnMoveMessageAndClose(taskId, gText_PkmnCantLearnMove);
        return;
    case ALREADY_KNOWS_MOVE:
        DisplayLearnMoveMessageAndClose(taskId, gText_PkmnAlreadyKnows);
        return;
    }

    if (GiveMoveToMon(mon, move[0]) != MON_HAS_MAX_MOVES)
    {
        gTasks[taskId].func = Task_LearnedMove;
    }
    else
    {
        DisplayLearnMoveMessage(gText_PkmnNeedsToReplaceMove);
        gTasks[taskId].func = Task_ReplaceMoveYesNo;
    }
}

static void Task_LearnedMove(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    s16 *move = &gPartyMenu.data1;
    u16 item = gSpecialVar_ItemId;

    if (move[1] == 0)
    {
        AdjustFriendship(mon, FRIENDSHIP_EVENT_LEARN_TMHM);
        if (item < ITEM_HM01)
            RemoveBagItem(item, 1);
    }
    GetMonNickname(mon, gStringVar1);
    StringCopy(gStringVar2, gMoveNames[move[0]]);
    StringExpandPlaceholders(gStringVar4, gText_PkmnLearnedMove3);
    DisplayPartyMenuMessage(gStringVar4, TRUE);
    ScheduleBgCopyTilemapToVram(2);
    gTasks[taskId].func = Task_DoLearnedMoveFanfareAfterText;
}

static void Task_DoLearnedMoveFanfareAfterText(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        PlayFanfare(MUS_LEVEL_UP);
        gTasks[taskId].func = Task_LearnNextMoveOrClosePartyMenu;
    }
}

static void Task_LearnNextMoveOrClosePartyMenu(u8 taskId)
{
    if (IsFanfareTaskInactive() && ((JOY_NEW(A_BUTTON)) || (JOY_NEW(B_BUTTON))))
    {
        if (gPartyMenu.learnMoveState == 1)
        {
            Task_TryLearningNextMove(taskId);
        }
        else
        {
            if (gPartyMenu.learnMoveState == 2) // never occurs
                gSpecialVar_Result = TRUE;
            Task_ClosePartyMenu(taskId);
        }
    }
}

static void Task_ReplaceMoveYesNo(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        PartyMenuDisplayYesNoMenu();
        gTasks[taskId].func = Task_HandleReplaceMoveYesNoInput;
    }
}

static void Task_HandleReplaceMoveYesNoInput(u8 taskId)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0:
        DisplayPartyMenuMessage(gText_WhichMoveToForget, TRUE);
        gTasks[taskId].func = Task_ShowSummaryScreenToForgetMove;
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        // fallthrough
    case 1:
        StopLearningMovePrompt(taskId);
        break;
    }
}

static void Task_ShowSummaryScreenToForgetMove(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        sPartyMenuInternal->exitCallback = CB2_ShowSummaryScreenToForgetMove;
        Task_ClosePartyMenu(taskId);
    }
}

static void CB2_ShowSummaryScreenToForgetMove(void)
{
    ShowSelectMovePokemonSummaryScreen(gPlayerParty, gPartyMenu.slotId, gPlayerPartyCount - 1, CB2_ReturnToPartyMenuWhileLearningMove, gPartyMenu.data1);
}

static void CB2_ReturnToPartyMenuWhileLearningMove(void)
{
    InitPartyMenu(PARTY_MENU_TYPE_FIELD, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_MON, TRUE, PARTY_MSG_NONE, Task_ReturnToPartyMenuWhileLearningMove, gPartyMenu.exitCallback);
}

static void Task_ReturnToPartyMenuWhileLearningMove(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        if (GetMoveSlotToReplace() != MAX_MON_MOVES)
            DisplayPartyMenuForgotMoveMessage(taskId);
        else
            StopLearningMovePrompt(taskId);
    }
}

static void DisplayPartyMenuForgotMoveMessage(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    u16 move = GetMonData(mon, MON_DATA_MOVE1 + GetMoveSlotToReplace());

    GetMonNickname(mon, gStringVar1);
    StringCopy(gStringVar2, gMoveNames[move]);
    DisplayLearnMoveMessage(gText_12PoofForgotMove);
    gTasks[taskId].func = Task_PartyMenuReplaceMove;
}

static void Task_PartyMenuReplaceMove(u8 taskId)
{
    struct Pokemon *mon;
    u16 move;

    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        mon = &gPlayerParty[gPartyMenu.slotId];
        RemoveMonPPBonus(mon, GetMoveSlotToReplace());
        move = gPartyMenu.data1;
        SetMonMoveSlot(mon, move, GetMoveSlotToReplace());
        Task_LearnedMove(taskId);
    }
}

static void StopLearningMovePrompt(u8 taskId)
{
    StringCopy(gStringVar2, gMoveNames[gPartyMenu.data1]);
    StringExpandPlaceholders(gStringVar4, gText_StopLearningMove2);
    DisplayPartyMenuMessage(gStringVar4, TRUE);
    ScheduleBgCopyTilemapToVram(2);
    gTasks[taskId].func = Task_StopLearningMoveYesNo;
}

static void Task_StopLearningMoveYesNo(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        PartyMenuDisplayYesNoMenu();
        gTasks[taskId].func = Task_HandleStopLearningMoveYesNoInput;
    }
}

static void Task_HandleStopLearningMoveYesNoInput(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];

    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0:
        GetMonNickname(mon, gStringVar1);
        StringCopy(gStringVar2, gMoveNames[gPartyMenu.data1]);
        StringExpandPlaceholders(gStringVar4, gText_MoveNotLearned);
        DisplayPartyMenuMessage(gStringVar4, TRUE);
        if (gPartyMenu.learnMoveState == 1)
        {
            gTasks[taskId].func = Task_TryLearningNextMoveAfterText;
        }
        else
        {
            if (gPartyMenu.learnMoveState == 2) // never occurs
                gSpecialVar_Result = FALSE;
            gTasks[taskId].func = Task_ClosePartyMenuAfterText;
        }
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        // fallthrough
    case 1:
        GetMonNickname(mon, gStringVar1);
        StringCopy(gStringVar2, gMoveNames[gPartyMenu.data1]);
        DisplayLearnMoveMessage(gText_PkmnNeedsToReplaceMove);
        gTasks[taskId].func = Task_ReplaceMoveYesNo;
        break;
    }
}

static void Task_TryLearningNextMoveAfterText(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
        Task_TryLearningNextMove(taskId);
}

void ItemUseCB_RareCandy(u8 taskId, TaskFunc task)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    struct PartyMenuInternal *ptr = sPartyMenuInternal;
    s16 *arrayPtr = ptr->data;
    u16 *itemPtr = &gSpecialVar_ItemId;
    bool8 cannotUseEffect;

    if (GetMonData(mon, MON_DATA_LEVEL) != MAX_LEVEL)
    {
        BufferMonStatsToTaskData(mon, arrayPtr);
        cannotUseEffect = ExecuteTableBasedItemEffect_(gPartyMenu.slotId, *itemPtr, 0);
        BufferMonStatsToTaskData(mon, &ptr->data[NUM_STATS]);
    }
    else
    {
        cannotUseEffect = TRUE;
    }
    PlaySE(SE_SELECT);
    if (cannotUseEffect)
    {
        gPartyMenuUseExitCallback = FALSE;
        DisplayPartyMenuMessage(gText_WontHaveEffect, TRUE);
        ScheduleBgCopyTilemapToVram(2);
        gTasks[taskId].func = task;
    }
    else
    {
        gPartyMenuUseExitCallback = TRUE;
        PlayFanfareByFanfareNum(FANFARE_LEVEL_UP);
        UpdateMonDisplayInfoAfterRareCandy(gPartyMenu.slotId, mon);
        RemoveBagItem(gSpecialVar_ItemId, 1);
        GetMonNickname(mon, gStringVar1);
        ConvertIntToDecimalStringN(gStringVar2, GetMonData(mon, MON_DATA_LEVEL), STR_CONV_MODE_LEFT_ALIGN, 3);
        StringExpandPlaceholders(gStringVar4, gText_PkmnElevatedToLvVar2);
        DisplayPartyMenuMessage(gStringVar4, TRUE);
        ScheduleBgCopyTilemapToVram(2);
        gTasks[taskId].func = Task_DisplayLevelUpStatsPg1;
    }
}

static void UpdateMonDisplayInfoAfterRareCandy(u8 slot, struct Pokemon *mon)
{
    SetPartyMonAilmentGfx(mon, &sPartyMenuBoxes[slot]);
    if (gSprites[sPartyMenuBoxes[slot].statusSpriteId].invisible)
        DisplayPartyPokemonLevelCheck(mon, &sPartyMenuBoxes[slot], 1);
    DisplayPartyPokemonHPCheck(mon, &sPartyMenuBoxes[slot], 1);
    DisplayPartyPokemonMaxHPCheck(mon, &sPartyMenuBoxes[slot], 1);
    DisplayPartyPokemonHPBarCheck(mon, &sPartyMenuBoxes[slot]);
    UpdatePartyMonHPBar(sPartyMenuBoxes[slot].monSpriteId, mon);
    AnimatePartySlot(slot, 1);
    ScheduleBgCopyTilemapToVram(0);
}

static void Task_DisplayLevelUpStatsPg1(u8 taskId)
{
    if (WaitFanfare(FALSE) && IsPartyMenuTextPrinterActive() != TRUE && ((JOY_NEW(A_BUTTON)) || (JOY_NEW(B_BUTTON))))
    {
        PlaySE(SE_SELECT);
        DisplayLevelUpStatsPg1(taskId);
        gTasks[taskId].func = Task_DisplayLevelUpStatsPg2;
    }
}

static void Task_DisplayLevelUpStatsPg2(u8 taskId)
{
    if ((JOY_NEW(A_BUTTON)) || (JOY_NEW(B_BUTTON)))
    {
        PlaySE(SE_SELECT);
        DisplayLevelUpStatsPg2(taskId);
        gTasks[taskId].func = Task_TryLearnNewMoves;
    }
}

static void DisplayLevelUpStatsPg1(u8 taskId)
{
    s16 *arrayPtr = sPartyMenuInternal->data;

    arrayPtr[12] = CreateLevelUpStatsWindow();
    DrawLevelUpWindowPg1(arrayPtr[12], arrayPtr, &arrayPtr[6], TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY);
    CopyWindowToVram(arrayPtr[12], COPYWIN_GFX);
    ScheduleBgCopyTilemapToVram(2);
}

static void DisplayLevelUpStatsPg2(u8 taskId)
{
    s16 *arrayPtr = sPartyMenuInternal->data;

    DrawLevelUpWindowPg2(arrayPtr[12], &arrayPtr[6], TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY);
    CopyWindowToVram(arrayPtr[12], COPYWIN_GFX);
    ScheduleBgCopyTilemapToVram(2);
}

static void Task_TryLearnNewMoves(u8 taskId)
{
    u16 learnMove;

    if (WaitFanfare(FALSE) && ((JOY_NEW(A_BUTTON)) || (JOY_NEW(B_BUTTON))))
    {
        RemoveLevelUpStatsWindow();
        learnMove = MonTryLearningNewMove(&gPlayerParty[gPartyMenu.slotId], TRUE);
        gPartyMenu.learnMoveState = 1;
        switch (learnMove)
        {
        case 0: // No moves to learn
            PartyMenuTryEvolution(taskId);
            break;
        case MON_HAS_MAX_MOVES:
            DisplayMonNeedsToReplaceMove(taskId);
            break;
        case MON_ALREADY_KNOWS_MOVE:
            gTasks[taskId].func = Task_TryLearningNextMove;
            break;
        default:
            DisplayMonLearnedMove(taskId, learnMove);
            break;
        }
    }
}

static void Task_TryLearningNextMove(u8 taskId)
{
    u16 result = MonTryLearningNewMove(&gPlayerParty[gPartyMenu.slotId], FALSE);

    switch (result)
    {
    case 0: // No moves to learn
        PartyMenuTryEvolution(taskId);
        break;
    case MON_HAS_MAX_MOVES:
        DisplayMonNeedsToReplaceMove(taskId);
        break;
    case MON_ALREADY_KNOWS_MOVE:
        return;
    default:
        DisplayMonLearnedMove(taskId, result);
        break;
    }
}

static void PartyMenuTryEvolution(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    u16 targetSpecies = GetEvolutionTargetSpecies(mon, EVO_MODE_NORMAL, ITEM_NONE);

    if (targetSpecies != SPECIES_NONE)
    {
        FreePartyPointers();
        gCB2_AfterEvolution = gPartyMenu.exitCallback;
        BeginEvolutionScene(mon, targetSpecies, TRUE, gPartyMenu.slotId);
        DestroyTask(taskId);
    }
    else
    {
        gTasks[taskId].func = Task_ClosePartyMenuAfterText;
    }
}

static void DisplayMonNeedsToReplaceMove(u8 taskId)
{
    GetMonNickname(&gPlayerParty[gPartyMenu.slotId], gStringVar1);
    StringCopy(gStringVar2, gMoveNames[gMoveToLearn]);
    StringExpandPlaceholders(gStringVar4, gText_PkmnNeedsToReplaceMove);
    DisplayPartyMenuMessage(gStringVar4, TRUE);
    ScheduleBgCopyTilemapToVram(2);
    gPartyMenu.data1 = gMoveToLearn;
    gTasks[taskId].func = Task_ReplaceMoveYesNo;
}

static void DisplayMonLearnedMove(u8 taskId, u16 move)
{
    GetMonNickname(&gPlayerParty[gPartyMenu.slotId], gStringVar1);
    StringCopy(gStringVar2, gMoveNames[move]);
    StringExpandPlaceholders(gStringVar4, gText_PkmnLearnedMove3);
    DisplayPartyMenuMessage(gStringVar4, TRUE);
    ScheduleBgCopyTilemapToVram(2);
    gPartyMenu.data1 = move;
    gTasks[taskId].func = Task_DoLearnedMoveFanfareAfterText;
}

static void BufferMonStatsToTaskData(struct Pokemon *mon, s16 *data)
{
    data[0] = GetMonData(mon, MON_DATA_MAX_HP);
    data[1] = GetMonData(mon, MON_DATA_ATK);
    data[2] = GetMonData(mon, MON_DATA_DEF);
    data[4] = GetMonData(mon, MON_DATA_SPATK);
    data[5] = GetMonData(mon, MON_DATA_SPDEF);
    data[3] = GetMonData(mon, MON_DATA_SPEED);
}

#define tUsedOnSlot   data[0]
#define tHadEffect    data[1]
#define tLastSlotUsed data[2]

void ItemUseCB_SacredAsh(u8 taskId, TaskFunc task)
{
    sPartyMenuInternal->tUsedOnSlot = FALSE;
    sPartyMenuInternal->tHadEffect = FALSE;
    sPartyMenuInternal->tLastSlotUsed = gPartyMenu.slotId;
    UseSacredAsh(taskId);
}

static void UseSacredAsh(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    u16 hp;

    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
    {
        gTasks[taskId].func = Task_SacredAshLoop;
        return;
    }

    hp = GetMonData(mon, MON_DATA_HP);
    if (ExecuteTableBasedItemEffect_(gPartyMenu.slotId, gSpecialVar_ItemId, 0))
    {
        gTasks[taskId].func = Task_SacredAshLoop;
        return;
    }

    PlaySE(SE_USE_ITEM);
    SetPartyMonAilmentGfx(mon, &sPartyMenuBoxes[gPartyMenu.slotId]);
    if (gSprites[sPartyMenuBoxes[gPartyMenu.slotId].statusSpriteId].invisible)
        DisplayPartyPokemonLevelCheck(mon, &sPartyMenuBoxes[gPartyMenu.slotId], 1);
    AnimatePartySlot(sPartyMenuInternal->tLastSlotUsed, 0);
    AnimatePartySlot(gPartyMenu.slotId, 1);
    PartyMenuModifyHP(taskId, gPartyMenu.slotId, 1, GetMonData(mon, MON_DATA_HP) - hp, Task_SacredAshDisplayHPRestored);
    ResetHPTaskData(taskId, 0, hp);
    sPartyMenuInternal->tUsedOnSlot = TRUE;
    sPartyMenuInternal->tHadEffect = TRUE;
}

static void Task_SacredAshLoop(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        if (sPartyMenuInternal->tUsedOnSlot == TRUE)
        {
            sPartyMenuInternal->tUsedOnSlot = FALSE;
            sPartyMenuInternal->tLastSlotUsed = gPartyMenu.slotId;
        }
        if (++(gPartyMenu.slotId) == PARTY_SIZE)
        {
            if (sPartyMenuInternal->tHadEffect == FALSE)
            {
                gPartyMenuUseExitCallback = FALSE;
                DisplayPartyMenuMessage(gText_WontHaveEffect, TRUE);
                ScheduleBgCopyTilemapToVram(2);
            }
            else
            {
                gPartyMenuUseExitCallback = TRUE;
                RemoveBagItem(gSpecialVar_ItemId, 1);
            }
            gTasks[taskId].func = Task_ClosePartyMenuAfterText;
            gPartyMenu.slotId = 0;
        }
        else
        {
            UseSacredAsh(taskId);
        }
    }
}

static void Task_SacredAshDisplayHPRestored(u8 taskId)
{
    GetMonNickname(&gPlayerParty[gPartyMenu.slotId], gStringVar1);
    StringExpandPlaceholders(gStringVar4, gText_PkmnHPRestoredByVar2);
    DisplayPartyMenuMessage(gStringVar4, FALSE);
    ScheduleBgCopyTilemapToVram(2);
    gTasks[taskId].func = Task_SacredAshLoop;
}

#undef tUsedOnSlot
#undef tHadEffect
#undef tLastSlotUsed

void ItemUseCB_EvolutionStone(u8 taskId, TaskFunc task)
{
    PlaySE(SE_SELECT);
    gCB2_AfterEvolution = gPartyMenu.exitCallback;
    if (ExecuteTableBasedItemEffect_(gPartyMenu.slotId, gSpecialVar_ItemId, 0))
    {
        gPartyMenuUseExitCallback = FALSE;
        DisplayPartyMenuMessage(gText_WontHaveEffect, TRUE);
        ScheduleBgCopyTilemapToVram(2);
        gTasks[taskId].func = task;
    }
    else
    {
        RemoveBagItem(gSpecialVar_ItemId, 1);
        FreePartyPointers();
    }
}

u8 GetItemEffectType(u16 item)
{
    const u8 *itemEffect;
    u32 statusCure;

    if (!ITEM_HAS_EFFECT(item))
        return ITEM_EFFECT_NONE;

    // Read the item's effect properties.
    if (item == ITEM_ENIGMA_BERRY)
        itemEffect = gSaveBlock1Ptr->enigmaBerry.itemEffect;
    else
        itemEffect = gItemEffectTable[item - ITEM_POTION];

    if ((itemEffect[0] & (ITEM0_DIRE_HIT | ITEM0_X_ATTACK)) || itemEffect[1] || itemEffect[2] || (itemEffect[3] & ITEM3_GUARD_SPEC))
        return ITEM_EFFECT_X_ITEM;
    else if (itemEffect[0] & ITEM0_SACRED_ASH)
        return ITEM_EFFECT_SACRED_ASH;
    else if (itemEffect[3] & ITEM3_LEVEL_UP)
        return ITEM_EFFECT_RAISE_LEVEL;

    statusCure = itemEffect[3] & ITEM3_STATUS_ALL;
    if (statusCure || (itemEffect[0] >> 7))
    {
        if (statusCure == ITEM3_SLEEP)
            return ITEM_EFFECT_CURE_SLEEP;
        else if (statusCure == ITEM3_POISON)
            return ITEM_EFFECT_CURE_POISON;
        else if (statusCure == ITEM3_BURN)
            return ITEM_EFFECT_CURE_BURN;
        else if (statusCure == ITEM3_FREEZE)
            return ITEM_EFFECT_CURE_FREEZE;
        else if (statusCure == ITEM3_PARALYSIS)
            return ITEM_EFFECT_CURE_PARALYSIS;
        else if (statusCure == ITEM3_CONFUSION)
            return ITEM_EFFECT_CURE_CONFUSION;
        else if (itemEffect[0] >> 7 && !statusCure)
            return ITEM_EFFECT_CURE_INFATUATION;
        else
            return ITEM_EFFECT_CURE_ALL_STATUS;
    }

    if (itemEffect[4] & (ITEM4_REVIVE | ITEM4_HEAL_HP))
        return ITEM_EFFECT_HEAL_HP;
    else if (itemEffect[4] & ITEM4_EV_ATK)
        return ITEM_EFFECT_ATK_EV;
    else if (itemEffect[4] & ITEM4_EV_HP)
        return ITEM_EFFECT_HP_EV;
    else if (itemEffect[5] & ITEM5_EV_SPATK)
        return ITEM_EFFECT_SPATK_EV;
    else if (itemEffect[5] & ITEM5_EV_SPDEF)
        return ITEM_EFFECT_SPDEF_EV;
    else if (itemEffect[5] & ITEM5_EV_SPEED)
        return ITEM_EFFECT_SPEED_EV;
    else if (itemEffect[5] & ITEM5_EV_DEF)
        return ITEM_EFFECT_DEF_EV;
    else if (itemEffect[4] & ITEM4_EVO_STONE)
        return ITEM_EFFECT_EVO_STONE;
    else if (itemEffect[4] & ITEM4_PP_UP)
        return ITEM_EFFECT_PP_UP;
    else if (itemEffect[5] & ITEM5_PP_MAX)
        return ITEM_EFFECT_PP_MAX;
    else if (itemEffect[4] & (ITEM4_HEAL_PP | ITEM4_HEAL_PP_ONE))
        return ITEM_EFFECT_HEAL_PP;
    else
        return ITEM_EFFECT_NONE;
}

static void TryTutorSelectedMon(u8 taskId)
{
    struct Pokemon *mon;
    s16 *move;

    if (!gPaletteFade.active)
    {
        mon = &gPlayerParty[gPartyMenu.slotId];
        move = &gPartyMenu.data1;
        GetMonNickname(mon, gStringVar1);
        gPartyMenu.data1 = GetTutorMove(gSpecialVar_0x8005);
        StringCopy(gStringVar2, gMoveNames[gPartyMenu.data1]);
        move[1] = 2;
        switch (CanMonLearnTMTutor(mon, 0, gSpecialVar_0x8005))
        {
        case CANNOT_LEARN_MOVE:
            DisplayLearnMoveMessageAndClose(taskId, gText_PkmnCantLearnMove);
            return;
        case ALREADY_KNOWS_MOVE:
            DisplayLearnMoveMessageAndClose(taskId, gText_PkmnAlreadyKnows);
            return;
        default:
            if (GiveMoveToMon(mon, gPartyMenu.data1) != MON_HAS_MAX_MOVES)
            {
                Task_LearnedMove(taskId);
                return;
            }
            break;
        }
        DisplayLearnMoveMessage(gText_PkmnNeedsToReplaceMove);
        gTasks[taskId].func = Task_ReplaceMoveYesNo;
    }
}

void CB2_PartyMenuFromStartMenu(void)
{
    InitPartyMenu(PARTY_MENU_TYPE_FIELD, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_MON, FALSE, PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput, CB2_ReturnToFieldWithOpenMenu);
}

// Giving an item by selecting Give from the bag menu
// As opposted to by selecting Give in the party menu, which is handled by CursorCb_Give
void CB2_ChooseMonToGiveItem(void)
{
    MainCallback callback = (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE) ? CB2_ReturnToBagMenu : CB2_ReturnToPyramidBagMenu;
    InitPartyMenu(PARTY_MENU_TYPE_FIELD, PARTY_LAYOUT_SINGLE, PARTY_ACTION_GIVE_ITEM, FALSE, PARTY_MSG_GIVE_TO_WHICH_MON, Task_HandleChooseMonInput, callback);
    gPartyMenu.bagItem = gSpecialVar_ItemId;
}

static void TryGiveItemOrMailToSelectedMon(u8 taskId)
{
    sPartyMenuItemId = GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_HELD_ITEM);
    if (sPartyMenuItemId == ITEM_NONE)
    {
        GiveItemOrMailToSelectedMon(taskId);
    }
    else if (ItemIsMail(sPartyMenuItemId))
    {
        DisplayItemMustBeRemovedFirstMessage(taskId);
    }
    else
    {
        DisplayAlreadyHoldingItemSwitchMessage(&gPlayerParty[gPartyMenu.slotId], sPartyMenuItemId, TRUE);
        gTasks[taskId].func = Task_SwitchItemsFromBagYesNo;
    }
}

static void GiveItemOrMailToSelectedMon(u8 taskId)
{
    if (ItemIsMail(gPartyMenu.bagItem))
    {
        RemoveItemToGiveFromBag(gPartyMenu.bagItem);
        sPartyMenuInternal->exitCallback = CB2_WriteMailToGiveMonFromBag;
        Task_ClosePartyMenu(taskId);
    }
    else
    {
        GiveItemToSelectedMon(taskId);
    }
}

static void GiveItemToSelectedMon(u8 taskId)
{
    u16 item;

    if (!gPaletteFade.active)
    {
        item = gPartyMenu.bagItem;
        DisplayGaveHeldItemMessage(&gPlayerParty[gPartyMenu.slotId], item, FALSE, 1);
        GiveItemToMon(&gPlayerParty[gPartyMenu.slotId], item);
        RemoveItemToGiveFromBag(item);
        gTasks[taskId].func = Task_UpdateHeldItemSpriteAndClosePartyMenu;
    }
}

static void Task_UpdateHeldItemSpriteAndClosePartyMenu(u8 taskId)
{
    s8 slot = gPartyMenu.slotId;

    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        UpdatePartyMonHeldItemSprite(&gPlayerParty[slot], &sPartyMenuBoxes[slot]);
        Task_ClosePartyMenu(taskId);
    }
}

static void CB2_WriteMailToGiveMonFromBag(void)
{
    u8 mail;

    GiveItemToMon(&gPlayerParty[gPartyMenu.slotId], gPartyMenu.bagItem);
    mail = GetMonData(&gPlayerParty[gPartyMenu.slotId], MON_DATA_MAIL);
    DoEasyChatScreen(
        EASY_CHAT_TYPE_MAIL,
        gSaveBlock1Ptr->mail[mail].words,
        CB2_ReturnToPartyOrBagMenuFromWritingMail,
        EASY_CHAT_PERSON_DISPLAY_NONE);
}

static void CB2_ReturnToPartyOrBagMenuFromWritingMail(void)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    u16 item = GetMonData(mon, MON_DATA_HELD_ITEM);

    // Canceled writing mail
    if (gSpecialVar_Result == FALSE)
    {
        TakeMailFromMon(mon);
        SetMonData(mon, MON_DATA_HELD_ITEM, &sPartyMenuItemId);
        RemoveBagItem(sPartyMenuItemId, 1);
        ReturnGiveItemToBagOrPC(item);
        SetMainCallback2(gPartyMenu.exitCallback);
    }
    // Wrote mail
    else
    {
        InitPartyMenu(gPartyMenu.menuType, KEEP_PARTY_LAYOUT, gPartyMenu.action, TRUE, PARTY_MSG_NONE, Task_DisplayGaveMailFromBagMessage, gPartyMenu.exitCallback);
    }
}

static void Task_DisplayGaveMailFromBagMessage(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        if (sPartyMenuItemId != ITEM_NONE)
            DisplaySwitchedHeldItemMessage(gPartyMenu.bagItem, sPartyMenuItemId, FALSE);
        else
            DisplayGaveHeldItemMessage(&gPlayerParty[gPartyMenu.slotId], gPartyMenu.bagItem, FALSE, 1);
        gTasks[taskId].func = Task_UpdateHeldItemSpriteAndClosePartyMenu;
    }
}

static void Task_SwitchItemsFromBagYesNo(u8 taskId)
{
    if (IsPartyMenuTextPrinterActive() != TRUE)
    {
        PartyMenuDisplayYesNoMenu();
        gTasks[taskId].func = Task_HandleSwitchItemsFromBagYesNoInput;
    }
}

static void Task_HandleSwitchItemsFromBagYesNoInput(u8 taskId)
{
    u16 item;

    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // Yes, switch items
        item = gPartyMenu.bagItem;
        RemoveItemToGiveFromBag(item);
        if (AddBagItem(sPartyMenuItemId, 1) == FALSE)
        {
            ReturnGiveItemToBagOrPC(item);
            BufferBagFullCantTakeItemMessage(sPartyMenuItemId);
            DisplayPartyMenuMessage(gStringVar4, FALSE);
            gTasks[taskId].func = Task_UpdateHeldItemSpriteAndClosePartyMenu;
        }
        else if (ItemIsMail(item))
        {
            sPartyMenuInternal->exitCallback = CB2_WriteMailToGiveMonFromBag;
            Task_ClosePartyMenu(taskId);
        }
        else
        {
            GiveItemToMon(&gPlayerParty[gPartyMenu.slotId], item);
            DisplaySwitchedHeldItemMessage(item, sPartyMenuItemId, TRUE);
            gTasks[taskId].func = Task_UpdateHeldItemSpriteAndClosePartyMenu;
        }
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        // fallthrough
    case 1: // No, dont switch items
        gTasks[taskId].func = Task_UpdateHeldItemSpriteAndClosePartyMenu;
        break;
    }
}

static void DisplayItemMustBeRemovedFirstMessage(u8 taskId)
{
    DisplayPartyMenuMessage(gText_RemoveMailBeforeItem, TRUE);
    ScheduleBgCopyTilemapToVram(2);
    gTasks[taskId].func = Task_UpdateHeldItemSpriteAndClosePartyMenu;
}

static void RemoveItemToGiveFromBag(u16 item)
{
    if (gPartyMenu.action == PARTY_ACTION_GIVE_PC_ITEM) // Unused, never occurs
        RemovePCItem(item, 1);
    else
        RemoveBagItem(item, 1);
}

// Returns FALSE if there was no space to return the item
// but there always should be, and the return is ignored in all uses
static bool8 ReturnGiveItemToBagOrPC(u16 item)
{
    if (gPartyMenu.action == PARTY_ACTION_GIVE_ITEM)
        return AddBagItem(item, 1);
    else
        return AddPCItem(item, 1);
}

void ChooseMonToGiveMailFromMailbox(void)
{
    InitPartyMenu(PARTY_MENU_TYPE_FIELD, PARTY_LAYOUT_SINGLE, PARTY_ACTION_GIVE_MAILBOX_MAIL, FALSE, PARTY_MSG_GIVE_TO_WHICH_MON, Task_HandleChooseMonInput, Mailbox_ReturnToMailListAfterDeposit);
}

static void TryGiveMailToSelectedMon(u8 taskId)
{
    struct Pokemon *mon = &gPlayerParty[gPartyMenu.slotId];
    struct Mail *mail;

    gPartyMenuUseExitCallback = FALSE;
    mail = &gSaveBlock1Ptr->mail[gPlayerPCItemPageInfo.itemsAbove + PARTY_SIZE + gPlayerPCItemPageInfo.cursorPos];
    if (GetMonData(mon, MON_DATA_HELD_ITEM) != ITEM_NONE)
    {
        DisplayPartyMenuMessage(gText_PkmnHoldingItemCantHoldMail, TRUE);
    }
    else
    {
        GiveMailToMon(mon, mail);
        ClearMail(mail);
        DisplayPartyMenuMessage(gText_MailTransferredFromMailbox, TRUE);
    }
    ScheduleBgCopyTilemapToVram(2);
    gTasks[taskId].func = Task_UpdateHeldItemSpriteAndClosePartyMenu;
}

void InitChooseHalfPartyForBattle(u8 unused)
{
    ClearSelectedPartyOrder();
    InitPartyMenu(PARTY_MENU_TYPE_CHOOSE_HALF, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_MON, FALSE, PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput, gMain.savedCallback);
    gPartyMenu.task = Task_ValidateChosenHalfParty;
}

void ClearSelectedPartyOrder(void)
{
    memset(gSelectedOrderFromParty, 0, sizeof(gSelectedOrderFromParty));
}

static u8 GetPartySlotEntryStatus(s8 slot)
{
    if (GetBattleEntryEligibility(&gPlayerParty[slot]) == FALSE)
        return 2;
    if (HasPartySlotAlreadyBeenSelected(slot + 1) == TRUE)
        return 1;
    return 0;
}

static bool8 GetBattleEntryEligibility(struct Pokemon *mon)
{
    u16 i = 0;
    u16 species;

    if (GetMonData(mon, MON_DATA_IS_EGG)
        || GetMonData(mon, MON_DATA_LEVEL) > GetBattleEntryLevelCap()
        || (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(BATTLE_FRONTIER_BATTLE_PYRAMID_LOBBY)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(BATTLE_FRONTIER_BATTLE_PYRAMID_LOBBY)
            && GetMonData(mon, MON_DATA_HELD_ITEM) != ITEM_NONE))
    {
        return FALSE;
    }

    switch (VarGet(VAR_FRONTIER_FACILITY))
    {
    case FACILITY_MULTI_OR_EREADER:
        if (GetMonData(mon, MON_DATA_HP) != 0)
            return TRUE;
        return FALSE;
    case FACILITY_UNION_ROOM:
        return TRUE;
    default: // Battle Frontier
        species = GetMonData(mon, MON_DATA_SPECIES);
        for (; gFrontierBannedSpecies[i] != 0xFFFF; i++)
        {
            if (gFrontierBannedSpecies[i] == species)
                return FALSE;
        }
        return TRUE;
    }
}

static u8 CheckBattleEntriesAndGetMessage(void)
{
    u8 maxBattlers;
    u8 i, j;
    u8 facility;
    struct Pokemon *party = gPlayerParty;
    u8 minBattlers = GetMinBattleEntries();
    u8 *order = gSelectedOrderFromParty;

    if (order[minBattlers - 1] == 0)
    {
        if (minBattlers == 1)
            return PARTY_MSG_NO_MON_FOR_BATTLE;
        ConvertIntToDecimalStringN(gStringVar1, minBattlers, STR_CONV_MODE_LEFT_ALIGN, 1);
        return PARTY_MSG_X_MONS_ARE_NEEDED;
    }

    facility = VarGet(VAR_FRONTIER_FACILITY);
    if (facility == FACILITY_UNION_ROOM || facility == FACILITY_MULTI_OR_EREADER)
        return 0xFF;

    maxBattlers = GetMaxBattleEntries();
    for (i = 0; i < maxBattlers - 1; i++)
    {
        u16 species = GetMonData(&party[order[i] - 1], MON_DATA_SPECIES);
        u16 item = GetMonData(&party[order[i] - 1], MON_DATA_HELD_ITEM);
        for (j = i + 1; j < maxBattlers; j++)
        {
            if (species == GetMonData(&party[order[j] - 1], MON_DATA_SPECIES))
                return PARTY_MSG_MONS_CANT_BE_SAME;
            if (item != ITEM_NONE && item == GetMonData(&party[order[j] - 1], MON_DATA_HELD_ITEM))
                return PARTY_MSG_NO_SAME_HOLD_ITEMS;
        }
    }

    return 0xFF;
}

static bool8 HasPartySlotAlreadyBeenSelected(u8 slot)
{
    u8 i;

    for (i = 0; i < ARRAY_COUNT(gSelectedOrderFromParty); i++)
    {
        if (gSelectedOrderFromParty[i] == slot)
            return TRUE;
    }
    return FALSE;
}

static void Task_ValidateChosenHalfParty(u8 taskId)
{
    u8 msgId = CheckBattleEntriesAndGetMessage();

    if (msgId != 0xFF)
    {
        PlaySE(SE_FAILURE);
        DisplayPartyMenuStdMessage(msgId);
        gTasks[taskId].func = Task_ContinueChoosingHalfParty;
    }
    else
    {
        PlaySE(SE_SELECT);
        Task_ClosePartyMenu(taskId);
    }
}

static void Task_ContinueChoosingHalfParty(u8 taskId)
{
    if ((JOY_NEW(A_BUTTON)) || (JOY_NEW(B_BUTTON)))
    {
        PlaySE(SE_SELECT);
        DisplayPartyMenuStdMessage(PARTY_MSG_CHOOSE_MON);
        gTasks[taskId].func = Task_HandleChooseMonInput;
    }
}

static u8 GetMaxBattleEntries(void)
{
    switch (VarGet(VAR_FRONTIER_FACILITY))
    {
    case FACILITY_MULTI_OR_EREADER:
        return MULTI_PARTY_SIZE;
    case FACILITY_UNION_ROOM:
        return UNION_ROOM_PARTY_SIZE;
    default: // Battle Frontier
        return gSpecialVar_0x8005;
    }
}

static u8 GetMinBattleEntries(void)
{
    switch (VarGet(VAR_FRONTIER_FACILITY))
    {
    case FACILITY_MULTI_OR_EREADER:
        return 1;
    case FACILITY_UNION_ROOM:
        return UNION_ROOM_PARTY_SIZE;
    default: // Battle Frontier
        return gSpecialVar_0x8005;
    }
}

static u8 GetBattleEntryLevelCap(void)
{
    switch (VarGet(VAR_FRONTIER_FACILITY))
    {
    case FACILITY_MULTI_OR_EREADER:
        return MAX_LEVEL;
    case FACILITY_UNION_ROOM:
        return UNION_ROOM_MAX_LEVEL;
    default: // Battle Frontier
        if (gSpecialVar_0x8004 == FRONTIER_LVL_50)
            return FRONTIER_MAX_LEVEL_50;
        return FRONTIER_MAX_LEVEL_OPEN;
    }
}

static const u8 *GetFacilityCancelString(void)
{
    u8 facilityNum = VarGet(VAR_FRONTIER_FACILITY);

    if (!(facilityNum != FACILITY_UNION_ROOM && facilityNum != FACILITY_MULTI_OR_EREADER))
        return gText_CancelBattle;
    else if (facilityNum == FRONTIER_FACILITY_DOME && gSpecialVar_0x8005 == 2)
        return gText_ReturnToWaitingRoom;
    else
        return gText_CancelChallenge;
}

void ChooseMonForTradingBoard(u8 menuType, MainCallback callback)
{
    InitPartyMenu(menuType, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_MON, FALSE, PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput, callback);
}

void ChooseMonForMoveTutor(void)
{
    InitPartyMenu(PARTY_MENU_TYPE_FIELD, PARTY_LAYOUT_SINGLE, PARTY_ACTION_MOVE_TUTOR, FALSE, PARTY_MSG_TEACH_WHICH_MON, Task_HandleChooseMonInput, CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

void ChooseMonForWirelessMinigame(void)
{
    InitPartyMenu(PARTY_MENU_TYPE_MINIGAME, PARTY_LAYOUT_SINGLE, PARTY_ACTION_MINIGAME, FALSE, PARTY_MSG_CHOOSE_MON_OR_CANCEL, Task_HandleChooseMonInput, CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

static u8 GetPartyLayoutFromBattleType(void)
{
    if (IsDoubleBattle() == FALSE)
        return PARTY_LAYOUT_SINGLE;
    if (IsMultiBattle() == TRUE)
        return PARTY_LAYOUT_MULTI;
    return PARTY_LAYOUT_DOUBLE;
}

void OpenPartyMenuInBattle(u8 partyAction)
{
    InitPartyMenu(PARTY_MENU_TYPE_IN_BATTLE, GetPartyLayoutFromBattleType(), partyAction, FALSE, PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput, CB2_SetUpReshowBattleScreenAfterMenu);
    ReshowBattleScreenDummy();
    UpdatePartyToBattleOrder();
}

void ChooseMonForInBattleItem(void)
{
    InitPartyMenu(PARTY_MENU_TYPE_IN_BATTLE, GetPartyLayoutFromBattleType(), PARTY_ACTION_USE_ITEM, FALSE, PARTY_MSG_USE_ON_WHICH_MON, Task_HandleChooseMonInput, CB2_ReturnToBagMenu);
    ReshowBattleScreenDummy();
    UpdatePartyToBattleOrder();
}

static u8 GetPartyMenuActionsTypeInBattle(struct Pokemon *mon)
{
    if (GetMonData(&gPlayerParty[1], MON_DATA_SPECIES) != SPECIES_NONE && GetMonData(mon, MON_DATA_IS_EGG) == FALSE)
    {
        if (gPartyMenu.action == PARTY_ACTION_SEND_OUT)
            return ACTIONS_SEND_OUT;
        if (!(gBattleTypeFlags & BATTLE_TYPE_ARENA))
            return ACTIONS_SHIFT;
    }
    return ACTIONS_SUMMARY_ONLY;
}

static bool8 TrySwitchInPokemon(void)
{
    u8 slot = GetCursorSelectionMonId();
    u8 newSlot;
    u8 i;

    // In a multi battle, slots 1, 4, and 5 are the partner's Pokémon
    if (IsMultiBattle() == TRUE && (slot == 1 || slot == 4 || slot == 5))
    {
        StringCopy(gStringVar1, GetTrainerPartnerName());
        StringExpandPlaceholders(gStringVar4, gText_CantSwitchWithAlly);
        return FALSE;
    }
    if (GetMonData(&gPlayerParty[slot], MON_DATA_HP) == 0)
    {
        GetMonNickname(&gPlayerParty[slot], gStringVar1);
        StringExpandPlaceholders(gStringVar4, gText_PkmnHasNoEnergy);
        return FALSE;
    }
    for (i = 0; i < gBattlersCount; i++)
    {
        if (GetBattlerSide(i) == B_SIDE_PLAYER && GetPartyIdFromBattleSlot(slot) == gBattlerPartyIndexes[i])
        {
            GetMonNickname(&gPlayerParty[slot], gStringVar1);
            StringExpandPlaceholders(gStringVar4, gText_PkmnAlreadyInBattle);
            return FALSE;
        }
    }
    if (GetMonData(&gPlayerParty[slot], MON_DATA_IS_EGG))
    {
        StringExpandPlaceholders(gStringVar4, gText_EggCantBattle);
        return FALSE;
    }
    if (GetPartyIdFromBattleSlot(slot) == gBattleStruct->prevSelectedPartySlot)
    {
        GetMonNickname(&gPlayerParty[slot], gStringVar1);
        StringExpandPlaceholders(gStringVar4, gText_PkmnAlreadySelected);
        return FALSE;
    }
    if (gPartyMenu.action == PARTY_ACTION_ABILITY_PREVENTS)
    {
        SetMonPreventsSwitchingString();
        return FALSE;
    }
    if (gPartyMenu.action == PARTY_ACTION_CANT_SWITCH)
    {
        u8 currBattler = gBattlerInMenuId;
        GetMonNickname(&gPlayerParty[GetPartyIdFromBattlePartyId(gBattlerPartyIndexes[currBattler])], gStringVar1);
        StringExpandPlaceholders(gStringVar4, gText_PkmnCantSwitchOut);
        return FALSE;
    }
    gSelectedMonPartyId = GetPartyIdFromBattleSlot(slot);
    gPartyMenuUseExitCallback = TRUE;
    newSlot = GetPartyIdFromBattlePartyId(gBattlerPartyIndexes[gBattlerInMenuId]);
    SwitchPartyMonSlots(newSlot, slot);
    SwapPartyPokemon(&gPlayerParty[newSlot], &gPlayerParty[slot]);
    return TRUE;
}

void BufferBattlePartyCurrentOrder(void)
{
    BufferBattlePartyOrder(gBattlePartyCurrentOrder, GetPlayerFlankId());
}

static void BufferBattlePartyOrder(u8 *partyBattleOrder, u8 flankId)
{
    u8 partyIds[PARTY_SIZE];
    int i, j;

    if (IsMultiBattle() == TRUE)
    {
        // Party ids are packed in 4 bits at a time
        // i.e. the party id order below would be 0, 3, 5, 4, 2, 1, and the two parties would be 0,5,4 and 3,2,1
        if (flankId != 0)
        {
            partyBattleOrder[0] = 0 | (3 << 4);
            partyBattleOrder[1] = 5 | (4 << 4);
            partyBattleOrder[2] = 2 | (1 << 4);
        }
        else
        {
            partyBattleOrder[0] = 3 | (0 << 4);
            partyBattleOrder[1] = 2 | (1 << 4);
            partyBattleOrder[2] = 5 | (4 << 4);
        }
        return;
    }
    else if (IsDoubleBattle() == FALSE)
    {
        j = 1;
        partyIds[0] = gBattlerPartyIndexes[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)];
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (i != partyIds[0])
            {
                partyIds[j] = i;
                j++;
            }
        }
    }
    else
    {
        j = 2;
        partyIds[0] = gBattlerPartyIndexes[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)];
        partyIds[1] = gBattlerPartyIndexes[GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT)];
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (i != partyIds[0] && i != partyIds[1])
            {
                partyIds[j] = i;
                j++;
            }
        }
    }
    for (i = 0; i < (int)ARRAY_COUNT(gBattlePartyCurrentOrder); i++)
        partyBattleOrder[i] = (partyIds[0 + (i * 2)] << 4) | partyIds[1 + (i * 2)];
}

void BufferBattlePartyCurrentOrderBySide(u8 battlerId, u8 flankId)
{
    BufferBattlePartyOrderBySide(gBattleStruct->battlerPartyOrders[battlerId], flankId, battlerId);
}

// when GetBattlerSide(battlerId) == B_SIDE_PLAYER, this function is identical the one above
static void BufferBattlePartyOrderBySide(u8 *partyBattleOrder, u8 flankId, u8 battlerId)
{
    u8 partyIndexes[PARTY_SIZE];
    int i, j;
    u8 leftBattler;
    u8 rightBattler;

    if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
    {
        leftBattler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        rightBattler = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
    }
    else
    {
        leftBattler = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        rightBattler = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
    }

    if (IsMultiBattle() == TRUE)
    {
        if (flankId != 0)
        {
            partyBattleOrder[0] = 0 | (3 << 4);
            partyBattleOrder[1] = 5 | (4 << 4);
            partyBattleOrder[2] = 2 | (1 << 4);
        }
        else
        {
            partyBattleOrder[0] = 3 | (0 << 4);
            partyBattleOrder[1] = 2 | (1 << 4);
            partyBattleOrder[2] = 5 | (4 << 4);
        }
        return;
    }
    else if (IsDoubleBattle() == FALSE)
    {
        j = 1;
        partyIndexes[0] = gBattlerPartyIndexes[leftBattler];
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (i != partyIndexes[0])
            {
                partyIndexes[j] = i;
                j++;
            }
        }
    }
    else
    {
        j = 2;
        partyIndexes[0] = gBattlerPartyIndexes[leftBattler];
        partyIndexes[1] = gBattlerPartyIndexes[rightBattler];
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (i != partyIndexes[0] && i != partyIndexes[1])
            {
                partyIndexes[j] = i;
                j++;
            }
        }
    }

    for (i = 0; i < 3; i++)
        partyBattleOrder[i] = (partyIndexes[0 + (i * 2)] << 4) | partyIndexes[1 + (i * 2)];
}

void SwitchPartyOrderLinkMulti(u8 battlerId, u8 slot, u8 slot2)
{
    u8 partyIds[PARTY_SIZE];
    u8 tempSlot = 0;
    int i, j;
    u8 *partyBattleOrder;
    u8 partyIdBuffer;

    if (IsMultiBattle())
    {
        partyBattleOrder = gBattleStruct->battlerPartyOrders[battlerId];
        for (i = j = 0; i < PARTY_SIZE / 2; j++, i++)
        {
            partyIds[j] = partyBattleOrder[i] >> 4;
            j++;
            partyIds[j] = partyBattleOrder[i] & 0xF;
        }
        partyIdBuffer = partyIds[slot2];
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (partyIds[i] == slot)
            {
                tempSlot = partyIds[i];
                partyIds[i] = partyIdBuffer;
                break;
            }
        }
        if (i != PARTY_SIZE)
        {
            partyIds[slot2] = tempSlot;
            partyBattleOrder[0] = (partyIds[0] << 4) | partyIds[1];
            partyBattleOrder[1] = (partyIds[2] << 4) | partyIds[3];
            partyBattleOrder[2] = (partyIds[4] << 4) | partyIds[5];
        }
    }
}

static u8 GetPartyIdFromBattleSlot(u8 slot)
{
    u8 modResult = slot & 1;
    u8 retVal;

    slot /= 2;
    if (modResult != 0)
        retVal = gBattlePartyCurrentOrder[slot] & 0xF;
    else
        retVal = gBattlePartyCurrentOrder[slot] >> 4;
    return retVal;
}

static void SetPartyIdAtBattleSlot(u8 slot, u8 setVal)
{
    bool32 modResult = slot & 1;

    slot /= 2;
    if (modResult != 0)
        gBattlePartyCurrentOrder[slot] = (gBattlePartyCurrentOrder[slot] & 0xF0) | setVal;
    else
        gBattlePartyCurrentOrder[slot] = (gBattlePartyCurrentOrder[slot] & 0xF) | (setVal << 4);
}

void SwitchPartyMonSlots(u8 slot, u8 slot2)
{
    u8 partyId = GetPartyIdFromBattleSlot(slot);
    SetPartyIdAtBattleSlot(slot, GetPartyIdFromBattleSlot(slot2));
    SetPartyIdAtBattleSlot(slot2, partyId);
}

u8 GetPartyIdFromBattlePartyId(u8 battlePartyId)
{
    u8 i, j;

    for (j = i = 0; i < (int)ARRAY_COUNT(gBattlePartyCurrentOrder); j++, i++)
    {
        if ((gBattlePartyCurrentOrder[i] >> 4) != battlePartyId)
        {
            j++;
            if ((gBattlePartyCurrentOrder[i] & 0xF) == battlePartyId)
                return j;
        }
        else
        {
            return j;
        }
    }
    return 0;
}

static void UpdatePartyToBattleOrder(void)
{
    struct Pokemon *partyBuffer = Alloc(sizeof(gPlayerParty));
    u8 i;

    memcpy(partyBuffer, gPlayerParty, sizeof(gPlayerParty));
    for (i = 0; i < PARTY_SIZE; i++)
        memcpy(&gPlayerParty[GetPartyIdFromBattlePartyId(i)], &partyBuffer[i], sizeof(struct Pokemon));
    Free(partyBuffer);
}

static void UpdatePartyToFieldOrder(void)
{
    struct Pokemon *partyBuffer = Alloc(sizeof(gPlayerParty));
    u8 i;

    memcpy(partyBuffer, gPlayerParty, sizeof(gPlayerParty));
    for (i = 0; i < PARTY_SIZE; i++)
        memcpy(&gPlayerParty[GetPartyIdFromBattleSlot(i)], &partyBuffer[i], sizeof(struct Pokemon));
    Free(partyBuffer);
}

static void UNUSED SwitchAliveMonIntoLeadSlot(void)
{
    u8 i;
    struct Pokemon *mon;
    u8 partyId;

    for (i = 1; i < PARTY_SIZE; i++)
    {
        mon = &gPlayerParty[GetPartyIdFromBattleSlot(i)];
        if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE && GetMonData(mon, MON_DATA_HP) != 0)
        {
            partyId = GetPartyIdFromBattleSlot(0);
            SwitchPartyMonSlots(0, i);
            SwapPartyPokemon(&gPlayerParty[partyId], mon);
            break;
        }
    }
}

static void CB2_SetUpExitToBattleScreen(void)
{
    SetMainCallback2(CB2_SetUpReshowBattleScreenAfterMenu);
}

void ShowPartyMenuToShowcaseMultiBattleParty(void)
{
    InitPartyMenu(PARTY_MENU_TYPE_MULTI_SHOWCASE, PARTY_LAYOUT_MULTI_SHOWCASE, PARTY_ACTION_CHOOSE_MON, FALSE, PARTY_MSG_NONE, Task_InitMultiPartnerPartySlideIn, gMain.savedCallback);
}

#define tXPos  data[0]

static void Task_InitMultiPartnerPartySlideIn(u8 taskId)
{
    // The first slide step also sets the sprites offscreen
    gTasks[taskId].tXPos = 256;
    SlideMultiPartyMenuBoxSpritesOneStep(taskId);
    ChangeBgX(2, 0x10000, BG_COORD_SET);
    gTasks[taskId].func = Task_MultiPartnerPartySlideIn;
}

static void Task_MultiPartnerPartySlideIn(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u8 i;

    if (!gPaletteFade.active)
    {
        tXPos -= 8;
        SlideMultiPartyMenuBoxSpritesOneStep(taskId);
        if (tXPos == 0)
        {
            for (i = MULTI_PARTY_SIZE; i < PARTY_SIZE; i++)
            {
                if (gMultiPartnerParty[i - MULTI_PARTY_SIZE].species != SPECIES_NONE)
                    AnimateSelectedPartyIcon(sPartyMenuBoxes[i].monSpriteId, 0);
            }
            PlaySE(SE_M_HARDEN); // The Harden SE plays once the partners party mons have slid on screen
            gTasks[taskId].func = Task_WaitAfterMultiPartnerPartySlideIn;
        }
    }
}

static void Task_WaitAfterMultiPartnerPartySlideIn(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    // data[0] used as a timer afterwards rather than the x pos
    if (++data[0] == 256)
        Task_ClosePartyMenu(taskId);
}

static void MoveMultiPartyMenuBoxSprite(u8 spriteId, s16 x)
{
    if (x >= 0)
        gSprites[spriteId].x2 = x;
}

static void SlideMultiPartyMenuBoxSpritesOneStep(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u8 i;

    for (i = MULTI_PARTY_SIZE; i < PARTY_SIZE; i++)
    {
        if (gMultiPartnerParty[i - MULTI_PARTY_SIZE].species != SPECIES_NONE)
        {
            MoveMultiPartyMenuBoxSprite(sPartyMenuBoxes[i].monSpriteId, tXPos - 8);
            MoveMultiPartyMenuBoxSprite(sPartyMenuBoxes[i].itemSpriteId, tXPos - 8);
            MoveMultiPartyMenuBoxSprite(sPartyMenuBoxes[i].pokeballSpriteId, tXPos - 8);
            MoveMultiPartyMenuBoxSprite(sPartyMenuBoxes[i].statusSpriteId, tXPos - 8);
        }
    }
    ChangeBgX(2, 0x800, BG_COORD_ADD);
}

#undef tXpos

void ChooseMonForDaycare(void)
{
    InitPartyMenu(PARTY_MENU_TYPE_DAYCARE, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_MON, FALSE, PARTY_MSG_CHOOSE_MON_2, Task_HandleChooseMonInput, BufferMonSelection);
}

static void UNUSED ChoosePartyMonByMenuType(u8 menuType)
{
    gFieldCallback2 = CB2_FadeFromPartyMenu;
    InitPartyMenu(menuType, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_AND_CLOSE, FALSE, PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput, CB2_ReturnToField);
}

static void BufferMonSelection(void)
{
    gSpecialVar_0x8004 = GetCursorSelectionMonId();
    if (gSpecialVar_0x8004 >= PARTY_SIZE)
        gSpecialVar_0x8004 = PARTY_NOTHING_CHOSEN;
    gFieldCallback2 = CB2_FadeFromPartyMenu;
    SetMainCallback2(CB2_ReturnToField);
}

bool8 CB2_FadeFromPartyMenu(void)
{
    FadeInFromBlack();
    CreateTask(Task_PartyMenuWaitForFade, 10);
    return TRUE;
}

static void Task_PartyMenuWaitForFade(u8 taskId)
{
    if (IsWeatherNotFadingIn())
    {
        DestroyTask(taskId);
        UnlockPlayerFieldControls();
        ScriptContext_Enable();
    }
}

void ChooseContestMon(void)
{
    LockPlayerFieldControls();
    FadeScreen(FADE_TO_BLACK, 0);
    CreateTask(Task_ChooseContestMon, 10);
}

static void Task_ChooseContestMon(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        InitPartyMenu(PARTY_MENU_TYPE_CONTEST, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_AND_CLOSE, FALSE, PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput, CB2_ChooseContestMon);
        DestroyTask(taskId);
    }
}

static void CB2_ChooseContestMon(void)
{
    gContestMonPartyIndex = GetCursorSelectionMonId();
    if (gContestMonPartyIndex >= PARTY_SIZE)
        gContestMonPartyIndex = PARTY_NOTHING_CHOSEN;
    gSpecialVar_0x8004 = gContestMonPartyIndex;
    gFieldCallback2 = CB2_FadeFromPartyMenu;
    SetMainCallback2(CB2_ReturnToField);
}

// Used as a script special for showing a party mon to various npcs (e.g. in-game trades, move deleter)
void ChoosePartyMon(void)
{
    LockPlayerFieldControls();
    FadeScreen(FADE_TO_BLACK, 0);
    CreateTask(Task_ChoosePartyMon, 10);
}

static void Task_ChoosePartyMon(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        InitPartyMenu(PARTY_MENU_TYPE_CHOOSE_MON, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_AND_CLOSE, FALSE, PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput, BufferMonSelection);
        DestroyTask(taskId);
    }
}

void ChooseMonForMoveRelearner(void)
{
    LockPlayerFieldControls();
    FadeScreen(FADE_TO_BLACK, 0);
    CreateTask(Task_ChooseMonForMoveRelearner, 10);
}

static void Task_ChooseMonForMoveRelearner(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        InitPartyMenu(PARTY_MENU_TYPE_MOVE_RELEARNER, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_AND_CLOSE, FALSE, PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput, CB2_ChooseMonForMoveRelearner);
        DestroyTask(taskId);
    }
}

static void CB2_ChooseMonForMoveRelearner(void)
{
    gSpecialVar_0x8004 = GetCursorSelectionMonId();
    if (gSpecialVar_0x8004 >= PARTY_SIZE)
        gSpecialVar_0x8004 = PARTY_NOTHING_CHOSEN;
    else
        gSpecialVar_0x8005 = GetNumberOfRelearnableMoves(&gPlayerParty[gSpecialVar_0x8004]);
    gFieldCallback2 = CB2_FadeFromPartyMenu;
    SetMainCallback2(CB2_ReturnToField);
}

void DoBattlePyramidMonsHaveHeldItem(void)
{
    u8 i;

    gSpecialVar_Result = FALSE;
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM) != ITEM_NONE)
        {
            gSpecialVar_Result = TRUE;
            break;
        }
    }
}

// Can be called if the Battle Pyramid Bag is full on exiting and at least one party mon still has held items
// The player can then select to toss items from the bag or take/toss held items from the party
void BattlePyramidChooseMonHeldItems(void)
{
    LockPlayerFieldControls();
    FadeScreen(FADE_TO_BLACK, 0);
    CreateTask(Task_BattlePyramidChooseMonHeldItems, 10);
}

static void Task_BattlePyramidChooseMonHeldItems(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        InitPartyMenu(PARTY_MENU_TYPE_STORE_PYRAMID_HELD_ITEMS, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_MON, FALSE, PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput, BufferMonSelection);
        DestroyTask(taskId);
    }
}

void MoveDeleterChooseMoveToForget(void)
{
    ShowPokemonSummaryScreen(SUMMARY_MODE_SELECT_MOVE, gPlayerParty, gSpecialVar_0x8004, gPlayerPartyCount - 1, CB2_ReturnToField);
    gFieldCallback = FieldCB_ContinueScriptHandleMusic;
}

void GetNumMovesSelectedMonHas(void)
{
    u8 i;

    gSpecialVar_Result = 0;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (GetMonData(&gPlayerParty[gSpecialVar_0x8004], MON_DATA_MOVE1 + i) != MOVE_NONE)
            gSpecialVar_Result++;
    }
}

void BufferMoveDeleterNicknameAndMove(void)
{
    struct Pokemon *mon = &gPlayerParty[gSpecialVar_0x8004];
    u16 move = GetMonData(mon, MON_DATA_MOVE1 + gSpecialVar_0x8005);

    GetMonNickname(mon, gStringVar1);
    StringCopy(gStringVar2, gMoveNames[move]);
}

void MoveDeleterForgetMove(void)
{
    u16 i;

    SetMonMoveSlot(&gPlayerParty[gSpecialVar_0x8004], MOVE_NONE, gSpecialVar_0x8005);
    RemoveMonPPBonus(&gPlayerParty[gSpecialVar_0x8004], gSpecialVar_0x8005);
    for (i = gSpecialVar_0x8005; i < MAX_MON_MOVES - 1; i++)
        ShiftMoveSlot(&gPlayerParty[gSpecialVar_0x8004], i, i + 1);
}

static void ShiftMoveSlot(struct Pokemon *mon, u8 slotTo, u8 slotFrom)
{
    u16 move1 = GetMonData(mon, MON_DATA_MOVE1 + slotTo);
    u16 move0 = GetMonData(mon, MON_DATA_MOVE1 + slotFrom);
    u8 pp1 = GetMonData(mon, MON_DATA_PP1 + slotTo);
    u8 pp0 = GetMonData(mon, MON_DATA_PP1 + slotFrom);
    u8 ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES);
    u8 ppBonusMask1 = gPPUpGetMask[slotTo];
    u8 ppBonusMove1 = (ppBonuses & ppBonusMask1) >> (slotTo * 2);
    u8 ppBonusMask2 = gPPUpGetMask[slotFrom];
    u8 ppBonusMove2 = (ppBonuses & ppBonusMask2) >> (slotFrom * 2);
    ppBonuses &= ~ppBonusMask1;
    ppBonuses &= ~ppBonusMask2;
    ppBonuses |= (ppBonusMove1 << (slotFrom * 2)) + (ppBonusMove2 << (slotTo * 2));
    SetMonData(mon, MON_DATA_MOVE1 + slotTo, &move0);
    SetMonData(mon, MON_DATA_MOVE1 + slotFrom, &move1);
    SetMonData(mon, MON_DATA_PP1 + slotTo, &pp0);
    SetMonData(mon, MON_DATA_PP1 + slotFrom, &pp1);
    SetMonData(mon, MON_DATA_PP_BONUSES, &ppBonuses);
}

void IsSelectedMonEgg(void)
{
    if (GetMonData(&gPlayerParty[gSpecialVar_0x8004], MON_DATA_IS_EGG))
        gSpecialVar_Result = TRUE;
    else
        gSpecialVar_Result = FALSE;
}

void IsLastMonThatKnowsSurf(void)
{
    u16 move;
    u32 i, j;

    gSpecialVar_Result = FALSE;
    move = GetMonData(&gPlayerParty[gSpecialVar_0x8004], MON_DATA_MOVE1 + gSpecialVar_0x8005);
    if (move == MOVE_SURF)
    {
        for (i = 0; i < CalculatePlayerPartyCount(); i++)
        {
            if (i != gSpecialVar_0x8004)
            {
                for (j = 0; j < MAX_MON_MOVES; j++)
                {
                    if (GetMonData(&gPlayerParty[i], MON_DATA_MOVE1 + j) == MOVE_SURF)
                        return;
                }
            }
        }
        if (AnyStorageMonWithMove(move) != TRUE)
            gSpecialVar_Result = TRUE;
    }
}
