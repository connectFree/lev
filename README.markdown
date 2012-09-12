# Welcome to lev -- Lua for Event-Based IO

> _`lev` is everything great about `Node`, without the "BS" of JS._  
― [kristopher tate](https://github.com/kristate), lev founder

Here are some initial reasons to jump on board:

### `lev` is faster than `node`, `python`, `ruby` and many others.
**why:** `lev` can easily reach over 10,000 requests per core per machine and scales near-linearly  
**how:** `lev` allows developers to work with IO streams without the raw data ever touching the VM. A special slab allocator and trash-stack handles memory availability while our cBuffer object integrates nicely with the VM. Multi CPU/Core support is built-in from the start.

### `lev` uses less memory than `node`, `python`, `ruby` and many others.
**why:** At start-up, `lev` only allocates around 2MB and after benchmarking HTTP a little over 3MB  
**how:** `lev` is based on the very compact Lua language. A special Buffer object was also written in C to help quickly allocate and work with streams.

### `lev` makes integrating great C-Libraries a breeze!
**why:** `lev` is based on Lua which has a very simple and compact C ABI  
**how:** Use FFI or write a wrapper in C. Porting to `lev` should be simpler than others such as `python`, `node` or `ruby`.


Join us on IRC at #levdev on freenode.net


### Building from git

Grab a copy of the source code:

`git clone https://github.com/connectFree/lev.git --recursive`


To use the Makefile build system (for embedded systems without python)
run:

```
cd lev
CC=gcc make
make test
./build/lev
```

To install with the Makefile build system run:

```
sudo make install
```

Note: Travis CI uses the Makefile build system. So please make sure that it builds OK with the Makefile build system before sending pull requests.
