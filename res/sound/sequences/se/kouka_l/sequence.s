track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 89
    program 123
    volume 127
    note G3, 80, 6
    rest 6
    note As3, 80, 12
    rest 12
    end
track_01:
    note_wait 0
    program 73
    volume 127
    note As2, 127, 18
    rest 18
    end
    .byte 0xff, 0x00, 0x00, 0x00
