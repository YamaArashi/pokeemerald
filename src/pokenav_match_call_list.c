#include "global.h"
#include "battle_setup.h"
#include "data.h"
#include "event_data.h"
#include "gym_leader_rematch.h"
#include "international_string_util.h"
#include "main.h"
#include "match_call.h"
#include "overworld.h"
#include "pokemon.h"
#include "pokenav.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "constants/songs.h"

struct Pokenav_MatchCallMenu
{
    u16 optionCursorPos;
    u16 maxOptionId;
    const u8 *matchCallOptions;
    u16 headerId;
    u16 numRegistered;
    u16 numSpecialTrainers;
    bool32 initFinished;
    u32 loopedTaskId;
    u32 (*callback)(struct Pokenav_MatchCallMenu *);
    struct PokenavMatchCallEntry matchCallEntries[MAX_REMATCH_ENTRIES - 1];
};

static u32 CB2_HandleMatchCallInput(struct Pokenav_MatchCallMenu *);
static u32 GetExitMatchCallMenuId(struct Pokenav_MatchCallMenu *);
static u32 CB2_HandleMatchCallOptionsInput(struct Pokenav_MatchCallMenu *);
static u32 CB2_HandleCheckPageInput(struct Pokenav_MatchCallMenu *);
static u32 CB2_HandleCallExitInput(struct Pokenav_MatchCallMenu *);
static u32 LoopedTask_BuildMatchCallList(s32);
static bool32 ShouldDoNearbyMessage(void);

#include "data/text/match_call_messages.h"

static const u8 sMatchCallOptionsNoCheckPage[] =
{
    MATCH_CALL_OPTION_CALL,
    MATCH_CALL_OPTION_CANCEL
};

static const u8 sMatchCallOptionsHasCheckPage[] =
{
    MATCH_CALL_OPTION_CALL,
    MATCH_CALL_OPTION_CHECK,
    MATCH_CALL_OPTION_CANCEL
};

bool32 PokenavCallback_Init_MatchCall(void)
{
    struct Pokenav_MatchCallMenu *state = AllocSubstruct(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN, sizeof(struct Pokenav_MatchCallMenu));
    if (!state)
        return FALSE;

    state->callback = CB2_HandleMatchCallInput;
    state->headerId = 0;
    state->initFinished = FALSE;
    state->loopedTaskId = CreateLoopedTask(LoopedTask_BuildMatchCallList, 1);
    return TRUE;
}

u32 GetMatchCallCallback(void)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    return state->callback(state);
}

