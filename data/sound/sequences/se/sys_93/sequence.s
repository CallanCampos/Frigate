track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 140
    program 0
    volume 60
    expression 127
    mod_depth 0
    priority 127
    rest 1
    note E3, 62, 5
    rest 5
    end
track_01:
    note_wait 0
    program 1
    volume 50
    expression 127
    mod_depth 0
    bend_range 27
    priority 127
    note C9, 99, 5
    rest 9
    note B6, 48, 5
    rest 5
    end
    .byte 0xff, 0x00, 0x00, 0x00
