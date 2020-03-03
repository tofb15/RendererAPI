###!!!                     Autogenerated file                       !!!
###!!!Any changes made to this file may be overwritten automatically!!!

reset
file="../data.MergedData"
set title "" font ", 16"
set xlabel "Frame Number"
set ylabel "Frame Draw Time (ms)"
set format x "%.f frame"
set format y "%.3f ms"
set grid ytics mytics  # draw lines for each ytics and mytics
set mytics 0.1         # set the spacing for the mytics
set grid
set key horizontal noinvert left
plot \
file using 1         w lines lc 0t "shader-0",\
file using 5         w lines lc 0t "shader-0",\
file using 9         w lines lc 0t "shader-0",\
file using 13        w lines lc 0t "shader-0",\