.PHONY: default clean clean-all mkweb

export CC=gcc-6
export CXX=g++-6
export CXXFLAGS=-Wall -Wextra -pedantic -O3 -ggdb -std=c++1z -static

STRIP=strip
STRIPFLAGS=-s

mkweb : bin/mkwebc

bin/mkwebc : config.o system.o mkweb.o
	$(CXX) $(CXXFLAGS) -o $@ $^ -L`pwd`/local/lib -lyaml-cpp -lfmt -lstdc++fs
	$(STRIP) $(STRIPFLAGS) $@

yaml :
	if [ ! -d local ] ; then mkdir local ;fi
	mkdir build
	(cd build ; cmake ~/local/repo/yaml-cpp -DCMAKE_INSTALL_PREFIX=`pwd`/../local -DYAML_CPP_BUILD_TOOLS=OFF -DYAML_CPP_BUILD_CONTRIB=OFF)
	(cd build ; make -j 8)
	(cd build ; make install)
	rm -fr build

fmt :
	if [ ! -d local ] ; then mkdir local ;fi
	mkdir build
	(cd build ; cmake ~/local/repo/fmt -DCMAKE_INSTALL_PREFIX=`pwd`/../local -DFMT_USE_CPP11=ON -DFMT_TEST=OFF -DFMT_DOC=OFF)
	(cd build ; make -j 8)
	(cd build ; make install)
	rm -fr build

clean :
	rm -f *.o
	rm -fr build
	rm -f bin/mkwebc

clean-all : clean
	rm -fr local

%.o : src/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $< -Isrc -I`pwd`/local/include

