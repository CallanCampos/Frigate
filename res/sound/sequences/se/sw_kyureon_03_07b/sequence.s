track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    tempo 75
    pan 10
    portamento_key As3
    portamento_time 0
    portamento 1
    pitch_bend 72
    note G5, 127, 628
    end
track_01:
    program 0
    tempo 75
    rest 1
    pan 118
    portamento_key As3
    portamento_time 0
    portamento 1
    pitch_bend 72
    note G5, 127, 628
    end
    .byte 0xff
