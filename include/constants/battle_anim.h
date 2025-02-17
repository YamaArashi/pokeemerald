#ifndef GUARD_CONSTANTS_BATTLE_ANIM_H
#define GUARD_CONSTANTS_BATTLE_ANIM_H

// Sprites start at 10000 and thus must be subtracted of 10000 to account for the true index.
#define GET_TRUE_SPRITE_INDEX(i) ((i - ANIM_SPRITES_START))

// Particle ids.
#define ANIM_SPRITES_START 10000

#define ANIM_TAG_BONE                       (ANIM_SPRITES_START + 0)
#define ANIM_TAG_SPARK                      (ANIM_SPRITES_START + 1)
#define ANIM_TAG_PENCIL                     (ANIM_SPRITES_START + 2)
#define ANIM_TAG_AIR_WAVE                   (ANIM_SPRITES_START + 3)
#define ANIM_TAG_ORB                        (ANIM_SPRITES_START + 4)
#define ANIM_TAG_SWORD                      (ANIM_SPRITES_START + 5)
#define ANIM_TAG_SEED                       (ANIM_SPRITES_START + 6)
#define ANIM_TAG_EXPLOSION_6                (ANIM_SPRITES_START + 7)
#define ANIM_TAG_PINK_ORB                   (ANIM_SPRITES_START + 8) // unused
#define ANIM_TAG_GUST                       (ANIM_SPRITES_START + 9)
#define ANIM_TAG_ICE_CUBE                   (ANIM_SPRITES_START + 10)
#define ANIM_TAG_SPARK_2                    (ANIM_SPRITES_START + 11)
#define ANIM_TAG_ORANGE                     (ANIM_SPRITES_START + 12) // unused
#define ANIM_TAG_YELLOW_BALL                (ANIM_SPRITES_START + 13)
#define ANIM_TAG_LOCK_ON                    (ANIM_SPRITES_START + 14)
#define ANIM_TAG_TIED_BAG                   (ANIM_SPRITES_START + 15)
#define ANIM_TAG_BLACK_SMOKE                (ANIM_SPRITES_START + 16)
#define ANIM_TAG_BLACK_BALL                 (ANIM_SPRITES_START + 17)
#define ANIM_TAG_CONVERSION                 (ANIM_SPRITES_START + 18)
#define ANIM_TAG_GLASS                      (ANIM_SPRITES_START + 19) // unused
#define ANIM_TAG_HORN_HIT                   (ANIM_SPRITES_START + 20)
#define ANIM_TAG_HIT                        (ANIM_SPRITES_START + 21)
#define ANIM_TAG_HIT_2                      (ANIM_SPRITES_START + 22)
#define ANIM_TAG_BLUE_SHARDS                (ANIM_SPRITES_START + 23) // unused
#define ANIM_TAG_CLOSING_EYE                (ANIM_SPRITES_START + 24) // unused
#define ANIM_TAG_WAVING_HAND                (ANIM_SPRITES_START + 25) // unused
#define ANIM_TAG_HIT_DUPLICATE              (ANIM_SPRITES_START + 26) // unused
#define ANIM_TAG_LEER                       (ANIM_SPRITES_START + 27)
#define ANIM_TAG_BLUE_BURST                 (ANIM_SPRITES_START + 28) // unused
#define ANIM_TAG_SMALL_EMBER                (ANIM_SPRITES_START + 29)
#define ANIM_TAG_GRAY_SMOKE                 (ANIM_SPRITES_START + 30)
#define ANIM_TAG_BLUE_STAR                  (ANIM_SPRITES_START + 31)
#define ANIM_TAG_BUBBLE_BURST               (ANIM_SPRITES_START + 32)
#define ANIM_TAG_FIRE                       (ANIM_SPRITES_START + 33)
#define ANIM_TAG_SPINNING_FIRE              (ANIM_SPRITES_START + 34) // unused
#define ANIM_TAG_FIRE_PLUME                 (ANIM_SPRITES_START + 35)
#define ANIM_TAG_LIGHTNING_2                (ANIM_SPRITES_START + 36) // unused
#define ANIM_TAG_LIGHTNING                  (ANIM_SPRITES_START + 37)
#define ANIM_TAG_CLAW_SLASH_2               (ANIM_SPRITES_START + 38) // unused
#define ANIM_TAG_CLAW_SLASH                 (ANIM_SPRITES_START + 39)
#define ANIM_TAG_SCRATCH_3                  (ANIM_SPRITES_START + 40) // unused
#define ANIM_TAG_SCRATCH_2                  (ANIM_SPRITES_START + 41) // unused
#define ANIM_TAG_BUBBLE_BURST_2             (ANIM_SPRITES_START + 42) // unused
#define ANIM_TAG_ICE_CHUNK                  (ANIM_SPRITES_START + 43)
#define ANIM_TAG_GLASS_2                    (ANIM_SPRITES_START + 44) // unused
#define ANIM_TAG_PINK_HEART_2               (ANIM_SPRITES_START + 45) // unused
#define ANIM_TAG_SAP_DRIP                   (ANIM_SPRITES_START + 46) // unused
#define ANIM_TAG_SAP_DRIP_2                 (ANIM_SPRITES_START + 47) // unused
#define ANIM_TAG_SPARKLE_1                  (ANIM_SPRITES_START + 48)
#define ANIM_TAG_SPARKLE_2                  (ANIM_SPRITES_START + 49)
#define ANIM_TAG_HUMANOID_FOOT              (ANIM_SPRITES_START + 50)
#define ANIM_TAG_MONSTER_FOOT               (ANIM_SPRITES_START + 51)
#define ANIM_TAG_HUMANOID_HAND              (ANIM_SPRITES_START + 52) // unused
#define ANIM_TAG_NOISE_LINE                 (ANIM_SPRITES_START + 53)
#define ANIM_TAG_YELLOW_UNK                 (ANIM_SPRITES_START + 54) // unused
#define ANIM_TAG_RED_FIST                   (ANIM_SPRITES_START + 55) // unused
#define ANIM_TAG_SLAM_HIT                   (ANIM_SPRITES_START + 56)
#define ANIM_TAG_RING                       (ANIM_SPRITES_START + 57) // unused
#define ANIM_TAG_ROCKS                      (ANIM_SPRITES_START + 58)
#define ANIM_TAG_Z                          (ANIM_SPRITES_START + 59) // unused
#define ANIM_TAG_YELLOW_UNK_2               (ANIM_SPRITES_START + 60) // unused
#define ANIM_TAG_AIR_SLASH                  (ANIM_SPRITES_START + 61) // unused
#define ANIM_TAG_SPINNING_GREEN_ORBS        (ANIM_SPRITES_START + 62) // unused
#define ANIM_TAG_LEAF                       (ANIM_SPRITES_START + 63)
#define ANIM_TAG_FINGER                     (ANIM_SPRITES_START + 64)
#define ANIM_TAG_POISON_POWDER              (ANIM_SPRITES_START + 65)
#define ANIM_TAG_BROWN_TRIANGLE             (ANIM_SPRITES_START + 66) // unused
#define ANIM_TAG_SLEEP_POWDER               (ANIM_SPRITES_START + 67)
#define ANIM_TAG_STUN_SPORE                 (ANIM_SPRITES_START + 68)
#define ANIM_TAG_POWDER                     (ANIM_SPRITES_START + 69) // unused
#define ANIM_TAG_SPARKLE_3                  (ANIM_SPRITES_START + 70)
#define ANIM_TAG_SPARKLE_4                  (ANIM_SPRITES_START + 71)
#define ANIM_TAG_MUSIC_NOTES                (ANIM_SPRITES_START + 72)
#define ANIM_TAG_DUCK                       (ANIM_SPRITES_START + 73)
#define ANIM_TAG_MUD_SAND                   (ANIM_SPRITES_START + 74)
#define ANIM_TAG_ALERT                      (ANIM_SPRITES_START + 75)
#define ANIM_TAG_BLUE_FLAMES                (ANIM_SPRITES_START + 76) // unused
#define ANIM_TAG_BLUE_FLAMES_2              (ANIM_SPRITES_START + 77) // unused
#define ANIM_TAG_SHOCK_4                    (ANIM_SPRITES_START + 78) // unused
#define ANIM_TAG_SHOCK                      (ANIM_SPRITES_START + 79)
#define ANIM_TAG_BELL_2                     (ANIM_SPRITES_START + 80) // unused
#define ANIM_TAG_PINK_GLOVE                 (ANIM_SPRITES_START + 81) // unused
#define ANIM_TAG_BLUE_LINES                 (ANIM_SPRITES_START + 82) // unused
#define ANIM_TAG_IMPACT_3                   (ANIM_SPRITES_START + 83) // unused
#define ANIM_TAG_IMPACT_2                   (ANIM_SPRITES_START + 84) // unused
#define ANIM_TAG_RETICLE                    (ANIM_SPRITES_START + 85) // unused
#define ANIM_TAG_BREATH                     (ANIM_SPRITES_START + 86)
#define ANIM_TAG_ANGER                      (ANIM_SPRITES_START + 87)
#define ANIM_TAG_SNOWBALL                   (ANIM_SPRITES_START + 88) // unused
#define ANIM_TAG_VINE                       (ANIM_SPRITES_START + 89) // unused
#define ANIM_TAG_SWORD_2                    (ANIM_SPRITES_START + 90) // unused
#define ANIM_TAG_CLAPPING                   (ANIM_SPRITES_START + 91) // unused
#define ANIM_TAG_RED_TUBE                   (ANIM_SPRITES_START + 92) // unused
#define ANIM_TAG_AMNESIA                    (ANIM_SPRITES_START + 93)
#define ANIM_TAG_STRING_2                   (ANIM_SPRITES_START + 94) // unused
#define ANIM_TAG_PENCIL_2                   (ANIM_SPRITES_START + 95) // unused
#define ANIM_TAG_PETAL                      (ANIM_SPRITES_START + 96) // unused
#define ANIM_TAG_BENT_SPOON                 (ANIM_SPRITES_START + 97)
#define ANIM_TAG_WEB                        (ANIM_SPRITES_START + 98) // unused
#define ANIM_TAG_MILK_BOTTLE                (ANIM_SPRITES_START + 99)
#define ANIM_TAG_COIN                       (ANIM_SPRITES_START + 100)
#define ANIM_TAG_CRACKED_EGG                (ANIM_SPRITES_START + 101) // unused
#define ANIM_TAG_HATCHED_EGG                (ANIM_SPRITES_START + 102) // unused
#define ANIM_TAG_FRESH_EGG                  (ANIM_SPRITES_START + 103) // unused
#define ANIM_TAG_FANGS                      (ANIM_SPRITES_START + 104) // unused
#define ANIM_TAG_EXPLOSION_2                (ANIM_SPRITES_START + 105) // unused
#define ANIM_TAG_EXPLOSION_3                (ANIM_SPRITES_START + 106) // unused
#define ANIM_TAG_WATER_DROPLET              (ANIM_SPRITES_START + 107) // unused
#define ANIM_TAG_WATER_DROPLET_2            (ANIM_SPRITES_START + 108) // unused
#define ANIM_TAG_SEED_2                     (ANIM_SPRITES_START + 109) // unused
#define ANIM_TAG_SPROUT                     (ANIM_SPRITES_START + 110) // unused
#define ANIM_TAG_RED_WAND                   (ANIM_SPRITES_START + 111) // unused
#define ANIM_TAG_PURPLE_GREEN_UNK           (ANIM_SPRITES_START + 112) // unused
#define ANIM_TAG_WATER_COLUMN               (ANIM_SPRITES_START + 113) // unused
#define ANIM_TAG_MUD_UNK                    (ANIM_SPRITES_START + 114) // unused
#define ANIM_TAG_RAIN_DROPS                 (ANIM_SPRITES_START + 115)
#define ANIM_TAG_FURY_SWIPES                (ANIM_SPRITES_START + 116) // unused
#define ANIM_TAG_VINE_2                     (ANIM_SPRITES_START + 117) // unused
#define ANIM_TAG_TEETH                      (ANIM_SPRITES_START + 118) // unused
#define ANIM_TAG_BONE_2                     (ANIM_SPRITES_START + 119) // unused
#define ANIM_TAG_WHITE_BAG                  (ANIM_SPRITES_START + 120) // unused
#define ANIM_TAG_UNKNOWN                    (ANIM_SPRITES_START + 121) // unused
#define ANIM_TAG_PURPLE_CORAL               (ANIM_SPRITES_START + 122) // unused
#define ANIM_TAG_PURPLE_DROPLET             (ANIM_SPRITES_START + 123) // unused
#define ANIM_TAG_SHOCK_2                    (ANIM_SPRITES_START + 124) // unused
#define ANIM_TAG_CLOSING_EYE_2              (ANIM_SPRITES_START + 125) // unused
#define ANIM_TAG_METAL_BALL                 (ANIM_SPRITES_START + 126) // unused
#define ANIM_TAG_MONSTER_DOLL               (ANIM_SPRITES_START + 127) // unused
#define ANIM_TAG_WHIRLWIND                  (ANIM_SPRITES_START + 128) // unused
#define ANIM_TAG_WHIRLWIND_2                (ANIM_SPRITES_START + 129) // unused
#define ANIM_TAG_EXPLOSION_4                (ANIM_SPRITES_START + 130) // unused
#define ANIM_TAG_EXPLOSION_5                (ANIM_SPRITES_START + 131) // unused
#define ANIM_TAG_TONGUE                     (ANIM_SPRITES_START + 132) // unused
#define ANIM_TAG_SMOKE                      (ANIM_SPRITES_START + 133) // unused
#define ANIM_TAG_SMOKE_2                    (ANIM_SPRITES_START + 134) // unused
#define ANIM_TAG_IMPACT                     (ANIM_SPRITES_START + 135)
#define ANIM_TAG_CIRCLE_IMPACT              (ANIM_SPRITES_START + 136)
#define ANIM_TAG_SCRATCH                    (ANIM_SPRITES_START + 137)
#define ANIM_TAG_CUT                        (ANIM_SPRITES_START + 138)
#define ANIM_TAG_SHARP_TEETH                (ANIM_SPRITES_START + 139)
#define ANIM_TAG_RAINBOW_RINGS              (ANIM_SPRITES_START + 140)
#define ANIM_TAG_ICE_CRYSTALS               (ANIM_SPRITES_START + 141)
#define ANIM_TAG_ICE_SPIKES                 (ANIM_SPRITES_START + 142)
#define ANIM_TAG_HANDS_AND_FEET             (ANIM_SPRITES_START + 143)
#define ANIM_TAG_MIST_CLOUD                 (ANIM_SPRITES_START + 144)
#define ANIM_TAG_CLAMP                      (ANIM_SPRITES_START + 145)
#define ANIM_TAG_BUBBLE                     (ANIM_SPRITES_START + 146)
#define ANIM_TAG_ORBS                       (ANIM_SPRITES_START + 147)
#define ANIM_TAG_WATER_IMPACT               (ANIM_SPRITES_START + 148)
#define ANIM_TAG_WATER_ORB                  (ANIM_SPRITES_START + 149)
#define ANIM_TAG_POISON_BUBBLE              (ANIM_SPRITES_START + 150)
#define ANIM_TAG_TOXIC_BUBBLE               (ANIM_SPRITES_START + 151)
#define ANIM_TAG_SPIKES                     (ANIM_SPRITES_START + 152)
#define ANIM_TAG_HORN_HIT_2                 (ANIM_SPRITES_START + 153)
#define ANIM_TAG_AIR_WAVE_2                 (ANIM_SPRITES_START + 154)
#define ANIM_TAG_SMALL_BUBBLES              (ANIM_SPRITES_START + 155)
#define ANIM_TAG_ROUND_SHADOW               (ANIM_SPRITES_START + 156)
#define ANIM_TAG_SUNLIGHT                   (ANIM_SPRITES_START + 157)
#define ANIM_TAG_SPORE                      (ANIM_SPRITES_START + 158)
#define ANIM_TAG_FLOWER                     (ANIM_SPRITES_START + 159)
#define ANIM_TAG_RAZOR_LEAF                 (ANIM_SPRITES_START + 160)
#define ANIM_TAG_NEEDLE                     (ANIM_SPRITES_START + 161)
#define ANIM_TAG_WHIRLWIND_LINES            (ANIM_SPRITES_START + 162)
#define ANIM_TAG_GOLD_RING                  (ANIM_SPRITES_START + 163)
#define ANIM_TAG_PURPLE_RING                (ANIM_SPRITES_START + 164)
#define ANIM_TAG_BLUE_RING                  (ANIM_SPRITES_START + 165)
#define ANIM_TAG_GREEN_LIGHT_WALL           (ANIM_SPRITES_START + 166)
#define ANIM_TAG_BLUE_LIGHT_WALL            (ANIM_SPRITES_START + 167)
#define ANIM_TAG_RED_LIGHT_WALL             (ANIM_SPRITES_START + 168)
#define ANIM_TAG_GRAY_LIGHT_WALL            (ANIM_SPRITES_START + 169)
#define ANIM_TAG_ORANGE_LIGHT_WALL          (ANIM_SPRITES_START + 170)
#define ANIM_TAG_BLACK_BALL_2               (ANIM_SPRITES_START + 171)
#define ANIM_TAG_PURPLE_GAS_CLOUD           (ANIM_SPRITES_START + 172)
#define ANIM_TAG_SPARK_H                    (ANIM_SPRITES_START + 173)
#define ANIM_TAG_YELLOW_STAR                (ANIM_SPRITES_START + 174)
#define ANIM_TAG_LARGE_FRESH_EGG            (ANIM_SPRITES_START + 175)
#define ANIM_TAG_SHADOW_BALL                (ANIM_SPRITES_START + 176)
#define ANIM_TAG_LICK                       (ANIM_SPRITES_START + 177)
#define ANIM_TAG_VOID_LINES                 (ANIM_SPRITES_START + 178)
#define ANIM_TAG_STRING                     (ANIM_SPRITES_START + 179)
#define ANIM_TAG_WEB_THREAD                 (ANIM_SPRITES_START + 180)
#define ANIM_TAG_SPIDER_WEB                 (ANIM_SPRITES_START + 181)
#define ANIM_TAG_LIGHTBULB                  (ANIM_SPRITES_START + 182) // unused
#define ANIM_TAG_SLASH                      (ANIM_SPRITES_START + 183)
#define ANIM_TAG_FOCUS_ENERGY               (ANIM_SPRITES_START + 184)
#define ANIM_TAG_SPHERE_TO_CUBE             (ANIM_SPRITES_START + 185)
#define ANIM_TAG_TENDRILS                   (ANIM_SPRITES_START + 186)
#define ANIM_TAG_EYE                        (ANIM_SPRITES_START + 187)
#define ANIM_TAG_WHITE_SHADOW               (ANIM_SPRITES_START + 188)
#define ANIM_TAG_TEAL_ALERT                 (ANIM_SPRITES_START + 189)
#define ANIM_TAG_OPENING_EYE                (ANIM_SPRITES_START + 190)
#define ANIM_TAG_ROUND_WHITE_HALO           (ANIM_SPRITES_START + 191)
#define ANIM_TAG_FANG_ATTACK                (ANIM_SPRITES_START + 192)
#define ANIM_TAG_PURPLE_HAND_OUTLINE        (ANIM_SPRITES_START + 193)
#define ANIM_TAG_MOON                       (ANIM_SPRITES_START + 194)
#define ANIM_TAG_GREEN_SPARKLE              (ANIM_SPRITES_START + 195)
#define ANIM_TAG_SPIRAL                     (ANIM_SPRITES_START + 196)
#define ANIM_TAG_SNORE_Z                    (ANIM_SPRITES_START + 197)
#define ANIM_TAG_EXPLOSION                  (ANIM_SPRITES_START + 198)
#define ANIM_TAG_NAIL                       (ANIM_SPRITES_START + 199)
#define ANIM_TAG_GHOSTLY_SPIRIT             (ANIM_SPRITES_START + 200)
#define ANIM_TAG_WARM_ROCK                  (ANIM_SPRITES_START + 201)
#define ANIM_TAG_BREAKING_EGG               (ANIM_SPRITES_START + 202)
#define ANIM_TAG_THIN_RING                  (ANIM_SPRITES_START + 203)
#define ANIM_TAG_PUNCH_IMPACT               (ANIM_SPRITES_START + 204) // unused
#define ANIM_TAG_BELL                       (ANIM_SPRITES_START + 205)
#define ANIM_TAG_MUSIC_NOTES_2              (ANIM_SPRITES_START + 206)
#define ANIM_TAG_SPEED_DUST                 (ANIM_SPRITES_START + 207)
#define ANIM_TAG_TORN_METAL                 (ANIM_SPRITES_START + 208)
#define ANIM_TAG_THOUGHT_BUBBLE             (ANIM_SPRITES_START + 209)
#define ANIM_TAG_MAGENTA_HEART              (ANIM_SPRITES_START + 210)
#define ANIM_TAG_ELECTRIC_ORBS              (ANIM_SPRITES_START + 211)
#define ANIM_TAG_CIRCLE_OF_LIGHT            (ANIM_SPRITES_START + 212)
#define ANIM_TAG_ELECTRICITY                (ANIM_SPRITES_START + 213)
#define ANIM_TAG_FINGER_2                   (ANIM_SPRITES_START + 214)
#define ANIM_TAG_MOVEMENT_WAVES             (ANIM_SPRITES_START + 215)
#define ANIM_TAG_RED_HEART                  (ANIM_SPRITES_START + 216)
#define ANIM_TAG_RED_ORB                    (ANIM_SPRITES_START + 217)
#define ANIM_TAG_EYE_SPARKLE                (ANIM_SPRITES_START + 218)
#define ANIM_TAG_PINK_HEART                 (ANIM_SPRITES_START + 219)
#define ANIM_TAG_ANGEL                      (ANIM_SPRITES_START + 220)
#define ANIM_TAG_DEVIL                      (ANIM_SPRITES_START + 221)
#define ANIM_TAG_SWIPE                      (ANIM_SPRITES_START + 222)
#define ANIM_TAG_ROOTS                      (ANIM_SPRITES_START + 223)
#define ANIM_TAG_ITEM_BAG                   (ANIM_SPRITES_START + 224)
#define ANIM_TAG_JAGGED_MUSIC_NOTE          (ANIM_SPRITES_START + 225)
#define ANIM_TAG_POKEBALL                   (ANIM_SPRITES_START + 226)
#define ANIM_TAG_SPOTLIGHT                  (ANIM_SPRITES_START + 227)
#define ANIM_TAG_LETTER_Z                   (ANIM_SPRITES_START + 228)
#define ANIM_TAG_RAPID_SPIN                 (ANIM_SPRITES_START + 229)
#define ANIM_TAG_TRI_ATTACK_TRIANGLE        (ANIM_SPRITES_START + 230)
#define ANIM_TAG_WISP_ORB                   (ANIM_SPRITES_START + 231)
#define ANIM_TAG_WISP_FIRE                  (ANIM_SPRITES_START + 232)
#define ANIM_TAG_GOLD_STARS                 (ANIM_SPRITES_START + 233)
#define ANIM_TAG_ECLIPSING_ORB              (ANIM_SPRITES_START + 234)
#define ANIM_TAG_GRAY_ORB                   (ANIM_SPRITES_START + 235)
#define ANIM_TAG_BLUE_ORB                   (ANIM_SPRITES_START + 236)
#define ANIM_TAG_RED_ORB_2                  (ANIM_SPRITES_START + 237)
#define ANIM_TAG_PINK_PETAL                 (ANIM_SPRITES_START + 238)
#define ANIM_TAG_PAIN_SPLIT                 (ANIM_SPRITES_START + 239)
#define ANIM_TAG_CONFETTI                   (ANIM_SPRITES_START + 240)
#define ANIM_TAG_GREEN_STAR                 (ANIM_SPRITES_START + 241)
#define ANIM_TAG_PINK_CLOUD                 (ANIM_SPRITES_START + 242)
#define ANIM_TAG_SWEAT_DROP                 (ANIM_SPRITES_START + 243)
#define ANIM_TAG_GUARD_RING                 (ANIM_SPRITES_START + 244)
#define ANIM_TAG_PURPLE_SCRATCH             (ANIM_SPRITES_START + 245)
#define ANIM_TAG_PURPLE_SWIPE               (ANIM_SPRITES_START + 246)
#define ANIM_TAG_TAG_HAND                   (ANIM_SPRITES_START + 247)
#define ANIM_TAG_SMALL_RED_EYE              (ANIM_SPRITES_START + 248)
#define ANIM_TAG_HOLLOW_ORB                 (ANIM_SPRITES_START + 249)
#define ANIM_TAG_X_SIGN                     (ANIM_SPRITES_START + 250)
#define ANIM_TAG_BLUEGREEN_ORB              (ANIM_SPRITES_START + 251)
#define ANIM_TAG_PAW_PRINT                  (ANIM_SPRITES_START + 252)
#define ANIM_TAG_PURPLE_FLAME               (ANIM_SPRITES_START + 253)
#define ANIM_TAG_RED_BALL                   (ANIM_SPRITES_START + 254)
#define ANIM_TAG_SMELLINGSALT_EFFECT        (ANIM_SPRITES_START + 255)
#define ANIM_TAG_METEOR                     (ANIM_SPRITES_START + 256)
#define ANIM_TAG_FLAT_ROCK                  (ANIM_SPRITES_START + 257)
#define ANIM_TAG_MAGNIFYING_GLASS           (ANIM_SPRITES_START + 258)
#define ANIM_TAG_BROWN_ORB                  (ANIM_SPRITES_START + 259)
#define ANIM_TAG_METAL_SOUND_WAVES          (ANIM_SPRITES_START + 260)
#define ANIM_TAG_FLYING_DIRT                (ANIM_SPRITES_START + 261)
#define ANIM_TAG_ICICLE_SPEAR               (ANIM_SPRITES_START + 262)
#define ANIM_TAG_HAIL                       (ANIM_SPRITES_START + 263)
#define ANIM_TAG_GLOWY_RED_ORB              (ANIM_SPRITES_START + 264)
#define ANIM_TAG_GLOWY_GREEN_ORB            (ANIM_SPRITES_START + 265)
#define ANIM_TAG_GREEN_SPIKE                (ANIM_SPRITES_START + 266)
#define ANIM_TAG_WHITE_CIRCLE_OF_LIGHT      (ANIM_SPRITES_START + 267)
#define ANIM_TAG_GLOWY_BLUE_ORB             (ANIM_SPRITES_START + 268)
#define ANIM_TAG_POKEBLOCK                  (ANIM_SPRITES_START + 269)
#define ANIM_TAG_WHITE_FEATHER              (ANIM_SPRITES_START + 270)
#define ANIM_TAG_SPARKLE_6                  (ANIM_SPRITES_START + 271)
#define ANIM_TAG_SPLASH                     (ANIM_SPRITES_START + 272)
#define ANIM_TAG_SWEAT_BEAD                 (ANIM_SPRITES_START + 273)
#define ANIM_TAG_GEM_1                      (ANIM_SPRITES_START + 274) // unused
#define ANIM_TAG_GEM_2                      (ANIM_SPRITES_START + 275) // unused
#define ANIM_TAG_GEM_3                      (ANIM_SPRITES_START + 276) // unused
#define ANIM_TAG_SLAM_HIT_2                 (ANIM_SPRITES_START + 277)
#define ANIM_TAG_RECYCLE                    (ANIM_SPRITES_START + 278)
#define ANIM_TAG_RED_PARTICLES              (ANIM_SPRITES_START + 279) // unused
#define ANIM_TAG_PROTECT                    (ANIM_SPRITES_START + 280)
#define ANIM_TAG_DIRT_MOUND                 (ANIM_SPRITES_START + 281)
#define ANIM_TAG_SHOCK_3                    (ANIM_SPRITES_START + 282)
#define ANIM_TAG_WEATHER_BALL               (ANIM_SPRITES_START + 283)
#define ANIM_TAG_BIRD                       (ANIM_SPRITES_START + 284)
#define ANIM_TAG_CROSS_IMPACT               (ANIM_SPRITES_START + 285)
#define ANIM_TAG_SLASH_2                    (ANIM_SPRITES_START + 286)
#define ANIM_TAG_WHIP_HIT                   (ANIM_SPRITES_START + 287)
#define ANIM_TAG_BLUE_RING_2                (ANIM_SPRITES_START + 288)

