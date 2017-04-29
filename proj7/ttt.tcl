#!/usr/bin/env wish

set width 300
set height 300

# game canvas
canvas .c -width $width -height $height -bg white
pack .c

# grid lines
.c create line   0 100 300 100
.c create line   0 200 300 200
.c create line 100   0 100 300
.c create line 200   0 200 300

# click events
bind .c <Button-1> { puts [px2pos %x %y] }

proc px2pos {x y} {
    global width
    global height

    if {$x < [ expr $width / 3.0 ]} {
        set col 0
    }  elseif {$x < [ expr (2 * $width) / 3.0 ]} {
        set col 1
    } else {
        set col 2
    }

    if {$y < [ expr $height / 3.0 ]} {
        set row 0
    }  elseif {$y < [ expr (2 * $height) / 3.0 ]} {
        set row 1
    } else {
        set row 2
    }

    return [ expr $col + (3 * $row) + 1 ]
}

# rerender the board
proc updateBoard {board} {
    global width
    global height
    set pos 0
    foreach p [ split $board {} ] {
        incr pos
        if {$pos != " "} {
            if {$pos < 4} {
                set y 0
            } elseif {$pos < 7} {
                set y 1
            } else {
                set y 2
            }
            if {$pos == 1 || $pos == 4 || $pos == 7} {
                set x 0
            } elseif {$pos == 2 || $pos == 5 || $pos == 8} {
                set x 1
            } else {
                set x 2
            }
            if {$p == "o"} {
                .c create oval \
                    [expr $x * ($width / 3)] \
                    [expr $y * ($height / 3)] \
                    [expr ($x + 1) * ($width / 3)] \
                    [expr ($y + 1) * ($height / 3)] -tags xo
            } elseif {$p == "x"} {
                .c create line \
                    [expr $x * ($width / 3)] \
                    [expr $y * ($height / 3)] \
                    [expr ($x + 1) * ($width / 3)] \
                    [expr ($y + 1) * ($height / 3)] -tags xo
                .c create line \
                    [expr ($x + 1) * ($width / 3)] \
                    [expr $y * ($height / 3)] \
                    [expr $x * ($width / 3)] \
                    [expr ($y + 1) * ($height / 3)] -tags xo
            }
        }
    }
}

proc setHandle {name symbol} {
    .cs create text 10 30 -anchor nw -text "You: $name ($symbol)"
}

proc setOpponentHandle {name symbol} {
    .cs create text 10 50 -anchor nw -text "Opponent: $name ($symbol)"
}

proc setStatus {status} {
    global width
    .cs create text $width 30 -anchor ne -text $status -tags status
}

# status canvas
canvas .cs -width 300 -height 100 -bg gray
pack .cs

# sound button
button .soundBtn -text "Sound"
pack .soundBtn

# redesign button
button .redesignBtn -text "Redesign"
pack .redesignBtn

# exit button
button .exitBtn -text "Exit"
pack .exitBtn

