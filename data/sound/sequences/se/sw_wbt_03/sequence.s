track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 270
label_000d:
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    mod_depth 5
    program 0
    volume 127
    note C3, 127, 3647
    mod_speed 3
    mod_type 2
    mod_range 2
    mod_delay 0
    priority 127
    rest 336
    expression 124
    rest 48
    expression 118
    rest 96
    expression 123
    rest 96
    expression 127
    rest 144
    expression 124
    rest 240
    expression 120
    rest 216
    expression 124
    rest 192
    expression 126
    rest 288
    expression 123
    rest 264
    expression 127
    rest 384
    expression 121
    rest 120
    expression 124
    rest 24
    expression 127
    rest 384
    expression 123
    rest 96
    expression 127
    rest 144
    expression 124
    rest 240
    expression 120
    rest 192
    expression 127
    rest 144
    jump label_000d
    .byte 0xff
track_01:
    note_wait 0
label_0088:
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    mod_depth 0
    program 1
    volume 117
    priority 127
    note F3, 120, 222
    expression 95
    pan 71
    rest 384
    pan 57
    rest 48
    note A2, 105, 107
    rest 108
    note C3, 127, 203
    rest 204
    pan 88
    rest 120
    note E3, 93, 203
    rest 480
    pan 71
    rest 48
    note As2, 127, 239
    rest 336
    pan 55
    rest 48
    note Ds3, 105, 71
    rest 72
    note F3, 127, 203
    rest 456
    pan 78
    rest 264
    note C3, 105, 203
    rest 576
    pan 64
    rest 48
    note D3, 105, 203
    rest 456
    jump label_0088
    .byte 0xff, 0xff, 0x00, 0x00
