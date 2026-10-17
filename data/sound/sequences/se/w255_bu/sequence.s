track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 250
    program 0
    note E3, 127, 209
    volume 127
    pan 64
    expression 127
    bend_range 80
    priority 127
    mod_depth 0
    mod_speed 100
    mod_type 0
    mod_range 20
    rest 1
    mod_delay 0
    rest 5
    pitch_bend 0
    rest 203
    end
track_01:
    note_wait 0
    program 1
    note C6, 70, 209
    volume 127
    pan 64
    expression 127
    bend_range 80
    priority 126
    mod_depth 50
    mod_speed 60
    mod_type 0
    mod_range 20
    rest 1
    mod_delay 0
    rest 5
    pitch_bend 0
    rest 203
    end
    .byte 0xff, 0x00, 0x00
