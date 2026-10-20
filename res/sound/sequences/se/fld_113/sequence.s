track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
    program 0
    mod_depth 0
    volume 127
    mod_speed 16
    pan 10
    mod_type 0
    mod_range 1
    bend_range 12
    mod_delay 0
    priority 127
    note C4, 127, 313
    rest 313
    end
track_01:
    note_wait 0
    program 0
    pitch_bend 0
    mod_depth 0
    volume 127
    mod_speed 16
    pan 100
    mod_type 0
    mod_range 1
    bend_range 12
    mod_delay 0
    priority 127
    rest 65
    note C4, 77, 313
    rest 313
    end
    .byte 0xff, 0x00, 0x00
