#include "global.h"
#include "main.h"
#include "constants/songs.h"
#include "constants/event_objects.h"
#include "mauville_old_man.h"
#include "event_data.h"
#include "string_util.h"
#include "text.h"
#include "easy_chat.h"
#include "script.h"
#include "random.h"
#include "event_scripts.h"
#include "task.h"
#include "menu.h"
#include "m4a.h"
#include "bard_music.h"
#include "sound.h"
#include "strings.h"
#include "overworld.h"
#include "field_message_box.h"
#include "script_menu.h"
#include "trader.h"
#include "m4a.h"
#include "constants/mauville_old_man.h"

static void InitGiddyTaleList(void);
static void StartBardSong(bool8 useNewSongLyrics);
static void Task_BardSong(u8 taskId);
static void StorytellerSetup(void);
static void Storyteller_ResetFlag(void);

static u8 sSelectedStory;

COMMON_DATA struct BardSong gBardSong = {0};

static EWRAM_DATA u16 sUnusedPitchTableIndex = 0;
static EWRAM_DATA struct MauvilleManStoryteller * sStorytellerPtr = NULL;
static EWRAM_DATA u8 sStorytellerWindowId = 0;

static const u16 sDefaultBardSongLyrics[NUM_BARD_SONG_WORDS] = {
    EC_WORD_SHAKE,
    EC_WORD_IT,
    EC_WORD_DO,
    EC_WORD_THE,
    EC_WORD_DIET,
    EC_WORD_DANCE
};

static const u8 * const sGiddyAdjectives[] = {
    GiddyText_SoPretty,
    GiddyText_SoDarling,
    GiddyText_SoRelaxed,
    GiddyText_SoSunny,
    GiddyText_SoDesirable,
    GiddyText_SoExciting,
    GiddyText_SoAmusing,
    GiddyText_SoMagical
};

// Non-random lines Giddy can say. Not all are strictly
// questions, but most are, and the player will receive
// a Yes/No prompt afterwards regardless.
static const u8 * const sGiddyQuestions[GIDDY_MAX_QUESTIONS] = {
    GiddyText_ISoWantToGoOnAVacation,
    GiddyText_IBoughtCrayonsWith120Colors,
    GiddyText_WouldntItBeNiceIfWeCouldFloat,
    GiddyText_WhenYouWriteOnASandyBeach,
    GiddyText_WhatsTheBottomOfTheSeaLike,
    GiddyText_WhenYouSeeTheSettingSunDoesIt,
    GiddyText_LyingBackInTheGreenGrass,
    GiddyText_SecretBasesAreSoWonderful
};

static void SetupBard(void)
{
    u16 i;
    struct MauvilleManBard *bard = &gSaveBlock1Ptr->oldMan.bard;

    bard->id = MAUVILLE_MAN_BARD;
    bard->hasChangedSong = FALSE;
    bard->language = gGameLanguage;
    for (i = 0; i < NUM_BARD_SONG_WORDS; i++)
        bard->songLyrics[i] = sDefaultBardSongLyrics[i];
}

static void SetupHipster(void)
{
    struct MauvilleManHipster *hipster = &gSaveBlock1Ptr->oldMan.hipster;

    hipster->id = MAUVILLE_MAN_HIPSTER;
    hipster->taughtWord = FALSE;
    hipster->language = gGameLanguage;
}

static void SetupStoryteller(void)
{
    StorytellerSetup();
}

static void SetupGiddy(void)
{
    struct MauvilleManGiddy *giddy = &gSaveBlock1Ptr->oldMan.giddy;

    giddy->id = MAUVILLE_MAN_GIDDY;
    giddy->taleCounter = 0;
    giddy->language = gGameLanguage;
}

static void SetupTrader(void)
{
    TraderSetup();
}

void SetMauvilleOldMan(void)
{
    u16 trainerId = (gSaveBlock2Ptr->playerTrainerId[1] << 8) | gSaveBlock2Ptr->playerTrainerId[0];


    // Determine man based on the last digit of the player's trainer ID.
    switch ((trainerId % 10) / 2)
    {
    case MAUVILLE_MAN_BARD:
        SetupBard();
        break;
    case MAUVILLE_MAN_HIPSTER:
        SetupHipster();
        break;
    case MAUVILLE_MAN_TRADER:
        SetupTrader();
        break;
    case MAUVILLE_MAN_STORYTELLER:
        SetupStoryteller();
        break;
    case MAUVILLE_MAN_GIDDY:
        SetupGiddy();
        break;
    }
    SetMauvilleOldManObjEventGfx();
}

u8 GetCurrentMauvilleOldMan(void)
{
    return gSaveBlock1Ptr->oldMan.common.id;
}

void Script_GetCurrentMauvilleMan(void)
{
    gSpecialVar_Result = GetCurrentMauvilleOldMan();
}

void HasBardSongBeenChanged(void)
{
    gSpecialVar_Result = (&gSaveBlock1Ptr->oldMan.bard)->hasChangedSong;
}

void SaveBardSongLyrics(void)
{
    u16 i;
    struct MauvilleManBard *bard = &gSaveBlock1Ptr->oldMan.bard;

    StringCopy(bard->playerName, gSaveBlock2Ptr->playerName);

    for (i = 0; i < TRAINER_ID_LENGTH; i++)
        bard->playerTrainerId[i] = gSaveBlock2Ptr->playerTrainerId[i];

    for (i = 0; i < NUM_BARD_SONG_WORDS; i++)
        bard->songLyrics[i] = bard->newSongLyrics[i];

    bard->hasChangedSong = TRUE;
}

