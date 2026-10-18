track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    tempo 75
    attack 127
    decay 10
    sustain 68
    release 100
    note C4, 127, 180
    end
track_01:
    program 0
    tempo 75
    attack 127
    decay 10
    sustain 68
    release 100
    note F3, 127, 180
    end
    .byte 0xff, 0x00, 0x00
