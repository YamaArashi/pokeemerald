CASTFORMGFXDIR := graphics/pokemon/castform
TILESETGFXDIR := data/tilesets
FONTGFXDIR := graphics/fonts
INTERFACEGFXDIR := graphics/interface
BTLANMSPRGFXDIR := graphics/battle_anims/sprites
UNUSEDGFXDIR := graphics/unused
UNKNOWNGFXDIR := graphics/unknown
BATINTGFXDIR := graphics/battle_interface
MASKSGFXDIR := graphics/battle_anims/masks
BATTRANSGFXDIR := graphics/battle_transitions
TYPESGFXDIR := graphics/types
RAYQUAZAGFXDIR := graphics/rayquaza_scene
ROULETTEGFXDIR := graphics/roulette
SLOTMACHINEGFXDIR := graphics/slot_machine
PKNAVGFXDIR := graphics/pokenav
PKNAVOPTIONSGFXDIR := graphics/pokenav/options
WALLPAPERGFXDIR := graphics/pokemon_storage/wallpapers
OBJEVENTGFXDIR := graphics/object_events
MISCGFXDIR := graphics/misc
JPCONTESTGFXDIR := graphics/contest/japanese
POKEDEXGFXDIR := graphics/pokedex
STARTERGFXDIR := graphics/starter_choose
NAMINGGFXDIR := graphics/naming_screen
SPINDAGFXDIR := graphics/pokemon/spinda/spots
TITLESCREENGFXDIR := graphics/title_screen

types := normal fight flying poison ground rock bug ghost steel mystery fire water grass electric psychic ice dragon dark
contest_types := cool beauty cute smart tough



### Castform ###

$(CASTFORMGFXDIR)/front.4bpp: $(CASTFORMGFXDIR)/normal/front.4bpp \
                              $(CASTFORMGFXDIR)/sunny/front.4bpp \
                              $(CASTFORMGFXDIR)/rainy/front.4bpp \
                              $(CASTFORMGFXDIR)/snowy/front.4bpp
	@cat $^ >$@

$(CASTFORMGFXDIR)/back.4bpp: $(CASTFORMGFXDIR)/normal/back.4bpp \
                             $(CASTFORMGFXDIR)/sunny/back.4bpp \
                             $(CASTFORMGFXDIR)/rainy/back.4bpp \
                             $(CASTFORMGFXDIR)/snowy/back.4bpp
	@cat $^ >$@

$(CASTFORMGFXDIR)/anim_front.4bpp: $(CASTFORMGFXDIR)/normal/anim_front.4bpp \
                                   $(CASTFORMGFXDIR)/sunny/anim_front.4bpp \
                                   $(CASTFORMGFXDIR)/rainy/anim_front.4bpp \
                                   $(CASTFORMGFXDIR)/snowy/anim_front.4bpp
	@cat $^ >$@

$(CASTFORMGFXDIR)/normal.gbapal: $(CASTFORMGFXDIR)/normal/normal.gbapal \
                                 $(CASTFORMGFXDIR)/sunny/normal.gbapal \
                                 $(CASTFORMGFXDIR)/rainy/normal.gbapal \
                                 $(CASTFORMGFXDIR)/snowy/normal.gbapal
	@cat $^ >$@

$(CASTFORMGFXDIR)/shiny.gbapal: $(CASTFORMGFXDIR)/normal/shiny.gbapal \
                                $(CASTFORMGFXDIR)/sunny/shiny.gbapal \
                                $(CASTFORMGFXDIR)/rainy/shiny.gbapal \
                                $(CASTFORMGFXDIR)/snowy/shiny.gbapal
	@cat $^ >$@



### Tilesets ###

$(TILESETGFXDIR)/secondary/petalburg/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 159 -Wnum_tiles

$(TILESETGFXDIR)/secondary/rustboro/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 498 -Wnum_tiles

$(TILESETGFXDIR)/secondary/dewford/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 503 -Wnum_tiles