// Copies lyrics into gStringVarBuffer.
// gSpecialVar_0x8004 is used in these functions to indicate which song should be played.
// If it's set to 0 the Bard's current song should be played, otherwise the new user-provided song should be played.
// Its set in the scripts right before 'PlayBardSong' is called.
static void PrepareSongText(void)
{
    struct MauvilleManBard *bard = &gSaveBlock1Ptr->oldMan.bard;
    u16 * lyrics = !gSpecialVar_0x8004 ? bard->songLyrics : bard->newSongLyrics;
    u8 * wordEnd = gStringVarBuffer;
    u8 * str = wordEnd;
    u16 paragraphNum;

    // Easy chat "words" aren't strictly single words, e.g. EC_WORD_MATCH_UP is the string "MATCH UP".
    // The bard song needs to know when it's at the end of an easy chat word and not just at a space in
    // the middle of one, so the loop below will replace spaces in each easy chat word with CHAR_BARD_WORD_DELIMIT.
    // When it comes time to print the song's text all the CHAR_BARD_WORD_DELIMIT will get replaced with CHAR_SPACE.
    //
    // The song text will be displayed in two paragraphs, each containing 3 easy chat words (2 on the first line and 1 on the second).
    for (paragraphNum = 0; paragraphNum < 2; paragraphNum++)
    {
        // Line 1, 1st word
        wordEnd = CopyEasyChatWord(wordEnd, *(lyrics++));
        while (wordEnd != str)
        {
            if (*str == CHAR_SPACE)
                *str = CHAR_BARD_WORD_DELIMIT;
            str++;
        }

        str++;
        *(wordEnd++) = CHAR_SPACE;

        // Line 1, 2nd word
        wordEnd = CopyEasyChatWord(wordEnd, *(lyrics++));
        while (wordEnd != str)
        {
            if (*str == CHAR_SPACE)
                *str = CHAR_BARD_WORD_DELIMIT;
            str++;
        }

        str++;
        *(wordEnd++) = CHAR_NEWLINE;

        // Line 2, 1st word
        wordEnd = CopyEasyChatWord(wordEnd, *(lyrics++));
        while (wordEnd != str)
        {
            if (*str == CHAR_SPACE)
                *str = CHAR_BARD_WORD_DELIMIT;
            str++;
        }

        if (paragraphNum == 0)
        {
            // Erase the 1st paragraph for displaying the 2nd.
            // The == 0 check assumes there are only 2 paragraphs.
            *(wordEnd++) = EXT_CTRL_CODE_BEGIN;
            *(wordEnd++) = EXT_CTRL_CODE_FILL_WINDOW;
        }
    }
}

void PlayBardSong(void)
{
    StartBardSong(gSpecialVar_0x8004);
    ScriptContext_Stop();
}

void HasHipsterTaughtWord(void)
{
    gSpecialVar_Result = (&gSaveBlock1Ptr->oldMan.hipster)->taughtWord;
}

void SetHipsterTaughtWord(void)
{
    (&gSaveBlock1Ptr->oldMan.hipster)->taughtWord = TRUE;
}

void HipsterTryTeachWord(void)
{
    u16 word = UnlockRandomTrendySaying();

    if (word == EC_EMPTY_WORD)
    {
        // All words already unlocked
        gSpecialVar_Result = FALSE;
    }
    else
    {
        CopyEasyChatWord(gStringVar1, word);
        gSpecialVar_Result = TRUE;
    }
}

void GiddyShouldTellAnotherTale(void)
{
    struct MauvilleManGiddy *giddy = &gSaveBlock1Ptr->oldMan.giddy;

    if (giddy->taleCounter == GIDDY_MAX_TALES)
    {
        gSpecialVar_Result = FALSE;
        giddy->taleCounter = 0;
    }
    else
    {
        gSpecialVar_Result = TRUE;
    }
}

void GenerateGiddyLine(void)
{
    struct MauvilleManGiddy *giddy = &gSaveBlock1Ptr->oldMan.giddy;

    if (giddy->taleCounter == 0)
        InitGiddyTaleList();

    // A line from Giddy is either a line following this format:
    // "{random word} is so {adjective}! Don't you agree?",
    // or one of the texts in sGiddyQuestions.
    if (giddy->randomWords[giddy->taleCounter] != EC_EMPTY_WORD)
    {
        u8 *stringPtr;
        u32 adjective = Random();
        adjective %= ARRAY_COUNT(sGiddyAdjectives);

        stringPtr = CopyEasyChatWord(gStringVarBuffer, giddy->randomWords[giddy->taleCounter]);
        stringPtr = StringCopy(stringPtr, GiddyText_Is);
        stringPtr = StringCopy(stringPtr, sGiddyAdjectives[adjective]);
        StringCopy(stringPtr, GiddyText_DontYouAgree);
    }
    else
    {
        StringCopy(gStringVarBuffer, sGiddyQuestions[giddy->questionList[giddy->questionNum++]]);
    }

    // 10% chance for Giddy to stop telling tales.
    if (!(Random() % 10))
        giddy->taleCounter = GIDDY_MAX_TALES;
    else
        giddy->taleCounter++;

    gSpecialVar_Result = TRUE;
}

static void InitGiddyTaleList(void)
{
    struct MauvilleManGiddy *giddy = &gSaveBlock1Ptr->oldMan.giddy;
    u16 wordGroupsAndCount[][2] = {
        {EC_GROUP_POKEMON,   0},
        {EC_GROUP_LIFESTYLE, 0},
        {EC_GROUP_HOBBIES,   0},
        {EC_GROUP_MOVE_1,    0},
        {EC_GROUP_MOVE_2,    0},
        {EC_GROUP_POKEMON_NATIONAL, 0}
    };
    u16 i;
    u16 totalWords;
    u16 temp;
    u16 var; // re-used

    // Shuffle question list
    for (i = 0; i < GIDDY_MAX_QUESTIONS; i++)
        giddy->questionList[i] = i;
    for (i = 0; i < GIDDY_MAX_QUESTIONS; i++)
    {
        var = Random() % (i + 1);
        SWAP(giddy->questionList[i], giddy->questionList[var], temp);
    }

    // Count total number of words in above word groups
    totalWords = 0;
    for (i = 0; i < ARRAY_COUNT(wordGroupsAndCount); i++)
    {
        wordGroupsAndCount[i][1] = EasyChat_GetNumWordsInGroup(wordGroupsAndCount[i][0]);
        totalWords += wordGroupsAndCount[i][1];
    }

    giddy->questionNum = 0;
    temp = 0;
    for (i = 0; i < GIDDY_MAX_TALES; i++)
    {
        var = Random() % 10;
        if (var < 3 && temp < GIDDY_MAX_QUESTIONS)
        {
            // 30% chance for word to be empty (in which case Giddy
            // will say one of his non-random questions), unless
            // the limit for questions has been reached already.
            giddy->randomWords[i] = EC_EMPTY_WORD;
            temp++;
        }
        else
        {
            // Pick a random word id, then advance through the word
            // groups until the group where that id landed.
            s16 randWord = Random() % totalWords;
            for (var = 0; i < ARRAY_COUNT(wordGroupsAndCount); var++)
                if ((randWord -= wordGroupsAndCount[var][1]) <= 0)
                    break;
            if (var == ARRAY_COUNT(wordGroupsAndCount))
                var = 0;

            // Save the randomly selected word
            giddy->randomWords[i] = GetRandomEasyChatWordFromUnlockedGroup(wordGroupsAndCount[var][0]);
        }
    }
}
static void ResetBardFlag(void)
{
    (&gSaveBlock1Ptr->oldMan.bard)->hasChangedSong = FALSE;
}

static void ResetHipsterFlag(void)
{
    (&gSaveBlock1Ptr->oldMan.hipster)->taughtWord = FALSE;
}

static void ResetTraderFlag(void)
{
    Trader_ResetFlag();
}

static void ResetStorytellerFlag(void)
{
    Storyteller_ResetFlag();
}

