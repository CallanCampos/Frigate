track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    tempo 75
    call label_0042
    note As1, 127, 48
    call label_0042
    note As1, 124, 50
    call label_0042
    note As1, 121, 50
    call label_0042
    note As1, 118, 50
    call label_0042
    note As1, 115, 50
    call label_0042
    release 55
    note As1, 112, 100
    end
label_0042:
    program 1
    mod_depth 42
    mod_range 2
    mod_speed 1
    mod_type 1
    random pitch_bend -64, 64
    portamento_time 0
    portamento 1
    portamento_key C2
    release 127
    return
track_01:
    tempo 75
    rest 64
    call label_0097
    pan 126
    note F2, 127, 50
    call label_0097
    pan 118
    note F2, 127, 50
    call label_0097
    pan 110
    note F2, 127, 42
    call label_0097
    pan 102
    note F2, 127, 44
    call label_0097
    pan 94
    note F2, 127, 49
    call label_0097
    pan 86
    note E2, 124, 0
    end
label_0097:
    program 2
    release 100
    random transpose 0, 1
    random pitch_bend 0, 127
    random expression 120, 127
    return
track_02:
    tempo 75
    rest 66
    call label_00ea
    pan 1
    note F2, 127, 50
    call label_00ea
    pan 8
    note F2, 127, 50
    call label_00ea
    pan 16
    note F2, 127, 42
    call label_00ea
    pan 24
    note F2, 127, 40
    call label_00ea
    pan 32
    note F2, 127, 50
    call label_00ea
    pan 40
    note E2, 124, 0
    end
label_00ea:
    program 2
    release 100
    random transpose -2, -1
    random pitch_bend -64, 64
    random expression 120, 127
    return
    .byte 0xff, 0x00, 0x00
