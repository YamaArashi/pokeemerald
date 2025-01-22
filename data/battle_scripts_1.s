#include "constants/global.h"
#include "constants/battle.h"
#include "constants/pokemon.h"
#include "constants/battle_arena.h"
#include "constants/battle_script_commands.h"
#include "constants/battle_anim.h"
#include "constants/battle_string_ids.h"
#include "constants/abilities.h"
#include "constants/moves.h"
#include "constants/songs.h"
#include "constants/game_stat.h"
#include "constants/trainers.h"
	.include "asm/macros.inc"
	.include "asm/macros/battle_script.inc"
	.include "constants/constants.inc"

	.section script_data, "aw", %progbits

.align 2
gBattleScriptsForMoveEffects::
	.4byte BattleScript_EffectHit                    @ EFFECT_HIT
	.4byte BattleScript_EffectSleep                  @ EFFECT_SLEEP
	.4byte BattleScript_EffectPoisonHit              @ EFFECT_POISON_HIT
	.4byte BattleScript_EffectAbsorb                 @ EFFECT_ABSORB
	.4byte BattleScript_EffectBurnHit                @ EFFECT_BURN_HIT
	.4byte BattleScript_EffectFreezeHit              @ EFFECT_FREEZE_HIT
	.4byte BattleScript_EffectParalyzeHit            @ EFFECT_PARALYZE_HIT
	.4byte BattleScript_EffectExplosion              @ EFFECT_EXPLOSION
	.4byte BattleScript_EffectDreamEater             @ EFFECT_DREAM_EATER
	.4byte BattleScript_EffectMirrorMove             @ EFFECT_MIRROR_MOVE
	.4byte BattleScript_EffectAttackUp               @ EFFECT_ATTACK_UP
	.4byte BattleScript_EffectDefenseUp              @ EFFECT_DEFENSE_UP
	.4byte BattleScript_EffectHit                    @ EFFECT_SPEED_UP
	.4byte BattleScript_EffectSpecialAttackUp        @ EFFECT_SPECIAL_ATTACK_UP
	.4byte BattleScript_EffectHit                    @ EFFECT_SPECIAL_DEFENSE_UP
	.4byte BattleScript_EffectHit                    @ EFFECT_ACCURACY_UP
	.4byte BattleScript_EffectEvasionUp              @ EFFECT_EVASION_UP
	.4byte BattleScript_EffectHit                    @ EFFECT_ALWAYS_HIT
	.4byte BattleScript_EffectAttackDown             @ EFFECT_ATTACK_DOWN
	.4byte BattleScript_EffectDefenseDown            @ EFFECT_DEFENSE_DOWN
	.4byte BattleScript_EffectSpeedDown              @ EFFECT_SPEED_DOWN
	.4byte BattleScript_EffectHit                    @ EFFECT_SPECIAL_ATTACK_DOWN
	.4byte BattleScript_EffectHit                    @ EFFECT_SPECIAL_DEFENSE_DOWN
	.4byte BattleScript_EffectAccuracyDown           @ EFFECT_ACCURACY_DOWN
	.4byte BattleScript_EffectEvasionDown            @ EFFECT_EVASION_DOWN
	.4byte BattleScript_EffectHaze                   @ EFFECT_HAZE
	.4byte BattleScript_EffectBide                   @ EFFECT_BIDE
	.4byte BattleScript_EffectRampage                @ EFFECT_RAMPAGE
	.4byte BattleScript_EffectRoar                   @ EFFECT_ROAR
	.4byte BattleScript_EffectMultiHit               @ EFFECT_MULTI_HIT
	.4byte BattleScript_EffectConversion             @ EFFECT_CONVERSION
	.4byte BattleScript_EffectFlinchHit              @ EFFECT_FLINCH_HIT
	.4byte BattleScript_EffectRestoreHp              @ EFFECT_RESTORE_HP
	.4byte BattleScript_EffectToxic                  @ EFFECT_TOXIC
	.4byte BattleScript_EffectPayDay                 @ EFFECT_PAY_DAY
	.4byte BattleScript_EffectLightScreen            @ EFFECT_LIGHT_SCREEN
	.4byte BattleScript_EffectTriAttack              @ EFFECT_TRI_ATTACK
	.4byte BattleScript_EffectRest                   @ EFFECT_REST
	.4byte BattleScript_EffectOHKO                   @ EFFECT_OHKO
	.4byte BattleScript_EffectRazorWind              @ EFFECT_RAZOR_WIND
	.4byte BattleScript_EffectSuperFang              @ EFFECT_SUPER_FANG
	.4byte BattleScript_EffectDragonRage             @ EFFECT_DRAGON_RAGE
	.4byte BattleScript_EffectTrap                   @ EFFECT_TRAP
	.4byte BattleScript_EffectHit                    @ EFFECT_HIGH_CRITICAL
	.4byte BattleScript_EffectDoubleHit              @ EFFECT_DOUBLE_HIT
	.4byte BattleScript_EffectRecoilIfMiss           @ EFFECT_RECOIL_IF_MISS
	.4byte BattleScript_EffectMist                   @ EFFECT_MIST
	.4byte BattleScript_EffectFocusEnergy            @ EFFECT_FOCUS_ENERGY
	.4byte BattleScript_EffectRecoil                 @ EFFECT_RECOIL
	.4byte BattleScript_EffectConfuse                @ EFFECT_CONFUSE
	.4byte BattleScript_EffectAttackUp2              @ EFFECT_ATTACK_UP_2
	.4byte BattleScript_EffectDefenseUp2             @ EFFECT_DEFENSE_UP_2
	.4byte BattleScript_EffectSpeedUp2               @ EFFECT_SPEED_UP_2
	.4byte BattleScript_EffectSpecialAttackUp2       @ EFFECT_SPECIAL_ATTACK_UP_2
	.4byte BattleScript_EffectSpecialDefenseUp2      @ EFFECT_SPECIAL_DEFENSE_UP_2
	.4byte BattleScript_EffectHit                    @ EFFECT_ACCURACY_UP_2
	.4byte BattleScript_EffectHit                    @ EFFECT_EVASION_UP_2
	.4byte BattleScript_EffectTransform              @ EFFECT_TRANSFORM
	.4byte BattleScript_EffectAttackDown2            @ EFFECT_ATTACK_DOWN_2
	.4byte BattleScript_EffectDefenseDown2           @ EFFECT_DEFENSE_DOWN_2
	.4byte BattleScript_EffectSpeedDown2             @ EFFECT_SPEED_DOWN_2
	.4byte BattleScript_EffectHit                    @ EFFECT_SPECIAL_ATTACK_DOWN_2
	.4byte BattleScript_EffectSpecialDefenseDown2    @ EFFECT_SPECIAL_DEFENSE_DOWN_2
	.4byte BattleScript_EffectHit                    @ EFFECT_ACCURACY_DOWN_2
	.4byte BattleScript_EffectHit                    @ EFFECT_EVASION_DOWN_2
	.4byte BattleScript_EffectReflect                @ EFFECT_REFLECT
	.4byte BattleScript_EffectPoison                 @ EFFECT_POISON
	.4byte BattleScript_EffectParalyze               @ EFFECT_PARALYZE
	.4byte BattleScript_EffectAttackDownHit          @ EFFECT_ATTACK_DOWN_HIT
	.4byte BattleScript_EffectDefenseDownHit         @ EFFECT_DEFENSE_DOWN_HIT
	.4byte BattleScript_EffectSpeedDownHit           @ EFFECT_SPEED_DOWN_HIT
	.4byte BattleScript_EffectSpecialAttackDownHit   @ EFFECT_SPECIAL_ATTACK_DOWN_HIT
	.4byte BattleScript_EffectSpecialDefenseDownHit  @ EFFECT_SPECIAL_DEFENSE_DOWN_HIT
	.4byte BattleScript_EffectAccuracyDownHit        @ EFFECT_ACCURACY_DOWN_HIT
	.4byte BattleScript_EffectHit                    @ EFFECT_EVASION_DOWN_HIT
	.4byte BattleScript_EffectSkyAttack              @ EFFECT_SKY_ATTACK
	.4byte BattleScript_EffectConfuseHit             @ EFFECT_CONFUSE_HIT
	.4byte BattleScript_EffectTwineedle              @ EFFECT_TWINEEDLE
	.4byte BattleScript_EffectHit                    @ EFFECT_VITAL_THROW
	.4byte BattleScript_EffectSubstitute             @ EFFECT_SUBSTITUTE
	.4byte BattleScript_EffectRecharge               @ EFFECT_RECHARGE
	.4byte BattleScript_EffectRage                   @ EFFECT_RAGE
	.4byte BattleScript_EffectMimic                  @ EFFECT_MIMIC
	.4byte BattleScript_EffectMetronome              @ EFFECT_METRONOME
	.4byte BattleScript_EffectLeechSeed              @ EFFECT_LEECH_SEED
	.4byte BattleScript_EffectSplash                 @ EFFECT_SPLASH
	.4byte BattleScript_EffectDisable                @ EFFECT_DISABLE
	.4byte BattleScript_EffectLevelDamage            @ EFFECT_LEVEL_DAMAGE
	.4byte BattleScript_EffectPsywave                @ EFFECT_PSYWAVE
	.4byte BattleScript_EffectCounter                @ EFFECT_COUNTER
	.4byte BattleScript_EffectEncore                 @ EFFECT_ENCORE
	.4byte BattleScript_EffectPainSplit              @ EFFECT_PAIN_SPLIT
	.4byte BattleScript_EffectSnore                  @ EFFECT_SNORE
	.4byte BattleScript_EffectConversion2            @ EFFECT_CONVERSION_2
	.4byte BattleScript_EffectLockOn                 @ EFFECT_LOCK_ON
	.4byte BattleScript_EffectSketch                 @ EFFECT_SKETCH
	.4byte BattleScript_EffectHit                    @ EFFECT_UNUSED_60
	.4byte BattleScript_EffectSleepTalk              @ EFFECT_SLEEP_TALK
	.4byte BattleScript_EffectDestinyBond            @ EFFECT_DESTINY_BOND
	.4byte BattleScript_EffectFlail                  @ EFFECT_FLAIL
	.4byte BattleScript_EffectSpite                  @ EFFECT_SPITE
	.4byte BattleScript_EffectHit                    @ EFFECT_FALSE_SWIPE
	.4byte BattleScript_EffectHealBell               @ EFFECT_HEAL_BELL
	.4byte BattleScript_EffectHit                    @ EFFECT_QUICK_ATTACK
	.4byte BattleScript_EffectTripleKick             @ EFFECT_TRIPLE_KICK
	.4byte BattleScript_EffectThief                  @ EFFECT_THIEF
	.4byte BattleScript_EffectMeanLook               @ EFFECT_MEAN_LOOK
	.4byte BattleScript_EffectNightmare              @ EFFECT_NIGHTMARE
	.4byte BattleScript_EffectMinimize               @ EFFECT_MINIMIZE
	.4byte BattleScript_EffectCurse                  @ EFFECT_CURSE
	.4byte BattleScript_EffectHit                    @ EFFECT_UNUSED_6E
	.4byte BattleScript_EffectProtect                @ EFFECT_PROTECT
	.4byte BattleScript_EffectSpikes                 @ EFFECT_SPIKES
	.4byte BattleScript_EffectForesight              @ EFFECT_FORESIGHT
	.4byte BattleScript_EffectPerishSong             @ EFFECT_PERISH_SONG
	.4byte BattleScript_EffectSandstorm              @ EFFECT_SANDSTORM
	.4byte BattleScript_EffectEndure                 @ EFFECT_ENDURE
	.4byte BattleScript_EffectRollout                @ EFFECT_ROLLOUT
	.4byte BattleScript_EffectSwagger                @ EFFECT_SWAGGER
	.4byte BattleScript_EffectFuryCutter             @ EFFECT_FURY_CUTTER
	.4byte BattleScript_EffectAttract                @ EFFECT_ATTRACT
	.4byte BattleScript_EffectReturn                 @ EFFECT_RETURN
	.4byte BattleScript_EffectPresent                @ EFFECT_PRESENT
	.4byte BattleScript_EffectFrustration            @ EFFECT_FRUSTRATION
	.4byte BattleScript_EffectSafeguard              @ EFFECT_SAFEGUARD
	.4byte BattleScript_EffectThawHit                @ EFFECT_THAW_HIT
	.4byte BattleScript_EffectMagnitude              @ EFFECT_MAGNITUDE
	.4byte BattleScript_EffectBatonPass              @ EFFECT_BATON_PASS
	.4byte BattleScript_EffectHit                    @ EFFECT_PURSUIT
	.4byte BattleScript_EffectRapidSpin              @ EFFECT_RAPID_SPIN
	.4byte BattleScript_EffectSonicboom              @ EFFECT_SONICBOOM
	.4byte BattleScript_EffectHit                    @ EFFECT_UNUSED_83
	.4byte BattleScript_EffectMorningSun             @ EFFECT_MORNING_SUN
	.4byte BattleScript_EffectSynthesis              @ EFFECT_SYNTHESIS
	.4byte BattleScript_EffectMoonlight              @ EFFECT_MOONLIGHT
	.4byte BattleScript_EffectHiddenPower            @ EFFECT_HIDDEN_POWER
	.4byte BattleScript_EffectRainDance              @ EFFECT_RAIN_DANCE
	.4byte BattleScript_EffectSunnyDay               @ EFFECT_SUNNY_DAY
	.4byte BattleScript_EffectDefenseUpHit           @ EFFECT_DEFENSE_UP_HIT
	.4byte BattleScript_EffectAttackUpHit            @ EFFECT_ATTACK_UP_HIT
	.4byte BattleScript_EffectAllStatsUpHit          @ EFFECT_ALL_STATS_UP_HIT
	.4byte BattleScript_EffectHit                    @ EFFECT_UNUSED_8D
	.4byte BattleScript_EffectBellyDrum              @ EFFECT_BELLY_DRUM
	.4byte BattleScript_EffectPsychUp                @ EFFECT_PSYCH_UP
	.4byte BattleScript_EffectMirrorCoat             @ EFFECT_MIRROR_COAT
	.4byte BattleScript_EffectSkullBash              @ EFFECT_SKULL_BASH
	.4byte BattleScript_EffectTwister                @ EFFECT_TWISTER
	.4byte BattleScript_EffectEarthquake             @ EFFECT_EARTHQUAKE
	.4byte BattleScript_EffectFutureSight            @ EFFECT_FUTURE_SIGHT
	.4byte BattleScript_EffectGust                   @ EFFECT_GUST
	.4byte BattleScript_EffectStomp                  @ EFFECT_FLINCH_MINIMIZE_HIT
	.4byte BattleScript_EffectSolarBeam              @ EFFECT_SOLAR_BEAM
	.4byte BattleScript_EffectThunder                @ EFFECT_THUNDER
	.4byte BattleScript_EffectTeleport               @ EFFECT_TELEPORT
	.4byte BattleScript_EffectBeatUp                 @ EFFECT_BEAT_UP
	.4byte BattleScript_EffectSemiInvulnerable       @ EFFECT_SEMI_INVULNERABLE
	.4byte BattleScript_EffectDefenseCurl            @ EFFECT_DEFENSE_CURL
	.4byte BattleScript_EffectSoftboiled             @ EFFECT_SOFTBOILED
	.4byte BattleScript_EffectFakeOut                @ EFFECT_FAKE_OUT
	.4byte BattleScript_EffectUproar                 @ EFFECT_UPROAR
	.4byte BattleScript_EffectStockpile              @ EFFECT_STOCKPILE
	.4byte BattleScript_EffectSpitUp                 @ EFFECT_SPIT_UP
	.4byte BattleScript_EffectSwallow                @ EFFECT_SWALLOW
	.4byte BattleScript_EffectHit                    @ EFFECT_UNUSED_A3
	.4byte BattleScript_EffectHail                   @ EFFECT_HAIL
	.4byte BattleScript_EffectTorment                @ EFFECT_TORMENT
	.4byte BattleScript_EffectFlatter                @ EFFECT_FLATTER
	.4byte BattleScript_EffectWillOWisp              @ EFFECT_WILL_O_WISP
	.4byte BattleScript_EffectMemento                @ EFFECT_MEMENTO
	.4byte BattleScript_EffectFacade                 @ EFFECT_FACADE
	.4byte BattleScript_EffectFocusPunch             @ EFFECT_FOCUS_PUNCH
	.4byte BattleScript_EffectSmellingsalt           @ EFFECT_SMELLINGSALT
	.4byte BattleScript_EffectFollowMe               @ EFFECT_FOLLOW_ME
	.4byte BattleScript_EffectNaturePower            @ EFFECT_NATURE_POWER
	.4byte BattleScript_EffectCharge                 @ EFFECT_CHARGE
	.4byte BattleScript_EffectTaunt                  @ EFFECT_TAUNT
	.4byte BattleScript_EffectHelpingHand            @ EFFECT_HELPING_HAND
	.4byte BattleScript_EffectTrick                  @ EFFECT_TRICK
	.4byte BattleScript_EffectRolePlay               @ EFFECT_ROLE_PLAY
	.4byte BattleScript_EffectWish                   @ EFFECT_WISH
	.4byte BattleScript_EffectAssist                 @ EFFECT_ASSIST
	.4byte BattleScript_EffectIngrain                @ EFFECT_INGRAIN
	.4byte BattleScript_EffectSuperpower             @ EFFECT_SUPERPOWER
	.4byte BattleScript_EffectMagicCoat              @ EFFECT_MAGIC_COAT
	.4byte BattleScript_EffectRecycle                @ EFFECT_RECYCLE
	.4byte BattleScript_EffectRevenge                @ EFFECT_REVENGE
	.4byte BattleScript_EffectBrickBreak             @ EFFECT_BRICK_BREAK
	.4byte BattleScript_EffectYawn                   @ EFFECT_YAWN
	.4byte BattleScript_EffectKnockOff               @ EFFECT_KNOCK_OFF
	.4byte BattleScript_EffectEndeavor               @ EFFECT_ENDEAVOR
	.4byte BattleScript_EffectEruption               @ EFFECT_ERUPTION
	.4byte BattleScript_EffectSkillSwap              @ EFFECT_SKILL_SWAP
	.4byte BattleScript_EffectImprison               @ EFFECT_IMPRISON
	.4byte BattleScript_EffectRefresh                @ EFFECT_REFRESH
	.4byte BattleScript_EffectGrudge                 @ EFFECT_GRUDGE
	.4byte BattleScript_EffectSnatch                 @ EFFECT_SNATCH
	.4byte BattleScript_EffectLowKick                @ EFFECT_LOW_KICK
	.4byte BattleScript_EffectSecretPower            @ EFFECT_SECRET_POWER
	.4byte BattleScript_EffectDoubleEdge             @ EFFECT_DOUBLE_EDGE
	.4byte BattleScript_EffectTeeterDance            @ EFFECT_TEETER_DANCE
	.4byte BattleScript_EffectBurnHit                @ EFFECT_BLAZE_KICK
	.4byte BattleScript_EffectMudSport               @ EFFECT_MUD_SPORT
	.4byte BattleScript_EffectPoisonFang             @ EFFECT_POISON_FANG
	.4byte BattleScript_EffectWeatherBall            @ EFFECT_WEATHER_BALL
	.4byte BattleScript_EffectOverheat               @ EFFECT_OVERHEAT
	.4byte BattleScript_EffectTickle                 @ EFFECT_TICKLE
	.4byte BattleScript_EffectCosmicPower            @ EFFECT_COSMIC_POWER
	.4byte BattleScript_EffectSkyUppercut            @ EFFECT_SKY_UPPERCUT
	.4byte BattleScript_EffectBulkUp                 @ EFFECT_BULK_UP
	.4byte BattleScript_EffectPoisonHit              @ EFFECT_POISON_TAIL
	.4byte BattleScript_EffectWaterSport             @ EFFECT_WATER_SPORT
	.4byte BattleScript_EffectCalmMind               @ EFFECT_CALM_MIND
	.4byte BattleScript_EffectDragonDance            @ EFFECT_DRAGON_DANCE
	.4byte BattleScript_EffectCamouflage             @ EFFECT_CAMOUFLAGE

BattleScript_EffectHit::
	jumpifnotmove MOVE_SURF, BattleScript_HitFromAtkCanceler
	jumpifnostatus3 BS_TARGET, STATUS3_UNDERWATER, BattleScript_HitFromAtkCanceler
	orword gHitMarker, HITMARKER_IGNORE_UNDERWATER
	setbyte sDMG_MULTIPLIER, 2
BattleScript_HitFromAtkCanceler::
	attackcanceler
BattleScript_HitFromAccCheck::
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
BattleScript_HitFromAtkString::
	attackstring
	ppreduce
BattleScript_HitFromCritCalc::
	critcalc
	damagecalc
	typecalc
	adjustnormaldamage