void ResetMauvilleOldManFlag(void)
{
    switch (GetCurrentMauvilleOldMan())
    {
    case MAUVILLE_MAN_BARD:
        ResetBardFlag();
        break;
    case MAUVILLE_MAN_HIPSTER:
        ResetHipsterFlag();
        break;
    case MAUVILLE_MAN_STORYTELLER:
        ResetStorytellerFlag();
        break;
    case MAUVILLE_MAN_TRADER:
        ResetTraderFlag();
        break;
    case MAUVILLE_MAN_GIDDY:
        break;
    }
    SetMauvilleOldManObjEventGfx();
}

// States and task data for Task_BardSong.
// The function BardSing receives this task as an
// argument and reads its state as well.
enum {
    BARD_STATE_INIT,
    BARD_STATE_WAIT_BGM,
    BARD_STATE_GET_WORD,
    BARD_STATE_HANDLE_WORD,
    BARD_STATE_WAIT_WORD,
    BARD_STATE_PAUSE,
};

#define tState              data[0]
#define tWordState          data[1]
#define tDelay              data[2]
#define tCharIndex          data[3]
#define tLyricsIndex        data[4]
#define tUseNewSongLyrics   data[5]

// Takes a 16-bit easy chat word value and returns a value 0-4 (i.e. a value less than NUM_BARD_PITCH_TABLES_PER_SIZE).
// The relationship between the easy chat word and the chosen pitch table is essentially arbitrary.
// This value will be used twice; once for an unused variable, and again to select a pitch table in CalcWordSounds.
#define WORD_TO_PITCH_TABLE_INDEX(a) ( MOD(a, (NUM_BARD_PITCH_TABLES_PER_SIZE-1)) + (((a) >> 3) & 1) )

static void StartBardSong(bool8 useNewSongLyrics)
{
    u8 taskId = CreateTask(Task_BardSong, 80);

    gTasks[taskId].tUseNewSongLyrics = useNewSongLyrics;
}

static void EnableTextPrinters(void)
{
    gDisableTextPrinters = FALSE;
}

static void DisableTextPrinters(struct TextPrinterTemplate * printer, u16 renderCmd)
{
    gDisableTextPrinters = TRUE;
}

static void DrawSongTextWindow(const u8 * str)
{
    DrawDialogueFrame(0, FALSE);
    AddTextPrinterParameterized(0, FONT_NORMAL, str, 0, 1, 1, DisableTextPrinters);
    gDisableTextPrinters = TRUE;
    CopyWindowToVram(0, COPYWIN_FULL);
}

#define BARD_SONG_BASE_VOLUME 0x100
#define BARD_SONG_BASE_PITCH  0x200

enum {
    SOUND_STATE_START,
    SOUND_STATE_PLAY,
    SOUND_STATE_SET_BASE,
    SOUND_STATE_END,
    SOUND_STATE_WAIT,
};

// Sing one frame of the bard's song. 'task' is a pointer to Task_BardSong, which handles changing the states in here.
static void BardSing(struct Task *task, struct BardSong *song)
{
    switch (task->tState)
    {
    case BARD_STATE_INIT:
    {
        struct MauvilleManBard *bard = &gSaveBlock1Ptr->oldMan.bard;
        u16 *lyrics;
        s32 i;

        // Copy lyrics
        if (!gSpecialVar_0x8004)
            lyrics = bard->songLyrics;
        else
            lyrics = bard->newSongLyrics;

        for (i = 0; i < NUM_BARD_SONG_WORDS; i++)
            song->lyrics[i] = lyrics[i];

        song->lyricsIndex = 0;
        break;
    }
    case BARD_STATE_GET_WORD:
    {
        u16 easyChatWord = song->lyrics[song->lyricsIndex];
        song->soundTemplates = GetWordSoundTemplates(easyChatWord);
        CalcWordSounds(song, WORD_TO_PITCH_TABLE_INDEX(easyChatWord));
        song->lyricsIndex++;
        if (song->soundTemplates[0].songId != PHONEME_ID_NONE)
        {
            // Word has valid sounds, begin playing.
            song->state = SOUND_STATE_START;
        }
        else
        {
            // Word has no valid sounds, skip to the end.
            song->state = SOUND_STATE_END;
            song->timer = 2;
        }
        break;
    }
    case BARD_STATE_HANDLE_WORD:
    case BARD_STATE_WAIT_WORD:
    {
        const struct BardSoundTemplate *template = &song->soundTemplates[song->soundIndex];

        switch (song->state)
        {
        case SOUND_STATE_START:
            song->timer = song->sounds[song->soundIndex].length;
            if (template->songId < NUM_PHONEME_SONGS)
            {
                // Phoneme "songs" come in triplets of PH_*_BLEND, PH_*_HELD, and PH_*_SOLO.
                // The division then multiplication by 3 below is rounding any value from one of these triplets to a PH_*_HELD.
                // This means the actual song files for any phoneme other than PH_*_HELD won't be played here, and the only difference
                // when specifying a PH_*_BLEND or PH_*_SOLO in the songId will be the length of the sound, determined by 'sPhonemeLengths'.
                u8 phonemeTripletId = template->songId / 3;
                m4aSongNumStart((FIRST_PHONEME_SONG + 1) + phonemeTripletId * 3);
            }
            song->state = SOUND_STATE_SET_BASE;
            song->timer--;
            break;
        case SOUND_STATE_SET_BASE:
            song->state = SOUND_STATE_PLAY;
            if (template->songId < NUM_PHONEME_SONGS)
            {
                // Adjust the song volume for the current phoneme.
                // In practice no phonemes use this, so volume here will always be BARD_SONG_BASE_VOLUME.
                song->volume = BARD_SONG_BASE_VOLUME + template->volume * 16;
                m4aMPlayVolumeControl(&gMPlayInfo_SE2, TRACKS_ALL, song->volume);

                // Adjust the song pitch for the current phoneme.
                song->pitch = BARD_SONG_BASE_PITCH + song->sounds[song->soundIndex].pitch;
                m4aMPlayPitchControl(&gMPlayInfo_SE2, TRACKS_ALL, song->pitch);
            }
            break;
        case SOUND_STATE_PLAY:
            // Modulate the volume and pitch to make it sound a little more like singing.
            if (song->voiceInflection > 10)
                song->volume -= 2;
            if (song->voiceInflection & 1)
                song->pitch += 64;
            else
                song->pitch -= 64;
            m4aMPlayVolumeControl(&gMPlayInfo_SE2, TRACKS_ALL, song->volume);
            m4aMPlayPitchControl(&gMPlayInfo_SE2, TRACKS_ALL, song->pitch);
            song->voiceInflection++;

            song->timer--;
            if (song->timer == 0)
            {
                if (++song->soundIndex != MAX_BARD_SOUNDS_PER_WORD && song->soundTemplates[song->soundIndex].songId != PHONEME_ID_NONE)
                {
                    // There are more sounds to play for this word, return to the start.
                    song->state = SOUND_STATE_START;
                }
                else
                {
                    // We've reached the final sound for this word, stop playing.
                    song->state = SOUND_STATE_END;
                    song->timer = 2;
                }
            }
            break;
        case SOUND_STATE_END:
            // Delay, then stop playing the phoneme.
            if (--song->timer == 0)
            {
                m4aMPlayStop(&gMPlayInfo_SE2);
                song->state = SOUND_STATE_WAIT; // We'll remain stuck at this sound state until Task_BardSong changes states from HANDLE_WORD/WAIT_WORD
            }
            break;
        }
        break;
    }
    case BARD_STATE_PAUSE:
    case BARD_STATE_WAIT_BGM:
        // Non-singing states.
        break;
    }
}

