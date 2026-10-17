track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 73
    mod_depth 0
    volume 127
    expression 127
    program 0
    note C4, 75, 11
    rest 12
    note E4, 85, 11
    rest 12
    note E4, 50, 5
    rest 6
    note E4, 51, 5
    rest 6
    note E4, 39, 5
    rest 6
    note E4, 11, 5
    rest 5
    end
track_01:
    note_wait 0
    volume 85
    expression 127
    mod_depth 3
    pitch_bend -14
    program 1
    rest 9
    note C4, 75, 11
    rest 12
    note E4, 85, 11
    rest 12
    note E4, 63, 11
    rest 12
    note E4, 27, 11
    rest 12
    note E4, 9, 11
    rest 11
    end
    .byte 0xff, 0x00, 0x00, 0x00
