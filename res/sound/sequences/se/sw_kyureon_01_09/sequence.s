track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    attack 70
    tempo 75
    portamento_key G3
    portamento_time 0
    portamento 1
    pan 64
    release 99
    note C5, 127, 88
    end
track_01:
    program 1
    attack 70
    tempo 75
    portamento_key G4
    portamento_time 0
    portamento 1
    pan 64
    release 99
    note C6, 90, 88
    end
    .byte 0xff
