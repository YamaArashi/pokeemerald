#include "global.h"
#include "malloc.h"
#include "bard_music.h"
#include "bg.h"
#include "data.h"
#include "decompress.h"
#include "dewford_trend.h"
#include "dynamic_placeholder_text_util.h"
#include "easy_chat.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_message_box.h"
#include "field_weather.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "main.h"
#include "mystery_gift.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "pokedex.h"
#include "random.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text_window.h"
#include "window.h"
#include "constants/event_objects.h"
#include "constants/lilycove_lady.h"
#include "constants/mauville_old_man.h"
#include "constants/songs.h"
#include "constants/rgb.h"

static EWRAM_DATA struct EasyChatScreen *sEasyChatScreen = NULL;
static EWRAM_DATA struct EasyChatScreenControl *sScreenControl = NULL;
static EWRAM_DATA struct EasyChatScreenWordData *sWordData = NULL;

static void Task_InitEasyChatScreen(u8);
static void CB2_EasyChatScreen(void);
static bool8 InitEasyChatScreen(u8);
static void Task_EasyChatScreen(u8);
static void ExitEasyChatScreen(MainCallback);
static bool32 IsFuncIdForQuizLadyScreen(u16);
static void EnterQuizLadyScreen(u16);
static bool8 InitEasyChatScreenStruct(u8, u16 *, u8);
static void FreeEasyChatScreenStruct(void);
static u16 HandleEasyChatInput(void);
static u16 HandleEasyChatInput_Phrase(void);
static u16 HandleEasyChatInput_MainScreenButtons(void);
static u16 HandleEasyChatInput_Keyboard(void);
static u16 HandleEasyChatInput_WordSelect(void);
static u16 HandleEasyChatInput_ExitPrompt(void);
static u16 HandleEasyChatInput_ConfirmWordsYesNo(void);
static u16 HandleEasyChatInput_DeleteAllYesNo(void);
static u16 HandleEasyChatInput_QuizQuestion(void);
static u16 HandleEasyChatInput_WaitForMsg(void);
static u16 HandleEasyChatInput_StartConfirmLyrics(void);
static u16 HandleEasyChatInput_ConfirmLyricsYesNo(void);
static u16 StartConfirmExitPrompt(void);
static u16 TryConfirmWords(void);
static u8 GetEasyChatScreenFrameId(void);
static u8 GetEachChatScreenTemplateId(u8);
static void GetQuizTitle(u8 *);
static void ClearUnusedField(void);
static bool8 InitEasyChatScreenControl(void);
static bool8 LoadEasyChatScreen(void);
static void FreeEasyChatScreenControl(void);
static void StartEasyChatFunction(u16);
static bool8 RunEasyChatFunction(void);
static bool8 InitEasyChatScreenWordData(void);
static void FreeEasyChatScreenWordData(void);
static u8 GetNumUnlockedEasyChatGroups(void);
static int FooterHasFourOptions(void);
static int DoDeleteAllButton(void);
static int DoQuizButton(void);
static int ExitKeyboardToMainScreen(void);
static int SelectKeyboardGroup(void);
static int StartSwitchKeyboardMode(void);
static int DeleteSelectedWord(void);
static u16 MoveKeyboardCursor(int);
static u16 MoveWordSelectCursor(u32);
static int SelectNewWord(void);
static u8 GetEasyChatBackupState(void);
static void SaveCurrentPhrase(void);
static void SetSpecialEasyChatResult(void);
static bool32 GetEasyChatCompleted(void);
static void ResetCurrentPhrase(void);
static void ResetCurrentPhraseToSaved(void);
static int IsQuizQuestionEmpty(void);
static int IsQuizAnswerEmpty(void);
static bool32 IsCurrentPhraseFull(void);
static bool32 IsCurrentPhraseEmpty(void);
static u16 GetSelectedGroupIndex(void);
static u8 GetUnlockedEasyChatGroupId(u8);
static void SetSelectedWordGroup(bool32, u16);
static int GetSelectedAlphabetGroupId(void);
static u16 GetNumWordsInSelectedGroup(void);
static void SetSelectedWord(u16);
static u16 GetSelectedWordIndex(void);
static u16 GetWordFromSelectedGroup(u16);
static bool32 DummyWordCheck(int);
static u16 GetWordIndexToReplace(void);
static int MoveKeyboardCursor_GroupNames(u32);
static int MoveKeyboardCursor_Alphabet(u32);
static int MoveKeyboardCursor_ButtonWindow(u32);
static void ReduceToValidKeyboardColumn(void);
static void SetKeyboardCursorInButtonWindow(void);
static bool8 IsSelectedKeyboardIndexInvalid(void);
static void SetKeyboardCursorToLastColumn(void);
static u8 GetLastAlphabetColumn(u8);
static void ReduceToValidWordSelectColumn(void);
static bool8 IsSelectedWordIndexInvalid(void);
static int DidPlayerInputMysteryGiftPhrase(void);
static u16 DidPlayerInputABerryMasterWifePhrase(void);
static bool8 InitEasyChatScreenControl_(void);
static void LoadEasyChatPalettes(void);
static void InitEasyChatBgs(void);
static void AdjustBgTilemapForFooter(void);
static void BufferFrameTilemap(u16 *);
static void AddPhraseWindow(void);
static void AddMainScreenButtonWindow(void);
static void PrintTitle(void);
static void PrintInitialInstructions(void);
static void PrintCurrentPhrase(void);
static void DrawLowerWindow(void);
static void LoadEasyChatGfx(void);
static void CreateMainCursorSprite(void);
static void SpriteCB_Cursor(struct Sprite *);
static void SetWindowDimensions(u8, u8, u8, u8);
static void CreateScrollIndicatorSprites(void);
static void CreateStartSelectButtonSprites(void);
static void TryAddInterviewObjectEvents(void);
static bool8 ReprintPhrase(void);
static bool8 UpdateMainCursor(void);
static bool8 UpdateMainCursorOnButtons(void);
static bool8 ShowConfirmDeleteAllPrompt(void);
static bool8 ShowConfirmExitPrompt(void);
static bool8 ShowConfirmPrompt(void);
static bool8 ClosePrompt(void);
static bool8 ClosePromptAfterDeleteAll(void);
static bool8 OpenKeyboard(void);
static bool8 CloseKeyboard(void);
static bool8 OpenWordSelect(void);
static bool8 CloseWordSelect(void);
static bool8 ShowConfirmLyricsPrompt(void);
static bool8 ReturnToKeyboard(void);
static bool8 UpdateKeyboardCursor(void);
static bool8 GroupNamesScrollDown(void);
static bool8 GroupNamesScrollUp(void);
static bool8 UpdateWordSelectCursor(void);
static bool8 WordSelectScrollUp(void);
static bool8 WordSelectScrollDown(void);
static bool8 WordSelectPageScrollUp(void);
static bool8 WordSelectPageScrollDown(void);
static bool8 SwitchKeyboardMode(void);
static bool8 ShowCreateQuizMsg(void);
static bool8 ShowSelectAnswerMsg(void);
static bool8 ShowSongTooShortMsg(void);
static bool8 ShowCantDeleteLyricsMsg(void);
static bool8 ShowCombineTwoWordsMsg(void);
static bool8 ShowCantExitMsg(void);
static void SetMainCursorPos(u8, u8);
static int GetFooterOptionXOffset(int);
static void StopMainCursorAnim(void);
static void PrintEasyChatStdMessage(u8);
static void CreateEasyChatYesNoMenu(u8);
static void StartMainCursorAnim(void);
static void PrintKeyboardText(void);
static void InitLowerWindowAnim(int);
static void CreateSideWindowSprites(void);
static bool8 ShowSideWindow(void);
static void CreateRectangleCursorSprites(void);
static void SetScrollIndicatorXPos(bool32);
static bool8 UpdateLowerWindowAnim(void);
static void UpdateScrollIndicatorsVisibility(void);
static void DestroyRectangleCursorSprites(void);
static void HideModeWindow(void);
static void HideScrollIndicators(void);
static void SetModeWindowToTransition(void);
static bool8 DestroySideWindowSprites(void);
static bool8 IsModeWindowAnimActive(void);
static void UpdateModeWindowAnim(void);
static void UpdateRectangleCursorPos(void);
static void InitLowerWindowScroll(s16, u8);
static bool8 UpdateLowerWindowScroll(void);
static void ClearWordSelectWindow(void);
static void InitLowerWindowText(u32);
static void CreateWordSelectCursorSprite(void);
static void UpdateStartSelectButtonsVisibility(void);
static void DestroyWordSelectCursorSprite(void);
static void HideStartSelectButtons(void);
static void UpdateWordSelectCursorPos(void);
static void PrintWordSelectNextRowDown(void);
static void PrintWordSelectNextRowUp(void);
static int GetLowerWindowScrollOffset(void);
static void PrintWordSelectRowsPageDown(void);
static void PrintWordSelectRowsPageUp(void);
static void PrintEasyChatTextWithColors(u8, u8, const u8 *, u8, u8, u8, u8, u8, u8);
static void ResetLowerWindowScroll(void);
static void PrintKeyboardGroupNames(void);
static void PrintKeyboardAlphabet(void);
static void PrintInitialWordSelectText(void);
static const u8 *GetEasyChatWordGroupName(u8);
static void PrintWordSelectText(u8, u8);
static void EraseWordSelectRows(u8, u8);
static void DrawLowerWindowFrame(u8);
static void BufferLowerWindowFrame(int, int, int, int);
static void SetRectangleCursorPos_GroupMode(s8, s8);
static void SetRectangleCursorPos_AlphabetMode(s8, s8);
static void SpriteCB_WordSelectCursor(struct Sprite *);
static void SetWordSelectCursorPos(u8, u8);
static bool8 EasyChatIsNationalPokedexEnabled(void);
static u16 GetRandomUnlockedEasyChatPokemon(void);
static void SetUnlockedEasyChatGroups(void);
static void SetUnlockedWordsByAlphabet(void);
static u8 *CopyEasyChatWordPadded(u8 *, u16, u16);
static u8 IsEasyChatWordUnlocked(u16);
static u16 SetSelectedWordGroup_GroupMode(u16);
static u16 SetSelectedWordGroup_AlphabetMode(u16);
static bool8 IsEasyChatIndexAndGroupUnlocked(u16, u8);
static int IsRestrictedWordSpecies(u16);
static void DoQuizAnswerEasyChatScreen(void);
static void DoQuizQuestionEasyChatScreen(void);
static void DoQuizSetAnswerEasyChatScreen(void);
static void DoQuizSetQuestionEasyChatScreen(void);

enum {
    PALTAG_TRIANGLE_CURSOR,
    PALTAG_RECTANGLE_CURSOR,
    PALTAG_MISC_UI,
    PALTAG_RS_INTERVIEW_FRAME,
};

enum {
    GFXTAG_TRIANGLE_CURSOR,
    GFXTAG_RECTANGLE_CURSOR,
    GFXTAG_SCROLL_INDICATOR,
    GFXTAG_START_SELECT_BUTTONS,
    GFXTAG_MODE_WINDOW,
    GFXTAG_RS_INTERVIEW_FRAME,
    GFXTAG_BUTTON_WINDOW,
};


// State values for sEasyChatScreen->inputState
// Control which input handler to use in HandleEasyChatInput
enum {
    INPUTSTATE_PHRASE,
    INPUTSTATE_MAIN_SCREEN_BUTTONS,
    INPUTSTATE_KEYBOARD,
    INPUTSTATE_WORD_SELECT,
    INPUTSTATE_EXIT_PROMPT,
    INPUTSTATE_DELETE_ALL_YES_NO,
    INPUTSTATE_CONFIRM_WORDS_YES_NO,
    INPUTSTATE_QUIZ_QUESTION,
    INPUTSTATE_WAIT_FOR_MSG,
    INPUTSTATE_START_CONFIRM_LYRICS,
    INPUTSTATE_CONFIRM_LYRICS_YES_NO,
};

// Task states for the 'main' task, Task_EasyChatScreen
enum {
    MAINSTATE_FADE_IN,
    MAINSTATE_HANDLE_INPUT,
    MAINSTATE_RUN_FUNC,
    MAINSTATE_TO_QUIZ_LADY,
    MAINSTATE_EXIT,
    MAINSTATE_WAIT_FADE_IN,
};

// IDs provided to PrintEasyChatStdMessage to print a standard message
enum {
    MSG_INSTRUCTIONS,
    MSG_CONFIRM_DELETE,
    MSG_CONFIRM_EXIT,
    MSG_CONFIRM,
    MSG_CREATE_QUIZ,
    MSG_SELECT_ANSWER,
    MSG_SONG_TOO_SHORT,
    MSG_CANT_DELETE_LYRICS,
    MSG_COMBINE_TWO_WORDS,
    MSG_CANT_QUIT,
};

// IDs for supplementary Easy Chat functions
// Returned by the input handler functions, and run
// in the main task (MAINSTATE_RUN_FUNC)
enum {
    ECFUNC_NONE,
    ECFUNC_REPRINT_PHRASE,
    ECFUNC_UPDATE_MAIN_CURSOR,
    ECFUNC_UPDATE_MAIN_CURSOR_ON_BUTTONS,
    ECFUNC_PROMPT_DELETE_ALL,
    ECFUNC_PROMPT_EXIT,
    ECFUNC_PROMPT_CONFIRM,
    ECFUNC_CLOSE_PROMPT,
    ECFUNC_CLOSE_PROMPT_AFTER_DELETE,
    ECFUNC_OPEN_KEYBOARD,
    ECFUNC_CLOSE_KEYBOARD,
    ECFUNC_OPEN_WORD_SELECT,
    ECFUNC_CLOSE_WORD_SELECT,
    ECFUNC_PROMPT_CONFIRM_LYRICS,
    ECFUNC_RETURN_TO_KEYBOARD,
    ECFUNC_UPDATE_KEYBOARD_CURSOR,
    ECFUNC_GROUP_NAMES_SCROLL_DOWN,
    ECFUNC_GROUP_NAMES_SCROLL_UP,
    ECFUNC_UPDATE_WORD_SELECT_CURSOR,
    ECFUNC_WORD_SELECT_SCROLL_UP,
    ECFUNC_WORD_SELECT_SCROLL_DOWN,
    ECFUNC_WORD_SELECT_PAGE_UP,
    ECFUNC_WORD_SELECT_PAGE_DOWN,
    ECFUNC_SWITCH_KEYBOARD_MODE,
    ECFUNC_EXIT,
    ECFUNC_QUIZ_QUESTION,
    ECFUNC_QUIZ_ANSWER,
    ECFUNC_SET_QUIZ_QUESTION,
    ECFUNC_SET_QUIZ_ANSWER,
    ECFUNC_MSG_CREATE_QUIZ,
    ECFUNC_MSG_SELECT_ANSWER,
    ECFUNC_MSG_SONG_TOO_SHORT,
    ECFUNC_MSG_CANT_DELETE_LYRICS,
    ECFUNC_MSG_COMBINE_TWO_WORDS,
    ECFUNC_MSG_CANT_EXIT,
};

// IDs for InitLowerWindowText
enum {
    TEXT_GROUPS,
    TEXT_ALPHABET,
    TEXT_WORD_SELECT,
};

#define NUM_ALPHABET_ROWS    4
#define NUM_GROUP_NAME_ROWS  4
#define NUM_WORD_SELECT_ROWS 4
#define NUM_BUTTON_ROWS      3

#define NUM_ALPHABET_COLUMNS    7
#define NUM_GROUP_NAME_COLUMNS  2
#define NUM_WORD_SELECT_COLUMNS 2

enum {
    FRAMEID_GENERAL_2x2,
    FRAMEID_GENERAL_2x3,
    FRAMEID_MAIL,
    FRAMEID_COMBINE_TWO_WORDS,
    FRAMEID_INTERVIEW_SHOW_PERSON,
    FRAMEID_INTERVIEW,
    FRAMEID_QUIZ_ANSWER,
    FRAMEID_QUIZ_QUESTION,
    FRAMEID_QUIZ_SET_QUESTION,
};

// IDs for the footer row of buttons on the main screen
enum {
    FOOTER_NORMAL,
    FOOTER_QUIZ,
    FOOTER_ANSWER,
    NUM_FOOTER_TYPES
};

enum {
    INPUT_RIGHT,
    INPUT_LEFT,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_START,
    INPUT_SELECT,
};

// Types of animations for the lower window (keyboard/word select), given to InitLowerWindowAnim
enum {
    WINANIM_OPEN_KEYBOARD,
    WINANIM_CLOSE_KEYBOARD,
    WINANIM_OPEN_WORD_SELECT,
    WINANIM_CLOSE_WORD_SELECT,
    WINANIM_RETURN_TO_KEYBOARD,
    WINANIM_KEYBOARD_SWITCH_OUT,
    WINANIM_KEYBOARD_SWITCH_IN,
};

// Window IDs
enum {
    WIN_TITLE,
    WIN_MSG,
    WIN_INPUT_SELECT, // Word groups, word list, and keyboard
};

// Values for text frame tilemap
#define FRAME_OFFSET_ORANGE 0x1000 // Orange frame, for phrase text
#define FRAME_OFFSET_GREEN  0x4000 // Green frame, for keyboard/word select

#define FRAME_TILE_TRANSPARENT     0x0
#define FRAME_TILE_TOP_L_CORNER    0x1
#define FRAME_TILE_TOP_EDGE        0x2
#define FRAME_TILE_TOP_R_CORNER    0x3
#define FRAME_TILE_L_EDGE          0x5
#define FRAME_TILE_R_EDGE          0x7
#define FRAME_TILE_BOTTOM_L_CORNER 0x9
#define FRAME_TILE_BOTTOM_EDGE     0xA
#define FRAME_TILE_BOTTOM_R_CORNER 0xB

struct
{
    u16 funcId;
    MainCallback callback;
} static const sQuizLadyEasyChatScreens[] = {
    {
        .funcId = ECFUNC_QUIZ_ANSWER,
        .callback = DoQuizAnswerEasyChatScreen,
    },
    {
        .funcId = ECFUNC_QUIZ_QUESTION,
        .callback = DoQuizQuestionEasyChatScreen,
    },
    {
        .funcId = ECFUNC_SET_QUIZ_ANSWER,
        .callback = DoQuizSetAnswerEasyChatScreen,
    },
    {
        .funcId = ECFUNC_SET_QUIZ_QUESTION,
        .callback = DoQuizSetQuestionEasyChatScreen,
    },
};

static const struct EasyChatScreenTemplate sEasyChatScreenTemplates[] = {
    {
        .type = EASY_CHAT_TYPE_PROFILE,
        .numColumns = 2,
        .numRows = 2,
        .frameId = FRAMEID_GENERAL_2x2,
        .fourFooterOptions = FALSE,
        .titleText = gText_Profile,
        .instructionsText1 = gText_CombineFourWordsOrPhrases,
        .instructionsText2 = gText_AndMakeYourProfile,
        .confirmText1 = gText_YourProfile,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_BATTLE_START,
        .numColumns = 2,
        .numRows = 3,
        .frameId = FRAMEID_GENERAL_2x3,
        .fourFooterOptions = FALSE,
        .titleText = gText_AtTheBattlesStart,
        .instructionsText1 = gText_CombineSixWordsOrPhrases,
        .instructionsText2 = gText_AndMakeAMessage,
        .confirmText1 = gText_YourFeelingAtTheBattlesStart,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_BATTLE_WON,
        .numColumns = 2,
        .numRows = 3,
        .frameId = FRAMEID_GENERAL_2x3,
        .fourFooterOptions = FALSE,
        .titleText = gText_UponWinningABattle,
        .instructionsText1 = gText_CombineSixWordsOrPhrases,
        .instructionsText2 = gText_AndMakeAMessage,
        .confirmText1 = gText_WhatYouSayIfYouWin,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_BATTLE_LOST,
        .numColumns = 2,
        .numRows = 3,
        .frameId = FRAMEID_GENERAL_2x3,
        .fourFooterOptions = FALSE,
        .titleText = gText_UponLosingABattle,
        .instructionsText1 = gText_CombineSixWordsOrPhrases,
        .instructionsText2 = gText_AndMakeAMessage,
        .confirmText1 = gText_WhatYouSayIfYouLose,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_MAIL,
        .numColumns = 2,
        .numRows = 5,
        .frameId = FRAMEID_MAIL,
        .fourFooterOptions = FALSE,
        .titleText = NULL,
        .instructionsText1 = gText_CombineNineWordsOrPhrases,
        .instructionsText2 = gText_AndMakeAMessage2,
        .confirmText1 = gText_TheMailMessage,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_INTERVIEW,
        .numColumns = 2,
        .numRows = 2,
        .frameId = FRAMEID_INTERVIEW,
        .fourFooterOptions = FALSE,
        .titleText = gText_Interview,
        .instructionsText1 = gText_CombineFourWordsOrPhrases,
        .instructionsText2 = gText_LetsReplyToTheInterview,
        .confirmText1 = gText_TheAnswer,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_BARD_SONG,
        .numColumns = 2,
        .numRows = 3,
        .frameId = FRAMEID_GENERAL_2x3,
        .fourFooterOptions = FALSE,
        .titleText = gText_TheBardsSong,
        .instructionsText1 = gText_ChangeJustOneWordOrPhrase,
        .instructionsText2 = gText_AndImproveTheBardsSong,
        .confirmText1 = gText_TheBardsSong2,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_FAN_CLUB,
        .numColumns = 1,
        .numRows = 1,
        .frameId = FRAMEID_INTERVIEW_SHOW_PERSON,
        .fourFooterOptions = FALSE,
        .titleText = gText_Interview,
        .instructionsText1 = gText_FindWordsThatDescribeYour,
        .instructionsText2 = gText_FeelingsRightNow,
        .confirmText1 = gText_TheAnswer,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_TRENDY_PHRASE,
        .numColumns = 2,
        .numRows = 1,
        .frameId = FRAMEID_COMBINE_TWO_WORDS,
        .fourFooterOptions = FALSE,
        .titleText = gText_WhatsHipAndHappening,
        .instructionsText1 = gText_CombineTwoWordsOrPhrases,
        .instructionsText2 = gText_AndMakeATrendySaying,
        .confirmText1 = gText_TheTrendySaying,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_QUIZ_QUESTION,
        .numColumns = 2,
        .numRows = 5,
        .frameId = FRAMEID_QUIZ_QUESTION,
        .fourFooterOptions = TRUE,
        .titleText = NULL,
        .instructionsText1 = gText_AfterYouHaveReadTheQuiz,
        .instructionsText2 = gText_QuestionPressTheAButton,
        .confirmText1 = NULL,
        .confirmText2 = NULL,
    },
    {
        .type = EASY_CHAT_TYPE_QUIZ_ANSWER,
        .numColumns = 1,
        .numRows = 1,
        .frameId = FRAMEID_QUIZ_ANSWER,
        .fourFooterOptions = TRUE,
        .titleText = gText_TheQuizAnswerIs,
        .instructionsText1 = gText_OutOfTheListedChoices,
        .instructionsText2 = gText_SelectTheAnswerToTheQuiz,
        .confirmText1 = gText_TheAnswerColon,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_QUIZ_SET_QUESTION,
        .numColumns = 2,
        .numRows = 5,
        .frameId = FRAMEID_QUIZ_SET_QUESTION,
        .fourFooterOptions = TRUE,
        .titleText = NULL,
        .instructionsText1 = gText_CombineNineWordsOrPhrases,
        .instructionsText2 = gText_AndCreateAQuiz,
        .confirmText1 = gText_IsThisQuizOK,
        .confirmText2 = NULL,
    },
    {
        .type = EASY_CHAT_TYPE_QUIZ_SET_ANSWER,
        .numColumns = 1,
        .numRows = 1,
        .frameId = FRAMEID_QUIZ_ANSWER,
        .fourFooterOptions = TRUE,
        .titleText = gText_TheQuizAnswerIs,
        .instructionsText1 = gText_PickAWordOrPhraseAnd,
        .instructionsText2 = gText_SetTheQuizAnswer,
        .confirmText1 = gText_IsThisQuizOK,
        .confirmText2 = NULL,
    },
    {
        .type = EASY_CHAT_TYPE_BARD_SONG,
        .numColumns = 2,
        .numRows = 3,
        .frameId = FRAMEID_GENERAL_2x3,
        .fourFooterOptions = FALSE,
        .titleText = gText_TheBardsSong,
        .instructionsText1 = gText_ChangeJustOneWordOrPhrase,
        .instructionsText2 = gText_AndImproveTheBardsSong,
        .confirmText1 = gText_TheBardsSong2,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_APPRENTICE,
        .numColumns = 2,
        .numRows = 3,
        .frameId = FRAMEID_GENERAL_2x3,
        .fourFooterOptions = FALSE,
        .titleText = gText_ApprenticesPhrase,
        .instructionsText1 = gText_FindWordsWhichFit,
        .instructionsText2 = gText_TheTrainersImage,
        .confirmText1 = gText_ApprenticePhrase,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_GOOD_SAYING,
        .numColumns = 2,
        .numRows = 1,
        .frameId = FRAMEID_COMBINE_TWO_WORDS,
        .fourFooterOptions = FALSE,
        .titleText = gText_GoodSaying,
        .instructionsText1 = gText_CombineTwoWordsOrPhrases2,
        .instructionsText2 = gText_ToTeachHerAGoodSaying,
        .confirmText1 = gText_TheAnswer,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_GABBY_AND_TY,
        .numColumns = 1,
        .numRows = 1,
        .frameId = FRAMEID_INTERVIEW_SHOW_PERSON,
        .fourFooterOptions = FALSE,
        .titleText = gText_Interview,
        .instructionsText1 = gText_FindWordsThatDescribeYour,
        .instructionsText2 = gText_FeelingsRightNow,
        .confirmText1 = gText_TheAnswer,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_BATTLE_TOWER_INTERVIEW,
        .numColumns = 1,
        .numRows = 1,
        .frameId = FRAMEID_INTERVIEW_SHOW_PERSON,
        .fourFooterOptions = FALSE,
        .titleText = gText_Interview,
        .instructionsText1 = gText_FindWordsThatDescribeYour,
        .instructionsText2 = gText_FeelingsRightNow,
        .confirmText1 = gText_TheAnswer,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_CONTEST_INTERVIEW,
        .numColumns = 1,
        .numRows = 1,
        .frameId = FRAMEID_INTERVIEW_SHOW_PERSON,
        .fourFooterOptions = FALSE,
        .titleText = gText_Interview,
        .instructionsText1 = gText_FindWordsThatDescribeYour,
        .instructionsText2 = gText_FeelingsRightNow,
        .confirmText1 = gText_TheAnswer,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_FAN_QUESTION,
        .numColumns = 1,
        .numRows = 1,
        .frameId = FRAMEID_INTERVIEW_SHOW_PERSON,
        .fourFooterOptions = FALSE,
        .titleText = gText_FansQuestion,
        .instructionsText1 = gText_FindWordsWhichFit,
        .instructionsText2 = gText_TheTrainersImage,
        .confirmText1 = gText_TheImage,
        .confirmText2 = gText_IsAsShownOkay,
    },
    {
        .type = EASY_CHAT_TYPE_QUESTIONNAIRE,
        .numColumns = 2,
        .numRows = 2,
        .frameId = FRAMEID_GENERAL_2x2,
        .fourFooterOptions = FALSE,
        .titleText = gText_Questionnaire,
        .instructionsText1 = gText_CombineFourWordsOrPhrases,
        .instructionsText2 = gText_AndFillOutTheQuestionnaire,
        .confirmText1 = gText_TheAnswer,
        .confirmText2 = gText_IsAsShownOkay,
    },
};

