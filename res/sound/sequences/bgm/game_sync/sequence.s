track_00:
    alloc_tracks 0x01ff
    open_track 1, track_01
    open_track 2, track_02
    open_track 3, track_03
    open_track 4, track_04
    open_track 5, track_05
    open_track 6, track_06
    open_track 7, track_07
    open_track 8, track_08
    note_wait 0
    tempo 90
    program 3
    volume 75
    pan 64
    expression 127
    bend_range 12
    priority 60
    mod_depth 15
    mod_speed 12
    mod_type 2
    mod_range 10
    mod_delay 30
label_0047:
    note As3, 108, 140
    rest 2
    note A4, 107, 140
    rest 1
    note C5, 109, 140
    rest 1
    note G5, 115, 140
    rest 140
    call label_00b8
    call label_00d2
    call label_00ec
    call label_0106
    call label_00b8
    call label_0120
    call label_00ec
    note As3, 108, 140
    rest 2
    note A4, 107, 140
    rest 1
    note C5, 109, 140
    rest 1
    note G5, 115, 140
    rest 140
    call label_00b8
    call label_00d2
    call label_00ec
    call label_0106
    call label_00b8
    call label_0120
    call label_00ec
    jump label_0047
    .byte 0x80, 0x01, 0xff
label_00b8:
    note G3, 104, 140
    rest 2
    note F4, 100, 140
    rest 1
    note As4, 98, 140
    rest 1
    note C5, 111, 140
    rest 140
    return
label_00d2:
    note Ds3, 109, 140
    rest 2
    note F4, 95, 140
    rest 1
    note As4, 89, 140
    rest 1
    note D5, 113, 140
    rest 140
    return
label_00ec:
    note As2, 100, 140
    rest 2
    note C4, 100, 140
    rest 1
    note F4, 95, 140
    rest 1
    note A4, 108, 140
    rest 140
    return
label_0106:
    note As3, 108, 140
    rest 2
    note A4, 101, 140
    rest 1
    note C5, 107, 140
    rest 1
    note G5, 115, 140
    rest 140
    return
label_0120:
    note Ds3, 104, 140
    rest 2
    note F4, 95, 140
    rest 1
    note As4, 89, 140
    rest 1
    note D5, 108, 140
    rest 140
    return
track_01:
    note_wait 0
    program 3
    volume 88
    pan 40
    expression 127
    bend_range 12
    priority 64
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_0153:
    rest 24
    note A5, 95, 5
    rest 6
    note A5, 64, 5
    rest 6
    note As5, 85, 5
    rest 6
    note As5, 54, 5
    rest 6
    note C6, 94, 5
    rest 6
    note C6, 63, 5
    rest 6
    note As5, 88, 5
    rest 6
    note As5, 57, 5
    rest 6
    note A5, 96, 5
    rest 6
    note A5, 65, 5
    rest 18
    note F5, 88, 5
    rest 6
    note F5, 57, 5
    rest 18
    note C5, 93, 5
    rest 6
    note C5, 62, 5
    rest 6
    note C5, 50, 5
    rest 6
    note C5, 32, 5
    rest 6
    call label_0234
    call label_024b
    call label_0234
    call label_02a6
    call label_0234
    call label_024b
    call label_0234
    rest 24
    note A5, 95, 5
    rest 6
    note A5, 64, 5
    rest 6
    note As5, 85, 5
    rest 6
    note As5, 54, 5
    rest 6
    note C6, 94, 5
    rest 6
    note C6, 63, 5
    rest 6
    note As5, 88, 5
    rest 6
    note As5, 57, 5
    rest 6
    note A5, 96, 5
    rest 6
    note A5, 65, 5
    rest 18
    note F5, 88, 5
    rest 6
    note F5, 57, 5
    rest 18
    note C5, 93, 5
    rest 6
    note C5, 62, 5
    rest 6
    note C5, 50, 5
    rest 6
    note C5, 32, 5
    rest 6
    call label_0234
    call label_024b
    call label_0234
    call label_02a6
    call label_0234
    call label_024b
    call label_0234
    jump label_0153
    .byte 0xff
label_0234:
    rest 96
    note A5, 42, 5
    rest 12
    note As5, 32, 5
    rest 12
    note C6, 41, 5
    rest 12
    note As5, 35, 5
    rest 12
    return
