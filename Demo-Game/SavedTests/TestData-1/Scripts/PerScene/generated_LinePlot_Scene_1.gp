###!!!                     Autogenerated file                       !!!
###!!!Any changes made to this file may be overwritten automatically!!!

reset
set title "" font ", 16"
set xlabel "Frame Number"
set ylabel "Frame Draw Time (ms)"
set format x "%.f frame"
set format y "%.3f ms"
set grid ytics mytics  # draw lines for each ytics and mytics
set mytics 0.1         # set the spacing for the mytics
set grid
set key horizontal noinvert left
set yrange [0:10]
set yrange [0:10]
plot \
"../../Data/Scene1#0.data" using 1 w lines lc 0t "shader-DICE-SIMPLE",\
"../../Data/Scene1#1.data" using 1 w lines lc 1t "shader-DICE-SIMPLE",\
"../../Data/Scene1#2.data" using 1 w lines lc 2t "shader-DICE-SIMPLE",\
"../../Data/Scene1#3.data" using 1 w lines lc 3t "shader-DICE-SIMPLE",\
"../../Data/Scene1#4.data" using 1 w lines lc 4t "shader-DICE-SIMPLE",\
"../../Data/Scene1#5.data" using 1 w lines lc 5t "shader-DICE-SIMPLE",\
"../../Data/Scene1#6.data" using 1 w lines lc 6t "shader-DICE-SIMPLE",\
"../../Data/Scene1#7.data" using 1 w lines lc 7t "shader-DICE-SIMPLE",\
"../../Data/Scene1#8.data" using 1 w lines lc 8t "shader-DICE-SIMPLE",\
"../../Data/Scene1#9.data" using 1 w lines lc 9t "shader-DICE-SIMPLE",\
"../../Data/Scene1#10.data" using 1 w lines lc 10t "shader-DICE-SIMPLE",\
"../../Data/Scene1#11.data" using 1 w lines lc 11t "shader-DICE-SIMPLE",\
"../../Data/Scene1#12.data" using 1 w lines lc 12t "shader-DICE-SIMPLE",\
"../../Data/Scene1#13.data" using 1 w lines lc 13t "shader-DICE-SIMPLE",\
"../../Data/Scene1#14.data" using 1 w lines lc 14t "shader-DICE-SIMPLE",\
"../../Data/Scene1#15.data" using 1 w lines lc 15t "shader-DICE-SIMPLE",\
