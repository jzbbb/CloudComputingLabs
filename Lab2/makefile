src = main.cpp locker.cpp http.cpp

target:http-server

http-server:$(src)
	g++ -O2 -o $@ $^ -pthread

clean:
	-rm -f http-server