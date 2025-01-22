#include "global.h"

const u32 gBattleTextboxTiles[] = INCBIN_U32("graphics/battle_interface/textbox.4bpp.lz");
const u32 gBattleTextboxPalette[] = INCBIN_U32("graphics/battle_interface/textbox.gbapal.lz");
const u32 gBattleTextboxTilemap[] = INCBIN_U32("graphics/battle_interface/textbox_map.bin.lz");

const u32 gMonStillFrontPic_CircledQuestionMark[] = INCBIN_U32("graphics/pokemon/question_mark/circled/front.4bpp.lz");
const u32 gMonBackPic_CircledQuestionMark[] = INCBIN_U32("graphics/pokemon/question_mark/circled/back.4bpp.lz");
const u32 gMonPalette_CircledQuestionMark[] = INCBIN_U32("graphics/pokemon/question_mark/circled/normal.gbapal.lz");
const u32 gMonShinyPalette_CircledQuestionMark[] = INCBIN_U32("graphics/pokemon/question_mark/circled/shiny.gbapal.lz");

const u32 gUnusedGfx_OldCharmap[] = INCBIN_U32("graphics/unused/old_charmap.4bpp.lz"); // japanese table and bunch of stuff
const u32 gUnusedTimemap_OldCharmap[] = INCBIN_U32("graphics/unused/old_charmap.bin.lz");
const u32 gUnusedPal_OldCharmap[] = INCBIN_U32("graphics/unused/old_charmap.gbapal.lz");

const u32 gSmokescreenImpactTiles[] = INCBIN_U32("graphics/battle_anims/sprites/smokescreen_impact.4bpp.lz");
const u32 gSmokescreenImpactPalette[] = INCBIN_U32("graphics/battle_anims/sprites/smokescreen_impact.gbapal.lz");

#include "data/graphics/pokeballs.h"

const u32 gBlankGfxCompressed[] = INCBIN_U32("graphics/interface/blank.4bpp.lz");

