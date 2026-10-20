track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    tempo 75
    pan 64
    pitch_bend 0
    note D3, 127, 0
    end
track_01:
    program 0
    tempo 75
    rest 1
    pan 64
    pitch_bend 75
    note F3, 1, 76
    rest 1
    attack 84
    portamento_key F3
    portamento_time 20
    portamento 1
    note Gs3, 110, 0
    end
    .byte 0xff, 0x00
