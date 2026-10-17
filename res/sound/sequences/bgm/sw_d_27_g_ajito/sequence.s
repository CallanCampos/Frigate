track_00:
    alloc_tracks 0x061f
    open_track 1, track_01
    open_track 2, track_02
    open_track 3, track_03
    open_track 4, track_04
    open_track 9, track_09
    open_track 10, track_10
    note_wait 0
    tempo 128
    program 1
    portamento_time 0
    pan 81
    volume 100
    expression 127
    bend_range 12
    priority 64
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_003f:
    rest 162
    note C4, 100, 2
    rest 9
    note C4, 100, 5
    rest 291
    note C4, 100, 23
    rest 402
    note C4, 100, 11
    rest 1335
    note C4, 80, 8
    rest 276
    note C4, 40, 41
    rest 597
    jump label_003f
    .byte 0xff
track_01:
    note_wait 0
    program 2
    portamento_time 0
    pan 39
    volume 100
    expression 127
    bend_range 12
    priority 64
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_0085:
    rest 342
    note C4, 100, 11
    rest 24
    note C4, 100, 113
    rest 120
    note C4, 100, 5
    rest 12
    note C4, 100, 2
    rest 54
    note C4, 100, 2
    rest 420
    note C4, 100, 5
    rest 24
    note C4, 100, 2
    rest 156
    note C4, 100, 47
    rest 60
    note C4, 100, 2
    rest 1632
    note C4, 100, 2
    rest 87
    note C4, 100, 8
    rest 141
    jump label_0085
    .byte 0xff
track_02:
    note_wait 0
    program 3
    portamento_time 0
    volume 100
    pan 64
    expression 127
    bend_range 12
    priority 62
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_00e3:
    rest 1020
    note C4, 100, 119
    rest 1632
    note C4, 100, 44
    rest 420
    jump label_00e3
    .byte 0xff
track_03:
    note_wait 0
    program 4
    portamento_time 0
    pan 91
    volume 100
    expression 127
    bend_range 12
    priority 60
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_0112:
    rest 1500
    note C4, 70, 41
    rest 72
    note C4, 70, 41
    rest 117
    note C4, 70, 44
    rest 1383
    jump label_0112
    .byte 0xff
track_04:
    note_wait 0
    program 5
    portamento_time 0
    pan 24
    volume 100
    expression 127
    bend_range 12
    priority 58
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_0145:
    rest 2010
    note C4, 50, 92
    rest 207
    note C4, 50, 92
    rest 855
    jump label_0145
    .byte 0xff
track_09:
    note_wait 0
    program 93
    portamento_time 0
    volume 100
    pan 64
    expression 127
    bend_range 12
    priority 48
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_0174:
    note C4, 100, 191
    pan 69
    rest 24
    pan 70
    rest 24
    pan 71
    rest 24
    pan 72
    rest 24
    pan 73
    rest 48
    pan 74
    rest 24
    pan 75
    rest 24
    call label_01d5
    call label_01f6
    call label_01d5
    call label_01f6
    call label_01d5
    call label_01d5
    call label_01d5
    call label_01d5
    call label_01d5
    call label_01d5
    call label_01d5
    call label_01d5
    call label_01d5
    call label_01f6
    call label_01d5
    jump label_0174
    .byte 0xff
label_01d5:
    note C4, 100, 191
    pan 69
    rest 24
    pan 70
    rest 24
    pan 71
    rest 24
    pan 72
    rest 24
    pan 73
    rest 48
    pan 74
    rest 24
    pan 75
    rest 24
    return
label_01f6:
    pan 69
    note C4, 100, 191
    rest 24
    pan 70
    rest 24
    pan 71
    rest 24
    pan 72
    rest 24
    pan 73
    rest 48
    pan 74
    rest 24
    pan 75
    rest 24
    return
track_10:
    note_wait 0
    portamento_time 0
    volume 100
    expression 127
    program 93
    bend_range 12
    priority 46
    pan 53
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
    pitch_bend -3
label_0234:
    rest 3
    note C4, 100, 191
    pan 59
    rest 24
    pan 58
    rest 24
    pan 57
    rest 24
    pan 56
    rest 24
    pan 55
    rest 48
    pan 54
    rest 24
    pan 53
    rest 21
    call label_02d3
    call label_02d3
    call label_02d3
    call label_02d3
    call label_02d3
    rest 3
    pan 59
    note C4, 100, 191
    rest 24
    pan 58
    rest 24
    pan 57
    rest 24
    pan 56
    rest 24
    pan 55
    rest 48
    pan 54
    rest 24
    pan 53
    rest 21
    call label_02d3
    call label_02d3
    call label_02d3
    call label_02d3
    call label_02d3
    call label_02d3
    call label_02d3
    call label_02d3
    rest 3
    pan 59
    note C4, 100, 187
    rest 24
    pan 58
    rest 24
    pan 57
    rest 24
    pan 56
    rest 24
    pan 55
    rest 48
    pan 54
    rest 24
    pan 53
    rest 21
    jump label_0234
    .byte 0xff
label_02d3:
    rest 3
    note C4, 100, 191
    pan 59
    rest 24
    pan 58
    rest 24
    pan 57
    rest 24
    pan 56
    rest 24
    pan 55
    rest 48
    pan 54
    rest 24
    pan 53
    rest 21
    return
    .byte 0xff, 0x00
