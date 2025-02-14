#include "global.h"
#include "apprentice.h"
#include "battle.h"
#include "battle_tower.h"
#include "data.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "international_string_util.h"
#include "item.h"
#include "item_menu.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "new_game.h"
#include "party_menu.h"
#include "random.h"
#include "script.h"
#include "script_menu.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "constants/battle_frontier.h"
#include "constants/items.h"
#include "constants/songs.h"
#include "constants/trainers.h"
#include "constants/moves.h"

/* Summary of Apprentice, because (as of writing at least) it's not very well documented online
 *
 * ## Basic info
 * In the Battle Tower lobby there is an NPC which asks to be taught by the player
 * They can be any 1 of 16 NPC trainers, each with their own name, class, and set of possible party species
 * They ask the player a series of questions once per day, and eventually depart the lobby to be replaced by a new Apprentice
 *
 * ## Initial Questions
 * The first question they always ask is a request to be taught, which cannot be rejected
 * The second question (which follows immediately after) is whether they should participate in Battle Tower Lv 50 or Open Lv
 * After these opening questions they always ask the player to choose between 2 mons, which they repeat 3 times
 *
 * ## Random Questions
 * After choosing 3 mons for them, the Apprentice will randomly ask between 1 and 8 questions of 4 different types, as follows
 * - Asking which mon to lead with, which they will only ask at most once
 * - Asking which move a mon should use, which they will ask at most 5 times
 * - Asking what held item to give to a mon, which they will ask at most 3 times (once for each mon)
 * - Asking what they should say when they win a battle, which will always be their final question before departing
 *
 * ## After departing
 * After telling them what they should say when they win a battle they will leave the lobby for a final time
 * They will then be replaced by a new random Apprentice (they can repeat)
 * Up to 4 old Apprentices are saved and can be encountered (or partnered with) during challenges of the mode they were told to battle in
 * They can also be record mixed to and from other Emerald games
 * Old/record mixed Apprentices are stored in struct Apprentice apprentices of SaveBlock2
 *   and the current Apprentice is stored in struct PlayersApprentice playerApprentice of SaveBlock2
 */

#define PLAYER_APPRENTICE gSaveBlock2Ptr->playerApprentice
#define CURRENT_QUESTION_NUM  PLAYER_APPRENTICE.questionsAnswered - NUM_WHICH_MON_QUESTIONS

struct ApprenticePartyMovesData
{
    u8 moveCounter;
    u16 moves[MULTI_PARTY_SIZE][NUM_WHICH_MOVE_QUESTIONS];
    u8 moveSlots[MULTI_PARTY_SIZE][NUM_WHICH_MOVE_QUESTIONS];
};

struct ApprenticeQuestionData
{
    u16 speciesId;
    u16 altSpeciesId;
    u16 moveId1;
    u16 moveId2;
};

// IWRAM common
COMMON_DATA struct ApprenticePartyMovesData *gApprenticePartyMovesData = NULL;
COMMON_DATA struct ApprenticeQuestionData *gApprenticeQuestionData = NULL;
COMMON_DATA void (*gApprenticeFunc)(void) = NULL;

// This file's functions.
static u16 GetRandomAlternateMove(u8 monId);
static bool8 TrySetMove(u8 monId, u16 moveId);
static void CreateChooseAnswerTask(bool8 noBButton, u8 itemsCount, u8 windowId);
static u8 CreateAndShowWindow(u8 left, u8 top, u8 width, u8 height);
static void RemoveAndHideWindow(u8 windowId);
static void ExecuteFuncAfterButtonPress(void (*func)(void));

static void Script_GivenApprenticeLvlMode(void);
static void Script_SetApprenticeLvlMode(void);
static void Script_SetApprenticeId(void);
static void ShuffleApprenticeSpecies(void);
static void Script_SetRandomQuestionData(void);
static void IncrementQuestionsAnswered(void);
static void IsFinalQuestion(void);
static void Script_CreateApprenticeMenu(void);
static void Script_PrintApprenticeMessage(void);
static void Script_ResetPlayerApprentice(void);
static void GetShouldCheckApprenticeGone(void);
static void ApprenticeGetQuestion(void);
static void GetNumApprenticePartyMonsAssigned(void);
static void SetApprenticePartyMon(void);
static void InitQuestionData(void);
static void FreeQuestionData(void);
static void ApprenticeBufferString(void);
static void SetApprenticeMonMove(void);
static void SetLeadApprenticeMon(void);
static void Script_ApprenticeOpenBagMenu(void);
static void TrySetApprenticeHeldItem(void);
static void SaveApprentice(void);
static void SetSavedApprenticeTrainerGfxId(void);
static void SetPlayerApprenticeTrainerGfxId(void);
static void GetShouldApprenticeLeave(void);
static void ShiftSavedApprentices(void);

#include "data/battle_frontier/apprentice.h"

void BufferApprenticeChallengeText(u8 saveApprenticeId)
{
    u8 i, num;
    const u8 *challengeText;

    num = gSaveBlock2Ptr->apprentices[saveApprenticeId].number;
    for (i = 0; num != 0 && i < APPRENTICE_COUNT; num /= 10, i++)
        ;

    StringCopy_PlayerName(gStringVar1, gSaveBlock2Ptr->apprentices[saveApprenticeId].playerName);
    ConvertInternationalString(gStringVar1, gSaveBlock2Ptr->apprentices[saveApprenticeId].language);
    ConvertIntToDecimalStringN(gStringVar2, gSaveBlock2Ptr->apprentices[saveApprenticeId].number, STR_CONV_MODE_RIGHT_ALIGN, i);
    challengeText = sApprenticeChallengeTexts[gSaveBlock2Ptr->apprentices[saveApprenticeId].id];
    StringExpandPlaceholders(gStringVarBuffer, challengeText);
}

void Apprentice_ScriptContext_Enable(void)
{
    ScriptContext_Enable();
}

void ResetApprenticeStruct(struct Apprentice *apprentice)
{
    u8 i;

    for (i = 0; i < ARRAY_COUNT(apprentice->speechWon); i++)
        apprentice->speechWon[i] = EC_EMPTY_WORD;

    apprentice->playerName[0] = EOS;
    apprentice->id = NUM_APPRENTICES;
}

