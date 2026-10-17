track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 300
    program 35
    volume 127
    bend_range 2
    note As4, 68, 5
    rest 6
    note G5, 127, 12
    rest 12
    note G5, 104, 5
    rest 6
    note G5, 40, 5
    rest 5
    note G5, 60, 5
    rest 7
    note G5, 30, 5
    rest 6
    note G5, 30, 5
    rest 5
    end
track_01:
    note_wait 0
    program 35
    volume 90
    bend_range 12
    pitch_bend 0
    note As4, 68, 5
    rest 6
    note G5, 127, 11
    rest 12
    note G5, 104, 5
    rest 6
    note G5, 40, 5
    rest 6
    note G5, 60, 5
    rest 6
    note G5, 30, 5
    rest 6
    note G5, 30, 5
    rest 6
    end
    .byte 0xff, 0x00, 0x00
