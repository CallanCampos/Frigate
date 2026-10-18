track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
    volume 29
    expression 127
    mod_depth 0
    program 0
    priority 126
    note A3, 127, 23
    rest 3
    volume 51
    rest 3
    volume 77
    rest 3
    volume 97
    rest 3
    volume 127
    rest 3
    volume 111
    rest 3
    volume 83
    rest 5
    end
track_01:
    note_wait 0
    volume 127
    expression 127
    mod_depth 0
    program 1
    bend_range 56
    priority 127
    note F1, 100, 17
    rest 3
    pitch_bend 4
    rest 3
    pitch_bend 15
    rest 3
    pitch_bend 26
    rest 3
    pitch_bend 63
    rest 6
    pitch_bend 0
    note A2, 87, 14
    rest 14
    end
    .byte 0xff, 0x00, 0x00, 0x00