static void Task_BardSong(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    BardSing(task, &gBardSong);

    switch (task->tState)
    {
    case BARD_STATE_INIT:
        PrepareSongText();
        DrawSongTextWindow(gStringVarBuffer);
        task->tWordState = 0;
        task->tDelay = 0;
        task->tCharIndex = 0;
        task->tLyricsIndex = 0;
        FadeOutBGMTemporarily(4);
        task->tState = BARD_STATE_WAIT_BGM;
        break;
    case BARD_STATE_WAIT_BGM:
        if (IsBGMPausedOrStopped())
            task->tState = BARD_STATE_GET_WORD;
        break;
    case BARD_STATE_GET_WORD:
    {
        struct MauvilleManBard *bard = &gSaveBlock1Ptr->oldMan.bard;
        u8 *str = &gStringVarBuffer[task->tCharIndex];
        u16 wordLen = 0;

        // Read letters until delimiter
        while (*str != CHAR_SPACE
            && *str != CHAR_NEWLINE
            && *str != EXT_CTRL_CODE_BEGIN
            && *str != EOS)
        {
            str++;
            wordLen++;
        }

        // sUnusedPitchTableIndex is never read. For debugging perhaps, or one of the other languages.
        if (!task->tUseNewSongLyrics)
            sUnusedPitchTableIndex = WORD_TO_PITCH_TABLE_INDEX(bard->songLyrics[task->tLyricsIndex]);
        else
            sUnusedPitchTableIndex = WORD_TO_PITCH_TABLE_INDEX(bard->newSongLyrics[task->tLyricsIndex]);

        gBardSong.length /= wordLen;
        if (gBardSong.length <= 0)
            gBardSong.length = 1;

        task->tLyricsIndex++;

        if (task->tDelay == 0)
        {
            task->tState = BARD_STATE_HANDLE_WORD;
            task->tWordState = 0;
        }
        else
        {
            task->tState = BARD_STATE_PAUSE;
            task->tWordState = 0;
        }
    }
        break;
    case BARD_STATE_PAUSE:
        // Wait before singing next word
        if (task->tDelay == 0)
            task->tState = BARD_STATE_HANDLE_WORD;
        else
            task->tDelay--;
        break;
    case BARD_STATE_HANDLE_WORD:
        if (gStringVarBuffer[task->tCharIndex] == EOS)
        {
            // End song
            FadeInBGM(6);
            m4aMPlayFadeOutTemporarily(&gMPlayInfo_SE2, 2);
            ScriptContext_Enable();
            DestroyTask(taskId);
        }
        else if (gStringVarBuffer[task->tCharIndex] == CHAR_SPACE)
        {
            // End of easy chat word, move on to the next one.
            EnableTextPrinters();
            task->tCharIndex++;
            task->tState = BARD_STATE_GET_WORD;
            task->tDelay = 0;
        }
        else if (gStringVarBuffer[task->tCharIndex] == CHAR_NEWLINE)
        {
            // Handle newline
            task->tCharIndex++;
            task->tState = BARD_STATE_GET_WORD;
            task->tDelay = 0;
        }
        else if (gStringVarBuffer[task->tCharIndex] == EXT_CTRL_CODE_BEGIN)
        {
            // Handle ctrl code
            // The only expected ctrl codes are those for clearing the end of the paragraph,
            // so this assumes there's a new word coming and does a short delay before the next paragraph.
            task->tCharIndex += 2;  // skip over control codes
            task->tState = BARD_STATE_GET_WORD;
            task->tDelay = 8;
        }
        else if (gStringVarBuffer[task->tCharIndex] == CHAR_BARD_WORD_DELIMIT)
        {
            // Space within the current easy chat word (see PrepareSongText), just replace it with a real space.
            gStringVarBuffer[task->tCharIndex] = CHAR_SPACE;
            EnableTextPrinters();
            task->tCharIndex++;
            task->tDelay = 0;
        }
        else
        {
            // Handle regular word
            switch (task->tWordState)
            {
            case 0:
                EnableTextPrinters();
                task->tWordState++;
                break;
            case 1:
                task->tWordState++;
                break;
            case 2:
                task->tCharIndex++;
                task->tWordState = 0;
                task->tDelay = gBardSong.length;
                task->tState = BARD_STATE_WAIT_WORD;
                break;
            }
        }
        break;
    case BARD_STATE_WAIT_WORD:
        // Wait for word to finish being sung.
        // BardSing will continue to play it.
        task->tDelay--;
        if (task->tDelay == 0)
            task->tState = BARD_STATE_HANDLE_WORD;
        break;
    }
    RunTextPrintersAndIsPrinter0Active();
}

void SetMauvilleOldManObjEventGfx(void)
{
    VarSet(VAR_OBJ_GFX_ID_0, OBJ_EVENT_GFX_BARD);
}

// Language fixers?

void SanitizeMauvilleOldManForRuby(union OldMan * oldMan)
{
    s32 i;
    u8 playerName[PLAYER_NAME_LENGTH + 1];

    switch (oldMan->common.id)
    {
    case MAUVILLE_MAN_TRADER:
    {
        struct MauvilleOldManTrader * trader = &oldMan->trader;
        for (i = 0; i < NUM_TRADER_ITEMS; i++)
        {
            if (trader->language[i] == LANGUAGE_JAPANESE)
                ConvertInternationalString(trader->playerNames[i], LANGUAGE_JAPANESE);
        }
        break;
    }
    case MAUVILLE_MAN_STORYTELLER:
    {
        struct MauvilleManStoryteller * storyteller = &oldMan->storyteller;
        for (i = 0; i < NUM_STORYTELLER_TALES; i++)
        {
            if (storyteller->gameStatIDs[i] != 0)
            {
                memcpy(playerName, storyteller->trainerNames[i], PLAYER_NAME_LENGTH);
                playerName[PLAYER_NAME_LENGTH] = EOS;
                if (IsStringJapanese(playerName))
                {
                    memset(playerName, CHAR_SPACE, PLAYER_NAME_LENGTH + 1);
                    StringCopy(playerName, gText_Friend);
                    memcpy(storyteller->trainerNames[i], playerName, PLAYER_NAME_LENGTH);
                    storyteller->language[i] = GAME_LANGUAGE;
                }
            }
        }
        break;
    }
    }
}

