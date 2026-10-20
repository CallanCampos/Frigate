track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0015
    open_track 1, track_01
    open_track 2, track_02
    tempo 75
track_00_0015:
    program 0
    note E3, 127, 0
    end
track_01:
    rest 40
    program 0
    attack 127
    decay 10
    sustain 64
    release 117
    pitch_bend -11
    note E3, 84, 0
    end
track_02:
    rest 80
    program 0
    attack 127
    decay 10
    sustain 64
    release 117
    pitch_bend -22
    note E3, 55, 0
    end
    .byte 0xff
