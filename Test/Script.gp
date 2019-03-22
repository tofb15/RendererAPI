#set terminal wxt 
set yrange[0:3]
set ytics ("GPU Copy" 1, "GPU Direct" 1.0001, "GPU Compute" 1.0002, "CPU" 1.0003)
#set grid

plot \
"TimeStamps0.tsv" u ($2):(1) w lines t "GPU Copy" ls 1 lw 2,\
"TimeStamps1.tsv" u ($2):(1.0001) w lines t "GPU Direct" ls 2 lw 2,\
"TimeStamps2.tsv" u ($2):(1.0002) w lines t "GPU Compute" ls 3 lw 10,\
"TimeStampsCPU0.tsv" u ($2):(1.0003) w lines t "CPU Record" ls 4 lw 2,\
"TimeStampsCPU1.tsv" u ($2):(1.0003) w lines t "CPU Present" ls 8 lw 2,\
"TimeStampsCPU2.tsv" u ($2):(1.0003) w lines t "CPU Update" ls 7 lw 2