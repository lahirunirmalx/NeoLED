# Host-side unit tests for NeoLED color utilities (no ESP-IDF required).
CXX ?= c++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -I../include

.PHONY: test clean
test: test_color
	./test_color

test_color: test_color.cpp ../include/neoled.h
	$(CXX) $(CXXFLAGS) test_color.cpp -o test_color

clean:
	rm -f test_color