// IDs are used indirectly as indexes into gEasyChatWordsByLetterPointers
// 0 is 'Others', 1-26 are the letters A-Z
// This array maps the group IDs to the alphabet keyboard
static const u8 sAlphabetGroupIdMap[NUM_ALPHABET_ROWS][NUM_ALPHABET_COLUMNS] = {
    { 1,  2,  3,  4,  5,  6,  0},
    { 7,  8,  9, 10, 11, 12,  0},
    {13, 14, 15, 16, 17, 18, 19},
    {20, 21, 22, 23, 24, 25, 26},
};

static const u16 sMysteryGiftPhrase[NUM_QUESTIONNAIRE_WORDS] = {
    EC_WORD_LINK,
    EC_WORD_TOGETHER,
    EC_WORD_WITH,
    EC_WORD_ALL,
};

static const u16 sBerryMasterWifePhrases[][2] = {
    [PHRASE_GREAT_BATTLE - 1]        = {EC_WORD_GREAT, EC_WORD_BATTLE},
    [PHRASE_CHALLENGE_CONTEST - 1]   = {EC_WORD_CHALLENGE, EC_WORD_CONTEST},
    [PHRASE_OVERWHELMING_LATIAS - 1] = {EC_WORD_OVERWHELMING, EC_POKEMON(LATIAS)},
    [PHRASE_COOL_LATIOS - 1]         = {EC_WORD_COOL, EC_POKEMON(LATIOS)},
    [PHRASE_SUPER_HUSTLE - 1]        = {EC_WORD_SUPER, EC_WORD_HUSTLE},
};

static const u16 sTriangleCursor_Pal[] = INCBIN_U16("graphics/easy_chat/triangle_cursor.gbapal");
static const u32 sTriangleCursor_Gfx[] = INCBIN_U32("graphics/easy_chat/triangle_cursor.4bpp");
static const u32 sScrollIndicator_Gfx[] = INCBIN_U32("graphics/easy_chat/scroll_indicator.4bpp");
static const u32 sStartSelectButtons_Gfx[] = INCBIN_U32("graphics/easy_chat/start_select_buttons.4bpp");
// In Ruby/Sapphire Easy Chat screens had a black background, and when the player & interviewer were present
// on screen the interview_frame gfx was shown behind them.
// In Emerald all Easy Chat screens have a filled background, so these gfx go unused
static const u16 sRSInterviewFrame_Pal[] = INCBIN_U16("graphics/easy_chat/interview_frame.gbapal");
static const u32 sRSInterviewFrame_Gfx[] = INCBIN_U32("graphics/easy_chat/interview_frame.4bpp.lz");
static const u16 sTextInputFrameOrange_Pal[] = INCBIN_U16("graphics/easy_chat/text_input_frame_orange.gbapal");
static const u16 sTextInputFrameGreen_Pal[] = INCBIN_U16("graphics/easy_chat/text_input_frame_green.gbapal");
static const u32 sTextInputFrame_Gfx[] = INCBIN_U32("graphics/easy_chat/text_input_frame.4bpp.lz");
static const u16 sTitleText_Pal[] = INCBIN_U16("graphics/easy_chat/title_text.gbapal");
static const u16 sText_Pal[] = INCBIN_U16("graphics/easy_chat/text.gbapal");

static const struct EasyChatPhraseFrameDimensions sPhraseFrameDimensions[] = {
    [FRAMEID_GENERAL_2x2] = {
        .left = 3,
        .top = 4,
        .width = 24,
        .height = 4,
        .footerId = FOOTER_NORMAL,
    },
    [FRAMEID_GENERAL_2x3] = {
        .left = 3,
        .top = 3,
        .width = 24,
        .height = 6,
        .footerId = FOOTER_NORMAL,
    },
    [FRAMEID_MAIL] = {
        .left = 3,
        .top = 0,
        .width = 24,
        .height = 10,
        .footerId = FOOTER_NORMAL,
    },
    [FRAMEID_COMBINE_TWO_WORDS] = {
        .left = 3,
        .top = 5,
        .width = 24,
        .height = 2,
        .footerId = FOOTER_NORMAL,
    },
    [FRAMEID_INTERVIEW_SHOW_PERSON] = {
        .left = 16,
        .top = 5,
        .width = 12,
        .height = 2,
        .footerId = FOOTER_NORMAL,
    },
    [FRAMEID_INTERVIEW] = {
        .left = 3,
        .top = 4,
        .width = 24,
        .height = 4,
        .footerId = FOOTER_NORMAL,
    },
    [FRAMEID_QUIZ_ANSWER] = {
        .left = 9,
        .top = 4,
        .width = 12,
        .height = 2,
        .footerId = FOOTER_QUIZ,
    },
    [FRAMEID_QUIZ_QUESTION] = {
        .left = 5,
        .top = 3,
        .width = 20,
        .height = 10,
        .footerId = NUM_FOOTER_TYPES,
    },
    [FRAMEID_QUIZ_SET_QUESTION] = {
        .left = 3,
        .top = 0,
        .width = 24,
        .height = 10,
        .footerId = FOOTER_ANSWER,
    },
};

static const struct BgTemplate sEasyChatBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 28,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    },
    {
        .bg = 1,
        .charBaseIndex = 3,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0x80,
    },
    {
        .bg = 3,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0,
    },
};

static const struct WindowTemplate sEasyChatWindowTemplates[] = {
    [WIN_TITLE] = {
        .bg = 1,
        .tilemapLeft = 6,
        .tilemapTop = 0,
        .width = 18,
        .height = 2,
        .paletteNum = 10,
        .baseBlock = 0x10,
    },
    [WIN_MSG] = {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 15,
        .width = 24,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0xA,
    },
    [WIN_INPUT_SELECT] = {
        .bg = 2,
        .tilemapLeft = 1,
        .tilemapTop = 0,
        .width = 28,
        .height = 32,
        .paletteNum = 3,
        .baseBlock = 0,
    },
    DUMMY_WIN_TEMPLATE,
};

static const struct WindowTemplate sEasyChatYesNoWindowTemplate = {
    .bg = 0,
    .tilemapLeft = 22,
    .tilemapTop = 9,
    .width = 5,
    .height = 4,
    .paletteNum = 15,
    .baseBlock = 0x6A,
};

static const u8 sText_Clear17[] = _("{CLEAR 17}");

static const u8 *const sEasyChatKeyboardAlphabet[NUM_ALPHABET_ROWS] =
{
    gText_EasyChatKeyboard_ABCDEFothers,
    gText_EasyChatKeyboard_GHIJKL,
    gText_EasyChatKeyboard_MNOPQRS,
    gText_EasyChatKeyboard_TUVWXYZ,
};

static const struct SpriteSheet sSpriteSheets[] = {
    {
        .data = sTriangleCursor_Gfx,
        .size = sizeof(sTriangleCursor_Gfx),
        .tag = GFXTAG_TRIANGLE_CURSOR
    },
    {
        .data = sScrollIndicator_Gfx,
        .size = sizeof(sScrollIndicator_Gfx),
        .tag = GFXTAG_SCROLL_INDICATOR
    },
    {
        .data = sStartSelectButtons_Gfx,
        .size = sizeof(sStartSelectButtons_Gfx),
        .tag = GFXTAG_START_SELECT_BUTTONS
    },
    {0}
};

static const struct SpritePalette sSpritePalettes[] = {
    {
        .data = sTriangleCursor_Pal,
        .tag = PALTAG_TRIANGLE_CURSOR,
    },
    {
        .data = gEasyChatRectangleCursor_Pal,
        .tag = PALTAG_RECTANGLE_CURSOR,
    },
    {
        .data = gEasyChatButtonWindow_Pal,
        .tag = PALTAG_MISC_UI, // The palette is generated from the button window but used for various parts of the UI
    },
    {
        .data = sRSInterviewFrame_Pal,
        .tag = PALTAG_RS_INTERVIEW_FRAME,
    },
    {0}
};

static const struct CompressedSpriteSheet sCompressedSpriteSheets[] = {
    {
        .data = sRSInterviewFrame_Gfx,
        .size = 0x800,
        .tag = GFXTAG_RS_INTERVIEW_FRAME,
    },
    {
        .data = gEasyChatRectangleCursor_Gfx,
        .size = 0x1000,
        .tag = GFXTAG_RECTANGLE_CURSOR,
    },
    {
        .data = gEasyChatButtonWindow_Gfx,
        .size = 0x800,
        .tag = GFXTAG_BUTTON_WINDOW,
    },
    {
        .data = gEasyChatMode_Gfx,
        .size = 0x1000,
        .tag = GFXTAG_MODE_WINDOW,
    },
};

static const u8 sAlphabetKeyboardColumnOffsets[NUM_ALPHABET_COLUMNS] = {0, 12, 24, 56, 68, 80, 92};