// Battle anims
const u32 gBattleAnimSpriteGfx_Bubble[] = INCBIN_U32("graphics/battle_anims/sprites/bubble.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Bone[] = INCBIN_U32("graphics/battle_anims/sprites/bone.4bpp.lz");
const u32 gBattleAnimSpriteGfx_AirWave[] = INCBIN_U32("graphics/battle_anims/sprites/air_wave.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Orb[] = INCBIN_U32("graphics/battle_anims/sprites/orb.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Sword[] = INCBIN_U32("graphics/battle_anims/sprites/sword.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Seed[] = INCBIN_U32("graphics/battle_anims/sprites/seed.4bpp.lz");

const u32 gBattleAnimSpritePal_RainDrops[] = INCBIN_U32("graphics/battle_anims/sprites/rain_drops.gbapal.lz");
const u32 gBattleAnimSpritePal_Bone[] = INCBIN_U32("graphics/battle_anims/sprites/bone.gbapal.lz");
const u32 gBattleAnimSpritePal_AirWave[] = INCBIN_U32("graphics/battle_anims/sprites/air_wave.gbapal.lz");
const u32 gBattleAnimSpritePal_Orb[] = INCBIN_U32("graphics/battle_anims/sprites/orb.gbapal.lz");
const u32 gBattleAnimSpritePal_Sword[] = INCBIN_U32("graphics/battle_anims/sprites/sword.gbapal.lz");
const u32 gBattleAnimSpritePal_Seed[] = INCBIN_U32("graphics/battle_anims/sprites/seed.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Needle[] = INCBIN_U32("graphics/battle_anims/sprites/needle.4bpp.lz");
const u32 gBattleAnimSpritePal_Needle[] = INCBIN_U32("graphics/battle_anims/sprites/needle.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Explosion6[] = INCBIN_U32("graphics/battle_anims/sprites/explosion_6.4bpp.lz");
const u32 gBattleAnimSpritePal_Explosion6[] = INCBIN_U32("graphics/battle_anims/sprites/explosion_6.gbapal.lz");

const u32 gBattleAnimSpriteGfx_PinkOrb[] = INCBIN_U32("graphics/battle_anims/sprites/pink_orb.4bpp.lz");
const u32 gBattleAnimSpritePal_PinkOrb[] = INCBIN_U32("graphics/battle_anims/sprites/pink_orb.gbapal.lz");

const u32 gBattleAnimSpritePal_IceCube[] = INCBIN_U32("graphics/battle_anims/sprites/ice_cube.gbapal.lz");
const u32 gBattleAnimSpriteGfx_IceCube[] = INCBIN_U32("graphics/battle_anims/sprites/ice_cube.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Gust[] = INCBIN_U32("graphics/battle_anims/sprites/gust.4bpp.lz");
const u32 gBattleAnimSpritePal_Gust[] = INCBIN_U32("graphics/battle_anims/sprites/gust.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Spark2[] = INCBIN_U32("graphics/battle_anims/sprites/spark_2.4bpp.lz");
const u32 gBattleAnimSpritePal_Spark2[] = INCBIN_U32("graphics/battle_anims/sprites/spark_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Orange[] = INCBIN_U32("graphics/battle_anims/sprites/orange.4bpp.lz");
const u32 gBattleAnimSpritePal_Orange[] = INCBIN_U32("graphics/battle_anims/sprites/orange.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Spikes[] = INCBIN_U32("graphics/battle_anims/sprites/spikes.4bpp.lz");
const u32 gBattleAnimSpritePal_Spikes[] = INCBIN_U32("graphics/battle_anims/sprites/spikes.gbapal.lz");

const u32 gBattleAnimSpriteGfx_YellowBall[] = INCBIN_U32("graphics/battle_anims/sprites/yellow_ball.4bpp.lz");
const u32 gBattleAnimSpritePal_YellowBall[] = INCBIN_U32("graphics/battle_anims/sprites/yellow_ball.gbapal.lz");

const u32 gBattleAnimSpriteGfx_TiedBag[] = INCBIN_U32("graphics/battle_anims/sprites/tied_bag.4bpp.lz");
const u32 gBattleAnimSpritePal_TiedBag[] = INCBIN_U32("graphics/battle_anims/sprites/tied_bag.gbapal.lz");

const u32 gBattleAnimSpriteGfx_BlackSmoke[] = INCBIN_U32("graphics/battle_anims/sprites/black_smoke.4bpp.lz");
const u32 gBattleAnimSpritePal_BlackSmoke[] = INCBIN_U32("graphics/battle_anims/sprites/black_smoke.gbapal.lz");

const u32 gBattleAnimSpriteGfx_BlackBall[] = INCBIN_U32("graphics/battle_anims/sprites/black_ball.4bpp.lz");
const u32 gBattleAnimSpritePal_BlackBall[] = INCBIN_U32("graphics/battle_anims/sprites/black_ball.gbapal.lz");

const u32 gBattleAnimSpritePal_Glass[] = INCBIN_U32("graphics/battle_anims/sprites/glass.gbapal.lz");
const u32 gBattleAnimSpriteGfx_Glass[] = INCBIN_U32("graphics/battle_anims/sprites/glass.4bpp.lz");

const u32 gBattleAnimSpriteGfx_HornHit[] = INCBIN_U32("graphics/battle_anims/sprites/horn_hit.4bpp.lz");
const u32 gBattleAnimSpritePal_HornHit[] = INCBIN_U32("graphics/battle_anims/sprites/horn_hit.gbapal.lz");

const u32 gBattleAnimSpritePal_BlueShards[] = INCBIN_U32("graphics/battle_anims/sprites/blue_shards.gbapal.lz");
const u32 gBattleAnimSpriteGfx_BlueShards[] = INCBIN_U32("graphics/battle_anims/sprites/blue_shards.4bpp.lz");

const u32 gBattleAnimUnusedPal_MusicNotes[] = INCBIN_U32("graphics/battle_anims/unused/music_notes.gbapal.lz");
const u32 gBattleAnimUnusedGfx_MusicNotes[] = INCBIN_U32("graphics/battle_anims/unused/music_notes.4bpp.lz");

const u32 gBattleAnimSpritePal_Hit[] = INCBIN_U32("graphics/battle_anims/sprites/hit.gbapal.lz");
const u32 gBattleAnimSpriteGfx_Hit[] = INCBIN_U32("graphics/battle_anims/sprites/hit.4bpp.lz");

const u32 gBattleAnimSpritePal_Hit2[] = INCBIN_U32("graphics/battle_anims/sprites/hit_2.gbapal.lz");

const u32 gBattleAnimSpritePal_WavingHand[] = INCBIN_U32("graphics/battle_anims/sprites/waving_hand.gbapal.lz");
const u32 gBattleAnimSpriteGfx_WavingHand[] = INCBIN_U32("graphics/battle_anims/sprites/waving_hand.4bpp.lz");

const u32 gBattleAnimSpriteGfx_ClosingEye[] = INCBIN_U32("graphics/battle_anims/sprites/closing_eye.4bpp.lz");
const u32 gBattleAnimSpritePal_ClosingEye[] = INCBIN_U32("graphics/battle_anims/sprites/closing_eye.gbapal.lz");

const u32 gBattleAnimSpriteGfx_BlueStar[] = INCBIN_U32("graphics/battle_anims/sprites/blue_star.4bpp.lz");
const u32 gBattleAnimSpritePal_BlueStar[] = INCBIN_U32("graphics/battle_anims/sprites/blue_star.gbapal.lz");

const u32 gBattleAnimSpritePal_BubbleBurst[] = INCBIN_U32("graphics/battle_anims/sprites/bubble_burst.gbapal.lz");
const u32 gBattleAnimSpriteGfx_BubbleBurst[] = INCBIN_U32("graphics/battle_anims/sprites/bubble_burst.4bpp.lz");

const u32 gBattleAnimSpriteGfx_HitDuplicate[] = INCBIN_U32("graphics/battle_anims/sprites/hit_duplicate.4bpp.lz");
const u32 gBattleAnimSpritePal_HitDuplicate[] = INCBIN_U32("graphics/battle_anims/sprites/hit_duplicate.gbapal.lz");

const u32 gBattleAnimSpritePal_Leer[] = INCBIN_U32("graphics/battle_anims/sprites/leer.gbapal.lz");
const u32 gBattleAnimSpriteGfx_Leer[] = INCBIN_U32("graphics/battle_anims/sprites/leer.4bpp.lz");

const u32 gBattleAnimSpritePal_BlueBurst[] = INCBIN_U32("graphics/battle_anims/sprites/blue_burst.gbapal.lz");
const u32 gBattleAnimSpriteGfx_BlueBurst[] = INCBIN_U32("graphics/battle_anims/sprites/blue_burst.4bpp.lz");

const u32 gBattleAnimSpriteGfx_SmallEmber[] = INCBIN_U32("graphics/battle_anims/sprites/small_ember.4bpp.lz");
const u32 gBattleAnimSpritePal_SmallEmber[] = INCBIN_U32("graphics/battle_anims/sprites/small_ember.gbapal.lz");

const u32 gBattleAnimSpriteGfx_GraySmoke[] = INCBIN_U32("graphics/battle_anims/sprites/gray_smoke.4bpp.lz");
const u32 gBattleAnimSpritePal_GraySmoke[] = INCBIN_U32("graphics/battle_anims/sprites/gray_smoke.gbapal.lz");

const u32 gBattleAnimSpritePal_Fire[] = INCBIN_U32("graphics/battle_anims/sprites/fire.gbapal.lz");
const u32 gBattleAnimSpriteGfx_Fire[] = INCBIN_U32("graphics/battle_anims/sprites/fire.4bpp.lz");

const u32 gBattleAnimSpriteGfx_SpinningFire[] = INCBIN_U32("graphics/battle_anims/sprites/spinning_fire.4bpp.lz");
const u32 gBattleAnimSpriteGfx_FirePlume[] = INCBIN_U32("graphics/battle_anims/sprites/fire_plume.4bpp.lz");

const u32 gBattleAnimSpritePal_Lightning2[] = INCBIN_U32("graphics/battle_anims/sprites/lightning_2.gbapal.lz");
const u32 gBattleAnimSpriteGfx_Lightning2[] = INCBIN_U32("graphics/battle_anims/sprites/lightning_2.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Lightning[] = INCBIN_U32("graphics/battle_anims/sprites/lightning.4bpp.lz");

const u32 gBattleAnimSpriteGfx_SpinningBall[] = INCBIN_U32("graphics/battle_anims/unused/spinning_ball.4bpp.lz");
const u32 gBattleAnimSpritePal_SpinningBall[] = INCBIN_U32("graphics/battle_anims/unused/spinning_ball.gbapal.lz");
const u32 gBattleAnimSpritePal_SpinningBall2[] = INCBIN_U32("graphics/battle_anims/unused/spinning_ball_2.gbapal.lz");

// old battle interface data, unused

const u32 gOldBattleInterfaceGfx[] = INCBIN_U32("graphics/unused/obi1.4bpp.lz");
const u32 gOldBattleInterfacePal_1_2_3[] = INCBIN_U32("graphics/unused/obi_palpak1.gbapal.lz"); // palettes 1-3
const u32 gOldBattleInterfacePal4[] = INCBIN_U32("graphics/unused/old_pal4.gbapal.lz"); // 4 is by itself
const u32 gOldBattleInterfacePal_5_6_7[] = INCBIN_U32("graphics/unused/obi_palpak3.gbapal.lz"); // palettes 5-7
const u32 gOldBattleInterfaceGfx2[] = INCBIN_U32("graphics/unused/obi2.4bpp.lz");
const u32 gOldBattleInterfaceTilemap[] = INCBIN_U32("graphics/unused/old_battle_interface_tilemap.bin.lz");

const u32 gBattleAnimSpritePal_ClawSlash2[] = INCBIN_U32("graphics/battle_anims/sprites/claw_slash_2.gbapal.lz");
const u32 gBattleAnimSpritePal_ClawSlash[] = INCBIN_U32("graphics/battle_anims/sprites/claw_slash.gbapal.lz");

const u32 gBattleAnimSpriteGfx_ClawSlash2[] = INCBIN_U32("graphics/battle_anims/sprites/claw_slash_2.4bpp.lz");
const u32 gBattleAnimSpriteGfx_ClawSlash[] = INCBIN_U32("graphics/battle_anims/sprites/claw_slash.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Scratch3[] = INCBIN_U32("graphics/battle_anims/sprites/scratch_3.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Scratch2[] = INCBIN_U32("graphics/battle_anims/sprites/scratch_2.4bpp.lz");

const u32 gUnusedHpBar_Gfx[] = INCBIN_U32("graphics/battle_interface/hpbar_unused.4bpp.lz");

const u32 gBattleAnimSpriteGfx_BubbleBurst2[] = INCBIN_U32("graphics/battle_anims/sprites/bubble_burst_2.4bpp.lz");

const u32 gBattleAnimSpritePal_BubbleBurst2[] = INCBIN_U32("graphics/battle_anims/sprites/bubble_burst_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_IceChunk[] = INCBIN_U32("graphics/battle_anims/sprites/ice_chunk.4bpp.lz");
const u32 gBattleAnimSpritePal_IceChunk[] = INCBIN_U32("graphics/battle_anims/sprites/ice_chunk.gbapal.lz");

const u32 gBattleAnimSpritePal_Glass2[] = INCBIN_U32("graphics/battle_anims/sprites/glass_2.gbapal.lz");
const u32 gBattleAnimSpriteGfx_Glass2[] = INCBIN_U32("graphics/battle_anims/sprites/glass_2.4bpp.lz");

const u32 gBattleAnimSpritePal_PinkHeart2[] = INCBIN_U32("graphics/battle_anims/sprites/pink_heart_2.gbapal.lz");
const u32 gBattleAnimSpriteGfx_PinkHeart2[] = INCBIN_U32("graphics/battle_anims/sprites/pink_heart_2.4bpp.lz");

const u32 gBattleInterfaceGfx_UnusedWindow1[] = INCBIN_U32("graphics/battle_interface/unused_window.4bpp.lz");
const u32 gBattleInterfacePal_UnusedWindow1[] = INCBIN_U32("graphics/battle_interface/unused_window.gbapal.lz");

const u32 gBattleInterfaceGfx_BattleBar[] = INCBIN_U32("graphics/battle_interface/battle_bar.4bpp.lz");

const u32 gBattleAnimSpriteGfx_SapDrip[] = INCBIN_U32("graphics/battle_anims/sprites/sap_drip.4bpp.lz");
const u32 gBattleAnimSpritePal_SapDrip[] = INCBIN_U32("graphics/battle_anims/sprites/sap_drip.gbapal.lz");

const u32 gBattleAnimSpritePal_SapDrip2[] = INCBIN_U32("graphics/battle_anims/sprites/sap_drip_2.gbapal.lz");

const u32 gBattleInterfaceGfx_UnusedWindow2[] = INCBIN_U32("graphics/battle_interface/unused_window2.4bpp.lz");
const u32 gBattleInterfaceGfx_UnusedWindow2Bar[] = INCBIN_U32("graphics/battle_interface/unused_window2bar.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Sparkle1[] = INCBIN_U32("graphics/battle_anims/sprites/sparkle_1.4bpp.lz");
const u32 gBattleAnimSpritePal_Sparkle1[] = INCBIN_U32("graphics/battle_anims/sprites/sparkle_1.gbapal.lz");

const u32 gBattleAnimSpritePal_Sparkle2[] = INCBIN_U32("graphics/battle_anims/sprites/sparkle_2.gbapal.lz");
const u32 gBattleAnimSpritePal_HumanoidFoot[] = INCBIN_U32("graphics/battle_anims/sprites/humanoid_foot.gbapal.lz");

const u32 gBattleAnimSpriteGfx_HumanoidFoot[] = INCBIN_U32("graphics/battle_anims/sprites/humanoid_foot.4bpp.lz");
const u32 gBattleAnimSpriteGfx_MonsterFoot[] = INCBIN_U32("graphics/battle_anims/sprites/monster_foot.4bpp.lz");
const u32 gBattleAnimSpriteGfx_HumanoidHand[] = INCBIN_U32("graphics/battle_anims/sprites/humanoid_hand.4bpp.lz");

const u32 gBattleAnimSpriteGfx_LineSketch[] = INCBIN_U32("graphics/battle_anims/unused/line_sketch.4bpp.lz");
const u32 gBattleAnimSpritePal_LineSketch[] = INCBIN_U32("graphics/battle_anims/unused/line_sketch.gbapal.lz");

const u32 gBattleAnimSpriteGfx_YellowUnk[] = INCBIN_U32("graphics/battle_anims/sprites/yellow_unk.4bpp.lz");
const u32 gBattleAnimSpritePal_YellowUnk[] = INCBIN_U32("graphics/battle_anims/sprites/yellow_unk.gbapal.lz");

const u32 gBattleAnimSpriteGfx_SlamHit[] = INCBIN_U32("graphics/battle_anims/sprites/slam_hit.4bpp.lz");
const u32 gBattleAnimSpritePal_SlamHit[] = INCBIN_U32("graphics/battle_anims/sprites/slam_hit.gbapal.lz");

const u32 gBattleAnimSpriteGfx_RedFist[] = INCBIN_U32("graphics/battle_anims/sprites/red_fist.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Ring[] = INCBIN_U32("graphics/battle_anims/sprites/ring.4bpp.lz");
const u32 gBattleAnimSpritePal_Ring[] = INCBIN_U32("graphics/battle_anims/sprites/ring.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Rocks[] = INCBIN_U32("graphics/battle_anims/sprites/rocks.4bpp.lz");
const u32 gBattleAnimSpritePal_Rocks[] = INCBIN_U32("graphics/battle_anims/sprites/rocks.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Z[] = INCBIN_U32("graphics/battle_anims/sprites/z.4bpp.lz");
const u32 gBattleAnimSpritePal_Z[] = INCBIN_U32("graphics/battle_anims/sprites/z.gbapal.lz");

const u32 gBattleAnimSpriteGfx_YellowUnk2[] = INCBIN_U32("graphics/battle_anims/sprites/yellow_unk_2.4bpp.lz");
const u32 gBattleAnimSpritePal_YellowUnk2[] = INCBIN_U32("graphics/battle_anims/sprites/yellow_unk_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_AirSlash[] = INCBIN_U32("graphics/battle_anims/sprites/air_slash.4bpp.lz");
const u32 gBattleAnimSpritePal_AirSlash[] = INCBIN_U32("graphics/battle_anims/sprites/air_slash.gbapal.lz");

const u32 gBattleAnimSpriteGfx_SpinningGreenOrbs[] = INCBIN_U32("graphics/battle_anims/sprites/spinning_green_orbs.4bpp.lz");
const u32 gBattleAnimSpritePal_SpinningGreenOrbs[] = INCBIN_U32("graphics/battle_anims/sprites/spinning_green_orbs.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Leaf[] = INCBIN_U32("graphics/battle_anims/sprites/leaf.4bpp.lz");
const u32 gBattleAnimSpritePal_Leaf[] = INCBIN_U32("graphics/battle_anims/sprites/leaf.gbapal.lz");

const u32 gBattleAnimSpriteGfx_MetronomeSmallHand[] = INCBIN_U32("graphics/battle_anims/unused/metronome_hand_small.4bpp.lz"); // unused, was for metronome at one point

const u32 gBattleAnimSpritePal_Clapping[] = INCBIN_U32("graphics/battle_anims/sprites/clapping.gbapal.lz");

const u32 gBattleAnimSpriteGfx_PoisonPowder[] = INCBIN_U32("graphics/battle_anims/sprites/poison_powder.4bpp.lz");
const u32 gBattleAnimSpritePal_PoisonPowder[] = INCBIN_U32("graphics/battle_anims/sprites/poison_powder.gbapal.lz");

const u32 gBattleAnimSpriteGfx_BrownTriangle[] = INCBIN_U32("graphics/battle_anims/sprites/brown_triangle.4bpp.lz");
const u32 gBattleAnimSpritePal_BrownTriangle[] = INCBIN_U32("graphics/battle_anims/sprites/brown_triangle.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Sparkle3[] = INCBIN_U32("graphics/battle_anims/sprites/sparkle_3.4bpp.lz");
const u32 gBattleAnimSpritePal_Sparkle3[] = INCBIN_U32("graphics/battle_anims/sprites/sparkle_3.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Sparkle4[] = INCBIN_U32("graphics/battle_anims/sprites/sparkle_4.4bpp.lz");

const u32 gBattleAnimSpriteGfx_MusicNotes[] = INCBIN_U32("graphics/battle_anims/sprites/music_notes.4bpp.lz");
const u32 gBattleAnimSpritePal_MusicNotes[] = INCBIN_U32("graphics/battle_anims/sprites/music_notes.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Duck[] = INCBIN_U32("graphics/battle_anims/sprites/duck.4bpp.lz");
const u32 gBattleAnimSpritePal_Duck[] = INCBIN_U32("graphics/battle_anims/sprites/duck.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Alert[] = INCBIN_U32("graphics/battle_anims/sprites/alert.4bpp.lz");
const u32 gBattleAnimSpritePal_Alert[] = INCBIN_U32("graphics/battle_anims/sprites/alert.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Shock4[] = INCBIN_U32("graphics/battle_anims/sprites/shock_4.4bpp.lz");
const u32 gBattleAnimSpritePal_Shock4[] = INCBIN_U32("graphics/battle_anims/sprites/shock_4.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Shock[] = INCBIN_U32("graphics/battle_anims/sprites/shock.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Bell2[] = INCBIN_U32("graphics/battle_anims/sprites/bell_2.4bpp.lz");
const u32 gBattleAnimSpritePal_Bell2[] = INCBIN_U32("graphics/battle_anims/sprites/bell_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_PinkGlove[] = INCBIN_U32("graphics/battle_anims/sprites/pink_glove.4bpp.lz");
const u32 gBattleAnimSpritePal_PinkGlove[] = INCBIN_U32("graphics/battle_anims/sprites/pink_glove.gbapal.lz");

const u16 gBattleAnimUnused_Unknown1[] = INCBIN_U16("graphics/battle_anims/unused/unknown_1.bin");
const u16 gBattleAnimUnused_Unknown2[] = INCBIN_U16("graphics/battle_anims/unused/unknown_2.bin");
const u16 gBattleAnimUnused_Unknown3[] = INCBIN_U16("graphics/battle_anims/unused/unknown_3.bin");

const u32 gBattleAnimUnusedGfx_LineSketch2[] = INCBIN_U32("graphics/battle_anims/unused/line_sketch_2.8bpp.lz");
const u16 gBattleAnimUnusedPal_LineSketch2[] = INCBIN_U16("graphics/battle_anims/unused/line_sketch_2.gbapal.lz");
const u32 gBattleAnimUnusedTilemap_LineSketch2[] = INCBIN_U32("graphics/battle_anims/unused/line_sketch_2.bin.lz");

const u32 gBattleAnimSpriteGfx_BlueLines[] = INCBIN_U32("graphics/battle_anims/sprites/blue_lines.4bpp.lz");
const u32 gBattleAnimSpritePal_BlueLines[] = INCBIN_U32("graphics/battle_anims/sprites/blue_lines.gbapal.lz");

const u32 gBattleAnimSpritePal_Impact3[] = INCBIN_U32("graphics/battle_anims/sprites/impact_3.gbapal.lz");
const u32 gBattleAnimSpritePal_Impact2[] = INCBIN_U32("graphics/battle_anims/sprites/impact_2.gbapal.lz");
const u32 gBattleAnimSpritePal_Reticle[] = INCBIN_U32("graphics/battle_anims/sprites/reticle.gbapal.lz");
const u32 gBattleAnimSpritePal_Breath[] = INCBIN_U32("graphics/battle_anims/sprites/breath.gbapal.lz");
const u32 gBattleAnimSpritePal_Snowball[] = INCBIN_U32("graphics/battle_anims/sprites/snowball.gbapal.lz");
const u32 gBattleAnimSpritePal_Vine[] = INCBIN_U32("graphics/battle_anims/sprites/vine.gbapal.lz");
const u32 gBattleAnimSpritePal_Sword2[] = INCBIN_U32("graphics/battle_anims/sprites/sword_2.gbapal.lz");
const u32 gBattleAnimSpritePal_RedTube[] = INCBIN_U32("graphics/battle_anims/sprites/red_tube.gbapal.lz");
const u32 gBattleAnimSpritePal_Amnesia[] = INCBIN_U32("graphics/battle_anims/sprites/amnesia.gbapal.lz");
const u32 gBattleAnimSpritePal_String2[] = INCBIN_U32("graphics/battle_anims/sprites/string_2.gbapal.lz");

const u32 gBattleAnimUnused_Unknown4[] = INCBIN_U32("graphics/battle_anims/unused/unknown_4.bin.lz");

const u32 gBattleAnimSpritePal_Pencil2[] = INCBIN_U32("graphics/battle_anims/sprites/pencil_2.gbapal.lz");
const u32 gBattleAnimSpritePal_Petal[] = INCBIN_U32("graphics/battle_anims/sprites/petal.gbapal.lz");
const u32 gBattleAnimSpritePal_BentSpoon[] = INCBIN_U32("graphics/battle_anims/sprites/bent_spoon.gbapal.lz");
const u32 gBattleAnimSpritePal_Coin[] = INCBIN_U32("graphics/battle_anims/sprites/coin.gbapal.lz");
const u32 gBattleAnimSpritePal_CrackedEgg[] = INCBIN_U32("graphics/battle_anims/sprites/cracked_egg.gbapal.lz");
const u32 gBattleAnimSpritePal_FreshEgg[] = INCBIN_U32("graphics/battle_anims/sprites/fresh_egg.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Impact3[] = INCBIN_U32("graphics/battle_anims/sprites/impact_3.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Impact2[] = INCBIN_U32("graphics/battle_anims/sprites/impact_2.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Reticle[] = INCBIN_U32("graphics/battle_anims/sprites/reticle.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Breath[] = INCBIN_U32("graphics/battle_anims/sprites/breath.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Snowball[] = INCBIN_U32("graphics/battle_anims/sprites/snowball.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Vine[] = INCBIN_U32("graphics/battle_anims/sprites/vine.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Sword2[] = INCBIN_U32("graphics/battle_anims/sprites/sword_2.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Clapping[] = INCBIN_U32("graphics/battle_anims/sprites/clapping.4bpp.lz");
const u32 gBattleAnimSpriteGfx_RedTube[] = INCBIN_U32("graphics/battle_anims/sprites/red_tube.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Amnesia[] = INCBIN_U32("graphics/battle_anims/sprites/amnesia.4bpp.lz");
const u32 gBattleAnimSpriteGfx_String2[] = INCBIN_U32("graphics/battle_anims/sprites/string_2.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Pencil2[] = INCBIN_U32("graphics/battle_anims/sprites/pencil_2.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Petal[] = INCBIN_U32("graphics/battle_anims/sprites/petal.4bpp.lz");
const u32 gBattleAnimSpriteGfx_BentSpoon[] = INCBIN_U32("graphics/battle_anims/sprites/bent_spoon.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Web[] = INCBIN_U32("graphics/battle_anims/sprites/web.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Coin[] = INCBIN_U32("graphics/battle_anims/sprites/coin.4bpp.lz");
const u32 gBattleAnimSpriteGfx_CrackedEgg[] = INCBIN_U32("graphics/battle_anims/sprites/cracked_egg.4bpp.lz");
const u32 gBattleAnimSpriteGfx_HatchedEgg[] = INCBIN_U32("graphics/battle_anims/sprites/hatched_egg.4bpp.lz");
const u32 gBattleAnimSpriteGfx_FreshEgg[] = INCBIN_U32("graphics/battle_anims/sprites/fresh_egg.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Fangs[] = INCBIN_U32("graphics/battle_anims/sprites/fangs.4bpp.lz");
const u32 gBattleAnimSpritePal_Fangs[] = INCBIN_U32("graphics/battle_anims/sprites/fangs.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Explosion2[] = INCBIN_U32("graphics/battle_anims/sprites/explosion_2.4bpp.lz");
const u32 gBattleAnimSpritePal_Explosion2[] = INCBIN_U32("graphics/battle_anims/sprites/explosion_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Explosion3[] = INCBIN_U32("graphics/battle_anims/sprites/explosion_3.4bpp.lz");

const u32 gBattleAnimSpriteGfx_WaterDroplet[] = INCBIN_U32("graphics/battle_anims/sprites/water_droplet.4bpp.lz");
const u32 gBattleAnimSpritePal_WaterDroplet[] = INCBIN_U32("graphics/battle_anims/sprites/water_droplet.gbapal.lz");

const u32 gBattleAnimSpriteGfx_WaterDroplet2[] = INCBIN_U32("graphics/battle_anims/sprites/water_droplet_2.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Seed2[] = INCBIN_U32("graphics/battle_anims/sprites/seed_2.4bpp.lz");
const u32 gBattleAnimSpritePal_Seed2[] = INCBIN_U32("graphics/battle_anims/sprites/seed_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Sprout[] = INCBIN_U32("graphics/battle_anims/sprites/sprout.4bpp.lz");

const u32 gBattleAnimSpriteGfx_RedWand[] = INCBIN_U32("graphics/battle_anims/sprites/red_wand.4bpp.lz");
const u32 gBattleAnimSpritePal_RedWand[] = INCBIN_U32("graphics/battle_anims/sprites/red_wand.gbapal.lz");

const u32 gBattleAnimSpriteGfx_PurpleGreenUnk[] = INCBIN_U32("graphics/battle_anims/sprites/purple_green_unk.4bpp.lz");
const u32 gBattleAnimSpritePal_PurpleGreenUnk[] = INCBIN_U32("graphics/battle_anims/sprites/purple_green_unk.gbapal.lz");

const u32 gBattleAnimSpriteGfx_WaterColumn[] = INCBIN_U32("graphics/battle_anims/sprites/water_column.4bpp.lz");
const u32 gBattleAnimSpritePal_WaterColumn[] = INCBIN_U32("graphics/battle_anims/sprites/water_column.gbapal.lz");

const u32 gBattleAnimSpriteGfx_MudUnk[] = INCBIN_U32("graphics/battle_anims/sprites/mud_unk.4bpp.lz");
const u32 gBattleAnimSpritePal_MudUnk[] = INCBIN_U32("graphics/battle_anims/sprites/mud_unk.gbapal.lz");

const u32 gUnusedTilemap_BlueFrame[] = INCBIN_U32("graphics/unused/blue_frame.bin.lz"); // P1, P2, P3 and P4 tilemaps?
const u32 gUnusedTilemap_RedYellowGreenFrame[] = INCBIN_U32("graphics/unused/redyellowgreen_frame.bin.lz");
const u32 gUnusedGfx_ColorFrames[] = INCBIN_U32("graphics/unused/color_frames.4bpp.lz");
const u32 gUnusedPal_ColorFrames[] = INCBIN_U32("graphics/unused/color_frames.gbapal.lz");

const u32 gBattleAnimSpriteGfx_RainDrops[] = INCBIN_U32("graphics/battle_anims/sprites/rain_drops.4bpp.lz");

const u32 gBattleAnimUnusedGfx_WaterSplash[] = INCBIN_U32("graphics/battle_anims/unused/water_splash.8bpp.lz");
const u32 gBattleAnimUnusedTilemap_WaterSplash[] = INCBIN_U32("graphics/battle_anims/unused/water_splash.bin.lz");
const u32 gBattleAnimUnusedPal_WaterSplash[] = INCBIN_U32("graphics/battle_anims/unused/water_splash.gbapal.lz");

const u32 gUnusedGfx_BasicFrame[] = INCBIN_U32("graphics/unused/basic_frame.4bpp.lz");
const u32 gUnusedPal_BasicFrame[] = INCBIN_U32("graphics/unused/basic_frame.gbapal.lz");
const u32 gUnusedTilemap_BasicFrame[] = INCBIN_U32("graphics/unused/basic_frame.bin.lz");

// Battle Interface

const u16 gBattleInterface_BallStatusBarPal[] = INCBIN_U16("graphics/battle_interface/ball_status_bar.gbapal");

const u16 gBattleInterface_BallDisplayPal[] = INCBIN_U16("graphics/battle_interface/ball_display.gbapal");

const u8 gHealthboxElementsGfxTable[] = INCBIN_U8("graphics/battle_interface/hpbar.4bpp",
                                                  "graphics/battle_interface/expbar.4bpp",
                                                  "graphics/battle_interface/status.4bpp",
                                                  "graphics/battle_interface/misc.4bpp",
                                                  "graphics/battle_interface/hpbar_anim.4bpp",
                                                  "graphics/battle_interface/misc_frameend.4bpp",
                                                  "graphics/battle_interface/ball_display.4bpp",
                                                  "graphics/battle_interface/ball_caught_indicator.4bpp",
                                                  "graphics/battle_interface/status2.4bpp", // these three duplicate sets of graphics are for the opponent/partner Pokémon
                                                  "graphics/battle_interface/status3.4bpp",
                                                  "graphics/battle_interface/status4.4bpp",
                                                  "graphics/battle_interface/healthbox_doubles_frameend.4bpp",
                                                  "graphics/battle_interface/healthbox_doubles_frameend_bar.4bpp");
const u32 gBattleInterfaceGfx_UnusedWindow3[] = INCBIN_U32("graphics/battle_interface/unused_window3.4bpp.lz");
const u32 gBattleInterfaceGfx_UnusedWindow4[] = INCBIN_U32("graphics/battle_interface/unused_window4.4bpp.lz");

const u32 gBattleAnimSpriteGfx_FurySwipes[] = INCBIN_U32("graphics/battle_anims/sprites/fury_swipes.4bpp.lz");
const u32 gBattleAnimSpritePal_FurySwipes[] = INCBIN_U32("graphics/battle_anims/sprites/fury_swipes.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Vine2[] = INCBIN_U32("graphics/battle_anims/sprites/vine_2.4bpp.lz");
const u32 gBattleAnimSpritePal_Vine2[] = INCBIN_U32("graphics/battle_anims/sprites/vine_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Teeth[] = INCBIN_U32("graphics/battle_anims/sprites/teeth.4bpp.lz");
const u32 gBattleAnimSpritePal_Teeth[] = INCBIN_U32("graphics/battle_anims/sprites/teeth.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Bone2[] = INCBIN_U32("graphics/battle_anims/sprites/bone_2.4bpp.lz");
const u32 gBattleAnimSpritePal_Bone2[] = INCBIN_U32("graphics/battle_anims/sprites/bone_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_WhiteBag[] = INCBIN_U32("graphics/battle_anims/sprites/white_bag.4bpp.lz");
const u32 gBattleAnimSpritePal_WhiteBag[] = INCBIN_U32("graphics/battle_anims/sprites/white_bag.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Unknown[] = INCBIN_U32("graphics/battle_anims/sprites/unknown.4bpp.lz");
const u32 gBattleAnimSpritePal_Unknown[] = INCBIN_U32("graphics/battle_anims/sprites/unknown.gbapal.lz");

const u32 gBattleAnimSpriteGfx_PurpleCoral[] = INCBIN_U32("graphics/battle_anims/sprites/purple_coral.4bpp.lz");
const u32 gBattleAnimSpritePal_PurpleCoral[] = INCBIN_U32("graphics/battle_anims/sprites/purple_coral.gbapal.lz");

const u32 gBattleAnimSpriteGfx_PurpleDroplet[] = INCBIN_U32("graphics/battle_anims/sprites/purple_droplet.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Shock2[] = INCBIN_U32("graphics/battle_anims/sprites/shock_2.4bpp.lz");
const u32 gBattleAnimSpritePal_Shock2[] = INCBIN_U32("graphics/battle_anims/sprites/shock_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_ClosingEye2[] = INCBIN_U32("graphics/battle_anims/sprites/closing_eye_2.4bpp.lz");
const u32 gBattleAnimSpritePal_ClosingEye2[] = INCBIN_U32("graphics/battle_anims/sprites/closing_eye_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_MetalBall[] = INCBIN_U32("graphics/battle_anims/sprites/metal_ball.4bpp.lz");
const u32 gBattleAnimSpritePal_MetalBall[] = INCBIN_U32("graphics/battle_anims/sprites/metal_ball.gbapal.lz");

const u32 gBattleAnimSpriteGfx_MonsterDoll[] = INCBIN_U32("graphics/battle_anims/sprites/monster_doll.4bpp.lz");
const u32 gBattleAnimSpritePal_MonsterDoll[] = INCBIN_U32("graphics/battle_anims/sprites/monster_doll.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Whirlwind[] = INCBIN_U32("graphics/battle_anims/sprites/whirlwind.4bpp.lz");
const u32 gBattleAnimSpritePal_Whirlwind[] = INCBIN_U32("graphics/battle_anims/sprites/whirlwind.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Whirlwind2[] = INCBIN_U32("graphics/battle_anims/sprites/whirlwind_2.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Explosion4[] = INCBIN_U32("graphics/battle_anims/sprites/explosion_4.4bpp.lz");
const u32 gBattleAnimSpritePal_Explosion4[] = INCBIN_U32("graphics/battle_anims/sprites/explosion_4.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Explosion5[] = INCBIN_U32("graphics/battle_anims/sprites/explosion_5.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Tongue[] = INCBIN_U32("graphics/battle_anims/sprites/tongue.4bpp.lz");
const u32 gBattleAnimSpritePal_Tongue[] = INCBIN_U32("graphics/battle_anims/sprites/tongue.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Smoke[] = INCBIN_U32("graphics/battle_anims/sprites/smoke.4bpp.lz");
const u32 gBattleAnimSpritePal_Smoke[] = INCBIN_U32("graphics/battle_anims/sprites/smoke.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Smoke2[] = INCBIN_U32("graphics/battle_anims/sprites/smoke_2.4bpp.lz");

const u32 gBattleAnimSpriteGfx_BlueFlames[] = INCBIN_U32("graphics/battle_anims/sprites/blue_flames.4bpp.lz");
const u32 gBattleAnimSpritePal_BlueFlames[] = INCBIN_U32("graphics/battle_anims/sprites/blue_flames.gbapal.lz");

const u32 gBattleAnimSpriteGfx_BlueFlames2[] = INCBIN_U32("graphics/battle_anims/sprites/blue_flames_2.4bpp.lz");

// Contest
const u32 gJPContestGfx1[] = INCBIN_U32("graphics/contest/japanese/composite_1.4bpp.lz");
const u32 gJPContestPal[] = INCBIN_U32("graphics/contest/japanese/palette.gbapal.lz");
const u32 gJPContestBgTilemap[] = INCBIN_U32("graphics/contest/japanese/bg.bin.lz");
const u32 gJPContestWindowsTilemap[] = INCBIN_U32("graphics/contest/japanese/windows.bin.lz");
const u32 gJPContestGfx2[] = INCBIN_U32("graphics/contest/japanese/composite_2.4bpp.lz");

const u32 gContestInterfaceAudiencePalette[] = INCBIN_U32("graphics/contest/interface.gbapal.lz");
const u32 gContestAudienceTilemap[] = INCBIN_U32("graphics/contest/audience.bin.lz");
const u32 gContestInterfaceTilemap[] = INCBIN_U32("graphics/contest/interface.bin.lz");
const u32 gJPContestInterfaceTilemap[] = INCBIN_U32("graphics/contest/japanese/interface.bin.lz");
const u32 gJPContestAudienceTilemap[] = INCBIN_U32("graphics/contest/japanese/audience.bin.lz");
const u32 gContestCurtainTilemap[] = INCBIN_U32("graphics/contest/curtain.bin.lz");

const u32 gContestInterfaceGfx[] = INCBIN_U32("graphics/contest/interface.4bpp.lz");
const u32 gContestAudienceGfx[] = INCBIN_U32("graphics/contest/audience.4bpp.lz");
const u32 gContestFaces_Gfx[] = INCBIN_U32("graphics/contest/faces.4bpp.lz");
const u32 gContestJudgeSymbolsGfx[] = INCBIN_U32("graphics/contest/judge_symbols.4bpp.lz");
const u32 gContestJudgeSymbolsPal[] = INCBIN_U32("graphics/contest/judge_symbols.gbapal.lz");
const u8 gContestSliderHeart_Gfx[] = INCBIN_U8("graphics/contest/slider_heart.4bpp");

// JP equivalent of the Applause meter
const u32 gJPContestVoltageGfx[] = INCBIN_U32("graphics/contest/japanese/voltage.4bpp.lz");
const u32 gJPContestVoltagePal[] = INCBIN_U32("graphics/contest/japanese/voltage.gbapal.lz");

// Contest results
const u32 gContestResults_Gfx[]                  = INCBIN_U32("graphics/contest/results_screen/tiles.4bpp.lz");
const u32 gContestResults_WinnerBanner_Tilemap[] = INCBIN_U32("graphics/contest/results_screen/winner_banner.bin.lz");
const u32 gContestResults_Interface_Tilemap[]    = INCBIN_U32("graphics/contest/results_screen/interface.bin.lz");
const u32 gContestResults_Bg_Tilemap[]           = INCBIN_U32("graphics/contest/results_screen/bg.bin.lz");
const u32 gContestResults_Pal[]                  = INCBIN_U32("graphics/contest/results_screen/tiles.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Impact[] = INCBIN_U32("graphics/battle_anims/sprites/impact.4bpp.lz");
const u32 gBattleAnimSpritePal_Impact[] = INCBIN_U32("graphics/battle_anims/sprites/impact.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Particles[] = INCBIN_U32("graphics/battle_anims/sprites/particles.4bpp.lz");

const u32 gBattleAnimSpriteGfx_CircleImpact[] = INCBIN_U32("graphics/battle_anims/sprites/circle_impact.4bpp.lz");
const u32 gBattleAnimSpritePal_CircleImpact[] = INCBIN_U32("graphics/battle_anims/sprites/circle_impact.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Scratch[] = INCBIN_U32("graphics/battle_anims/sprites/scratch.4bpp.lz");

const u32 gBattleAnimSpriteGfx_SharpTeeth[] = INCBIN_U32("graphics/battle_anims/sprites/sharp_teeth.4bpp.lz");
const u32 gBattleAnimSpritePal_SharpTeeth[] = INCBIN_U32("graphics/battle_anims/sprites/sharp_teeth.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Clamp[] = INCBIN_U32("graphics/battle_anims/sprites/clamp.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Cut[] = INCBIN_U32("graphics/battle_anims/sprites/cut.4bpp.lz");

const u32 gBattleAnimSpriteGfx_RainbowRings[] = INCBIN_U32("graphics/battle_anims/sprites/rainbow_rings.4bpp.lz");
const u32 gBattleAnimSpritePal_RainbowRings[] = INCBIN_U32("graphics/battle_anims/sprites/rainbow_rings.gbapal.lz");

const u32 gBattleAnimSpriteGfx_IceCrystals[] = INCBIN_U32("graphics/battle_anims/sprites/ice_crystals.4bpp.lz");
const u32 gBattleAnimSpritePal_IceCrystals[] = INCBIN_U32("graphics/battle_anims/sprites/ice_crystals.gbapal.lz");

const u32 gBattleAnimSpriteGfx_IceSpikes[] = INCBIN_U32("graphics/battle_anims/sprites/ice_spikes.4bpp.lz");

const u32 gBattleAnimSpriteGfx_OldBeatUp[] = INCBIN_U32("graphics/battle_anims/unused/old_beatup.4bpp.lz");
const u32 gBattleAnimSpritePal_OldBeatUp[] = INCBIN_U32("graphics/battle_anims/unused/old_beatup.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Orbs[] = INCBIN_U32("graphics/battle_anims/sprites/orbs.4bpp.lz");
const u32 gBattleAnimSpritePal_Orbs[] = INCBIN_U32("graphics/battle_anims/sprites/orbs.gbapal.lz");

const u32 gBattleAnimSpriteGfx_WaterOrb[] = INCBIN_U32("graphics/battle_anims/sprites/water_orb.4bpp.lz");

const u32 gBattleAnimSpriteGfx_WaterImpact[] = INCBIN_U32("graphics/battle_anims/sprites/water_impact.4bpp.lz");
const u32 gBattleAnimSpritePal_WaterImpact[] = INCBIN_U32("graphics/battle_anims/sprites/water_impact.gbapal.lz");

const u32 gBattleAnimSpritePal_BrownOrb[] = INCBIN_U32("graphics/battle_anims/sprites/brown_orb.gbapal.lz");

const u32 gBattleAnimSpriteGfx_MudSand[] = INCBIN_U32("graphics/battle_anims/sprites/mud_sand.4bpp.lz");
const u32 gBattleAnimSpritePal_MudSand[] = INCBIN_U32("graphics/battle_anims/sprites/mud_sand.gbapal.lz");

const u32 gBattleAnimSpriteGfx_PoisonBubble[] = INCBIN_U32("graphics/battle_anims/sprites/poison_bubble.4bpp.lz");
const u32 gBattleAnimSpritePal_PoisonBubble[] = INCBIN_U32("graphics/battle_anims/sprites/poison_bubble.gbapal.lz");

const u32 gBattleAnimSpriteGfx_ToxicBubble[] = INCBIN_U32("graphics/battle_anims/sprites/toxic_bubble.4bpp.lz");

const u32 gBattleAnimSpriteGfx_HornHit2[] = INCBIN_U32("graphics/battle_anims/sprites/horn_hit_2.4bpp.lz");
const u32 gBattleAnimSpritePal_HornHit2[] = INCBIN_U32("graphics/battle_anims/sprites/horn_hit_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_AirWave2[] = INCBIN_U32("graphics/battle_anims/sprites/air_wave_2.4bpp.lz");
const u32 gBattleAnimSpritePal_AirWave2[] = INCBIN_U32("graphics/battle_anims/sprites/air_wave_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_SmallBubbles[] = INCBIN_U32("graphics/battle_anims/sprites/small_bubbles.4bpp.lz");
const u32 gBattleAnimSpritePal_SmallBubbles[] = INCBIN_U32("graphics/battle_anims/sprites/small_bubbles.gbapal.lz");

const u32 gBattleAnimSpriteGfx_RoundShadow[] = INCBIN_U32("graphics/battle_anims/sprites/round_shadow.4bpp.lz");
const u32 gBattleAnimSpritePal_RoundShadow[] = INCBIN_U32("graphics/battle_anims/sprites/round_shadow.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Sunlight[] = INCBIN_U32("graphics/battle_anims/sprites/sunlight.4bpp.lz");
const u32 gBattleAnimSpritePal_Sunlight[] = INCBIN_U32("graphics/battle_anims/sprites/sunlight.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Spore[] = INCBIN_U32("graphics/battle_anims/sprites/spore.4bpp.lz");

const u32 gBattleAnimSpritePal_Spore[] = INCBIN_U32("graphics/battle_anims/sprites/spore.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Flower[] = INCBIN_U32("graphics/battle_anims/sprites/flower.4bpp.lz");
const u32 gBattleAnimSpritePal_Flower[] = INCBIN_U32("graphics/battle_anims/sprites/flower.gbapal.lz");

const u32 gBattleAnimSpriteGfx_RazorLeaf[] = INCBIN_U32("graphics/battle_anims/sprites/razor_leaf.4bpp.lz");
const u32 gBattleAnimSpritePal_RazorLeaf[] = INCBIN_U32("graphics/battle_anims/sprites/razor_leaf.gbapal.lz");

const u32 gBattleAnimSpriteGfx_MistCloud[] = INCBIN_U32("graphics/battle_anims/sprites/mist_cloud.4bpp.lz");
const u32 gBattleAnimSpritePal_MistCloud[] = INCBIN_U32("graphics/battle_anims/sprites/mist_cloud.gbapal.lz");

const u32 gBattleAnimUnusedGfx_Lights[] = INCBIN_U32("graphics/battle_anims/unused/lights.4bpp.lz");
const u32 gBattleAnimUnusedPal_Lights[] = INCBIN_U32("graphics/battle_anims/unused/lights.gbapal.lz");
const u32 gBattleAnimUnusedTilemap_Lights[] = INCBIN_U32("graphics/battle_anims/unused/lights.bin.lz");

const u32 gBattleAnimSpriteGfx_WhirlwindLines[] = INCBIN_U32("graphics/battle_anims/sprites/whirlwind_lines.4bpp.lz");
const u32 gBattleAnimSpritePal_WhirlwindLines[] = INCBIN_U32("graphics/battle_anims/sprites/whirlwind_lines.gbapal.lz");

const u32 gBattleAnimSpriteGfx_GoldRing[] = INCBIN_U32("graphics/battle_anims/sprites/gold_ring.4bpp.lz");
const u32 gBattleAnimSpritePal_GoldRing[] = INCBIN_U32("graphics/battle_anims/sprites/gold_ring.gbapal.lz");

const u32 gBattleAnimSpritePal_BlueRing2[] = INCBIN_U32("graphics/battle_anims/sprites/blue_ring_2.gbapal.lz");
const u32 gBattleAnimSpritePal_PurpleRing[] = INCBIN_U32("graphics/battle_anims/sprites/purple_ring.gbapal.lz");
const u32 gBattleAnimSpritePal_BlueRing[] = INCBIN_U32("graphics/battle_anims/sprites/blue_ring.gbapal.lz");

const u32 gBattleAnimSpriteGfx_GreenLightWall[] = INCBIN_U32("graphics/battle_anims/sprites/green_light_wall.4bpp.lz");
const u32 gBattleAnimSpritePal_GreenLightWall[] = INCBIN_U32("graphics/battle_anims/sprites/green_light_wall.gbapal.lz");

const u32 gBattleAnimSpritePal_BlueLightWall[] = INCBIN_U32("graphics/battle_anims/sprites/blue_light_wall.gbapal.lz");
const u32 gBattleAnimSpritePal_RedLightWall[] = INCBIN_U32("graphics/battle_anims/sprites/red_light_wall.gbapal.lz");
const u32 gBattleAnimSpritePal_GrayLightWall[] = INCBIN_U32("graphics/battle_anims/sprites/gray_light_wall.gbapal.lz");
const u32 gBattleAnimSpritePal_OrangeLightWall[] = INCBIN_U32("graphics/battle_anims/sprites/orange_light_wall.gbapal.lz");

const u32 gBattleAnimSpriteGfx_BlackBall2[] = INCBIN_U32("graphics/battle_anims/sprites/black_ball_2.4bpp.lz");
const u32 gBattleAnimSpritePal_BlackBall2[] = INCBIN_U32("graphics/battle_anims/sprites/black_ball_2.gbapal.lz");

const u32 gBattleAnimSpritePal_PurpleGasCloud[] = INCBIN_U32("graphics/battle_anims/sprites/purple_gas_cloud.gbapal.lz");

const u32 gContestJudgeGfx[] = INCBIN_U32("graphics/contest/judge.4bpp.lz");
const u32 gContest2Pal[] = INCBIN_U32("graphics/contest/judge.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Spark[] = INCBIN_U32("graphics/battle_anims/sprites/spark.4bpp.lz");
const u32 gBattleAnimSpritePal_Spark[] = INCBIN_U32("graphics/battle_anims/sprites/spark.gbapal.lz");

const u32 gBattleAnimSpriteGfx_SparkH[] = INCBIN_U32("graphics/battle_anims/sprites/spark_h.4bpp.lz");

const u32 gBattleAnimBgImage_Dark[] = INCBIN_U32("graphics/battle_anims/backgrounds/dark.4bpp.lz");
const u32 gBattleAnimBgPalette_Dark[] = INCBIN_U32("graphics/battle_anims/backgrounds/dark.gbapal.lz");
const u32 gBattleAnimBgTilemap_Dark[] = INCBIN_U32("graphics/battle_anims/backgrounds/dark.bin.lz");

const u32 gMetalShineGfx[] = INCBIN_U32("graphics/battle_anims/masks/metal_shine.4bpp.lz");
const u32 gMetalShinePalette[] = INCBIN_U32("graphics/battle_anims/masks/metal_shine.gbapal.lz");
const u32 gMetalShineTilemap[] = INCBIN_U32("graphics/battle_anims/masks/metal_shine.bin.lz");

const u32 gUnusedGfx_Goosuto[] = INCBIN_U32("graphics/battle_anims/unused/goosuto.4bpp.lz"); // ghost
const u32 gUnusedPal_Goosuto[] = INCBIN_U32("graphics/battle_anims/unused/goosuto.gbapal.lz");
const u32 gUnusedTilemap_Goosuto[] = INCBIN_U32("graphics/battle_anims/unused/goosuto.bin.lz");

const u32 gBattleAnimSpriteGfx_YellowStar[] = INCBIN_U32("graphics/battle_anims/sprites/yellow_star.4bpp.lz");
const u32 gBattleAnimSpritePal_YellowStar[] = INCBIN_U32("graphics/battle_anims/sprites/yellow_star.gbapal.lz");

const u32 gBattleAnimSpriteGfx_LargeFreshEgg[] = INCBIN_U32("graphics/battle_anims/sprites/large_fresh_egg.4bpp.lz");
const u32 gBattleAnimSpritePal_LargeFreshEgg[] = INCBIN_U32("graphics/battle_anims/sprites/large_fresh_egg.gbapal.lz");

const u32 gBattleAnimSpriteGfx_ShadowBall[] = INCBIN_U32("graphics/battle_anims/sprites/shadow_ball.4bpp.lz");
const u32 gBattleAnimSpritePal_ShadowBall[] = INCBIN_U32("graphics/battle_anims/sprites/shadow_ball.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Lick[] = INCBIN_U32("graphics/battle_anims/sprites/lick.4bpp.lz");
const u32 gBattleAnimSpritePal_Lick[] = INCBIN_U32("graphics/battle_anims/sprites/lick.gbapal.lz");

const u32 gBattleAnimSpriteGfx_VoidLines[] = INCBIN_U32("graphics/battle_anims/sprites/void_lines.4bpp.lz");
const u32 gBattleAnimSpritePal_VoidLines[] = INCBIN_U32("graphics/battle_anims/sprites/void_lines.gbapal.lz");

const u32 gBattleAnimSpritePal_String[] = INCBIN_U32("graphics/battle_anims/sprites/string.gbapal.lz");
const u32 gBattleAnimSpriteGfx_String[] = INCBIN_U32("graphics/battle_anims/sprites/string.4bpp.lz");

const u32 gBattleAnimSpriteGfx_WebThread[] = INCBIN_U32("graphics/battle_anims/sprites/web_thread.4bpp.lz");
const u32 gBattleAnimSpriteGfx_SpiderWeb[] = INCBIN_U32("graphics/battle_anims/sprites/spider_web.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Lightbulb[] = INCBIN_U32("graphics/battle_anims/sprites/lightbulb.4bpp.lz");
const u32 gBattleAnimSpritePal_Lightbulb[] = INCBIN_U32("graphics/battle_anims/sprites/lightbulb.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Slash[] = INCBIN_U32("graphics/battle_anims/sprites/slash.4bpp.lz");
const u32 gBattleAnimSpritePal_Slash[] = INCBIN_U32("graphics/battle_anims/sprites/slash.gbapal.lz");

const u32 gBattleAnimSpriteGfx_FocusEnergy[] = INCBIN_U32("graphics/battle_anims/sprites/focus_energy.4bpp.lz");
const u32 gBattleAnimSpritePal_FocusEnergy[] = INCBIN_U32("graphics/battle_anims/sprites/focus_energy.gbapal.lz");

const u32 gBattleAnimSpriteGfx_SphereToCube[] = INCBIN_U32("graphics/battle_anims/sprites/sphere_to_cube.4bpp.lz");
const u32 gBattleAnimSpritePal_SphereToCube[] = INCBIN_U32("graphics/battle_anims/sprites/sphere_to_cube.gbapal.lz");

const u32 gBattleAnimBgImage_Psychic[] = INCBIN_U32("graphics/battle_anims/backgrounds/psychic.4bpp.lz");
const u32 gBattleAnimBgPalette_Psychic[] = INCBIN_U32("graphics/battle_anims/backgrounds/psychic.gbapal.lz");
const u32 gBattleAnimBgTilemap_Psychic[] = INCBIN_U32("graphics/battle_anims/backgrounds/psychic.bin.lz");

const u32 gBattleAnimSpriteGfx_Eye[] = INCBIN_U32("graphics/battle_anims/sprites/eye.4bpp.lz");
const u32 gBattleAnimSpritePal_Eye[] = INCBIN_U32("graphics/battle_anims/sprites/eye.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Tendrils[] = INCBIN_U32("graphics/battle_anims/sprites/tendrils.4bpp.lz");
const u32 gBattleAnimSpritePal_Tendrils[] = INCBIN_U32("graphics/battle_anims/sprites/tendrils.gbapal.lz");

const u32 gHealthboxSinglesPlayerGfx[] = INCBIN_U32("graphics/battle_interface/healthbox_singles_player.4bpp.lz");
const u32 gHealthboxSinglesOpponentGfx[] = INCBIN_U32("graphics/battle_interface/healthbox_singles_opponent.4bpp.lz");
const u32 gHealthboxDoublesPlayerGfx[] = INCBIN_U32( "graphics/battle_interface/healthbox_doubles_player.4bpp.lz");
const u32 gHealthboxDoublesOpponentGfx[] = INCBIN_U32("graphics/battle_interface/healthbox_doubles_opponent.4bpp.lz");
const u32 gHealthboxSafariGfx[] = INCBIN_U32("graphics/battle_interface/healthbox_safari.4bpp.lz");

const u32 gUnusedGfx_Shadow[] = INCBIN_U32("graphics/unused/shadow.4bpp.lz");
const u32 gUnusedPal_Shadow[] = INCBIN_U32("graphics/unused/shadow.gbapal.lz");

const u32 gBattleAnimSpriteGfx_LockOn[] = INCBIN_U32("graphics/battle_anims/sprites/lock_on.4bpp.lz");
const u32 gBattleAnimSpritePal_LockOn[] = INCBIN_U32("graphics/battle_anims/sprites/lock_on.gbapal.lz");

const u32 gBattleAnimSpriteGfx_OpeningEye[] = INCBIN_U32("graphics/battle_anims/sprites/opening_eye.4bpp.lz");
const u32 gBattleAnimSpritePal_OpeningEye[] = INCBIN_U32("graphics/battle_anims/sprites/opening_eye.gbapal.lz");

const u32 gBattleAnimSpriteGfx_RoundWhiteHalo[] = INCBIN_U32("graphics/battle_anims/sprites/round_white_halo.4bpp.lz");
const u32 gBattleAnimSpritePal_RoundWhiteHalo[] = INCBIN_U32("graphics/battle_anims/sprites/round_white_halo.gbapal.lz");

const u32 gBattleAnimSpriteGfx_TealAlert[] = INCBIN_U32("graphics/battle_anims/sprites/teal_alert.4bpp.lz");
const u32 gBattleAnimSpritePal_TealAlert[] = INCBIN_U32("graphics/battle_anims/sprites/teal_alert.gbapal.lz");

const u32 gBattleAnimSpriteGfx_FangAttack[] = INCBIN_U32("graphics/battle_anims/sprites/fang_attack.4bpp.lz");
const u32 gBattleAnimSpritePal_FangAttack[] = INCBIN_U32("graphics/battle_anims/sprites/fang_attack.gbapal.lz");

const u32 gBattleAnimSpriteGfx_PurpleHandOutline[] = INCBIN_U32("graphics/battle_anims/sprites/purple_hand_outline.4bpp.lz");
const u32 gBattleAnimSpritePal_PurpleHandOutline[] = INCBIN_U32("graphics/battle_anims/sprites/purple_hand_outline.gbapal.lz");

const u32 gBattleAnimMaskImage_Curse[] = INCBIN_U32("graphics/battle_anims/masks/curse.4bpp.lz");
const u32 gBattleAnimMaskTilemap_Curse[] = INCBIN_U32("graphics/battle_anims/masks/curse.bin.lz");

const u32 gBattleAnimSpriteGfx_Pencil[] = INCBIN_U32("graphics/battle_anims/sprites/pencil.4bpp.lz");
const u32 gBattleAnimSpritePal_Pencil[] = INCBIN_U32("graphics/battle_anims/sprites/pencil.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Spiral[] = INCBIN_U32("graphics/battle_anims/sprites/spiral.4bpp.lz");
const u32 gBattleAnimSpritePal_Spiral[] = INCBIN_U32("graphics/battle_anims/sprites/spiral.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Moon[] = INCBIN_U32("graphics/battle_anims/sprites/moon.4bpp.lz");
const u32 gBattleAnimSpritePal_Moon[] = INCBIN_U32("graphics/battle_anims/sprites/moon.gbapal.lz");

const u32 gBattleAnimSpriteGfx_GreenSparkle[] = INCBIN_U32("graphics/battle_anims/sprites/green_sparkle.4bpp.lz");
const u32 gBattleAnimSpritePal_GreenSparkle[] = INCBIN_U32("graphics/battle_anims/sprites/green_sparkle.gbapal.lz");

const u32 gBattleAnimSpriteGfx_SnoreZ[] = INCBIN_U32("graphics/battle_anims/sprites/snore_z.4bpp.lz");
const u32 gBattleAnimSpritePal_SnoreZ[] = INCBIN_U32("graphics/battle_anims/sprites/snore_z.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Explosion[] = INCBIN_U32("graphics/battle_anims/sprites/explosion.4bpp.lz");
const u32 gBattleAnimSpritePal_Explosion[] = INCBIN_U32("graphics/battle_anims/sprites/explosion.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Nail[] = INCBIN_U32("graphics/battle_anims/sprites/nail.4bpp.lz");
const u32 gBattleAnimSpritePal_Nail[] = INCBIN_U32("graphics/battle_anims/sprites/nail.gbapal.lz");

const u32 gBattleAnimSpriteGfx_GhostlySpirit[] = INCBIN_U32("graphics/battle_anims/sprites/ghostly_spirit.4bpp.lz");
const u32 gBattleAnimSpritePal_GhostlySpirit[] = INCBIN_U32("graphics/battle_anims/sprites/ghostly_spirit.gbapal.lz");

const u32 gBattleAnimSpriteGfx_WarmRock[] = INCBIN_U32("graphics/battle_anims/sprites/warm_rock.4bpp.lz");
const u32 gBattleAnimSpritePal_WarmRock[] = INCBIN_U32("graphics/battle_anims/sprites/warm_rock.gbapal.lz");

const u32 gBattleAnimSpriteGfx_PunchImpact[] = INCBIN_U32("graphics/battle_anims/sprites/punch_impact.4bpp.lz");
const u32 gBattleAnimSpritePal_PunchImpact[] = INCBIN_U32("graphics/battle_anims/sprites/punch_impact.gbapal.lz");

const u32 gBattleAnimSpriteGfx_BreakingEgg[] = INCBIN_U32("graphics/battle_anims/sprites/breaking_egg.4bpp.lz");
const u32 gBattleAnimSpritePal_BreakingEgg[] = INCBIN_U32("graphics/battle_anims/sprites/breaking_egg.gbapal.lz");

const u32 gBattleAnimSpriteGfx_ThinRing[] = INCBIN_U32("graphics/battle_anims/sprites/thin_ring.4bpp.lz");
const u32 gBattleAnimSpritePal_ThinRing[] = INCBIN_U32("graphics/battle_anims/sprites/thin_ring.gbapal.lz");

const u32 gBattleAnimSpriteGfx_MusicNotes2[] = INCBIN_U32("graphics/battle_anims/sprites/music_notes_2.4bpp.lz");
const u32 gBattleAnimSpritePal_MusicNotes2[] = INCBIN_U32("graphics/battle_anims/sprites/music_notes_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Bell[] = INCBIN_U32("graphics/battle_anims/sprites/bell.4bpp.lz");
const u32 gBattleAnimSpritePal_Bell[] = INCBIN_U32("graphics/battle_anims/sprites/bell.gbapal.lz");

const u32 gBattleAnimSpriteGfx_SpeedDust[] = INCBIN_U32("graphics/battle_anims/sprites/speed_dust.4bpp.lz");
const u32 gBattleAnimSpritePal_SpeedDust[] = INCBIN_U32("graphics/battle_anims/sprites/speed_dust.gbapal.lz");

const u32 gBattleAnimSpriteGfx_TornMetal[] = INCBIN_U32("graphics/battle_anims/sprites/torn_metal.4bpp.lz");

const u32 gBattleAnimSpriteGfx_ThoughtBubble[] = INCBIN_U32("graphics/battle_anims/sprites/thought_bubble.4bpp.lz");
const u32 gBattleAnimSpritePal_ThoughtBubble[] = INCBIN_U32("graphics/battle_anims/sprites/thought_bubble.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Finger[] = INCBIN_U32("graphics/battle_anims/sprites/finger.4bpp.lz");
const u32 gBattleAnimSpritePal_Finger[] = INCBIN_U32("graphics/battle_anims/sprites/finger.gbapal.lz");

const u32 gBattleAnimSpriteGfx_MagentaHeart[] = INCBIN_U32("graphics/battle_anims/sprites/magenta_heart.4bpp.lz");

const u32 gBattleAnimSpritePal_PinkHeart[] = INCBIN_U32("graphics/battle_anims/sprites/pink_heart.gbapal.lz");
const u32 gBattleAnimSpritePal_MagentaHeart[] = INCBIN_U32("graphics/battle_anims/sprites/magenta_heart.gbapal.lz");
const u32 gBattleAnimSpritePal_RedHeart[] = INCBIN_U32("graphics/battle_anims/sprites/red_heart.gbapal.lz");

const u32 gBattleAnimBgImage_Attract[] = INCBIN_U32("graphics/battle_anims/backgrounds/attract.4bpp.lz");
const u32 gBattleAnimBgPalette_Attract[] = INCBIN_U32("graphics/battle_anims/backgrounds/attract.gbapal.lz");
const u32 gBattleAnimBgTilemap_Attract[] = INCBIN_U32("graphics/battle_anims/backgrounds/attract.bin.lz");

const u32 gBattleAnimSpriteGfx_RedOrb[] = INCBIN_U32("graphics/battle_anims/sprites/red_orb.4bpp.lz");
const u32 gBattleAnimSpritePal_RedOrb[] = INCBIN_U32("graphics/battle_anims/sprites/red_orb.gbapal.lz");

const u32 gBattleAnimSpriteGfx_CircleOfLight[] = INCBIN_U32("graphics/battle_anims/sprites/circle_of_light.4bpp.lz");
const u32 gBattleAnimSpriteGfx_ElectricOrbs[] = INCBIN_U32("graphics/battle_anims/sprites/electric_orbs.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Electricity[] = INCBIN_U32("graphics/battle_anims/sprites/electricity.4bpp.lz");

const u32 gBattleAnimSpritePal_ElectricOrbs[] = INCBIN_U32("graphics/battle_anims/sprites/electric_orbs.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Finger2[] = INCBIN_U32("graphics/battle_anims/sprites/finger_2.4bpp.lz");

const u32 gBattleAnimSpriteGfx_MovementWaves[] = INCBIN_U32("graphics/battle_anims/sprites/movement_waves.4bpp.lz");
const u32 gBattleAnimSpritePal_MovementWaves[] = INCBIN_U32("graphics/battle_anims/sprites/movement_waves.gbapal.lz");

const u32 gBattleAnimBgPalette_ScaryFace[] = INCBIN_U32("graphics/battle_anims/backgrounds/scary_face.gbapal.lz");
const u32 gBattleAnimBgImage_ScaryFace[] = INCBIN_U32("graphics/battle_anims/backgrounds/scary_face.4bpp.lz");

const u32 gBattleAnimSpritePal_EyeSparkle[] = INCBIN_U32("graphics/battle_anims/sprites/eye_sparkle.gbapal.lz");
const u32 gBattleAnimSpriteGfx_EyeSparkle[] = INCBIN_U32("graphics/battle_anims/sprites/eye_sparkle.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Anger[] = INCBIN_U32("graphics/battle_anims/sprites/anger.4bpp.lz");
const u32 gBattleAnimSpritePal_Anger[] = INCBIN_U32("graphics/battle_anims/sprites/anger.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Conversion[] = INCBIN_U32("graphics/battle_anims/sprites/conversion.4bpp.lz");
const u32 gBattleAnimSpritePal_Conversion[] = INCBIN_U32("graphics/battle_anims/sprites/conversion.gbapal.lz");

const u32 gBattleAnimSpritePal_Angel[] = INCBIN_U32("graphics/battle_anims/sprites/angel.gbapal.lz");
const u32 gBattleAnimSpriteGfx_Angel[] = INCBIN_U32("graphics/battle_anims/sprites/angel.4bpp.lz");

const u32 gBattleAnimSpritePal_Devil[] = INCBIN_U32("graphics/battle_anims/sprites/devil.gbapal.lz");
const u32 gBattleAnimSpriteGfx_Devil[] = INCBIN_U32("graphics/battle_anims/sprites/devil.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Swipe[] = INCBIN_U32("graphics/battle_anims/sprites/swipe.4bpp.lz");
const u32 gBattleAnimSpritePal_Swipe[] = INCBIN_U32("graphics/battle_anims/sprites/swipe.gbapal.lz");

const u32 gBattleAnimSpritePal_Roots[] = INCBIN_U32("graphics/battle_anims/sprites/roots.gbapal.lz");
const u32 gBattleAnimSpriteGfx_Roots[] = INCBIN_U32("graphics/battle_anims/sprites/roots.4bpp.lz");

const u32 gBattleAnimSpritePal_ItemBag[] = INCBIN_U32("graphics/battle_anims/sprites/item_bag.gbapal.lz");
const u32 gBattleAnimSpriteGfx_ItemBag[] = INCBIN_U32("graphics/battle_anims/sprites/item_bag.4bpp.lz");

const u32 gBattleAnimSpritePal_TriAttackTriangle[] = INCBIN_U32("graphics/battle_anims/sprites/tri_attack_triangle.gbapal.lz");
const u32 gBattleAnimSpriteGfx_TriAttackTriangle[] = INCBIN_U32("graphics/battle_anims/sprites/tri_attack_triangle.4bpp.lz");

const u32 gBattleAnimSpritePal_LetterZ[] = INCBIN_U32("graphics/battle_anims/sprites/letter_z.gbapal.lz");
const u32 gBattleAnimSpriteGfx_LetterZ[] = INCBIN_U32("graphics/battle_anims/sprites/letter_z.4bpp.lz");

const u32 gBattleAnimBgPalette_Impact[] = INCBIN_U32("graphics/battle_anims/backgrounds/impact.gbapal.lz");
const u32 gBattleAnimBgImage_Impact[] = INCBIN_U32("graphics/battle_anims/backgrounds/impact.4bpp.lz");
const u32 gBattleAnimBgTilemap_ImpactOpponent[] = INCBIN_U32("graphics/battle_anims/backgrounds/impact_opponent.bin.lz");

const u32 gBattleAnimBgTilemap_ImpactPlayer[] = INCBIN_U32("graphics/battle_anims/backgrounds/impact_player.bin.lz");
const u32 gBattleAnimBgTilemap_ImpactContests[] = INCBIN_U32("graphics/battle_anims/backgrounds/impact_contests.bin.lz");

const u32 gBattleAnimSpriteGfx_JaggedMusicNote[] = INCBIN_U32("graphics/battle_anims/sprites/jagged_music_note.4bpp.lz");
const u32 gBattleAnimSpritePal_JaggedMusicNote[] = INCBIN_U32("graphics/battle_anims/sprites/jagged_music_note.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Spotlight[] = INCBIN_U32("graphics/battle_anims/sprites/spotlight.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Pokeball[] = INCBIN_U32("graphics/battle_anims/sprites/pokeball.4bpp.lz");
const u32 gBattleAnimSpritePal_Pokeball[] = INCBIN_U32("graphics/battle_anims/sprites/pokeball.gbapal.lz");

const u32 gBattleAnimSpriteGfx_RapidSpin[] = INCBIN_U32("graphics/battle_anims/sprites/rapid_spin.4bpp.lz");
const u32 gBattleAnimSpritePal_RapidSpin[] = INCBIN_U32("graphics/battle_anims/sprites/rapid_spin.gbapal.lz");

const u32 gBattleAnimSpriteGfx_MilkBottle[] = INCBIN_U32("graphics/battle_anims/sprites/milk_bottle.4bpp.lz");
const u32 gBattleAnimSpritePal_MilkBottle[] = INCBIN_U32("graphics/battle_anims/sprites/milk_bottle.gbapal.lz");

const u32 gBattleAnimSpriteGfx_WispFire[] = INCBIN_U32("graphics/battle_anims/sprites/wisp_fire.4bpp.lz");

const u32 gBattleAnimSpritePal_WispOrb[] = INCBIN_U32("graphics/battle_anims/sprites/wisp_orb.gbapal.lz");
const u32 gBattleAnimSpriteGfx_WispOrb[] = INCBIN_U32("graphics/battle_anims/sprites/wisp_orb.4bpp.lz");

const u32 gBattleAnimSpriteGfx_GoldStars[] = INCBIN_U32("graphics/battle_anims/sprites/gold_stars.4bpp.lz");
const u32 gBattleAnimSpritePal_GoldStars[] = INCBIN_U32("graphics/battle_anims/sprites/gold_stars.gbapal.lz");

const u32 gBattleAnimSpriteGfx_EclipsingOrb[] = INCBIN_U32("graphics/battle_anims/sprites/eclipsing_orb.4bpp.lz");
const u32 gBattleAnimSpritePal_EclipsingOrb[] = INCBIN_U32("graphics/battle_anims/sprites/eclipsing_orb.gbapal.lz");

const u32 gBattleAnimSpriteGfx_PinkPetal[] = INCBIN_U32("graphics/battle_anims/sprites/pink_petal.4bpp.lz");
const u32 gBattleAnimSpritePal_PinkPetal[] = INCBIN_U32("graphics/battle_anims/sprites/pink_petal.gbapal.lz");

const u32 gBattleAnimSpriteGfx_GrayOrb[] = INCBIN_U32("graphics/battle_anims/sprites/gray_orb.4bpp.lz");
const u32 gBattleAnimSpritePal_GrayOrb[] = INCBIN_U32("graphics/battle_anims/sprites/gray_orb.gbapal.lz");

const u32 gBattleAnimSpritePal_BlueOrb[] = INCBIN_U32("graphics/battle_anims/sprites/blue_orb.gbapal.lz");
const u32 gBattleAnimSpritePal_RedOrb2[] = INCBIN_U32("graphics/battle_anims/sprites/red_orb_2.gbapal.lz");

const u32 gBattleAnimBgImage_Drill[] = INCBIN_U32("graphics/battle_anims/backgrounds/drill.4bpp.lz");
const u32 gBattleAnimBgPalette_Drill[] = INCBIN_U32("graphics/battle_anims/backgrounds/drill.gbapal.lz");

const u32 gBattleAnimBgPalette_Sky[] = INCBIN_U32("graphics/battle_anims/backgrounds/sky.gbapal.lz");

const u32 gBattleAnimBgTilemap_Drill[] = INCBIN_U32("graphics/battle_anims/backgrounds/drill.bin.lz");
const u32 gBattleAnimBgTilemap_DrillContests[] = INCBIN_U32("graphics/battle_anims/backgrounds/drill_contests.bin.lz");

const u32 gBattleAnimBgImage_Aurora[] = INCBIN_U32("graphics/battle_anims/backgrounds/aurora.4bpp.lz");
const u32 gBattleAnimBgPalette_Aurora[] = INCBIN_U32("graphics/battle_anims/backgrounds/aurora.gbapal.lz");
const u32 gBattleAnimBgTilemap_Aurora[] = INCBIN_U32("graphics/battle_anims/backgrounds/aurora.bin.lz");

const u32 gBattleAnimBgTilemap_HighspeedOpponent[] = INCBIN_U32("graphics/battle_anims/backgrounds/highspeed_opponent.bin.lz");
const u32 gBattleAnimBgPalette_Highspeed[] = INCBIN_U32("graphics/battle_anims/backgrounds/highspeed.gbapal.lz");

const u32 gBattleAnimBgPalette_Bug[] = INCBIN_U32("graphics/battle_anims/backgrounds/bug.gbapal.lz");

const u32 gBattleAnimBgImage_Highspeed[] = INCBIN_U32("graphics/battle_anims/backgrounds/highspeed.4bpp.lz");
const u32 gBattleAnimBgTilemap_HighspeedPlayer[] = INCBIN_U32("graphics/battle_anims/backgrounds/highspeed_player.bin.lz");

const u32 gBattleAnimMaskImage_LightBeam[] = INCBIN_U32("graphics/battle_anims/masks/light_beam.4bpp.lz");
const u32 gBattleAnimMaskPalette_LightBeam[] = INCBIN_U32("graphics/battle_anims/masks/light_beam.gbapal.lz");
const u32 gBattleAnimMaskTilemap_LightBeam[] = INCBIN_U32("graphics/battle_anims/masks/light_beam.bin.lz");

const u32 gBattleAnimBgTilemap_GuillotineOpponent[] = INCBIN_U32("graphics/battle_anims/backgrounds/guillotine_opponent.bin.lz");
const u32 gBattleAnimBgTilemap_GuillotinePlayer[] = INCBIN_U32("graphics/battle_anims/backgrounds/guillotine_player.bin.lz");
const u32 gBattleAnimBgTilemap_GuillotineContests[] = INCBIN_U32("graphics/battle_anims/backgrounds/guillotine_contests.bin.lz");

const u32 gBattleAnimBgImage_Guillotine[] = INCBIN_U32("graphics/battle_anims/backgrounds/guillotine.4bpp.lz");
const u32 gBattleAnimBgPalette_Guillotine[] = INCBIN_U32("graphics/battle_anims/backgrounds/guillotine.gbapal.lz");

const u32 gBattleAnimBgImage_Thunder[] = INCBIN_U32("graphics/battle_anims/backgrounds/thunder.4bpp.lz");
const u32 gBattleAnimBgPalette_Thunder[] = INCBIN_U32("graphics/battle_anims/backgrounds/thunder.gbapal.lz");
const u32 gBattleAnimBgTilemap_Thunder[] = INCBIN_U32("graphics/battle_anims/backgrounds/thunder.bin.lz");

const u32 gBattleAnimSpriteGfx_PainSplit[] = INCBIN_U32("graphics/battle_anims/sprites/pain_split.4bpp.lz");
const u32 gBattleAnimSpritePal_PainSplit[] = INCBIN_U32("graphics/battle_anims/sprites/pain_split.gbapal.lz");

const u32 gBattleAnimSpriteGfx_HandsAndFeet[] = INCBIN_U32("graphics/battle_anims/sprites/hands_and_feet.4bpp.lz");
const u32 gBattleAnimSpritePal_HandsAndFeet[] = INCBIN_U32("graphics/battle_anims/sprites/hands_and_feet.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Confetti[] = INCBIN_U32("graphics/battle_anims/sprites/confetti.4bpp.lz");
const u32 gBattleAnimSpritePal_Confetti[] = INCBIN_U32("graphics/battle_anims/sprites/confetti.gbapal.lz");

const u32 gSubstituteDollPal[] = INCBIN_U32("graphics/battle_anims/sprites/substitute.gbapal.lz");
const u32 gSubstituteDollFrontGfx[] = INCBIN_U32("graphics/battle_anims/sprites/substitute.4bpp.lz");
const u32 gSubstituteDollBackGfx[] = INCBIN_U32("graphics/battle_anims/sprites/substitute_back.4bpp.lz");

const u32 gBattleAnimSpriteGfx_GreenStar[] = INCBIN_U32("graphics/battle_anims/sprites/green_star.4bpp.lz");
const u32 gBattleAnimSpritePal_GreenStar[] = INCBIN_U32("graphics/battle_anims/sprites/green_star.gbapal.lz");

const u32 gConfetti_Gfx[] = INCBIN_U32("graphics/misc/confetti.4bpp.lz");
const u32 gConfetti_Pal[] = INCBIN_U32("graphics/misc/confetti.gbapal.lz");

const u32 gBattleAnimSpriteGfx_PinkCloud[] = INCBIN_U32("graphics/battle_anims/sprites/pink_cloud.4bpp.lz");
const u32 gBattleAnimSpritePal_PinkCloud[] = INCBIN_U32("graphics/battle_anims/sprites/pink_cloud.gbapal.lz");

const u32 gBattleAnimSpriteGfx_SweatDrop[] = INCBIN_U32("graphics/battle_anims/sprites/sweat_drop.4bpp.lz");
const u32 gBattleAnimSpritePal_SweatDrop[] = INCBIN_U32("graphics/battle_anims/sprites/sweat_drop.gbapal.lz");

const u32 gStatAnim_Gfx[]              = INCBIN_U32("graphics/battle_anims/stat_change/tiles.4bpp.lz");
const u32 gStatAnim_Increase_Tilemap[] = INCBIN_U32("graphics/battle_anims/stat_change/increase.bin.lz");
const u32 gStatAnim_Decrease_Tilemap[] = INCBIN_U32("graphics/battle_anims/stat_change/decrease.bin.lz");
const u32 gStatAnim_Defense_Pal[]      = INCBIN_U32("graphics/battle_anims/stat_change/defense.gbapal.lz");
const u32 gStatAnim_Attack_Pal[]       = INCBIN_U32("graphics/battle_anims/stat_change/attack.gbapal.lz");
const u32 gStatAnim_Accuracy_Pal[]     = INCBIN_U32("graphics/battle_anims/stat_change/accuracy.gbapal.lz");
const u32 gStatAnim_Speed_Pal[]        = INCBIN_U32("graphics/battle_anims/stat_change/speed.gbapal.lz");
const u32 gStatAnim_Multiple_Pal[]     = INCBIN_U32("graphics/battle_anims/stat_change/multiple.gbapal.lz");
const u32 gStatAnim_Evasion_Pal[]      = INCBIN_U32("graphics/battle_anims/stat_change/evasion.gbapal.lz");
const u32 gStatAnim_SpAttack_Pal[]     = INCBIN_U32("graphics/battle_anims/stat_change/sp_attack.gbapal.lz");
const u32 gStatAnim_SpDefense_Pal[]    = INCBIN_U32("graphics/battle_anims/stat_change/sp_defense.gbapal.lz");

const u32 gCureBubblesGfx[] = INCBIN_U32("graphics/battle_anims/masks/cure_bubbles.4bpp.lz");
const u32 gCureBubblesPal[] = INCBIN_U32("graphics/battle_anims/masks/cure_bubbles.gbapal.lz");
const u32 gCureBubblesTilemap[] = INCBIN_U32("graphics/battle_anims/masks/cure_bubbles.bin.lz");

const u32 gBattleAnimSpritePal_PurpleScratch[] = INCBIN_U32("graphics/battle_anims/sprites/purple_scratch.gbapal.lz");
const u32 gBattleAnimSpriteGfx_PurpleScratch[] = INCBIN_U32("graphics/battle_anims/sprites/purple_scratch.4bpp.lz");

const u32 gBattleAnimSpriteGfx_PurpleSwipe[] = INCBIN_U32("graphics/battle_anims/sprites/purple_swipe.4bpp.lz");

const u32 gBattleAnimSpriteGfx_GuardRing[] = INCBIN_U32("graphics/battle_anims/sprites/guard_ring.4bpp.lz");
const u32 gBattleAnimSpritePal_GuardRing[] = INCBIN_U32("graphics/battle_anims/sprites/guard_ring.gbapal.lz");

const u32 gBattleAnimSpriteGfx_TagHand[] = INCBIN_U32("graphics/battle_anims/sprites/tag_hand.4bpp.lz");

const u32 gBattleAnimSpriteGfx_NoiseLine[] = INCBIN_U32("graphics/battle_anims/sprites/noise_line.4bpp.lz");

const u32 UnusedLevelupAnimationGfx[] = INCBIN_U32("graphics/battle_anims/masks/unused_level_up.4bpp.lz");
const u32 UnusedLevelupAnimationTilemap[] = INCBIN_U32("graphics/battle_anims/masks/unused_level_up.bin.lz");

const u32 gBattleAnimSpriteGfx_SmallRedEye[] = INCBIN_U32("graphics/battle_anims/sprites/small_red_eye.4bpp.lz");
const u32 gBattleAnimSpritePal_SmallRedEye[] = INCBIN_U32("graphics/battle_anims/sprites/small_red_eye.gbapal.lz");

const u32 gBattleAnimSpriteGfx_HollowOrb[] = INCBIN_U32("graphics/battle_anims/sprites/hollow_orb.4bpp.lz");
const u32 gBattleAnimSpritePal_HollowOrb[] = INCBIN_U32("graphics/battle_anims/sprites/hollow_orb.gbapal.lz");

const u32 gBattleAnimSpriteGfx_XSign[] = INCBIN_U32("graphics/battle_anims/sprites/x_sign.4bpp.lz");

const u32 gBattleAnimSpriteGfx_BluegreenOrb[] = INCBIN_U32("graphics/battle_anims/sprites/bluegreen_orb.4bpp.lz");
const u32 gBattleAnimSpritePal_BluegreenOrb[] = INCBIN_U32("graphics/battle_anims/sprites/bluegreen_orb.gbapal.lz");

const u32 gBattleAnimSpriteGfx_PawPrint[] = INCBIN_U32("graphics/battle_anims/sprites/paw_print.4bpp.lz");
const u32 gBattleAnimSpritePal_PawPrint[] = INCBIN_U32("graphics/battle_anims/sprites/paw_print.gbapal.lz");

const u32 gBattleAnimSpriteGfx_PurpleFlame[] = INCBIN_U32("graphics/battle_anims/sprites/purple_flame.4bpp.lz");
const u32 gBattleAnimSpritePal_PurpleFlame[] = INCBIN_U32("graphics/battle_anims/sprites/purple_flame.gbapal.lz");

const u32 gBattleAnimSpriteGfx_RedBall[] = INCBIN_U32("graphics/battle_anims/sprites/red_ball.4bpp.lz");
const u32 gBattleAnimSpritePal_RedBall[] = INCBIN_U32("graphics/battle_anims/sprites/red_ball.gbapal.lz");

const u32 gBattleAnimSpriteGfx_SmellingsaltEffect[] = INCBIN_U32("graphics/battle_anims/sprites/smellingsalt_effect.4bpp.lz");
const u32 gBattleAnimSpritePal_SmellingsaltEffect[] = INCBIN_U32("graphics/battle_anims/sprites/smellingsalt_effect.gbapal.lz");

const u32 gBattleAnimSpriteGfx_MagnifyingGlass[] = INCBIN_U32("graphics/battle_anims/sprites/magnifying_glass.4bpp.lz");
const u32 gBattleAnimSpritePal_MagnifyingGlass[] = INCBIN_U32("graphics/battle_anims/sprites/magnifying_glass.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Meteor[] = INCBIN_U32("graphics/battle_anims/sprites/meteor.4bpp.lz");
const u32 gBattleAnimSpritePal_Meteor[] = INCBIN_U32("graphics/battle_anims/sprites/meteor.gbapal.lz");

const u32 gBattleAnimSpriteGfx_FlatRock[] = INCBIN_U32("graphics/battle_anims/sprites/flat_rock.4bpp.lz");
const u32 gBattleAnimSpritePal_FlatRock[] = INCBIN_U32("graphics/battle_anims/sprites/flat_rock.gbapal.lz");

const u32 gBattleAnimUnusedPal_Unknown2[] = INCBIN_U32("graphics/battle_anims/unused/unknown_2.gbapal.lz");

#include "data/graphics/pokemon.h"
#include "data/graphics/trainers.h"

const u8 gMonIcon_QuestionMark[] = INCBIN_U8("graphics/pokemon/question_mark/icon.4bpp");
const u8 gMonFootprint_QuestionMark[] = INCBIN_U8("graphics/pokemon/question_mark/footprint.1bpp");

const u32 gBattleVSFrame_Gfx[] = INCBIN_U32("graphics/battle_transitions/vs_frame.4bpp.lz");
const u32 gBattleVSFrame_Tilemap[] = INCBIN_U32("graphics/battle_transitions/vs_frame.bin.lz");
const u32 gBattleVSFrame_Pal[] = INCBIN_U32("graphics/battle_transitions/vs_frame.gbapal.lz");

const u32 gVsLettersGfx[] = INCBIN_U32("graphics/battle_transitions/vs.4bpp.lz");

#include "data/graphics/battle_environment.h"

// Battle Dome
const u32 gDomeTourneyTree_Gfx[] = INCBIN_U32("graphics/battle_frontier/tourney_tree.4bpp.lz");
const u32 gDomeTourneyLine_Gfx[] = INCBIN_U32("graphics/battle_frontier/tourney_line.4bpp.lz"); // the red glow mask for the tourney advancement lines
const u32 gDomeTourneyTree_Tilemap[] = INCBIN_U32("graphics/battle_frontier/tourney_tree.bin.lz");
const u32 gDomeTourneyLineDown_Tilemap[] = INCBIN_U32("graphics/battle_frontier/tourney_line_down_map.bin.lz");
const u32 gDomeTourneyLineUp_Tilemap[] = INCBIN_U32("graphics/battle_frontier/tourney_line_up_map.bin.lz");
const u32 gDomeTourneyInfoCard_Gfx[] = INCBIN_U32("graphics/battle_frontier/tourney_info_card.4bpp.lz");
const u32 gDomeTourneyInfoCard_Tilemap[] = INCBIN_U32("graphics/battle_frontier/tourney_info_card_tilemap.bin.lz");
const u32 gDomeTourneyInfoCardBg_Tilemap[] = INCBIN_U32("graphics/battle_frontier/tourney_info_card_bg.bin.lz");
const u32 gDomeTourneyTreeButtons_Gfx[] = INCBIN_U32("graphics/battle_frontier/tourney_buttons.4bpp.lz"); // exit/cancel and Poké Ball buttons
const u32 gDomeTourneyTree_Pal[] = INCBIN_U32("graphics/battle_frontier/tourney_tree.gbapal.lz");
const u32 gDomeTourneyTreeButtons_Pal[] = INCBIN_U32("graphics/battle_frontier/tourney_buttons.gbapal.lz");
const u32 gDomeTourneyMatchCardBg_Pal[] = INCBIN_U32("graphics/battle_frontier/tourney_match_card_bg.gbapal.lz");

const u32 gBattleArenaJudgmentSymbolsGfx[] = INCBIN_U32("graphics/battle_frontier/arena_judgment_symbols.4bpp.lz");
const u32 gBattleArenaJudgmentSymbolsPalette[] = INCBIN_U32("graphics/battle_frontier/arena_judgment_symbols.gbapal.lz");

const u32 gBattleWindowTextPalette[] = INCBIN_U32("graphics/battle_interface/text.gbapal.lz");
const u16 gPPTextPalette[] = INCBIN_U16("graphics/battle_interface/text_pp.gbapal");

const u16 gTilesetAnims_BattleDomePals0_0[] = INCBIN_U16("graphics/battle_frontier/dome_anim1.gbapal");
const u16 gTilesetAnims_BattleDomePals0_1[] = INCBIN_U16("graphics/battle_frontier/dome_anim2.gbapal");
const u16 gTilesetAnims_BattleDomePals0_2[] = INCBIN_U16("graphics/battle_frontier/dome_anim3.gbapal");
const u16 gTilesetAnims_BattleDomePals0_3[] = INCBIN_U16("graphics/battle_frontier/dome_anim4.gbapal");

static const u16 sUnused0[] = {0x13F, 0x119, 0x113, 0x10E};

const u16 gBattlePyramidFloor_Pal[] = INCBIN_U16("graphics/battle_frontier/pyramid_floor.gbapal");

const u32 gMultiBattleIntroBg_Opponent_Tilemap[] = INCBIN_U32("graphics/battle_frontier/multi_battle_intro_bg_opponent.bin.lz");
const u32 gMultiBattleIntroBg_Player_Tilemap[] = INCBIN_U32("graphics/battle_frontier/multi_battle_intro_bg_player.bin.lz");

#include "data/graphics/intro_scene.h"

const u32 gBattleAnimSpriteGfx_FlyingDirt[] = INCBIN_U32("graphics/battle_anims/sprites/flying_dirt.4bpp.lz");

const u32 gBattleAnimBgTilemap_Sandstorm[] = INCBIN_U32("graphics/battle_anims/backgrounds/sandstorm_brew.bin.lz");
const u32 gBattleAnimBgImage_Sandstorm[] = INCBIN_U32("graphics/battle_anims/backgrounds/sandstorm_brew.4bpp.lz");

const u32 gBattleAnimSpritePal_FlyingDirt[] = INCBIN_U32("graphics/battle_anims/sprites/flying_dirt.gbapal.lz");

const u32 gBattleAnimSpriteGfx_MetalSoundWaves[] = INCBIN_U32("graphics/battle_anims/sprites/metal_sound_waves.4bpp.lz");
const u32 gBattleAnimSpritePal_MetalSoundWaves[] = INCBIN_U32("graphics/battle_anims/sprites/metal_sound_waves.gbapal.lz");

const u32 gBattleAnimBgImage_Ice[] = INCBIN_U32("graphics/battle_anims/backgrounds/ice.4bpp.lz");
const u32 gBattleAnimBgPalette_Ice[] = INCBIN_U32("graphics/battle_anims/backgrounds/ice.gbapal.lz");
const u32 gBattleAnimBgTilemap_Ice[] = INCBIN_U32("graphics/battle_anims/backgrounds/ice.bin.lz");

const u32 gBattleAnimSpriteGfx_IcicleSpear[] = INCBIN_U32("graphics/battle_anims/sprites/icicle_spear.4bpp.lz");
const u32 gBattleAnimSpritePal_IcicleSpear[] = INCBIN_U32("graphics/battle_anims/sprites/icicle_spear.gbapal.lz");

const u32 gContestNextTurnGfx[] = INCBIN_U32("graphics/contest/nextturn.4bpp.lz");
const u8 gContestNextTurnNumbersGfx[] = INCBIN_U8("graphics/contest/nextturn_numbers.4bpp");
const u8 gContestNextTurnRandomGfx[] = INCBIN_U8("graphics/contest/nextturn_random.4bpp");

const u32 gBattleAnimSpriteGfx_GlowyRedOrb[] = INCBIN_U32("graphics/battle_anims/sprites/glowy_red_orb.4bpp.lz");
const u32 gBattleAnimSpritePal_GlowyRedOrb[] = INCBIN_U32("graphics/battle_anims/sprites/glowy_red_orb.gbapal.lz");
const u32 gBattleAnimSpritePal_GlowyGreenOrb[] = INCBIN_U32("graphics/battle_anims/sprites/glowy_green_orb.gbapal.lz");
const u32 gBattleAnimSpritePal_SleepPowder[] = INCBIN_U32("graphics/battle_anims/sprites/sleep_powder.gbapal.lz");
const u32 gBattleAnimSpritePal_StunSpore[] = INCBIN_U32("graphics/battle_anims/sprites/stun_spore.gbapal.lz");

const u32 gContestApplauseGfx[] = INCBIN_U32("graphics/contest/applause.4bpp.lz");
const u8 gContestApplauseMeterGfx[] = INCBIN_U8("graphics/contest/applause_meter.4bpp");

const u16 gContestPal[] = INCBIN_U16("graphics/contest/nextturn.gbapal");

const u32 gBattleAnimSpriteGfx_Splash[] = INCBIN_U32("graphics/battle_anims/sprites/splash.4bpp.lz");
const u32 gBattleAnimSpritePal_Splash[] = INCBIN_U32("graphics/battle_anims/sprites/splash.gbapal.lz");

const u32 gBattleAnimSpriteGfx_SweatBead[] = INCBIN_U32("graphics/battle_anims/sprites/sweat_bead.4bpp.lz");

const u32 gBattleAnimSpriteGfx_Pokeblock[] = INCBIN_U32("graphics/battle_anims/sprites/pokeblock.4bpp.lz");
const u32 gBattleAnimSpritePal_Pokeblock[] = INCBIN_U32("graphics/battle_anims/sprites/pokeblock.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Gem1[] = INCBIN_U32("graphics/battle_anims/sprites/gem_1.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Gem2[] = INCBIN_U32("graphics/battle_anims/sprites/gem_2.4bpp.lz");
const u32 gBattleAnimSpriteGfx_Gem3[] = INCBIN_U32("graphics/battle_anims/sprites/gem_3.4bpp.lz");

const u32 gBattleAnimSpritePal_Gem1[] = INCBIN_U32("graphics/battle_anims/sprites/gem_1.gbapal.lz");

const u32 gBattleAnimBgImage_InAir[] = INCBIN_U32("graphics/battle_anims/backgrounds/in_air.4bpp.lz");
const u32 gBattleAnimBgPalette_InAir[] = INCBIN_U32("graphics/battle_anims/backgrounds/in_air.gbapal.lz");
const u32 gBattleAnimBgTilemap_InAir[] = INCBIN_U32("graphics/battle_anims/backgrounds/in_air.bin.lz");

const u32 gBattleAnimSpriteGfx_Protect[] = INCBIN_U32("graphics/battle_anims/sprites/protect.4bpp.lz");
const u32 gBattleAnimSpritePal_Protect[] = INCBIN_U32("graphics/battle_anims/sprites/protect.gbapal.lz");

const u32 gBattleAnimBackgroundImageMuddyWater_Pal[] = INCBIN_U32("graphics/battle_anims/backgrounds/water_muddy.gbapal.lz");

const u32 gEnemyMonShadow_Gfx[] = INCBIN_U32("graphics/battle_interface/enemy_mon_shadow.4bpp.lz");

const u32 gBattleInterface_BallStatusBarGfx[] = INCBIN_U32("graphics/battle_interface/ball_status_bar.4bpp.lz");

const u8 gMonIcon_Egg[] = INCBIN_U8("graphics/pokemon/egg/icon.4bpp");

const u32 gBattleAnimBgImage_Ghost[] = INCBIN_U32("graphics/battle_anims/backgrounds/ghost.4bpp.lz");
const u32 gBattleAnimBgPalette_Ghost[] = INCBIN_U32("graphics/battle_anims/backgrounds/ghost.gbapal.lz");
const u32 gBattleAnimBgTilemap_Ghost[] = INCBIN_U32("graphics/battle_anims/backgrounds/ghost.bin.lz");

const u32 gBattleAnimSpritePal_WhipHit[] = INCBIN_U32("graphics/battle_anims/sprites/whip_hit.gbapal.lz");

const u32 gBattleAnimBgPalette_SolarBeam[] = INCBIN_U32("graphics/battle_anims/backgrounds/solarbeam.gbapal.lz");
const u32 gBattleAnimBgTilemap_SolarBeam[] = INCBIN_U32("graphics/battle_anims/backgrounds/solarbeam.bin.lz"); // Unused

const u32 gBerryBlenderCenter_Gfx[] = INCBIN_U32("graphics/berry_blender/center.8bpp.lz");
const u32 gBerryBlenderOuter_Gfx[] = INCBIN_U32("graphics/berry_blender/outer.4bpp.lz");
const u32 gBerryBlenderOuter_Tilemap[] = INCBIN_U32("graphics/berry_blender/outer_map.bin.lz");

const u32 gBattleAnimBgPalette_Cosmic[] = INCBIN_U32("graphics/battle_anims/backgrounds/cosmic.gbapal.lz");
const u32 gBattleAnimBgImage_Cosmic[] = INCBIN_U32("graphics/battle_anims/backgrounds/cosmic.4bpp.lz");
const u32 gBattleAnimBgTilemap_Cosmic[] = INCBIN_U32("graphics/battle_anims/backgrounds/cosmic.bin.lz");

const u32 gBattleAnimSpritePal_SlamHit2[] = INCBIN_U32("graphics/battle_anims/sprites/slam_hit_2.gbapal.lz");
const u32 gBattleAnimSpriteGfx_SlamHit2[] = INCBIN_U32("graphics/battle_anims/sprites/slam_hit_2.4bpp.lz");

const u32 gBattleAnimFogTilemap[] = INCBIN_U32("graphics/battle_anims/backgrounds/fog.bin.lz");

const u32 gBattleAnimSpritePal_WeatherBall[] = INCBIN_U32("graphics/battle_anims/sprites/weather_ball.gbapal.lz");
const u32 gBattleAnimSpriteGfx_WeatherBall[] = INCBIN_U32("graphics/battle_anims/sprites/weather_ball.4bpp.lz");

const u32 gBattleAnimBgTilemap_ScaryFacePlayer[] = INCBIN_U32("graphics/battle_anims/backgrounds/scary_face_player.bin.lz");
const u32 gBattleAnimBgTilemap_ScaryFaceOpponent[] = INCBIN_U32("graphics/battle_anims/backgrounds/scary_face_opponent.bin.lz");
const u32 gBattleAnimBgTilemap_ScaryFaceContest[] = INCBIN_U32("graphics/battle_anims/backgrounds/scary_face_contest.bin.lz");

const u32 gBattleAnimSpriteGfx_Hail[] = INCBIN_U32("graphics/battle_anims/sprites/hail.4bpp.lz");
const u32 gBattleAnimSpritePal_Hail[] = INCBIN_U32("graphics/battle_anims/sprites/hail.gbapal.lz");

const u32 gBattleAnimSpriteGfx_GreenSpike[] = INCBIN_U32("graphics/battle_anims/sprites/green_spike.4bpp.lz");
const u32 gBattleAnimSpritePal_GreenSpike[] = INCBIN_U32("graphics/battle_anims/sprites/green_spike.gbapal.lz");
const u32 gBattleAnimSpritePal_WhiteCircleOfLight[] = INCBIN_U32("graphics/battle_anims/sprites/white_circle_of_light.gbapal.lz");
const u32 gBattleAnimSpritePal_GlowyBlueOrb[] = INCBIN_U32("graphics/battle_anims/sprites/glowy_blue_orb.gbapal.lz");

const u32 gBattleAnimSpriteGfx_Recycle[] = INCBIN_U32("graphics/battle_anims/sprites/recycle.4bpp.lz");
const u32 gBattleAnimSpritePal_Recycle[] = INCBIN_U32("graphics/battle_anims/sprites/recycle.gbapal.lz");

const u32 gBattleAnimSpriteGfx_RedParticles[] = INCBIN_U32("graphics/battle_anims/sprites/red_particles.4bpp.lz");
const u32 gBattleAnimSpritePal_RedParticles[] = INCBIN_U32("graphics/battle_anims/sprites/red_particles.gbapal.lz");

const u32 gBattleAnimSpriteGfx_DirtMound[] = INCBIN_U32("graphics/battle_anims/sprites/dirt_mound.4bpp.lz");
const u32 gBattleAnimSpritePal_DirtMound[] = INCBIN_U32("graphics/battle_anims/sprites/dirt_mound.gbapal.lz");

const u32 gBattleAnimBgImage_Fissure[] = INCBIN_U32("graphics/battle_anims/backgrounds/fissure.4bpp.lz");
const u32 gBattleAnimBgPalette_Fissure[] = INCBIN_U32("graphics/battle_anims/backgrounds/fissure.gbapal.lz");
const u32 gBattleAnimBgTilemap_Fissure[] = INCBIN_U32("graphics/battle_anims/backgrounds/fissure.bin.lz");

const u32 gBattleAnimSpriteGfx_Bird[] = INCBIN_U32("graphics/battle_anims/sprites/bird.4bpp.lz");
const u32 gBattleAnimSpritePal_Bird[] = INCBIN_U32("graphics/battle_anims/sprites/bird.gbapal.lz");

const u32 gBattleAnimSpriteGfx_CrossImpact[] = INCBIN_U32("graphics/battle_anims/sprites/cross_impact.4bpp.lz");
const u32 gBattleAnimSpritePal_CrossImpact[] = INCBIN_U32("graphics/battle_anims/sprites/cross_impact.gbapal.lz");

const u32 gBattleAnimBgImage_Surf[] = INCBIN_U32("graphics/battle_anims/backgrounds/water.4bpp.lz");
const u32 gBattleAnimBgPalette_Surf[] = INCBIN_U32("graphics/battle_anims/backgrounds/water.gbapal.lz");

const u32 gBattleAnimBgTilemap_SurfOpponent[] = INCBIN_U32("graphics/battle_anims/backgrounds/water_opponent.bin.lz");
const u32 gBattleAnimBgTilemap_SurfPlayer[] = INCBIN_U32("graphics/battle_anims/backgrounds/water_player.bin.lz");
const u32 gBattleAnimBgTilemap_SurfContest[] = INCBIN_U32("graphics/battle_anims/backgrounds/water_contest.bin.lz");

const u32 gBattleAnimSpritePal_Slash2[] = INCBIN_U32("graphics/battle_anims/sprites/slash_2.gbapal.lz");

const u32 gBattleAnimSpriteGfx_WhiteShadow[] = INCBIN_U32("graphics/battle_anims/sprites/white_shadow.4bpp.lz");
const u32 gBattleAnimSpritePal_WhiteShadow[] = INCBIN_U32("graphics/battle_anims/sprites/white_shadow.gbapal.lz");

const u32 gPartyMenuBg_Gfx[] = INCBIN_U32("graphics/party_menu/bg.4bpp.lz");
const u32 gPartyMenuBg_Pal[] = INCBIN_U32("graphics/party_menu/bg.gbapal.lz");
const u32 gPartyMenuBg_Tilemap[] = INCBIN_U32("graphics/party_menu/bg.bin.lz");

const u32 gPartyMenuPokeball_Gfx[] = INCBIN_U32("graphics/party_menu/pokeball.4bpp.lz");
const u32 gPartyMenuPokeballSmall_Gfx[] = INCBIN_U32("graphics/party_menu/pokeball_small.4bpp.lz"); //unused
const u32 gPartyMenuPokeball_Pal[] = INCBIN_U32("graphics/party_menu/pokeball.gbapal.lz");

const u32 gStatusGfx_Icons[] = INCBIN_U32("graphics/interface/status_icons.4bpp.lz");
const u32 gStatusPal_Icons[] = INCBIN_U32("graphics/interface/status_icons.gbapal.lz");

const u32 gMoveTypes_Gfx[] = INCBIN_U32("graphics/types/move_types.4bpp.lz");
const u32 gMoveTypes_Pal[] = INCBIN_U32("graphics/types/move_types.gbapal.lz");

const u32 gSummaryMoveSelect_Gfx[] = INCBIN_U32("graphics/summary_screen/move_select.4bpp.lz");
const u32 gSummaryMoveSelect_Pal[] = INCBIN_U32("graphics/summary_screen/move_select.gbapal.lz");

const u32 gSummaryScreen_Gfx[]                = INCBIN_U32("graphics/summary_screen/tiles.4bpp.lz");
const u32 gSummaryScreen_Pal[]                = INCBIN_U32("graphics/summary_screen/tiles.gbapal.lz");
const u32 gSummaryPage_Info_Tilemap[]         = INCBIN_U32("graphics/summary_screen/page_info.bin.lz");
const u32 gSummaryPage_Skills_Tilemap[]       = INCBIN_U32("graphics/summary_screen/page_skills.bin.lz");
const u32 gSummaryPage_BattleMoves_Tilemap[]  = INCBIN_U32("graphics/summary_screen/page_battle_moves.bin.lz");
const u32 gSummaryPage_ContestMoves_Tilemap[] = INCBIN_U32("graphics/summary_screen/page_contest_moves.bin.lz");
const u32 gSummaryPage_InfoEgg_Tilemap[]      = INCBIN_U32("graphics/summary_screen/page_info_egg.bin.lz");

const u32 gBagMaleTiles[] = INCBIN_U32("graphics/bag/bag_male.4bpp.lz");
const u32 gBagFemaleTiles[] = INCBIN_U32("graphics/bag/bag_female.4bpp.lz");
const u32 gBagPalette[] = INCBIN_U32("graphics/bag/bag.gbapal.lz");

const u32 gBagScreenMale_Pal[] = INCBIN_U32("graphics/bag/menu_male.gbapal.lz");
const u32 gBagScreenFemale_Pal[] = INCBIN_U32("graphics/bag/menu_female.gbapal.lz");

const u32 gBagScreen_Gfx[] = INCBIN_U32("graphics/bag/menu.4bpp.lz");
const u32 gBagScreen_GfxTileMap[] = INCBIN_U32("graphics/bag/menu.bin.lz");

const u32 gBattlePyramidBag_Gfx[]          = INCBIN_U32("graphics/bag/bag_pyramid.4bpp.lz");
const u32 gBattlePyramidBag_Pal[]          = INCBIN_U32("graphics/bag/bag_pyramid.gbapal.lz"); // female palette is first and male is second.
const u32 gBattlePyramidBagTilemap[]       = INCBIN_U32("graphics/bag/menu_pyramid.bin.lz");
const u32 gBattlePyramidBagInterface_Pal[] = INCBIN_U32("graphics/bag/menu_pyramid.gbapal.lz");

const u32 gSwapLineGfx[] = INCBIN_U32("graphics/interface/swap_line.4bpp.lz");
const u32 gSwapLinePal[] = INCBIN_U32("graphics/interface/swap_line.gbapal.lz");

const u32 gShopMenu_Gfx[] = INCBIN_U32("graphics/shop/menu.4bpp.lz");
const u32 gShopMenu_Pal[] = INCBIN_U32("graphics/shop/menu.gbapal.lz");
const u32 gShopMenu_Tilemap[] = INCBIN_U32("graphics/shop/menu.bin.lz");
const u32 gShopMenuMoney_Gfx[] = INCBIN_U32("graphics/shop/money.4bpp.lz");

// Pokeblock

const u32 gMenuPokeblock_Gfx[] = INCBIN_U32("graphics/pokeblock/menu.4bpp.lz");
const u32 gMenuPokeblock_Pal[] = INCBIN_U32("graphics/pokeblock/menu.gbapal.lz");

const u32 gMenuPokeblockDevice_Gfx[] = INCBIN_U32("graphics/pokeblock/device.4bpp.lz");
const u32 gMenuPokeblockDevice_Pal[] = INCBIN_U32("graphics/pokeblock/device.gbapal.lz");

const u32 gMenuPokeblock_Tilemap[] = INCBIN_U32("graphics/pokeblock/menu.bin.lz");

const u32 gPokeblock_Gfx[] = INCBIN_U32("graphics/pokeblock/pokeblock.4bpp.lz");
const u32 gPokeblockRed_Pal[] = INCBIN_U32("graphics/pokeblock/red.gbapal.lz");
const u32 gPokeblockBlue_Pal[] = INCBIN_U32("graphics/pokeblock/blue.gbapal.lz");
const u32 gPokeblockPink_Pal[] = INCBIN_U32("graphics/pokeblock/pink.gbapal.lz");
const u32 gPokeblockGreen_Pal[] = INCBIN_U32("graphics/pokeblock/green.gbapal.lz");
const u32 gPokeblockYellow_Pal[] = INCBIN_U32("graphics/pokeblock/yellow.gbapal.lz");
const u32 gPokeblockPurple_Pal[] = INCBIN_U32("graphics/pokeblock/purple.gbapal.lz");
const u32 gPokeblockIndigo_Pal[] = INCBIN_U32("graphics/pokeblock/indigo.gbapal.lz");
const u32 gPokeblockBrown_Pal[] = INCBIN_U32("graphics/pokeblock/brown.gbapal.lz");
const u32 gPokeblockLiteBlue_Pal[] = INCBIN_U32("graphics/pokeblock/liteblue.gbapal.lz");
const u32 gPokeblockOlive_Pal[] = INCBIN_U32("graphics/pokeblock/olive.gbapal.lz");
const u32 gPokeblockGray_Pal[] = INCBIN_U32("graphics/pokeblock/gray.gbapal.lz");
const u32 gPokeblockBlack_Pal[] = INCBIN_U32("graphics/pokeblock/black.gbapal.lz");
const u32 gPokeblockWhite_Pal[] = INCBIN_U32("graphics/pokeblock/white.gbapal.lz");
const u32 gPokeblockGold_Pal[] = INCBIN_U32("graphics/pokeblock/gold.gbapal.lz");

const u32 gPokeblockFeedBg_Tilemap[] = INCBIN_U32("graphics/pokeblock/feeding_bg.bin.lz");

#include "data/graphics/berries.h"
#include "data/graphics/rayquaza_scene.h"
#include "data/graphics/items.h"
#include "data/graphics/decorations.h"

const u32 gBattleAnimSpritePal_Shock3[] = INCBIN_U32("graphics/battle_anims/sprites/shock_3.gbapal.lz");
const u32 gBattleAnimSpriteGfx_Shock3[] = INCBIN_U32("graphics/battle_anims/sprites/shock_3.4bpp.lz");

const u32 gBattleAnimSpritePal_WhiteFeather[] = INCBIN_U32("graphics/battle_anims/sprites/white_feather.gbapal.lz");
const u32 gBattleAnimSpriteGfx_WhiteFeather[] = INCBIN_U32("graphics/battle_anims/sprites/white_feather.4bpp.lz");

const u32 gBattleAnimSpritePal_Sparkle6[] = INCBIN_U32("graphics/battle_anims/sprites/sparkle_6.gbapal.lz");
const u32 gBattleAnimSpriteGfx_Sparkle6[] = INCBIN_U32("graphics/battle_anims/sprites/sparkle_6.4bpp.lz");

const u16 gCableCarBg_Pal[]    = INCBIN_U16("graphics/cable_car/bg.gbapal");
const u16 gCableCar_Pal[]      = INCBIN_U16("graphics/cable_car/cable_car.gbapal");
const u32 gCableCarBg_Gfx[]    = INCBIN_U32("graphics/cable_car/bg.4bpp.lz");
const u32 gCableCar_Gfx[]      = INCBIN_U32("graphics/cable_car/cable_car.4bpp.lz");
const u32 gCableCarDoor_Gfx[]  = INCBIN_U32("graphics/cable_car/door.4bpp.lz");
const u32 gCableCarCable_Gfx[] = INCBIN_U32("graphics/cable_car/cable.4bpp.lz");

// Roulette
const u32 gRouletteMenu_Gfx[] = INCBIN_U32("graphics/roulette/window.4bpp.lz");
const u32 gRouletteWheel_Gfx[] = INCBIN_U32("graphics/roulette/wheel.8bpp.lz");
const u32 gRouletteCenter_Gfx[] = INCBIN_U32("graphics/roulette/center.4bpp.lz");
const u32 gRouletteHeaders_Gfx[] = INCBIN_U32("graphics/roulette/headers.4bpp.lz");
const u32 gRouletteCredit_Gfx[] = INCBIN_U32("graphics/roulette/credit.4bpp.lz");
const u32 gRouletteNumbers_Gfx[] = INCBIN_U32("graphics/roulette/numbers.4bpp.lz");
const u32 gRouletteMultiplier_Gfx[] = INCBIN_U32("graphics/roulette/multiplier.4bpp.lz");

#include "data/graphics/mail.h"

const u16 gFrontierFactoryMenu_Pal[] = INCBIN_U16("graphics/battle_frontier/factory_screen/menu.gbapal");
const u16 gFrontierFactoryMenu_Gfx[] = INCBIN_U16("graphics/battle_frontier/factory_screen/menu.4bpp");
const u16 gFrontierFactoryMenu_Tilemap[] = INCBIN_U16("graphics/battle_frontier/factory_screen/menu.bin");

const u32 gFrontierPassMedals_Gfx[] = INCBIN_U32("graphics/frontier_pass/medals.4bpp.lz");

const u16 gFrontierPassCursor_Pal[] = INCBIN_U16("graphics/frontier_pass/cursor.gbapal");

const u16 gFrontierPassMapCursor_Pal[] = INCBIN_U16("graphics/frontier_pass/map_cursor.gbapal");

const u16 gFrontierPassMedalsSilver_Pal[] = INCBIN_U16("graphics/frontier_pass/silver.gbapal");
const u16 gFrontierPassMedalsGold_Pal[] = INCBIN_U16("graphics/frontier_pass/gold.gbapal");

// Pokédex
const u16 gPokedexBgHoenn_Pal[] = INCBIN_U16("graphics/pokedex/bg_hoenn.gbapal");
const u16 gPokedexSearchResults_Pal[] = INCBIN_U16("graphics/pokedex/search_results_bg.gbapal");
const u16 gPokedexBgNational_Pal[] = INCBIN_U16("graphics/pokedex/bg_national.gbapal");
const u32 gPokedexMenu_Gfx[] = INCBIN_U32("graphics/pokedex/menu.4bpp.lz");
const u32 gPokedexInterface_Gfx[] = INCBIN_U32("graphics/pokedex/interface.4bpp.lz");
const u32 gUnused_PokedexNoBall[] = INCBIN_U32("graphics/pokedex/noball_unused.4bpp.lz");
const u32 gPokedexStartMenuMain_Tilemap[] = INCBIN_U32("graphics/pokedex/start_menu_main.bin.lz");
const u32 gPokedexStartMenuSearchResults_Tilemap[] = INCBIN_U32("graphics/pokedex/start_menu_search_results.bin.lz");
const u32 gPokedexList_Tilemap[] = INCBIN_U32("graphics/pokedex/list.bin.lz");
const u32 gPokedexListUnderlay_Tilemap[] = INCBIN_U32("graphics/pokedex/list_underlay.bin.lz");
const u32 gPokedexSizeScreen_Tilemap[] = INCBIN_U32("graphics/pokedex/size_screen.bin.lz");
const u32 gPokedexScreenSelectBarMain_Tilemap[] = INCBIN_U32("graphics/pokedex/screen_select_bar_main.bin.lz");
const u32 gPokedexScreenSelectBarSubmenu_Tilemap[] = INCBIN_U32("graphics/pokedex/screen_select_bar_submenu.bin.lz");
const u32 gPokedexInfoScreen_Tilemap[] = INCBIN_U32("graphics/pokedex/info_screen.bin.lz");
const u32 gPokedexCryScreen_Tilemap[] = INCBIN_U32("graphics/pokedex/cry_screen.bin.lz");
const u16 gPokedexSearchMenu_Pal[] = INCBIN_U16("graphics/pokedex/search_menu.gbapal");
const u32 gPokedexSearchMenu_Gfx[] = INCBIN_U32("graphics/pokedex/search_menu.4bpp.lz");
const u32 gPokedexSearchMenuNational_Tilemap[] = INCBIN_U32("graphics/pokedex/search_menu_national.bin.lz");
const u32 gPokedexSearchMenuHoenn_Tilemap[] = INCBIN_U32("graphics/pokedex/search_menu_hoenn.bin.lz");

const u16 gSummaryScreen_MoveEffect_Battle_Tilemap[] = INCBIN_U16("graphics/summary_screen/effect_battle.bin");
const u16 gSummaryScreen_MoveEffect_Contest_Tilemap[] = INCBIN_U16("graphics/summary_screen/effect_contest.bin");
const u16 gSummaryScreen_MoveEffect_Cancel_Tilemap[] = INCBIN_U16("graphics/summary_screen/effect_cancel.bin");

const u16 gIntroCopyright_Pal[] = INCBIN_U16("graphics/intro/copyright.gbapal");
const u32 gIntroCopyright_Gfx[] = INCBIN_U32("graphics/intro/copyright.4bpp.lz");
const u32 gIntroCopyright_Tilemap[] = INCBIN_U32("graphics/intro/copyright.bin.lz");

const u16 gPokedexAreaScreenAreaUnknown_Pal[] = INCBIN_U16("graphics/pokedex/area_unknown.gbapal");
const u32 gPokedexAreaScreenAreaUnknown_Gfx[] = INCBIN_U32("graphics/pokedex/area_unknown.4bpp.lz");

const u16 gMenuInfoElements1_Pal[] = INCBIN_U16("graphics/interface/menu_info1.gbapal");
const u16 gMenuInfoElements2_Pal[] = INCBIN_U16("graphics/interface/menu_info2.gbapal");
const u16 gMenuInfoElements3_Pal[] = INCBIN_U16("graphics/interface/menu_info3.gbapal");
const u8 gMenuInfoElements_Gfx[] = INCBIN_U8("graphics/interface/menu_info.4bpp");

const u8 gBagMenuHMIcon_Gfx[] = INCBIN_U8("graphics/bag/hm.4bpp");

// contest results screen

const u16 gContestResultsTitle_Normal_Tilemap[] = INCBIN_U16("graphics/contest/results_screen/title_normal.bin");
const u16 gContestResultsTitle_Super_Tilemap[]  = INCBIN_U16("graphics/contest/results_screen/title_super.bin");
const u16 gContestResultsTitle_Hyper_Tilemap[]  = INCBIN_U16("graphics/contest/results_screen/title_hyper.bin");
const u16 gContestResultsTitle_Master_Tilemap[] = INCBIN_U16("graphics/contest/results_screen/title_master.bin");
const u16 gContestResultsTitle_Link_Tilemap[]   = INCBIN_U16("graphics/contest/results_screen/title_link.bin");
const u16 gContestResultsTitle_Cool_Tilemap[]   = INCBIN_U16("graphics/contest/results_screen/title_cool.bin");
const u16 gContestResultsTitle_Beauty_Tilemap[] = INCBIN_U16("graphics/contest/results_screen/title_beauty.bin");
const u16 gContestResultsTitle_Cute_Tilemap[]   = INCBIN_U16("graphics/contest/results_screen/title_cute.bin");
const u16 gContestResultsTitle_Smart_Tilemap[]  = INCBIN_U16("graphics/contest/results_screen/title_smart.bin");
const u16 gContestResultsTitle_Tough_Tilemap[]  = INCBIN_U16("graphics/contest/results_screen/title_tough.bin");
const u16 gContestResultsTitle_Tilemap[]        = INCBIN_U16("graphics/contest/results_screen/title.bin");

// PokéNav

const u16 gPokenavCondition_Pal[] = INCBIN_U16("graphics/pokenav/condition/graph.gbapal");
const u32 gPokenavCondition_Gfx[] = INCBIN_U32("graphics/pokenav/condition/graph.4bpp.lz");
const u32 gPokenavCondition_Tilemap[] = INCBIN_U32("graphics/pokenav/condition/graph.bin.lz");

const u16 gPokenavOptions_Tilemap[] = INCBIN_U16("graphics/pokenav/options/options.bin");
const u32 gPokenavOptions_Gfx[] = INCBIN_U32("graphics/pokenav/options/options.4bpp.lz");
const u16 gPokenavOptions_Pal[] = INCBIN_U16("graphics/pokenav/options/options.gbapal");

const u16 gPokenavHeader_Pal[] = INCBIN_U16("graphics/pokenav/header.gbapal");
const u32 gPokenavHeader_Gfx[] = INCBIN_U32("graphics/pokenav/header.4bpp.lz"); // TODO: use width 9 and makefile rule for cleanliness, make wasnt behaving, didnt want to apply num_tiles to this
const u32 gPokenavHeader_Tilemap[] = INCBIN_U32("graphics/pokenav/header.bin.lz");

const u16 gPokenavLeftHeader_Pal[] = INCBIN_U16("graphics/pokenav/left_headers/palette.gbapal");
const u32 gPokenavLeftHeaderBeauty_Gfx[] = INCBIN_U32("graphics/pokenav/left_headers/beauty.4bpp.lz");
const u32 gPokenavLeftHeaderSmart_Gfx[] = INCBIN_U32("graphics/pokenav/left_headers/smart.4bpp.lz");
const u32 gPokenavLeftHeaderCondition_Gfx[] = INCBIN_U32("graphics/pokenav/left_headers/condition.4bpp.lz");
const u32 gPokenavLeftHeaderCute_Gfx[] = INCBIN_U32("graphics/pokenav/left_headers/cute.4bpp.lz");
const u32 gPokenavLeftHeaderMatchCall_Gfx[] = INCBIN_U32("graphics/pokenav/left_headers/match_call.4bpp.lz");
const u32 gPokenavLeftHeaderMainMenu_Gfx[] = INCBIN_U32("graphics/pokenav/left_headers/main_menu.4bpp.lz");
const u32 gPokenavLeftHeaderHoennMap_Gfx[] = INCBIN_U32("graphics/pokenav/left_headers/hoenn_map.4bpp.lz");
const u32 gPokenavLeftHeaderRibbons_Gfx[] = INCBIN_U32("graphics/pokenav/left_headers/ribbons.4bpp.lz");
const u32 gPokenavLeftHeaderSearch_Gfx[] = INCBIN_U32("graphics/pokenav/left_headers/search.4bpp.lz");
const u32 gPokenavLeftHeaderTough_Gfx[] = INCBIN_U32("graphics/pokenav/left_headers/tough.4bpp.lz");
const u32 gPokenavLeftHeaderCool_Gfx[] = INCBIN_U32("graphics/pokenav/left_headers/cool.4bpp.lz");
const u32 gPokenavLeftHeaderParty_Gfx[] = INCBIN_U32("graphics/pokenav/left_headers/party.4bpp.lz");

const u16 gPokenavMessageBox_Pal[] = INCBIN_U16("graphics/pokenav/message.gbapal");
const u32 gPokenavMessageBox_Gfx[] = INCBIN_U32("graphics/pokenav/message.4bpp.lz");
const u32 gPokenavMessageBox_Tilemap[] = INCBIN_U32("graphics/pokenav/message.bin.lz");

const u16 gRegionMapCityZoomTiles_Pal[] = INCBIN_U16("graphics/pokenav/region_map/zoom_tiles.gbapal");
const u32 gRegionMapCityZoomText_Gfx[] = INCBIN_U32("graphics/pokenav/region_map/city_zoom_text.4bpp.lz");

const u16 gPokenavConditionCancel_Pal[] = INCBIN_U16("graphics/pokenav/condition/cancel.gbapal");
const u8 gPokenavConditionCancel_Gfx[] = INCBIN_U8("graphics/pokenav/condition/cancel.4bpp");

const u16 gMonMarkingsMenu_Pal[] = INCBIN_U16("graphics/interface/mon_markings_menu.gbapal");
const u8 gMonMarkingsMenu_Gfx[] = INCBIN_U8("graphics/interface/mon_markings_menu.4bpp");

const u16 gBerryBlenderMiscPalette[] = INCBIN_U16("graphics/berry_blender/misc.gbapal");
const u16 gBerryBlenderArrowPalette[] = INCBIN_U16("graphics/berry_blender/arrow.gbapal");
const u8 gBerryBlenderBetaArrow_Gfx[] = INCBIN_U8("graphics/berry_blender/arrow_old.4bpp"); //unused
const u8 gBerryBlenderScoreSymbols_Gfx[] = INCBIN_U8("graphics/berry_blender/score_symbols.4bpp");
const u8 gBerryBlenderParticles_Gfx[] = INCBIN_U8("graphics/berry_blender/particles.4bpp");
static const u8 sEmpty0[0x120] = {0};
const u8 gBerryBlenderCountdownNumbers_Gfx[] = INCBIN_U8("graphics/berry_blender/countdown_numbers.4bpp");
const u8 gBerryBlenderStart_Gfx[] = INCBIN_U8("graphics/berry_blender/start.4bpp");
static const u8 sEmpty1[0x200] = {0};
const u8 gBerryBlenderPlayerArrow_Gfx[] = INCBIN_U8("graphics/berry_blender/arrow.4bpp");
static const u8 sEmpty2[0x2C0] = {0};

const u16 gEasyChatRectangleCursor_Pal[] = INCBIN_U16("graphics/easy_chat/rectangle_cursor.gbapal");
const u32 gEasyChatRectangleCursor_Gfx[] = INCBIN_U32("graphics/easy_chat/rectangle_cursor.4bpp.lz");

const u16 gEasyChatButtonWindow_Pal[] = INCBIN_U16("graphics/easy_chat/button_window.gbapal");
const u32 gEasyChatButtonWindow_Gfx[] = INCBIN_U32("graphics/easy_chat/button_window.4bpp.lz");

const u32 gEasyChatMode_Gfx[] = INCBIN_U32("graphics/easy_chat/mode.4bpp.lz");
const u16 gEasyChatMode_Pal[] = INCBIN_U16("graphics/easy_chat/mode.gbapal");

const u32 gEasyChatWindow_Gfx[] = INCBIN_U32("graphics/easy_chat/window.4bpp.lz"); // uses mode pal
const u32 gEasyChatWindow_Tilemap[] = INCBIN_U32("graphics/easy_chat/window.bin.lz");

const u16 gWallClockMale_Pal[]      = INCBIN_U16("graphics/wallclock/male.gbapal");
const u16 gWallClockFemale_Pal[]    = INCBIN_U16("graphics/wallclock/female.gbapal");
const u32 gWallClock_Gfx[]          = INCBIN_U32("graphics/wallclock/clock.4bpp.lz");
const u32 gWallClockStart_Tilemap[] = INCBIN_U32("graphics/wallclock/clock_start.bin.lz");
const u32 gWallClockView_Tilemap[]  = INCBIN_U32("graphics/wallclock/clock_view.bin.lz");

const u16 gUsePokeblockCondition_Pal[] = INCBIN_U16("graphics/pokeblock/use_screen/condition.gbapal");
const u32 gUsePokeblockCondition_Gfx[] = INCBIN_U32("graphics/pokeblock/use_screen/condition.4bpp.lz");

const u16 gUsePokeblockUpDown_Pal[] = INCBIN_U16("graphics/pokeblock/use_screen/updown.gbapal");
const u8 gUsePokeblockUpDown_Gfx[] = INCBIN_U8("graphics/pokeblock/use_screen/updown.4bpp");

const u16 gUsePokeblockGraph_Pal[] = INCBIN_U16("graphics/pokeblock/use_screen/graph.gbapal");
const u32 gUsePokeblockGraph_Gfx[] = INCBIN_U32("graphics/pokeblock/use_screen/graph.4bpp.lz");

const u32 gUsePokeblockGraph_Tilemap[] = INCBIN_U32("graphics/pokeblock/use_screen/graph.bin.lz");
const u16 gUsePokeblockNatureWin_Pal[] = INCBIN_U16("graphics/pokeblock/use_screen/nature.gbapal");

#include "data/graphics/slot_machine.h"

// trainer card

const u16 gHoennTrainerCardGreen_Pal[] = INCBIN_U16("graphics/trainer_card/green.gbapal");
const u32 gHoennTrainerCard_Gfx[] = INCBIN_U32("graphics/trainer_card/tiles.4bpp.lz");
const u32 gHoennTrainerCardBg_Tilemap[] = INCBIN_U32("graphics/trainer_card/bg.bin.lz");
const u32 gHoennTrainerCardFront_Tilemap[] = INCBIN_U32("graphics/trainer_card/front.bin.lz");
const u32 gHoennTrainerCardBack_Tilemap[] = INCBIN_U32("graphics/trainer_card/back.bin.lz");
const u32 gHoennTrainerCardFrontLink_Tilemap[] = INCBIN_U32("graphics/trainer_card/front_link.bin.lz");

const u16 gKantoTrainerCardBlue_Pal[] = INCBIN_U16("graphics/trainer_card/frlg/blue.gbapal");
const u32 gKantoTrainerCard_Gfx[] = INCBIN_U32("graphics/trainer_card/frlg/tiles.4bpp.lz");
const u32 gKantoTrainerCardBg_Tilemap[] = INCBIN_U32("graphics/trainer_card/frlg/bg.bin.lz");
const u32 gKantoTrainerCardFront_Tilemap[] = INCBIN_U32("graphics/trainer_card/frlg/front.bin.lz");
const u32 gKantoTrainerCardBack_Tilemap[] = INCBIN_U32("graphics/trainer_card/frlg/back.bin.lz");
const u32 gKantoTrainerCardFrontLink_Tilemap[] = INCBIN_U32("graphics/trainer_card/frlg/front_link.bin.lz");

// Pokémon storage system

const u32 gStorageSystemMenu_Gfx[] = INCBIN_U32("graphics/pokemon_storage/menu.4bpp.lz");
const u16 gStorageSystemPartyMenu_Pal[] = INCBIN_U16("graphics/pokemon_storage/party_menu.gbapal");
const u32 gStorageSystemPartyMenu_Tilemap[] = INCBIN_U32("graphics/pokemon_storage/party_menu.bin.lz");

// naming screen

const u16 gNamingScreenMenu_Pal[6][16] =
{
    INCBIN_U16("graphics/naming_screen/menu.gbapal"),
    INCBIN_U16("graphics/naming_screen/page_swap_upper.gbapal"),
    INCBIN_U16("graphics/naming_screen/page_swap_lower.gbapal"),
    INCBIN_U16("graphics/naming_screen/page_swap_others.gbapal"),
    INCBIN_U16("graphics/naming_screen/buttons.gbapal"),
    INCBIN_U16("graphics/naming_screen/cursor.gbapal"),
};
const u32 gNamingScreenMenu_Gfx[]                = INCBIN_U32("graphics/naming_screen/menu.4bpp.lz");
const u32 gNamingScreenPageSwapFrame_Gfx[]       = INCBIN_U32("graphics/naming_screen/page_swap_frame.4bpp");
const u32 gNamingScreenBackButton_Gfx[]          = INCBIN_U32("graphics/naming_screen/back_button.4bpp");
const u32 gNamingScreenOKButton_Gfx[]            = INCBIN_U32("graphics/naming_screen/ok_button.4bpp");
const u32 gNamingScreenPageSwapUpper_Gfx[]       = INCBIN_U32("graphics/naming_screen/page_swap_upper.4bpp");
const u32 gNamingScreenPageSwapLower_Gfx[]       = INCBIN_U32("graphics/naming_screen/page_swap_lower.4bpp");
const u32 gNamingScreenPageSwapOthers_Gfx[]      = INCBIN_U32("graphics/naming_screen/page_swap_others.4bpp");
const u32 gNamingScreenCursor_Gfx[]              = INCBIN_U32("graphics/naming_screen/cursor.4bpp");
const u32 gNamingScreenCursorSquished_Gfx[]      = INCBIN_U32("graphics/naming_screen/cursor_squished.4bpp");
const u32 gNamingScreenCursorFilled_Gfx[]        = INCBIN_U32("graphics/naming_screen/cursor_filled.4bpp");
const u32 gNamingScreenPageSwapButton_Gfx[]      = INCBIN_U32("graphics/naming_screen/page_swap_button.4bpp");
const u32 gNamingScreenInputArrow_Gfx[]          = INCBIN_U32("graphics/naming_screen/input_arrow.4bpp");
const u32 gNamingScreenUnderscore_Gfx[]          = INCBIN_U32("graphics/naming_screen/underscore.4bpp");
const u32 gNamingScreenBackground_Tilemap[]      = INCBIN_U32("graphics/naming_screen/background.bin.lz");
const u32 gNamingScreenKeyboardUpper_Tilemap[]   = INCBIN_U32("graphics/naming_screen/keyboard_upper.bin.lz");
const u32 gNamingScreenKeyboardLower_Tilemap[]   = INCBIN_U32("graphics/naming_screen/keyboard_lower.bin.lz");
const u32 gNamingScreenKeyboardSymbols_Tilemap[] = INCBIN_U32("graphics/naming_screen/keyboard_symbols.bin.lz");

// union room chat
const u16 gUnionRoomChat_Background_Pal[] = INCBIN_U16("graphics/union_room_chat/background.gbapal");
const u32 gUnionRoomChat_Background_Gfx[] = INCBIN_U32("graphics/union_room_chat/background.4bpp.lz");
const u32 gUnionRoomChat_Background_Tilemap[] = INCBIN_U32("graphics/union_room_chat/background.bin.lz");
const u16 gUnionRoomChat_InputText_Pal[] = INCBIN_U16("graphics/union_room_chat/input_text.gbapal");
const u16 gUnionRoomChat_Keyboard_Pal[] = INCBIN_U16("graphics/union_room_chat/keyboard.gbapal");
const u32 gUnionRoomChat_Keyboard_Gfx[] = INCBIN_U32("graphics/union_room_chat/keyboard.4bpp.lz");
const u32 gUnionRoomChat_Keyboard_Tilemap[] = INCBIN_U32("graphics/union_room_chat/keyboard.bin.lz");
const u32 gUnionRoomChat_RButtonLabels[] = INCBIN_U32("graphics/union_room_chat/r_button_labels.4bpp.lz");

const u16 gTilesetPalettes_General[][16] =
{
    INCBIN_U16("data/tilesets/primary/general/palettes/00.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/01.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/02.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/03.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/04.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/05.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/06.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/07.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/08.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/09.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/10.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/11.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/12.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/13.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/14.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/15.gbapal"),
};

const u32 gTilesetTiles_General[] = INCBIN_U32("data/tilesets/primary/general/tiles.4bpp.lz");

// trade/egg hatch

const u16 gTradeGba_Pal[] = INCBIN_U16("graphics/trade/gba.gbapal");
const u16 gTradeGba2_Pal[] = INCBIN_U16("graphics/trade/gba_pal2.gbapal");
const u8 gTradeGba_Gfx[] = INCBIN_U8("graphics/trade/gba.4bpp");

static const u16 sEmptyPal[16] = {0};

#include "data/graphics/berry_fix.h"

// Trade
const u16 gTradeMenu_Pal[] = INCBIN_U16("graphics/trade/menu.gbapal");
const u16 gTradeCursor_Pal[] = INCBIN_U16("graphics/trade/cursor.gbapal");
const u8 gTradeMenu_Gfx[] = INCBIN_U8("graphics/trade/menu.4bpp");
const u8 gTradeCursor_Gfx[] = INCBIN_U8("graphics/trade/cursor.4bpp");
const u16 gTradeUnused_Tilemap[] = INCBIN_U16("graphics/trade/unused.bin");
const u16 gTradeMenu_Tilemap[] = INCBIN_U16("graphics/trade/menu.bin");
const u16 gTradeMenuMonBox_Tilemap[] = INCBIN_U16("graphics/trade/menu_mon_box.bin");

const u16 gMessageBox_Pal[] = INCBIN_U16("graphics/text_window/message_box.gbapal");
const u8 gMessageBox_Gfx[] = INCBIN_U8("graphics/text_window/message_box.4bpp");

const u32 gWallpaperIcon_Cross[] = INCBIN_U32("graphics/pokemon_storage/wallpapers/icons/cross.4bpp.lz");
const u32 gWallpaperIcon_Bolt[] = INCBIN_U32("graphics/pokemon_storage/wallpapers/icons/bolt.4bpp.lz");
const u32 gWallpaperIcon_Plusle[] = INCBIN_U32("graphics/pokemon_storage/wallpapers/icons/plusle.4bpp.lz");

const u16 gWallpaperPalettes_Horizontal[][16] =
{
    INCBIN_U16("graphics/pokemon_storage/wallpapers/friends_frame2.gbapal"),
    INCBIN_U16("graphics/pokemon_storage/wallpapers/horizontal/bg.gbapal"),
};

const u32 gWallpaperTiles_Horizontal[] = INCBIN_U32("graphics/pokemon_storage/wallpapers/horizontal/tiles.4bpp.lz");
const u32 gWallpaperTilemap_Horizontal[] = INCBIN_U32("graphics/pokemon_storage/wallpapers/horizontal/tilemap.bin.lz");

const u16 gWallpaperPalettes_Ribbon[][16] =
{
    INCBIN_U16("graphics/pokemon_storage/wallpapers/ribbon/frame.gbapal"),
    INCBIN_U16("graphics/pokemon_storage/wallpapers/ribbon/bg.gbapal"),
};

const u32 gWallpaperTiles_Ribbon[] = INCBIN_U32("graphics/pokemon_storage/wallpapers/ribbon/tiles.4bpp.lz");
const u32 gWallpaperTilemap_Ribbon[] = INCBIN_U32("graphics/pokemon_storage/wallpapers/ribbon/tilemap.bin.lz");

const u16 gPokenavRibbonsSummaryBg_Pal[] = INCBIN_U16("graphics/pokenav/ribbons/summary_bg.gbapal");
const u32 gPokenavRibbonsSummaryBg_Gfx[] = INCBIN_U32("graphics/pokenav/ribbons/summary_bg.4bpp.lz");
const u32 gPokenavRibbonsSummaryBg_Tilemap[] = INCBIN_U32("graphics/pokenav/ribbons/summary_bg.bin.lz");

const u16 gMonIconPalettes[][16] =
{
    INCBIN_U16("graphics/pokemon/icon_palettes/icon_palette_0.gbapal"),
    INCBIN_U16("graphics/pokemon/icon_palettes/icon_palette_1.gbapal"),
    INCBIN_U16("graphics/pokemon/icon_palettes/icon_palette_2.gbapal"),
};

const u16 gTitleScreenBgPalettes[]         = INCBIN_U16("graphics/title_screen/pokemon_logo.gbapal",
                                                        "graphics/title_screen/rayquaza_and_clouds.gbapal");
const u16 gTitleScreenEmeraldVersionPal[]  = INCBIN_U16("graphics/title_screen/emerald_version.gbapal");
const u32 gTitleScreenCloudsTilemap[]      = INCBIN_U32("graphics/title_screen/clouds.bin.lz");
const u32 gTitleScreenPokemonLogoGfx[]     = INCBIN_U32("graphics/title_screen/pokemon_logo.8bpp.lz");
const u32 gTitleScreenEmeraldVersionGfx[]  = INCBIN_U32("graphics/title_screen/emerald_version.8bpp.lz");
const u16 gTitleScreenPressStartPal[]      = INCBIN_U16("graphics/title_screen/press_start.gbapal");
const u32 gTitleScreenPressStartGfx[]      = INCBIN_U32("graphics/title_screen/press_start.4bpp.lz");
const u32 gTitleScreenPokemonLogoTilemap[] = INCBIN_U32("graphics/title_screen/pokemon_logo.bin.lz");

const u16 gFrontierPassBg_Pal[]                          = INCBIN_U16("graphics/frontier_pass/bg.gbapal"); // 8 x 16
const u32 gFrontierPassBg_Gfx[]                          = INCBIN_U32("graphics/frontier_pass/bg.4bpp.lz");
const u32 gFrontierPassMapAndCard_Gfx[]                  = INCBIN_U32("graphics/frontier_pass/map_and_card.8bpp.lz");
const u32 gFrontierPassBg_Tilemap[]                      = INCBIN_U32("graphics/frontier_pass/bg.bin.lz");
const u16 gFrontierPassCancelButton_Tilemap[]            = INCBIN_U16("graphics/frontier_pass/cancel.bin");
const u16 gFrontierPassCancelButtonHighlighted_Tilemap[] = INCBIN_U16("graphics/frontier_pass/cancel_highlighted.bin");

// Berry Crush
const u16 gBerryCrush_Crusher_Pal[] = INCBIN_U16("graphics/berry_crush/crusher.gbapal");
const u32 gBerryCrush_Crusher_Gfx[] = INCBIN_U32("graphics/berry_crush/crusher.4bpp.lz");
const u32 gBerryCrush_TextWindows_Tilemap[] = INCBIN_U32("graphics/berry_crush/text_windows.bin.lz");

// random garbage at the end.
static const u8 sEmpty3[0x54BAC] = {0};
static const u8 sUnused1[]       = {0x0D, 0x00, 0x58, 0x02};
static const u8 sEmpty4[0x1145]  = {0};
static const u8 sUnused2[]       = {0x02};
static const u8 sEmpty5[0x3242]  = {0};
static const u8 sUnused3[]       = {0x40};
static const u8 sEmpty6[0x13]    = {0};
