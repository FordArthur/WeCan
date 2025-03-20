set datafile separator ","
set key autotitle columnheader
while (1) {
  plot "x" using 1:2 with lines title "Temperatura", \
    "x" using 1:3 with lines title "Presión", \
    "x" using 1:4 with lines title "Altitud"
  pause 1
}
