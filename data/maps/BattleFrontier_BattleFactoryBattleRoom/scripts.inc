.set LOCALID_SCIENTIST_1, 1
.set LOCALID_OPPONENT, 2
.set LOCALID_SCIENTIST_2, 3
.set LOCALID_SCIENTIST_3, 4
.set LOCALID_SCIENTIST_4, 5
.set LOCALID_SCIENTIST_5, 6
.set LOCALID_SCIENTIST_6, 7
.set LOCALID_PLAYER, 8

BattleFrontier_BattleFactoryBattleRoom_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, BattleFrontier_BattleFactoryBattleRoom_OnTransition
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, BattleFrontier_BattleFactoryBattleRoom_OnWarp
	map_script MAP_SCRIPT_ON_FRAME_TABLE, BattleFrontier_BattleFactoryBattleRoom_OnFrame
	.byte 0

	@ On this map the player (OBJ_EVENT_ID_PLAYER) is hidden
	@ The player is represented instead by LOCALID_PLAYER, which has the gfx id VAR_OBJ_GFX_ID_F

BattleFrontier_BattleFactoryBattleRoom_OnTransition:
	frontier_settrainers
	checkplayergender
	call_if_eq VAR_RESULT, MALE, BattleFrontier_BattleFactoryBattleRoom_EventScript_SetPlayerGfxMale
	call_if_eq VAR_RESULT, FEMALE, BattleFrontier_BattleFactoryBattleRoom_EventScript_SetPlayerGfxFemale
	frontier_getbrainstatus
	copyvar VAR_TEMP_F, VAR_RESULT
	goto_if_ne VAR_RESULT, FRONTIER_BRAIN_NOT_READY, BattleFrontier_BattleFactoryBattleRoom_EventScript_SetUpFactoryHeadObj
	end

BattleFrontier_BattleFactoryBattleRoom_EventScript_SetUpFactoryHeadObj::
	call BattleFrontier_EventScript_SetBrainObjectGfx
	setobjectxyperm LOCALID_OPPONENT, 7, 9
	end

BattleFrontier_BattleFactoryBattleRoom_OnWarp:
	map_script_2 VAR_TEMP_1, 0, BattleFrontier_BattleFactoryBattleRoom_EventScript_HideObjects
	.2byte 0

BattleFrontier_BattleFactoryBattleRoom_EventScript_HideObjects::
	setvar VAR_TEMP_1, 1
	hideobjectat OBJ_EVENT_ID_PLAYER, MAP_BATTLE_FRONTIER_BATTLE_FACTORY_BATTLE_ROOM
	goto_if_ne VAR_TEMP_F, FRONTIER_BRAIN_NOT_READY, BattleFrontier_BattleFactoryBattleRoom_EventScript_EndHideObjects
	hideobjectat LOCALID_OPPONENT, MAP_BATTLE_FRONTIER_BATTLE_FACTORY_BATTLE_ROOM
BattleFrontier_BattleFactoryBattleRoom_EventScript_EndHideObjects::
	end

BattleFrontier_BattleFactoryBattleRoom_EventScript_SetPlayerGfxMale::
	setvar VAR_OBJ_GFX_ID_F, OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL
	return

BattleFrontier_BattleFactoryBattleRoom_EventScript_SetPlayerGfxFemale::
	setvar VAR_OBJ_GFX_ID_F, OBJ_EVENT_GFX_RIVAL_MAY_NORMAL
	return

BattleFrontier_BattleFactoryBattleRoom_OnFrame:
	map_script_2 VAR_TEMP_0, 0, BattleFrontier_BattleFactoryBattleRoom_EventScript_EnterRoom
	.2byte 0

BattleFrontier_BattleFactoryBattleRoom_EventScript_EnterRoomFactoryHeadBattle::
	msgbox BattleFrontier_BattleFactoryBattleRoom_Text_GetAMoveOn, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleFactoryBattleRoom_Movement_NolandMoveToBattle
	applymovement LOCALID_PLAYER, BattleFrontier_BattleFactoryBattleRoom_Movement_PlayerEnterRoom
	applymovement OBJ_EVENT_ID_PLAYER, BattleFrontier_BattleFactoryBattleRoom_Movement_PlayerEnterRoom
	waitmovement 0
	call BattleFrontier_BattleFactoryBattleRoom_EventScript_ScientistsFaceBattle
	goto BattleFrontier_BattleFactoryBattleRoom_EventScript_BattleOpponent
	end

