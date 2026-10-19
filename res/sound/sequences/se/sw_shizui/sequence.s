track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 120
    program 0
    random pitch_bend -90, 100
    note A3, 127, 0
    end
track_01:
    tempo 75
    program 1
    random pitch_bend -100, 80
    note A3, 127, 0
    end
    .byte 0xff
