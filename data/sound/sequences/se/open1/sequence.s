track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 220
    rest 36
    program 1
    volume 127
    pan 64
    note D5, 97, 4
    rest 7
    volume 53
    rest 1
    note Ds8, 87, 29
    rest 1
    volume 127
    rest 21
    volume 101
    rest 8
    end
track_01:
    note_wait 0
    program 0
    volume 127
    pan 64
    note D4, 127, 47
    rest 47
    end
    .byte 0xff, 0x00
