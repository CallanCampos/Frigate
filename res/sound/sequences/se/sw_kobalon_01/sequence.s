track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    attack 70
    program 0
    tempo 75
    pan 110
    attack 40
    note C4, 110, 0
    end
track_01:
    attack 70
    mod_depth 33
    mod_speed 7
    mod_type 1
    program 0
    attack 72
    tempo 75
    pan 110
    note G4, 80, 32
    pan 72
    note C4, 86, 0
    end
    .byte 0xff, 0x00