$(TILESETGFXDIR)/secondary/slateport/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 504 -Wnum_tiles

$(TILESETGFXDIR)/secondary/mauville/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 503 -Wnum_tiles

$(TILESETGFXDIR)/secondary/lavaridge/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 450 -Wnum_tiles

$(TILESETGFXDIR)/secondary/fortree/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 493 -Wnum_tiles

$(TILESETGFXDIR)/secondary/pacifidlog/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 504 -Wnum_tiles

$(TILESETGFXDIR)/secondary/sootopolis/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 328 -Wnum_tiles

$(TILESETGFXDIR)/secondary/battle_frontier_outside_west/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 508 -Wnum_tiles

$(TILESETGFXDIR)/secondary/battle_frontier_outside_east/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 508 -Wnum_tiles

$(TILESETGFXDIR)/primary/building/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 502 -Wnum_tiles

$(TILESETGFXDIR)/secondary/shop/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 502 -Wnum_tiles

$(TILESETGFXDIR)/secondary/pokemon_center/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 478 -Wnum_tiles

$(TILESETGFXDIR)/secondary/cave/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 425 -Wnum_tiles

$(TILESETGFXDIR)/secondary/pokemon_school/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 278 -Wnum_tiles

$(TILESETGFXDIR)/secondary/pokemon_fan_club/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 319 -Wnum_tiles

$(TILESETGFXDIR)/secondary/unused_1/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 17 -Wnum_tiles

$(TILESETGFXDIR)/secondary/meteor_falls/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 460 -Wnum_tiles

$(TILESETGFXDIR)/secondary/oceanic_museum/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 319 -Wnum_tiles

$(TILESETGFXDIR)/secondary/cable_club/unknown_tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 120 -Wnum_tiles

$(TILESETGFXDIR)/secondary/seashore_house/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 312 -Wnum_tiles

$(TILESETGFXDIR)/secondary/pretty_petal_flower_shop/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 345 -Wnum_tiles

$(TILESETGFXDIR)/secondary/pokemon_day_care/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 355 -Wnum_tiles

$(TILESETGFXDIR)/secondary/secret_base/brown_cave/unused_tiles.4bpp: $(TILESETGFXDIR)/secondary/secret_base/brown_cave/tiles.png
	$(GFX) $< $@ -num_tiles 82 -Wnum_tiles

$(TILESETGFXDIR)/secondary/secret_base/tree/unused_tiles.4bpp: $(TILESETGFXDIR)/secondary/secret_base/tree/tiles.png
	$(GFX) $< $@ -num_tiles 82 -Wnum_tiles

$(TILESETGFXDIR)/secondary/secret_base/shrub/unused_tiles.4bpp: $(TILESETGFXDIR)/secondary/secret_base/shrub/tiles.png
	$(GFX) $< $@ -num_tiles 82 -Wnum_tiles

$(TILESETGFXDIR)/secondary/secret_base/blue_cave/unused_tiles.4bpp: $(TILESETGFXDIR)/secondary/secret_base/blue_cave/tiles.png
	$(GFX) $< $@ -num_tiles 82 -Wnum_tiles

$(TILESETGFXDIR)/secondary/secret_base/yellow_cave/unused_tiles.4bpp: $(TILESETGFXDIR)/secondary/secret_base/yellow_cave/tiles.png
	$(GFX) $< $@ -num_tiles 82 -Wnum_tiles

$(TILESETGFXDIR)/secondary/secret_base/red_cave/unused_tiles.4bpp: $(TILESETGFXDIR)/secondary/secret_base/red_cave/tiles.png
	$(GFX) $< $@ -num_tiles 82 -Wnum_tiles

$(TILESETGFXDIR)/secondary/secret_base/brown_cave/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 83 -Wnum_tiles

$(TILESETGFXDIR)/secondary/secret_base/tree/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 83 -Wnum_tiles

$(TILESETGFXDIR)/secondary/secret_base/shrub/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 83 -Wnum_tiles

