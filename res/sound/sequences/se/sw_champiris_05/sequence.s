track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    tempo 75
    program 0
    attack 50
    note G1, 70, 20
    attack 127
    random pitch_bend -30, 30
    note G2, 112, 16
    random pitch_bend -30, 30
    note G2, 118, 10
    random pitch_bend -30, 30
    note G2, 124, 0
    end
track_01:
    tempo 75
    program 0
    rest 30
    random pitch_bend -30, 30
    note Ds2, 105, 12
    random pitch_bend -30, 30
    note Ds2, 111, 8
    random pitch_bend -30, 30
    note Ds2, 117, 20
    end
track_02:
    tempo 75
    program 1
    rest 20
    random pitch_bend -80, 80
    note C4, 97, 10
    random pitch_bend -80, 80
    note C4, 100, 6
    random pitch_bend -80, 80
    note C4, 103, 6
    random pitch_bend -80, 80
    note C4, 109, 4
    random pitch_bend -80, 80
    note C4, 115, 4
    random pitch_bend -80, 80
    note C4, 121, 6
    random pitch_bend -80, 80
    note C4, 127, 0
    end
    .byte 0xff, 0x00, 0x00, 0x00
