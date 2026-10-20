track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 297
    program 0
    pan 127
    bend_range 20
    volume 100
    mod_speed 50
    mod_range 50
    priority 127
    note A3, 100, 167
    pitch_bend 0
    rest 167
    end
track_01:
    note_wait 0
    program 0
    pan 0
    bend_range 20
    volume 100
    mod_speed 50
    mod_range 50
    priority 127
    pitch_bend -3
    rest 23
    note A3, 100, 167
    rest 167
    end
    .byte 0xff, 0x00, 0x00