label_024b:
    note A5, 56, 5
    rest 6
    note A5, 15, 5
    rest 18
    note A5, 95, 5
    rest 6
    note A5, 54, 5
    rest 6
    note As5, 85, 5
    rest 6
    note As5, 44, 5
    rest 6
    note C6, 94, 5
    rest 6
    note C6, 53, 5
    rest 6
    note As5, 88, 5
    rest 6
    note As5, 47, 5
    rest 6
    note A5, 96, 5
    rest 6
    note A5, 55, 5
    rest 18
    note F5, 88, 5
    rest 6
    note F5, 47, 5
    rest 18
    note C5, 93, 5
    rest 6
    note C5, 52, 5
    rest 6
    note C5, 40, 5
    rest 6
    note C5, 22, 5
    rest 6
    return
label_02a6:
    rest 24
    note A5, 95, 5
    rest 6
    note A5, 54, 5
    rest 6
    note As5, 85, 5
    rest 6
    note As5, 44, 5
    rest 6
    note C6, 94, 5
    rest 6
    note C6, 53, 5
    rest 6
    note As5, 88, 5
    rest 6
    note As5, 47, 5
    rest 6
    note A5, 96, 5
    rest 6
    note A5, 55, 5
    rest 18
    note F5, 88, 5
    rest 6
    note F5, 47, 5
    rest 18
    note C5, 93, 5
    rest 6
    note C5, 52, 5
    rest 6
    note C5, 40, 5
    rest 6
    note C5, 22, 5
    rest 6
    return
track_02:
    note_wait 0
    program 3
    volume 60
    pan 100
    expression 127
    bend_range 12
    priority 62
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_0312:
    rest 96
    note A5, 95, 5
    rest 6
    note A5, 54, 5
    rest 6
    note As5, 85, 5
    rest 6
    note As5, 44, 5
    rest 6
    note C6, 94, 5
    rest 6
    note C6, 53, 5
    rest 6
    note As5, 88, 5
    rest 6
    note As5, 47, 5
    rest 6
    call label_037d
    call label_03a6
    call label_037d
    call label_03a6
    call label_037d
    call label_03a6
    call label_037d
    call label_03a6
    call label_037d
    call label_03a6
    call label_037d
    call label_03a6
    call label_037d
    call label_03a6
    call label_037d
    jump label_0312
    .byte 0xff
label_037d:
    note A5, 96, 5
    rest 6
    note A5, 55, 5
    rest 18
    note F5, 88, 5
    rest 6
    note F5, 47, 5
    rest 18
    note C5, 93, 5
    rest 6
    note C5, 52, 5
    rest 6
    note C5, 40, 5
    rest 6
    note C5, 22, 5
    rest 78
    return
label_03a6:
    rest 96
    note A5, 95, 5
    rest 6
    note A5, 54, 5
    rest 6
    note As5, 85, 5
    rest 6
    note As5, 44, 5
    rest 6
    note C6, 94, 5
    rest 6
    note C6, 53, 5
    rest 6
    note As5, 88, 5
    rest 6
    note As5, 47, 5
    rest 6
    return
track_03:
    note_wait 0
    program 18
    volume 100
    pan 64
    expression 127
    bend_range 12
    priority 60
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_03ea:
    pan 24
    note As1, 105, 5
    rest 12
    note As1, 97, 5
    rest 72
    pan 104
    note As1, 105, 5
    rest 12
    note As1, 97, 5
    rest 48
    call label_0443
    call label_045e
    call label_0443
    call label_045e
    call label_0443
    call label_045e
    call label_0443
    call label_045e
    call label_0443
    call label_045e
    call label_0443
    call label_045e
    call label_0443
    call label_045e
    call label_0443
    jump label_03ea
    .byte 0xff
label_0443:
    rest 24
    pan 24
    note As1, 105, 5
    rest 12
    note As1, 97, 5
    rest 72
    pan 104
    note As1, 105, 5
    rest 12
    note As1, 97, 5
    rest 24
    return
label_045e:
    pan 24
    note As1, 105, 5
    rest 12
    note As1, 97, 5
    rest 72
    pan 104
    note As1, 105, 5
    rest 12
    note As1, 97, 5
    rest 48
    return
