@ commands

	.macro loadspritegfx tag:req
	.byte 0x0
	.2byte \tag
	.endm

	.macro unloadspritegfx tag:req
	.byte 0x1
	.2byte \tag
	.endm

	.macro createsprite template:req, anim_battler:req, subpriority_offset:req, argv:vararg
	.byte 0x02
	.4byte \template
	.if \anim_battler == ANIM_TARGET
	.byte ANIMSPRITE_IS_TARGET | (\subpriority_offset & 0x7F)
	.else
	.byte (\subpriority_offset & 0x7F)
	.endif
	.byte (.Lsprite_\@_2 - .Lsprite_\@_1) / 2
.Lsprite_\@_1:
	.2byte \argv
.Lsprite_\@_2:
	.endm

	.macro createvisualtask addr:req, priority:req, argv:vararg
	.byte 0x03
	.4byte \addr
	.byte \priority
	.byte (.Lcreatetask_\@_2 - .Lcreatetask_\@_1) / 2
.Lcreatetask_\@_1:
	.2byte \argv
.Lcreatetask_\@_2:
	.endm

	.macro delay frames:req
	.byte 0x4
	.byte \frames
	.endm

	.macro waitforvisualfinish
	.byte 0x5
	.endm

	.macro nop
	.byte 0x6
	.endm

	.macro nop2
	.byte 0x7
	.endm

	.macro end
	.byte 0x8
	.endm

	.macro playse se:req
	.byte 0x9
	.2byte \se
	.endm

	.macro monbg battler:req
	.byte 0xa
	.byte \battler
	.endm

	.macro clearmonbg battler:req
	.byte 0xb
	.byte \battler
	.endm

	.macro setalpha eva:req, evb:req
	.byte 0x0C
	.2byte ((\evb) << 8) | (\eva)
	.endm

	.macro blendoff
	.byte 0xd
	.endm

	.macro call ptr:req
	.byte 0xe
	.4byte \ptr
	.endm

	.macro return
	.byte 0xf
	.endm

	.macro setarg argId:req, value:req
	.byte 0x10
	.byte \argId
	.2byte \value
	.endm

	.macro choosetwoturnanim ptr1:req, ptr2:req
	.byte 0x11
	.4byte \ptr1
	.4byte \ptr2
	.endm

	.macro jumpifmoveturn value:req, ptr:req
	.byte 0x12
	.byte \value
	.4byte \ptr
	.endm

	.macro goto ptr:req
	.byte 0x13
	.4byte \ptr
	.endm

	.macro fadetobg bg:req
	.byte 0x14
	.byte \bg
	.endm

	.macro restorebg
	.byte 0x15
	.endm

	.macro waitbgfadeout
	.byte 0x16
	.endm

	.macro waitbgfadein
	.byte 0x17
	.endm

	.macro changebg bg:req
	.byte 0x18
	.byte \bg
	.endm

	.macro playsewithpan se:req, pan:req
	.byte 0x19
	.2byte \se
	.byte \pan
	.endm

	.macro setpan pan:req
	.byte 0x1a
	.byte \pan
	.endm

	.macro panse se:req, currentPan:req, targetPan:req, incrementPan:req, delay:req
	.byte 0x1b
	.2byte \se
	.byte \currentPan
	.byte \targetPan
	.byte \incrementPan
	.byte \delay
	.endm

	.macro loopsewithpan se:req, pan:req, wait:req, times:req
	.byte 0x1c
	.2byte \se
	.byte \pan
	.byte \wait
	.byte \times
	.endm

	.macro waitplaysewithpan se:req, pan:req, wait:req
	.byte 0x1d
	.2byte \se
	.byte \pan
	.byte \wait
	.endm

	.macro setbldcnt bldcnt:req
	.byte 0x1e
	.2byte \bldcnt
	.endm

	.macro createsoundtask addr:req, argv:vararg
	.byte 0x1F
	.4byte \addr
	.byte (.Lcreatetask_1F_\@_2 - .Lcreatetask_1F_\@_1) / 2
.Lcreatetask_1F_\@_1:
	.2byte \argv
