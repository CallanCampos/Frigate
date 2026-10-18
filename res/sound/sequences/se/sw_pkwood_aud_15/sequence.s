track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    attack 100
    release 98
    note C4, 127, 160
    end
track_01:
    program 0
    attack 98
    release 98
    note F4, 52, 160
    end
    .byte 0xff
