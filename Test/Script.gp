#set terminal wxt 
set yrange[0:3]
set ytics ("CPU Thread 1" 1, "CPU Thread 2" 1.01, "GPU Direct" 1.02, "GPU Copy" 1.03)
set grid

plot \
"TimeStamps0.tsv" using ($2 * 1e-3):(1) with lines
#"TimeStampsRenderer.tsv" using 2:(1) with lines title "Renderer CPU" ls 7,\
#"TimeStampsSource.tsv" using 2:(1) with lines title "Update CPU" ls 1,\
#"TimeStampsTextureLoader.tsv" using 2:(1.01) with lines title "TextureLoader CPU" ls 2,\
#"TimeStampsRenderer.tsv" using 3:(1.02) with lines title "Renderer GPU" ls 3,\
#"TimeStampsTextureLoader.tsv" using 3:(1.03) with lines title "TextureLoader GPU" ls 4