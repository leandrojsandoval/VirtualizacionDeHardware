CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11
TARGETS = daemon sensor

all: $(TARGETS)

daemon: daemon.cpp
	$(CXX) $(CXXFLAGS) -o daemon daemon.cpp

sensor: sensor.cpp
	$(CXX) $(CXXFLAGS) -o sensor sensor.cpp

clean:
	rm -f $(TARGETS)
	rm -f /tmp/sensor_fifo
