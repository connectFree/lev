# Global

These Variables are available in all modules. Some of these Variables aren't actually in the global scope but in the module scope.



## Variables

### Buffer
Used to handle binary data. See the [buffer section][].

* type : table

### WorkerID
The worker ID. When lev was started, set worker ID to this variable.

* type : string

### \_G
A global variable (not a function) that holds the global environment. See the [lua manual page][].

* type : table

### \_\_file\_\_
The filename of the code being executed. This is the relative path.

* type : string

### \_\_path\_\_
The name of the directory that the currently executing script resides in. This is the relative path.

* type : string

### arg
The command line argument array.

* type : table 

The following is `$ lev hello.lua hoge foo var`.

    for i, v in ipairs(arg) do
      print(i, v)
    end
    -- 1       hoge
    -- 2       foo
    -- 3       var

### bit
The bit operation variable. See the luajit [bit operations][].

* type : table


### lev
The lev core variable.

* type : table

### mbox
* type : table

### process
* type : table



## Functions

### debug(...)

A synchronous dump output function. See the `debug` function of [utils section][].

### p(...)

This function is alias of `prettyPrint`. See the [utils section][].



## Lua standard libraries
* type
* tostring
* tonumber
* pcall
* xpcall
* pairs
* ipairs
* next
* select
* unpack
* newproxy
* assert
* error
* print
* require
* module
* package
* getfenv
* setfenv
* getmetatable
* setmetatable
* rawset
* rawget
* rawequal
* loadstring
* loadfile
* load
* dofile
* gcinfo
* collectgarbage
* \_VERSION

See the [lua manual page][].


[buffer section]: buffer.html
[utils section]: utils.html
[bit operations]: http://bitop.luajit.org/api.html
[lua manual page]: http://www.lua.org/manual/5.2/