static void UNUSED SetMauvilleOldManLanguage(union OldMan * oldMan, u32 language1, u32 language2, u32 language3)
{
    s32 i;

    switch (oldMan->common.id)
    {
    case MAUVILLE_MAN_TRADER:
    {
        struct MauvilleOldManTrader * trader = &oldMan->trader;

        for (i = 0; i < NUM_TRADER_ITEMS; i++)
        {
            if (IsStringJapanese(trader->playerNames[i]))
                trader->language[i] = language1;
            else
                trader->language[i] = language2;
        }
    }
    break;
    case MAUVILLE_MAN_STORYTELLER:
    {
        struct MauvilleManStoryteller * storyteller = &oldMan->storyteller;

        for (i = 0; i < NUM_STORYTELLER_TALES; i++)
        {
            if (IsStringJapanese(storyteller->trainerNames[i]))
                storyteller->language[i] = language1;
            else
                storyteller->language[i] = language2;
        }
    }
    break;
    case MAUVILLE_MAN_BARD:
    {
        struct MauvilleManBard * bard = &oldMan->bard;

        if (language3 == LANGUAGE_JAPANESE)
            bard->language = language1;
        else
            bard->language = language2;
    }
    break;
    case MAUVILLE_MAN_HIPSTER:
    {
        struct MauvilleManHipster * hipster = &oldMan->hipster;

        if (language3 == LANGUAGE_JAPANESE)
            hipster->language = language1;
        else
            hipster->language = language2;
    }
    break;
    case MAUVILLE_MAN_GIDDY:
    {
        struct MauvilleManGiddy * giddy = &oldMan->giddy;

        if (language3 == LANGUAGE_JAPANESE)
            giddy->language = language1;
        else
            giddy->language = language2;
    }
    break;
    }
}

void SanitizeReceivedEmeraldOldMan(union OldMan * oldMan, u32 version, u32 language)
{
    u8 playerName[PLAYER_NAME_LENGTH + 1];
    s32 i;
    if (oldMan->common.id == MAUVILLE_MAN_STORYTELLER && language == LANGUAGE_JAPANESE)
    {
        struct MauvilleManStoryteller * storyteller = &oldMan->storyteller;

        for (i = 0; i < NUM_STORYTELLER_TALES; i++)
        {
            if (storyteller->gameStatIDs[i] != 0)
            {
                memcpy(playerName, storyteller->trainerNames[i], PLAYER_NAME_LENGTH);
                playerName[PLAYER_NAME_LENGTH] = EOS;
                if (IsStringJapanese(playerName))
                    storyteller->language[i] = LANGUAGE_JAPANESE;
                else
                    storyteller->language[i] = GAME_LANGUAGE;
            }
        }
    }
}

void SanitizeReceivedRubyOldMan(union OldMan * oldMan, u32 version, u32 language)
{
    bool32 isRuby = (version == VERSION_SAPPHIRE || version == VERSION_RUBY);

    switch (oldMan->common.id)
    {
    case MAUVILLE_MAN_TRADER:
    {
        struct MauvilleOldManTrader * trader = &oldMan->trader;
        s32 i;

        if (isRuby)
        {
            for (i = 0; i < NUM_TRADER_ITEMS; i++)
            {
                u8 * str = trader->playerNames[i];
                if (str[0] == EXT_CTRL_CODE_BEGIN && str[1] == EXT_CTRL_CODE_JPN)
                {
                    StripExtCtrlCodes(str);
                    trader->language[i] = LANGUAGE_JAPANESE;
                }
                else
                {
                    trader->language[i] = language;
                }
            }
        }
        else
        {
            for (i = 0; i < NUM_TRADER_ITEMS; i++)
            {
                if (trader->language[i] == LANGUAGE_JAPANESE)
                {
                    StripExtCtrlCodes(trader->playerNames[i]);
                }
            }
        }
    }
    break;
    case MAUVILLE_MAN_STORYTELLER:
    {

        struct MauvilleManStoryteller * storyteller = &oldMan->storyteller;
        s32 i;

        if (isRuby)
        {
            for (i = 0; i < NUM_STORYTELLER_TALES; i++)
            {
                if (storyteller->gameStatIDs[i] != 0)
                    storyteller->language[i] = language;
            }
        }
    }
    break;
    case MAUVILLE_MAN_BARD:
    {
        struct MauvilleManBard * bard = &oldMan->bard;

        if (isRuby)
        {
            bard->language = language;
        }
    }
    break;
    case MAUVILLE_MAN_HIPSTER:
    {
        struct MauvilleManHipster * hipster = &oldMan->hipster;

        if (isRuby)
        {
            hipster->language = language;
        }
    }
    break;
    case MAUVILLE_MAN_GIDDY:
    {
        struct MauvilleManGiddy * giddy = &oldMan->giddy;

        if (isRuby)
        {
            giddy->language = language;
        }
    }
    break;
    }
}

struct Story
{
    u8 stat;
    u8 minVal;
    const u8 *title;
    const u8 *action;
    const u8 *fullText;
};

