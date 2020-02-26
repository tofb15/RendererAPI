reset
    file = "data.MergedData"
    file2 = "StatDat.stats"

    set title "" font ",16"

    set xlabel "Frame Number"
    set ylabel "Frame Draw Time (ms)"
    set format x "%.000f frame"
    set format y "%.000f ms"

    set grid

    set key horizontal noinvert left

    set yrange [0:10]
show variables
    #set label "Tab 1" at 7,560
    #set label "Tab 2" at 17,760
    #set label "Tab 3" at 27,960
    #set label "Tab 4" at 37,1100
    #set label "Tab 5" at 47,1280
    #//set arrow 1 from 10,500 to 10,300 head filled lw 1 lc rgb "#000000"
    #//set arrow 2 from 20,700 to 20,500 head filled lw 1 lc rgb "#000000"
    #//set arrow 3 from 30,900 to 30,700 head filled lw 1 lc rgb "#000000"
    #//set arrow 4 from 40,1040 to 40,840 head filled lw 1 lc rgb "#000000"
    #//set arrow 5 from 50,1240 to 50,1040 head filled lw 1 lc rgb "#000000"

	#stats file 
	
	##f1(x) = a+b*x
	##fit f1(x) file using 2:($1) via a,b
	
    plot \
	for[i=0:5] file using i w lines dt 1 t columnheader,\
	for[j=5:10] file using j w lines dt 2 t columnheader,\
	for[i=10:15] file using i w lines dt 3 t columnheader,\
	for[i=15:20] file using i w lines dt 4 t columnheader,
	#f1(x)
	#file2 us 1