// battlers
#define ANIM_ATTACKER         0
#define ANIM_TARGET           1
#define ANIM_ATK_PARTNER      2
#define ANIM_DEF_PARTNER      3

// Below are used by AnimTask_ShakeMon2 and AnimTask_SetGrayscaleOrOriginalPal
#define ANIM_PLAYER_LEFT      (MAX_BATTLERS_COUNT + 0)
#define ANIM_PLAYER_RIGHT     (MAX_BATTLERS_COUNT + 1)
#define ANIM_OPPONENT_LEFT    (MAX_BATTLERS_COUNT + 2)
#define ANIM_OPPONENT_RIGHT   (MAX_BATTLERS_COUNT + 3)
#define ANIM_ATTACKER_FORCE   (MAX_BATTLERS_COUNT + 4)

// stereo panning constants [0-255]
//
//          0
//         .  .
//      .        .
// 192 .          . 63
//     .          .
//      .        .
//         .  .
//          127
//
#define SOUND_PAN_ATTACKER -64
#define SOUND_PAN_TARGET    63

// move background ids
#define BG_NONE 0 // the same as BG_DARK but is unused
#define BG_DARK 1
#define BG_GHOST 2
#define BG_PSYCHIC 3
#define BG_IMPACT_OPPONENT 4
#define BG_IMPACT_PLAYER 5
#define BG_IMPACT_CONTESTS 6
#define BG_DRILL 7
#define BG_DRILL_CONTESTS 8
#define BG_HIGHSPEED_OPPONENT 9
#define BG_HIGHSPEED_PLAYER 10
#define BG_THUNDER 11
#define BG_GUILLOTINE_OPPONENT 12
#define BG_GUILLOTINE_PLAYER 13
#define BG_GUILLOTINE_CONTESTS 14
#define BG_ICE 15
#define BG_COSMIC 16
#define BG_IN_AIR 17
#define BG_SKY 18
#define BG_SKY_CONTESTS 19
#define BG_AURORA 20
#define BG_FISSURE 21
#define BG_BUG_OPPONENT 22
#define BG_BUG_PLAYER 23
#define BG_SOLAR_BEAM_OPPONENT 24
#define BG_SOLAR_BEAM_PLAYER 25
#define BG_SOLAR_BEAM_CONTESTS 26

