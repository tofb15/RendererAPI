###!!!                     Autogenerated file                       !!!
###!!!Any changes made to this file may be overwritten automatically!!!

reset
set xlabel "Test Case"
set ylabel "Frame Draw Time (ms)"
set boxwidth 0.05
set style data histogram
set style histogram cluster
set style boxplot nooutliers
set xtics ("Scene 0" 0.000000,"Scene 1" 0.100000,"Scene 2" 0.200000,"Scene 3" 0.300000)
file="../../Data/data.MergedData"
set xrange [-0.1:0.400000]
set yrange [:4]
set title "Shader Config 0" font ", 16"
set title "Shader Config 1" font ", 16"
set title "Shader Config 2" font ", 16"
set title "Shader Config 3" font ", 16"
set title "Shader Config 4" font ", 16"
set title "Shader Config 5" font ", 16"
plot \
file using (0.000000):6:xticlabels(1) w boxplot t "Scene-0",\
file using (0.100000):22:xticlabels(1) w boxplot t "Scene-1",\
file using (0.200000):38:xticlabels(1) w boxplot t "Scene-2",\
file using (0.300000):54:xticlabels(1) w boxplot t "Scene-3",\
