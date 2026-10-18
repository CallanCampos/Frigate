track_00:
    alloc_tracks 0x0007
    open_track 1, track_01
    open_track 2, track_02
    note_wait 0
    tempo 300
    program 0
    volume 127
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    pan 64
    mod_depth 0
    expression 127
    note C4, 127, 191
    rest 191
    end
track_01:
    note_wait 0
    program 1
    volume 127
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    pan 64
    mod_depth 0
    expression 127
    rest 8
    attack 84
    note Ds4, 90, 191
    rest 191
    end
track_02:
    note_wait 1
    program 1
    volume 127
    pan 64
    note Ds3, 120, 0
    end
    .byte 0xff, 0x00, 0x00, 0x00