void ResetAllApprenticeData(void)
{
    u8 i, j;

    PLAYER_APPRENTICE.saveId = 0;
    for (i = 0; i < APPRENTICE_COUNT; i++)
    {
        for (j = 0; j < ARRAY_COUNT(gSaveBlock2Ptr->apprentices[i].speechWon); j++)
            gSaveBlock2Ptr->apprentices[i].speechWon[j] = EC_EMPTY_WORD;
        gSaveBlock2Ptr->apprentices[i].id = NUM_APPRENTICES;
        gSaveBlock2Ptr->apprentices[i].playerName[0] = EOS;
        gSaveBlock2Ptr->apprentices[i].lvlMode = 0;
        gSaveBlock2Ptr->apprentices[i].number = 0;
        gSaveBlock2Ptr->apprentices[i].numQuestions = 0;
        for (j = 0; j < TRAINER_ID_LENGTH; j++)
            gSaveBlock2Ptr->apprentices[i].playerId[j] = 0;
        gSaveBlock2Ptr->apprentices[i].language = gGameLanguage;
        gSaveBlock2Ptr->apprentices[i].checksum = 0;
    }

    Script_ResetPlayerApprentice();
}

static bool8 GivenApprenticeLvlMode(void)
{
    return (PLAYER_APPRENTICE.lvlMode != 0);
}

static void SetApprenticeId(void)
{
    if (gSaveBlock2Ptr->apprentices[0].number == 0)
    {
        do
        {
            PLAYER_APPRENTICE.id = sInitialApprenticeIds[Random() % ARRAY_COUNT(sInitialApprenticeIds)];
        } while (PLAYER_APPRENTICE.id == gSaveBlock2Ptr->apprentices[0].id);
    }
    else
    {
        do
        {
            PLAYER_APPRENTICE.id = Random() % (NUM_APPRENTICES);
        } while (PLAYER_APPRENTICE.id == gSaveBlock2Ptr->apprentices[0].id);
    }
}

static void SetPlayersApprenticeLvlMode(u8 mode)
{
    PLAYER_APPRENTICE.lvlMode = mode;
}

static void ShuffleApprenticeSpecies(void)
{
    u8 species[APPRENTICE_SPECIES_COUNT];
    u8 i;

    for (i = 0; i < ARRAY_COUNT(species); i++)
        species[i] = i;

    // Shuffle the possible species an arbitrary 50 times
    for (i = 0; i < 50; i++)
    {
        u8 temp;
        u8 rand1 = Random() % ARRAY_COUNT(species);
        u8 rand2 = Random() % ARRAY_COUNT(species);
        SWAP(species[rand1], species[rand2], temp);
    }

    for (i = 0; i < MULTI_PARTY_SIZE; i++)
        PLAYER_APPRENTICE.speciesIds[i] = ((species[i * 2] & 0xF) << 4) | ((species[i * 2 + 1]) & 0xF);
}

// Pick one of the Apprentice's mons to ask the question about
// Picking a move chooses a random mon, picking a held item is sequential (so that none are repeated)
static u8 GetMonIdForQuestion(u8 questionId, u8 *party, u8 *partySlot)
{
    u8 i, count;
    u8 monId = 0;

    if (questionId == QUESTION_ID_WHICH_MOVE)
    {
        do
        {
            monId = Random() % (MULTI_PARTY_SIZE);
            for (count = 0, i = 0; i < NUM_WHICH_MOVE_QUESTIONS; i++)
            {
                if (gApprenticePartyMovesData->moves[monId][i] != MOVE_NONE)
                    count++;
            }
        } while (count > MULTI_PARTY_SIZE);
    }
    else if (questionId == QUESTION_ID_WHAT_ITEM)
    {
        monId = party[*partySlot];
        (*partySlot)++;
    }

    return monId;
}

// Sets the random order and data for the remaining questions after the initial "choose mon" questions
static void SetRandomQuestionData(void)
{
    u8 questionOrder[APPRENTICE_MAX_QUESTIONS + 1];
    u8 partyOrder[MULTI_PARTY_SIZE];
    u8 partySlot;
    u8 i, j;
    u8 rand1, rand2;
    u8 id;

    for (i = 0; i < ARRAY_COUNT(partyOrder); i++)
        partyOrder[i] = i;

    // Shuffle the party an arbitrary 10 times
    for (i = 0; i < 10; i++)
    {
        u8 temp;
        rand1 = Random() % ARRAY_COUNT(partyOrder);
        rand2 = Random() % ARRAY_COUNT(partyOrder);
        SWAP(partyOrder[rand1], partyOrder[rand2], temp);
    }

    for (i = 0; i < ARRAY_COUNT(questionOrder); i++)
        questionOrder[i] = sQuestionPossibilities[i];

    // Shuffle the questions an arbitrary 50 times
    for (i = 0; i < 50; i++)
    {
        u8 temp;
        rand1 = Random() % ARRAY_COUNT(questionOrder);
        rand2 = Random() % ARRAY_COUNT(questionOrder);
        SWAP(questionOrder[rand1], questionOrder[rand2], temp);
    }

    gApprenticePartyMovesData = AllocZeroed(sizeof(*gApprenticePartyMovesData));
    gApprenticePartyMovesData->moveCounter = 0;
    for (i = 0; i < NUM_WHICH_MOVE_QUESTIONS; i++)
    {
        for (j = 0; j < MULTI_PARTY_SIZE; j++)
            gApprenticePartyMovesData->moveSlots[j][i] = MAX_MON_MOVES;
    }

    partySlot = 0;
    for (i = 0; i < APPRENTICE_MAX_QUESTIONS; i++)
    {
        PLAYER_APPRENTICE.questions[i].questionId = questionOrder[i];
        if (questionOrder[i] != QUESTION_ID_WHICH_FIRST)
        {
            PLAYER_APPRENTICE.questions[i].monId = GetMonIdForQuestion(questionOrder[i], partyOrder, &partySlot);
            id = PLAYER_APPRENTICE.questions[i].monId;
            if (questionOrder[i] == QUESTION_ID_WHICH_MOVE)
            {
                do
                {
                    rand1 = Random() % MAX_MON_MOVES;
                    for (j = 0; j < gApprenticePartyMovesData->moveCounter + 1; j++)
                    {
                        if (gApprenticePartyMovesData->moveSlots[id][j] == rand1)
                            break;
                    }
                } while (j != gApprenticePartyMovesData->moveCounter + 1);

                gApprenticePartyMovesData->moveSlots[id][gApprenticePartyMovesData->moveCounter] = rand1;
                PLAYER_APPRENTICE.questions[i].moveSlot = rand1;
                PLAYER_APPRENTICE.questions[i].data = GetRandomAlternateMove(PLAYER_APPRENTICE.questions[i].monId);
            }
        }
    }

    FREE_AND_SET_NULL(gApprenticePartyMovesData);
}

