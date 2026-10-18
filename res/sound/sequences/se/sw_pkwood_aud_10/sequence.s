track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    note D4, 127, 10
    rest 10
    note Gs4, 127, 10
    rest 23
    note D4, 127, 10
    rest 10
    note Gs4, 127, 10
    rest 23
    note D4, 127, 10
    rest 10
    note Gs4, 127, 10
    end
track_01:
    program 119
    note C2, 60, 10
    rest 10
    note Fs2, 60, 10
    rest 23
    note C2, 60, 10
    rest 10
    note Fs2, 60, 10
    rest 23
    note C2, 60, 10
    rest 10
    note Fs2, 60, 10
    end
    .byte 0xff
