
mkweb - Static Website Generator
================================

Copyright 2018 Mario Konrad (mario.konrad@gmx.net)

Static website generator supporting themes and plugins.
Using `pandoc` to convert pages from markdown to HTML.


License
-------

GPL v3, see file `LICENSE`.


Build
-----

Environment:

- GCC-6, GCC-7, GCC-8 (C++17)
- CMake 3.13 (or newer)

Library dependencies:

- nlohman/json : v3.1.1  : https://github.com/nlohmann/json.git
- yaml-cpp     : 0.6.2   : https://github.com/jbeder/yaml-cpp.git
- cxxopts      : v1.0.0  : https://github.com/jarro2783/cxxopts.git
- fmt          : 3.0.1   : https://github.com/fmtlib/fmt.git

Will be built from local repositories (`${HOME}/local/repo`) if available
or from the remote repositories (shallow clones in `/tmp`) listed above,
and installed in `$(pwd)/local`. Execute to build and install dependencies:

	mkdir build
	cd build
	cmake -DCMAKE_CXX_COMPILER=g++-8 -DCMAKE_BUILD_TYPE=Release ..
	make
	make package

