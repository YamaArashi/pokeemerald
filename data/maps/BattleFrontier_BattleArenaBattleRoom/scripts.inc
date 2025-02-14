.set LOCALID_BLACK_BELT_1, 1
.set LOCALID_BLACK_BELT_2, 2
.set LOCALID_BLACK_BELT_3, 3
.set LOCALID_BLACK_BELT_4, 4
.set LOCALID_ATTENDANT, 5
.set LOCALID_OPPONENT, 7
.set LOCALID_PLAYER, 8
.set LOCALID_ANNOUNCER, 9

BattleFrontier_BattleArenaBattleRoom_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, BattleFrontier_BattleArenaBattleRoom_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, BattleFrontier_BattleArenaBattleRoom_OnFrame
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, BattleFrontier_BattleArenaBattleRoom_OnWarp
	map_script MAP_SCRIPT_ON_RESUME, BattleFrontier_BattleArenaBattleRoom_OnResume
	.byte 0

	@ On this map the player (OBJ_EVENT_ID_PLAYER) is hidden
	@ The player is represented instead by LOCALID_PLAYER, which has the gfx id VAR_OBJ_GFX_ID_1

BattleFrontier_BattleArenaBattleRoom_OnResume:
	special OffsetCameraForBattle
	end

BattleFrontier_BattleArenaBattleRoom_OnTransition:
	frontier_settrainers
	call BattleFrontier_BattleArenaBattleRoom_EventScript_SetPlayerGfx
	end

BattleFrontier_BattleArenaBattleRoom_EventScript_SetPlayerGfx::
	checkplayergender
	goto_if_eq VAR_RESULT, MALE, BattleFrontier_BattleArenaBattleRoom_EventScript_SetPlayerGfxMale
	goto_if_eq VAR_RESULT, FEMALE, BattleFrontier_BattleArenaBattleRoom_EventScript_SetPlayerGfxFemale
	return

BattleFrontier_BattleArenaBattleRoom_EventScript_SetPlayerGfxMale::
	setvar VAR_OBJ_GFX_ID_1, OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL
	return

BattleFrontier_BattleArenaBattleRoom_EventScript_SetPlayerGfxFemale::
	setvar VAR_OBJ_GFX_ID_1, OBJ_EVENT_GFX_RIVAL_MAY_NORMAL
	return

BattleFrontier_BattleArenaBattleRoom_OnFrame:
	map_script_2 VAR_TEMP_0, 0, BattleFrontier_BattleArenaBattleRoom_EventScript_EnterRoom
	.2byte 0

BattleFrontier_BattleArenaBattleRoom_EventScript_EnterRoom::
	lockall
	showobjectat LOCALID_PLAYER, MAP_BATTLE_FRONTIER_BATTLE_ARENA_BATTLE_ROOM
	applymovement LOCALID_PLAYER, BattleFrontier_BattleArenaBattleRoom_Movement_PlayerEnter
	waitmovement 0
	frontier_get FRONTIER_DATA_BATTLE_NUM
	goto_if_eq VAR_RESULT, 0, BattleFrontier_BattleArenaBattleRoom_EventScript_AnnounceTrainers
	applymovement LOCALID_ATTENDANT, BattleFrontier_BattleArenaBattleRoom_Movement_WalkInPlaceDown
	applymovement LOCALID_PLAYER, BattleFrontier_BattleArenaBattleRoom_Movement_WalkInPlaceLeft
	setvar VAR_TEMP_2, 1
	frontier_set FRONTIER_DATA_RECORD_DISABLED, TRUE
	goto BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForOpponent

