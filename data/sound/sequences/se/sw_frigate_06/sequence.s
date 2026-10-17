track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    pan 10
    attack 127
    decay 22
    sustain 28
    release 100
    volume 127
    bend_range 1
    random pitch_bend -90, -85
    note G3, 127, 180
    end
track_01:
    tempo 75
    program 0
    pan 118
    attack 127
    decay 22
    sustain 28
    release 100
    rest 1
    volume 127
    bend_range 1
    random pitch_bend 75, 80
    note G3, 127, 180
    end
    .byte 0xff