BattleFrontier_BattleFactoryBattleRoom_EventScript_EnterRoom::
	goto_if_ne VAR_TEMP_F, FRONTIER_BRAIN_NOT_READY, BattleFrontier_BattleFactoryBattleRoom_EventScript_EnterRoomFactoryHeadBattle
	applymovement LOCALID_PLAYER, BattleFrontier_BattleFactoryBattleRoom_Movement_PlayerEnterRoom
	applymovement OBJ_EVENT_ID_PLAYER, BattleFrontier_BattleFactoryBattleRoom_Movement_PlayerEnterRoom
	waitmovement 0
	call BattleFrontier_BattleFactoryBattleRoom_EventScript_ScientistsFaceBattle
	factory_setopponentgfx
	removeobject LOCALID_OPPONENT
	setobjectxyperm LOCALID_OPPONENT, 7, 1
	addobject LOCALID_OPPONENT
	applymovement LOCALID_OPPONENT, BattleFrontier_BattleFactoryBattleRoom_Movement_OpponentEnter
	waitmovement 0
BattleFrontier_BattleFactoryBattleRoom_EventScript_BattleOpponent::
	goto_if_ne VAR_TEMP_F, FRONTIER_BRAIN_NOT_READY, BattleFrontier_BattleFactoryBattleRoom_EventScript_BattleNoland
	palace_getopponentintro
	lockall
	msgbox gStringVarBuffer, MSGBOX_DEFAULT
	waitmessage
	closemessage
	frontier_set FRONTIER_DATA_RECORD_DISABLED, FALSE
	special HealPlayerParty
	setvar VAR_0x8004, SPECIAL_BATTLE_FACTORY
	setvar VAR_0x8005, 0
	special DoSpecialTrainerBattle
	waitstate
	switch VAR_RESULT
	case 1, BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedOpponent
BattleFrontier_BattleFactoryBattleRoom_EventScript_WarpToLobbyLost::
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, CHALLENGE_STATUS_LOST
	goto BattleFrontier_BattleFactoryBattleRoom_EventScript_WarpToLobby

BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedOpponent::
	factory_get FACTORY_DATA_WIN_STREAK_SWAPS
	goto_if_eq VAR_RESULT, MAX_STREAK, BattleFrontier_BattleFactoryBattleRoom_EventScript_IncrementWinStreak
	addvar VAR_RESULT, 1
	setorcopyvar VAR_0x8006, VAR_RESULT
	factory_set FACTORY_DATA_WIN_STREAK_SWAPS  @ uses VAR_0x8006 above
BattleFrontier_BattleFactoryBattleRoom_EventScript_IncrementWinStreak::
	call BattleFrontier_EventScript_IncrementWinStreak
BattleFrontier_BattleFactoryBattleRoom_EventScript_IncrementBattleNum::
	frontier_get FRONTIER_DATA_BATTLE_NUM
	addvar VAR_RESULT, 1
	frontier_set FRONTIER_DATA_BATTLE_NUM, VAR_RESULT
	switch VAR_RESULT
	case 7, BattleFrontier_BattleFactoryBattleRoom_EventScript_WarpToLobbyWon
	setvar VAR_0x8006, 1
	warp MAP_BATTLE_FRONTIER_BATTLE_FACTORY_PRE_BATTLE_ROOM, 8, 8
	waitstate
BattleFrontier_BattleFactoryBattleRoom_EventScript_WarpToLobbyWon::
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, CHALLENGE_STATUS_WON
	goto BattleFrontier_BattleFactoryBattleRoom_EventScript_WarpToLobby

BattleFrontier_BattleFactoryBattleRoom_EventScript_BattleNoland::
	switch VAR_TEMP_F
	case FRONTIER_BRAIN_GOLD, BattleFrontier_BattleFactoryBattleRoom_EventScript_IntroNolandGold
	case FRONTIER_BRAIN_STREAK, BattleFrontier_BattleFactoryBattleRoom_EventScript_BattleNolandSilver
	case FRONTIER_BRAIN_STREAK_LONG, BattleFrontier_BattleFactoryBattleRoom_EventScript_BattleNolandGold
	frontier_get FRONTIER_DATA_HEARD_BRAIN_SPEECH
	goto_if_ne VAR_RESULT, FALSE, BattleFrontier_BattleFactoryBattleRoom_EventScript_BattleNolandSilver
	msgbox BattleFrontier_BattleFactoryBattleRoom_Text_NolandImFactoryHead, MSGBOX_DEFAULT
	frontier_set FRONTIER_DATA_HEARD_BRAIN_SPEECH
BattleFrontier_BattleFactoryBattleRoom_EventScript_BattleNolandSilver::
	msgbox BattleFrontier_BattleFactoryBattleRoom_Text_ShakeOutKnowledgeBringItOn, MSGBOX_DEFAULT
	call BattleFrontier_BattleFactoryBattleRoom_EventScript_DoNolandBattle
	goto_if_eq VAR_RESULT, 1, BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedNolandSilver
	goto BattleFrontier_BattleFactoryBattleRoom_EventScript_WarpToLobbyLost

BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedNolandSilver::
	frontier_getsymbols
	goto_if_ne VAR_RESULT, 0, BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedNoland
	msgbox BattleFrontier_BattleFactoryBattleRoom_Text_NolandLetsSeeFrontierPass, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PLAYER, BattleFrontier_BattleFactoryBattleRoom_Movement_PlayerApproachNoland
	waitmovement 0
	playfanfare MUS_OBTAIN_SYMBOL
	message BattleFrontier_BattleFactoryBattleRoom_Text_ReceivedKnowledgeSymbol
	waitmessage
	waitfanfare
	frontier_givesymbol
	msgbox BattleFrontier_BattleFactoryBattleRoom_Text_NextTimeNoHoldsBarred, MSGBOX_DEFAULT
	goto BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedNoland

BattleFrontier_BattleFactoryBattleRoom_EventScript_IntroNolandGold::
	frontier_get FRONTIER_DATA_HEARD_BRAIN_SPEECH
	goto_if_ne VAR_RESULT, FALSE, BattleFrontier_BattleFactoryBattleRoom_EventScript_BattleNolandGold
	msgbox BattleFrontier_BattleFactoryBattleRoom_Text_HarderLookThanLastTime, MSGBOX_DEFAULT
	frontier_set FRONTIER_DATA_HEARD_BRAIN_SPEECH
BattleFrontier_BattleFactoryBattleRoom_EventScript_BattleNolandGold::
	msgbox BattleFrontier_BattleFactoryBattleRoom_Text_AllRightBringItOn, MSGBOX_DEFAULT
	call BattleFrontier_BattleFactoryBattleRoom_EventScript_DoNolandBattle
	goto_if_eq VAR_RESULT, 1, BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedNolandGold
	goto BattleFrontier_BattleFactoryBattleRoom_EventScript_WarpToLobbyLost

BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedNolandGold::
	frontier_getsymbols
	goto_if_eq VAR_RESULT, 2, BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedNoland
	msgbox BattleFrontier_BattleFactoryBattleRoom_Text_OutOfMyLeagueLetsSeePass, MSGBOX_DEFAULT
	waitmessage
	applymovement LOCALID_PLAYER, BattleFrontier_BattleFactoryBattleRoom_Movement_PlayerApproachNoland
	waitmovement 0
	playfanfare MUS_OBTAIN_SYMBOL
	message BattleFrontier_BattleFactoryBattleRoom_Text_KnowledgeSymbolTookGoldenShine
	waitmessage
	waitfanfare
	frontier_givesymbol
	msgbox BattleFrontier_BattleFactoryBattleRoom_Text_LastTimeILoseToYou, MSGBOX_DEFAULT
	goto BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedNoland

BattleFrontier_BattleFactoryBattleRoom_EventScript_DoNolandBattle::
	closemessage
	frontier_set FRONTIER_DATA_RECORD_DISABLED, FALSE
	special HealPlayerParty
	setvar VAR_0x8004, SPECIAL_BATTLE_FACTORY
	setvar VAR_0x8005, 0
	special DoSpecialTrainerBattle
	waitstate
	return

BattleFrontier_BattleFactoryBattleRoom_EventScript_DefeatedNoland::
	factory_get FACTORY_DATA_WIN_STREAK_SWAPS
	goto_if_eq VAR_RESULT, MAX_STREAK, BattleFrontier_BattleFactoryBattleRoom_EventScript_IncrementWinStreak
	addvar VAR_RESULT, 1
	setorcopyvar VAR_0x8006, VAR_RESULT
	factory_set FACTORY_DATA_WIN_STREAK_SWAPS  @ uses VAR_0x8006 above
	factory_get FACTORY_DATA_WIN_STREAK
	goto_if_eq VAR_RESULT, MAX_STREAK, BattleFrontier_BattleFactoryBattleRoom_EventScript_IncrementBattleNum
	addvar VAR_RESULT, 1
	factory_set FACTORY_DATA_WIN_STREAK, VAR_RESULT
	frontier_get FRONTIER_DATA_BATTLE_NUM
	addvar VAR_RESULT, 1
	goto BattleFrontier_BattleFactoryBattleRoom_EventScript_WarpToLobbyWon
	end

BattleFrontier_BattleFactoryBattleRoom_Movement_PlayerEnterRoom:
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	face_right
	step_end

BattleFrontier_BattleFactoryBattleRoom_Movement_PlayerApproachNoland:
	walk_right
	step_end

BattleFrontier_BattleFactoryBattleRoom_Movement_OpponentEnter:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	face_left
	step_end

