###!!!                     Autogenerated file                       !!!
###!!!Any changes made to this file may be overwritten automatically!!!

reset
set xlabel "Scene Name"
set ylabel "Dispatch Time (ms)"
set boxwidth 0.05
set style data histogram
set style histogram cluster
set style boxplot nooutliers
set xtics ("Forest BC7" 0.000000,"Forest RGBA" 0.100000,"Scene0 BC7" 0.200000,"Scene0 RGBA" 0.300000)
set xrange [-0.1:0.800000]
set yrange [0:]
set title "Dispatch Times for shader: RGBA" font ", 16"
plot \
"../../Data/Forest_BC7#0.data" using (0.000000):1:xticlabels(1) w boxplot t "Forest BC7",\
"../../Data/Forest_RGBA#0.data" using (0.100000):1:xticlabels(1) w boxplot t "Forest RGBA",\
"../../Data/Scene0_BC7#0.data" using (0.200000):1:xticlabels(1) w boxplot t "Scene0 BC7",\
"../../Data/Scene0_RGBA#0.data" using (0.300000):1:xticlabels(1) w boxplot t "Scene0 RGBA",\
