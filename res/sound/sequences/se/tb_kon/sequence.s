track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 150
    program 98
    volume 127
    pitch_bend 2
    pan 64
    note Gs6, 90, 6
    rest 2
    pitch_bend -4
    rest 2
    pitch_bend -20
    volume 68
    rest 3
    volume 33
    end
track_01:
    note_wait 0
    rest 12
    program 98
    volume 50
    pitch_bend 2
    pan 64
    priority 10
    note Gs6, 30, 6
    rest 2
    pitch_bend -4
    rest 2
    pitch_bend -20
    rest 2
    end
    .byte 0xff