BattleFrontier_BattleArenaBattleRoom_EventScript_AnnounceTrainers::
	tower_setopponent
	addobject LOCALID_OPPONENT
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleArenaBattleRoom_Movement_OpponentEnter
	waitmovement 0
	applymovement LOCALID_ANNOUNCER, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceDown
	playse SE_M_BELLY_DRUM
	waitse
	waitmovement 0
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_PlayerStepForward, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PLAYER, BattleFrontier_BattleArenaBattleRoom_Movement_PlayerStepForward
	waitmovement 0
	applymovement LOCALID_ANNOUNCER, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceDown
	playse SE_M_BELLY_DRUM
	waitse
	waitmovement 0
	arena_gettrainername
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_OpponentStepForward, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleArenaBattleRoom_Movement_OpponentStepForward
	waitmovement 0
	applymovement LOCALID_ANNOUNCER, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceDown
	playse SE_M_BELLY_DRUM
	waitse
	waitmovement 0
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_SetKOTourneyBegin, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PLAYER, BattleFrontier_BattleArenaBattleRoom_Movement_PlayerStepForward
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleArenaBattleRoom_Movement_OpponentStepForward
	waitmovement 0
	palace_getopponentintro
	msgbox gStringVarBuffer, MSGBOX_DEFAULT
	waitmessage
	call BattleFrontier_BattleArenaBattleRoom_EventScript_DoArenaBattle
	switch VAR_RESULT
	case 1, BattleFrontier_BattleArenaBattleRoom_EventScript_DefeatedOpponent
BattleFrontier_BattleArenaBattleRoom_EventScript_DeclareOpponentWinner::
	applymovement LOCALID_ANNOUNCER, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceDown
	applymovement LOCALID_BLACK_BELT_1, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceDown
	applymovement LOCALID_BLACK_BELT_2, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceDown
	applymovement LOCALID_BLACK_BELT_3, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceUp
	applymovement LOCALID_BLACK_BELT_4, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceUp
	playse SE_M_SNORE
	waitse
	waitmovement 0
	arena_gettrainername
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_WinnerIsOpponent, MSGBOX_DEFAULT
BattleFrontier_BattleArenaBattleRoom_EventScript_WarpToLobbyLost::
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, CHALLENGE_STATUS_LOST
	warp MAP_BATTLE_FRONTIER_BATTLE_ARENA_LOBBY, 7, 8
	waitstate

BattleFrontier_BattleArenaBattleRoom_EventScript_DefeatedOpponent::
	call BattleFrontier_BattleArenaBattleRoom_EventScript_DeclarePlayerWinner
	frontier_get FRONTIER_DATA_BATTLE_NUM
	addvar VAR_RESULT, 1
	frontier_set FRONTIER_DATA_BATTLE_NUM, VAR_RESULT
	switch VAR_RESULT
	case 7, BattleFrontier_BattleArenaBattleRoom_EventScript_ReturnToLobbyWon
	applymovement LOCALID_PLAYER, BattleFrontier_BattleArenaBattleRoom_Movement_PlayerWalkBackToLine
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleArenaBattleRoom_Movement_OpponentExit
	waitmovement 0
	removeobject LOCALID_OPPONENT
	applymovement LOCALID_ATTENDANT, BattleFrontier_BattleArenaBattleRoom_Movement_WalkInPlaceDown
	applymovement LOCALID_PLAYER, BattleFrontier_BattleArenaBattleRoom_Movement_WalkInPlaceLeft
	waitmovement 0
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_MonsWillBeRestored, MSGBOX_DEFAULT
	special LoadPlayerParty
	special SavePlayerParty
	frontier_setpartyorder FRONTIER_PARTY_SIZE
	playfanfare MUS_HEAL
	waitfanfare
	special HealPlayerParty
BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForOpponent::
	frontier_getbrainstatus
	copyvar VAR_TEMP_F, VAR_RESULT
	goto_if_ne VAR_RESULT, FRONTIER_BRAIN_NOT_READY, BattleFrontier_BattleArenaBattleRoom_EventScript_TycoonUpNext
	frontier_get FRONTIER_DATA_BATTLE_NUM
	call_if_eq VAR_RESULT, 1, BattleFrontier_BattleArenaBattleRoom_EventScript_ReadyFor2ndOpponent
	call_if_eq VAR_RESULT, 2, BattleFrontier_BattleArenaBattleRoom_EventScript_ReadyFor3rdOpponent
	call_if_eq VAR_RESULT, 3, BattleFrontier_BattleArenaBattleRoom_EventScript_ReadyFor4thOpponent
	call_if_eq VAR_RESULT, 4, BattleFrontier_BattleArenaBattleRoom_EventScript_ReadyFor5thOpponent
	call_if_eq VAR_RESULT, 5, BattleFrontier_BattleArenaBattleRoom_EventScript_ReadyFor6thOpponent
	call_if_eq VAR_RESULT, 6, BattleFrontier_BattleArenaBattleRoom_EventScript_ReadyFor7thOpponent
	call BattleFrontier_EventScript_GetCantRecordBattle
	goto_if_eq VAR_RESULT, TRUE, BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForOpponentNoRecord
	multichoice 19, 4, MULTI_GO_ON_RECORD_REST_RETIRE, TRUE
	switch VAR_RESULT
	case 0, BattleFrontier_BattleArenaBattleRoom_EventScript_ContinueChallenge
	case 1, BattleFrontier_BattleArenaBattleRoom_EventScript_AskRecordBattle
	case 2, BattleFrontier_BattleArenaBattleRoom_EventScript_AskPauseChallenge
	case 3, BattleFrontier_BattleArenaBattleRoom_EventScript_AskRetireChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForOpponent

BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForOpponentNoRecord::
	multichoice 20, 6, MULTI_GO_ON_REST_RETIRE, TRUE
	switch VAR_RESULT
	case 0, BattleFrontier_BattleArenaBattleRoom_EventScript_ContinueChallenge
	case 1, BattleFrontier_BattleArenaBattleRoom_EventScript_AskPauseChallenge
	case 2, BattleFrontier_BattleArenaBattleRoom_EventScript_AskRetireChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForOpponent

BattleFrontier_BattleArenaBattleRoom_EventScript_AskRecordBattle::
	message BattleFrontier_BattleArenaBattleRoom_Text_RecordLastBattle
	waitmessage
	multichoicedefault 20, 8, MULTI_YESNO, 1, FALSE
	switch VAR_RESULT
	case 1, BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForOpponent
	case 0, BattleFrontier_BattleArenaBattleRoom_EventScript_RecordBattle
	case MULTI_B_PRESSED, BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForOpponent

BattleFrontier_BattleArenaBattleRoom_EventScript_RecordBattle::
	call BattleFrontier_EventScript_SaveBattle
	goto BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForOpponent

BattleFrontier_BattleArenaBattleRoom_EventScript_AskPauseChallenge::
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_SaveAndShutDown, MSGBOX_YESNO
	switch VAR_RESULT
	case NO, BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForOpponent
	case YES, BattleFrontier_BattleArenaBattleRoom_EventScript_PauseChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForOpponent

BattleFrontier_BattleArenaBattleRoom_EventScript_AskRetireChallenge::
	message BattleFrontier_BattleArenaBattleRoom_Text_RetireFromChallenge
	waitmessage
	multichoicedefault 20, 8, MULTI_YESNO, 1, FALSE
	switch VAR_RESULT
	case 1, BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForOpponent
	case 0, BattleFrontier_BattleArenaBattleRoom_EventScript_WarpToLobbyLost
	case MULTI_B_PRESSED, BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForOpponent

BattleFrontier_BattleArenaBattleRoom_EventScript_ContinueChallenge::
	closemessage
	applymovement LOCALID_PLAYER, BattleFrontier_BattleArenaBattleRoom_Movement_WalkInPlaceRight
	applymovement LOCALID_ATTENDANT, BattleFrontier_BattleArenaBattleRoom_Movement_WalkInPlaceRight
	waitmovement 0
	goto BattleFrontier_BattleArenaBattleRoom_EventScript_AnnounceTrainers
	waitstate
BattleFrontier_BattleArenaBattleRoom_EventScript_ReturnToLobbyWon::
	delay 60
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, CHALLENGE_STATUS_WON
	warp MAP_BATTLE_FRONTIER_BATTLE_ARENA_LOBBY, 7, 8
	waitstate

BattleFrontier_BattleArenaBattleRoom_EventScript_ReadyFor2ndOpponent::
	message BattleFrontier_BattleArenaBattleRoom_Text_ReadyFor2ndOpponent
	waitmessage
	return