static const struct OamData sOamData_TriangleCursor = {
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
    .priority = 3,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct SpriteTemplate sSpriteTemplate_TriangleCursor =
{
    .tileTag = PALTAG_TRIANGLE_CURSOR,
    .paletteTag = GFXTAG_TRIANGLE_CURSOR,
    .oam = &sOamData_TriangleCursor,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Cursor,
};

static const struct OamData sOamData_RectangleCursor = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sAnim_RectangleCursor_OnGroup[] = {
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END,
};

static const union AnimCmd sAnim_RectangleCursor_OnButton[] = {
    ANIMCMD_FRAME(32, 0),
    ANIMCMD_END,
};

static const union AnimCmd sAnim_RectangleCursor_OnOthers[] = {
    ANIMCMD_FRAME(64, 0),
    ANIMCMD_END,
};

static const union AnimCmd sAnim_RectangleCursor_OnLetter[] = {
    ANIMCMD_FRAME(96, 0),
    ANIMCMD_END,
};

// Each anim changes the width of the rectangle cursor to fit what it should be selecting
enum {
    RECTCURSOR_ANIM_ON_GROUP,
    RECTCURSOR_ANIM_ON_BUTTON,
    RECTCURSOR_ANIM_ON_OTHERS,
    RECTCURSOR_ANIM_ON_LETTER,
};

static const union AnimCmd *const sAnims_RectangleCursor[] = {
    [RECTCURSOR_ANIM_ON_GROUP]  = sAnim_RectangleCursor_OnGroup,
    [RECTCURSOR_ANIM_ON_BUTTON] = sAnim_RectangleCursor_OnButton,
    [RECTCURSOR_ANIM_ON_OTHERS] = sAnim_RectangleCursor_OnOthers,
    [RECTCURSOR_ANIM_ON_LETTER] = sAnim_RectangleCursor_OnLetter,
};

static const struct SpriteTemplate sSpriteTemplate_RectangleCursor =
{
    .tileTag = GFXTAG_RECTANGLE_CURSOR,
    .paletteTag = PALTAG_RECTANGLE_CURSOR,
    .oam = &sOamData_RectangleCursor,
    .anims = sAnims_RectangleCursor,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Cursor,
};

static const struct OamData sOamData_ModeWindow = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sAnim_ModeWindow_Hidden[] = {
    ANIMCMD_FRAME(96, 0),
    ANIMCMD_END,
};

static const union AnimCmd sAnim_ModeWindow_ToGroup[] = {
    ANIMCMD_FRAME(64, 4), // Transition frame
    ANIMCMD_FRAME(32, 4), // 'Group' frame
    ANIMCMD_END,
};

static const union AnimCmd sAnim_ModeWindow_ToAlphabet[] = {
    ANIMCMD_FRAME(64, 4), // Transition frame
    ANIMCMD_FRAME(0, 4),  // 'A-Z' frame
    ANIMCMD_END,
};

static const union AnimCmd sAnim_ModeWindow_ToHidden[] = {
    ANIMCMD_FRAME(64, 4), // Transition frame
    ANIMCMD_FRAME(96, 0), // Hidden frame
    ANIMCMD_END,
};

static const union AnimCmd sAnim_ModeWindow_Transition[] = {
    ANIMCMD_FRAME(64, 4),
    ANIMCMD_END,
};

enum {
    MODEWINDOW_ANIM_HIDDEN,
    MODEWINDOW_ANIM_TO_GROUP,
    MODEWINDOW_ANIM_TO_ALPHABET,
    MODEWINDOW_ANIM_TO_HIDDEN,
    MODEWINDOW_ANIM_TRANSITION,
};

static const union AnimCmd *const sAnims_ModeWindow[] = {
    [MODEWINDOW_ANIM_HIDDEN]      = sAnim_ModeWindow_Hidden,
    [MODEWINDOW_ANIM_TO_GROUP]    = sAnim_ModeWindow_ToGroup,
    [MODEWINDOW_ANIM_TO_ALPHABET] = sAnim_ModeWindow_ToAlphabet,
    [MODEWINDOW_ANIM_TO_HIDDEN]   = sAnim_ModeWindow_ToHidden,
    [MODEWINDOW_ANIM_TRANSITION]  = sAnim_ModeWindow_Transition,
};

static const struct SpriteTemplate sSpriteTemplate_ModeWindow =
{
    .tileTag = GFXTAG_MODE_WINDOW,
    .paletteTag = PALTAG_MISC_UI,
    .oam = &sOamData_ModeWindow,
    .anims = sAnims_ModeWindow,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct OamData sOamData_ButtonWindow = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 3,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct SpriteTemplate sSpriteTemplate_ButtonWindow =
{
    .tileTag = GFXTAG_BUTTON_WINDOW,
    .paletteTag = PALTAG_MISC_UI,
    .oam = &sOamData_ButtonWindow,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct OamData sOamData_StartSelectButton = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x8),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct OamData sOamData_ScrollIndicator = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sAnim_Frame0[] = {
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END,
};

static const union AnimCmd sAnim_Frame1[] = {
    ANIMCMD_FRAME(4, 0),
    ANIMCMD_END,
};

// Frame0 is Start button, Frame1 is Select button, both are identical for the scroll indicators
static const union AnimCmd *const sAnims_TwoFrame[] = {
    sAnim_Frame0,
    sAnim_Frame1,
};

static const struct SpriteTemplate sSpriteTemplate_StartSelectButton =
{
    .tileTag = GFXTAG_START_SELECT_BUTTONS,
    .paletteTag = PALTAG_MISC_UI,
    .oam = &sOamData_StartSelectButton,
    .anims = sAnims_TwoFrame,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sSpriteTemplate_ScrollIndicator =
{
    .tileTag = GFXTAG_SCROLL_INDICATOR,
    .paletteTag = PALTAG_MISC_UI,
    .oam = &sOamData_ScrollIndicator,
    .anims = sAnims_TwoFrame,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const u8 sFooterOptionXOffsets[NUM_FOOTER_TYPES][4] = {
    [FOOTER_NORMAL] = {16, 111, 196,   0},
    [FOOTER_QUIZ]   = {16,  78, 130, 160},
    [FOOTER_ANSWER] = {16,  80, 134, 170},
};

static const u8 *const sFooterTextOptions[NUM_FOOTER_TYPES][4] = {
    [FOOTER_NORMAL] = {gText_DelAll, gText_Cancel5, gText_Ok2, NULL},
    [FOOTER_QUIZ]   = {gText_DelAll, gText_Cancel5, gText_Ok2, gText_Quiz},
    [FOOTER_ANSWER] = {gText_DelAll, gText_Cancel5, gText_Ok2, gText_Answer},
};

#include "data/easy_chat/easy_chat_groups.h"
#include "data/easy_chat/easy_chat_words_by_letter.h"

static const u8 *const sEasyChatGroupNamePointers[EC_NUM_GROUPS] = {
    [EC_GROUP_POKEMON]          = gEasyChatGroupName_Pokemon,
    [EC_GROUP_TRAINER]          = gEasyChatGroupName_Trainer,
    [EC_GROUP_STATUS]           = gEasyChatGroupName_Status,
    [EC_GROUP_BATTLE]           = gEasyChatGroupName_Battle,
    [EC_GROUP_GREETINGS]        = gEasyChatGroupName_Greetings,
    [EC_GROUP_PEOPLE]           = gEasyChatGroupName_People,
    [EC_GROUP_VOICES]           = gEasyChatGroupName_Voices,
    [EC_GROUP_SPEECH]           = gEasyChatGroupName_Speech,
    [EC_GROUP_ENDINGS]          = gEasyChatGroupName_Endings,
    [EC_GROUP_FEELINGS]         = gEasyChatGroupName_Feelings,
    [EC_GROUP_CONDITIONS]       = gEasyChatGroupName_Conditions,
    [EC_GROUP_ACTIONS]          = gEasyChatGroupName_Actions,
    [EC_GROUP_LIFESTYLE]        = gEasyChatGroupName_Lifestyle,
    [EC_GROUP_HOBBIES]          = gEasyChatGroupName_Hobbies,
    [EC_GROUP_TIME]             = gEasyChatGroupName_Time,
    [EC_GROUP_MISC]             = gEasyChatGroupName_Misc,
    [EC_GROUP_ADJECTIVES]       = gEasyChatGroupName_Adjectives,
    [EC_GROUP_EVENTS]           = gEasyChatGroupName_Events,
    [EC_GROUP_MOVE_1]           = gEasyChatGroupName_Move1,
    [EC_GROUP_MOVE_2]           = gEasyChatGroupName_Move2,
    [EC_GROUP_TRENDY_SAYING]    = gEasyChatGroupName_TrendySaying,
    [EC_GROUP_POKEMON_NATIONAL] = gEasyChatGroupName_Pokemon2,
};

static const u16 sDefaultProfileWords[EASY_CHAT_BATTLE_WORDS_COUNT - 2] = {
    EC_WORD_I_AM,
    EC_WORD_A,
    EC_WORD_POKEMON,
    EC_WORD_FRIEND,
};

static const u16 sDefaultBattleStartWords[EASY_CHAT_BATTLE_WORDS_COUNT] = {
    EC_WORD_ARE,
    EC_WORD_YOU,
    EC_WORD_READY,
    EC_WORD_QUES,
    EC_WORD_HERE_I_COME,
    EC_WORD_EXCL,
};

static const u16 sDefaultBattleWonWords[EASY_CHAT_BATTLE_WORDS_COUNT] = {
    EC_WORD_YAY,
    EC_WORD_YAY,
    EC_WORD_EXCL_EXCL,
    EC_WORD_I_VE,
    EC_WORD_WON,
    EC_WORD_EXCL_EXCL,
};

static const u16 sDefaultBattleLostWords[EASY_CHAT_BATTLE_WORDS_COUNT] = {
    EC_WORD_TOO,
    EC_WORD_BAD,
    EC_WORD_ELLIPSIS,
    EC_WORD_WE,
    EC_WORD_LOST,
    EC_WORD_ELLIPSIS,
};

static const u16 sRestrictedWordSpecies[] = {
    SPECIES_DEOXYS,
};


// In addition to the task defines below, these two elements
// have their indexes used explicitly because they are 4-byte
// pointers, and occupy the next data element as well.
// SetWordTaskArg/GetWordTaskArg use these defines to
// read the pointer from the two elements
#define TASKIDX_WORDS         2
#define TASKIDX_EXIT_CALLBACK 4

#define tState        data[0]
#define tType         data[1]
#define tWords        data[TASKIDX_WORDS] // Occupies 2 and 3
#define tExitCallback data[TASKIDX_EXIT_CALLBACK] // Occupies 4 and 5
#define tFuncId       data[6]
#define tPersonType   data[7]

void DoEasyChatScreen(u8 type, u16 *words, MainCallback exitCallback, u8 displayedPersonType)
{
    u8 taskId;

    ResetTasks();
    taskId = CreateTask(Task_InitEasyChatScreen, 0);
    gTasks[taskId].tType = type;
    gTasks[taskId].tPersonType = displayedPersonType;
    SetWordTaskArg(taskId, TASKIDX_WORDS, (u32)words);
    SetWordTaskArg(taskId, TASKIDX_EXIT_CALLBACK, (u32)exitCallback);
    SetMainCallback2(CB2_EasyChatScreen);
}

static void CB2_EasyChatScreen(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_EasyChatScreen(void)
{
    TransferPlttBuffer();
    LoadOam();
    ProcessSpriteCopyRequests();
}

static void StartEasyChatScreen(u8 taskId, TaskFunc taskFunc)
{
    gTasks[taskId].func = taskFunc;
    gTasks[taskId].tState = MAINSTATE_FADE_IN;
}

static void Task_InitEasyChatScreen(u8 taskId)
{
    if (!IsOverworldLinkActive())
    {
        while (InitEasyChatScreen(taskId));
    }
    else
    {
        if (InitEasyChatScreen(taskId) == TRUE)
            return;
    }
    StartEasyChatScreen(taskId, Task_EasyChatScreen);
}

// After loading, this is the 'main' Easy Chat task
static void Task_EasyChatScreen(u8 taskId)
{
    u16 funcId;
    s16 *data;

    data = gTasks[taskId].data;
    switch (tState)
    {
    case MAINSTATE_FADE_IN:
        SetVBlankCallback(VBlankCB_EasyChatScreen);
        BlendPalettes(PALETTES_ALL, 16, 0);
        BeginNormalPaletteFade(PALETTES_ALL, -1, 16, 0, RGB_BLACK);
        tState = MAINSTATE_WAIT_FADE_IN;
        break;
    case MAINSTATE_HANDLE_INPUT:
        funcId = HandleEasyChatInput();
        if (IsFuncIdForQuizLadyScreen(funcId))
        {
            // Fade to Quiz Lady screen
            BeginNormalPaletteFade(PALETTES_ALL, -2, 0, 16, RGB_BLACK);
            tState = MAINSTATE_TO_QUIZ_LADY;
            tFuncId = funcId;
        }
        else if (funcId == ECFUNC_EXIT)
        {
            // Fade and exit Easy Chat
            BeginNormalPaletteFade(PALETTES_ALL, -1, 0, 16, RGB_BLACK);
            tState = MAINSTATE_EXIT;
        }
        else if (funcId != ECFUNC_NONE)
        {
            PlaySE(SE_SELECT);
            StartEasyChatFunction(funcId);
            tState++; // MAINSTATE_RUN_FUNC
        }
        break;
    case MAINSTATE_RUN_FUNC:
        if (!RunEasyChatFunction())
            tState = MAINSTATE_HANDLE_INPUT;
        break;
    case MAINSTATE_TO_QUIZ_LADY:
        if (!gPaletteFade.active)
            EnterQuizLadyScreen(tFuncId);
        break;
    case MAINSTATE_EXIT:
        if (!gPaletteFade.active)
            ExitEasyChatScreen((MainCallback)GetWordTaskArg(taskId, TASKIDX_EXIT_CALLBACK));
        break;
    case MAINSTATE_WAIT_FADE_IN:
        if (!gPaletteFade.active)
            tState = MAINSTATE_HANDLE_INPUT;
        break;
    }
}

// Returns TRUE if still initializing, FALSE when finished
// If an allocation fails it will switch to the exit callback
static bool8 InitEasyChatScreen(u8 taskId)
{
    s16 *data;

    data = gTasks[taskId].data;
    switch (tState)
    {
    case 0:
        SetVBlankCallback(NULL);
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetPaletteFade();
        break;
    case 1:
        if (!InitEasyChatScreenWordData())
        {
            // Alloc failed, exit
            ExitEasyChatScreen((MainCallback)GetWordTaskArg(taskId, TASKIDX_EXIT_CALLBACK));
        }
        break;
    case 2:
        if (!InitEasyChatScreenStruct(tType, (u16 *)GetWordTaskArg(taskId, TASKIDX_WORDS), tPersonType))
        {
            // Alloc failed, exit
            ExitEasyChatScreen((MainCallback)GetWordTaskArg(taskId, TASKIDX_EXIT_CALLBACK));
        }
        break;
    case 3:
        if (!InitEasyChatScreenControl())
        {
            // Alloc failed, exit
            ExitEasyChatScreen((MainCallback)GetWordTaskArg(taskId, TASKIDX_EXIT_CALLBACK));
        }
        break;
    case 4:
        if (LoadEasyChatScreen())
        {
            return TRUE;
        }
        break;
    default:
        return FALSE;
    }
    tState++;
    return TRUE;
}

static void ExitEasyChatScreen(MainCallback callback)
{
    FreeEasyChatScreenControl();
    FreeEasyChatScreenStruct();
    FreeEasyChatScreenWordData();
    FreeAllWindowBuffers();
    SetMainCallback2(callback);
}

void ShowEasyChatScreen(void)
{
    int i;
    u16 *words;
    struct MauvilleManBard *bard;
    u8 displayedPersonType = EASY_CHAT_PERSON_DISPLAY_NONE;
    switch (gSpecialVar_0x8004)
    {
    case EASY_CHAT_TYPE_PROFILE:
        words = gSaveBlock1Ptr->easyChatProfile;
        break;
    case EASY_CHAT_TYPE_BATTLE_START:
        words = gSaveBlock1Ptr->easyChatBattleStart;
        break;
    case EASY_CHAT_TYPE_BATTLE_WON:
        words = gSaveBlock1Ptr->easyChatBattleWon;
        break;
    case EASY_CHAT_TYPE_BATTLE_LOST:
        words = gSaveBlock1Ptr->easyChatBattleLost;
        break;
    case EASY_CHAT_TYPE_MAIL:
        words = gSaveBlock1Ptr->mail[gSpecialVar_0x8005].words;
        break;
    case EASY_CHAT_TYPE_BARD_SONG:
        bard = &gSaveBlock1Ptr->oldMan.bard;
        for (i = 0; i < NUM_BARD_SONG_WORDS; i ++)
            bard->newSongLyrics[i] = bard->songLyrics[i];

        words = bard->newSongLyrics;
        break;
    case EASY_CHAT_TYPE_INTERVIEW:
        words = gSaveBlock1Ptr->tvShows[gSpecialVar_0x8005].bravoTrainer.words;
        displayedPersonType = gSpecialVar_0x8006;
        break;
    case EASY_CHAT_TYPE_FAN_CLUB:
        words = &gSaveBlock1Ptr->tvShows[gSpecialVar_0x8005].fanclubOpinions.words[gSpecialVar_0x8006];
        displayedPersonType = EASY_CHAT_PERSON_REPORTER_FEMALE;
        break;
    case EASY_CHAT_TYPE_DUMMY_SHOW:
        words = gSaveBlock1Ptr->tvShows[gSpecialVar_0x8005].dummy.words;
        displayedPersonType = EASY_CHAT_PERSON_REPORTER_MALE;
        break;
    case EASY_CHAT_TYPE_TRENDY_PHRASE:
        words = (u16 *)gStringVar3;
        words[0] = gSaveBlock1Ptr->dewfordTrends[0].words[0];
        words[1] = gSaveBlock1Ptr->dewfordTrends[0].words[1];
        break;
    case EASY_CHAT_TYPE_GABBY_AND_TY:
        words = gSaveBlock1Ptr->gabbyAndTyData.quote;
        *words = EC_EMPTY_WORD;
        displayedPersonType = EASY_CHAT_PERSON_REPORTER_FEMALE;
        break;
    case EASY_CHAT_TYPE_CONTEST_INTERVIEW:
        words = &gSaveBlock1Ptr->tvShows[gSpecialVar_0x8005].bravoTrainer.words[gSpecialVar_0x8006];
        displayedPersonType = EASY_CHAT_PERSON_REPORTER_MALE;
        break;
    case EASY_CHAT_TYPE_BATTLE_TOWER_INTERVIEW:
        words = gSaveBlock1Ptr->tvShows[gSpecialVar_0x8005].bravoTrainerTower.words;
        displayedPersonType = EASY_CHAT_PERSON_REPORTER_FEMALE;
        break;
    case EASY_CHAT_TYPE_GOOD_SAYING:
        words = (u16 *)gStringVar3;
        InitializeEasyChatWordArray(words, 2);
        break;
    case EASY_CHAT_TYPE_FAN_QUESTION:
        words = gSaveBlock1Ptr->tvShows[gSpecialVar_0x8005].fanClubSpecial.words;
        words[0] = EC_EMPTY_WORD;
        displayedPersonType = EASY_CHAT_PERSON_BOY;
        break;
    case EASY_CHAT_TYPE_QUIZ_ANSWER:
        words = &gSaveBlock1Ptr->lilycoveLady.quiz.playerAnswer;
        break;
    case EASY_CHAT_TYPE_QUIZ_QUESTION:
        return;
    case EASY_CHAT_TYPE_QUIZ_SET_QUESTION:
        words = gSaveBlock1Ptr->lilycoveLady.quiz.question;
        break;
    case EASY_CHAT_TYPE_QUIZ_SET_ANSWER:
        words = &gSaveBlock1Ptr->lilycoveLady.quiz.correctAnswer;
        break;
    case EASY_CHAT_TYPE_APPRENTICE:
        words = gSaveBlock2Ptr->apprentices[0].speechWon;
        break;
    case EASY_CHAT_TYPE_QUESTIONNAIRE:
        words = GetQuestionnaireWordsPtr();
        break;
    default:
        return;
    }

    CleanupOverworldWindowsAndTilemaps();
    DoEasyChatScreen(gSpecialVar_0x8004, words, CB2_ReturnToFieldContinueScript, displayedPersonType);
}

static void CB2_QuizLadyQuestion(void)
{
    LilycoveLady *lilycoveLady;

    UpdatePaletteFade();
    switch (gMain.state)
    {
    case 0:
        FadeScreen(FADE_TO_BLACK, 0);
        break;
    case 1:
        if (!gPaletteFade.active)
        {
            lilycoveLady = &gSaveBlock1Ptr->lilycoveLady;
            lilycoveLady->quiz.playerAnswer = EC_EMPTY_WORD;
            CleanupOverworldWindowsAndTilemaps();
            DoQuizQuestionEasyChatScreen();
        }
        return;
    }
    gMain.state ++;
}

void QuizLadyShowQuizQuestion(void)
{
    SetMainCallback2(CB2_QuizLadyQuestion);
}

static int GetQuizLadyScreenByFuncId(u16 funcId)
{
    int i;

    for (i = 0; i < ARRAY_COUNT(sQuizLadyEasyChatScreens); i ++)
    {
        if (funcId == sQuizLadyEasyChatScreens[i].funcId)
            return i;
    }
    return -1;
}

static bool32 IsFuncIdForQuizLadyScreen(u16 funcId)
{
    return GetQuizLadyScreenByFuncId(funcId) == -1 ? FALSE : TRUE;
}

static void EnterQuizLadyScreen(u16 funcId)
{
    int i;

    i = GetQuizLadyScreenByFuncId(funcId);
    ResetTasks();
    ExitEasyChatScreen(sQuizLadyEasyChatScreens[i].callback);
}

static void DoQuizAnswerEasyChatScreen(void)
{
    DoEasyChatScreen(
        EASY_CHAT_TYPE_QUIZ_ANSWER,
        &gSaveBlock1Ptr->lilycoveLady.quiz.playerAnswer,
        CB2_ReturnToFieldContinueScript,
        EASY_CHAT_PERSON_DISPLAY_NONE);
}

static void DoQuizQuestionEasyChatScreen(void)
{
    DoEasyChatScreen(EASY_CHAT_TYPE_QUIZ_QUESTION,
        gSaveBlock1Ptr->lilycoveLady.quiz.question,
        CB2_ReturnToFieldContinueScript,
        EASY_CHAT_PERSON_DISPLAY_NONE);
}

static void DoQuizSetAnswerEasyChatScreen(void)
{
    DoEasyChatScreen(EASY_CHAT_TYPE_QUIZ_SET_ANSWER,
        &gSaveBlock1Ptr->lilycoveLady.quiz.correctAnswer,
        CB2_ReturnToFieldContinueScript,
        EASY_CHAT_PERSON_DISPLAY_NONE);
}

static void DoQuizSetQuestionEasyChatScreen(void)
{
    DoEasyChatScreen(EASY_CHAT_TYPE_QUIZ_SET_QUESTION,
        gSaveBlock1Ptr->lilycoveLady.quiz.question,
        CB2_ReturnToFieldContinueScript,
        EASY_CHAT_PERSON_DISPLAY_NONE);
}

static bool8 InitEasyChatScreenStruct(u8 type, u16 *words, u8 displayedPersonType)
{
    u8 templateId;
    int i;

    sEasyChatScreen = Alloc(sizeof(*sEasyChatScreen));
    if (sEasyChatScreen == NULL)
        return FALSE;

    sEasyChatScreen->type = type;
    sEasyChatScreen->savedPhrase = words;
    sEasyChatScreen->mainCursorColumn = 0;
    sEasyChatScreen->mainCursorRow = 0;
    sEasyChatScreen->inAlphabetMode = FALSE;
    sEasyChatScreen->displayedPersonType = displayedPersonType;
    sEasyChatScreen->unused = 0;
    templateId = GetEachChatScreenTemplateId(type);
    if (type == EASY_CHAT_TYPE_QUIZ_QUESTION)
    {
        GetQuizTitle(sEasyChatScreen->quizTitle);
        sEasyChatScreen->titleText = sEasyChatScreen->quizTitle;
        sEasyChatScreen->inputState = INPUTSTATE_QUIZ_QUESTION;
    }
    else
    {
        sEasyChatScreen->inputState = INPUTSTATE_PHRASE;
        sEasyChatScreen->titleText = sEasyChatScreenTemplates[templateId].titleText;
    }

    sEasyChatScreen->numColumns = sEasyChatScreenTemplates[templateId].numColumns;
    sEasyChatScreen->numRows = sEasyChatScreenTemplates[templateId].numRows;
    sEasyChatScreen->maxWords = sEasyChatScreen->numColumns * sEasyChatScreen->numRows;
    sEasyChatScreen->templateId = templateId;
    if (sEasyChatScreen->maxWords > ARRAY_COUNT(sEasyChatScreen->currentPhrase))
        sEasyChatScreen->maxWords = ARRAY_COUNT(sEasyChatScreen->currentPhrase);

    if (words != NULL)
    {
        // Phrase starts with words filled in, copy to current phrase
        CpuCopy16(words, sEasyChatScreen->currentPhrase, sEasyChatScreen->maxWords * sizeof(u16));
    }
    else
    {
        // Phrase starts with no words, fill with empty words and save
        for (i = 0; i < sEasyChatScreen->maxWords; i ++)
            sEasyChatScreen->currentPhrase[i] = EC_EMPTY_WORD;

        sEasyChatScreen->savedPhrase = sEasyChatScreen->currentPhrase;
    }

    sEasyChatScreen->keyboardLastRow = (GetNumUnlockedEasyChatGroups() - 1) / 2 + 1;
    return TRUE;
}

static void FreeEasyChatScreenStruct(void)
{
    TRY_FREE_AND_SET_NULL(sEasyChatScreen);
}

// Returns the function ID of the action to take as a result of player's input.
// If no action is needed, returns ECFUNC_NONE
static u16 HandleEasyChatInput(void)
{
    switch (sEasyChatScreen->inputState)
    {
    case INPUTSTATE_PHRASE:
        return HandleEasyChatInput_Phrase();
    case INPUTSTATE_MAIN_SCREEN_BUTTONS:
        return HandleEasyChatInput_MainScreenButtons();
    case INPUTSTATE_KEYBOARD:
        return HandleEasyChatInput_Keyboard();
    case INPUTSTATE_WORD_SELECT:
        return HandleEasyChatInput_WordSelect();
    case INPUTSTATE_EXIT_PROMPT:
        return HandleEasyChatInput_ExitPrompt();
    case INPUTSTATE_DELETE_ALL_YES_NO:
        return HandleEasyChatInput_DeleteAllYesNo();
    case INPUTSTATE_CONFIRM_WORDS_YES_NO:
        return HandleEasyChatInput_ConfirmWordsYesNo();
    case INPUTSTATE_QUIZ_QUESTION:
        return HandleEasyChatInput_QuizQuestion();
    case INPUTSTATE_WAIT_FOR_MSG:
        return HandleEasyChatInput_WaitForMsg();
    case INPUTSTATE_START_CONFIRM_LYRICS:
        return HandleEasyChatInput_StartConfirmLyrics();
    case INPUTSTATE_CONFIRM_LYRICS_YES_NO:
        return HandleEasyChatInput_ConfirmLyricsYesNo();
    }
    return ECFUNC_NONE;
}

static bool32 IsCurrentFrame2x5(void)
{
    switch (GetEasyChatScreenFrameId())
    {
    case FRAMEID_MAIL:
    case FRAMEID_QUIZ_QUESTION:
    case FRAMEID_QUIZ_SET_QUESTION:
        return TRUE;
    }
    return FALSE;
}

// Handles main screen input while cursor is on a word in the phrase
static u16 HandleEasyChatInput_Phrase(void)
{
    do
    {
        if (JOY_NEW(A_BUTTON))
        {
            ClearUnusedField();
            sEasyChatScreen->inputState = INPUTSTATE_KEYBOARD;
            sEasyChatScreen->keyboardColumn = 0;
            sEasyChatScreen->keyboardRow = 0;
            sEasyChatScreen->keyboardScrollOffset = 0;
            return ECFUNC_OPEN_KEYBOARD;
        }
        else if (JOY_NEW(B_BUTTON))
        {
            return StartConfirmExitPrompt();
        }
        else if (JOY_NEW(START_BUTTON))
        {
            return TryConfirmWords();
        }
        else if (JOY_NEW(DPAD_UP))
        {
            sEasyChatScreen->mainCursorRow--;
            break;
        }
        else if (JOY_NEW(DPAD_LEFT))
        {
            sEasyChatScreen->mainCursorColumn--;
            break;
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            sEasyChatScreen->mainCursorRow++;
            break;
        }
        else if (JOY_NEW(DPAD_RIGHT))
        {
            sEasyChatScreen->mainCursorColumn++;
            break;
        }

        return ECFUNC_NONE;
    } while (0);

    // Handle D-Pad input

    // Wrap row
    if (sEasyChatScreen->mainCursorRow < 0)
        sEasyChatScreen->mainCursorRow = sEasyChatScreenTemplates[sEasyChatScreen->templateId].numRows;
    if (sEasyChatScreen->mainCursorRow > sEasyChatScreenTemplates[sEasyChatScreen->templateId].numRows)
        sEasyChatScreen->mainCursorRow = 0;

    if (sEasyChatScreen->mainCursorRow == sEasyChatScreenTemplates[sEasyChatScreen->templateId].numRows)
    {
        // Moved onto bottom row (buttons)
        if (sEasyChatScreen->mainCursorColumn > 2)
            sEasyChatScreen->mainCursorColumn = 2;

        sEasyChatScreen->inputState = INPUTSTATE_MAIN_SCREEN_BUTTONS;
        return ECFUNC_UPDATE_MAIN_CURSOR_ON_BUTTONS;
    }

    // Wrap column
    if (sEasyChatScreen->mainCursorColumn < 0)
        sEasyChatScreen->mainCursorColumn = sEasyChatScreenTemplates[sEasyChatScreen->templateId].numColumns - 1;
    if (sEasyChatScreen->mainCursorColumn >= sEasyChatScreenTemplates[sEasyChatScreen->templateId].numColumns)
        sEasyChatScreen->mainCursorColumn = 0;

    // All 2x5 phrases are only 9 words long, exclude the bottom right (10th) position
    if (IsCurrentFrame2x5() && sEasyChatScreen->mainCursorColumn == 1 && sEasyChatScreen->mainCursorRow == 4)
        sEasyChatScreen->mainCursorColumn = 0;

    return ECFUNC_UPDATE_MAIN_CURSOR;
}

// Handles main screen input while cursor is below the phrase on one of the buttons, e.g. Del. All or Cancel
static u16 HandleEasyChatInput_MainScreenButtons(void)
{
    do
    {
        if (JOY_NEW(A_BUTTON))
        {
            switch (sEasyChatScreen->mainCursorColumn)
            {
            case 0: // Del. All button
                return DoDeleteAllButton();
            case 1: // Cancel button
                return StartConfirmExitPrompt();
            case 2: // OK button
                return TryConfirmWords();
            case 3: // Quiz/Answer button
                return DoQuizButton();
            }
        }

        if (JOY_NEW(B_BUTTON))
        {
            return StartConfirmExitPrompt();
        }
        else if (JOY_NEW(START_BUTTON))
        {
            return TryConfirmWords();
        }
        else if (JOY_NEW(DPAD_UP))
        {
            sEasyChatScreen->mainCursorRow--;
            break;
        }
        else if (JOY_NEW(DPAD_LEFT))
        {
            sEasyChatScreen->mainCursorColumn--;
            break;
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            sEasyChatScreen->mainCursorRow = 0;
            break;
        }
        else if (JOY_NEW(DPAD_RIGHT))
        {
            sEasyChatScreen->mainCursorColumn++;
            break;
        }

        return ECFUNC_NONE;
    } while (0);

    if (sEasyChatScreen->mainCursorRow == sEasyChatScreenTemplates[sEasyChatScreen->templateId].numRows)
    {
        int numFooterColumns = FooterHasFourOptions() ? 4 : 3;
        if (sEasyChatScreen->mainCursorColumn < 0)
            sEasyChatScreen->mainCursorColumn = numFooterColumns - 1;

        if (sEasyChatScreen->mainCursorColumn >= numFooterColumns)
            sEasyChatScreen->mainCursorColumn = 0;

        return ECFUNC_UPDATE_MAIN_CURSOR_ON_BUTTONS;
    }

    if (sEasyChatScreen->mainCursorColumn >= sEasyChatScreenTemplates[sEasyChatScreen->templateId].numColumns)
        sEasyChatScreen->mainCursorColumn = sEasyChatScreenTemplates[sEasyChatScreen->templateId].numColumns - 1;

    // All 2x5 phrases are only 9 words long, exclude the bottom right (10th) position
    if (IsCurrentFrame2x5() && sEasyChatScreen->mainCursorColumn == 1 && sEasyChatScreen->mainCursorRow == 4)
        sEasyChatScreen->mainCursorColumn = 0;

    sEasyChatScreen->inputState = INPUTSTATE_PHRASE;
    return ECFUNC_UPDATE_MAIN_CURSOR;
}

static u16 HandleEasyChatInput_Keyboard(void)
{
    if (JOY_NEW(B_BUTTON))
        return ExitKeyboardToMainScreen();

    if (JOY_NEW(A_BUTTON))
    {
        if (sEasyChatScreen->keyboardColumn != -1)
            return SelectKeyboardGroup();

        // Cursor is in button window
        switch (sEasyChatScreen->keyboardRow)
        {
        case 0: // Mode button
            return StartSwitchKeyboardMode();
        case 1: // Delete button
            return DeleteSelectedWord();
        case 2: // Cancel button
            return ExitKeyboardToMainScreen();
        }
    }

    if (JOY_NEW(SELECT_BUTTON))
        return StartSwitchKeyboardMode();

    if (JOY_REPEAT(DPAD_UP))
        return MoveKeyboardCursor(INPUT_UP);

    if (JOY_REPEAT(DPAD_DOWN))
        return MoveKeyboardCursor(INPUT_DOWN);

    if (JOY_REPEAT(DPAD_LEFT))
        return MoveKeyboardCursor(INPUT_LEFT);

    if (JOY_REPEAT(DPAD_RIGHT))
        return MoveKeyboardCursor(INPUT_RIGHT);

    return ECFUNC_NONE;
}

// Input handling for the lower window after a word group has been selected
static u16 HandleEasyChatInput_WordSelect(void)
{
    if (JOY_NEW(B_BUTTON))
    {
        sEasyChatScreen->inputState = INPUTSTATE_KEYBOARD;
        return ECFUNC_RETURN_TO_KEYBOARD;
    }

    if (JOY_NEW(A_BUTTON))
        return SelectNewWord();

    if (JOY_NEW(START_BUTTON))
        return MoveWordSelectCursor(INPUT_START);

    if (JOY_NEW(SELECT_BUTTON))
        return MoveWordSelectCursor(INPUT_SELECT);

    if (JOY_REPEAT(DPAD_UP))
        return MoveWordSelectCursor(INPUT_UP);

    if (JOY_REPEAT(DPAD_DOWN))
        return MoveWordSelectCursor(INPUT_DOWN);

    if (JOY_REPEAT(DPAD_LEFT))
        return MoveWordSelectCursor(INPUT_LEFT);

    if (JOY_REPEAT(DPAD_RIGHT))
        return MoveWordSelectCursor(INPUT_RIGHT);

    return ECFUNC_NONE;
}

static u16 HandleEasyChatInput_ExitPrompt(void)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case MENU_B_PRESSED:
    case 1: // No (Continue)
        sEasyChatScreen->inputState = GetEasyChatBackupState();
        return ECFUNC_CLOSE_PROMPT;
    case 0: // Yes (Exit)
        gSpecialVar_Result = 0;
        if (sEasyChatScreen->type == EASY_CHAT_TYPE_QUIZ_SET_QUESTION
         || sEasyChatScreen->type == EASY_CHAT_TYPE_QUIZ_SET_ANSWER)
            SaveCurrentPhrase();

        return ECFUNC_EXIT;
    default:
        return ECFUNC_NONE;
    }
}

static u16 HandleEasyChatInput_ConfirmWordsYesNo(void)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case MENU_B_PRESSED:
    case 1: // No
        sEasyChatScreen->inputState = GetEasyChatBackupState();
        return ECFUNC_CLOSE_PROMPT;
    case 0: // Yes
        SetSpecialEasyChatResult();
        gSpecialVar_Result = GetEasyChatCompleted();
        SaveCurrentPhrase();
        return ECFUNC_EXIT;
    default:
        return ECFUNC_NONE;
    }
}

static u16 HandleEasyChatInput_DeleteAllYesNo(void)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case MENU_B_PRESSED:
    case 1: // No
        sEasyChatScreen->inputState = INPUTSTATE_MAIN_SCREEN_BUTTONS;
        return ECFUNC_CLOSE_PROMPT;
    case 0: // Yes
        ResetCurrentPhrase();
        sEasyChatScreen->inputState = INPUTSTATE_MAIN_SCREEN_BUTTONS;
        return ECFUNC_CLOSE_PROMPT_AFTER_DELETE;
    default:
        return ECFUNC_NONE;
    }
}

