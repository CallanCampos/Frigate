track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 150
    program 84
    volume 127
    volume 127
    note C5, 90, 2
    rest 7
    note C6, 90, 2
    rest 12
    note E4, 127, 4
    rest 4
    end
track_01:
    note_wait 0
    program 84
    volume 127
    volume 127
    rest 6
    note C5, 50, 2
    rest 7
    note C6, 50, 2
    rest 12
    note E4, 50, 4
    rest 4
    end
    .byte 0xff, 0x00, 0x00
