CFLAGS = -lpthread -std=c++14 -g
SOURCES = src/svg.hpp tests/catch.hpp tests/svg_tests.cpp

all: test

test:
	$(CXX) -o test $(SOURCES) $(CFLAGS) -Isrc/
	./test

clean:
	rm -rf test

.PHONY: all