$(TILESETGFXDIR)/secondary/secret_base/blue_cave/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 83 -Wnum_tiles

$(TILESETGFXDIR)/secondary/secret_base/yellow_cave/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 83 -Wnum_tiles

$(TILESETGFXDIR)/secondary/secret_base/red_cave/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 83 -Wnum_tiles

$(TILESETGFXDIR)/secondary/inside_of_truck/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 62 -Wnum_tiles

$(TILESETGFXDIR)/secondary/contest/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 430 -Wnum_tiles

$(TILESETGFXDIR)/secondary/lilycove_museum/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 431 -Wnum_tiles

$(TILESETGFXDIR)/secondary/lab/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 500 -Wnum_tiles

$(TILESETGFXDIR)/secondary/underwater/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 500 -Wnum_tiles

$(TILESETGFXDIR)/secondary/generic_building/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 509 -Wnum_tiles

$(TILESETGFXDIR)/secondary/mauville_game_corner/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 469 -Wnum_tiles

$(TILESETGFXDIR)/secondary/unused_2/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 150 -Wnum_tiles

$(TILESETGFXDIR)/secondary/rustboro_gym/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 60 -Wnum_tiles

$(TILESETGFXDIR)/secondary/dewford_gym/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 61 -Wnum_tiles

$(TILESETGFXDIR)/secondary/lavaridge_gym/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 54 -Wnum_tiles

$(TILESETGFXDIR)/secondary/petalburg_gym/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 148 -Wnum_tiles

$(TILESETGFXDIR)/secondary/fortree_gym/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 61 -Wnum_tiles

$(TILESETGFXDIR)/secondary/mossdeep_gym/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 82 -Wnum_tiles

$(TILESETGFXDIR)/secondary/sootopolis_gym/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 484 -Wnum_tiles

$(TILESETGFXDIR)/secondary/trick_house_puzzle/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 294 -Wnum_tiles

$(TILESETGFXDIR)/secondary/inside_ship/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 342 -Wnum_tiles

$(TILESETGFXDIR)/secondary/elite_four/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 505 -Wnum_tiles

$(TILESETGFXDIR)/secondary/battle_frontier/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 310 -Wnum_tiles

$(TILESETGFXDIR)/secondary/battle_factory/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 424 -Wnum_tiles

$(TILESETGFXDIR)/secondary/battle_pike/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 382 -Wnum_tiles

$(TILESETGFXDIR)/secondary/mirage_tower/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 420 -Wnum_tiles

$(TILESETGFXDIR)/secondary/mossdeep_game_corner/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 95 -Wnum_tiles

$(TILESETGFXDIR)/secondary/island_harbor/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 503 -Wnum_tiles

$(TILESETGFXDIR)/secondary/trainer_hill/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 374 -Wnum_tiles

$(TILESETGFXDIR)/secondary/navel_rock/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 420 -Wnum_tiles

$(TILESETGFXDIR)/secondary/battle_frontier_ranking_hall/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 136 -Wnum_tiles

$(TILESETGFXDIR)/secondary/mystery_events_house/tiles.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 509 -Wnum_tiles



### Fonts ###

$(FONTGFXDIR)/small.latfont: $(FONTGFXDIR)/latin_small.png
	$(GFX) $< $@

$(FONTGFXDIR)/normal.latfont: $(FONTGFXDIR)/latin_normal.png
	$(GFX) $< $@

$(FONTGFXDIR)/short.latfont: $(FONTGFXDIR)/latin_short.png
	$(GFX) $< $@

$(FONTGFXDIR)/narrow.latfont: $(FONTGFXDIR)/latin_narrow.png
	$(GFX) $< $@

$(FONTGFXDIR)/small_narrow.latfont: $(FONTGFXDIR)/latin_small_narrow.png
	$(GFX) $< $@

$(FONTGFXDIR)/small.hwjpnfont: $(FONTGFXDIR)/japanese_small.png
	$(GFX) $< $@

