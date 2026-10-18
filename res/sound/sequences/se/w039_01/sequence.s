track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
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
    note C4, 127, 575
    rest 575
    end
track_01:
    note_wait 0
    rest 60
    program 0
    volume 127
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    pan 64
    mod_depth 0
    expression 127
    note C4, 40, 659
    rest 659
    end
    .byte 0xff, 0x00, 0x00
