track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    program 1
    tempo 75
    portamento_time 0
    portamento 1
    portamento_key A2
    note E2, 127, 140
    release 108
    end
track_01:
    program 2
    tempo 75
    rest 120
    note As1, 127, 0
    end
track_02:
    program 2
    tempo 75
    rest 125
    note F2, 127, 0
    end
    .byte 0xff, 0x00
