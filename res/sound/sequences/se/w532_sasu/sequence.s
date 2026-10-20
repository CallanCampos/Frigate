track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 120
    program 0
    volume 127
    pan 64
    expression 127
    bend_range 12
    priority 64
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
    note C5, 127, 28
    rest 28
    end
track_01:
    note_wait 0
    program 1
    pitch_bend 0
    volume 100
    pan 64
    bend_range 12
    priority 64
    mod_depth 100
    mod_speed 30
    mod_type 0
    mod_range 20
    mod_delay 0
    note C5, 127, 2
    expression 127
    rest 5
    note Cs6, 38, 21
    rest 3
    pitch_bend 13
    rest 3
    pitch_bend 45
    rest 3
    expression 100
    pitch_bend 82
    rest 3
    expression 72
    pitch_bend 119
    rest 3
    expression 47
    pitch_bend 127
    rest 3
    expression 27
    rest 3
    end
    .byte 0xff, 0x00