static u16 HandleEasyChatInput_QuizQuestion(void)
{
    if (JOY_NEW(A_BUTTON))
        return ECFUNC_QUIZ_ANSWER;

    if (JOY_NEW(B_BUTTON))
        return StartConfirmExitPrompt();

    return ECFUNC_NONE;
}

// A message has been printed. Wait for player to
// press A or B, then return to previous state
static u16 HandleEasyChatInput_WaitForMsg(void)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        sEasyChatScreen->inputState = GetEasyChatBackupState();
        return ECFUNC_CLOSE_PROMPT;
    }

    return ECFUNC_NONE;
}

// Odd, could have been skipped. Just passes to HandleEasyChatInput_ConfirmLyricsYesNo
static u16 HandleEasyChatInput_StartConfirmLyrics(void)
{
    sEasyChatScreen->inputState = INPUTSTATE_CONFIRM_LYRICS_YES_NO;
    return ECFUNC_PROMPT_CONFIRM;
}

static u16 HandleEasyChatInput_ConfirmLyricsYesNo(void)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case MENU_B_PRESSED:
    case 1: // No
        ResetCurrentPhraseToSaved();
        sEasyChatScreen->inputStateBackup = INPUTSTATE_PHRASE;
        sEasyChatScreen->inputState = INPUTSTATE_WAIT_FOR_MSG;
        return ECFUNC_MSG_SONG_TOO_SHORT;
    case 0: // Yes
        gSpecialVar_Result = GetEasyChatCompleted();
        SaveCurrentPhrase();
        return ECFUNC_EXIT;
    default:
        return ECFUNC_NONE;
    }
}

static u16 StartConfirmExitPrompt(void)
{
    if (sEasyChatScreen->type == EASY_CHAT_TYPE_APPRENTICE
     || sEasyChatScreen->type == EASY_CHAT_TYPE_CONTEST_INTERVIEW)
    {
        sEasyChatScreen->inputStateBackup = sEasyChatScreen->inputState;
        sEasyChatScreen->inputState = INPUTSTATE_WAIT_FOR_MSG;
        return ECFUNC_MSG_CANT_EXIT;
    }
    else
    {
        sEasyChatScreen->inputStateBackup = sEasyChatScreen->inputState;
        sEasyChatScreen->inputState = INPUTSTATE_EXIT_PROMPT;
        return ECFUNC_PROMPT_EXIT;
    }
}

static int DoDeleteAllButton(void)
{
    sEasyChatScreen->inputStateBackup = sEasyChatScreen->inputState;
    if (sEasyChatScreen->type != EASY_CHAT_TYPE_BARD_SONG)
    {
        // Show Delete yes/no
        sEasyChatScreen->inputState = INPUTSTATE_DELETE_ALL_YES_NO;
        return ECFUNC_PROMPT_DELETE_ALL;
    }
    else
    {
        // Cannot delete lyrics when setting Bard's song
        sEasyChatScreen->inputStateBackup = sEasyChatScreen->inputState;
        sEasyChatScreen->inputState = INPUTSTATE_WAIT_FOR_MSG;
        return ECFUNC_MSG_CANT_DELETE_LYRICS;
    }
}

static u16 TryConfirmWords(void)
{
    sEasyChatScreen->inputStateBackup = sEasyChatScreen->inputState;
    if (sEasyChatScreen->type == EASY_CHAT_TYPE_QUIZ_SET_QUESTION)
    {
        if (IsQuizQuestionEmpty())
        {
            sEasyChatScreen->inputState = INPUTSTATE_WAIT_FOR_MSG;
            return ECFUNC_MSG_CREATE_QUIZ;
        }

        if (IsQuizAnswerEmpty())
        {
            sEasyChatScreen->inputState = INPUTSTATE_WAIT_FOR_MSG;
            return ECFUNC_MSG_SELECT_ANSWER;
        }

        sEasyChatScreen->inputState = INPUTSTATE_CONFIRM_WORDS_YES_NO;
        return ECFUNC_PROMPT_CONFIRM;
    }
    else if (sEasyChatScreen->type == EASY_CHAT_TYPE_QUIZ_SET_ANSWER)
    {
        if (IsQuizAnswerEmpty())
        {
            sEasyChatScreen->inputState = INPUTSTATE_WAIT_FOR_MSG;
            return ECFUNC_MSG_SELECT_ANSWER;
        }

        if (IsQuizQuestionEmpty())
        {
            sEasyChatScreen->inputState = INPUTSTATE_WAIT_FOR_MSG;
            return ECFUNC_MSG_CREATE_QUIZ;
        }

        sEasyChatScreen->inputState = INPUTSTATE_CONFIRM_WORDS_YES_NO;
        return ECFUNC_PROMPT_CONFIRM;
    }
    else if (sEasyChatScreen->type == EASY_CHAT_TYPE_TRENDY_PHRASE
          || sEasyChatScreen->type == EASY_CHAT_TYPE_GOOD_SAYING)
    {
        if (!IsCurrentPhraseFull())
        {
            sEasyChatScreen->inputState = INPUTSTATE_WAIT_FOR_MSG;
            return ECFUNC_MSG_COMBINE_TWO_WORDS;
        }

        sEasyChatScreen->inputState = INPUTSTATE_CONFIRM_WORDS_YES_NO;
        return ECFUNC_PROMPT_CONFIRM;
    }
    else if (sEasyChatScreen->type == EASY_CHAT_TYPE_APPRENTICE
          || sEasyChatScreen->type == EASY_CHAT_TYPE_CONTEST_INTERVIEW)
    {
        if (IsCurrentPhraseEmpty())
        {
            sEasyChatScreen->inputState = INPUTSTATE_WAIT_FOR_MSG;
            return ECFUNC_MSG_CANT_EXIT;
        }

        sEasyChatScreen->inputState = INPUTSTATE_CONFIRM_WORDS_YES_NO;
        return ECFUNC_PROMPT_CONFIRM;
    }
    else if (sEasyChatScreen->type == EASY_CHAT_TYPE_QUESTIONNAIRE)
    {
        sEasyChatScreen->inputState = INPUTSTATE_CONFIRM_WORDS_YES_NO;
        return ECFUNC_PROMPT_CONFIRM;
    }
    else
    {
        if (IsCurrentPhraseEmpty() == TRUE || !GetEasyChatCompleted())
        {
            sEasyChatScreen->inputState = INPUTSTATE_EXIT_PROMPT;
            return ECFUNC_PROMPT_EXIT;
        }

        sEasyChatScreen->inputState = INPUTSTATE_CONFIRM_WORDS_YES_NO;
        return ECFUNC_PROMPT_CONFIRM;
    }
}

static int DoQuizButton(void)
{
    sEasyChatScreen->inputStateBackup = sEasyChatScreen->inputState;
    switch (sEasyChatScreen->type)
    {
    case EASY_CHAT_TYPE_QUIZ_ANSWER:
        return ECFUNC_QUIZ_QUESTION;
    case EASY_CHAT_TYPE_QUIZ_SET_QUESTION:
        SaveCurrentPhrase();
        return ECFUNC_SET_QUIZ_ANSWER;
    case EASY_CHAT_TYPE_QUIZ_SET_ANSWER:
        SaveCurrentPhrase();
        return ECFUNC_SET_QUIZ_QUESTION;
    default:
        return ECFUNC_NONE;
    }
}

static u8 GetEasyChatBackupState(void)
{
    return sEasyChatScreen->inputStateBackup;
}

static int SelectKeyboardGroup(void)
{
    u16 numWords;

    if (!sEasyChatScreen->inAlphabetMode)
    {
        u8 groupId = GetUnlockedEasyChatGroupId(GetSelectedGroupIndex());
        SetSelectedWordGroup(FALSE, groupId);
    }
    else
    {
        SetSelectedWordGroup(TRUE, GetSelectedAlphabetGroupId());
    }

    numWords = GetNumWordsInSelectedGroup();
    if (numWords == 0)
        return ECFUNC_NONE;

    sEasyChatScreen->wordSelectLastRow = (numWords - 1) / 2;
    sEasyChatScreen->wordSelectScrollOffset = 0;
    sEasyChatScreen->wordSelectColumn = 0;
    sEasyChatScreen->wordSelectRow = 0;
    sEasyChatScreen->inputState = INPUTSTATE_WORD_SELECT;
    return ECFUNC_OPEN_WORD_SELECT;
}

static int ExitKeyboardToMainScreen(void)
{
    sEasyChatScreen->inputState = INPUTSTATE_PHRASE;
    return ECFUNC_CLOSE_KEYBOARD;
}

static int StartSwitchKeyboardMode(void)
{
    sEasyChatScreen->keyboardColumn = 0;
    sEasyChatScreen->keyboardRow = 0;
    sEasyChatScreen->keyboardScrollOffset = 0;
    if (!sEasyChatScreen->inAlphabetMode)
        sEasyChatScreen->inAlphabetMode = TRUE;
    else
        sEasyChatScreen->inAlphabetMode = FALSE;

    return ECFUNC_SWITCH_KEYBOARD_MODE;
}

static int DeleteSelectedWord(void)
{
    if (sEasyChatScreen->type == EASY_CHAT_TYPE_BARD_SONG)
    {
        PlaySE(SE_FAILURE);
        return ECFUNC_NONE;
    }
    else
    {
        SetSelectedWord(EC_EMPTY_WORD);
        return ECFUNC_REPRINT_PHRASE;
    }
}

static int SelectNewWord(void)
{
    u16 easyChatWord = GetWordFromSelectedGroup(GetSelectedWordIndex());
    if (DummyWordCheck(easyChatWord))
    {
        // Never reached. Would disallow selecting certain words
        PlaySE(SE_FAILURE);
        return ECFUNC_NONE;
    }
    else
    {
        SetSelectedWord(easyChatWord);
        if (sEasyChatScreen->type != EASY_CHAT_TYPE_BARD_SONG)
        {
            sEasyChatScreen->inputState = INPUTSTATE_PHRASE;
            return ECFUNC_CLOSE_WORD_SELECT;
        }
        else
        {
            sEasyChatScreen->inputState = INPUTSTATE_START_CONFIRM_LYRICS;
            return ECFUNC_PROMPT_CONFIRM_LYRICS;
        }
    }
}

static void SaveCurrentPhrase(void)
{
    int i;
    for (i = 0; i < sEasyChatScreen->maxWords; i++)
        sEasyChatScreen->savedPhrase[i] = sEasyChatScreen->currentPhrase[i];
}

static void ResetCurrentPhrase(void)
{
    int i;
    for (i = 0; i < sEasyChatScreen->maxWords; i++)
        sEasyChatScreen->currentPhrase[i] = EC_EMPTY_WORD;
}

static void ResetCurrentPhraseToSaved(void)
{
    int i;
    for (i = 0; i < sEasyChatScreen->maxWords; i++)
        sEasyChatScreen->currentPhrase[i] = sEasyChatScreen->savedPhrase[i];
}

static void SetSelectedWord(u16 easyChatWord)
{
    u16 index = GetWordIndexToReplace();
    sEasyChatScreen->currentPhrase[index] = easyChatWord;
}

// Compare current phrase to the original saved phrase
static bool8 DidPhraseChange(void)
{
    u16 i;
    for (i = 0; i < sEasyChatScreen->maxWords; i++)
    {
        if (sEasyChatScreen->currentPhrase[i] != sEasyChatScreen->savedPhrase[i])
            return TRUE;
    }

    return FALSE;
}

// 'Completed' if the phrase was changed, or in the case of making a quiz, the question and answer were filled out
static bool32 GetEasyChatCompleted(void)
{
    if (sEasyChatScreen->type == EASY_CHAT_TYPE_QUIZ_SET_QUESTION
     || sEasyChatScreen->type == EASY_CHAT_TYPE_QUIZ_SET_ANSWER)
    {
        if (IsQuizQuestionEmpty())
            return FALSE;

        if (IsQuizAnswerEmpty())
            return FALSE;

        return TRUE;
    }
    else
    {
        return DidPhraseChange();
    }
}

static u16 MoveKeyboardCursor(int input)
{
    if (sEasyChatScreen->keyboardColumn != -1)
    {
        if (!sEasyChatScreen->inAlphabetMode)
            return MoveKeyboardCursor_GroupNames(input);
        else
            return MoveKeyboardCursor_Alphabet(input);
    }
    else
    {
        return MoveKeyboardCursor_ButtonWindow(input);
    }
}

static int MoveKeyboardCursor_GroupNames(u32 input)
{
    switch (input)
    {
    case INPUT_UP:
        if (sEasyChatScreen->keyboardRow != -sEasyChatScreen->keyboardScrollOffset)
        {
            if (sEasyChatScreen->keyboardRow)
            {
                sEasyChatScreen->keyboardRow--;
                return ECFUNC_UPDATE_KEYBOARD_CURSOR;
            }
            else
            {
                sEasyChatScreen->keyboardScrollOffset--;
                return ECFUNC_GROUP_NAMES_SCROLL_UP;
            }
        }
        break;
    case INPUT_DOWN:
        if (sEasyChatScreen->keyboardRow + sEasyChatScreen->keyboardScrollOffset < sEasyChatScreen->keyboardLastRow - 1)
        {
            int funcId;
            if (sEasyChatScreen->keyboardRow < NUM_GROUP_NAME_ROWS - 1)
            {
                sEasyChatScreen->keyboardRow++;
                funcId = ECFUNC_UPDATE_KEYBOARD_CURSOR;
            }
            else
            {
                sEasyChatScreen->keyboardScrollOffset++;
                funcId = ECFUNC_GROUP_NAMES_SCROLL_DOWN;
            }

            ReduceToValidKeyboardColumn();
            return funcId;
        }
        break;
    case INPUT_LEFT:
        if (sEasyChatScreen->keyboardColumn)
            sEasyChatScreen->keyboardColumn--;
        else
            SetKeyboardCursorInButtonWindow();

        return ECFUNC_UPDATE_KEYBOARD_CURSOR;
    case INPUT_RIGHT:
        if (sEasyChatScreen->keyboardColumn < 1)
        {
            sEasyChatScreen->keyboardColumn++;
            if (IsSelectedKeyboardIndexInvalid())
                SetKeyboardCursorInButtonWindow();
        }
        else
        {
            SetKeyboardCursorInButtonWindow();
        }
        return ECFUNC_UPDATE_KEYBOARD_CURSOR;
    }

    return ECFUNC_NONE;
}

static int MoveKeyboardCursor_Alphabet(u32 input)
{
    switch (input)
    {
    case INPUT_UP:
        if (sEasyChatScreen->keyboardRow > 0)
            sEasyChatScreen->keyboardRow--;
        else
            sEasyChatScreen->keyboardRow = NUM_ALPHABET_ROWS - 1;

        ReduceToValidKeyboardColumn();
        return ECFUNC_UPDATE_KEYBOARD_CURSOR;
    case INPUT_DOWN:
        if (sEasyChatScreen->keyboardRow < NUM_ALPHABET_ROWS - 1)
            sEasyChatScreen->keyboardRow++;
        else
            sEasyChatScreen->keyboardRow = 0;

        ReduceToValidKeyboardColumn();
        return ECFUNC_UPDATE_KEYBOARD_CURSOR;
    case INPUT_RIGHT:
        sEasyChatScreen->keyboardColumn++;
        if (IsSelectedKeyboardIndexInvalid())
            SetKeyboardCursorInButtonWindow();

        return ECFUNC_UPDATE_KEYBOARD_CURSOR;
    case INPUT_LEFT:
        sEasyChatScreen->keyboardColumn--;
        if (sEasyChatScreen->keyboardColumn < 0)
            SetKeyboardCursorInButtonWindow();

        return ECFUNC_UPDATE_KEYBOARD_CURSOR;
    }

    return ECFUNC_NONE;
}

static int MoveKeyboardCursor_ButtonWindow(u32 input)
{
    switch (input)
    {
    case INPUT_UP:
        if (sEasyChatScreen->keyboardRow)
            sEasyChatScreen->keyboardRow--;
        else
            sEasyChatScreen->keyboardRow = NUM_BUTTON_ROWS - 1;

        return ECFUNC_UPDATE_KEYBOARD_CURSOR;
    case INPUT_DOWN:
        if (sEasyChatScreen->keyboardRow < NUM_BUTTON_ROWS - 1)
            sEasyChatScreen->keyboardRow++;
        else
            sEasyChatScreen->keyboardRow = 0;

        return ECFUNC_UPDATE_KEYBOARD_CURSOR;
    case INPUT_LEFT:
        sEasyChatScreen->keyboardRow++;
        SetKeyboardCursorToLastColumn();
        return ECFUNC_UPDATE_KEYBOARD_CURSOR;
    case INPUT_RIGHT:
        sEasyChatScreen->keyboardColumn = 0;
        sEasyChatScreen->keyboardRow++;
        return ECFUNC_UPDATE_KEYBOARD_CURSOR;
    }

    return ECFUNC_NONE;
}

static void SetKeyboardCursorInButtonWindow(void)
{
    sEasyChatScreen->keyboardColumn = -1;
    if (sEasyChatScreen->keyboardRow)
        sEasyChatScreen->keyboardRow--;
}

static void SetKeyboardCursorToLastColumn(void)
{
    if (!sEasyChatScreen->inAlphabetMode)
    {
        sEasyChatScreen->keyboardColumn = 1;
        ReduceToValidKeyboardColumn();
    }
    else
    {
        sEasyChatScreen->keyboardColumn = GetLastAlphabetColumn(sEasyChatScreen->keyboardRow);
    }
}

static u16 MoveWordSelectCursor(u32 input)
{
    u16 funcId;
    switch (input)
    {
    case INPUT_UP:
        if (sEasyChatScreen->wordSelectRow + sEasyChatScreen->wordSelectScrollOffset > 0)
        {
            if (sEasyChatScreen->wordSelectRow > 0)
            {
                sEasyChatScreen->wordSelectRow--;
                funcId = ECFUNC_UPDATE_WORD_SELECT_CURSOR;
            }
            else
            {
                sEasyChatScreen->wordSelectScrollOffset--;
                funcId = ECFUNC_WORD_SELECT_SCROLL_UP;
            }

            ReduceToValidWordSelectColumn();
            return funcId;
        }
        break;
    case INPUT_DOWN:
        if (sEasyChatScreen->wordSelectRow + sEasyChatScreen->wordSelectScrollOffset < sEasyChatScreen->wordSelectLastRow)
        {
            if (sEasyChatScreen->wordSelectRow < NUM_WORD_SELECT_ROWS - 1)
            {
                sEasyChatScreen->wordSelectRow++;
                funcId = ECFUNC_UPDATE_WORD_SELECT_CURSOR;
            }
            else
            {
                sEasyChatScreen->wordSelectScrollOffset++;
                funcId = ECFUNC_WORD_SELECT_SCROLL_DOWN;
            }

            ReduceToValidWordSelectColumn();
            return funcId;
        }
        break;
    case INPUT_LEFT:
        if (sEasyChatScreen->wordSelectColumn > 0)
            sEasyChatScreen->wordSelectColumn--;
        else
            sEasyChatScreen->wordSelectColumn = 1;

        ReduceToValidWordSelectColumn();
        return ECFUNC_UPDATE_WORD_SELECT_CURSOR;
    case INPUT_RIGHT:
        if (sEasyChatScreen->wordSelectColumn < 1)
        {
            sEasyChatScreen->wordSelectColumn++;
            if (IsSelectedWordIndexInvalid())
                sEasyChatScreen->wordSelectColumn = 0;
        }
        else
        {
            sEasyChatScreen->wordSelectColumn = 0;
        }
        return ECFUNC_UPDATE_WORD_SELECT_CURSOR;
    case INPUT_START:
        // Page scroll up
        if (sEasyChatScreen->wordSelectScrollOffset)
        {
            if (sEasyChatScreen->wordSelectScrollOffset >= NUM_WORD_SELECT_ROWS)
                sEasyChatScreen->wordSelectScrollOffset -= NUM_WORD_SELECT_ROWS;
            else
                sEasyChatScreen->wordSelectScrollOffset = 0;

            return ECFUNC_WORD_SELECT_PAGE_UP;
        }
        break;
    case INPUT_SELECT:
        // Page scroll down
        if (sEasyChatScreen->wordSelectScrollOffset <= sEasyChatScreen->wordSelectLastRow - NUM_WORD_SELECT_ROWS)
        {
            sEasyChatScreen->wordSelectScrollOffset += NUM_WORD_SELECT_ROWS;
            if (sEasyChatScreen->wordSelectScrollOffset > sEasyChatScreen->wordSelectLastRow - NUM_WORD_SELECT_ROWS + 1)
                sEasyChatScreen->wordSelectScrollOffset = sEasyChatScreen->wordSelectLastRow - NUM_WORD_SELECT_ROWS + 1;

            ReduceToValidWordSelectColumn();
            return ECFUNC_WORD_SELECT_PAGE_DOWN;
        }
        break;
    }

    return ECFUNC_NONE;
}

static u16 GetWordIndexToReplace(void)
{
    return (sEasyChatScreen->mainCursorRow * sEasyChatScreen->numColumns) + sEasyChatScreen->mainCursorColumn;
}

static u16 GetSelectedGroupIndex(void)
{
    return NUM_GROUP_NAME_COLUMNS * (sEasyChatScreen->keyboardRow + sEasyChatScreen->keyboardScrollOffset) + sEasyChatScreen->keyboardColumn;
}

static int GetSelectedAlphabetGroupId(void)
{
    int column = (u8)sEasyChatScreen->keyboardColumn < NUM_ALPHABET_COLUMNS ? sEasyChatScreen->keyboardColumn : 0;
    int row = (u8)sEasyChatScreen->keyboardRow < NUM_ALPHABET_ROWS ? sEasyChatScreen->keyboardRow : 0;
    return sAlphabetGroupIdMap[row][column];
}

static u16 GetSelectedWordIndex(void)
{
    return NUM_WORD_SELECT_COLUMNS * (sEasyChatScreen->wordSelectRow + sEasyChatScreen->wordSelectScrollOffset) + sEasyChatScreen->wordSelectColumn;
}

// Get the index of the last column in the alphabet keyboard, depending on current row
static u8 GetLastAlphabetColumn(u8 row)
{
    switch (row)
    {
    case 0:
    default:
        return NUM_ALPHABET_COLUMNS - 1;
    case 1:
        return NUM_ALPHABET_COLUMNS - 2; // At 6 letters, only the 2nd row (index 1) has less than the max columns
                                         // The 3rd and 4th row have 7 letters, the 1st row has 6 letters and 'Others'
    }
}

static void ReduceToValidKeyboardColumn(void)
{
    while (IsSelectedKeyboardIndexInvalid())
    {
        if (sEasyChatScreen->keyboardColumn)
            sEasyChatScreen->keyboardColumn--;
        else
            break;
    }
}

static void ReduceToValidWordSelectColumn(void)
{
    while (IsSelectedWordIndexInvalid())
    {
        if (sEasyChatScreen->wordSelectColumn)
            sEasyChatScreen->wordSelectColumn--;
        else
            break;
    }
}

static bool8 IsSelectedKeyboardIndexInvalid(void)
{
    if (!sEasyChatScreen->inAlphabetMode)
        return GetSelectedGroupIndex() >= GetNumUnlockedEasyChatGroups() ? TRUE : FALSE;
    else
        return sEasyChatScreen->keyboardColumn > GetLastAlphabetColumn(sEasyChatScreen->keyboardRow) ? TRUE : FALSE;
}

static bool8 IsSelectedWordIndexInvalid(void)
{
    return GetSelectedWordIndex() >= GetNumWordsInSelectedGroup() ? TRUE : FALSE;
}

static int FooterHasFourOptions(void)
{
    return sEasyChatScreenTemplates[sEasyChatScreen->templateId].fourFooterOptions;
}

static u8 GetEasyChatScreenType(void)
{
    return sEasyChatScreen->type;
}

