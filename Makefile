CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -g

TARGETS = sender receiver

all: $(TARGETS)

sender: sender.cpp packet.hpp logger.hpp timer.hpp
	$(CXX) $(CXXFLAGS) sender.cpp -o sender.o

receiver: receiver.cpp packet.hpp logger.hpp timer.hpp
	$(CXX) $(CXXFLAGS) receiver.cpp -o receiver.o

clean:
	rm -f $(TARGETS) *.o output.txt

.PHONY: all clean