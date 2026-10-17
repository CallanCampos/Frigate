track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    note Gs3, 127, 0
    end
track_01:
    tempo 75
    random rest 9, 15
    program 1
    note F3, 118, 0
    end
    .byte 0xff, 0x00
