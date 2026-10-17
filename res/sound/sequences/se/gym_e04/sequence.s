track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
label_000d:
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    mod_depth 0
    expression 127
    program 0
    volume 127
    priority 127
    rest 1
    note Ds4, 127, 959
    rest 8
    note Gs4, 127, 948
    rest 951
    jump label_000d
    .byte 0xff
track_01:
    note_wait 0
label_0035:
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    mod_depth 0
    expression 127
    program 1
    volume 117
    priority 127
    note G2, 67, 320
    rest 75
    note C3, 122, 222
    rest 348
    note Ds3, 67, 107
    rest 111
    note C3, 122, 106
    rest 105
    note G2, 67, 296
    rest 321
    jump label_0035
    .byte 0xff, 0xff, 0x00