$(FONTGFXDIR)/normal.hwjpnfont: $(FONTGFXDIR)/japanese_normal.png
	$(GFX) $< $@

$(FONTGFXDIR)/bold.hwjpnfont: $(FONTGFXDIR)/japanese_bold.png
	$(GFX) $< $@

$(FONTGFXDIR)/short.fwjpnfont: $(FONTGFXDIR)/japanese_short.png
	$(GFX) $< $@

$(FONTGFXDIR)/braille.fwjpnfont: $(FONTGFXDIR)/braille.png
	$(GFX) $< $@

$(FONTGFXDIR)/frlg_male.fwjpnfont: $(FONTGFXDIR)/japanese_frlg_male.png
	$(GFX) $< $@

$(FONTGFXDIR)/frlg_female.fwjpnfont: $(FONTGFXDIR)/japanese_frlg_female.png
	$(GFX) $< $@


### Miscellaneous ###

$(TITLESCREENGFXDIR)/pokemon_logo.gbapal: %.gbapal: %.pal
	$(GFX) $< $@ -num_colors 224

$(TITLESCREENGFXDIR)/emerald_version.8bpp: %.8bpp: %.png
	$(GFX) $< $@ -mwidth 8 -mheight 4

graphics/pokemon_jump/bg.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 63 -Wnum_tiles

graphics/pokenav/region_map/map.8bpp: %.8bpp: %.png
	$(GFX) $< $@ -num_tiles 233 -Wnum_tiles

$(MISCGFXDIR)/japanese_hof.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 29 -Wnum_tiles

$(BATINTGFXDIR)/textbox.gbapal: $(BATINTGFXDIR)/textbox_0.gbapal \
                                $(BATINTGFXDIR)/textbox_1.gbapal
	@cat $^ >$@

$(BTLANMSPRGFXDIR)/ice_cube.4bpp: $(BTLANMSPRGFXDIR)/ice_cube_0.4bpp \
                                  $(BTLANMSPRGFXDIR)/ice_cube_1.4bpp \
                                  $(BTLANMSPRGFXDIR)/ice_cube_2.4bpp \
                                  $(BTLANMSPRGFXDIR)/ice_cube_3.4bpp
	@cat $^ >$@

$(UNUSEDGFXDIR)/obi_palpak1.gbapal: $(UNUSEDGFXDIR)/old_pal1.gbapal \
                                    $(UNUSEDGFXDIR)/old_pal2.gbapal \
                                    $(UNUSEDGFXDIR)/old_pal3.gbapal
	@cat $^ >$@

$(UNUSEDGFXDIR)/obi_palpak3.gbapal: $(UNUSEDGFXDIR)/old_pal5.gbapal \
                                    $(UNUSEDGFXDIR)/old_pal6.gbapal \
                                    $(UNUSEDGFXDIR)/old_pal7.gbapal
	@cat $^ >$@

$(UNUSEDGFXDIR)/obi1.4bpp: $(UNUSEDGFXDIR)/old_bulbasaur.4bpp \
                           $(UNUSEDGFXDIR)/old_charizard.4bpp
	@cat $^ >$@

$(UNUSEDGFXDIR)/obi2.4bpp: $(UNUSEDGFXDIR)/old_bulbasaur2.4bpp \
                           $(UNUSEDGFXDIR)/old_battle_interface_1.4bpp \
                           $(UNUSEDGFXDIR)/old_battle_interface_2.4bpp \
                           $(UNUSEDGFXDIR)/old_battle_interface_3.4bpp
	@cat $^ >$@

$(BATINTGFXDIR)/battle_bar.4bpp: $(BATINTGFXDIR)/hpbar_anim_unused.4bpp \
                                 $(BATINTGFXDIR)/numbers1.4bpp \
                                 $(BATINTGFXDIR)/numbers2.4bpp
	@cat $^ >$@

