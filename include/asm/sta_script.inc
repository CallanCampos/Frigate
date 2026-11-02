// Macros for the musical stage's scripts, written by tools/script/sta_script.py inc

#include "constants/sound.h"

    // In .data, which LLVM pads with zeros: in .text, clang 18 pads `.balign 4, 0` with ARM NOPs
    .data

    .set FALSE, 0
    .set TRUE, 1

    // The Pokémon the script was started for, or that SetPokeMask set (StaScript_GetPokeMask)
    .set STA_SCRIPT_OWN_POKE, -1

    .macro StaScriptCommand id
    .2byte \id
    .endm

    // The start of a script file, followed by a ScriptEntry for each of its scripts
    .macro ScriptHeader
ScriptStart:
    .endm

    // A script's offset in the file
    .macro ScriptEntry label
    .4byte \label - ScriptStart
    .endm

    // Ends the script
    .macro End
    StaScriptCommand 0
    .endm

    // Waits a number of frames
    .macro Wait frames
    StaScriptCommand 1
    .4byte \frames
    .endm

    // Waits until the script has run a number of frames
    .macro WaitFrame frame
    StaScriptCommand 2
    .4byte \frame
    .endm

    // In a synced script, waits until every synced script waits; the value isn't read
    .macro SyncWait unused
    StaScriptCommand 3
    .4byte \unused
    .endm

    // Opens the curtain
    .macro CurtainOpen wait
    StaScriptCommand 4
    .4byte \wait
    .endm

    // Closes the curtain
    .macro CurtainClose wait
    StaScriptCommand 5
    .4byte \wait
    .endm

    // Moves the curtain to an offset
    .macro CurtainMove frames, offset, wait
    StaScriptCommand 6
    .4byte \frames
    .4byte \offset
    .4byte \wait
    .endm

    // Scrolls the stage to an offset
    .macro Scroll frames, offset, wait
    StaScriptCommand 7
    .4byte \frames
    .4byte \offset
    .4byte \wait
    .endm

    // Does nothing
    .macro Nop2 unused
    StaScriptCommand 8
    .4byte \unused
    .endm

    // Shows or hides Pokémon
    .macro PokeShow poke, show
    StaScriptCommand 9
    .4byte \poke
    .4byte \show
    .endm

    // Turns Pokémon to face the other way, or back
    .macro PokeFlip poke, flip
    StaScriptCommand 10
    .4byte \poke
    .4byte \flip
    .endm

    // Moves Pokémon to a position
    .macro PokeMove poke, frames, x, y, z, wait
    StaScriptCommand 11
    .4byte \poke
    .4byte \frames
    .4byte \x
    .4byte \y
    .4byte \z
    .4byte \wait
    .endm

    // Moves Pokémon by an offset
    .macro PokeMoveBy poke, frames, x, y, z, wait
    StaScriptCommand 12
    .4byte \poke
    .4byte \frames
    .4byte \x
    .4byte \y
    .4byte \z
    .4byte \wait
    .endm

    // Stops Pokémon's animation
    .macro PokeStopAnime poke
    StaScriptCommand 13
    .4byte \poke
    .endm

    // Starts Pokémon's animation
    .macro PokeStartAnime poke
    StaScriptCommand 14
    .4byte \poke
    .endm

    // Changes Pokémon's animation
    .macro PokeChangeAnime poke, anime
    StaScriptCommand 15
    .4byte \poke
    .4byte \anime
    .endm

    // Makes Pokémon jump a number of times, each taking period frames
    .macro PokeJump poke, period, count, height, wait
    StaScriptCommand 16
    .4byte \poke
    .4byte \period
    .4byte \count
    .4byte \height
    .4byte \wait
    .endm

    // Sets the positions STA_SCRIPT_OWN_POKE means
    .macro SetPokeMask poke0, poke1, poke2, poke3
    StaScriptCommand 17
    .4byte \poke0
    .4byte \poke1
    .4byte \poke2
    .4byte \poke3
    .endm

    // Adds a stage object in a slot
    .macro ObjAdd index, objId
    StaScriptCommand 18
    .4byte \index
    .4byte \objId
    .endm

    // Removes the object of a slot
    .macro ObjDel index
    StaScriptCommand 19
    .4byte \index
    .endm

    // Shows the object of a slot
    .macro ObjShow index
    StaScriptCommand 20
    .4byte \index
    .endm

    // Hides the object of a slot
    .macro ObjHide index
    StaScriptCommand 21
    .4byte \index
    .endm

    // Moves the object of a slot
    .macro ObjMove index, frames, x, y, z
    StaScriptCommand 22
    .4byte \index
    .4byte \frames
    .4byte \x
    .4byte \y
    .4byte \z
    .endm

    // Adds a particle effect in a slot
    .macro EffectAdd index, effectNo
    StaScriptCommand 23
    .4byte \index
    .4byte \effectNo
    .endm

    // Removes the effect of a slot
    .macro EffectDel index
    StaScriptCommand 24
    .4byte \index
    .endm

    // Creates an emitter of a slot's effect at a position
    .macro EmitterCreate index, emitterNo, x, y, z
    StaScriptCommand 25
    .4byte \index
    .4byte \emitterNo
    .4byte \x
    .4byte \y
    .4byte \z
    .endm

    // Deletes an emitter of a slot's effect
    .macro EmitterDelete index, emitterNo
    StaScriptCommand 26
    .4byte \index
    .4byte \emitterNo
    .endm

    // Creates an emitter at random positions in a box, count times, every period frames
    .macro EmitterRandom index, emitterNo, period, count, minX, minY, minZ, maxX, maxY, maxZ
    StaScriptCommand 27
    .4byte \index
    .4byte \emitterNo
    .4byte \period
    .4byte \count
    .4byte \minX
    .4byte \minY
    .4byte \minZ
    .4byte \maxX
    .4byte \maxY
    .4byte \maxZ
    .endm

    // Adds a spotlight in a slot
    .macro LightAdd index, unk
    StaScriptCommand 28
    .4byte \index
    .4byte \unk
    .endm

    // Removes the light of a slot
    .macro LightDel index
    StaScriptCommand 29
    .4byte \index
    .endm

    // Moves the light of a slot
    .macro LightMove index, frames, x, y, z
    StaScriptCommand 30
    .4byte \index
    .4byte \frames
    .4byte \x
    .4byte \y
    .4byte \z
    .endm

    // Makes the light of a slot follow a Pokémon at an offset for frames
    .macro LightFollow index, poke, frames, x, y, z
    StaScriptCommand 31
    .4byte \index
    .4byte \poke
    .4byte \frames
    .4byte \x
    .4byte \y
    .4byte \z
    .endm

    // Sets the color of the light of a slot
    .macro LightColor index, r, g, b, alpha
    StaScriptCommand 32
    .4byte \index
    .4byte \r
    .4byte \g
    .4byte \b
    .4byte \alpha
    .endm

    // Prints a message of the program's messages.txt, by its line
    .macro PrintMessage msgId, wait
    StaScriptCommand 33
    .4byte \msgId
    .4byte \wait
    .endm

    // Clears the message
    .macro ClearMessage
    StaScriptCommand 34
    .endm

    // Sets the message's colors
    .macro MessageColor letter, shadow, background
    StaScriptCommand 35
    .4byte \letter
    .4byte \shadow
    .4byte \background
    .endm

    // Plays a sequence
    .macro PlaySeq seq
    StaScriptCommand 36
    .4byte \seq
    .endm

    // Stops the sequence
    .macro StopSeq
    StaScriptCommand 37
    .endm

    // Rotates Pokémon from an angle by an angle
    .macro PokeRotate poke, frames, start, angle, wait
    StaScriptCommand 38
    .4byte \poke
    .4byte \frames
    .4byte \start
    .4byte \angle
    .4byte \wait
    .endm

    // Turns Pokémon to the front, or back
    .macro PokeFront poke, front
    StaScriptCommand 39
    .4byte \poke
    .4byte \front
    .endm

    // Shows or hides Pokémon's props
    .macro PokeShowItem poke, show
    StaScriptCommand 40
    .4byte \poke
    .4byte \show
    .endm

    // Makes Pokémon appeal
    .macro PokeAppeal poke
    StaScriptCommand 41
    .4byte \poke
    .endm

    // Turns the audience to Pokémon
    .macro AudienceFocus poke
    StaScriptCommand 42
    .4byte \poke
    .endm

    // Turns the audience away from every Pokémon
    .macro AudienceUnfocus
    StaScriptCommand 43
    .endm

    // Enables the props' buttons
    .macro ButtonEnable
    StaScriptCommand 44
    .endm

    // Disables the props' buttons
    .macro ButtonDisable
    StaScriptCommand 45
    .endm

    // Lines the Pokémon up by their points at the end
    .macro LineUp frames, wait
    StaScriptCommand 46
    .4byte \frames
    .4byte \wait
    .endm

    // Does nothing
    .macro Nop unused
    StaScriptCommand 47
    .4byte \unused
    .endm

    // Plays a BGM
    .macro PlayBgm seq
    StaScriptCommand 48
    .4byte \seq
    .endm

    // Stops the BGM
    .macro StopBgm
    StaScriptCommand 49
    .endm

    // Starts an appeal script for a position, by its points
    .macro AppealScript pos, file
    StaScriptCommand 50
    .4byte \pos
    .4byte \file
    .endm

    // Starts a Pokémon script
    .macro PokeScript poke, file
    StaScriptCommand 51
    .4byte \poke
    .4byte \file
    .endm

    // Replaces a wave of the sequence's bank's first wave archive with the program's wave
    .macro PlayWave seq, a1, a2
    StaScriptCommand 52
    .4byte \seq
    .4byte \a1
    .4byte \a2
    .endm

    // Plays the program's own sequence and bank as a sequence's
    .macro PlayStrm seq
    StaScriptCommand 53
    .4byte \seq
    .endm
