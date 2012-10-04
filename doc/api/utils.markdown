# Utils

These functions and properties are in the module `'utils'`. Use `require('utils')` to access
them.

---

## Variables

### useColors
Use the color in the output. This variable used `dump`, `prettyPrint` and `debug` functions.

* type : boolean
* default : true

    local utils = require('utils')
    utils.useColors = false
    ...
    utils.prettyPrint('hello world')

If you would not like to color output, set `false` to this variable.

---

## Functions

### bind(fn, self, [...])
Creates a new function that, when called, has its `self` keyword set to the provided value, with a given sequence of arguments preceding any provided when the new function was called.

#### parameters
* fn : a target function (required)
* self : a target object (required)
* ... : arguments of `fn` function (optional)

#### returns
Return a bound function.

    local say = function (self, x, y, z)
      print(self.message, x, y, z)
      return self.message, x, y, z
    end

    local fn = utils.bind(say, { message = 'hello' })
    print(fn(1, 2, 3)) -- 'hello', 1, 2, 3


### debug([...])
A synchronous dump output function. Will block the process and output arguments immediately to `stderr`.

#### parameters
* ... : some arguments (optional)

#### returns
Return the `nil`.

    utils.debug('hello world', 12, true)

If `useColors` is `false`, this function does not output the color codes.


### dump(v, [depth])
Return a string representation of value, which is useful for debugging.

#### parameters
* o : a target value (required)
* depth : an indent level (optional, default: 0)

#### returns
Return a string of value. If `useColors` is `false`, Return a string does not include the color codes.

    print(utils.dump(true))
    print(utils.dump({ hoge = 'hoge', foo = 1, bar = { a = 1, b = 2 } }, 2)


### prettyPrint([...])
A synchronous dump output function. Will block the process and output arguments immediately to `stdout`.

#### parameters
* ... : some arguments (optional)

#### returns
Return the `nil`.

    utils.prettyPrint('hello world', 12, true)

If `useColors` is `false`, this function does not output the color codes.

