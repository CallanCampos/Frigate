track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 150
    program 3
    volume 127
    pan 64
    pan 64
    volume 127
    bend_range 64
    note C7, 5, 1
    note B7, 127, 6
    rest 6
    note B6, 127, 6
    rest 6
    note B7, 127, 6
    rest 6
    note B7, 30, 6
    rest 6
    end
track_01:
    note_wait 0
    program 3
    pan 64
    volume 127
    bend_range 12
    pitch_bend 0
    note C7, 5, 1
    note B7, 127, 6
    rest 6
    note B6, 127, 6
    rest 6
    note B7, 127, 6
    rest 6
    note B7, 30, 6
    rest 6
    end
    .byte 0xff, 0x00, 0x00
