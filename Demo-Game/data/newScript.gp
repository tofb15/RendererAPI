#Parameters

#set terminal wxt size 1200, 600
#set multiplot layout 1,3 title "Member dice rolls" font ",14"

set xlabel "Test Case"
set ylabel "Dispatch Time ms"
set title "Titel" font ",16"
set xtics ("Test 1" 0, "Test 2" 0.1, "Test 3" 0.2)
set xrange [-0.1:0.3]
set yrange [:3]

file = "data.MergedData"

set boxwidth 0.05
set style data histogram
set style histogram cluster
#set style data boxplot
#set style boxplot fration 0.95
#set errorbars 0 

plot \
file using (0.0):1:xticlabels(1) with boxplot t columnheader ls 1, \
file using (0.1):2:xticlabels(1) with boxplot t columnheader ls 2, \
file using (0.2):3:xticlabels(1) with boxplot t columnheader ls 3, \

#unset title
#set title "Human - Dice rolls" font ",16"
#
#plot \
#file using (0):1:xticlabels(1) with boxplot t "Strength" ls 1, \
#file using (1):2:xticlabels(1) with boxplot t "Dexterity" ls 2, \
#file using (2):3:xticlabels(1) with boxplot t "Endurance" ls 3, \
#
#unset title
#set title "Firbolg - Dice rolls" font ",16"
#
#plot \
#file using (0):1:xticlabels(1) with boxplot t "Strength" ls 1, \
#file using (1):2:xticlabels(1) with boxplot t "Dexterity" ls 2, \
#file using (2):3:xticlabels(1) with boxplot t "Endurance" ls 3, \