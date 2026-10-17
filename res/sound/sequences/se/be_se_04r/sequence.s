track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 150
    program 18
    volume 127
    pan 64
    volume 127
    priority 64
    note F7, 110, 2
    rest 9
    pan 70
    note Ds8, 40, 4
    rest 7
    pan 59
    note G7, 110, 2
    rest 3
    program 0
    pan 64
    note Ds8, 110, 2
    rest 7
    pan 70
    note Ds8, 20, 4
    rest 7
    pan 59
    note G7, 64, 2
    rest 3
    pan 64
    note Ds8, 64, 2
    rest 7
    pan 75
    note Ds8, 20, 4
    rest 7
    pan 51
    note G7, 64, 2
    rest 2
    pan 64
    note Ds8, 64, 2
    rest 2
    end
track_01:
    note_wait 0
    program 126
    volume 127
    pan 64
    priority 30
    note D6, 40, 2
    rest 2
    end
    .byte 0xff, 0x00