static const struct Story sStorytellerStories[] = {
    // The 50 below is replaced with GAME_STAT_SAVED_GAME
    {
        50, 1,
        MauvilleCity_PokemonCenter_1F_Text_SavedGameTitle,
        MauvilleCity_PokemonCenter_1F_Text_SavedGameAction,
        MauvilleCity_PokemonCenter_1F_Text_SavedGameStory
    },
    {
        GAME_STAT_STARTED_TRENDS, 1,
        MauvilleCity_PokemonCenter_1F_Text_TrendsStartedTitle,
        MauvilleCity_PokemonCenter_1F_Text_TrendsStartedAction,
        MauvilleCity_PokemonCenter_1F_Text_TrendsStartedStory
    },
    {
        GAME_STAT_PLANTED_BERRIES, 1,
        MauvilleCity_PokemonCenter_1F_Text_BerriesPlantedTitle,
        MauvilleCity_PokemonCenter_1F_Text_BerriesPlantedAction,
        MauvilleCity_PokemonCenter_1F_Text_BerriesPlantedStory
    },
    {
        GAME_STAT_TRADED_BIKES, 1,
        MauvilleCity_PokemonCenter_1F_Text_BikeTradesTitle,
        MauvilleCity_PokemonCenter_1F_Text_BikeTradesAction,
        MauvilleCity_PokemonCenter_1F_Text_BikeTradesStory
    },
    {
        GAME_STAT_GOT_INTERVIEWED, 1,
        MauvilleCity_PokemonCenter_1F_Text_InterviewsTitle,
        MauvilleCity_PokemonCenter_1F_Text_InterviewsAction,
        MauvilleCity_PokemonCenter_1F_Text_InterviewsStory
    },
    {
        GAME_STAT_TRAINER_BATTLES, 1,
        MauvilleCity_PokemonCenter_1F_Text_TrainerBattlesTitle,
        MauvilleCity_PokemonCenter_1F_Text_TrainerBattlesAction,
        MauvilleCity_PokemonCenter_1F_Text_TrainerBattlesStory
    },
    {
        GAME_STAT_POKEMON_CAPTURES, 1,
        MauvilleCity_PokemonCenter_1F_Text_PokemonCaughtTitle,
        MauvilleCity_PokemonCenter_1F_Text_PokemonCaughtAction,
        MauvilleCity_PokemonCenter_1F_Text_PokemonCaughtStory
    },
    {
        GAME_STAT_FISHING_ENCOUNTERS, 1,
        MauvilleCity_PokemonCenter_1F_Text_FishingPokemonCaughtTitle,
        MauvilleCity_PokemonCenter_1F_Text_FishingPokemonCaughtAction,
        MauvilleCity_PokemonCenter_1F_Text_FishingPokemonCaughtStory
    },
    {
        GAME_STAT_HATCHED_EGGS, 1,
        MauvilleCity_PokemonCenter_1F_Text_EggsHatchedTitle,
        MauvilleCity_PokemonCenter_1F_Text_EggsHatchedAction,
        MauvilleCity_PokemonCenter_1F_Text_EggsHatchedStory
    },
    {
        GAME_STAT_EVOLVED_POKEMON, 1,
        MauvilleCity_PokemonCenter_1F_Text_PokemonEvolvedTitle,
        MauvilleCity_PokemonCenter_1F_Text_PokemonEvolvedAction,
        MauvilleCity_PokemonCenter_1F_Text_PokemonEvolvedStory
    },
    {
        GAME_STAT_USED_POKECENTER, 1,
        MauvilleCity_PokemonCenter_1F_Text_UsedPokemonCenterTitle,
        MauvilleCity_PokemonCenter_1F_Text_UsedPokemonCenterAction,
        MauvilleCity_PokemonCenter_1F_Text_UsedPokemonCenterStory
    },
    {
        GAME_STAT_RESTED_AT_HOME, 1,
        MauvilleCity_PokemonCenter_1F_Text_RestedAtHomeTitle,
        MauvilleCity_PokemonCenter_1F_Text_RestedAtHomeAction,
        MauvilleCity_PokemonCenter_1F_Text_RestedAtHomeStory
    },
    {
        GAME_STAT_ENTERED_SAFARI_ZONE, 1,
        MauvilleCity_PokemonCenter_1F_Text_SafariGamesTitle,
        MauvilleCity_PokemonCenter_1F_Text_SafariGamesAction,
        MauvilleCity_PokemonCenter_1F_Text_SafariGamesStory
    },
    {
        GAME_STAT_USED_CUT, 1,
        MauvilleCity_PokemonCenter_1F_Text_UsedCutTitle,
        MauvilleCity_PokemonCenter_1F_Text_UsedCutAction,
        MauvilleCity_PokemonCenter_1F_Text_UsedCutStory
    },
    {
        GAME_STAT_USED_ROCK_SMASH, 1,
        MauvilleCity_PokemonCenter_1F_Text_UsedRockSmashTitle,
        MauvilleCity_PokemonCenter_1F_Text_UsedRockSmashAction,
        MauvilleCity_PokemonCenter_1F_Text_UsedRockSmashStory
    },
    {
        GAME_STAT_MOVED_SECRET_BASE, 1,
        MauvilleCity_PokemonCenter_1F_Text_MovedBasesTitle,
        MauvilleCity_PokemonCenter_1F_Text_MovedBasesAction,
        MauvilleCity_PokemonCenter_1F_Text_MovedBasesStory
    },
    {
        GAME_STAT_USED_SPLASH, 1,
        MauvilleCity_PokemonCenter_1F_Text_UsedSplashTitle,
        MauvilleCity_PokemonCenter_1F_Text_UsedSplashAction,
        MauvilleCity_PokemonCenter_1F_Text_UsedSplashStory
    },
    {
        GAME_STAT_USED_STRUGGLE, 1,
        MauvilleCity_PokemonCenter_1F_Text_UsedStruggleTitle,
        MauvilleCity_PokemonCenter_1F_Text_UsedStruggleAction,
        MauvilleCity_PokemonCenter_1F_Text_UsedStruggleStory
    },
    {
        GAME_STAT_SLOT_JACKPOTS, 1,
        MauvilleCity_PokemonCenter_1F_Text_SlotJackpotsTitle,
        MauvilleCity_PokemonCenter_1F_Text_SlotJackpotsAction,
        MauvilleCity_PokemonCenter_1F_Text_SlotJackpotsStory
    },
    {
        GAME_STAT_CONSECUTIVE_ROULETTE_WINS, 2,
        MauvilleCity_PokemonCenter_1F_Text_RouletteWinsTitle,
        MauvilleCity_PokemonCenter_1F_Text_RouletteWinsAction,
        MauvilleCity_PokemonCenter_1F_Text_RouletteWinsStory
    },
    {
        GAME_STAT_ENTERED_BATTLE_TOWER, 1,
        MauvilleCity_PokemonCenter_1F_Text_BattleTowerChallengesTitle,
        MauvilleCity_PokemonCenter_1F_Text_BattleTowerChallengesAction,
        MauvilleCity_PokemonCenter_1F_Text_BattleTowerChallengesStory
    },
    {
        GAME_STAT_POKEBLOCKS, 1,
        MauvilleCity_PokemonCenter_1F_Text_MadePokeblocksTitle,
        MauvilleCity_PokemonCenter_1F_Text_MadePokeblocksAction,
        MauvilleCity_PokemonCenter_1F_Text_MadePokeblocksStory
    },
    {
        GAME_STAT_ENTERED_CONTEST, 1,
        MauvilleCity_PokemonCenter_1F_Text_EnteredContestsTitle,
        MauvilleCity_PokemonCenter_1F_Text_EnteredContestsAction,
        MauvilleCity_PokemonCenter_1F_Text_EnteredContestsStory
    },
    {
        GAME_STAT_WON_CONTEST, 1,
        MauvilleCity_PokemonCenter_1F_Text_WonContestsTitle,
        MauvilleCity_PokemonCenter_1F_Text_WonContestsAction,
        MauvilleCity_PokemonCenter_1F_Text_WonContestsStory
    },
    {
        GAME_STAT_SHOPPED, 1,
        MauvilleCity_PokemonCenter_1F_Text_TimesShoppedTitle,
        MauvilleCity_PokemonCenter_1F_Text_TimesShoppedAction,
        MauvilleCity_PokemonCenter_1F_Text_TimesShoppedStory
    },
    {
        GAME_STAT_USED_ITEMFINDER, 1,
        MauvilleCity_PokemonCenter_1F_Text_UsedItemFinderTitle,
        MauvilleCity_PokemonCenter_1F_Text_UsedItemFinderAction,
        MauvilleCity_PokemonCenter_1F_Text_UsedItemFinderStory
    },
    {
        GAME_STAT_GOT_RAINED_ON, 1,
        MauvilleCity_PokemonCenter_1F_Text_TimesRainedTitle,
        MauvilleCity_PokemonCenter_1F_Text_TimesRainedAction,
        MauvilleCity_PokemonCenter_1F_Text_TimesRainedStory
    },
    {
        GAME_STAT_CHECKED_POKEDEX, 1,
        MauvilleCity_PokemonCenter_1F_Text_CheckedPokedexTitle,
        MauvilleCity_PokemonCenter_1F_Text_CheckedPokedexAction,
        MauvilleCity_PokemonCenter_1F_Text_CheckedPokedexStory
    },
    {
        GAME_STAT_RECEIVED_RIBBONS, 1,
        MauvilleCity_PokemonCenter_1F_Text_ReceivedRibbonsTitle,
        MauvilleCity_PokemonCenter_1F_Text_ReceivedRibbonsAction,
        MauvilleCity_PokemonCenter_1F_Text_ReceivedRibbonsStory
    },
    {
        GAME_STAT_JUMPED_DOWN_LEDGES, 1,
        MauvilleCity_PokemonCenter_1F_Text_LedgesJumpedTitle,
        MauvilleCity_PokemonCenter_1F_Text_LedgesJumpedAction,
        MauvilleCity_PokemonCenter_1F_Text_LedgesJumpedStory
    },
    {
        GAME_STAT_WATCHED_TV, 1,
        MauvilleCity_PokemonCenter_1F_Text_TVWatchedTitle,
        MauvilleCity_PokemonCenter_1F_Text_TVWatchedAction,
        MauvilleCity_PokemonCenter_1F_Text_TVWatchedStory
    },
    {
        GAME_STAT_CHECKED_CLOCK, 1,
        MauvilleCity_PokemonCenter_1F_Text_CheckedClockTitle,
        MauvilleCity_PokemonCenter_1F_Text_CheckedClockAction,
        MauvilleCity_PokemonCenter_1F_Text_CheckedClockStory
    },
    {
        GAME_STAT_WON_POKEMON_LOTTERY, 1,
        MauvilleCity_PokemonCenter_1F_Text_WonLotteryTitle,
        MauvilleCity_PokemonCenter_1F_Text_WonLotteryAction,
        MauvilleCity_PokemonCenter_1F_Text_WonLotteryStory
    },
    {
        GAME_STAT_USED_DAYCARE, 1,
        MauvilleCity_PokemonCenter_1F_Text_UsedDaycareTitle,
        MauvilleCity_PokemonCenter_1F_Text_UsedDaycareAction,
        MauvilleCity_PokemonCenter_1F_Text_UsedDaycareStory
    },
    {
        GAME_STAT_RODE_CABLE_CAR, 1,
        MauvilleCity_PokemonCenter_1F_Text_RodeCableCarTitle,
        MauvilleCity_PokemonCenter_1F_Text_RodeCableCarAction,
        MauvilleCity_PokemonCenter_1F_Text_RodeCableCarStory
    },
    {
        GAME_STAT_ENTERED_HOT_SPRINGS, 1,
        MauvilleCity_PokemonCenter_1F_Text_HotSpringsTitle,
        MauvilleCity_PokemonCenter_1F_Text_HotSpringsAction,
        MauvilleCity_PokemonCenter_1F_Text_HotSpringsStory
    }
};

