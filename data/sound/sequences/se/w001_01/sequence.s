track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
    program 0
    volume 50
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    pan 64
    mod_depth 0
    expression 127
    note E5, 125, 47
    rest 1
    pitch_bend 54
    rest 7
    pitch_bend 79
    rest 7
    pitch_bend 101
    rest 8
    pitch_bend 123
    rest 7
    pitch_bend 127
    rest 17
    end
track_01:
    note_wait 0
    program 1
    volume 127
    bend_range 12
    sweep_pitch 0
    sweep_pitch 0
    pan 64
    mod_depth 0
    expression 127
    note E4, 125, 47
    note F4, 125, 47
    rest 47
    end
    .byte 0xff, 0x00
