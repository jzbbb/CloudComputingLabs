CFLAGS = -o1 -pthread -lpthread

SRCS = main.cpp kvstore2pcsystem.h kvstore2pcsystem.cpp coordinator.h coordinator.cpp participant.h participant.cpp

kvstore2pcsystem: $(SRCS)
	  g++ $(CFLAGS) $(SRCS) -o kvstore2pcsystem

clean:
	  -rm -f kvstore2pcsystem