$(UNUSEDGFXDIR)/redyellowgreen_frame.bin: $(UNUSEDGFXDIR)/red_frame.bin \
                                          $(UNUSEDGFXDIR)/yellow_frame.bin \
                                          $(UNUSEDGFXDIR)/green_frame.bin \
                                          $(UNUSEDGFXDIR)/blank_frame.bin
	@cat $^ >$@

$(UNUSEDGFXDIR)/color_frames.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 353 -Wnum_tiles

$(BATINTGFXDIR)/unused_window2bar.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 5 -Wnum_tiles

$(JPCONTESTGFXDIR)/composite_1.4bpp: $(JPCONTESTGFXDIR)/frame_1.4bpp \
                                     $(JPCONTESTGFXDIR)/floor.4bpp \
                                     $(JPCONTESTGFXDIR)/frame_2.4bpp \
                                     $(JPCONTESTGFXDIR)/symbols.4bpp \
                                     $(JPCONTESTGFXDIR)/meter.4bpp \
                                     $(JPCONTESTGFXDIR)/letters.4bpp \
                                     $(JPCONTESTGFXDIR)/numbers.4bpp
	@cat $^ >$@

$(JPCONTESTGFXDIR)/composite_2.4bpp: $(JPCONTESTGFXDIR)/interface.4bpp \
                                     $(JPCONTESTGFXDIR)/audience.4bpp
	@cat $^ >$@

$(JPCONTESTGFXDIR)/voltage.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 36 -Wnum_tiles

$(BTLANMSPRGFXDIR)/ice_crystals.4bpp: $(BTLANMSPRGFXDIR)/ice_crystals_0.4bpp \
                                      $(BTLANMSPRGFXDIR)/ice_crystals_1.4bpp \
                                      $(BTLANMSPRGFXDIR)/ice_crystals_2.4bpp \
                                      $(BTLANMSPRGFXDIR)/ice_crystals_3.4bpp \
                                      $(BTLANMSPRGFXDIR)/ice_crystals_4.4bpp
	@cat $^ >$@

$(BTLANMSPRGFXDIR)/mud_sand.4bpp: $(BTLANMSPRGFXDIR)/mud_sand_0.4bpp \
                                  $(BTLANMSPRGFXDIR)/mud_sand_1.4bpp
	@cat $^ >$@

$(BTLANMSPRGFXDIR)/flower.4bpp: $(BTLANMSPRGFXDIR)/flower_0.4bpp \
                                $(BTLANMSPRGFXDIR)/flower_1.4bpp
	@cat $^ >$@

$(BTLANMSPRGFXDIR)/spark.4bpp: $(BTLANMSPRGFXDIR)/spark_0.4bpp \
                               $(BTLANMSPRGFXDIR)/spark_1.4bpp
	@cat $^ >$@

$(MASKSGFXDIR)/unused_level_up.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 14 -Wnum_tiles

$(BATTRANSGFXDIR)/vs_frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 16 -Wnum_tiles

graphics/party_menu/bg.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 62 -Wnum_tiles

$(TYPESGFXDIR)/move_types.4bpp: $(types:%=$(TYPESGFXDIR)/%.4bpp) $(contest_types:%=$(TYPESGFXDIR)/contest_%.4bpp)
	@cat $^ >$@

$(TYPESGFXDIR)/move_types.gbapal: $(TYPESGFXDIR)/move_types_1.gbapal \
                                  $(TYPESGFXDIR)/move_types_2.gbapal \
                                  $(TYPESGFXDIR)/move_types_3.gbapal
	@cat $^ >$@

graphics/bag/menu.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 53 -Wnum_tiles

$(RAYQUAZAGFXDIR)/scene_2/rayquaza.8bpp: %.8bpp: %.png
	$(GFX) $< $@ -num_tiles 227 -Wnum_tiles

$(RAYQUAZAGFXDIR)/scene_2/bg.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 313 -Wnum_tiles

$(RAYQUAZAGFXDIR)/scene_3/rayquaza.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 124 -Wnum_tiles