BattleScript_HitFromAtkAnimation::
	attackanimation
	waitanimation
	effectivenesssound
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	critmessage
	waitmessage B_WAIT_TIME_LONG
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	seteffectwithchance
	tryfaintmon BS_TARGET
BattleScript_MoveEnd::
	moveendall
	end

BattleScript_MakeMoveMissed::
	orbyte gMoveResultFlags, MOVE_RESULT_MISSED
BattleScript_PrintMoveMissed::
	attackstring
	ppreduce
BattleScript_MoveMissedPause::
	pause B_WAIT_TIME_SHORT
BattleScript_MoveMissed::
	effectivenesssound
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectSleep::
	attackcanceler
	attackstring
	ppreduce
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	jumpifstatus BS_TARGET, STATUS1_SLEEP, BattleScript_AlreadyAsleep
	jumpifcantmakeasleep BattleScript_CantMakeAsleep
	jumpifstatus BS_TARGET, STATUS1_ANY, BattleScript_ButItFailed
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	jumpifsideaffecting BS_TARGET, SIDE_STATUS_SAFEGUARD, BattleScript_SafeguardProtected
	attackanimation
	waitanimation
	setmoveeffect MOVE_EFFECT_SLEEP
	seteffectprimary
	goto BattleScript_MoveEnd

BattleScript_AlreadyAsleep::
	setalreadystatusedmoveattempt BS_ATTACKER
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNALREADYASLEEP
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_WasntAffected::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNWASNTAFFECTED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_CantMakeAsleep::
	pause B_WAIT_TIME_SHORT
	printfromtable gUproarAwakeStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectPoisonHit::
	setmoveeffect MOVE_EFFECT_POISON
	goto BattleScript_EffectHit

BattleScript_EffectAbsorb::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	critcalc
	damagecalc
	typecalc
	adjustnormaldamage
	attackanimation
	waitanimation
	effectivenesssound
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	critmessage
	waitmessage B_WAIT_TIME_LONG
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	negativedamage
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	jumpifability BS_TARGET, ABILITY_LIQUID_OOZE, BattleScript_AbsorbLiquidOoze
	setbyte cMULTISTRING_CHOOSER, B_MSG_ABSORB
	goto BattleScript_AbsorbUpdateHp
BattleScript_AbsorbLiquidOoze::
	manipulatedamage DMG_CHANGE_SIGN
	setbyte cMULTISTRING_CHOOSER, B_MSG_ABSORB_OOZE
BattleScript_AbsorbUpdateHp::
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	jumpifmovehadnoeffect BattleScript_AbsorbTryFainting
	printfromtable gAbsorbDrainStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_AbsorbTryFainting::
	tryfaintmon BS_ATTACKER
	tryfaintmon BS_TARGET
	goto BattleScript_MoveEnd

BattleScript_EffectBurnHit::
	setmoveeffect MOVE_EFFECT_BURN
	goto BattleScript_EffectHit

BattleScript_EffectFreezeHit::
	setmoveeffect MOVE_EFFECT_FREEZE
	goto BattleScript_EffectHit

BattleScript_EffectParalyzeHit::
	setmoveeffect MOVE_EFFECT_PARALYSIS
	goto BattleScript_EffectHit

BattleScript_EffectExplosion::
	attackcanceler
	attackstring
	ppreduce
@ Below jumps to BattleScript_DampStopsExplosion if it fails (only way it can)
	tryexplosion
	setatkhptozero
	waitstate
	jumpifbyte CMP_NO_COMMON_BITS, gMoveResultFlags, MOVE_RESULT_MISSED, BattleScript_ExplosionDoAnimStartLoop
	call BattleScript_PreserveMissedBitDoMoveAnim
	goto BattleScript_ExplosionLoop
BattleScript_ExplosionDoAnimStartLoop:
	attackanimation
	waitanimation
BattleScript_ExplosionLoop:
	movevaluescleanup
	critcalc
	damagecalc
	typecalc
	adjustnormaldamage
	accuracycheck BattleScript_ExplosionMissed, ACC_CURR_MOVE
	effectivenesssound
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	critmessage
	waitmessage B_WAIT_TIME_LONG
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	tryfaintmon BS_TARGET
	moveendto MOVEEND_NEXT_TARGET
	jumpifnexttargetvalid BattleScript_ExplosionLoop
	tryfaintmon BS_ATTACKER
	end
BattleScript_ExplosionMissed:
	effectivenesssound
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	moveendto MOVEEND_NEXT_TARGET
	jumpifnexttargetvalid BattleScript_ExplosionLoop
	tryfaintmon BS_ATTACKER
	end

BattleScript_PreserveMissedBitDoMoveAnim:
	bicbyte gMoveResultFlags, MOVE_RESULT_MISSED
	attackanimation
	waitanimation
	orbyte gMoveResultFlags, MOVE_RESULT_MISSED
	return

BattleScript_EffectDreamEater::
	attackcanceler
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_DreamEaterNoEffect
	jumpifstatus BS_TARGET, STATUS1_SLEEP, BattleScript_DreamEaterWorked
BattleScript_DreamEaterNoEffect:
	attackstring
	ppreduce
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_WasntAffected
BattleScript_DreamEaterWorked:
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	critcalc
	damagecalc
	typecalc
	adjustnormaldamage
	attackanimation
	waitanimation
	effectivenesssound
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	critmessage
	waitmessage B_WAIT_TIME_LONG
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	negativedamage
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	jumpifmovehadnoeffect BattleScript_DreamEaterTryFaintEnd
	printstring STRINGID_PKMNDREAMEATEN
	waitmessage B_WAIT_TIME_LONG
BattleScript_DreamEaterTryFaintEnd:
	tryfaintmon BS_TARGET
	goto BattleScript_MoveEnd

BattleScript_EffectMirrorMove::
	attackcanceler
	attackstring
	pause B_WAIT_TIME_LONG
	trymirrormove
	ppreduce
	orbyte gMoveResultFlags, MOVE_RESULT_FAILED
	printstring STRINGID_MIRRORMOVEFAILED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectAttackUp::
	setstatchanger STAT_ATK, 1, FALSE
	goto BattleScript_EffectStatUp

BattleScript_EffectDefenseUp::
	setstatchanger STAT_DEF, 1, FALSE
	goto BattleScript_EffectStatUp

BattleScript_EffectSpecialAttackUp::
	setstatchanger STAT_SPATK, 1, FALSE
	goto BattleScript_EffectStatUp

BattleScript_EffectEvasionUp::
	setstatchanger STAT_EVASION, 1, FALSE
BattleScript_EffectStatUp::
	attackcanceler
BattleScript_EffectStatUpAfterAtkCanceler::
	attackstring
	ppreduce
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_StatUpEnd
	jumpifbyte CMP_NOT_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_StatUpAttackAnim
	pause B_WAIT_TIME_SHORT
	goto BattleScript_StatUpPrintString
BattleScript_StatUpAttackAnim::
	attackanimation
	waitanimation
BattleScript_StatUpDoAnim::
	setgraphicalstatchangevalues
	playanimation BS_ATTACKER, B_ANIM_STATS_CHANGE, sB_ANIM_ARG1
BattleScript_StatUpPrintString::
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_StatUpEnd::
	goto BattleScript_MoveEnd

BattleScript_StatUp::
	playanimation BS_EFFECT_BATTLER, B_ANIM_STATS_CHANGE, sB_ANIM_ARG1
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_EffectAttackDown::
	setstatchanger STAT_ATK, 1, TRUE
	goto BattleScript_EffectStatDown

BattleScript_EffectDefenseDown::
	setstatchanger STAT_DEF, 1, TRUE
	goto BattleScript_EffectStatDown

BattleScript_EffectSpeedDown::
	setstatchanger STAT_SPEED, 1, TRUE
	goto BattleScript_EffectStatDown

BattleScript_EffectAccuracyDown::
	setstatchanger STAT_ACC, 1, TRUE
	goto BattleScript_EffectStatDown

BattleScript_EffectEvasionDown::
	setstatchanger STAT_EVASION, 1, TRUE
BattleScript_EffectStatDown::
	attackcanceler
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_FailedFromAtkString
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	statbuffchange STAT_CHANGE_ALLOW_PTR, BattleScript_StatDownEnd
	jumpifbyte CMP_LESS_THAN, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_DECREASE, BattleScript_StatDownDoAnim
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_FELL_EMPTY, BattleScript_StatDownEnd
	pause B_WAIT_TIME_SHORT
	goto BattleScript_StatDownPrintString
BattleScript_StatDownDoAnim::
	attackanimation
	waitanimation
	setgraphicalstatchangevalues
	playanimation BS_TARGET, B_ANIM_STATS_CHANGE, sB_ANIM_ARG1
BattleScript_StatDownPrintString::
	printfromtable gStatDownStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_StatDownEnd::
	goto BattleScript_MoveEnd

BattleScript_StatDown::
	playanimation BS_EFFECT_BATTLER, B_ANIM_STATS_CHANGE, sB_ANIM_ARG1
	printfromtable gStatDownStringIds
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_EffectHaze::
	attackcanceler
	attackstring
	ppreduce
	attackanimation
	waitanimation
	normalisebuffs
	printstring STRINGID_STATCHANGESGONE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectBide::
	attackcanceler
	attackstring
	ppreduce
	attackanimation
	waitanimation
	orword gHitMarker, HITMARKER_CHARGING
	setbide
	goto BattleScript_MoveEnd

BattleScript_EffectRampage::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	jumpifstatus2 BS_ATTACKER, STATUS2_MULTIPLETURNS, BattleScript_EffectRampage2
	ppreduce
BattleScript_EffectRampage2:
	confuseifrepeatingattackends
	goto BattleScript_HitFromCritCalc

BattleScript_EffectRoar::
	attackcanceler
	attackstring
	ppreduce
	jumpifability BS_TARGET, ABILITY_SUCTION_CUPS, BattleScript_AbilityPreventsPhasingOut
	jumpifstatus3 BS_TARGET, STATUS3_ROOTED, BattleScript_PrintMonIsRooted
	accuracycheck BattleScript_ButItFailed, NO_ACC_CALC_CHECK_LOCK_ON
	accuracycheck BattleScript_MoveMissedPause, ACC_CURR_MOVE
	jumpifbattletype BATTLE_TYPE_ARENA, BattleScript_ButItFailed
	forcerandomswitch BattleScript_ButItFailed

BattleScript_EffectMultiHit::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	setmultihitcounter 0
	initmultihitstring
	setbyte sMULTIHIT_EFFECT, 0
BattleScript_MultiHitLoop::
	jumpifhasnohp BS_ATTACKER, BattleScript_MultiHitEnd
	jumpifhasnohp BS_TARGET, BattleScript_MultiHitPrintStrings
	jumpifhalfword CMP_EQUAL, gChosenMove, MOVE_SLEEP_TALK, BattleScript_DoMultiHit
	jumpifstatus BS_ATTACKER, STATUS1_SLEEP, BattleScript_MultiHitPrintStrings
BattleScript_DoMultiHit::
	movevaluescleanup
	copybyte cEFFECT_CHOOSER, sMULTIHIT_EFFECT
	critcalc
	damagecalc
	typecalc
	jumpifmovehadnoeffect BattleScript_MultiHitNoMoreHits
	adjustnormaldamage
	attackanimation
	waitanimation
	effectivenesssound
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	critmessage
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_EMPTYSTRING3
	waitmessage 1
	addbyte sMULTIHIT_STRING + 4, 1
	moveendto MOVEEND_NEXT_TARGET
	jumpifbyte CMP_COMMON_BITS, gMoveResultFlags, MOVE_RESULT_FOE_ENDURED, BattleScript_MultiHitPrintStrings
	decrementmultihit BattleScript_MultiHitLoop
	goto BattleScript_MultiHitPrintStrings
BattleScript_MultiHitNoMoreHits::
	pause B_WAIT_TIME_SHORT
BattleScript_MultiHitPrintStrings::
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	jumpifmovehadnoeffect BattleScript_MultiHitEnd
	copyarray gBattleTextBuff1, sMULTIHIT_STRING, 6
	printstring STRINGID_HITXTIMES
	waitmessage B_WAIT_TIME_LONG
BattleScript_MultiHitEnd::
	seteffectwithchance
	tryfaintmon BS_TARGET
	moveendcase MOVEEND_SYNCHRONIZE_TARGET
	moveendfrom MOVEEND_IMMUNITY_ABILITIES
	end

BattleScript_EffectConversion::
	attackcanceler
	attackstring
	ppreduce
	tryconversiontypechange BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNCHANGEDTYPE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectFlinchHit::
	setmoveeffect MOVE_EFFECT_FLINCH
	goto BattleScript_EffectHit

BattleScript_EffectRestoreHp::
	attackcanceler
	attackstring
	ppreduce
	tryhealhalfhealth BattleScript_AlreadyAtFullHp, BS_ATTACKER
	attackanimation
	waitanimation
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	printstring STRINGID_PKMNREGAINEDHEALTH
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectToxic::
	attackcanceler
	attackstring
	ppreduce
	jumpifability BS_TARGET, ABILITY_IMMUNITY, BattleScript_ImmunityProtected
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	jumpifstatus BS_TARGET, STATUS1_POISON, BattleScript_AlreadyPoisoned
	jumpifstatus BS_TARGET, STATUS1_TOXIC_POISON, BattleScript_AlreadyPoisoned
	jumpifstatus BS_TARGET, STATUS1_ANY, BattleScript_ButItFailed
	jumpiftype BS_TARGET, TYPE_POISON, BattleScript_NotAffected
	jumpiftype BS_TARGET, TYPE_STEEL, BattleScript_NotAffected
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	jumpifsideaffecting BS_TARGET, SIDE_STATUS_SAFEGUARD, BattleScript_SafeguardProtected
	attackanimation
	waitanimation
	setmoveeffect MOVE_EFFECT_TOXIC
	seteffectprimary
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_AlreadyPoisoned::
	setalreadystatusedmoveattempt BS_ATTACKER
	pause B_WAIT_TIME_LONG
	printstring STRINGID_PKMNALREADYPOISONED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_ImmunityProtected::
	copybyte gEffectBattler, gBattlerTarget
	setbyte cMULTISTRING_CHOOSER, B_MSG_ABILITY_PREVENTS_MOVE_STATUS
	call BattleScript_PSNPrevention
	goto BattleScript_MoveEnd

BattleScript_EffectPayDay::
	setmoveeffect MOVE_EFFECT_PAYDAY
	goto BattleScript_EffectHit

BattleScript_EffectLightScreen::
	attackcanceler
	attackstring
	ppreduce
	setlightscreen
	goto BattleScript_PrintReflectLightScreenSafeguardString

BattleScript_EffectTriAttack::
	setmoveeffect MOVE_EFFECT_TRI_ATTACK
	goto BattleScript_EffectHit

BattleScript_EffectRest::
	attackcanceler
	attackstring
	ppreduce
	jumpifstatus BS_ATTACKER, STATUS1_SLEEP, BattleScript_RestIsAlreadyAsleep
	jumpifcantmakeasleep BattleScript_RestCantSleep
	trysetrest BattleScript_AlreadyAtFullHp
	pause B_WAIT_TIME_SHORT
	printfromtable gRestUsedStringIds
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_ATTACKER
	waitstate
	goto BattleScript_PresentHealTarget

BattleScript_RestCantSleep::
	pause B_WAIT_TIME_LONG
	printfromtable gUproarAwakeStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_RestIsAlreadyAsleep::
	setalreadystatusedmoveattempt BS_ATTACKER
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNALREADYASLEEP2
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectOHKO::
	attackcanceler
	attackstring
	ppreduce
	accuracycheck BattleScript_ButItFailed, NO_ACC_CALC_CHECK_LOCK_ON
	typecalc
	jumpifmovehadnoeffect BattleScript_HitFromAtkAnimation
	tryKO BattleScript_KOFail
	trysetdestinybondtohappen
	goto BattleScript_HitFromAtkAnimation
BattleScript_KOFail::
	pause B_WAIT_TIME_LONG
	printfromtable gKOFailedStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectRazorWind::
	jumpifstatus2 BS_ATTACKER, STATUS2_MULTIPLETURNS, BattleScript_TwoTurnMovesSecondTurn
	jumpifword CMP_COMMON_BITS, gHitMarker, HITMARKER_NO_ATTACKSTRING, BattleScript_TwoTurnMovesSecondTurn
	setbyte sTWOTURN_STRINGID, B_MSG_TURN1_RAZOR_WIND
	call BattleScriptFirstChargingTurn
	goto BattleScript_MoveEnd

BattleScript_TwoTurnMovesSecondTurn::
	attackcanceler
	setmoveeffect MOVE_EFFECT_CHARGING
	setbyte sB_ANIM_TURN, 1
	clearstatusfromeffect BS_ATTACKER
	orword gHitMarker, HITMARKER_NO_PPDEDUCT
	jumpifnotmove MOVE_SKY_ATTACK, BattleScript_HitFromAccCheck
	setmoveeffect MOVE_EFFECT_FLINCH
	goto BattleScript_HitFromAccCheck

BattleScriptFirstChargingTurn::
	attackcanceler
	printstring STRINGID_EMPTYSTRING3
	ppreduce
	attackanimation
	waitanimation
	orword gHitMarker, HITMARKER_CHARGING
	setmoveeffect MOVE_EFFECT_CHARGING | MOVE_EFFECT_AFFECTS_USER
	seteffectprimary
	copybyte cMULTISTRING_CHOOSER, sTWOTURN_STRINGID
	printfromtable gFirstTurnOfTwoStringIds
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_EffectSuperFang::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	typecalc
	bicbyte gMoveResultFlags, MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE
	damagetohalftargethp
	goto BattleScript_HitFromAtkAnimation

BattleScript_EffectDragonRage::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	typecalc
	bicbyte gMoveResultFlags, MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE
	setword gBattleMoveDamage, 40
	adjustsetdamage
	goto BattleScript_HitFromAtkAnimation

BattleScript_EffectTrap::
	jumpifnotmove MOVE_WHIRLPOOL, BattleScript_DoWrapEffect
	jumpifnostatus3 BS_TARGET, STATUS3_UNDERWATER, BattleScript_DoWrapEffect
	orword gHitMarker, HITMARKER_IGNORE_UNDERWATER
	setbyte sDMG_MULTIPLIER, 2
BattleScript_DoWrapEffect::
	setmoveeffect MOVE_EFFECT_WRAP
	goto BattleScript_EffectHit

BattleScript_EffectDoubleHit::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	setmultihitcounter 2
	initmultihitstring
	setbyte sMULTIHIT_EFFECT, 0
	goto BattleScript_MultiHitLoop

BattleScript_EffectRecoilIfMiss::
	attackcanceler
	accuracycheck BattleScript_MoveMissedDoDamage, ACC_CURR_MOVE
	goto BattleScript_HitFromAtkString
BattleScript_MoveMissedDoDamage::
	attackstring
	ppreduce
	pause B_WAIT_TIME_LONG
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	jumpifbyte CMP_COMMON_BITS, gMoveResultFlags, MOVE_RESULT_DOESNT_AFFECT_FOE, BattleScript_MoveEnd
	printstring STRINGID_PKMNCRASHED
	waitmessage B_WAIT_TIME_LONG
	damagecalc
	typecalc
	adjustnormaldamage
	manipulatedamage DMG_RECOIL_FROM_MISS
	bicbyte gMoveResultFlags, MOVE_RESULT_MISSED
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	tryfaintmon BS_ATTACKER
	orbyte gMoveResultFlags, MOVE_RESULT_MISSED
	goto BattleScript_MoveEnd

BattleScript_EffectMist::
	attackcanceler
	attackstring
	ppreduce
	setmist
	attackanimation
	waitanimation
	printfromtable gMistUsedStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectFocusEnergy::
	attackcanceler
	attackstring
	ppreduce
	jumpifstatus2 BS_ATTACKER, STATUS2_FOCUS_ENERGY, BattleScript_ButItFailed
	setfocusenergy
	attackanimation
	waitanimation
	printfromtable gFocusEnergyUsedStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectRecoil::
	setmoveeffect MOVE_EFFECT_RECOIL_25 | MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN
	jumpifnotmove MOVE_STRUGGLE, BattleScript_EffectHit
	incrementgamestat GAME_STAT_USED_STRUGGLE
	goto BattleScript_EffectHit

BattleScript_EffectConfuse::
	attackcanceler
	attackstring
	ppreduce
	jumpifability BS_TARGET, ABILITY_OWN_TEMPO, BattleScript_OwnTempoPrevents
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	jumpifstatus2 BS_TARGET, STATUS2_CONFUSION, BattleScript_AlreadyConfused
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	jumpifsideaffecting BS_TARGET, SIDE_STATUS_SAFEGUARD, BattleScript_SafeguardProtected
	attackanimation
	waitanimation
	setmoveeffect MOVE_EFFECT_CONFUSION
	seteffectprimary
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_AlreadyConfused::
	setalreadystatusedmoveattempt BS_ATTACKER
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNALREADYCONFUSED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectAttackUp2::
	setstatchanger STAT_ATK, 2, FALSE
	goto BattleScript_EffectStatUp

