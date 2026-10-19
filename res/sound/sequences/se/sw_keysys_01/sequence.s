track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
label_000d:
    program 4
    volume 127
    volume 47
    bend_range 12
    pitch_bend 16
    note G5, 100, 96
    rest 5
    pan 89
    rest 5
    volume 102
    rest 12
    pan 49
    rest 4
    volume 36
    rest 12
    pan 77
    rest 6
    volume 85
    rest 6
    pan 48
    rest 14
    volume 57
    rest 15
    pan 78
    rest 10
    pan 54
    rest 7
    note G5, 100, 96
    volume 33
    rest 6
    pan 75
    rest 15
    volume 42
    rest 17
    volume 65
    pan 38
    rest 21
    volume 71
    rest 1
    pan 76
    rest 16
    pan 54
    rest 1
    volume 39
    rest 5
    pan 68
    rest 9
    pan 43
    rest 5
    jump label_000d
    .byte 0xff
track_01:
    note_wait 0
label_0078:
    program 35
    volume 47
    bend_range 12
    pitch_bend 16
    note G5, 127, 96
    rest 10
    volume 102
    rest 16
    volume 36
    rest 18
    volume 85
    rest 20
    volume 57
    rest 32
    note G5, 127, 96
    volume 33
    rest 21
    volume 42
    rest 17
    volume 65
    rest 21
    volume 71
    rest 18
    volume 39
    rest 19
    jump label_0078
    .byte 0xff, 0xff, 0x00, 0x00
