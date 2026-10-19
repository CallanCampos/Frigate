track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    note As3, 127, 10
    note As3, 109, 10
    note As3, 101, 10
    note As3, 92, 10
    note As3, 80, 10
    rest 33
    program 1
    tempo 75
    attack 40
    mod_depth 22
    mod_range 2
    mod_speed 3
    mod_type 1
    release 100
    note C5, 100, 250
    end
track_01:
    program 1
    tempo 75
    rest 80
    attack 40
    mod_depth 34
    mod_range 2
    mod_speed 1
    mod_type 2
    release 100
    note C5, 127, 260
    end
    .byte 0xff, 0x00, 0x00
