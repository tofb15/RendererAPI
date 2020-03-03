reset
file = "data.MergedData"
file2 = "StatDat.stats"

set title "" font ",16"

set xlabel "Frame Number"
set ylabel "Frame Draw Time (ms)"
set format x "%.f frame"
set format y "%.3f ms"

set grid ytics mytics  # draw lines for each ytics and mytics
set mytics 0.1         # set the spacing for the mytics
set grid

set key horizontal noinvert left
set yrange [0:10]