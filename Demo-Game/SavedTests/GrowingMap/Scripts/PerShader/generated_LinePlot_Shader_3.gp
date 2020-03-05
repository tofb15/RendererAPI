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
set title "Dispatch Times for shader: Close1-NS" font ", 16"
plot \
"../../Data/Scene0#3.data" using 1 w lines lc 0 t "Scene0",\
"../../Data/Scene1#3.data" using 1 w lines lc 1 t "Scene1",\
"../../Data/Scene2#3.data" using 1 w lines lc 2 t "Scene2",\
"../../Data/Scene3#3.data" using 1 w lines lc 3 t "Scene3",\
"../../Data/Scene4#3.data" using 1 w lines lc 4 t "Scene4",\
"../../Data/Scene5#3.data" using 1 w lines lc 5 t "Scene5",\