$(RAYQUAZAGFXDIR)/scene_3/rayquaza_tail_fix.4bpp: $(RAYQUAZAGFXDIR)/scene_3/rayquaza_tail.4bpp
	cp $< $@
	dd if=/dev/zero bs=1 count=12 >> $@

$(RAYQUAZAGFXDIR)/scene_4/streaks.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 19 -Wnum_tiles

$(RAYQUAZAGFXDIR)/scene_4/rayquaza.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 155 -Wnum_tiles

graphics/picture_frame/lobby.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 86 -Wnum_tiles

$(ROULETTEGFXDIR)/roulette_tilt.4bpp: $(ROULETTEGFXDIR)/shroomish.4bpp \
                                      $(ROULETTEGFXDIR)/tailow.4bpp
	@cat $^ >$@

$(ROULETTEGFXDIR)/wheel_icons.4bpp: $(ROULETTEGFXDIR)/wynaut.4bpp \
                                    $(ROULETTEGFXDIR)/azurill.4bpp \
                                    $(ROULETTEGFXDIR)/skitty.4bpp \
                                    $(ROULETTEGFXDIR)/makuhita.4bpp
	@cat $^ >$@

$(BATTRANSGFXDIR)/regis.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 53 -Wnum_tiles

$(BATTRANSGFXDIR)/rayquaza.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 938 -Wnum_tiles

$(BATTRANSGFXDIR)/frontier_square_1.4bpp: $(BATTRANSGFXDIR)/frontier_squares_blanktiles.4bpp \
                                          $(BATTRANSGFXDIR)/frontier_squares_1.4bpp
	@cat $^ >$@

$(BATTRANSGFXDIR)/frontier_square_2.4bpp: $(BATTRANSGFXDIR)/frontier_squares_blanktiles.4bpp \
                                          $(BATTRANSGFXDIR)/frontier_squares_2.4bpp
	@cat $^ >$@

$(BATTRANSGFXDIR)/frontier_square_3.4bpp: $(BATTRANSGFXDIR)/frontier_squares_blanktiles.4bpp \
                                          $(BATTRANSGFXDIR)/frontier_squares_3.4bpp
	@cat $^ >$@

$(BATTRANSGFXDIR)/frontier_square_4.4bpp: $(BATTRANSGFXDIR)/frontier_squares_blanktiles.4bpp \
                                          $(BATTRANSGFXDIR)/frontier_squares_4.4bpp
	@cat $^ >$@

$(SLOTMACHINEGFXDIR)/reel_time_gfx.4bpp: $(SLOTMACHINEGFXDIR)/reel_time_pikachu.4bpp \
                                         $(SLOTMACHINEGFXDIR)/reel_time_machine.4bpp
	@cat $^ >$@

graphics/birch_speech/unused_beauty.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 822 -Wnum_tiles



### Pokémon Storage System ###

$(WALLPAPERGFXDIR)/forest/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 55 -Wnum_tiles

$(WALLPAPERGFXDIR)/forest/tiles.4bpp: $(WALLPAPERGFXDIR)/forest/frame.4bpp $(WALLPAPERGFXDIR)/forest/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/city/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 52 -Wnum_tiles

$(WALLPAPERGFXDIR)/city/tiles.4bpp: $(WALLPAPERGFXDIR)/city/frame.4bpp $(WALLPAPERGFXDIR)/city/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/desert/tiles.4bpp: $(WALLPAPERGFXDIR)/desert/frame.4bpp $(WALLPAPERGFXDIR)/desert/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/savanna/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 45 -Wnum_tiles

$(WALLPAPERGFXDIR)/savanna/bg.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 23 -Wnum_tiles

$(WALLPAPERGFXDIR)/savanna/tiles.4bpp: $(WALLPAPERGFXDIR)/savanna/frame.4bpp $(WALLPAPERGFXDIR)/savanna/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/crag/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 49 -Wnum_tiles

