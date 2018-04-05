CFLAGS = -lpthread -std=c++14 -g --coverage
SOURCES = src/svg.hpp tests/catch.hpp tests/svg_tests.cpp

all: test_all

test_all:
	make test
	make code_cov

test:
	$(CXX) -o test $(SOURCES) $(CFLAGS) -Isrc/
	./test
	
code_cov: test
	mkdir -p test_results
	mv *.gcno *.gcda $(PWD)/test_results
	gcov $(SOURCES) -o test_results --relative-only
	mv *.gcov test_results

clean:
	rm -rf test

.PHONY: all