track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    note C3, 127, 32
    note C3, 127, 0
    end
track_01:
    tempo 75
    program 0
    note G2, 110, 16
    note C3, 125, 62
    end
    .byte 0xff, 0x00, 0x00