BattleScript_EffectDefenseUp2::
	setstatchanger STAT_DEF, 2, FALSE
	goto BattleScript_EffectStatUp

BattleScript_EffectSpeedUp2::
	setstatchanger STAT_SPEED, 2, FALSE
	goto BattleScript_EffectStatUp

BattleScript_EffectSpecialAttackUp2::
	setstatchanger STAT_SPATK, 2, FALSE
	goto BattleScript_EffectStatUp

BattleScript_EffectSpecialDefenseUp2::
	setstatchanger STAT_SPDEF, 2, FALSE
	goto BattleScript_EffectStatUp

BattleScript_EffectTransform::
	attackcanceler
	attackstring
	ppreduce
	transformdataexecution
	attackanimation
	waitanimation
	printfromtable gTransformUsedStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectAttackDown2::
	setstatchanger STAT_ATK, 2, TRUE
	goto BattleScript_EffectStatDown

BattleScript_EffectDefenseDown2::
	setstatchanger STAT_DEF, 2, TRUE
	goto BattleScript_EffectStatDown

BattleScript_EffectSpeedDown2::
	setstatchanger STAT_SPEED, 2, TRUE
	goto BattleScript_EffectStatDown

BattleScript_EffectSpecialDefenseDown2::
	setstatchanger STAT_SPDEF, 2, TRUE
	goto BattleScript_EffectStatDown

BattleScript_EffectReflect::
	attackcanceler
	attackstring
	ppreduce
	setreflect
BattleScript_PrintReflectLightScreenSafeguardString::
	attackanimation
	waitanimation
	printfromtable gReflectLightScreenSafeguardStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectPoison::
	attackcanceler
	attackstring
	ppreduce
	jumpifability BS_TARGET, ABILITY_IMMUNITY, BattleScript_ImmunityProtected
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	jumpifstatus BS_TARGET, STATUS1_POISON, BattleScript_AlreadyPoisoned
	jumpifstatus BS_TARGET, STATUS1_TOXIC_POISON, BattleScript_AlreadyPoisoned
	jumpiftype BS_TARGET, TYPE_POISON, BattleScript_NotAffected
	jumpiftype BS_TARGET, TYPE_STEEL, BattleScript_NotAffected
	jumpifstatus BS_TARGET, STATUS1_ANY, BattleScript_ButItFailed
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	jumpifsideaffecting BS_TARGET, SIDE_STATUS_SAFEGUARD, BattleScript_SafeguardProtected
	attackanimation
	waitanimation
	setmoveeffect MOVE_EFFECT_POISON
	seteffectprimary
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectParalyze::
	attackcanceler
	attackstring
	ppreduce
	jumpifability BS_TARGET, ABILITY_LIMBER, BattleScript_LimberProtected
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	typecalc
	jumpifmovehadnoeffect BattleScript_ButItFailed
	jumpifstatus BS_TARGET, STATUS1_PARALYSIS, BattleScript_AlreadyParalyzed
	jumpifstatus BS_TARGET, STATUS1_ANY, BattleScript_ButItFailed
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	jumpifsideaffecting BS_TARGET, SIDE_STATUS_SAFEGUARD, BattleScript_SafeguardProtected
	attackanimation
	waitanimation
	setmoveeffect MOVE_EFFECT_PARALYSIS
	seteffectprimary
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_AlreadyParalyzed::
	setalreadystatusedmoveattempt BS_ATTACKER
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNISALREADYPARALYZED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_LimberProtected::
	copybyte gEffectBattler, gBattlerTarget
	setbyte cMULTISTRING_CHOOSER, B_MSG_ABILITY_PREVENTS_MOVE_STATUS
	call BattleScript_PRLZPrevention
	goto BattleScript_MoveEnd

BattleScript_EffectAttackDownHit::
	setmoveeffect MOVE_EFFECT_ATK_MINUS_1
	goto BattleScript_EffectHit

BattleScript_EffectDefenseDownHit::
	setmoveeffect MOVE_EFFECT_DEF_MINUS_1
	goto BattleScript_EffectHit

BattleScript_EffectSpeedDownHit::
	setmoveeffect MOVE_EFFECT_SPD_MINUS_1
	goto BattleScript_EffectHit

BattleScript_EffectSpecialAttackDownHit::
	setmoveeffect MOVE_EFFECT_SP_ATK_MINUS_1
	goto BattleScript_EffectHit

BattleScript_EffectSpecialDefenseDownHit::
	setmoveeffect MOVE_EFFECT_SP_DEF_MINUS_1
	goto BattleScript_EffectHit

BattleScript_EffectAccuracyDownHit::
	setmoveeffect MOVE_EFFECT_ACC_MINUS_1
	goto BattleScript_EffectHit

BattleScript_EffectSkyAttack::
	jumpifstatus2 BS_ATTACKER, STATUS2_MULTIPLETURNS, BattleScript_TwoTurnMovesSecondTurn
	jumpifword CMP_COMMON_BITS, gHitMarker, HITMARKER_NO_ATTACKSTRING, BattleScript_TwoTurnMovesSecondTurn
	setbyte sTWOTURN_STRINGID, B_MSG_TURN1_SKY_ATTACK
	call BattleScriptFirstChargingTurn
	goto BattleScript_MoveEnd

BattleScript_EffectConfuseHit::
	setmoveeffect MOVE_EFFECT_CONFUSION
	goto BattleScript_EffectHit

BattleScript_EffectTwineedle::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	setbyte sMULTIHIT_EFFECT, MOVE_EFFECT_POISON
	attackstring
	ppreduce
	setmultihitcounter 2
	initmultihitstring
	goto BattleScript_MultiHitLoop

BattleScript_EffectSubstitute::
	attackcanceler
	ppreduce
	attackstring
	waitstate
	jumpifstatus2 BS_ATTACKER, STATUS2_SUBSTITUTE, BattleScript_AlreadyHasSubstitute
	setsubstitute
	jumpifbyte CMP_NOT_EQUAL, cMULTISTRING_CHOOSER, B_MSG_SUBSTITUTE_FAILED, BattleScript_SubstituteAnim
	pause B_WAIT_TIME_SHORT
	goto BattleScript_SubstituteString
BattleScript_SubstituteAnim::
	attackanimation
	waitanimation
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
BattleScript_SubstituteString::
	printfromtable gSubstituteUsedStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd
BattleScript_AlreadyHasSubstitute::
	setalreadystatusedmoveattempt BS_ATTACKER
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNHASSUBSTITUTE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectRecharge::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	setmoveeffect MOVE_EFFECT_RECHARGE | MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN
	goto BattleScript_HitFromAtkString

BattleScript_MoveUsedMustRecharge::
	printstring STRINGID_PKMNMUSTRECHARGE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectRage::
	attackcanceler
	accuracycheck BattleScript_RageMiss, ACC_CURR_MOVE
	setmoveeffect MOVE_EFFECT_RAGE
	seteffectprimary
	setmoveeffect 0
	goto BattleScript_HitFromAtkString
BattleScript_RageMiss::
	setmoveeffect MOVE_EFFECT_RAGE
	clearstatusfromeffect BS_ATTACKER
	goto BattleScript_PrintMoveMissed

BattleScript_EffectMimic::
	attackcanceler
	attackstring
	ppreduce
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	accuracycheck BattleScript_ButItFailed, NO_ACC_CALC_CHECK_LOCK_ON
	mimicattackcopy BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNLEARNEDMOVE2
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectMetronome::
	attackcanceler
	attackstring
	pause B_WAIT_TIME_SHORT
	attackanimation
	waitanimation
	setbyte sB_ANIM_TURN, 0
	setbyte sB_ANIM_TARGETS_HIT, 0
	metronome

BattleScript_EffectLeechSeed::
	attackcanceler
	attackstring
	pause B_WAIT_TIME_SHORT
	ppreduce
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	accuracycheck BattleScript_DoLeechSeed, ACC_CURR_MOVE
BattleScript_DoLeechSeed::
	setseeded
	attackanimation
	waitanimation
	printfromtable gLeechSeedStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectSplash::
	attackcanceler
	attackstring
	ppreduce
	attackanimation
	waitanimation
	incrementgamestat GAME_STAT_USED_SPLASH
	printstring STRINGID_BUTNOTHINGHAPPENED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectDisable::
	attackcanceler
	attackstring
	ppreduce
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	disablelastusedattack BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNMOVEWASDISABLED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectLevelDamage::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	typecalc
	bicbyte gMoveResultFlags, MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE
	dmgtolevel
	adjustsetdamage
	goto BattleScript_HitFromAtkAnimation

BattleScript_EffectPsywave::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	typecalc
	bicbyte gMoveResultFlags, MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE
	psywavedamageeffect
	adjustsetdamage
	goto BattleScript_HitFromAtkAnimation

BattleScript_EffectCounter::
	attackcanceler
	counterdamagecalculator BattleScript_FailedFromAtkString
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	typecalc2
	adjustsetdamage
	goto BattleScript_HitFromAtkAnimation

BattleScript_EffectEncore::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	trysetencore BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNGOTENCORE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectPainSplit::
	attackcanceler
	attackstring
	ppreduce
	accuracycheck BattleScript_ButItFailed, NO_ACC_CALC_CHECK_LOCK_ON
	painsplitdmgcalc BattleScript_ButItFailed
	attackanimation
	waitanimation
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	copyword gBattleMoveDamage, sPAINSPLIT_HP
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	printstring STRINGID_SHAREDPAIN
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectSnore::
	attackcanceler
	jumpifstatus BS_ATTACKER, STATUS1_SLEEP, BattleScript_SnoreIsAsleep
	attackstring
	ppreduce
	goto BattleScript_ButItFailed
BattleScript_SnoreIsAsleep::
	jumpifhalfword CMP_EQUAL, gChosenMove, MOVE_SLEEP_TALK, BattleScript_DoSnore
	printstring STRINGID_PKMNFASTASLEEP
	waitmessage B_WAIT_TIME_LONG
	statusanimation BS_ATTACKER
BattleScript_DoSnore::
	attackstring
	ppreduce
	accuracycheck BattleScript_MoveMissedPause, ACC_CURR_MOVE
	setmoveeffect MOVE_EFFECT_FLINCH
	goto BattleScript_HitFromCritCalc

BattleScript_EffectConversion2::
	attackcanceler
	attackstring
	ppreduce
	settypetorandomresistance BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNCHANGEDTYPE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectLockOn::
	attackcanceler
	attackstring
	ppreduce
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	setalwayshitflag
	attackanimation
	waitanimation
	printstring STRINGID_PKMNTOOKAIM
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectSketch::
	attackcanceler
	attackstring
	ppreduce
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	copymovepermanently BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNSKETCHEDMOVE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectSleepTalk::
	attackcanceler
	jumpifstatus BS_ATTACKER, STATUS1_SLEEP, BattleScript_SleepTalkIsAsleep
	attackstring
	ppreduce
	goto BattleScript_ButItFailed
BattleScript_SleepTalkIsAsleep::
	printstring STRINGID_PKMNFASTASLEEP
	waitmessage B_WAIT_TIME_LONG
	statusanimation BS_ATTACKER
	attackstring
	ppreduce
	orword gHitMarker, HITMARKER_NO_PPDEDUCT
	trychoosesleeptalkmove BattleScript_SleepTalkUsingMove
	pause B_WAIT_TIME_LONG
	goto BattleScript_ButItFailed
BattleScript_SleepTalkUsingMove::
	attackanimation
	waitanimation
	setbyte sB_ANIM_TURN, 0
	setbyte sB_ANIM_TARGETS_HIT, 0
	jumptocalledmove TRUE

BattleScript_EffectDestinyBond::
	attackcanceler
	attackstring
	ppreduce
	setdestinybond
	attackanimation
	waitanimation
	printstring STRINGID_PKMNTRYINGTOTAKEFOE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectFlail::
	remaininghptopower
	goto BattleScript_EffectHit

BattleScript_EffectSpite::
	attackcanceler
	attackstring
	ppreduce
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	tryspiteppreduce BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNREDUCEDPP
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectHealBell::
	attackcanceler
	attackstring
	ppreduce
	healpartystatus
	waitstate
	attackanimation
	waitanimation
	printfromtable gPartyStatusHealStringIds
	waitmessage B_WAIT_TIME_LONG
	jumpifnotmove MOVE_HEAL_BELL, BattleScript_PartyHealEnd
	jumpifbyte CMP_NO_COMMON_BITS, cMULTISTRING_CHOOSER, B_MSG_BELL_SOUNDPROOF_ATTACKER, BattleScript_CheckHealBellMon2Unaffected
	printstring STRINGID_PKMNSXBLOCKSY
	waitmessage B_WAIT_TIME_LONG
BattleScript_CheckHealBellMon2Unaffected::
	jumpifbyte CMP_NO_COMMON_BITS, cMULTISTRING_CHOOSER, B_MSG_BELL_SOUNDPROOF_PARTNER, BattleScript_PartyHealEnd
	printstring STRINGID_PKMNSXBLOCKSY2
	waitmessage B_WAIT_TIME_LONG
BattleScript_PartyHealEnd::
	updatestatusicon BS_ATTACKER_WITH_PARTNER
	waitstate
	goto BattleScript_MoveEnd

BattleScript_EffectTripleKick::
	attackcanceler
	attackstring
	ppreduce
	sethword sTRIPLE_KICK_POWER, 0
	initmultihitstring
	setmultihit 3
BattleScript_TripleKickLoop::
	jumpifhasnohp BS_ATTACKER, BattleScript_TripleKickEnd
	jumpifhasnohp BS_TARGET, BattleScript_TripleKickNoMoreHits
	jumpifhalfword CMP_EQUAL, gChosenMove, MOVE_SLEEP_TALK, BattleScript_DoTripleKickAttack
	jumpifstatus BS_ATTACKER, STATUS1_SLEEP, BattleScript_TripleKickNoMoreHits
BattleScript_DoTripleKickAttack::
	accuracycheck BattleScript_TripleKickNoMoreHits, ACC_CURR_MOVE
	movevaluescleanup
	addbyte sTRIPLE_KICK_POWER, 10
	addbyte sMULTIHIT_STRING + 4, 1
	copyhword gDynamicBasePower, sTRIPLE_KICK_POWER
	critcalc
	damagecalc
	typecalc
	adjustnormaldamage
	jumpifmovehadnoeffect BattleScript_TripleKickNoMoreHits
	attackanimation
	waitanimation
	effectivenesssound
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	critmessage
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_EMPTYSTRING3
	waitmessage 1
	moveendto MOVEEND_NEXT_TARGET
	jumpifbyte CMP_COMMON_BITS, gMoveResultFlags, MOVE_RESULT_FOE_ENDURED, BattleScript_TripleKickPrintStrings
	decrementmultihit BattleScript_TripleKickLoop
	goto BattleScript_TripleKickPrintStrings
BattleScript_TripleKickNoMoreHits::
	pause B_WAIT_TIME_SHORT
	jumpifbyte CMP_EQUAL, sMULTIHIT_STRING + 4, 0, BattleScript_TripleKickPrintStrings
	bicbyte gMoveResultFlags, MOVE_RESULT_MISSED
BattleScript_TripleKickPrintStrings::
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	jumpifbyte CMP_EQUAL, sMULTIHIT_STRING + 4, 0, BattleScript_TripleKickEnd
	jumpifbyte CMP_COMMON_BITS, gMoveResultFlags, MOVE_RESULT_DOESNT_AFFECT_FOE, BattleScript_TripleKickEnd
	copyarray gBattleTextBuff1, sMULTIHIT_STRING, 6
	printstring STRINGID_HITXTIMES
	waitmessage B_WAIT_TIME_LONG
BattleScript_TripleKickEnd::
	seteffectwithchance
	tryfaintmon BS_TARGET
	moveendfrom MOVEEND_UPDATE_LAST_MOVES
	end

BattleScript_EffectThief::
	setmoveeffect MOVE_EFFECT_STEAL_ITEM
	goto BattleScript_EffectHit

BattleScript_EffectMeanLook::
	attackcanceler
	attackstring
	ppreduce
	accuracycheck BattleScript_ButItFailed, NO_ACC_CALC
	jumpifstatus2 BS_TARGET, STATUS2_ESCAPE_PREVENTION, BattleScript_ButItFailed
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	attackanimation
	waitanimation
	setmoveeffect MOVE_EFFECT_PREVENT_ESCAPE
	seteffectprimary
	printstring STRINGID_TARGETCANTESCAPENOW
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectNightmare::
	attackcanceler
	attackstring
	ppreduce
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	jumpifstatus2 BS_TARGET, STATUS2_NIGHTMARE, BattleScript_ButItFailed
	jumpifstatus BS_TARGET, STATUS1_SLEEP, BattleScript_NightmareWorked
	goto BattleScript_ButItFailed
BattleScript_NightmareWorked::
	attackanimation
	waitanimation
	setmoveeffect MOVE_EFFECT_NIGHTMARE
	seteffectprimary
	printstring STRINGID_PKMNFELLINTONIGHTMARE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectMinimize::
	attackcanceler
	setminimize
	setstatchanger STAT_EVASION, 1, FALSE
	goto BattleScript_EffectStatUpAfterAtkCanceler

BattleScript_EffectCurse::
	jumpiftype2 BS_ATTACKER, TYPE_GHOST, BattleScript_GhostCurse
	attackcanceler
	attackstring
	ppreduce
	jumpifstat BS_ATTACKER, CMP_GREATER_THAN, STAT_SPEED, MIN_STAT_STAGE, BattleScript_CurseTrySpeed
	jumpifstat BS_ATTACKER, CMP_NOT_EQUAL, STAT_ATK, MAX_STAT_STAGE, BattleScript_CurseTrySpeed
	jumpifstat BS_ATTACKER, CMP_EQUAL, STAT_DEF, MAX_STAT_STAGE, BattleScript_ButItFailed
BattleScript_CurseTrySpeed::
	copybyte gBattlerTarget, gBattlerAttacker
	setbyte sB_ANIM_TURN, 1
	attackanimation
	waitanimation
	setstatchanger STAT_SPEED, 1, TRUE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_CurseTryAttack
	printfromtable gStatDownStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_CurseTryAttack::
	setstatchanger STAT_ATK, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_CurseTryDefense
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_CurseTryDefense::
	setstatchanger STAT_DEF, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_CurseEnd
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_CurseEnd::
	goto BattleScript_MoveEnd
BattleScript_GhostCurse::
	jumpifbytenotequal gBattlerAttacker, gBattlerTarget, BattleScript_DoGhostCurse
	getmovetarget BS_ATTACKER
BattleScript_DoGhostCurse::
	attackcanceler
	attackstring
	ppreduce
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	accuracycheck BattleScript_ButItFailed, NO_ACC_CALC_CHECK_LOCK_ON
	cursetarget BattleScript_ButItFailed
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	setbyte sB_ANIM_TURN, 0
	attackanimation
	waitanimation
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	printstring STRINGID_PKMNLAIDCURSE
	waitmessage B_WAIT_TIME_LONG
	tryfaintmon BS_ATTACKER
	goto BattleScript_MoveEnd

BattleScript_EffectProtect::
BattleScript_EffectEndure::
	attackcanceler
	attackstring
	ppreduce
	setprotectlike
	attackanimation
	waitanimation
	printfromtable gProtectLikeUsedStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectSpikes::
	attackcanceler
	trysetspikes BattleScript_FailedFromAtkString
	attackstring
	ppreduce
	attackanimation
	waitanimation
	printstring STRINGID_SPIKESSCATTERED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectForesight::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	setforesight
	attackanimation
	waitanimation
	printstring STRINGID_PKMNIDENTIFIED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectPerishSong::
	attackcanceler
	attackstring
	ppreduce
	trysetperishsong BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_FAINTINTHREE
	waitmessage B_WAIT_TIME_LONG
	setbyte sBATTLER, 0
BattleScript_PerishSongLoop::
	jumpifability BS_SCRIPTING, ABILITY_SOUNDPROOF, BattleScript_PerishSongNotAffected
BattleScript_PerishSongLoopIncrement::
	addbyte sBATTLER, 1
	jumpifbytenotequal sBATTLER, gBattlersCount, BattleScript_PerishSongLoop
	goto BattleScript_MoveEnd

BattleScript_PerishSongNotAffected::
	printstring STRINGID_PKMNSXBLOCKSY2
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_PerishSongLoopIncrement

BattleScript_EffectSandstorm::
	attackcanceler
	attackstring
	ppreduce
	setsandstorm
	goto BattleScript_MoveWeatherChange

BattleScript_EffectRollout::
	attackcanceler
	attackstring
	jumpifstatus2 BS_ATTACKER, STATUS2_MULTIPLETURNS, BattleScript_RolloutCheckAccuracy
	ppreduce