BattleFrontier_BattleArenaBattleRoom_EventScript_ReadyFor3rdOpponent::
	message BattleFrontier_BattleArenaBattleRoom_Text_ReadyFor3rdOpponent
	waitmessage
	return

BattleFrontier_BattleArenaBattleRoom_EventScript_ReadyFor4thOpponent::
	message BattleFrontier_BattleArenaBattleRoom_Text_ReadyFor4thOpponent
	waitmessage
	return

BattleFrontier_BattleArenaBattleRoom_EventScript_ReadyFor5thOpponent::
	message BattleFrontier_BattleArenaBattleRoom_Text_ReadyFor5thOpponent
	waitmessage
	return

BattleFrontier_BattleArenaBattleRoom_EventScript_ReadyFor6thOpponent::
	message BattleFrontier_BattleArenaBattleRoom_Text_ReadyFor6thOpponent
	waitmessage
	return

BattleFrontier_BattleArenaBattleRoom_EventScript_ReadyFor7thOpponent::
	message BattleFrontier_BattleArenaBattleRoom_Text_ReadyFor7thOpponent
	waitmessage
	return

BattleFrontier_BattleArenaBattleRoom_EventScript_PauseChallenge::
	message BattleFrontier_BattleArenaBattleRoom_Text_SavingPleaseWait
	waitmessage
	arena_save CHALLENGE_STATUS_PAUSED
	playse SE_SAVE
	waitse
	fadescreen FADE_TO_BLACK
	frontier_reset
	end

BattleFrontier_BattleArenaBattleRoom_EventScript_TycoonUpNext::
	goto_if_eq VAR_TEMP_2, 1, BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForTycoon
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_NowFaceTycoon, MSGBOX_DEFAULT
	setvar VAR_TEMP_2, 1
BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForTycoon::
	message BattleFrontier_BattleArenaBattleRoom_Text_PreparedForTycoon
	waitmessage
	call BattleFrontier_EventScript_GetCantRecordBattle
	goto_if_eq VAR_RESULT, TRUE, BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForTycoonNoRecord
	multichoice 19, 4, MULTI_GO_ON_RECORD_REST_RETIRE, TRUE
	switch VAR_RESULT
	case 0, BattleFrontier_BattleArenaBattleRoom_EventScript_BattleGreta
	case 1, BattleFrontier_BattleArenaBattleRoom_EventScript_AskRecordBattle
	case 2, BattleFrontier_BattleArenaBattleRoom_EventScript_AskPauseChallenge
	case 3, BattleFrontier_BattleArenaBattleRoom_EventScript_AskRetireChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForTycoon

BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForTycoonNoRecord::
	multichoice 20, 6, MULTI_GO_ON_REST_RETIRE, TRUE
	switch VAR_RESULT
	case 0, BattleFrontier_BattleArenaBattleRoom_EventScript_BattleGreta
	case 1, BattleFrontier_BattleArenaBattleRoom_EventScript_AskPauseChallenge
	case 2, BattleFrontier_BattleArenaBattleRoom_EventScript_AskRetireChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattleArenaBattleRoom_EventScript_AskReadyForTycoon