track_04:
    note_wait 0
    program 0
    volume 90
    pan 64
    expression 127
    bend_range 12
    priority 58
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_0490:
    note G6, 116, 47
    rest 72
    note G6, 60, 47
    rest 72
    note C6, 111, 47
    rest 72
    note C6, 55, 47
    rest 72
    note D6, 121, 47
    rest 72
    note D6, 65, 47
    rest 72
    note A5, 114, 47
    rest 72
    note A5, 58, 47
    rest 72
    note G6, 116, 47
    rest 72
    note G6, 60, 47
    rest 72
    note C6, 111, 47
    rest 72
    note C6, 55, 47
    rest 72
    note D6, 121, 47
    rest 72
    note D6, 65, 47
    rest 72
    note A6, 114, 47
    rest 72
    note A6, 58, 47
    rest 72
    note D6, 91, 47
    note G6, 112, 47
    rest 72
    note D6, 35, 47
    note G6, 56, 47
    rest 72
    note G5, 84, 47
    note C6, 105, 47
    rest 72
    note G5, 28, 47
    note C6, 49, 47
    rest 72
    note A5, 94, 47
    note D6, 115, 47
    rest 72
    note A5, 38, 47
    note D6, 59, 47
    rest 72
    note C5, 87, 47
    note A5, 108, 47
    rest 72
    note C5, 31, 47
    note A5, 52, 47
    rest 72
    note D6, 89, 47
    note G6, 110, 47
    rest 72
    note D6, 33, 47
    note G6, 54, 47
    rest 72
    note G5, 84, 47
    note C6, 105, 47
    rest 72
    note G5, 28, 47
    note C6, 49, 47
    rest 72
    note A5, 94, 47
    note D6, 115, 47
    rest 72
    note A5, 38, 47
    note D6, 59, 47
    rest 72
    note C6, 87, 47
    note A6, 108, 47
    rest 72
    note C6, 31, 47
    note A6, 52, 47
    rest 72
    jump label_0490
    .byte 0xff
track_05:
    note_wait 0
    program 0
    volume 50
    pan 120
    expression 127
    bend_range 12
    priority 30
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_057e:
    rest 24
    note G6, 116, 59
    rest 84
    note G6, 60, 47
    rest 60
    note C6, 111, 59
    rest 84
    note C6, 55, 47
    rest 60
    note D6, 121, 59
    rest 84
    note D6, 65, 47
    rest 60
    note A5, 114, 59
    rest 84
    note A5, 58, 47
    rest 60
    note G6, 116, 59
    rest 84
    note G6, 60, 47
    rest 60
    note C6, 111, 59
    rest 84
    note C6, 55, 47
    rest 60
    note D6, 121, 59
    rest 84
    note D6, 65, 47
    rest 60
    note A6, 114, 59
    rest 84
    note A6, 58, 47
    rest 60
    note D6, 93, 59
    note G6, 109, 59
    rest 84
    note D6, 37, 47
    note G6, 53, 47
    rest 60
    note G5, 88, 59
    note C6, 104, 59
    rest 84
    note G5, 32, 47
    note C6, 48, 47
    rest 60
    note A5, 98, 59
    note D6, 114, 59
    rest 84
    note A5, 42, 47
    note D6, 58, 47
    rest 60
    note C5, 91, 59
    note A5, 107, 59
    rest 84
    note C5, 35, 47
    note A5, 51, 47
    rest 60
    note D6, 93, 59
    note G6, 109, 59
    rest 84
    note D6, 37, 47
    note G6, 53, 47
    rest 60
    note G5, 88, 59
    note C6, 104, 59
    rest 84
    note G5, 32, 47
    note C6, 48, 47
    rest 60
    note A5, 98, 59
    note D6, 114, 59
    rest 84
    note A5, 42, 47
    note D6, 58, 47
    rest 60
    note C6, 91, 59
    note A6, 107, 59
    rest 84
    note C6, 35, 47
    note A6, 51, 47
    rest 36
    jump label_057e
    .byte 0x80, 0x0b, 0xff
