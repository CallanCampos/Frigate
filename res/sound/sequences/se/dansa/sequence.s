track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 120
    sweep_pitch -1104
    pitch_bend -12
    volume 127
    pan 64
    program 49
    note Ds5, 50, 3
    rest 4
    note G6, 90, 11
    sweep_pitch 0
    rest 11
    end
track_01:
    note_wait 0
    program 49
    rest 6
    volume 70
    pan 64
    priority 10
    rest 6
    sweep_pitch -1104
    pitch_bend -12
    note Ds5, 40, 3
    rest 4
    note G6, 40, 11
    sweep_pitch 0
    rest 11
    end
    .byte 0xff
