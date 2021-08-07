all:
	g++ -pthread ospf.cpp -o ospf
clean:
	rm -rf ospf
