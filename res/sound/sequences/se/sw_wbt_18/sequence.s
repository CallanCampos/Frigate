track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    note Fs2, 127, 0
    end
track_01:
    program 1
    tempo 75
    release 100
    note F3, 127, 48
    end
    .byte 0xff, 0x00, 0x00
