track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 150
    program 3
    volume 127
    pan 64
    rest 6
    note A5, 127, 15
    rest 16
    pan 17
    rest 2
    note E6, 55, 10
    rest 12
    pan 107
    rest 6
    note A5, 45, 10
    rest 16
    pan 14
    rest 2
    note E6, 33, 12
    rest 12
    end
track_01:
    note_wait 0
    program 3
    volume 127
    pan 66
    note E6, 100, 10
    rest 36
    note E6, 38, 10
    rest 41
    note E6, 15, 10
    rest 10
    end
    .byte 0xff, 0x00
