track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 120
    program 114
    volume 127
    pitch_bend -15
    pan 64
    note C4, 110, 7
    rest 8
    note C3, 110, 7
    rest 8
    note C2, 110, 7
    rest 8
    note C2, 22, 7
    rest 7
    end
track_01:
    note_wait 0
    pan 64
    rest 12
    program 114
    volume 80
    pitch_bend -15
    priority 10
    note C4, 40, 7
    rest 8
    note C3, 40, 7
    rest 8
    note C2, 40, 7
    rest 8
    note C2, 40, 7
    rest 7
    end
    .byte 0xff, 0x00, 0x00
