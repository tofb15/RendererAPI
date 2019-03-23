#set terminal wxt 
set yrange[0:3]
set ytics ("GPU Copy" 1, "GPU Direct" 1.0001, "GPU Compute" 1.0002, "CPU Child Record Copy" 1.0003, "CPU Child Record Direct 0" 1.0004, "CPU Main" 1.0005 )
#set grid

plot \
"TimeStamps0.tsv" u ($2):(1) w lines t "GPU Copy" ls 1 lw 8,\
"TimeStamps1.tsv" u ($2):(1.0001) w lines t "GPU Direct" ls 2 lw 8,\
"TimeStamps2.tsv" u ($2):(1.0002) w lines t "GPU Compute" ls 3 lw 8,\
"TimeStampsCPU0.tsv" u ($2):(1.0003) w lines t "CPU Child Record Copy" ls 7 lw 2,\
"TimeStampsCPU1.tsv" u ($2):(1.0004) w lines t "CPU Child Record Direct 0" ls 9 lw 2,\
"TimeStampsCPU2.tsv" u ($2):(1.0005) w lines t "CPU Main Record Direct" ls 4 lw 2,\
"TimeStampsCPU3.tsv" u ($2):(1.0005) w lines t "CPU Main Present" ls 8 lw 2,\
"TimeStampsCPU4.tsv" u ($2):(1.0005) w lines t "CPU Main Present Wait" ls 2 lw 2,\
"TimeStampsCPU5.tsv" u ($2):(1.0005) w lines t "CPU Main Record Compute" ls 3 lw 2,\
"TimeStampsCPU6.tsv" u ($2):(1.0005) w lines t "CPU Main Update" ls 7 lw 2