$(WALLPAPERGFXDIR)/crag/tiles.4bpp: $(WALLPAPERGFXDIR)/crag/frame.4bpp $(WALLPAPERGFXDIR)/crag/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/volcano/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 56 -Wnum_tiles

$(WALLPAPERGFXDIR)/volcano/tiles.4bpp: $(WALLPAPERGFXDIR)/volcano/frame.4bpp $(WALLPAPERGFXDIR)/volcano/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/snow/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 57 -Wnum_tiles

$(WALLPAPERGFXDIR)/snow/tiles.4bpp: $(WALLPAPERGFXDIR)/snow/frame.4bpp $(WALLPAPERGFXDIR)/snow/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/cave/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 55 -Wnum_tiles

$(WALLPAPERGFXDIR)/cave/tiles.4bpp: $(WALLPAPERGFXDIR)/cave/frame.4bpp $(WALLPAPERGFXDIR)/cave/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/beach/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 46 -Wnum_tiles

$(WALLPAPERGFXDIR)/beach/bg.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 23 -Wnum_tiles

$(WALLPAPERGFXDIR)/beach/tiles.4bpp: $(WALLPAPERGFXDIR)/beach/frame.4bpp $(WALLPAPERGFXDIR)/beach/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/seafloor/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 54 -Wnum_tiles

$(WALLPAPERGFXDIR)/seafloor/tiles.4bpp: $(WALLPAPERGFXDIR)/seafloor/frame.4bpp $(WALLPAPERGFXDIR)/seafloor/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/river/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 51 -Wnum_tiles

$(WALLPAPERGFXDIR)/river/bg.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 11 -Wnum_tiles

$(WALLPAPERGFXDIR)/river/tiles.4bpp: $(WALLPAPERGFXDIR)/river/frame.4bpp $(WALLPAPERGFXDIR)/river/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/sky/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 45 -Wnum_tiles

$(WALLPAPERGFXDIR)/sky/tiles.4bpp: $(WALLPAPERGFXDIR)/sky/frame.4bpp $(WALLPAPERGFXDIR)/sky/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/polkadot/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 54 -Wnum_tiles

$(WALLPAPERGFXDIR)/polkadot/tiles.4bpp: $(WALLPAPERGFXDIR)/polkadot/frame.4bpp $(WALLPAPERGFXDIR)/polkadot/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/pokecenter/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 35 -Wnum_tiles

$(WALLPAPERGFXDIR)/pokecenter/tiles.4bpp: $(WALLPAPERGFXDIR)/pokecenter/frame.4bpp $(WALLPAPERGFXDIR)/pokecenter/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/machine/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 33 -Wnum_tiles

$(WALLPAPERGFXDIR)/machine/tiles.4bpp: $(WALLPAPERGFXDIR)/machine/frame.4bpp $(WALLPAPERGFXDIR)/machine/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/plain/frame.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 18 -Wnum_tiles

$(WALLPAPERGFXDIR)/plain/tiles.4bpp: $(WALLPAPERGFXDIR)/plain/frame.4bpp $(WALLPAPERGFXDIR)/plain/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/friends_frame1.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 57 -Wnum_tiles

$(WALLPAPERGFXDIR)/friends_frame2.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 57 -Wnum_tiles