#define APPRENTICE_SPECIES_ID(monId) \
    ((monId < MULTI_PARTY_SIZE) ? (PLAYER_APPRENTICE.speciesIds[monId] >> (((PLAYER_APPRENTICE.party >> monId) & 1) << 2) & 0xF) : 0)

#define APPRENTICE_SPECIES_ID_NO_COND(monId, count) \
    monId = ((PLAYER_APPRENTICE.party >> count) & 1); \
    monId = ((PLAYER_APPRENTICE.speciesIds[count]) >> (monId << 2)) & 0xF; \

// Get the second move choice for the "Which move" question
// Unlike the first move choice, this can be either a level up move or a TM/HM move
static u16 GetRandomAlternateMove(u8 monId)
{
    u8 i, j;
    u8 id;
    u8 numLearnsetMoves;
    u16 species;
    const u16 *learnset;
    bool32 needTMs = FALSE;
    u16 moveId = MOVE_NONE;
    bool32 shouldUseMove;
    u8 level;

    id = APPRENTICE_SPECIES_ID(monId);
    species = gApprentices[PLAYER_APPRENTICE.id].species[id];
    learnset = gLevelUpLearnsets[species];
    j = 0;

    if (PLAYER_APPRENTICE.lvlMode == APPRENTICE_LVL_MODE_50)
        level = FRONTIER_MAX_LEVEL_50;
    else // == APPRENTICE_LVL_MODE_OPEN
        level = 60; // Despite being open level, level up moves are only read up to level 60

    for (j = 0; learnset[j] != LEVEL_UP_END; j++)
    {
        if ((learnset[j] & LEVEL_UP_MOVE_LV) > (level << 9))
            break;
    }

    numLearnsetMoves = j;
    i = 0;

    // i < 5 here is arbitrary, i isnt used and is only incremented when the selected move isnt in sValidApprenticeMoves
    // This while loop contains 3 potential infinite loops, though none of them would occur in the base game
    while (i < 5)
    {
        if (Random() % 2 == 0 || needTMs == TRUE)
        {
            // Get TM move
            // NOTE: Below is an infinite loop if a species that only learns TMs for moves
            //       that are also in its level up learnset is assigned to an Apprentice
            do
            {
                // NOTE: Below is an infinite loop if a species which cannot learn TMs is assigned to an Apprentice
                do
                {
                    id = Random() % (NUM_TECHNICAL_MACHINES + NUM_HIDDEN_MACHINES);
                    shouldUseMove = CanSpeciesLearnTMHM(species, id);
                }
                while (!shouldUseMove);

                moveId = ItemIdToBattleMoveId(ITEM_TM01 + id);
                shouldUseMove = TRUE;

                if (numLearnsetMoves <= MAX_MON_MOVES)
                    j = 0;
                else
                    j = numLearnsetMoves - MAX_MON_MOVES;

                for (; j < numLearnsetMoves; j++)
                {
                    // Keep looking for TMs until one not in the level up learnset is found
                    if ((learnset[j] & LEVEL_UP_MOVE_ID) == moveId)
                    {
                        shouldUseMove = FALSE;
                        break;
                    }
                }
            } while (shouldUseMove != TRUE);
        }
        else
        {
            if (numLearnsetMoves <= MAX_MON_MOVES)
            {
                needTMs = TRUE;
                continue;
            }
            else
            {
                // Get level up move
                // NOTE: Below is an infinite loop if a mon whose last 4 moves contain
                //       all the moves in the rest of its learnset is assigned to an Apprentice
                do
                {
                    // Get a random move excluding the 4 it would know at max level
                    u8 learnsetId = Random() % (numLearnsetMoves - MAX_MON_MOVES);
                    moveId = learnset[learnsetId] & LEVEL_UP_MOVE_ID;
                    shouldUseMove = TRUE;

                    for (j = numLearnsetMoves - MAX_MON_MOVES; j < numLearnsetMoves; j++)
                    {
                        // Keep looking for moves until one not in the last 4 is found
                        if ((learnset[j] & LEVEL_UP_MOVE_ID) == moveId)
                        {
                            shouldUseMove = FALSE;
                            break;
                        }
                    }
                } while (shouldUseMove != TRUE);
            }
        }

        if (TrySetMove(monId, moveId))
        {
            if (sValidApprenticeMoves[moveId])
                break;
            i++;
        }
    }

    gApprenticePartyMovesData->moveCounter++;
    return moveId;
}

static bool8 TrySetMove(u8 monId, u16 moveId)
{
    u8 i;

    for (i = 0; i < NUM_WHICH_MOVE_QUESTIONS; i++)
    {
        if (gApprenticePartyMovesData->moves[monId][i] == moveId)
            return FALSE;
    }

    gApprenticePartyMovesData->moves[monId][gApprenticePartyMovesData->moveCounter] = moveId;
    return TRUE;
}

static void GetLatestLearnedMoves(u16 species, u16 *moves)
{
    u8 i, j;
    u8 level, numLearnsetMoves;
    const u16 *learnset;

    if (PLAYER_APPRENTICE.lvlMode == APPRENTICE_LVL_MODE_50)
        level = FRONTIER_MAX_LEVEL_50;
    else // == APPRENTICE_LVL_MODE_OPEN
        level = 60;

    learnset = gLevelUpLearnsets[species];
    for (i = 0; learnset[i] != LEVEL_UP_END; i++)
    {
        if ((learnset[i] & LEVEL_UP_MOVE_LV) > (level << 9))
            break;
    }

    numLearnsetMoves = i;
    if (numLearnsetMoves > MAX_MON_MOVES)
        numLearnsetMoves = MAX_MON_MOVES;

    for (j = 0; j < numLearnsetMoves; j++)
        moves[j] = learnset[(i - 1) - j] & LEVEL_UP_MOVE_ID;
}

