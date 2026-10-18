track_00:
    tempo 75
    rest 70
    call label_002a
    pan 84
    note D3, 127, 56
    call label_002a
    pan 44
    note D3, 127, 44
    call label_002a
    pan 104
    note D3, 127, 64
    call label_002a
    pan 24
    note C3, 127, 0
    end
label_002a:
    program 0
    release 100
    random pitch_bend 0, 127
    random expression 120, 127
    return
    .byte 0xff