void FreeMatchCallSubstruct1(void)
{
    FreePokenavSubstruct(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
}

static u32 CB2_HandleMatchCallInput(struct Pokenav_MatchCallMenu *state)
{
    int selection;

    if (JOY_REPEAT(DPAD_UP))
        return POKENAV_MC_FUNC_UP;
    if (JOY_REPEAT(DPAD_DOWN))
        return POKENAV_MC_FUNC_DOWN;
    if (JOY_REPEAT(DPAD_LEFT))
        return POKENAV_MC_FUNC_PG_UP;
    if (JOY_REPEAT(DPAD_RIGHT))
        return POKENAV_MC_FUNC_PG_DOWN;

    if (JOY_NEW(A_BUTTON))
    {
        state->callback = CB2_HandleMatchCallOptionsInput;
        state->optionCursorPos = 0;
        selection = PokenavList_GetSelectedIndex();

        if (!state->matchCallEntries[selection].isSpecialTrainer || MatchCall_HasCheckPage(state->matchCallEntries[selection].headerId))
        {
            state->matchCallOptions = sMatchCallOptionsHasCheckPage;
            state->maxOptionId = ARRAY_COUNT(sMatchCallOptionsHasCheckPage) - 1;
        }
        else
        {
            state->matchCallOptions = sMatchCallOptionsNoCheckPage;
            state->maxOptionId = ARRAY_COUNT(sMatchCallOptionsNoCheckPage) - 1;
        }

        return POKENAV_MC_FUNC_SELECT;
    }

    if (JOY_NEW(B_BUTTON))
    {
        if (GetPokenavMode() != POKENAV_MODE_FORCE_CALL_READY)
        {
            state->callback = GetExitMatchCallMenuId;
            return POKENAV_MC_FUNC_EXIT;
        }
        else
        {
            // Cant exit Match Call menu before calling Mr Stone during tutorial
            PlaySE(SE_FAILURE);
        }
    }

    return POKENAV_MC_FUNC_NONE;
}

static u32 GetExitMatchCallMenuId(struct Pokenav_MatchCallMenu *state)
{
    return POKENAV_MAIN_MENU_CURSOR_ON_MATCH_CALL;
}

static u32 CB2_HandleMatchCallOptionsInput(struct Pokenav_MatchCallMenu *state)
{
    if (JOY_NEW(DPAD_UP) && state->optionCursorPos)
    {
        state->optionCursorPos--;
        return POKENAV_MC_FUNC_MOVE_OPTIONS_CURSOR;
    }

    if (JOY_NEW(DPAD_DOWN) && state->optionCursorPos < state->maxOptionId)
    {
        state->optionCursorPos++;
        return POKENAV_MC_FUNC_MOVE_OPTIONS_CURSOR;
    }

    if (JOY_NEW(A_BUTTON))
    {
        switch (state->matchCallOptions[state->optionCursorPos])
        {
        case MATCH_CALL_OPTION_CANCEL:
            state->callback = CB2_HandleMatchCallInput;
            return POKENAV_MC_FUNC_CANCEL;
        case MATCH_CALL_OPTION_CALL:
            if (GetPokenavMode() == POKENAV_MODE_FORCE_CALL_READY)
                SetPokenavMode(POKENAV_MODE_FORCE_CALL_EXIT);

            state->callback = CB2_HandleCallExitInput;
            if (ShouldDoNearbyMessage())
                return POKENAV_MC_FUNC_NEARBY_MSG;

            return POKENAV_MC_FUNC_CALL_MSG;
        case MATCH_CALL_OPTION_CHECK:
            state->callback = CB2_HandleCheckPageInput;
            return POKENAV_MC_FUNC_SHOW_CHECK_PAGE;
        }
    }

    if (JOY_NEW(B_BUTTON))
    {
        state->callback = CB2_HandleMatchCallInput;
        return POKENAV_MC_FUNC_CANCEL;
    }

    return POKENAV_MC_FUNC_NONE;
}

static u32 CB2_HandleCheckPageInput(struct Pokenav_MatchCallMenu *state)
{
    if (JOY_REPEAT(DPAD_UP))
        return POKENAV_MC_FUNC_CHECK_PAGE_UP;
    if (JOY_REPEAT(DPAD_DOWN))
        return POKENAV_MC_FUNC_CHECK_PAGE_DOWN;

    if (JOY_NEW(B_BUTTON))
    {
        state->callback = CB2_HandleMatchCallInput;
        return POKENAV_MC_FUNC_EXIT_CHECK_PAGE;
    }

    return POKENAV_MC_FUNC_NONE;
}

static u32 CB2_HandleCallExitInput(struct Pokenav_MatchCallMenu *state)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        state->callback = CB2_HandleMatchCallInput;
        return POKENAV_MC_FUNC_EXIT_CALL;
    }

    return POKENAV_MC_FUNC_NONE;
}

