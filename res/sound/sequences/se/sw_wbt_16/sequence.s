track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    note_wait 1
    tempo 75
    pan 104
    program 0
    note Fs3, 127, 0
    rest 3
    pan 96
    rest 3
    pan 88
    rest 3
    pan 80
    rest 3
    pan 72
    rest 3
    pan 66
    end
track_01:
    note_wait 1
    tempo 75
    rest 1
    pan 24
    program 0
    note Fs3, 127, 0
    rest 3
    pan 32
    rest 3
    pan 40
    rest 3
    pan 48
    rest 3
    pan 56
    rest 3
    pan 62
    end
track_02:
    program 1
    tempo 75
    rest 30
    note Ds3, 127, 0
    end
    .byte 0xff, 0x00, 0x00
