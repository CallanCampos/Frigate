track_00:
    random_var 0, 100
    cmp_le 0, 50
    if jump label_0016
    cmp_le 0, 100
    if jump label_002d
label_0016:
    random pan 14, 44
    program 0
    note C4, 127, 0
    random pan 84, 114
    program 1
    note C4, 84, 0
    end
label_002d:
    random pan 84, 114
    program 1
    note C4, 127, 0
    random pan 14, 44
    program 0
    note C4, 82, 0
    end
    .byte 0xff, 0x00, 0x00, 0x00