static u8 GetEasyChatScreenFrameId(void)
{
    return sEasyChatScreenTemplates[sEasyChatScreen->templateId].frameId;
}

const u8 *GetTitleText(void)
{
    return sEasyChatScreen->titleText;
}

static u16 *GetCurrentPhrase(void)
{
    return sEasyChatScreen->currentPhrase;
}

static u8 GetNumRows(void)
{
    return sEasyChatScreen->numRows;
}

static u8 GetNumColumns(void)
{
    return sEasyChatScreen->numColumns;
}

static u8 GetMainCursorColumn(void)
{
    return sEasyChatScreen->mainCursorColumn;
}

static u8 GetMainCursorRow(void)
{
    return sEasyChatScreen->mainCursorRow;
}

static void GetEasyChatInstructionsText(const u8 **str1, const u8 **str2)
{
    *str1 = sEasyChatScreenTemplates[sEasyChatScreen->templateId].instructionsText1;
    *str2 = sEasyChatScreenTemplates[sEasyChatScreen->templateId].instructionsText2;
}

static void GetEasyChatConfirmText(const u8 **str1, const u8 **str2)
{
    *str1 = sEasyChatScreenTemplates[sEasyChatScreen->templateId].confirmText1;
    *str2 = sEasyChatScreenTemplates[sEasyChatScreen->templateId].confirmText2;
}

static void GetEasyChatConfirmExitText(const u8 **str1, const u8 **str2)
{
    switch (sEasyChatScreen->type)
    {
    case EASY_CHAT_TYPE_MAIL:
        *str1 = gText_StopGivingPkmnMail;
        *str2 = NULL;
        break;
    case EASY_CHAT_TYPE_QUIZ_ANSWER:
    case EASY_CHAT_TYPE_QUIZ_QUESTION:
        *str1 = gText_LikeToQuitQuiz;
        *str2 = gText_ChallengeQuestionMark;
        break;
    default:
        *str1 = gText_QuitEditing;
        *str2 = NULL;
        break;
    }

}

static void GetEasyChatConfirmDeletionText(const u8 **str1, const u8 **str2)
{
    *str1 = gText_AllTextBeingEditedWill;
    *str2 = gText_BeDeletedThatOkay;
}

static void GetKeyboardCursorColAndRow(s8 *column, s8 *row)
{
    *column = sEasyChatScreen->keyboardColumn;
    *row = sEasyChatScreen->keyboardRow;
}

static bool8 GetInAlphabetMode(void)
{
    return sEasyChatScreen->inAlphabetMode;
}

static u8 GetKeyboardScrollOffset(void)
{
    return sEasyChatScreen->keyboardScrollOffset;
}

static void GetWordSelectColAndRow(s8 *column, s8 *row)
{
    *column = sEasyChatScreen->wordSelectColumn;
    *row = sEasyChatScreen->wordSelectRow;
}

static u8 GetWordSelectScrollOffset(void)
{
    return sEasyChatScreen->wordSelectScrollOffset;
}

static u8 GetWordSelectLastRow(void)
{
    return sEasyChatScreen->wordSelectLastRow;
}

static u8 UNUSED UnusedDummy(void)
{
    return FALSE;
}

static bool32 CanScrollUp(void)
{
    switch (sEasyChatScreen->inputState)
    {
    case INPUTSTATE_KEYBOARD:
        if (!sEasyChatScreen->inAlphabetMode && sEasyChatScreen->keyboardScrollOffset)
            return TRUE;
        break;
    case INPUTSTATE_WORD_SELECT:
        if (sEasyChatScreen->wordSelectScrollOffset)
            return TRUE;
        break;
    }

    return FALSE;
}

static bool32 CanScrollDown(void)
{
    switch (sEasyChatScreen->inputState)
    {
    case INPUTSTATE_KEYBOARD:
        if (!sEasyChatScreen->inAlphabetMode && sEasyChatScreen->keyboardScrollOffset + NUM_GROUP_NAME_ROWS <= sEasyChatScreen->keyboardLastRow - 1)
            return TRUE;
        break;
    case INPUTSTATE_WORD_SELECT:
        if (sEasyChatScreen->wordSelectScrollOffset + NUM_WORD_SELECT_ROWS <= sEasyChatScreen->wordSelectLastRow)
            return TRUE;
        break;
    }

    return FALSE;
}

static int FooterHasFourOptions_(void)
{
    return FooterHasFourOptions();
}

static bool8 IsPhraseDifferentThanPlayerInput(const u16 *phrase, u8 phraseLength)
{
    u8 i;

    for (i = 0; i < phraseLength; i++)
    {
        if (phrase[i] != sEasyChatScreen->currentPhrase[i])
            return TRUE;
    }

    return FALSE;
}

static u8 GetDisplayedPersonType(void)
{
    return sEasyChatScreen->displayedPersonType;
}

static u8 GetEachChatScreenTemplateId(u8 type)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sEasyChatScreenTemplates); i++)
    {
        if (sEasyChatScreenTemplates[i].type == type)
            return i;
    }

    return 0;
}

static bool32 IsCurrentPhraseEmpty(void)
{
    int i;

    for (i = 0; i < sEasyChatScreen->maxWords; i++)
    {
        if (sEasyChatScreen->currentPhrase[i] != EC_EMPTY_WORD)
            return FALSE;
    }

    return TRUE;
}

static bool32 IsCurrentPhraseFull(void)
{
    int i;

    for (i = 0; i < sEasyChatScreen->maxWords; i++)
    {
        if (sEasyChatScreen->currentPhrase[i] == EC_EMPTY_WORD)
            return FALSE;
    }

    return TRUE;
}

static int IsQuizQuestionEmpty(void)
{
    int i;
    struct SaveBlock1 *saveBlock1;

    if (sEasyChatScreen->type == EASY_CHAT_TYPE_QUIZ_SET_QUESTION)
        return IsCurrentPhraseEmpty();

    saveBlock1 = gSaveBlock1Ptr;
    for (i = 0; i < QUIZ_QUESTION_LEN; i++)
    {
        if (saveBlock1->lilycoveLady.quiz.question[i] != EC_EMPTY_WORD)
            return FALSE;
    }

    return TRUE;
}

static int IsQuizAnswerEmpty(void)
{
    struct LilycoveLadyQuiz *quiz;
    if (sEasyChatScreen->type == EASY_CHAT_TYPE_QUIZ_SET_ANSWER)
        return IsCurrentPhraseEmpty();

    quiz = &gSaveBlock1Ptr->lilycoveLady.quiz;
    return quiz->correctAnswer == EC_EMPTY_WORD ? TRUE : FALSE;
}

static void GetQuizTitle(u8 *dst)
{
    u8 name[32];
    struct SaveBlock1 *saveBlock1 = gSaveBlock1Ptr;
    DynamicPlaceholderTextUtil_Reset();

    // Buffer author's name
    if (StringLength(saveBlock1->lilycoveLady.quiz.playerName) != 0)
    {
        TVShowConvertInternationalString(name, saveBlock1->lilycoveLady.quiz.playerName, saveBlock1->lilycoveLady.quiz.language);
        DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, name);
    }
    else
    {
        DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, gText_Lady);
    }

    // "<author>'s Quiz"
    DynamicPlaceholderTextUtil_ExpandPlaceholders(dst, gText_F700sQuiz);
}

static void BufferCurrentPhraseToStringVar2(void)
{
    int i;
    u16 *phrase;
    u8 *str;

    phrase = sEasyChatScreen->currentPhrase;
    str = gStringVar2;
    i = 0;
    while (i < sEasyChatScreen->maxWords)
    {
        str = CopyEasyChatWordPadded(str, *phrase, 0);
        *str = 0;
        str++;
        phrase++;
        i++;
    }

    str--;
    str[0] = EOS;
}

static void SetSpecialEasyChatResult(void)
{
    switch (sEasyChatScreen->type)
    {
    case EASY_CHAT_TYPE_PROFILE:
        FlagSet(FLAG_SYS_CHAT_USED);
        break;
    case EASY_CHAT_TYPE_QUESTIONNAIRE:
        if (DidPlayerInputMysteryGiftPhrase())
            gSpecialVar_0x8004 = 2;
        else
            gSpecialVar_0x8004 = 0;
        break;
    case EASY_CHAT_TYPE_TRENDY_PHRASE:
        BufferCurrentPhraseToStringVar2();
        gSpecialVar_0x8004 = TrySetTrendyPhrase(sEasyChatScreen->currentPhrase);
        break;
    case EASY_CHAT_TYPE_GOOD_SAYING:
        gSpecialVar_0x8004 = DidPlayerInputABerryMasterWifePhrase();
        break;
    }
}

static int DidPlayerInputMysteryGiftPhrase(void)
{
    return !IsPhraseDifferentThanPlayerInput(sMysteryGiftPhrase, ARRAY_COUNT(sMysteryGiftPhrase));
}

static u16 DidPlayerInputABerryMasterWifePhrase(void)
{
    int i;
    for (i = 0; i < (int)ARRAY_COUNT(sBerryMasterWifePhrases); i++)
    {
        if (!IsPhraseDifferentThanPlayerInput(sBerryMasterWifePhrases[i], ARRAY_COUNT(*sBerryMasterWifePhrases)))
            return i + 1;
    }

    return 0;
}

static void ClearUnusedField(void)
{
    sEasyChatScreen->unused = 0;
}

static bool32 DummyWordCheck(int easyChatWord)
{
    return FALSE;
}

static bool8 InitEasyChatScreenControl(void)
{
    if (!InitEasyChatScreenControl_())
        return FALSE;
    else
        return TRUE;
}

static bool8 LoadEasyChatScreen(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sEasyChatBgTemplates, ARRAY_COUNT(sEasyChatBgTemplates));
        SetBgTilemapBuffer(3, sScreenControl->bg3TilemapBuffer);
        SetBgTilemapBuffer(1, sScreenControl->bg1TilemapBuffer);
        InitWindows(sEasyChatWindowTemplates);
        DeactivateAllTextPrinters();
        LoadEasyChatPalettes();
        InitEasyChatBgs();
        CpuFastFill(0, (void *)OAM, OAM_SIZE);
        break;
    case 1:
        DecompressAndLoadBgGfxUsingHeap(3, gEasyChatWindow_Gfx, 0, 0, 0);
        CopyToBgTilemapBuffer(3, gEasyChatWindow_Tilemap, 0, 0);
        AdjustBgTilemapForFooter();
        BufferFrameTilemap(sScreenControl->bg1TilemapBuffer);
        AddPhraseWindow();
        AddMainScreenButtonWindow();
        CopyBgTilemapBufferToVram(3);
        break;
    case 2:
        DecompressAndLoadBgGfxUsingHeap(1, sTextInputFrame_Gfx, 0, 0, 0);
        CopyBgTilemapBufferToVram(1);
        break;
    case 3:
        PrintTitle();
        PrintInitialInstructions();
        PrintCurrentPhrase();
        DrawLowerWindow();
        break;
    case 4:
        LoadEasyChatGfx();
        if (GetEasyChatScreenType() != EASY_CHAT_TYPE_QUIZ_QUESTION)
            CreateMainCursorSprite();
        break;
    case 5:
        if (IsDma3ManagerBusyWithBgCopy())
        {
            return TRUE;
        }
        else
        {
            SetWindowDimensions(0, 0, 0, 0);
            SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR);
            SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0
                                       | WINOUT_WIN01_BG1
                                       | WINOUT_WIN01_BG3
                                       | WINOUT_WIN01_OBJ
                                       | WINOUT_WIN01_CLR);
            ShowBg(3);
            ShowBg(1);
            ShowBg(2);
            ShowBg(0);
            CreateScrollIndicatorSprites();
            CreateStartSelectButtonSprites();
            TryAddInterviewObjectEvents();
        }
        break;
    default:
        return FALSE;
    }

    sScreenControl->funcState++;
    return TRUE;
}

static void FreeEasyChatScreenControl(void)
{
    TRY_FREE_AND_SET_NULL(sScreenControl);
}

static void StartEasyChatFunction(u16 funcId)
{
    sScreenControl->currentFuncId = funcId;
    sScreenControl->funcState = 0;
    RunEasyChatFunction();
}

// Returns FALSE when called function has finished
static bool8 RunEasyChatFunction(void)
{
    switch (sScreenControl->currentFuncId)
    {
    case ECFUNC_NONE: return FALSE;
    case ECFUNC_REPRINT_PHRASE: return ReprintPhrase();
    case ECFUNC_UPDATE_MAIN_CURSOR: return UpdateMainCursor();
    case ECFUNC_UPDATE_MAIN_CURSOR_ON_BUTTONS: return UpdateMainCursorOnButtons();
    case ECFUNC_PROMPT_DELETE_ALL: return ShowConfirmDeleteAllPrompt();
    case ECFUNC_PROMPT_EXIT: return ShowConfirmExitPrompt();
    case ECFUNC_PROMPT_CONFIRM: return ShowConfirmPrompt();
    case ECFUNC_CLOSE_PROMPT: return ClosePrompt();
    case ECFUNC_CLOSE_PROMPT_AFTER_DELETE: return ClosePromptAfterDeleteAll();
    case ECFUNC_OPEN_KEYBOARD: return OpenKeyboard();
    case ECFUNC_CLOSE_KEYBOARD: return CloseKeyboard();
    case ECFUNC_OPEN_WORD_SELECT: return OpenWordSelect();
    case ECFUNC_CLOSE_WORD_SELECT: return CloseWordSelect();
    case ECFUNC_PROMPT_CONFIRM_LYRICS: return ShowConfirmLyricsPrompt();
    case ECFUNC_RETURN_TO_KEYBOARD: return ReturnToKeyboard();
    case ECFUNC_UPDATE_KEYBOARD_CURSOR: return UpdateKeyboardCursor();
    case ECFUNC_GROUP_NAMES_SCROLL_DOWN: return GroupNamesScrollDown();
    case ECFUNC_GROUP_NAMES_SCROLL_UP: return GroupNamesScrollUp();
    case ECFUNC_UPDATE_WORD_SELECT_CURSOR: return UpdateWordSelectCursor();
    case ECFUNC_WORD_SELECT_SCROLL_UP: return WordSelectScrollUp();
    case ECFUNC_WORD_SELECT_SCROLL_DOWN: return WordSelectScrollDown();
    case ECFUNC_WORD_SELECT_PAGE_UP: return WordSelectPageScrollUp();
    case ECFUNC_WORD_SELECT_PAGE_DOWN: return WordSelectPageScrollDown();
    case ECFUNC_SWITCH_KEYBOARD_MODE: return SwitchKeyboardMode();
    case ECFUNC_EXIT: return FALSE;
    case ECFUNC_QUIZ_QUESTION: return FALSE;     // The 4 quiz functions
    case ECFUNC_QUIZ_ANSWER: return FALSE;       // 'finish' automatically
    case ECFUNC_SET_QUIZ_QUESTION: return FALSE; // because they switch to a
    case ECFUNC_SET_QUIZ_ANSWER: return FALSE;   // callback in sQuizLadyEasyChatScreens
    case ECFUNC_MSG_CREATE_QUIZ: return ShowCreateQuizMsg();
    case ECFUNC_MSG_SELECT_ANSWER: return ShowSelectAnswerMsg();
    case ECFUNC_MSG_SONG_TOO_SHORT: return ShowSongTooShortMsg();
    case ECFUNC_MSG_CANT_DELETE_LYRICS: return ShowCantDeleteLyricsMsg();
    case ECFUNC_MSG_COMBINE_TWO_WORDS: return ShowCombineTwoWordsMsg();
    case ECFUNC_MSG_CANT_EXIT: return ShowCantExitMsg();
    default:        return FALSE;
    }
}

// Only used to update the current phrase after a word deletion
static bool8 ReprintPhrase(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        PrintCurrentPhrase();
        sScreenControl->funcState++;
        break;
    case 1:
        return IsDma3ManagerBusyWithBgCopy();
    }

    return TRUE;
}

static bool8 UpdateMainCursor(void)
{
    u8 i;
    u16 *currentPhrase;
    u16 *ecWord;
    u8 frameId;
    u8 cursorColumn, cursorRow, numColumns;
    s16 x;
    int stringWidth;
    int trueStringWidth;
    u8 y;
    u8 str[64];

    currentPhrase = GetCurrentPhrase();
    frameId = GetEasyChatScreenFrameId();
    cursorColumn = GetMainCursorColumn();
    cursorRow = GetMainCursorRow();
    numColumns = GetNumColumns();
    ecWord = &currentPhrase[cursorRow * numColumns];
    x = 8 * sPhraseFrameDimensions[frameId].left + 13;
    for (i = 0; i < cursorColumn; i++)
    {
        if (*ecWord == EC_EMPTY_WORD)
        {
            stringWidth = 72;
        }
        else
        {
            CopyEasyChatWord(str, *ecWord);
            stringWidth = GetStringWidth(FONT_NORMAL, str, 0);
        }

        trueStringWidth = stringWidth + 17;
        x += trueStringWidth;
        ecWord++;
    }

    y = 8 * (sPhraseFrameDimensions[frameId].top + cursorRow * 2);
    SetMainCursorPos(x, y + 8);
    return FALSE;
}

static bool8 UpdateMainCursorOnButtons(void)
{
    u8 xOffset = GetFooterOptionXOffset(GetMainCursorColumn());
    SetMainCursorPos(xOffset, 96);
    return FALSE;
}

static bool8 ShowConfirmExitPrompt(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        StopMainCursorAnim();
        PrintEasyChatStdMessage(MSG_CONFIRM_EXIT);
        CreateEasyChatYesNoMenu(1);
        sScreenControl->funcState++;
        break;
    case 1:
        return IsDma3ManagerBusyWithBgCopy();
    }

    return TRUE;
}

static bool8 ShowConfirmPrompt(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        StopMainCursorAnim();
        PrintEasyChatStdMessage(MSG_CONFIRM);
        CreateEasyChatYesNoMenu(0);
        sScreenControl->funcState++;
        break;
    case 1:
        return IsDma3ManagerBusyWithBgCopy();
    }

    return TRUE;
}

static bool8 ShowConfirmDeleteAllPrompt(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        StopMainCursorAnim();
        PrintEasyChatStdMessage(MSG_CONFIRM_DELETE);
        CreateEasyChatYesNoMenu(1);
        sScreenControl->funcState++;
        break;
    case 1:
        return IsDma3ManagerBusyWithBgCopy();
    }

    return TRUE;
}

static bool8 ClosePrompt(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        StartMainCursorAnim();
        PrintEasyChatStdMessage(MSG_INSTRUCTIONS);
        PrintCurrentPhrase();
        ShowBg(0);
        sScreenControl->funcState++;
        break;
    case 1:
        return IsDma3ManagerBusyWithBgCopy();
    }

    return TRUE;
}

static bool8 ClosePromptAfterDeleteAll(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        StartMainCursorAnim();
        PrintEasyChatStdMessage(MSG_INSTRUCTIONS);
        PrintCurrentPhrase();
        sScreenControl->funcState++;
        // Fall through
    case 1:
        return IsDma3ManagerBusyWithBgCopy();
    }

    return TRUE;
}

static bool8 OpenKeyboard(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        StopMainCursorAnim();
        HideBg(0);
        SetWindowDimensions(0, 0, 0, 0);
        PrintKeyboardText();
        sScreenControl->funcState++;
        break;
    case 1:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            InitLowerWindowAnim(WINANIM_OPEN_KEYBOARD);
            sScreenControl->funcState++;
        }
        break;
    case 2:
        if (!IsDma3ManagerBusyWithBgCopy() && !UpdateLowerWindowAnim())
            sScreenControl->funcState++;
        break;
    case 3:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            CreateSideWindowSprites();
            sScreenControl->funcState++;
        }
        break;
    case 4:
        if (!ShowSideWindow())
        {
            CreateRectangleCursorSprites();
            SetScrollIndicatorXPos(FALSE);
            UpdateScrollIndicatorsVisibility();
            sScreenControl->funcState++;
            return FALSE;
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

static bool8 CloseKeyboard(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        DestroyRectangleCursorSprites();
        HideModeWindow();
        HideScrollIndicators();
        sScreenControl->funcState++;
        break;
    case 1:
        if (DestroySideWindowSprites() == TRUE)
            break;

        InitLowerWindowAnim(WINANIM_CLOSE_KEYBOARD);
        sScreenControl->funcState++;
        // Fall through
    case 2:
        if (!UpdateLowerWindowAnim())
            sScreenControl->funcState++;
        break;
    case 3:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            StartMainCursorAnim();
            ShowBg(0);
            sScreenControl->funcState++;
        }
        break;
    case 4:
        return FALSE;
    }

    return TRUE;
}

static bool8 SwitchKeyboardMode(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        DestroyRectangleCursorSprites();
        HideScrollIndicators();
        SetModeWindowToTransition();
        InitLowerWindowAnim(WINANIM_KEYBOARD_SWITCH_OUT);
        sScreenControl->funcState++;
        break;
    case 1:
        if (!UpdateLowerWindowAnim() && !IsModeWindowAnimActive())
        {
            PrintKeyboardText();
            sScreenControl->funcState++;
        }
        break;
    case 2:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            InitLowerWindowAnim(WINANIM_KEYBOARD_SWITCH_IN);
            UpdateModeWindowAnim();
            sScreenControl->funcState++;
        }
        break;
    case 3:
        if (!UpdateLowerWindowAnim() && !IsModeWindowAnimActive())
        {
            UpdateScrollIndicatorsVisibility();
            CreateRectangleCursorSprites();
            sScreenControl->funcState++;
            return FALSE;
        }
        break;
    case 4:
        return FALSE;
    }

    return TRUE;
}

static bool8 UpdateKeyboardCursor(void)
{
    UpdateRectangleCursorPos();
    return FALSE;
}

static bool8 GroupNamesScrollDown(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        InitLowerWindowScroll(1, 4);
        sScreenControl->funcState++;
        // Fall through
    case 1:
        if (!UpdateLowerWindowScroll())
        {
            UpdateRectangleCursorPos();
            UpdateScrollIndicatorsVisibility();
            return FALSE;
        }
        break;
    }

    return TRUE;
}

static bool8 GroupNamesScrollUp(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        InitLowerWindowScroll(-1, 4);
        sScreenControl->funcState++;
        // Fall through
    case 1:
        if (!UpdateLowerWindowScroll())
        {
            UpdateScrollIndicatorsVisibility();
            sScreenControl->funcState++;
            return FALSE;
        }
        break;
    case 2:
        return FALSE;
    }

    return TRUE;
}

static bool8 OpenWordSelect(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        DestroyRectangleCursorSprites();
        HideModeWindow();
        HideScrollIndicators();
        sScreenControl->funcState++;
        break;
    case 1:
        if (!DestroySideWindowSprites())
        {
            ClearWordSelectWindow();
            sScreenControl->funcState++;
        }
        break;
    case 2:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            InitLowerWindowAnim(WINANIM_OPEN_WORD_SELECT);
            sScreenControl->funcState++;
        }
        break;
    case 3:
        if (!UpdateLowerWindowAnim())
        {
            InitLowerWindowText(TEXT_WORD_SELECT);
            sScreenControl->funcState++;
        }
        break;
    case 4:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            CreateWordSelectCursorSprite();
            SetScrollIndicatorXPos(TRUE);
            UpdateScrollIndicatorsVisibility();
            UpdateStartSelectButtonsVisibility();
            sScreenControl->funcState++;
            return FALSE;
        }
        break;
    case 5:
        return FALSE;
    }

    return TRUE;
}

static bool8 CloseWordSelect(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        PrintCurrentPhrase();
        sScreenControl->funcState++;
        break;
    case 1:
        DestroyWordSelectCursorSprite();
        HideScrollIndicators();
        HideStartSelectButtons();
        ClearWordSelectWindow();
        sScreenControl->funcState++;
        break;
    case 2:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            InitLowerWindowAnim(WINANIM_CLOSE_WORD_SELECT);
            sScreenControl->funcState++;
        }
        break;
    case 3:
        if (!UpdateLowerWindowAnim())
        {
            ShowBg(0);
            sScreenControl->funcState++;
        }
        break;
    case 4:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            StartMainCursorAnim();
            sScreenControl->funcState++;
            return FALSE;
        }
        break;
    case 5:
        return FALSE;
    }

    return TRUE;
}