static const s32 sNumStories = ARRAY_COUNT(sStorytellerStories);
static const u32 sUnused = 8;

static void StorytellerSetup(void)
{
    s32 i;
    sStorytellerPtr = &gSaveBlock1Ptr->oldMan.storyteller;

    sStorytellerPtr->id = MAUVILLE_MAN_STORYTELLER;
    sStorytellerPtr->alreadyRecorded = FALSE;
    for (i = 0; i < NUM_STORYTELLER_TALES; i++)
    {
        sStorytellerPtr->gameStatIDs[i] = 0;
        sStorytellerPtr->trainerNames[0][i] = EOS;  // Maybe they meant storyteller->trainerNames[i][0] instead?
    }
}

static void Storyteller_ResetFlag(void)
{
    sStorytellerPtr = &gSaveBlock1Ptr->oldMan.storyteller;

    sStorytellerPtr->id = MAUVILLE_MAN_STORYTELLER;
    sStorytellerPtr->alreadyRecorded = FALSE;
}

static u32 StorytellerGetGameStat(u8 stat)
{
    if (stat == 50)
        stat = GAME_STAT_SAVED_GAME;
    return GetGameStat(stat);
}

static const struct Story *GetStoryByStat(u32 stat)
{
    s32 i;

    for (i = 0; i < sNumStories; i++)
    {
        if (sStorytellerStories[i].stat == stat)
            return &sStorytellerStories[i];
    }
    return &sStorytellerStories[sNumStories - 1];
}

static const u8 *GetStoryTitleByStat(u32 stat)
{
    return GetStoryByStat(stat)->title;
}

static const u8 *GetStoryTextByStat(u32 stat)
{
    return GetStoryByStat(stat)->fullText;
}

static const u8 *GetStoryActionByStat(u32 stat)
{
    return GetStoryByStat(stat)->action;
}

static u8 GetFreeStorySlot(void)
{
    u8 i;

    for (i = 0; i < NUM_STORYTELLER_TALES; i++)
    {
        if (sStorytellerPtr->gameStatIDs[i] == 0)
            break;
    }
    return i;
}

static u32 StorytellerGetRecordedTrainerStat(u32 trainer)
{
    u8 *ptr = sStorytellerPtr->statValues[trainer];

    return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}

static void StorytellerSetRecordedTrainerStat(u32 trainer, u32 val)
{
    u8 *ptr = sStorytellerPtr->statValues[trainer];

    ptr[0] = val;
    ptr[1] = val >> 8;
    ptr[2] = val >> 16;
    ptr[3] = val >> 24;
}

static bool32 HasTrainerStatIncreased(u32 trainer)
{
    if (StorytellerGetGameStat(sStorytellerPtr->gameStatIDs[trainer]) > StorytellerGetRecordedTrainerStat(trainer))
        return TRUE;
    else
        return FALSE;
}