BattleFrontier_BattleArenaBattleRoom_EventScript_BattleGreta::
	call BattleFrontier_EventScript_SetBrainObjectGfx
	applymovement LOCALID_PLAYER, BattleFrontier_BattleArenaBattleRoom_Movement_WalkInPlaceRight
	applymovement LOCALID_ATTENDANT, BattleFrontier_BattleArenaBattleRoom_Movement_WalkInPlaceRight
	waitmovement 0
	applymovement LOCALID_ANNOUNCER, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceDown
	playse SE_M_BELLY_DRUM
	waitse
	waitmovement 0
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_PlayerStepForward, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PLAYER, BattleFrontier_BattleArenaBattleRoom_Movement_PlayerStepForwardLong
	waitmovement 0
	applymovement LOCALID_ANNOUNCER, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceDown
	playse SE_M_BELLY_DRUM
	waitse
	waitmovement 0
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_MakeWayForGreta, MSGBOX_DEFAULT
	closemessage
	addobject LOCALID_OPPONENT
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleArenaBattleRoom_Movement_GretaEnter
	waitmovement 0
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleArenaBattleRoom_Movement_OpponentStepForward
	waitmovement 0
	switch VAR_TEMP_F
	case FRONTIER_BRAIN_GOLD, BattleFrontier_BattleArenaBattleRoom_EventScript_IntroGretaGold
	case FRONTIER_BRAIN_STREAK, BattleFrontier_BattleArenaBattleRoom_EventScript_BattleGretaSilver
	case FRONTIER_BRAIN_STREAK_LONG, BattleFrontier_BattleArenaBattleRoom_EventScript_BattleGretaGold
	frontier_get FRONTIER_DATA_HEARD_BRAIN_SPEECH
	goto_if_ne VAR_RESULT, FALSE, BattleFrontier_BattleArenaBattleRoom_EventScript_BattleGretaSilver
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_GretaYoureChallenger, MSGBOX_DEFAULT
	closemessage
	frontier_set FRONTIER_DATA_HEARD_BRAIN_SPEECH
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleArenaBattleRoom_Movement_GretaLookAroundPlayer
	waitmovement 0
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_IsThatRight, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleArenaBattleRoom_Movement_GretaWalkBackToCenter
	waitmovement 0
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_YouLookWeakTakeThingsEasy, MSGBOX_DEFAULT
BattleFrontier_BattleArenaBattleRoom_EventScript_BattleGretaSilver::
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_IgniteMyPassionForBattle, MSGBOX_DEFAULT
	call BattleFrontier_BattleArenaBattleRoom_EventScript_StartArenaBattle
	goto_if_eq VAR_RESULT, 1, BattleFrontier_BattleArenaBattleRoom_EventScript_DefeatedGretaSilver
	goto BattleFrontier_BattleArenaBattleRoom_EventScript_DeclareOpponentWinner

BattleFrontier_BattleArenaBattleRoom_EventScript_DefeatedGretaSilver::
	call BattleFrontier_BattleArenaBattleRoom_EventScript_DeclarePlayerWinner
	frontier_getsymbols
	goto_if_ne VAR_RESULT, 0, BattleFrontier_BattleArenaBattleRoom_EventScript_ReturnToLobbyWon
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleArenaBattleRoom_Movement_OpponentStepForwardLong
	waitmovement 0
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_GretaYoureToughAfterAll, MSGBOX_DEFAULT
	playfanfare MUS_OBTAIN_SYMBOL
	message BattleFrontier_BattleArenaBattleRoom_Text_ReceivedGutsSymbol
	waitmessage
	waitfanfare
	frontier_givesymbol
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_GoingToBeFunNextTime, MSGBOX_DEFAULT
	goto BattleFrontier_BattleArenaBattleRoom_EventScript_ReturnToLobbyWon

BattleFrontier_BattleArenaBattleRoom_EventScript_IntroGretaGold::
	frontier_get FRONTIER_DATA_HEARD_BRAIN_SPEECH
	goto_if_ne VAR_RESULT, FALSE, BattleFrontier_BattleArenaBattleRoom_EventScript_BattleGretaGold
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_GretaLookingForwardToSeeingAgain, MSGBOX_DEFAULT
	closemessage
	frontier_set FRONTIER_DATA_HEARD_BRAIN_SPEECH
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleArenaBattleRoom_Movement_GretaLookAroundPlayer
	waitmovement 0
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_SoAreYouReady, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleArenaBattleRoom_Movement_GretaWalkBackToCenter
	waitmovement 0
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_WontAllowHalfheartedEffort, MSGBOX_DEFAULT
BattleFrontier_BattleArenaBattleRoom_EventScript_BattleGretaGold::
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_LetsGetThisStarted, MSGBOX_DEFAULT
	call BattleFrontier_BattleArenaBattleRoom_EventScript_StartArenaBattle
	goto_if_eq VAR_RESULT, 1, BattleFrontier_BattleArenaBattleRoom_EventScript_DefeatedGretaGold
	goto BattleFrontier_BattleArenaBattleRoom_EventScript_DeclareOpponentWinner

