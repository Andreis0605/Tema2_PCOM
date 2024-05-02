all: server subscriber

server: server.cpp my_io.cpp
	g++ -g -Wall -o server server.cpp my_io.cpp

subscriber: subscriber.cpp my_io.cpp
	g++ -g -Wall -o subscriber subscriber.cpp my_io.cpp -lm

clean:
	rm -rf subscriber server