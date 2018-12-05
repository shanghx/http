bin=HttpdServer
cc=g++
LDFLAGS=-lpthread


PHONY:all
all:$(bin) 

$(bin):HttpdServer.cc
	$(cc) -o $@ $^ $(LDFLAGS) -std=c++11
#.PHONY:cgi
#cgi:
#	g++ -o cal cal.cpp
.PHONY:clean
clean:
	rm -f $(bin)
