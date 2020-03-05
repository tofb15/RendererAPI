###!!!                     Autogenerated file                       !!!
###!!!Any changes made to this file may be overwritten automatically!!!

reset
set xlabel "Scene Name"
set ylabel "Dispatch Time (ms)"
set boxwidth 0.05
set style data histogram
set style histogram cluster
set style boxplot nooutliers
set xtics ("Scene 0" 0.000000,"Scene 1" 0.100000,"Scene 2" 0.200000)
set xrange [-0.1:0.300000]
set yrange [0:]
set title "Dispatch Times for shader: Close1-NS" font ", 16"
plot \
"../../Data/Scene0#3.data" using (0.000000):1:xticlabels(1) w boxplot t "Scene0",\
"../../Data/Scene1#3.data" using (0.100000):1:xticlabels(1) w boxplot t "Scene1",\
"../../Data/Scene2#3.data" using (0.200000):1:xticlabels(1) w boxplot t "Scene2",\
