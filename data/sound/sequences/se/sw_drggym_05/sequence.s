track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    attack 127
    decay 10
    sustain 64
    release 117
    note As3, 127, 0
    end
track_01:
    program 0
    attack 127
    decay 10
    sustain 64
    release 117
    note E3, 127, 0
    end
    .byte 0xff, 0x00, 0x00