static u32 LoopedTask_BuildMatchCallList(s32 taskState)
{
    int i, j;
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    switch (taskState)
    {
    case 0:
        state->headerId = 0;
        state->numRegistered = 0;
        return LT_INC_AND_CONTINUE;
    case 1:
        // Load special trainers (e.g. Rival, gym leaders)
        for (i = 0, j = state->headerId; i < 30; i++, j++)
        {
            if (MatchCall_GetEnabled(j))
            {
                state->matchCallEntries[state->numRegistered].headerId = j;
                state->matchCallEntries[state->numRegistered].isSpecialTrainer = TRUE;
                state->matchCallEntries[state->numRegistered].mapSec = MatchCall_GetMapSec(j);
                state->numRegistered++;
            }

            if (++state->headerId >= MC_HEADER_COUNT)
            {
                state->numSpecialTrainers = state->headerId;
                state->headerId = 0;
                return LT_INC_AND_CONTINUE;
            }
        }

        return LT_CONTINUE;
    case 2:
        // Load normal trainers
        for (i = 0, j = state->headerId; i < 30; i++, j++)
        {
            if (!MatchCall_HasRematchId(state->headerId) && IsRematchEntryRegistered(state->headerId))
            {
                state->matchCallEntries[state->numRegistered].headerId = state->headerId;
                state->matchCallEntries[state->numRegistered].isSpecialTrainer = FALSE;
                state->matchCallEntries[state->numRegistered].mapSec = GetMatchTableMapSectionId(j);
                state->numRegistered++;
            }

            if (++state->headerId > REMATCH_TABLE_ENTRIES - 1)
                return LT_INC_AND_CONTINUE;
        }

        return LT_CONTINUE;
    case 3:
        state->initFinished = TRUE;
        break;
    }

    return LT_FINISH;
}

bool32 IsRematchEntryRegistered(int rematchIndex)
{
    if (rematchIndex < REMATCH_TABLE_ENTRIES)
        return FlagGet(TRAINER_REGISTERED_FLAGS_START + rematchIndex);

    return FALSE;
}

int IsMatchCallListInitFinished(void)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    return state->initFinished;
}

int GetNumberRegistered(void)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    return state->numRegistered;
}

static int UNUSED GetNumSpecialTrainers(void)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    return state->numSpecialTrainers;
}

static int UNUSED GetNumNormalTrainers(void)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    return state->numRegistered - state->numSpecialTrainers;
}

static int UNUSED GetNormalTrainerHeaderId(int index)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    index += state->numSpecialTrainers;
    if (index >= state->numRegistered)
        return REMATCH_TABLE_ENTRIES;

    return state->matchCallEntries[index].headerId;
}

struct PokenavMatchCallEntry *GetMatchCallList(void)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    return state->matchCallEntries;
}

u16 GetMatchCallMapSec(int index)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    return state->matchCallEntries[index].mapSec;
}

bool32 ShouldDrawRematchPokeballIcon(int index)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    if (!state->matchCallEntries[index].isSpecialTrainer)
        index = state->matchCallEntries[index].headerId;
    else
        index = MatchCall_GetRematchTableIdx(state->matchCallEntries[index].headerId);

    if (index == REMATCH_TABLE_ENTRIES)
        return FALSE;

    return gSaveBlock1Ptr->trainerRematches[index] != 0;
}

int GetMatchCallTrainerPic(int index)
{
    int headerId;
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    if (!state->matchCallEntries[index].isSpecialTrainer)
    {
        index = GetTrainerIdxByRematchIdx(state->matchCallEntries[index].headerId);
        return gTrainers[index].trainerPic;
    }

    headerId = state->matchCallEntries[index].headerId;
    index = MatchCall_GetRematchTableIdx(headerId);
    if (index != REMATCH_TABLE_ENTRIES)
    {
        index = GetTrainerIdxByRematchIdx(index);
        return gTrainers[index].trainerPic;
    }

    index = MatchCall_GetOverrideFacilityClass(headerId);
    return gFacilityClassToPicIndex[index];
}

const u8 *GetMatchCallMessageText(int index, bool8 *newRematchRequest)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    *newRematchRequest = FALSE;
    if (!Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType))
        return gText_CallCantBeMadeHere;

    if (!state->matchCallEntries[index].isSpecialTrainer)
        *newRematchRequest = SelectMatchCallMessage(GetTrainerIdxByRematchIdx(state->matchCallEntries[index].headerId), gStringVarBuffer);
    else
        MatchCall_GetMessage(state->matchCallEntries[index].headerId, gStringVarBuffer);

    return gStringVarBuffer;
}

const u8 *GetMatchCallFlavorText(int index, int checkPageEntry)
{
    int rematchId;
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    if (state->matchCallEntries[index].isSpecialTrainer)
    {
        rematchId = MatchCall_GetRematchTableIdx(state->matchCallEntries[index].headerId);
        if (rematchId == REMATCH_TABLE_ENTRIES)
            return MatchCall_GetOverrideFlavorText(state->matchCallEntries[index].headerId, checkPageEntry);
    }
    else
    {
        rematchId = state->matchCallEntries[index].headerId;
    }

    return gMatchCallFlavorTexts[rematchId][checkPageEntry];
}