// Get the level up move or previously suggested move to be the first move choice
// Compare to GetRandomAlternateMove, which gets the move that will be the second choice
static u16 GetDefaultMove(u8 monId, u8 speciesArrayId, u8 moveSlot)
{
    u16 moves[MAX_MON_MOVES];
    u8 i, numQuestions;

    if (PLAYER_APPRENTICE.questionsAnswered < NUM_WHICH_MON_QUESTIONS)
        return MOVE_NONE;

    numQuestions = 0;
    for (i = 0; i < APPRENTICE_MAX_QUESTIONS && PLAYER_APPRENTICE.questions[i].questionId != QUESTION_ID_WIN_SPEECH; i++)
        numQuestions++;

    GetLatestLearnedMoves(gApprentices[PLAYER_APPRENTICE.id].species[speciesArrayId], moves);
    for (i = 0; i < numQuestions && i < CURRENT_QUESTION_NUM; i++)
    {
        if (PLAYER_APPRENTICE.questions[i].questionId == QUESTION_ID_WHICH_MOVE
            && PLAYER_APPRENTICE.questions[i].monId == monId
            && PLAYER_APPRENTICE.questions[i].suggestedChange)
        {
            moves[PLAYER_APPRENTICE.questions[i].moveSlot] = PLAYER_APPRENTICE.questions[i].data;
        }
    }

    return moves[moveSlot];
}

static void SaveApprenticeParty(u8 numQuestions)
{
    struct ApprenticeMon *apprenticeMons[MULTI_PARTY_SIZE];
    u8 i, j;
    u32 speciesTableId;

    for (i = 0; i < MULTI_PARTY_SIZE; i++)
    {
        gSaveBlock2Ptr->apprentices[0].party[i].species = SPECIES_NONE;
        gSaveBlock2Ptr->apprentices[0].party[i].item = ITEM_NONE;
        for (j = 0; j < MAX_MON_MOVES; j++)
            gSaveBlock2Ptr->apprentices[0].party[i].moves[j] = MOVE_NONE;
    }

    // Save party order
    j = PLAYER_APPRENTICE.leadMonId;
    for (i = 0; i < MULTI_PARTY_SIZE; i++)
    {
        apprenticeMons[j] = &gSaveBlock2Ptr->apprentices[0].party[i];
        j = (j + 1) % (MULTI_PARTY_SIZE);
    }

    // Save party species
    for (i = 0; i < MULTI_PARTY_SIZE; i++)
    {
        speciesTableId = APPRENTICE_SPECIES_ID(i);
        apprenticeMons[i]->species = gApprentices[PLAYER_APPRENTICE.id].species[speciesTableId];
        GetLatestLearnedMoves(apprenticeMons[i]->species, apprenticeMons[i]->moves);
    }

    // Update party based on response to held item / move choice questions
    for (i = 0; i < numQuestions; i++)
    {
        u8 questionId = PLAYER_APPRENTICE.questions[i].questionId;
        u8 monId = PLAYER_APPRENTICE.questions[i].monId;
        if (questionId == QUESTION_ID_WHAT_ITEM)
        {
            if (PLAYER_APPRENTICE.questions[i].suggestedChange)
                apprenticeMons[monId]->item = PLAYER_APPRENTICE.questions[i].data;
        }
        else if (questionId == QUESTION_ID_WHICH_MOVE)
        {
            if (PLAYER_APPRENTICE.questions[i].suggestedChange)
            {
                u32 moveSlot = PLAYER_APPRENTICE.questions[i].moveSlot;
                apprenticeMons[monId]->moves[moveSlot] = PLAYER_APPRENTICE.questions[i].data;
            }
        }
    }
}

static void CreateApprenticeMenu(u8 menu)
{
    u8 i;
    u8 windowId;
    const u8 *strings[3];
    u8 count = 2;
    u8 width;
    u8 left;
    u8 top;
    s32 pixelWidth;

    switch (menu)
    {
    case APPRENTICE_ASK_WHICH_LEVEL:
        left = 18;
        top = 8;
        strings[0] = gText_Lv50;
        strings[1] = gText_OpenLevel;
        break;
    case APPRENTICE_ASK_3SPECIES:
        count = MULTI_PARTY_SIZE;
        left = 18;
        top = 6;
        for (i = 0; i < MULTI_PARTY_SIZE; i++)
        {
            u16 species;
            u32 speciesTableId;

            speciesTableId = APPRENTICE_SPECIES_ID(i);
            species =  gApprentices[PLAYER_APPRENTICE.id].species[speciesTableId];
            strings[i] = gSpeciesNames[species];
        }
        break;
    case APPRENTICE_ASK_2SPECIES:
        left = 18;
        top = 8;
        if (PLAYER_APPRENTICE.questionsAnswered >= NUM_WHICH_MON_QUESTIONS)
            return;
        strings[1] = gSpeciesNames[gApprenticeQuestionData->altSpeciesId];
        strings[0] = gSpeciesNames[gApprenticeQuestionData->speciesId];
        break;
    case APPRENTICE_ASK_MOVES:
        left = 17;
        top = 8;
        strings[0] = gMoveNames[gApprenticeQuestionData->moveId1];
        strings[1] = gMoveNames[gApprenticeQuestionData->moveId2];
        break;
    case APPRENTICE_ASK_GIVE:
        left = 18;
        top = 8;
        strings[0] = gText_Give;
        strings[1] = gText_NoNeed;
        break;
    case APPRENTICE_ASK_YES_NO:
        left = 20;
        top = 8;
        strings[0] = gText_Yes;
        strings[1] = gText_No;
        break;
    default:
        left = 0;
        top = 0;
#ifdef UBFIX
        return;
#endif
        break;
    }

    pixelWidth = 0;
    for (i = 0; i < count; i++)
    {
        s32 width = GetStringWidth(FONT_NORMAL, strings[i], 0);
        if (width > pixelWidth)
            pixelWidth = width;
    }

    width = ConvertPixelWidthToTileWidth(pixelWidth);
    left = ScriptMenu_AdjustLeftCoordFromWidth(left, width);
    windowId = CreateAndShowWindow(left, top, width, count * 2);
    SetStandardWindowBorderStyle(windowId, FALSE);

    for (i = 0; i < count; i++)
        AddTextPrinterParameterized(windowId, FONT_NORMAL, strings[i], 8, (i * 16) + 1, TEXT_SKIP_DRAW, NULL);

    InitMenuInUpperLeftCornerNormal(windowId, count, 0);
    CreateChooseAnswerTask(TRUE, count, windowId);
}

#define tNoBButton data[4]
#define tWrapAround data[5]
#define tWindowId data[6]

