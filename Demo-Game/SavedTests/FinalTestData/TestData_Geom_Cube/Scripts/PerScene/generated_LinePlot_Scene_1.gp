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
set title "Dispatch Times for scene: Cube_1" font ", 16"
plot \
"../../Data/Cube_1#0.data" using 1 w lines lc 0 t "shader-RGBA",\
