set print "StatDat.stats" 
do for [i=1:10] { # Here you will use i for the column.
  stats  'data.MergedData' u i nooutput; 
  #print i, STATS_median, STATS_mean , STATS_stddev # ...
  print STATS_mean
 } 
set print
#plot "StatDat.stats" us 1 # or whatever column you want...