.Lcreatetask_1F_\@_2:
	.endm

	.macro waitsound
	.byte 0x20
	.endm

	.macro jumpargeq argId:req, value:req, ptr:req
	.byte 0x21
	.byte \argId
	.2byte \value
	.4byte \ptr
	.endm

	.macro monbg_static battler:req
	.byte 0x22
	.byte \battler
	.endm

	.macro clearmonbg_static battler:req
	.byte 0x23
	.byte \battler
	.endm

	.macro jumpifcontest ptr:req
	.byte 0x24
	.4byte \ptr
	.endm

	.macro fadetobgfromset bgOpponent:req, bgPlayer:req, bgContest:req
	.byte 0x25
	.byte \bgOpponent
	.byte \bgPlayer
	.byte \bgContest
	.endm

	.macro panse_adjustnone se:req, currentPan:req, targetPan:req, incrementPan:req, delay:req
	.byte 0x26
	.2byte \se
	.byte \currentPan
	.byte \targetPan
	.byte \incrementPan
	.byte \delay
	.endm

	.macro panse_adjustall se:req, currentPan:req, targetPan:req, incrementPan:req, delay:req
	.byte 0x27
	.2byte \se
	.byte \currentPan
	.byte \targetPan
	.byte \incrementPan
	.byte \delay
	.endm

	.macro splitbgprio battler:req
	.byte 0x28
	.byte \battler
	.endm

	.macro splitbgprio_all
	.byte 0x29
	.endm

	.macro splitbgprio_foes battler:req
	.byte 0x2a
	.byte \battler
	.endm

	.macro invisible battler:req
	.byte 0x2b
	.byte \battler
	.endm

	.macro visible battler:req
	.byte 0x2c
	.byte \battler
	.endm

	.macro teamattack_moveback battler:req
	.byte 0x2d
	.byte \battler
	.endm

	.macro teamattack_movefwd battler:req
	.byte 0x2e
	.byte \battler
	.endm

	.macro stopsound
	.byte 0x2f
	.endm

@ useful macros
	.macro jumpreteq value:req, ptr:req
	jumpargeq ARG_RET_ID, \value, \ptr
	.endm

	.macro jumprettrue ptr:req
	jumpreteq TRUE, \ptr
	.endm

	.macro jumpretfalse ptr:req
	jumpreteq FALSE, \ptr
	.endm

