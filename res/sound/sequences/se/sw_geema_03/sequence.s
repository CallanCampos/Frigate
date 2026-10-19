track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
    end
track_01:
    note_wait 0
    program 0
    expression 127
    pan 67
    note Cs3, 120, 161
    rest 156
    program 2
    rest 6
    note C2, 120, 347
    note Gs3, 120, 155
    rest 60
    note F3, 70, 155
    rest 192
    expression 126
    rest 12
    expression 122
    rest 12
    expression 112
    rest 12
    expression 100
    rest 12
    expression 89
    rest 12
    expression 69
    rest 12
    expression 49
    rest 12
    expression 19
    rest 12
    expression 0
    rest 12
    expression 0
    end
    .byte 0xff, 0x00, 0x00
