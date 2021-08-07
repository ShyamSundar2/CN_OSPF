make
timeout 45 ./ospf 0 $1 outfile-0.txt 1 5 20 &
timeout 45 ./ospf 1 $1 outfile-1.txt 1 5 20 &
timeout 45 ./ospf 2 $1 outfile-2.txt 1 5 20 &
timeout 45 ./ospf 3 $1 outfile-3.txt 1 5 20 &
timeout 45 ./ospf 4 $1 outfile-4.txt 1 5 20 &
timeout 45 ./ospf 5 $1 outfile-5.txt 1 5 20 &
timeout 45 ./ospf 6 $1 outfile-6.txt 1 5 20 &
timeout 45 ./ospf 7 $1 outfile-7.txt 1 5 20

sleep 5

make clean
