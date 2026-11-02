// Macros for the battle effect VM's scripts, written by tools/script/btlv_effvm_script.py inc

#include "constants/moves.h"
#include "constants/sound.h"

    // In .data, which LLVM pads with zeros: in .text, clang 18 pads `.balign 4, 0` with ARM NOPs
    .data

    .set FALSE, 0
    .set TRUE, 1

    // The first effect number of a/0/6/6 (btlv_effvm.c); those below it are moves
    .set BTLV_EFFECT_FIRST, 0x231

    // The targets besides the positions 0 to 13; ALL and the sides only for several (BtlvEffvm_GetTargetPositions)
    .set EFFVM_ATTACKER, 14
    .set EFFVM_ATTACKER_PARTNER, 15
    .set EFFVM_DEFENDER, 16
    .set EFFVM_DEFENDER_PARTNER, 17
    .set EFFVM_ALL, 18
    .set EFFVM_PLAYER_SIDE, 19
    .set EFFVM_ENEMY_SIDE, 20

    // The emitters' positions besides 0 to 7 (BtlvEffvm_InitEmitter); MIRRORED mirrors the spin axis
    .set EFFVM_EMIT_ATTACKER, 9
    .set EFFVM_EMIT_ATTACKER_MIRRORED, 10
    .set EFFVM_EMIT_DEFENDER, 11
    .set EFFVM_EMIT_DEFENDER_MIRRORED, 12

    // The sides of PlaySE's pan, which takes a position too
    .set EFFVM_PAN_LEFT, 0
    .set EFFVM_PAN_RIGHT, 1
    .set EFFVM_PAN_CENTER, 2

    // What Wait waits for (BtlvEffvm_WaitNative)
    .set EFFVM_WAIT_ALL, 0
    .set EFFVM_WAIT_CAMERA, 1
    .set EFFVM_WAIT_PARTICLES, 2
    .set EFFVM_WAIT_MCSS, 3
    .set EFFVM_WAIT_TRAINER_ANIM, 4
    .set EFFVM_WAIT_BG, 5
    .set EFFVM_WAIT_STAGE_FADE, 6
    .set EFFVM_WAIT_FIELD_FADE, 7
    .set EFFVM_WAIT_STAGE_FIELD_FADE, 8
    .set EFFVM_WAIT_PALETTE_FADE, 9
    .set EFFVM_WAIT_SE, 10
    .set EFFVM_WAIT_SE_PLAYER_1, 11
    .set EFFVM_WAIT_SE_PLAYER_2, 12
    .set EFFVM_WAIT_SE_PLAYER_4, 13
    .set EFFVM_WAIT_SE_PLAYER_3, 14
    .set EFFVM_WAIT_SE_PLAYER_0, 15
    .set EFFVM_WAIT_CRY, 16
    .set EFFVM_WAIT_WINDOW, 17
    .set EFFVM_WAIT_PAL_ANIM, 18
    .set EFFVM_WAIT_BG_WAVE, 19

    // The comparisons of IfVarConst and IfVarVar (BtlvEffvm_JumpIf)
    .set EFFVM_CMP_EQ, 0
    .set EFFVM_CMP_NE, 1
    .set EFFVM_CMP_LT, 2
    .set EFFVM_CMP_GT, 3
    .set EFFVM_CMP_LE, 4
    .set EFFVM_CMP_GE, 5

    // The variables (BtlvEffvm_GetVariable); those of each position take it added, from 0 to 7
    .set EFFVM_VAR_WEIGHT, 0
    .set EFFVM_VAR_ATTACKER_WEIGHT, 8
    .set EFFVM_VAR_TARGET, 9
    .set EFFVM_VAR_VARIANT, 10
    .set EFFVM_VAR_PARAM_2, 11
    .set EFFVM_VAR_SHAKES, 12
    .set EFFVM_VAR_CAUGHT, 13
    .set EFFVM_VAR_CRITICAL, 14
    .set EFFVM_VAR_ITEM, 15
    .set EFFVM_VAR_VALUE, 16
    .set EFFVM_VAR_ATTACKER, 17
    .set EFFVM_VAR_ATTACKER_VANISH, 18
    .set EFFVM_VAR_ATTACKER_SIDE, 19
    .set EFFVM_VAR_RARE, 20
    .set EFFVM_VAR_ATTACKER_RARE, 28
    .set EFFVM_VAR_NO_BOUNCE, 29
    .set EFFVM_VAR_ATTACKER_NO_BOUNCE, 37
    .set EFFVM_VAR_IS_MULTI, 38
    .set EFFVM_VAR_BATTLE_STYLE, 39
    .set EFFVM_VAR_TRAINER_TYPE, 40
    .set EFFVM_VAR_MCSS_SCALED, 53
    .set EFFVM_VAR_CAMERA_SAVED, 54
    .set EFFVM_VAR_CAMERA_LOCKED, 56
    .set EFFVM_VAR_DEFENDER, 57
    .set EFFVM_VAR_N_POKEMON, 58
    .set EFFVM_VAR_ATTACKER_N_POKEMON, 66
    .set EFFVM_VAR_FAMOUS, 67
    .set EFFVM_VAR_ATTACKER_FAMOUS, 75

    .macro EffvmCommand id
    .2byte \id
    .endm

    // The start of a script file: the number of its variants, each followed by ScriptVariant
    .macro ScriptHeader variants
