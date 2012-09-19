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

local lev = require("lev")
local process = lev.process

local exports = {}


exports['lev.process:\tenviron'] = function (test)
  local found = false
  for k, v in process.environ() do
    if k == 'LEV_WORKER_ID' then
      found = true
    end
  end
  test.ok(found)
  test.done()
end

exports['lev.process:\tpid'] = function (test)
  test.is_number(process.pid)
  test.ok(process.pid > 0)

  test.done()
end

exports['lev.process:\texecpath'] = function (test)
  local path = process.execpath()

  test.is_string(path)
  test.ok(#path > 0)

  test.done()
end

exports['lev.process:\tmemory_usage'] = function (test)
  local mem = process.memory_usage()

  test.is_table(mem)
  test.is_number(mem.rss)
  test.ok(mem.rss >= 0)
  -- NOTE: disable luajit GC infomation
  --test.is_table(mem.gc)
  --test.is_number(mem.gc.total)
  --test.ok(mem.gc.total >= 0)
  --test.is_number(mem.gc.threshold)
  --test.ok(mem.gc.threshold >= 0)
  --test.is_number(mem.gc.stepmul)
  --test.ok(mem.gc.stepmul >= 0)
  --test.is_number(mem.gc.debt)
  --test.ok(mem.gc.debt >= 0)
  --test.is_number(mem.gc.estimate)
  --test.ok(mem.gc.estimate >= 0)
  --test.is_number(mem.gc.pause)
  --test.ok(mem.gc.pause >= 0)

  test.done()
end

exports['lev.process:\ttitle'] = function (test)
  local title = process.title

  test.is_string(title)
  test.ok(#title > 0)

  test.done()
end

exports['lev.process:\tcwd'] = function (test)
  local cur = process.cwd()

  test.is_string(cur)
  test.ok(#cur > 0)

  test.done()
end

exports['lev.process:\tchdir'] = function (test)
  local old = process.cwd()
  local ret = process.chdir('../')
  local new = process.cwd()
  process.chdir(old)

  test.is_nil(ret)
  test.not_equal(new, old)

  test.done()
end

exports['lev.process:\tchdir: spcify invalid path'] = function (test)
  test.throws(process.chdir, 1)
  test.throws(process.chdir, 'hoge')

  test.done()
end

exports['lev.process:\tgetenv'] = function (test)
  process.setenv('FOO', 'BAR')
  local val = process.getenv('FOO')

  test.equal('BAR', val)

  test.done()
end

exports['lev.process:\tsetenv'] = function (test)
  local err = process.setenv('BAZ', 'FOO')
  local val = process.getenv('BAZ')

  test.is_nil(err)
  test.equal('FOO', val)

  test.done()
end

exports['lev.process:\tsetenv: specify invalid name'] = function (test)
  test.throws(process.setenv, {}, 'FOO')

  test.done()
end

exports['lev.process:\tsetenv: specify invalid value'] = function (test)
  test.throws(process.setenv, "FOO", {})

  test.done()
end

exports['lev.process:\tunsetenv'] = function (test)
  local err = process.unsetenv('FOO')

  test.is_nil(err)
  test.is_nil(process.getenv('FOO'))

  test.done()
end

exports['lev.process:\t unsetenv: specify invalid name'] = function (test)
  test.throws(process.unsetenv, {})

  test.done()
end


return exports

