track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 100
    program 0
    volume 120
    pan 64
    expression 127
    bend_range 12
    priority 64
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
    note G3, 117, 5
    rest 11
    note Ds4, 13, 5
    rest 5
    end
track_01:
    note_wait 0
    program 1
    volume 120
    pan 64
    expression 127
    bend_range 12
    priority 64
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
    rest 8
    priority 10
    note F6, 21, 5
    rest 9
    note F6, 8, 5
    rest 9
    note F6, 4, 5
    rest 5
    end
    .byte 0xff, 0x00, 0x00, 0x00
