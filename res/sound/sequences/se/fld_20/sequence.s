track_00:
    alloc_tracks 0x0005
    open_track 2, track_02
    note_wait 0
    tempo 110
    program 84
    volume 127
    note As2, 117, 5
    rest 12
    note As3, 84, 15
    rest 12
    volume 112
    rest 6
    volume 69
    rest 6
    volume 87
    end
track_02:
    note_wait 0
    program 84
    volume 127
    rest 24
    volume 112
    note As2, 57, 5
    rest 6
    volume 69
    rest 6
    volume 87
    note As3, 24, 15
    rest 15
    end
    .byte 0xff, 0x00, 0x00
