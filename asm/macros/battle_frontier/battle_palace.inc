	@ Initialize the Battle Palace challenge
	.macro palace_init
	setvar VAR_0x8004, BATTLE_PALACE_FUNC_INIT
	special CallBattlePalaceFunction
	.endm

	@ Get the value of some PALACE_DATA_*. See GetPalaceData for the data types that can be retrieved
	.macro palace_get data:req
	setvar VAR_0x8004, BATTLE_PALACE_FUNC_GET_DATA
	setvar VAR_0x8005, \data
	special CallBattlePalaceFunction
	.endm

	@ Set some PALACE_DATA_* to val. See SetPalaceData for the data types that can be set
	.macro palace_set data:req, val:req
	setvar VAR_0x8004, BATTLE_PALACE_FUNC_SET_DATA
	setvar VAR_0x8005, \data
	setvar VAR_0x8006, \val
	special CallBattlePalaceFunction
	.endm

	@ Get the id of the comment to be made while being led through the corridor
	.macro palace_getcomment
	setvar VAR_0x8004, BATTLE_PALACE_FUNC_GET_COMMENT_ID
	special CallBattlePalaceFunction
	.endm

	@ Unused. Choose and set the opponent gfx id. The equivalent macro from Battle Tower is used instead
	.macro palace_setopponent
	setvar VAR_0x8004, BATTLE_PALACE_FUNC_SET_OPPONENT
	special CallBattlePalaceFunction
	.endm

	@ Buffer the opponents intro speech to gStringVarBuffer. Also used by Battle Arena and Factory
	.macro palace_getopponentintro
	setvar VAR_0x8004, BATTLE_PALACE_FUNC_GET_OPPONENT_INTRO
	special CallBattlePalaceFunction
	.endm

	@ Increments the current win streak by 1
	.macro palace_incrementstreak
	setvar VAR_0x8004, BATTLE_PALACE_FUNC_INCREMENT_STREAK
	special CallBattlePalaceFunction
	.endm

	@ Save the game and set the challenge status
	.macro palace_save challengeStatus:req
	setvar VAR_0x8004, BATTLE_PALACE_FUNC_SAVE
	setvar VAR_0x8005, \challengeStatus
	special CallBattlePalaceFunction
	.endm

	@ Unused. Set the prize item to give from one of two lists of possible prizes, depending on streak.
	.macro palace_setprize
	setvar VAR_0x8004, BATTLE_PALACE_FUNC_SET_PRIZE
	special CallBattleArenaFunction
	.endm

	@ Unused. Give the set prize item to the player. TRUE if room in Bag for prize, FALSE otherwise
	.macro palace_giveprize
	setvar VAR_0x8004, BATTLE_PALACE_FUNC_GIVE_PRIZE
	special CallBattleArenaFunction
	.endm