static void GetStoryByStattellerPlayerName(u32 player, void *dst)
{
    u8 *name = sStorytellerPtr->trainerNames[player];

    memset(dst, EOS, PLAYER_NAME_LENGTH + 1);
    memcpy(dst, name, PLAYER_NAME_LENGTH);
}

static void StorytellerSetPlayerName(u32 player, const u8 * src)
{
    u8 * name = sStorytellerPtr->trainerNames[player];
    memset(name, EOS, PLAYER_NAME_LENGTH);
    memcpy(name, src, PLAYER_NAME_LENGTH);
}


static void StorytellerRecordNewStat(u32 player, u32 stat)
{
    sStorytellerPtr->gameStatIDs[player] = stat;
    StorytellerSetPlayerName(player, gSaveBlock2Ptr->playerName);
    StorytellerSetRecordedTrainerStat(player, StorytellerGetGameStat(stat));
    ConvertIntToDecimalStringN(gStringVar1, StorytellerGetGameStat(stat), STR_CONV_MODE_LEFT_ALIGN, 10);
    StringCopy(gStringVar2, GetStoryActionByStat(stat));
    sStorytellerPtr->language[player] = gGameLanguage;
}

static void ScrambleStatList(u8 * arr, s32 count)
{
    s32 i;

    for (i = 0; i < count; i++)
        arr[i] = i;
    for (i = 0; i < count; i++)
    {
        u32 a = Random() % count;
        u32 b = Random() % count;
        u8 temp;
        SWAP(arr[a], arr[b], temp);
    }
}

static bool8 StorytellerInitializeRandomStat(void)
{
    u8 storyIds[sNumStories];
    s32 i, j;

    ScrambleStatList(storyIds, sNumStories);
    for (i = 0; i < sNumStories; i++)
    {
        u8 stat = sStorytellerStories[storyIds[i]].stat;
        u8 minVal = sStorytellerStories[storyIds[i]].minVal;

        for (j = 0; j < NUM_STORYTELLER_TALES; j++)
        {
            if (sStorytellerPtr->gameStatIDs[j] == stat)
                break;
        }
        if (j == NUM_STORYTELLER_TALES && StorytellerGetGameStat(stat) >= minVal)
        {
            sStorytellerPtr->alreadyRecorded = TRUE;
            if (GetFreeStorySlot() == NUM_STORYTELLER_TALES)
                StorytellerRecordNewStat(sSelectedStory, stat);
            else
                StorytellerRecordNewStat(GetFreeStorySlot(), stat);
            return TRUE;
        }
    }
    return FALSE;
}

static void StorytellerDisplayStory(u32 player)
{
    u8 stat = sStorytellerPtr->gameStatIDs[player];

    ConvertIntToDecimalStringN(gStringVar1, StorytellerGetRecordedTrainerStat(player), STR_CONV_MODE_LEFT_ALIGN, 10);
    StringCopy(gStringVar2, GetStoryActionByStat(stat));
    GetStoryByStattellerPlayerName(player, gStringVar3);
    ConvertInternationalString(gStringVar3, sStorytellerPtr->language[player]);
    ShowFieldMessage(GetStoryTextByStat(stat));
}

static void PrintStoryList(void)
{
    s32 i;
    s32 width = GetStringWidth(FONT_NORMAL, gText_Exit, 0);
    for (i = 0; i < NUM_STORYTELLER_TALES; i++)
    {
        s32 curWidth;
        u16 gameStatID = sStorytellerPtr->gameStatIDs[i];

        if (gameStatID == 0)
            break;
        curWidth = GetStringWidth(FONT_NORMAL, GetStoryTitleByStat(gameStatID), 0);
        if (curWidth > width)
            width = curWidth;
    }
    sStorytellerWindowId = CreateWindowFromRect(0, 0, ConvertPixelWidthToTileWidth(width), GetFreeStorySlot() * 2 + 2);
    SetStandardWindowBorderStyle(sStorytellerWindowId, FALSE);
    for (i = 0; i < NUM_STORYTELLER_TALES; i++)
    {
        u16 gameStatID = sStorytellerPtr->gameStatIDs[i];
        if (gameStatID == 0)
            break;
        AddTextPrinterParameterized(sStorytellerWindowId, FONT_NORMAL, GetStoryTitleByStat(gameStatID), 8, 16 * i + 1, TEXT_SKIP_DRAW, NULL);
    }
    AddTextPrinterParameterized(sStorytellerWindowId, FONT_NORMAL, gText_Exit, 8, 16 * i + 1, TEXT_SKIP_DRAW, NULL);
    InitMenuInUpperLeftCornerNormal(sStorytellerWindowId, GetFreeStorySlot() + 1, 0);
    CopyWindowToVram(sStorytellerWindowId, COPYWIN_FULL);
}

static void Task_StoryListMenu(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    s32 selection;

    switch (task->data[0])
    {
    case 0:
        PrintStoryList();
        task->data[0]++;
        break;
    case 1:
        selection = Menu_ProcessInput();
        if (selection == MENU_NOTHING_CHOSEN)
            break;
        if (selection == MENU_B_PRESSED || selection == GetFreeStorySlot())
        {
            gSpecialVar_Result = 0;
        }
        else
        {
            gSpecialVar_Result = 1;
            sSelectedStory = selection;
        }
        ClearToTransparentAndRemoveWindow(sStorytellerWindowId);
        DestroyTask(taskId);
        ScriptContext_Enable();
        break;
    }
}

// Sets gSpecialVar_Result to TRUE if player selected a story
void StorytellerStoryListMenu(void)
{
    CreateTask(Task_StoryListMenu, 80);
}

void Script_StorytellerDisplayStory(void)
{
    StorytellerDisplayStory(sSelectedStory);
}

u8 StorytellerGetFreeStorySlot(void)
{
    sStorytellerPtr = &gSaveBlock1Ptr->oldMan.storyteller;
    return GetFreeStorySlot();
}

// Returns TRUE if stat has increased
bool8 StorytellerUpdateStat(void)
{
    u8 stat;
    sStorytellerPtr = &gSaveBlock1Ptr->oldMan.storyteller;
    stat = sStorytellerPtr->gameStatIDs[sSelectedStory];

    if (HasTrainerStatIncreased(sSelectedStory) == TRUE)
    {
        StorytellerRecordNewStat(sSelectedStory, stat);
        return TRUE;
    }
    return FALSE;
}

bool8 HasStorytellerAlreadyRecorded(void)
{
    sStorytellerPtr = &gSaveBlock1Ptr->oldMan.storyteller;

    if (sStorytellerPtr->alreadyRecorded == FALSE)
        return FALSE;
    else
        return TRUE;
}

bool8 Script_StorytellerInitializeRandomStat(void)
{
    sStorytellerPtr = &gSaveBlock1Ptr->oldMan.storyteller;
    return StorytellerInitializeRandomStat();
}

