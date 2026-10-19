track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
    program 0
    volume 40
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    pan 64
    mod_depth 0
    expression 127
    note B5, 127, 35
    rest 36
    note F4, 127, 59
    rest 59
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
    note C4, 127, 191
    note C5, 127, 191
    rest 72
    note C4, 40, 191
    rest 191
    end
    .byte 0xff, 0x00, 0x00, 0x00