static bool8 ShowConfirmLyricsPrompt(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        PrintCurrentPhrase();
        sScreenControl->funcState++;
        break;
    case 1:
        DestroyWordSelectCursorSprite();
        HideScrollIndicators();
        HideStartSelectButtons();
        ClearWordSelectWindow();
        sScreenControl->funcState++;
        break;
    case 2:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            InitLowerWindowAnim(WINANIM_CLOSE_WORD_SELECT);
            sScreenControl->funcState++;
        }
        break;
    case 3:
        if (!UpdateLowerWindowAnim())
        {
            PrintEasyChatStdMessage(MSG_CONFIRM);
            sScreenControl->funcState++;
        }
        break;
    case 4:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            ShowBg(0);
            sScreenControl->funcState++;
        }
        break;
    case 5:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            StartMainCursorAnim();
            sScreenControl->funcState++;
            return FALSE;
        }
        break;
    case 6:
        return FALSE;
    }

    return TRUE;
}

static bool8 ReturnToKeyboard(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        DestroyWordSelectCursorSprite();
        HideScrollIndicators();
        HideStartSelectButtons();
        ClearWordSelectWindow();
        sScreenControl->funcState++;
        break;
    case 1:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            InitLowerWindowAnim(WINANIM_RETURN_TO_KEYBOARD);
            sScreenControl->funcState++;
        }
        break;
    case 2:
        if (!UpdateLowerWindowAnim())
        {
            PrintKeyboardText();
            sScreenControl->funcState++;
        }
        break;
    case 3:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            CreateSideWindowSprites();
            sScreenControl->funcState++;
        }
        break;
    case 4:
        if (!ShowSideWindow())
        {
            CreateRectangleCursorSprites();
            SetScrollIndicatorXPos(FALSE);
            UpdateScrollIndicatorsVisibility();
            sScreenControl->funcState++;
            return FALSE;
        }
        break;
    }

    return TRUE;
}

static bool8 UpdateWordSelectCursor(void)
{
    UpdateWordSelectCursorPos();
    return FALSE;
}

static bool8 WordSelectScrollDown(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        PrintWordSelectNextRowDown();
        sScreenControl->funcState++;
        break;
    case 1:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            InitLowerWindowScroll(1, 4);
            sScreenControl->funcState++;
        }
        break;
    case 2:
        if (!UpdateLowerWindowScroll())
        {
            UpdateWordSelectCursorPos();
            UpdateScrollIndicatorsVisibility();
            UpdateStartSelectButtonsVisibility();
            sScreenControl->funcState++;
            return FALSE;
        }
        break;
    case 3:
        return FALSE;
    }

    return TRUE;
}

static bool8 WordSelectScrollUp(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        PrintWordSelectNextRowUp();
        sScreenControl->funcState++;
        break;
    case 1:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            InitLowerWindowScroll(-1, 4);
            sScreenControl->funcState++;
        }
        break;
    case 2:
        if (!UpdateLowerWindowScroll())
        {
            UpdateScrollIndicatorsVisibility();
            UpdateStartSelectButtonsVisibility();
            sScreenControl->funcState++;
            return FALSE;
        }
        break;
    case 3:
        return FALSE;
    }

    return TRUE;
}

static bool8 WordSelectPageScrollDown(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        PrintWordSelectRowsPageDown();
        sScreenControl->funcState++;
        break;
    case 1:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            s16 scrollChange = GetWordSelectScrollOffset() - GetLowerWindowScrollOffset();
            InitLowerWindowScroll(scrollChange, 8);
            sScreenControl->funcState++;
        }
        break;
    case 2:
        if (!UpdateLowerWindowScroll())
        {
            UpdateWordSelectCursorPos();
            UpdateScrollIndicatorsVisibility();
            UpdateStartSelectButtonsVisibility();
            sScreenControl->funcState++;
            return FALSE;
        }
        break;
    case 3:
        return FALSE;
    }

    return TRUE;
}

static bool8 WordSelectPageScrollUp(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        PrintWordSelectRowsPageUp();
        sScreenControl->funcState++;
        break;
    case 1:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            s16 scrollChange = GetWordSelectScrollOffset() - GetLowerWindowScrollOffset();
            InitLowerWindowScroll(scrollChange, 8);
            sScreenControl->funcState++;
        }
        break;
    case 2:
        if (!UpdateLowerWindowScroll())
        {
            UpdateScrollIndicatorsVisibility();
            UpdateStartSelectButtonsVisibility();
            sScreenControl->funcState++;
            return FALSE;
        }
        break;
    case 3:
        return FALSE;
    }

    return TRUE;
}

static bool8 ShowCreateQuizMsg(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        StopMainCursorAnim();
        PrintEasyChatStdMessage(MSG_CREATE_QUIZ);
        sScreenControl->funcState++;
        break;
    case 1:
        return IsDma3ManagerBusyWithBgCopy();
    }

    return TRUE;
}

static bool8 ShowSelectAnswerMsg(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        StopMainCursorAnim();
        PrintEasyChatStdMessage(MSG_SELECT_ANSWER);
        sScreenControl->funcState++;
        break;
    case 1:
        return IsDma3ManagerBusyWithBgCopy();
    }

    return TRUE;
}

static bool8 ShowSongTooShortMsg(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        StopMainCursorAnim();
        PrintEasyChatStdMessage(MSG_SONG_TOO_SHORT);
        sScreenControl->funcState++;
        break;
    case 1:
        return IsDma3ManagerBusyWithBgCopy();
    }

    return TRUE;
}

static bool8 ShowCantDeleteLyricsMsg(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        StopMainCursorAnim();
        PrintEasyChatStdMessage(MSG_CANT_DELETE_LYRICS);
        sScreenControl->funcState++;
        break;
    case 1:
        return IsDma3ManagerBusyWithBgCopy();
    }

    return TRUE;
}

static bool8 ShowCombineTwoWordsMsg(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        StopMainCursorAnim();
        PrintEasyChatStdMessage(MSG_COMBINE_TWO_WORDS);
        sScreenControl->funcState++;
        break;
    case 1:
        return IsDma3ManagerBusyWithBgCopy();
    }

    return TRUE;
}

static bool8 ShowCantExitMsg(void)
{
    switch (sScreenControl->funcState)
    {
    case 0:
        StopMainCursorAnim();
        PrintEasyChatStdMessage(MSG_CANT_QUIT);
        sScreenControl->funcState++;
        break;
    case 1:
        return IsDma3ManagerBusyWithBgCopy();
    }

    return TRUE;
}

static bool8 InitEasyChatScreenControl_(void)
{
    sScreenControl = Alloc(sizeof(*sScreenControl));
    if (!sScreenControl)
        return FALSE;

    sScreenControl->funcState = 0;
    sScreenControl->mainCursorSprite = NULL;
    sScreenControl->rectangleCursorSpriteRight = NULL;
    sScreenControl->rectangleCursorSpriteLeft = NULL;
    sScreenControl->wordSelectCursorSprite = NULL;
    sScreenControl->buttonWindowSprite = NULL;
    sScreenControl->modeWindowSprite = NULL;
    sScreenControl->scrollIndicatorUpSprite = NULL;
    sScreenControl->scrollIndicatorDownSprite = NULL;
    sScreenControl->startButtonSprite = NULL;
    sScreenControl->selectButtonSprite = NULL;
    sScreenControl->fourFooterOptions = FooterHasFourOptions_();
    return TRUE;
}

static void InitEasyChatBgs(void)
{
    ChangeBgX(3, 0, BG_COORD_SET);
    ChangeBgY(3, 0, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
    ChangeBgX(2, 0, BG_COORD_SET);
    ChangeBgY(2, 0, BG_COORD_SET);
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON);
}

static void LoadEasyChatPalettes(void)
{
    ResetPaletteFade();
    LoadPalette(gEasyChatMode_Pal, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
    LoadPalette(sTextInputFrameOrange_Pal, BG_PLTT_ID(1), sizeof(sTextInputFrameOrange_Pal));
    LoadPalette(sTextInputFrameGreen_Pal, BG_PLTT_ID(4), sizeof(sTextInputFrameGreen_Pal));
    LoadPalette(sTitleText_Pal, BG_PLTT_ID(10), sizeof(sTitleText_Pal));
    LoadPalette(sText_Pal, BG_PLTT_ID(11), sizeof(sText_Pal));
    LoadPalette(sText_Pal, BG_PLTT_ID(15), sizeof(sText_Pal));
    LoadPalette(sText_Pal, BG_PLTT_ID(3), sizeof(sText_Pal));
}

static void PrintTitle(void)
{
    int xOffset;
    const u8 *titleText = GetTitleText();
    if (!titleText)
        return;

    xOffset = GetStringCenterAlignXOffset(FONT_NORMAL, titleText, 144);
    FillWindowPixelBuffer(WIN_TITLE, PIXEL_FILL(0));
    PrintEasyChatTextWithColors(WIN_TITLE, FONT_NORMAL, titleText, xOffset, 1, TEXT_SKIP_DRAW, TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY);
    PutWindowTilemap(WIN_TITLE);
    CopyWindowToVram(WIN_TITLE, COPYWIN_FULL);
}

static void PrintEasyChatText(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, u8 speed, void (*callback)(struct TextPrinterTemplate *, u16))
{
    AddTextPrinterParameterized(windowId, fontId, str, x, y, speed, callback);
}

static void PrintEasyChatTextWithColors(u8 windowId, u8 fontId, const u8 *str, u8 left, u8 top, u8 speed, u8 bg, u8 fg, u8 shadow)
{
    u8 color[3];
    color[0] = bg;
    color[1] = fg;
    color[2] = shadow;
    AddTextPrinterParameterized3(windowId, fontId, left, top, color, speed, str);
}

static void PrintInitialInstructions(void)
{
    FillBgTilemapBufferRect(0, 0, 0, 0, 32, 20, 17);
    LoadUserWindowBorderGfx(WIN_MSG, 1, BG_PLTT_ID(14));
    DrawTextBorderOuter(WIN_MSG, 1, 14);
    PrintEasyChatStdMessage(MSG_INSTRUCTIONS);
    PutWindowTilemap(WIN_MSG);
    CopyBgTilemapBufferToVram(0);
}

static void PrintEasyChatStdMessage(u8 msgId)
{
    const u8 *text2 = NULL;
    const u8 *text1 = NULL;
    switch (msgId)
    {
    case MSG_INSTRUCTIONS:
        GetEasyChatInstructionsText(&text1, &text2);
        break;
    case MSG_CONFIRM_EXIT:
        GetEasyChatConfirmExitText(&text1, &text2);
        break;
    case MSG_CONFIRM:
        GetEasyChatConfirmText(&text1, &text2);
        break;
    case MSG_CONFIRM_DELETE:
        GetEasyChatConfirmDeletionText(&text1, &text2);
        break;
    case MSG_CREATE_QUIZ:
        text1 = gText_CreateAQuiz;
        break;
    case MSG_SELECT_ANSWER:
        text1 = gText_SelectTheAnswer;
        break;
    case MSG_SONG_TOO_SHORT:
        text1 = gText_OnlyOnePhrase;
        text2 = gText_OriginalSongWillBeUsed;
        break;
    case MSG_CANT_DELETE_LYRICS:
        text1 = gText_LyricsCantBeDeleted;
        break;
    case MSG_COMBINE_TWO_WORDS:
        text1 = gText_CombineTwoWordsOrPhrases3;
        break;
    case MSG_CANT_QUIT:
        text1 = gText_YouCannotQuitHere;
        text2 = gText_SectionMustBeCompleted;
        break;
    }

    FillWindowPixelBuffer(WIN_MSG, PIXEL_FILL(1));
    if (text1)
        PrintEasyChatText(WIN_MSG, FONT_NORMAL, text1, 0, 1, TEXT_SKIP_DRAW, 0);

    if (text2)
        PrintEasyChatText(WIN_MSG, FONT_NORMAL, text2, 0, 17, TEXT_SKIP_DRAW, 0);

    CopyWindowToVram(WIN_MSG, COPYWIN_FULL);
}

static void CreateEasyChatYesNoMenu(u8 initialCursorPos)
{
    CreateYesNoMenu(&sEasyChatYesNoWindowTemplate, 1, 14, initialCursorPos);
}

static void AddPhraseWindow(void)
{
    u8 frameId;
    struct WindowTemplate template;

    frameId = GetEasyChatScreenFrameId();
    template.bg = 3;
    template.tilemapLeft = sPhraseFrameDimensions[frameId].left;
    template.tilemapTop = sPhraseFrameDimensions[frameId].top;
    template.width = sPhraseFrameDimensions[frameId].width;
    template.height = sPhraseFrameDimensions[frameId].height;
    template.paletteNum = 11;
    template.baseBlock = 0x6C;
    sScreenControl->windowId = AddWindow(&template);
    PutWindowTilemap(sScreenControl->windowId);
}

static void PrintCurrentPhrase(void)
{
    u8 strClear[4];
    u16 *currentPhrase;
    u8 numColumns, numRows;
    u8 *str;
    int frameId;
    bool32 isQuizQuestion;
    int i, j, k;

    currentPhrase = GetCurrentPhrase();
    numColumns = GetNumColumns();
    numRows = GetNumRows();
    frameId = GetEasyChatScreenFrameId();

    isQuizQuestion = FALSE;
    if (frameId == FRAMEID_QUIZ_QUESTION)
        isQuizQuestion = TRUE;

    FillWindowPixelBuffer(sScreenControl->windowId, PIXEL_FILL(1));
    for (i = 0; i < numRows; i++)
    {
        memcpy(strClear, sText_Clear17, sizeof(sText_Clear17));
        if (isQuizQuestion)
            strClear[2] = 6;

        str = sScreenControl->phrasePrintBuffer;
        sScreenControl->phrasePrintBuffer[0] = EOS;
        str = StringAppend(str, strClear);
        for (j = 0; j < numColumns; j++)
        {
            if (*currentPhrase != EC_EMPTY_WORD)
            {
                str = CopyEasyChatWord(str, *currentPhrase);
                currentPhrase++;
            }
            else
            {
                currentPhrase++;
                if (!isQuizQuestion)
                {
                    str = WriteColorChangeControlCode(str, 0, 4);
                    for (k = 0; k < 12; k++)
                    {
                        *str = CHAR_HYPHEN;
                        str++;
                    }

                    str = WriteColorChangeControlCode(str, 0, 2);
                }
            }

            if (isQuizQuestion)
                strClear[2] = 3;

            str = StringAppend(str, strClear);

            if (frameId == FRAMEID_MAIL || frameId == FRAMEID_QUIZ_QUESTION || frameId == FRAMEID_QUIZ_SET_QUESTION)
            {
                // Is 2x5 frame, end on 9th word
                if (j == 0 && i == 4)
                    break;
            }
        }

        *str = EOS;
        PrintEasyChatText(sScreenControl->windowId, FONT_NORMAL, sScreenControl->phrasePrintBuffer, 0, i * 16 + 1, TEXT_SKIP_DRAW, 0);
    }

    CopyWindowToVram(sScreenControl->windowId, COPYWIN_FULL);
}

static void BufferFrameTilemap(u16 *tilemap)
{
    u8 frameId;
    int right, bottom;
    int x, y;

    frameId = GetEasyChatScreenFrameId();
    CpuFastFill(0, tilemap, BG_SCREEN_SIZE);
    if (frameId == FRAMEID_MAIL || frameId == FRAMEID_QUIZ_SET_QUESTION)
    {
        // These frames fill the screen, no need to draw top/bottom edges
        right = sPhraseFrameDimensions[frameId].left + sPhraseFrameDimensions[frameId].width;
        bottom = sPhraseFrameDimensions[frameId].top + sPhraseFrameDimensions[frameId].height;

        // Draw middle section
        for (y = sPhraseFrameDimensions[frameId].top; y < bottom; y++)
        {
            x = sPhraseFrameDimensions[frameId].left - 1;
            tilemap[y * 32 + x] = FRAME_OFFSET_ORANGE + FRAME_TILE_L_EDGE;
            x++;
            for (; x < right; x++)
                tilemap[y * 32 + x] = FRAME_OFFSET_ORANGE + FRAME_TILE_TRANSPARENT;

            tilemap[y* 32 + x] = FRAME_OFFSET_ORANGE + FRAME_TILE_R_EDGE;
        }
    }
    else
    {
        y = sPhraseFrameDimensions[frameId].top - 1;
        x = sPhraseFrameDimensions[frameId].left - 1;
        right = sPhraseFrameDimensions[frameId].left + sPhraseFrameDimensions[frameId].width;
        bottom = sPhraseFrameDimensions[frameId].top + sPhraseFrameDimensions[frameId].height;

        // Draw top edge
        tilemap[y * 32 + x] = FRAME_OFFSET_ORANGE + FRAME_TILE_TOP_L_CORNER;
        x++;
        for (; x < right; x++)
            tilemap[y * 32 + x] = FRAME_OFFSET_ORANGE + FRAME_TILE_TOP_EDGE;

        tilemap[y * 32 + x] = FRAME_OFFSET_ORANGE + FRAME_TILE_TOP_R_CORNER;
        y++;

        // Draw middle section
        for (; y < bottom; y++)
        {
            x = sPhraseFrameDimensions[frameId].left - 1;
            tilemap[y * 32 + x] = FRAME_OFFSET_ORANGE + FRAME_TILE_L_EDGE;
            x++;
            for (; x < right; x++)
                tilemap[y * 32 + x] = FRAME_OFFSET_ORANGE + FRAME_TILE_TRANSPARENT;

            tilemap[y* 32 + x] = FRAME_OFFSET_ORANGE + FRAME_TILE_R_EDGE;
        }

        // Draw bottom edge
        x = sPhraseFrameDimensions[frameId].left - 1;
        tilemap[y * 32 + x] = FRAME_OFFSET_ORANGE + FRAME_TILE_BOTTOM_L_CORNER;
        x++;
        for (; x < right; x++)
            tilemap[y * 32 + x] = FRAME_OFFSET_ORANGE + FRAME_TILE_BOTTOM_EDGE;

        tilemap[y * 32 + x] = FRAME_OFFSET_ORANGE + FRAME_TILE_BOTTOM_R_CORNER;
    }
}

static void AdjustBgTilemapForFooter(void)
{
    u8 frameId;
    u16 *tilemap;

    tilemap = GetBgTilemapBuffer(3);
    frameId = GetEasyChatScreenFrameId();
    switch (sPhraseFrameDimensions[frameId].footerId)
    {
    case FOOTER_ANSWER:
        tilemap += 0x2A0;
        CopyToBgTilemapBufferRect(3, tilemap, 0, 11, 32, 2);
        break;
    case FOOTER_QUIZ:
        tilemap += 0x300;
        CopyToBgTilemapBufferRect(3, tilemap, 0, 11, 32, 2);
        break;
    case NUM_FOOTER_TYPES:
        CopyToBgTilemapBufferRect(3, tilemap, 0, 10, 32, 4);
        break;
    }
}

static void DrawLowerWindow(void)
{
    PutWindowTilemap(WIN_INPUT_SELECT);
    CopyBgTilemapBufferToVram(WIN_INPUT_SELECT);
}

static void InitLowerWindowText(u32 whichText)
{
    ResetLowerWindowScroll();
    FillWindowPixelBuffer(WIN_INPUT_SELECT, PIXEL_FILL(1));
    switch (whichText)
    {
    case TEXT_GROUPS:
        PrintKeyboardGroupNames();
        break;
    case TEXT_ALPHABET:
        PrintKeyboardAlphabet();
        break;
    case TEXT_WORD_SELECT:
        PrintInitialWordSelectText();
        break;
    }

    CopyWindowToVram(WIN_INPUT_SELECT, COPYWIN_GFX);
}

static void PrintKeyboardText(void)
{
    if (!GetInAlphabetMode())
        InitLowerWindowText(TEXT_GROUPS);
    else
        InitLowerWindowText(TEXT_ALPHABET);
}

static void PrintKeyboardGroupNames(void)
{
    int i;
    int x, y;

    i = 0;
    y = 97;
    while (1)
    {
        for (x = 0; x < 2; x++)
        {
            u8 groupId = GetUnlockedEasyChatGroupId(i++);
            if (groupId == EC_NUM_GROUPS)
            {
                InitLowerWindowScroll(GetKeyboardScrollOffset(), 0);
                return;
            }

            PrintEasyChatText(WIN_INPUT_SELECT, FONT_NORMAL, GetEasyChatWordGroupName(groupId), x * 84 + 10, y, TEXT_SKIP_DRAW, NULL);
        }

        y += 16;
    }
}

static void PrintKeyboardAlphabet(void)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sEasyChatKeyboardAlphabet); i++)
        PrintEasyChatText(WIN_INPUT_SELECT, FONT_NORMAL, sEasyChatKeyboardAlphabet[i], 10, 97 + i * 16, TEXT_SKIP_DRAW, NULL);
}

static void PrintInitialWordSelectText(void)
{
    PrintWordSelectText(0, NUM_WORD_SELECT_ROWS);
}

static void PrintWordSelectNextRowDown(void)
{
    u8 wordScroll = GetWordSelectScrollOffset() + NUM_WORD_SELECT_ROWS - 1;
    EraseWordSelectRows(wordScroll, 1);
    PrintWordSelectText(wordScroll, 1);
}

static void PrintWordSelectNextRowUp(void)
{
    u8 wordScroll = GetWordSelectScrollOffset();
    EraseWordSelectRows(wordScroll, 1);
    PrintWordSelectText(wordScroll, 1);
}

static void PrintWordSelectRowsPageDown(void)
{
    u8 wordScroll = GetWordSelectScrollOffset();
    u8 maxScroll = wordScroll + NUM_WORD_SELECT_ROWS;
    u8 maxRows = GetWordSelectLastRow() + 1;
    if (maxScroll > maxRows)
        maxScroll = maxRows;

    if (wordScroll < maxScroll)
    {
        u8 numRows = maxScroll - wordScroll;
        EraseWordSelectRows(wordScroll, numRows);
        PrintWordSelectText(wordScroll, numRows);
    }
}

static void PrintWordSelectRowsPageUp(void)
{
    u8 wordScroll = GetWordSelectScrollOffset();
    u8 windowScroll = GetLowerWindowScrollOffset();
    if (wordScroll < windowScroll)
    {
        u8 numRows = windowScroll - wordScroll;
        EraseWordSelectRows(wordScroll, numRows);
        PrintWordSelectText(wordScroll, numRows);
    }
}

// Print the easy chat words available for selection in
// the currently selected group and at the given offset and row
static void PrintWordSelectText(u8 scrollOffset, u8 numRows)
{
    int i, j;
    u16 easyChatWord;
    int y;
    int wordIndex;

    wordIndex = scrollOffset * NUM_WORD_SELECT_COLUMNS;
    y = (scrollOffset * 16 + 96) & 0xFF;
    y++;
    for (i = 0; i < numRows; i++)
    {
        for (j = 0; j < 2; j++)
        {
            easyChatWord = GetWordFromSelectedGroup(wordIndex++);
            if (easyChatWord != EC_EMPTY_WORD)
            {
                CopyEasyChatWordPadded(sScreenControl->wordSelectPrintBuffer, easyChatWord, 0);
                if (!DummyWordCheck(easyChatWord))
                    PrintEasyChatText(WIN_INPUT_SELECT, FONT_NORMAL, sScreenControl->wordSelectPrintBuffer, (j * 13 + 3) * 8, y, TEXT_SKIP_DRAW, NULL);
                else // Never reached
                    PrintEasyChatTextWithColors(WIN_INPUT_SELECT, FONT_NORMAL, sScreenControl->wordSelectPrintBuffer, (j * 13 + 3) * 8, y, TEXT_SKIP_DRAW, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_RED, TEXT_COLOR_LIGHT_GRAY);
            }
        }

        y += 16;
    }

    CopyWindowToVram(WIN_INPUT_SELECT, COPYWIN_GFX);
}

static void EraseWordSelectRows(u8 scrollOffset, u8 numRows)
{
    int y;
    int var0;
    int var1;
    int var2;

    y = (scrollOffset * 16 + 96) & 0xFF;
    var2 = numRows * 16;
    var0 = y + var2;

    if (var0 > 255)
    {
        var1 = var0 - 256;
        var2 = 256 - y;
    }
    else
    {
        var1 = 0;
    }

    FillWindowPixelRect(WIN_INPUT_SELECT, PIXEL_FILL(1), 0, y, 224, var2);
    if (var1)
        FillWindowPixelRect(WIN_INPUT_SELECT, PIXEL_FILL(1), 0, 0, 224, var1);
}

