track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    release 94
    portamento_time 20
    portamento 1
    portamento_key F4
    note F5, 127, 0
    end
track_01:
    program 0
    release 94
    note F3, 127, 0
    end
    .byte 0xff, 0x00
