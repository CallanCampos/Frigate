track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    tempo 75
    attack 120
    decay 78
    sustain 8
    release 115
    note F3, 127, 82
    end
track_01:
    program 0
    tempo 75
    attack 40
    decay 78
    sustain 8
    release 115
    note F4, 127, 82
    end
    .byte 0xff
