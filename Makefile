all: server

server: helper.o final.o
	g++ helper.o final.o -o server

final.o: final.cpp
	g++ -std=c++11 -c final.cpp

helper.o : helper.cpp
	g++ -c helper.cpp

clean:
	rm -rf *.o server
