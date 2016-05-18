.PHONY: default clean clean-all mkweb

CXX=g++-5
CXXFLAGS=-Wall -Wextra -pedantic -O3 -ggdb -std=c++1z -static

STRIP=strip
STRIPFLAGS=-s

default : mkweb

filter : src/filter.cpp
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $^
	$(STRIP) $(STRIPFLAGS) $@

meta : src/meta.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ -Isrc -I`pwd`/local/include -L`pwd`/local/lib -lyaml-cpp
	$(STRIP) $(STRIPFLAGS) $@

mkweb : bin/mkwebc

bin/mkwebc : config.o fs_util.o system.o mkweb.o
	$(CXX) $(CXXFLAGS) -o $@ $^ -L`pwd`/local/lib -lyaml-cpp -lstdc++fs

configdump : config.o fs_util.o system.o configdump.o
	$(CXX) $(CXXFLAGS) -o $@ $^ -L`pwd`/local/lib -lyaml-cpp

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
	rm -f configdump
	rm -f bin/mkwebc

clean-all : clean
	rm -fr local

%.o : src/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $< -Isrc -I`pwd`/local/include

