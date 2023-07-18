## How to build and run using `mimalloc`

[`mimalloc`](https://github.com/microsoft/mimalloc) is a high-performance
allocator from Microsoft Research.  If this repository's root is `PROJECT_ROOT`, the Makefile
expects `mimalloc` to be at `$PROJECT_ROOT/../mimalloc`.  To use it with our C++ codebase, compile
and set your envvars like so:

```bash
USE_MIMALLOC=1 make
export LD_LIBRARY_PATH=../../mimalloc/out/release
## start running code
```

For extra stats, run `mimalloc` in verbose mode:

```bash
export MIMALLOC_VERBOSE=1
export MIMALLOC_SHOW_STATS=1
```
