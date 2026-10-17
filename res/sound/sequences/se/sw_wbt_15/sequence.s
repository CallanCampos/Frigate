track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    note_wait 1
    tempo 75
    attack 72
    mod_depth 8
    mod_range 1
    mod_speed 3
    mod_type 2
    portamento_time 0
    portamento 1
    portamento_key A2
    program 1
    note E5, 127, 90
    end
track_01:
    program 0
    tempo 75
    rest 106
    note A2, 127, 0
    end
track_02:
    program 1
    tempo 75
    rest 106
    note D4, 127, 24
    end
    .byte 0xff
