track_00:
    program 0
    tempo 75
    random rest 50, 100
    random pan 40, 88
    note C3, 127, 0
    random volume 50, 127
    bend_range 1
    random pitch_bend -127, 127
    random mod_depth 20, 40
    mod_range 1
    random mod_speed 1, 3
    mod_type 1
    jump track_00
    .byte 0xff, 0xff
