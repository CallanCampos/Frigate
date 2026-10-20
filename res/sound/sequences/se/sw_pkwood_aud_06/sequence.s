track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    note D3, 127, 0
    end
track_01:
    random rest 95, 134
    attack 84
    random pan 40, 88
    program 1
    random volume 97, 60
    note C4, 127, 0
    end
    .byte 0xff, 0x00, 0x00
