###!!!                     Autogenerated file                       !!!
###!!!Any changes made to this file may be overwritten automatically!!!

reset
set xlabel "Shader Name"
set ylabel "Dispatch Time (ms)"
set boxwidth 0.05
set style data histogram
set style histogram cluster
set style boxplot nooutliers
set xtics (" Minimal-NS" 0.000000," DICE-NS" 0.100000," RayGen-NS" 0.200000," Close1-NS" 0.300000," Close2-NS" 0.400000," RayGen-Debug" 0.500000)
set xrange [-0.1:0.600000]
set yrange [:4]
set title "Dispatch Times for scene: Scene4" font ", 16"
plot \
"../../Data/Scene4#0.data" using (0.000000):1:xticlabels(1) w boxplot t "Minimal-NS",\
"../../Data/Scene4#1.data" using (0.100000):1:xticlabels(1) w boxplot t "DICE-NS",\
"../../Data/Scene4#2.data" using (0.200000):1:xticlabels(1) w boxplot t "RayGen-NS",\
"../../Data/Scene4#3.data" using (0.300000):1:xticlabels(1) w boxplot t "Close1-NS",\
"../../Data/Scene4#4.data" using (0.400000):1:xticlabels(1) w boxplot t "Close2-NS",\
"../../Data/Scene4#5.data" using (0.500000):1:xticlabels(1) w boxplot t "RayGen-Debug",\