static void Task_ChooseAnswer(u8 taskId)
{
    s8 input;
    s16 *data = gTasks[taskId].data;

    if (!tWrapAround)
        input = Menu_ProcessInputNoWrap();
    else
        input = Menu_ProcessInput();

    switch (input)
    {
    case MENU_NOTHING_CHOSEN:
        return;
    case MENU_B_PRESSED:
        // Only ever true. Answering Apprentice questions is required.
        if (tNoBButton)
            return;

        PlaySE(SE_SELECT);
        gSpecialVar_Result = MULTI_B_PRESSED;
        break;
    default:
        gSpecialVar_Result = input;
        break;
    }

    RemoveAndHideWindow(tWindowId);
    DestroyTask(taskId);
    ScriptContext_Enable();
}

static u8 CreateAndShowWindow(u8 left, u8 top, u8 width, u8 height)
{
    u8 windowId;
    struct WindowTemplate winTemplate = CreateWindowTemplate(0, left + 1, top + 1, width, height, 15, 100);

    windowId = AddWindow(&winTemplate);
    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_FULL);
    return windowId;
}

static void RemoveAndHideWindow(u8 windowId)
{
    ClearStdWindowAndFrameToTransparent(windowId, TRUE);
    RemoveWindow(windowId);
}

static void CreateChooseAnswerTask(bool8 noBButton, u8 answers, u8 windowId)
{
    u8 taskId = CreateTask(Task_ChooseAnswer, 80);
    gTasks[taskId].tNoBButton = noBButton;

    if (answers > 3)
        gTasks[taskId].tWrapAround = TRUE;
    else
        gTasks[taskId].tWrapAround = FALSE;

    gTasks[taskId].tWindowId = windowId;
}

#undef tNoBButton
#undef tWrapAround
#undef tWindowId

void CallApprenticeFunction(void)
{
    sApprenticeFunctions[gSpecialVar_0x8004]();
}

static void Script_ResetPlayerApprentice(void)
{
    u8 i;

    SetApprenticeId();
    PLAYER_APPRENTICE.lvlMode = 0;
    PLAYER_APPRENTICE.questionsAnswered = 0;
    PLAYER_APPRENTICE.leadMonId = 0;
    PLAYER_APPRENTICE.party = 0;

    for (i = 0; i < MULTI_PARTY_SIZE; i++)
        PLAYER_APPRENTICE.speciesIds[i] = 0;

    for (i = 0; i < APPRENTICE_MAX_QUESTIONS; i++)
    {
        PLAYER_APPRENTICE.questions[i].questionId = 0;
        PLAYER_APPRENTICE.questions[i].monId = 0;
        PLAYER_APPRENTICE.questions[i].moveSlot = 0;
        PLAYER_APPRENTICE.questions[i].suggestedChange = 0;
        PLAYER_APPRENTICE.questions[i].data = 0;
    }
}

static void Script_GivenApprenticeLvlMode(void)
{
    if (!GivenApprenticeLvlMode())
        gSpecialVar_Result = FALSE;
    else
        gSpecialVar_Result = TRUE;
}

// VAR_0x8005 is 1 + the selection value from the multichoice APPRENTICE_ASK_WHICH_LEVEL
// i.e. APPRENTICE_LVL_MODE_50 or APPRENTICE_LVL_MODE_OPEN
static void Script_SetApprenticeLvlMode(void)
{
    SetPlayersApprenticeLvlMode(gSpecialVar_0x8005);
}

// Never called, APPRENTICE_FUNC_SET_ID is unused
static void Script_SetApprenticeId(void)
{
    SetApprenticeId();
}

static void Script_SetRandomQuestionData(void)
{
    SetRandomQuestionData();
}

static void IncrementQuestionsAnswered(void)
{
    PLAYER_APPRENTICE.questionsAnswered++;
}

// The first 3 questions answered after meeting the Apprentice are always selecting party mons
//  after which this is never called
static void GetNumApprenticePartyMonsAssigned(void)
{
    gSpecialVar_Result = PLAYER_APPRENTICE.questionsAnswered;
}

// Never called, APPRENTICE_FUNC_IS_FINAL_QUESTION is unused
static void IsFinalQuestion(void)
{
    s32 questionNum = CURRENT_QUESTION_NUM;

    if (questionNum < 0)
    {
        // Not finished asking initial questions
        gSpecialVar_Result = FALSE;
    }
    else
    {
        if (questionNum > APPRENTICE_MAX_QUESTIONS - 1)
            gSpecialVar_Result = TRUE;

        if (PLAYER_APPRENTICE.questions[questionNum].questionId == QUESTION_ID_WIN_SPEECH)
            gSpecialVar_Result = TRUE;
        else
            gSpecialVar_Result = FALSE;
    }
}

static void Script_CreateApprenticeMenu(void)
{
    CreateApprenticeMenu(gSpecialVar_0x8005);
}

static void Task_WaitForPrintingMessage(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        DestroyTask(taskId);
        if (gSpecialVar_0x8005)
            ExecuteFuncAfterButtonPress(ScriptContext_Enable);
        else
            ScriptContext_Enable();
    }
}

