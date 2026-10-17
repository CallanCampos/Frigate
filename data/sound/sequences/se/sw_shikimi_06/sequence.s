track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    rest 8
    call label_0053
    random pan 44, 84
    random note B3, 127, 6, 11
    call label_0053
    random pan 44, 84
    random note A3, 127, 15, 22
    call label_0053
    random pan 44, 84
    random note B3, 127, 14, 19
    call label_0053
    random pan 44, 84
    note A3, 127, 0
    end
label_0053:
    program 1
    bend_range 2
    random pitch_bend -64, 127
    random expression 88, 127
    return
track_01:
    tempo 75
    call label_00a8
    random pan 44, 84
    random note C4, 127, 10, 18
    call label_00a8
    random pan 44, 84
    random note C4, 127, 18, 26
    call label_00a8
    random pan 44, 84
    random note C4, 127, 13, 21
    call label_00a8
    random pan 44, 84
    note C4, 127, 0
    end
label_00a8:
    program 2
    release 100
    random pitch_bend -64, 64
    random expression 70, 127
    return
    .byte 0xff, 0x00, 0x00
