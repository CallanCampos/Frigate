track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    portamento_key A3
    portamento_time 220
    portamento 1
    mod_depth 9
    mod_delay 18
    mod_range 1
    mod_speed 1
    mod_type 1
    attack 48
    decay 127
    sustain 120
    release 84
    note G2, 110, 130
    end
track_01:
    program 0
    portamento_key Cs4
    portamento_time 150
    portamento 1
    mod_depth 32
    mod_delay 32
    mod_range 2
    mod_speed 4
    mod_type 0
    attack 15
    decay 127
    sustain 120
    release 80
    note C5, 127, 130
    end
    .byte 0xff, 0x00, 0x00
