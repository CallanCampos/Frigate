track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    mod_depth 22
    mod_range 1
    mod_speed 23
    mod_type 1
    random pitch_bend -64, 64
    portamento_time 0
    portamento 1
    portamento_key Fs3
    note E4, 127, 36
    rest 12
    random pitch_bend -64, 64
    portamento_time 0
    portamento 1
    portamento_key Fs3
    note E4, 127, 36
    rest 14
    random pitch_bend -64, 64
    portamento_time 0
    portamento 1
    portamento_key Fs3
    note E4, 127, 36
    rest 14
    random pitch_bend -64, 64
    portamento_time 0
    portamento 1
    portamento_key Fs3
    note E4, 127, 36
    end
track_01:
    program 1
    attack 127
    mod_depth 34
    mod_range 2
    mod_speed 1
    mod_type 1
    random pitch_bend -64, 64
    release 84
    note Gs2, 127, 48
    note Gs2, 127, 50
    note Gs2, 127, 50
    note Gs2, 127, 50
    end
    .byte 0xff, 0x00
