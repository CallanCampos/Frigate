track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 277
label_000d:
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    mod_depth 6
    note C3, 127, 3263
    mod_speed 1
    mod_type 2
    mod_range 3
    mod_delay 0
    expression 127
    volume 127
    priority 127
    rest 3264
    jump label_000d
    .byte 0xff
track_01:
    note_wait 0
label_0034:
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    mod_depth 0
    expression 95
    program 1
    volume 117
    priority 127
    pan 68
    rest 132
    note A3, 107, 131
    rest 60
    pan 68
    rest 156
    pan 60
    rest 72
    note C3, 127, 71
    rest 72
    note D3, 127, 222
    rest 576
    pan 64
    rest 180
    note A2, 105, 71
    rest 72
    note B2, 105, 222
    rest 228
    pan 55
    rest 24
    note F3, 95, 131
    rest 576
    pan 64
    rest 120
    note As2, 105, 222
    rest 996
    jump label_0034
    .byte 0xff, 0xff, 0x00