BattleFrontier_BattleArenaBattleRoom_EventScript_DefeatedGretaGold::
	call BattleFrontier_BattleArenaBattleRoom_EventScript_DeclarePlayerWinner
	frontier_getsymbols
	goto_if_eq VAR_RESULT, 2, BattleFrontier_BattleArenaBattleRoom_EventScript_ReturnToLobbyWon
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleArenaBattleRoom_Movement_OpponentStepForwardLong
	waitmovement 0
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_GretaBlownAway, MSGBOX_DEFAULT
	playfanfare MUS_OBTAIN_SYMBOL
	message BattleFrontier_BattleArenaBattleRoom_Text_GutsSymbolTookGoldenShine
	waitmessage
	waitfanfare
	frontier_givesymbol
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_IfWeBattleAgainWontLose, MSGBOX_DEFAULT
	goto BattleFrontier_BattleArenaBattleRoom_EventScript_ReturnToLobbyWon

BattleFrontier_BattleArenaBattleRoom_EventScript_StartArenaBattle::
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_SetKOTourneyBegin, MSGBOX_DEFAULT
	call BattleFrontier_BattleArenaBattleRoom_EventScript_DoArenaBattle
	return

BattleFrontier_BattleArenaBattleRoom_EventScript_DoArenaBattle::
	closemessage
	setvar VAR_TEMP_2, 0
	frontier_set FRONTIER_DATA_RECORD_DISABLED, FALSE
	special HealPlayerParty
	setvar VAR_0x8004, SPECIAL_BATTLE_ARENA
	setvar VAR_0x8005, 0
	special DoSpecialTrainerBattle
	waitstate
	frontier_restorehelditems
	special HealPlayerParty
	frontier_resetsketch
	return

BattleFrontier_BattleArenaBattleRoom_EventScript_DeclarePlayerWinner::
	applymovement LOCALID_ANNOUNCER, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceDown
	applymovement LOCALID_BLACK_BELT_1, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceDown
	applymovement LOCALID_BLACK_BELT_2, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceDown
	applymovement LOCALID_BLACK_BELT_3, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceUp
	applymovement LOCALID_BLACK_BELT_4, BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceUp
	playse SE_BANG
	waitse
	waitmovement 0
	msgbox BattleFrontier_BattleArenaBattleRoom_Text_WinnerIsPlayer, MSGBOX_DEFAULT
	closemessage
	call BattleFrontier_EventScript_IncrementWinStreak
	return

BattleFrontier_BattleArenaBattleRoom_Movement_PlayerEnter:
	walk_right
	walk_right
BattleFrontier_BattleArenaBattleRoom_Movement_PlayerStepForwardLong:
	walk_right
BattleFrontier_BattleArenaBattleRoom_Movement_PlayerStepForward:
	walk_right
	step_end

BattleFrontier_BattleArenaBattleRoom_Movement_PlayerWalkBackToLine:
	walk_left
	walk_left
	walk_in_place_faster_right
	step_end

BattleFrontier_BattleArenaBattleRoom_Movement_WalkInPlaceLeft:
	walk_in_place_faster_up
	step_end

BattleFrontier_BattleArenaBattleRoom_Movement_WalkInPlaceRight:
	walk_in_place_faster_right
	step_end

BattleFrontier_BattleArenaBattleRoom_Movement_OpponentEnter:
	walk_left
	walk_left
BattleFrontier_BattleArenaBattleRoom_Movement_OpponentStepForwardLong:
	walk_left
BattleFrontier_BattleArenaBattleRoom_Movement_OpponentStepForward:
	walk_left
	step_end

BattleFrontier_BattleArenaBattleRoom_Movement_GretaEnter:
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	step_end

BattleFrontier_BattleArenaBattleRoom_Movement_OpponentExit:
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceDown:
	disable_jump_landing_ground_effect
	jump_in_place_down
	step_end

BattleFrontier_BattleArenaBattleRoom_Movement_JumpInPlaceUp:
	disable_jump_landing_ground_effect
	jump_in_place_up
	step_end

