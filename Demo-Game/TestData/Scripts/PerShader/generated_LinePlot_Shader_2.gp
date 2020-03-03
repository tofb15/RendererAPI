###!!!                     Autogenerated file                       !!!
###!!!Any changes made to this file may be overwritten automatically!!!

reset
file="../../Data/data.MergedData"
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
file using 3         w lines lc 2t "shader-2",\
file using 19        w lines lc 2t "shader-2",\
file using 35        w lines lc 2t "shader-2",\
file using 51        w lines lc 2t "shader-2",\
