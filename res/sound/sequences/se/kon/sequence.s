track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 150
    program 50
    volume 127
    bend_range 2
    pitch_bend 2
    pan 64
    note F5, 110, 23
    rest 24
    note F5, 60, 11
    rest 12
    note F5, 30, 11
    rest 11
    end
track_01:
    note_wait 0
    program 50
    volume 127
    pitch_bend 0
    pan 64
    note F5, 100, 17
    rest 24
    note F5, 30, 11
    rest 11
    end
    .byte 0xff, 0x00, 0x00, 0x00
