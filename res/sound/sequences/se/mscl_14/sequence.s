track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    mod_depth 0
    expression 127
    program 0
    note C4, 120, 95
    rest 95
    end
track_01:
    note_wait 0
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    mod_depth 0
    expression 127
    program 0
    rest 12
    note C4, 50, 95
    rest 95
    end
    .byte 0xff, 0x00, 0x00
