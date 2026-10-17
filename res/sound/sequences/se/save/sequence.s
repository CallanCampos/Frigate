track_00:
    alloc_tracks 0x0003
    open_track 1, track_01
    note_wait 0
    tempo 150
    bend_range 2
    pitch_bend 34
    volume 127
    pan 64
    program 3
    priority 64
    note E4, 95, 5
    rest 6
    note B3, 95, 5
    rest 6
    note E4, 95, 5
    rest 6
    note B4, 95, 5
    rest 6
    note C6, 95, 11
    rest 12
    note C7, 95, 47
    rest 47
    end
track_01:
    note_wait 0
    program 43
    volume 127
    pan 64
    priority 10
    note E4, 100, 5
    rest 6
    note B3, 100, 5
    rest 6
    note E4, 100, 5
    rest 6
    note B4, 100, 5
    rest 6
    note C6, 100, 11
    rest 12
    note C7, 100, 47
    rest 47
    end
    .byte 0xff, 0x00, 0x00
