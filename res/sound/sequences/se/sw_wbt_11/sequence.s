track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    mod_depth 22
    mod_range 2
    mod_speed 4
    mod_type 1
    portamento_time 10
    portamento 1
    portamento_key As2
    release 111
    note Fs3, 127, 200
    end
track_01:
    tempo 75
    program 0
    attack 100
    mod_depth 34
    mod_range 2
    mod_speed 8
    mod_type 1
    portamento_time 6
    portamento 1
    portamento_key C1
    release 110
    note F3, 127, 198
    end
    .byte 0xff