BattleFrontier_BattleArenaBattleRoom_Movement_WalkInPlaceDown:
	walk_in_place_faster_down
	step_end

@ Unused, redundant
BattleFrontier_BattleArenaBattleRoom_Movement_WalkInPlaceRight2:
	walk_in_place_faster_right
	step_end

BattleFrontier_BattleArenaBattleRoom_OnWarp:
	map_script_2 VAR_TEMP_1, 0, BattleFrontier_BattleArenaBattleRoom_EventScript_SetUpRoomObjects
	.2byte 0

BattleFrontier_BattleArenaBattleRoom_EventScript_SetUpRoomObjects::
	hideobjectat LOCALID_PLAYER, MAP_BATTLE_FRONTIER_BATTLE_ARENA_BATTLE_ROOM
	removeobject LOCALID_OPPONENT
	call BattleFrontier_BattleDomeBattleRoom_EventScript_SetPlayerGfx
	applymovement OBJ_EVENT_ID_PLAYER, BattleFrontier_BattleDomeBattleRoom_Movement_SetInvisible
	setvar VAR_TEMP_1, 1
	end

BattleFrontier_BattleArenaBattleRoom_Movement_GretaLookAroundPlayer:
	walk_down
	walk_in_place_faster_left
	delay_16
	delay_16
	delay_16
	delay_16
	walk_up
	walk_up
	walk_in_place_faster_left
	step_end

BattleFrontier_BattleArenaBattleRoom_Movement_GretaWalkBackToCenter:
	walk_down
	walk_in_place_faster_left
	step_end

BattleFrontier_BattleArenaBattleRoom_Text_PlayerStepForward:
	.string "REFEREE: TRAINER {PLAYER}!\n"
	.string "Step forward, please!$"

BattleFrontier_BattleArenaBattleRoom_Text_OpponentStepForward:
	.string "REFEREE: TRAINER {STR_VAR_1}!\n"
	.string "Step forward, please!$"

BattleFrontier_BattleArenaBattleRoom_Text_SetKOTourneyBegin:
	.string "REFEREE: Set KO Tourney!\n"
	.string "Begin!$"

BattleFrontier_BattleArenaBattleRoom_Text_WinnerIsPlayer:
	.string "REFEREE: The winner is {PLAYER}!$"

BattleFrontier_BattleArenaBattleRoom_Text_WinnerIsOpponent:
	.string "REFEREE: The winner is {STR_VAR_1}!$"

BattleFrontier_BattleArenaBattleRoom_Text_MonsWillBeRestored:
	.string "Your POKéMON will be restored to\n"
	.string "full health.$"

BattleFrontier_BattleArenaBattleRoom_Text_ReadyFor2ndOpponent:
	.string "Next up, your second opponent!\n"
	.string "Are you ready to move on?$"

BattleFrontier_BattleArenaBattleRoom_Text_ReadyFor3rdOpponent:
	.string "Next up, your third opponent!\n"
	.string "Are you ready to move on?$"

BattleFrontier_BattleArenaBattleRoom_Text_ReadyFor4thOpponent:
	.string "Next up, your fourth opponent!\n"
	.string "Are you ready to move on?$"

BattleFrontier_BattleArenaBattleRoom_Text_ReadyFor5thOpponent:
	.string "Next up, your fifth opponent!\n"
	.string "Are you ready to move on?$"

BattleFrontier_BattleArenaBattleRoom_Text_ReadyFor6thOpponent:
	.string "Next up, your sixth opponent!\n"
	.string "Are you ready to move on?$"

BattleFrontier_BattleArenaBattleRoom_Text_ReadyFor7thOpponent:
	.string "Next up, your seventh opponent!\n"
	.string "Are you ready to move on?$"

BattleFrontier_BattleArenaBattleRoom_Text_SaveAndShutDown:
	.string "Would you like to save the game and\n"
	.string "shut down now?$"

BattleFrontier_BattleArenaBattleRoom_Text_RetireFromChallenge:
	.string "Would you like to retire from your\n"
	.string "Set KO Tourney challenge?$"