$(WALLPAPERGFXDIR)/zigzagoon/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame1.4bpp $(WALLPAPERGFXDIR)/zigzagoon/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/screen/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame1.4bpp $(WALLPAPERGFXDIR)/screen/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/horizontal/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame1.4bpp $(WALLPAPERGFXDIR)/horizontal/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/diagonal/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame1.4bpp $(WALLPAPERGFXDIR)/diagonal/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/block/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame1.4bpp $(WALLPAPERGFXDIR)/block/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/ribbon/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame1.4bpp $(WALLPAPERGFXDIR)/ribbon/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/pokecenter2/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame1.4bpp $(WALLPAPERGFXDIR)/pokecenter2/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/frame/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame1.4bpp $(WALLPAPERGFXDIR)/frame/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/blank/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame1.4bpp $(WALLPAPERGFXDIR)/blank/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/circles/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame1.4bpp $(WALLPAPERGFXDIR)/circles/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/azumarill/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame2.4bpp $(WALLPAPERGFXDIR)/azumarill/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/pikachu/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame2.4bpp $(WALLPAPERGFXDIR)/pikachu/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/legendary/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame2.4bpp $(WALLPAPERGFXDIR)/legendary/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/dusclops/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame2.4bpp $(WALLPAPERGFXDIR)/dusclops/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/ludicolo/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame2.4bpp $(WALLPAPERGFXDIR)/ludicolo/bg.4bpp
	@cat $^ >$@

$(WALLPAPERGFXDIR)/whiscash/tiles.4bpp: $(WALLPAPERGFXDIR)/friends_frame2.4bpp $(WALLPAPERGFXDIR)/whiscash/bg.4bpp
	@cat $^ >$@

$(INTERFACEGFXDIR)/outline_cursor.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 8 -Wnum_tiles

$(BATTRANSGFXDIR)/frontier_logo_center.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 43 -Wnum_tiles



### Pokenav ###

$(PKNAVOPTIONSGFXDIR)/options.4bpp: $(PKNAVOPTIONSGFXDIR)/hoenn_map.4bpp \
                                    $(PKNAVOPTIONSGFXDIR)/condition.4bpp \
                                    $(PKNAVOPTIONSGFXDIR)/match_call.4bpp \
                                    $(PKNAVOPTIONSGFXDIR)/ribbons.4bpp \
                                    $(PKNAVOPTIONSGFXDIR)/switch_off.4bpp \
                                    $(PKNAVOPTIONSGFXDIR)/party.4bpp \
                                    $(PKNAVOPTIONSGFXDIR)/search.4bpp \
                                    $(PKNAVOPTIONSGFXDIR)/cool.4bpp \
                                    $(PKNAVOPTIONSGFXDIR)/beauty.4bpp \
                                    $(PKNAVOPTIONSGFXDIR)/cute.4bpp \
                                    $(PKNAVOPTIONSGFXDIR)/smart.4bpp \
                                    $(PKNAVOPTIONSGFXDIR)/tough.4bpp \
                                    $(PKNAVOPTIONSGFXDIR)/cancel.4bpp
	@cat $^ >$@

$(PKNAVGFXDIR)/header.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 53 -Wnum_tiles

$(PKNAVGFXDIR)/device_outline.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 53 -Wnum_tiles

$(PKNAVGFXDIR)/match_call/ui.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 13 -Wnum_tiles

$(POKEDEXGFXDIR)/region_map.8bpp: %.8bpp: %.png
	$(GFX) $< $@ -num_tiles 232 -Wnum_tiles

$(POKEDEXGFXDIR)/region_map_affine.8bpp: %.8bpp: %.png
	$(GFX) $< $@ -num_tiles 233 -Wnum_tiles

$(NAMINGGFXDIR)/cursor.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 5 -Wnum_tiles

$(NAMINGGFXDIR)/cursor_squished.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 5 -Wnum_tiles

$(NAMINGGFXDIR)/cursor_filled.4bpp: %.4bpp: %.png
	$(GFX) $< $@ -num_tiles 5 -Wnum_tiles

$(SPINDAGFXDIR)/spot_0.1bpp: %.1bpp: %.png
	$(GFX) $< $@ -plain -data_width 2

$(SPINDAGFXDIR)/spot_1.1bpp: %.1bpp: %.png
	$(GFX) $< $@ -plain -data_width 2

$(SPINDAGFXDIR)/spot_2.1bpp: %.1bpp: %.png
	$(GFX) $< $@ -plain -data_width 2

$(SPINDAGFXDIR)/spot_3.1bpp: %.1bpp: %.png
	$(GFX) $< $@ -plain -data_width 2