ScriptStart:
    .4byte \variants
    .endm

    // A variant's code, whose offset is written 14 times; the game reads the first
    .macro ScriptVariant label
    .rept 14
    .4byte \label - ScriptStart
    .endr
    .endm

    // Moves the camera to one of its preset places
    .macro CameraMovePreset move_type, camera, frames, wait, brake
    EffvmCommand 0
    .4byte \move_type
    .4byte \camera
    .4byte \frames
    .4byte \wait
    .4byte \brake
    .endm

    // Moves the camera and its target
    .macro CameraMove move_type, x, y, z, target_x, target_y, target_z, frames, wait, brake
    EffvmCommand 1
    .4byte \move_type
    .4byte \x
    .4byte \y
    .4byte \z
    .4byte \target_x
    .4byte \target_y
    .4byte \target_z
    .4byte \frames
    .4byte \wait
    .4byte \brake
    .endm

    .macro CameraRotate move_type, pitch, yaw, frames, wait, brake
    EffvmCommand 2
    .4byte \move_type
    .4byte \pitch
    .4byte \yaw
    .4byte \frames
    .4byte \wait
    .4byte \brake
    .endm

    .macro CameraShake dir, amplitude, unused, frames, wait, count
    EffvmCommand 3
    .4byte \dir
    .4byte \amplitude
    .4byte \unused
    .4byte \frames
    .4byte \wait
    .4byte \count
    .endm

    // 0 for attacker_only sets how the Pokemon face the camera
    .macro McssCameraMode type, attacker_only
    EffvmCommand 4
    .4byte \type
    .4byte \attacker_only
    .endm

    // Saves the camera's place, which camera preset 13 returns to
    .macro CameraSave
    EffvmCommand 5
    .endm

    // Loads a particle resource
    .macro ParticleLoad particle
    EffvmCommand 6
    .4byte \particle
    .endm

    // Adds an emitter; to 8 is the same as from
    .macro EmitterAdd particle, emitter, from, to, offset_y, unk18, unused, radius_scale, life_scale, scale_scale, camera_scale
    EffvmCommand 7
    .4byte \particle
    .4byte \emitter
    .4byte \from
    .4byte \to
    .4byte \offset_y
    .4byte \unk18
    .4byte \unused
    .4byte \radius_scale
    .4byte \life_scale
    .4byte \scale_scale
    .4byte \camera_scale
    .endm

    // Adds an emitter between two points
    .macro EmitterAddAt particle, emitter, from_x, from_y, from_z, to_x, to_y, to_z, offset_y, unk18, unused, radius_scale, life_scale, scale_scale, camera_scale
    EffvmCommand 8
    .4byte \particle
    .4byte \emitter
    .4byte \from_x
    .4byte \from_y
    .4byte \from_z
    .4byte \to_x
    .4byte \to_y
    .4byte \to_z
    .4byte \offset_y
    .4byte \unk18
    .4byte \unused
    .4byte \radius_scale
    .4byte \life_scale
    .4byte \scale_scale
    .4byte \camera_scale
    .endm

    .macro EmitterAddOrtho particle, emitter, from, to, offset_x, offset_y, offset_z, radius_scale, life_scale, scale_scale, camera_scale
    EffvmCommand 9
    .4byte \particle
    .4byte \emitter
    .4byte \from
    .4byte \to
    .4byte \offset_x
    .4byte \offset_y
    .4byte \offset_z
    .4byte \radius_scale
    .4byte \life_scale
    .4byte \scale_scale
    .4byte \camera_scale
    .endm

    // Adds every emitter of the resource
    .macro EmitterAddAll particle, from, to, offset_y, unk18, ortho, radius_scale, life_scale, scale_scale, camera_scale
    EffvmCommand 10
    .4byte \particle
    .4byte \from
    .4byte \to
    .4byte \offset_y
    .4byte \unk18
    .4byte \ortho
    .4byte \radius_scale
    .4byte \life_scale
    .4byte \scale_scale
    .4byte \camera_scale
    .endm

    // Deletes a particle resource and its emitters
    .macro ParticleDelete particle
    EffvmCommand 11
    .4byte \particle
    .endm

    // Adds an emitter that moves from from to to
    .macro EmitterMove particle, emitter, motion, from, to, offset_y, frames, height, life_scale, camera_scale, turns
    EffvmCommand 12
    .4byte \particle
    .4byte \emitter
    .4byte \motion
    .4byte \from
    .4byte \to
    .4byte \offset_y
    .4byte \frames
    .4byte \height
    .4byte \life_scale
    .4byte \camera_scale
    .4byte \turns
    .endm

    .macro EmitterMoveFrom particle, emitter, motion, from_x, from_y, from_z, to, offset_y, frames, height, life_scale, camera_scale, turns
    EffvmCommand 13
    .4byte \particle
    .4byte \emitter
    .4byte \motion
    .4byte \from_x
    .4byte \from_y
    .4byte \from_z
    .4byte \to
    .4byte \offset_y
    .4byte \frames
    .4byte \height
    .4byte \life_scale
    .4byte \camera_scale
    .4byte \turns
    .endm

    .macro EmitterMoveOrtho particle, emitter, motion, from, to, offset_y, frames, height, life_scale, camera_scale, turns
    EffvmCommand 14
    .4byte \particle
    .4byte \emitter
    .4byte \motion
    .4byte \from
    .4byte \to
    .4byte \offset_y
    .4byte \frames
    .4byte \height
    .4byte \life_scale
    .4byte \camera_scale
    .4byte \turns
    .endm

    .macro EmitterMoveFromOrtho particle, emitter, motion, from_x, from_y, from_z, to, offset_y, frames, height, life_scale, camera_scale, scale_scale
    EffvmCommand 15
    .4byte \particle
    .4byte \emitter
    .4byte \motion
    .4byte \from_x
    .4byte \from_y
    .4byte \from_z
    .4byte \to
    .4byte \offset_y
    .4byte \frames
    .4byte \height
    .4byte \life_scale
    .4byte \camera_scale
    .4byte \scale_scale
    .endm

    // Adds an emitter that circles the attacker (type 0, 1), the defender (2, 3) or the origin (4, 5)
    .macro OrbitEmitters particle, emitter, type, radius_x, radius_z, offset_y, frames, wait, unk30, delay
    EffvmCommand 16
    .4byte \particle
    .4byte \emitter
    .4byte \type
    .4byte \radius_x
    .4byte \radius_z
    .4byte \offset_y
    .4byte \frames
    .4byte \wait
    .4byte \unk30
    .4byte \delay
    .endm

    .macro OrbitEmittersAlt particle, emitter, type, radius_x, radius_z, offset_y, frames, wait, unk30, delay
    EffvmCommand 17
    .4byte \particle
    .4byte \emitter
    .4byte \type
    .4byte \radius_x
    .4byte \radius_z
    .4byte \offset_y
    .4byte \frames
    .4byte \wait
    .4byte \unk30
    .4byte \delay
    .endm

    // Moves the Pokemon
    .macro McssMoveXY targets, type, x, y, frames, wait, times
    EffvmCommand 18
    .4byte \targets
    .4byte \type
    .4byte \x
    .4byte \y
    .4byte \frames
    .4byte \wait
    .4byte \times
    .endm

    // Moves the Pokemon around a circle
    .macro McssCircle targets, mode, start, radius1, radius2, frames, wait, count, turn_wait
    EffvmCommand 19
    .4byte \targets
    .4byte \mode
    .4byte \start
    .4byte \radius1
    .4byte \radius2
    .4byte \frames
    .4byte \wait
    .4byte \count
    .4byte \turn_wait
    .endm

    .macro McssShake targets, axis, start_angle, end_angle, amplitude, frames
    EffvmCommand 20
    .4byte \targets
    .4byte \axis
    .4byte \start_angle
    .4byte \end_angle
    .4byte \amplitude
    .4byte \frames
    .endm

    .macro McssScale targets, type, x, y, frames, wait, times
    EffvmCommand 21
    .4byte \targets
    .4byte \type
    .4byte \x
    .4byte \y
    .4byte \frames
    .4byte \wait
    .4byte \times
    .endm

    .macro McssRotate targets, type, angle, frames, wait, times
    EffvmCommand 22
    .4byte \targets
    .4byte \type
    .4byte \angle
    .4byte \frames
    .4byte \wait
    .4byte \times
    .endm

    .macro McssAlpha targets, type, alpha, frames, wait, times
    EffvmCommand 23
    .4byte \targets
    .4byte \type
    .4byte \alpha
    .4byte \frames
    .4byte \wait
    .4byte \times
    .endm

    .macro McssMosaic targets, type, level, frames, wait, times
    EffvmCommand 24
    .4byte \targets
    .4byte \type
    .4byte \level
    .4byte \frames
    .4byte \wait
    .4byte \times
    .endm

    .macro McssBlink targets, type, wait, times
    EffvmCommand 25
    .4byte \targets
    .4byte \type
    .4byte \wait
    .4byte \times
    .endm

    // Hides or shows the Pokemon
    .macro McssSetVanish targets, vanish
    EffvmCommand 26
    .4byte \targets
    .4byte \vanish
    .endm

    .macro McssPalFade targets, start_evy, end_evy, wait, color
    EffvmCommand 27
    .4byte \targets
    .4byte \start_evy
    .4byte \end_evy
    .4byte \wait
    .4byte \color
    .endm

    .macro McssSetMode targets, mode
    EffvmCommand 28
    .4byte \targets
    .4byte \mode
    .endm

    .macro McssSetValue targets, value
    EffvmCommand 29
    .4byte \targets
    .4byte \value
    .endm

    .macro McssScaleTo targets, type, x, y, frames, wait, times
    EffvmCommand 30
    .4byte \targets
    .4byte \type
    .4byte \x
    .4byte \y
    .4byte \frames
    .4byte \wait
    .4byte \times
    .endm

    // Hides the Pokemon's gauges
    .macro GaugeHide targets
    EffvmCommand 31
    .4byte \targets
    .endm

    // Shows a trainer at a position; trainer_type -1 takes the script's value
    .macro SetTrainer trainer_type, target, x, y, z
    EffvmCommand 32
    .4byte \trainer_type
    .4byte \target
    .4byte \x
    .4byte \y
    .4byte \z
    .endm

    .macro TrainerMove trainer, type, x, y, z, frames, wait, times
    EffvmCommand 33
    .4byte \trainer
    .4byte \type
    .4byte \x
    .4byte \y
    .4byte \z
    .4byte \frames
    .4byte \wait
    .4byte \times
    .endm

    .macro McssPlayAnim pos, anim
    EffvmCommand 34
    .4byte \pos
    .4byte \anim
    .endm

    .macro DeleteMcss trainer
    EffvmCommand 35
    .4byte \trainer
    .endm

    // Loads a background into BG 3
    .macro LoadBg file
    EffvmCommand 36
    .4byte \file
    .endm

    .macro BgMove mode, x, y, frames, wait, count
    EffvmCommand 37
    .4byte \mode
    .4byte \x
    .4byte \y
    .4byte \frames
    .4byte \wait
    .4byte \count
    .endm

    .macro BgWave mode, amplitude, step, frames, fade_mode, fade_frames
    EffvmCommand 38
    .4byte \mode
    .4byte \amplitude
    .4byte \step
    .4byte \frames
    .4byte \fade_mode
    .4byte \fade_frames
    .endm

    // Animates BG 3's palette from a file of ARCID_BTLV_EFFVM_PAL_ANIM
    .macro BgPalAnim file, loops
    EffvmCommand 39
    .4byte \file
    .4byte \loops
    .endm

    // Sets BG 3's priority
    .macro BgSetPriority priority
    EffvmCommand 40
    .4byte \priority
    .endm

    .macro SetAlphaBlend unused0, unused1, alpha, unused3, unused4, unused5
    EffvmCommand 41
    .4byte \unused0
    .4byte \unused1
    .4byte \alpha
    .4byte \unused3
    .4byte \unused4
    .4byte \unused5
    .endm

    // Fades the stage's, the field's or the palettes' colors (BtlvEffect_StartPaletteFade)
    .macro PaletteFade fade, evy, target_evy, wait, color
    EffvmCommand 42
    .4byte \fade
    .4byte \evy
    .4byte \target_evy
    .4byte \wait
    .4byte \color
    .endm

    // Hides or shows the stage, the field or BG 3 (BtlvEffect_SetVanish)
    .macro SetVanish what, hide
    EffvmCommand 43
    .4byte \what
    .4byte \hide
    .endm

    // Opens or closes window 0 step by step
    .macro QueueCommands dir, win_h, win_v, win_in_out, count, wait, keep_window
    EffvmCommand 44
    .4byte \dir
    .4byte \win_h
    .4byte \win_v
    .4byte \win_in_out
    .4byte \count
    .4byte \wait
    .4byte \keep_window
    .endm

    // Adds a cell actor
    .macro ClactAdd index, file, target, offset_x, offset_y, scale_x, scale_y
    EffvmCommand 45
    .4byte \index
    .4byte \file
    .4byte \target
    .4byte \offset_x
    .4byte \offset_y
    .4byte \scale_x
    .4byte \scale_y
    .endm

    .macro ClactMove index, type, x, y, frames, wait, count
    EffvmCommand 46
    .4byte \index
    .4byte \type
    .4byte \x
    .4byte \y
    .4byte \frames
    .4byte \wait
    .4byte \count
    .endm

    .macro ClactScale index, type, x, y, frames, wait, count
    EffvmCommand 47
    .4byte \index
    .4byte \type
    .4byte \x
    .4byte \y
    .4byte \frames
    .4byte \wait
    .4byte \count
    .endm

    .macro ClactSetAnim index, sequence
    EffvmCommand 48
    .4byte \index
    .4byte \sequence
    .endm

    .macro ClactPalFade index, start, end, delay, color
    EffvmCommand 49
    .4byte \index
    .4byte \start
    .4byte \end
    .4byte \delay
    .4byte \color
    .endm

    .macro ClactDelete index
    EffvmCommand 50
    .4byte \index
    .endm

    .macro Gauge mode, target
    EffvmCommand 51
    .4byte \mode
    .4byte \target
    .endm

    // Plays a sound effect after wait frames; player 5 is the sound's own
    .macro PlaySE se, player, pan, wait, pitch, volume, mod_depth, mod_speed, unused
    EffvmCommand 52
    .4byte \se
    .4byte \player
    .4byte \pan
    .4byte \wait
    .4byte \pitch
    .4byte \volume
    .4byte \mod_depth
    .4byte \mod_speed
    .4byte \unused
    .endm

    .macro StopSE player
    EffvmCommand 53
    .4byte \player
    .endm

    // Pans a sound from one side to the other
    .macro PanSE player, type, from, to, delay, frames, step_wait, count
    EffvmCommand 54
    .4byte \player
    .4byte \type
    .4byte \from
    .4byte \to
    .4byte \delay
    .4byte \frames
    .4byte \step_wait
    .4byte \count
    .endm

    .macro MoveSE player, type, param, start, end, delay, frames, step_wait, count
    EffvmCommand 55
    .4byte \player
    .4byte \type
    .4byte \param
    .4byte \start
    .4byte \end
    .4byte \delay
    .4byte \frames
    .4byte \step_wait
    .4byte \count
    .endm

    // Waits until what is done (EFFVM_WAIT_*)
    .macro Wait what
    EffvmCommand 56
    .4byte \what
    .endm

    .macro WaitFrames frames
    EffvmCommand 57
    .4byte \frames
    .endm

    // Whether the commands return to the battle each frame
    .macro SetYield yield
    EffvmCommand 58
    .4byte \yield
    .endm

    // Jumps if the variable compares with the value so (EFFVM_CMP_*)
    .macro IfVarConst var, cmp, value, jump
    EffvmCommand 59
    .4byte \var
    .4byte \cmp
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    .macro IfVarVar var, cmp, var2, jump
    EffvmCommand 60
    .4byte \var
    .4byte \cmp
    .4byte \var2
    .4byte \jump - (. + 4)
    .endm

    .macro IfPokeExists pos, exists, jump
    EffvmCommand 61
    .4byte \pos
    .4byte \exists
    .4byte \jump - (. + 4)
    .endm

    // Sets the script's value, variable EFFVM_VAR_VALUE
    .macro SetValue value
    EffvmCommand 62
    .4byte \value
    .endm

    // Loads the variable into the script's value
    .macro LoadVar var
    EffvmCommand 63
    .4byte \var
    .endm

    // Sets the variables that come from BtlvEffvmParam, and 53 to 56
    .macro SetVar var, value
    EffvmCommand 64
    .4byte \var
    .4byte \value
    .endm

    .macro McssEffect type, targets, arg
    EffvmCommand 65
    .4byte \type
    .4byte \targets
    .4byte \arg
    .endm

    // Gives the attacker the defender's sprite, as Transform does
    .macro McssSwap
    EffvmCommand 66
    .endm

    // Plays the Pokemon's cries
    .macro PlayCry targets, speed, volume, chorus_volume, chorus_speed, reverse, delay
    EffvmCommand 67
    .4byte \targets
    .4byte \speed
    .4byte \volume
    .4byte \chorus_volume
    .4byte \chorus_speed
    .4byte \reverse
    .4byte \delay
    .endm

    // Whose ball the ball particles show: 8 for the effect's item, 9 the attacker's
    .macro SetBallSource source
    EffvmCommand 68
    .4byte \source
    .endm

    .macro BallAdd index, target, offset_x, offset_y, scale_x, scale_y
    EffvmCommand 69
    .4byte \index
    .4byte \target
    .4byte \offset_x
    .4byte \offset_y
    .4byte \scale_x
    .4byte \scale_y
    .endm

    // Runs another effect's script and comes back; EFFVM_ATTACKER and EFFVM_DEFENDER keep the attacker and the defender
    .macro Call effect, attacker, defender
    EffvmCommand 70
    .4byte \effect
    .4byte \attacker
    .4byte \defender
    .endm

    // Returns from Call
    .macro Return
    EffvmCommand 71
    .endm

    .macro Jump jump
    EffvmCommand 72
    .4byte \jump - (. + 4)
    .endm

    // Waits for btlv_effect.c to resume the script
    .macro WaitExternal
    EffvmCommand 73
    .endm

    // Goes on with another effect's script
    .macro ChangeScript effect
    EffvmCommand 74
    .4byte \effect
    .endm

    .macro WaitSide side
    EffvmCommand 75
    .4byte \side
    .endm

    .macro McssReset check
    EffvmCommand 76
    .4byte \check
    .endm

    // Ends the script
    .macro End
    EffvmCommand 77
    .endm
