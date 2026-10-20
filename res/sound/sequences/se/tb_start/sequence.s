track_00:
    alloc_tracks 0x0007
    open_track 1, track_01
    open_track 2, track_02
    note_wait 0
    tempo 150
    end
track_01:
    note_wait 0
    program 125
    volume 127
    pan 64
    pitch_bend 2
    note Fs5, 90, 19
    rest 19
    end
track_02:
    note_wait 0
    rest 30
    program 125
    volume 80
    pan 64
    pitch_bend 2
    note Fs5, 40, 19
    rest 19
    end
    .byte 0xff, 0x00, 0x00
