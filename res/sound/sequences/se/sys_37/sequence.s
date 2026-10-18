track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 98
    end
track_01:
    note_wait 0
    volume 127
    expression 127
    mod_depth 0
    program 1
    note B3, 127, 2
    rest 3
    note E4, 127, 2
    rest 3
    note B3, 78, 2
    rest 3
    note E4, 77, 2
    rest 3
    note B3, 32, 2
    rest 3
    note E4, 34, 2
    rest 2
    end
    .byte 0xff
