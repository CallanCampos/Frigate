track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
    volume 127
    expression 127
    mod_depth 0
    program 0
    bend_range 16
    priority 64
    note G2, 127, 575
    rest 575
    end
track_01:
    note_wait 0
    volume 85
    expression 127
    mod_depth 0
    program 1
    pitch_bend 1
    priority 30
    rest 1
    bend_range 23
    rest 17
    note G2, 127, 575
    rest 575
    end
    .byte 0xff, 0x00, 0x00
