#include "config.h"

#ifndef GUARD_CONSTANTS_TMS_HMS_H
#define GUARD_CONSTANTS_TMS_HMS_H

# ifdef PIT_GEN_3_MODE
#define FOREACH_TM(F) \
    F(FOCUS_PUNCH) \
    F(DRAGON_CLAW) \
    F(WATER_PULSE) \
    F(CALM_MIND) \
    F(ROAR) \
    F(TOXIC) \
    F(HAIL) \
    F(BULK_UP) \
    F(BULLET_SEED) \
    F(HIDDEN_POWER) \
    F(SUNNY_DAY) \
    F(TAUNT) \
    F(ICE_BEAM) \
    F(BLIZZARD) \
    F(HYPER_BEAM) \
    F(LIGHT_SCREEN) \
    F(PROTECT) \
    F(RAIN_DANCE) \
    F(GIGA_DRAIN) \
    F(SAFEGUARD) \
    F(FRUSTRATION) \
    F(SOLAR_BEAM) \
    F(IRON_TAIL) \
    F(THUNDERBOLT) \
    F(THUNDER) \
    F(EARTHQUAKE) \
    F(RETURN) \
    F(DIG) \
    F(PSYCHIC) \
    F(SHADOW_BALL) \
    F(BRICK_BREAK) \
    F(DOUBLE_TEAM) \
    F(REFLECT) \
    F(SHOCK_WAVE) \
    F(FLAMETHROWER) \
    F(SLUDGE_BOMB) \
    F(SANDSTORM) \
    F(FIRE_BLAST) \
    F(ROCK_TOMB) \
    F(AERIAL_ACE) \
    F(TORMENT) \
    F(FACADE) \
    F(SECRET_POWER) \
    F(REST) \
    F(ATTRACT) \
    F(THIEF) \
    F(STEEL_WING) \
    F(SKILL_SWAP) \
    F(SNATCH) \
    F(OVERHEAT) \
    F(DOUBLE_EDGE) \
    F(DYNAMIC_PUNCH) \
    F(EXPLOSION) \
    F(FURY_CUTTER) \
    F(METRONOME) \
    F(MIMIC) \
    F(ROLLOUT) \
    F(SLEEP_TALK) \
    F(SUBSTITUTE) \
    F(SWAGGER) \
    F(SOFTBOILED) \
    F(SEISMIC_TOSS) \
    F(DREAM_EATER) \
    F(MEGA_PUNCH) \
    F(MEGA_KICK) \
    F(BODY_SLAM) \
    F(ROCK_SLIDE) \
    F(COUNTER) \
    F(THUNDER_WAVE) \
    F(SWORDS_DANCE) \
    F(DEFENSE_CURL) \
    F(SNORE) \
    F(MUD_SLAP) \
    F(SWIFT) \
    F(ICY_WIND) \
    F(ENDURE) \
    F(PSYCH_UP) \
    F(ICE_PUNCH) \
    F(THUNDER_PUNCH) \
    F(FIRE_PUNCH)
# elif defined PIT_GEN_5_MODE
#define FOREACH_TM(F) \
    F(HONE_CLAWS) \
    F(DRAGON_CLAW) \
    F(PSYSHOCK) \
    F(CALM_MIND) \
    F(ROAR) \
    F(TOXIC) \
    F(HAIL) \
    F(BULK_UP) \
    F(VENOSHOCK) \
    F(HIDDEN_POWER) \
    F(SUNNY_DAY) \
    F(TAUNT) \
    F(ICE_BEAM) \
    F(BLIZZARD) \
    F(HYPER_BEAM) \
    F(LIGHT_SCREEN) \
    F(PROTECT) \
    F(RAIN_DANCE) \
    F(TELEKINESIS) \
    F(SAFEGUARD) \
    F(FRUSTRATION) \
    F(SOLAR_BEAM) \
    F(SMACK_DOWN) \
    F(THUNDERBOLT) \
    F(THUNDER) \
    F(EARTHQUAKE) \
    F(RETURN) \
    F(DIG) \
    F(PSYCHIC) \
    F(SHADOW_BALL) \
    F(BRICK_BREAK) \
    F(DOUBLE_TEAM) \
    F(REFLECT) \
    F(SLUDGE_WAVE) \
    F(FLAMETHROWER) \
    F(SLUDGE_BOMB) \
    F(SANDSTORM) \
    F(FIRE_BLAST) \
    F(ROCK_TOMB) \
    F(AERIAL_ACE) \
    F(TORMENT) \
    F(FACADE) \
    F(FLAME_CHARGE) \
    F(REST) \
    F(ATTRACT) \
    F(THIEF) \
    F(LOW_SWEEP) \
    F(ROUND) \
    F(ECHOED_VOICE) \
    F(OVERHEAT) \
    F(ALLY_SWITCH) \
    F(FOCUS_BLAST) \
    F(ENERGY_BALL) \
    F(FALSE_SWIPE) \
    F(SCALD) \
    F(FLING) \
    F(CHARGE_BEAM) \
    F(SKY_DROP) \
    F(INCINERATE) \
    F(QUASH) \
    F(WILL_O_WISP) \
    F(ACROBATICS) \
    F(EMBARGO) \
    F(EXPLOSION) \
    F(SHADOW_CLAW) \
    F(PAYBACK) \
    F(RETALIATE) \
    F(GIGA_IMPACT) \
    F(ROCK_POLISH) \
    F(FOCUS_PUNCH) \
    F(STONE_EDGE) \
    F(VOLT_SWITCH) \
    F(THUNDER_WAVE) \
    F(GYRO_BALL) \
    F(SWORDS_DANCE) \
    F(STRUGGLE_BUG) \
    F(PSYCH_UP) \
    F(BULLDOZE) \
    F(FROST_BREATH) \
    F(ROCK_SLIDE) \
    F(X_SCISSOR) \
    F(DRAGON_TAIL) \
    F(WORK_UP) \
    F(POISON_JAB) \
    F(DREAM_EATER) \
    F(GRASS_KNOT) \
    F(SWAGGER) \
    F(PLUCK) \
    F(U_TURN) \
    F(SUBSTITUTE) \
    F(FLASH_CANNON) \
    F(TRICK_ROOM) \
    F(WILD_CHARGE) \
    F(WATER_PULSE) \
    F(SNARL) \
    F(BULLET_SEED) \
    F(GIGA_DRAIN) \
    F(IRON_TAIL) \
    F(SHOCK_WAVE) \
    F(SKILL_SWAP)
