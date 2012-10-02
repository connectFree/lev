# Global


## Properties

### Buffer
* {table} The buffer object.

Used to handle binary data. See the [buffer section][]

### WorkerID
* {string}

### \_G
* {table} The global object.

### \_\_file\_\_
* {string}

### \_\_path\_\_
* {string}

### arg
* {table} The command line argument array.

### bit
* {table} The bit operation object.

See the luajit [bit operations][]

### lev
* {table} The lev core object.

### mbox
* {table}

### process
* {table}


## functions

### debug(...)

### p(...)


## lua standard libraries
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

See the [lua manual page][]


[buffer section]: buffer.html
[bit operations]: http://bitop.luajit.org/api.html
[lua manual page]: http://www.lua.org/manual/5.2/
