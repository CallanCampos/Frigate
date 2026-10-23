#ifndef POKEBW2_DEMO_INTRO_SCRIPT_H
#define POKEBW2_DEMO_INTRO_SCRIPT_H

#include "types.h"

// The intro's scripts: lists of commands that intro_cmd.c runs. A command keeps running each frame until its handler
// returns TRUE, and the next one starts when every running command has ended. With runNext set, the next command
// starts in the same frame instead, so the two run together.
typedef struct {
    u32 cmd;
    s32 args[4];
    u8 runNext : 1;
} IntroCmdEntry;

enum {
    INTRO_CMD_NONE,
    // script: jumps to another script, at once
    INTRO_CMD_GOTO,
    // Jumps to the script numbered after the intro's mode
    INTRO_CMD_GOTO_MODE_SCRIPT,
    // yesMessage, noMessage, a2: asks yes or no, then runs the next command for yes or the one after for no
    INTRO_CMD_YES_NO,
    // condition: runs the next command if the condition holds, or the one after if not
    INTRO_CMD_IF,
    // withBand: loads the BGs, and with withBand, the band that INTRO_CMD_OPEN_BAND shows
    INTRO_CMD_LOAD_GRAPHICS,
    // result: sets the result that ov162 reads once the intro ends
    INTRO_CMD_SET_RESULT,
    // frames
    INTRO_CMD_WAIT,
    // mode, start, end, slowness: GFL_FadeSet, then waits for the fade
    INTRO_CMD_FADE,
    // brightness
    INTRO_CMD_SET_BRIGHTNESS,
    // steps, target, start
    INTRO_CMD_START_BRIGHTNESS_TRANSITION,
    // Checks whether the transition has ended, but ends at once whatever the answer
    INTRO_CMD_CHECK_BRIGHTNESS,
    // sequence
    INTRO_CMD_BGM_PLAY,
    // frames
    INTRO_CMD_BGM_FADE_OUT,
    // frames
    INTRO_CMD_BGM_FADE_IN,
    // Waits until GFL_SndBGMIsPlaying
    INTRO_CMD_WAIT_BGM_PLAYING,
    // sequence, volume, a2: volume and a2 are only set when not 0
    INTRO_CMD_SE_PLAY,
    // sequence
    INTRO_CMD_SE_STOP,
    // Waits for a button or the touch screen
    INTRO_CMD_WAIT_INPUT,
    // preload, file: loads a file of the system message archive for the messages
    INTRO_CMD_LOAD_MESSAGES,
    // source, slot: puts a name in a slot of the messages' word set
    INTRO_CMD_SET_WORD,
    // message, frame: prints a message and waits for it to be read
    INTRO_CMD_MESSAGE,
    INTRO_CMD_CLEAR_MESSAGE,
    // sprite, species, x, y: species 0 adds Professor Juniper, anything else that Pokémon
    INTRO_CMD_ADD_SPRITE,
    // sprite, visible
    INTRO_CMD_SET_SPRITE_VISIBLE,
    // sprite, animation, restartAtEnd
    INTRO_CMD_SET_SPRITE_ANIMATION,
    // sprite
    INTRO_CMD_WAIT_SPRITE_ANIMATION,
    // sprite
    INTRO_CMD_CLEAR_SPRITE_ANIMATION_ENDED,
    // sprite, step, target
    INTRO_CMD_MOVE_SPRITE_X,
    // sprite, visible: fades a sprite in or out
    INTRO_CMD_FADE_SPRITE,
    // message, sprite, frame: prints a message, switching the sprite's frames while the text prints
    INTRO_CMD_TALK,
    // language: sets the config's message language, the kana or kanji text of the Japanese version
    INTRO_CMD_SET_MSG_LANGUAGE,
    // gender
    INTRO_CMD_SET_GENDER,
    // The Pokémon, sprite 1, drops in and cries
    INTRO_CMD_POKEMON_APPEAR,
    // x, y: particles at a position
    INTRO_CMD_PARTICLES,
    // visible: shows or hides the 3D model
    INTRO_CMD_SET_MODEL_VISIBLE,
    // frame
    INTRO_CMD_SET_MODEL_FRAME,
    INTRO_CMD_OPEN_MODEL,
    // Picks boy or girl with left and right on the 3D model, and sets the player's gender
    INTRO_CMD_CHOOSE_GENDER,
    // Turns the 3D model back from the chosen gender
    INTRO_CMD_MODEL_BACK,
    // Start, pause, resume and wait for ov162's creation of the save data
    INTRO_CMD_SAVE_START,
    INTRO_CMD_SAVE_PAUSE,
    INTRO_CMD_SAVE_RESUME,
    INTRO_CMD_SAVE_43,
    // Shows "Creating save data" until it is done
    INTRO_CMD_SAVE_WAIT,
    // Opens and closes a band across the top screen, from its middle
    INTRO_CMD_OPEN_BAND,
    INTRO_CMD_CLOSE_BAND,
    // A cell actor from the intro's archive, shown on the band when the professor asks about the rival
    INTRO_CMD_CREATE_OBJ,
    INTRO_CMD_FREE_OBJ,
    INTRO_CMD_OBJ_FADE_IN,
    INTRO_CMD_OBJ_FADE_OUT,
    INTRO_CMD_NOP,
    INTRO_CMD_END,
};

enum {
    // Loads the graphics and messages, and goes on to the mode's script
    INTRO_SCRIPT_BEGIN,
    INTRO_SCRIPT_START,
    INTRO_SCRIPT_GREETING,
    INTRO_SCRIPT_POKEMON,
    INTRO_SCRIPT_ABOUT_YOU,
    INTRO_SCRIPT_GENDER,
    INTRO_SCRIPT_ASK_NAME,
    INTRO_SCRIPT_PLAYER_NAMED,
    INTRO_SCRIPT_RENAME_PLAYER,
    INTRO_SCRIPT_ASK_RIVAL_NAME,
    INTRO_SCRIPT_RIVAL_NAMED,
    INTRO_SCRIPT_RENAME_RIVAL,
    INTRO_SCRIPT_FAREWELL,
    INTRO_SCRIPT_FINISH,
    INTRO_SCRIPT_COUNT,
};

// INTRO_CMD_IF's conditions
#define INTRO_COND_PLAYER_IS_MALE 0

// INTRO_CMD_SET_WORD's sources
enum {
    INTRO_WORD_PLAYER_NAME,
    INTRO_WORD_RIVAL_NAME,
};

// The sprites of intro_mcss.c
enum {
    INTRO_SPRITE_PROFESSOR,
    INTRO_SPRITE_POKEMON,
};

const IntroCmdEntry *IntroScript_Get(u32 script);

#endif // POKEBW2_DEMO_INTRO_SCRIPT_H