BattleFrontier_BattleArenaBattleRoom_Text_SavingPleaseWait:
	.string "I am saving your game data.\n"
	.string "Please wait.$"

BattleFrontier_BattleArenaBattleRoom_Text_RecordLastBattle:
	.string "Would you like to record your last\n"
	.string "battle on your FRONTIER PASS?$"

BattleFrontier_BattleArenaBattleRoom_Text_NowFaceTycoon:
	.string "My dear challenger!\p"
	.string "Your skill level is truly astounding!\p"
	.string "We now would like you to face our\n"
	.string "leader, the ARENA TYCOON!$"

BattleFrontier_BattleArenaBattleRoom_Text_PreparedForTycoon:
	.string "A battle with the ARENA TYCOON!\n"
	.string "Are you prepared?$"

BattleFrontier_BattleArenaBattleRoom_Text_MakeWayForGreta:
	.string "REFEREE: The ARENA TYCOON!\n"
	.string "Make way for GRETA!$"

BattleFrontier_BattleArenaBattleRoom_Text_GretaYoureChallenger:
	.string "GRETA: Hey!\n"
	.string "Howdy!\p"
	.string "…Wait, are you the challenger?$"

BattleFrontier_BattleArenaBattleRoom_Text_IsThatRight:
	.string "Is that right? Hmm…\n"
	.string "Hmhm…$"

BattleFrontier_BattleArenaBattleRoom_Text_YouLookWeakTakeThingsEasy:
	.string "I don't know how to say it, but…\n"
	.string "To put it bluntly, you look pretty weak.\l"
	.string "Are you sure you're up for me?\p"
	.string "Hmm…\p"
	.string "Well, all right!\n"
	.string "We'll take things easy to start with!$"

BattleFrontier_BattleArenaBattleRoom_Text_IgniteMyPassionForBattle:
	.string "Okay! Let's see you ignite my passion\n"
	.string "for battle!$"

BattleFrontier_BattleArenaBattleRoom_Text_GretaYoureToughAfterAll:
	.string "GRETA: Ow, wait a second!\n"
	.string "You are tough after all!\p"
	.string "I like you!\n"
	.string "Let's see your FRONTIER PASS.$"

BattleFrontier_BattleArenaBattleRoom_Text_ReceivedGutsSymbol:
	.string "The Guts Symbol was embossed on\n"
	.string "the FRONTIER PASS!$"

BattleFrontier_BattleArenaBattleRoom_Text_GoingToBeFunNextTime:
	.string "Hmm…\p"
	.string "It's going to be fun the next time!\n"
	.string "I'm looking forward to it!$"

BattleFrontier_BattleArenaBattleRoom_Text_GretaLookingForwardToSeeingAgain:
	.string "GRETA: Hey! Howdy!\n"
	.string "You finally won your way up to me!\p"
	.string "I was getting worried waiting for you!\n"
	.string "I was really looking forward to seeing\l"
	.string "you again!$"

BattleFrontier_BattleArenaBattleRoom_Text_SoAreYouReady:
	.string "… … …\n"
	.string "So, are you ready?$"

BattleFrontier_BattleArenaBattleRoom_Text_WontAllowHalfheartedEffort:
	.string "I won't allow a halfhearted effort!\n"
	.string "Be ready for a thrashing!$"

BattleFrontier_BattleArenaBattleRoom_Text_LetsGetThisStarted:
	.string "Come on, REFEREE!\n"
	.string "Let's get this started!$"

BattleFrontier_BattleArenaBattleRoom_Text_GretaBlownAway:
	.string "GRETA: Gaaah! Blown away!\n"
	.string "Let's see your FRONTIER PASS!$"

BattleFrontier_BattleArenaBattleRoom_Text_GutsSymbolTookGoldenShine:
	.string "The Guts Symbol took on\n"
	.string "a golden shine!$"

BattleFrontier_BattleArenaBattleRoom_Text_IfWeBattleAgainWontLose:
	.string "Arrrgh!\n"
	.string "This is so infuriating!\p"
	.string "If we ever battle again, I won't lose!\n"
	.string "Don't you forget it! Bye-bye!$"