// table ids for general animations (gBattleAnims_General)
#define B_ANIM_CASTFORM_CHANGE          0
#define B_ANIM_STATS_CHANGE             1
#define B_ANIM_SUBSTITUTE_FADE          2
#define B_ANIM_SUBSTITUTE_APPEAR        3
#define B_ANIM_POKEBLOCK_THROW          4
#define B_ANIM_ITEM_KNOCKOFF            5
#define B_ANIM_TURN_TRAP                6
#define B_ANIM_HELD_ITEM_EFFECT         7
#define B_ANIM_SMOKEBALL_ESCAPE         8
#define B_ANIM_FOCUS_BAND               9
#define B_ANIM_RAIN_CONTINUES           10
#define B_ANIM_SUN_CONTINUES            11
#define B_ANIM_SANDSTORM_CONTINUES      12
#define B_ANIM_HAIL_CONTINUES           13
#define B_ANIM_LEECH_SEED_DRAIN         14
#define B_ANIM_MON_HIT                  15
#define B_ANIM_ITEM_STEAL               16
#define B_ANIM_SNATCH_MOVE              17
#define B_ANIM_FUTURE_SIGHT_HIT         18
#define B_ANIM_DOOM_DESIRE_HIT          19
#define B_ANIM_FOCUS_PUNCH_SETUP        20
#define B_ANIM_INGRAIN_HEAL             21
#define B_ANIM_WISH_HEAL                22