u16 GetMatchCallOptionCursorPos(void)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    return state->optionCursorPos;
}

u16 GetMatchCallOptionId(int optionId)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    if (state->maxOptionId < optionId)
        return MATCH_CALL_OPTION_COUNT;

    return state->matchCallOptions[optionId];
}

void BufferMatchCallNameAndDesc(struct PokenavMatchCallEntry *matchCallEntry, u8 *str)
{
    const u8 *trainerName;
    const u8 *className;
    if (!matchCallEntry->isSpecialTrainer)
    {
        int index = GetTrainerIdxByRematchIdx(matchCallEntry->headerId);
        const struct Trainer *trainer = &gTrainers[index];
        int class = trainer->trainerClass;
        className = gTrainerClassNames[class];
        trainerName = trainer->trainerName;
    }
    else
    {
        MatchCall_GetNameAndDesc(matchCallEntry->headerId, &className, &trainerName);
    }

    if (className && trainerName)
    {
        u8 *str2 = GetStringClearToWidth(str, FONT_NARROW, className, 69);
        GetStringClearToWidth(str2, FONT_NARROW, trainerName, 51);
    }
    else
    {
        GetStringClearToWidth(str, FONT_NARROW, NULL, 120);
    }
}

u8 GetMatchTableMapSectionId(int rematchIndex)
{
    int mapGroup = gRematchTable[rematchIndex].mapGroup;
    int mapNum = gRematchTable[rematchIndex].mapNum;
    return Overworld_GetMapHeaderByGroupAndId(mapGroup, mapNum)->regionMapSectionId;
}

int GetIndexDeltaOfNextCheckPageDown(int index)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    int count = 1;
    while (++index < state->numRegistered)
    {
        if (!state->matchCallEntries[index].isSpecialTrainer)
            return count;
        if (MatchCall_HasCheckPage(state->matchCallEntries[index].headerId))
            return count;

        count++;
    }

    return 0;
}

int GetIndexDeltaOfNextCheckPageUp(int index)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    int count = -1;
    while (--index >= 0)
    {
        if (!state->matchCallEntries[index].isSpecialTrainer)
            return count;
        if (MatchCall_HasCheckPage(state->matchCallEntries[index].headerId))
            return count;

        count--;
    }

    return 0;
}

static bool32 UNUSED HasRematchEntry(void)
{
    int i;

    for (i = 0; i < REMATCH_TABLE_ENTRIES; i++)
    {
        if (IsRematchEntryRegistered(i) && gSaveBlock1Ptr->trainerRematches[i])
            return TRUE;
    }

    for (i = 0; i < MC_HEADER_COUNT; i++)
    {
        if (MatchCall_GetEnabled(i))
        {
            int index = MatchCall_GetRematchTableIdx(i);
            if (gSaveBlock1Ptr->trainerRematches[index])
                return TRUE;
        }
    }

    return FALSE;
}

static bool32 ShouldDoNearbyMessage(void)
{
    struct Pokenav_MatchCallMenu *state = GetSubstructPtr(POKENAV_SUBSTRUCT_MATCH_CALL_MAIN);
    int selection = PokenavList_GetSelectedIndex();
    if (!state->matchCallEntries[selection].isSpecialTrainer)
    {
        if (GetMatchCallMapSec(selection) == gMapHeader.regionMapSectionId)
        {
            if (!gSaveBlock1Ptr->trainerRematches[state->matchCallEntries[selection].headerId])
                return TRUE;
        }
    }
    else
    {
        if (state->matchCallEntries[selection].headerId == MC_HEADER_WATTSON)
        {
            if (GetMatchCallMapSec(selection) == gMapHeader.regionMapSectionId
             && FlagGet(FLAG_BADGE05_GET) == TRUE)
            {
                if (!FlagGet(FLAG_WATTSON_REMATCH_AVAILABLE))
                    return TRUE;
            }
        }
    }

    return FALSE;
}