static void PrintApprenticeMessage(void)
{
    const u8 *string;

    if (gSpecialVar_0x8006 == APPRENTICE_MSG_WHICH_MON)
    {
        string = sApprenticeWhichMonTexts[PLAYER_APPRENTICE.id][0];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_THANKS_MON)
    {
        string = sApprenticeWhichMonTexts[PLAYER_APPRENTICE.id][1];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_WHICH_MOVE)
    {
        string = sApprenticeWhichMoveTexts[PLAYER_APPRENTICE.id][0];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_THANKS_MOVE)
    {
        string = sApprenticeWhichMoveTexts[PLAYER_APPRENTICE.id][1];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_WHICH_MON_FIRST)
    {
        string = sApprenticeWhichMonFirstTexts[PLAYER_APPRENTICE.id][0];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_THANKS_MON_FIRST)
    {
        string = sApprenticeWhichMonFirstTexts[PLAYER_APPRENTICE.id][1];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_WHAT_HELD_ITEM)
    {
        string = sApprenticeHeldItemTexts[PLAYER_APPRENTICE.id][0];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_PICK_WIN_SPEECH)
    {
        string = sApprenticePickWinSpeechTexts[PLAYER_APPRENTICE.id][0];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_THANKS_HELD_ITEM)
    {
        string = sApprenticeHeldItemTexts[PLAYER_APPRENTICE.id][3];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_HOLD_NOTHING)
    {
        string = sApprenticeHeldItemTexts[PLAYER_APPRENTICE.id][1];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_ITEM_ALREADY_SUGGESTED)
    {
        string = sApprenticeHeldItemTexts[PLAYER_APPRENTICE.id][4];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_THANKS_NO_HELD_ITEM)
    {
        string = sApprenticeHeldItemTexts[PLAYER_APPRENTICE.id][2];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_THANKS_WIN_SPEECH)
    {
        string = sApprenticePickWinSpeechTexts[PLAYER_APPRENTICE.id][1];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_PLEASE_TEACH)
    {
        string = sApprenticeFirstMeetingTexts[PLAYER_APPRENTICE.id][0];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_REJECT)
    {
        string = sApprenticeFirstMeetingTexts[PLAYER_APPRENTICE.id][1];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_WHICH_LVL_MODE)
    {
        string = sApprenticeFirstMeetingTexts[PLAYER_APPRENTICE.id][2];
    }
    else if (gSpecialVar_0x8006 == APPRENTICE_MSG_THANKS_LVL_MODE)
    {
        string = sApprenticeFirstMeetingTexts[PLAYER_APPRENTICE.id][3];
    }
    else
    {
        ScriptContext_Enable();
        return;
    }

    StringExpandPlaceholders(gStringVarBuffer, string);
    AddTextPrinterForMessage(TRUE);
    CreateTask(Task_WaitForPrintingMessage, 1);
}

static void Script_PrintApprenticeMessage(void)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    PlayerFreeze();
    StopPlayerAvatar();
    DrawDialogueFrame(0, TRUE);
    PrintApprenticeMessage();
}

static void ApprenticeGetQuestion(void)
{
    if (PLAYER_APPRENTICE.questionsAnswered < NUM_WHICH_MON_QUESTIONS)
    {
        gSpecialVar_Result = APPRENTICE_QUESTION_WHICH_MON;
    }
    else if (PLAYER_APPRENTICE.questionsAnswered > (APPRENTICE_MAX_QUESTIONS + NUM_WHICH_MON_QUESTIONS - 1))
    {
        gSpecialVar_Result = APPRENTICE_QUESTION_WIN_SPEECH;
    }
    else
    {
        s32 id = CURRENT_QUESTION_NUM;
        switch (PLAYER_APPRENTICE.questions[id].questionId)
        {
        case QUESTION_ID_WHAT_ITEM:
            gSpecialVar_Result = APPRENTICE_QUESTION_WHAT_ITEM;
            break;
        case QUESTION_ID_WHICH_MOVE:
            gSpecialVar_Result = APPRENTICE_QUESTION_WHICH_MOVE;
            break;
        case QUESTION_ID_WHICH_FIRST:
            gSpecialVar_Result = APPRENTICE_QUESTION_WHICH_FIRST;
            break;
        default:
      //case QUESTION_ID_WIN_SPEECH:
            gSpecialVar_Result = APPRENTICE_QUESTION_WIN_SPEECH;
            break;
        }
    }
}

// gSpecialVar_0x8005 is 0 or 1 for the mon selection (0 is already on the team)
// gSpecialVar_0x8006 is 0-2 for the number of party mons selected so far
static void SetApprenticePartyMon(void)
{
    if (gSpecialVar_0x8005)
    {
        u8 partySlot = gSpecialVar_0x8006;
        PLAYER_APPRENTICE.party |= 1 << partySlot;
    }
}

// gSpecialVar_0x8005 is 0 or 1 for the move selection
// Selection 0 is implicitly the default move assigned
static void SetApprenticeMonMove(void)
{
    if (PLAYER_APPRENTICE.questionsAnswered >= NUM_WHICH_MON_QUESTIONS)
    {
        u8 id = CURRENT_QUESTION_NUM;
        if (gSpecialVar_0x8005)
            PLAYER_APPRENTICE.questions[id].suggestedChange = TRUE;
        else
            PLAYER_APPRENTICE.questions[id].suggestedChange = FALSE;
    }
}

static void InitQuestionData(void)
{
    u8 i;
    u8 count = 0;
    u8 id1, id2;

    for (i = 0; i < APPRENTICE_MAX_QUESTIONS && (PLAYER_APPRENTICE.questions[i].questionId != QUESTION_ID_WIN_SPEECH); count++, i++)
        ;

    gApprenticeQuestionData = AllocZeroed(sizeof(*gApprenticeQuestionData));
    if (gSpecialVar_0x8005 == APPRENTICE_QUESTION_WHICH_MON)
    {
        if (PLAYER_APPRENTICE.questionsAnswered < NUM_WHICH_MON_QUESTIONS)
        {
            // For the first MULTI_PARTY_SIZE (3) questions, a mon is asked to be selected for the Apprentice's party
            id1 = PLAYER_APPRENTICE.speciesIds[PLAYER_APPRENTICE.questionsAnswered] >> 4;
            gApprenticeQuestionData->altSpeciesId = gApprentices[PLAYER_APPRENTICE.id].species[id1];

            id2 = PLAYER_APPRENTICE.speciesIds[PLAYER_APPRENTICE.questionsAnswered] & 0xF;
            gApprenticeQuestionData->speciesId = gApprentices[PLAYER_APPRENTICE.id].species[id2];
        }
    }
    else if (gSpecialVar_0x8005 == APPRENTICE_QUESTION_WHICH_MOVE)
    {
        if (PLAYER_APPRENTICE.questionsAnswered >= NUM_WHICH_MON_QUESTIONS
            && PLAYER_APPRENTICE.questionsAnswered < count + NUM_WHICH_MON_QUESTIONS
            && PLAYER_APPRENTICE.questions[CURRENT_QUESTION_NUM].questionId == QUESTION_ID_WHICH_MOVE)
        {
            // count re-used as monId
            count = PLAYER_APPRENTICE.questions[CURRENT_QUESTION_NUM].monId;
            APPRENTICE_SPECIES_ID_NO_COND(id1, count);
            gApprenticeQuestionData->speciesId = gApprentices[PLAYER_APPRENTICE.id].species[id1];
            gApprenticeQuestionData->moveId1 = GetDefaultMove(count, id1, PLAYER_APPRENTICE.questions[CURRENT_QUESTION_NUM].moveSlot);
            gApprenticeQuestionData->moveId2 = PLAYER_APPRENTICE.questions[CURRENT_QUESTION_NUM].data;
        }
    }
    else if (gSpecialVar_0x8005 == APPRENTICE_QUESTION_WHAT_ITEM)
    {
        if (PLAYER_APPRENTICE.questionsAnswered >= NUM_WHICH_MON_QUESTIONS
            && PLAYER_APPRENTICE.questionsAnswered < count + NUM_WHICH_MON_QUESTIONS
            && PLAYER_APPRENTICE.questions[CURRENT_QUESTION_NUM].questionId == QUESTION_ID_WHAT_ITEM)
        {
            // count re-used as monId
            count = PLAYER_APPRENTICE.questions[CURRENT_QUESTION_NUM].monId;
            APPRENTICE_SPECIES_ID_NO_COND(id2, count);
            gApprenticeQuestionData->speciesId = gApprentices[PLAYER_APPRENTICE.id].species[id2];
        }
    }
}

