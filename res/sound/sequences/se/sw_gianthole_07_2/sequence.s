track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    pan 40
    tempo 75
    rest 1
    note Ds3, 127, 0
    end
track_01:
    program 0
    pan 88
    note D3, 127, 0
    end
    .byte 0xff, 0x00
