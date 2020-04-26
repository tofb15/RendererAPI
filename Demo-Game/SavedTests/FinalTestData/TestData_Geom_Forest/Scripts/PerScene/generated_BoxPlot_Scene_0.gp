###!!!                     Autogenerated file                       !!!
###!!!Any changes made to this file may be overwritten automatically!!!

reset
set xlabel "Shader Name"
set ylabel "Dispatch Time (ms)"
set boxwidth 0.05
set style data histogram
set style histogram cluster
set style boxplot nooutliers
set xtics (" RGBA" 0.000000)
set xrange [-0.1:0.100000]
set yrange [0:]
set title "Dispatch Times for scene: Forest_0" font ", 16"
plot \
"../../Data/Forest_0#0.data" using (0.000000):1:xticlabels(1) w boxplot t "RGBA",\