# else
#define FOREACH_TM(F) \
    F(TAKE_DOWN) \
    F(CHARM) \
    F(FAKE_TEARS) \
    F(AGILITY) \
    F(MUD_SLAP) \
    F(SCARY_FACE) \
    F(PROTECT) \
    F(FIRE_FANG) \
    F(THUNDER_FANG) \
    F(ICE_FANG) \
    F(WATER_PULSE) \
    F(LOW_KICK) \
    F(ACID_SPRAY) \
    F(ACROBATICS) \
    F(STRUGGLE_BUG) \
    F(PSYBEAM) \
    F(CONFUSE_RAY) \
    F(THIEF) \
    F(DISARMING_VOICE) \
    F(TRAILBLAZE) \
    F(POUNCE) \
    F(CHILLING_WATER) \
    F(CHARGE_BEAM) \
    F(FIRE_SPIN) \
    F(FACADE) \
    F(POISON_TAIL) \
    F(AERIAL_ACE) \
    F(BULLDOZE) \
    F(HEX) \
    F(SNARL) \
    F(METAL_CLAW) \
    F(SWIFT) \
    F(MAGICAL_LEAF) \
    F(ICY_WIND) \
    F(MUD_SHOT) \
    F(ROCK_TOMB) \
    F(DRAINING_KISS) \
    F(FLAME_CHARGE) \
    F(LOW_SWEEP) \
    F(AIR_CUTTER) \
    F(STORED_POWER) \
    F(NIGHT_SHADE) \
    F(FLING) \
    F(DRAGON_TAIL) \
    F(VENOSHOCK) \
    F(AVALANCHE) \
    F(ENDURE) \
    F(VOLT_SWITCH) \
    F(SUNNY_DAY) \
    F(RAIN_DANCE) \
    F(SANDSTORM) \
    F(SNOWSCAPE) \
    F(SMART_STRIKE) \
    F(PSYSHOCK) \
    F(DIG) \
    F(BULLET_SEED) \
    F(FALSE_SWIPE) \
    F(BRICK_BREAK) \
    F(ZEN_HEADBUTT) \
    F(U_TURN) \
    F(SHADOW_CLAW) \
    F(FOUL_PLAY) \
    F(PSYCHIC_FANGS) \
    F(BULK_UP) \
    F(AIR_SLASH) \
    F(BODY_SLAM) \
    F(FIRE_PUNCH) \
    F(THUNDER_PUNCH) \
    F(ICE_PUNCH) \
    F(SLEEP_TALK) \
    F(SEED_BOMB) \
    F(ELECTRO_BALL) \
    F(DRAIN_PUNCH) \
    F(REFLECT) \
    F(LIGHT_SCREEN) \
    F(ROCK_BLAST) \
    F(WATERFALL) \
    F(DRAGON_CLAW) \
    F(DAZZLING_GLEAM) \
    F(METRONOME) \
    F(GRASS_KNOT) \
    F(THUNDER_WAVE) \
    F(POISON_JAB) \
    F(STOMPING_TANTRUM) \
    F(REST) \
    F(ROCK_SLIDE) \
    F(TAUNT) \
    F(SWORDS_DANCE) \
    F(BODY_PRESS) \
    F(SPIKES) \
    F(TOXIC_SPIKES) \
    F(IMPRISON) \
    F(FLASH_CANNON) \
    F(DARK_PULSE) \
    F(LEECH_LIFE) \
    F(EERIE_IMPULSE) \
    F(FLY) \
    F(SKILL_SWAP) \
    F(IRON_HEAD) \
    F(DRAGON_DANCE) \
    F(POWER_GEM) \
    F(GUNK_SHOT) \
    F(SUBSTITUTE) \
    F(IRON_DEFENSE) \
    F(X_SCISSOR) \
    F(DRILL_RUN) \
    F(WILL_O_WISP) \
    F(CRUNCH) \
    F(TRICK) \
    F(LIQUIDATION) \
    F(GIGA_DRAIN) \
    F(AURA_SPHERE) \
    F(TAILWIND) \
    F(SHADOW_BALL) \
    F(DRAGON_PULSE) \
    F(STEALTH_ROCK) \
    F(HYPER_VOICE) \
    F(HEAT_WAVE) \
    F(ENERGY_BALL) \
    F(PSYCHIC) \
    F(HEAVY_SLAM) \
    F(ENCORE) \
    F(SURF) \
    F(ICE_SPINNER) \
    F(FLAMETHROWER) \
    F(THUNDERBOLT) \
    F(PLAY_ROUGH) \
    F(AMNESIA) \
    F(CALM_MIND) \
    F(HELPING_HAND) \
    F(POLLEN_PUFF) \
    F(BATON_PASS) \
    F(EARTH_POWER) \
    F(REVERSAL) \
    F(ICE_BEAM) \
    F(ELECTRIC_TERRAIN) \
    F(GRASSY_TERRAIN) \
    F(PSYCHIC_TERRAIN) \
    F(MISTY_TERRAIN) \
    F(NASTY_PLOT) \
    F(FIRE_BLAST) \
    F(HYDRO_PUMP) \
    F(BLIZZARD) \
    F(FIRE_PLEDGE) \
    F(WATER_PLEDGE) \
    F(GRASS_PLEDGE) \
    F(WILD_CHARGE) \
    F(SLUDGE_BOMB) \
    F(EARTHQUAKE) \
    F(STONE_EDGE) \
    F(PHANTOM_FORCE) \
    F(GIGA_IMPACT) \
    F(BLAST_BURN) \
    F(HYDRO_CANNON) \
    F(FRENZY_PLANT) \
    F(OUTRAGE) \
    F(OVERHEAT) \
    F(FOCUS_BLAST) \
    F(LEAF_STORM) \
    F(HURRICANE) \
    F(TRICK_ROOM) \
    F(BUG_BUZZ) \
    F(HYPER_BEAM) \
    F(BRAVE_BIRD) \
    F(FLARE_BLITZ) \
    F(THUNDER) \
    F(CLOSE_COMBAT) \
    F(SOLAR_BEAM) \
    F(DRACO_METEOR) \
    F(STEEL_BEAM) \
    F(TERA_BLAST) \
    F(ROAR) \
    F(CHARGE) \
    F(HAZE) \
    F(TOXIC) \
    F(SAND_TOMB) \
    F(SPITE) \
    F(GRAVITY) \
    F(SMACK_DOWN) \
    F(GYRO_BALL) \
    F(KNOCK_OFF) \
    F(BUG_BITE) \
    F(SUPER_FANG) \
    F(VACUUM_WAVE) \
    F(LUNGE) \
    F(HIGH_HORSEPOWER) \
    F(ICICLE_SPEAR) \
    F(SCALD) \
    F(HEAT_CRASH) \
    F(SOLAR_BLADE) \
    F(UPROAR) \
    F(FOCUS_PUNCH) \
    F(WEATHER_BALL) \
    F(GRASSY_GLIDE) \
    F(BURNING_JEALOUSY) \
    F(FLIP_TURN) \
    F(DUAL_WINGBEAT) \
    F(POLTERGEIST) \
    F(LASH_OUT) \
    F(SCALE_SHOT) \
    F(MISTY_EXPLOSION) \
    F(PAIN_SPLIT) \
    F(PSYCH_UP) \
    F(DOUBLE_EDGE) \
    F(ENDEAVOR) \
    F(PETAL_BLIZZARD) \
    F(TEMPER_FLARE) \
    F(WHIRLPOOL) \
    F(MUDDY_WATER) \
    F(SUPERCELL_SLAM) \
    F(ELECTROWEB) \
    F(TRIPLE_AXEL) \
    F(COACHING) \
    F(SLUDGE_WAVE) \
    F(SCORCHING_SANDS) \
    F(FEATHER_DANCE) \
    F(FUTURE_SIGHT) \
    F(EXPANDING_FORCE) \
    F(SKITTER_SMACK) \
    F(METEOR_BEAM) \
    F(THROAT_CHOP) \
    F(BREAKING_SWIPE) \
    F(METAL_SOUND) \
    F(CURSE) \
    F(HARD_PRESS) \
    F(DRAGON_CHEER) \
    F(ALLURING_VOICE) \
    F(PSYCHIC_NOISE) \
    F(UPPER_HAND) \
    F(HIDDEN_POWER)
# endif

#define FOREACH_HM(F) \
    F(CUT) \
    F(FLY) \
    F(SURF) \
    F(STRENGTH) \
    F(FLASH) \
    F(ROCK_SMASH) \
    F(WATERFALL) \
    F(DIVE)

#define FOREACH_TMHM(F) \
    FOREACH_TM(F) \
    FOREACH_HM(F)

#endif