BattleScript_RolloutCheckAccuracy::
	accuracycheck BattleScript_RolloutHit, ACC_CURR_MOVE
BattleScript_RolloutHit::
	typecalc2
	rolloutdamagecalculation
	goto BattleScript_HitFromCritCalc

BattleScript_EffectSwagger::
	attackcanceler
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_MakeMoveMissed
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	jumpifconfusedandstatmaxed STAT_ATK, BattleScript_ButItFailed
	attackanimation
	waitanimation
	setstatchanger STAT_ATK, 2, FALSE
	statbuffchange STAT_CHANGE_ALLOW_PTR, BattleScript_SwaggerTryConfuse
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_SwaggerTryConfuse
	setgraphicalstatchangevalues
	playanimation BS_TARGET, B_ANIM_STATS_CHANGE, sB_ANIM_ARG1
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_SwaggerTryConfuse::
	jumpifability BS_TARGET, ABILITY_OWN_TEMPO, BattleScript_OwnTempoPrevents
	jumpifsideaffecting BS_TARGET, SIDE_STATUS_SAFEGUARD, BattleScript_SafeguardProtected
	setmoveeffect MOVE_EFFECT_CONFUSION
	seteffectprimary
	goto BattleScript_MoveEnd

BattleScript_EffectFuryCutter::
	attackcanceler
	attackstring
	ppreduce
	accuracycheck BattleScript_FuryCutterHit, ACC_CURR_MOVE
BattleScript_FuryCutterHit::
	furycuttercalc
	critcalc
	damagecalc
	typecalc
	jumpifmovehadnoeffect BattleScript_FuryCutterHit
	adjustnormaldamage
	goto BattleScript_HitFromAtkAnimation

BattleScript_EffectAttract::
	attackcanceler
	attackstring
	ppreduce
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	tryinfatuating BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNFELLINLOVE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectReturn::
BattleScript_EffectFrustration::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	friendshiptodamagecalculation
	goto BattleScript_HitFromAtkString

BattleScript_EffectPresent::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	typecalc
	presentdamagecalculation

BattleScript_EffectSafeguard::
	attackcanceler
	attackstring
	ppreduce
	setsafeguard
	goto BattleScript_PrintReflectLightScreenSafeguardString

BattleScript_EffectThawHit::
	setmoveeffect MOVE_EFFECT_BURN
	goto BattleScript_EffectHit

BattleScript_EffectMagnitude::
	attackcanceler
	attackstring
	ppreduce
	selectfirstvalidtarget
	magnitudedamagecalculation
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_MAGNITUDESTRENGTH
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_HitsAllWithUndergroundBonusLoop

BattleScript_EffectBatonPass::
	attackcanceler
	attackstring
	ppreduce
	jumpifbattletype BATTLE_TYPE_ARENA, BattleScript_ButItFailed
	jumpifcantswitch SWITCH_IGNORE_ESCAPE_PREVENTION | BS_ATTACKER, BattleScript_ButItFailed
	attackanimation
	waitanimation
	openpartyscreen BS_ATTACKER, BattleScript_ButItFailed
	switchoutabilities BS_ATTACKER
	waitstate
	switchhandleorder BS_ATTACKER, 2
	returntoball BS_ATTACKER
	getswitchedmondata BS_ATTACKER
	switchindataupdate BS_ATTACKER
	hpthresholds BS_ATTACKER
	printstring STRINGID_SWITCHINMON
	switchinanim BS_ATTACKER, TRUE
	waitstate
	switchineffects BS_ATTACKER
	goto BattleScript_MoveEnd

BattleScript_EffectRapidSpin::
	setmoveeffect MOVE_EFFECT_RAPIDSPIN | MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN
	goto BattleScript_EffectHit

BattleScript_EffectSonicboom::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	typecalc
	bicbyte gMoveResultFlags, MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE
	setword gBattleMoveDamage, 20
	adjustsetdamage
	goto BattleScript_HitFromAtkAnimation

BattleScript_EffectMorningSun::
BattleScript_EffectSynthesis::
BattleScript_EffectMoonlight::
	attackcanceler
	attackstring
	ppreduce
	recoverbasedonsunlight BattleScript_AlreadyAtFullHp
	goto BattleScript_PresentHealTarget

BattleScript_EffectHiddenPower::
	hiddenpowercalc
	goto BattleScript_EffectHit

BattleScript_EffectRainDance::
	attackcanceler
	attackstring
	ppreduce
	setrain
BattleScript_MoveWeatherChange::
	attackanimation
	waitanimation
	printfromtable gMoveWeatherChangeStringIds
	waitmessage B_WAIT_TIME_LONG
	call BattleScript_WeatherFormChanges
	goto BattleScript_MoveEnd

BattleScript_EffectSunnyDay::
	attackcanceler
	attackstring
	ppreduce
	setsunny
	goto BattleScript_MoveWeatherChange

BattleScript_EffectDefenseUpHit::
	setmoveeffect MOVE_EFFECT_DEF_PLUS_1 | MOVE_EFFECT_AFFECTS_USER
	goto BattleScript_EffectHit

BattleScript_EffectAttackUpHit::
	setmoveeffect MOVE_EFFECT_ATK_PLUS_1 | MOVE_EFFECT_AFFECTS_USER
	goto BattleScript_EffectHit

BattleScript_EffectAllStatsUpHit::
	setmoveeffect MOVE_EFFECT_ALL_STATS_UP | MOVE_EFFECT_AFFECTS_USER
	goto BattleScript_EffectHit

BattleScript_EffectBellyDrum::
	attackcanceler
	attackstring
	ppreduce
	maxattackhalvehp BattleScript_ButItFailed
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	attackanimation
	waitanimation
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	printstring STRINGID_PKMNCUTHPMAXEDATTACK
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectPsychUp::
	attackcanceler
	attackstring
	ppreduce
	copyfoestats BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNCOPIEDSTATCHANGES
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectMirrorCoat::
	attackcanceler
	mirrorcoatdamagecalculator BattleScript_FailedFromAtkString
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	typecalc2
	adjustsetdamage
	goto BattleScript_HitFromAtkAnimation

BattleScript_EffectSkullBash::
	jumpifstatus2 BS_ATTACKER, STATUS2_MULTIPLETURNS, BattleScript_TwoTurnMovesSecondTurn
	jumpifword CMP_COMMON_BITS, gHitMarker, HITMARKER_NO_ATTACKSTRING, BattleScript_TwoTurnMovesSecondTurn
	setbyte sTWOTURN_STRINGID, B_MSG_TURN1_SKULL_BASH
	call BattleScriptFirstChargingTurn
	setstatchanger STAT_DEF, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_SkullBashEnd
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_SkullBashEnd
	setgraphicalstatchangevalues
	playanimation BS_ATTACKER, B_ANIM_STATS_CHANGE, sB_ANIM_ARG1
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_SkullBashEnd::
	goto BattleScript_MoveEnd

BattleScript_EffectTwister::
	jumpifnostatus3 BS_TARGET, STATUS3_ON_AIR, BattleScript_FlinchEffect
	orword gHitMarker, HITMARKER_IGNORE_ON_AIR
	setbyte sDMG_MULTIPLIER, 2
BattleScript_FlinchEffect::
	setmoveeffect MOVE_EFFECT_FLINCH
	goto BattleScript_EffectHit

BattleScript_EffectEarthquake::
	attackcanceler
	attackstring
	ppreduce
	selectfirstvalidtarget
BattleScript_HitsAllWithUndergroundBonusLoop::
	movevaluescleanup
	jumpifnostatus3 BS_TARGET, STATUS3_UNDERGROUND, BattleScript_HitsAllNoUndergroundBonus
	orword gHitMarker, HITMARKER_IGNORE_UNDERGROUND
	setbyte sDMG_MULTIPLIER, 2
	goto BattleScript_DoHitAllWithUndergroundBonus
BattleScript_HitsAllNoUndergroundBonus::
	bicword gHitMarker, HITMARKER_IGNORE_UNDERGROUND
	setbyte sDMG_MULTIPLIER, 1
BattleScript_DoHitAllWithUndergroundBonus::
	accuracycheck BattleScript_HitAllWithUndergroundBonusMissed, ACC_CURR_MOVE
	critcalc
	damagecalc
	typecalc
	adjustnormaldamage
	attackanimation
	waitanimation
	effectivenesssound
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	critmessage
	waitmessage B_WAIT_TIME_LONG
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_EMPTYSTRING3
	waitmessage 1
	tryfaintmon BS_TARGET
	moveendto MOVEEND_NEXT_TARGET
	jumpifnexttargetvalid BattleScript_HitsAllWithUndergroundBonusLoop
	end
BattleScript_HitAllWithUndergroundBonusMissed::
	pause B_WAIT_TIME_SHORT
	typecalc
	effectivenesssound
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	moveendto MOVEEND_NEXT_TARGET
	jumpifnexttargetvalid BattleScript_HitsAllWithUndergroundBonusLoop
	end

BattleScript_EffectFutureSight::
	attackcanceler
	attackstring
	ppreduce
	trysetfutureattack BattleScript_ButItFailed
	attackanimation
	waitanimation
	printfromtable gFutureMoveUsedStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectGust::
	jumpifnostatus3 BS_TARGET, STATUS3_ON_AIR, BattleScript_EffectHit
	orword gHitMarker, HITMARKER_IGNORE_ON_AIR
	setbyte sDMG_MULTIPLIER, 2
	goto BattleScript_EffectHit

BattleScript_EffectStomp::
	jumpifnostatus3 BS_TARGET, STATUS3_MINIMIZED, BattleScript_FlinchEffect
	setbyte sDMG_MULTIPLIER, 2
	goto BattleScript_FlinchEffect

BattleScript_EffectSolarBeam::
	jumpifabilitypresent ABILITY_CLOUD_NINE, BattleScript_SolarBeamDecideTurn
	jumpifabilitypresent ABILITY_AIR_LOCK, BattleScript_SolarBeamDecideTurn
	jumpifhalfword CMP_COMMON_BITS, gBattleWeather, B_WEATHER_SUN, BattleScript_SolarBeamOnFirstTurn
BattleScript_SolarBeamDecideTurn::
	jumpifstatus2 BS_ATTACKER, STATUS2_MULTIPLETURNS, BattleScript_TwoTurnMovesSecondTurn
	jumpifword CMP_COMMON_BITS, gHitMarker, HITMARKER_NO_ATTACKSTRING, BattleScript_TwoTurnMovesSecondTurn
	setbyte sTWOTURN_STRINGID, B_MSG_TURN1_SOLAR_BEAM
	call BattleScriptFirstChargingTurn
	goto BattleScript_MoveEnd
BattleScript_SolarBeamOnFirstTurn::
	orword gHitMarker, HITMARKER_CHARGING
	setmoveeffect MOVE_EFFECT_CHARGING | MOVE_EFFECT_AFFECTS_USER
	seteffectprimary
	ppreduce
	goto BattleScript_TwoTurnMovesSecondTurn

BattleScript_EffectThunder::
	setmoveeffect MOVE_EFFECT_PARALYSIS
	orword gHitMarker, HITMARKER_IGNORE_ON_AIR
	goto BattleScript_EffectHit

BattleScript_EffectTeleport::
	attackcanceler
	attackstring
	ppreduce
	jumpifbattletype BATTLE_TYPE_TRAINER, BattleScript_ButItFailed
	getifcantrunfrombattle BS_ATTACKER
	jumpifbyte CMP_EQUAL, gBattleCommunication, BATTLE_RUN_FORBIDDEN, BattleScript_ButItFailed
	jumpifbyte CMP_EQUAL, gBattleCommunication, BATTLE_RUN_FAILURE, BattleScript_PrintAbilityMadeIneffective
	attackanimation
	waitanimation
	printstring STRINGID_PKMNFLEDFROMBATTLE
	waitmessage B_WAIT_TIME_LONG
	setoutcomeonteleport BS_ATTACKER
	goto BattleScript_MoveEnd

BattleScript_EffectBeatUp::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	pause B_WAIT_TIME_SHORT
	ppreduce
	setbyte gBattleCommunication, 0
BattleScript_BeatUpLoop::
	movevaluescleanup
	trydobeatup BattleScript_BeatUpEnd, BattleScript_ButItFailed
	printstring STRINGID_PKMNATTACK
	critcalc
	jumpifbyte CMP_NOT_EQUAL, gCritMultiplier, 2, BattleScript_BeatUpAttack
	manipulatedamage DMG_DOUBLED
BattleScript_BeatUpAttack::
	adjustnormaldamage
	attackanimation
	waitanimation
	effectivenesssound
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	critmessage
	waitmessage B_WAIT_TIME_LONG
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	tryfaintmon BS_TARGET
	moveendto MOVEEND_NEXT_TARGET
	goto BattleScript_BeatUpLoop
BattleScript_BeatUpEnd::
	end

BattleScript_EffectSemiInvulnerable::
	jumpifstatus2 BS_ATTACKER, STATUS2_MULTIPLETURNS, BattleScript_SecondTurnSemiInvulnerable
	jumpifword CMP_COMMON_BITS, gHitMarker, HITMARKER_NO_ATTACKSTRING, BattleScript_SecondTurnSemiInvulnerable
	jumpifmove MOVE_FLY, BattleScript_FirstTurnFly
	jumpifmove MOVE_DIVE, BattleScript_FirstTurnDive
	jumpifmove MOVE_BOUNCE, BattleScript_FirstTurnBounce
	setbyte sTWOTURN_STRINGID, B_MSG_TURN1_DIG
	goto BattleScript_FirstTurnSemiInvulnerable

BattleScript_FirstTurnBounce::
	setbyte sTWOTURN_STRINGID, B_MSG_TURN1_BOUNCE
	goto BattleScript_FirstTurnSemiInvulnerable

BattleScript_FirstTurnDive::
	setbyte sTWOTURN_STRINGID, B_MSG_TURN1_DIVE
	goto BattleScript_FirstTurnSemiInvulnerable

BattleScript_FirstTurnFly::
	setbyte sTWOTURN_STRINGID, B_MSG_TURN1_FLY
BattleScript_FirstTurnSemiInvulnerable::
	call BattleScriptFirstChargingTurn
	setsemiinvulnerablebit
	goto BattleScript_MoveEnd

BattleScript_SecondTurnSemiInvulnerable::
	attackcanceler
	setmoveeffect MOVE_EFFECT_CHARGING
	setbyte sB_ANIM_TURN, 1
	clearstatusfromeffect BS_ATTACKER
	orword gHitMarker, HITMARKER_NO_PPDEDUCT
	jumpifnotmove MOVE_BOUNCE, BattleScript_SemiInvulnerableTryHit
	setmoveeffect MOVE_EFFECT_PARALYSIS
BattleScript_SemiInvulnerableTryHit::
	accuracycheck BattleScript_SemiInvulnerableMiss, ACC_CURR_MOVE
	clearsemiinvulnerablebit
	goto BattleScript_HitFromAtkString

BattleScript_SemiInvulnerableMiss::
	clearsemiinvulnerablebit
	goto BattleScript_PrintMoveMissed

BattleScript_EffectDefenseCurl::
	attackcanceler
	attackstring
	ppreduce
	setdefensecurlbit
	setstatchanger STAT_DEF, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_DefenseCurlDoStatUpAnim
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_StatUpPrintString
	attackanimation
	waitanimation
BattleScript_DefenseCurlDoStatUpAnim::
	goto BattleScript_StatUpDoAnim

BattleScript_EffectSoftboiled::
	attackcanceler
	attackstring
	ppreduce
	tryhealhalfhealth BattleScript_AlreadyAtFullHp, BS_TARGET
BattleScript_PresentHealTarget::
	attackanimation
	waitanimation
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	printstring STRINGID_PKMNREGAINEDHEALTH
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_AlreadyAtFullHp::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNHPFULL
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectFakeOut::
	attackcanceler
	jumpifnotfirstturn BattleScript_FailedFromAtkString
	setmoveeffect MOVE_EFFECT_FLINCH | MOVE_EFFECT_CERTAIN
	goto BattleScript_EffectHit

BattleScript_FailedFromAtkString::
	attackstring
BattleScript_FailedFromPpReduce::
	ppreduce
BattleScript_ButItFailed::
	pause B_WAIT_TIME_SHORT
	orbyte gMoveResultFlags, MOVE_RESULT_FAILED
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_NotAffected::
	pause B_WAIT_TIME_SHORT
	orbyte gMoveResultFlags, MOVE_RESULT_DOESNT_AFFECT_FOE
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectUproar::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	setmoveeffect MOVE_EFFECT_UPROAR | MOVE_EFFECT_AFFECTS_USER
	attackstring
	jumpifstatus2 BS_ATTACKER, STATUS2_MULTIPLETURNS, BattleScript_UproarHit
	ppreduce
BattleScript_UproarHit::
	nop
	goto BattleScript_HitFromCritCalc

BattleScript_EffectStockpile::
	attackcanceler
	attackstring
	ppreduce
	stockpile
	attackanimation
	waitanimation
	printfromtable gStockpileUsedStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectSpitUp::
	attackcanceler
	jumpifbyte CMP_EQUAL, cMISS_TYPE, B_MSG_PROTECTED, BattleScript_SpitUpFailProtect
	attackstring
	ppreduce
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	stockpiletobasedamage BattleScript_SpitUpFail
	typecalc
	adjustsetdamage
	goto BattleScript_HitFromAtkAnimation
BattleScript_SpitUpFail::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_FAILEDTOSPITUP
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_SpitUpFailProtect::
	attackstring
	ppreduce
	pause B_WAIT_TIME_LONG
	stockpiletobasedamage BattleScript_SpitUpFail
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectSwallow::
	attackcanceler
	attackstring
	ppreduce
	stockpiletohpheal BattleScript_SwallowFail
	goto BattleScript_PresentHealTarget

BattleScript_SwallowFail::
	pause B_WAIT_TIME_SHORT
	printfromtable gSwallowFailStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectHail::
	attackcanceler
	attackstring
	ppreduce
	sethail
	goto BattleScript_MoveWeatherChange

BattleScript_EffectTorment::
	attackcanceler
	attackstring
	ppreduce
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	settorment BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNSUBJECTEDTOTORMENT
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectFlatter::
	attackcanceler
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_MakeMoveMissed
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	jumpifconfusedandstatmaxed STAT_SPATK, BattleScript_ButItFailed
	attackanimation
	waitanimation
	setstatchanger STAT_SPATK, 1, FALSE
	statbuffchange STAT_CHANGE_ALLOW_PTR, BattleScript_FlatterTryConfuse
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_FlatterTryConfuse
	setgraphicalstatchangevalues
	playanimation BS_TARGET, B_ANIM_STATS_CHANGE, sB_ANIM_ARG1
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_FlatterTryConfuse::
	jumpifability BS_TARGET, ABILITY_OWN_TEMPO, BattleScript_OwnTempoPrevents
	jumpifsideaffecting BS_TARGET, SIDE_STATUS_SAFEGUARD, BattleScript_SafeguardProtected
	setmoveeffect MOVE_EFFECT_CONFUSION
	seteffectprimary
	goto BattleScript_MoveEnd

BattleScript_EffectWillOWisp::
	attackcanceler
	attackstring
	ppreduce
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	jumpifstatus BS_TARGET, STATUS1_BURN, BattleScript_AlreadyBurned
	jumpiftype BS_TARGET, TYPE_FIRE, BattleScript_NotAffected
	jumpifability BS_TARGET, ABILITY_WATER_VEIL, BattleScript_WaterVeilPrevents
	jumpifstatus BS_TARGET, STATUS1_ANY, BattleScript_ButItFailed
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	jumpifsideaffecting BS_TARGET, SIDE_STATUS_SAFEGUARD, BattleScript_SafeguardProtected
	attackanimation
	waitanimation
	setmoveeffect MOVE_EFFECT_BURN
	seteffectprimary
	goto BattleScript_MoveEnd

BattleScript_WaterVeilPrevents::
	copybyte gEffectBattler, gBattlerTarget
	setbyte cMULTISTRING_CHOOSER, B_MSG_ABILITY_PREVENTS_MOVE_STATUS
	call BattleScript_BRNPrevention
	goto BattleScript_MoveEnd

BattleScript_AlreadyBurned::
	setalreadystatusedmoveattempt BS_ATTACKER
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNALREADYHASBURN
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectMemento::
	attackcanceler
	jumpifbyte CMP_EQUAL, cMISS_TYPE, B_MSG_PROTECTED, BattleScript_MementoTargetProtect
	attackstring
	ppreduce
	trymemento BattleScript_ButItFailed
	setatkhptozero
	attackanimation
	waitanimation
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_EffectMementoPrintNoEffect
	setbyte sSTAT_ANIM_PLAYED, FALSE
	playstatchangeanimation BS_TARGET, BIT_ATK | BIT_SPATK, STAT_CHANGE_NEGATIVE | STAT_CHANGE_BY_TWO | STAT_CHANGE_MULTIPLE_STATS
	playstatchangeanimation BS_TARGET, BIT_ATK, STAT_CHANGE_NEGATIVE | STAT_CHANGE_BY_TWO
	setstatchanger STAT_ATK, 2, TRUE
	statbuffchange STAT_CHANGE_ALLOW_PTR, BattleScript_EffectMementoTrySpAtk
