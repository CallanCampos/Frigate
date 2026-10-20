track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    note_wait 0
    tempo 300
    program 0
    volume 127
    bend_range 12
    sweep_pitch 0
    pan 64
    mod_depth 0
    expression 127
    note Gs3, 127, 227
    rest 227
    end
track_01:
    program 1
    note_wait 1
    tempo 75
    mod_depth 20
    mod_range 1
    mod_speed 18
    mod_type 1
    attack 28
    rest 64
    note Fs4, 101, 60
    release 110
    bend_range 8
    rest 2
    pitch_bend 15
    rest 2
    pitch_bend 30
    rest 2
    pitch_bend 45
    rest 2
    pitch_bend 60
    rest 20
    pitch_bend 58
    rest 4
    pitch_bend 56
    rest 3
    pitch_bend 54
    rest 2
    pitch_bend 52
    rest 1
    pitch_bend 50
    end
track_02:
    note_wait 1
    tempo 75
    rest 124
    program 2
    random volume 127, 105
    pan 72
    note Fs3, 127, 10
    program 2
    pan 56
    random volume 122, 100
    note Fs3, 110, 0
    end
    .byte 0xff
