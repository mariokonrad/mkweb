
mkweb - Static Website Generator
================================

Copyright 2017 Mario Konrad <mario.konrad@gmx.net>

Static website generator supporting themes and plugins.
Using `pandoc` to convert pages from markdown to HTML.

Build
-----

Environment:

- GCC-6 (C++17)
- CMake 3.7.20170202-g2b025 (or newer)

Library dependencies:

- nlohman/json : https://github.com/nlohmann/json.git
- yaml-cpp     : https://github.com/jbeder/yaml-cpp.git
- cxxopts      : https://github.com/jarro2783/cxxopts.git
- fmt          : https://github.com/fmtlib/fmt.git

Will be built from local repositories (`$HOME/local/repo`)
and installed in `pwd/local`. Execute to build and install
dependencies:

	bin/prepare

Build (default):

	bin/prepare-build
	cd build
	make

