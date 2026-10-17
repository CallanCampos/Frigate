track_00:
    alloc_tracks 0x000f
    open_track 1, track_01
    open_track 2, track_02
    open_track 3, track_03
    note_wait 0
    tempo 120
    program 60
    volume 100
    pan 80
    expression 127
    bend_range 12
    priority 64
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
    note A3, 110, 11
    note A5, 100, 11
    rest 12
    note B3, 116, 9
    note B5, 106, 9
    rest 36
    program 127
    note C5, 100, 1
    end
track_01:
    note_wait 0
    program 61
    volume 100
    pan 50
    expression 127
    bend_range 12
    priority 64
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
    note A3, 110, 11
    note A4, 110, 11
    rest 12
    note B3, 116, 9
    note B4, 116, 9
    rest 9
    end
track_02:
    note_wait 0
    program 60
    volume 50
    pan 120
    expression 127
    bend_range 12
    priority 64
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
    rest 9
    note A5, 100, 11
    rest 12
    note B5, 106, 9
    rest 9
    end
track_03:
    note_wait 0
    program 27
    volume 110
    pan 64
    expression 127
    bend_range 12
    priority 64
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
    note A1, 110, 11
    rest 12
    note B1, 116, 9
    rest 9
    end
    .byte 0xff, 0x00, 0x00, 0x00
