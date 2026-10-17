track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
    program 0
    volume 70
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    pan 64
    mod_depth 0
    expression 127
    priority 127
    note Gs5, 127, 35
    rest 36
    note D4, 97, 59
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
    priority 125
    note Gs3, 127, 191
    program 0
    volume 127
    priority 125
    rest 72
    note A3, 79, 191
    rest 191
    end
    .byte 0xff, 0x00
