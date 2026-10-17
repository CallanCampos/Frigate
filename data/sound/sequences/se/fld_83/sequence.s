track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
    volume 127
    expression 127
    mod_depth 0
    bend_range 42
    mod_range 22
    mod_speed 31
    program 0
    priority 64
    note E2, 127, 3
    rest 4
    note A3, 127, 239
    rest 239
    end
track_01:
    note_wait 0
    volume 50
    expression 127
    bend_range 42
    mod_range 22
    mod_speed 31
    program 1
    mod_depth 0
    priority 30
    note D4, 127, 7
    rest 16
    note C6, 122, 77
    rest 24
    mod_depth 15
    rest 72
    note C6, 78, 17
    rest 36
    note C6, 46, 17
    rest 36
    note C6, 29, 17
    rest 36
    note C6, 13, 17
    rest 17
    end
    .byte 0xff