// special animations table (gBattleAnims_Special)
#define B_ANIM_LVL_UP                   0
#define B_ANIM_SWITCH_OUT_PLAYER_MON    1
#define B_ANIM_SWITCH_OUT_OPPONENT_MON  2
#define B_ANIM_BALL_THROW               3
#define B_ANIM_BALL_THROW_WITH_TRAINER  4
#define B_ANIM_SUBSTITUTE_TO_MON        5
#define B_ANIM_MON_TO_SUBSTITUTE        6

// status animation table (gBattleAnims_StatusConditions)
#define B_ANIM_STATUS_PSN               0
#define B_ANIM_STATUS_CONFUSION         1
#define B_ANIM_STATUS_BRN               2
#define B_ANIM_STATUS_INFATUATION       3
#define B_ANIM_STATUS_SLP               4
#define B_ANIM_STATUS_PRZ               5
#define B_ANIM_STATUS_FRZ               6
#define B_ANIM_STATUS_CURSED            7
#define B_ANIM_STATUS_NIGHTMARE         8
#define B_ANIM_STATUS_WRAPPED           9 // does not actually exist

// Tasks with return values often assign them to gBattleAnimArgs[7].
#define ARG_RET_ID 7

// For createsprite macro to use internally
#define ANIMSPRITE_IS_TARGET (1 << 7)