@ Greater than B_MSG_DEFENDER_STAT_FELL is checking if the stat cannot decrease
	jumpifbyte CMP_GREATER_THAN, cMULTISTRING_CHOOSER, B_MSG_DEFENDER_STAT_FELL, BattleScript_EffectMementoTrySpAtk
	printfromtable gStatDownStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_EffectMementoTrySpAtk:
	playstatchangeanimation BS_TARGET, BIT_SPATK, STAT_CHANGE_NEGATIVE | STAT_CHANGE_BY_TWO
	setstatchanger STAT_SPATK, 2, TRUE
	statbuffchange STAT_CHANGE_ALLOW_PTR, BattleScript_EffectMementoTryFaint
@ Greater than B_MSG_DEFENDER_STAT_FELL is checking if the stat cannot decrease
	jumpifbyte CMP_GREATER_THAN, cMULTISTRING_CHOOSER, B_MSG_DEFENDER_STAT_FELL, BattleScript_EffectMementoTryFaint
	printfromtable gStatDownStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_EffectMementoTryFaint:
	tryfaintmon BS_ATTACKER
	goto BattleScript_MoveEnd
BattleScript_EffectMementoPrintNoEffect:
	printstring STRINGID_BUTNOEFFECT
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_EffectMementoTryFaint
@ If the target is protected there's no need to check the target's stats or animate, the user will just faint
BattleScript_MementoTargetProtect:
	attackstring
	ppreduce
	trymemento BattleScript_MementoTargetProtectEnd
BattleScript_MementoTargetProtectEnd:
	setatkhptozero
	pause B_WAIT_TIME_LONG
	effectivenesssound
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	tryfaintmon BS_ATTACKER
	goto BattleScript_MoveEnd

BattleScript_EffectFacade::
	jumpifstatus BS_ATTACKER, STATUS1_POISON | STATUS1_BURN | STATUS1_PARALYSIS | STATUS1_TOXIC_POISON, BattleScript_FacadeDoubleDmg
	goto BattleScript_EffectHit

BattleScript_FacadeDoubleDmg::
	setbyte sDMG_MULTIPLIER, 2
	goto BattleScript_EffectHit

BattleScript_EffectFocusPunch::
	attackcanceler
	jumpifnodamage BattleScript_HitFromAccCheck
	ppreduce
	printstring STRINGID_PKMNLOSTFOCUS
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectSmellingsalt::
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_EffectHit
	setmoveeffect MOVE_EFFECT_REMOVE_PARALYSIS | MOVE_EFFECT_CERTAIN
	jumpifstatus BS_TARGET, STATUS1_PARALYSIS, BattleScript_SmellingsaltDoubleDmg
	goto BattleScript_EffectHit
BattleScript_SmellingsaltDoubleDmg:
	setbyte sDMG_MULTIPLIER, 2
	goto BattleScript_EffectHit

BattleScript_EffectFollowMe::
	attackcanceler
	attackstring
	ppreduce
	setforcedtarget
	attackanimation
	waitanimation
	printstring STRINGID_PKMNCENTERATTENTION
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectNaturePower::
	attackcanceler
	attackstring
	pause B_WAIT_TIME_SHORT
	callenvironmentattack
	printstring STRINGID_NATUREPOWERTURNEDINTO
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_EffectCharge::
	attackcanceler
	attackstring
	ppreduce
	setcharge
	attackanimation
	waitanimation
	printstring STRINGID_PKMNCHARGINGPOWER
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectTaunt::
	attackcanceler
	attackstring
	ppreduce
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	settaunt BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNFELLFORTAUNT
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectHelpingHand::
	attackcanceler
	attackstring
	ppreduce
	trysethelpinghand BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNREADYTOHELP
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectTrick::
	attackcanceler
	attackstring
	ppreduce
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	tryswapitems BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNSWITCHEDITEMS
	waitmessage B_WAIT_TIME_LONG
	printfromtable gItemSwapStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectRolePlay::
	attackcanceler
	attackstring
	ppreduce
	accuracycheck BattleScript_ButItFailed, NO_ACC_CALC_CHECK_LOCK_ON
	trycopyability BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNCOPIEDFOE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectWish::
	attackcanceler
	attackstring
	ppreduce
	trywish 0, BattleScript_ButItFailed
	attackanimation
	waitanimation
	goto BattleScript_MoveEnd

BattleScript_EffectAssist::
	attackcanceler
	attackstring
	assistattackselect BattleScript_FailedFromPpReduce
	attackanimation
	waitanimation
	setbyte sB_ANIM_TURN, 0
	setbyte sB_ANIM_TARGETS_HIT, 0
	jumptocalledmove TRUE

BattleScript_EffectIngrain::
	attackcanceler
	attackstring
	ppreduce
	trysetroots BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNPLANTEDROOTS
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectSuperpower::
	setmoveeffect MOVE_EFFECT_ATK_DEF_DOWN | MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN
	goto BattleScript_EffectHit

BattleScript_EffectMagicCoat::
	attackcanceler
	trysetmagiccoat BattleScript_FailedFromAtkString
	attackstring
	ppreduce
	attackanimation
	waitanimation
	printstring STRINGID_PKMNSHROUDEDITSELF
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectRecycle::
	attackcanceler
	attackstring
	ppreduce
	tryrecycleitem BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_XFOUNDONEY
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectRevenge::
	doubledamagedealtifdamaged
	goto BattleScript_EffectHit

BattleScript_EffectBrickBreak::
	attackcanceler
	accuracycheck BattleScript_PrintMoveMissed, ACC_CURR_MOVE
	attackstring
	ppreduce
	removelightscreenreflect
	critcalc
	damagecalc
	typecalc
	adjustnormaldamage
	jumpifbyte CMP_EQUAL, sB_ANIM_TURN, 0, BattleScript_BrickBreakAnim
	bicbyte gMoveResultFlags, MOVE_RESULT_MISSED | MOVE_RESULT_DOESNT_AFFECT_FOE
BattleScript_BrickBreakAnim::
	attackanimation
	waitanimation
	jumpifbyte CMP_LESS_THAN, sB_ANIM_TURN, 2, BattleScript_BrickBreakDoHit
	printstring STRINGID_THEWALLSHATTERED
	waitmessage B_WAIT_TIME_LONG
BattleScript_BrickBreakDoHit::
	typecalc2
	effectivenesssound
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	critmessage
	waitmessage B_WAIT_TIME_LONG
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	seteffectwithchance
	tryfaintmon BS_TARGET
	goto BattleScript_MoveEnd

BattleScript_EffectYawn::
	attackcanceler
	attackstring
	ppreduce
	jumpifability BS_TARGET, ABILITY_VITAL_SPIRIT, BattleScript_PrintBankAbilityMadeIneffective
	jumpifability BS_TARGET, ABILITY_INSOMNIA, BattleScript_PrintBankAbilityMadeIneffective
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_ButItFailed
	jumpifsideaffecting BS_TARGET, SIDE_STATUS_SAFEGUARD, BattleScript_SafeguardProtected
	accuracycheck BattleScript_ButItFailed, NO_ACC_CALC_CHECK_LOCK_ON
	jumpifcantmakeasleep BattleScript_ButItFailed
	setyawn BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNWASMADEDROWSY
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd
BattleScript_PrintBankAbilityMadeIneffective::
	copybyte sBATTLER, sBATTLER_WITH_ABILITY
BattleScript_PrintAbilityMadeIneffective::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNSXMADEITINEFFECTIVE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectKnockOff::
	setmoveeffect MOVE_EFFECT_KNOCK_OFF
	goto BattleScript_EffectHit

BattleScript_EffectEndeavor::
	attackcanceler
	attackstring
	ppreduce
	setdamagetohealthdifference BattleScript_ButItFailed
	copyword gHpDealt, gBattleMoveDamage
	accuracycheck BattleScript_MoveMissedPause, ACC_CURR_MOVE
	typecalc
	jumpifmovehadnoeffect BattleScript_HitFromAtkAnimation
	bicbyte gMoveResultFlags, MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE
	copyword gBattleMoveDamage, gHpDealt
	adjustsetdamage
	goto BattleScript_HitFromAtkAnimation

BattleScript_EffectEruption::
	scaledamagebyhealthratio
	goto BattleScript_EffectHit

BattleScript_EffectSkillSwap::
	attackcanceler
	attackstring
	ppreduce
	accuracycheck BattleScript_ButItFailed, NO_ACC_CALC_CHECK_LOCK_ON
	tryswapabilities BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNSWAPPEDABILITIES
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectImprison::
	attackcanceler
	attackstring
	ppreduce
	tryimprison BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNSEALEDOPPONENTMOVE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectRefresh::
	attackcanceler
	attackstring
	ppreduce
	cureifburnedparalysedorpoisoned BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNSTATUSNORMAL
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_ATTACKER
	goto BattleScript_MoveEnd

BattleScript_EffectGrudge::
	attackcanceler
	attackstring
	ppreduce
	trysetgrudge BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNWANTSGRUDGE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectSnatch::
	attackcanceler
	trysetsnatch BattleScript_FailedFromAtkString
	attackstring
	ppreduce
	attackanimation
	waitanimation
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNWAITSFORTARGET
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectLowKick::
	attackcanceler
	attackstring
	ppreduce
	weightdamagecalculation
	accuracycheck BattleScript_MoveMissedPause, ACC_CURR_MOVE
	goto BattleScript_HitFromCritCalc

BattleScript_EffectSecretPower::
	getsecretpowereffect
	goto BattleScript_EffectHit

BattleScript_EffectDoubleEdge::
	setmoveeffect MOVE_EFFECT_RECOIL_33 | MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN
	goto BattleScript_EffectHit

BattleScript_EffectTeeterDance::
	attackcanceler
	attackstring
	ppreduce
	setbyte gBattlerTarget, 0
BattleScript_TeeterDanceLoop::
	movevaluescleanup
	setmoveeffect MOVE_EFFECT_CONFUSION
	jumpifbyteequal gBattlerAttacker, gBattlerTarget, BattleScript_TeeterDanceLoopIncrement
	jumpifability BS_TARGET, ABILITY_OWN_TEMPO, BattleScript_TeeterDanceOwnTempoPrevents
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_TeeterDanceSubstitutePrevents
	jumpifstatus2 BS_TARGET, STATUS2_CONFUSION, BattleScript_TeeterDanceAlreadyConfused
	jumpifhasnohp BS_TARGET, BattleScript_TeeterDanceLoopIncrement
	accuracycheck BattleScript_TeeterDanceMissed, ACC_CURR_MOVE
	jumpifsideaffecting BS_TARGET, SIDE_STATUS_SAFEGUARD, BattleScript_TeeterDanceSafeguardProtected
	attackanimation
	waitanimation
	seteffectprimary
	resultmessage
	waitmessage B_WAIT_TIME_LONG
BattleScript_TeeterDanceDoMoveEndIncrement::
	moveendto MOVEEND_NEXT_TARGET
BattleScript_TeeterDanceLoopIncrement::
	addbyte gBattlerTarget, 1
	jumpifbytenotequal gBattlerTarget, gBattlersCount, BattleScript_TeeterDanceLoop
	end

BattleScript_TeeterDanceOwnTempoPrevents::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNPREVENTSCONFUSIONWITH
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_TeeterDanceDoMoveEndIncrement

BattleScript_TeeterDanceSafeguardProtected::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNUSEDSAFEGUARD
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_TeeterDanceDoMoveEndIncrement

BattleScript_TeeterDanceSubstitutePrevents::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_BUTITFAILED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_TeeterDanceDoMoveEndIncrement

BattleScript_TeeterDanceAlreadyConfused::
	setalreadystatusedmoveattempt BS_ATTACKER
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNALREADYCONFUSED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_TeeterDanceDoMoveEndIncrement

BattleScript_TeeterDanceMissed::
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_TeeterDanceDoMoveEndIncrement

BattleScript_EffectMudSport::
BattleScript_EffectWaterSport::
	attackcanceler
	attackstring
	ppreduce
	settypebasedhalvers BattleScript_ButItFailed
	attackanimation
	waitanimation
	printfromtable gSportsUsedStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectPoisonFang::
	setmoveeffect MOVE_EFFECT_TOXIC
	goto BattleScript_EffectHit

BattleScript_EffectWeatherBall::
	setweatherballtype
	goto BattleScript_EffectHit

BattleScript_EffectOverheat::
	setmoveeffect MOVE_EFFECT_SP_ATK_TWO_DOWN | MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN
	goto BattleScript_EffectHit

BattleScript_EffectTickle::
	attackcanceler
	attackstring
	ppreduce
	jumpifstat BS_TARGET, CMP_GREATER_THAN, STAT_ATK, MIN_STAT_STAGE, BattleScript_TickleDoMoveAnim
	jumpifstat BS_TARGET, CMP_EQUAL, STAT_DEF, MIN_STAT_STAGE, BattleScript_CantLowerMultipleStats
BattleScript_TickleDoMoveAnim::
	accuracycheck BattleScript_ButItFailed, ACC_CURR_MOVE
	attackanimation
	waitanimation
	setbyte sSTAT_ANIM_PLAYED, FALSE
	playstatchangeanimation BS_TARGET, BIT_ATK | BIT_DEF, STAT_CHANGE_NEGATIVE | STAT_CHANGE_MULTIPLE_STATS
	playstatchangeanimation BS_TARGET, BIT_ATK, STAT_CHANGE_NEGATIVE
	setstatchanger STAT_ATK, 1, TRUE
	statbuffchange STAT_CHANGE_ALLOW_PTR, BattleScript_TickleTryLowerDef
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_DECREASE, BattleScript_TickleTryLowerDef
	printfromtable gStatDownStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_TickleTryLowerDef::
	playstatchangeanimation BS_TARGET, BIT_DEF, STAT_CHANGE_NEGATIVE
	setstatchanger STAT_DEF, 1, TRUE
	statbuffchange STAT_CHANGE_ALLOW_PTR, BattleScript_TickleEnd
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_DECREASE, BattleScript_TickleEnd
	printfromtable gStatDownStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_TickleEnd::
	goto BattleScript_MoveEnd

BattleScript_CantLowerMultipleStats::
	pause B_WAIT_TIME_SHORT
	orbyte gMoveResultFlags, MOVE_RESULT_FAILED
	printstring STRINGID_STATSWONTDECREASE2
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectCosmicPower::
	attackcanceler
	attackstring
	ppreduce
	jumpifstat BS_ATTACKER, CMP_LESS_THAN, STAT_DEF, MAX_STAT_STAGE, BattleScript_CosmicPowerDoMoveAnim
	jumpifstat BS_ATTACKER, CMP_EQUAL, STAT_SPDEF, MAX_STAT_STAGE, BattleScript_CantRaiseMultipleStats
BattleScript_CosmicPowerDoMoveAnim::
	attackanimation
	waitanimation
	setbyte sSTAT_ANIM_PLAYED, FALSE
	playstatchangeanimation BS_ATTACKER, BIT_DEF | BIT_SPDEF, 0
	setstatchanger STAT_DEF, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_CosmicPowerTrySpDef
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_CosmicPowerTrySpDef
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_CosmicPowerTrySpDef::
	setstatchanger STAT_SPDEF, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_CosmicPowerEnd
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_CosmicPowerEnd
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_CosmicPowerEnd::
	goto BattleScript_MoveEnd

BattleScript_EffectSkyUppercut::
	orword gHitMarker, HITMARKER_IGNORE_ON_AIR
	goto BattleScript_EffectHit

BattleScript_EffectBulkUp::
	attackcanceler
	attackstring
	ppreduce
	jumpifstat BS_ATTACKER, CMP_LESS_THAN, STAT_ATK, MAX_STAT_STAGE, BattleScript_BulkUpDoMoveAnim
	jumpifstat BS_ATTACKER, CMP_EQUAL, STAT_DEF, MAX_STAT_STAGE, BattleScript_CantRaiseMultipleStats
BattleScript_BulkUpDoMoveAnim::
	attackanimation
	waitanimation
	setbyte sSTAT_ANIM_PLAYED, FALSE
	playstatchangeanimation BS_ATTACKER, BIT_ATK | BIT_DEF, 0
	setstatchanger STAT_ATK, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_BulkUpTryDef
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_BulkUpTryDef
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_BulkUpTryDef::
	setstatchanger STAT_DEF, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_BulkUpEnd
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_BulkUpEnd
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_BulkUpEnd::
	goto BattleScript_MoveEnd

BattleScript_EffectCalmMind::
	attackcanceler
	attackstring
	ppreduce
	jumpifstat BS_ATTACKER, CMP_LESS_THAN, STAT_SPATK, MAX_STAT_STAGE, BattleScript_CalmMindDoMoveAnim
	jumpifstat BS_ATTACKER, CMP_EQUAL, STAT_SPDEF, MAX_STAT_STAGE, BattleScript_CantRaiseMultipleStats
BattleScript_CalmMindDoMoveAnim::
	attackanimation
	waitanimation
	setbyte sSTAT_ANIM_PLAYED, FALSE
	playstatchangeanimation BS_ATTACKER, BIT_SPATK | BIT_SPDEF, 0
	setstatchanger STAT_SPATK, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_CalmMindTrySpDef
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_CalmMindTrySpDef
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_CalmMindTrySpDef::
	setstatchanger STAT_SPDEF, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_CalmMindEnd
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_CalmMindEnd
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_CalmMindEnd::
	goto BattleScript_MoveEnd

BattleScript_CantRaiseMultipleStats::
	pause B_WAIT_TIME_SHORT
	orbyte gMoveResultFlags, MOVE_RESULT_FAILED
	printstring STRINGID_STATSWONTINCREASE2
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_EffectDragonDance::
	attackcanceler
	attackstring
	ppreduce
	jumpifstat BS_ATTACKER, CMP_LESS_THAN, STAT_ATK, MAX_STAT_STAGE, BattleScript_DragonDanceDoMoveAnim
	jumpifstat BS_ATTACKER, CMP_EQUAL, STAT_SPEED, MAX_STAT_STAGE, BattleScript_CantRaiseMultipleStats
BattleScript_DragonDanceDoMoveAnim::
	attackanimation
	waitanimation
	setbyte sSTAT_ANIM_PLAYED, FALSE
	playstatchangeanimation BS_ATTACKER, BIT_ATK | BIT_SPEED, 0
	setstatchanger STAT_ATK, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_DragonDanceTrySpeed
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_DragonDanceTrySpeed
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_DragonDanceTrySpeed::
	setstatchanger STAT_SPEED, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_DragonDanceEnd
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_INCREASE, BattleScript_DragonDanceEnd
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_DragonDanceEnd::
	goto BattleScript_MoveEnd

BattleScript_EffectCamouflage::
	attackcanceler
	attackstring
	ppreduce
	settypetoenvironment BattleScript_ButItFailed
	attackanimation
	waitanimation
	printstring STRINGID_PKMNCHANGEDTYPE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_FaintAttacker::
	playfaintcry BS_ATTACKER
	pause B_WAIT_TIME_LONG
	dofaintanimation BS_ATTACKER
	cleareffectsonfaint BS_ATTACKER
	printstring STRINGID_ATTACKERFAINTED
	return

BattleScript_FaintTarget::
	playfaintcry BS_TARGET
	pause B_WAIT_TIME_LONG
	dofaintanimation BS_TARGET
	cleareffectsonfaint BS_TARGET
	printstring STRINGID_TARGETFAINTED
	return

BattleScript_GiveExp::
	setbyte sGIVEEXP_STATE, 0
	getexp BS_TARGET
	end2

BattleScript_HandleFaintedMon::
	checkteamslost BattleScript_LinkHandleFaintedMonMultiple
	jumpifbyte CMP_NOT_EQUAL, gBattleOutcome, 0, BattleScript_FaintedMonEnd
	jumpifbattletype BATTLE_TYPE_TRAINER, BattleScript_FaintedMonTryChoose
	jumpifword CMP_NO_COMMON_BITS, gHitMarker, HITMARKER_PLAYER_FAINTED, BattleScript_FaintedMonTryChoose
@ Yes/No for sending out a new Pokémon if one is defeated in a wild battle
	printstring STRINGID_USENEXTPKMN
	setbyte gBattleCommunication, 0
	yesnobox
	jumpifbyte CMP_EQUAL, gBattleCommunication + 1, 0, BattleScript_FaintedMonTryChoose
@ Player said no, try to run
	jumpifplayerran BattleScript_FaintedMonEnd
	printstring STRINGID_CANTESCAPE2