static void FreeQuestionData(void)
{
    FREE_AND_SET_NULL(gApprenticeQuestionData);
}

static void ApprenticeBufferString(void)
{
    u8 *stringDst;
    u8 text[16];
    u32 speciesArrayId;

    switch (gSpecialVar_0x8005)
    {
    case 0:
        stringDst = gStringVar1;
        break;
    case 1:
        stringDst = gStringVar2;
        break;
    case 2:
        stringDst = gStringVar3;
        break;
    default:
        return;
    }

    switch (gSpecialVar_0x8006)
    {
    case APPRENTICE_BUFF_SPECIES1:
        StringCopy(stringDst, gSpeciesNames[gApprenticeQuestionData->speciesId]);
        break;
    case APPRENTICE_BUFF_SPECIES2:
        StringCopy(stringDst, gSpeciesNames[gApprenticeQuestionData->altSpeciesId]);
        break;
    case APPRENTICE_BUFF_SPECIES3:
        StringCopy(stringDst, gSpeciesNames[gApprenticeQuestionData->speciesId]);
        break;
    case APPRENTICE_BUFF_MOVE1:
        StringCopy(stringDst, gMoveNames[gApprenticeQuestionData->moveId1]);
        break;
    case APPRENTICE_BUFF_MOVE2:
        StringCopy(stringDst, gMoveNames[gApprenticeQuestionData->moveId2]);
        break;
    case APPRENTICE_BUFF_ITEM:
        StringCopy(stringDst, ItemId_GetName(PLAYER_APPRENTICE.questions[CURRENT_QUESTION_NUM].data));
        break;
    case APPRENTICE_BUFF_NAME:
        TVShowConvertInternationalString(text, GetApprenticeNameInLanguage(PLAYER_APPRENTICE.id, GAME_LANGUAGE), GAME_LANGUAGE);
        StringCopy(stringDst, text);
        break;
    case APPRENTICE_BUFF_LEVEL:
        if (PLAYER_APPRENTICE.lvlMode == APPRENTICE_LVL_MODE_50)
            StringCopy(stringDst, gText_Lv50);
        else // == APPRENTICE_LVL_MODE_OPEN
            StringCopy(stringDst, gText_OpenLevel);
        break;
    case APPRENTICE_BUFF_WIN_SPEECH:
        FrontierSpeechToString(gSaveBlock2Ptr->apprentices[0].speechWon);
        StringCopy(stringDst, gStringVarBuffer);
        break;
    case APPRENTICE_BUFF_LEAD_MON_SPECIES:
        speciesArrayId = APPRENTICE_SPECIES_ID(PLAYER_APPRENTICE.leadMonId);
        StringCopy(stringDst, gSpeciesNames[gApprentices[PLAYER_APPRENTICE.id].species[speciesArrayId]]);
        break;
    }
}

static void SetLeadApprenticeMon(void)
{
    PLAYER_APPRENTICE.leadMonId = gSpecialVar_0x8005;
}

static void Script_ApprenticeOpenBagMenu(void)
{
    ApprenticeOpenBagMenu();
}

static void TrySetApprenticeHeldItem(void)
{
    u8 i, j;
    u8 count;

    if (PLAYER_APPRENTICE.questionsAnswered < NUM_WHICH_MON_QUESTIONS)
        return;

    count = 0;
    for (j = 0; j < APPRENTICE_MAX_QUESTIONS; j++)
    {
        if (PLAYER_APPRENTICE.questions[j].questionId == QUESTION_ID_WIN_SPEECH)
            break;
        count++;
    }

    // Make sure the item hasn't already been suggested in previous questions
    for (i = 0; i < count; i++)
    {
        if (i >= CURRENT_QUESTION_NUM)
            break;
        if (PLAYER_APPRENTICE.questions[i].questionId != QUESTION_ID_WHAT_ITEM ||
            PLAYER_APPRENTICE.questions[i].suggestedChange == 0)
            continue;
        if (PLAYER_APPRENTICE.questions[i].data == gSpecialVar_0x8005)
        {
            PLAYER_APPRENTICE.questions[CURRENT_QUESTION_NUM].suggestedChange = FALSE;
            PLAYER_APPRENTICE.questions[CURRENT_QUESTION_NUM].data = gSpecialVar_0x8005;
            gSpecialVar_Result = FALSE;
            return;
        }
    }

    PLAYER_APPRENTICE.questions[CURRENT_QUESTION_NUM].suggestedChange = TRUE;
    PLAYER_APPRENTICE.questions[CURRENT_QUESTION_NUM].data = gSpecialVar_0x8005;
    gSpecialVar_Result = TRUE;
}

static void ShiftSavedApprentices(void)
{
    s32 i;
    s32 apprenticeNum;
    s32 apprenticeIdx;

    if (gSaveBlock2Ptr->apprentices[0].playerName[0] == EOS)
        return;

    for (i = 0; i < APPRENTICE_COUNT - 1; i++)
    {
        if (gSaveBlock2Ptr->apprentices[i + 1].playerName[0] == EOS)
        {
            gSaveBlock2Ptr->apprentices[i + 1] = gSaveBlock2Ptr->apprentices[0];
            return;
        }
    }

    apprenticeNum = 0xFFFF;
    apprenticeIdx = -1;
    for (i = 1; i < APPRENTICE_COUNT; i++)
    {
        if (GetTrainerId(gSaveBlock2Ptr->apprentices[i].playerId) == GetTrainerId(gSaveBlock2Ptr->playerTrainerId)
            && gSaveBlock2Ptr->apprentices[i].number < apprenticeNum)
        {
            apprenticeNum = gSaveBlock2Ptr->apprentices[i].number;
            apprenticeIdx = i;
        }
    }

    if (apprenticeIdx > 0)
        gSaveBlock2Ptr->apprentices[apprenticeIdx] = gSaveBlock2Ptr->apprentices[0];
}

