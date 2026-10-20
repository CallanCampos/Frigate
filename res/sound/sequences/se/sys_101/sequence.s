track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 150
    program 3
    volume 127
    note E5, 100, 12
    rest 12
    note Fs5, 100, 12
    rest 12
    note A5, 100, 12
    rest 12
    note D6, 125, 24
    rest 24
    note D6, 50, 24
    rest 24
    note D6, 30, 24
    rest 24
    end
track_01:
    note_wait 0
    program 73
    volume 100
    note E4, 100, 12
    rest 12
    note Fs4, 100, 12
    rest 12
    note A4, 100, 12
    rest 12
    note D5, 126, 24
    rest 24
    note D5, 50, 24
    rest 24
    note D5, 30, 24
    rest 24
    end
    .byte 0xff, 0x00, 0x00
