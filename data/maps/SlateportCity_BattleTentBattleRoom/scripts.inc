.set LOCALID_OPPONENT, 2
.set LOCALID_PLAYER, 3

SlateportCity_BattleTentBattleRoom_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, SlateportCity_BattleTentBattleRoom_OnTransition
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, SlateportCity_BattleTentBattleRoom_OnWarp
	map_script MAP_SCRIPT_ON_FRAME_TABLE, SlateportCity_BattleTentBattleRoom_OnFrame
	.byte 0

	@ On this map the player (OBJ_EVENT_ID_PLAYER) is hidden
	@ The player is represented instead by LOCALID_PLAYER, which has the gfx id VAR_OBJ_GFX_ID_1

SlateportCity_BattleTentBattleRoom_OnTransition:
	call SlateportCity_BattleTentBattleRoom_EventScript_SetPlayerGfx
	end

SlateportCity_BattleTentBattleRoom_EventScript_SetPlayerGfx::
	checkplayergender
	goto_if_eq VAR_RESULT, MALE, SlateportCity_BattleTentBattleRoom_EventScript_SetPlayerGfxMale
	goto_if_eq VAR_RESULT, FEMALE, SlateportCity_BattleTentBattleRoom_EventScript_SetPlayerGfxFemale
	return

SlateportCity_BattleTentBattleRoom_EventScript_SetPlayerGfxMale::
	setvar VAR_OBJ_GFX_ID_1, OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL
	return

SlateportCity_BattleTentBattleRoom_EventScript_SetPlayerGfxFemale::
	setvar VAR_OBJ_GFX_ID_1, OBJ_EVENT_GFX_RIVAL_MAY_NORMAL
	return

SlateportCity_BattleTentBattleRoom_OnWarp:
	map_script_2 VAR_TEMP_1, 0, SlateportCity_BattleTentBattleRoom_EventScript_SetUpObjects
	.2byte 0

SlateportCity_BattleTentBattleRoom_EventScript_SetUpObjects::
	setvar VAR_TEMP_1, 1
	hideobjectat OBJ_EVENT_ID_PLAYER, MAP_FALLARBOR_TOWN_BATTLE_TENT_BATTLE_ROOM
	hideobjectat LOCALID_OPPONENT, MAP_SLATEPORT_CITY_BATTLE_TENT_BATTLE_ROOM
	end

SlateportCity_BattleTentBattleRoom_OnFrame:
	map_script_2 VAR_TEMP_0, 0, SlateportCity_BattleTentBattleRoom_EventScript_EnterRoom
	.2byte 0

SlateportCity_BattleTentBattleRoom_EventScript_EnterRoom::
	applymovement LOCALID_PLAYER, SlateportCity_BattleTentBattleRoom_Movement_PlayerEnter
	waitmovement 0
	factory_setopponentgfx
	setobjectxyperm LOCALID_OPPONENT, 5, 1
	removeobject LOCALID_OPPONENT
	addobject LOCALID_OPPONENT
	applymovement LOCALID_OPPONENT, SlateportCity_BattleTentBattleRoom_Movement_OpponentEnter
	waitmovement 0
	battletent_getopponentintro
	lockall
	msgbox gStringVarBuffer, MSGBOX_DEFAULT
	waitmessage
	closemessage
	special HealPlayerParty
	setvar VAR_0x8004, SPECIAL_BATTLE_FACTORY
	setvar VAR_0x8005, 0
	special DoSpecialTrainerBattle
	waitstate
	switch VAR_RESULT
	case 1, SlateportCity_BattleTentBattleRoom_EventScript_DefeatedOpponent
SlateportCity_BattleTent_EventScript_WarpToLobbyLost::
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, CHALLENGE_STATUS_LOST
	special LoadPlayerParty
	warp MAP_SLATEPORT_CITY_BATTLE_TENT_LOBBY, 6, 6
	waitstate
@ forced stop

SlateportCity_BattleTentBattleRoom_EventScript_DefeatedOpponent::
	frontier_get FRONTIER_DATA_BATTLE_NUM
	addvar VAR_RESULT, 1
	frontier_set FRONTIER_DATA_BATTLE_NUM, VAR_RESULT
	switch VAR_RESULT
	case 3, SlateportCity_BattleTentBattleRoom_EventScript_WarpToLobbyWon
	setvar VAR_0x8006, 1
	warp MAP_SLATEPORT_CITY_BATTLE_TENT_CORRIDOR, 2, 3
	waitstate
@ forced stop

SlateportCity_BattleTentBattleRoom_EventScript_WarpToLobbyWon::
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, CHALLENGE_STATUS_WON
	special LoadPlayerParty
	warp MAP_SLATEPORT_CITY_BATTLE_TENT_LOBBY, 6, 6
	waitstate
@ forced stop

SlateportCity_BattleTentBattleRoom_Movement_PlayerEnter:
	walk_up
	walk_up
	walk_up
	walk_in_place_faster_right
	step_end

SlateportCity_BattleTentBattleRoom_Movement_OpponentEnter:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_in_place_faster_left
	step_end