@ createsprite wrappers
@ The arguments are based on the CMD_ARGS of the callback associated with the template.

	.macro simplepaletteblend unused_anim_battler=ANIM_ATTACKER, unused_subpriority_offset=2, selector:req, delay:req, initial_blend_y:req, target_blend_y:req, color:req
	createsprite gSimplePaletteBlendSpriteTemplate, \unused_anim_battler, \unused_subpriority_offset, \selector, \delay, \initial_blend_y, \target_blend_y, \color
	.endm

	.macro complexpaletteblend unused_anim_battler=ANIM_ATTACKER, unused_subpriority_offset=2, selector:req, delay:req, num_blends:req, color1:req, blend_y1:req, color2:req, blend_y2:req
	createsprite gComplexPaletteBlendSpriteTemplate, \unused_anim_battler, \unused_subpriority_offset, \selector, \delay, \num_blends, \color1, \blend_y1, \color2, \blend_y2
	.endm

	.macro shakemonorterrain unused_anim_battler=ANIM_ATTACKER, unused_subpriority_offset=2, velocity:req, shake_duration:req, duration:req, type:req, battler_selector
	.if \type == SHAKE_MON_X || \type == SHAKE_MON_Y
	.ifb \battler_selector
	.error "battler_selector required for SHAKE_MON_X or SHAKE_MON_Y"
	.endif
	.else
	.ifnb \battler_selector
	.warning "unused battler_selector in shakemonorterrain"
	.endif
	.endif
	.ifb \battler_selector
	createsprite gShakeMonOrTerrainSpriteTemplate, \unused_anim_battler, \unused_subpriority_offset, \velocity, \shake_duration, \duration, \type
	.else
	createsprite gShakeMonOrTerrainSpriteTemplate, \unused_anim_battler, \unused_subpriority_offset, \velocity, \shake_duration, \duration, \type, \battler_selector
	.endif
	.endm

	.macro createmegahornhornsprite anim_battler:req, subpriority_offset:req, x1:req, y1:req, x2:req, y2:req, duration:req
	createsprite gMegahornHornSpriteTemplate, \anim_battler, \subpriority_offset, \x1, \y1, \x2, \y2, \duration
	.endm

	.macro createleechlifeneedlesprite anim_battler:req, subpriority_offset:req, x:req, y:req, duration:req
	createsprite gLeechLifeNeedleSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y, \duration
	.endm

	.macro createwebthreadsprite anim_battler:req, subpriority_offset:req, x:req, y:req, unk2:req, amplitude:req, targets_both:req
	createsprite gWebThreadSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y, \unk2, \amplitude, \targets_both
	.endm

	.macro createstringwrapsprite anim_battler:req, subpriority_offset:req, x:req, y:req
	createsprite gStringWrapSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y
	.endm

	.macro createlinearstingersprite anim_battler:req, subpriority_offset:req, initial_x:req, initial_y:req, target_x:req, target_y:req, duration:req
	createsprite gLinearStingerSpriteTemplate, \anim_battler, \subpriority_offset, \initial_x, \initial_y, \target_x, \target_y, \duration
	.endm

	.macro createpinmissilesprite anim_battler:req, subpriority_offset:req, initial_x:req, initial_y:req, target_x:req, target_y:req, duration:req, wave_amplitude:req
	createsprite gPinMissileSpriteTemplate, \anim_battler, \subpriority_offset, \initial_x, \initial_y, \target_x, \target_y, \duration, \wave_amplitude
	.endm

	.macro createtailgloworbsprite anim_battler:req, subpriority_offset:req, relative_to:req
	createsprite gTailGlowOrbSpriteTemplate, \anim_battler, \subpriority_offset, \relative_to
	.endm

	.macro createsharpteethsprite anim_battler:req, subpriority_offset:req, x:req, y:req, animation:req, x_velocity:req, y_velocity:req, half_duration:req
	createsprite gSharpTeethSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y, \animation, \x_velocity, \y_velocity, \half_duration
	.endm

	.macro createclampjawsprite anim_battler:req, subpriority_offset:req, x:req, y:req, animation:req, x_velocity:req, y_velocity:req, half_duration:req
	createsprite gClampJawSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y, \animation, \x_velocity, \y_velocity, \half_duration
	.endm

	.macro createteardropsprite anim_battler:req, subpriority_offset:req, relative_to:req, type:req
	createsprite gTearDropSpriteTemplate, \anim_battler, \subpriority_offset, \relative_to, \type
	.endm

	.macro createclawslashsprite anim_battler:req, subpriority_offset:req, x:req, y:req, animation:req
	createsprite gClawSlashSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y, \animation
	.endm

	.macro createoutrageflamesprite anim_battler:req, subpriority_offset:req, x:req, y:req, duration:req, x_velocity:req, y_velocity:req, flicker_duration:req
	createsprite gOutrageFlameSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y, \duration, \x_velocity, \y_velocity, \flicker_duration
	.endm

	.macro createdragonbreathfiresprite anim_battler:req, subpriority_offset:req, initial_x:req, initial_y:req, target_x:req, target_y:req, duration:req
	createsprite gDragonBreathFireSpriteTemplate, \anim_battler, \subpriority_offset, \initial_x, \initial_y, \target_x, \target_y, \duration
	.endm

	.macro createdragonragefirespitsprite anim_battler:req, subpriority_offset:req, initial_x:req, initial_y:req, target_x:req, target_y:req, duration:req
	createsprite gDragonRageFireSpitSpriteTemplate, \anim_battler, \subpriority_offset, \initial_x, \initial_y, \target_x, \target_y, \duration
	.endm

	.macro createdragonragefireplumesprite anim_battler:req, subpriority_offset:req, relative_to:req, x:req, y:req
	createsprite gDragonRageFirePlumeSpriteTemplate, \anim_battler, \subpriority_offset, \relative_to, \x, \y
	.endm

	.macro createdragondanceorbsprite anim_battler:req, subpriority_offset:req, angle:req
	createsprite gDragonDanceOrbSpriteTemplate, \anim_battler, \subpriority_offset, \angle
	.endm

	.macro createoverheatflamesprite anim_battler:req, subpriority_offset:req, speed:req, unk1:req, unk2:req, duration:req, y:req
	createsprite gOverheatFlameSpriteTemplate, \anim_battler, \subpriority_offset, \speed, \unk1, \unk2, \duration, \y
	.endm

	.macro createcirclingsparklesprite anim_battler:req, subpriority_offset:req, x:req, y:req
	createsprite sCirclingSparkleSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y
	.endm

	.macro createconfusionducksprite anim_battler:req, subpriority_offset:req, x:req, y:req, wave_offset:req, wave_period:req, duration:req
	createsprite gConfusionDuckSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y, \wave_offset, \wave_period, \duration
	.endm

	.macro createbasichitsplatsprite anim_battler:req, subpriority_offset:req, x:req, y:req, relative_to:req, animation:req
	createsprite gBasicHitSplatSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y, \relative_to, \animation
	.endm

	.macro createpersisthitsplatsprite anim_battler:req, subpriority_offset:req, x:req, y:req, relative_to:req, animation:req, duration:req
	createsprite gPersistHitSplatSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y, \relative_to, \animation, \duration
	.endm

	.macro createhandleinverthitsplatsprite anim_battler:req, subpriority_offset:req, x:req, y:req, relative_to:req, animation:req
	createsprite gHandleInvertHitSplatSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y, \relative_to, \animation
	.endm

	.macro createrandomposhitsplatsprite anim_battler:req, subpriority_offset:req, relative_to:req, animation
	.ifnb \animation
	createsprite gRandomPosHitSplatSpriteTemplate, \anim_battler, \subpriority_offset, \relative_to, \animation
	.else
	createsprite gRandomPosHitSplatSpriteTemplate, \anim_battler, \subpriority_offset, \relative_to, -1
	.endif
	.endm

	.macro createmonedgehitsplatsprite anim_battler:req, subpriority_offset:req, relative_to:req, x:req, y:req, animation:req
	createsprite gMonEdgeHitSplatSpriteTemplate, \anim_battler, \subpriority_offset, \relative_to, \x, \y, \animation
	.endm

	.macro createcrossimpactsprite anim_battler:req, subpriority_offset:req, x:req, y:req, relative_to:req, duration:req
	createsprite gCrossImpactSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y, \relative_to, \duration
	.endm

	.macro createflashinghitsplatsprite anim_battler:req, subpriority_offset:req, x:req, y:req, relative_to:req, animation:req
	createsprite gFlashingHitSplatSpriteTemplate, \anim_battler, \subpriority_offset, \x, \y, \relative_to, \animation
	.endm

