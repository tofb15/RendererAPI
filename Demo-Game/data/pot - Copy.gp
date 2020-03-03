reset
file = "data.MergedData"
file2 = "StatDat.stats"

set title "" font ",16"

set xlabel "Frame Number"
set ylabel "Frame Draw Time (ms)"
set format x "%.f frame"
set format y "%.3f ms"

set grid ytics mytics  # draw lines for each ytics and mytics
set mytics 0.1           # set the spacing for the mytics
set grid

set key horizontal noinvert left

set yrange [0:10]
show variables

#stats file 

##f1(x) = a+b*x
##fit f1(x) file using 2:($1) via a,b

plot \
for[i=1:4] file using i+12 w lines  lc i t columnheader,\
for[i=1:4] file using i+4 w lines   lc 1 t columnheader,\
for[i=1:4] file using i+8 w lines   lc 2 t columnheader,\
for[i=1:4] file using i+12 w lines  lc 3 t columnheader,
#f1(x)
#file2 us 1