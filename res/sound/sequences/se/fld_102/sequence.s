track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 120
    program 0
    volume 127
    pan 45
    expression 127
    bend_range 12
    priority 127
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
    note C4, 127, 53
    rest 54
    note As3, 98, 59
    rest 60
    note Ds4, 64, 50
    rest 51
    end
track_01:
    note_wait 0
    program 0
    volume 100
    pan 80
    expression 127
    bend_range 12
    priority 127
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    pitch_bend 1
    mod_delay 0
    rest 26
    note C4, 53, 53
    rest 54
    note Gs3, 29, 59
    rest 60
    note Ds4, 29, 50
    rest 51
    end
    .byte 0xff, 0x00, 0x00