// Trapping Wrap-like moves end turn animation.
#define TRAP_ANIM_BIND 0
#define TRAP_ANIM_WRAP 0
#define TRAP_ANIM_FIRE_SPIN 1
#define TRAP_ANIM_WHIRLPOOL 2
#define TRAP_ANIM_CLAMP 3
#define TRAP_ANIM_SAND_TOMB 4

// Weather defines for battle animation scripts.
#define ANIM_WEATHER_NONE 0
#define ANIM_WEATHER_SUN 1
#define ANIM_WEATHER_RAIN 2
#define ANIM_WEATHER_SANDSTORM 3
#define ANIM_WEATHER_HAIL 4

// Flags given to various functions to indicate which palettes to consider.
// Handled by UnpackSelectedBattlePalettes
#define F_PAL_BG          (1 << 0)
#define F_PAL_ATTACKER    (1 << 1)
#define F_PAL_TARGET      (1 << 2)
#define F_PAL_ATK_PARTNER (1 << 3)
#define F_PAL_DEF_PARTNER (1 << 4)
#define F_PAL_ANIM_1      (1 << 5) // Palette set for GetBattleAnimBg1Data/GetBgDataForTransform. Only used (ineffectually?) by Aromatherapy.
#define F_PAL_ANIM_2      (1 << 6) // Palette set for GetBattleAnimBgData/GetBgDataForTransform. Unused.
#define F_PAL_ATK_SIDE    (F_PAL_ATTACKER | F_PAL_ATK_PARTNER)
#define F_PAL_DEF_SIDE    (F_PAL_TARGET | F_PAL_DEF_PARTNER)
#define F_PAL_BATTLERS    (F_PAL_ATK_SIDE | F_PAL_DEF_SIDE)
// The below are only used by AnimTask_BlendBattleAnimPal to get battler sprite palettes by position rather than by role.
// It's redundant with F_PAL_BATTLERS, because they're only ever used together to refer to all the battlers at once.
#define F_PAL_BATTLERS_2  (1 << 7 | 1 << 8 | 1 << 9 | 1 << 10)

enum { SHAKE_BG_X, SHAKE_BG_Y, SHAKE_MON_X, SHAKE_MON_Y };
enum { SHAKE_MON_ATTACKER, SHAKE_MON_TARGET, SHAKE_MON_BOTH };

#endif // GUARD_CONSTANTS_BATTLE_ANIM_H