@ createvisualtask wrappers
@ The arguments are based on the CMD_ARGS of the task.

	.macro metallicshine priority=5, permanent:req, color
	.ifb \color
	createvisualtask AnimTask_MetallicShine, \priority, \permanent, FALSE, RGB_BLACK
	.else
	createvisualtask AnimTask_MetallicShine, \priority, \permanent, TRUE, \color
	.endif
	.endm

	.macro setgrayscalepal priority=5, battler:req
	createvisualtask AnimTask_SetGrayscaleOrOriginalPal, \priority, \battler, FALSE
	.endm

	.macro setoriginalpal priority=5, battler:req
	createvisualtask AnimTask_SetGrayscaleOrOriginalPal, \priority, \battler, TRUE
	.endm

	.macro attackerfadetoinvisible priority=2, step_delay:req
	createvisualtask AnimTask_AttackerFadeToInvisible, \priority, \step_delay
	.endm

	.macro attackerfadefrominvisible priority=2, step_delay:req
	createvisualtask AnimTask_AttackerFadeFromInvisible, \priority, \step_delay
	.endm

	.macro blendcolorcycle priority=2, selector:req, delay:req, num_blends:req, initial_blend_y:req, target_blend_y:req, color:req
	createvisualtask AnimTask_BlendColorCycle, \priority, \selector, \delay, \num_blends, \initial_blend_y, \target_blend_y, \color
	.endm

	.macro blendcolorcycleexclude priority=2, unk0:req, delay:req, num_blends:req, initial_blend_y:req, target_blend_y:req, color:req
	createvisualtask AnimTask_BlendColorCycleExclude, \priority, \unk0, \delay, \num_blends, \initial_blend_y, \target_blend_y, \color
	.endm

	.macro blendcolorcyclebytag priority=2, tag:req, delay:req, num_blends:req, initial_blend_y:req, target_blend_y:req, color:req
	createvisualtask AnimTask_BlendColorCycleByTag, \priority, \tag, \delay, \num_blends, \initial_blend_y, \target_blend_y, \color
	.endm

	.macro flashanimtagwithcolor priority=2, tag:req, delay:req, num_blends:req, color1:req, blend_y1:req, color2:req, blend_y2:req
	createvisualtask AnimTask_FlashAnimTagWithColor, \priority, \tag, \delay, \num_blends, \color1, \blend_y1, \color2, \blend_y2
	.endm

	.macro invertscreencolor priority=2, scenery:req, attacker:req, target:req
	@ NOTE: These generate 0x000 or 0x101 to match, but the code checks for '& 0x100'.
	createvisualtask AnimTask_InvertScreenColor, \priority, (\scenery << 8) | \scenery, (\attacker << 8) | \attacker, (\target << 8) | \target
	.endm

	.macro tintpalettes priority=2, scenery:req, attacker:req, target:req, duration:req, color:req
	createvisualtask AnimTask_TintPalettes, \priority, \scenery << 8, \attacker << 8, \target << 8, \duration, \color & 0x1F, (\color >> 5) & 0x1F, (\color >> 10) & 0x1F
	.endm

	.macro shakebattleterrain priority=2, x_offset:req, y_offset:req, shakes:req, delay:req
	createvisualtask AnimTask_ShakeBattleTerrain, \priority, \x_offset, \y_offset, \shakes, \delay
	.endm

@ fixed-point constructors
@ Q8(n, d) means "n/d represented in a Q8", e.g. 'Q8(32, 10)' is a Q8
@ repserentation of 32/10, aka 3.2.

#define Q8(n, d) ((n * 256) / d)
