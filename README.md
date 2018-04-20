# LD41

## Building

### Native

You're on your own.

### Emscripten

Install the [Emscripten SDK][emsdk].

Don't forget to `source emsdk/emsdk_env.sh` whenever you start a new shell.

#### Configuration

```shell
$ mkdir build
$ cd build
$ cmake .. \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake \
    -DCMAKE_BUILD_TYPE=Release
```

Debug builds are crazy slow, only use if necessary.

#### Building

```shell
$ ninja ld41_client
```

#### Output

```shell
$ ls www
ld41_client.data
ld41_client.html
ld41_client.html.mem
ld41_client.js
scripts/
```

#### Running

Opening `www/ld41_client.html` in a web browser should load the game.

If you get a black screen, check your browser's console.
Likely causes are:

- Your browser has blacklisted your GPU or drivers. Try another browser or update drivers.
- Your machine has no 3D hardware acceleration. Install drivers, don't use a VM.
- The loader scripts failed for some reason. Debug.

[emsdk]: https://kripken.github.io/emscripten-site/docs/getting_started/downloads.html
