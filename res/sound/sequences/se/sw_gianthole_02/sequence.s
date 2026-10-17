track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    portamento_key C2
    portamento_time 0
    portamento 1
    program 0
    attack 14
    volume 127
    note F3, 127, 480
    end
track_01:
    tempo 75
    rest 2
    program 0
    attack 12
    portamento_key F3
    portamento_time 17
    portamento 1
    mod_depth 22
    mod_range 2
    mod_speed 12
    mod_type 2
    note As3, 111, 480
    end
    .byte 0xff, 0x00, 0x00
