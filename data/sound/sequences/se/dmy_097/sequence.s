track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 68
label_000d:
    program 0
    volume 127
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    pan 64
    mod_depth 0
    expression 127
    note As2, 127, 6911
    rest 1728
    jump label_000d
    .byte 0x80, 0xa8, 0x3f, 0xff
track_01:
    note_wait 0
label_0030:
    program 1
    volume 30
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    pan 64
    mod_depth 100
    expression 120
    note B3, 127, 6911
    rest 1728
    jump label_0030
    .byte 0x80, 0xa8, 0x3f, 0xff, 0xff, 0x00, 0x00