static void ClearWordSelectWindow(void)
{
    FillWindowPixelBuffer(WIN_INPUT_SELECT, PIXEL_FILL(1));
    CopyWindowToVram(WIN_INPUT_SELECT, COPYWIN_GFX);
}

static void InitLowerWindowAnim(int winAnimType)
{
    switch (winAnimType)
    {
    case WINANIM_OPEN_KEYBOARD:
        sScreenControl->curWindowAnimState = 0;
        sScreenControl->destWindowAnimState = 10;
        break;
    case WINANIM_CLOSE_KEYBOARD:
        sScreenControl->curWindowAnimState = 9;
        sScreenControl->destWindowAnimState = 0;
        break;
    case WINANIM_OPEN_WORD_SELECT:
        sScreenControl->curWindowAnimState = 11;
        sScreenControl->destWindowAnimState = 17;
        break;
    case WINANIM_CLOSE_WORD_SELECT:
        sScreenControl->curWindowAnimState = 17;
        sScreenControl->destWindowAnimState = 0;
        break;
    case WINANIM_RETURN_TO_KEYBOARD:
        sScreenControl->curWindowAnimState = 17;
        sScreenControl->destWindowAnimState = 10;
        break;
    case WINANIM_KEYBOARD_SWITCH_OUT:
        sScreenControl->curWindowAnimState = 18;
        sScreenControl->destWindowAnimState = 22;
        break;
    case WINANIM_KEYBOARD_SWITCH_IN:
        sScreenControl->curWindowAnimState = 22;
        sScreenControl->destWindowAnimState = 18;
        break;
    }

    sScreenControl->windowAnimStateDir = sScreenControl->curWindowAnimState < sScreenControl->destWindowAnimState ? 1 : -1;
}

// Returns FALSE if the anim is finished
static bool8 UpdateLowerWindowAnim(void)
{
    u8 curState, destState;
    if (sScreenControl->curWindowAnimState == sScreenControl->destWindowAnimState)
        return FALSE;

    sScreenControl->curWindowAnimState += sScreenControl->windowAnimStateDir;
    DrawLowerWindowFrame(sScreenControl->curWindowAnimState);
    curState = sScreenControl->curWindowAnimState;
    destState = sScreenControl->destWindowAnimState;
    return (curState ^ destState) > 0;
}

// States in this function are used incrementally with differing start/end cases
// to draw the lower window and create the appearance that it's opening/closing/animating.
// See InitLowerWindowAnim
static void DrawLowerWindowFrame(u8 type)
{
    FillBgTilemapBufferRect_Palette0(1, 0, 0, 10, 30, 10);
    switch (type)
    {
    case 0: // Closed
        break;
    case 1:
        BufferLowerWindowFrame(11, 14, 3, 2);
        break;
    case 2:
        BufferLowerWindowFrame(9, 14, 7, 2);
        break;
    case 3:
        BufferLowerWindowFrame(7, 14, 11, 2);
        break;
    case 4:
        BufferLowerWindowFrame(5, 14, 15, 2);
        break;
    case 5:
        BufferLowerWindowFrame(3, 14, 19, 2);
        break;
    case 6:
        BufferLowerWindowFrame(1, 14, 23, 2);
        break;
    case 7:
        BufferLowerWindowFrame(1, 13, 23, 4);
        break;
    case 8:
        BufferLowerWindowFrame(1, 12, 23, 6);
        break;
    case 9:
        BufferLowerWindowFrame(1, 11, 23, 8);
        break;
    case 10:
        BufferLowerWindowFrame(1, 10, 23, 10);
        break;
    case 11:
        BufferLowerWindowFrame(1, 10, 24, 10);
        break;
    case 12:
        BufferLowerWindowFrame(1, 10, 25, 10);
        break;
    case 13:
        BufferLowerWindowFrame(1, 10, 26, 10);
        break;
    case 14:
        BufferLowerWindowFrame(1, 10, 27, 10);
        break;
    case 15:
        BufferLowerWindowFrame(1, 10, 28, 10);
        break;
    case 16:
        BufferLowerWindowFrame(1, 10, 29, 10);
        break;
    case 17:
        BufferLowerWindowFrame(0, 10, 30, 10);
        break;
    case 18:
        BufferLowerWindowFrame(1, 10, 23, 10);
        break;
    case 19:
        BufferLowerWindowFrame(1, 11, 23, 8);
        break;
    case 20:
        BufferLowerWindowFrame(1, 12, 23, 6);
        break;
    case 21:
        BufferLowerWindowFrame(1, 13, 23, 4);
        break;
    case 22:
        BufferLowerWindowFrame(1, 14, 23, 2);
        break;
    }

    CopyBgTilemapBufferToVram(1);
}

static void BufferLowerWindowFrame(int left, int top, int width, int height)
{
    u16 *tilemap;
    int right;
    int bottom;
    int x, y;

    tilemap = sScreenControl->bg1TilemapBuffer;
    right = left + width - 1;
    bottom = top + height - 1;
    x = left;
    y = top;

    // Draw top edge
    tilemap[y * 32 + x] = FRAME_OFFSET_GREEN + FRAME_TILE_TOP_L_CORNER;
    x++;
    for (; x < right; x++)
        tilemap[y * 32 + x] = FRAME_OFFSET_GREEN + FRAME_TILE_TOP_EDGE;

    tilemap[y * 32 + x] = FRAME_OFFSET_GREEN + FRAME_TILE_TOP_R_CORNER;
    y++;

    // Draw middle section
    for (; y < bottom; y++)
    {
        tilemap[y * 32 + left] = FRAME_OFFSET_GREEN + FRAME_TILE_L_EDGE;
        x = left + 1;
        for (; x < right; x++)
            tilemap[y * 32 + x] = FRAME_OFFSET_GREEN + FRAME_TILE_TRANSPARENT;

        tilemap[y * 32 + x] = FRAME_OFFSET_GREEN + FRAME_TILE_R_EDGE;
    }

    // Draw bottom edge
    tilemap[y * 32 + left] = FRAME_OFFSET_GREEN + FRAME_TILE_BOTTOM_L_CORNER;
    x = left + 1;
    for (; x < right; x++)
        tilemap[y * 32 + x] = FRAME_OFFSET_GREEN + FRAME_TILE_BOTTOM_EDGE;

    tilemap[y * 32 + x] = FRAME_OFFSET_GREEN + FRAME_TILE_BOTTOM_R_CORNER;

    SetWindowDimensions((left + 1) * 8, (top + 1) * 8, (width - 2) * 8, (height - 2) * 8);
}

static void ResetLowerWindowScroll(void)
{
    ChangeBgY(2, 0x800, BG_COORD_SET);
    sScreenControl->scrollOffset = 0;
}

static void InitLowerWindowScroll(s16 scrollChange, u8 speed)
{
    int bgY;
    s16 yChange;

    bgY = GetBgY(2);
    sScreenControl->scrollOffset += scrollChange;
    yChange = scrollChange * 16;
    bgY += yChange * 256;
    if (speed)
    {
        sScreenControl->scrollDest = bgY;
        sScreenControl->scrollSpeed = speed * 256;
        if (yChange < 0)
            sScreenControl->scrollSpeed = -sScreenControl->scrollSpeed;
    }
    else
    {
        ChangeBgY(2, bgY, BG_COORD_SET);
    }
}

static bool8 UpdateLowerWindowScroll(void)
{
    int bgY;

    bgY = GetBgY(2);
    if (bgY == sScreenControl->scrollDest)
    {
        return FALSE;
    }
    else
    {
        ChangeBgY(2, sScreenControl->scrollSpeed, BG_COORD_ADD);
        return TRUE;
    }
}

static int GetLowerWindowScrollOffset(void)
{
    return sScreenControl->scrollOffset;
}

static void SetWindowDimensions(u8 left, u8 top, u8 width, u8 height)
{
    u16 horizontalDimensions = WIN_RANGE(left, left + width);
    u16 verticalDimensions = WIN_RANGE(top, top + height);
    SetGpuReg(REG_OFFSET_WIN0H, horizontalDimensions);
    SetGpuReg(REG_OFFSET_WIN0V, verticalDimensions);
}

static void LoadEasyChatGfx(void)
{
    u32 i;

    LoadSpriteSheets(sSpriteSheets);
    LoadSpritePalettes(sSpritePalettes);
    for (i = 0; i < ARRAY_COUNT(sCompressedSpriteSheets); i++)
        LoadCompressedSpriteSheet(&sCompressedSpriteSheets[i]);
}

#define sDelayTimer    data[0]
#define sAnimateCursor data[1]

static void CreateMainCursorSprite(void)
{
    u8 frameId = GetEasyChatScreenFrameId();
    int x = sPhraseFrameDimensions[frameId].left * 8 + 13;
    int y = sPhraseFrameDimensions[frameId].top * 8 + 8;
    u8 spriteId = CreateSprite(&sSpriteTemplate_TriangleCursor, x, y, 2);
    sScreenControl->mainCursorSprite = &gSprites[spriteId];
    gSprites[spriteId].sAnimateCursor = TRUE;
}

static void SpriteCB_Cursor(struct Sprite *sprite)
{
    if (sprite->sAnimateCursor)
    {
        if (++sprite->sDelayTimer > 2)
        {
            sprite->sDelayTimer = 0;
            if (++sprite->x2 > 0)
                sprite->x2 = -6;
        }
    }
}

static void SetMainCursorPos(u8 x, u8 y)
{
    sScreenControl->mainCursorSprite->x = x;
    sScreenControl->mainCursorSprite->y = y;
    sScreenControl->mainCursorSprite->x2 = 0;
    sScreenControl->mainCursorSprite->sDelayTimer = 0;
}

static void StopMainCursorAnim(void)
{
    sScreenControl->mainCursorSprite->sDelayTimer = 0;
    sScreenControl->mainCursorSprite->sAnimateCursor = FALSE;
    sScreenControl->mainCursorSprite->x2 = 0;
}

static void StartMainCursorAnim(void)
{
    sScreenControl->mainCursorSprite->sAnimateCursor = TRUE;
}

static void CreateRectangleCursorSprites(void)
{
    u8 spriteId = CreateSprite(&sSpriteTemplate_RectangleCursor, 0, 0, 3);
    sScreenControl->rectangleCursorSpriteRight = &gSprites[spriteId];
    sScreenControl->rectangleCursorSpriteRight->x2 = 32;

    spriteId = CreateSprite(&sSpriteTemplate_RectangleCursor, 0, 0, 3);
    sScreenControl->rectangleCursorSpriteLeft = &gSprites[spriteId];
    sScreenControl->rectangleCursorSpriteLeft->x2 = -32;

    sScreenControl->rectangleCursorSpriteRight->hFlip = TRUE;
    UpdateRectangleCursorPos();
}

static void DestroyRectangleCursorSprites(void)
{
    DestroySprite(sScreenControl->rectangleCursorSpriteRight);
    sScreenControl->rectangleCursorSpriteRight = NULL;
    DestroySprite(sScreenControl->rectangleCursorSpriteLeft);
    sScreenControl->rectangleCursorSpriteLeft = NULL;
}

static void UpdateRectangleCursorPos(void)
{
    s8 column;
    s8 row;

    if (sScreenControl->rectangleCursorSpriteRight
     && sScreenControl->rectangleCursorSpriteLeft)
    {
        GetKeyboardCursorColAndRow(&column, &row);
        if (!GetInAlphabetMode())
            SetRectangleCursorPos_GroupMode(column, row);
        else
            SetRectangleCursorPos_AlphabetMode(column, row);
    }
}

static void SetRectangleCursorPos_GroupMode(s8 column, s8 row)
{
    if (column != -1)
    {
        // In group name window
        StartSpriteAnim(sScreenControl->rectangleCursorSpriteRight, RECTCURSOR_ANIM_ON_GROUP);
        sScreenControl->rectangleCursorSpriteRight->x = column * 84 + 58;
        sScreenControl->rectangleCursorSpriteRight->y = row * 16 + 96;

        StartSpriteAnim(sScreenControl->rectangleCursorSpriteLeft, RECTCURSOR_ANIM_ON_GROUP);
        sScreenControl->rectangleCursorSpriteLeft->x = column * 84 + 58;
        sScreenControl->rectangleCursorSpriteLeft->y = row * 16 + 96;
    }
    else
    {
        // In button window
        StartSpriteAnim(sScreenControl->rectangleCursorSpriteRight, RECTCURSOR_ANIM_ON_BUTTON);
        sScreenControl->rectangleCursorSpriteRight->x = 216;
        sScreenControl->rectangleCursorSpriteRight->y = row * 16 + 112;

        StartSpriteAnim(sScreenControl->rectangleCursorSpriteLeft, RECTCURSOR_ANIM_ON_BUTTON);
        sScreenControl->rectangleCursorSpriteLeft->x = 216;
        sScreenControl->rectangleCursorSpriteLeft->y = row * 16 + 112;
    }
}

static void SetRectangleCursorPos_AlphabetMode(s8 column, s8 row)
{
    int anim;
    int x, y;

    if (column != -1)
    {
        y = row * 16 + 96;
        x = 32;
        if (column == NUM_ALPHABET_COLUMNS - 1 && row == 0)
        {
            // Cursor is on 'Others'
            x = 158;
            anim = RECTCURSOR_ANIM_ON_OTHERS;
        }
        else
        {
            // Cursor is on a letter
            x += sAlphabetKeyboardColumnOffsets[(u8)column < NUM_ALPHABET_COLUMNS ? column : 0];
            anim = RECTCURSOR_ANIM_ON_LETTER;
        }

        StartSpriteAnim(sScreenControl->rectangleCursorSpriteRight, anim);
        sScreenControl->rectangleCursorSpriteRight->x = x;
        sScreenControl->rectangleCursorSpriteRight->y = y;

        StartSpriteAnim(sScreenControl->rectangleCursorSpriteLeft, anim);
        sScreenControl->rectangleCursorSpriteLeft->x = x;
        sScreenControl->rectangleCursorSpriteLeft->y = y;
    }
    else
    {
        // In button window
        StartSpriteAnim(sScreenControl->rectangleCursorSpriteRight, RECTCURSOR_ANIM_ON_BUTTON);
        sScreenControl->rectangleCursorSpriteRight->x = 216;
        sScreenControl->rectangleCursorSpriteRight->y = row * 16 + 112;

        StartSpriteAnim(sScreenControl->rectangleCursorSpriteLeft, RECTCURSOR_ANIM_ON_BUTTON);
        sScreenControl->rectangleCursorSpriteLeft->x = 216;
        sScreenControl->rectangleCursorSpriteLeft->y = row * 16 + 112;
    }
}

// Cursor for selecting a new word
// Identical in appearance to the 'main' cursor
static void CreateWordSelectCursorSprite(void)
{
    u8 spriteId = CreateSprite(&sSpriteTemplate_TriangleCursor, 0, 0, 4);
    sScreenControl->wordSelectCursorSprite = &gSprites[spriteId];
    sScreenControl->wordSelectCursorSprite->callback = SpriteCB_WordSelectCursor;
    sScreenControl->wordSelectCursorSprite->oam.priority = 2;
    UpdateWordSelectCursorPos();
}

static void SpriteCB_WordSelectCursor(struct Sprite *sprite)
{
    if (++sprite->sDelayTimer > 2)
    {
        sprite->sDelayTimer = 0;
        if (++sprite->x2 > 0)
            sprite->x2 = -6;
    }
}

static void UpdateWordSelectCursorPos(void)
{
    s8 column, row, x, y;

    GetWordSelectColAndRow(&column, &row);
    x = column * 13;
    x = x * 8 + 28;
    y = row * 16 + 96;
    SetWordSelectCursorPos(x, y);
}

static void SetWordSelectCursorPos(u8 x, u8 y)
{
    if (sScreenControl->wordSelectCursorSprite)
    {
        sScreenControl->wordSelectCursorSprite->x = x;
        sScreenControl->wordSelectCursorSprite->y = y;
        sScreenControl->wordSelectCursorSprite->x2 = 0;
        sScreenControl->wordSelectCursorSprite->sDelayTimer = 0;
    }
}

static void DestroyWordSelectCursorSprite(void)
{
    if (sScreenControl->wordSelectCursorSprite)
    {
        DestroySprite(sScreenControl->wordSelectCursorSprite);
        sScreenControl->wordSelectCursorSprite = NULL;
    }
}

static void CreateSideWindowSprites(void)
{
    u8 spriteId = CreateSprite(&sSpriteTemplate_ButtonWindow, 208, 128, 6);
    sScreenControl->buttonWindowSprite = &gSprites[spriteId];
    sScreenControl->buttonWindowSprite->x2 = -64;

    spriteId = CreateSprite(&sSpriteTemplate_ModeWindow, 208, 80, 5);
    sScreenControl->modeWindowSprite = &gSprites[spriteId];
    sScreenControl->modeWindowState = 0;
}

static bool8 ShowSideWindow(void)
{
    switch (sScreenControl->modeWindowState)
    {
    default:
        return FALSE;
    case 0:
        // Slide button window on
        sScreenControl->buttonWindowSprite->x2 += 8;
        if (sScreenControl->buttonWindowSprite->x2 >= 0)
        {
            sScreenControl->buttonWindowSprite->x2 = 0;

            // Set mode window anim
            if (!GetInAlphabetMode())
                StartSpriteAnim(sScreenControl->modeWindowSprite, MODEWINDOW_ANIM_TO_GROUP);
            else
                StartSpriteAnim(sScreenControl->modeWindowSprite, MODEWINDOW_ANIM_TO_ALPHABET);

            sScreenControl->modeWindowState++;
        }
        break;
    case 1:
        if (sScreenControl->modeWindowSprite->animEnded)
        {
            sScreenControl->modeWindowState = 2;
            return FALSE;
        }
    }

    return TRUE;
}

static void HideModeWindow(void)
{
    sScreenControl->modeWindowState = 0;
    StartSpriteAnim(sScreenControl->modeWindowSprite, MODEWINDOW_ANIM_TO_HIDDEN);
}

static bool8 DestroySideWindowSprites(void)
{
    switch (sScreenControl->modeWindowState)
    {
    default:
        return FALSE;
    case 0:
        if (sScreenControl->modeWindowSprite->animEnded)
            sScreenControl->modeWindowState = 1;
        break;
    case 1:
        sScreenControl->buttonWindowSprite->x2 -= 8;
        if (sScreenControl->buttonWindowSprite->x2 <= -64)
        {
            DestroySprite(sScreenControl->modeWindowSprite);
            DestroySprite(sScreenControl->buttonWindowSprite);
            sScreenControl->modeWindowSprite = NULL;
            sScreenControl->buttonWindowSprite = NULL;
            sScreenControl->modeWindowState++;
            return FALSE;
        }
    }

    return TRUE;
}

static void SetModeWindowToTransition(void)
{
    StartSpriteAnim(sScreenControl->modeWindowSprite, MODEWINDOW_ANIM_TRANSITION);
}

static void UpdateModeWindowAnim(void)
{
    if (!GetInAlphabetMode())
        StartSpriteAnim(sScreenControl->modeWindowSprite, MODEWINDOW_ANIM_TO_GROUP);
    else
        StartSpriteAnim(sScreenControl->modeWindowSprite, MODEWINDOW_ANIM_TO_ALPHABET);
}

static bool8 IsModeWindowAnimActive(void)
{
    return !sScreenControl->modeWindowSprite->animEnded;
}

static void CreateScrollIndicatorSprites(void)
{
    u8 spriteId = CreateSprite(&sSpriteTemplate_ScrollIndicator, 96, 80, 0);
    if (spriteId != MAX_SPRITES)
        sScreenControl->scrollIndicatorUpSprite = &gSprites[spriteId];

    spriteId = CreateSprite(&sSpriteTemplate_ScrollIndicator, 96, 156, 0);
    if (spriteId != MAX_SPRITES)
    {
        sScreenControl->scrollIndicatorDownSprite = &gSprites[spriteId];
        sScreenControl->scrollIndicatorDownSprite->vFlip = TRUE;
    }

    HideScrollIndicators();
}

static void UpdateScrollIndicatorsVisibility(void)
{
    sScreenControl->scrollIndicatorUpSprite->invisible = !CanScrollUp();
    sScreenControl->scrollIndicatorDownSprite->invisible = !CanScrollDown();
}

static void HideScrollIndicators(void)
{
    sScreenControl->scrollIndicatorUpSprite->invisible = TRUE;
    sScreenControl->scrollIndicatorDownSprite->invisible = TRUE;
}

static void SetScrollIndicatorXPos(bool32 inWordSelect)
{
    if (!inWordSelect)
    {
        // Keyboard (only relevant for group mode, can't scroll in alphabet mode)
        sScreenControl->scrollIndicatorUpSprite->x = 96;
        sScreenControl->scrollIndicatorDownSprite->x = 96;
    }
    else
    {
        // Word select
        sScreenControl->scrollIndicatorUpSprite->x = 120;
        sScreenControl->scrollIndicatorDownSprite->x = 120;
    }
}

// The Start/Select buttons are used as page scroll indicators
static void CreateStartSelectButtonSprites(void)
{
    u8 spriteId = CreateSprite(&sSpriteTemplate_StartSelectButton, 220, 84, 1);
    if (spriteId != MAX_SPRITES)
        sScreenControl->startButtonSprite = &gSprites[spriteId];

    spriteId = CreateSprite(&sSpriteTemplate_StartSelectButton, 220, 156, 1);
    if (spriteId != MAX_SPRITES)
    {
        sScreenControl->selectButtonSprite = &gSprites[spriteId];
        StartSpriteAnim(sScreenControl->selectButtonSprite, 1);
    }

    HideStartSelectButtons();
}

static void UpdateStartSelectButtonsVisibility(void)
{
    sScreenControl->startButtonSprite->invisible = !CanScrollUp();
    sScreenControl->selectButtonSprite->invisible = !CanScrollDown();
}

static void HideStartSelectButtons(void)
{
    sScreenControl->startButtonSprite->invisible = TRUE;
    sScreenControl->selectButtonSprite->invisible = TRUE;
}

static void TryAddInterviewObjectEvents(void)
{
    int graphicsId;
    u8 spriteId;

    switch (GetDisplayedPersonType())
    {
    case EASY_CHAT_PERSON_REPORTER_MALE:
        graphicsId = OBJ_EVENT_GFX_REPORTER_M;
        break;
    case EASY_CHAT_PERSON_REPORTER_FEMALE:
        graphicsId = OBJ_EVENT_GFX_REPORTER_F;
        break;
    case EASY_CHAT_PERSON_BOY:
        graphicsId = OBJ_EVENT_GFX_BOY_1;
        break;
    default:
        return;
    }

    if (GetEasyChatScreenFrameId() != FRAMEID_INTERVIEW_SHOW_PERSON)
        return;

    // Add object for reporter/interviewing fan (facing left)
    spriteId = CreateObjectGraphicsSprite(graphicsId, SpriteCallbackDummy, 76, 40, 0);
    if (spriteId != MAX_SPRITES)
    {
        gSprites[spriteId].oam.priority = 0;
        StartSpriteAnim(&gSprites[spriteId], 2);
    }

    // Add object for player (facing right)
    spriteId = CreateObjectGraphicsSprite(
        gSaveBlock2Ptr->playerGender == MALE ? OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL : OBJ_EVENT_GFX_RIVAL_MAY_NORMAL,
        SpriteCallbackDummy,
        52,
        40,
        0);

    if (spriteId != MAX_SPRITES)
    {
        gSprites[spriteId].oam.priority = 0;
        StartSpriteAnim(&gSprites[spriteId], 3);
    }
}

int GetFooterIndex(void)
{
    u8 frameId = GetEasyChatScreenFrameId();
    switch (sPhraseFrameDimensions[frameId].footerId)
    {
    case FOOTER_QUIZ:
        return FOOTER_QUIZ;
    case FOOTER_ANSWER:
        return FOOTER_ANSWER;
    case FOOTER_NORMAL:
        return FOOTER_NORMAL;
    default:
        return NUM_FOOTER_TYPES;
    }
}

static int GetFooterOptionXOffset(int option)
{
    int footerIndex = GetFooterIndex();
    if (footerIndex < NUM_FOOTER_TYPES)
        return sFooterOptionXOffsets[footerIndex][option] + 4;
    else
        return 0;
}

