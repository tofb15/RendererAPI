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
set title "Dispatch Times for shader: RayGen-NS" font ", 16"
plot \
"../../Data/Scene0#2.data" using 1 w lines lc 0 t "Scene0",\
"../../Data/Scene1#2.data" using 1 w lines lc 1 t "Scene1",\
"../../Data/Scene2#2.data" using 1 w lines lc 2 t "Scene2",\
