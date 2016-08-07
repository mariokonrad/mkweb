export CC=gcc-6
export CXX=g++-6
export CXXFLAGS=-Wall -Wextra -pedantic -O3 -ggdb -std=c++1z -static

STRIP=strip
STRIPFLAGS=-s

.PHONY: mkweb
mkweb : bin/mkweb

bin/mkweb : config.o system.o mkweb.o
	$(CXX) $(CXXFLAGS) -o $@ $^ -L`pwd`/local/lib -lyaml-cpp -lfmt -lstdc++fs
	$(STRIP) $(STRIPFLAGS) $@

.PHONY: prepare
prepare : yaml fmt cxxopts json

.PHONY: yaml
yaml :
	bin/build-yaml-cpp.sh `pwd`/build `pwd`/local

.PHONY: fmt
fmt :
	bin/build-fmt.sh `pwd`/build `pwd`/local

.PHONY: cxxopts
cxxopts :
	bin/build-cxxopts.sh `pwd`/build `pwd`/local

.PHONY: json
json :
	bin/build-json.sh `pwd`/build `pwd`/local

.PHONY: clean
clean :
	rm -f *.o
	rm -fr build

.PHONY: clean-all
clean-all : clean
	rm -fr local

.PHONY: clean-complete
clean-complete : clean-all
	rm -f bin/mkweb

%.o : src/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $< -Isrc -I`pwd`/local/include

