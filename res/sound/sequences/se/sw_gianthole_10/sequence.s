track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    program 0
    tempo 75
    pan 64
    attack 40
    note C3, 127, 32
    note F3, 127, 0
    end
track_01:
    program 0
    tempo 75
    rest 2
    pan 114
    attack 65
    note As2, 127, 0
    end
track_02:
    program 0
    tempo 75
    pan 14
    attack 90
    note As2, 127, 0
    end
    .byte 0xff, 0x00
