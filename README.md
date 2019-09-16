# About
This is a reverse-engineered re-creation of the [Sonic Battle](https://en.wikipedia.org/wiki/Sonic_Battle) 3D Game Boy Advance renderer.

Links:
* [Demo](https://fouramgames.com/demo/battle/BattleApp.html)
* [Analysis](https://fouramgames.com/blog/sonic-battle-renderer)

# Libraries
* Built with the [oryol](https://github.com/floooh/oryol) framework
* [stb_image](https://github.com/nothings/stb) library for loading `.png` files
* [pystring](https://github.com/imageworks/pystring) for parsing the level data files

# Building
* `stb_image` and `pystring` are header-only libraries and are already included in this repository. The only dependency you will need to set up is the `oryol`/`fips` ecosystem.
* The project was tested for fips' `win64-vstudio-debug` (Visual Studio 2017) and `webgl2-wasm-ninja-release` configurations.
