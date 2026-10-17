track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    tempo 75
    program 0
    pan 1
    note F3, 127, 0
    end
track_01:
    tempo 75
    rest 1
    program 0
    pan 127
    pitch_bend 60
    note F3, 127, 0
    end
track_02:
    tempo 75
    program 1
    note F2, 127, 0
    end
    .byte 0xff, 0x00, 0x00
