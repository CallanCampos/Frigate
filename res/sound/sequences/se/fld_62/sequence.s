track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 32
    program 1
    volume 60
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    pan 64
    mod_depth 127
    expression 127
    priority 127
    rest 3
    note A2, 100, 5372
    rest 5372
    end
track_01:
    note_wait 0
    program 2
    volume 127
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    pan 64
    mod_depth 64
    expression 127
    priority 127
    note A2, 127, 407
    rest 407
    end
    .byte 0xff, 0x00, 0x00
