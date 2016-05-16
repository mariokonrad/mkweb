.PHONY: all clean webclean clean-all

CXX=g++-5
CXXFLAGS=-Wall -Wextra -pedantic -O0 -ggdb -std=c++11 -static

STRIP=strip
STRIPFLAGS=-s

all :
	@echo "error: specify a specific target"

filter : src/filter.cpp
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $^
	$(STRIP) $(STRIPFLAGS) $@

meta : src/meta.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ -Isrc -I`pwd`/local/include -L`pwd`/local/lib -lyaml-cpp
	$(STRIP) $(STRIPFLAGS) $@

config : src/config.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ -Isrc -I`pwd`/local/include -L`pwd`/local/lib -lyaml-cpp

yaml :
	mkdir local
	mkdir build
	(cd build ; cmake ~/local/repo/yaml-cpp -DCMAKE_INSTALL_PREFIX=`pwd`/../local)
	(cd build ; make -j 8)
	(cd build ; make install)
	rm -fr build

clean :
	rm -fr build
	rm -f *.o
	rm -f filter
	rm -f meta
	rm -f config

clean-all : clean
	rm -fr local

