track_00:
    alloc_tracks 0x000f
    open_track 0, track_00_0017
    open_track 1, track_01
    open_track 2, track_02
    open_track 3, track_03
track_00_0017:
    program 0
    tempo 75
    pan 64
    portamento_key F2
    portamento_time 0
    portamento 1
    release 101
    note F3, 127, 196
    end
track_01:
    program 1
    tempo 75
    note F5, 1, 196
    attack 80
    decay 45
    sustain 32
    release 115
    note Fs3, 127, 0
    end
track_02:
    program 0
    tempo 75
    mod_depth 45
    mod_range 1
    mod_speed 9
    mod_type 2
    note F4, 1, 196
    attack 80
    decay 45
    sustain 32
    release 115
    note F5, 127, 80
    end
track_03:
    program 0
    tempo 75
    attack 8
    pan 64
    portamento_key F3
    portamento_time 0
    portamento 1
    mod_depth 40
    mod_range 1
    mod_speed 5
    mod_type 2
    release 101
    note F4, 108, 196
    end
    .byte 0xff