// Apprentice is always saved in the first slot. Pre-existing Apprentices are moved by ShiftSavedApprentices
static void SaveApprentice(void)
{
    u8 i;

    gSaveBlock2Ptr->apprentices[0].id = PLAYER_APPRENTICE.id;
    gSaveBlock2Ptr->apprentices[0].lvlMode = PLAYER_APPRENTICE.lvlMode;

    // Count questions asked until the final (win speech) question was reached
    for (i = 0; i < APPRENTICE_MAX_QUESTIONS && (PLAYER_APPRENTICE.questions[i].questionId != QUESTION_ID_WIN_SPEECH); i++)
        ;

    gSaveBlock2Ptr->apprentices[0].numQuestions = i;
    if (gSaveBlock2Ptr->apprentices[0].number < 255)
        gSaveBlock2Ptr->apprentices[0].number++;

    SaveApprenticeParty(gSaveBlock2Ptr->apprentices[0].numQuestions);
    for (i = 0; i < TRAINER_ID_LENGTH; i++)
        gSaveBlock2Ptr->apprentices[0].playerId[i] = gSaveBlock2Ptr->playerTrainerId[i];

    StringCopy(gSaveBlock2Ptr->apprentices[0].playerName, gSaveBlock2Ptr->playerName);
    gSaveBlock2Ptr->apprentices[0].language = gGameLanguage;
    CalcApprenticeChecksum(&gSaveBlock2Ptr->apprentices[0]);
}

// Never called, APPRENTICE_FUNC_SET_GFX_SAVED is unused
static void SetSavedApprenticeTrainerGfxId(void)
{
    u8 i;
    u8 objectEventGfxId;
    u8 class = gApprentices[gSaveBlock2Ptr->apprentices[0].id].facilityClass;

    for (i = 0; i < ARRAY_COUNT(gTowerMaleFacilityClasses) && gTowerMaleFacilityClasses[i] != class; i++)
        ;
    if (i != ARRAY_COUNT(gTowerMaleFacilityClasses))
    {
        objectEventGfxId = gTowerMaleTrainerGfxIds[i];
        VarSet(VAR_OBJ_GFX_ID_0, objectEventGfxId);
        return;
    }

    for (i = 0; i < ARRAY_COUNT(gTowerFemaleFacilityClasses) && gTowerFemaleFacilityClasses[i] != class; i++)
        ;
    if (i != ARRAY_COUNT(gTowerFemaleFacilityClasses))
    {
        objectEventGfxId = gTowerFemaleTrainerGfxIds[i];
        VarSet(VAR_OBJ_GFX_ID_0, objectEventGfxId);
    }
}

static void SetPlayerApprenticeTrainerGfxId(void)
{
    u8 i;
    u8 objectEventGfxId;
    u8 class = gApprentices[PLAYER_APPRENTICE.id].facilityClass;

    for (i = 0; i < ARRAY_COUNT(gTowerMaleFacilityClasses) && gTowerMaleFacilityClasses[i] != class; i++)
        ;
    if (i != ARRAY_COUNT(gTowerMaleFacilityClasses))
    {
        objectEventGfxId = gTowerMaleTrainerGfxIds[i];
        VarSet(VAR_OBJ_GFX_ID_0, objectEventGfxId);
        return;
    }

    for (i = 0; i < ARRAY_COUNT(gTowerFemaleFacilityClasses) && gTowerFemaleFacilityClasses[i] != class; i++)
        ;
    if (i != ARRAY_COUNT(gTowerFemaleFacilityClasses))
    {
        objectEventGfxId = gTowerFemaleTrainerGfxIds[i];
        VarSet(VAR_OBJ_GFX_ID_0, objectEventGfxId);
    }
}

// Both of the below functions may have been dummied / used for debug
// In all cases theres a conditional for VAR_0x8004 right after the call to these functions
static void GetShouldCheckApprenticeGone(void)
{
    gSpecialVar_0x8004 = TRUE;
}

static void GetShouldApprenticeLeave(void)
{
    gSpecialVar_0x8004 = TRUE;
}

const u8 *GetApprenticeNameInLanguage(u32 apprenticeId, s32 language)
{
    const struct ApprenticeTrainer *apprentice = &gApprentices[apprenticeId];

    switch (language)
    {
    case LANGUAGE_JAPANESE:
        return apprentice->name[0];
    case LANGUAGE_ENGLISH:
        return apprentice->name[1];
    case LANGUAGE_FRENCH:
        return apprentice->name[2];
    case LANGUAGE_ITALIAN:
        return apprentice->name[3];
    case LANGUAGE_GERMAN:
        return apprentice->name[4];
    case LANGUAGE_SPANISH:
    default:
        return apprentice->name[5];
    }
}

static void UNUSED Task_SwitchToFollowupFuncAfterButtonPress(u8 taskId)
{
    if (JOY_NEW(A_BUTTON) || JOY_NEW(B_BUTTON))
        SwitchTaskToFollowupFunc(taskId);
}

static void Task_ExecuteFuncAfterButtonPress(u8 taskId)
{
    if (JOY_NEW(A_BUTTON) || JOY_NEW(B_BUTTON))
    {
        gApprenticeFunc = (void *)(u32)(((u16)gTasks[taskId].data[0] | (gTasks[taskId].data[1] << 16)));
        gApprenticeFunc();
        DestroyTask(taskId);
    }
}

static void ExecuteFuncAfterButtonPress(void (*func)(void))
{
    u8 taskId = CreateTask(Task_ExecuteFuncAfterButtonPress, 1);
    gTasks[taskId].data[0] = (u32)(func);
    gTasks[taskId].data[1] = (u32)(func) >> 16;
}

static void UNUSED ExecuteFollowupFuncAfterButtonPress(TaskFunc task)
{
    u8 taskId = CreateTask(Task_SwitchToFollowupFuncAfterButtonPress, 1);
    SetTaskFuncWithFollowupFunc(taskId, Task_SwitchToFollowupFuncAfterButtonPress, task);
}
