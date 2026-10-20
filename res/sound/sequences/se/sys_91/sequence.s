track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
    program 0
    bend_range 10
    mod_depth 80
    expression 127
    pan 64
    priority 127
    mod_speed 30
    mod_type 0
    mod_range 20
    mod_delay 0
    volume 100
    note D4, 38, 5
    rest 6
    note Ds5, 64, 5
    rest 5
    end
track_01:
    note_wait 0
    program 1
    bend_range 10
    mod_depth 80
    expression 127
    pan 64
    priority 127
    mod_speed 30
    mod_type 0
    mod_range 20
    mod_delay 0
    volume 60
    note D4, 100, 5
    rest 6
    note A5, 100, 5
    rest 5
    end
    .byte 0xff
