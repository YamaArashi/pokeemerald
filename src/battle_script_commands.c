#include "global.h"
#include "battle.h"
#include "battle_message.h"
#include "battle_anim.h"
#include "battle_ai_script_commands.h"
#include "battle_scripts.h"
#include "item.h"
#include "util.h"
#include "pokemon.h"
#include "random.h"
#include "battle_controllers.h"
#include "battle_interface.h"
#include "text.h"
#include "sound.h"
#include "pokedex.h"
#include "recorded_battle.h"
#include "window.h"
#include "reshow_battle_screen.h"
#include "main.h"
#include "palette.h"
#include "money.h"
#include "bg.h"
#include "string_util.h"
#include "pokemon_icon.h"
#include "m4a.h"
#include "mail.h"
#include "event_data.h"
#include "pokemon_storage_system.h"
#include "task.h"
#include "naming_screen.h"
#include "battle_setup.h"
#include "overworld.h"
#include "party_menu.h"
#include "battle_arena.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "field_specials.h"
#include "pokemon_summary_screen.h"
#include "pokenav.h"
#include "menu_specialized.h"
#include "data.h"
#include "constants/abilities.h"
#include "constants/battle_anim.h"
#include "constants/battle_move_effects.h"
#include "constants/battle_string_ids.h"
#include "constants/hold_effects.h"
#include "constants/items.h"
#include "constants/map_types.h"
#include "constants/moves.h"
#include "constants/party_menu.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/trainers.h"

extern const u8 *const gBattleScriptsForMoveEffects[];

#define DEFENDER_IS_PROTECTED ((gProtectStructs[gBattlerTarget].protected) && (gBattleMoves[gCurrentMove].flags & FLAG_PROTECT_AFFECTED))

#define LEVEL_UP_BANNER_START 416
#define LEVEL_UP_BANNER_END   512

#define TAG_LVLUP_BANNER_MON_ICON 55130

static bool8 IsTwoTurnsMove(u16 move);
static void TrySetDestinyBondToHappen(void);
static u8 AttacksThisTurn(u8 battlerId, u16 move); // Note: returns 1 if it's a charging turn, otherwise 2.
static void CheckWonderGuardAndLevitate(void);
static u8 ChangeStatBuffs(s8 statValue, u8 statId, u8, const u8 *BS_ptr);
static bool32 IsMonGettingExpSentOut(void);
static void InitLevelUpBanner(void);
static bool8 SlideInLevelUpBanner(void);
static bool8 SlideOutLevelUpBanner(void);
static void DrawLevelUpWindow1(void);
static void DrawLevelUpWindow2(void);
static void PutMonIconOnLvlUpBanner(void);
static void DrawLevelUpBannerText(void);
static void SpriteCB_MonIconOnLvlUpBanner(struct Sprite *sprite);

static void Cmd_attackcanceler(void);
static void Cmd_accuracycheck(void);
static void Cmd_attackstring(void);
static void Cmd_ppreduce(void);
static void Cmd_critcalc(void);
static void Cmd_damagecalc(void);
static void Cmd_typecalc(void);
static void Cmd_adjustnormaldamage(void);
static void Cmd_adjustnormaldamage2(void);
static void Cmd_attackanimation(void);
static void Cmd_waitanimation(void);
static void Cmd_healthbarupdate(void);
static void Cmd_datahpupdate(void);
static void Cmd_critmessage(void);
static void Cmd_effectivenesssound(void);
static void Cmd_resultmessage(void);
static void Cmd_printstring(void);
static void Cmd_printselectionstring(void);
static void Cmd_waitmessage(void);
static void Cmd_printfromtable(void);
static void Cmd_printselectionstringfromtable(void);
static void Cmd_seteffectwithchance(void);
static void Cmd_seteffectprimary(void);
static void Cmd_seteffectsecondary(void);
static void Cmd_clearstatusfromeffect(void);
static void Cmd_tryfaintmon(void);
static void Cmd_dofaintanimation(void);
static void Cmd_cleareffectsonfaint(void);
static void Cmd_jumpifstatus(void);
static void Cmd_jumpifstatus2(void);
static void Cmd_jumpifability(void);
static void Cmd_jumpifsideaffecting(void);
static void Cmd_jumpifstat(void);
static void Cmd_jumpifstatus3condition(void);
static void Cmd_jumpiftype(void);
static void Cmd_getexp(void);
static void Cmd_checkteamslost(void);
static void Cmd_movevaluescleanup(void);
static void Cmd_setmultihit(void);
static void Cmd_decrementmultihit(void);
static void Cmd_goto(void);
static void Cmd_jumpifbyte(void);
static void Cmd_jumpifhalfword(void);
static void Cmd_jumpifword(void);
static void Cmd_jumpifarrayequal(void);
static void Cmd_jumpifarraynotequal(void);
static void Cmd_setbyte(void);
static void Cmd_addbyte(void);
static void Cmd_subbyte(void);
static void Cmd_copyarray(void);
static void Cmd_copyarraywithindex(void);
static void Cmd_orbyte(void);
static void Cmd_orhalfword(void);
static void Cmd_orword(void);
static void Cmd_bicbyte(void);
static void Cmd_bichalfword(void);
static void Cmd_bicword(void);
static void Cmd_pause(void);
static void Cmd_waitstate(void);
static void Cmd_healthbar_update(void);
static void Cmd_return(void);
static void Cmd_end(void);
static void Cmd_end2(void);
static void Cmd_end3(void);
static void Cmd_jumpifaffectedbyprotect(void);
static void Cmd_call(void);
static void Cmd_jumpiftype2(void);
static void Cmd_jumpifabilitypresent(void);
static void Cmd_endselectionscript(void);
static void Cmd_playanimation(void);
static void Cmd_playanimation_var(void);
static void Cmd_setgraphicalstatchangevalues(void);
static void Cmd_playstatchangeanimation(void);
static void Cmd_moveend(void);
static void Cmd_typecalc2(void);
static void Cmd_returnatktoball(void);
static void Cmd_getswitchedmondata(void);
static void Cmd_switchindataupdate(void);
static void Cmd_switchinanim(void);
static void Cmd_jumpifcantswitch(void);
static void Cmd_openpartyscreen(void);
static void Cmd_switchhandleorder(void);
static void Cmd_switchineffects(void);
static void Cmd_trainerslidein(void);
static void Cmd_playse(void);
static void Cmd_fanfare(void);
static void Cmd_playfaintcry(void);
static void Cmd_endlinkbattle(void);
static void Cmd_returntoball(void);
static void Cmd_handlelearnnewmove(void);
static void Cmd_yesnoboxlearnmove(void);
static void Cmd_yesnoboxstoplearningmove(void);
static void Cmd_hitanimation(void);
static void Cmd_getmoneyreward(void);
static void Cmd_updatebattlermoves(void);
static void Cmd_swapattackerwithtarget(void);
static void Cmd_incrementgamestat(void);
static void Cmd_drawpartystatussummary(void);
static void Cmd_hidepartystatussummary(void);
static void Cmd_jumptocalledmove(void);
static void Cmd_statusanimation(void);
static void Cmd_status2animation(void);
static void Cmd_chosenstatusanimation(void);
static void Cmd_yesnobox(void);
static void Cmd_cancelallactions(void);
static void Cmd_adjustsetdamage(void);
static void Cmd_removeitem(void);
static void Cmd_atknameinbuff1(void);
static void Cmd_drawlvlupbox(void);
static void Cmd_resetsentmonsvalue(void);
static void Cmd_setatktoplayer0(void);
static void Cmd_makevisible(void);
static void Cmd_recordlastability(void);
static void Cmd_buffermovetolearn(void);
static void Cmd_jumpifplayerran(void);
static void Cmd_hpthresholds(void);
static void Cmd_hpthresholds2(void);
static void Cmd_useitemonopponent(void);
static void Cmd_various(void);
static void Cmd_setprotectlike(void);
static void Cmd_tryexplosion(void);
static void Cmd_setatkhptozero(void);
static void Cmd_jumpifnexttargetvalid(void);
static void Cmd_tryhealhalfhealth(void);
static void Cmd_trymirrormove(void);
static void Cmd_setrain(void);
static void Cmd_setreflect(void);
static void Cmd_setseeded(void);
static void Cmd_manipulatedamage(void);
static void Cmd_trysetrest(void);
static void Cmd_jumpifnotfirstturn(void);
static void Cmd_nop(void);
static void Cmd_jumpifcantmakeasleep(void);
static void Cmd_stockpile(void);
static void Cmd_stockpiletobasedamage(void);
static void Cmd_stockpiletohpheal(void);
static void Cmd_negativedamage(void);
static void Cmd_statbuffchange(void);
static void Cmd_normalisebuffs(void);
static void Cmd_setbide(void);
static void Cmd_confuseifrepeatingattackends(void);
static void Cmd_setmultihitcounter(void);
static void Cmd_initmultihitstring(void);
static void Cmd_forcerandomswitch(void);
static void Cmd_tryconversiontypechange(void);
static void Cmd_givepaydaymoney(void);
static void Cmd_setlightscreen(void);
static void Cmd_tryKO(void);
static void Cmd_damagetohalftargethp(void);
static void Cmd_setsandstorm(void);
static void Cmd_weatherdamage(void);
static void Cmd_tryinfatuating(void);
static void Cmd_updatestatusicon(void);
static void Cmd_setmist(void);
static void Cmd_setfocusenergy(void);
static void Cmd_transformdataexecution(void);
static void Cmd_setsubstitute(void);
static void Cmd_mimicattackcopy(void);
static void Cmd_metronome(void);
static void Cmd_dmgtolevel(void);
static void Cmd_psywavedamageeffect(void);
static void Cmd_counterdamagecalculator(void);
static void Cmd_mirrorcoatdamagecalculator(void);
static void Cmd_disablelastusedattack(void);
static void Cmd_trysetencore(void);
static void Cmd_painsplitdmgcalc(void);
static void Cmd_settypetorandomresistance(void);
static void Cmd_setalwayshitflag(void);
static void Cmd_copymovepermanently(void);
static void Cmd_trychoosesleeptalkmove(void);
static void Cmd_setdestinybond(void);
static void Cmd_trysetdestinybondtohappen(void);
static void Cmd_remaininghptopower(void);
static void Cmd_tryspiteppreduce(void);
static void Cmd_healpartystatus(void);
static void Cmd_cursetarget(void);
static void Cmd_trysetspikes(void);
static void Cmd_setforesight(void);
static void Cmd_trysetperishsong(void);
static void Cmd_rolloutdamagecalculation(void);
static void Cmd_jumpifconfusedandstatmaxed(void);
static void Cmd_furycuttercalc(void);
static void Cmd_friendshiptodamagecalculation(void);
static void Cmd_presentdamagecalculation(void);
static void Cmd_setsafeguard(void);
static void Cmd_magnitudedamagecalculation(void);
static void Cmd_jumpifnopursuitswitchdmg(void);
static void Cmd_setsunny(void);
static void Cmd_maxattackhalvehp(void);
static void Cmd_copyfoestats(void);
static void Cmd_rapidspinfree(void);
static void Cmd_setdefensecurlbit(void);
static void Cmd_recoverbasedonsunlight(void);
static void Cmd_hiddenpowercalc(void);
static void Cmd_selectfirstvalidtarget(void);
static void Cmd_trysetfutureattack(void);
static void Cmd_trydobeatup(void);
static void Cmd_setsemiinvulnerablebit(void);
static void Cmd_clearsemiinvulnerablebit(void);
static void Cmd_setminimize(void);
static void Cmd_sethail(void);
static void Cmd_trymemento(void);
static void Cmd_setforcedtarget(void);
static void Cmd_setcharge(void);
static void Cmd_callenvironmentattack(void);
static void Cmd_cureifburnedparalysedorpoisoned(void);
static void Cmd_settorment(void);
static void Cmd_jumpifnodamage(void);
static void Cmd_settaunt(void);
static void Cmd_trysethelpinghand(void);
static void Cmd_tryswapitems(void);
static void Cmd_trycopyability(void);
static void Cmd_trywish(void);
static void Cmd_trysetroots(void);
static void Cmd_doubledamagedealtifdamaged(void);
static void Cmd_setyawn(void);
static void Cmd_setdamagetohealthdifference(void);
static void Cmd_scaledamagebyhealthratio(void);
static void Cmd_tryswapabilities(void);
static void Cmd_tryimprison(void);
static void Cmd_trysetgrudge(void);
static void Cmd_weightdamagecalculation(void);
static void Cmd_assistattackselect(void);
static void Cmd_trysetmagiccoat(void);
static void Cmd_trysetsnatch(void);
static void Cmd_trygetintimidatetarget(void);
static void Cmd_switchoutabilities(void);
static void Cmd_jumpifhasnohp(void);
static void Cmd_getsecretpowereffect(void);
static void Cmd_pickup(void);
static void Cmd_docastformchangeanimation(void);
static void Cmd_trycastformdatachange(void);
static void Cmd_settypebasedhalvers(void);
static void Cmd_setweatherballtype(void);
static void Cmd_tryrecycleitem(void);
static void Cmd_settypetoenvironment(void);
static void Cmd_pursuitdoubles(void);
static void Cmd_snatchsetbattlers(void);
static void Cmd_removelightscreenreflect(void);
static void Cmd_handleballthrow(void);
static void Cmd_givecaughtmon(void);
static void Cmd_trysetcaughtmondexflags(void);
static void Cmd_displaydexinfo(void);
static void Cmd_trygivecaughtmonnick(void);
static void Cmd_subattackerhpbydmg(void);
static void Cmd_removeattackerstatus1(void);
static void Cmd_finishaction(void);
static void Cmd_finishturn(void);
static void Cmd_trainerslideout(void);

void (* const gBattleScriptingCommandsTable[])(void) =
{
    Cmd_attackcanceler,                          //0x0
    Cmd_accuracycheck,                           //0x1
    Cmd_attackstring,                            //0x2
    Cmd_ppreduce,                                //0x3
    Cmd_critcalc,                                //0x4
    Cmd_damagecalc,                              //0x5
    Cmd_typecalc,                                //0x6
    Cmd_adjustnormaldamage,                      //0x7
    Cmd_adjustnormaldamage2,                     //0x8
    Cmd_attackanimation,                         //0x9
    Cmd_waitanimation,                           //0xA
    Cmd_healthbarupdate,                         //0xB
    Cmd_datahpupdate,                            //0xC
    Cmd_critmessage,                             //0xD
    Cmd_effectivenesssound,                      //0xE
    Cmd_resultmessage,                           //0xF
    Cmd_printstring,                             //0x10
    Cmd_printselectionstring,                    //0x11
    Cmd_waitmessage,                             //0x12
    Cmd_printfromtable,                          //0x13
    Cmd_printselectionstringfromtable,           //0x14
    Cmd_seteffectwithchance,                     //0x15
    Cmd_seteffectprimary,                        //0x16
    Cmd_seteffectsecondary,                      //0x17
    Cmd_clearstatusfromeffect,                   //0x18
    Cmd_tryfaintmon,                             //0x19
    Cmd_dofaintanimation,                        //0x1A
    Cmd_cleareffectsonfaint,                     //0x1B
    Cmd_jumpifstatus,                            //0x1C
    Cmd_jumpifstatus2,                           //0x1D
    Cmd_jumpifability,                           //0x1E
    Cmd_jumpifsideaffecting,                     //0x1F
    Cmd_jumpifstat,                              //0x20
    Cmd_jumpifstatus3condition,                  //0x21
    Cmd_jumpiftype,                              //0x22
    Cmd_getexp,                                  //0x23
    Cmd_checkteamslost,                          //0x24
    Cmd_movevaluescleanup,                       //0x25
    Cmd_setmultihit,                             //0x26
    Cmd_decrementmultihit,                       //0x27
    Cmd_goto,                                    //0x28
    Cmd_jumpifbyte,                              //0x29
    Cmd_jumpifhalfword,                          //0x2A
    Cmd_jumpifword,                              //0x2B
    Cmd_jumpifarrayequal,                        //0x2C
    Cmd_jumpifarraynotequal,                     //0x2D
    Cmd_setbyte,                                 //0x2E
    Cmd_addbyte,                                 //0x2F
    Cmd_subbyte,                                 //0x30
    Cmd_copyarray,                               //0x31
    Cmd_copyarraywithindex,                      //0x32
    Cmd_orbyte,                                  //0x33
    Cmd_orhalfword,                              //0x34
    Cmd_orword,                                  //0x35
    Cmd_bicbyte,                                 //0x36
    Cmd_bichalfword,                             //0x37
    Cmd_bicword,                                 //0x38
    Cmd_pause,                                   //0x39
    Cmd_waitstate,                               //0x3A
    Cmd_healthbar_update,                        //0x3B
    Cmd_return,                                  //0x3C
    Cmd_end,                                     //0x3D
    Cmd_end2,                                    //0x3E
    Cmd_end3,                                    //0x3F
    Cmd_jumpifaffectedbyprotect,                 //0x40
    Cmd_call,                                    //0x41
    Cmd_jumpiftype2,                             //0x42
    Cmd_jumpifabilitypresent,                    //0x43
    Cmd_endselectionscript,                      //0x44
    Cmd_playanimation,                           //0x45
    Cmd_playanimation_var,                       //0x46
    Cmd_setgraphicalstatchangevalues,            //0x47
    Cmd_playstatchangeanimation,                 //0x48
    Cmd_moveend,                                 //0x49
    Cmd_typecalc2,                               //0x4A
    Cmd_returnatktoball,                         //0x4B
    Cmd_getswitchedmondata,                      //0x4C
    Cmd_switchindataupdate,                      //0x4D
    Cmd_switchinanim,                            //0x4E
    Cmd_jumpifcantswitch,                        //0x4F
    Cmd_openpartyscreen,                         //0x50
    Cmd_switchhandleorder,                       //0x51
    Cmd_switchineffects,                         //0x52
    Cmd_trainerslidein,                          //0x53
    Cmd_playse,                                  //0x54
    Cmd_fanfare,                                 //0x55
    Cmd_playfaintcry,                            //0x56
    Cmd_endlinkbattle,                           //0x57
    Cmd_returntoball,                            //0x58
    Cmd_handlelearnnewmove,                      //0x59
    Cmd_yesnoboxlearnmove,                       //0x5A
    Cmd_yesnoboxstoplearningmove,                //0x5B
    Cmd_hitanimation,                            //0x5C
    Cmd_getmoneyreward,                          //0x5D
    Cmd_updatebattlermoves,                      //0x5E
    Cmd_swapattackerwithtarget,                  //0x5F
    Cmd_incrementgamestat,                       //0x60
    Cmd_drawpartystatussummary,                  //0x61
    Cmd_hidepartystatussummary,                  //0x62
    Cmd_jumptocalledmove,                        //0x63
    Cmd_statusanimation,                         //0x64
    Cmd_status2animation,                        //0x65
    Cmd_chosenstatusanimation,                   //0x66
    Cmd_yesnobox,                                //0x67
    Cmd_cancelallactions,                        //0x68
    Cmd_adjustsetdamage,                         //0x69
    Cmd_removeitem,                              //0x6A
    Cmd_atknameinbuff1,                          //0x6B
    Cmd_drawlvlupbox,                            //0x6C
    Cmd_resetsentmonsvalue,                      //0x6D
    Cmd_setatktoplayer0,                         //0x6E
    Cmd_makevisible,                             //0x6F
    Cmd_recordlastability,                       //0x70
    Cmd_buffermovetolearn,                       //0x71
    Cmd_jumpifplayerran,                         //0x72
    Cmd_hpthresholds,                            //0x73
    Cmd_hpthresholds2,                           //0x74
    Cmd_useitemonopponent,                       //0x75
    Cmd_various,                                 //0x76
    Cmd_setprotectlike,                          //0x77
    Cmd_tryexplosion,                            //0x78
    Cmd_setatkhptozero,                          //0x79
    Cmd_jumpifnexttargetvalid,                   //0x7A
    Cmd_tryhealhalfhealth,                       //0x7B
    Cmd_trymirrormove,                           //0x7C
    Cmd_setrain,                                 //0x7D
    Cmd_setreflect,                              //0x7E
    Cmd_setseeded,                               //0x7F
    Cmd_manipulatedamage,                        //0x80
    Cmd_trysetrest,                              //0x81
    Cmd_jumpifnotfirstturn,                      //0x82
    Cmd_nop,                                     //0x83
    Cmd_jumpifcantmakeasleep,                    //0x84
    Cmd_stockpile,                               //0x85
    Cmd_stockpiletobasedamage,                   //0x86
    Cmd_stockpiletohpheal,                       //0x87
    Cmd_negativedamage,                          //0x88
    Cmd_statbuffchange,                          //0x89
    Cmd_normalisebuffs,                          //0x8A
    Cmd_setbide,                                 //0x8B
    Cmd_confuseifrepeatingattackends,            //0x8C
    Cmd_setmultihitcounter,                      //0x8D
    Cmd_initmultihitstring,                      //0x8E
    Cmd_forcerandomswitch,                       //0x8F
    Cmd_tryconversiontypechange,                 //0x90
    Cmd_givepaydaymoney,                         //0x91
    Cmd_setlightscreen,                          //0x92
    Cmd_tryKO,                                   //0x93
    Cmd_damagetohalftargethp,                    //0x94
    Cmd_setsandstorm,                            //0x95
    Cmd_weatherdamage,                           //0x96
    Cmd_tryinfatuating,                          //0x97
    Cmd_updatestatusicon,                        //0x98
    Cmd_setmist,                                 //0x99
    Cmd_setfocusenergy,                          //0x9A
    Cmd_transformdataexecution,                  //0x9B
    Cmd_setsubstitute,                           //0x9C
    Cmd_mimicattackcopy,                         //0x9D
    Cmd_metronome,                               //0x9E
    Cmd_dmgtolevel,                              //0x9F
    Cmd_psywavedamageeffect,                     //0xA0
    Cmd_counterdamagecalculator,                 //0xA1
    Cmd_mirrorcoatdamagecalculator,              //0xA2
    Cmd_disablelastusedattack,                   //0xA3
    Cmd_trysetencore,                            //0xA4
    Cmd_painsplitdmgcalc,                        //0xA5
    Cmd_settypetorandomresistance,               //0xA6
    Cmd_setalwayshitflag,                        //0xA7
    Cmd_copymovepermanently,                     //0xA8
    Cmd_trychoosesleeptalkmove,                  //0xA9
    Cmd_setdestinybond,                          //0xAA
    Cmd_trysetdestinybondtohappen,               //0xAB
    Cmd_remaininghptopower,                      //0xAC
    Cmd_tryspiteppreduce,                        //0xAD
    Cmd_healpartystatus,                         //0xAE
    Cmd_cursetarget,                             //0xAF
    Cmd_trysetspikes,                            //0xB0
    Cmd_setforesight,                            //0xB1
    Cmd_trysetperishsong,                        //0xB2
    Cmd_rolloutdamagecalculation,                //0xB3
    Cmd_jumpifconfusedandstatmaxed,              //0xB4
    Cmd_furycuttercalc,                          //0xB5
    Cmd_friendshiptodamagecalculation,           //0xB6
    Cmd_presentdamagecalculation,                //0xB7
    Cmd_setsafeguard,                            //0xB8
    Cmd_magnitudedamagecalculation,              //0xB9
    Cmd_jumpifnopursuitswitchdmg,                //0xBA
    Cmd_setsunny,                                //0xBB
    Cmd_maxattackhalvehp,                        //0xBC
    Cmd_copyfoestats,                            //0xBD
    Cmd_rapidspinfree,                           //0xBE
    Cmd_setdefensecurlbit,                       //0xBF
    Cmd_recoverbasedonsunlight,                  //0xC0
    Cmd_hiddenpowercalc,                         //0xC1
    Cmd_selectfirstvalidtarget,                  //0xC2
    Cmd_trysetfutureattack,                      //0xC3
    Cmd_trydobeatup,                             //0xC4
    Cmd_setsemiinvulnerablebit,                  //0xC5
    Cmd_clearsemiinvulnerablebit,                //0xC6
    Cmd_setminimize,                             //0xC7
    Cmd_sethail,                                 //0xC8
    Cmd_trymemento,                              //0xC9
    Cmd_setforcedtarget,                         //0xCA
    Cmd_setcharge,                               //0xCB
    Cmd_callenvironmentattack,                   //0xCC
    Cmd_cureifburnedparalysedorpoisoned,         //0xCD
    Cmd_settorment,                              //0xCE
    Cmd_jumpifnodamage,                          //0xCF
    Cmd_settaunt,                                //0xD0
    Cmd_trysethelpinghand,                       //0xD1
    Cmd_tryswapitems,                            //0xD2
    Cmd_trycopyability,                          //0xD3
    Cmd_trywish,                                 //0xD4
    Cmd_trysetroots,                             //0xD5
    Cmd_doubledamagedealtifdamaged,              //0xD6
    Cmd_setyawn,                                 //0xD7
    Cmd_setdamagetohealthdifference,             //0xD8
    Cmd_scaledamagebyhealthratio,                //0xD9
    Cmd_tryswapabilities,                        //0xDA
    Cmd_tryimprison,                             //0xDB
    Cmd_trysetgrudge,                            //0xDC
    Cmd_weightdamagecalculation,                 //0xDD
    Cmd_assistattackselect,                      //0xDE
    Cmd_trysetmagiccoat,                         //0xDF
    Cmd_trysetsnatch,                            //0xE0
    Cmd_trygetintimidatetarget,                  //0xE1
    Cmd_switchoutabilities,                      //0xE2
    Cmd_jumpifhasnohp,                           //0xE3
    Cmd_getsecretpowereffect,                    //0xE4
    Cmd_pickup,                                  //0xE5
    Cmd_docastformchangeanimation,               //0xE6
    Cmd_trycastformdatachange,                   //0xE7
    Cmd_settypebasedhalvers,                     //0xE8
    Cmd_setweatherballtype,                      //0xE9
    Cmd_tryrecycleitem,                          //0xEA
    Cmd_settypetoenvironment,                    //0xEB
    Cmd_pursuitdoubles,                          //0xEC
    Cmd_snatchsetbattlers,                       //0xED
    Cmd_removelightscreenreflect,                //0xEE
    Cmd_handleballthrow,                         //0xEF
    Cmd_givecaughtmon,                           //0xF0
    Cmd_trysetcaughtmondexflags,                 //0xF1
    Cmd_displaydexinfo,                          //0xF2
    Cmd_trygivecaughtmonnick,                    //0xF3
    Cmd_subattackerhpbydmg,                      //0xF4
    Cmd_removeattackerstatus1,                   //0xF5
    Cmd_finishaction,                            //0xF6
    Cmd_finishturn,                              //0xF7
    Cmd_trainerslideout                          //0xF8
};

struct StatFractions
{
    u8 dividend;
    u8 divisor;
};

static const struct StatFractions sAccuracyStageRatios[] =
{
    { 33, 100}, // -6
    { 36, 100}, // -5
    { 43, 100}, // -4
    { 50, 100}, // -3
    { 60, 100}, // -2
    { 75, 100}, // -1
    {  1,   1}, //  0
    {133, 100}, // +1
    {166, 100}, // +2
    {  2,   1}, // +3
    {233, 100}, // +4
    {133,  50}, // +5
    {  3,   1}, // +6
};

// The chance is 1/N for each stage.
static const u16 sCriticalHitChance[] = {16, 8, 4, 3, 2};

static const u32 sStatusFlagsForMoveEffects[NUM_MOVE_EFFECTS] =
{
    [MOVE_EFFECT_SLEEP]          = STATUS1_SLEEP,
    [MOVE_EFFECT_POISON]         = STATUS1_POISON,
    [MOVE_EFFECT_BURN]           = STATUS1_BURN,
    [MOVE_EFFECT_FREEZE]         = STATUS1_FREEZE,
    [MOVE_EFFECT_PARALYSIS]      = STATUS1_PARALYSIS,
    [MOVE_EFFECT_TOXIC]          = STATUS1_TOXIC_POISON,
    [MOVE_EFFECT_CONFUSION]      = STATUS2_CONFUSION,
    [MOVE_EFFECT_FLINCH]         = STATUS2_FLINCHED,
    [MOVE_EFFECT_UPROAR]         = STATUS2_UPROAR,
    [MOVE_EFFECT_CHARGING]       = STATUS2_MULTIPLETURNS,
    [MOVE_EFFECT_WRAP]           = STATUS2_WRAPPED,
    [MOVE_EFFECT_RECHARGE]       = STATUS2_RECHARGE,
    [MOVE_EFFECT_PREVENT_ESCAPE] = STATUS2_ESCAPE_PREVENTION,
    [MOVE_EFFECT_NIGHTMARE]      = STATUS2_NIGHTMARE,
    [MOVE_EFFECT_THRASH]         = STATUS2_LOCK_CONFUSE,
};

static const u8 *const sMoveEffectBS_Ptrs[] =
{
    [0]                            = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_SLEEP]            = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_POISON]           = BattleScript_MoveEffectPoison,
    [MOVE_EFFECT_BURN]             = BattleScript_MoveEffectBurn,
    [MOVE_EFFECT_FREEZE]           = BattleScript_MoveEffectFreeze,
    [MOVE_EFFECT_PARALYSIS]        = BattleScript_MoveEffectParalysis,
    [MOVE_EFFECT_TOXIC]            = BattleScript_MoveEffectToxic,
    [MOVE_EFFECT_CONFUSION]        = BattleScript_MoveEffectConfusion,
    [MOVE_EFFECT_FLINCH]           = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_TRI_ATTACK]       = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_UPROAR]           = BattleScript_MoveEffectUproar,
    [MOVE_EFFECT_PAYDAY]           = BattleScript_MoveEffectPayDay,
    [MOVE_EFFECT_CHARGING]         = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_WRAP]             = BattleScript_MoveEffectWrap,
    [MOVE_EFFECT_RECOIL_25]        = BattleScript_MoveEffectRecoil,
    [MOVE_EFFECT_ATK_PLUS_1]       = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_DEF_PLUS_1]       = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_SPD_PLUS_1]       = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_SP_ATK_PLUS_1]    = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_SP_DEF_PLUS_1]    = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_ACC_PLUS_1]       = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_EVS_PLUS_1]       = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_ATK_MINUS_1]      = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_DEF_MINUS_1]      = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_SPD_MINUS_1]      = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_SP_ATK_MINUS_1]   = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_SP_DEF_MINUS_1]   = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_ACC_MINUS_1]      = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_EVS_MINUS_1]      = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_RECHARGE]         = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_RAGE]             = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_STEAL_ITEM]       = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_PREVENT_ESCAPE]   = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_NIGHTMARE]        = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_ALL_STATS_UP]     = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_RAPIDSPIN]        = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_REMOVE_PARALYSIS] = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_ATK_DEF_DOWN]     = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_RECOIL_33]        = BattleScript_MoveEffectRecoil,
};

static const struct WindowTemplate sUnusedWinTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 3,
    .width = 7,
    .height = 15,
    .paletteNum = 31,
    .baseBlock = 0x3F
};

static const u16 sLevelUpBanner_Pal[] = INCBIN_U16("graphics/battle_interface/level_up_banner.gbapal");
static const u32 sLevelUpBanner_Gfx[] = INCBIN_U32("graphics/battle_interface/level_up_banner.4bpp.lz");

// unused
static const u8 sRubyLevelUpStatBoxStats[] =
{
    MON_DATA_MAX_HP, MON_DATA_SPATK, MON_DATA_ATK,
    MON_DATA_SPDEF, MON_DATA_DEF, MON_DATA_SPEED
};

static const struct OamData sOamData_MonIconOnLvlUpBanner =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct SpriteTemplate sSpriteTemplate_MonIconOnLvlUpBanner =
{
    .tileTag = TAG_LVLUP_BANNER_MON_ICON,
    .paletteTag = TAG_LVLUP_BANNER_MON_ICON,
    .oam = &sOamData_MonIconOnLvlUpBanner,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_MonIconOnLvlUpBanner
};

static const u16 sProtectSuccessRates[] = {USHRT_MAX, USHRT_MAX / 2, USHRT_MAX / 4, USHRT_MAX / 8};

#define MIMIC_FORBIDDEN_END             0xFFFE
#define METRONOME_FORBIDDEN_END         0xFFFF
#define ASSIST_FORBIDDEN_END            0xFFFF

static const u16 sMovesForbiddenToCopy[] =
{
    MOVE_METRONOME,
    MOVE_STRUGGLE,
    MOVE_SKETCH,
    MOVE_MIMIC,
    MIMIC_FORBIDDEN_END,
    MOVE_COUNTER,
    MOVE_MIRROR_COAT,
    MOVE_PROTECT,
    MOVE_DETECT,
    MOVE_ENDURE,
    MOVE_DESTINY_BOND,
    MOVE_SLEEP_TALK,
    MOVE_THIEF,
    MOVE_FOLLOW_ME,
    MOVE_SNATCH,
    MOVE_HELPING_HAND,
    MOVE_COVET,
    MOVE_TRICK,
    MOVE_FOCUS_PUNCH,
    METRONOME_FORBIDDEN_END
};

static const u8 sFlailHpScaleToPowerTable[] =
{
    1, 200,
    4, 150,
    9, 100,
    16, 80,
    32, 40,
    48, 20
};

static const u16 sNaturePowerMoves[] =
{
    [BATTLE_ENVIRONMENT_GRASS]      = MOVE_STUN_SPORE,
    [BATTLE_ENVIRONMENT_LONG_GRASS] = MOVE_RAZOR_LEAF,
    [BATTLE_ENVIRONMENT_SAND]       = MOVE_EARTHQUAKE,
    [BATTLE_ENVIRONMENT_UNDERWATER] = MOVE_HYDRO_PUMP,
    [BATTLE_ENVIRONMENT_WATER]      = MOVE_SURF,
    [BATTLE_ENVIRONMENT_POND]       = MOVE_BUBBLE_BEAM,
    [BATTLE_ENVIRONMENT_MOUNTAIN]   = MOVE_ROCK_SLIDE,
    [BATTLE_ENVIRONMENT_CAVE]       = MOVE_SHADOW_BALL,
    [BATTLE_ENVIRONMENT_BUILDING]   = MOVE_SWIFT,
    [BATTLE_ENVIRONMENT_PLAIN]      = MOVE_SWIFT
};

// format: min. weight (hectograms), base power
static const u16 sWeightToDamageTable[] =
{
    100, 20,
    250, 40,
    500, 60,
    1000, 80,
    2000, 100,
    0xFFFF, 0xFFFF
};

static const u16 sPickupItems[] =
{
    ITEM_POTION,
    ITEM_ANTIDOTE,
    ITEM_SUPER_POTION,
    ITEM_GREAT_BALL,
    ITEM_REPEL,
    ITEM_ESCAPE_ROPE,
    ITEM_X_ATTACK,
    ITEM_FULL_HEAL,
    ITEM_ULTRA_BALL,
    ITEM_HYPER_POTION,
    ITEM_RARE_CANDY,
    ITEM_PROTEIN,
    ITEM_REVIVE,
    ITEM_HP_UP,
    ITEM_FULL_RESTORE,
    ITEM_MAX_REVIVE,
    ITEM_PP_UP,
    ITEM_MAX_ELIXIR,
};

static const u16 sRarePickupItems[] =
{
    ITEM_HYPER_POTION,
    ITEM_NUGGET,
    ITEM_KINGS_ROCK,
    ITEM_FULL_RESTORE,
    ITEM_ETHER,
    ITEM_WHITE_HERB,
    ITEM_TM_REST,
    ITEM_ELIXIR,
    ITEM_TM_FOCUS_PUNCH,
    ITEM_LEFTOVERS,
    ITEM_TM_EARTHQUAKE,
};

static const u8 sPickupProbabilities[] =
{
    30, 40, 50, 60, 70, 80, 90, 94, 98
};

static const u8 sEnvironmentToType[] =
{
    [BATTLE_ENVIRONMENT_GRASS]      = TYPE_GRASS,
    [BATTLE_ENVIRONMENT_LONG_GRASS] = TYPE_GRASS,
    [BATTLE_ENVIRONMENT_SAND]       = TYPE_GROUND,
    [BATTLE_ENVIRONMENT_UNDERWATER] = TYPE_WATER,
    [BATTLE_ENVIRONMENT_WATER]      = TYPE_WATER,
    [BATTLE_ENVIRONMENT_POND]       = TYPE_WATER,
    [BATTLE_ENVIRONMENT_MOUNTAIN]   = TYPE_ROCK,
    [BATTLE_ENVIRONMENT_CAVE]       = TYPE_ROCK,
    [BATTLE_ENVIRONMENT_BUILDING]   = TYPE_NORMAL,
    [BATTLE_ENVIRONMENT_PLAIN]      = TYPE_NORMAL,
};

// - ITEM_ULTRA_BALL skips Master Ball and ITEM_NONE
static const u8 sBallCatchBonuses[] =
{
    [ITEM_ULTRA_BALL - ITEM_ULTRA_BALL]  = 20,
    [ITEM_GREAT_BALL - ITEM_ULTRA_BALL]  = 15,
    [ITEM_POKE_BALL - ITEM_ULTRA_BALL]   = 10,
    [ITEM_SAFARI_BALL - ITEM_ULTRA_BALL] = 15
};

// In Battle Palace, moves are chosen based on the pokemons nature rather than by the player
// Moves are grouped into "Attack", "Defense", or "Support" (see PALACE_MOVE_GROUP_*)
// Each nature has a certain percent chance of selecting a move from a particular group
// and a separate percent chance for each group when at or below 50% HP
// The table below doesn't list percentages for Support because you can subtract the other two
// Support percentages are listed in comments off to the side instead
#define PALACE_STYLE(atk, def, atkLow, defLow) {atk, atk + def, atkLow, atkLow + defLow}

const ALIGNED(4) u8 gBattlePalaceNatureToMoveGroupLikelihood[NUM_NATURES][4] =
{
    [NATURE_HARDY]   = PALACE_STYLE(61,  7, 61,  7), // 32% support >= 50% HP, 32% support < 50% HP
    [NATURE_LONELY]  = PALACE_STYLE(20, 25, 84,  8), // 55%,  8%
    [NATURE_BRAVE]   = PALACE_STYLE(70, 15, 32, 60), // 15%,  8%
    [NATURE_ADAMANT] = PALACE_STYLE(38, 31, 70, 15), // 31%, 15%
    [NATURE_NAUGHTY] = PALACE_STYLE(20, 70, 70, 22), // 10%,  8%
    [NATURE_BOLD]    = PALACE_STYLE(30, 20, 32, 58), // 50%, 10%
    [NATURE_DOCILE]  = PALACE_STYLE(56, 22, 56, 22), // 22%, 22%
    [NATURE_RELAXED] = PALACE_STYLE(25, 15, 75, 15), // 60%, 10%
    [NATURE_IMPISH]  = PALACE_STYLE(69,  6, 28, 55), // 25%, 17%
    [NATURE_LAX]     = PALACE_STYLE(35, 10, 29,  6), // 55%, 65%
    [NATURE_TIMID]   = PALACE_STYLE(62, 10, 30, 20), // 28%, 50%
    [NATURE_HASTY]   = PALACE_STYLE(58, 37, 88,  6), //  5%,  6%
    [NATURE_SERIOUS] = PALACE_STYLE(34, 11, 29, 11), // 55%, 60%
    [NATURE_JOLLY]   = PALACE_STYLE(35,  5, 35, 60), // 60%,  5%
    [NATURE_NAIVE]   = PALACE_STYLE(56, 22, 56, 22), // 22%, 22%
    [NATURE_MODEST]  = PALACE_STYLE(35, 45, 34, 60), // 20%,  6%
    [NATURE_MILD]    = PALACE_STYLE(44, 50, 34,  6), //  6%, 60%
    [NATURE_QUIET]   = PALACE_STYLE(56, 22, 56, 22), // 22%, 22%
    [NATURE_BASHFUL] = PALACE_STYLE(30, 58, 30, 58), // 12%, 12%
    [NATURE_RASH]    = PALACE_STYLE(30, 13, 27,  6), // 57%, 67%
    [NATURE_CALM]    = PALACE_STYLE(40, 50, 25, 62), // 10%, 13%
    [NATURE_GENTLE]  = PALACE_STYLE(18, 70, 90,  5), // 12%,  5%
    [NATURE_SASSY]   = PALACE_STYLE(88,  6, 22, 20), //  6%, 58%
    [NATURE_CAREFUL] = PALACE_STYLE(42, 50, 42,  5), //  8%, 53%
    [NATURE_QUIRKY]  = PALACE_STYLE(56, 22, 56, 22)  // 22%, 22%
};

static const u8 sBattlePalaceNatureToFlavorTextId[NUM_NATURES] =
{
    [NATURE_HARDY]   = B_MSG_EAGER_FOR_MORE,
    [NATURE_LONELY]  = B_MSG_GLINT_IN_EYE,
    [NATURE_BRAVE]   = B_MSG_GETTING_IN_POS,
    [NATURE_ADAMANT] = B_MSG_GLINT_IN_EYE,
    [NATURE_NAUGHTY] = B_MSG_GLINT_IN_EYE,
    [NATURE_BOLD]    = B_MSG_GETTING_IN_POS,
    [NATURE_DOCILE]  = B_MSG_EAGER_FOR_MORE,
    [NATURE_RELAXED] = B_MSG_GLINT_IN_EYE,
    [NATURE_IMPISH]  = B_MSG_GETTING_IN_POS,
    [NATURE_LAX]     = B_MSG_GROWL_DEEPLY,
    [NATURE_TIMID]   = B_MSG_GROWL_DEEPLY,
    [NATURE_HASTY]   = B_MSG_GLINT_IN_EYE,
    [NATURE_SERIOUS] = B_MSG_EAGER_FOR_MORE,
    [NATURE_JOLLY]   = B_MSG_GETTING_IN_POS,
    [NATURE_NAIVE]   = B_MSG_EAGER_FOR_MORE,
    [NATURE_MODEST]  = B_MSG_GETTING_IN_POS,
    [NATURE_MILD]    = B_MSG_GROWL_DEEPLY,
    [NATURE_QUIET]   = B_MSG_EAGER_FOR_MORE,
    [NATURE_BASHFUL] = B_MSG_EAGER_FOR_MORE,
    [NATURE_RASH]    = B_MSG_GROWL_DEEPLY,
    [NATURE_CALM]    = B_MSG_GETTING_IN_POS,
    [NATURE_GENTLE]  = B_MSG_GLINT_IN_EYE,
    [NATURE_SASSY]   = B_MSG_GROWL_DEEPLY,
    [NATURE_CAREFUL] = B_MSG_GROWL_DEEPLY,
    [NATURE_QUIRKY]  = B_MSG_EAGER_FOR_MORE,
};

static void Cmd_attackcanceler(void)
{
    s32 i;

    if (gBattleOutcome != 0)
    {
        gCurrentActionFuncId = B_ACTION_FINISHED;
        return;
    }
    if (gBattleMons[gBattlerAttacker].hp == 0 && !(gHitMarker & HITMARKER_NO_ATTACKSTRING))
    {
        gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
        gBattlescriptCurrInstr = BattleScript_MoveEnd;
        return;
    }
    if (AtkCanceller_UnableToUseMove())
        return;
    if (AbilityBattleEffects(ABILITYEFFECT_MOVES_BLOCK, gBattlerTarget, 0, 0, 0))
        return;
    if (!gBattleMons[gBattlerAttacker].pp[gCurrMovePos] && gCurrentMove != MOVE_STRUGGLE && !(gHitMarker & (HITMARKER_ALLOW_NO_PP | HITMARKER_NO_ATTACKSTRING))
     && !(gBattleMons[gBattlerAttacker].status2 & STATUS2_MULTIPLETURNS))
    {
        gBattlescriptCurrInstr = BattleScript_NoPPForMove;
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        return;
    }

    gHitMarker &= ~HITMARKER_ALLOW_NO_PP;

    if (!(gHitMarker & HITMARKER_OBEYS) && !(gBattleMons[gBattlerAttacker].status2 & STATUS2_MULTIPLETURNS))
    {
        i = IsMonDisobedient(); // why use the 'i' variable...?
        switch (i)
        {
        case 0:
            break;
        case 2:
            gHitMarker |= HITMARKER_OBEYS;
            return;
        default:
            gMoveResultFlags |= MOVE_RESULT_MISSED;
            return;
        }
    }

    gHitMarker |= HITMARKER_OBEYS;

    if (gProtectStructs[gBattlerTarget].bounceMove && gBattleMoves[gCurrentMove].flags & FLAG_MAGIC_COAT_AFFECTED)
    {
        PressurePPLose(gBattlerAttacker, gBattlerTarget, MOVE_MAGIC_COAT);
        gProtectStructs[gBattlerTarget].bounceMove = FALSE;
        BattleScriptPushCursor();
        gBattlescriptCurrInstr = BattleScript_MagicCoatBounce;
        return;
    }

    for (i = 0; i < gBattlersCount; i++)
    {
        if ((gProtectStructs[gBattlerByTurnOrder[i]].stealMove) && gBattleMoves[gCurrentMove].flags & FLAG_SNATCH_AFFECTED)
        {
            PressurePPLose(gBattlerAttacker, gBattlerByTurnOrder[i], MOVE_SNATCH);
            gProtectStructs[gBattlerByTurnOrder[i]].stealMove = FALSE;
            gBattleScripting.battler = gBattlerByTurnOrder[i];
            BattleScriptPushCursor();
            gBattlescriptCurrInstr = BattleScript_SnatchedMove;
            return;
        }
    }

    if (gSpecialStatuses[gBattlerTarget].lightningRodRedirected)
    {
        gSpecialStatuses[gBattlerTarget].lightningRodRedirected = FALSE;
        gLastUsedAbility = ABILITY_LIGHTNING_ROD;
        BattleScriptPushCursor();
        gBattlescriptCurrInstr = BattleScript_TookAttack;
        RecordAbilityBattle(gBattlerTarget, gLastUsedAbility);
    }
    else if (DEFENDER_IS_PROTECTED
     && (gCurrentMove != MOVE_CURSE || IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_GHOST))
     && ((!IsTwoTurnsMove(gCurrentMove) || (gBattleMons[gBattlerAttacker].status2 & STATUS2_MULTIPLETURNS))))
    {
        CancelMultiTurnMoves(gBattlerAttacker);
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gLastLandedMoves[gBattlerTarget] = 0;
        gLastHitByType[gBattlerTarget] = 0;
        gBattleCommunication[MISS_TYPE] = B_MSG_PROTECTED;
        gBattlescriptCurrInstr++;
    }
    else
    {
        gBattlescriptCurrInstr++;
    }
}

static void JumpIfMoveFailed(u8 adder, u16 move)
{
    const u8 *BS_ptr = gBattlescriptCurrInstr + adder;
    if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
    {
        gLastLandedMoves[gBattlerTarget] = 0;
        gLastHitByType[gBattlerTarget] = 0;
        BS_ptr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        TrySetDestinyBondToHappen();
        if (AbilityBattleEffects(ABILITYEFFECT_ABSORBING, gBattlerTarget, 0, 0, move))
            return;
    }
    gBattlescriptCurrInstr = BS_ptr;
}

static void Cmd_jumpifaffectedbyprotect(void)
{
    if (DEFENDER_IS_PROTECTED)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        JumpIfMoveFailed(5, 0);
        gBattleCommunication[MISS_TYPE] = B_MSG_PROTECTED;
    }
    else
    {
        gBattlescriptCurrInstr += 5;
    }
}

static bool8 JumpIfMoveAffectedByProtect(u16 move)
{
    bool8 affected = FALSE;
    if (DEFENDER_IS_PROTECTED)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        JumpIfMoveFailed(7, move);
        gBattleCommunication[MISS_TYPE] = B_MSG_PROTECTED;
        affected = TRUE;
    }
    return affected;
}

static bool8 AccuracyCalcHelper(u16 move)
{
    if (gStatuses3[gBattlerTarget] & STATUS3_ALWAYS_HITS && gDisableStructs[gBattlerTarget].battlerWithSureHit == gBattlerAttacker)
    {
        JumpIfMoveFailed(7, move);
        return TRUE;
    }

    if (!(gHitMarker & HITMARKER_IGNORE_ON_AIR) && gStatuses3[gBattlerTarget] & STATUS3_ON_AIR)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        JumpIfMoveFailed(7, move);
        return TRUE;
    }

    gHitMarker &= ~HITMARKER_IGNORE_ON_AIR;

    if (!(gHitMarker & HITMARKER_IGNORE_UNDERGROUND) && gStatuses3[gBattlerTarget] & STATUS3_UNDERGROUND)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        JumpIfMoveFailed(7, move);
        return TRUE;
    }

    gHitMarker &= ~HITMARKER_IGNORE_UNDERGROUND;

    if (!(gHitMarker & HITMARKER_IGNORE_UNDERWATER) && gStatuses3[gBattlerTarget] & STATUS3_UNDERWATER)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        JumpIfMoveFailed(7, move);
        return TRUE;
    }

    gHitMarker &= ~HITMARKER_IGNORE_UNDERWATER;

    if ((WEATHER_HAS_EFFECT && (gBattleWeather & B_WEATHER_RAIN) && gBattleMoves[move].effect == EFFECT_THUNDER)
     || (gBattleMoves[move].effect == EFFECT_ALWAYS_HIT || gBattleMoves[move].effect == EFFECT_VITAL_THROW))
    {
        JumpIfMoveFailed(7, move);
        return TRUE;
    }

    return FALSE;
}

static void Cmd_accuracycheck(void)
{
    u16 move = T2_READ_16(gBattlescriptCurrInstr + 5);

    if (move == NO_ACC_CALC || move == NO_ACC_CALC_CHECK_LOCK_ON)
    {
        if (gStatuses3[gBattlerTarget] & STATUS3_ALWAYS_HITS && move == NO_ACC_CALC_CHECK_LOCK_ON && gDisableStructs[gBattlerTarget].battlerWithSureHit == gBattlerAttacker)
            gBattlescriptCurrInstr += 7;
        else if (gStatuses3[gBattlerTarget] & (STATUS3_ON_AIR | STATUS3_UNDERGROUND | STATUS3_UNDERWATER))
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        else if (!JumpIfMoveAffectedByProtect(0))
            gBattlescriptCurrInstr += 7;
    }
    else
    {
        u8 type, moveAcc, holdEffect, param;
        s8 buff;
        u16 calc;

        if (move == ACC_CURR_MOVE)
            move = gCurrentMove;

        GET_MOVE_TYPE(move, type);

        if (JumpIfMoveAffectedByProtect(move))
            return;
        if (AccuracyCalcHelper(move))
            return;

        if (gBattleMons[gBattlerTarget].status2 & STATUS2_FORESIGHT)
        {
            u8 acc = gBattleMons[gBattlerAttacker].statStages[STAT_ACC];
            buff = acc;
        }
        else
        {
            u8 acc = gBattleMons[gBattlerAttacker].statStages[STAT_ACC];
            buff = acc + DEFAULT_STAT_STAGE - gBattleMons[gBattlerTarget].statStages[STAT_EVASION];
        }

        if (buff < MIN_STAT_STAGE)
            buff = MIN_STAT_STAGE;
        if (buff > MAX_STAT_STAGE)
            buff = MAX_STAT_STAGE;

        moveAcc = gBattleMoves[move].accuracy;
        // check Thunder on sunny weather
        if (WEATHER_HAS_EFFECT && gBattleWeather & B_WEATHER_SUN && gBattleMoves[move].effect == EFFECT_THUNDER)
            moveAcc = 50;

        calc = sAccuracyStageRatios[buff].dividend * moveAcc;
        calc /= sAccuracyStageRatios[buff].divisor;

        if (gBattleMons[gBattlerAttacker].ability == ABILITY_COMPOUND_EYES)
            calc = (calc * 130) / 100; // 1.3 compound eyes boost
        if (WEATHER_HAS_EFFECT && gBattleMons[gBattlerTarget].ability == ABILITY_SAND_VEIL && gBattleWeather & B_WEATHER_SANDSTORM)
            calc = (calc * 80) / 100; // 1.2 sand veil loss
        if (gBattleMons[gBattlerAttacker].ability == ABILITY_HUSTLE && IS_TYPE_PHYSICAL(type))
            calc = (calc * 80) / 100; // 1.2 hustle loss

        if (gBattleMons[gBattlerTarget].item == ITEM_ENIGMA_BERRY)
        {
            holdEffect = gEnigmaBerries[gBattlerTarget].holdEffect;
            param = gEnigmaBerries[gBattlerTarget].holdEffectParam;
        }
        else
        {
            holdEffect = ItemId_GetHoldEffect(gBattleMons[gBattlerTarget].item);
            param = ItemId_GetHoldEffectParam(gBattleMons[gBattlerTarget].item);
        }

        gPotentialItemEffectBattler = gBattlerTarget;

        if (holdEffect == HOLD_EFFECT_EVASION_UP)
            calc = (calc * (100 - param)) / 100;

        // final calculation
        if ((Random() % 100 + 1) > calc)
        {
            gMoveResultFlags |= MOVE_RESULT_MISSED;
            if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
             && (gBattleMoves[move].target == MOVE_TARGET_BOTH || gBattleMoves[move].target == MOVE_TARGET_FOES_AND_ALLY))
                gBattleCommunication[MISS_TYPE] = B_MSG_AVOIDED_ATK;
            else
                gBattleCommunication[MISS_TYPE] = B_MSG_MISSED;

            CheckWonderGuardAndLevitate();
        }
        JumpIfMoveFailed(7, move);
    }
}

static void Cmd_attackstring(void)
{
    if (gBattleControllerExecFlags)
        return;

    if (!(gHitMarker & (HITMARKER_NO_ATTACKSTRING | HITMARKER_ATTACKSTRING_PRINTED)))
    {
        PrepareStringBattle(STRINGID_USEDMOVE, gBattlerAttacker);
        gHitMarker |= HITMARKER_ATTACKSTRING_PRINTED;
    }
    gBattlescriptCurrInstr++;
    gBattleCommunication[MSG_DISPLAY] = 0;
}

static void Cmd_ppreduce(void)
{
    s32 ppToDeduct = 1;

    if (gBattleControllerExecFlags)
        return;

    if (!gSpecialStatuses[gBattlerAttacker].ppNotAffectedByPressure)
    {
        switch (gBattleMoves[gCurrentMove].target)
        {
        case MOVE_TARGET_FOES_AND_ALLY:
            ppToDeduct += AbilityBattleEffects(ABILITYEFFECT_COUNT_ON_FIELD, gBattlerAttacker, ABILITY_PRESSURE, 0, 0);
            break;
        case MOVE_TARGET_BOTH:
        case MOVE_TARGET_OPPONENTS_FIELD:
            ppToDeduct += AbilityBattleEffects(ABILITYEFFECT_COUNT_OTHER_SIDE, gBattlerAttacker, ABILITY_PRESSURE, 0, 0);
            break;
        default:
            if (gBattlerAttacker != gBattlerTarget && gBattleMons[gBattlerTarget].ability == ABILITY_PRESSURE)
                ppToDeduct++;
            break;
        }
    }

    if (!(gHitMarker & (HITMARKER_NO_PPDEDUCT | HITMARKER_NO_ATTACKSTRING)) && gBattleMons[gBattlerAttacker].pp[gCurrMovePos])
    {
        gProtectStructs[gBattlerAttacker].notFirstStrike = 1;

        if (gBattleMons[gBattlerAttacker].pp[gCurrMovePos] > ppToDeduct)
            gBattleMons[gBattlerAttacker].pp[gCurrMovePos] -= ppToDeduct;
        else
            gBattleMons[gBattlerAttacker].pp[gCurrMovePos] = 0;

        if (MOVE_IS_PERMANENT(gBattlerAttacker, gCurrMovePos))
        {
            gActiveBattler = gBattlerAttacker;
            BtlController_EmitSetMonData(BUFFER_A, REQUEST_PPMOVE1_BATTLE + gCurrMovePos, 0,
                                         sizeof(gBattleMons[gBattlerAttacker].pp[gCurrMovePos]),
                                         &gBattleMons[gBattlerAttacker].pp[gCurrMovePos]);
            MarkBattlerForControllerExec(gBattlerAttacker);
        }
    }

    gHitMarker &= ~HITMARKER_NO_PPDEDUCT;
    gBattlescriptCurrInstr++;
}

static void Cmd_critcalc(void)
{
    u8 holdEffect;
    u16 item, critChance;

    item = gBattleMons[gBattlerAttacker].item;

    if (item == ITEM_ENIGMA_BERRY)
        holdEffect = gEnigmaBerries[gBattlerAttacker].holdEffect;
    else
        holdEffect = ItemId_GetHoldEffect(item);

    gPotentialItemEffectBattler = gBattlerAttacker;

    critChance  = 2 * ((gBattleMons[gBattlerAttacker].status2 & STATUS2_FOCUS_ENERGY) != 0)
                + (gBattleMoves[gCurrentMove].effect == EFFECT_HIGH_CRITICAL)
                + (gBattleMoves[gCurrentMove].effect == EFFECT_SKY_ATTACK)
                + (gBattleMoves[gCurrentMove].effect == EFFECT_BLAZE_KICK)
                + (gBattleMoves[gCurrentMove].effect == EFFECT_POISON_TAIL)
                + (holdEffect == HOLD_EFFECT_SCOPE_LENS)
                + 2 * (holdEffect == HOLD_EFFECT_LUCKY_PUNCH && gBattleMons[gBattlerAttacker].species == SPECIES_CHANSEY)
                + 2 * (holdEffect == HOLD_EFFECT_STICK && gBattleMons[gBattlerAttacker].species == SPECIES_FARFETCHD);

    if (critChance >= ARRAY_COUNT(sCriticalHitChance))
        critChance = ARRAY_COUNT(sCriticalHitChance) - 1;

    if ((gBattleMons[gBattlerTarget].ability != ABILITY_BATTLE_ARMOR && gBattleMons[gBattlerTarget].ability != ABILITY_SHELL_ARMOR)
     && !(gStatuses3[gBattlerAttacker] & STATUS3_CANT_SCORE_A_CRIT)
     && !(gBattleTypeFlags & (BATTLE_TYPE_WALLY_TUTORIAL | BATTLE_TYPE_FIRST_BATTLE))
     && !(Random() % sCriticalHitChance[critChance]))
        gCritMultiplier = 2;
    else
        gCritMultiplier = 1;

    gBattlescriptCurrInstr++;
}

static void Cmd_damagecalc(void)
{
    u16 sideStatus = gSideStatuses[GET_BATTLER_SIDE(gBattlerTarget)];
    gBattleMoveDamage = CalculateBaseDamage(&gBattleMons[gBattlerAttacker], &gBattleMons[gBattlerTarget], gCurrentMove,
                                            sideStatus, gDynamicBasePower,
                                            gBattleStruct->dynamicMoveType, gBattlerAttacker, gBattlerTarget);
    gBattleMoveDamage = gBattleMoveDamage * gCritMultiplier * gBattleScripting.dmgMultiplier;

    if (gStatuses3[gBattlerAttacker] & STATUS3_CHARGED_UP && gBattleMoves[gCurrentMove].type == TYPE_ELECTRIC)
        gBattleMoveDamage *= 2;
    if (gProtectStructs[gBattlerAttacker].helpingHand)
        gBattleMoveDamage = gBattleMoveDamage * 15 / 10;

    gBattlescriptCurrInstr++;
}

void AI_CalcDmg(u8 attacker, u8 defender)
{
    u16 sideStatus = gSideStatuses[GET_BATTLER_SIDE(defender)];
    gBattleMoveDamage = CalculateBaseDamage(&gBattleMons[attacker], &gBattleMons[defender], gCurrentMove,
                                            sideStatus, gDynamicBasePower,
                                            gBattleStruct->dynamicMoveType, attacker, defender);
    gDynamicBasePower = 0;
    gBattleMoveDamage = gBattleMoveDamage * gCritMultiplier * gBattleScripting.dmgMultiplier;

    if (gStatuses3[attacker] & STATUS3_CHARGED_UP && gBattleMoves[gCurrentMove].type == TYPE_ELECTRIC)
        gBattleMoveDamage *= 2;
    if (gProtectStructs[attacker].helpingHand)
        gBattleMoveDamage = gBattleMoveDamage * 15 / 10;
}

static void ModulateDmgByType(u8 multiplier)
{
    gBattleMoveDamage = gBattleMoveDamage * multiplier / 10;
    if (gBattleMoveDamage == 0 && multiplier != 0)
        gBattleMoveDamage = 1;

    switch (multiplier)
    {
    case TYPE_MUL_NO_EFFECT:
        gMoveResultFlags |= MOVE_RESULT_DOESNT_AFFECT_FOE;
        gMoveResultFlags &= ~MOVE_RESULT_NOT_VERY_EFFECTIVE;
        gMoveResultFlags &= ~MOVE_RESULT_SUPER_EFFECTIVE;
        break;
    case TYPE_MUL_NOT_EFFECTIVE:
        if (gBattleMoves[gCurrentMove].power && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
        {
            if (gMoveResultFlags & MOVE_RESULT_SUPER_EFFECTIVE)
                gMoveResultFlags &= ~MOVE_RESULT_SUPER_EFFECTIVE;
            else
                gMoveResultFlags |= MOVE_RESULT_NOT_VERY_EFFECTIVE;
        }
        break;
    case TYPE_MUL_SUPER_EFFECTIVE:
        if (gBattleMoves[gCurrentMove].power && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
        {
            if (gMoveResultFlags & MOVE_RESULT_NOT_VERY_EFFECTIVE)
                gMoveResultFlags &= ~MOVE_RESULT_NOT_VERY_EFFECTIVE;
            else
                gMoveResultFlags |= MOVE_RESULT_SUPER_EFFECTIVE;
        }
        break;
    }
}

static void Cmd_typecalc(void)
{
    s32 i = 0;
    u8 moveType;

    if (gCurrentMove == MOVE_STRUGGLE)
    {
        gBattlescriptCurrInstr++;
        return;
    }

    GET_MOVE_TYPE(gCurrentMove, moveType);

    // check stab
    if (IS_BATTLER_OF_TYPE(gBattlerAttacker, moveType))
    {
        gBattleMoveDamage = gBattleMoveDamage * 15;
        gBattleMoveDamage = gBattleMoveDamage / 10;
    }

    if (gBattleMons[gBattlerTarget].ability == ABILITY_LEVITATE && moveType == TYPE_GROUND)
    {
        gLastUsedAbility = gBattleMons[gBattlerTarget].ability;
        gMoveResultFlags |= (MOVE_RESULT_MISSED | MOVE_RESULT_DOESNT_AFFECT_FOE);
        gLastLandedMoves[gBattlerTarget] = 0;
        gLastHitByType[gBattlerTarget] = 0;
        gBattleCommunication[MISS_TYPE] = B_MSG_GROUND_MISS;
        RecordAbilityBattle(gBattlerTarget, gLastUsedAbility);
    }
    else
    {
        while (TYPE_EFFECT_ATK_TYPE(i) != TYPE_ENDTABLE)
        {
            if (TYPE_EFFECT_ATK_TYPE(i) == TYPE_FORESIGHT)
            {
                if (gBattleMons[gBattlerTarget].status2 & STATUS2_FORESIGHT)
                    break;
                i += 3;
                continue;
            }
            else if (TYPE_EFFECT_ATK_TYPE(i) == moveType)
            {
                // check type1
                if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[gBattlerTarget].types[0])
                    ModulateDmgByType(TYPE_EFFECT_MULTIPLIER(i));
                // check type2
                if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[gBattlerTarget].types[1] &&
                    gBattleMons[gBattlerTarget].types[0] != gBattleMons[gBattlerTarget].types[1])
                    ModulateDmgByType(TYPE_EFFECT_MULTIPLIER(i));
            }
            i += 3;
        }
    }

    if (gBattleMons[gBattlerTarget].ability == ABILITY_WONDER_GUARD && AttacksThisTurn(gBattlerAttacker, gCurrentMove) == 2
     && (!(gMoveResultFlags & MOVE_RESULT_SUPER_EFFECTIVE) || ((gMoveResultFlags & (MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE)) == (MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE)))
     && gBattleMoves[gCurrentMove].power)
    {
        gLastUsedAbility = ABILITY_WONDER_GUARD;
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gLastLandedMoves[gBattlerTarget] = 0;
        gLastHitByType[gBattlerTarget] = 0;
        gBattleCommunication[MISS_TYPE] = B_MSG_AVOIDED_DMG;
        RecordAbilityBattle(gBattlerTarget, gLastUsedAbility);
    }
    if (gMoveResultFlags & MOVE_RESULT_DOESNT_AFFECT_FOE)
        gProtectStructs[gBattlerAttacker].targetNotAffected = 1;

    gBattlescriptCurrInstr++;
}

static void CheckWonderGuardAndLevitate(void)
{
    u8 flags = 0;
    s32 i = 0;
    u8 moveType;

    if (gCurrentMove == MOVE_STRUGGLE || !gBattleMoves[gCurrentMove].power)
        return;

    GET_MOVE_TYPE(gCurrentMove, moveType);

    if (gBattleMons[gBattlerTarget].ability == ABILITY_LEVITATE && moveType == TYPE_GROUND)
    {
        gLastUsedAbility = ABILITY_LEVITATE;
        gBattleCommunication[MISS_TYPE] = B_MSG_GROUND_MISS;
        RecordAbilityBattle(gBattlerTarget, ABILITY_LEVITATE);
        return;
    }

    while (TYPE_EFFECT_ATK_TYPE(i) != TYPE_ENDTABLE)
    {
        if (TYPE_EFFECT_ATK_TYPE(i) == TYPE_FORESIGHT)
        {
            if (gBattleMons[gBattlerTarget].status2 & STATUS2_FORESIGHT)
                break;
            i += 3;
            continue;
        }
        if (TYPE_EFFECT_ATK_TYPE(i) == moveType)
        {
            // check no effect
            if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[gBattlerTarget].types[0]
                && TYPE_EFFECT_MULTIPLIER(i) == TYPE_MUL_NO_EFFECT)
            {
                gMoveResultFlags |= MOVE_RESULT_DOESNT_AFFECT_FOE;
                gProtectStructs[gBattlerAttacker].targetNotAffected = 1;
            }
            if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[gBattlerTarget].types[1] &&
                gBattleMons[gBattlerTarget].types[0] != gBattleMons[gBattlerTarget].types[1] &&
                TYPE_EFFECT_MULTIPLIER(i) == TYPE_MUL_NO_EFFECT)
            {
                gMoveResultFlags |= MOVE_RESULT_DOESNT_AFFECT_FOE;
                gProtectStructs[gBattlerAttacker].targetNotAffected = 1;
            }

            // check super effective
            if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[gBattlerTarget].types[0] && TYPE_EFFECT_MULTIPLIER(i) == 20)
                flags |= 1;
            if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[gBattlerTarget].types[1]
             && gBattleMons[gBattlerTarget].types[0] != gBattleMons[gBattlerTarget].types[1]
             && TYPE_EFFECT_MULTIPLIER(i) == TYPE_MUL_SUPER_EFFECTIVE)
                flags |= 1;

            // check not very effective
            if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[gBattlerTarget].types[0] && TYPE_EFFECT_MULTIPLIER(i) == 5)
                flags |= 2;
            if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[gBattlerTarget].types[1]
             && gBattleMons[gBattlerTarget].types[0] != gBattleMons[gBattlerTarget].types[1]
             && TYPE_EFFECT_MULTIPLIER(i) == TYPE_MUL_NOT_EFFECTIVE)
                flags |= 2;
        }
        i += 3;
    }

    if (gBattleMons[gBattlerTarget].ability == ABILITY_WONDER_GUARD && AttacksThisTurn(gBattlerAttacker, gCurrentMove) == 2)
    {
        if (((flags & 2) || !(flags & 1)) && gBattleMoves[gCurrentMove].power)
        {
            gLastUsedAbility = ABILITY_WONDER_GUARD;
            gBattleCommunication[MISS_TYPE] = B_MSG_AVOIDED_DMG;
            RecordAbilityBattle(gBattlerTarget, ABILITY_WONDER_GUARD);
        }
    }
}

// Same as ModulateDmgByType except different arguments
static void ModulateDmgByType2(u8 multiplier, u16 move, u8 *flags)
{
    gBattleMoveDamage = gBattleMoveDamage * multiplier / 10;
    if (gBattleMoveDamage == 0 && multiplier != 0)
        gBattleMoveDamage = 1;

    switch (multiplier)
    {
    case TYPE_MUL_NO_EFFECT:
        *flags |= MOVE_RESULT_DOESNT_AFFECT_FOE;
        *flags &= ~MOVE_RESULT_NOT_VERY_EFFECTIVE;
        *flags &= ~MOVE_RESULT_SUPER_EFFECTIVE;
        break;
    case TYPE_MUL_NOT_EFFECTIVE:
        if (gBattleMoves[move].power && !(*flags & MOVE_RESULT_NO_EFFECT))
        {
            if (*flags & MOVE_RESULT_SUPER_EFFECTIVE)
                *flags &= ~MOVE_RESULT_SUPER_EFFECTIVE;
            else
                *flags |= MOVE_RESULT_NOT_VERY_EFFECTIVE;
        }
        break;
    case TYPE_MUL_SUPER_EFFECTIVE:
        if (gBattleMoves[move].power && !(*flags & MOVE_RESULT_NO_EFFECT))
        {
            if (*flags & MOVE_RESULT_NOT_VERY_EFFECTIVE)
                *flags &= ~MOVE_RESULT_NOT_VERY_EFFECTIVE;
            else
                *flags |= MOVE_RESULT_SUPER_EFFECTIVE;
        }
        break;
    }
}

u8 TypeCalc(u16 move, u8 attacker, u8 defender)
{
    s32 i = 0;
    u8 flags = 0;
    u8 moveType;

    if (move == MOVE_STRUGGLE)
        return 0;

    moveType = gBattleMoves[move].type;

    // check stab
    if (IS_BATTLER_OF_TYPE(attacker, moveType))
    {
        gBattleMoveDamage = gBattleMoveDamage * 15;
        gBattleMoveDamage = gBattleMoveDamage / 10;
    }

    if (gBattleMons[defender].ability == ABILITY_LEVITATE && moveType == TYPE_GROUND)
    {
        flags |= (MOVE_RESULT_MISSED | MOVE_RESULT_DOESNT_AFFECT_FOE);
    }
    else
    {
        while (TYPE_EFFECT_ATK_TYPE(i) != TYPE_ENDTABLE)
        {
            if (TYPE_EFFECT_ATK_TYPE(i) == TYPE_FORESIGHT)
            {
                if (gBattleMons[defender].status2 & STATUS2_FORESIGHT)
                    break;
                i += 3;
                continue;
            }

            else if (TYPE_EFFECT_ATK_TYPE(i) == moveType)
            {
                // check type1
                if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[defender].types[0])
                    ModulateDmgByType2(TYPE_EFFECT_MULTIPLIER(i), move, &flags);
                // check type2
                if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[defender].types[1] &&
                    gBattleMons[defender].types[0] != gBattleMons[defender].types[1])
                    ModulateDmgByType2(TYPE_EFFECT_MULTIPLIER(i), move, &flags);
            }
            i += 3;
        }
    }

    if (gBattleMons[defender].ability == ABILITY_WONDER_GUARD && !(flags & MOVE_RESULT_MISSED)
        && AttacksThisTurn(attacker, move) == 2
        && (!(flags & MOVE_RESULT_SUPER_EFFECTIVE) || ((flags & (MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE)) == (MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE)))
        && gBattleMoves[move].power)
    {
        flags |= MOVE_RESULT_MISSED;
    }
    return flags;
}

u8 AI_TypeCalc(u16 move, u16 targetSpecies, u8 targetAbility)
{
    s32 i = 0;
    u8 flags = 0;
    u8 type1 = gSpeciesInfo[targetSpecies].types[0], type2 = gSpeciesInfo[targetSpecies].types[1];
    u8 moveType;

    if (move == MOVE_STRUGGLE)
        return 0;

    moveType = gBattleMoves[move].type;

    if (targetAbility == ABILITY_LEVITATE && moveType == TYPE_GROUND)
    {
        flags = MOVE_RESULT_MISSED | MOVE_RESULT_DOESNT_AFFECT_FOE;
    }
    else
    {
        while (TYPE_EFFECT_ATK_TYPE(i) != TYPE_ENDTABLE)
        {
            if (TYPE_EFFECT_ATK_TYPE(i) == TYPE_FORESIGHT)
            {
                i += 3;
                continue;
            }
            if (TYPE_EFFECT_ATK_TYPE(i) == moveType)
            {
                // check type1
                if (TYPE_EFFECT_DEF_TYPE(i) == type1)
                    ModulateDmgByType2(TYPE_EFFECT_MULTIPLIER(i), move, &flags);
                // check type2
                if (TYPE_EFFECT_DEF_TYPE(i) == type2 && type1 != type2)
                    ModulateDmgByType2(TYPE_EFFECT_MULTIPLIER(i), move, &flags);
            }
            i += 3;
        }
    }
    if (targetAbility == ABILITY_WONDER_GUARD
     && (!(flags & MOVE_RESULT_SUPER_EFFECTIVE) || ((flags & (MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE)) == (MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE)))
     && gBattleMoves[move].power)
        flags |= MOVE_RESULT_DOESNT_AFFECT_FOE;
    return flags;
}

// Multiplies the damage by a random factor between 85% to 100% inclusive
static inline void ApplyRandomDmgMultiplier(void)
{
    u16 rand = Random();
    u16 randPercent = 100 - (rand % 16);

    if (gBattleMoveDamage != 0)
    {
        gBattleMoveDamage *= randPercent;
        gBattleMoveDamage /= 100;
        if (gBattleMoveDamage == 0)
            gBattleMoveDamage = 1;
    }
}

static void UNUSED Unused_ApplyRandomDmgMultiplier(void)
{
    ApplyRandomDmgMultiplier();
}

static void Cmd_adjustnormaldamage(void)
{
    u8 holdEffect, param;

    ApplyRandomDmgMultiplier();

    if (gBattleMons[gBattlerTarget].item == ITEM_ENIGMA_BERRY)
    {
        holdEffect = gEnigmaBerries[gBattlerTarget].holdEffect;
        param = gEnigmaBerries[gBattlerTarget].holdEffectParam;
    }
    else
    {
        holdEffect = ItemId_GetHoldEffect(gBattleMons[gBattlerTarget].item);
        param = ItemId_GetHoldEffectParam(gBattleMons[gBattlerTarget].item);
    }

    gPotentialItemEffectBattler = gBattlerTarget;

    if (holdEffect == HOLD_EFFECT_FOCUS_BAND && (Random() % 100) < param)
    {
        RecordItemEffectBattle(gBattlerTarget, holdEffect);
        gSpecialStatuses[gBattlerTarget].focusBanded = 1;
    }
    if (!(gBattleMons[gBattlerTarget].status2 & STATUS2_SUBSTITUTE)
     && (gBattleMoves[gCurrentMove].effect == EFFECT_FALSE_SWIPE || gProtectStructs[gBattlerTarget].endured || gSpecialStatuses[gBattlerTarget].focusBanded)
     && gBattleMons[gBattlerTarget].hp <= gBattleMoveDamage)
    {
        gBattleMoveDamage = gBattleMons[gBattlerTarget].hp - 1;
        if (gProtectStructs[gBattlerTarget].endured)
        {
            gMoveResultFlags |= MOVE_RESULT_FOE_ENDURED;
        }
        else if (gSpecialStatuses[gBattlerTarget].focusBanded)
        {
            gMoveResultFlags |= MOVE_RESULT_FOE_HUNG_ON;
            gLastUsedItem = gBattleMons[gBattlerTarget].item;
        }
    }
    gBattlescriptCurrInstr++;
}

// The same as adjustnormaldamage except it doesn't check for false swipe move effect.
static void Cmd_adjustnormaldamage2(void)
{
    u8 holdEffect, param;

    ApplyRandomDmgMultiplier();

    if (gBattleMons[gBattlerTarget].item == ITEM_ENIGMA_BERRY)
    {
        holdEffect = gEnigmaBerries[gBattlerTarget].holdEffect;
        param = gEnigmaBerries[gBattlerTarget].holdEffectParam;
    }
    else
    {
        holdEffect = ItemId_GetHoldEffect(gBattleMons[gBattlerTarget].item);
        param = ItemId_GetHoldEffectParam(gBattleMons[gBattlerTarget].item);
    }

    gPotentialItemEffectBattler = gBattlerTarget;

    if (holdEffect == HOLD_EFFECT_FOCUS_BAND && (Random() % 100) < param)
    {
        RecordItemEffectBattle(gBattlerTarget, holdEffect);
        gSpecialStatuses[gBattlerTarget].focusBanded = 1;
    }
    if (!(gBattleMons[gBattlerTarget].status2 & STATUS2_SUBSTITUTE)
     && (gProtectStructs[gBattlerTarget].endured || gSpecialStatuses[gBattlerTarget].focusBanded)
     && gBattleMons[gBattlerTarget].hp <= gBattleMoveDamage)
    {
        gBattleMoveDamage = gBattleMons[gBattlerTarget].hp - 1;
        if (gProtectStructs[gBattlerTarget].endured)
        {
            gMoveResultFlags |= MOVE_RESULT_FOE_ENDURED;
        }
        else if (gSpecialStatuses[gBattlerTarget].focusBanded)
        {
            gMoveResultFlags |= MOVE_RESULT_FOE_HUNG_ON;
            gLastUsedItem = gBattleMons[gBattlerTarget].item;
        }
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_attackanimation(void)
{
    if (gBattleControllerExecFlags)
        return;

    if ((gHitMarker & HITMARKER_NO_ANIMATIONS) && (gCurrentMove != MOVE_TRANSFORM && gCurrentMove != MOVE_SUBSTITUTE))
    {
        BattleScriptPush(gBattlescriptCurrInstr + 1);
        gBattlescriptCurrInstr = BattleScript_Pausex20;
        gBattleScripting.animTurn++;
        gBattleScripting.animTargetsHit++;
    }
    else
    {
        if ((gBattleMoves[gCurrentMove].target & MOVE_TARGET_BOTH
            || gBattleMoves[gCurrentMove].target & MOVE_TARGET_FOES_AND_ALLY
            || gBattleMoves[gCurrentMove].target & MOVE_TARGET_DEPENDS)
            && gBattleScripting.animTargetsHit)
        {
            gBattlescriptCurrInstr++;
            return;
        }
        if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
        {
            u8 multihit;

            gActiveBattler = gBattlerAttacker;

            if (gBattleMons[gBattlerTarget].status2 & STATUS2_SUBSTITUTE)
            {
                multihit = gMultiHitCounter;
            }
            else if (gMultiHitCounter != 0 && gMultiHitCounter != 1)
            {
                if (gBattleMons[gBattlerTarget].hp <= gBattleMoveDamage)
                    multihit = 1;
                else
                    multihit = gMultiHitCounter;
            }
            else
            {
                multihit = gMultiHitCounter;
            }

            BtlController_EmitMoveAnimation(BUFFER_A, gCurrentMove, gBattleScripting.animTurn, gBattleMovePower, gBattleMoveDamage, gBattleMons[gBattlerAttacker].friendship, &gDisableStructs[gBattlerAttacker], multihit);
            gBattleScripting.animTurn++;
            gBattleScripting.animTargetsHit++;
            MarkBattlerForControllerExec(gBattlerAttacker);
            gBattlescriptCurrInstr++;
        }
        else
        {
            BattleScriptPush(gBattlescriptCurrInstr + 1);
            gBattlescriptCurrInstr = BattleScript_Pausex20;
        }
    }
}

static void Cmd_waitanimation(void)
{
    if (gBattleControllerExecFlags == 0)
        gBattlescriptCurrInstr++;
}

static void Cmd_healthbarupdate(void)
{
    if (gBattleControllerExecFlags)
        return;

    if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
    {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

        if (gBattleMons[gActiveBattler].status2 & STATUS2_SUBSTITUTE && gDisableStructs[gActiveBattler].substituteHP && !(gHitMarker & HITMARKER_IGNORE_SUBSTITUTE))
        {
            PrepareStringBattle(STRINGID_SUBSTITUTEDAMAGED, gActiveBattler);
        }
        else
        {
            s16 healthValue;

            s32 currDmg = gBattleMoveDamage;
            s32 maxPossibleDmgValue = 10000; // not present in R/S, ensures that huge damage values don't change sign

            if (currDmg <= maxPossibleDmgValue)
                healthValue = currDmg;
            else
                healthValue = maxPossibleDmgValue;

            BtlController_EmitHealthBarUpdate(BUFFER_A, healthValue);
            MarkBattlerForControllerExec(gActiveBattler);

            if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER && gBattleMoveDamage > 0)
                gBattleResults.playerMonWasDamaged = TRUE;
        }
    }

    gBattlescriptCurrInstr += 2;
}

// Update the active battler's HP and various HP trackers (Substitute, Bide, etc.)
static void Cmd_datahpupdate(void)
{
    u32 moveType;

    if (gBattleControllerExecFlags)
        return;

    // moveType will be used later to record for Counter/Mirror Coat whether this was physical or special damage.
    // For moves with a dynamic type that have F_DYNAMIC_TYPE_IGNORE_PHYSICALITY set (in vanilla, just Hidden Power) this will ignore
    // the dynamic type and use the move's base type instead, meaning (as a Normal type) Hidden Power will only ever trigger Counter.
    // It also means that Hidden Power Fire is unable to defrost targets.
    if (gBattleStruct->dynamicMoveType == 0)
        moveType = gBattleMoves[gCurrentMove].type;
    else if (!(gBattleStruct->dynamicMoveType & F_DYNAMIC_TYPE_IGNORE_PHYSICALITY))
        moveType = gBattleStruct->dynamicMoveType & DYNAMIC_TYPE_MASK;
    else
        moveType = gBattleMoves[gCurrentMove].type;

    if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
    {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        if (gBattleMons[gActiveBattler].status2 & STATUS2_SUBSTITUTE && gDisableStructs[gActiveBattler].substituteHP && !(gHitMarker & HITMARKER_IGNORE_SUBSTITUTE))
        {
            // Target has an active Substitute, deal damage to that instead.
            if (gDisableStructs[gActiveBattler].substituteHP >= gBattleMoveDamage)
            {
                if (gSpecialStatuses[gActiveBattler].shellBellDmg == 0)
                    gSpecialStatuses[gActiveBattler].shellBellDmg = gBattleMoveDamage;
                gDisableStructs[gActiveBattler].substituteHP -= gBattleMoveDamage;
                gHpDealt = gBattleMoveDamage;
            }
            else
            {
                // Substitute has less HP than the damage dealt, set its HP to 0.
                if (gSpecialStatuses[gActiveBattler].shellBellDmg == 0)
                    gSpecialStatuses[gActiveBattler].shellBellDmg = gDisableStructs[gActiveBattler].substituteHP;
                gHpDealt = gDisableStructs[gActiveBattler].substituteHP;
                gDisableStructs[gActiveBattler].substituteHP = 0;
            }

            if (gDisableStructs[gActiveBattler].substituteHP == 0)
            {
                // Substitute fades
                gBattlescriptCurrInstr += 2;
                BattleScriptPushCursor();
                gBattlescriptCurrInstr = BattleScript_SubstituteFade;
                return;
            }
        }
        else
        {
            gHitMarker &= ~HITMARKER_IGNORE_SUBSTITUTE;
            if (gBattleMoveDamage < 0)
            {
                // Negative damage is HP gain
                gBattleMons[gActiveBattler].hp += -gBattleMoveDamage;
                if (gBattleMons[gActiveBattler].hp > gBattleMons[gActiveBattler].maxHP)
                    gBattleMons[gActiveBattler].hp = gBattleMons[gActiveBattler].maxHP;
            }
            else
            {
                if (gHitMarker & HITMARKER_IGNORE_BIDE)
                {
                    gHitMarker &= ~HITMARKER_IGNORE_BIDE;
                }
                else
                {
                    // Record damage/attacker for Bide
                    gBideDmg[gActiveBattler] += gBattleMoveDamage;
                    if (gBattlescriptCurrInstr[1] == BS_TARGET)
                        gBideTarget[gActiveBattler] = gBattlerAttacker;
                    else
                        gBideTarget[gActiveBattler] = gBattlerTarget;
                }

                // Deal damage to the battler
                if (gBattleMons[gActiveBattler].hp > gBattleMoveDamage)
                {
                    gBattleMons[gActiveBattler].hp -= gBattleMoveDamage;
                    gHpDealt = gBattleMoveDamage;
                }
                else
                {
                    gHpDealt = gBattleMons[gActiveBattler].hp;
                    gBattleMons[gActiveBattler].hp = 0;
                }

                // Record damage for Shell Bell
                if (gSpecialStatuses[gActiveBattler].shellBellDmg == 0 && !(gHitMarker & HITMARKER_PASSIVE_DAMAGE))
                    gSpecialStatuses[gActiveBattler].shellBellDmg = gHpDealt;

                // Note: While physicalDmg/specialDmg below are only distinguished between for Counter/Mirror Coat, they are
                //       used in combination as general damage trackers for other purposes. specialDmg is additionally used
                //       to help determine if a fire move should defrost the target.
                if (IS_TYPE_PHYSICAL(moveType) && !(gHitMarker & HITMARKER_PASSIVE_DAMAGE) && gCurrentMove != MOVE_PAIN_SPLIT)
                {
                    // Record physical damage/attacker for Counter
                    gProtectStructs[gActiveBattler].physicalDmg = gHpDealt;
                    gSpecialStatuses[gActiveBattler].physicalDmg = gHpDealt;
                    if (gBattlescriptCurrInstr[1] == BS_TARGET)
                    {
                        gProtectStructs[gActiveBattler].physicalBattlerId = gBattlerAttacker;
                        gSpecialStatuses[gActiveBattler].physicalBattlerId = gBattlerAttacker;
                    }
                    else
                    {
                        gProtectStructs[gActiveBattler].physicalBattlerId = gBattlerTarget;
                        gSpecialStatuses[gActiveBattler].physicalBattlerId = gBattlerTarget;
                    }
                }
                else if (!IS_TYPE_PHYSICAL(moveType) && !(gHitMarker & HITMARKER_PASSIVE_DAMAGE))
                {
                    // Record special damage/attacker for Mirror Coat
                    gProtectStructs[gActiveBattler].specialDmg = gHpDealt;
                    gSpecialStatuses[gActiveBattler].specialDmg = gHpDealt;
                    if (gBattlescriptCurrInstr[1] == BS_TARGET)
                    {
                        gProtectStructs[gActiveBattler].specialBattlerId = gBattlerAttacker;
                        gSpecialStatuses[gActiveBattler].specialBattlerId = gBattlerAttacker;
                    }
                    else
                    {
                        gProtectStructs[gActiveBattler].specialBattlerId = gBattlerTarget;
                        gSpecialStatuses[gActiveBattler].specialBattlerId = gBattlerTarget;
                    }
                }
            }
            gHitMarker &= ~HITMARKER_PASSIVE_DAMAGE;

            // Send updated HP
            BtlController_EmitSetMonData(BUFFER_A, REQUEST_HP_BATTLE, 0, sizeof(gBattleMons[gActiveBattler].hp), &gBattleMons[gActiveBattler].hp);
            MarkBattlerForControllerExec(gActiveBattler);
        }
    }
    else
    {
        // MOVE_RESULT_NO_EFFECT was set
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        if (gSpecialStatuses[gActiveBattler].shellBellDmg == 0)
            gSpecialStatuses[gActiveBattler].shellBellDmg = IGNORE_SHELL_BELL;
    }
    gBattlescriptCurrInstr += 2;
}

static void Cmd_critmessage(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        if (gCritMultiplier == 2 && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
        {
            PrepareStringBattle(STRINGID_CRITICALHIT, gBattlerAttacker);
            gBattleCommunication[MSG_DISPLAY] = 1;
        }
        gBattlescriptCurrInstr++;
    }
}

static void Cmd_effectivenesssound(void)
{
    if (gBattleControllerExecFlags)
        return;

    gActiveBattler = gBattlerTarget;
    if (!(gMoveResultFlags & MOVE_RESULT_MISSED))
    {
        switch (gMoveResultFlags & (u8)(~MOVE_RESULT_MISSED))
        {
        case MOVE_RESULT_SUPER_EFFECTIVE:
            BtlController_EmitPlaySE(BUFFER_A, SE_SUPER_EFFECTIVE);
            MarkBattlerForControllerExec(gActiveBattler);
            break;
        case MOVE_RESULT_NOT_VERY_EFFECTIVE:
            BtlController_EmitPlaySE(BUFFER_A, SE_NOT_EFFECTIVE);
            MarkBattlerForControllerExec(gActiveBattler);
            break;
        case MOVE_RESULT_DOESNT_AFFECT_FOE:
        case MOVE_RESULT_FAILED:
            // no sound
            break;
        case MOVE_RESULT_FOE_ENDURED:
        case MOVE_RESULT_ONE_HIT_KO:
        case MOVE_RESULT_FOE_HUNG_ON:
        default:
            if (gMoveResultFlags & MOVE_RESULT_SUPER_EFFECTIVE)
            {
                BtlController_EmitPlaySE(BUFFER_A, SE_SUPER_EFFECTIVE);
                MarkBattlerForControllerExec(gActiveBattler);
            }
            else if (gMoveResultFlags & MOVE_RESULT_NOT_VERY_EFFECTIVE)
            {
                BtlController_EmitPlaySE(BUFFER_A, SE_NOT_EFFECTIVE);
                MarkBattlerForControllerExec(gActiveBattler);
            }
            else if (!(gMoveResultFlags & (MOVE_RESULT_DOESNT_AFFECT_FOE | MOVE_RESULT_FAILED)))
            {
                BtlController_EmitPlaySE(BUFFER_A, SE_EFFECTIVE);
                MarkBattlerForControllerExec(gActiveBattler);
            }
            break;
        }
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_resultmessage(void)
{
    u32 stringId = 0;

    if (gBattleControllerExecFlags)
        return;

    if (gMoveResultFlags & MOVE_RESULT_MISSED && (!(gMoveResultFlags & MOVE_RESULT_DOESNT_AFFECT_FOE) || gBattleCommunication[MISS_TYPE] > B_MSG_AVOIDED_ATK))
    {
        stringId = gMissStringIds[gBattleCommunication[MISS_TYPE]];
        gBattleCommunication[MSG_DISPLAY] = 1;
    }
    else
    {
        gBattleCommunication[MSG_DISPLAY] = 1;
        switch (gMoveResultFlags & (u8)(~MOVE_RESULT_MISSED))
        {
        case MOVE_RESULT_SUPER_EFFECTIVE:
            stringId = STRINGID_SUPEREFFECTIVE;
            break;
        case MOVE_RESULT_NOT_VERY_EFFECTIVE:
            stringId = STRINGID_NOTVERYEFFECTIVE;
            break;
        case MOVE_RESULT_ONE_HIT_KO:
            stringId = STRINGID_ONEHITKO;
            break;
        case MOVE_RESULT_FOE_ENDURED:
            stringId = STRINGID_PKMNENDUREDHIT;
            break;
        case MOVE_RESULT_FAILED:
            stringId = STRINGID_BUTITFAILED;
            break;
        case MOVE_RESULT_DOESNT_AFFECT_FOE:
            stringId = STRINGID_ITDOESNTAFFECT;
            break;
        case MOVE_RESULT_FOE_HUNG_ON:
            gLastUsedItem = gBattleMons[gBattlerTarget].item;
            gPotentialItemEffectBattler = gBattlerTarget;
            gMoveResultFlags &= ~(MOVE_RESULT_FOE_ENDURED | MOVE_RESULT_FOE_HUNG_ON);
            BattleScriptPushCursor();
            gBattlescriptCurrInstr = BattleScript_FocusBandActivates;
            return;
        default:
            if (gMoveResultFlags & MOVE_RESULT_DOESNT_AFFECT_FOE)
            {
                stringId = STRINGID_ITDOESNTAFFECT;
            }
            else if (gMoveResultFlags & MOVE_RESULT_ONE_HIT_KO)
            {
                gMoveResultFlags &= ~MOVE_RESULT_ONE_HIT_KO;
                gMoveResultFlags &= ~MOVE_RESULT_SUPER_EFFECTIVE;
                gMoveResultFlags &= ~MOVE_RESULT_NOT_VERY_EFFECTIVE;
                BattleScriptPushCursor();
                gBattlescriptCurrInstr = BattleScript_OneHitKOMsg;
                return;
            }
            else if (gMoveResultFlags & MOVE_RESULT_FOE_ENDURED)
            {
                gMoveResultFlags &= ~(MOVE_RESULT_FOE_ENDURED | MOVE_RESULT_FOE_HUNG_ON);
                BattleScriptPushCursor();
                gBattlescriptCurrInstr = BattleScript_EnduredMsg;
                return;
            }
            else if (gMoveResultFlags & MOVE_RESULT_FOE_HUNG_ON)
            {
                gLastUsedItem = gBattleMons[gBattlerTarget].item;
                gPotentialItemEffectBattler = gBattlerTarget;
                gMoveResultFlags &= ~(MOVE_RESULT_FOE_ENDURED | MOVE_RESULT_FOE_HUNG_ON);
                BattleScriptPushCursor();
                gBattlescriptCurrInstr = BattleScript_FocusBandActivates;
                return;
            }
            else if (gMoveResultFlags & MOVE_RESULT_FAILED)
            {
                stringId = STRINGID_BUTITFAILED;
            }
            else
            {
                gBattleCommunication[MSG_DISPLAY] = 0;
            }
        }
    }

    if (stringId)
        PrepareStringBattle(stringId, gBattlerAttacker);

    gBattlescriptCurrInstr++;
}

static void Cmd_printstring(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        u16 var = T2_READ_16(gBattlescriptCurrInstr + 1);
        PrepareStringBattle(var, gBattlerAttacker);
        gBattlescriptCurrInstr += 3;
        gBattleCommunication[MSG_DISPLAY] = 1;
    }
}

static void Cmd_printselectionstring(void)
{
    gActiveBattler = gBattlerAttacker;

    BtlController_EmitPrintSelectionString(BUFFER_A, T2_READ_16(gBattlescriptCurrInstr + 1));
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 3;
    gBattleCommunication[MSG_DISPLAY] = 1;
}

static void Cmd_waitmessage(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        if (!gBattleCommunication[MSG_DISPLAY])
        {
            gBattlescriptCurrInstr += 3;
        }
        else
        {
            u16 toWait = T2_READ_16(gBattlescriptCurrInstr + 1);
            if (++gPauseCounterBattle >= toWait)
            {
                gPauseCounterBattle = 0;
                gBattlescriptCurrInstr += 3;
                gBattleCommunication[MSG_DISPLAY] = 0;
            }
        }
    }
}

static void Cmd_printfromtable(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        const u16 *ptr = (const u16 *) T1_READ_PTR(gBattlescriptCurrInstr + 1);
        ptr += gBattleCommunication[MULTISTRING_CHOOSER];

        PrepareStringBattle(*ptr, gBattlerAttacker);

        gBattlescriptCurrInstr += 5;
        gBattleCommunication[MSG_DISPLAY] = 1;
    }
}

static void Cmd_printselectionstringfromtable(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        const u16 *ptr = (const u16 *) T1_READ_PTR(gBattlescriptCurrInstr + 1);
        ptr += gBattleCommunication[MULTISTRING_CHOOSER];

        gActiveBattler = gBattlerAttacker;
        BtlController_EmitPrintSelectionString(BUFFER_A, *ptr);
        MarkBattlerForControllerExec(gActiveBattler);

        gBattlescriptCurrInstr += 5;
        gBattleCommunication[MSG_DISPLAY] = 1;
    }
}

u8 GetBattlerTurnOrderNum(u8 battlerId)
{
    s32 i;
    for (i = 0; i < gBattlersCount; i++)
    {
        if (gBattlerByTurnOrder[i] == battlerId)
            break;
    }
    return i;
}

#define INCREMENT_RESET_RETURN                  \
{                                               \
    gBattlescriptCurrInstr++;                   \
    gBattleCommunication[MOVE_EFFECT_BYTE] = 0; \
    return;                                     \
}

#define RESET_RETURN                            \
{                                               \
    gBattleCommunication[MOVE_EFFECT_BYTE] = 0; \
    return;                                     \
}

void SetMoveEffect(bool8 primary, u8 certain)
{
    bool32 statusChanged = FALSE;
    u8 affectsUser = 0; // 0x40 otherwise
    bool32 noSunCanFreeze = TRUE;

    if (gBattleCommunication[MOVE_EFFECT_BYTE] & MOVE_EFFECT_AFFECTS_USER)
    {
        gEffectBattler = gBattlerAttacker; // battlerId that effects get applied on
        gBattleCommunication[MOVE_EFFECT_BYTE] &= ~MOVE_EFFECT_AFFECTS_USER;
        affectsUser = MOVE_EFFECT_AFFECTS_USER;
        gBattleScripting.battler = gBattlerTarget; // theoretically the attacker
    }
    else
    {
        gEffectBattler = gBattlerTarget;
        gBattleScripting.battler = gBattlerAttacker;
    }

    if (gBattleMons[gEffectBattler].ability == ABILITY_SHIELD_DUST && !(gHitMarker & HITMARKER_STATUS_ABILITY_EFFECT)
        && !primary && gBattleCommunication[MOVE_EFFECT_BYTE] <= 9)
        INCREMENT_RESET_RETURN

    if (gSideStatuses[GET_BATTLER_SIDE(gEffectBattler)] & SIDE_STATUS_SAFEGUARD && !(gHitMarker & HITMARKER_STATUS_ABILITY_EFFECT)
        && !primary && gBattleCommunication[MOVE_EFFECT_BYTE] <= 7)
        INCREMENT_RESET_RETURN

    if (gBattleMons[gEffectBattler].hp == 0
        && gBattleCommunication[MOVE_EFFECT_BYTE] != MOVE_EFFECT_PAYDAY
        && gBattleCommunication[MOVE_EFFECT_BYTE] != MOVE_EFFECT_STEAL_ITEM)
        INCREMENT_RESET_RETURN

    if (gBattleMons[gEffectBattler].status2 & STATUS2_SUBSTITUTE && affectsUser != MOVE_EFFECT_AFFECTS_USER)
        INCREMENT_RESET_RETURN

    if (gBattleCommunication[MOVE_EFFECT_BYTE] <= PRIMARY_STATUS_MOVE_EFFECT)
    {
        switch (sStatusFlagsForMoveEffects[gBattleCommunication[MOVE_EFFECT_BYTE]])
        {
        case STATUS1_SLEEP:
            // check active uproar
            if (gBattleMons[gEffectBattler].ability != ABILITY_SOUNDPROOF)
            {
                for (gActiveBattler = 0;
                    gActiveBattler < gBattlersCount && !(gBattleMons[gActiveBattler].status2 & STATUS2_UPROAR);
                    gActiveBattler++)
                {}
            }
            else
            {
                gActiveBattler = gBattlersCount;
            }

            if (gBattleMons[gEffectBattler].status1)
                break;
            if (gActiveBattler != gBattlersCount)
                break;
            if (gBattleMons[gEffectBattler].ability == ABILITY_VITAL_SPIRIT)
                break;
            if (gBattleMons[gEffectBattler].ability == ABILITY_INSOMNIA)
                break;

            CancelMultiTurnMoves(gEffectBattler);
            statusChanged = TRUE;
            break;
        case STATUS1_POISON:
            if (gBattleMons[gEffectBattler].ability == ABILITY_IMMUNITY
                && (primary == TRUE || certain == MOVE_EFFECT_CERTAIN))
            {
                gLastUsedAbility = ABILITY_IMMUNITY;
                RecordAbilityBattle(gEffectBattler, ABILITY_IMMUNITY);

                BattleScriptPush(gBattlescriptCurrInstr + 1);
                gBattlescriptCurrInstr = BattleScript_PSNPrevention;

                if (gHitMarker & HITMARKER_STATUS_ABILITY_EFFECT)
                {
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_ABILITY_STATUS;
                    gHitMarker &= ~HITMARKER_STATUS_ABILITY_EFFECT;
                }
                else
                {
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_MOVE_STATUS;
                }
                RESET_RETURN
            }
            if ((IS_BATTLER_OF_TYPE(gEffectBattler, TYPE_POISON) || IS_BATTLER_OF_TYPE(gEffectBattler, TYPE_STEEL))
                && (gHitMarker & HITMARKER_STATUS_ABILITY_EFFECT)
                && (primary == TRUE || certain == MOVE_EFFECT_CERTAIN))
            {
                BattleScriptPush(gBattlescriptCurrInstr + 1);
                gBattlescriptCurrInstr = BattleScript_PSNPrevention;

                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STATUS_HAD_NO_EFFECT;
                RESET_RETURN
            }
            if (IS_BATTLER_OF_TYPE(gEffectBattler, TYPE_POISON))
                break;
            if (IS_BATTLER_OF_TYPE(gEffectBattler, TYPE_STEEL))
                break;
            if (gBattleMons[gEffectBattler].status1)
                break;
            if (gBattleMons[gEffectBattler].ability == ABILITY_IMMUNITY)
                break;

            statusChanged = TRUE;
            break;
        case STATUS1_BURN:
            if (gBattleMons[gEffectBattler].ability == ABILITY_WATER_VEIL
                && (primary == TRUE || certain == MOVE_EFFECT_CERTAIN))
            {
                gLastUsedAbility = ABILITY_WATER_VEIL;
                RecordAbilityBattle(gEffectBattler, ABILITY_WATER_VEIL);

                BattleScriptPush(gBattlescriptCurrInstr + 1);
                gBattlescriptCurrInstr = BattleScript_BRNPrevention;
                if (gHitMarker & HITMARKER_STATUS_ABILITY_EFFECT)
                {
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_ABILITY_STATUS;
                    gHitMarker &= ~HITMARKER_STATUS_ABILITY_EFFECT;
                }
                else
                {
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_MOVE_STATUS;
                }
                RESET_RETURN
            }
            if (IS_BATTLER_OF_TYPE(gEffectBattler, TYPE_FIRE)
                && (gHitMarker & HITMARKER_STATUS_ABILITY_EFFECT)
                && (primary == TRUE || certain == MOVE_EFFECT_CERTAIN))
            {
                BattleScriptPush(gBattlescriptCurrInstr + 1);
                gBattlescriptCurrInstr = BattleScript_BRNPrevention;

                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STATUS_HAD_NO_EFFECT;
                RESET_RETURN
            }
            if (IS_BATTLER_OF_TYPE(gEffectBattler, TYPE_FIRE))
                break;
            if (gBattleMons[gEffectBattler].ability == ABILITY_WATER_VEIL)
                break;
            if (gBattleMons[gEffectBattler].status1)
                break;

            statusChanged = TRUE;
            break;
        case STATUS1_FREEZE:
            if (WEATHER_HAS_EFFECT && gBattleWeather & B_WEATHER_SUN)
                noSunCanFreeze = FALSE;
            if (IS_BATTLER_OF_TYPE(gEffectBattler, TYPE_ICE))
                break;
            if (gBattleMons[gEffectBattler].status1)
                break;
            if (noSunCanFreeze == FALSE)
                break;
            if (gBattleMons[gEffectBattler].ability == ABILITY_MAGMA_ARMOR)
                break;

            CancelMultiTurnMoves(gEffectBattler);
            statusChanged = TRUE;
            break;
        case STATUS1_PARALYSIS:
            if (gBattleMons[gEffectBattler].ability == ABILITY_LIMBER)
            {
                if (primary == TRUE || certain == MOVE_EFFECT_CERTAIN)
                {
                    gLastUsedAbility = ABILITY_LIMBER;
                    RecordAbilityBattle(gEffectBattler, ABILITY_LIMBER);

                    BattleScriptPush(gBattlescriptCurrInstr + 1);
                    gBattlescriptCurrInstr = BattleScript_PRLZPrevention;

                    if (gHitMarker & HITMARKER_STATUS_ABILITY_EFFECT)
                    {
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_ABILITY_STATUS;
                        gHitMarker &= ~HITMARKER_STATUS_ABILITY_EFFECT;
                    }
                    else
                    {
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_MOVE_STATUS;
                    }
                    RESET_RETURN
                }
                else
                {
                    break;
                }
            }
            if (gBattleMons[gEffectBattler].status1)
                break;

            statusChanged = TRUE;
            break;
        case STATUS1_TOXIC_POISON:
            if (gBattleMons[gEffectBattler].ability == ABILITY_IMMUNITY && (primary == TRUE || certain == MOVE_EFFECT_CERTAIN))
            {
                gLastUsedAbility = ABILITY_IMMUNITY;
                RecordAbilityBattle(gEffectBattler, ABILITY_IMMUNITY);

                BattleScriptPush(gBattlescriptCurrInstr + 1);
                gBattlescriptCurrInstr = BattleScript_PSNPrevention;

                if (gHitMarker & HITMARKER_STATUS_ABILITY_EFFECT)
                {
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_ABILITY_STATUS;
                    gHitMarker &= ~HITMARKER_STATUS_ABILITY_EFFECT;
                }
                else
                {
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_MOVE_STATUS;
                }
                RESET_RETURN
            }
            if ((IS_BATTLER_OF_TYPE(gEffectBattler, TYPE_POISON) || IS_BATTLER_OF_TYPE(gEffectBattler, TYPE_STEEL))
                && (gHitMarker & HITMARKER_STATUS_ABILITY_EFFECT)
                && (primary == TRUE || certain == MOVE_EFFECT_CERTAIN))
            {
                BattleScriptPush(gBattlescriptCurrInstr + 1);
                gBattlescriptCurrInstr = BattleScript_PSNPrevention;

                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STATUS_HAD_NO_EFFECT;
                RESET_RETURN
            }
            if (gBattleMons[gEffectBattler].status1)
                break;
            if (!IS_BATTLER_OF_TYPE(gEffectBattler, TYPE_POISON) && !IS_BATTLER_OF_TYPE(gEffectBattler, TYPE_STEEL))
            {
                if (gBattleMons[gEffectBattler].ability == ABILITY_IMMUNITY)
                    break;

                // It's redundant, because at this point we know the status1 value is 0.
                gBattleMons[gEffectBattler].status1 &= ~STATUS1_TOXIC_POISON;
                gBattleMons[gEffectBattler].status1 &= ~STATUS1_POISON;
                statusChanged = TRUE;
                break;
            }
            else
            {
                gMoveResultFlags |= MOVE_RESULT_DOESNT_AFFECT_FOE;
            }
            break;
        }
        if (statusChanged == TRUE)
        {
            BattleScriptPush(gBattlescriptCurrInstr + 1);

            if (sStatusFlagsForMoveEffects[gBattleCommunication[MOVE_EFFECT_BYTE]] == STATUS1_SLEEP)
                gBattleMons[gEffectBattler].status1 |= STATUS1_SLEEP_TURN((Random() & 3) + 2); // 2-5 turns
            else
                gBattleMons[gEffectBattler].status1 |= sStatusFlagsForMoveEffects[gBattleCommunication[MOVE_EFFECT_BYTE]];

            gBattlescriptCurrInstr = sMoveEffectBS_Ptrs[gBattleCommunication[MOVE_EFFECT_BYTE]];

            gActiveBattler = gEffectBattler;
            BtlController_EmitSetMonData(BUFFER_A, REQUEST_STATUS_BATTLE, 0, sizeof(gBattleMons[gEffectBattler].status1), &gBattleMons[gEffectBattler].status1);
            MarkBattlerForControllerExec(gActiveBattler);

            if (gHitMarker & HITMARKER_STATUS_ABILITY_EFFECT)
            {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STATUSED_BY_ABILITY;
                gHitMarker &= ~HITMARKER_STATUS_ABILITY_EFFECT;
            }
            else
            {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STATUSED;
            }

            // for synchronize

            if (gBattleCommunication[MOVE_EFFECT_BYTE] == MOVE_EFFECT_POISON
             || gBattleCommunication[MOVE_EFFECT_BYTE] == MOVE_EFFECT_TOXIC
             || gBattleCommunication[MOVE_EFFECT_BYTE] == MOVE_EFFECT_PARALYSIS
             || gBattleCommunication[MOVE_EFFECT_BYTE] == MOVE_EFFECT_BURN)
             {
                u8 *synchronizeEffect = &gBattleStruct->synchronizeMoveEffect;
                *synchronizeEffect = gBattleCommunication[MOVE_EFFECT_BYTE];
                gHitMarker |= HITMARKER_SYNCHRONISE_EFFECT;
             }
            return;
        }
        else if (statusChanged == FALSE)
        {
            gBattleCommunication[MOVE_EFFECT_BYTE] = 0;
            gBattlescriptCurrInstr++;
            return;
        }
        return;
    }
    else
    {
        if (gBattleMons[gEffectBattler].status2 & sStatusFlagsForMoveEffects[gBattleCommunication[MOVE_EFFECT_BYTE]])
        {
            gBattlescriptCurrInstr++;
        }
        else
        {
            u8 side;
            switch (gBattleCommunication[MOVE_EFFECT_BYTE])
            {
            case MOVE_EFFECT_CONFUSION:
                if (gBattleMons[gEffectBattler].ability == ABILITY_OWN_TEMPO
                    || gBattleMons[gEffectBattler].status2 & STATUS2_CONFUSION)
                {
                    gBattlescriptCurrInstr++;
                }
                else
                {
                    gBattleMons[gEffectBattler].status2 |= STATUS2_CONFUSION_TURN(((Random()) % 4) + 2); // 2-5 turns

                    BattleScriptPush(gBattlescriptCurrInstr + 1);
                    gBattlescriptCurrInstr = sMoveEffectBS_Ptrs[gBattleCommunication[MOVE_EFFECT_BYTE]];
                }
                break;
            case MOVE_EFFECT_FLINCH:
                if (gBattleMons[gEffectBattler].ability == ABILITY_INNER_FOCUS)
                {
                    if (primary == TRUE || certain == MOVE_EFFECT_CERTAIN)
                    {
                        gLastUsedAbility = ABILITY_INNER_FOCUS;
                        RecordAbilityBattle(gEffectBattler, ABILITY_INNER_FOCUS);
                        gBattlescriptCurrInstr = BattleScript_FlinchPrevention;
                    }
                    else
                    {
                        gBattlescriptCurrInstr++;
                    }
                }
                else
                {
                    if (GetBattlerTurnOrderNum(gEffectBattler) > gCurrentTurnActionNumber)
                        gBattleMons[gEffectBattler].status2 |= sStatusFlagsForMoveEffects[gBattleCommunication[MOVE_EFFECT_BYTE]];
                    gBattlescriptCurrInstr++;
                }
                break;
            case MOVE_EFFECT_UPROAR:
                if (!(gBattleMons[gEffectBattler].status2 & STATUS2_UPROAR))
                {
                    gBattleMons[gEffectBattler].status2 |= STATUS2_MULTIPLETURNS;
                    gLockedMoves[gEffectBattler] = gCurrentMove;
                    gBattleMons[gEffectBattler].status2 |= STATUS2_UPROAR_TURN((Random() & 3) + 2); // 2-5 turns

                    BattleScriptPush(gBattlescriptCurrInstr + 1);
                    gBattlescriptCurrInstr = sMoveEffectBS_Ptrs[gBattleCommunication[MOVE_EFFECT_BYTE]];
                }
                else
                {
                    gBattlescriptCurrInstr++;
                }
                break;
            case MOVE_EFFECT_PAYDAY:
                if (GET_BATTLER_SIDE(gBattlerAttacker) == B_SIDE_PLAYER)
                {
                    u16 payday = gPaydayMoney;
                    gPaydayMoney += (gBattleMons[gBattlerAttacker].level * 5);
                    if (payday > gPaydayMoney)
                        gPaydayMoney = 0xFFFF;
                }
                BattleScriptPush(gBattlescriptCurrInstr + 1);
                gBattlescriptCurrInstr = sMoveEffectBS_Ptrs[gBattleCommunication[MOVE_EFFECT_BYTE]];
                break;
            case MOVE_EFFECT_TRI_ATTACK:
                if (gBattleMons[gEffectBattler].status1)
                {
                    gBattlescriptCurrInstr++;
                }
                else
                {
                    gBattleCommunication[MOVE_EFFECT_BYTE] = Random() % 3 + 3;
                    SetMoveEffect(FALSE, 0);
                }
                break;
            case MOVE_EFFECT_CHARGING:
                gBattleMons[gEffectBattler].status2 |= STATUS2_MULTIPLETURNS;
                gLockedMoves[gEffectBattler] = gCurrentMove;
                gProtectStructs[gEffectBattler].chargingTurn = 1;
                gBattlescriptCurrInstr++;
                break;
            case MOVE_EFFECT_WRAP:
                if (gBattleMons[gEffectBattler].status2 & STATUS2_WRAPPED)
                {
                    gBattlescriptCurrInstr++;
                }
                else
                {
                    gBattleMons[gEffectBattler].status2 |= STATUS2_WRAPPED_TURN((Random() & 3) + 3); // 3-6 turns

                    *(gBattleStruct->wrappedMove + gEffectBattler * 2 + 0) = gCurrentMove;
                    *(gBattleStruct->wrappedMove + gEffectBattler * 2 + 1) = gCurrentMove >> 8;
                    *(gBattleStruct->wrappedBy + gEffectBattler) = gBattlerAttacker;

                    BattleScriptPush(gBattlescriptCurrInstr + 1);
                    gBattlescriptCurrInstr = sMoveEffectBS_Ptrs[gBattleCommunication[MOVE_EFFECT_BYTE]];

                    for (gBattleCommunication[MULTISTRING_CHOOSER] = 0; ; gBattleCommunication[MULTISTRING_CHOOSER]++)
                    {
                        if (gBattleCommunication[MULTISTRING_CHOOSER] >= NUM_TRAPPING_MOVES - 1)
                            break;
                        if (gTrappingMoves[gBattleCommunication[MULTISTRING_CHOOSER]] == gCurrentMove)
                            break;
                    }
                }
                break;
            case MOVE_EFFECT_RECOIL_25: // 25% recoil
                gBattleMoveDamage = (gHpDealt) / 4;
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;

                BattleScriptPush(gBattlescriptCurrInstr + 1);
                gBattlescriptCurrInstr = sMoveEffectBS_Ptrs[gBattleCommunication[MOVE_EFFECT_BYTE]];
                break;
            case MOVE_EFFECT_ATK_PLUS_1:
            case MOVE_EFFECT_DEF_PLUS_1:
            case MOVE_EFFECT_SPD_PLUS_1:
            case MOVE_EFFECT_SP_ATK_PLUS_1:
            case MOVE_EFFECT_SP_DEF_PLUS_1:
            case MOVE_EFFECT_ACC_PLUS_1:
            case MOVE_EFFECT_EVS_PLUS_1:
                if (ChangeStatBuffs(SET_STAT_BUFF_VALUE(1),
                                    gBattleCommunication[MOVE_EFFECT_BYTE] - MOVE_EFFECT_ATK_PLUS_1 + 1,
                                    affectsUser, 0))
                {
                    gBattlescriptCurrInstr++;
                }
                else
                {
                    gBattleScripting.animArg1 = gBattleCommunication[MOVE_EFFECT_BYTE] & ~(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN);
                    gBattleScripting.animArg2 = 0;
                    BattleScriptPush(gBattlescriptCurrInstr + 1);
                    gBattlescriptCurrInstr = BattleScript_StatUp;
                }
                break;
            case MOVE_EFFECT_ATK_MINUS_1:
            case MOVE_EFFECT_DEF_MINUS_1:
            case MOVE_EFFECT_SPD_MINUS_1:
            case MOVE_EFFECT_SP_ATK_MINUS_1:
            case MOVE_EFFECT_SP_DEF_MINUS_1:
            case MOVE_EFFECT_ACC_MINUS_1:
            case MOVE_EFFECT_EVS_MINUS_1:
                if (ChangeStatBuffs(SET_STAT_BUFF_VALUE(1) | STAT_BUFF_NEGATIVE,
                                    gBattleCommunication[MOVE_EFFECT_BYTE] - MOVE_EFFECT_ATK_MINUS_1 + 1,
                                    affectsUser, 0))
                {
                    gBattlescriptCurrInstr++;
                }
                else
                {
                    gBattleScripting.animArg1 = gBattleCommunication[MOVE_EFFECT_BYTE] & ~(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN);
                    gBattleScripting.animArg2 = 0;
                    BattleScriptPush(gBattlescriptCurrInstr + 1);
                    gBattlescriptCurrInstr = BattleScript_StatDown;
                }
                break;
            case MOVE_EFFECT_ATK_PLUS_2:
            case MOVE_EFFECT_DEF_PLUS_2:
            case MOVE_EFFECT_SPD_PLUS_2:
            case MOVE_EFFECT_SP_ATK_PLUS_2:
            case MOVE_EFFECT_SP_DEF_PLUS_2:
            case MOVE_EFFECT_ACC_PLUS_2:
            case MOVE_EFFECT_EVS_PLUS_2:
                if (ChangeStatBuffs(SET_STAT_BUFF_VALUE(2),
                                    gBattleCommunication[MOVE_EFFECT_BYTE] - MOVE_EFFECT_ATK_PLUS_2 + 1,
                                    affectsUser, 0))
                {
                    gBattlescriptCurrInstr++;
                }
                else
                {
                    gBattleScripting.animArg1 = gBattleCommunication[MOVE_EFFECT_BYTE] & ~(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN);
                    gBattleScripting.animArg2 = 0;
                    BattleScriptPush(gBattlescriptCurrInstr + 1);
                    gBattlescriptCurrInstr = BattleScript_StatUp;
                }
                break;
            case MOVE_EFFECT_ATK_MINUS_2:
            case MOVE_EFFECT_DEF_MINUS_2:
            case MOVE_EFFECT_SPD_MINUS_2:
            case MOVE_EFFECT_SP_ATK_MINUS_2:
            case MOVE_EFFECT_SP_DEF_MINUS_2:
            case MOVE_EFFECT_ACC_MINUS_2:
            case MOVE_EFFECT_EVS_MINUS_2:
                if (ChangeStatBuffs(SET_STAT_BUFF_VALUE(2) | STAT_BUFF_NEGATIVE,
                                    gBattleCommunication[MOVE_EFFECT_BYTE] - MOVE_EFFECT_ATK_MINUS_2 + 1,
                                    affectsUser, 0))
                {
                    gBattlescriptCurrInstr++;
                }
                else
                {
                    gBattleScripting.animArg1 = gBattleCommunication[MOVE_EFFECT_BYTE] & ~(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN);
                    gBattleScripting.animArg2 = 0;
                    BattleScriptPush(gBattlescriptCurrInstr + 1);
                    gBattlescriptCurrInstr = BattleScript_StatDown;
                }
                break;
            case MOVE_EFFECT_RECHARGE:
                gBattleMons[gEffectBattler].status2 |= STATUS2_RECHARGE;
                gDisableStructs[gEffectBattler].rechargeTimer = 2;
                gLockedMoves[gEffectBattler] = gCurrentMove;
                gBattlescriptCurrInstr++;
                break;
            case MOVE_EFFECT_RAGE:
                gBattleMons[gBattlerAttacker].status2 |= STATUS2_RAGE;
                gBattlescriptCurrInstr++;
                break;
            case MOVE_EFFECT_STEAL_ITEM:
                {
                    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_HILL)
                    {
                        gBattlescriptCurrInstr++;
                        break;
                    }

                    side = GetBattlerSide(gBattlerAttacker);
                    if (GetBattlerSide(gBattlerAttacker) == B_SIDE_OPPONENT
                        && !(gBattleTypeFlags &
                            (BATTLE_TYPE_EREADER_TRAINER
                            | BATTLE_TYPE_FRONTIER
                            | BATTLE_TYPE_LINK
                            | BATTLE_TYPE_RECORDED_LINK
                            | BATTLE_TYPE_SECRET_BASE)))
                    {
                        gBattlescriptCurrInstr++;
                    }
                    else if (!(gBattleTypeFlags &
                            (BATTLE_TYPE_EREADER_TRAINER
                            | BATTLE_TYPE_FRONTIER
                            | BATTLE_TYPE_LINK
                            | BATTLE_TYPE_RECORDED_LINK
                            | BATTLE_TYPE_SECRET_BASE))
                        && (gWishFutureKnock.knockedOffMons[side] & gBitTable[gBattlerPartyIndexes[gBattlerAttacker]]))
                    {
                        gBattlescriptCurrInstr++;
                    }
                    else if (gBattleMons[gBattlerTarget].item
                        && gBattleMons[gBattlerTarget].ability == ABILITY_STICKY_HOLD)
                    {
                        BattleScriptPushCursor();
                        gBattlescriptCurrInstr = BattleScript_NoItemSteal;

                        gLastUsedAbility = gBattleMons[gBattlerTarget].ability;
                        RecordAbilityBattle(gBattlerTarget, gLastUsedAbility);
                    }
                    else if (gBattleMons[gBattlerAttacker].item != ITEM_NONE
                        || gBattleMons[gBattlerTarget].item == ITEM_ENIGMA_BERRY
                        || IS_ITEM_MAIL(gBattleMons[gBattlerTarget].item)
                        || gBattleMons[gBattlerTarget].item == ITEM_NONE)
                    {
                        gBattlescriptCurrInstr++;
                    }
                    else
                    {
                        u16 *changedItem = &gBattleStruct->changedItems[gBattlerAttacker];
                        gLastUsedItem = *changedItem = gBattleMons[gBattlerTarget].item;
                        gBattleMons[gBattlerTarget].item = ITEM_NONE;

                        gActiveBattler = gBattlerAttacker;
                        BtlController_EmitSetMonData(BUFFER_A, REQUEST_HELDITEM_BATTLE, 0, sizeof(gLastUsedItem), &gLastUsedItem);
                        MarkBattlerForControllerExec(gBattlerAttacker);

                        gActiveBattler = gBattlerTarget;
                        BtlController_EmitSetMonData(BUFFER_A, REQUEST_HELDITEM_BATTLE, 0, sizeof(gBattleMons[gBattlerTarget].item), &gBattleMons[gBattlerTarget].item);
                        MarkBattlerForControllerExec(gBattlerTarget);

                        BattleScriptPush(gBattlescriptCurrInstr + 1);
                        gBattlescriptCurrInstr = BattleScript_ItemSteal;

                        *(u8 *)((u8 *)(&gBattleStruct->choicedMove[gBattlerTarget]) + 0) = 0;
                        *(u8 *)((u8 *)(&gBattleStruct->choicedMove[gBattlerTarget]) + 1) = 0;
                    }
                }
                break;
            case MOVE_EFFECT_PREVENT_ESCAPE:
                gBattleMons[gBattlerTarget].status2 |= STATUS2_ESCAPE_PREVENTION;
                gDisableStructs[gBattlerTarget].battlerPreventingEscape = gBattlerAttacker;
                gBattlescriptCurrInstr++;
                break;
            case MOVE_EFFECT_NIGHTMARE:
                gBattleMons[gBattlerTarget].status2 |= STATUS2_NIGHTMARE;
                gBattlescriptCurrInstr++;
                break;
            case MOVE_EFFECT_ALL_STATS_UP:
                BattleScriptPush(gBattlescriptCurrInstr + 1);
                gBattlescriptCurrInstr = BattleScript_AllStatsUp;
                break;
            case MOVE_EFFECT_RAPIDSPIN:
                BattleScriptPush(gBattlescriptCurrInstr + 1);
                gBattlescriptCurrInstr = BattleScript_RapidSpinAway;
                break;
            case MOVE_EFFECT_REMOVE_PARALYSIS: // Smelling salts
                if (!(gBattleMons[gBattlerTarget].status1 & STATUS1_PARALYSIS))
                {
                    gBattlescriptCurrInstr++;
                }
                else
                {
                    gBattleMons[gBattlerTarget].status1 &= ~STATUS1_PARALYSIS;

                    gActiveBattler = gBattlerTarget;
                    BtlController_EmitSetMonData(BUFFER_A, REQUEST_STATUS_BATTLE, 0, sizeof(gBattleMons[gActiveBattler].status1), &gBattleMons[gActiveBattler].status1);
                    MarkBattlerForControllerExec(gActiveBattler);

                    BattleScriptPush(gBattlescriptCurrInstr + 1);
                    gBattlescriptCurrInstr = BattleScript_TargetPRLZHeal;
                }
                break;
            case MOVE_EFFECT_ATK_DEF_DOWN: // SuperPower
                BattleScriptPush(gBattlescriptCurrInstr + 1);
                gBattlescriptCurrInstr = BattleScript_AtkDefDown;
                break;
            case MOVE_EFFECT_RECOIL_33: // Double Edge
                gBattleMoveDamage = gHpDealt / 3;
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;

                BattleScriptPush(gBattlescriptCurrInstr + 1);
                gBattlescriptCurrInstr = sMoveEffectBS_Ptrs[gBattleCommunication[MOVE_EFFECT_BYTE]];
                break;
            case MOVE_EFFECT_THRASH:
                if (gBattleMons[gEffectBattler].status2 & STATUS2_LOCK_CONFUSE)
                {
                    gBattlescriptCurrInstr++;
                }
                else
                {
                    gBattleMons[gEffectBattler].status2 |= STATUS2_MULTIPLETURNS;
                    gLockedMoves[gEffectBattler] = gCurrentMove;
                    gBattleMons[gEffectBattler].status2 |= STATUS2_LOCK_CONFUSE_TURN((Random() & 1) + 2); // thrash for 2-3 turns
                }
                break;
            case MOVE_EFFECT_KNOCK_OFF:
                if (gBattleMons[gEffectBattler].ability == ABILITY_STICKY_HOLD)
                {
                    if (gBattleMons[gEffectBattler].item == ITEM_NONE)
                    {
                        gBattlescriptCurrInstr++;
                    }
                    else
                    {
                        gLastUsedAbility = ABILITY_STICKY_HOLD;
                        gBattlescriptCurrInstr = BattleScript_StickyHoldActivates;
                        RecordAbilityBattle(gEffectBattler, ABILITY_STICKY_HOLD);
                    }
                    break;
                }
                if (gBattleMons[gEffectBattler].item)
                {
                    side = GetBattlerSide(gEffectBattler);

                    gLastUsedItem = gBattleMons[gEffectBattler].item;
                    gBattleMons[gEffectBattler].item = ITEM_NONE;
                    gWishFutureKnock.knockedOffMons[side] |= gBitTable[gBattlerPartyIndexes[gEffectBattler]];

                    BattleScriptPush(gBattlescriptCurrInstr + 1);
                    gBattlescriptCurrInstr = BattleScript_KnockedOff;

                    *(u8 *)((u8 *)(&gBattleStruct->choicedMove[gEffectBattler]) + 0) = 0;
                    *(u8 *)((u8 *)(&gBattleStruct->choicedMove[gEffectBattler]) + 1) = 0;
                }
                else
                {
                    gBattlescriptCurrInstr++;
                }
                break;
            case MOVE_EFFECT_SP_ATK_TWO_DOWN: // Overheat
                BattleScriptPush(gBattlescriptCurrInstr + 1);
                gBattlescriptCurrInstr = BattleScript_SAtkDown2;
                break;
            }
        }
    }

    gBattleCommunication[MOVE_EFFECT_BYTE] = 0;
}

static void Cmd_seteffectwithchance(void)
{
    u32 percentChance;

    if (gBattleMons[gBattlerAttacker].ability == ABILITY_SERENE_GRACE)
        percentChance = gBattleMoves[gCurrentMove].secondaryEffectChance * 2;
    else
        percentChance = gBattleMoves[gCurrentMove].secondaryEffectChance;

    if (gBattleCommunication[MOVE_EFFECT_BYTE] & MOVE_EFFECT_CERTAIN
        && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
    {
        gBattleCommunication[MOVE_EFFECT_BYTE] &= ~MOVE_EFFECT_CERTAIN;
        SetMoveEffect(FALSE, MOVE_EFFECT_CERTAIN);
    }
    else if (Random() % 100 < percentChance
             && gBattleCommunication[MOVE_EFFECT_BYTE]
             && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
    {
        if (percentChance >= 100)
            SetMoveEffect(FALSE, MOVE_EFFECT_CERTAIN);
        else
            SetMoveEffect(FALSE, 0);
    }
    else
    {
        gBattlescriptCurrInstr++;
    }

    gBattleCommunication[MOVE_EFFECT_BYTE] = 0;
    gBattleScripting.multihitMoveEffect = 0;
}

static void Cmd_seteffectprimary(void)
{
    SetMoveEffect(TRUE, 0);
}

static void Cmd_seteffectsecondary(void)
{
    SetMoveEffect(FALSE, 0);
}

static void Cmd_clearstatusfromeffect(void)
{
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    if (gBattleCommunication[MOVE_EFFECT_BYTE] <= PRIMARY_STATUS_MOVE_EFFECT)
        gBattleMons[gActiveBattler].status1 &= (~sStatusFlagsForMoveEffects[gBattleCommunication[MOVE_EFFECT_BYTE]]);
    else
        gBattleMons[gActiveBattler].status2 &= (~sStatusFlagsForMoveEffects[gBattleCommunication[MOVE_EFFECT_BYTE]]);

    gBattleCommunication[MOVE_EFFECT_BYTE] = 0;
    gBattlescriptCurrInstr += 2;
    gBattleScripting.multihitMoveEffect = 0;
}

static void Cmd_tryfaintmon(void)
{
    const u8 *BS_ptr;

    if (gBattlescriptCurrInstr[2] != 0)
    {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        if (gHitMarker & HITMARKER_FAINTED(gActiveBattler))
        {
            BS_ptr = T1_READ_PTR(gBattlescriptCurrInstr + 3);

            BattleScriptPop();
            gBattlescriptCurrInstr = BS_ptr;
            gSideStatuses[GetBattlerSide(gActiveBattler)] &= ~SIDE_STATUS_SPIKES_DAMAGED;
        }
        else
        {
            gBattlescriptCurrInstr += 7;
        }
    }
    else
    {
        u8 battlerId;

        if (gBattlescriptCurrInstr[1] == BS_ATTACKER)
        {
            gActiveBattler = gBattlerAttacker;
            battlerId = gBattlerTarget;
            BS_ptr = BattleScript_FaintAttacker;
        }
        else
        {
            gActiveBattler = gBattlerTarget;
            battlerId = gBattlerAttacker;
            BS_ptr = BattleScript_FaintTarget;
        }
        if (!(gAbsentBattlerFlags & gBitTable[gActiveBattler])
         && gBattleMons[gActiveBattler].hp == 0)
        {
            gHitMarker |= HITMARKER_FAINTED(gActiveBattler);
            BattleScriptPush(gBattlescriptCurrInstr + 7);
            gBattlescriptCurrInstr = BS_ptr;
            if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
            {
                gHitMarker |= HITMARKER_PLAYER_FAINTED;
                if (gBattleResults.playerFaintCounter < 255)
                    gBattleResults.playerFaintCounter++;
                AdjustFriendshipOnBattleFaint(gActiveBattler);
            }
            else
            {
                if (gBattleResults.opponentFaintCounter < 255)
                    gBattleResults.opponentFaintCounter++;
                gBattleResults.lastOpponentSpecies = GetMonData(&gEnemyParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_SPECIES, NULL);
            }
            if ((gHitMarker & HITMARKER_DESTINYBOND) && gBattleMons[gBattlerAttacker].hp != 0)
            {
                gHitMarker &= ~HITMARKER_DESTINYBOND;
                BattleScriptPush(gBattlescriptCurrInstr);
                gBattleMoveDamage = gBattleMons[battlerId].hp;
                gBattlescriptCurrInstr = BattleScript_DestinyBondTakesLife;
            }
            if ((gStatuses3[gBattlerTarget] & STATUS3_GRUDGE)
             && !(gHitMarker & HITMARKER_GRUDGE)
             && GetBattlerSide(gBattlerAttacker) != GetBattlerSide(gBattlerTarget)
             && gBattleMons[gBattlerAttacker].hp != 0
             && gCurrentMove != MOVE_STRUGGLE)
            {
                u8 moveIndex = *(gBattleStruct->chosenMovePositions + gBattlerAttacker);

                gBattleMons[gBattlerAttacker].pp[moveIndex] = 0;
                BattleScriptPush(gBattlescriptCurrInstr);
                gBattlescriptCurrInstr = BattleScript_GrudgeTakesPp;
                gActiveBattler = gBattlerAttacker;
                BtlController_EmitSetMonData(BUFFER_A, moveIndex + REQUEST_PPMOVE1_BATTLE, 0, sizeof(gBattleMons[gActiveBattler].pp[moveIndex]), &gBattleMons[gActiveBattler].pp[moveIndex]);
                MarkBattlerForControllerExec(gActiveBattler);

                PREPARE_MOVE_BUFFER(gBattleTextBuff1, gBattleMons[gBattlerAttacker].moves[moveIndex])
            }
        }
        else
        {
            gBattlescriptCurrInstr += 7;
        }
    }
}

static void Cmd_dofaintanimation(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        BtlController_EmitFaintAnimation(BUFFER_A);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 2;
    }
}

static void Cmd_cleareffectsonfaint(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

        if (!(gBattleTypeFlags & BATTLE_TYPE_ARENA) || gBattleMons[gActiveBattler].hp == 0)
        {
            gBattleMons[gActiveBattler].status1 = 0;
            BtlController_EmitSetMonData(BUFFER_A, REQUEST_STATUS_BATTLE, 0, sizeof(gBattleMons[gActiveBattler].status1), &gBattleMons[gActiveBattler].status1);
            MarkBattlerForControllerExec(gActiveBattler);
        }

        FaintClearSetData(); // Effects like attractions, trapping, etc.
        gBattlescriptCurrInstr += 2;
    }
}

static void Cmd_jumpifstatus(void)
{
    u8 battlerId = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    u32 flags = T2_READ_32(gBattlescriptCurrInstr + 2);
    const u8 *jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 6);

    if (gBattleMons[battlerId].status1 & flags && gBattleMons[battlerId].hp != 0)
        gBattlescriptCurrInstr = jumpPtr;
    else
        gBattlescriptCurrInstr += 10;
}

static void Cmd_jumpifstatus2(void)
{
    u8 battlerId = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    u32 flags = T2_READ_32(gBattlescriptCurrInstr + 2);
    const u8 *jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 6);

    if (gBattleMons[battlerId].status2 & flags && gBattleMons[battlerId].hp != 0)
        gBattlescriptCurrInstr = jumpPtr;
    else
        gBattlescriptCurrInstr += 10;
}

static void Cmd_jumpifability(void)
{
    u8 battlerId;
    u8 ability = gBattlescriptCurrInstr[2];
    const u8 *jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 3);

    if (gBattlescriptCurrInstr[1] == BS_ATTACKER_SIDE)
    {
        battlerId = AbilityBattleEffects(ABILITYEFFECT_CHECK_BATTLER_SIDE, gBattlerAttacker, ability, 0, 0);
        if (battlerId)
        {
            gLastUsedAbility = ability;
            gBattlescriptCurrInstr = jumpPtr;
            RecordAbilityBattle(battlerId - 1, gLastUsedAbility);
            gBattleScripting.battlerWithAbility = battlerId - 1;
        }
        else
        {
            gBattlescriptCurrInstr += 7;
        }
    }
    else if (gBattlescriptCurrInstr[1] == BS_NOT_ATTACKER_SIDE)
    {
        battlerId = AbilityBattleEffects(ABILITYEFFECT_CHECK_OTHER_SIDE, gBattlerAttacker, ability, 0, 0);
        if (battlerId)
        {
            gLastUsedAbility = ability;
            gBattlescriptCurrInstr = jumpPtr;
            RecordAbilityBattle(battlerId - 1, gLastUsedAbility);
            gBattleScripting.battlerWithAbility = battlerId - 1;
        }
        else
        {
            gBattlescriptCurrInstr += 7;
        }
    }
    else
    {
        battlerId = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        if (gBattleMons[battlerId].ability == ability)
        {
            gLastUsedAbility = ability;
            gBattlescriptCurrInstr = jumpPtr;
            RecordAbilityBattle(battlerId, gLastUsedAbility);
            gBattleScripting.battlerWithAbility = battlerId;
        }
        else
        {
            gBattlescriptCurrInstr += 7;
        }
    }
}

static void Cmd_jumpifsideaffecting(void)
{
    u8 side;
    u16 flags;
    const u8 *jumpPtr;

    if (gBattlescriptCurrInstr[1] == BS_ATTACKER)
        side = GET_BATTLER_SIDE(gBattlerAttacker);
    else
        side = GET_BATTLER_SIDE(gBattlerTarget);

    flags = T2_READ_16(gBattlescriptCurrInstr + 2);
    jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 4);

    if (gSideStatuses[side] & flags)
        gBattlescriptCurrInstr = jumpPtr;
    else
        gBattlescriptCurrInstr += 8;
}

static void Cmd_jumpifstat(void)
{
    u8 ret = 0;
    u8 battlerId = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    u8 value = gBattleMons[battlerId].statStages[gBattlescriptCurrInstr[3]];

    switch (gBattlescriptCurrInstr[2])
    {
    case CMP_EQUAL:
        if (value == gBattlescriptCurrInstr[4])
            ret++;
        break;
    case CMP_NOT_EQUAL:
        if (value != gBattlescriptCurrInstr[4])
            ret++;
        break;
    case CMP_GREATER_THAN:
        if (value > gBattlescriptCurrInstr[4])
            ret++;
        break;
    case CMP_LESS_THAN:
        if (value < gBattlescriptCurrInstr[4])
            ret++;
        break;
    case CMP_COMMON_BITS:
        if (value & gBattlescriptCurrInstr[4])
            ret++;
        break;
    case CMP_NO_COMMON_BITS:
        if (!(value & gBattlescriptCurrInstr[4]))
            ret++;
        break;
    }

    if (ret)
        gBattlescriptCurrInstr = T2_READ_PTR(gBattlescriptCurrInstr + 5);
    else
        gBattlescriptCurrInstr += 9;
}

static void Cmd_jumpifstatus3condition(void)
{
    u32 status;
    const u8 *jumpPtr;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    status = T2_READ_32(gBattlescriptCurrInstr + 2);
    jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 7);

    if (gBattlescriptCurrInstr[6])
    {
        if ((gStatuses3[gActiveBattler] & status) != 0)
            gBattlescriptCurrInstr += 11;
        else
            gBattlescriptCurrInstr = jumpPtr;
    }
    else
    {
        if ((gStatuses3[gActiveBattler] & status) != 0)
            gBattlescriptCurrInstr = jumpPtr;
        else
            gBattlescriptCurrInstr += 11;
    }
}

static void Cmd_jumpiftype(void)
{
    u8 battlerId = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    u8 type = gBattlescriptCurrInstr[2];
    const u8 *jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 3);

    if (IS_BATTLER_OF_TYPE(battlerId, type))
        gBattlescriptCurrInstr = jumpPtr;
    else
        gBattlescriptCurrInstr += 7;
}

static void Cmd_getexp(void)
{
    u16 item;
    s32 i; // also used as stringId
    u8 holdEffect;
    s32 sentIn;
    s32 viaExpShare = 0;
    u16 *exp = &gBattleStruct->expValue;

    gBattlerFainted = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    sentIn = gSentPokesToOpponent[(gBattlerFainted & 2) >> 1];

    switch (gBattleScripting.getexpState)
    {
    case 0: // check if should receive exp at all
        if (GetBattlerSide(gBattlerFainted) != B_SIDE_OPPONENT || (gBattleTypeFlags &
             (BATTLE_TYPE_LINK
              | BATTLE_TYPE_RECORDED_LINK
              | BATTLE_TYPE_TRAINER_HILL
              | BATTLE_TYPE_FRONTIER
              | BATTLE_TYPE_SAFARI
              | BATTLE_TYPE_BATTLE_TOWER
              | BATTLE_TYPE_EREADER_TRAINER)))
        {
            gBattleScripting.getexpState = 6; // goto last case
        }
        else
        {
            gBattleScripting.getexpState++;
            gBattleStruct->givenExpMons |= gBitTable[gBattlerPartyIndexes[gBattlerFainted]];
        }
        break;
    case 1: // calculate experience points to redistribute
        {
            u16 calculatedExp;
            s32 viaSentIn;

            for (viaSentIn = 0, i = 0; i < PARTY_SIZE; i++)
            {
                if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(&gPlayerParty[i], MON_DATA_HP) == 0)
                    continue;
                if (gBitTable[i] & sentIn)
                    viaSentIn++;

                item = GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM);

                if (item == ITEM_ENIGMA_BERRY)
                    holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
                else
                    holdEffect = ItemId_GetHoldEffect(item);

                if (holdEffect == HOLD_EFFECT_EXP_SHARE)
                    viaExpShare++;
            }

            calculatedExp = gSpeciesInfo[gBattleMons[gBattlerFainted].species].expYield * gBattleMons[gBattlerFainted].level / 7;

            if (viaExpShare) // at least one mon is getting exp via exp share
            {
                *exp = SAFE_DIV(calculatedExp / 2, viaSentIn);
                if (*exp == 0)
                    *exp = 1;

                gExpShareExp = calculatedExp / 2 / viaExpShare;
                if (gExpShareExp == 0)
                    gExpShareExp = 1;
            }
            else
            {
                *exp = SAFE_DIV(calculatedExp, viaSentIn);
                if (*exp == 0)
                    *exp = 1;
                gExpShareExp = 0;
            }

            gBattleScripting.getexpState++;
            gBattleStruct->expGetterMonId = 0;
            gBattleStruct->sentInPokes = sentIn;
        }
        // fall through
    case 2: // set exp value to the poke in expgetter_id and print message
        if (gBattleControllerExecFlags == 0)
        {
            item = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_HELD_ITEM);

            if (item == ITEM_ENIGMA_BERRY)
                holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
            else
                holdEffect = ItemId_GetHoldEffect(item);

            if (holdEffect != HOLD_EFFECT_EXP_SHARE && !(gBattleStruct->sentInPokes & 1))
            {
                *(&gBattleStruct->sentInPokes) >>= 1;
                gBattleScripting.getexpState = 5;
                gBattleMoveDamage = 0; // used for exp
            }
            else if (GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL) == MAX_LEVEL)
            {
                *(&gBattleStruct->sentInPokes) >>= 1;
                gBattleScripting.getexpState = 5;
                gBattleMoveDamage = 0; // used for exp
            }
            else
            {
                // music change in wild battle after fainting a poke
                if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER) && gBattleMons[0].hp != 0 && !gBattleStruct->wildVictorySong)
                {
                    BattleStopLowHpSound();
                    PlayBGM(MUS_VICTORY_WILD);
                    gBattleStruct->wildVictorySong++;
                }

                if (GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_HP))
                {
                    if (gBattleStruct->sentInPokes & 1)
                        gBattleMoveDamage = *exp;
                    else
                        gBattleMoveDamage = 0;

                    if (holdEffect == HOLD_EFFECT_EXP_SHARE)
                        gBattleMoveDamage += gExpShareExp;
                    if (holdEffect == HOLD_EFFECT_LUCKY_EGG)
                        gBattleMoveDamage = (gBattleMoveDamage * 150) / 100;
                    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
                        gBattleMoveDamage = (gBattleMoveDamage * 150) / 100;

                    if (IsTradedMon(&gPlayerParty[gBattleStruct->expGetterMonId]))
                    {
                        // check if the Pokémon doesn't belong to the player
                        if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER && gBattleStruct->expGetterMonId >= 3)
                        {
                            i = STRINGID_EMPTYSTRING4;
                        }
                        else
                        {
                            gBattleMoveDamage = (gBattleMoveDamage * 150) / 100;
                            i = STRINGID_ABOOSTED;
                        }
                    }
                    else
                    {
                        i = STRINGID_EMPTYSTRING4;
                    }

                    // get exp getter battlerId
                    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
                    {
                        if (gBattlerPartyIndexes[2] == gBattleStruct->expGetterMonId && !(gAbsentBattlerFlags & gBitTable[2]))
                            gBattleStruct->expGetterBattlerId = 2;
                        else if (!(gAbsentBattlerFlags & gBitTable[0]))
                            gBattleStruct->expGetterBattlerId = 0;
                        else
                            gBattleStruct->expGetterBattlerId = 2;
                    }
                    else
                    {
                        gBattleStruct->expGetterBattlerId = 0;
                    }

                    PREPARE_MON_NICK_WITH_PREFIX_BUFFER(gBattleTextBuff1, gBattleStruct->expGetterBattlerId, gBattleStruct->expGetterMonId);
                    // buffer 'gained' or 'gained a boosted'
                    PREPARE_STRING_BUFFER(gBattleTextBuff2, i);
                    PREPARE_WORD_NUMBER_BUFFER(gBattleTextBuff3, 5, gBattleMoveDamage);

                    PrepareStringBattle(STRINGID_PKMNGAINEDEXP, gBattleStruct->expGetterBattlerId);
                    MonGainEVs(&gPlayerParty[gBattleStruct->expGetterMonId], gBattleMons[gBattlerFainted].species);
                }
                gBattleStruct->sentInPokes >>= 1;
                gBattleScripting.getexpState++;
            }
        }
        break;
    case 3: // Set stats and give exp
        if (gBattleControllerExecFlags == 0)
        {
            gBattleBufferB[gBattleStruct->expGetterBattlerId][0] = 0;
            if (GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_HP) && GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL) != MAX_LEVEL)
            {
                gBattleResources->beforeLvlUp->stats[STAT_HP]    = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_MAX_HP);
                gBattleResources->beforeLvlUp->stats[STAT_ATK]   = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_ATK);
                gBattleResources->beforeLvlUp->stats[STAT_DEF]   = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_DEF);
                gBattleResources->beforeLvlUp->stats[STAT_SPEED] = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPEED);
                gBattleResources->beforeLvlUp->stats[STAT_SPATK] = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPATK);
                gBattleResources->beforeLvlUp->stats[STAT_SPDEF] = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPDEF);

                gActiveBattler = gBattleStruct->expGetterBattlerId;
                BtlController_EmitExpUpdate(BUFFER_A, gBattleStruct->expGetterMonId, gBattleMoveDamage);
                MarkBattlerForControllerExec(gActiveBattler);
            }
            gBattleScripting.getexpState++;
        }
        break;
    case 4: // lvl up if necessary
        if (gBattleControllerExecFlags == 0)
        {
            gActiveBattler = gBattleStruct->expGetterBattlerId;
            if (gBattleBufferB[gActiveBattler][0] == CONTROLLER_TWORETURNVALUES && gBattleBufferB[gActiveBattler][1] == RET_VALUE_LEVELED_UP)
            {
                if (gBattleTypeFlags & BATTLE_TYPE_TRAINER && gBattlerPartyIndexes[gActiveBattler] == gBattleStruct->expGetterMonId)
                    HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);

                PREPARE_MON_NICK_WITH_PREFIX_BUFFER(gBattleTextBuff1, gActiveBattler, gBattleStruct->expGetterMonId);
                PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff2, 3, GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL));

                BattleScriptPushCursor();
                gLeveledUpInBattle |= gBitTable[gBattleStruct->expGetterMonId];
                gBattlescriptCurrInstr = BattleScript_LevelUp;
                gBattleMoveDamage = (gBattleBufferB[gActiveBattler][2] | (gBattleBufferB[gActiveBattler][3] << 8));
                AdjustFriendship(&gPlayerParty[gBattleStruct->expGetterMonId], FRIENDSHIP_EVENT_GROW_LEVEL);

                // update battle mon structure after level up
                if (gBattlerPartyIndexes[0] == gBattleStruct->expGetterMonId && gBattleMons[0].hp)
                {
                    gBattleMons[0].level = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL);
                    gBattleMons[0].hp = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_HP);
                    gBattleMons[0].maxHP = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_MAX_HP);
                    gBattleMons[0].attack = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_ATK);
                    gBattleMons[0].defense = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_DEF);
                    // Speed is duplicated, likely due to a copy-paste error.
                    gBattleMons[0].speed = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPEED);
                    gBattleMons[0].speed = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPEED);
                    gBattleMons[0].spAttack = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPATK);
                    gBattleMons[0].spDefense = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPDEF);
                }

                if (gBattlerPartyIndexes[2] == gBattleStruct->expGetterMonId && gBattleMons[2].hp && (gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
                {
                    gBattleMons[2].level = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL);
                    gBattleMons[2].hp = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_HP);
                    gBattleMons[2].maxHP = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_MAX_HP);
                    gBattleMons[2].attack = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_ATK);
                    gBattleMons[2].defense = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_DEF);
                    gBattleMons[2].speed = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPEED);
                    // Speed is duplicated again, but Special Defense is missing.
#ifdef BUGFIX
                    gBattleMons[2].spDefense = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPDEF);
#else
                    gBattleMons[2].speed = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPEED);
#endif
                    gBattleMons[2].spAttack = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPATK);
                }
                gBattleScripting.getexpState = 5;
            }
            else
            {
                gBattleMoveDamage = 0;
                gBattleScripting.getexpState = 5;
            }
        }
        break;
    case 5: // looper increment
        if (gBattleMoveDamage) // there is exp to give, goto case 3 that gives exp
        {
            gBattleScripting.getexpState = 3;
        }
        else
        {
            gBattleStruct->expGetterMonId++;
            if (gBattleStruct->expGetterMonId < PARTY_SIZE)
                gBattleScripting.getexpState = 2; // loop again
            else
                gBattleScripting.getexpState = 6; // we're done
        }
        break;
    case 6: // increment instruction
        if (gBattleControllerExecFlags == 0)
        {
            // not sure why gf clears the item and ability here
            gBattleMons[gBattlerFainted].item = ITEM_NONE;
            gBattleMons[gBattlerFainted].ability = ABILITY_NONE;
            gBattlescriptCurrInstr += 2;
        }
        break;
    }
}

// For battles that aren't BATTLE_TYPE_LINK or BATTLE_TYPE_RECORDED_LINK, the only thing this
// command does is check whether the player has won/lost by totaling each team's HP. It then
// sets gBattleOutcome accordingly, if necessary.
static void Cmd_checkteamslost(void)
{
    u16 HP_count = 0;
    s32 i;

    if (gBattleControllerExecFlags)
        return;

    // Get total HP for the player's party to determine if the player has lost
    if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER && gPartnerTrainerId == TRAINER_STEVEN_PARTNER)
    {
        // In multi battle with Steven, skip his Pokémon
        for (i = 0; i < MULTI_PARTY_SIZE; i++)
        {
            if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG))
                HP_count += GetMonData(&gPlayerParty[i], MON_DATA_HP);
        }
    }
    else
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG)
             && (!(gBattleTypeFlags & BATTLE_TYPE_ARENA) || !(gBattleStruct->arenaLostPlayerMons & gBitTable[i])))
            {
                HP_count += GetMonData(&gPlayerParty[i], MON_DATA_HP);
            }
        }
    }
    if (HP_count == 0)
        gBattleOutcome |= B_OUTCOME_LOST;
    HP_count = 0;

    // Get total HP for the enemy's party to determine if the player has won
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gEnemyParty[i], MON_DATA_SPECIES) && !GetMonData(&gEnemyParty[i], MON_DATA_IS_EGG)
         && (!(gBattleTypeFlags & BATTLE_TYPE_ARENA) || !(gBattleStruct->arenaLostOpponentMons & gBitTable[i])))
        {
            HP_count += GetMonData(&gEnemyParty[i], MON_DATA_HP);
        }
    }
    if (HP_count == 0)
        gBattleOutcome |= B_OUTCOME_WON;

    // For link battles that haven't ended, count number of empty battler spots
    // In link multi battles, jump to pointer if more than 1 spot empty
    // In non-multi battles, jump to pointer if 1 spot is missing on both sides
    if (gBattleOutcome == 0 && (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)))
    {
        s32 emptyPlayerSpots = 0;
        s32 emptyOpponentSpots;

        for (i = 0; i < gBattlersCount; i += 2)
        {
            if ((gHitMarker & HITMARKER_FAINTED2(i)) && (!gSpecialStatuses[i].faintedHasReplacement))
                emptyPlayerSpots++;
        }

        emptyOpponentSpots = 0;
        for (i = 1; i < gBattlersCount; i += 2)
        {
            if ((gHitMarker & HITMARKER_FAINTED2(i)) && (!gSpecialStatuses[i].faintedHasReplacement))
                emptyOpponentSpots++;
        }

        if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
        {
            if (emptyOpponentSpots + emptyPlayerSpots > 1)
                gBattlescriptCurrInstr = T2_READ_PTR(gBattlescriptCurrInstr + 1);
            else
                gBattlescriptCurrInstr += 5;
        }
        else
        {
            if (emptyOpponentSpots != 0 && emptyPlayerSpots != 0)
                gBattlescriptCurrInstr = T2_READ_PTR(gBattlescriptCurrInstr + 1);
            else
                gBattlescriptCurrInstr += 5;
        }
    }
    else
    {
        gBattlescriptCurrInstr += 5;
    }
}

static void MoveValuesCleanUp(void)
{
    gMoveResultFlags = 0;
    gBattleScripting.dmgMultiplier = 1;
    gCritMultiplier = 1;
    gBattleCommunication[MOVE_EFFECT_BYTE] = 0;
    gBattleCommunication[MISS_TYPE] = 0;
    gHitMarker &= ~HITMARKER_DESTINYBOND;
    gHitMarker &= ~HITMARKER_SYNCHRONISE_EFFECT;
}

static void Cmd_movevaluescleanup(void)
{
    MoveValuesCleanUp();
    gBattlescriptCurrInstr += 1;
}

static void Cmd_setmultihit(void)
{
    gMultiHitCounter = gBattlescriptCurrInstr[1];
    gBattlescriptCurrInstr += 2;
}

static void Cmd_decrementmultihit(void)
{
    if (--gMultiHitCounter == 0)
        gBattlescriptCurrInstr += 5;
    else
        gBattlescriptCurrInstr = T2_READ_PTR(gBattlescriptCurrInstr + 1);
}

static void Cmd_goto(void)
{
    gBattlescriptCurrInstr = T2_READ_PTR(gBattlescriptCurrInstr + 1);
}

static void Cmd_jumpifbyte(void)
{
    u8 caseID = gBattlescriptCurrInstr[1];
    const u8 *memByte = T2_READ_PTR(gBattlescriptCurrInstr + 2);
    u8 value = gBattlescriptCurrInstr[6];
    const u8 *jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 7);

    gBattlescriptCurrInstr += 11;

    switch (caseID)
    {
    case CMP_EQUAL:
        if (*memByte == value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_NOT_EQUAL:
        if (*memByte != value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_GREATER_THAN:
        if (*memByte > value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_LESS_THAN:
        if (*memByte < value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_COMMON_BITS:
        if (*memByte & value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_NO_COMMON_BITS:
        if (!(*memByte & value))
            gBattlescriptCurrInstr = jumpPtr;
        break;
    }
}

static void Cmd_jumpifhalfword(void)
{
    u8 caseID = gBattlescriptCurrInstr[1];
    const u16 *memHword = T2_READ_PTR(gBattlescriptCurrInstr + 2);
    u16 value = T2_READ_16(gBattlescriptCurrInstr + 6);
    const u8 *jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 8);

    gBattlescriptCurrInstr += 12;

    switch (caseID)
    {
    case CMP_EQUAL:
        if (*memHword == value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_NOT_EQUAL:
        if (*memHword != value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_GREATER_THAN:
        if (*memHword > value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_LESS_THAN:
        if (*memHword < value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_COMMON_BITS:
        if (*memHword & value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_NO_COMMON_BITS:
        if (!(*memHword & value))
            gBattlescriptCurrInstr = jumpPtr;
        break;
    }
}

static void Cmd_jumpifword(void)
{
    u8 caseID = gBattlescriptCurrInstr[1];
    const u32 *memWord = T2_READ_PTR(gBattlescriptCurrInstr + 2);
    u32 value = T1_READ_32(gBattlescriptCurrInstr + 6);
    const u8 *jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 10);

    gBattlescriptCurrInstr += 14;

    switch (caseID)
    {
    case CMP_EQUAL:
        if (*memWord == value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_NOT_EQUAL:
        if (*memWord != value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_GREATER_THAN:
        if (*memWord > value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_LESS_THAN:
        if (*memWord < value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_COMMON_BITS:
        if (*memWord & value)
            gBattlescriptCurrInstr = jumpPtr;
        break;
    case CMP_NO_COMMON_BITS:
        if (!(*memWord & value))
            gBattlescriptCurrInstr = jumpPtr;
        break;
    }
}

static void Cmd_jumpifarrayequal(void)
{
    const u8 *mem1 = T2_READ_PTR(gBattlescriptCurrInstr + 1);
    const u8 *mem2 = T2_READ_PTR(gBattlescriptCurrInstr + 5);
    u32 size = gBattlescriptCurrInstr[9];
    const u8 *jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 10);

    u8 i;
    for (i = 0; i < size; i++)
    {
        if (*mem1 != *mem2)
        {
            gBattlescriptCurrInstr += 14;
            break;
        }
        mem1++, mem2++;
    }

    if (i == size)
        gBattlescriptCurrInstr = jumpPtr;
}

static void Cmd_jumpifarraynotequal(void)
{
    u8 equalBytes = 0;
    const u8 *mem1 = T2_READ_PTR(gBattlescriptCurrInstr + 1);
    const u8 *mem2 = T2_READ_PTR(gBattlescriptCurrInstr + 5);
    u32 size = gBattlescriptCurrInstr[9];
    const u8 *jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 10);

    u8 i;
    for (i = 0; i < size; i++)
    {
        if (*mem1 == *mem2)
            equalBytes++;
        mem1++, mem2++;
    }

    if (equalBytes != size)
        gBattlescriptCurrInstr = jumpPtr;
    else
        gBattlescriptCurrInstr += 14;
}

static void Cmd_setbyte(void)
{
    u8 *memByte = T2_READ_PTR(gBattlescriptCurrInstr + 1);
    *memByte = gBattlescriptCurrInstr[5];

    gBattlescriptCurrInstr += 6;
}

static void Cmd_addbyte(void)
{
    u8 *memByte = T2_READ_PTR(gBattlescriptCurrInstr + 1);
    *memByte += gBattlescriptCurrInstr[5];
    gBattlescriptCurrInstr += 6;
}

static void Cmd_subbyte(void)
{
    u8 *memByte = T2_READ_PTR(gBattlescriptCurrInstr + 1);
    *memByte -= gBattlescriptCurrInstr[5];
    gBattlescriptCurrInstr += 6;
}

static void Cmd_copyarray(void)
{
    u8 *dest = T2_READ_PTR(gBattlescriptCurrInstr + 1);
    const u8 *src = T2_READ_PTR(gBattlescriptCurrInstr + 5);
    s32 size = gBattlescriptCurrInstr[9];

    s32 i;
    for (i = 0; i < size; i++)
        dest[i] = src[i];

    gBattlescriptCurrInstr += 10;
}

static void Cmd_copyarraywithindex(void)
{
    u8 *dest = T2_READ_PTR(gBattlescriptCurrInstr + 1);
    const u8 *src = T2_READ_PTR(gBattlescriptCurrInstr + 5);
    const u8 *index = T2_READ_PTR(gBattlescriptCurrInstr + 9);
    s32 size = gBattlescriptCurrInstr[13];

    s32 i;
    for (i = 0; i < size; i++)
        dest[i] = src[i + *index];

    gBattlescriptCurrInstr += 14;
}

static void Cmd_orbyte(void)
{
    u8 *memByte = T2_READ_PTR(gBattlescriptCurrInstr + 1);
    *memByte |= gBattlescriptCurrInstr[5];
    gBattlescriptCurrInstr += 6;
}

static void Cmd_orhalfword(void)
{
    u16 *memHword = T2_READ_PTR(gBattlescriptCurrInstr + 1);
    u16 val = T2_READ_16(gBattlescriptCurrInstr + 5);

    *memHword |= val;
    gBattlescriptCurrInstr += 7;
}

static void Cmd_orword(void)
{
    u32 *memWord = T2_READ_PTR(gBattlescriptCurrInstr + 1);
    u32 val = T2_READ_32(gBattlescriptCurrInstr + 5);

    *memWord |= val;
    gBattlescriptCurrInstr += 9;
}

static void Cmd_bicbyte(void)
{
    u8 *memByte = T2_READ_PTR(gBattlescriptCurrInstr + 1);
    *memByte &= ~(gBattlescriptCurrInstr[5]);
    gBattlescriptCurrInstr += 6;
}

static void Cmd_bichalfword(void)
{
    u16 *memHword = T2_READ_PTR(gBattlescriptCurrInstr + 1);
    u16 val = T2_READ_16(gBattlescriptCurrInstr + 5);

    *memHword &= ~val;
    gBattlescriptCurrInstr += 7;
}

static void Cmd_bicword(void)
{
    u32 *memWord = T2_READ_PTR(gBattlescriptCurrInstr + 1);
    u32 val = T2_READ_32(gBattlescriptCurrInstr + 5);

    *memWord &= ~val;
    gBattlescriptCurrInstr += 9;
}

static void Cmd_pause(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        u16 value = T2_READ_16(gBattlescriptCurrInstr + 1);
        if (++gPauseCounterBattle >= value)
        {
            gPauseCounterBattle = 0;
            gBattlescriptCurrInstr += 3;
        }
    }
}

static void Cmd_waitstate(void)
{
    if (gBattleControllerExecFlags == 0)
        gBattlescriptCurrInstr++;
}

static void Cmd_healthbar_update(void)
{
    if (gBattlescriptCurrInstr[1] == BS_TARGET)
        gActiveBattler = gBattlerTarget;
    else
        gActiveBattler = gBattlerAttacker;

    BtlController_EmitHealthBarUpdate(BUFFER_A, gBattleMoveDamage);
    MarkBattlerForControllerExec(gActiveBattler);
    gBattlescriptCurrInstr += 2;
}

static void Cmd_return(void)
{
    BattleScriptPop();
}

static void Cmd_end(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_ARENA)
        BattleArena_AddSkillPoints(gBattlerAttacker);

    gMoveResultFlags = 0;
    gActiveBattler = 0;
    gCurrentActionFuncId = B_ACTION_TRY_FINISH;
}

static void Cmd_end2(void)
{
    gActiveBattler = 0;
    gCurrentActionFuncId = B_ACTION_TRY_FINISH;
}

// Pops the main function stack
static void Cmd_end3(void)
{
    BattleScriptPop();
    if (gBattleResources->battleCallbackStack->size != 0)
        gBattleResources->battleCallbackStack->size--;
    gBattleMainFunc = gBattleResources->battleCallbackStack->function[gBattleResources->battleCallbackStack->size];
}

static void Cmd_call(void)
{
    BattleScriptPush(gBattlescriptCurrInstr + 5);
    gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
}

static void Cmd_jumpiftype2(void)
{
    u8 battlerId = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    if (gBattlescriptCurrInstr[2] == gBattleMons[battlerId].types[0] || gBattlescriptCurrInstr[2] == gBattleMons[battlerId].types[1])
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 3);
    else
        gBattlescriptCurrInstr += 7;
}

static void Cmd_jumpifabilitypresent(void)
{
    if (AbilityBattleEffects(ABILITYEFFECT_CHECK_ON_FIELD, 0, gBattlescriptCurrInstr[1], 0, 0))
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
    else
        gBattlescriptCurrInstr += 6;
}

static void Cmd_endselectionscript(void)
{
    *(gBattlerAttacker + gBattleStruct->selectionScriptFinished) = TRUE;
}

static void Cmd_playanimation(void)
{
    const u16 *argumentPtr;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    argumentPtr = T2_READ_PTR(gBattlescriptCurrInstr + 3);

    if (gBattlescriptCurrInstr[2] == B_ANIM_STATS_CHANGE
     || gBattlescriptCurrInstr[2] == B_ANIM_SNATCH_MOVE
     || gBattlescriptCurrInstr[2] == B_ANIM_SUBSTITUTE_FADE)
    {
        BtlController_EmitBattleAnimation(BUFFER_A, gBattlescriptCurrInstr[2], *argumentPtr);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 7;
    }
    else if (gHitMarker & HITMARKER_NO_ANIMATIONS)
    {
        BattleScriptPush(gBattlescriptCurrInstr + 7);
        gBattlescriptCurrInstr = BattleScript_Pausex20;
    }
    else if (gBattlescriptCurrInstr[2] == B_ANIM_RAIN_CONTINUES
          || gBattlescriptCurrInstr[2] == B_ANIM_SUN_CONTINUES
          || gBattlescriptCurrInstr[2] == B_ANIM_SANDSTORM_CONTINUES
          || gBattlescriptCurrInstr[2] == B_ANIM_HAIL_CONTINUES)
    {
        BtlController_EmitBattleAnimation(BUFFER_A, gBattlescriptCurrInstr[2], *argumentPtr);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 7;
    }
    else if (gStatuses3[gActiveBattler] & STATUS3_SEMI_INVULNERABLE)
    {
        gBattlescriptCurrInstr += 7;
    }
    else
    {
        BtlController_EmitBattleAnimation(BUFFER_A, gBattlescriptCurrInstr[2], *argumentPtr);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 7;
    }
}

// Same as playanimation, except it takes a pointer to some animation id, instead of taking the value directly
static void Cmd_playanimation_var(void)
{
    const u16 *argumentPtr;
    const u8 *animationIdPtr;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    animationIdPtr = T2_READ_PTR(gBattlescriptCurrInstr + 2);
    argumentPtr = T2_READ_PTR(gBattlescriptCurrInstr + 6);

    if (*animationIdPtr == B_ANIM_STATS_CHANGE
     || *animationIdPtr == B_ANIM_SNATCH_MOVE
     || *animationIdPtr == B_ANIM_SUBSTITUTE_FADE)
    {
        BtlController_EmitBattleAnimation(BUFFER_A, *animationIdPtr, *argumentPtr);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 10;
    }
    else if (gHitMarker & HITMARKER_NO_ANIMATIONS)
    {
        gBattlescriptCurrInstr += 10;
    }
    else if (*animationIdPtr == B_ANIM_RAIN_CONTINUES
          || *animationIdPtr == B_ANIM_SUN_CONTINUES
          || *animationIdPtr == B_ANIM_SANDSTORM_CONTINUES
          || *animationIdPtr == B_ANIM_HAIL_CONTINUES)
    {
        BtlController_EmitBattleAnimation(BUFFER_A, *animationIdPtr, *argumentPtr);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 10;
    }
    else if (gStatuses3[gActiveBattler] & STATUS3_SEMI_INVULNERABLE)
    {
        gBattlescriptCurrInstr += 10;
    }
    else
    {
        BtlController_EmitBattleAnimation(BUFFER_A, *animationIdPtr, *argumentPtr);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 10;
    }
}

static void Cmd_setgraphicalstatchangevalues(void)
{
    u8 value = 0;
    switch (GET_STAT_BUFF_VALUE2(gBattleScripting.statChanger))
    {
    case SET_STAT_BUFF_VALUE(1): // +1
        value = STAT_ANIM_PLUS1 + 1;
        break;
    case SET_STAT_BUFF_VALUE(2): // +2
        value = STAT_ANIM_PLUS2 + 1;
        break;
    case SET_STAT_BUFF_VALUE(1) | STAT_BUFF_NEGATIVE: // -1
        value = STAT_ANIM_MINUS1 + 1;
        break;
    case SET_STAT_BUFF_VALUE(2) | STAT_BUFF_NEGATIVE: // -2
        value = STAT_ANIM_MINUS2 + 1;
        break;
    }
    gBattleScripting.animArg1 = GET_STAT_BUFF_ID(gBattleScripting.statChanger) + value - 1;
    gBattleScripting.animArg2 = 0;
    gBattlescriptCurrInstr++;
}

static void Cmd_playstatchangeanimation(void)
{
    u32 currStat = 0;
    u16 statAnimId = 0;
    s32 changeableStatsCount = 0;
    u8 statsToCheck = 0;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    statsToCheck = gBattlescriptCurrInstr[2];

    if (gBattlescriptCurrInstr[3] & STAT_CHANGE_NEGATIVE) // goes down
    {
        s16 startingStatAnimId;
        if (gBattlescriptCurrInstr[3] & STAT_CHANGE_BY_TWO)
            startingStatAnimId = STAT_ANIM_MINUS2;
        else
            startingStatAnimId = STAT_ANIM_MINUS1;

        while (statsToCheck != 0)
        {
            if (statsToCheck & 1)
            {
                if (gBattlescriptCurrInstr[3] & STAT_CHANGE_CANT_PREVENT)
                {
                    if (gBattleMons[gActiveBattler].statStages[currStat] > MIN_STAT_STAGE)
                    {
                        statAnimId = startingStatAnimId + currStat;
                        changeableStatsCount++;
                    }
                }
                else if (!gSideTimers[GET_BATTLER_SIDE(gActiveBattler)].mistTimer
                        && gBattleMons[gActiveBattler].ability != ABILITY_CLEAR_BODY
                        && gBattleMons[gActiveBattler].ability != ABILITY_WHITE_SMOKE
                        && !(gBattleMons[gActiveBattler].ability == ABILITY_KEEN_EYE && currStat == STAT_ACC)
                        && !(gBattleMons[gActiveBattler].ability == ABILITY_HYPER_CUTTER && currStat == STAT_ATK))
                {
                    if (gBattleMons[gActiveBattler].statStages[currStat] > MIN_STAT_STAGE)
                    {
                        statAnimId = startingStatAnimId + currStat;
                        changeableStatsCount++;
                    }
                }
            }
            statsToCheck >>= 1, currStat++;
        }

        if (changeableStatsCount > 1) // more than one stat, so the color is gray
        {
            if (gBattlescriptCurrInstr[3] & STAT_CHANGE_BY_TWO)
                statAnimId = STAT_ANIM_MULTIPLE_MINUS2;
            else
                statAnimId = STAT_ANIM_MULTIPLE_MINUS1;
        }
    }
    else // goes up
    {
        s16 startingStatAnimId;
        if (gBattlescriptCurrInstr[3] & STAT_CHANGE_BY_TWO)
            startingStatAnimId = STAT_ANIM_PLUS2;
        else
            startingStatAnimId = STAT_ANIM_PLUS1;

        while (statsToCheck != 0)
        {
            if (statsToCheck & 1 && gBattleMons[gActiveBattler].statStages[currStat] < MAX_STAT_STAGE)
            {
                statAnimId = startingStatAnimId + currStat;
                changeableStatsCount++;
            }
            statsToCheck >>= 1, currStat++;
        }

        if (changeableStatsCount > 1) // more than one stat, so the color is gray
        {
            if (gBattlescriptCurrInstr[3] & STAT_CHANGE_BY_TWO)
                statAnimId = STAT_ANIM_MULTIPLE_PLUS2;
            else
                statAnimId = STAT_ANIM_MULTIPLE_PLUS1;
        }
    }

    if (gBattlescriptCurrInstr[3] & STAT_CHANGE_MULTIPLE_STATS && changeableStatsCount < 2)
    {
        gBattlescriptCurrInstr += 4;
    }
    else if (changeableStatsCount != 0 && !gBattleScripting.statAnimPlayed)
    {
        BtlController_EmitBattleAnimation(BUFFER_A, B_ANIM_STATS_CHANGE, statAnimId);
        MarkBattlerForControllerExec(gActiveBattler);
        if (gBattlescriptCurrInstr[3] & STAT_CHANGE_MULTIPLE_STATS && changeableStatsCount > 1)
            gBattleScripting.statAnimPlayed = TRUE;
        gBattlescriptCurrInstr += 4;
    }
    else
    {
        gBattlescriptCurrInstr += 4;
    }
}

static void Cmd_moveend(void)
{
    s32 i;
    bool32 effect = FALSE;
    u8 moveType = 0;
    u8 holdEffectAtk = 0;
    u16 *choicedMoveAtk = NULL;
    u8 endMode, endState;
    u16 originallyUsedMove;

    if (gChosenMove == MOVE_UNAVAILABLE)
        originallyUsedMove = MOVE_NONE;
    else
        originallyUsedMove = gChosenMove;

    endMode = gBattlescriptCurrInstr[1];
    endState = gBattlescriptCurrInstr[2];

    if (gBattleMons[gBattlerAttacker].item == ITEM_ENIGMA_BERRY)
        holdEffectAtk = gEnigmaBerries[gBattlerAttacker].holdEffect;
    else
        holdEffectAtk = ItemId_GetHoldEffect(gBattleMons[gBattlerAttacker].item);

    choicedMoveAtk = &gBattleStruct->choicedMove[gBattlerAttacker];
    GET_MOVE_TYPE(gCurrentMove, moveType);

    do
    {
        switch (gBattleScripting.moveendState)
        {
        case MOVEEND_RAGE: // rage check
            if (gBattleMons[gBattlerTarget].status2 & STATUS2_RAGE
                && gBattleMons[gBattlerTarget].hp != 0
                && gBattlerAttacker != gBattlerTarget
                && GetBattlerSide(gBattlerAttacker) != GetBattlerSide(gBattlerTarget)
                && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
                && TARGET_TURN_DAMAGED
                && gBattleMoves[gCurrentMove].power != 0
                && gBattleMons[gBattlerTarget].statStages[STAT_ATK] < MAX_STAT_STAGE)
            {
                gBattleMons[gBattlerTarget].statStages[STAT_ATK]++;
                BattleScriptPushCursor();
                gBattlescriptCurrInstr = BattleScript_RageIsBuilding;
                effect = TRUE;
            }
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_DEFROST: // defrosting check
            if (gBattleMons[gBattlerTarget].status1 & STATUS1_FREEZE
                && gBattleMons[gBattlerTarget].hp != 0
                && gBattlerAttacker != gBattlerTarget
                && gSpecialStatuses[gBattlerTarget].specialDmg
                && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
                && moveType == TYPE_FIRE)
            {
                gBattleMons[gBattlerTarget].status1 &= ~STATUS1_FREEZE;
                gActiveBattler = gBattlerTarget;
                BtlController_EmitSetMonData(BUFFER_A, REQUEST_STATUS_BATTLE, 0, sizeof(gBattleMons[gBattlerTarget].status1), &gBattleMons[gBattlerTarget].status1);
                MarkBattlerForControllerExec(gActiveBattler);
                BattleScriptPushCursor();
                gBattlescriptCurrInstr = BattleScript_DefrostedViaFireMove;
                effect = TRUE;
            }
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_SYNCHRONIZE_TARGET: // target synchronize
            if (AbilityBattleEffects(ABILITYEFFECT_SYNCHRONIZE, gBattlerTarget, 0, 0, 0))
                effect = TRUE;
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_ON_DAMAGE_ABILITIES: // Such as abilities activating on contact (Effect Spore, Rough Skin, etc.).
            if (AbilityBattleEffects(ABILITYEFFECT_ON_DAMAGE, gBattlerTarget, 0, 0, 0))
                effect = TRUE;
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_IMMUNITY_ABILITIES: // status immunities
            if (AbilityBattleEffects(ABILITYEFFECT_IMMUNITY, 0, 0, 0, 0))
                effect = TRUE; // it loops through all battlers, so we increment after its done with all battlers
            else
                gBattleScripting.moveendState++;
            break;
        case MOVEEND_SYNCHRONIZE_ATTACKER: // attacker synchronize
            if (AbilityBattleEffects(ABILITYEFFECT_ATK_SYNCHRONIZE, gBattlerAttacker, 0, 0, 0))
                effect = TRUE;
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_CHOICE_MOVE: // update choice band move
            if (gHitMarker & HITMARKER_OBEYS
             && holdEffectAtk == HOLD_EFFECT_CHOICE_BAND
             && gChosenMove != MOVE_STRUGGLE
             && (*choicedMoveAtk == MOVE_NONE || *choicedMoveAtk == MOVE_UNAVAILABLE))
            {
                if (gChosenMove == MOVE_BATON_PASS && !(gMoveResultFlags & MOVE_RESULT_FAILED))
                {
                    gBattleScripting.moveendState++;
                    break;
                }
                *choicedMoveAtk = gChosenMove;
            }
            for (i = 0; i < MAX_MON_MOVES; i++)
            {
                if (gBattleMons[gBattlerAttacker].moves[i] == *choicedMoveAtk)
                    break;
            }
            if (i == MAX_MON_MOVES)
                *choicedMoveAtk = MOVE_NONE;
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_CHANGED_ITEMS: // changed held items
            for (i = 0; i < gBattlersCount; i++)
            {
                u16 *changedItem = &gBattleStruct->changedItems[i];
                if (*changedItem != ITEM_NONE)
                {
                    gBattleMons[i].item = *changedItem;
                    *changedItem = ITEM_NONE;
                }
            }
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_ITEM_EFFECTS_ALL: // item effects for all battlers
            if (ItemBattleEffects(ITEMEFFECT_MOVE_END, 0, FALSE))
                effect = TRUE;
            else
                gBattleScripting.moveendState++;
            break;
        case MOVEEND_KINGSROCK_SHELLBELL: // king's rock and shell bell
            if (ItemBattleEffects(ITEMEFFECT_KINGSROCK_SHELLBELL, 0, FALSE))
                effect = TRUE;
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_ATTACKER_INVISIBLE: // make attacker sprite invisible
            if (gStatuses3[gBattlerAttacker] & (STATUS3_SEMI_INVULNERABLE)
                && gHitMarker & HITMARKER_NO_ANIMATIONS)
            {
                gActiveBattler = gBattlerAttacker;
                BtlController_EmitSpriteInvisibility(BUFFER_A, TRUE);
                MarkBattlerForControllerExec(gActiveBattler);
                gBattleScripting.moveendState++;
                return;
            }
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_ATTACKER_VISIBLE: // make attacker sprite visible
            if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT
                || !(gStatuses3[gBattlerAttacker] & (STATUS3_SEMI_INVULNERABLE))
                || WasUnableToUseMove(gBattlerAttacker))
            {
                gActiveBattler = gBattlerAttacker;
                BtlController_EmitSpriteInvisibility(BUFFER_A, FALSE);
                MarkBattlerForControllerExec(gActiveBattler);
                gStatuses3[gBattlerAttacker] &= ~STATUS3_SEMI_INVULNERABLE;
                gSpecialStatuses[gBattlerAttacker].restoredBattlerSprite = 1;
                gBattleScripting.moveendState++;
                return;
            }
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_TARGET_VISIBLE: // make target sprite visible
            if (!gSpecialStatuses[gBattlerTarget].restoredBattlerSprite && gBattlerTarget < gBattlersCount
                && !(gStatuses3[gBattlerTarget] & STATUS3_SEMI_INVULNERABLE))
            {
                gActiveBattler = gBattlerTarget;
                BtlController_EmitSpriteInvisibility(BUFFER_A, FALSE);
                MarkBattlerForControllerExec(gActiveBattler);
                gStatuses3[gBattlerTarget] &= ~STATUS3_SEMI_INVULNERABLE;
                gBattleScripting.moveendState++;
                return;
            }
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_SUBSTITUTE: // update substitute
            for (i = 0; i < gBattlersCount; i++)
            {
                if (gDisableStructs[i].substituteHP == 0)
                    gBattleMons[i].status2 &= ~STATUS2_SUBSTITUTE;
            }
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_UPDATE_LAST_MOVES:
            if (gHitMarker & HITMARKER_SWAP_ATTACKER_TARGET)
            {
                gActiveBattler = gBattlerAttacker;
                gBattlerAttacker = gBattlerTarget;
                gBattlerTarget = gActiveBattler;
                gHitMarker &= ~HITMARKER_SWAP_ATTACKER_TARGET;
            }
            if (gHitMarker & HITMARKER_ATTACKSTRING_PRINTED)
            {
                gLastPrintedMoves[gBattlerAttacker] = gChosenMove;
            }
            if (!(gAbsentBattlerFlags & gBitTable[gBattlerAttacker])
                && !(gBattleStruct->absentBattlerFlags & gBitTable[gBattlerAttacker])
                && gBattleMoves[originallyUsedMove].effect != EFFECT_BATON_PASS)
            {
                if (gHitMarker & HITMARKER_OBEYS)
                {
                    gLastMoves[gBattlerAttacker] = gChosenMove;
                    gLastResultingMoves[gBattlerAttacker] = gCurrentMove;
                }
                else
                {
                    gLastMoves[gBattlerAttacker] = MOVE_UNAVAILABLE;
                    gLastResultingMoves[gBattlerAttacker] = MOVE_UNAVAILABLE;
                }

                if (!(gHitMarker & HITMARKER_FAINTED(gBattlerTarget)))
                    gLastHitBy[gBattlerTarget] = gBattlerAttacker;

                if (gHitMarker & HITMARKER_OBEYS && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
                {
                    if (gChosenMove == MOVE_UNAVAILABLE)
                    {
                        gLastLandedMoves[gBattlerTarget] = gChosenMove;
                    }
                    else
                    {
                        gLastLandedMoves[gBattlerTarget] = gCurrentMove;
                        GET_MOVE_TYPE(gCurrentMove, gLastHitByType[gBattlerTarget]);
                    }
                }
                else
                {
                    gLastLandedMoves[gBattlerTarget] = MOVE_UNAVAILABLE;
                }
            }
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_MIRROR_MOVE: // mirror move
            if (!(gAbsentBattlerFlags & gBitTable[gBattlerAttacker])
                && !(gBattleStruct->absentBattlerFlags & gBitTable[gBattlerAttacker])
                && gBattleMoves[originallyUsedMove].flags & FLAG_MIRROR_MOVE_AFFECTED
                && gHitMarker & HITMARKER_OBEYS
                && gBattlerAttacker != gBattlerTarget
                && !(gHitMarker & HITMARKER_FAINTED(gBattlerTarget))
                && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
            {
                u8 target, attacker;

                *(gBattleStruct->lastTakenMove + gBattlerTarget * 2 + 0) = gChosenMove;
                *(gBattleStruct->lastTakenMove + gBattlerTarget * 2 + 1) = gChosenMove >> 8;

                target = gBattlerTarget;
                attacker = gBattlerAttacker;
                *(attacker * 2 + target * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 0) = gChosenMove;

                target = gBattlerTarget;
                attacker = gBattlerAttacker;
                *(attacker * 2 + target * 8 + (u8 *)(gBattleStruct->lastTakenMoveFrom) + 1) = gChosenMove >> 8;
            }
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_NEXT_TARGET: // For moves hitting two opposing Pokémon.
            if (!(gHitMarker & HITMARKER_UNABLE_TO_USE_MOVE) && gBattleTypeFlags & BATTLE_TYPE_DOUBLE
                && !gProtectStructs[gBattlerAttacker].chargingTurn && gBattleMoves[gCurrentMove].target == MOVE_TARGET_BOTH
                && !(gHitMarker & HITMARKER_NO_ATTACKSTRING))
            {
                u8 battlerId = GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gBattlerTarget)));
                if (gBattleMons[battlerId].hp != 0)
                {
                    gBattlerTarget = battlerId;
                    gHitMarker |= HITMARKER_NO_ATTACKSTRING;
                    gBattleScripting.moveendState = 0;
                    MoveValuesCleanUp();
                    BattleScriptPush(gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect]);
                    gBattlescriptCurrInstr = BattleScript_FlushMessageBox;
                    return;
                }
                else
                {
                    gHitMarker |= HITMARKER_NO_ATTACKSTRING;
                }
            }
            gBattleScripting.moveendState++;
            break;
        case MOVEEND_COUNT:
            break;
        }

        if (endMode == 1 && effect == FALSE)
            gBattleScripting.moveendState = MOVEEND_COUNT;
        if (endMode == 2 && endState == gBattleScripting.moveendState)
            gBattleScripting.moveendState = MOVEEND_COUNT;

    } while (gBattleScripting.moveendState != MOVEEND_COUNT && effect == FALSE);

    if (gBattleScripting.moveendState == MOVEEND_COUNT && effect == FALSE)
        gBattlescriptCurrInstr += 3;
}

static void Cmd_typecalc2(void)
{
    u8 flags = 0;
    s32 i = 0;
    u8 moveType = gBattleMoves[gCurrentMove].type;

    if (gBattleMons[gBattlerTarget].ability == ABILITY_LEVITATE && moveType == TYPE_GROUND)
    {
        gLastUsedAbility = gBattleMons[gBattlerTarget].ability;
        gMoveResultFlags |= (MOVE_RESULT_MISSED | MOVE_RESULT_DOESNT_AFFECT_FOE);
        gLastLandedMoves[gBattlerTarget] = 0;
        gBattleCommunication[MISS_TYPE] = B_MSG_GROUND_MISS;
        RecordAbilityBattle(gBattlerTarget, gLastUsedAbility);
    }
    else
    {
        while (TYPE_EFFECT_ATK_TYPE(i) != TYPE_ENDTABLE)
        {
            if (TYPE_EFFECT_ATK_TYPE(i) == TYPE_FORESIGHT)
            {
                if (gBattleMons[gBattlerTarget].status2 & STATUS2_FORESIGHT)
                {
                    break;
                }
                else
                {
                    i += 3;
                    continue;
                }
            }

            if (TYPE_EFFECT_ATK_TYPE(i) == moveType)
            {
                // check type1
                if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[gBattlerTarget].types[0])
                {
                    if (TYPE_EFFECT_MULTIPLIER(i) == TYPE_MUL_NO_EFFECT)
                    {
                        gMoveResultFlags |= MOVE_RESULT_DOESNT_AFFECT_FOE;
                        break;
                    }
                    if (TYPE_EFFECT_MULTIPLIER(i) == TYPE_MUL_NOT_EFFECTIVE)
                    {
                        flags |= MOVE_RESULT_NOT_VERY_EFFECTIVE;
                    }
                    if (TYPE_EFFECT_MULTIPLIER(i) == TYPE_MUL_SUPER_EFFECTIVE)
                    {
                        flags |= MOVE_RESULT_SUPER_EFFECTIVE;
                    }
                }
                // check type2
                if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[gBattlerTarget].types[1])
                {
                    if (gBattleMons[gBattlerTarget].types[0] != gBattleMons[gBattlerTarget].types[1]
                        && TYPE_EFFECT_MULTIPLIER(i) == TYPE_MUL_NO_EFFECT)
                    {
                        gMoveResultFlags |= MOVE_RESULT_DOESNT_AFFECT_FOE;
                        break;
                    }
                    if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[gBattlerTarget].types[1]
                        && gBattleMons[gBattlerTarget].types[0] != gBattleMons[gBattlerTarget].types[1]
                        && TYPE_EFFECT_MULTIPLIER(i) == TYPE_MUL_NOT_EFFECTIVE)
                    {
                        flags |= MOVE_RESULT_NOT_VERY_EFFECTIVE;
                    }
                    if (TYPE_EFFECT_DEF_TYPE(i) == gBattleMons[gBattlerTarget].types[1]
                        && gBattleMons[gBattlerTarget].types[0] != gBattleMons[gBattlerTarget].types[1]
                        && TYPE_EFFECT_MULTIPLIER(i) == TYPE_MUL_SUPER_EFFECTIVE)
                    {
                        flags |= MOVE_RESULT_SUPER_EFFECTIVE;
                    }
                }
            }
            i += 3;
        }
    }

    if (gBattleMons[gBattlerTarget].ability == ABILITY_WONDER_GUARD
        && !(flags & MOVE_RESULT_NO_EFFECT)
        && AttacksThisTurn(gBattlerAttacker, gCurrentMove) == 2
        && (!(flags & MOVE_RESULT_SUPER_EFFECTIVE) || ((flags & (MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE)) == (MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE)))
        && gBattleMoves[gCurrentMove].power)
    {
        gLastUsedAbility = ABILITY_WONDER_GUARD;
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gLastLandedMoves[gBattlerTarget] = 0;
        gBattleCommunication[MISS_TYPE] = B_MSG_AVOIDED_DMG;
        RecordAbilityBattle(gBattlerTarget, gLastUsedAbility);
    }
    if (gMoveResultFlags & MOVE_RESULT_DOESNT_AFFECT_FOE)
        gProtectStructs[gBattlerAttacker].targetNotAffected = 1;

    gBattlescriptCurrInstr++;
}

static void Cmd_returnatktoball(void)
{
    gActiveBattler = gBattlerAttacker;
    if (!(gHitMarker & HITMARKER_FAINTED(gActiveBattler)))
    {
        BtlController_EmitReturnMonToBall(BUFFER_A, FALSE);
        MarkBattlerForControllerExec(gActiveBattler);
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_getswitchedmondata(void)
{
    if (gBattleControllerExecFlags)
        return;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    gBattlerPartyIndexes[gActiveBattler] = *(gBattleStruct->monToSwitchIntoId + gActiveBattler);

    BtlController_EmitGetMonData(BUFFER_A, REQUEST_ALL_BATTLE, gBitTable[gBattlerPartyIndexes[gActiveBattler]]);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_switchindataupdate(void)
{
    struct BattlePokemon oldData;
    s32 i;
    u8 *monData;

    if (gBattleControllerExecFlags)
        return;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    oldData = gBattleMons[gActiveBattler];
    monData = (u8 *)(&gBattleMons[gActiveBattler]);

    for (i = 0; i < sizeof(struct BattlePokemon); i++)
        monData[i] = gBattleBufferB[gActiveBattler][4 + i];

    gBattleMons[gActiveBattler].types[0] = gSpeciesInfo[gBattleMons[gActiveBattler].species].types[0];
    gBattleMons[gActiveBattler].types[1] = gSpeciesInfo[gBattleMons[gActiveBattler].species].types[1];
    gBattleMons[gActiveBattler].ability = GetAbilityBySpecies(gBattleMons[gActiveBattler].species, gBattleMons[gActiveBattler].abilityNum);

    // check knocked off item
    i = GetBattlerSide(gActiveBattler);
    if (gWishFutureKnock.knockedOffMons[i] & gBitTable[gBattlerPartyIndexes[gActiveBattler]])
    {
        gBattleMons[gActiveBattler].item = ITEM_NONE;
    }

    if (gBattleMoves[gCurrentMove].effect == EFFECT_BATON_PASS)
    {
        for (i = 0; i < NUM_BATTLE_STATS; i++)
        {
            gBattleMons[gActiveBattler].statStages[i] = oldData.statStages[i];
        }
        gBattleMons[gActiveBattler].status2 = oldData.status2;
    }

    SwitchInClearSetData();

    if (gBattleTypeFlags & BATTLE_TYPE_PALACE
        && gBattleMons[gActiveBattler].maxHP / 2 >= gBattleMons[gActiveBattler].hp
        && gBattleMons[gActiveBattler].hp != 0
        && !(gBattleMons[gActiveBattler].status1 & STATUS1_SLEEP))
    {
        gBattleStruct->palaceFlags |= gBitTable[gActiveBattler];
    }

    gBattleScripting.battler = gActiveBattler;

    PREPARE_MON_NICK_BUFFER(gBattleTextBuff1, gActiveBattler, gBattlerPartyIndexes[gActiveBattler]);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_switchinanim(void)
{
    if (gBattleControllerExecFlags)
        return;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT
        && !(gBattleTypeFlags & (BATTLE_TYPE_LINK
                                 | BATTLE_TYPE_EREADER_TRAINER
                                 | BATTLE_TYPE_RECORDED_LINK
                                 | BATTLE_TYPE_TRAINER_HILL
                                 | BATTLE_TYPE_FRONTIER)))
        HandleSetPokedexFlag(SpeciesToNationalPokedexNum(gBattleMons[gActiveBattler].species), FLAG_SET_SEEN, gBattleMons[gActiveBattler].personality);

    gAbsentBattlerFlags &= ~(gBitTable[gActiveBattler]);

    BtlController_EmitSwitchInAnim(BUFFER_A, gBattlerPartyIndexes[gActiveBattler], gBattlescriptCurrInstr[2]);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 3;

    if (gBattleTypeFlags & BATTLE_TYPE_ARENA)
        BattleArena_InitPoints();
}

static void Cmd_jumpifcantswitch(void)
{
    s32 i;
    s32 lastMonId;
    struct Pokemon *party;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1] & ~SWITCH_IGNORE_ESCAPE_PREVENTION);

    if (!(gBattlescriptCurrInstr[1] & SWITCH_IGNORE_ESCAPE_PREVENTION)
        && ((gBattleMons[gActiveBattler].status2 & (STATUS2_WRAPPED | STATUS2_ESCAPE_PREVENTION))
            || (gStatuses3[gActiveBattler] & STATUS3_ROOTED)))
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
    {
        if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT)
            party = gEnemyParty;
        else
            party = gPlayerParty;

        lastMonId = 0;
        if (gActiveBattler & 2)
            lastMonId = MULTI_PARTY_SIZE;

        for (i = lastMonId; i < lastMonId + MULTI_PARTY_SIZE; i++)
        {
            if (GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE
             && !GetMonData(&party[i], MON_DATA_IS_EGG)
             && GetMonData(&party[i], MON_DATA_HP) != 0
             && gBattlerPartyIndexes[gActiveBattler] != i)
                break;
        }

        if (i == lastMonId + MULTI_PARTY_SIZE)
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
        else
            gBattlescriptCurrInstr += 6;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_TOWER_LINK_MULTI)
        {
            if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
            {
                party = gPlayerParty;

                lastMonId = 0;
                if (GetLinkTrainerFlankId(GetBattlerMultiplayerId(gActiveBattler)) == TRUE)
                    lastMonId = MULTI_PARTY_SIZE;
            }
            else
            {
                party = gEnemyParty;

                if (gActiveBattler == 1)
                    lastMonId = 0;
                else
                    lastMonId = MULTI_PARTY_SIZE;
            }
        }
        else
        {
            if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT)
                party = gEnemyParty;
            else
                party = gPlayerParty;

            lastMonId = 0;
            if (GetLinkTrainerFlankId(GetBattlerMultiplayerId(gActiveBattler)) == TRUE)
                lastMonId = MULTI_PARTY_SIZE;
        }

        for (i = lastMonId; i < lastMonId + MULTI_PARTY_SIZE; i++)
        {
            if (GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE
             && !GetMonData(&party[i], MON_DATA_IS_EGG)
             && GetMonData(&party[i], MON_DATA_HP) != 0
             && gBattlerPartyIndexes[gActiveBattler] != i)
                break;
        }

        if (i == lastMonId + MULTI_PARTY_SIZE)
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
        else
            gBattlescriptCurrInstr += 6;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS && GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT)
    {
        party = gEnemyParty;

        lastMonId = 0;
        if (gActiveBattler == B_POSITION_OPPONENT_RIGHT)
            lastMonId = PARTY_SIZE / 2;

        for (i = lastMonId; i < lastMonId + (PARTY_SIZE / 2); i++)
        {
            if (GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE
             && !GetMonData(&party[i], MON_DATA_IS_EGG)
             && GetMonData(&party[i], MON_DATA_HP) != 0
             && gBattlerPartyIndexes[gActiveBattler] != i)
                break;
        }

        if (i == lastMonId + (PARTY_SIZE / 2))
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
        else
            gBattlescriptCurrInstr += 6;
    }
    else
    {
        u8 battlerIn1, battlerIn2;

        if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT)
        {
            battlerIn1 = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);

            if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
                battlerIn2 = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
            else
                battlerIn2 = battlerIn1;

            party = gEnemyParty;
        }
        else
        {
            battlerIn1 = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);

            if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
                battlerIn2 = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
            else
                battlerIn2 = battlerIn1;

            party = gPlayerParty;
        }

        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&party[i], MON_DATA_HP) != 0
             && GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE
             && !GetMonData(&party[i], MON_DATA_IS_EGG)
             && i != gBattlerPartyIndexes[battlerIn1] && i != gBattlerPartyIndexes[battlerIn2])
                break;
        }

        if (i == PARTY_SIZE)
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
        else
            gBattlescriptCurrInstr += 6;
    }
}

// Opens the party screen to choose a new Pokémon to send out.
// slotId is the Pokémon to replace.
// Note that this is not used by the Switch action, only replacing fainted Pokémon or Baton Pass
static void ChooseMonToSendOut(u8 slotId)
{
    *(gBattleStruct->battlerPartyIndexes + gActiveBattler) = gBattlerPartyIndexes[gActiveBattler];
    *(gBattleStruct->monToSwitchIntoId + gActiveBattler) = PARTY_SIZE;
    gBattleStruct->field_93 &= ~(gBitTable[gActiveBattler]);

    BtlController_EmitChoosePokemon(BUFFER_A, PARTY_ACTION_SEND_OUT, slotId, ABILITY_NONE, gBattleStruct->battlerPartyOrders[gActiveBattler]);
    MarkBattlerForControllerExec(gActiveBattler);
}

static void Cmd_openpartyscreen(void)
{
    u32 flags;
    u8 hitmarkerFaintBits;
    u8 battlerId;
    const u8 *jumpPtr;

    battlerId = 0;
    flags = 0;
    jumpPtr = T1_READ_PTR(gBattlescriptCurrInstr + 2);

    if (gBattlescriptCurrInstr[1] == BS_FAINTED_LINK_MULTIPLE_1)
    {
        if ((gBattleTypeFlags & BATTLE_TYPE_MULTI) || !(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
        {
            for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
            {
                if (gHitMarker & HITMARKER_FAINTED(gActiveBattler))
                {
                    if (HasNoMonsToSwitch(gActiveBattler, PARTY_SIZE, PARTY_SIZE))
                    {
                        gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                        gHitMarker &= ~HITMARKER_FAINTED(gActiveBattler);
                        BtlController_EmitLinkStandbyMsg(BUFFER_A, LINK_STANDBY_MSG_ONLY, FALSE);
                        MarkBattlerForControllerExec(gActiveBattler);
                    }
                    else if (!gSpecialStatuses[gActiveBattler].faintedHasReplacement)
                    {
                        ChooseMonToSendOut(PARTY_SIZE);
                        gSpecialStatuses[gActiveBattler].faintedHasReplacement = TRUE;
                    }
                }
                else
                {
                    BtlController_EmitLinkStandbyMsg(BUFFER_A, LINK_STANDBY_MSG_ONLY, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
            }
        }
        else if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
        {
            bool8 hasReplacement_0, hasReplacement_1, hasReplacement_2, hasReplacement_3;

            hitmarkerFaintBits = gHitMarker >> 28;

            if (gBitTable[0] & hitmarkerFaintBits)
            {
                gActiveBattler = 0;
                if (HasNoMonsToSwitch(gActiveBattler, PARTY_SIZE, PARTY_SIZE))
                {
                    gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                    gHitMarker &= ~HITMARKER_FAINTED(gActiveBattler);
                    BtlController_EmitCantSwitch(BUFFER_A);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
                else if (!gSpecialStatuses[gActiveBattler].faintedHasReplacement)
                {
                    ChooseMonToSendOut(gBattleStruct->monToSwitchIntoId[2]);
                    gSpecialStatuses[gActiveBattler].faintedHasReplacement = TRUE;
                }
                else
                {
                    BtlController_EmitLinkStandbyMsg(BUFFER_A, LINK_STANDBY_MSG_ONLY, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                    flags |= 1;
                }
            }
            if (gBitTable[2] & hitmarkerFaintBits && !(gBitTable[0] & hitmarkerFaintBits))
            {
                gActiveBattler = 2;
                if (HasNoMonsToSwitch(gActiveBattler, PARTY_SIZE, PARTY_SIZE))
                {
                    gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                    gHitMarker &= ~HITMARKER_FAINTED(gActiveBattler);
                    BtlController_EmitCantSwitch(BUFFER_A);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
                else if (!gSpecialStatuses[gActiveBattler].faintedHasReplacement)
                {
                    ChooseMonToSendOut(gBattleStruct->monToSwitchIntoId[0]);
                    gSpecialStatuses[gActiveBattler].faintedHasReplacement = TRUE;
                }
                else if (!(flags & 1))
                {
                    BtlController_EmitLinkStandbyMsg(BUFFER_A, LINK_STANDBY_MSG_ONLY, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
            }
            if (gBitTable[1] & hitmarkerFaintBits)
            {
                gActiveBattler = 1;
                if (HasNoMonsToSwitch(gActiveBattler, PARTY_SIZE, PARTY_SIZE))
                {
                    gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                    gHitMarker &= ~HITMARKER_FAINTED(gActiveBattler);
                    BtlController_EmitCantSwitch(BUFFER_A);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
                else if (!gSpecialStatuses[gActiveBattler].faintedHasReplacement)
                {
                    ChooseMonToSendOut(gBattleStruct->monToSwitchIntoId[3]);
                    gSpecialStatuses[gActiveBattler].faintedHasReplacement = TRUE;
                }
                else
                {
                    BtlController_EmitLinkStandbyMsg(BUFFER_A, LINK_STANDBY_MSG_ONLY, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                    flags |= 2;
                }
            }
            if (gBitTable[3] & hitmarkerFaintBits && !(gBitTable[1] & hitmarkerFaintBits))
            {
                gActiveBattler = 3;
                if (HasNoMonsToSwitch(gActiveBattler, PARTY_SIZE, PARTY_SIZE))
                {
                    gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                    gHitMarker &= ~HITMARKER_FAINTED(gActiveBattler);
                    BtlController_EmitCantSwitch(BUFFER_A);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
                else if (!gSpecialStatuses[gActiveBattler].faintedHasReplacement)
                {
                    ChooseMonToSendOut(gBattleStruct->monToSwitchIntoId[1]);
                    gSpecialStatuses[gActiveBattler].faintedHasReplacement = TRUE;
                }
                else if (!(flags & 2))
                {
                    BtlController_EmitLinkStandbyMsg(BUFFER_A, LINK_STANDBY_MSG_ONLY, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
            }

            hasReplacement_0 = gSpecialStatuses[0].faintedHasReplacement;
            if (!hasReplacement_0)
            {
                hasReplacement_2 = gSpecialStatuses[2].faintedHasReplacement;
                if (!hasReplacement_2 && hitmarkerFaintBits != 0)
                {
                    if (gAbsentBattlerFlags & gBitTable[0])
                        gActiveBattler = 2;
                    else
                        gActiveBattler = 0;

                    BtlController_EmitLinkStandbyMsg(BUFFER_A, LINK_STANDBY_MSG_ONLY, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                }

            }
            hasReplacement_1 = gSpecialStatuses[1].faintedHasReplacement;
            if (!hasReplacement_1)
            {
                hasReplacement_3 = gSpecialStatuses[3].faintedHasReplacement;
                if (!hasReplacement_3 && hitmarkerFaintBits != 0)
                {
                    if (gAbsentBattlerFlags & gBitTable[1])
                        gActiveBattler = 3;
                    else
                        gActiveBattler = 1;

                    BtlController_EmitLinkStandbyMsg(BUFFER_A, LINK_STANDBY_MSG_ONLY, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
            }
        }
        gBattlescriptCurrInstr += 6;
    }
    else if (gBattlescriptCurrInstr[1] == BS_FAINTED_LINK_MULTIPLE_2)
    {
        if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI))
        {
            if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
            {
                hitmarkerFaintBits = gHitMarker >> 28;
                if (gBitTable[2] & hitmarkerFaintBits && gBitTable[0] & hitmarkerFaintBits)
                {
                    gActiveBattler = 2;
                    if (HasNoMonsToSwitch(gActiveBattler, gBattleBufferB[0][1], PARTY_SIZE))
                    {
                        gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                        gHitMarker &= ~HITMARKER_FAINTED(gActiveBattler);
                        BtlController_EmitCantSwitch(BUFFER_A);
                        MarkBattlerForControllerExec(gActiveBattler);
                    }
                    else if (!gSpecialStatuses[gActiveBattler].faintedHasReplacement)
                    {
                        ChooseMonToSendOut(gBattleStruct->monToSwitchIntoId[0]);
                        gSpecialStatuses[gActiveBattler].faintedHasReplacement = TRUE;
                    }
                }
                if (gBitTable[3] & hitmarkerFaintBits && hitmarkerFaintBits & gBitTable[1])
                {
                    gActiveBattler = 3;
                    if (HasNoMonsToSwitch(gActiveBattler, gBattleBufferB[1][1], PARTY_SIZE))
                    {
                        gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                        gHitMarker &= ~HITMARKER_FAINTED(gActiveBattler);
                        BtlController_EmitCantSwitch(BUFFER_A);
                        MarkBattlerForControllerExec(gActiveBattler);
                    }
                    else if (!gSpecialStatuses[gActiveBattler].faintedHasReplacement)
                    {
                        ChooseMonToSendOut(gBattleStruct->monToSwitchIntoId[1]);
                        gSpecialStatuses[gActiveBattler].faintedHasReplacement = TRUE;
                    }
                }
                gBattlescriptCurrInstr += 6;
            }
            else
            {
                // Not multi or double battle
                gBattlescriptCurrInstr += 6;
            }
        }
        else
        {
            // Multi battle
            gBattlescriptCurrInstr += 6;
        }

        hitmarkerFaintBits = gHitMarker >> 28;

        gBattlerFainted = 0;
        while (!(gBitTable[gBattlerFainted] & hitmarkerFaintBits)
               && gBattlerFainted < gBattlersCount)
            gBattlerFainted++;

        if (gBattlerFainted == gBattlersCount)
            gBattlescriptCurrInstr = jumpPtr;
    }
    else
    {
        if (gBattlescriptCurrInstr[1] & PARTY_SCREEN_OPTIONAL)
            hitmarkerFaintBits = PARTY_ACTION_CHOOSE_MON; // Used here as the caseId for the EmitChoose function.
        else
            hitmarkerFaintBits = PARTY_ACTION_SEND_OUT;

        battlerId = GetBattlerForBattleScript(gBattlescriptCurrInstr[1] & ~PARTY_SCREEN_OPTIONAL);
        if (gSpecialStatuses[battlerId].faintedHasReplacement)
        {
            gBattlescriptCurrInstr += 6;
        }
        else if (HasNoMonsToSwitch(battlerId, PARTY_SIZE, PARTY_SIZE))
        {
            gActiveBattler = battlerId;
            gAbsentBattlerFlags |= gBitTable[gActiveBattler];
            gHitMarker &= ~HITMARKER_FAINTED(gActiveBattler);
            gBattlescriptCurrInstr = jumpPtr;
        }
        else
        {
            gActiveBattler = battlerId;
            *(gBattleStruct->battlerPartyIndexes + gActiveBattler) = gBattlerPartyIndexes[gActiveBattler];
            *(gBattleStruct->monToSwitchIntoId + gActiveBattler) = PARTY_SIZE;
            gBattleStruct->field_93 &= ~(gBitTable[gActiveBattler]);

            BtlController_EmitChoosePokemon(BUFFER_A, hitmarkerFaintBits, *(gBattleStruct->monToSwitchIntoId + BATTLE_PARTNER(gActiveBattler)), ABILITY_NONE, gBattleStruct->battlerPartyOrders[gActiveBattler]);
            MarkBattlerForControllerExec(gActiveBattler);

            gBattlescriptCurrInstr += 6;

            if (GetBattlerPosition(gActiveBattler) == B_POSITION_PLAYER_LEFT && gBattleResults.playerSwitchesCounter < 255)
                gBattleResults.playerSwitchesCounter++;

            if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
            {
                for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
                {
                    if (gActiveBattler != battlerId)
                    {
                        BtlController_EmitLinkStandbyMsg(BUFFER_A, LINK_STANDBY_MSG_ONLY, FALSE);
                        MarkBattlerForControllerExec(gActiveBattler);
                    }
                }
            }
            else
            {
                gActiveBattler = GetBattlerAtPosition(BATTLE_OPPOSITE(GetBattlerPosition(battlerId)));
                if (gAbsentBattlerFlags & gBitTable[gActiveBattler])
                    gActiveBattler ^= BIT_FLANK;

                BtlController_EmitLinkStandbyMsg(BUFFER_A, LINK_STANDBY_MSG_ONLY, FALSE);
                MarkBattlerForControllerExec(gActiveBattler);
            }
        }
    }
}

static void Cmd_switchhandleorder(void)
{
    s32 i;
    if (gBattleControllerExecFlags)
        return;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    switch (gBattlescriptCurrInstr[2])
    {
    case 0:
        for (i = 0; i < gBattlersCount; i++)
        {
            if (gBattleBufferB[i][0] == CONTROLLER_CHOSENMONRETURNVALUE)
            {
                *(gBattleStruct->monToSwitchIntoId + i) = gBattleBufferB[i][1];
                if (!(gBattleStruct->field_93 & gBitTable[i]))
                {
                    RecordedBattle_SetBattlerAction(i, gBattleBufferB[i][1]);
                    gBattleStruct->field_93 |= gBitTable[i];
                }
            }
        }
        break;
    case 1:
        if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI))
            SwitchPartyOrder(gActiveBattler);
        break;
    case 2:
        if (!(gBattleStruct->field_93 & gBitTable[gActiveBattler]))
        {
            RecordedBattle_SetBattlerAction(gActiveBattler, gBattleBufferB[gActiveBattler][1]);
            gBattleStruct->field_93 |= gBitTable[gActiveBattler];
        }
        // fall through
    case 3:
        gBattleCommunication[0] = gBattleBufferB[gActiveBattler][1];
        *(gBattleStruct->monToSwitchIntoId + gActiveBattler) = gBattleBufferB[gActiveBattler][1];

        if (gBattleTypeFlags & BATTLE_TYPE_LINK && gBattleTypeFlags & BATTLE_TYPE_MULTI)
        {
            *(gActiveBattler * 3 + (u8 *)(gBattleStruct->battlerPartyOrders) + 0) &= 0xF;
            *(gActiveBattler * 3 + (u8 *)(gBattleStruct->battlerPartyOrders) + 0) |= (gBattleBufferB[gActiveBattler][2] & 0xF0);
            *(gActiveBattler * 3 + (u8 *)(gBattleStruct->battlerPartyOrders) + 1) = gBattleBufferB[gActiveBattler][3];

            *((BATTLE_PARTNER(gActiveBattler)) * 3 + (u8 *)(gBattleStruct->battlerPartyOrders) + 0) &= (0xF0);
            *((BATTLE_PARTNER(gActiveBattler)) * 3 + (u8 *)(gBattleStruct->battlerPartyOrders) + 0) |= (gBattleBufferB[gActiveBattler][2] & 0xF0) >> 4;
            *((BATTLE_PARTNER(gActiveBattler)) * 3 + (u8 *)(gBattleStruct->battlerPartyOrders) + 2) = gBattleBufferB[gActiveBattler][3];
        }
        else if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
        {
            SwitchPartyOrderInGameMulti(gActiveBattler, *(gBattleStruct->monToSwitchIntoId + gActiveBattler));
        }
        else
        {
            SwitchPartyOrder(gActiveBattler);
        }

        PREPARE_SPECIES_BUFFER(gBattleTextBuff1, gBattleMons[gBattlerAttacker].species)
        PREPARE_MON_NICK_BUFFER(gBattleTextBuff2, gActiveBattler, gBattleBufferB[gActiveBattler][1])

        break;
    }

    gBattlescriptCurrInstr += 3;
}

static void Cmd_switchineffects(void)
{
    s32 i;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    UpdateSentPokesToOpponentValue(gActiveBattler);

    gHitMarker &= ~HITMARKER_FAINTED(gActiveBattler);
    gSpecialStatuses[gActiveBattler].faintedHasReplacement = FALSE;

    if (!(gSideStatuses[GetBattlerSide(gActiveBattler)] & SIDE_STATUS_SPIKES_DAMAGED)
        && (gSideStatuses[GetBattlerSide(gActiveBattler)] & SIDE_STATUS_SPIKES)
        && !IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_FLYING)
        && gBattleMons[gActiveBattler].ability != ABILITY_LEVITATE)
    {
        u8 spikesDmg;

        gSideStatuses[GetBattlerSide(gActiveBattler)] |= SIDE_STATUS_SPIKES_DAMAGED;

        gBattleMons[gActiveBattler].status2 &= ~STATUS2_DESTINY_BOND;
        gHitMarker &= ~HITMARKER_DESTINYBOND;

        spikesDmg = (5 - gSideTimers[GetBattlerSide(gActiveBattler)].spikesAmount) * 2;
        gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / (spikesDmg);
        if (gBattleMoveDamage == 0)
            gBattleMoveDamage = 1;

        gBattleScripting.battler = gActiveBattler;
        BattleScriptPushCursor();

        if (gBattlescriptCurrInstr[1] == BS_TARGET)
            gBattlescriptCurrInstr = BattleScript_SpikesOnTarget;
        else if (gBattlescriptCurrInstr[1] == BS_ATTACKER)
            gBattlescriptCurrInstr = BattleScript_SpikesOnAttacker;
        else
            gBattlescriptCurrInstr = BattleScript_SpikesOnFaintedBattler;
    }
    else
    {
        // There is a hack here to ensure the truant counter will be 0 when the battler's next turn starts.
        // The truant counter is not updated in the case where a mon switches in after a lost judgment in the battle arena.
        if (gBattleMons[gActiveBattler].ability == ABILITY_TRUANT && !gDisableStructs[gActiveBattler].truantSwitchInHack)
            gDisableStructs[gActiveBattler].truantCounter = 1;

        gDisableStructs[gActiveBattler].truantSwitchInHack = 0;

        if (!AbilityBattleEffects(ABILITYEFFECT_ON_SWITCHIN, gActiveBattler, 0, 0, 0)
            && !ItemBattleEffects(ITEMEFFECT_ON_SWITCH_IN, gActiveBattler, FALSE))
        {
            gSideStatuses[GetBattlerSide(gActiveBattler)] &= ~SIDE_STATUS_SPIKES_DAMAGED;

            for (i = 0; i < gBattlersCount; i++)
            {
                if (gBattlerByTurnOrder[i] == gActiveBattler)
                    gActionsByTurnOrder[i] = B_ACTION_CANCEL_PARTNER;
            }

            for (i = 0; i < gBattlersCount; i++)
            {
                u16 *hpOnSwitchout = &gBattleStruct->hpOnSwitchout[GetBattlerSide(i)];
                *hpOnSwitchout = gBattleMons[i].hp;
            }

            if (gBattlescriptCurrInstr[1] == BS_FAINTED_LINK_MULTIPLE_1)
            {
                u32 hitmarkerFaintBits = gHitMarker >> 28;

                gBattlerFainted++;
                while (TRUE)
                {
                    if (hitmarkerFaintBits & gBitTable[gBattlerFainted] && !(gAbsentBattlerFlags & gBitTable[gBattlerFainted]))
                        break;
                    if (gBattlerFainted >= gBattlersCount)
                        break;
                    gBattlerFainted++;
                }
            }
            gBattlescriptCurrInstr += 2;
        }
    }
}

static void Cmd_trainerslidein(void)
{
    gActiveBattler = GetBattlerAtPosition(gBattlescriptCurrInstr[1]);
    BtlController_EmitTrainerSlide(BUFFER_A);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_playse(void)
{
    gActiveBattler = gBattlerAttacker;
    BtlController_EmitPlaySE(BUFFER_A, T2_READ_16(gBattlescriptCurrInstr + 1));
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 3;
}

static void Cmd_fanfare(void)
{
    gActiveBattler = gBattlerAttacker;
    BtlController_EmitPlayFanfareOrBGM(BUFFER_A, T2_READ_16(gBattlescriptCurrInstr + 1), FALSE);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 3;
}

static void Cmd_playfaintcry(void)
{
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    BtlController_EmitFaintingCry(BUFFER_A);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_endlinkbattle(void)
{
    gActiveBattler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
    BtlController_EmitEndLinkBattle(BUFFER_A, gBattleOutcome);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 1;
}

static void Cmd_returntoball(void)
{
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    BtlController_EmitReturnMonToBall(BUFFER_A, TRUE);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_handlelearnnewmove(void)
{
    const u8 *learnedMovePtr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    const u8 *nothingToLearnPtr = T1_READ_PTR(gBattlescriptCurrInstr + 5);

    u16 learnMove = MonTryLearningNewMove(&gPlayerParty[gBattleStruct->expGetterMonId], gBattlescriptCurrInstr[9]);
    while (learnMove == MON_ALREADY_KNOWS_MOVE)
        learnMove = MonTryLearningNewMove(&gPlayerParty[gBattleStruct->expGetterMonId], FALSE);

    if (learnMove == MOVE_NONE)
    {
        gBattlescriptCurrInstr = nothingToLearnPtr;
    }
    else if (learnMove == MON_HAS_MAX_MOVES)
    {
        gBattlescriptCurrInstr += 10;
    }
    else
    {
        gActiveBattler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);

        if (gBattlerPartyIndexes[gActiveBattler] == gBattleStruct->expGetterMonId
            && !(gBattleMons[gActiveBattler].status2 & STATUS2_TRANSFORMED))
        {
            GiveMoveToBattleMon(&gBattleMons[gActiveBattler], learnMove);
        }
        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
        {
            gActiveBattler = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
            if (gBattlerPartyIndexes[gActiveBattler] == gBattleStruct->expGetterMonId
                && !(gBattleMons[gActiveBattler].status2 & STATUS2_TRANSFORMED))
            {
                GiveMoveToBattleMon(&gBattleMons[gActiveBattler], learnMove);
            }
        }

        gBattlescriptCurrInstr = learnedMovePtr;
    }
}

static void Cmd_yesnoboxlearnmove(void)
{
    gActiveBattler = 0;

    switch (gBattleScripting.learnMoveState)
    {
    case 0:
        HandleBattleWindow(YESNOBOX_X_Y, 0);
        BattlePutTextOnWindow(gText_BattleYesNoChoice, B_WIN_YESNO);
        gBattleScripting.learnMoveState++;
        gBattleCommunication[CURSOR_POSITION] = 0;
        BattleCreateYesNoCursorAt(0);
        break;
    case 1:
        if (JOY_NEW(DPAD_UP) && gBattleCommunication[CURSOR_POSITION] != 0)
        {
            PlaySE(SE_SELECT);
            BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
            gBattleCommunication[CURSOR_POSITION] = 0;
            BattleCreateYesNoCursorAt(0);
        }
        if (JOY_NEW(DPAD_DOWN) && gBattleCommunication[CURSOR_POSITION] == 0)
        {
            PlaySE(SE_SELECT);
            BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
            gBattleCommunication[CURSOR_POSITION] = 1;
            BattleCreateYesNoCursorAt(1);
        }
        if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            if (gBattleCommunication[1] == 0)
            {
                HandleBattleWindow(YESNOBOX_X_Y, WINDOW_CLEAR);
                BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
                gBattleScripting.learnMoveState++;
            }
            else
            {
                gBattleScripting.learnMoveState = 5;
            }
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            gBattleScripting.learnMoveState = 5;
        }
        break;
    case 2:
        if (!gPaletteFade.active)
        {
            FreeAllWindowBuffers();
            ShowSelectMovePokemonSummaryScreen(gPlayerParty, gBattleStruct->expGetterMonId, gPlayerPartyCount - 1, ReshowBattleScreenAfterMenu, gMoveToLearn);
            gBattleScripting.learnMoveState++;
        }
        break;
    case 3:
        if (!gPaletteFade.active && gMain.callback2 == BattleMainCB2)
        {
            gBattleScripting.learnMoveState++;
        }
        break;
    case 4:
        if (!gPaletteFade.active && gMain.callback2 == BattleMainCB2)
        {
            u8 movePosition = GetMoveSlotToReplace();
            if (movePosition == MAX_MON_MOVES)
            {
                gBattleScripting.learnMoveState = 5;
            }
            else
            {
                u16 moveId = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_MOVE1 + movePosition);
                if (IsHMMove2(moveId))
                {
                    PrepareStringBattle(STRINGID_HMMOVESCANTBEFORGOTTEN, gActiveBattler);
                    gBattleScripting.learnMoveState = 6;
                }
                else
                {
                    gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);

                    PREPARE_MOVE_BUFFER(gBattleTextBuff2, moveId)

                    RemoveMonPPBonus(&gPlayerParty[gBattleStruct->expGetterMonId], movePosition);
                    SetMonMoveSlot(&gPlayerParty[gBattleStruct->expGetterMonId], gMoveToLearn, movePosition);

                    if (gBattlerPartyIndexes[0] == gBattleStruct->expGetterMonId && MOVE_IS_PERMANENT(0, movePosition))
                    {
                        RemoveBattleMonPPBonus(&gBattleMons[0], movePosition);
                        SetBattleMonMoveSlot(&gBattleMons[0], gMoveToLearn, movePosition);
                    }
                    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
                        && gBattlerPartyIndexes[2] == gBattleStruct->expGetterMonId
                        && MOVE_IS_PERMANENT(2, movePosition))
                    {
                        RemoveBattleMonPPBonus(&gBattleMons[2], movePosition);
                        SetBattleMonMoveSlot(&gBattleMons[2], gMoveToLearn, movePosition);
                    }
                }
            }
        }
        break;
    case 5:
        HandleBattleWindow(YESNOBOX_X_Y, WINDOW_CLEAR);
        gBattlescriptCurrInstr += 5;
        break;
    case 6:
        if (gBattleControllerExecFlags == 0)
        {
            gBattleScripting.learnMoveState = 2;
        }
        break;
    }
}

static void Cmd_yesnoboxstoplearningmove(void)
{
    switch (gBattleScripting.learnMoveState)
    {
    case 0:
        HandleBattleWindow(YESNOBOX_X_Y, 0);
        BattlePutTextOnWindow(gText_BattleYesNoChoice, B_WIN_YESNO);
        gBattleScripting.learnMoveState++;
        gBattleCommunication[CURSOR_POSITION] = 0;
        BattleCreateYesNoCursorAt(0);
        break;
    case 1:
        if (JOY_NEW(DPAD_UP) && gBattleCommunication[CURSOR_POSITION] != 0)
        {
            PlaySE(SE_SELECT);
            BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
            gBattleCommunication[CURSOR_POSITION] = 0;
            BattleCreateYesNoCursorAt(0);
        }
        if (JOY_NEW(DPAD_DOWN) && gBattleCommunication[CURSOR_POSITION] == 0)
        {
            PlaySE(SE_SELECT);
            BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
            gBattleCommunication[CURSOR_POSITION] = 1;
            BattleCreateYesNoCursorAt(1);
        }
        if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);

            if (gBattleCommunication[1] != 0)
                gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
            else
                gBattlescriptCurrInstr += 5;

            HandleBattleWindow(YESNOBOX_X_Y, WINDOW_CLEAR);
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
            HandleBattleWindow(YESNOBOX_X_Y, WINDOW_CLEAR);
        }
        break;
    }
}

static void Cmd_hitanimation(void)
{
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
    {
        gBattlescriptCurrInstr += 2;
    }
    else if (!(gHitMarker & HITMARKER_IGNORE_SUBSTITUTE) || !(gBattleMons[gActiveBattler].status2 & STATUS2_SUBSTITUTE) || gDisableStructs[gActiveBattler].substituteHP == 0)
    {
        BtlController_EmitHitAnimation(BUFFER_A);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 2;
    }
    else
    {
        gBattlescriptCurrInstr += 2;
    }
}

static u32 GetTrainerMoneyToGive(u16 trainerId)
{
    u32 i = 0;
    u32 lastMonLevel = 0;
    u32 moneyReward;

    if (trainerId == TRAINER_SECRET_BASE)
    {
        moneyReward = 20 * gBattleResources->secretBase->party.levels[0] * gBattleStruct->moneyMultiplier;
    }
    else
    {
        switch (gTrainers[trainerId].partyFlags)
        {
        case 0:
            {
                const struct TrainerMonNoItemDefaultMoves *party = gTrainers[trainerId].party.NoItemDefaultMoves;
                lastMonLevel = party[gTrainers[trainerId].partySize - 1].lvl;
            }
            break;
        case F_TRAINER_PARTY_CUSTOM_MOVESET:
            {
                const struct TrainerMonNoItemCustomMoves *party = gTrainers[trainerId].party.NoItemCustomMoves;
                lastMonLevel = party[gTrainers[trainerId].partySize - 1].lvl;
            }
            break;
        case F_TRAINER_PARTY_HELD_ITEM:
            {
                const struct TrainerMonItemDefaultMoves *party = gTrainers[trainerId].party.ItemDefaultMoves;
                lastMonLevel = party[gTrainers[trainerId].partySize - 1].lvl;
            }
            break;
        case F_TRAINER_PARTY_CUSTOM_MOVESET | F_TRAINER_PARTY_HELD_ITEM:
            {
                const struct TrainerMonItemCustomMoves *party = gTrainers[trainerId].party.ItemCustomMoves;
                lastMonLevel = party[gTrainers[trainerId].partySize - 1].lvl;
            }
            break;
        }

        for (; gTrainerMoneyTable[i].classId != 0xFF; i++)
        {
            if (gTrainerMoneyTable[i].classId == gTrainers[trainerId].trainerClass)
                break;
        }

        if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
            moneyReward = 4 * lastMonLevel * gBattleStruct->moneyMultiplier * gTrainerMoneyTable[i].value;
        else if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
            moneyReward = 4 * lastMonLevel * gBattleStruct->moneyMultiplier * 2 * gTrainerMoneyTable[i].value;
        else
            moneyReward = 4 * lastMonLevel * gBattleStruct->moneyMultiplier * gTrainerMoneyTable[i].value;
    }

    return moneyReward;
}

static void Cmd_getmoneyreward(void)
{
    u32 moneyReward = GetTrainerMoneyToGive(gTrainerBattleOpponent_A);
    if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
        moneyReward += GetTrainerMoneyToGive(gTrainerBattleOpponent_B);

    AddMoney(&gSaveBlock1Ptr->money, moneyReward);
    PREPARE_WORD_NUMBER_BUFFER(gBattleTextBuff1, 5, moneyReward);

    gBattlescriptCurrInstr++;
}

// Command is never used
static void Cmd_updatebattlermoves(void)
{
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    switch (gBattleCommunication[0])
    {
    case 0:
        BtlController_EmitGetMonData(BUFFER_A, REQUEST_ALL_BATTLE, 0);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattleCommunication[0]++;
        break;
    case 1:
         if (gBattleControllerExecFlags == 0)
         {
            s32 i;
            struct BattlePokemon *bufferPoke = (struct BattlePokemon *) &gBattleBufferB[gActiveBattler][4];
            for (i = 0; i < MAX_MON_MOVES; i++)
            {
                gBattleMons[gActiveBattler].moves[i] = bufferPoke->moves[i];
                gBattleMons[gActiveBattler].pp[i] = bufferPoke->pp[i];
            }
            gBattlescriptCurrInstr += 2;
         }
         break;
    }
}

static void Cmd_swapattackerwithtarget(void)
{
    gActiveBattler = gBattlerAttacker;
    gBattlerAttacker = gBattlerTarget;
    gBattlerTarget = gActiveBattler;

    if (gHitMarker & HITMARKER_SWAP_ATTACKER_TARGET)
        gHitMarker &= ~HITMARKER_SWAP_ATTACKER_TARGET;
    else
        gHitMarker |= HITMARKER_SWAP_ATTACKER_TARGET;

    gBattlescriptCurrInstr++;
}

static void Cmd_incrementgamestat(void)
{
    if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
        IncrementGameStat(gBattlescriptCurrInstr[1]);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_drawpartystatussummary(void)
{
    s32 i;
    struct Pokemon *party;
    struct HpAndStatus hpStatuses[PARTY_SIZE];

    if (gBattleControllerExecFlags)
        return;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
        party = gPlayerParty;
    else
        party = gEnemyParty;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&party[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_NONE
            || GetMonData(&party[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_EGG)
        {
            hpStatuses[i].hp = 0xFFFF;
            hpStatuses[i].status = 0;
        }
        else
        {
            hpStatuses[i].hp = GetMonData(&party[i], MON_DATA_HP);
            hpStatuses[i].status = GetMonData(&party[i], MON_DATA_STATUS);
        }
    }

    BtlController_EmitDrawPartyStatusSummary(BUFFER_A, hpStatuses, 1);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_hidepartystatussummary(void)
{
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    BtlController_EmitHidePartyStatusSummary(BUFFER_A);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_jumptocalledmove(void)
{
    if (gBattlescriptCurrInstr[1])
        gCurrentMove = gCalledMove;
    else
        gChosenMove = gCurrentMove = gCalledMove;

    gBattlescriptCurrInstr = gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect];
}

static void Cmd_statusanimation(void)
{
    if (gBattleControllerExecFlags == 0)
    {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        if (!(gStatuses3[gActiveBattler] & STATUS3_SEMI_INVULNERABLE)
            && gDisableStructs[gActiveBattler].substituteHP == 0
            && !(gHitMarker & HITMARKER_NO_ANIMATIONS))
        {
            BtlController_EmitStatusAnimation(BUFFER_A, FALSE, gBattleMons[gActiveBattler].status1);
            MarkBattlerForControllerExec(gActiveBattler);
        }
        gBattlescriptCurrInstr += 2;
    }
}

static void Cmd_status2animation(void)
{
    u32 wantedToAnimate;

    if (gBattleControllerExecFlags == 0)
    {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        wantedToAnimate = T1_READ_32(gBattlescriptCurrInstr + 2);
        if (!(gStatuses3[gActiveBattler] & STATUS3_SEMI_INVULNERABLE)
            && gDisableStructs[gActiveBattler].substituteHP == 0
            && !(gHitMarker & HITMARKER_NO_ANIMATIONS))
        {
            BtlController_EmitStatusAnimation(BUFFER_A, TRUE, gBattleMons[gActiveBattler].status2 & wantedToAnimate);
            MarkBattlerForControllerExec(gActiveBattler);
        }
        gBattlescriptCurrInstr += 6;
    }
}

static void Cmd_chosenstatusanimation(void)
{
    u32 wantedStatus;

    if (gBattleControllerExecFlags == 0)
    {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        wantedStatus = T1_READ_32(gBattlescriptCurrInstr + 3);
        if (!(gStatuses3[gActiveBattler] & STATUS3_SEMI_INVULNERABLE)
            && gDisableStructs[gActiveBattler].substituteHP == 0
            && !(gHitMarker & HITMARKER_NO_ANIMATIONS))
        {
            BtlController_EmitStatusAnimation(BUFFER_A, gBattlescriptCurrInstr[2], wantedStatus);
            MarkBattlerForControllerExec(gActiveBattler);
        }
        gBattlescriptCurrInstr += 7;
    }
}

static void Cmd_yesnobox(void)
{
    switch (gBattleCommunication[0])
    {
    case 0:
        HandleBattleWindow(YESNOBOX_X_Y, 0);
        BattlePutTextOnWindow(gText_BattleYesNoChoice, B_WIN_YESNO);
        gBattleCommunication[0]++;
        gBattleCommunication[CURSOR_POSITION] = 0;
        BattleCreateYesNoCursorAt(0);
        break;
    case 1:
        if (JOY_NEW(DPAD_UP) && gBattleCommunication[CURSOR_POSITION] != 0)
        {
            PlaySE(SE_SELECT);
            BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
            gBattleCommunication[CURSOR_POSITION] = 0;
            BattleCreateYesNoCursorAt(0);
        }
        if (JOY_NEW(DPAD_DOWN) && gBattleCommunication[CURSOR_POSITION] == 0)
        {
            PlaySE(SE_SELECT);
            BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
            gBattleCommunication[CURSOR_POSITION] = 1;
            BattleCreateYesNoCursorAt(1);
        }
        if (JOY_NEW(B_BUTTON))
        {
            gBattleCommunication[CURSOR_POSITION] = 1;
            PlaySE(SE_SELECT);
            HandleBattleWindow(YESNOBOX_X_Y, WINDOW_CLEAR);
            gBattlescriptCurrInstr++;
        }
        else if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            HandleBattleWindow(YESNOBOX_X_Y, WINDOW_CLEAR);
            gBattlescriptCurrInstr++;
        }
        break;
    }
}

static void Cmd_cancelallactions(void)
{
    s32 i;

    for (i = 0; i < gBattlersCount; i++)
        gActionsByTurnOrder[i] = B_ACTION_CANCEL_PARTNER;

    gBattlescriptCurrInstr++;
}

// The same as adjustnormaldamage, except there's no random damage multiplier.
static void Cmd_adjustsetdamage(void)
{
    u8 holdEffect, param;

    if (gBattleMons[gBattlerTarget].item == ITEM_ENIGMA_BERRY)
    {
        holdEffect = gEnigmaBerries[gBattlerTarget].holdEffect;
        param = gEnigmaBerries[gBattlerTarget].holdEffectParam;
    }
    else
    {
        holdEffect = ItemId_GetHoldEffect(gBattleMons[gBattlerTarget].item);
        param = ItemId_GetHoldEffectParam(gBattleMons[gBattlerTarget].item);
    }

    gPotentialItemEffectBattler = gBattlerTarget;

    if (holdEffect == HOLD_EFFECT_FOCUS_BAND && (Random() % 100) < param)
    {
        RecordItemEffectBattle(gBattlerTarget, holdEffect);
        gSpecialStatuses[gBattlerTarget].focusBanded = 1;
    }
    if (!(gBattleMons[gBattlerTarget].status2 & STATUS2_SUBSTITUTE)
     && (gBattleMoves[gCurrentMove].effect == EFFECT_FALSE_SWIPE || gProtectStructs[gBattlerTarget].endured || gSpecialStatuses[gBattlerTarget].focusBanded)
     && gBattleMons[gBattlerTarget].hp <= gBattleMoveDamage)
    {
        gBattleMoveDamage = gBattleMons[gBattlerTarget].hp - 1;
        if (gProtectStructs[gBattlerTarget].endured)
        {
            gMoveResultFlags |= MOVE_RESULT_FOE_ENDURED;
        }
        else if (gSpecialStatuses[gBattlerTarget].focusBanded)
        {
            gMoveResultFlags |= MOVE_RESULT_FOE_HUNG_ON;
            gLastUsedItem = gBattleMons[gBattlerTarget].item;
        }
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_removeitem(void)
{
    u16 *usedHeldItem;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    usedHeldItem = &gBattleStruct->usedHeldItems[gActiveBattler];
    *usedHeldItem = gBattleMons[gActiveBattler].item;
    gBattleMons[gActiveBattler].item = ITEM_NONE;

    BtlController_EmitSetMonData(BUFFER_A, REQUEST_HELDITEM_BATTLE, 0, sizeof(gBattleMons[gActiveBattler].item), &gBattleMons[gActiveBattler].item);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_atknameinbuff1(void)
{
    PREPARE_MON_NICK_BUFFER(gBattleTextBuff1, gBattlerAttacker, gBattlerPartyIndexes[gBattlerAttacker])

    gBattlescriptCurrInstr++;
}

static void Cmd_drawlvlupbox(void)
{
    if (gBattleScripting.drawlvlupboxState == 0)
    {
        // If the Pokémon getting exp is not in-battle then
        // slide out a banner with their name and icon on it.
        // Otherwise skip ahead.
        if (IsMonGettingExpSentOut())
            gBattleScripting.drawlvlupboxState = 3;
        else
            gBattleScripting.drawlvlupboxState = 1;
    }

    switch (gBattleScripting.drawlvlupboxState)
    {
    case 1:
        // Start level up banner
        gBattle_BG2_Y = 96;
        SetBgAttribute(2, BG_ATTR_PRIORITY, 0);
        ShowBg(2);
        InitLevelUpBanner();
        gBattleScripting.drawlvlupboxState = 2;
        break;
    case 2:
        if (!SlideInLevelUpBanner())
            gBattleScripting.drawlvlupboxState = 3;
        break;
    case 3:
        // Init level up box
        gBattle_BG1_X = 0;
        gBattle_BG1_Y = 256;
        SetBgAttribute(0, BG_ATTR_PRIORITY, 1);
        SetBgAttribute(1, BG_ATTR_PRIORITY, 0);
        ShowBg(0);
        ShowBg(1);
        HandleBattleWindow(18, 7, 29, 19, WINDOW_BG1);
        gBattleScripting.drawlvlupboxState = 4;
        break;
    case 4:
        // Draw page 1 of level up box
        DrawLevelUpWindow1();
        PutWindowTilemap(B_WIN_LEVEL_UP_BOX);
        CopyWindowToVram(B_WIN_LEVEL_UP_BOX, COPYWIN_FULL);
        gBattleScripting.drawlvlupboxState++;
        break;
    case 5:
    case 7:
        // Wait for draw after each page
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            gBattle_BG1_Y = 0;
            gBattleScripting.drawlvlupboxState++;
        }
        break;
    case 6:
        if (gMain.newKeys != 0)
        {
            // Draw page 2 of level up box
            PlaySE(SE_SELECT);
            DrawLevelUpWindow2();
            CopyWindowToVram(B_WIN_LEVEL_UP_BOX, COPYWIN_GFX);
            gBattleScripting.drawlvlupboxState++;
        }
        break;
    case 8:
        if (gMain.newKeys != 0)
        {
            // Close level up box
            PlaySE(SE_SELECT);
            HandleBattleWindow(18, 7, 29, 19, WINDOW_BG1 | WINDOW_CLEAR);
            gBattleScripting.drawlvlupboxState++;
        }
        break;
    case 9:
        if (!SlideOutLevelUpBanner())
        {
            ClearWindowTilemap(B_WIN_LEVEL_UP_BANNER);
            CopyWindowToVram(B_WIN_LEVEL_UP_BANNER, COPYWIN_MAP);

            ClearWindowTilemap(B_WIN_LEVEL_UP_BOX);
            CopyWindowToVram(B_WIN_LEVEL_UP_BOX, COPYWIN_MAP);

            SetBgAttribute(2, BG_ATTR_PRIORITY, 2);
            ShowBg(2);

            gBattleScripting.drawlvlupboxState = 10;
        }
        break;
    case 10:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            SetBgAttribute(0, BG_ATTR_PRIORITY, 0);
            SetBgAttribute(1, BG_ATTR_PRIORITY, 1);
            ShowBg(0);
            ShowBg(1);
            gBattlescriptCurrInstr++;
        }
        break;
    }
}

static void DrawLevelUpWindow1(void)
{
    u16 currStats[NUM_STATS];

    GetMonLevelUpWindowStats(&gPlayerParty[gBattleStruct->expGetterMonId], currStats);
    DrawLevelUpWindowPg1(B_WIN_LEVEL_UP_BOX, gBattleResources->beforeLvlUp->stats, currStats, TEXT_DYNAMIC_COLOR_5, TEXT_DYNAMIC_COLOR_4, TEXT_DYNAMIC_COLOR_6);
}

static void DrawLevelUpWindow2(void)
{
    u16 currStats[NUM_STATS];

    GetMonLevelUpWindowStats(&gPlayerParty[gBattleStruct->expGetterMonId], currStats);
    DrawLevelUpWindowPg2(B_WIN_LEVEL_UP_BOX, currStats, TEXT_DYNAMIC_COLOR_5, TEXT_DYNAMIC_COLOR_4, TEXT_DYNAMIC_COLOR_6);
}

static void InitLevelUpBanner(void)
{
    gBattle_BG2_Y = 0;
    gBattle_BG2_X = LEVEL_UP_BANNER_START;

    LoadPalette(sLevelUpBanner_Pal, BG_PLTT_ID(6), sizeof(sLevelUpBanner_Pal));
    CopyToWindowPixelBuffer(B_WIN_LEVEL_UP_BANNER, sLevelUpBanner_Gfx, 0, 0);
    PutWindowTilemap(B_WIN_LEVEL_UP_BANNER);
    CopyWindowToVram(B_WIN_LEVEL_UP_BANNER, COPYWIN_FULL);

    PutMonIconOnLvlUpBanner();
}

static bool8 SlideInLevelUpBanner(void)
{
    if (IsDma3ManagerBusyWithBgCopy())
        return TRUE;

    if (gBattle_BG2_X == LEVEL_UP_BANNER_END)
        return FALSE;

    if (gBattle_BG2_X == LEVEL_UP_BANNER_START)
        DrawLevelUpBannerText();

    gBattle_BG2_X += 8;
    if (gBattle_BG2_X >= LEVEL_UP_BANNER_END)
        gBattle_BG2_X = LEVEL_UP_BANNER_END;

    return (gBattle_BG2_X != LEVEL_UP_BANNER_END);
}

static void DrawLevelUpBannerText(void)
{
    u16 monLevel;
    u8 monGender;
    struct TextPrinterTemplate printerTemplate;
    u8 *txtPtr;
    u32 var;

    monLevel = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL);
    monGender = GetMonGender(&gPlayerParty[gBattleStruct->expGetterMonId]);
    GetMonNickname(&gPlayerParty[gBattleStruct->expGetterMonId], gStringVar4);

    printerTemplate.currentChar = gStringVar4;
    printerTemplate.windowId = B_WIN_LEVEL_UP_BANNER;
    printerTemplate.fontId = FONT_SMALL;
    printerTemplate.x = 32;
    printerTemplate.y = 0;
    printerTemplate.currentX = 32;
    printerTemplate.currentY = 0;
    printerTemplate.letterSpacing = 0;
    printerTemplate.lineSpacing = 0;
    printerTemplate.unk = 0;
    printerTemplate.fgColor = TEXT_COLOR_WHITE;
    printerTemplate.bgColor = TEXT_COLOR_TRANSPARENT;
    printerTemplate.shadowColor = TEXT_COLOR_DARK_GRAY;

    AddTextPrinter(&printerTemplate, TEXT_SKIP_DRAW, NULL);

    txtPtr = gStringVar4;
    *(txtPtr)++ = CHAR_EXTRA_SYMBOL;
    *(txtPtr)++ = CHAR_LV_2;

    var = (u32)(txtPtr);
    txtPtr = ConvertIntToDecimalStringN(txtPtr, monLevel, STR_CONV_MODE_LEFT_ALIGN, 3);
    var = (u32)(txtPtr) - var;
    txtPtr = StringFill(txtPtr, CHAR_SPACER, 4 - var);

    if (monGender != MON_GENDERLESS)
    {
        if (monGender == MON_MALE)
        {
            txtPtr = WriteColorChangeControlCode(txtPtr, 0, TEXT_DYNAMIC_COLOR_3);
            txtPtr = WriteColorChangeControlCode(txtPtr, 1, TEXT_DYNAMIC_COLOR_4);
            *(txtPtr++) = CHAR_MALE;
        }
        else
        {
            txtPtr = WriteColorChangeControlCode(txtPtr, 0, TEXT_DYNAMIC_COLOR_5);
            txtPtr = WriteColorChangeControlCode(txtPtr, 1, TEXT_DYNAMIC_COLOR_6);
            *(txtPtr++) = CHAR_FEMALE;
        }
        *(txtPtr++) = EOS;
    }

    printerTemplate.y = 10;
    printerTemplate.currentY = 10;
    AddTextPrinter(&printerTemplate, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(B_WIN_LEVEL_UP_BANNER, COPYWIN_GFX);
}

static bool8 SlideOutLevelUpBanner(void)
{
    if (gBattle_BG2_X == LEVEL_UP_BANNER_START)
        return FALSE;

    if (gBattle_BG2_X - 16 < LEVEL_UP_BANNER_START)
        gBattle_BG2_X = LEVEL_UP_BANNER_START;
    else
        gBattle_BG2_X -= 16;

    return (gBattle_BG2_X != LEVEL_UP_BANNER_START);
}

#define sDestroy data[0]
#define sXOffset data[1]

static void PutMonIconOnLvlUpBanner(void)
{
    u8 spriteId;
    const u16 *iconPal;
    struct SpriteSheet iconSheet;
    struct SpritePalette iconPalSheet;

    u16 species = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPECIES);
    u32 personality = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_PERSONALITY);

    const u8 *iconPtr = GetMonIconPtr(species, personality, 1);
    iconSheet.data = iconPtr;
    iconSheet.size = 0x200;
    iconSheet.tag = TAG_LVLUP_BANNER_MON_ICON;

    iconPal = GetValidMonIconPalettePtr(species);
    iconPalSheet.data = iconPal;
    iconPalSheet.tag = TAG_LVLUP_BANNER_MON_ICON;

    LoadSpriteSheet(&iconSheet);
    LoadSpritePalette(&iconPalSheet);

    spriteId = CreateSprite(&sSpriteTemplate_MonIconOnLvlUpBanner, 256, 10, 0);
    gSprites[spriteId].sDestroy = FALSE;
    gSprites[spriteId].sXOffset = gBattle_BG2_X;
}

static void SpriteCB_MonIconOnLvlUpBanner(struct Sprite *sprite)
{
    sprite->x2 = sprite->sXOffset - gBattle_BG2_X;

    if (sprite->x2 != 0)
    {
        sprite->sDestroy = TRUE;
    }
    else if (sprite->sDestroy)
    {
        DestroySprite(sprite);
        FreeSpriteTilesByTag(TAG_LVLUP_BANNER_MON_ICON);
        FreeSpritePaletteByTag(TAG_LVLUP_BANNER_MON_ICON);
    }
}

#undef sDestroy
#undef sXOffset

static bool32 IsMonGettingExpSentOut(void)
{
    if (gBattlerPartyIndexes[0] == gBattleStruct->expGetterMonId)
        return TRUE;
    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && gBattlerPartyIndexes[2] == gBattleStruct->expGetterMonId)
        return TRUE;

    return FALSE;
}

static void Cmd_resetsentmonsvalue(void)
{
    ResetSentPokesToOpponentValue();
    gBattlescriptCurrInstr++;
}

static void Cmd_setatktoplayer0(void)
{
    gBattlerAttacker = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
    gBattlescriptCurrInstr++;
}

static void Cmd_makevisible(void)
{
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    BtlController_EmitSpriteInvisibility(BUFFER_A, FALSE);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_recordlastability(void)
{
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    RecordAbilityBattle(gActiveBattler, gLastUsedAbility);

#ifdef BUGFIX
    // This command occupies two bytes (one for the command id, and one for the battler id parameter).
    gBattlescriptCurrInstr += 2;
#else
    gBattlescriptCurrInstr += 1;
#endif
}

void BufferMoveToLearnIntoBattleTextBuff2(void)
{
    PREPARE_MOVE_BUFFER(gBattleTextBuff2, gMoveToLearn);
}

static void Cmd_buffermovetolearn(void)
{
    BufferMoveToLearnIntoBattleTextBuff2();
    gBattlescriptCurrInstr++;
}

static void Cmd_jumpifplayerran(void)
{
    if (TryRunFromBattle(gBattlerFainted))
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    else
        gBattlescriptCurrInstr += 5;
}

static void Cmd_hpthresholds(void)
{
    u8 opposingBattler;
    s32 result;

    if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
    {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        opposingBattler = BATTLE_OPPOSITE(gActiveBattler);

        result = gBattleMons[opposingBattler].hp * 100 / gBattleMons[opposingBattler].maxHP;
        if (result == 0)
            result = 1;

        if (result > 69 || gBattleMons[opposingBattler].hp == 0)
            gBattleStruct->hpScale = 0;
        else if (result > 39)
            gBattleStruct->hpScale = 1;
        else if (result > 9)
            gBattleStruct->hpScale = 2;
        else
            gBattleStruct->hpScale = 3;
    }

    gBattlescriptCurrInstr += 2;
}

static void Cmd_hpthresholds2(void)
{
    u8 opposingBattler;
    s32 result;
    u8 hpSwitchout;

    if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
    {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        opposingBattler = BATTLE_OPPOSITE(gActiveBattler);
        hpSwitchout = *(gBattleStruct->hpOnSwitchout + GetBattlerSide(opposingBattler));
        result = (hpSwitchout - gBattleMons[opposingBattler].hp) * 100 / hpSwitchout;

        if (gBattleMons[opposingBattler].hp >= hpSwitchout)
            gBattleStruct->hpScale = 0;
        else if (result <= 29)
            gBattleStruct->hpScale = 1;
        else if (result <= 69)
            gBattleStruct->hpScale = 2;
        else
            gBattleStruct->hpScale = 3;
    }

    gBattlescriptCurrInstr += 2;
}

static void Cmd_useitemonopponent(void)
{
    gBattlerInMenuId = gBattlerAttacker;
    PokemonUseItemEffects(&gEnemyParty[gBattlerPartyIndexes[gBattlerAttacker]], gLastUsedItem, gBattlerPartyIndexes[gBattlerAttacker], 0, TRUE);
    gBattlescriptCurrInstr++;
}

static void Cmd_various(void)
{
    u8 side;
    s32 i;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    switch (gBattlescriptCurrInstr[2])
    {
    case VARIOUS_CANCEL_MULTI_TURN_MOVES:
        CancelMultiTurnMoves(gActiveBattler);
        break;
    case VARIOUS_SET_MAGIC_COAT_TARGET:
        gBattlerAttacker = gBattlerTarget;
        side = BATTLE_OPPOSITE(GetBattlerSide(gBattlerAttacker));
        if (gSideTimers[side].followmeTimer != 0 && gBattleMons[gSideTimers[side].followmeTarget].hp != 0)
            gBattlerTarget = gSideTimers[side].followmeTarget;
        else
            gBattlerTarget = gActiveBattler;
        break;
    case VARIOUS_IS_RUNNING_IMPOSSIBLE:
        gBattleCommunication[0] = IsRunningFromBattleImpossible();
        break;
    case VARIOUS_GET_MOVE_TARGET:
        gBattlerTarget = GetMoveTarget(gCurrentMove, NO_TARGET_OVERRIDE);
        break;
    case VARIOUS_GET_BATTLER_FAINTED:
        if (gHitMarker & HITMARKER_FAINTED(gActiveBattler))
            gBattleCommunication[0] = TRUE;
        else
            gBattleCommunication[0] = FALSE;
        break;
    case VARIOUS_RESET_INTIMIDATE_TRACE_BITS:
        gSpecialStatuses[gActiveBattler].intimidatedMon = 0;
        gSpecialStatuses[gActiveBattler].traced = 0;
        break;
    case VARIOUS_UPDATE_CHOICE_MOVE_ON_LVL_UP:
        if (gBattlerPartyIndexes[0] == gBattleStruct->expGetterMonId || gBattlerPartyIndexes[2] == gBattleStruct->expGetterMonId)
        {
            u16 *choicedMove;

            if (gBattlerPartyIndexes[0] == gBattleStruct->expGetterMonId)
                gActiveBattler = 0;
            else
                gActiveBattler = 2;

            choicedMove = &gBattleStruct->choicedMove[gActiveBattler];

            for (i = 0; i < MAX_MON_MOVES; i++)
            {
                if (gBattleMons[gActiveBattler].moves[i] == *choicedMove)
                    break;
            }
            if (i == MAX_MON_MOVES)
                *choicedMove = MOVE_NONE;
        }
        break;
    case VARIOUS_RESET_PLAYER_FAINTED:
        if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_DOUBLE))
            && gBattleTypeFlags & BATTLE_TYPE_TRAINER
            && gBattleMons[0].hp != 0
            && gBattleMons[1].hp != 0)
        {
            gHitMarker &= ~HITMARKER_PLAYER_FAINTED;
        }
        break;
    case VARIOUS_PALACE_FLAVOR_TEXT:
        // Try and print end-of-turn Battle Palace flavor text (e.g. "A glint appears in mon's eyes")
        gBattleCommunication[0] = FALSE; // whether or not msg should be printed
        gBattleScripting.battler = gActiveBattler = gBattleCommunication[1];

        if (!(gBattleStruct->palaceFlags & gBitTable[gActiveBattler])
            && gBattleMons[gActiveBattler].maxHP / 2 >= gBattleMons[gActiveBattler].hp
            && gBattleMons[gActiveBattler].hp != 0
            && !(gBattleMons[gActiveBattler].status1 & STATUS1_SLEEP))
        {
            gBattleStruct->palaceFlags |= gBitTable[gActiveBattler];
            gBattleCommunication[0] = TRUE;
            gBattleCommunication[MULTISTRING_CHOOSER] = sBattlePalaceNatureToFlavorTextId[GetNatureFromPersonality(gBattleMons[gActiveBattler].personality)];
        }
        break;
    case VARIOUS_ARENA_JUDGMENT_WINDOW:
        i = BattleArena_ShowJudgmentWindow(&gBattleCommunication[0]);

        // BattleArena_ShowJudgmentWindow's last state was an intermediate step.
        // Return without advancing the current instruction so that it will be called again.
        if (i == ARENA_RESULT_RUNNING)
            return;

        gBattleCommunication[1] = i;
        break;
    case VARIOUS_ARENA_OPPONENT_MON_LOST:
        gBattleMons[1].hp = 0;
        gHitMarker |= HITMARKER_FAINTED(1);
        gBattleStruct->arenaLostOpponentMons |= gBitTable[gBattlerPartyIndexes[1]];
        gDisableStructs[1].truantSwitchInHack = 1;
        break;
    case VARIOUS_ARENA_PLAYER_MON_LOST:
        gBattleMons[0].hp = 0;
        gHitMarker |= HITMARKER_FAINTED(0);
        gHitMarker |= HITMARKER_PLAYER_FAINTED;
        gBattleStruct->arenaLostPlayerMons |= gBitTable[gBattlerPartyIndexes[0]];
        gDisableStructs[0].truantSwitchInHack = 1;
        break;
    case VARIOUS_ARENA_BOTH_MONS_LOST:
        gBattleMons[0].hp = 0;
        gBattleMons[1].hp = 0;
        gHitMarker |= HITMARKER_FAINTED(0);
        gHitMarker |= HITMARKER_FAINTED(1);
        gHitMarker |= HITMARKER_PLAYER_FAINTED;
        gBattleStruct->arenaLostPlayerMons |= gBitTable[gBattlerPartyIndexes[0]];
        gBattleStruct->arenaLostOpponentMons |= gBitTable[gBattlerPartyIndexes[1]];
        gDisableStructs[0].truantSwitchInHack = 1;
        gDisableStructs[1].truantSwitchInHack = 1;
        break;
    case VARIOUS_EMIT_YESNOBOX:
        BtlController_EmitYesNoBox(BUFFER_A);
        MarkBattlerForControllerExec(gActiveBattler);
        break;
    case VARIOUS_DRAW_ARENA_REF_TEXT_BOX:
        DrawArenaRefereeTextBox();
        break;
    case VARIOUS_ERASE_ARENA_REF_TEXT_BOX:
        EraseArenaRefereeTextBox();
        break;
    case VARIOUS_ARENA_JUDGMENT_STRING:
        BattleStringExpandPlaceholdersToDisplayedString(gRefereeStringsTable[gBattlescriptCurrInstr[1]]);
        BattlePutTextOnWindow(gDisplayedStringBattle, ARENA_WIN_JUDGMENT_TEXT);
        break;
    case VARIOUS_ARENA_WAIT_STRING:
        if (IsTextPrinterActive(ARENA_WIN_JUDGMENT_TEXT))
            return;
        break;
    case VARIOUS_WAIT_CRY:
        if (!IsCryFinished())
            return;
        break;
    case VARIOUS_RETURN_OPPONENT_MON1:
        gActiveBattler = 1;
        if (gBattleMons[gActiveBattler].hp != 0)
        {
            BtlController_EmitReturnMonToBall(BUFFER_A, FALSE);
            MarkBattlerForControllerExec(gActiveBattler);
        }
        break;
    case VARIOUS_RETURN_OPPONENT_MON2:
        if (gBattlersCount > 3)
        {
            gActiveBattler = 3;
            if (gBattleMons[gActiveBattler].hp != 0)
            {
                BtlController_EmitReturnMonToBall(BUFFER_A, FALSE);
                MarkBattlerForControllerExec(gActiveBattler);
            }
        }
        break;
    case VARIOUS_VOLUME_DOWN:
        m4aMPlayVolumeControl(&gMPlayInfo_BGM, TRACKS_ALL, 0x55);
        break;
    case VARIOUS_VOLUME_UP:
        m4aMPlayVolumeControl(&gMPlayInfo_BGM, TRACKS_ALL, 0x100);
        break;
    case VARIOUS_SET_ALREADY_STATUS_MOVE_ATTEMPT:
        gBattleStruct->alreadyStatusedMoveAttempt |= gBitTable[gActiveBattler];
        break;
    case VARIOUS_PALACE_TRY_ESCAPE_STATUS:
        if (BattlePalace_TryEscapeStatus(gActiveBattler))
            return;
        break;
    case VARIOUS_SET_TELEPORT_OUTCOME:
        if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
            gBattleOutcome = B_OUTCOME_PLAYER_TELEPORTED;
        else
            gBattleOutcome = B_OUTCOME_MON_TELEPORTED;
        break;
    case VARIOUS_PLAY_TRAINER_DEFEATED_MUSIC:
        BtlController_EmitPlayFanfareOrBGM(BUFFER_A, MUS_VICTORY_TRAINER, TRUE);
        MarkBattlerForControllerExec(gActiveBattler);
        break;
    }

    gBattlescriptCurrInstr += 3;
}

 // Protect and Endure
static void Cmd_setprotectlike(void)
{
    bool8 notLastTurn = TRUE;
    u16 lastMove = gLastResultingMoves[gBattlerAttacker];

    if (lastMove != MOVE_PROTECT && lastMove != MOVE_DETECT && lastMove != MOVE_ENDURE)
        gDisableStructs[gBattlerAttacker].protectUses = 0;

    if (gCurrentTurnActionNumber == (gBattlersCount - 1))
        notLastTurn = FALSE;

    if (sProtectSuccessRates[gDisableStructs[gBattlerAttacker].protectUses] >= Random() && notLastTurn)
    {
        if (gBattleMoves[gCurrentMove].effect == EFFECT_PROTECT)
        {
            gProtectStructs[gBattlerAttacker].protected = 1;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PROTECTED_ITSELF;
        }
        if (gBattleMoves[gCurrentMove].effect == EFFECT_ENDURE)
        {
            gProtectStructs[gBattlerAttacker].endured = 1;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_BRACED_ITSELF;
        }
        gDisableStructs[gBattlerAttacker].protectUses++;
    }
    else
    {
        gDisableStructs[gBattlerAttacker].protectUses = 0;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PROTECT_FAILED;
        gMoveResultFlags |= MOVE_RESULT_MISSED;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_tryexplosion(void)
{
    if (gBattleControllerExecFlags)
        return;

    // Explosion can only fail if any battler has Damp
    for (gBattlerTarget = 0; gBattlerTarget < gBattlersCount; gBattlerTarget++)
    {
        if (gBattleMons[gBattlerTarget].ability == ABILITY_DAMP)
            break;
    }

    if (gBattlerTarget == gBattlersCount)
    {
        // Success, no battlers with Damp. Drop user's HP bar to 0
        gActiveBattler = gBattlerAttacker;
        gBattleMoveDamage = gBattleMons[gActiveBattler].hp;
        BtlController_EmitHealthBarUpdate(BUFFER_A, INSTANT_HP_BAR_DROP);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr++;

        // Find first target
        for (gBattlerTarget = 0; gBattlerTarget < gBattlersCount; gBattlerTarget++)
        {
            if (gBattlerTarget == gBattlerAttacker)
                continue;
            if (!(gAbsentBattlerFlags & gBitTable[gBattlerTarget]))
                break;
        }
    }
    else
    {
        // Failed, a battler has Damp
        gLastUsedAbility = ABILITY_DAMP;
        RecordAbilityBattle(gBattlerTarget, gBattleMons[gBattlerTarget].ability);
        gBattlescriptCurrInstr = BattleScript_DampStopsExplosion;
    }
}

static void Cmd_setatkhptozero(void)
{
    if (gBattleControllerExecFlags)
        return;

    gActiveBattler = gBattlerAttacker;
    gBattleMons[gActiveBattler].hp = 0;
    BtlController_EmitSetMonData(BUFFER_A, REQUEST_HP_BATTLE, 0, sizeof(gBattleMons[gActiveBattler].hp), &gBattleMons[gActiveBattler].hp);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr++;
}

static void Cmd_jumpifnexttargetvalid(void)
{
    const u8 *jumpPtr = T1_READ_PTR(gBattlescriptCurrInstr + 1);

    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
    {
        for (gBattlerTarget++; ; gBattlerTarget++)
        {
            if (gBattlerTarget == gBattlerAttacker)
                continue;
            if (!(gAbsentBattlerFlags & gBitTable[gBattlerTarget]))
                break;
        }

        if (gBattlerTarget >= gBattlersCount)
            gBattlescriptCurrInstr += 5;
        else
            gBattlescriptCurrInstr = jumpPtr;
    }
    else
    {
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_tryhealhalfhealth(void)
{
    const u8 *failPtr = T1_READ_PTR(gBattlescriptCurrInstr + 1);

    if (gBattlescriptCurrInstr[5] == BS_ATTACKER)
        gBattlerTarget = gBattlerAttacker;

    gBattleMoveDamage = gBattleMons[gBattlerTarget].maxHP / 2;
    if (gBattleMoveDamage == 0)
        gBattleMoveDamage = 1;
    gBattleMoveDamage *= -1;

    if (gBattleMons[gBattlerTarget].hp == gBattleMons[gBattlerTarget].maxHP)
        gBattlescriptCurrInstr = failPtr;
    else
        gBattlescriptCurrInstr += 6;
}

static void Cmd_trymirrormove(void)
{
    s32 validMovesCount;
    s32 i;
    u16 move;
    u16 validMoves[MAX_BATTLERS_COUNT];

    for (i = 0; i < (MAX_BATTLERS_COUNT - 1); i++) // -1 to exclude the user
        validMoves[i] = MOVE_NONE;

    for (validMovesCount = 0, i = 0; i < gBattlersCount; i++)
    {
        if (i != gBattlerAttacker)
        {
            move = T1_READ_16(i * 2 + gBattlerAttacker * 8 + gBattleStruct->lastTakenMoveFrom);

            if (move != MOVE_NONE && move != MOVE_UNAVAILABLE)
            {
                validMoves[validMovesCount] = move;
                validMovesCount++;
            }
        }
    }

    move = T1_READ_16(gBattleStruct->lastTakenMove + gBattlerAttacker * 2);

    if (move != MOVE_NONE && move != MOVE_UNAVAILABLE)
    {
        gHitMarker &= ~HITMARKER_ATTACKSTRING_PRINTED;
        gCurrentMove = move;
        gBattlerTarget = GetMoveTarget(gCurrentMove, NO_TARGET_OVERRIDE);
        gBattlescriptCurrInstr = gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect];
    }
    else if (validMovesCount != 0)
    {
        gHitMarker &= ~HITMARKER_ATTACKSTRING_PRINTED;
        i = Random() % validMovesCount;
        gCurrentMove = validMoves[i];
        gBattlerTarget = GetMoveTarget(gCurrentMove, NO_TARGET_OVERRIDE);
        gBattlescriptCurrInstr = gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect];
    }
    else // no valid moves found
    {
        gSpecialStatuses[gBattlerAttacker].ppNotAffectedByPressure = TRUE;
        gBattlescriptCurrInstr++;
    }
}

static void Cmd_setrain(void)
{
    if (gBattleWeather & B_WEATHER_RAIN)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_WEATHER_FAILED;
    }
    else
    {
        gBattleWeather = B_WEATHER_RAIN_TEMPORARY;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STARTED_RAIN;
        gWishFutureKnock.weatherDuration = 5;
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_setreflect(void)
{
    if (gSideStatuses[GET_BATTLER_SIDE(gBattlerAttacker)] & SIDE_STATUS_REFLECT)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SIDE_STATUS_FAILED;
    }
    else
    {
        gSideStatuses[GET_BATTLER_SIDE(gBattlerAttacker)] |= SIDE_STATUS_REFLECT;
        gSideTimers[GET_BATTLER_SIDE(gBattlerAttacker)].reflectTimer = 5;
        gSideTimers[GET_BATTLER_SIDE(gBattlerAttacker)].reflectBattlerId = gBattlerAttacker;

        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && CountAliveMonsInBattle(BATTLE_ALIVE_ATK_SIDE) == 2)
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SET_REFLECT_DOUBLE;
        else
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SET_REFLECT_SINGLE;
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_setseeded(void)
{
    if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT || gStatuses3[gBattlerTarget] & STATUS3_LEECHSEED)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_LEECH_SEED_MISS;
    }
    else if (IS_BATTLER_OF_TYPE(gBattlerTarget, TYPE_GRASS))
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_LEECH_SEED_FAIL;
    }
    else
    {
        gStatuses3[gBattlerTarget] |= gBattlerAttacker;
        gStatuses3[gBattlerTarget] |= STATUS3_LEECHSEED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_LEECH_SEED_SET;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_manipulatedamage(void)
{
    switch (gBattlescriptCurrInstr[1])
    {
    case DMG_CHANGE_SIGN:
        gBattleMoveDamage *= -1;
        break;
    case DMG_RECOIL_FROM_MISS:
        gBattleMoveDamage /= 2;
        if (gBattleMoveDamage == 0)
            gBattleMoveDamage = 1;
        if ((gBattleMons[gBattlerTarget].maxHP / 2) < gBattleMoveDamage)
            gBattleMoveDamage = gBattleMons[gBattlerTarget].maxHP / 2;
        break;
    case DMG_DOUBLED:
        gBattleMoveDamage *= 2;
        break;
    }

    gBattlescriptCurrInstr += 2;
}

static void Cmd_trysetrest(void)
{
    const u8 *failJump = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    gActiveBattler = gBattlerTarget = gBattlerAttacker;
    gBattleMoveDamage = gBattleMons[gBattlerTarget].maxHP * (-1);

    if (gBattleMons[gBattlerTarget].hp == gBattleMons[gBattlerTarget].maxHP)
    {
        gBattlescriptCurrInstr = failJump;
    }
    else
    {
        if (gBattleMons[gBattlerTarget].status1 & ((u8)(~STATUS1_SLEEP)))
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_REST_STATUSED;
        else
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_REST;

        gBattleMons[gBattlerTarget].status1 = STATUS1_SLEEP_TURN(3);
        BtlController_EmitSetMonData(BUFFER_A, REQUEST_STATUS_BATTLE, 0, sizeof(gBattleMons[gActiveBattler].status1), &gBattleMons[gActiveBattler].status1);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_jumpifnotfirstturn(void)
{
    const u8 *failJump = T1_READ_PTR(gBattlescriptCurrInstr + 1);

    if (gDisableStructs[gBattlerAttacker].isFirstTurn)
        gBattlescriptCurrInstr += 5;
    else
        gBattlescriptCurrInstr = failJump;
}

static void Cmd_nop(void)
{
    gBattlescriptCurrInstr++;
}

bool8 UproarWakeUpCheck(u8 battlerId)
{
    s32 i;

    for (i = 0; i < gBattlersCount; i++)
    {
        if (!(gBattleMons[i].status2 & STATUS2_UPROAR) || gBattleMons[battlerId].ability == ABILITY_SOUNDPROOF)
            continue;

        gBattleScripting.battler = i;

        if (gBattlerTarget == 0xFF)
            gBattlerTarget = i;
        else if (gBattlerTarget == i)
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CANT_SLEEP_UPROAR;
        else
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_UPROAR_KEPT_AWAKE;

        break;
    }

    if (i == gBattlersCount)
        return FALSE;
    else
        return TRUE;
}

static void Cmd_jumpifcantmakeasleep(void)
{
    const u8 *jumpPtr = T1_READ_PTR(gBattlescriptCurrInstr + 1);

    if (UproarWakeUpCheck(gBattlerTarget))
    {
        gBattlescriptCurrInstr = jumpPtr;
    }
    else if (gBattleMons[gBattlerTarget].ability == ABILITY_INSOMNIA
            || gBattleMons[gBattlerTarget].ability == ABILITY_VITAL_SPIRIT)
    {
        gLastUsedAbility = gBattleMons[gBattlerTarget].ability;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAYED_AWAKE_USING;
        gBattlescriptCurrInstr = jumpPtr;
        RecordAbilityBattle(gBattlerTarget, gLastUsedAbility);
    }
    else
    {
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_stockpile(void)
{
    if (gDisableStructs[gBattlerAttacker].stockpileCounter == 3)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CANT_STOCKPILE;
    }
    else
    {
        gDisableStructs[gBattlerAttacker].stockpileCounter++;

        PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff1, 1, gDisableStructs[gBattlerAttacker].stockpileCounter)

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STOCKPILED;
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_stockpiletobasedamage(void)
{
    const u8 *jumpPtr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    if (gDisableStructs[gBattlerAttacker].stockpileCounter == 0)
    {
        gBattlescriptCurrInstr = jumpPtr;
    }
    else
    {
        if (gBattleCommunication[MISS_TYPE] != B_MSG_PROTECTED)
        {
            gBattleMoveDamage = CalculateBaseDamage(&gBattleMons[gBattlerAttacker], &gBattleMons[gBattlerTarget], gCurrentMove,
                                                    gSideStatuses[GET_BATTLER_SIDE(gBattlerTarget)], 0,
                                                    0, gBattlerAttacker, gBattlerTarget)
                                * gDisableStructs[gBattlerAttacker].stockpileCounter;
            gBattleScripting.animTurn = gDisableStructs[gBattlerAttacker].stockpileCounter;

            if (gProtectStructs[gBattlerAttacker].helpingHand)
                gBattleMoveDamage = gBattleMoveDamage * 15 / 10;
        }

        gDisableStructs[gBattlerAttacker].stockpileCounter = 0;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_stockpiletohpheal(void)
{
    const u8 *jumpPtr = T1_READ_PTR(gBattlescriptCurrInstr + 1);

    if (gDisableStructs[gBattlerAttacker].stockpileCounter == 0)
    {
        gBattlescriptCurrInstr = jumpPtr;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWALLOW_FAILED;
    }
    else if (gBattleMons[gBattlerAttacker].maxHP == gBattleMons[gBattlerAttacker].hp)
    {
        gDisableStructs[gBattlerAttacker].stockpileCounter = 0;
        gBattlescriptCurrInstr = jumpPtr;
        gBattlerTarget = gBattlerAttacker;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWALLOW_FULL_HP;
    }
    else
    {
        gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / (1 << (3 - gDisableStructs[gBattlerAttacker].stockpileCounter));

        if (gBattleMoveDamage == 0)
            gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;

        gBattleScripting.animTurn = gDisableStructs[gBattlerAttacker].stockpileCounter;
        gDisableStructs[gBattlerAttacker].stockpileCounter = 0;
        gBattlescriptCurrInstr += 5;
        gBattlerTarget = gBattlerAttacker;
    }
}

static void Cmd_negativedamage(void)
{
    gBattleMoveDamage = -(gHpDealt / 2);
    if (gBattleMoveDamage == 0)
        gBattleMoveDamage = -1;

    gBattlescriptCurrInstr++;
}

#define STAT_CHANGE_WORKED      0
#define STAT_CHANGE_DIDNT_WORK  1

static u8 ChangeStatBuffs(s8 statValue, u8 statId, u8 flags, const u8 *BS_ptr)
{
    bool8 certain = FALSE;
    bool8 notProtectAffected = FALSE;
    u32 index;

    if (flags & MOVE_EFFECT_AFFECTS_USER)
        gActiveBattler = gBattlerAttacker;
    else
        gActiveBattler = gBattlerTarget;

    flags &= ~MOVE_EFFECT_AFFECTS_USER;

    if (flags & MOVE_EFFECT_CERTAIN)
        certain++;
    flags &= ~MOVE_EFFECT_CERTAIN;

    if (flags & STAT_CHANGE_NOT_PROTECT_AFFECTED)
        notProtectAffected++;
    flags &= ~STAT_CHANGE_NOT_PROTECT_AFFECTED;

    PREPARE_STAT_BUFFER(gBattleTextBuff1, statId)

    if (statValue <= -1) // Stat decrease.
    {
        if (gSideTimers[GET_BATTLER_SIDE(gActiveBattler)].mistTimer
            && !certain && gCurrentMove != MOVE_CURSE)
        {
            if (flags == STAT_CHANGE_ALLOW_PTR)
            {
                if (gSpecialStatuses[gActiveBattler].statLowered)
                {
                    gBattlescriptCurrInstr = BS_ptr;
                }
                else
                {
                    BattleScriptPush(BS_ptr);
                    gBattleScripting.battler = gActiveBattler;
                    gBattlescriptCurrInstr = BattleScript_MistProtected;
                    gSpecialStatuses[gActiveBattler].statLowered = 1;
                }
            }
            return STAT_CHANGE_DIDNT_WORK;
        }
        else if (gCurrentMove != MOVE_CURSE
                 && notProtectAffected != TRUE && JumpIfMoveAffectedByProtect(0))
        {
            gBattlescriptCurrInstr = BattleScript_ButItFailed;
            return STAT_CHANGE_DIDNT_WORK;
        }
        else if ((gBattleMons[gActiveBattler].ability == ABILITY_CLEAR_BODY
                  || gBattleMons[gActiveBattler].ability == ABILITY_WHITE_SMOKE)
                 && !certain && gCurrentMove != MOVE_CURSE)
        {
            if (flags == STAT_CHANGE_ALLOW_PTR)
            {
                if (gSpecialStatuses[gActiveBattler].statLowered)
                {
                    gBattlescriptCurrInstr = BS_ptr;
                }
                else
                {
                    BattleScriptPush(BS_ptr);
                    gBattleScripting.battler = gActiveBattler;
                    gBattlescriptCurrInstr = BattleScript_AbilityNoStatLoss;
                    gLastUsedAbility = gBattleMons[gActiveBattler].ability;
                    RecordAbilityBattle(gActiveBattler, gLastUsedAbility);
                    gSpecialStatuses[gActiveBattler].statLowered = 1;
                }
            }
            return STAT_CHANGE_DIDNT_WORK;
        }
        else if (gBattleMons[gActiveBattler].ability == ABILITY_KEEN_EYE
                 && !certain && statId == STAT_ACC)
        {
            if (flags == STAT_CHANGE_ALLOW_PTR)
            {
                BattleScriptPush(BS_ptr);
                gBattleScripting.battler = gActiveBattler;
                gBattlescriptCurrInstr = BattleScript_AbilityNoSpecificStatLoss;
                gLastUsedAbility = gBattleMons[gActiveBattler].ability;
                RecordAbilityBattle(gActiveBattler, gLastUsedAbility);
            }
            return STAT_CHANGE_DIDNT_WORK;
        }
        else if (gBattleMons[gActiveBattler].ability == ABILITY_HYPER_CUTTER
                 && !certain && statId == STAT_ATK)
        {
            if (flags == STAT_CHANGE_ALLOW_PTR)
            {
                BattleScriptPush(BS_ptr);
                gBattleScripting.battler = gActiveBattler;
                gBattlescriptCurrInstr = BattleScript_AbilityNoSpecificStatLoss;
                gLastUsedAbility = gBattleMons[gActiveBattler].ability;
                RecordAbilityBattle(gActiveBattler, gLastUsedAbility);
            }
            return STAT_CHANGE_DIDNT_WORK;
        }
        else if (gBattleMons[gActiveBattler].ability == ABILITY_SHIELD_DUST && flags == 0)
        {
            return STAT_CHANGE_DIDNT_WORK;
        }
        else // try to decrease
        {
            statValue = -GET_STAT_BUFF_VALUE(statValue);
            gBattleTextBuff2[0] = B_BUFF_PLACEHOLDER_BEGIN;
            index = 1;
            if (statValue == -2)
            {
                gBattleTextBuff2[1] = B_BUFF_STRING;
                gBattleTextBuff2[2] = STRINGID_STATHARSHLY;
                gBattleTextBuff2[3] = STRINGID_STATHARSHLY >> 8;
                index = 4;
            }
            gBattleTextBuff2[index++] = B_BUFF_STRING;
            gBattleTextBuff2[index++] = STRINGID_STATFELL;
            gBattleTextBuff2[index++] = STRINGID_STATFELL >> 8;
            gBattleTextBuff2[index] = B_BUFF_EOS;

            if (gBattleMons[gActiveBattler].statStages[statId] == MIN_STAT_STAGE)
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_WONT_DECREASE;
            else
                gBattleCommunication[MULTISTRING_CHOOSER] = (gBattlerTarget == gActiveBattler); // B_MSG_ATTACKER_STAT_FELL or B_MSG_DEFENDER_STAT_FELL
        }
    }
    else // stat increase
    {
        statValue = GET_STAT_BUFF_VALUE(statValue);
        gBattleTextBuff2[0] = B_BUFF_PLACEHOLDER_BEGIN;
        index = 1;
        if (statValue == 2)
        {
            gBattleTextBuff2[1] = B_BUFF_STRING;
            gBattleTextBuff2[2] = STRINGID_STATSHARPLY;
            gBattleTextBuff2[3] = STRINGID_STATSHARPLY >> 8;
            index = 4;
        }
        gBattleTextBuff2[index++] = B_BUFF_STRING;
        gBattleTextBuff2[index++] = STRINGID_STATROSE;
        gBattleTextBuff2[index++] = STRINGID_STATROSE >> 8;
        gBattleTextBuff2[index] = B_BUFF_EOS;

        if (gBattleMons[gActiveBattler].statStages[statId] == MAX_STAT_STAGE)
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_WONT_INCREASE;
        else
            gBattleCommunication[MULTISTRING_CHOOSER] = (gBattlerTarget == gActiveBattler); // B_MSG_ATTACKER_STAT_ROSE or B_MSG_DEFENDER_STAT_ROSE
    }

    gBattleMons[gActiveBattler].statStages[statId] += statValue;
    if (gBattleMons[gActiveBattler].statStages[statId] < MIN_STAT_STAGE)
        gBattleMons[gActiveBattler].statStages[statId] = MIN_STAT_STAGE;
    if (gBattleMons[gActiveBattler].statStages[statId] > MAX_STAT_STAGE)
        gBattleMons[gActiveBattler].statStages[statId] = MAX_STAT_STAGE;

    if (gBattleCommunication[MULTISTRING_CHOOSER] == B_MSG_STAT_WONT_INCREASE && flags & STAT_CHANGE_ALLOW_PTR)
        gMoveResultFlags |= MOVE_RESULT_MISSED;

    if (gBattleCommunication[MULTISTRING_CHOOSER] == B_MSG_STAT_WONT_INCREASE && !(flags & STAT_CHANGE_ALLOW_PTR))
        return STAT_CHANGE_DIDNT_WORK;

    return STAT_CHANGE_WORKED;
}

static void Cmd_statbuffchange(void)
{
    const u8 *jumpPtr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
    if (ChangeStatBuffs(gBattleScripting.statChanger & 0xF0, GET_STAT_BUFF_ID(gBattleScripting.statChanger), gBattlescriptCurrInstr[1], jumpPtr) == STAT_CHANGE_WORKED)
        gBattlescriptCurrInstr += 6;
}

// Haze
static void Cmd_normalisebuffs(void)
{
    s32 i, j;

    for (i = 0; i < gBattlersCount; i++)
    {
        for (j = 0; j < NUM_BATTLE_STATS; j++)
            gBattleMons[i].statStages[j] = DEFAULT_STAT_STAGE;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_setbide(void)
{
    gBattleMons[gBattlerAttacker].status2 |= STATUS2_MULTIPLETURNS;
    gLockedMoves[gBattlerAttacker] = gCurrentMove;
    gBideDmg[gBattlerAttacker] = 0;
    gBattleMons[gBattlerAttacker].status2 |= STATUS2_BIDE_TURN(2);

    gBattlescriptCurrInstr++;
}

static void Cmd_confuseifrepeatingattackends(void)
{
    if (!(gBattleMons[gBattlerAttacker].status2 & STATUS2_LOCK_CONFUSE))
        gBattleCommunication[MOVE_EFFECT_BYTE] = (MOVE_EFFECT_THRASH | MOVE_EFFECT_AFFECTS_USER);

    gBattlescriptCurrInstr++;
}

static void Cmd_setmultihitcounter(void)
{
    if (gBattlescriptCurrInstr[1])
    {
        gMultiHitCounter = gBattlescriptCurrInstr[1];
    }
    else
    {
        gMultiHitCounter = Random() & 3;
        if (gMultiHitCounter > 1)
            gMultiHitCounter = (Random() & 3) + 2;
        else
            gMultiHitCounter += 2;
    }

    gBattlescriptCurrInstr += 2;
}

static void Cmd_initmultihitstring(void)
{
    PREPARE_BYTE_NUMBER_BUFFER(gBattleScripting.multihitString, 1, 0)

    gBattlescriptCurrInstr++;
}

static bool8 TryDoForceSwitchOut(void)
{
    if (gBattleMons[gBattlerAttacker].level >= gBattleMons[gBattlerTarget].level)
    {
        *(gBattleStruct->battlerPartyIndexes + gBattlerTarget) = gBattlerPartyIndexes[gBattlerTarget];
    }
    else
    {
        u16 random = Random() & 0xFF;
        if ((u32)((random * (gBattleMons[gBattlerAttacker].level + gBattleMons[gBattlerTarget].level) >> 8) + 1) <= (gBattleMons[gBattlerTarget].level / 4))
        {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
            return FALSE;
        }
        *(gBattleStruct->battlerPartyIndexes + gBattlerTarget) = gBattlerPartyIndexes[gBattlerTarget];
    }

    gBattlescriptCurrInstr = BattleScript_SuccessForceOut;
    return TRUE;
}

static void Cmd_forcerandomswitch(void)
{
    s32 i;
    s32 battler1PartyId = 0;
    s32 battler2PartyId = 0;

    s32 firstMonId;
    s32 lastMonId = 0;
    s32 monsCount;
    struct Pokemon *party = NULL;
    s32 validMons = 0;
    s32 minNeeded;

    if ((gBattleTypeFlags & BATTLE_TYPE_TRAINER))
    {
        if (GetBattlerSide(gBattlerTarget) == B_SIDE_PLAYER)
            party = gPlayerParty;
        else
            party = gEnemyParty;

        if ((gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER && gBattleTypeFlags & BATTLE_TYPE_LINK)
            || (gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER && gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
            || (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER))
        {
            if ((gBattlerTarget & BIT_FLANK) != B_FLANK_LEFT)
            {
                firstMonId = PARTY_SIZE / 2;
                #ifdef BUGFIX
                lastMonId = PARTY_SIZE - 1;
                #else
                lastMonId = PARTY_SIZE;
                #endif
            }
            else
            {
                firstMonId = 0;
                #ifdef BUGFIX
                lastMonId = PARTY_SIZE / 2 - 1;
                #else
                lastMonId = PARTY_SIZE / 2;
                #endif
            }
            monsCount = PARTY_SIZE / 2;
            minNeeded = 1;
            battler2PartyId = gBattlerPartyIndexes[gBattlerTarget];
            battler1PartyId = gBattlerPartyIndexes[BATTLE_PARTNER(gBattlerTarget)];
        }
        else if ((gBattleTypeFlags & BATTLE_TYPE_MULTI && gBattleTypeFlags & BATTLE_TYPE_LINK)
                 || (gBattleTypeFlags & BATTLE_TYPE_MULTI && gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK))
        {
            if (GetLinkTrainerFlankId(GetBattlerMultiplayerId(gBattlerTarget)) == B_FLANK_RIGHT)
            {
                firstMonId = PARTY_SIZE / 2;
                #ifdef BUGFIX
                lastMonId = PARTY_SIZE - 1;
                #else
                lastMonId = PARTY_SIZE;
                #endif
            }
            else
            {
                firstMonId = 0;
                #ifdef BUGFIX
                lastMonId = PARTY_SIZE / 2 - 1;
                #else
                lastMonId = PARTY_SIZE / 2;
                #endif
            }
            monsCount = PARTY_SIZE / 2;
            minNeeded = 1;
            battler2PartyId = gBattlerPartyIndexes[gBattlerTarget];
            battler1PartyId = gBattlerPartyIndexes[BATTLE_PARTNER(gBattlerTarget)];
        }
        else if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
        {
            if (GetBattlerSide(gBattlerTarget) == B_SIDE_PLAYER)
            {
                firstMonId = 0;
                #ifdef BUGFIX
                lastMonId = PARTY_SIZE - 1;
                #else
                lastMonId = PARTY_SIZE;
                #endif
                monsCount = PARTY_SIZE;
                minNeeded = 2; // since there are two opponents, it has to be a double battle
            }
            else
            {
                if ((gBattlerTarget & BIT_FLANK) != B_FLANK_LEFT)
                {
                    firstMonId = PARTY_SIZE / 2;
                    #ifdef BUGFIX
                    lastMonId = PARTY_SIZE - 1;
                    #else
                    lastMonId = PARTY_SIZE;
                    #endif
                }
                else
                {
                    firstMonId = 0;
                    #ifdef BUGFIX
                    lastMonId = PARTY_SIZE / 2 - 1;
                    #else
                    lastMonId = PARTY_SIZE / 2;
                    #endif
                }
                monsCount = PARTY_SIZE / 2;
                minNeeded = 1;
            }
            battler2PartyId = gBattlerPartyIndexes[gBattlerTarget];
            battler1PartyId = gBattlerPartyIndexes[BATTLE_PARTNER(gBattlerTarget)];
        }
        else if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
        {
            firstMonId = 0;
            #ifdef BUGFIX
            lastMonId = PARTY_SIZE - 1;
            #else
            lastMonId = PARTY_SIZE;
            #endif
            monsCount = PARTY_SIZE;
            minNeeded = 2;
            battler2PartyId = gBattlerPartyIndexes[gBattlerTarget];
            battler1PartyId = gBattlerPartyIndexes[BATTLE_PARTNER(gBattlerTarget)];
        }
        else
        {
            firstMonId = 0;
            #ifdef BUGFIX
            lastMonId = PARTY_SIZE - 1;
            #else
            lastMonId = PARTY_SIZE;
            #endif
            monsCount = PARTY_SIZE;
            minNeeded = 1;
            battler2PartyId = gBattlerPartyIndexes[gBattlerTarget]; // there is only one Pokémon out in single battles
            battler1PartyId = gBattlerPartyIndexes[gBattlerTarget];
        }

        for (i = firstMonId; i < lastMonId; i++)
        {
            if (GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE
             && !GetMonData(&party[i], MON_DATA_IS_EGG)
             && GetMonData(&party[i], MON_DATA_HP) != 0)
             {
                 validMons++;
             }
        }

        if (validMons <= minNeeded)
        {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        }
        else
        {
            #ifdef BUGFIX
            if (TryDoForceSwitchOut())
            {
                do
                {
                    do
                    {
                        i = Random() % monsCount;
                        i += firstMonId;
                    }
                    while (i == battler2PartyId || i == battler1PartyId);
                } while (GetMonData(&party[i], MON_DATA_SPECIES) == SPECIES_NONE
                       || GetMonData(&party[i], MON_DATA_IS_EGG) == TRUE
                       || GetMonData(&party[i], MON_DATA_HP) == 0); //should be one while loop, but that doesn't match.
                *(gBattleStruct->monToSwitchIntoId + gBattlerTarget) = i;

                if (!IsMultiBattle())
                    SwitchPartyOrder(gBattlerTarget);

                if ((gBattleTypeFlags & BATTLE_TYPE_LINK && gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER)
                    || (gBattleTypeFlags & BATTLE_TYPE_LINK && gBattleTypeFlags & BATTLE_TYPE_MULTI)
                    || (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK && gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER)
                    || (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK && gBattleTypeFlags & BATTLE_TYPE_MULTI))
                {
                    SwitchPartyOrderLinkMulti(gBattlerTarget, i, 0);
                    SwitchPartyOrderLinkMulti(BATTLE_PARTNER(gBattlerTarget), i, 1);
                }

                if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
                    SwitchPartyOrderInGameMulti(gBattlerTarget, i);
            }
            #else
            if (TryDoForceSwitchOut())
            {
                do
                {
                    do
                    {
                        i = Random() % monsCount;
                        i += firstMonId;
                    }
                    while (i == battler2PartyId || i == battler1PartyId);
                } while (GetMonData(&party[i], MON_DATA_SPECIES) == SPECIES_NONE
                       || GetMonData(&party[i], MON_DATA_IS_EGG) == TRUE
                       || GetMonData(&party[i], MON_DATA_HP) == 0); //should be one while loop, but that doesn't match.
            }
            *(gBattleStruct->monToSwitchIntoId + gBattlerTarget) = i;

            if (!IsMultiBattle())
                SwitchPartyOrder(gBattlerTarget);

            if ((gBattleTypeFlags & BATTLE_TYPE_LINK && gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER)
                || (gBattleTypeFlags & BATTLE_TYPE_LINK && gBattleTypeFlags & BATTLE_TYPE_MULTI)
                || (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK && gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER)
                || (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK && gBattleTypeFlags & BATTLE_TYPE_MULTI))
            {
                SwitchPartyOrderLinkMulti(gBattlerTarget, i, 0);
                SwitchPartyOrderLinkMulti(BATTLE_PARTNER(gBattlerTarget), i, 1);
            }

            if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
                SwitchPartyOrderInGameMulti(gBattlerTarget, i);
            #endif
        }
    }
    else
    {
        TryDoForceSwitchOut();
    }
}

// Randomly changes user's type to one of its moves' type
static void Cmd_tryconversiontypechange(void)
{
    u8 validMoves = 0;
    u8 moveChecked;
    u8 moveType;

    while (validMoves < MAX_MON_MOVES)
    {
        if (gBattleMons[gBattlerAttacker].moves[validMoves] == MOVE_NONE)
            break;

        validMoves++;
    }

    for (moveChecked = 0; moveChecked < validMoves; moveChecked++)
    {
        moveType = gBattleMoves[gBattleMons[gBattlerAttacker].moves[moveChecked]].type;

        if (moveType == TYPE_MYSTERY)
        {
            if (IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_GHOST))
                moveType = TYPE_GHOST;
            else
                moveType = TYPE_NORMAL;
        }
        if (moveType != gBattleMons[gBattlerAttacker].types[0]
            && moveType != gBattleMons[gBattlerAttacker].types[1])
        {
            break;
        }
    }

    if (moveChecked == validMoves)
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        do
        {
            while ((moveChecked = MOD(Random(), MAX_MON_MOVES)) >= validMoves);

            moveType = gBattleMoves[gBattleMons[gBattlerAttacker].moves[moveChecked]].type;

            if (moveType == TYPE_MYSTERY)
            {
                if (IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_GHOST))
                    moveType = TYPE_GHOST;
                else
                    moveType = TYPE_NORMAL;
            }
        }
        while (moveType == gBattleMons[gBattlerAttacker].types[0] || moveType == gBattleMons[gBattlerAttacker].types[1]);

        SET_BATTLER_TYPE(gBattlerAttacker, moveType);
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, moveType);

        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_givepaydaymoney(void)
{
    if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)) && gPaydayMoney != 0)
    {
        u32 bonusMoney = gPaydayMoney * gBattleStruct->moneyMultiplier;
        AddMoney(&gSaveBlock1Ptr->money, bonusMoney);

        PREPARE_HWORD_NUMBER_BUFFER(gBattleTextBuff1, 5, bonusMoney)

        BattleScriptPush(gBattlescriptCurrInstr + 1);
        gBattlescriptCurrInstr = BattleScript_PrintPayDayMoneyString;
    }
    else
    {
        gBattlescriptCurrInstr++;
    }
}

static void Cmd_setlightscreen(void)
{
    if (gSideStatuses[GET_BATTLER_SIDE(gBattlerAttacker)] & SIDE_STATUS_LIGHTSCREEN)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SIDE_STATUS_FAILED;
    }
    else
    {
        gSideStatuses[GET_BATTLER_SIDE(gBattlerAttacker)] |= SIDE_STATUS_LIGHTSCREEN;
        gSideTimers[GET_BATTLER_SIDE(gBattlerAttacker)].lightscreenTimer = 5;
        gSideTimers[GET_BATTLER_SIDE(gBattlerAttacker)].lightscreenBattlerId = gBattlerAttacker;

        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && CountAliveMonsInBattle(BATTLE_ALIVE_ATK_SIDE) == 2)
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SET_LIGHTSCREEN_DOUBLE;
        else
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SET_LIGHTSCREEN_SINGLE;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_tryKO(void)
{
    u8 holdEffect, param;

    if (gBattleMons[gBattlerTarget].item == ITEM_ENIGMA_BERRY)
    {
       holdEffect = gEnigmaBerries[gBattlerTarget].holdEffect;
       param = gEnigmaBerries[gBattlerTarget].holdEffectParam;
    }
    else
    {
        holdEffect = ItemId_GetHoldEffect(gBattleMons[gBattlerTarget].item);
        param = ItemId_GetHoldEffectParam(gBattleMons[gBattlerTarget].item);
    }

    gPotentialItemEffectBattler = gBattlerTarget;

    if (holdEffect == HOLD_EFFECT_FOCUS_BAND && (Random() % 100) < param)
    {
        RecordItemEffectBattle(gBattlerTarget, HOLD_EFFECT_FOCUS_BAND);
        gSpecialStatuses[gBattlerTarget].focusBanded = 1;
    }

    if (gBattleMons[gBattlerTarget].ability == ABILITY_STURDY)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gLastUsedAbility = ABILITY_STURDY;
        gBattlescriptCurrInstr = BattleScript_SturdyPreventsOHKO;
        RecordAbilityBattle(gBattlerTarget, ABILITY_STURDY);
    }
    else
    {
        u16 chance;
        if (!(gStatuses3[gBattlerTarget] & STATUS3_ALWAYS_HITS))
        {
            chance = gBattleMoves[gCurrentMove].accuracy + (gBattleMons[gBattlerAttacker].level - gBattleMons[gBattlerTarget].level);
            if (Random() % 100 + 1 < chance && gBattleMons[gBattlerAttacker].level >= gBattleMons[gBattlerTarget].level)
                chance = TRUE;
            else
                chance = FALSE;
        }
        else if (gDisableStructs[gBattlerTarget].battlerWithSureHit == gBattlerAttacker
                 && gBattleMons[gBattlerAttacker].level >= gBattleMons[gBattlerTarget].level)
        {
            chance = TRUE;
        }
        else
        {
            chance = gBattleMoves[gCurrentMove].accuracy + (gBattleMons[gBattlerAttacker].level - gBattleMons[gBattlerTarget].level);
            if (Random() % 100 + 1 < chance && gBattleMons[gBattlerAttacker].level >= gBattleMons[gBattlerTarget].level)
                chance = TRUE;
            else
                chance = FALSE;
        }
        if (chance)
        {
            if (gProtectStructs[gBattlerTarget].endured)
            {
                gBattleMoveDamage = gBattleMons[gBattlerTarget].hp - 1;
                gMoveResultFlags |= MOVE_RESULT_FOE_ENDURED;
            }
            else if (gSpecialStatuses[gBattlerTarget].focusBanded)
            {
                gBattleMoveDamage = gBattleMons[gBattlerTarget].hp - 1;
                gMoveResultFlags |= MOVE_RESULT_FOE_HUNG_ON;
                gLastUsedItem = gBattleMons[gBattlerTarget].item;
            }
            else
            {
                gBattleMoveDamage = gBattleMons[gBattlerTarget].hp;
                gMoveResultFlags |= MOVE_RESULT_ONE_HIT_KO;
            }
            gBattlescriptCurrInstr += 5;
        }
        else
        {
            gMoveResultFlags |= MOVE_RESULT_MISSED;
            if (gBattleMons[gBattlerAttacker].level >= gBattleMons[gBattlerTarget].level)
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_KO_MISS;
            else
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_KO_UNAFFECTED;
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        }
    }
}

// Super Fang
static void Cmd_damagetohalftargethp(void)
{
    gBattleMoveDamage = gBattleMons[gBattlerTarget].hp / 2;
    if (gBattleMoveDamage == 0)
        gBattleMoveDamage = 1;

    gBattlescriptCurrInstr++;
}

static void Cmd_setsandstorm(void)
{
    if (gBattleWeather & B_WEATHER_SANDSTORM)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_WEATHER_FAILED;
    }
    else
    {
        gBattleWeather = B_WEATHER_SANDSTORM_TEMPORARY;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STARTED_SANDSTORM;
        gWishFutureKnock.weatherDuration = 5;
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_weatherdamage(void)
{
    if (WEATHER_HAS_EFFECT)
    {
        if (gBattleWeather & B_WEATHER_SANDSTORM)
        {
            if (gBattleMons[gBattlerAttacker].types[0] != TYPE_ROCK
                && gBattleMons[gBattlerAttacker].types[0] != TYPE_STEEL
                && gBattleMons[gBattlerAttacker].types[0] != TYPE_GROUND
                && gBattleMons[gBattlerAttacker].types[1] != TYPE_ROCK
                && gBattleMons[gBattlerAttacker].types[1] != TYPE_STEEL
                && gBattleMons[gBattlerAttacker].types[1] != TYPE_GROUND
                && gBattleMons[gBattlerAttacker].ability != ABILITY_SAND_VEIL
                && !(gStatuses3[gBattlerAttacker] & STATUS3_UNDERGROUND)
                && !(gStatuses3[gBattlerAttacker] & STATUS3_UNDERWATER))
            {
                gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 16;
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;
            }
            else
            {
                gBattleMoveDamage = 0;
            }
        }
        if (gBattleWeather & B_WEATHER_HAIL)
        {
            if (!IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_ICE)
                && !(gStatuses3[gBattlerAttacker] & STATUS3_UNDERGROUND)
                && !(gStatuses3[gBattlerAttacker] & STATUS3_UNDERWATER))
            {
                gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 16;
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;
            }
            else
            {
                gBattleMoveDamage = 0;
            }
        }
    }
    else
    {
        gBattleMoveDamage = 0;
    }

    if (gAbsentBattlerFlags & gBitTable[gBattlerAttacker])
        gBattleMoveDamage = 0;

    gBattlescriptCurrInstr++;
}

static void Cmd_tryinfatuating(void)
{
    struct Pokemon *monAttacker, *monTarget;
    u16 speciesAttacker, speciesTarget;
    u32 personalityAttacker, personalityTarget;

    if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
        monAttacker = &gPlayerParty[gBattlerPartyIndexes[gBattlerAttacker]];
    else
        monAttacker = &gEnemyParty[gBattlerPartyIndexes[gBattlerAttacker]];

    if (GetBattlerSide(gBattlerTarget) == B_SIDE_PLAYER)
        monTarget = &gPlayerParty[gBattlerPartyIndexes[gBattlerTarget]];
    else
        monTarget = &gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]];

    speciesAttacker = GetMonData(monAttacker, MON_DATA_SPECIES);
    personalityAttacker = GetMonData(monAttacker, MON_DATA_PERSONALITY);

    speciesTarget = GetMonData(monTarget, MON_DATA_SPECIES);
    personalityTarget = GetMonData(monTarget, MON_DATA_PERSONALITY);

    if (gBattleMons[gBattlerTarget].ability == ABILITY_OBLIVIOUS)
    {
        gBattlescriptCurrInstr = BattleScript_ObliviousPreventsAttraction;
        gLastUsedAbility = ABILITY_OBLIVIOUS;
        RecordAbilityBattle(gBattlerTarget, ABILITY_OBLIVIOUS);
    }
    else
    {
        if (GetGenderFromSpeciesAndPersonality(speciesAttacker, personalityAttacker) == GetGenderFromSpeciesAndPersonality(speciesTarget, personalityTarget)
            || gBattleMons[gBattlerTarget].status2 & STATUS2_INFATUATION
            || GetGenderFromSpeciesAndPersonality(speciesAttacker, personalityAttacker) == MON_GENDERLESS
            || GetGenderFromSpeciesAndPersonality(speciesTarget, personalityTarget) == MON_GENDERLESS)
        {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        }
        else
        {
            gBattleMons[gBattlerTarget].status2 |= STATUS2_INFATUATED_WITH(gBattlerAttacker);
            gBattlescriptCurrInstr += 5;
        }
    }
}

static void Cmd_updatestatusicon(void)
{
    if (gBattleControllerExecFlags)
        return;

    if (gBattlescriptCurrInstr[1] != BS_ATTACKER_WITH_PARTNER)
    {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        BtlController_EmitStatusIconUpdate(BUFFER_A, gBattleMons[gActiveBattler].status1, gBattleMons[gActiveBattler].status2);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 2;
    }
    else
    {
        gActiveBattler = gBattlerAttacker;
        if (!(gAbsentBattlerFlags & gBitTable[gActiveBattler]))
        {
            BtlController_EmitStatusIconUpdate(BUFFER_A, gBattleMons[gActiveBattler].status1, gBattleMons[gActiveBattler].status2);
            MarkBattlerForControllerExec(gActiveBattler);
        }
        if ((gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
        {
            gActiveBattler = GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gBattlerAttacker)));
            if (!(gAbsentBattlerFlags & gBitTable[gActiveBattler]))
            {
                BtlController_EmitStatusIconUpdate(BUFFER_A, gBattleMons[gActiveBattler].status1, gBattleMons[gActiveBattler].status2);
                MarkBattlerForControllerExec(gActiveBattler);
            }
        }
        gBattlescriptCurrInstr += 2;
    }
}

static void Cmd_setmist(void)
{
    if (gSideTimers[GET_BATTLER_SIDE(gBattlerAttacker)].mistTimer)
    {
        gMoveResultFlags |= MOVE_RESULT_FAILED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MIST_FAILED;
    }
    else
    {
        gSideTimers[GET_BATTLER_SIDE(gBattlerAttacker)].mistTimer = 5;
        gSideTimers[GET_BATTLER_SIDE(gBattlerAttacker)].mistBattlerId = gBattlerAttacker;
        gSideStatuses[GET_BATTLER_SIDE(gBattlerAttacker)] |= SIDE_STATUS_MIST;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SET_MIST;
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_setfocusenergy(void)
{
    if (gBattleMons[gBattlerAttacker].status2 & STATUS2_FOCUS_ENERGY)
    {
        gMoveResultFlags |= MOVE_RESULT_FAILED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_FOCUS_ENERGY_FAILED;
    }
    else
    {
        gBattleMons[gBattlerAttacker].status2 |= STATUS2_FOCUS_ENERGY;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_GETTING_PUMPED;
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_transformdataexecution(void)
{
    gChosenMove = MOVE_UNAVAILABLE;
    gBattlescriptCurrInstr++;
    if (gBattleMons[gBattlerTarget].status2 & STATUS2_TRANSFORMED
        || gStatuses3[gBattlerTarget] & STATUS3_SEMI_INVULNERABLE)
    {
        gMoveResultFlags |= MOVE_RESULT_FAILED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TRANSFORM_FAILED;
    }
    else
    {
        s32 i;
        u8 *battleMonAttacker, *battleMonTarget;

        gBattleMons[gBattlerAttacker].status2 |= STATUS2_TRANSFORMED;
        gDisableStructs[gBattlerAttacker].disabledMove = MOVE_NONE;
        gDisableStructs[gBattlerAttacker].disableTimer = 0;
        gDisableStructs[gBattlerAttacker].transformedMonPersonality = gBattleMons[gBattlerTarget].personality;
        gDisableStructs[gBattlerAttacker].mimickedMoves = 0;

        PREPARE_SPECIES_BUFFER(gBattleTextBuff1, gBattleMons[gBattlerTarget].species)

        battleMonAttacker = (u8 *)(&gBattleMons[gBattlerAttacker]);
        battleMonTarget = (u8 *)(&gBattleMons[gBattlerTarget]);

        for (i = 0; i < offsetof(struct BattlePokemon, pp); i++)
            battleMonAttacker[i] = battleMonTarget[i];

        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            if (gBattleMoves[gBattleMons[gBattlerAttacker].moves[i]].pp < 5)
                gBattleMons[gBattlerAttacker].pp[i] = gBattleMoves[gBattleMons[gBattlerAttacker].moves[i]].pp;
            else
                gBattleMons[gBattlerAttacker].pp[i] = 5;
        }

        gActiveBattler = gBattlerAttacker;
        BtlController_EmitResetActionMoveSelection(BUFFER_A, RESET_MOVE_SELECTION);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TRANSFORMED;
    }
}

static void Cmd_setsubstitute(void)
{
    u32 hp = gBattleMons[gBattlerAttacker].maxHP / 4;
    if (gBattleMons[gBattlerAttacker].maxHP / 4 == 0)
        hp = 1;

    if (gBattleMons[gBattlerAttacker].hp <= hp)
    {
        gBattleMoveDamage = 0;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SUBSTITUTE_FAILED;
    }
    else
    {
        gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 4; // one bit value will only work for Pokémon which max hp can go to 1020(which is more than possible in games)
        if (gBattleMoveDamage == 0)
            gBattleMoveDamage = 1;

        gBattleMons[gBattlerAttacker].status2 |= STATUS2_SUBSTITUTE;
        gBattleMons[gBattlerAttacker].status2 &= ~STATUS2_WRAPPED;
        gDisableStructs[gBattlerAttacker].substituteHP = gBattleMoveDamage;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SET_SUBSTITUTE;
        gHitMarker |= HITMARKER_IGNORE_SUBSTITUTE;
    }

    gBattlescriptCurrInstr++;
}

static bool8 IsMoveUncopyableByMimic(u16 move)
{
    s32 i;
    for (i = 0; sMovesForbiddenToCopy[i] != MIMIC_FORBIDDEN_END
                && sMovesForbiddenToCopy[i] != move; i++);

    return (sMovesForbiddenToCopy[i] != MIMIC_FORBIDDEN_END);
}

static void Cmd_mimicattackcopy(void)
{
    gChosenMove = MOVE_UNAVAILABLE;

    if (IsMoveUncopyableByMimic(gLastMoves[gBattlerTarget])
        || gBattleMons[gBattlerAttacker].status2 & STATUS2_TRANSFORMED
        || gLastMoves[gBattlerTarget] == MOVE_NONE
        || gLastMoves[gBattlerTarget] == MOVE_UNAVAILABLE)
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        int i;

        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            if (gBattleMons[gBattlerAttacker].moves[i] == gLastMoves[gBattlerTarget])
                break;
        }

        if (i == MAX_MON_MOVES)
        {
            gBattleMons[gBattlerAttacker].moves[gCurrMovePos] = gLastMoves[gBattlerTarget];
            if (gBattleMoves[gLastMoves[gBattlerTarget]].pp < 5)
                gBattleMons[gBattlerAttacker].pp[gCurrMovePos] = gBattleMoves[gLastMoves[gBattlerTarget]].pp;
            else
                gBattleMons[gBattlerAttacker].pp[gCurrMovePos] = 5;

            PREPARE_MOVE_BUFFER(gBattleTextBuff1, gLastMoves[gBattlerTarget])

            gDisableStructs[gBattlerAttacker].mimickedMoves |= gBitTable[gCurrMovePos];
            gBattlescriptCurrInstr += 5;
        }
        else
        {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        }
    }
}

static void Cmd_metronome(void)
{
    while (TRUE)
    {
        s32 i;

    #if MOVES_COUNT < 512
        // Original GF method of move selection is to pick a random
        // number between 1-511. 355-511 are not valid moves, so if it
        // picks in this range it retries. If MOVES_COUNT exceeds 511 we
        // instead use a simpler solution.
        gCurrentMove = (Random() & 0x1FF) + 1;
        if (gCurrentMove >= MOVES_COUNT)
            continue;
    #else
        // Just pick a valid move value (between 1 and MOVES_COUNT-1)
        gCurrentMove = (Random() % (MOVES_COUNT - 1)) + 1;
    #endif

        for (i = 0; i < MAX_MON_MOVES; i++); // ?

        i = -1;
        while (TRUE)
        {
            i++;
            if (sMovesForbiddenToCopy[i] == gCurrentMove)
                break;
            if (sMovesForbiddenToCopy[i] == METRONOME_FORBIDDEN_END)
                break;
        }

        if (sMovesForbiddenToCopy[i] == METRONOME_FORBIDDEN_END)
        {
            gHitMarker &= ~HITMARKER_ATTACKSTRING_PRINTED;
            gBattlescriptCurrInstr = gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect];
            gBattlerTarget = GetMoveTarget(gCurrentMove, NO_TARGET_OVERRIDE);
            return;
        }
    }
}

static void Cmd_dmgtolevel(void)
{
    gBattleMoveDamage = gBattleMons[gBattlerAttacker].level;
    gBattlescriptCurrInstr++;
}

static void Cmd_psywavedamageeffect(void)
{
    s32 randDamage;

    while ((randDamage = Random() % 16) > 10);

    randDamage *= 10;
    gBattleMoveDamage = gBattleMons[gBattlerAttacker].level * (randDamage + 50) / 100;
    gBattlescriptCurrInstr++;
}

static void Cmd_counterdamagecalculator(void)
{
    u8 sideAttacker = GetBattlerSide(gBattlerAttacker);
    u8 sideTarget = GetBattlerSide(gProtectStructs[gBattlerAttacker].physicalBattlerId);

    if (gProtectStructs[gBattlerAttacker].physicalDmg
        && sideAttacker != sideTarget
        && gBattleMons[gProtectStructs[gBattlerAttacker].physicalBattlerId].hp)
    {
        gBattleMoveDamage = gProtectStructs[gBattlerAttacker].physicalDmg * 2;

        if (gSideTimers[sideTarget].followmeTimer && gBattleMons[gSideTimers[sideTarget].followmeTarget].hp)
            gBattlerTarget = gSideTimers[sideTarget].followmeTarget;
        else
            gBattlerTarget = gProtectStructs[gBattlerAttacker].physicalBattlerId;

        gBattlescriptCurrInstr += 5;
    }
    else
    {
        gSpecialStatuses[gBattlerAttacker].ppNotAffectedByPressure = 1;
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

// A copy of Cmd with the physical -> special field changes
static void Cmd_mirrorcoatdamagecalculator(void)
{
    u8 sideAttacker = GetBattlerSide(gBattlerAttacker);
    u8 sideTarget = GetBattlerSide(gProtectStructs[gBattlerAttacker].specialBattlerId);

    if (gProtectStructs[gBattlerAttacker].specialDmg && sideAttacker != sideTarget && gBattleMons[gProtectStructs[gBattlerAttacker].specialBattlerId].hp)
    {
        gBattleMoveDamage = gProtectStructs[gBattlerAttacker].specialDmg * 2;

        if (gSideTimers[sideTarget].followmeTimer && gBattleMons[gSideTimers[sideTarget].followmeTarget].hp)
            gBattlerTarget = gSideTimers[sideTarget].followmeTarget;
        else
            gBattlerTarget = gProtectStructs[gBattlerAttacker].specialBattlerId;

        gBattlescriptCurrInstr += 5;
    }
    else
    {
        gSpecialStatuses[gBattlerAttacker].ppNotAffectedByPressure = 1;
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_disablelastusedattack(void)
{
    s32 i;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (gBattleMons[gBattlerTarget].moves[i] == gLastMoves[gBattlerTarget])
            break;
    }
    if (gDisableStructs[gBattlerTarget].disabledMove == MOVE_NONE
        && i != MAX_MON_MOVES && gBattleMons[gBattlerTarget].pp[i] != 0)
    {
        PREPARE_MOVE_BUFFER(gBattleTextBuff1, gBattleMons[gBattlerTarget].moves[i])

        gDisableStructs[gBattlerTarget].disabledMove = gBattleMons[gBattlerTarget].moves[i];
        gDisableStructs[gBattlerTarget].disableTimer = (Random() & 3) + 2;
        gDisableStructs[gBattlerTarget].disableTimerStartValue = gDisableStructs[gBattlerTarget].disableTimer; // used to save the random amount of turns?
        gBattlescriptCurrInstr += 5;
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_trysetencore(void)
{
    s32 i;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (gBattleMons[gBattlerTarget].moves[i] == gLastMoves[gBattlerTarget])
            break;
    }

    if (gLastMoves[gBattlerTarget] == MOVE_STRUGGLE
        || gLastMoves[gBattlerTarget] == MOVE_ENCORE
        || gLastMoves[gBattlerTarget] == MOVE_MIRROR_MOVE)
    {
        i = MAX_MON_MOVES;
    }

    if (gDisableStructs[gBattlerTarget].encoredMove == MOVE_NONE
        && i != MAX_MON_MOVES && gBattleMons[gBattlerTarget].pp[i] != 0)
    {
        gDisableStructs[gBattlerTarget].encoredMove = gBattleMons[gBattlerTarget].moves[i];
        gDisableStructs[gBattlerTarget].encoredMovePos = i;
        gDisableStructs[gBattlerTarget].encoreTimer = (Random() & 3) + 3;
        gDisableStructs[gBattlerTarget].encoreTimerStartValue = gDisableStructs[gBattlerTarget].encoreTimer;
        gBattlescriptCurrInstr += 5;
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_painsplitdmgcalc(void)
{
    if (!(gBattleMons[gBattlerTarget].status2 & STATUS2_SUBSTITUTE))
    {
        s32 hpDiff = (gBattleMons[gBattlerAttacker].hp + gBattleMons[gBattlerTarget].hp) / 2;
        s32 painSplitHp = gBattleMoveDamage = gBattleMons[gBattlerTarget].hp - hpDiff;
        u8 *storeLoc = (void *)(&gBattleScripting.painSplitHp);

        storeLoc[0] = (painSplitHp);
        storeLoc[1] = (painSplitHp & 0x0000FF00) >> 8;
        storeLoc[2] = (painSplitHp & 0x00FF0000) >> 16;
        storeLoc[3] = (painSplitHp & 0xFF000000) >> 24;

        gBattleMoveDamage = gBattleMons[gBattlerAttacker].hp - hpDiff;
        gSpecialStatuses[gBattlerTarget].shellBellDmg = IGNORE_SHELL_BELL;

        gBattlescriptCurrInstr += 5;
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

// Conversion 2
static void Cmd_settypetorandomresistance(void)
{
    if (gLastLandedMoves[gBattlerAttacker] == MOVE_NONE
     || gLastLandedMoves[gBattlerAttacker] == MOVE_UNAVAILABLE)
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else if (IsTwoTurnsMove(gLastLandedMoves[gBattlerAttacker])
            && gBattleMons[gLastHitBy[gBattlerAttacker]].status2 & STATUS2_MULTIPLETURNS)
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        s32 i, j, rands;

        for (rands = 0; rands < 1000; rands++)
        {
            while (((i = Random() % 128) > sizeof(gTypeEffectiveness) / 3));

            i *= 3;

            if (TYPE_EFFECT_ATK_TYPE(i) == gLastHitByType[gBattlerAttacker]
                && TYPE_EFFECT_MULTIPLIER(i) <= TYPE_MUL_NOT_EFFECTIVE
                && !IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_EFFECT_DEF_TYPE(i)))
            {
                SET_BATTLER_TYPE(gBattlerAttacker, TYPE_EFFECT_DEF_TYPE(i));
                PREPARE_TYPE_BUFFER(gBattleTextBuff1, TYPE_EFFECT_DEF_TYPE(i));

                gBattlescriptCurrInstr += 5;
                return;
            }
        }

        for (j = 0, rands = 0; rands < sizeof(gTypeEffectiveness); j += 3, rands += 3)
        {
            switch (TYPE_EFFECT_ATK_TYPE(j))
            {
            case TYPE_ENDTABLE:
            case TYPE_FORESIGHT:
                break;
            default:
                if (TYPE_EFFECT_ATK_TYPE(j) == gLastHitByType[gBattlerAttacker]
                 && TYPE_EFFECT_MULTIPLIER(j) <= 5
                 && !IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_EFFECT_DEF_TYPE(i)))
                {
                    SET_BATTLER_TYPE(gBattlerAttacker, TYPE_EFFECT_DEF_TYPE(rands));
                    PREPARE_TYPE_BUFFER(gBattleTextBuff1, TYPE_EFFECT_DEF_TYPE(rands))

                    gBattlescriptCurrInstr += 5;
                    return;
                }
                break;
            }
        }

        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_setalwayshitflag(void)
{
    gStatuses3[gBattlerTarget] &= ~STATUS3_ALWAYS_HITS;
    gStatuses3[gBattlerTarget] |= STATUS3_ALWAYS_HITS_TURN(2);
    gDisableStructs[gBattlerTarget].battlerWithSureHit = gBattlerAttacker;
    gBattlescriptCurrInstr++;
}

// Sketch
static void Cmd_copymovepermanently(void)
{
    gChosenMove = MOVE_UNAVAILABLE;

    if (!(gBattleMons[gBattlerAttacker].status2 & STATUS2_TRANSFORMED)
        && gLastPrintedMoves[gBattlerTarget] != MOVE_STRUGGLE
        && gLastPrintedMoves[gBattlerTarget] != MOVE_NONE
        && gLastPrintedMoves[gBattlerTarget] != MOVE_UNAVAILABLE
        && gLastPrintedMoves[gBattlerTarget] != MOVE_SKETCH)
    {
        s32 i;

        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            if (gBattleMons[gBattlerAttacker].moves[i] == MOVE_SKETCH)
                continue;
            if (gBattleMons[gBattlerAttacker].moves[i] == gLastPrintedMoves[gBattlerTarget])
                break;
        }

        if (i != MAX_MON_MOVES)
        {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        }
        else // sketch worked
        {
            struct MovePpInfo movePpData;

            gBattleMons[gBattlerAttacker].moves[gCurrMovePos] = gLastPrintedMoves[gBattlerTarget];
            gBattleMons[gBattlerAttacker].pp[gCurrMovePos] = gBattleMoves[gLastPrintedMoves[gBattlerTarget]].pp;
            gActiveBattler = gBattlerAttacker;

            for (i = 0; i < MAX_MON_MOVES; i++)
            {
                movePpData.moves[i] = gBattleMons[gBattlerAttacker].moves[i];
                movePpData.pp[i] = gBattleMons[gBattlerAttacker].pp[i];
            }
            movePpData.ppBonuses = gBattleMons[gBattlerAttacker].ppBonuses;

            BtlController_EmitSetMonData(BUFFER_A, REQUEST_MOVES_PP_BATTLE, 0, sizeof(movePpData), &movePpData);
            MarkBattlerForControllerExec(gActiveBattler);

            PREPARE_MOVE_BUFFER(gBattleTextBuff1, gLastPrintedMoves[gBattlerTarget])

            gBattlescriptCurrInstr += 5;
        }
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static bool8 IsTwoTurnsMove(u16 move)
{
    if (gBattleMoves[move].effect == EFFECT_SKULL_BASH
     || gBattleMoves[move].effect == EFFECT_RAZOR_WIND
     || gBattleMoves[move].effect == EFFECT_SKY_ATTACK
     || gBattleMoves[move].effect == EFFECT_SOLAR_BEAM
     || gBattleMoves[move].effect == EFFECT_SEMI_INVULNERABLE
     || gBattleMoves[move].effect == EFFECT_BIDE)
        return TRUE;
    else
        return FALSE;
}

static bool8 IsInvalidForSleepTalkOrAssist(u16 move)
{
    if (move == MOVE_NONE
     || move == MOVE_SLEEP_TALK
     || move == MOVE_ASSIST
     || move == MOVE_MIRROR_MOVE
     || move == MOVE_METRONOME)
        return TRUE;
    else
        return FALSE;
}

static u8 AttacksThisTurn(u8 battlerId, u16 move) // Note: returns 1 if it's a charging turn, otherwise 2
{
    // first argument is unused
    if (gBattleMoves[move].effect == EFFECT_SOLAR_BEAM
        && (gBattleWeather & B_WEATHER_SUN))
        return 2;

    if (gBattleMoves[move].effect == EFFECT_SKULL_BASH
     || gBattleMoves[move].effect == EFFECT_RAZOR_WIND
     || gBattleMoves[move].effect == EFFECT_SKY_ATTACK
     || gBattleMoves[move].effect == EFFECT_SOLAR_BEAM
     || gBattleMoves[move].effect == EFFECT_SEMI_INVULNERABLE
     || gBattleMoves[move].effect == EFFECT_BIDE)
    {
        if ((gHitMarker & HITMARKER_CHARGING))
            return 1;
    }
    return 2;
}

static void Cmd_trychoosesleeptalkmove(void)
{
    s32 i;
    u8 unusableMovesBits = 0;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (IsInvalidForSleepTalkOrAssist(gBattleMons[gBattlerAttacker].moves[i])
            || gBattleMons[gBattlerAttacker].moves[i] == MOVE_FOCUS_PUNCH
            || gBattleMons[gBattlerAttacker].moves[i] == MOVE_UPROAR
            || IsTwoTurnsMove(gBattleMons[gBattlerAttacker].moves[i]))
        {
            unusableMovesBits |= gBitTable[i];
        }
    }

    unusableMovesBits = CheckMoveLimitations(gBattlerAttacker, unusableMovesBits, ~MOVE_LIMITATION_PP);
    if (unusableMovesBits == ALL_MOVES_MASK) // all 4 moves cannot be chosen
    {
        gBattlescriptCurrInstr += 5;
    }
    else // at least one move can be chosen
    {
        u32 movePosition;

        do
        {
            movePosition = MOD(Random(), MAX_MON_MOVES);
        } while ((gBitTable[movePosition] & unusableMovesBits));

        gCalledMove = gBattleMons[gBattlerAttacker].moves[movePosition];
        gCurrMovePos = movePosition;
        gHitMarker &= ~HITMARKER_ATTACKSTRING_PRINTED;
        gBattlerTarget = GetMoveTarget(gCalledMove, NO_TARGET_OVERRIDE);
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_setdestinybond(void)
{
    gBattleMons[gBattlerAttacker].status2 |= STATUS2_DESTINY_BOND;
    gBattlescriptCurrInstr++;
}

static void TrySetDestinyBondToHappen(void)
{
    u8 sideAttacker = GetBattlerSide(gBattlerAttacker);
    u8 sideTarget = GetBattlerSide(gBattlerTarget);
    if (gBattleMons[gBattlerTarget].status2 & STATUS2_DESTINY_BOND
        && sideAttacker != sideTarget
        && !(gHitMarker & HITMARKER_GRUDGE))
    {
        gHitMarker |= HITMARKER_DESTINYBOND;
    }
}

static void Cmd_trysetdestinybondtohappen(void)
{
    TrySetDestinyBondToHappen();
    gBattlescriptCurrInstr++;
}

static void Cmd_remaininghptopower(void)
{
    s32 i;
    s32 hpFraction = GetScaledHPFraction(gBattleMons[gBattlerAttacker].hp, gBattleMons[gBattlerAttacker].maxHP, 48);

    for (i = 0; i < (s32) sizeof(sFlailHpScaleToPowerTable); i += 2)
    {
        if (hpFraction <= sFlailHpScaleToPowerTable[i])
            break;
    }

    gDynamicBasePower = sFlailHpScaleToPowerTable[i + 1];
    gBattlescriptCurrInstr++;
}

static void Cmd_tryspiteppreduce(void)
{
    if (gLastMoves[gBattlerTarget] != MOVE_NONE
     && gLastMoves[gBattlerTarget] != MOVE_UNAVAILABLE)
    {
        s32 i;

        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            if (gLastMoves[gBattlerTarget] == gBattleMons[gBattlerTarget].moves[i])
                break;
        }

        if (i != MAX_MON_MOVES && gBattleMons[gBattlerTarget].pp[i] > 1)
        {
            s32 ppToDeduct = (Random() & 3) + 2;
            if (gBattleMons[gBattlerTarget].pp[i] < ppToDeduct)
                ppToDeduct = gBattleMons[gBattlerTarget].pp[i];

            PREPARE_MOVE_BUFFER(gBattleTextBuff1, gLastMoves[gBattlerTarget])

            ConvertIntToDecimalStringN(gBattleTextBuff2, ppToDeduct, STR_CONV_MODE_LEFT_ALIGN, 1);

            PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff2, 1, ppToDeduct)

            gBattleMons[gBattlerTarget].pp[i] -= ppToDeduct;
            gActiveBattler = gBattlerTarget;

            // if (MOVE_IS_PERMANENT(gActiveBattler, i)), but backwards
            if (!(gDisableStructs[gActiveBattler].mimickedMoves & gBitTable[i])
                && !(gBattleMons[gActiveBattler].status2 & STATUS2_TRANSFORMED))
            {
                BtlController_EmitSetMonData(BUFFER_A, REQUEST_PPMOVE1_BATTLE + i, 0, sizeof(gBattleMons[gActiveBattler].pp[i]), &gBattleMons[gActiveBattler].pp[i]);
                MarkBattlerForControllerExec(gActiveBattler);
            }

            gBattlescriptCurrInstr += 5;

            if (gBattleMons[gBattlerTarget].pp[i] == 0)
                CancelMultiTurnMoves(gBattlerTarget);
        }
        else
        {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        }
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_healpartystatus(void)
{
    u32 zero = 0;
    u8 toHeal = 0;

    if (gCurrentMove == MOVE_HEAL_BELL)
    {
        struct Pokemon *party;
        s32 i;

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_BELL;

        if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
            party = gPlayerParty;
        else
            party = gEnemyParty;

        if (gBattleMons[gBattlerAttacker].ability != ABILITY_SOUNDPROOF)
        {
            gBattleMons[gBattlerAttacker].status1 = 0;
            gBattleMons[gBattlerAttacker].status2 &= ~STATUS2_NIGHTMARE;
        }
        else
        {
            RecordAbilityBattle(gBattlerAttacker, gBattleMons[gBattlerAttacker].ability);
            gBattleCommunication[MULTISTRING_CHOOSER] |= B_MSG_BELL_SOUNDPROOF_ATTACKER;
        }

        gActiveBattler = gBattleScripting.battler = GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gBattlerAttacker)));

        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
            && !(gAbsentBattlerFlags & gBitTable[gActiveBattler]))
        {
            if (gBattleMons[gActiveBattler].ability != ABILITY_SOUNDPROOF)
            {
                gBattleMons[gActiveBattler].status1 = 0;
                gBattleMons[gActiveBattler].status2 &= ~STATUS2_NIGHTMARE;
            }
            else
            {
                RecordAbilityBattle(gActiveBattler, gBattleMons[gActiveBattler].ability);
                gBattleCommunication[MULTISTRING_CHOOSER] |= B_MSG_BELL_SOUNDPROOF_PARTNER;
            }
        }

        // Because the above MULTISTRING_CHOOSER are ORd, if both are set then it will be B_MSG_BELL_BOTH_SOUNDPROOF

        for (i = 0; i < PARTY_SIZE; i++)
        {
            u16 species = GetMonData(&party[i], MON_DATA_SPECIES_OR_EGG);
            u8 abilityNum = GetMonData(&party[i], MON_DATA_ABILITY_NUM);

            if (species != SPECIES_NONE && species != SPECIES_EGG)
            {
                u8 ability;

                if (gBattlerPartyIndexes[gBattlerAttacker] == i)
                    ability = gBattleMons[gBattlerAttacker].ability;
                else if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
                         && gBattlerPartyIndexes[gActiveBattler] == i
                         && !(gAbsentBattlerFlags & gBitTable[gActiveBattler]))
                    ability = gBattleMons[gActiveBattler].ability;
                else
                    ability = GetAbilityBySpecies(species, abilityNum);

                if (ability != ABILITY_SOUNDPROOF)
                    toHeal |= (1 << i);
            }
        }
    }
    else // Aromatherapy
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SOOTHING_AROMA;
        toHeal = (1 << PARTY_SIZE) - 1;

        gBattleMons[gBattlerAttacker].status1 = 0;
        gBattleMons[gBattlerAttacker].status2 &= ~STATUS2_NIGHTMARE;

        gActiveBattler = GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gBattlerAttacker)));
        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
            && !(gAbsentBattlerFlags & gBitTable[gActiveBattler]))
        {
            gBattleMons[gActiveBattler].status1 = 0;
            gBattleMons[gActiveBattler].status2 &= ~STATUS2_NIGHTMARE;
        }

    }

    if (toHeal)
    {
        gActiveBattler = gBattlerAttacker;
        BtlController_EmitSetMonData(BUFFER_A, REQUEST_STATUS_BATTLE, toHeal, sizeof(zero), &zero);
        MarkBattlerForControllerExec(gActiveBattler);
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_cursetarget(void)
{
    if (gBattleMons[gBattlerTarget].status2 & STATUS2_CURSED)
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        gBattleMons[gBattlerTarget].status2 |= STATUS2_CURSED;
        gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 2;
        if (gBattleMoveDamage == 0)
            gBattleMoveDamage = 1;

        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_trysetspikes(void)
{
    u8 targetSide = BATTLE_OPPOSITE(GetBattlerSide(gBattlerAttacker));

    if (gSideTimers[targetSide].spikesAmount == 3)
    {
        gSpecialStatuses[gBattlerAttacker].ppNotAffectedByPressure = 1;
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        gSideStatuses[targetSide] |= SIDE_STATUS_SPIKES;
        gSideTimers[targetSide].spikesAmount++;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_setforesight(void)
{
    gBattleMons[gBattlerTarget].status2 |= STATUS2_FORESIGHT;
    gBattlescriptCurrInstr++;
}

static void Cmd_trysetperishsong(void)
{
    s32 i;
    s32 notAffectedCount = 0;

    for (i = 0; i < gBattlersCount; i++)
    {
        if (gStatuses3[i] & STATUS3_PERISH_SONG
            || gBattleMons[i].ability == ABILITY_SOUNDPROOF)
        {
            notAffectedCount++;
        }
        else
        {
            gStatuses3[i] |= STATUS3_PERISH_SONG;
            gDisableStructs[i].perishSongTimer = 3;
            gDisableStructs[i].perishSongTimerStartValue = 3;
        }
    }

    PressurePPLoseOnUsingPerishSong(gBattlerAttacker);

    if (notAffectedCount == gBattlersCount)
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    else
        gBattlescriptCurrInstr += 5;
}

static void Cmd_rolloutdamagecalculation(void)
{
    if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
    {
        CancelMultiTurnMoves(gBattlerAttacker);
        gBattlescriptCurrInstr = BattleScript_MoveMissedPause;
    }
    else
    {
        s32 i;

        if (!(gBattleMons[gBattlerAttacker].status2 & STATUS2_MULTIPLETURNS)) // first hit
        {
            gDisableStructs[gBattlerAttacker].rolloutTimer = 5;
            gDisableStructs[gBattlerAttacker].rolloutTimerStartValue = 5;
            gBattleMons[gBattlerAttacker].status2 |= STATUS2_MULTIPLETURNS;
            gLockedMoves[gBattlerAttacker] = gCurrentMove;
        }
        if (--gDisableStructs[gBattlerAttacker].rolloutTimer == 0) // last hit
        {
            gBattleMons[gBattlerAttacker].status2 &= ~STATUS2_MULTIPLETURNS;
        }

        gDynamicBasePower = gBattleMoves[gCurrentMove].power;

        for (i = 1; i < (5 - gDisableStructs[gBattlerAttacker].rolloutTimer); i++)
            gDynamicBasePower *= 2;

        if (gBattleMons[gBattlerAttacker].status2 & STATUS2_DEFENSE_CURL)
            gDynamicBasePower *= 2;

        gBattlescriptCurrInstr++;
    }
}

static void Cmd_jumpifconfusedandstatmaxed(void)
{
    if (gBattleMons[gBattlerTarget].status2 & STATUS2_CONFUSION
        && gBattleMons[gBattlerTarget].statStages[gBattlescriptCurrInstr[1]] == MAX_STAT_STAGE)
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
    else
        gBattlescriptCurrInstr += 6;
}

static void Cmd_furycuttercalc(void)
{
    if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
    {
        gDisableStructs[gBattlerAttacker].furyCutterCounter = 0;
        gBattlescriptCurrInstr = BattleScript_MoveMissedPause;
    }
    else
    {
        s32 i;

        if (gDisableStructs[gBattlerAttacker].furyCutterCounter != 5)
            gDisableStructs[gBattlerAttacker].furyCutterCounter++;

        gDynamicBasePower = gBattleMoves[gCurrentMove].power;

        for (i = 1; i < gDisableStructs[gBattlerAttacker].furyCutterCounter; i++)
            gDynamicBasePower *= 2;

        gBattlescriptCurrInstr++;
    }
}

static void Cmd_friendshiptodamagecalculation(void)
{
    if (gBattleMoves[gCurrentMove].effect == EFFECT_RETURN)
        gDynamicBasePower = 10 * (gBattleMons[gBattlerAttacker].friendship) / 25;
    else // EFFECT_FRUSTRATION
        gDynamicBasePower = 10 * (MAX_FRIENDSHIP - gBattleMons[gBattlerAttacker].friendship) / 25;

    gBattlescriptCurrInstr++;
}

static void Cmd_presentdamagecalculation(void)
{
    s32 rand = Random() & 0xFF;

    if (rand < 102)
    {
        gDynamicBasePower = 40;
    }
    else if (rand < 178)
    {
        gDynamicBasePower = 80;
    }
    else if (rand < 204)
    {
        gDynamicBasePower = 120;
    }
    else
    {
        gBattleMoveDamage = gBattleMons[gBattlerTarget].maxHP / 4;
        if (gBattleMoveDamage == 0)
            gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
    }

    if (rand < 204)
    {
        gBattlescriptCurrInstr = BattleScript_HitFromCritCalc;
    }
    else if (gBattleMons[gBattlerTarget].maxHP == gBattleMons[gBattlerTarget].hp)
    {
        gBattlescriptCurrInstr = BattleScript_AlreadyAtFullHp;
    }
    else
    {
        gMoveResultFlags &= ~MOVE_RESULT_DOESNT_AFFECT_FOE;
        gBattlescriptCurrInstr = BattleScript_PresentHealTarget;
    }
}

static void Cmd_setsafeguard(void)
{
    if (gSideStatuses[GET_BATTLER_SIDE(gBattlerAttacker)] & SIDE_STATUS_SAFEGUARD)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SIDE_STATUS_FAILED;
    }
    else
    {
        gSideStatuses[GET_BATTLER_SIDE(gBattlerAttacker)] |= SIDE_STATUS_SAFEGUARD;
        gSideTimers[GET_BATTLER_SIDE(gBattlerAttacker)].safeguardTimer = 5;
        gSideTimers[GET_BATTLER_SIDE(gBattlerAttacker)].safeguardBattlerId = gBattlerAttacker;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SET_SAFEGUARD;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_magnitudedamagecalculation(void)
{
    s32 magnitude = Random() % 100;

    if (magnitude < 5)
    {
        gDynamicBasePower = 10;
        magnitude = 4;
    }
    else if (magnitude < 15)
    {
        gDynamicBasePower = 30;
        magnitude = 5;
    }
    else if (magnitude < 35)
    {
        gDynamicBasePower = 50;
        magnitude = 6;
    }
    else if (magnitude < 65)
    {
        gDynamicBasePower = 70;
        magnitude = 7;
    }
    else if (magnitude < 85)
    {
        gDynamicBasePower = 90;
        magnitude = 8;
    }
    else if (magnitude < 95)
    {
        gDynamicBasePower = 110;
        magnitude = 9;
    }
    else
    {
        gDynamicBasePower = 150;
        magnitude = 10;
    }

    PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff1, 2, magnitude)

    for (gBattlerTarget = 0; gBattlerTarget < gBattlersCount; gBattlerTarget++)
    {
        if (gBattlerTarget == gBattlerAttacker)
            continue;
        if (!(gAbsentBattlerFlags & gBitTable[gBattlerTarget])) // a valid target was found
            break;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_jumpifnopursuitswitchdmg(void)
{
    if (gMultiHitCounter == 1)
    {
        if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
            gBattlerTarget = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        else
            gBattlerTarget = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
    }
    else
    {
        if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
            gBattlerTarget = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
        else
            gBattlerTarget = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
    }

    if (gChosenActionByBattler[gBattlerTarget] == B_ACTION_USE_MOVE
        && gBattlerAttacker == *(gBattleStruct->moveTarget + gBattlerTarget)
        && !(gBattleMons[gBattlerTarget].status1 & (STATUS1_SLEEP | STATUS1_FREEZE))
        && gBattleMons[gBattlerAttacker].hp
        && !gDisableStructs[gBattlerTarget].truantCounter
        && gChosenMoveByBattler[gBattlerTarget] == MOVE_PURSUIT)
    {
        s32 i;

        for (i = 0; i < gBattlersCount; i++)
        {
            if (gBattlerByTurnOrder[i] == gBattlerTarget)
                gActionsByTurnOrder[i] = B_ACTION_TRY_FINISH;
        }

        gCurrentMove = MOVE_PURSUIT;
        gCurrMovePos = gChosenMovePos = *(gBattleStruct->chosenMovePositions + gBattlerTarget);
        gBattlescriptCurrInstr += 5;
        gBattleScripting.animTurn = 1;
        gHitMarker &= ~HITMARKER_ATTACKSTRING_PRINTED;
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_setsunny(void)
{
    if (gBattleWeather & B_WEATHER_SUN)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_WEATHER_FAILED;
    }
    else
    {
        gBattleWeather = B_WEATHER_SUN_TEMPORARY;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STARTED_SUNLIGHT;
        gWishFutureKnock.weatherDuration = 5;
    }

    gBattlescriptCurrInstr++;
}

// Belly Drum
static void Cmd_maxattackhalvehp(void)
{
    u32 halfHp = gBattleMons[gBattlerAttacker].maxHP / 2;

    if (!(gBattleMons[gBattlerAttacker].maxHP / 2))
        halfHp = 1;

    if (gBattleMons[gBattlerAttacker].statStages[STAT_ATK] < MAX_STAT_STAGE
        && gBattleMons[gBattlerAttacker].hp > halfHp)
    {
        gBattleMons[gBattlerAttacker].statStages[STAT_ATK] = MAX_STAT_STAGE;
        gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 2;
        if (gBattleMoveDamage == 0)
            gBattleMoveDamage = 1;

        gBattlescriptCurrInstr += 5;
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

// Psych Up
static void Cmd_copyfoestats(void)
{
    s32 i;

    for (i = 0; i < NUM_BATTLE_STATS; i++)
    {
        gBattleMons[gBattlerAttacker].statStages[i] = gBattleMons[gBattlerTarget].statStages[i];
    }

    gBattlescriptCurrInstr += 5; // Has an unused jump ptr(possibly for a failed attempt) parameter.
}

static void Cmd_rapidspinfree(void)
{
    if (gBattleMons[gBattlerAttacker].status2 & STATUS2_WRAPPED)
    {
        gBattleScripting.battler = gBattlerTarget;
        gBattleMons[gBattlerAttacker].status2 &= ~STATUS2_WRAPPED;
        gBattlerTarget = *(gBattleStruct->wrappedBy + gBattlerAttacker);

        gBattleTextBuff1[0] = B_BUFF_PLACEHOLDER_BEGIN;
        gBattleTextBuff1[1] = B_BUFF_MOVE;
        gBattleTextBuff1[2] = *(gBattleStruct->wrappedMove + gBattlerAttacker * 2 + 0);
        gBattleTextBuff1[3] = *(gBattleStruct->wrappedMove + gBattlerAttacker * 2 + 1);
        gBattleTextBuff1[4] = B_BUFF_EOS;

        BattleScriptPushCursor();
        gBattlescriptCurrInstr = BattleScript_WrapFree;
    }
    else if (gStatuses3[gBattlerAttacker] & STATUS3_LEECHSEED)
    {
        gStatuses3[gBattlerAttacker] &= ~STATUS3_LEECHSEED;
        gStatuses3[gBattlerAttacker] &= ~STATUS3_LEECHSEED_BATTLER;
        BattleScriptPushCursor();
        gBattlescriptCurrInstr = BattleScript_LeechSeedFree;
    }
    else if (gSideStatuses[GetBattlerSide(gBattlerAttacker)] & SIDE_STATUS_SPIKES)
    {
        gSideStatuses[GetBattlerSide(gBattlerAttacker)] &= ~SIDE_STATUS_SPIKES;
        gSideTimers[GetBattlerSide(gBattlerAttacker)].spikesAmount = 0;
        BattleScriptPushCursor();
        gBattlescriptCurrInstr = BattleScript_SpikesFree;
    }
    else
    {
        gBattlescriptCurrInstr++;
    }
}

static void Cmd_setdefensecurlbit(void)
{
    gBattleMons[gBattlerAttacker].status2 |= STATUS2_DEFENSE_CURL;
    gBattlescriptCurrInstr++;
}

static void Cmd_recoverbasedonsunlight(void)
{
    gBattlerTarget = gBattlerAttacker;

    if (gBattleMons[gBattlerAttacker].hp != gBattleMons[gBattlerAttacker].maxHP)
    {
        if (gBattleWeather == 0 || !WEATHER_HAS_EFFECT)
            gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 2;
        else if (gBattleWeather & B_WEATHER_SUN)
            gBattleMoveDamage = 20 * gBattleMons[gBattlerAttacker].maxHP / 30;
        else // not sunny weather
            gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 4;

        if (gBattleMoveDamage == 0)
            gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;

        gBattlescriptCurrInstr += 5;
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_hiddenpowercalc(void)
{
    u8 powerBits = ((gBattleMons[gBattlerAttacker].hpIV & 2) >> 1)
                 | ((gBattleMons[gBattlerAttacker].attackIV & 2) << 0)
                 | ((gBattleMons[gBattlerAttacker].defenseIV & 2) << 1)
                 | ((gBattleMons[gBattlerAttacker].speedIV & 2) << 2)
                 | ((gBattleMons[gBattlerAttacker].spAttackIV & 2) << 3)
                 | ((gBattleMons[gBattlerAttacker].spDefenseIV & 2) << 4);

    u8 typeBits  = ((gBattleMons[gBattlerAttacker].hpIV & 1) << 0)
                 | ((gBattleMons[gBattlerAttacker].attackIV & 1) << 1)
                 | ((gBattleMons[gBattlerAttacker].defenseIV & 1) << 2)
                 | ((gBattleMons[gBattlerAttacker].speedIV & 1) << 3)
                 | ((gBattleMons[gBattlerAttacker].spAttackIV & 1) << 4)
                 | ((gBattleMons[gBattlerAttacker].spDefenseIV & 1) << 5);

    gDynamicBasePower = (40 * powerBits) / 63 + 30;

    // Subtract 3 instead of 1 below because 2 types are excluded (TYPE_NORMAL and TYPE_MYSTERY)
    // The final + 1 skips past Normal, and the following conditional skips TYPE_MYSTERY
    gBattleStruct->dynamicMoveType = ((NUMBER_OF_MON_TYPES - 3) * typeBits) / 63 + 1;
    if (gBattleStruct->dynamicMoveType >= TYPE_MYSTERY)
        gBattleStruct->dynamicMoveType++;
    gBattleStruct->dynamicMoveType |= F_DYNAMIC_TYPE_IGNORE_PHYSICALITY | F_DYNAMIC_TYPE_SET;

    gBattlescriptCurrInstr++;
}

static void Cmd_selectfirstvalidtarget(void)
{
    for (gBattlerTarget = 0; gBattlerTarget < gBattlersCount; gBattlerTarget++)
    {
        if (gBattlerTarget == gBattlerAttacker)
            continue;
        if (!(gAbsentBattlerFlags & gBitTable[gBattlerTarget]))
            break;
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_trysetfutureattack(void)
{
    if (gWishFutureKnock.futureSightCounter[gBattlerTarget] != 0)
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        gSideStatuses[GET_BATTLER_SIDE(gBattlerTarget)] |= SIDE_STATUS_FUTUREATTACK;
        gWishFutureKnock.futureSightMove[gBattlerTarget] = gCurrentMove;
        gWishFutureKnock.futureSightAttacker[gBattlerTarget] = gBattlerAttacker;
        gWishFutureKnock.futureSightCounter[gBattlerTarget] = 3;
        gWishFutureKnock.futureSightDmg[gBattlerTarget] = CalculateBaseDamage(&gBattleMons[gBattlerAttacker], &gBattleMons[gBattlerTarget], gCurrentMove,
                                                    gSideStatuses[GET_BATTLER_SIDE(gBattlerTarget)], 0,
                                                    0, gBattlerAttacker, gBattlerTarget);

        if (gProtectStructs[gBattlerAttacker].helpingHand)
            gWishFutureKnock.futureSightDmg[gBattlerTarget] = gWishFutureKnock.futureSightDmg[gBattlerTarget] * 15 / 10;

        if (gCurrentMove == MOVE_DOOM_DESIRE)
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_DOOM_DESIRE;
        else
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_FUTURE_SIGHT;

        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_trydobeatup(void)
{
    struct Pokemon *party;

    if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
        party = gPlayerParty;
    else
        party = gEnemyParty;

    if (gBattleMons[gBattlerTarget].hp == 0)
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        u8 beforeLoop = gBattleCommunication[0];
        for (;gBattleCommunication[0] < PARTY_SIZE; gBattleCommunication[0]++)
        {
            if (GetMonData(&party[gBattleCommunication[0]], MON_DATA_HP)
                && GetMonData(&party[gBattleCommunication[0]], MON_DATA_SPECIES_OR_EGG) != SPECIES_NONE
                && GetMonData(&party[gBattleCommunication[0]], MON_DATA_SPECIES_OR_EGG) != SPECIES_EGG
                && !GetMonData(&party[gBattleCommunication[0]], MON_DATA_STATUS))
                break;
        }

        if (gBattleCommunication[0] < PARTY_SIZE)
        {
            PREPARE_MON_NICK_WITH_PREFIX_BUFFER(gBattleTextBuff1, gBattlerAttacker, gBattleCommunication[0])

            gBattlescriptCurrInstr += 9;

            gBattleMoveDamage = gSpeciesInfo[GetMonData(&party[gBattleCommunication[0]], MON_DATA_SPECIES)].baseAttack;
            gBattleMoveDamage *= gBattleMoves[gCurrentMove].power;
            gBattleMoveDamage *= (GetMonData(&party[gBattleCommunication[0]], MON_DATA_LEVEL) * 2 / 5 + 2);
            gBattleMoveDamage /= gSpeciesInfo[gBattleMons[gBattlerTarget].species].baseDefense;
            gBattleMoveDamage = (gBattleMoveDamage / 50) + 2;
            if (gProtectStructs[gBattlerAttacker].helpingHand)
                gBattleMoveDamage = gBattleMoveDamage * 15 / 10;

            gBattleCommunication[0]++;
        }
        else if (beforeLoop != 0)
        {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        }
        else
        {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 5);
        }
    }
}

static void Cmd_setsemiinvulnerablebit(void)
{
    switch (gCurrentMove)
    {
    case MOVE_FLY:
    case MOVE_BOUNCE:
        gStatuses3[gBattlerAttacker] |= STATUS3_ON_AIR;
        break;
    case MOVE_DIG:
        gStatuses3[gBattlerAttacker] |= STATUS3_UNDERGROUND;
        break;
    case MOVE_DIVE:
        gStatuses3[gBattlerAttacker] |= STATUS3_UNDERWATER;
        break;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_clearsemiinvulnerablebit(void)
{
    switch (gCurrentMove)
    {
    case MOVE_FLY:
    case MOVE_BOUNCE:
        gStatuses3[gBattlerAttacker] &= ~STATUS3_ON_AIR;
        break;
    case MOVE_DIG:
        gStatuses3[gBattlerAttacker] &= ~STATUS3_UNDERGROUND;
        break;
    case MOVE_DIVE:
        gStatuses3[gBattlerAttacker] &= ~STATUS3_UNDERWATER;
        break;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_setminimize(void)
{
    if (gHitMarker & HITMARKER_OBEYS)
        gStatuses3[gBattlerAttacker] |= STATUS3_MINIMIZED;

    gBattlescriptCurrInstr++;
}

static void Cmd_sethail(void)
{
    if (gBattleWeather & B_WEATHER_HAIL)
    {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_WEATHER_FAILED;
    }
    else
    {
        gBattleWeather = B_WEATHER_HAIL_TEMPORARY;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STARTED_HAIL;
        gWishFutureKnock.weatherDuration = 5;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_trymemento(void)
{
    if (gBattleMons[gBattlerTarget].statStages[STAT_ATK] == MIN_STAT_STAGE
        && gBattleMons[gBattlerTarget].statStages[STAT_SPATK] == MIN_STAT_STAGE
        && gBattleCommunication[MISS_TYPE] != B_MSG_PROTECTED)
    {
        // Failed, unprotected target already has minimum Attack and Special Attack.
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        // Success, drop user's HP bar to 0
        gActiveBattler = gBattlerAttacker;
        gBattleMoveDamage = gBattleMons[gActiveBattler].hp;
        BtlController_EmitHealthBarUpdate(BUFFER_A, INSTANT_HP_BAR_DROP);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 5;
    }
}

// Follow Me
static void Cmd_setforcedtarget(void)
{
    gSideTimers[GetBattlerSide(gBattlerAttacker)].followmeTimer = 1;
    gSideTimers[GetBattlerSide(gBattlerAttacker)].followmeTarget = gBattlerAttacker;
    gBattlescriptCurrInstr++;
}

static void Cmd_setcharge(void)
{
    gStatuses3[gBattlerAttacker] |= STATUS3_CHARGED_UP;
    gDisableStructs[gBattlerAttacker].chargeTimer = 2;
    gDisableStructs[gBattlerAttacker].chargeTimerStartValue = 2;
    gBattlescriptCurrInstr++;
}

// Nature Power
static void Cmd_callenvironmentattack(void)
{
    gHitMarker &= ~HITMARKER_ATTACKSTRING_PRINTED;
    gCurrentMove = sNaturePowerMoves[gBattleEnvironment];
    gBattlerTarget = GetMoveTarget(gCurrentMove, NO_TARGET_OVERRIDE);
    BattleScriptPush(gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect]);
    gBattlescriptCurrInstr++;
}

// Refresh
static void Cmd_cureifburnedparalysedorpoisoned(void)
{
    if (gBattleMons[gBattlerAttacker].status1 & (STATUS1_POISON | STATUS1_BURN | STATUS1_PARALYSIS | STATUS1_TOXIC_POISON))
    {
        gBattleMons[gBattlerAttacker].status1 = 0;
        gBattlescriptCurrInstr += 5;
        gActiveBattler = gBattlerAttacker;
        BtlController_EmitSetMonData(BUFFER_A, REQUEST_STATUS_BATTLE, 0, sizeof(gBattleMons[gActiveBattler].status1), &gBattleMons[gActiveBattler].status1);
        MarkBattlerForControllerExec(gActiveBattler);
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_settorment(void)
{
    if (gBattleMons[gBattlerTarget].status2 & STATUS2_TORMENT)
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        gBattleMons[gBattlerTarget].status2 |= STATUS2_TORMENT;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_jumpifnodamage(void)
{
    if (gProtectStructs[gBattlerAttacker].physicalDmg || gProtectStructs[gBattlerAttacker].specialDmg)
        gBattlescriptCurrInstr += 5;
    else
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
}

static void Cmd_settaunt(void)
{
    if (gDisableStructs[gBattlerTarget].tauntTimer == 0)
    {
        gDisableStructs[gBattlerTarget].tauntTimer = 2;
        gDisableStructs[gBattlerTarget].tauntTimer2 = 2;
        gBattlescriptCurrInstr += 5;
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_trysethelpinghand(void)
{
    gBattlerTarget = GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gBattlerAttacker)));

    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
        && !(gAbsentBattlerFlags & gBitTable[gBattlerTarget])
        && !gProtectStructs[gBattlerAttacker].helpingHand
        && !gProtectStructs[gBattlerTarget].helpingHand)
    {
        gProtectStructs[gBattlerTarget].helpingHand = 1;
        gBattlescriptCurrInstr += 5;
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

// Trick
static void Cmd_tryswapitems(void)
{
    // opponent can't swap items with player in regular battles
    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_HILL
        || (GetBattlerSide(gBattlerAttacker) == B_SIDE_OPPONENT
            && !(gBattleTypeFlags & (BATTLE_TYPE_LINK
                                  | BATTLE_TYPE_EREADER_TRAINER
                                  | BATTLE_TYPE_FRONTIER
                                  | BATTLE_TYPE_SECRET_BASE
                                  | BATTLE_TYPE_RECORDED_LINK))))
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        u8 sideAttacker = GetBattlerSide(gBattlerAttacker);
        u8 sideTarget = GetBattlerSide(gBattlerTarget);

        // you can't swap items if they were knocked off in regular battles
        if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK
                             | BATTLE_TYPE_EREADER_TRAINER
                             | BATTLE_TYPE_FRONTIER
                             | BATTLE_TYPE_SECRET_BASE
                             | BATTLE_TYPE_RECORDED_LINK))
            && (gWishFutureKnock.knockedOffMons[sideAttacker] & gBitTable[gBattlerPartyIndexes[gBattlerAttacker]]
                || gWishFutureKnock.knockedOffMons[sideTarget] & gBitTable[gBattlerPartyIndexes[gBattlerTarget]]))
        {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        }
        // can't swap if two Pokémon don't have an item
        // or if either of them is an enigma berry or a mail
        else if ((gBattleMons[gBattlerAttacker].item == ITEM_NONE && gBattleMons[gBattlerTarget].item == ITEM_NONE)
                 || gBattleMons[gBattlerAttacker].item == ITEM_ENIGMA_BERRY
                 || gBattleMons[gBattlerTarget].item == ITEM_ENIGMA_BERRY
                 || IS_ITEM_MAIL(gBattleMons[gBattlerAttacker].item)
                 || IS_ITEM_MAIL(gBattleMons[gBattlerTarget].item))
        {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        }
        // check if ability prevents swapping
        else if (gBattleMons[gBattlerTarget].ability == ABILITY_STICKY_HOLD)
        {
            gBattlescriptCurrInstr = BattleScript_StickyHoldActivates;
            gLastUsedAbility = gBattleMons[gBattlerTarget].ability;
            RecordAbilityBattle(gBattlerTarget, gLastUsedAbility);
        }
        // took a while, but all checks passed and items can be safely swapped
        else
        {
            u16 oldItemAtk, *newItemAtk;

            newItemAtk = &gBattleStruct->changedItems[gBattlerAttacker];
            oldItemAtk = gBattleMons[gBattlerAttacker].item;
            *newItemAtk = gBattleMons[gBattlerTarget].item;

            gBattleMons[gBattlerAttacker].item = ITEM_NONE;
            gBattleMons[gBattlerTarget].item = oldItemAtk;

            gActiveBattler = gBattlerAttacker;
            BtlController_EmitSetMonData(BUFFER_A, REQUEST_HELDITEM_BATTLE, 0, sizeof(*newItemAtk), newItemAtk);
            MarkBattlerForControllerExec(gBattlerAttacker);

            gActiveBattler = gBattlerTarget;
            BtlController_EmitSetMonData(BUFFER_A, REQUEST_HELDITEM_BATTLE, 0, sizeof(gBattleMons[gBattlerTarget].item), &gBattleMons[gBattlerTarget].item);
            MarkBattlerForControllerExec(gBattlerTarget);

            *(u8 *)((u8 *)(&gBattleStruct->choicedMove[gBattlerTarget]) + 0) = 0;
            *(u8 *)((u8 *)(&gBattleStruct->choicedMove[gBattlerTarget]) + 1) = 0;

            *(u8 *)((u8 *)(&gBattleStruct->choicedMove[gBattlerAttacker]) + 0) = 0;
            *(u8 *)((u8 *)(&gBattleStruct->choicedMove[gBattlerAttacker]) + 1) = 0;

            gBattlescriptCurrInstr += 5;

            PREPARE_ITEM_BUFFER(gBattleTextBuff1, *newItemAtk)
            PREPARE_ITEM_BUFFER(gBattleTextBuff2, oldItemAtk)

            if (oldItemAtk != ITEM_NONE && *newItemAtk != ITEM_NONE)
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ITEM_SWAP_BOTH;  // attacker's item -> <- target's item
            else if (oldItemAtk == ITEM_NONE && *newItemAtk != ITEM_NONE)
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ITEM_SWAP_TAKEN; // nothing -> <- target's item
            else
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ITEM_SWAP_GIVEN; // attacker's item -> <- nothing
        }
    }
}

// Role Play
static void Cmd_trycopyability(void)
{
    if (gBattleMons[gBattlerTarget].ability != ABILITY_NONE
        && gBattleMons[gBattlerTarget].ability != ABILITY_WONDER_GUARD)
    {
        gBattleMons[gBattlerAttacker].ability = gBattleMons[gBattlerTarget].ability;
        gLastUsedAbility = gBattleMons[gBattlerTarget].ability;
        gBattlescriptCurrInstr += 5;
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_trywish(void)
{
    switch (gBattlescriptCurrInstr[1])
    {
    case 0: // use wish
        if (gWishFutureKnock.wishCounter[gBattlerAttacker] == 0)
        {
            gWishFutureKnock.wishCounter[gBattlerAttacker] = 2;
            gWishFutureKnock.wishMonId[gBattlerAttacker] = gBattlerPartyIndexes[gBattlerAttacker];
            gBattlescriptCurrInstr += 6;
        }
        else
        {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
        }
        break;
    case 1: // heal effect
        PREPARE_MON_NICK_WITH_PREFIX_BUFFER(gBattleTextBuff1, gBattlerTarget, gWishFutureKnock.wishMonId[gBattlerTarget])

        gBattleMoveDamage = gBattleMons[gBattlerTarget].maxHP / 2;
        if (gBattleMoveDamage == 0)
            gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;

        if (gBattleMons[gBattlerTarget].hp == gBattleMons[gBattlerTarget].maxHP)
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
        else
            gBattlescriptCurrInstr += 6;

        break;
    }
}

// Ingrain
static void Cmd_trysetroots(void)
{
    if (gStatuses3[gBattlerAttacker] & STATUS3_ROOTED)
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        gStatuses3[gBattlerAttacker] |= STATUS3_ROOTED;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_doubledamagedealtifdamaged(void)
{
    if ((gProtectStructs[gBattlerAttacker].physicalDmg != 0
         && gProtectStructs[gBattlerAttacker].physicalBattlerId == gBattlerTarget)
        || (gProtectStructs[gBattlerAttacker].specialDmg != 0
            && gProtectStructs[gBattlerAttacker].specialBattlerId == gBattlerTarget))
    {
        gBattleScripting.dmgMultiplier = 2;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_setyawn(void)
{
    if (gStatuses3[gBattlerTarget] & STATUS3_YAWN
        || gBattleMons[gBattlerTarget].status1 & STATUS1_ANY)
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        gStatuses3[gBattlerTarget] |= STATUS3_YAWN_TURN(2);
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_setdamagetohealthdifference(void)
{
    if (gBattleMons[gBattlerTarget].hp <= gBattleMons[gBattlerAttacker].hp)
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        gBattleMoveDamage = gBattleMons[gBattlerTarget].hp - gBattleMons[gBattlerAttacker].hp;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_scaledamagebyhealthratio(void)
{
    if (gDynamicBasePower == 0)
    {
        u8 power = gBattleMoves[gCurrentMove].power;
        gDynamicBasePower = gBattleMons[gBattlerAttacker].hp * power / gBattleMons[gBattlerAttacker].maxHP;
        if (gDynamicBasePower == 0)
            gDynamicBasePower = 1;
    }
    gBattlescriptCurrInstr++;
}

// Skill Swap
static void Cmd_tryswapabilities(void)
{
    if ((gBattleMons[gBattlerAttacker].ability == ABILITY_NONE
        && gBattleMons[gBattlerTarget].ability == ABILITY_NONE)
     || gBattleMons[gBattlerAttacker].ability == ABILITY_WONDER_GUARD
     || gBattleMons[gBattlerTarget].ability == ABILITY_WONDER_GUARD
     || gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
     {
         gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
     }
    else
    {
        u8 abilityAtk = gBattleMons[gBattlerAttacker].ability;
        gBattleMons[gBattlerAttacker].ability = gBattleMons[gBattlerTarget].ability;
        gBattleMons[gBattlerTarget].ability = abilityAtk;

            gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_tryimprison(void)
{
    if ((gStatuses3[gBattlerAttacker] & STATUS3_IMPRISONED_OTHERS))
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        u8 battlerId, sideAttacker;

        sideAttacker = GetBattlerSide(gBattlerAttacker);
        PressurePPLoseOnUsingImprison(gBattlerAttacker);
        for (battlerId = 0; battlerId < gBattlersCount; battlerId++)
        {
            if (sideAttacker != GetBattlerSide(battlerId))
            {
                s32 attackerMoveId;
                for (attackerMoveId = 0; attackerMoveId < MAX_MON_MOVES; attackerMoveId++)
                {
                    s32 i;
                    for (i = 0; i < MAX_MON_MOVES; i++)
                    {
                        if (gBattleMons[gBattlerAttacker].moves[attackerMoveId] == gBattleMons[battlerId].moves[i]
                            && gBattleMons[gBattlerAttacker].moves[attackerMoveId] != MOVE_NONE)
                            break;
                    }
                    if (i != MAX_MON_MOVES)
                        break;
                }
                if (attackerMoveId != MAX_MON_MOVES)
                {
                    gStatuses3[gBattlerAttacker] |= STATUS3_IMPRISONED_OTHERS;
                    gBattlescriptCurrInstr += 5;
                    break;
                }
            }
        }
        if (battlerId == gBattlersCount) // In Generation 3 games, Imprison fails if the user doesn't share any moves with any of the foes
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_trysetgrudge(void)
{
    if (gStatuses3[gBattlerAttacker] & STATUS3_GRUDGE)
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        gStatuses3[gBattlerAttacker] |= STATUS3_GRUDGE;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_weightdamagecalculation(void)
{
    s32 i;
    for (i = 0; sWeightToDamageTable[i] != 0xFFFF; i += 2)
    {
        if (sWeightToDamageTable[i] > GetPokedexHeightWeight(SpeciesToNationalPokedexNum(gBattleMons[gBattlerTarget].species), 1))
            break;
    }

    if (sWeightToDamageTable[i] != 0xFFFF)
        gDynamicBasePower = sWeightToDamageTable[i + 1];
    else
        gDynamicBasePower = 120;

    gBattlescriptCurrInstr++;
}

static void Cmd_assistattackselect(void)
{
    s32 chooseableMovesNo = 0;
    struct Pokemon *party;
    s32 monId, moveId;
    u16 *validMoves = gBattleStruct->assistPossibleMoves;

    if (GET_BATTLER_SIDE(gBattlerAttacker) != B_SIDE_PLAYER)
        party = gEnemyParty;
    else
        party = gPlayerParty;

    for (monId = 0; monId < PARTY_SIZE; monId++)
    {
        if (monId == gBattlerPartyIndexes[gBattlerAttacker])
            continue;
        if (GetMonData(&party[monId], MON_DATA_SPECIES_OR_EGG) == SPECIES_NONE)
            continue;
        if (GetMonData(&party[monId], MON_DATA_SPECIES_OR_EGG) == SPECIES_EGG)
            continue;

        for (moveId = 0; moveId < MAX_MON_MOVES; moveId++)
        {
            s32 i = 0;
            u16 move = GetMonData(&party[monId], MON_DATA_MOVE1 + moveId);

            if (IsInvalidForSleepTalkOrAssist(move))
                continue;

            for (; sMovesForbiddenToCopy[i] != ASSIST_FORBIDDEN_END && move != sMovesForbiddenToCopy[i]; i++);

            if (sMovesForbiddenToCopy[i] != ASSIST_FORBIDDEN_END)
                continue;
            if (move == MOVE_NONE)
                continue;

            validMoves[chooseableMovesNo] = move;
            chooseableMovesNo++;
        }
    }
    if (chooseableMovesNo)
    {
        gHitMarker &= ~HITMARKER_ATTACKSTRING_PRINTED;
        gCalledMove = validMoves[((Random() & 0xFF) * chooseableMovesNo) >> 8];
        gBattlerTarget = GetMoveTarget(gCalledMove, NO_TARGET_OVERRIDE);
        gBattlescriptCurrInstr += 5;
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_trysetmagiccoat(void)
{
    gBattlerTarget = gBattlerAttacker;
    gSpecialStatuses[gBattlerAttacker].ppNotAffectedByPressure = 1;
    if (gCurrentTurnActionNumber == gBattlersCount - 1) // moves last turn
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        gProtectStructs[gBattlerAttacker].bounceMove = TRUE;
        gBattlescriptCurrInstr += 5;
    }
}

// Snatch
static void Cmd_trysetsnatch(void)
{
    gSpecialStatuses[gBattlerAttacker].ppNotAffectedByPressure = 1;
    if (gCurrentTurnActionNumber == gBattlersCount - 1) // moves last turn
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        gProtectStructs[gBattlerAttacker].stealMove = 1;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_trygetintimidatetarget(void)
{
    u8 side;

    gBattleScripting.battler = gBattleStruct->intimidateBattler;
    side = GetBattlerSide(gBattleScripting.battler);

    PREPARE_ABILITY_BUFFER(gBattleTextBuff1, gBattleMons[gBattleScripting.battler].ability)

    for (;gBattlerTarget < gBattlersCount; gBattlerTarget++)
    {
        if (GetBattlerSide(gBattlerTarget) == side)
            continue;
        if (!(gAbsentBattlerFlags & gBitTable[gBattlerTarget]))
            break;
    }

    if (gBattlerTarget >= gBattlersCount)
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    else
        gBattlescriptCurrInstr += 5;
}

static void Cmd_switchoutabilities(void)
{
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    switch (gBattleMons[gActiveBattler].ability)
    {
    case ABILITY_NATURAL_CURE:
        gBattleMons[gActiveBattler].status1 = 0;
        BtlController_EmitSetMonData(BUFFER_A, REQUEST_STATUS_BATTLE,
                                     gBitTable[*(gBattleStruct->battlerPartyIndexes + gActiveBattler)],
                                     sizeof(gBattleMons[gActiveBattler].status1),
                                     &gBattleMons[gActiveBattler].status1);
        MarkBattlerForControllerExec(gActiveBattler);
        break;
    }

    gBattlescriptCurrInstr += 2;
}

static void Cmd_jumpifhasnohp(void)
{
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    if (gBattleMons[gActiveBattler].hp == 0)
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
    else
        gBattlescriptCurrInstr += 6;
}

static void Cmd_getsecretpowereffect(void)
{
    switch (gBattleEnvironment)
    {
    case BATTLE_ENVIRONMENT_GRASS:
        gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_POISON;
        break;
    case BATTLE_ENVIRONMENT_LONG_GRASS:
        gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_SLEEP;
        break;
    case BATTLE_ENVIRONMENT_SAND:
        gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_ACC_MINUS_1;
        break;
    case BATTLE_ENVIRONMENT_UNDERWATER:
        gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_DEF_MINUS_1;
        break;
    case BATTLE_ENVIRONMENT_WATER:
        gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_ATK_MINUS_1;
        break;
    case BATTLE_ENVIRONMENT_POND:
        gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_SPD_MINUS_1;
        break;
    case BATTLE_ENVIRONMENT_MOUNTAIN:
        gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_CONFUSION;
        break;
    case BATTLE_ENVIRONMENT_CAVE:
        gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_FLINCH;
        break;
    default:
        gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_PARALYSIS;
        break;
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_pickup(void)
{
    s32 i;
    u16 species, heldItem;
    u8 ability;

    if (InBattlePike())
    {

    }
    else if (InBattlePyramid())
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG);
            heldItem = GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM);

            if (GetMonData(&gPlayerParty[i], MON_DATA_ABILITY_NUM))
                ability = gSpeciesInfo[species].abilities[1];
            else
                ability = gSpeciesInfo[species].abilities[0];

            if (ability == ABILITY_PICKUP
                && species != SPECIES_NONE
                && species != SPECIES_EGG
                && heldItem == ITEM_NONE
                && (Random() % 10) == 0)
            {
                heldItem = GetBattlePyramidPickupItemId();
                SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &heldItem);
            }
        }
    }
    else
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG);
            heldItem = GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM);

            if (GetMonData(&gPlayerParty[i], MON_DATA_ABILITY_NUM))
                ability = gSpeciesInfo[species].abilities[1];
            else
                ability = gSpeciesInfo[species].abilities[0];

            if (ability == ABILITY_PICKUP
                && species != SPECIES_NONE
                && species != SPECIES_EGG
                && heldItem == ITEM_NONE
                && (Random() % 10) == 0)
            {
                s32 j;
                s32 rand = Random() % 100;
                u8 lvlDivBy10 = (GetMonData(&gPlayerParty[i], MON_DATA_LEVEL) - 1) / 10;
                if (lvlDivBy10 > 9)
                    lvlDivBy10 = 9;

                for (j = 0; j < (int)ARRAY_COUNT(sPickupProbabilities); j++)
                {
                    if (sPickupProbabilities[j] > rand)
                    {
                        SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &sPickupItems[lvlDivBy10 + j]);
                        break;
                    }
                    else if (rand == 99 || rand == 98)
                    {
                        SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &sRarePickupItems[lvlDivBy10 + (99 - rand)]);
                        break;
                    }
                }
            }
        }
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_docastformchangeanimation(void)
{
    gActiveBattler = gBattleScripting.battler;

    if (gBattleMons[gActiveBattler].status2 & STATUS2_SUBSTITUTE)
        *(&gBattleStruct->formToChangeInto) |= CASTFORM_SUBSTITUTE;

    BtlController_EmitBattleAnimation(BUFFER_A, B_ANIM_CASTFORM_CHANGE, gBattleStruct->formToChangeInto);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr++;
}

static void Cmd_trycastformdatachange(void)
{
    u8 form;

    gBattlescriptCurrInstr++;
    form = CastformDataTypeChange(gBattleScripting.battler);
    if (form)
    {
        BattleScriptPushCursorAndCallback(BattleScript_CastformChange);
        *(&gBattleStruct->formToChangeInto) = form - 1;
    }
}

// Water and Mud Sport
static void Cmd_settypebasedhalvers(void)
{
    bool8 worked = FALSE;

    if (gBattleMoves[gCurrentMove].effect == EFFECT_MUD_SPORT)
    {
        if (!(gStatuses3[gBattlerAttacker] & STATUS3_MUDSPORT))
        {
            gStatuses3[gBattlerAttacker] |= STATUS3_MUDSPORT;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_WEAKEN_ELECTRIC;
            worked = TRUE;
        }
    }
    else // Water Sport
    {
        if (!(gStatuses3[gBattlerAttacker] & STATUS3_WATERSPORT))
        {
            gStatuses3[gBattlerAttacker] |= STATUS3_WATERSPORT;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_WEAKEN_FIRE;
            worked = TRUE;
        }
    }

    if (worked)
        gBattlescriptCurrInstr += 5;
    else
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
}

static void Cmd_setweatherballtype(void)
{
    if (WEATHER_HAS_EFFECT)
    {
        if (gBattleWeather & B_WEATHER_ANY)
            gBattleScripting.dmgMultiplier = 2;
        if (gBattleWeather & B_WEATHER_RAIN)
            *(&gBattleStruct->dynamicMoveType) = TYPE_WATER | F_DYNAMIC_TYPE_SET;
        else if (gBattleWeather & B_WEATHER_SANDSTORM)
            *(&gBattleStruct->dynamicMoveType) = TYPE_ROCK | F_DYNAMIC_TYPE_SET;
        else if (gBattleWeather & B_WEATHER_SUN)
            *(&gBattleStruct->dynamicMoveType) = TYPE_FIRE | F_DYNAMIC_TYPE_SET;
        else if (gBattleWeather & B_WEATHER_HAIL)
            *(&gBattleStruct->dynamicMoveType) = TYPE_ICE | F_DYNAMIC_TYPE_SET;
        else
            *(&gBattleStruct->dynamicMoveType) = TYPE_NORMAL | F_DYNAMIC_TYPE_SET;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_tryrecycleitem(void)
{
    u16 *usedHeldItem;

    gActiveBattler = gBattlerAttacker;
    usedHeldItem = &gBattleStruct->usedHeldItems[gActiveBattler];
    if (*usedHeldItem != ITEM_NONE && gBattleMons[gActiveBattler].item == ITEM_NONE)
    {
        gLastUsedItem = *usedHeldItem;
        *usedHeldItem = ITEM_NONE;
        gBattleMons[gActiveBattler].item = gLastUsedItem;

        BtlController_EmitSetMonData(BUFFER_A, REQUEST_HELDITEM_BATTLE, 0, sizeof(gBattleMons[gActiveBattler].item), &gBattleMons[gActiveBattler].item);
        MarkBattlerForControllerExec(gActiveBattler);

        gBattlescriptCurrInstr += 5;
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_settypetoenvironment(void)
{
    if (!IS_BATTLER_OF_TYPE(gBattlerAttacker, sEnvironmentToType[gBattleEnvironment]))
    {
        SET_BATTLER_TYPE(gBattlerAttacker, sEnvironmentToType[gBattleEnvironment]);
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, sEnvironmentToType[gBattleEnvironment]);

        gBattlescriptCurrInstr += 5;
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

// Unused
static void Cmd_pursuitdoubles(void)
{
    gActiveBattler = GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gBattlerAttacker)));

    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
        && !(gAbsentBattlerFlags & gBitTable[gActiveBattler])
        && gChosenActionByBattler[gActiveBattler] == B_ACTION_USE_MOVE
        && gChosenMoveByBattler[gActiveBattler] == MOVE_PURSUIT)
    {
        gActionsByTurnOrder[gActiveBattler] = B_ACTION_TRY_FINISH;
        gCurrentMove = MOVE_PURSUIT;
        gBattlescriptCurrInstr += 5;
        gBattleScripting.animTurn = 1;
        gBattleScripting.pursuitDoublesAttacker = gBattlerAttacker;
        gBattlerAttacker = gActiveBattler;
    }
    else
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_snatchsetbattlers(void)
{
    gEffectBattler = gBattlerAttacker;

    if (gBattlerAttacker == gBattlerTarget)
        gBattlerAttacker = gBattlerTarget = gBattleScripting.battler;
    else
        gBattlerTarget = gBattleScripting.battler;

    gBattleScripting.battler = gEffectBattler;
    gBattlescriptCurrInstr++;
}

// Brick Break
static void Cmd_removelightscreenreflect(void)
{
    u8 opposingSide = BATTLE_OPPOSITE(GetBattlerSide(gBattlerAttacker));

    if (gSideTimers[opposingSide].reflectTimer || gSideTimers[opposingSide].lightscreenTimer)
    {
        gSideStatuses[opposingSide] &= ~SIDE_STATUS_REFLECT;
        gSideStatuses[opposingSide] &= ~SIDE_STATUS_LIGHTSCREEN;
        gSideTimers[opposingSide].reflectTimer = 0;
        gSideTimers[opposingSide].lightscreenTimer = 0;
        gBattleScripting.animTurn = 1;
        gBattleScripting.animTargetsHit = 1;
    }
    else
    {
        gBattleScripting.animTurn = 0;
        gBattleScripting.animTargetsHit = 0;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_handleballthrow(void)
{
    u8 ballMultiplier = 0;

    if (gBattleControllerExecFlags)
        return;

    gActiveBattler = gBattlerAttacker;
    gBattlerTarget = BATTLE_OPPOSITE(gBattlerAttacker);

    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
    {
        BtlController_EmitBallThrowAnim(BUFFER_A, BALL_TRAINER_BLOCK);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr = BattleScript_TrainerBallBlock;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_WALLY_TUTORIAL)
    {
        BtlController_EmitBallThrowAnim(BUFFER_A, BALL_3_SHAKES_SUCCESS);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr = BattleScript_WallyBallThrow;
    }
    else
    {
        u32 odds;
        u8 catchRate;

        if (gLastUsedItem == ITEM_SAFARI_BALL)
            catchRate = gBattleStruct->safariCatchFactor * 1275 / 100;
        else
            catchRate = gSpeciesInfo[gBattleMons[gBattlerTarget].species].catchRate;

        if (gLastUsedItem > ITEM_SAFARI_BALL)
        {
            switch (gLastUsedItem)
            {
            case ITEM_NET_BALL:
                if (IS_BATTLER_OF_TYPE(gBattlerTarget, TYPE_WATER) || IS_BATTLER_OF_TYPE(gBattlerTarget, TYPE_BUG))
                    ballMultiplier = 30;
                else
                    ballMultiplier = 10;
                break;
            case ITEM_DIVE_BALL:
                if (GetCurrentMapType() == MAP_TYPE_UNDERWATER)
                    ballMultiplier = 35;
                else
                    ballMultiplier = 10;
                break;
            case ITEM_NEST_BALL:
                if (gBattleMons[gBattlerTarget].level < 40)
                {
                    ballMultiplier = 40 - gBattleMons[gBattlerTarget].level;
                    if (ballMultiplier <= 9)
                        ballMultiplier = 10;
                }
                else
                {
                    ballMultiplier = 10;
                }
                break;
            case ITEM_REPEAT_BALL:
                if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(gBattleMons[gBattlerTarget].species), FLAG_GET_CAUGHT))
                    ballMultiplier = 30;
                else
                    ballMultiplier = 10;
                break;
            case ITEM_TIMER_BALL:
                ballMultiplier = gBattleResults.battleTurnCounter + 10;
                if (ballMultiplier > 40)
                    ballMultiplier = 40;
                break;
            case ITEM_LUXURY_BALL:
            case ITEM_PREMIER_BALL:
                ballMultiplier = 10;
                break;
            }
        }
        else
        {
            ballMultiplier = sBallCatchBonuses[gLastUsedItem - ITEM_ULTRA_BALL];
        }

        odds = (catchRate * ballMultiplier / 10)
            * (gBattleMons[gBattlerTarget].maxHP * 3 - gBattleMons[gBattlerTarget].hp * 2)
            / (3 * gBattleMons[gBattlerTarget].maxHP);

        if (gBattleMons[gBattlerTarget].status1 & (STATUS1_SLEEP | STATUS1_FREEZE))
            odds *= 2;
        if (gBattleMons[gBattlerTarget].status1 & (STATUS1_POISON | STATUS1_BURN | STATUS1_PARALYSIS | STATUS1_TOXIC_POISON))
            odds = (odds * 15) / 10;

        if (gLastUsedItem != ITEM_SAFARI_BALL)
        {
            if (gLastUsedItem == ITEM_MASTER_BALL)
            {
                gBattleResults.usedMasterBall = TRUE;
            }
            else
            {
                if (gBattleResults.catchAttempts[gLastUsedItem - ITEM_ULTRA_BALL] < 255)
                    gBattleResults.catchAttempts[gLastUsedItem - ITEM_ULTRA_BALL]++;
            }
        }

        if (odds > 254) // mon caught
        {
            BtlController_EmitBallThrowAnim(BUFFER_A, BALL_3_SHAKES_SUCCESS);
            MarkBattlerForControllerExec(gActiveBattler);
            gBattlescriptCurrInstr = BattleScript_SuccessBallThrow;
            SetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_POKEBALL, &gLastUsedItem);

            if (CalculatePlayerPartyCount() == PARTY_SIZE)
                gBattleCommunication[MULTISTRING_CHOOSER] = 0;
            else
                gBattleCommunication[MULTISTRING_CHOOSER] = 1;
        }
        else // mon may be caught, calculate shakes
        {
            u8 shakes;

            odds = Sqrt(Sqrt(16711680 / odds));
            odds = 1048560 / odds;

            for (shakes = 0; shakes < BALL_3_SHAKES_SUCCESS && Random() < odds; shakes++);

            if (gLastUsedItem == ITEM_MASTER_BALL)
                shakes = BALL_3_SHAKES_SUCCESS; // why calculate the shakes before that check?

            BtlController_EmitBallThrowAnim(BUFFER_A, shakes);
            MarkBattlerForControllerExec(gActiveBattler);

            if (shakes == BALL_3_SHAKES_SUCCESS) // mon caught, copy of the code above
            {
                gBattlescriptCurrInstr = BattleScript_SuccessBallThrow;
                SetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_POKEBALL, &gLastUsedItem);

                if (CalculatePlayerPartyCount() == PARTY_SIZE)
                    gBattleCommunication[MULTISTRING_CHOOSER] = 0;
                else
                    gBattleCommunication[MULTISTRING_CHOOSER] = 1;
            }
            else // not caught
            {
                gBattleCommunication[MULTISTRING_CHOOSER] = shakes;
                gBattlescriptCurrInstr = BattleScript_ShakeBallThrow;
            }
        }
    }
}

static void Cmd_givecaughtmon(void)
{
    if (GiveMonToPlayer(&gEnemyParty[gBattlerPartyIndexes[BATTLE_OPPOSITE(gBattlerAttacker)]]) != MON_GIVEN_TO_PARTY)
    {
        if (!ShouldShowBoxWasFullMessage())
        {
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SENT_SOMEONES_PC;
            StringCopy(gStringVar1, GetBoxNamePtr(VarGet(VAR_PC_BOX_TO_SEND_MON)));
            GetMonData(&gEnemyParty[gBattlerPartyIndexes[BATTLE_OPPOSITE(gBattlerAttacker)]], MON_DATA_NICKNAME, gStringVar2);
        }
        else
        {
            StringCopy(gStringVar1, GetBoxNamePtr(VarGet(VAR_PC_BOX_TO_SEND_MON))); // box the mon was sent to
            GetMonData(&gEnemyParty[gBattlerPartyIndexes[BATTLE_OPPOSITE(gBattlerAttacker)]], MON_DATA_NICKNAME, gStringVar2);
            StringCopy(gStringVar3, GetBoxNamePtr(GetPCBoxToSendMon())); //box the mon was going to be sent to
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SOMEONES_BOX_FULL;
        }

        // Change to B_MSG_SENT_LANETTES_PC or B_MSG_LANETTES_BOX_FULL
        if (FlagGet(FLAG_SYS_PC_LANETTE))
            gBattleCommunication[MULTISTRING_CHOOSER]++;
    }

    gBattleResults.caughtMonSpecies = GetMonData(&gEnemyParty[gBattlerPartyIndexes[BATTLE_OPPOSITE(gBattlerAttacker)]], MON_DATA_SPECIES, NULL);
    GetMonData(&gEnemyParty[gBattlerPartyIndexes[BATTLE_OPPOSITE(gBattlerAttacker)]], MON_DATA_NICKNAME, gBattleResults.caughtMonNick);
    gBattleResults.caughtMonBall = GetMonData(&gEnemyParty[gBattlerPartyIndexes[BATTLE_OPPOSITE(gBattlerAttacker)]], MON_DATA_POKEBALL, NULL);

    gBattlescriptCurrInstr++;
}

static void Cmd_trysetcaughtmondexflags(void)
{
    u16 species = GetMonData(&gEnemyParty[0], MON_DATA_SPECIES, NULL);
    u32 personality = GetMonData(&gEnemyParty[0], MON_DATA_PERSONALITY, NULL);

    if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
    else
    {
        HandleSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_SET_CAUGHT, personality);
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_displaydexinfo(void)
{
    u16 species = GetMonData(&gEnemyParty[0], MON_DATA_SPECIES, NULL);

    switch (gBattleCommunication[0])
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gBattleCommunication[0]++;
        break;
    case 1:
        if (!gPaletteFade.active)
        {
            FreeAllWindowBuffers();
            gBattleCommunication[TASK_ID] = DisplayCaughtMonDexPage(SpeciesToNationalPokedexNum(species),
                                                                        gBattleMons[gBattlerTarget].otId,
                                                                        gBattleMons[gBattlerTarget].personality);
            gBattleCommunication[0]++;
        }
        break;
    case 2:
        if (!gPaletteFade.active
            && gMain.callback2 == BattleMainCB2
            && !gTasks[gBattleCommunication[TASK_ID]].isActive)
        {
            SetVBlankCallback(VBlankCB_Battle);
            gBattleCommunication[0]++;
        }
        break;
    case 3:
        InitBattleBgsVideo();
        LoadBattleTextboxAndBackground();
        gBattle_BG3_X = 256;
        gBattleCommunication[0]++;
        break;
    case 4:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            BeginNormalPaletteFade(PALETTES_BG, 0, 16, 0, RGB_BLACK);
            ShowBg(0);
            ShowBg(3);
            gBattleCommunication[0]++;
        }
        break;
    case 5:
        if (!gPaletteFade.active)
            gBattlescriptCurrInstr++;
        break;
    }
}

void HandleBattleWindow(u8 xStart, u8 yStart, u8 xEnd, u8 yEnd, u8 flags)
{
    s32 destY, destX;
    u16 var = 0;

    for (destY = yStart; destY <= yEnd; destY++)
    {
        for (destX = xStart; destX <= xEnd; destX++)
        {
            if (destY == yStart)
            {
                if (destX == xStart)
                    var = 0x1022;
                else if (destX == xEnd)
                    var = 0x1024;
                else
                    var = 0x1023;
            }
            else if (destY == yEnd)
            {
                if (destX == xStart)
                    var = 0x1028;
                else if (destX == xEnd)
                    var = 0x102A;
                else
                    var = 0x1029;
            }
            else
            {
                if (destX == xStart)
                    var = 0x1025;
                else if (destX == xEnd)
                    var = 0x1027;
                else
                    var = 0x1026;
            }

            if (flags & WINDOW_CLEAR)
                var = 0;

            if (flags & WINDOW_BG1)
                CopyToBgTilemapBufferRect_ChangePalette(1, &var, destX, destY, 1, 1, 0x11);
            else
                CopyToBgTilemapBufferRect_ChangePalette(0, &var, destX, destY, 1, 1, 0x11);
        }
    }
}

void BattleCreateYesNoCursorAt(u8 cursorPosition)
{
    u16 src[2];
    src[0] = 1;
    src[1] = 2;

    CopyToBgTilemapBufferRect_ChangePalette(0, src, 0x19, 9 + (2 * cursorPosition), 1, 2, 0x11);
    CopyBgTilemapBufferToVram(0);
}

void BattleDestroyYesNoCursorAt(u8 cursorPosition)
{
    u16 src[2];
    src[0] = 0x1016;
    src[1] = 0x1016;

    CopyToBgTilemapBufferRect_ChangePalette(0, src, 0x19, 9 + (2 * cursorPosition), 1, 2, 0x11);
    CopyBgTilemapBufferToVram(0);
}

static void Cmd_trygivecaughtmonnick(void)
{
    switch (gBattleCommunication[MULTIUSE_STATE])
    {
    case 0:
        HandleBattleWindow(YESNOBOX_X_Y, 0);
        BattlePutTextOnWindow(gText_BattleYesNoChoice, B_WIN_YESNO);
        gBattleCommunication[MULTIUSE_STATE]++;
        gBattleCommunication[CURSOR_POSITION] = 0;
        BattleCreateYesNoCursorAt(0);
        break;
    case 1:
        if (JOY_NEW(DPAD_UP) && gBattleCommunication[CURSOR_POSITION] != 0)
        {
            PlaySE(SE_SELECT);
            BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
            gBattleCommunication[CURSOR_POSITION] = 0;
            BattleCreateYesNoCursorAt(0);
        }
        if (JOY_NEW(DPAD_DOWN) && gBattleCommunication[CURSOR_POSITION] == 0)
        {
            PlaySE(SE_SELECT);
            BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
            gBattleCommunication[CURSOR_POSITION] = 1;
            BattleCreateYesNoCursorAt(1);
        }
        if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            if (gBattleCommunication[CURSOR_POSITION] == 0)
            {
                gBattleCommunication[MULTIUSE_STATE]++;
                BeginFastPaletteFade(3);
            }
            else
            {
                gBattleCommunication[MULTIUSE_STATE] = 4;
            }
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            gBattleCommunication[MULTIUSE_STATE] = 4;
        }
        break;
    case 2:
        if (!gPaletteFade.active)
        {
            GetMonData(&gEnemyParty[gBattlerPartyIndexes[BATTLE_OPPOSITE(gBattlerAttacker)]], MON_DATA_NICKNAME, gBattleStruct->caughtMonNick);
            FreeAllWindowBuffers();

            DoNamingScreen(NAMING_SCREEN_CAUGHT_MON, gBattleStruct->caughtMonNick,
                           GetMonData(&gEnemyParty[gBattlerPartyIndexes[BATTLE_OPPOSITE(gBattlerAttacker)]], MON_DATA_SPECIES),
                           GetMonGender(&gEnemyParty[gBattlerPartyIndexes[BATTLE_OPPOSITE(gBattlerAttacker)]]),
                           GetMonData(&gEnemyParty[gBattlerPartyIndexes[BATTLE_OPPOSITE(gBattlerAttacker)]], MON_DATA_PERSONALITY, NULL),
                           BattleMainCB2);

            gBattleCommunication[MULTIUSE_STATE]++;
        }
        break;
    case 3:
        if (gMain.callback2 == BattleMainCB2 && !gPaletteFade.active)
        {
            SetMonData(&gEnemyParty[gBattlerPartyIndexes[BATTLE_OPPOSITE(gBattlerAttacker)]], MON_DATA_NICKNAME, gBattleStruct->caughtMonNick);
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        }
        break;
    case 4:
        if (CalculatePlayerPartyCount() == PARTY_SIZE)
            gBattlescriptCurrInstr += 5;
        else
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        break;
    }
}

static void Cmd_subattackerhpbydmg(void)
{
    gBattleMons[gBattlerAttacker].hp -= gBattleMoveDamage;
    gBattlescriptCurrInstr++;
}

static void Cmd_removeattackerstatus1(void)
{
    gBattleMons[gBattlerAttacker].status1 = 0;
    gBattlescriptCurrInstr++;
}

static void Cmd_finishaction(void)
{
    gCurrentActionFuncId = B_ACTION_FINISHED;
}

static void Cmd_finishturn(void)
{
    gCurrentActionFuncId = B_ACTION_FINISHED;
    gCurrentTurnActionNumber = gBattlersCount;
}

static void Cmd_trainerslideout(void)
{
    gActiveBattler = GetBattlerAtPosition(gBattlescriptCurrInstr[1]);
    BtlController_EmitTrainerSlideBack(BUFFER_A);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}