BattleScript_FaintedMonTryChoose::
	openpartyscreen BS_FAINTED, BattleScript_FaintedMonEnd
	switchhandleorder BS_FAINTED, 2
	jumpifnotbattletype BATTLE_TYPE_TRAINER, BattleScript_FaintedMonSendOutNew
	jumpifbattletype BATTLE_TYPE_LINK, BattleScript_FaintedMonSendOutNew
	jumpifbattletype BATTLE_TYPE_RECORDED_LINK, BattleScript_FaintedMonSendOutNew
	jumpifbattletype BATTLE_TYPE_FRONTIER, BattleScript_FaintedMonSendOutNew
	jumpifbattletype BATTLE_TYPE_DOUBLE, BattleScript_FaintedMonSendOutNew
	jumpifword CMP_COMMON_BITS, gHitMarker, HITMARKER_PLAYER_FAINTED, BattleScript_FaintedMonSendOutNew
	jumpifbyte CMP_EQUAL, sBATTLE_STYLE, OPTIONS_BATTLE_STYLE_SET, BattleScript_FaintedMonSendOutNew
	jumpifcantswitch BS_PLAYER1, BattleScript_FaintedMonSendOutNew
@ Yes/No for sending out a new Pokémon when the opponent is switching
	printstring STRINGID_ENEMYABOUTTOSWITCHPKMN
	setbyte gBattleCommunication, 0
	yesnobox
	jumpifbyte CMP_EQUAL, gBattleCommunication + 1, 1, BattleScript_FaintedMonSendOutNew
@ Player said yes, go to party screen (note optional flag, player may exit the menu instead)
	setatktoplayer0
	openpartyscreen BS_ATTACKER | PARTY_SCREEN_OPTIONAL, BattleScript_FaintedMonSendOutNew
	switchhandleorder BS_ATTACKER, 2
	jumpifbyte CMP_EQUAL, gBattleCommunication, PARTY_SIZE, BattleScript_FaintedMonSendOutNew
@ Switch Pokémon before opponent
	atknameinbuff1
	resetintimidatetracebits BS_ATTACKER
	hpthresholds2 BS_ATTACKER
	printstring STRINGID_RETURNMON
	switchoutabilities BS_ATTACKER
	waitstate
	returnatktoball
	waitstate
	drawpartystatussummary BS_ATTACKER
	getswitchedmondata BS_ATTACKER
	switchindataupdate BS_ATTACKER
	hpthresholds BS_ATTACKER
	printstring STRINGID_SWITCHINMON
	hidepartystatussummary BS_ATTACKER
	switchinanim BS_ATTACKER, 0
	waitstate
	switchineffects BS_ATTACKER
	resetsentmonsvalue
BattleScript_FaintedMonSendOutNew::
	drawpartystatussummary BS_FAINTED
	getswitchedmondata BS_FAINTED
	switchindataupdate BS_FAINTED
	hpthresholds BS_FAINTED
	printstring STRINGID_SWITCHINMON
	hidepartystatussummary BS_FAINTED
	switchinanim BS_FAINTED, FALSE
	waitstate
	resetplayerfainted
	switchineffects BS_FAINTED
	jumpifbattletype BATTLE_TYPE_DOUBLE, BattleScript_FaintedMonEnd
	cancelallactions
BattleScript_FaintedMonEnd::
	end2

BattleScript_LinkHandleFaintedMonMultiple::
	openpartyscreen BS_FAINTED_LINK_MULTIPLE_1, BattleScript_LinkHandleFaintedMonMultipleStart
BattleScript_LinkHandleFaintedMonMultipleStart::
	switchhandleorder BS_FAINTED, 0
	openpartyscreen BS_FAINTED_LINK_MULTIPLE_2, BattleScript_LinkHandleFaintedMonMultipleEnd
	switchhandleorder BS_FAINTED, 0
BattleScript_LinkHandleFaintedMonLoop::
	switchhandleorder BS_FAINTED, 3
	drawpartystatussummary BS_FAINTED
	getswitchedmondata BS_FAINTED
	switchindataupdate BS_FAINTED
	hpthresholds BS_FAINTED
	printstring STRINGID_SWITCHINMON
	hidepartystatussummary BS_FAINTED
	switchinanim BS_FAINTED, FALSE
	waitstate
	switchineffects BS_FAINTED_LINK_MULTIPLE_1
	jumpifbytenotequal gBattlerFainted, gBattlersCount, BattleScript_LinkHandleFaintedMonLoop
BattleScript_LinkHandleFaintedMonMultipleEnd::
	end2

BattleScript_LocalTrainerBattleWon::
	jumpifbattletype BATTLE_TYPE_TWO_OPPONENTS, BattleScript_LocalTwoTrainersDefeated
	printstring STRINGID_PLAYERDEFEATEDTRAINER1
	goto BattleScript_LocalBattleWonLoseTexts
BattleScript_LocalTwoTrainersDefeated::
	printstring STRINGID_TWOENEMIESDEFEATED
BattleScript_LocalBattleWonLoseTexts::
	trainerslidein BS_ATTACKER
	waitstate
	printstring STRINGID_TRAINER1LOSETEXT
	jumpifnotbattletype BATTLE_TYPE_TWO_OPPONENTS, BattleScript_LocalBattleWonReward
	trainerslideout B_POSITION_OPPONENT_LEFT
	waitstate
	trainerslidein BS_FAINTED
	waitstate
	printstring STRINGID_TRAINER2LOSETEXT
BattleScript_LocalBattleWonReward::
	getmoneyreward
	printstring STRINGID_PLAYERGOTMONEY
	waitmessage B_WAIT_TIME_LONG
BattleScript_PayDayMoneyAndPickUpItems::
	givepaydaymoney
	pickup
	end2

BattleScript_LocalBattleLost::
	jumpifbattletype BATTLE_TYPE_DOME, BattleScript_CheckDomeDrew
	jumpifbattletype BATTLE_TYPE_FRONTIER, BattleScript_LocalBattleLostPrintTrainersWinText
	jumpifbattletype BATTLE_TYPE_TRAINER_HILL, BattleScript_LocalBattleLostPrintTrainersWinText
	jumpifbattletype BATTLE_TYPE_EREADER_TRAINER, BattleScript_LocalBattleLostEnd
	jumpifhalfword CMP_EQUAL, gTrainerBattleOpponent_A, TRAINER_SECRET_BASE, BattleScript_LocalBattleLostEnd
BattleScript_LocalBattleLostPrintWhiteOut::
	printstring STRINGID_PLAYERWHITEOUT
	waitmessage B_WAIT_TIME_LONG
	printstring STRINGID_PLAYERWHITEOUT2
	waitmessage B_WAIT_TIME_LONG
BattleScript_LocalBattleLostEnd::
	end2
BattleScript_CheckDomeDrew::
	jumpifbyte CMP_EQUAL, gBattleOutcome, B_OUTCOME_DREW, BattleScript_LocalBattleLostEnd_
BattleScript_LocalBattleLostPrintTrainersWinText::
	jumpifnotbattletype BATTLE_TYPE_TRAINER, BattleScript_LocalBattleLostPrintWhiteOut
	returnopponentmon1toball BS_ATTACKER
	waitstate
	returnopponentmon2toball BS_ATTACKER
	waitstate
	trainerslidein BS_ATTACKER
	waitstate
	printstring STRINGID_TRAINER1WINTEXT
	jumpifbattletype BATTLE_TYPE_TOWER_LINK_MULTI, BattleScript_LocalBattleLostDoTrainer2WinText
	jumpifnotbattletype BATTLE_TYPE_TWO_OPPONENTS, BattleScript_LocalBattleLostEnd_
BattleScript_LocalBattleLostDoTrainer2WinText::
	trainerslideout B_POSITION_OPPONENT_LEFT
	waitstate
	trainerslidein BS_FAINTED
	waitstate
	printstring STRINGID_TRAINER2WINTEXT
BattleScript_LocalBattleLostEnd_::
	end2

BattleScript_FrontierLinkBattleLost::
	returnopponentmon1toball BS_ATTACKER
	waitstate
	returnopponentmon2toball BS_ATTACKER
	waitstate
	trainerslidein BS_ATTACKER
	waitstate
	printstring STRINGID_TRAINER1WINTEXT
	trainerslideout B_POSITION_OPPONENT_LEFT
	waitstate
	trainerslidein BS_FAINTED
	waitstate
	printstring STRINGID_TRAINER2WINTEXT
	jumpifbattletype BATTLE_TYPE_RECORDED, BattleScript_FrontierLinkBattleLostEnd
	endlinkbattle
BattleScript_FrontierLinkBattleLostEnd::
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_LinkBattleWonOrLost::
	jumpifbattletype BATTLE_TYPE_BATTLE_TOWER, BattleScript_TowerLinkBattleWon
	printstring STRINGID_BATTLEEND
	waitmessage B_WAIT_TIME_LONG
	jumpifbattletype BATTLE_TYPE_RECORDED, BattleScript_LinkBattleWonOrLostWaitEnd
	endlinkbattle
BattleScript_LinkBattleWonOrLostWaitEnd::
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_TowerLinkBattleWon::
	playtrainerdefeatbgm BS_ATTACKER
	printstring STRINGID_BATTLEEND
	waitmessage B_WAIT_TIME_LONG
	trainerslidein BS_ATTACKER
	waitstate
	printstring STRINGID_TRAINER1LOSETEXT
	trainerslideout B_POSITION_OPPONENT_LEFT
	waitstate
	trainerslidein BS_FAINTED
	waitstate
	printstring STRINGID_TRAINER2LOSETEXT
	jumpifbattletype BATTLE_TYPE_RECORDED, BattleScript_TowerLinkBattleWonEnd
	endlinkbattle
BattleScript_TowerLinkBattleWonEnd::
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_FrontierTrainerBattleWon::
	jumpifnotbattletype BATTLE_TYPE_TRAINER, BattleScript_PayDayMoneyAndPickUpItems
	jumpifbattletype BATTLE_TYPE_TWO_OPPONENTS, BattleScript_FrontierTrainerBattleWon_TwoDefeated
	printstring STRINGID_PLAYERDEFEATEDTRAINER1
	goto BattleScript_FrontierTrainerBattleWon_LoseTexts
BattleScript_FrontierTrainerBattleWon_TwoDefeated:
	printstring STRINGID_TWOENEMIESDEFEATED
BattleScript_FrontierTrainerBattleWon_LoseTexts:
	trainerslidein BS_ATTACKER
	waitstate
	printstring STRINGID_TRAINER1LOSETEXT
	jumpifnotbattletype BATTLE_TYPE_TWO_OPPONENTS, BattleScript_TryPickUpItems
	trainerslideout B_POSITION_OPPONENT_LEFT
	waitstate
	trainerslidein BS_FAINTED
	waitstate
	printstring STRINGID_TRAINER2LOSETEXT
BattleScript_TryPickUpItems:
	jumpifnotbattletype BATTLE_TYPE_PYRAMID, BattleScript_FrontierTrainerBattleWon_End
	pickup
BattleScript_FrontierTrainerBattleWon_End:
	end2

BattleScript_SmokeBallEscape::
	playanimation BS_ATTACKER, B_ANIM_SMOKEBALL_ESCAPE
	printstring STRINGID_PKMNFLEDUSINGITS
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_RanAwayUsingMonAbility::
	printstring STRINGID_PKMNFLEDUSING
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_GotAwaySafely::
	printstring STRINGID_GOTAWAYSAFELY
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_WildMonFled::
	printstring STRINGID_WILDPKMNFLED
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_PrintCantRunFromTrainer::
	printstring STRINGID_NORUNNINGFROMTRAINERS
	end2

BattleScript_PrintFailedToRunString::
	printfromtable gNoEscapeStringIds
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_PrintCantEscapeFromBattle::
	printselectionstringfromtable gNoEscapeStringIds
	endselectionscript

BattleScript_PrintFullBox::
	printselectionstring STRINGID_BOXISFULL
	endselectionscript

BattleScript_ActionSwitch::
	hpthresholds2 BS_ATTACKER
	printstring STRINGID_RETURNMON
	setbyte sDMG_MULTIPLIER, 2
	jumpifbattletype BATTLE_TYPE_DOUBLE, BattleScript_PursuitSwitchDmgSetMultihit
	setmultihit 1
	goto BattleScript_PursuitSwitchDmgLoop
BattleScript_PursuitSwitchDmgSetMultihit::
	setmultihit 2
BattleScript_PursuitSwitchDmgLoop::
	jumpifnopursuitswitchdmg BattleScript_DoSwitchOut
	swapattackerwithtarget
	trysetdestinybondtohappen
	call BattleScript_PursuitDmgOnSwitchOut
	swapattackerwithtarget
BattleScript_DoSwitchOut::
	decrementmultihit BattleScript_PursuitSwitchDmgLoop
	switchoutabilities BS_ATTACKER
	waitstate
	returnatktoball
	waitstate
	drawpartystatussummary BS_ATTACKER
	switchhandleorder BS_ATTACKER, 1
	getswitchedmondata BS_ATTACKER
	switchindataupdate BS_ATTACKER
	hpthresholds BS_ATTACKER
	printstring STRINGID_SWITCHINMON
	hidepartystatussummary BS_ATTACKER
	switchinanim BS_ATTACKER, FALSE
	waitstate
	switchineffects BS_ATTACKER
	moveendcase MOVEEND_IMMUNITY_ABILITIES
	moveendcase MOVEEND_MIRROR_MOVE
	end2

BattleScript_PursuitDmgOnSwitchOut::
	pause B_WAIT_TIME_SHORT
	attackstring
	ppreduce
	critcalc
	damagecalc
	typecalc
	adjustnormaldamage
	attackanimation
	waitanimation
	effectivenesssound
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	critmessage
	waitmessage B_WAIT_TIME_LONG
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	tryfaintmon BS_TARGET
	moveendfromto MOVEEND_ON_DAMAGE_ABILITIES, MOVEEND_CHOICE_MOVE
	getbattlerfainted BS_TARGET
	jumpifbyte CMP_EQUAL, gBattleCommunication, FALSE, BattleScript_PursuitDmgOnSwitchOutRet
	setbyte sGIVEEXP_STATE, 0
	getexp BS_TARGET
BattleScript_PursuitDmgOnSwitchOutRet:
	return

BattleScript_Pausex20::
	pause B_WAIT_TIME_SHORT
	return

BattleScript_LevelUp::
	fanfare MUS_LEVEL_UP
	printstring STRINGID_PKMNGREWTOLV
	setbyte sLVLBOX_STATE, 0
	drawlvlupbox
	handlelearnnewmove BattleScript_LearnedNewMove, BattleScript_LearnMoveReturn, TRUE
	goto BattleScript_AskToLearnMove
BattleScript_TryLearnMoveLoop::
	handlelearnnewmove BattleScript_LearnedNewMove, BattleScript_LearnMoveReturn, FALSE
BattleScript_AskToLearnMove::
	buffermovetolearn
	printstring STRINGID_TRYTOLEARNMOVE1
	printstring STRINGID_TRYTOLEARNMOVE2
	printstring STRINGID_TRYTOLEARNMOVE3
	waitstate
	setbyte sLEARNMOVE_STATE, 0
	yesnoboxlearnmove BattleScript_ForgotAndLearnedNewMove
	printstring STRINGID_STOPLEARNINGMOVE
	waitstate
	setbyte sLEARNMOVE_STATE, 0
	yesnoboxstoplearningmove BattleScript_AskToLearnMove
	printstring STRINGID_DIDNOTLEARNMOVE
	goto BattleScript_TryLearnMoveLoop
BattleScript_ForgotAndLearnedNewMove::
	printstring STRINGID_123POOF
	printstring STRINGID_PKMNFORGOTMOVE
	printstring STRINGID_ANDELLIPSIS
BattleScript_LearnedNewMove::
	buffermovetolearn
	fanfare MUS_LEVEL_UP
	printstring STRINGID_PKMNLEARNEDMOVE
	waitmessage B_WAIT_TIME_LONG
	updatechoicemoveonlvlup BS_ATTACKER
	goto BattleScript_TryLearnMoveLoop
BattleScript_LearnMoveReturn::
	return

BattleScript_RainContinuesOrEnds::
	printfromtable gRainContinuesStringIds
	waitmessage B_WAIT_TIME_LONG
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_RAIN_STOPPED, BattleScript_RainContinuesOrEndsEnd
	playanimation BS_ATTACKER, B_ANIM_RAIN_CONTINUES
BattleScript_RainContinuesOrEndsEnd::
	end2

BattleScript_DamagingWeatherContinues::
	printfromtable gSandStormHailContinuesStringIds
	waitmessage B_WAIT_TIME_LONG
	playanimation_var BS_ATTACKER, sB_ANIM_ARG1
	setbyte gBattleCommunication, 0
BattleScript_DamagingWeatherLoop::
	copyarraywithindex gBattlerAttacker, gBattlerByTurnOrder, gBattleCommunication, 1
	weatherdamage
	jumpifword CMP_EQUAL, gBattleMoveDamage, 0, BattleScript_DamagingWeatherLoopIncrement
	printfromtable gSandStormHailDmgStringIds
	waitmessage B_WAIT_TIME_LONG
	orword gHitMarker, HITMARKER_IGNORE_BIDE | HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE | HITMARKER_GRUDGE
	effectivenesssound
	hitanimation BS_ATTACKER
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	tryfaintmon BS_ATTACKER
	checkteamslost BattleScript_DamagingWeatherLoopIncrement
BattleScript_DamagingWeatherLoopIncrement::
	jumpifbyte CMP_NOT_EQUAL, gBattleOutcome, 0, BattleScript_DamagingWeatherContinuesEnd
	addbyte gBattleCommunication, 1
	jumpifbytenotequal gBattleCommunication, gBattlersCount, BattleScript_DamagingWeatherLoop
BattleScript_DamagingWeatherContinuesEnd::
	bicword gHitMarker, HITMARKER_IGNORE_BIDE | HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE | HITMARKER_GRUDGE
	end2

BattleScript_SandStormHailEnds::
	printfromtable gSandStormHailEndStringIds
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_SunlightContinues::
	printstring STRINGID_SUNLIGHTSTRONG
	waitmessage B_WAIT_TIME_LONG
	playanimation BS_ATTACKER, B_ANIM_SUN_CONTINUES
	end2

BattleScript_SunlightFaded::
	printstring STRINGID_SUNLIGHTFADED
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_OverworldWeatherStarts::
	printfromtable gWeatherStartsStringIds
	waitmessage B_WAIT_TIME_LONG
	playanimation_var BS_ATTACKER, sB_ANIM_ARG1
	end3

BattleScript_SideStatusWoreOff::
	printstring STRINGID_PKMNSXWOREOFF
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_SafeguardProtected::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNUSEDSAFEGUARD
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_SafeguardEnds::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNSAFEGUARDEXPIRED
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_LeechSeedTurnDrain::
	playanimation BS_ATTACKER, B_ANIM_LEECH_SEED_DRAIN, sB_ANIM_ARG1
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	copyword gBattleMoveDamage, gHpDealt
	jumpifability BS_ATTACKER, ABILITY_LIQUID_OOZE, BattleScript_LeechSeedTurnPrintLiquidOoze
	manipulatedamage DMG_CHANGE_SIGN
	setbyte cMULTISTRING_CHOOSER, B_MSG_LEECH_SEED_DRAIN
	goto BattleScript_LeechSeedTurnPrintAndUpdateHp
BattleScript_LeechSeedTurnPrintLiquidOoze::
	setbyte cMULTISTRING_CHOOSER, B_MSG_LEECH_SEED_OOZE
BattleScript_LeechSeedTurnPrintAndUpdateHp::
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	printfromtable gLeechSeedStringIds
	waitmessage B_WAIT_TIME_LONG
	tryfaintmon BS_ATTACKER
	tryfaintmon BS_TARGET
	end2

BattleScript_BideStoringEnergy::
	printstring STRINGID_PKMNSTORINGENERGY
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_BideAttack::
	attackcanceler
	setmoveeffect MOVE_EFFECT_CHARGING
	clearstatusfromeffect BS_ATTACKER
	printstring STRINGID_PKMNUNLEASHEDENERGY
	waitmessage B_WAIT_TIME_LONG
	accuracycheck BattleScript_MoveMissed, ACC_CURR_MOVE
	typecalc
	bicbyte gMoveResultFlags, MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE
	copyword gBattleMoveDamage, sBIDE_DMG
	adjustsetdamage
	setbyte sB_ANIM_TURN, 1
	attackanimation
	waitanimation
	effectivenesssound
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	tryfaintmon BS_TARGET
	goto BattleScript_MoveEnd

BattleScript_BideNoEnergyToAttack::
	attackcanceler
	setmoveeffect MOVE_EFFECT_CHARGING
	clearstatusfromeffect BS_ATTACKER
	printstring STRINGID_PKMNUNLEASHEDENERGY
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_ButItFailed

