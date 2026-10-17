track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    note_wait 0
    tempo 164
    release 100
    program 0
    note G3, 127, 11
    rest 12
    note A3, 127, 11
    rest 12
    note C4, 127, 11
    rest 12
    note E4, 127, 11
    rest 12
    note Gs4, 127, 11
    rest 12
    note C5, 127, 11
    rest 12
    note G5, 127, 11
    rest 12
    note As5, 127, 11
    rest 11
    end
track_01:
    note_wait 0
    tempo 164
    release 100
    program 0
    rest 3
    pan 24
    note G4, 94, 11
    rest 12
    pan 104
    note A4, 94, 11
    rest 12
    pan 24
    note C5, 94, 11
    rest 12
    pan 104
    note E5, 94, 11
    rest 12
    pan 24
    note Gs5, 94, 11
    rest 12
    pan 104
    note C6, 94, 11
    rest 12
    pan 24
    note G6, 94, 11
    rest 12
    pan 104
    note As6, 94, 11
    rest 11
    end
track_02:
    note_wait 0
    tempo 164
    program 3
    expression 16
    note Cs6, 120, 160
    mod_depth 28
    mod_range 2
    mod_speed 8
    mod_type 2
    rest 12
    expression 54
    rest 12
    expression 80
    rest 12
    expression 103
    rest 12
    expression 127
    rest 12
    expression 118
    rest 12
    expression 103
    rest 12
    expression 93
    rest 12
    expression 75
    rest 12
    expression 55
    rest 12
    expression 32
    rest 12
    expression 18
    rest 12
    expression 9
    rest 6
    expression 4
    rest 5
    end
    .byte 0xff, 0x00, 0x00, 0x00
