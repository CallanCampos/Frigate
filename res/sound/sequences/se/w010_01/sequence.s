track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 220
    program 0
    volume 60
    pan 64
    bend_range 12
    pitch_bend 78
    note G6, 127, 20
    rest 4
    pitch_bend 46
    rest 3
    pitch_bend 0
    rest 2
    pitch_bend -45
    rest 3
    pitch_bend -67
    rest 2
    pitch_bend -110
    rest 2
    pitch_bend -128
    rest 5
    pitch_bend 127
    note C6, 127, 2
    rest 3
    note G6, 127, 2
    rest 4
    note G6, 100, 2
    rest 5
    note G6, 60, 14
    rest 19
    note G6, 30, 14
    rest 18
    note G6, 20, 14
    rest 15
    end
track_01:
    note_wait 0
    program 1
    volume 127
    pan 64
    bend_range 12
    pitch_bend -128
    note C5, 127, 23
    rest 24
    note C6, 127, 47
    rest 47
    end
    .byte 0xff, 0x00, 0x00, 0x00
