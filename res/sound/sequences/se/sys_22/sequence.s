track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 128
    program 109
    volume 127
    bend_range 12
    pitch_bend -2
    note Ds4, 90, 2
    rest 4
    note Gs4, 100, 7
    rest 10
    note F6, 70, 33
    rest 33
    end
track_01:
    note_wait 0
    program 109
    volume 127
    bend_range 2
    pitch_bend -15
    rest 4
    note Gs4, 60, 7
    rest 10
    note F6, 60, 33
    rest 33
    end
    .byte 0xff, 0x00, 0x00, 0x00