BattleScript_SuccessForceOut::
	attackanimation
	waitanimation
	switchoutabilities BS_TARGET
	returntoball BS_TARGET
	waitstate
	jumpifbattletype BATTLE_TYPE_TRAINER, BattleScript_TrainerBattleForceOut
	setoutcomeonteleport BS_ATTACKER
	finishaction

BattleScript_TrainerBattleForceOut::
	getswitchedmondata BS_TARGET
	switchindataupdate BS_TARGET
	switchinanim BS_TARGET, FALSE
	waitstate
	printstring STRINGID_PKMNWASDRAGGEDOUT
	switchineffects BS_TARGET
	goto BattleScript_MoveEnd

BattleScript_MistProtected::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNPROTECTEDBYMIST
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_RageIsBuilding::
	printstring STRINGID_PKMNRAGEBUILDING
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_MoveUsedIsDisabled::
	printstring STRINGID_PKMNMOVEISDISABLED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_SelectingDisabledMove::
	printselectionstring STRINGID_PKMNMOVEISDISABLED
	endselectionscript

BattleScript_DisabledNoMore::
	printstring STRINGID_PKMNMOVEDISABLEDNOMORE
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_SelectingDisabledMoveInPalace::
	printstring STRINGID_PKMNMOVEISDISABLED
BattleScript_SelectingUnusableMoveInPalace::
	moveendto MOVEEND_NEXT_TARGET
	end

BattleScript_EncoredNoMore::
	printstring STRINGID_PKMNENCOREENDED
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_DestinyBondTakesLife::
	printstring STRINGID_PKMNTOOKFOE
	waitmessage B_WAIT_TIME_LONG
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	tryfaintmon BS_ATTACKER
	return

BattleScript_SpikesOnAttacker::
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	call BattleScript_PrintHurtBySpikes
	tryfaintmon BS_ATTACKER
	tryfaintmon_spikes BS_ATTACKER, BattleScript_SpikesOnAttackerFainted
	return

BattleScript_SpikesOnAttackerFainted::
	setbyte sGIVEEXP_STATE, 0
	getexp BS_ATTACKER
	moveendall
	goto BattleScript_HandleFaintedMon

BattleScript_SpikesOnTarget::
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	call BattleScript_PrintHurtBySpikes
	tryfaintmon BS_TARGET
	tryfaintmon_spikes BS_TARGET, BattleScript_SpikesOnTargetFainted
	return

BattleScript_SpikesOnTargetFainted::
	setbyte sGIVEEXP_STATE, 0
	getexp BS_TARGET
	moveendall
	goto BattleScript_HandleFaintedMon

BattleScript_SpikesOnFaintedBattler::
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE
	healthbarupdate BS_FAINTED
	datahpupdate BS_FAINTED
	call BattleScript_PrintHurtBySpikes
	tryfaintmon BS_FAINTED
	tryfaintmon_spikes BS_FAINTED, BattleScript_SpikesOnFaintedBattlerFainted
	return

BattleScript_SpikesOnFaintedBattlerFainted::
	setbyte sGIVEEXP_STATE, 0
	getexp BS_FAINTED
	moveendall
	goto BattleScript_HandleFaintedMon

BattleScript_PrintHurtBySpikes::
	printstring STRINGID_PKMNHURTBYSPIKES
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_PerishSongTakesLife::
	printstring STRINGID_PKMNPERISHCOUNTFELL
	waitmessage B_WAIT_TIME_LONG
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	tryfaintmon BS_ATTACKER
	end2

BattleScript_PerishSongCountGoesDown::
	printstring STRINGID_PKMNPERISHCOUNTFELL
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_AllStatsUp::
	jumpifstat BS_ATTACKER, CMP_LESS_THAN, STAT_ATK, MAX_STAT_STAGE, BattleScript_AllStatsUpAtk
	jumpifstat BS_ATTACKER, CMP_LESS_THAN, STAT_DEF, MAX_STAT_STAGE, BattleScript_AllStatsUpAtk
	jumpifstat BS_ATTACKER, CMP_LESS_THAN, STAT_SPEED, MAX_STAT_STAGE, BattleScript_AllStatsUpAtk
	jumpifstat BS_ATTACKER, CMP_LESS_THAN, STAT_SPATK, MAX_STAT_STAGE, BattleScript_AllStatsUpAtk
	jumpifstat BS_ATTACKER, CMP_EQUAL, STAT_SPDEF, MAX_STAT_STAGE, BattleScript_AllStatsUpRet
BattleScript_AllStatsUpAtk::
	setbyte sSTAT_ANIM_PLAYED, FALSE
	playstatchangeanimation BS_ATTACKER, BIT_ATK | BIT_DEF | BIT_SPEED | BIT_SPATK | BIT_SPDEF, 0
	setstatchanger STAT_ATK, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_AllStatsUpDef
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_AllStatsUpDef::
	setstatchanger STAT_DEF, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_AllStatsUpSpeed
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_AllStatsUpSpeed::
	setstatchanger STAT_SPEED, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_AllStatsUpSpAtk
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_AllStatsUpSpAtk::
	setstatchanger STAT_SPATK, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_AllStatsUpSpDef
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_AllStatsUpSpDef::
	setstatchanger STAT_SPDEF, 1, FALSE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_AllStatsUpRet
	printfromtable gStatUpStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_AllStatsUpRet::
	return

BattleScript_RapidSpinAway::
	rapidspinfree
	return

BattleScript_WrapFree::
	printstring STRINGID_PKMNGOTFREE
	waitmessage B_WAIT_TIME_LONG
	copybyte gBattlerTarget, sBATTLER
	return

BattleScript_LeechSeedFree::
	printstring STRINGID_PKMNSHEDLEECHSEED
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_SpikesFree::
	printstring STRINGID_PKMNBLEWAWAYSPIKES
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_MonTookFutureAttack::
	printstring STRINGID_PKMNTOOKATTACK
	waitmessage B_WAIT_TIME_LONG
	jumpifbyte CMP_NOT_EQUAL, cMULTISTRING_CHOOSER, B_MSG_FUTURE_SIGHT, BattleScript_CheckDoomDesireMiss
	accuracycheck BattleScript_FutureAttackMiss, MOVE_FUTURE_SIGHT
	goto BattleScript_FutureAttackAnimate
BattleScript_CheckDoomDesireMiss::
	accuracycheck BattleScript_FutureAttackMiss, MOVE_DOOM_DESIRE
BattleScript_FutureAttackAnimate::
	adjustnormaldamage2
	jumpifbyte CMP_NOT_EQUAL, cMULTISTRING_CHOOSER, B_MSG_FUTURE_SIGHT, BattleScript_FutureHitAnimDoomDesire
	playanimation BS_ATTACKER, B_ANIM_FUTURE_SIGHT_HIT
	goto BattleScript_DoFutureAttackHit
BattleScript_FutureHitAnimDoomDesire::
	playanimation BS_ATTACKER, B_ANIM_DOOM_DESIRE_HIT
BattleScript_DoFutureAttackHit::
	effectivenesssound
	hitanimation BS_TARGET
	waitstate
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	tryfaintmon BS_TARGET
	checkteamslost BattleScript_FutureAttackEnd
BattleScript_FutureAttackEnd::
	moveendcase MOVEEND_RAGE
	moveendfromto MOVEEND_ITEM_EFFECTS_ALL, MOVEEND_UPDATE_LAST_MOVES
	setbyte gMoveResultFlags, 0
	end2

BattleScript_FutureAttackMiss::
	pause B_WAIT_TIME_SHORT
	setbyte gMoveResultFlags, 0
	orbyte gMoveResultFlags, MOVE_RESULT_FAILED
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	setbyte gMoveResultFlags, 0
	end2

BattleScript_NoMovesLeft::
	printselectionstring STRINGID_PKMNHASNOMOVESLEFT
	endselectionscript

BattleScript_SelectingMoveWithNoPP::
	printselectionstring STRINGID_NOPPLEFT
	endselectionscript

BattleScript_NoPPForMove::
	attackstring
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_BUTNOPPLEFT
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_SelectingTormentedMove::
	printselectionstring STRINGID_PKMNCANTUSEMOVETORMENT
	endselectionscript

BattleScript_MoveUsedIsTormented::
	printstring STRINGID_PKMNCANTUSEMOVETORMENT
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_SelectingTormentedMoveInPalace::
	printstring STRINGID_PKMNCANTUSEMOVETORMENT
	goto BattleScript_SelectingUnusableMoveInPalace

BattleScript_SelectingNotAllowedMoveTaunt::
	printselectionstring STRINGID_PKMNCANTUSEMOVETAUNT
	endselectionscript

BattleScript_MoveUsedIsTaunted::
	printstring STRINGID_PKMNCANTUSEMOVETAUNT
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_SelectingNotAllowedMoveTauntInPalace::
	printstring STRINGID_PKMNCANTUSEMOVETAUNT
	goto BattleScript_SelectingUnusableMoveInPalace

BattleScript_WishComesTrue::
	trywish 1, BattleScript_WishButFullHp
	playanimation BS_TARGET, B_ANIM_WISH_HEAL
	printstring STRINGID_PKMNWISHCAMETRUE
	waitmessage B_WAIT_TIME_LONG
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	printstring STRINGID_PKMNREGAINEDHEALTH
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_WishButFullHp::
	printstring STRINGID_PKMNWISHCAMETRUE
	waitmessage B_WAIT_TIME_LONG
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNHPFULL
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_IngrainTurnHeal::
	playanimation BS_ATTACKER, B_ANIM_INGRAIN_HEAL
	printstring STRINGID_PKMNABSORBEDNUTRIENTS
	waitmessage B_WAIT_TIME_LONG
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	end2

BattleScript_PrintMonIsRooted::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNANCHOREDITSELF
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_AtkDefDown::
	setbyte sSTAT_ANIM_PLAYED, FALSE
	playstatchangeanimation BS_ATTACKER, BIT_DEF | BIT_ATK, STAT_CHANGE_CANT_PREVENT | STAT_CHANGE_NEGATIVE | STAT_CHANGE_MULTIPLE_STATS
	playstatchangeanimation BS_ATTACKER, BIT_ATK, STAT_CHANGE_CANT_PREVENT | STAT_CHANGE_NEGATIVE
	setstatchanger STAT_ATK, 1, TRUE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN | STAT_CHANGE_ALLOW_PTR, BattleScript_AtkDefDown_TryDef
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_DECREASE, BattleScript_AtkDefDown_TryDef
	printfromtable gStatDownStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_AtkDefDown_TryDef::
	playstatchangeanimation BS_ATTACKER, BIT_DEF, STAT_CHANGE_CANT_PREVENT | STAT_CHANGE_NEGATIVE
	setstatchanger STAT_DEF, 1, TRUE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN | STAT_CHANGE_ALLOW_PTR, BattleScript_AtkDefDown_End
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_DECREASE, BattleScript_AtkDefDown_End
	printfromtable gStatDownStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_AtkDefDown_End::
	return

BattleScript_KnockedOff::
	playanimation BS_TARGET, B_ANIM_ITEM_KNOCKOFF
	printstring STRINGID_PKMNKNOCKEDOFF
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_MoveUsedIsImprisoned::
	printstring STRINGID_PKMNCANTUSEMOVESEALED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_SelectingImprisonedMove::
	printselectionstring STRINGID_PKMNCANTUSEMOVESEALED
	endselectionscript

BattleScript_SelectingImprisonedMoveInPalace::
	printstring STRINGID_PKMNCANTUSEMOVESEALED
	goto BattleScript_SelectingUnusableMoveInPalace

BattleScript_GrudgeTakesPp::
	printstring STRINGID_PKMNLOSTPPGRUDGE
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_MagicCoatBounce::
	attackstring
	ppreduce
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNMOVEBOUNCED
	waitmessage B_WAIT_TIME_LONG
	orword gHitMarker, HITMARKER_ATTACKSTRING_PRINTED | HITMARKER_NO_PPDEDUCT | HITMARKER_ALLOW_NO_PP
	setmagiccoattarget BS_ATTACKER
	return

BattleScript_SnatchedMove::
	attackstring
	ppreduce
	snatchsetbattlers
	playanimation BS_TARGET, B_ANIM_SNATCH_MOVE
	printstring STRINGID_PKMNSNATCHEDMOVE
	waitmessage B_WAIT_TIME_LONG
	orword gHitMarker, HITMARKER_ATTACKSTRING_PRINTED | HITMARKER_NO_PPDEDUCT | HITMARKER_ALLOW_NO_PP
	swapattackerwithtarget
	return

BattleScript_EnduredMsg::
	printstring STRINGID_PKMNENDUREDHIT
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_OneHitKOMsg::
	printstring STRINGID_ONEHITKO
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_SAtkDown2::
	setbyte sSTAT_ANIM_PLAYED, FALSE
	playstatchangeanimation BS_ATTACKER, BIT_SPATK, STAT_CHANGE_CANT_PREVENT | STAT_CHANGE_NEGATIVE | STAT_CHANGE_BY_TWO
	setstatchanger STAT_SPATK, 2, TRUE
	statbuffchange MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN | STAT_CHANGE_ALLOW_PTR, BattleScript_SAtkDown2End
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, B_MSG_STAT_WONT_DECREASE, BattleScript_SAtkDown2End
	printfromtable gStatDownStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_SAtkDown2End::
	return

BattleScript_FocusPunchSetUp::
	printstring STRINGID_EMPTYSTRING3
	waitmessage 1
	playanimation BS_ATTACKER, B_ANIM_FOCUS_PUNCH_SETUP
	printstring STRINGID_PKMNTIGHTENINGFOCUS
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_MoveUsedIsAsleep::
	printstring STRINGID_PKMNFASTASLEEP
	waitmessage B_WAIT_TIME_LONG
	statusanimation BS_ATTACKER
	goto BattleScript_MoveEnd

BattleScript_MoveUsedWokeUp::
	bicword gHitMarker, HITMARKER_WAKE_UP_CLEAR
	printfromtable gWokeUpStringIds
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_ATTACKER
	return

BattleScript_MonWokeUpInUproar::
	printstring STRINGID_PKMNWOKEUPINUPROAR
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_ATTACKER
	end2

BattleScript_PoisonTurnDmg::
	printstring STRINGID_PKMNHURTBYPOISON
	waitmessage B_WAIT_TIME_LONG
BattleScript_DoStatusTurnDmg::
	statusanimation BS_ATTACKER
BattleScript_DoTurnDmg::
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	tryfaintmon BS_ATTACKER
	checkteamslost BattleScript_DoTurnDmgEnd
BattleScript_DoTurnDmgEnd::
	end2

BattleScript_BurnTurnDmg::
	printstring STRINGID_PKMNHURTBYBURN
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_DoStatusTurnDmg

BattleScript_MoveUsedIsFrozen::
	printstring STRINGID_PKMNISFROZEN
	waitmessage B_WAIT_TIME_LONG
	statusanimation BS_ATTACKER
	goto BattleScript_MoveEnd

BattleScript_MoveUsedUnfroze::
	printfromtable gGotDefrostedStringIds
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_ATTACKER
	return

BattleScript_DefrostedViaFireMove::
	printstring STRINGID_PKMNWASDEFROSTED
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_TARGET
	return

BattleScript_MoveUsedIsParalyzed::
	printstring STRINGID_PKMNISPARALYZED
	waitmessage B_WAIT_TIME_LONG
	statusanimation BS_ATTACKER
	cancelmultiturnmoves BS_ATTACKER
	goto BattleScript_MoveEnd

BattleScript_MoveUsedFlinched::
	printstring STRINGID_PKMNFLINCHED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_PrintUproarOverTurns::
	printfromtable gUproarOverTurnStringIds
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_ThrashConfuses::
	chosenstatus2animation BS_ATTACKER, STATUS2_CONFUSION
	printstring STRINGID_PKMNFATIGUECONFUSION
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_MoveUsedIsConfused::
	printstring STRINGID_PKMNISCONFUSED
	waitmessage B_WAIT_TIME_LONG
	status2animation BS_ATTACKER, STATUS2_CONFUSION
	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, FALSE, BattleScript_MoveUsedIsConfusedRet
BattleScript_DoSelfConfusionDmg::
	cancelmultiturnmoves BS_ATTACKER
	adjustnormaldamage2
	printstring STRINGID_ITHURTCONFUSION
	waitmessage B_WAIT_TIME_LONG
	effectivenesssound
	hitanimation BS_ATTACKER
	waitstate
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	resultmessage
	waitmessage B_WAIT_TIME_LONG
	tryfaintmon BS_ATTACKER
	goto BattleScript_MoveEnd
BattleScript_MoveUsedIsConfusedRet::
	return

BattleScript_MoveUsedIsConfusedNoMore::
	printstring STRINGID_PKMNHEALEDCONFUSION
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_PrintPayDayMoneyString::
	printstring STRINGID_PLAYERPICKEDUPMONEY
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_WrapTurnDmg::
	playanimation BS_ATTACKER, B_ANIM_TURN_TRAP, sB_ANIM_ARG1
	printstring STRINGID_PKMNHURTBY
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_DoTurnDmg

BattleScript_WrapEnds::
	printstring STRINGID_PKMNFREEDFROM
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_MoveUsedIsInLove::
	printstring STRINGID_PKMNINLOVE
	waitmessage B_WAIT_TIME_LONG
	status2animation BS_ATTACKER, STATUS2_INFATUATION
	return

BattleScript_MoveUsedIsInLoveCantAttack::
	printstring STRINGID_PKMNIMMOBILIZEDBYLOVE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_NightmareTurnDmg::
	printstring STRINGID_PKMNLOCKEDINNIGHTMARE
	waitmessage B_WAIT_TIME_LONG
	status2animation BS_ATTACKER, STATUS2_NIGHTMARE
	goto BattleScript_DoTurnDmg

BattleScript_CurseTurnDmg::
	printstring STRINGID_PKMNAFFLICTEDBYCURSE
	waitmessage B_WAIT_TIME_LONG
	status2animation BS_ATTACKER, STATUS2_CURSED
	goto BattleScript_DoTurnDmg

BattleScript_TargetPRLZHeal::
	printstring STRINGID_PKMNHEALEDPARALYSIS
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_TARGET
	return

BattleScript_MoveEffectSleep::
	statusanimation BS_EFFECT_BATTLER
	printfromtable gFellAsleepStringIds
	waitmessage B_WAIT_TIME_LONG
BattleScript_UpdateEffectStatusIconRet::
	updatestatusicon BS_EFFECT_BATTLER
	waitstate
	return

BattleScript_YawnMakesAsleep::
	statusanimation BS_EFFECT_BATTLER
	printstring STRINGID_PKMNFELLASLEEP
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_EFFECT_BATTLER
	waitstate
	makevisible BS_EFFECT_BATTLER
	end2

BattleScript_MoveEffectPoison::
	statusanimation BS_EFFECT_BATTLER
	printfromtable gGotPoisonedStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_UpdateEffectStatusIconRet

BattleScript_MoveEffectBurn::
	statusanimation BS_EFFECT_BATTLER
	printfromtable gGotBurnedStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_UpdateEffectStatusIconRet

BattleScript_MoveEffectFreeze::
	statusanimation BS_EFFECT_BATTLER
	printfromtable gGotFrozenStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_UpdateEffectStatusIconRet

BattleScript_MoveEffectParalysis::
	statusanimation BS_EFFECT_BATTLER
	printfromtable gGotParalyzedStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_UpdateEffectStatusIconRet

BattleScript_MoveEffectUproar::
	printstring STRINGID_PKMNCAUSEDUPROAR
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_MoveEffectToxic::
	statusanimation BS_EFFECT_BATTLER
	printstring STRINGID_PKMNBADLYPOISONED
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_UpdateEffectStatusIconRet

BattleScript_MoveEffectPayDay::
	printstring STRINGID_COINSSCATTERED
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_MoveEffectWrap::
	printfromtable gWrappedStringIds
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_MoveEffectConfusion::
	chosenstatus2animation BS_EFFECT_BATTLER, STATUS2_CONFUSION
	printstring STRINGID_PKMNWASCONFUSED
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_MoveEffectRecoil::
	jumpifmove MOVE_STRUGGLE, BattleScript_DoRecoil
	jumpifability BS_ATTACKER, ABILITY_ROCK_HEAD, BattleScript_RecoilEnd
BattleScript_DoRecoil::
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	printstring STRINGID_PKMNHITWITHRECOIL
	waitmessage B_WAIT_TIME_LONG
	tryfaintmon BS_ATTACKER
BattleScript_RecoilEnd::
	return

BattleScript_ItemSteal::
	playanimation BS_TARGET, B_ANIM_ITEM_STEAL
	printstring STRINGID_PKMNSTOLEITEM
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_DrizzleActivates::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNMADEITRAIN
	waitstate
	playanimation BS_BATTLER_0, B_ANIM_RAIN_CONTINUES
	call BattleScript_WeatherFormChanges
	end3

BattleScript_SpeedBoostActivates::
	playanimation BS_ATTACKER, B_ANIM_STATS_CHANGE, sB_ANIM_ARG1
	printstring STRINGID_PKMNRAISEDSPEED
	waitmessage B_WAIT_TIME_LONG
	end3

