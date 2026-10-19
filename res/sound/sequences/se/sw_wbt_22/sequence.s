track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    tempo 75
    pan 117
    program 1
    note B2, 127, 0
    end
track_01:
    tempo 75
    rest 1
    pan 10
    program 1
    note B2, 127, 0
    end
track_02:
    program 0
    note C3, 127, 0
    end
    .byte 0xff, 0x00, 0x00, 0x00
