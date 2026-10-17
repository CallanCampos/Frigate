track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
    volume 127
    expression 127
    mod_depth 0
    program 0
    priority 127
    note C4, 127, 239
    rest 239
    end
track_01:
    note_wait 0
    volume 85
    expression 127
    mod_depth 0
    priority 127
    rest 12
    program 1
    rest 42
    note C4, 127, 41
    rest 42
    note C4, 89, 29
    rest 30
    note C4, 56, 29
    rest 30
    note C4, 31, 29
    rest 29
    end
    .byte 0xff, 0x00, 0x00, 0x00
