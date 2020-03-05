###!!!                     Autogenerated file                       !!!
###!!!Any changes made to this file may be overwritten automatically!!!

reset
set xlabel "Scene Name"
set ylabel "Dispatch Time (ms)"
set boxwidth 0.05
set style data histogram
set style histogram cluster
set style boxplot nooutliers
set xtics ("Scene 0" 0.000000,"Scene 1" 0.100000,"Scene 2" 0.200000,"Scene 3" 0.300000,"Scene 4" 0.400000,"Scene 5" 0.500000)
set xrange [-0.1:0.600000]
set yrange [:6]
set title "Dispatch Times for shader: DICE-NS" font ", 16"
plot \
"../../Data/Scene0#1.data" using (0.000000):1:xticlabels(1) w boxplot t "Scene0",\
"../../Data/Scene1#1.data" using (0.100000):1:xticlabels(1) w boxplot t "Scene1",\
"../../Data/Scene2#1.data" using (0.200000):1:xticlabels(1) w boxplot t "Scene2",\
"../../Data/Scene3#1.data" using (0.300000):1:xticlabels(1) w boxplot t "Scene3",\
"../../Data/Scene4#1.data" using (0.400000):1:xticlabels(1) w boxplot t "Scene4",\
"../../Data/Scene5#1.data" using (0.500000):1:xticlabels(1) w boxplot t "Scene5",\