static void AddMainScreenButtonWindow(void)
{
    int i;
    u16 windowId;
    struct WindowTemplate template;
    int footerIndex = GetFooterIndex();
    if (footerIndex == NUM_FOOTER_TYPES)
        return;

    template.bg = 3;
    template.tilemapLeft = 1;
    template.tilemapTop = 11;
    template.width = 28;
    template.height = 2;
    template.paletteNum = 11;
    template.baseBlock = 0x34;
    windowId = AddWindow(&template);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    for (i = 0; i < (int)ARRAY_COUNT(sFooterTextOptions[0]); i++)
    {
        const u8 *str = sFooterTextOptions[footerIndex][i];
        if (str)
        {
            int x = sFooterOptionXOffsets[footerIndex][i];
            PrintEasyChatText(windowId, FONT_NORMAL, str, x, 1, 0, NULL);
        }
    }

    PutWindowTilemap(windowId);
}

static bool8 IsEasyChatGroupUnlocked(u8 groupId)
{
    switch (groupId)
    {
    case EC_GROUP_TRENDY_SAYING:
        return FlagGet(FLAG_UNLOCKED_TRENDY_SAYINGS);
    case EC_GROUP_EVENTS:
    case EC_GROUP_MOVE_1:
    case EC_GROUP_MOVE_2:
        return FlagGet(FLAG_SYS_GAME_CLEAR);
    case EC_GROUP_POKEMON_NATIONAL:
        return EasyChatIsNationalPokedexEnabled();
    default:
        return TRUE;
    }
}

u16 EasyChat_GetNumWordsInGroup(u8 groupId)
{
    if (groupId == EC_GROUP_POKEMON)
        return GetNationalPokedexCount(FLAG_GET_SEEN);

    if (IsEasyChatGroupUnlocked(groupId))
        return gEasyChatGroups[groupId].numEnabledWords;

    return 0;
}

static bool8 IsEasyChatWordInvalid(u16 easyChatWord)
{
    u16 i;
    u8 groupId;
    u32 index;
    u16 numWords;
    const u16 *list;
    if (easyChatWord == EC_EMPTY_WORD)
        return FALSE;

    groupId = EC_GROUP(easyChatWord);
    index = EC_INDEX(easyChatWord);
    if (groupId >= EC_NUM_GROUPS)
        return TRUE;

    numWords = gEasyChatGroups[groupId].numWords;
    switch (groupId)
    {
    case EC_GROUP_POKEMON:
    case EC_GROUP_POKEMON_NATIONAL:
    case EC_GROUP_MOVE_1:
    case EC_GROUP_MOVE_2:
        list = gEasyChatGroups[groupId].wordData.valueList;
        for (i = 0; i < numWords; i++)
        {
            if (index == list[i])
                return FALSE;
        }
        return TRUE;
    }

    if (index >= numWords)
        return TRUE;
    else
        return FALSE;
}

bool8 IsBardWordInvalid(u16 easyChatWord)
{
    int numWordsInGroup;
    u8 groupId = EC_GROUP(easyChatWord);
    u32 index = EC_INDEX(easyChatWord);
    if (groupId >= EC_NUM_GROUPS)
        return TRUE;

    switch (groupId)
    {
    case EC_GROUP_POKEMON:
    case EC_GROUP_POKEMON_NATIONAL:
        numWordsInGroup = gNumBardWords_Species;
        break;
    case EC_GROUP_MOVE_1:
    case EC_GROUP_MOVE_2:
        numWordsInGroup = gNumBardWords_Moves;
        break;
    default:
        numWordsInGroup = gEasyChatGroups[groupId].numWords;
        break;
    }

    if (numWordsInGroup <= index)
        return TRUE;
    else
        return FALSE;
}

static const u8 *GetEasyChatWord(u8 groupId, u16 index)
{
    switch (groupId)
    {
    case EC_GROUP_POKEMON:
    case EC_GROUP_POKEMON_NATIONAL:
        return gSpeciesNames[index];
    case EC_GROUP_MOVE_1:
    case EC_GROUP_MOVE_2:
        return gMoveNames[index];
    default:
        return gEasyChatGroups[groupId].wordData.words[index].text;
    }
}

u8 *CopyEasyChatWord(u8 *dest, u16 easyChatWord)
{
    u8 *resultStr;
    if (IsEasyChatWordInvalid(easyChatWord))
    {
        resultStr = StringCopy(dest, gText_ThreeQuestionMarks);
    }
    else if (easyChatWord != EC_EMPTY_WORD)
    {
        u16 index = EC_INDEX(easyChatWord);
        u8 groupId = EC_GROUP(easyChatWord);
        resultStr = StringCopy(dest, GetEasyChatWord(groupId, index));
    }
    else
    {
        *dest = EOS;
        resultStr = dest;
    }

    return resultStr;
}

u8 *ConvertEasyChatWordsToString(u8 *dest, const u16 *src, u16 columns, u16 rows)
{
    u16 i, j;
    u16 numColumns = columns - 1;

    for (i = 0; i < rows; i++)
    {
        for (j = 0; j < numColumns; j++)
        {
            dest = CopyEasyChatWord(dest, *src);
            if (*src != EC_EMPTY_WORD)
            {
                *dest = CHAR_SPACE;
                dest++;
            }

            src++;
        }

        dest = CopyEasyChatWord(dest, *(src++));
        *dest = CHAR_NEWLINE;
        dest++;
    }

    dest--;
    *dest = EOS;
    return dest;
}

static u8 UNUSED *UnusedConvertEasyChatWordsToString(u8 *dest, const u16 *src, u16 columns, u16 rows)
{
    u16 i, j, k;
    u16 numColumns;
    int notEmpty, lineNumber;

    numColumns = columns;
    lineNumber = 0;
    columns--;
    for (i = 0; i < rows; i++)
    {
        const u16 *str = src;
        notEmpty = FALSE;
        for (j = 0; j < numColumns; j++)
        {
            if (str[j] != EC_EMPTY_WORD)
                notEmpty = TRUE;
        }

        if (!notEmpty)
        {
            src += numColumns;
            continue;
        }

        for (k = 0; k < columns; k++)
        {
            dest = CopyEasyChatWord(dest, *src);
            if (*src != EC_EMPTY_WORD)
            {
                *dest = CHAR_SPACE;
                dest++;
            }

            src++;
        }

        dest = CopyEasyChatWord(dest, *(src++));
        if (lineNumber == 0)
            *dest = CHAR_NEWLINE;
        else
            *dest = CHAR_PROMPT_SCROLL;

        dest++;
        lineNumber++;
    }

    dest--;
    *dest = EOS;
    return dest;
}

static u16 GetEasyChatWordStringLength(u16 easyChatWord)
{
    if (easyChatWord == EC_EMPTY_WORD)
        return 0;

    if (IsEasyChatWordInvalid(easyChatWord))
    {
        return StringLength(gText_ThreeQuestionMarks);
    }
    else
    {
        u16 index = EC_INDEX(easyChatWord);
        u8 groupId = EC_GROUP(easyChatWord);
        return StringLength(GetEasyChatWord(groupId, index));
    }
}

static bool8 CanPhraseFitInXRowsYCols(const u16 *easyChatWords, u8 numRows, u8 numColumns, u16 maxLength)
{
    u8 i, j;

    for (i = 0; i < numColumns; i++)
    {
        u16 totalLength = numRows - 1;
        for (j = 0; j < numRows; j++)
            totalLength += GetEasyChatWordStringLength(*(easyChatWords++));

        if (totalLength > maxLength)
            return TRUE;
    }

    return FALSE;
}

u16 GetRandomEasyChatWordFromGroup(u16 groupId)
{
    u16 index = Random() % gEasyChatGroups[groupId].numWords;
    if (groupId == EC_GROUP_POKEMON
     || groupId == EC_GROUP_POKEMON_NATIONAL
     || groupId == EC_GROUP_MOVE_1
     || groupId == EC_GROUP_MOVE_2)
    {
        index = gEasyChatGroups[groupId].wordData.valueList[index];
    }

    return EC_WORD(groupId, index);
}

u16 GetRandomEasyChatWordFromUnlockedGroup(u16 groupId)
{
    if (!IsEasyChatGroupUnlocked(groupId))
        return EC_EMPTY_WORD;

    if (groupId == EC_GROUP_POKEMON)
        return GetRandomUnlockedEasyChatPokemon();

    return GetRandomEasyChatWordFromGroup(groupId);
}

void ShowEasyChatProfile(void)
{
    u16 *easyChatWords;
    int columns, rows;
    switch (gSpecialVar_0x8004)
    {
    case 0:
        easyChatWords = gSaveBlock1Ptr->easyChatProfile;
        columns = 2;
        rows = 2;
        break;
    case 1:
        easyChatWords = gSaveBlock1Ptr->easyChatBattleStart;
        if (CanPhraseFitInXRowsYCols(gSaveBlock1Ptr->easyChatBattleStart, 3, 2, 18))
        {
            columns = 2;
            rows = 3;
        }
        else
        {
            columns = 3;
            rows = 2;
        }
        break;
    case 2:
        easyChatWords = gSaveBlock1Ptr->easyChatBattleWon;
        columns = 3;
        rows = 2;
        break;
    case 3:
        easyChatWords = gSaveBlock1Ptr->easyChatBattleLost;
        columns = 3;
        rows = 2;
        break;
    default:
        return;
    }

    ConvertEasyChatWordsToString(gStringVarBuffer, easyChatWords, columns, rows);
    ShowFieldAutoScrollMessage(gStringVarBuffer);
}

// The phrase that a man in Dewford Hall suggests has a "deep link" to the current trendy phrase
void BufferDeepLinkPhrase(void)
{
    int groupId = Random() & 1 ? EC_GROUP_HOBBIES : EC_GROUP_LIFESTYLE;
    u16 easyChatWord = GetRandomEasyChatWordFromUnlockedGroup(groupId);
    CopyEasyChatWord(gStringVar2, easyChatWord);
}

/*
    ### Trendy Sayings

    Not to be confused with Dewford Town's "trendy phrase".

    This is a group of easy chat words (EC_GROUP_TRENDY_SAYING) that are normally inaccessible.
    They can be unlocked either through Mystery Event (where they're referred to as "rare" words)
    or from the "Hipster" variety of the Mauville Old Man. The Hipster can unlock one word each
    time he is received via record mixing (and once if he is the player's default Old Man).

    Which words have been unlocked is saved in the unlockedTrendySayings bitfield in SaveBlock1

    Unlocked trendy saying words are only accessible if the flag FLAG_UNLOCKED_TRENDY_SAYINGS is set.
    It's set any time the player talks to the Hipster, but is not apparently set by Mystery Event,
    meaning trendy saying words unlocked via Mystery Event may not be available until the player has
    talked to the Hipster.
*/
static bool8 IsTrendySayingUnlocked(u8 wordIndex)
{
    int byteOffset = wordIndex / 8;
    int shift = wordIndex % 8;
    return (gSaveBlock1Ptr->unlockedTrendySayings[byteOffset] >> shift) & 1;
}

void UnlockTrendySaying(u8 wordIndex)
{
    if (wordIndex < NUM_TRENDY_SAYINGS)
    {
        int byteOffset = wordIndex / 8;
        int shift = wordIndex % 8;
        gSaveBlock1Ptr->unlockedTrendySayings[byteOffset] |= 1 << shift;
    }
}

static u8 GetNumTrendySayingsUnlocked(void)
{
    u8 i;
    u8 numUnlocked;

    for (i = 0, numUnlocked = 0; i < NUM_TRENDY_SAYINGS; i++)
    {
        if (IsTrendySayingUnlocked(i))
            numUnlocked++;
    }

    return numUnlocked;
}

u16 UnlockRandomTrendySaying(void)
{
    u16 i;
    u16 numToSkip;
    u8 numUnlocked = GetNumTrendySayingsUnlocked();
    if (numUnlocked == NUM_TRENDY_SAYINGS)
        return EC_EMPTY_WORD;

    numToSkip = Random() % (NUM_TRENDY_SAYINGS - numUnlocked);
    for (i = 0; i < NUM_TRENDY_SAYINGS; i++)
    {
        if (!IsTrendySayingUnlocked(i))
        {
            if (numToSkip)
            {
                // Skip the first n locked words, as determined by the Random call above.
                numToSkip--;
            }
            else
            {
                UnlockTrendySaying(i);
                return EC_WORD(EC_GROUP_TRENDY_SAYING, i);
            }
        }
    }

    // Would only be reached if there are no new words to teach, which is handled at the start.
    return EC_EMPTY_WORD;
}

static u16 UNUSED GetRandomUnlockedTrendySaying(void)
{
    u16 i;
    u16 n = GetNumTrendySayingsUnlocked();
    if (n == 0)
        return EC_EMPTY_WORD;

    n = Random() % n;
    for (i = 0; i < NUM_TRENDY_SAYINGS; i++)
    {
        if (IsTrendySayingUnlocked(i))
        {
            if (n)
                n--;
            else
                return EC_WORD(EC_GROUP_TRENDY_SAYING, i);
        }
    }

    return EC_EMPTY_WORD;
}

static bool8 EasyChatIsNationalPokedexEnabled(void)
{
    return IsNationalPokedexEnabled();
}

static u16 GetRandomUnlockedEasyChatPokemon(void)
{
    u16 i;
    u16 numWords;
    const u16 *species;
    u16 index = EasyChat_GetNumWordsInGroup(EC_GROUP_POKEMON);
    if (index == 0)
        return EC_EMPTY_WORD;

    index = Random() % index;
    species = gEasyChatGroups[EC_GROUP_POKEMON].wordData.valueList;
    numWords = gEasyChatGroups[EC_GROUP_POKEMON].numWords;
    for (i = 0; i < numWords; i++)
    {
        u16 dexNum = SpeciesToNationalPokedexNum(*species);
        if (GetSetPokedexFlag(dexNum, FLAG_GET_SEEN))
        {
            if (index)
                index--;
            else
                return EC_WORD(EC_GROUP_POKEMON, *species);
        }

        species++;
    }

    return EC_EMPTY_WORD;
}

void InitEasyChatPhrases(void)
{
    u16 i, j;

    for (i = 0; i < ARRAY_COUNT(sDefaultProfileWords); i++)
        gSaveBlock1Ptr->easyChatProfile[i] = sDefaultProfileWords[i];

    for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
        gSaveBlock1Ptr->easyChatBattleStart[i] = sDefaultBattleStartWords[i];

    for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
        gSaveBlock1Ptr->easyChatBattleWon[i] = sDefaultBattleWonWords[i];

    for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
        gSaveBlock1Ptr->easyChatBattleLost[i] = sDefaultBattleLostWords[i];

    for (i = 0; i < MAIL_COUNT; i++)
    {
        for (j = 0; j < MAIL_WORDS_COUNT; j++)
            gSaveBlock1Ptr->mail[i].words[j] = EC_EMPTY_WORD;
    }

#ifndef UBFIX
    // BUG: This is supposed to clear 64 bits, but this loop is clearing 64 bytes.
    // However, this bug has no resulting effect on gameplay because only the
    // Mauville old man data is corrupted, which is initialized directly after
    // this function is called when starting a new game.
    for (i = 0; i < 64; i++)
        gSaveBlock1Ptr->unlockedTrendySayings[i] = 0;
#else
    for (i = 0; i < ARRAY_COUNT(gSaveBlock1Ptr->unlockedTrendySayings); i++)
        gSaveBlock1Ptr->unlockedTrendySayings[i] = 0;
#endif
}

static bool8 InitEasyChatScreenWordData(void)
{
    sWordData = Alloc(sizeof(*sWordData));
    if (!sWordData)
        return FALSE;

    SetUnlockedEasyChatGroups();
    SetUnlockedWordsByAlphabet();
    return TRUE;
}

static void FreeEasyChatScreenWordData(void)
{
    TRY_FREE_AND_SET_NULL(sWordData);
}

static void SetUnlockedEasyChatGroups(void)
{
    int i;

    sWordData->numUnlockedGroups = 0;
    if (GetNationalPokedexCount(FLAG_GET_SEEN))
        sWordData->unlockedGroupIds[sWordData->numUnlockedGroups++] = EC_GROUP_POKEMON;

    // These groups are unlocked automatically
    for (i = EC_GROUP_TRAINER; i <= EC_GROUP_ADJECTIVES; i++)
        sWordData->unlockedGroupIds[sWordData->numUnlockedGroups++] = i;

    if (FlagGet(FLAG_SYS_GAME_CLEAR))
    {
        sWordData->unlockedGroupIds[sWordData->numUnlockedGroups++] = EC_GROUP_EVENTS;
        sWordData->unlockedGroupIds[sWordData->numUnlockedGroups++] = EC_GROUP_MOVE_1;
        sWordData->unlockedGroupIds[sWordData->numUnlockedGroups++] = EC_GROUP_MOVE_2;
    }

    if (FlagGet(FLAG_UNLOCKED_TRENDY_SAYINGS))
        sWordData->unlockedGroupIds[sWordData->numUnlockedGroups++] = EC_GROUP_TRENDY_SAYING;

    if (IsNationalPokedexEnabled())
        sWordData->unlockedGroupIds[sWordData->numUnlockedGroups++] = EC_GROUP_POKEMON_NATIONAL;
}

static u8 GetNumUnlockedEasyChatGroups(void)
{
    return sWordData->numUnlockedGroups;
}

static u8 GetUnlockedEasyChatGroupId(u8 index)
{
    if (index >= sWordData->numUnlockedGroups)
        return EC_NUM_GROUPS;
    else
        return sWordData->unlockedGroupIds[index];
}

static u8 UNUSED *BufferEasyChatWordGroupName(u8 *dest, u8 groupId, u16 totalChars)
{
    u16 i;
    u8 *str = StringCopy(dest, sEasyChatGroupNamePointers[groupId]);
    for (i = str - dest; i < totalChars; i++)
    {
        *str = CHAR_SPACE;
        str++;
    }

    *str = EOS;
    return str;
}

static const u8 *GetEasyChatWordGroupName(u8 groupId)
{
    return sEasyChatGroupNamePointers[groupId];
}

static u8 *CopyEasyChatWordPadded(u8 *dest, u16 easyChatWord, u16 totalChars)
{
    u16 i;
    u8 *str = CopyEasyChatWord(dest, easyChatWord);
    for (i = str - dest; i < totalChars; i++)
    {
        *str = CHAR_SPACE;
        str++;
    }

    *str = EOS;
    return str;
}

static void SetUnlockedWordsByAlphabet(void)
{
    int i, j, k;
    int numWords;
    const u16 *words;
    u16 numToProcess;
    int index;

    for (i = 0; i < EC_NUM_ALPHABET_GROUPS; i++)
    {
        numWords = gEasyChatWordsByLetterPointers[i].numWords;
        words = gEasyChatWordsByLetterPointers[i].words;
        sWordData->numUnlockedAlphabetWords[i] = 0;
        index = 0;
        for (j = 0; j < numWords; j++)
        {
            if (*words == EC_EMPTY_WORD)
            {
                words++;
                numToProcess = *words;
                words++;
                j += 1 + numToProcess;
            }
            else
            {
                numToProcess = 1;
            }

            for (k = 0; k < numToProcess; k++)
            {
                if (IsEasyChatWordUnlocked(words[k]))
                {
                    sWordData->unlockedAlphabetWords[i][index++] = words[k];
                    sWordData->numUnlockedAlphabetWords[i]++;
                    break;
                }
            }

            words += numToProcess;
        }
    }
}

static void SetSelectedWordGroup(bool32 inAlphabetMode, u16 groupId)
{
    if (!inAlphabetMode)
        sWordData->numSelectedGroupWords = SetSelectedWordGroup_GroupMode(groupId);
    else
        sWordData->numSelectedGroupWords = SetSelectedWordGroup_AlphabetMode(groupId);
}

static u16 GetWordFromSelectedGroup(u16 index)
{
    if (index >= sWordData->numSelectedGroupWords)
        return EC_EMPTY_WORD;
    else
        return sWordData->selectedGroupWords[index];
}

static u16 GetNumWordsInSelectedGroup(void)
{
    return sWordData->numSelectedGroupWords;
}

static u16 SetSelectedWordGroup_GroupMode(u16 groupId)
{
    u32 i;
    int totalWords;
    const u16 *list;
    const struct EasyChatWordInfo *wordInfo;
    u16 numWords = gEasyChatGroups[groupId].numWords;

    if (groupId == EC_GROUP_POKEMON || groupId == EC_GROUP_POKEMON_NATIONAL
     || groupId == EC_GROUP_MOVE_1  || groupId == EC_GROUP_MOVE_2)
    {
        list = gEasyChatGroups[groupId].wordData.valueList;
        for (i = 0, totalWords = 0; i < numWords; i++)
        {
            if (IsEasyChatIndexAndGroupUnlocked(list[i], groupId))
                sWordData->selectedGroupWords[totalWords++] = EC_WORD(groupId, list[i]);
        }

        return totalWords;
    }
    else
    {
        wordInfo = gEasyChatGroups[groupId].wordData.words;
        for (i = 0, totalWords = 0; i < numWords; i++)
        {
            u16 alphabeticalOrder = wordInfo[i].alphabeticalOrder;
            if (IsEasyChatIndexAndGroupUnlocked(alphabeticalOrder, groupId))
                sWordData->selectedGroupWords[totalWords++] = EC_WORD(groupId, alphabeticalOrder);
        }

        return totalWords;
    }
}

static u16 SetSelectedWordGroup_AlphabetMode(u16 groupId)
{
    u16 i;
    u16 totalWords;

    for (i = 0, totalWords = 0; i < sWordData->numUnlockedAlphabetWords[groupId]; i++)
        sWordData->selectedGroupWords[totalWords++] = sWordData->unlockedAlphabetWords[groupId][i];

    return totalWords;
}

static bool8 IsEasyChatGroupUnlocked2(u8 groupId)
{
    int i;
    for (i = 0; i < sWordData->numUnlockedGroups; i++)
    {
        if (sWordData->unlockedGroupIds[i] == groupId)
            return TRUE;
    }

    return FALSE;
}

static bool8 IsEasyChatIndexAndGroupUnlocked(u16 wordIndex, u8 groupId)
{
    switch (groupId)
    {
    case EC_GROUP_POKEMON:
        return GetSetPokedexFlag(SpeciesToNationalPokedexNum(wordIndex), FLAG_GET_SEEN);
    case EC_GROUP_POKEMON_NATIONAL:
        if (IsRestrictedWordSpecies(wordIndex))
            GetSetPokedexFlag(SpeciesToNationalPokedexNum(wordIndex), FLAG_GET_SEEN);
        return TRUE;
    case EC_GROUP_MOVE_1:
    case EC_GROUP_MOVE_2:
        return TRUE;
    case EC_GROUP_TRENDY_SAYING:
        return IsTrendySayingUnlocked(wordIndex);
    default:
        return gEasyChatGroups[groupId].wordData.words[wordIndex].enabled;
    }
}

// Pokémon words in EC_GROUP_POKEMON_NATIONAL are always allowed (assuming the group is unlocked)
// unless they are in this group. If they are in this group (just Deoxys), they must also have been seen.
static int IsRestrictedWordSpecies(u16 species)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sRestrictedWordSpecies); i++)
    {
        if (sRestrictedWordSpecies[i] == species)
            return TRUE;
    }

    return FALSE;
}

static u8 IsEasyChatWordUnlocked(u16 easyChatWord)
{
    u8 groupId = EC_GROUP(easyChatWord);
    u32 index = EC_INDEX(easyChatWord);
    if (!IsEasyChatGroupUnlocked2(groupId))
        return FALSE;
    else
        return IsEasyChatIndexAndGroupUnlocked(index, groupId);
}

void InitializeEasyChatWordArray(u16 *words, u16 length)
{
    u16 i;
    for (i = length - 1; i != EC_EMPTY_WORD; i--)
        *(words++) = EC_EMPTY_WORD;
}

void InitQuestionnaireWords(void)
{
    int i;
    u16 *words = GetQuestionnaireWordsPtr();
    for (i = 0; i < NUM_QUESTIONNAIRE_WORDS; i++)
        words[i] = EC_EMPTY_WORD;
}

bool32 IsEasyChatAnswerUnlocked(int easyChatWord)
{
    int groupId = EC_GROUP(easyChatWord);
    int mask = EC_MASK_GROUP;
    int index = EC_INDEX(easyChatWord);
    if (!IsEasyChatGroupUnlocked(groupId & mask))
        return FALSE;
    else
        return IsEasyChatIndexAndGroupUnlocked(index, groupId & mask);
}
