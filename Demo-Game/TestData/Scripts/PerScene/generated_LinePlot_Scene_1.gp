###!!!                     Autogenerated file                       !!!
###!!!Any changes made to this file may be overwritten automatically!!!

reset
set xlabel "Frame Number"
set ylabel "Dispatch Time (ms)"
set format x "%.f frame"
set format y "%.3f ms"
set grid ytics mytics  # draw lines for each ytics and mytics
set mytics 0.1         # set the spacing for the mytics
set grid
set key horizontal noinvert left
set yrange [0:10]
set yrange [0:]
set title "Dispatch Times for scene: Scene1" font ", 16"
plot \
"../../Data/Scene1#0.data" using 1 w lines lc 0 t "shader-Minimal-NS",\
"../../Data/Scene1#1.data" using 1 w lines lc 1 t "shader-DICE-NS",\
"../../Data/Scene1#2.data" using 1 w lines lc 2 t "shader-RayGen-NS",\
"../../Data/Scene1#3.data" using 1 w lines lc 3 t "shader-Close1-NS",\
"../../Data/Scene1#4.data" using 1 w lines lc 4 t "shader-Close2-NS",\
"../../Data/Scene1#5.data" using 1 w lines lc 5 t "shader-RayGen-Debug",\