BattleScript_TraceActivates::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNTRACED
	waitmessage B_WAIT_TIME_LONG
	end3

BattleScript_RainDishActivates::
	printstring STRINGID_PKMNSXRESTOREDHPALITTLE2
	waitmessage B_WAIT_TIME_LONG
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	end3

BattleScript_SandstreamActivates::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNSXWHIPPEDUPSANDSTORM
	waitstate
	playanimation BS_BATTLER_0, B_ANIM_SANDSTORM_CONTINUES
	call BattleScript_WeatherFormChanges
	end3

BattleScript_ShedSkinActivates::
	printstring STRINGID_PKMNSXCUREDYPROBLEM
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_ATTACKER
	end3

BattleScript_WeatherFormChanges::
	setbyte sBATTLER, 0
BattleScript_WeatherFormChangesLoop::
	trycastformdatachange
	addbyte sBATTLER, 1
	jumpifbytenotequal sBATTLER, gBattlersCount, BattleScript_WeatherFormChangesLoop
	return

BattleScript_CastformChange::
	call BattleScript_DoCastformChange
	end3

BattleScript_DoCastformChange::
	docastformchangeanimation
	waitstate
	printstring STRINGID_PKMNTRANSFORMED
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_IntimidateActivatesEnd3::
	call BattleScript_PauseIntimidateActivates
	end3

BattleScript_PauseIntimidateActivates:
	pause B_WAIT_TIME_SHORT
BattleScript_IntimidateActivates::
	setbyte gBattlerTarget, 0
	setstatchanger STAT_ATK, 1, TRUE
BattleScript_IntimidateActivatesLoop:
	trygetintimidatetarget BattleScript_IntimidateActivatesReturn
	jumpifstatus2 BS_TARGET, STATUS2_SUBSTITUTE, BattleScript_IntimidateActivatesLoopIncrement
	jumpifability BS_TARGET, ABILITY_CLEAR_BODY, BattleScript_IntimidatePrevented
	jumpifability BS_TARGET, ABILITY_HYPER_CUTTER, BattleScript_IntimidatePrevented
	jumpifability BS_TARGET, ABILITY_WHITE_SMOKE, BattleScript_IntimidatePrevented
	statbuffchange STAT_CHANGE_NOT_PROTECT_AFFECTED | STAT_CHANGE_ALLOW_PTR, BattleScript_IntimidateActivatesLoopIncrement
	jumpifbyte CMP_GREATER_THAN, cMULTISTRING_CHOOSER, 1, BattleScript_IntimidateActivatesLoopIncrement
	setgraphicalstatchangevalues
	playanimation BS_TARGET, B_ANIM_STATS_CHANGE, sB_ANIM_ARG1
	printstring STRINGID_PKMNCUTSATTACKWITH
	waitmessage B_WAIT_TIME_LONG
BattleScript_IntimidateActivatesLoopIncrement:
	addbyte gBattlerTarget, 1
	goto BattleScript_IntimidateActivatesLoop
BattleScript_IntimidateActivatesReturn:
	return
BattleScript_IntimidatePrevented:
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PREVENTEDFROMWORKING
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_IntimidateActivatesLoopIncrement

BattleScript_DroughtActivates::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNSXINTENSIFIEDSUN
	waitstate
	playanimation BS_BATTLER_0, B_ANIM_SUN_CONTINUES
	call BattleScript_WeatherFormChanges
	end3

BattleScript_TookAttack::
	attackstring
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNSXTOOKATTACK
	waitmessage B_WAIT_TIME_LONG
	orword gHitMarker, HITMARKER_ATTACKSTRING_PRINTED
	return

BattleScript_SturdyPreventsOHKO::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNPROTECTEDBY
	pause B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_DampStopsExplosion::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNPREVENTSUSAGE
	pause B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_MoveHPDrain_PPLoss::
	ppreduce
BattleScript_MoveHPDrain::
	attackstring
	pause B_WAIT_TIME_SHORT
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	healthbarupdate BS_TARGET
	datahpupdate BS_TARGET
	printstring STRINGID_PKMNRESTOREDHPUSING
	waitmessage B_WAIT_TIME_LONG
	orbyte gMoveResultFlags, MOVE_RESULT_DOESNT_AFFECT_FOE
	goto BattleScript_MoveEnd

BattleScript_MonMadeMoveUseless_PPLoss::
	ppreduce
BattleScript_MonMadeMoveUseless::
	attackstring
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNSXMADEYUSELESS
	waitmessage B_WAIT_TIME_LONG
	orbyte gMoveResultFlags, MOVE_RESULT_DOESNT_AFFECT_FOE
	goto BattleScript_MoveEnd

BattleScript_FlashFireBoost_PPLoss::
	ppreduce
BattleScript_FlashFireBoost::
	attackstring
	pause B_WAIT_TIME_SHORT
	printfromtable gFlashFireStringIds
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_AbilityPreventsPhasingOut::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNANCHORSITSELFWITH
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_AbilityNoStatLoss::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNPREVENTSSTATLOSSWITH
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_BRNPrevention::
	pause B_WAIT_TIME_SHORT
	printfromtable gBRNPreventionStringIds
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_PRLZPrevention::
	pause B_WAIT_TIME_SHORT
	printfromtable gPRLZPreventionStringIds
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_PSNPrevention::
	pause B_WAIT_TIME_SHORT
	printfromtable gPSNPreventionStringIds
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_ObliviousPreventsAttraction::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNPREVENTSROMANCEWITH
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_FlinchPrevention::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNSXPREVENTSFLINCHING
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_OwnTempoPrevents::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNPREVENTSCONFUSIONWITH
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_SoundproofProtected::
	attackstring
	ppreduce
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNSXBLOCKSY
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_AbilityNoSpecificStatLoss::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNSXPREVENTSYLOSS
	waitmessage B_WAIT_TIME_LONG
	setbyte cMULTISTRING_CHOOSER, B_MSG_STAT_FELL_EMPTY
	return

BattleScript_StickyHoldActivates::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNSXMADEYINEFFECTIVE
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_MoveEnd

BattleScript_ColorChangeActivates::
	printstring STRINGID_PKMNCHANGEDTYPEWITH
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_RoughSkinActivates::
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	printstring STRINGID_PKMNHURTSWITH
	waitmessage B_WAIT_TIME_LONG
	tryfaintmon BS_ATTACKER
	return

BattleScript_CuteCharmActivates::
	status2animation BS_ATTACKER, STATUS2_INFATUATION
	printstring STRINGID_PKMNSXINFATUATEDY
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_ApplySecondaryEffect::
	waitstate
	seteffectsecondary
	return

BattleScript_SynchronizeActivates::
	waitstate
	seteffectprimary
	return

BattleScript_NoItemSteal::
	pause B_WAIT_TIME_SHORT
	printstring STRINGID_PKMNSXMADEYINEFFECTIVE
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_AbilityCuredStatus::
	printstring STRINGID_PKMNSXCUREDITSYPROBLEM
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_SCRIPTING
	return

BattleScript_IgnoresWhileAsleep::
	printstring STRINGID_PKMNIGNORESASLEEP
	waitmessage B_WAIT_TIME_LONG
	moveendto MOVEEND_NEXT_TARGET
	end

BattleScript_IgnoresAndUsesRandomMove::
	printstring STRINGID_PKMNIGNOREDORDERS
	waitmessage B_WAIT_TIME_LONG
	jumptocalledmove FALSE

BattleScript_MoveUsedLoafingAround::
	@ Skip ahead if not the Battle Palace message
	jumpifbyte CMP_NOT_EQUAL, cMULTISTRING_CHOOSER, B_MSG_INCAPABLE_OF_POWER, BattleScript_MoveUsedLoafingAroundMsg
	setbyte gBattleCommunication, 0
	palacetryescapestatus BS_ATTACKER
	setbyte cMULTISTRING_CHOOSER, B_MSG_INCAPABLE_OF_POWER
BattleScript_MoveUsedLoafingAroundMsg::
	printfromtable gInobedientStringIds
	waitmessage B_WAIT_TIME_LONG
	moveendto MOVEEND_NEXT_TARGET
	end

BattleScript_IgnoresAndFallsAsleep::
	printstring STRINGID_PKMNBEGANTONAP
	waitmessage B_WAIT_TIME_LONG
	setmoveeffect MOVE_EFFECT_SLEEP | MOVE_EFFECT_AFFECTS_USER
	seteffectprimary
	moveendto MOVEEND_NEXT_TARGET
	end

BattleScript_IgnoresAndHitsItself::
	printstring STRINGID_PKMNWONTOBEY
	waitmessage B_WAIT_TIME_LONG
	goto BattleScript_DoSelfConfusionDmg

BattleScript_SubstituteFade::
	playanimation BS_TARGET, B_ANIM_SUBSTITUTE_FADE
	printstring STRINGID_PKMNSUBSTITUTEFADED
	return

BattleScript_BerryCurePrlzEnd2::
	call BattleScript_BerryCureParRet
	end2

BattleScript_BerryCureParRet::
	playanimation BS_SCRIPTING, B_ANIM_HELD_ITEM_EFFECT
	printstring STRINGID_PKMNSITEMCUREDPARALYSIS
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_SCRIPTING
	removeitem BS_SCRIPTING
	return

BattleScript_BerryCurePsnEnd2::
	call BattleScript_BerryCurePsnRet
	end2

BattleScript_BerryCurePsnRet::
	playanimation BS_SCRIPTING, B_ANIM_HELD_ITEM_EFFECT
	printstring STRINGID_PKMNSITEMCUREDPOISON
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_SCRIPTING
	removeitem BS_SCRIPTING
	return

BattleScript_BerryCureBrnEnd2::
	call BattleScript_BerryCureBrnRet
	end2

BattleScript_BerryCureBrnRet::
	playanimation BS_SCRIPTING, B_ANIM_HELD_ITEM_EFFECT
	printstring STRINGID_PKMNSITEMHEALEDBURN
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_SCRIPTING
	removeitem BS_SCRIPTING
	return

BattleScript_BerryCureFrzEnd2::
	call BattleScript_BerryCureFrzRet
	end2

BattleScript_BerryCureFrzRet::
	playanimation BS_SCRIPTING, B_ANIM_HELD_ITEM_EFFECT
	printstring STRINGID_PKMNSITEMDEFROSTEDIT
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_SCRIPTING
	removeitem BS_SCRIPTING
	return

BattleScript_BerryCureSlpEnd2::
	call BattleScript_BerryCureSlpRet
	end2

BattleScript_BerryCureSlpRet::
	playanimation BS_SCRIPTING, B_ANIM_HELD_ITEM_EFFECT
	printstring STRINGID_PKMNSITEMWOKEIT
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_SCRIPTING
	removeitem BS_SCRIPTING
	return

BattleScript_BerryCureConfusionEnd2::
	call BattleScript_BerryCureConfusionRet
	end2

BattleScript_BerryCureConfusionRet::
	playanimation BS_SCRIPTING, B_ANIM_HELD_ITEM_EFFECT
	printstring STRINGID_PKMNSITEMSNAPPEDOUT
	waitmessage B_WAIT_TIME_LONG
	removeitem BS_SCRIPTING
	return

BattleScript_BerryCureChosenStatusEnd2::
	call BattleScript_BerryCureChosenStatusRet
	end2

BattleScript_BerryCureChosenStatusRet::
	playanimation BS_SCRIPTING, B_ANIM_HELD_ITEM_EFFECT
	printfromtable gBerryEffectStringIds
	waitmessage B_WAIT_TIME_LONG
	updatestatusicon BS_SCRIPTING
	removeitem BS_SCRIPTING
	return

BattleScript_WhiteHerbEnd2::
	call BattleScript_WhiteHerbRet
	end2

BattleScript_WhiteHerbRet::
	playanimation BS_SCRIPTING, B_ANIM_HELD_ITEM_EFFECT
	printstring STRINGID_PKMNSITEMRESTOREDSTATUS
	waitmessage B_WAIT_TIME_LONG
	removeitem BS_SCRIPTING
	return

BattleScript_ItemHealHP_RemoveItem::
	playanimation BS_ATTACKER, B_ANIM_HELD_ITEM_EFFECT
	printstring STRINGID_PKMNSITEMRESTOREDHEALTH
	waitmessage B_WAIT_TIME_LONG
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	removeitem BS_ATTACKER
	end2

BattleScript_BerryPPHealEnd2::
	playanimation BS_ATTACKER, B_ANIM_HELD_ITEM_EFFECT
	printstring STRINGID_PKMNSITEMRESTOREDPP
	waitmessage B_WAIT_TIME_LONG
	removeitem BS_ATTACKER
	end2

BattleScript_ItemHealHP_End2::
	call BattleScript_ItemHealHP_Ret
	end2

BattleScript_ItemHealHP_Ret::
	playanimation BS_ATTACKER, B_ANIM_HELD_ITEM_EFFECT
	printstring STRINGID_PKMNSITEMRESTOREDHPALITTLE
	waitmessage B_WAIT_TIME_LONG
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	return

BattleScript_SelectingNotAllowedMoveChoiceItem::
	printselectionstring STRINGID_ITEMALLOWSONLYYMOVE
	endselectionscript

BattleScript_FocusBandActivates::
	playanimation BS_TARGET, B_ANIM_FOCUS_BAND
	printstring STRINGID_PKMNHUNGONWITHX
	waitmessage B_WAIT_TIME_LONG
	return

BattleScript_BerryConfuseHealEnd2::
	playanimation BS_ATTACKER, B_ANIM_HELD_ITEM_EFFECT
	printstring STRINGID_PKMNSITEMRESTOREDHEALTH
	waitmessage B_WAIT_TIME_LONG
	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE
	healthbarupdate BS_ATTACKER
	datahpupdate BS_ATTACKER
	printstring STRINGID_FORXCOMMAYZ
	waitmessage B_WAIT_TIME_LONG
	setmoveeffect MOVE_EFFECT_CONFUSION | MOVE_EFFECT_AFFECTS_USER
	seteffectprimary
	removeitem BS_ATTACKER
	end2

BattleScript_BerryStatRaiseEnd2::
	playanimation BS_ATTACKER, B_ANIM_HELD_ITEM_EFFECT
	statbuffchange MOVE_EFFECT_AFFECTS_USER | STAT_CHANGE_ALLOW_PTR, BattleScript_BerryStatRaiseDoStatUp
BattleScript_BerryStatRaiseDoStatUp::
	setbyte cMULTISTRING_CHOOSER, B_MSG_STAT_ROSE_ITEM
	call BattleScript_StatUp
	removeitem BS_ATTACKER
	end2

BattleScript_BerryFocusEnergyEnd2::
	playanimation BS_ATTACKER, B_ANIM_HELD_ITEM_EFFECT
	printstring STRINGID_PKMNUSEDXTOGETPUMPED
	waitmessage B_WAIT_TIME_LONG
	removeitem BS_ATTACKER
	end2

BattleScript_ActionSelectionItemsCantBeUsed::
	printselectionstring STRINGID_ITEMSCANTBEUSEDNOW
	endselectionscript

BattleScript_FlushMessageBox::
	printstring STRINGID_EMPTYSTRING3
	return

BattleScript_PalacePrintFlavorText::
	setbyte gBattleCommunication + 1, 0
BattleScript_PalaceTryBattlerFlavorText::
	palaceflavortext BS_ATTACKER @ BS_ATTACKER here overwritten by gBattleCommunication + 1
	jumpifbyte CMP_NOT_EQUAL, gBattleCommunication, TRUE, BattleScript_PalaceEndFlavorText
	printfromtable gBattlePalaceFlavorTextTable
	waitmessage B_WAIT_TIME_LONG
BattleScript_PalaceEndFlavorText::
	addbyte gBattleCommunication + 1, 1
	jumpifbytenotequal gBattleCommunication + 1, gBattlersCount, BattleScript_PalaceTryBattlerFlavorText
	setbyte gBattleCommunication, 0
	setbyte gBattleCommunication + 1, 0
	end2

BattleScript_ArenaTurnBeginning::
	waitcry BS_ATTACKER
	volumedown
	playse SE_ARENA_TIMEUP1
	pause 8
	playse SE_ARENA_TIMEUP1
	arenadrawreftextbox
	arenajudgmentstring B_MSG_REF_COMMENCE_BATTLE
	arenawaitmessage B_MSG_REF_COMMENCE_BATTLE
	pause B_WAIT_TIME_LONG
	arenaerasereftextbox
	volumeup
	end2

@ Unused
BattleScript_ArenaNothingDecided::
	playse SE_DING_DONG
	arenadrawreftextbox
	arenajudgmentstring B_MSG_REF_NOTHING_IS_DECIDED
	arenawaitmessage B_MSG_REF_NOTHING_IS_DECIDED
	pause B_WAIT_TIME_LONG
	arenaerasereftextbox
	end2

BattleScript_ArenaDoJudgment::
	makevisible BS_PLAYER1
	waitstate
	makevisible BS_OPPONENT1
	waitstate
	volumedown
	playse SE_ARENA_TIMEUP1
	pause 8
	playse SE_ARENA_TIMEUP1
	pause B_WAIT_TIME_LONG
	arenadrawreftextbox
	arenajudgmentstring B_MSG_REF_THATS_IT
	arenawaitmessage B_MSG_REF_THATS_IT
	pause B_WAIT_TIME_LONG
	setbyte gBattleCommunication, 0  @ Reset state for arenajudgmentwindow
	arenajudgmentwindow
	pause B_WAIT_TIME_LONG
	arenajudgmentwindow
	arenajudgmentstring B_MSG_REF_JUDGE_MIND
	arenawaitmessage B_MSG_REF_JUDGE_MIND
	arenajudgmentwindow
	arenajudgmentstring B_MSG_REF_JUDGE_SKILL
	arenawaitmessage B_MSG_REF_JUDGE_SKILL
	arenajudgmentwindow
	arenajudgmentstring B_MSG_REF_JUDGE_BODY
	arenawaitmessage B_MSG_REF_JUDGE_BODY
	arenajudgmentwindow
	jumpifbyte CMP_EQUAL, gBattleCommunication + 1, ARENA_RESULT_PLAYER_LOST, BattleScript_ArenaJudgmentPlayerLoses
	jumpifbyte CMP_EQUAL, gBattleCommunication + 1, ARENA_RESULT_TIE, BattleScript_ArenaJudgmentDraw
@ ARENA_RESULT_PLAYER_WON
	arenajudgmentstring B_MSG_REF_PLAYER_WON
	arenawaitmessage B_MSG_REF_PLAYER_WON
	arenajudgmentwindow
	arenaerasereftextbox
	printstring STRINGID_DEFEATEDOPPONENTBYREFEREE
	waitmessage B_WAIT_TIME_LONG
	playfaintcry BS_OPPONENT1
	waitcry BS_ATTACKER
	dofaintanimation BS_OPPONENT1
	cleareffectsonfaint BS_OPPONENT1
	arenaopponentmonlost
	end2

BattleScript_ArenaJudgmentPlayerLoses:
	arenajudgmentstring B_MSG_REF_OPPONENT_WON
	arenawaitmessage B_MSG_REF_OPPONENT_WON
	arenajudgmentwindow
	arenaerasereftextbox
	printstring STRINGID_LOSTTOOPPONENTBYREFEREE
	waitmessage B_WAIT_TIME_LONG
	playfaintcry BS_PLAYER1
	waitcry BS_ATTACKER
	dofaintanimation BS_PLAYER1
	cleareffectsonfaint BS_PLAYER1
	arenaplayermonlost
	end2

BattleScript_ArenaJudgmentDraw:
	arenajudgmentstring B_MSG_REF_DRAW
	arenawaitmessage B_MSG_REF_DRAW
	arenajudgmentwindow
	arenaerasereftextbox
	printstring STRINGID_TIEDOPPONENTBYREFEREE
	waitmessage B_WAIT_TIME_LONG
	playfaintcry BS_PLAYER1
	waitcry BS_ATTACKER
	dofaintanimation BS_PLAYER1
	cleareffectsonfaint BS_PLAYER1
	playfaintcry BS_OPPONENT1
	waitcry BS_ATTACKER
	dofaintanimation BS_OPPONENT1
	cleareffectsonfaint BS_OPPONENT1
	arenabothmonlost
	end2

BattleScript_AskIfWantsToForfeitMatch::
	printselectionstring STRINGID_QUESTIONFORFEITMATCH
	forfeityesnobox BS_ATTACKER
	endselectionscript

BattleScript_PrintPlayerForfeited::
	printstring STRINGID_FORFEITEDMATCH
	waitmessage B_WAIT_TIME_LONG
	end2

BattleScript_PrintPlayerForfeitedLinkBattle::
	printstring STRINGID_FORFEITEDMATCH
	waitmessage B_WAIT_TIME_LONG
	endlinkbattle
	waitmessage B_WAIT_TIME_LONG
	end2