BattleFrontier_BattleFactoryBattleRoom_Movement_NolandMoveToBattle:
	walk_up
	walk_up
	walk_up
	face_left
	step_end

BattleFrontier_BattleFactoryBattleRoom_EventScript_WarpToLobby::
	copyvar VAR_RESULT, VAR_FRONTIER_BATTLE_MODE
	goto_if_eq VAR_RESULT, FRONTIER_MODE_DOUBLES, BattleFrontier_BattleFactoryBattleRoom_EventScript_WarpToLobbyDoubles
	warp MAP_BATTLE_FRONTIER_BATTLE_FACTORY_LOBBY, 4, 8
	waitstate
	end

BattleFrontier_BattleFactoryBattleRoom_EventScript_WarpToLobbyDoubles::
	warp MAP_BATTLE_FRONTIER_BATTLE_FACTORY_LOBBY, 14, 8
	waitstate
	end

BattleFrontier_BattleFactoryBattleRoom_EventScript_ScientistsFaceBattle::
	applymovement LOCALID_SCIENTIST_1, Common_Movement_WalkInPlaceFasterRight
	applymovement LOCALID_SCIENTIST_2, Common_Movement_WalkInPlaceFasterRight
	applymovement LOCALID_SCIENTIST_3, Common_Movement_WalkInPlaceFasterRight
	applymovement LOCALID_SCIENTIST_4, Common_Movement_WalkInPlaceFasterLeft
	applymovement LOCALID_SCIENTIST_5, Common_Movement_WalkInPlaceFasterLeft
	applymovement LOCALID_SCIENTIST_6, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	return

BattleFrontier_BattleFactoryBattleRoom_Text_GetAMoveOn:
	.string "Hey, hey!\n"
	.string "Get a move on!$"

BattleFrontier_BattleFactoryBattleRoom_Text_NolandImFactoryHead:
	.string "Hey, my name's NOLAND!\n"
	.string "I'm basically in charge of this place,\l"
	.string "which is why I'm the FACTORY HEAD!\p"
	.string "I had a look at your Battle Swap data.\p"
	.string "You seem to have the right idea,\n"
	.string "but you're still square in your head!\p"
	.string "Listen up, okay? Knowledge isn't only\n"
	.string "about reading books or doing desk work.\p"
	.string "Just doing that sort of thing…\n"
	.string "It may as well be useless!\p"
	.string "You have to experience things with\n"
	.string "your heart and your body, understand?\p"
	.string "I'll take you on under the same\n"
	.string "conditions as you.\p"
	.string "I'll be using rental POKéMON, too!$"

BattleFrontier_BattleFactoryBattleRoom_Text_ShakeOutKnowledgeBringItOn:
	.string "Shake out every last bit of your\n"
	.string "knowledge and bring it on!$"

BattleFrontier_BattleFactoryBattleRoom_Text_NolandLetsSeeFrontierPass:
	.string "NOLAND: Smart going!\n"
	.string "Let's see your FRONTIER PASS.$"

BattleFrontier_BattleFactoryBattleRoom_Text_ReceivedKnowledgeSymbol:
	.string "The Knowledge Symbol was embossed\n"
	.string "on the FRONTIER PASS!$"

BattleFrontier_BattleFactoryBattleRoom_Text_NextTimeNoHoldsBarred:
	.string "Heh…\n"
	.string "You're a pretty bright spark…\p"
	.string "Next time, I'll come after you hard.\n"
	.string "No holds barred, understand?\p"
	.string "You keep up your studies!$"

BattleFrontier_BattleFactoryBattleRoom_Text_HarderLookThanLastTime:
	.string "NOLAND: Hey, hey! How's it going?\n"
	.string "You keeping up with your studies?\p"
	.string "…Oh?\p"
	.string "You've taken on a harder look than\n"
	.string "the last time I saw you.\p"
	.string "Now, this should be fun!\n"
	.string "I'm getting excited, hey!$"

BattleFrontier_BattleFactoryBattleRoom_Text_AllRightBringItOn:
	.string "All right!\n"
	.string "Bring it on!$"

BattleFrontier_BattleFactoryBattleRoom_Text_OutOfMyLeagueLetsSeePass:
	.string "NOLAND: What the…\n"
	.string "You're getting out of my league!\p"
	.string "Sheesh!\n"
	.string "Let's see that FRONTIER PASS!$"

BattleFrontier_BattleFactoryBattleRoom_Text_KnowledgeSymbolTookGoldenShine:
	.string "The Knowledge Symbol took on\n"
	.string "a golden shine!$"

BattleFrontier_BattleFactoryBattleRoom_Text_LastTimeILoseToYou:
	.string "Pfft, man!\p"
	.string "That's absolutely the last time\n"
	.string "I lose to you!\p"
	.string "We have to do this again, hey?$"

