--[[

Copyright 2012 The lev Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

--]]

local utils = require('utils')

local exports = {}

exports['lev.utils:\tdump: default'] = function (test)
  local dump = utils.dump
  test.ok(utils.useColors)

  test.equal("\27[1;30mnil\27[0m", dump(nil)) -- nil
  test.equal('\27[1;32m"\27[0;32mh\27[1;32m\\\\\27[0;32me\27[1;32m\\n\27[0;32ml\27[1;32m\\r\27[0;32ml\27[1;32m\\t\27[0;32mowor\27[1;32m\\0\27[0;32mld\27[1;32m"\27[0m', dump('h\\e\nl\rl\towor\0ld')) -- string
  test.equal("\27[0;33mtrue\27[0m", dump(true)) -- boolean
  test.equal("\27[0;34m42\27[0m", dump(42)) -- number
  test.equal("\27[0;36m" .. tostring(print) .. "\27[0m", dump(print)) -- function

  local thread = _coroutine.create(function () end)
  test.equal("\27[1;31m" .. tostring(thread) .. "\27[0m", dump(thread)) -- thread

  -- table
  test.equal("{  }", dump({}))  -- empty
  local inner_table = {}
  local ot = "{ a = \27[0;34m1\27[0m, b = { a = \27[0;34m2\27[0m, c = \27[0;33m" .. tostring(inner_table) .. "\27[0m } }"
  test.equal(ot, dump({ a = 1, b = { a = 2, c = inner_table } }))
  
  -- TODO: dump userdata & cdata tests
  -- TODO: dump depth test

  test.done()
end

exports['lev.utils:\tdump: not use color'] = function (test)
  local dump = utils.dump
  utils.useColors = false

  test.equal("nil", dump(nil)) -- nil
  test.equal('"h\\\\e\\nl\\rl\\towor\\0ld"', dump('h\\e\nl\rl\towor\0ld')) -- string
  test.equal("true", dump(true)) -- boolean
  test.equal("42", dump(42)) -- number
  test.equal(tostring(print), dump(print)) -- function

  local thread = _coroutine.create(function () end)
  test.equal(tostring(thread), dump(thread)) -- thread

  -- table
  test.equal("{  }", dump({}))  -- empty
  local inner_table = {}
  local ot = "{ a = 1, b = { a = 2, c = " .. tostring(inner_table) .. " } }"
  test.equal(ot, dump({ a = 1, b = { a = 2, c = inner_table } }))
  
  -- TODO: dump userdata & cdata tests
  -- TODO: dump depth test

  test.done()
end

exports['lev.utils:\tprint'] = function (test)
  test.not_throws(utils.print, 1)
  test.not_throws(utils.print, 1, 2)
  test.not_throws(utils.print, 1, 2, 'hello')

  test.done()
end

exports['lev.utils:\tprettyPrint'] = function (test)
  local thread = _coroutine.create(function () end)
  local io = require('io')
  test.not_throws(utils.prettyPrint, 1)
  test.not_throws(utils.prettyPrint, 1, true)
  test.not_throws(utils.prettyPrint, 1, true, 'hello', print, thread, {}, io)

  test.done()
end

exports['lev.utils:\tdebug'] = function (test)
  local thread = _coroutine.create(function () end)
  local io = require('io')
  test.not_throws(utils.debug, 1)
  test.not_throws(utils.debug, 1, true)
  test.not_throws(utils.debug, 1, true, 'hello', print, thread, {}, io)

  test.done()
end

exports['lev.utils:\tbind'] = function (test)
  local say = function (self, x, y, z)
    print(self.message, x, y, z)
    return self.message, x, y, z
  end

  local fn = utils.bind(say, { message = 'hello' });
  test.ok(type(fn) == 'function')
  local msg, x, y, z = fn(1, 2, 3)
  test.equal('hello', msg)
  test.equal(1, x)
  test.equal(2, y)
  test.equal(3, z)

  test.done()
end

return exports