track_06:
    note_wait 0
    program 0
    volume 50
    pan 10
    expression 127
    bend_range 12
    priority 20
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_0670:
    rest 48
    note G6, 116, 59
    rest 84
    note G6, 60, 47
    rest 60
    note C6, 111, 59
    rest 84
    note C6, 55, 47
    rest 60
    note D6, 121, 59
    rest 84
    note D6, 65, 47
    rest 60
    note A5, 114, 59
    rest 84
    note A5, 58, 47
    rest 60
    note G6, 116, 59
    rest 84
    note G6, 60, 47
    rest 60
    note C6, 111, 59
    rest 84
    note C6, 55, 47
    rest 60
    note D6, 121, 59
    rest 84
    note D6, 65, 47
    rest 60
    note A6, 114, 59
    rest 84
    note A6, 58, 47
    rest 60
    note G6, 116, 59
    rest 84
    note G6, 60, 47
    rest 60
    note C6, 111, 59
    rest 84
    note C6, 55, 47
    rest 60
    note D6, 121, 59
    rest 84
    note D6, 65, 47
    rest 60
    note A5, 114, 59
    rest 84
    note A5, 58, 47
    rest 60
    note G6, 116, 59
    rest 84
    note G6, 60, 47
    rest 60
    note C6, 111, 59
    rest 84
    note C6, 55, 47
    rest 60
    note D6, 121, 59
    rest 84
    note D6, 65, 47
    rest 60
    note A6, 114, 59
    rest 84
    note A6, 58, 47
    rest 12
    jump label_0670
    .byte 0x80, 0x23, 0xff
track_07:
    note_wait 0
    program 3
    volume 40
    pan 30
    expression 127
    bend_range 12
    priority 10
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_0732:
    rest 144
    call label_076f
    rest 144
    call label_076f
    rest 144
    call label_076f
    rest 144
    call label_076f
    rest 144
    call label_076f
    rest 144
    call label_076f
    rest 144
    call label_076f
    rest 144
    call label_076f
    jump label_0732
    .byte 0xff
label_076f:
    rest 24
    note A5, 95, 5
    rest 6
    note A5, 54, 5
    rest 6
    note As5, 85, 5
    rest 6
    note As5, 44, 5
    rest 6
    note C6, 94, 5
    rest 6
    note C6, 53, 5
    rest 6
    note As5, 88, 5
    rest 6
    note As5, 47, 5
    rest 6
    note A5, 96, 5
    rest 6
    note A5, 55, 5
    rest 18
    note F5, 88, 5
    rest 6
    note F5, 47, 5
    rest 18
    note C5, 93, 5
    rest 6
    note C5, 52, 5
    rest 6
    note C5, 40, 5
    rest 6
    note C5, 22, 5
    rest 6
    return
track_08:
    note_wait 0
    program 81
    volume 100
    pan 64
    expression 127
    bend_range 12
    priority 50
    mod_depth 0
    mod_speed 16
    mod_type 0
    mod_range 1
    mod_delay 0
label_07db:
    note B1, 114, 10
    rest 12
    note B1, 105, 10
    rest 60
    note Fs2, 79, 10
    rest 12
    note Fs2, 68, 10
    rest 12
    note Fs2, 57, 10
    rest 12
    note B1, 95, 10
    note Fs2, 49, 10
    rest 12
    note B1, 90, 10
    note Fs2, 44, 10
    rest 12
    note Fs2, 30, 10
    rest 12
    call label_084a
    call label_0864
    call label_084a
    call label_0893
    call label_084a
    call label_0864
    call label_084a
    call label_0893
    call label_084a
    call label_0864
    call label_084a
    call label_0893
    call label_084a
    call label_0864
    call label_084a
    jump label_07db
    .byte 0xff
label_084a:
    note Fs2, 20, 10
    rest 12
    note Fs2, 9, 10
    rest 12
    note Fs2, 4, 10
    rest 84
    note B1, 107, 10
    rest 12
    note B1, 101, 10
    rest 24
    return
label_0864:
    note B1, 109, 10
    rest 12
    note B1, 99, 10
    rest 60
    note Fs2, 79, 10
    rest 12
    note Fs2, 68, 10
    rest 12
    note Fs2, 57, 10
    rest 12
    note B1, 95, 10
    note Fs2, 46, 10
    rest 12
    note B1, 90, 10
    note Fs2, 33, 10
    rest 12
    note Fs2, 22, 10
    rest 12
    return
label_0893:
    note B1, 114, 10
    rest 12
    note B1, 105, 10
    rest 60
    note Fs2, 79, 10
    rest 12
    note Fs2, 68, 10
    rest 12
    note Fs2, 57, 10
    rest 12
    note B1, 95, 10
    note Fs2, 49, 10
    rest 12
    note B1, 90, 10
    note Fs2, 44, 10
    rest 12
    note Fs2, 30, 10
    rest 12
    return
    .byte 0xff, 0x00
