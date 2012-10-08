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


local exports = {}

-- emulate noop function
local noop = function ()
  local s = ''
  for i = 1, (4096 * 2) do
    s = s .. tostring(i)
  end
end

exports['lev.core:\tnow'] = function (test)
  local t = lev.now()

  test.is_number(t)
  test.ok(t >= 0)

  test.done()
end

exports['lev.core:\tupdate_time'] = function (test)
  local t1 = lev.now()

  noop()
  test.is_nil(lev.update_time())
  test.ok((lev.now() - t1) >= 0)

  test.done()
end

exports['lev.core:\thrtime'] = function (test)
  local t = lev.hrtime()
  test.is_number(t)
  test.ok(t >= 0)

  test.done()
end

exports['lev.core:\tuptime'] = function (test)
  local t = lev.uptime()
  p('uptime', t)
  test.is_number(t)
  test.ok(t >= 0)

  test.done()
end

exports['lev.core:\tcpu_info'] = function (test)
  local cpu_info = lev.cpu_info()

  test.is_table(cpu_info)
  for _, v in pairs(cpu_info) do
    test.is_table(v)
    test.ok(v.times)
    test.is_table(v.times)
    test.ok(v.times.irq)
    test.ok(v.times.user)
    test.ok(v.times.idle)
    test.ok(v.times.sys)
    test.ok(v.times.nice)
    test.ok(v.model)
    test.ok(v.speed)
  end

  test.done()
end

exports['lev.core:\tinterface_addresses'] = function (test)
  local addresses = lev.interface_addresses()

  test.is_table(addresses)
  for _, address in pairs(addresses) do
    for _, v in pairs(address) do
      test.is_table(v)
      test.not_is_nil(v.address)
      test.not_is_nil(v.internal)
      test.not_is_nil(v.family)
    end
  end

  test.done()
end

exports['lev.core:\tloadavg'] = function (test)
  local avg1, avg2, avg3 = lev.loadavg()

  test.is_number(avg1)
  test.ok(avg1 >= 0)
  test.is_number(avg2)
  test.ok(avg2 >= 0)
  test.is_number(avg3)
  test.ok(avg3 >= 0)

  test.done()
end

exports['lev.core:\tget_free_memory'] = function (test)
  local free_mem = lev.get_free_memory()

  test.ok(free_mem >= 0)

  test.done()
end

exports['lev.core:\tget_total_memory'] = function (test)
  local total_mem = lev.get_free_memory()

  test.ok(total_mem >= 0)

  test.done()
end

-- TODO; should be support debug module
--[[
exports['lev.core:\tprint_active_handles'] = function (test)
  test.is_nil(lev.print_active_handles())

  test.done()
end

exports['lev.core:\tprint_all_handles'] = function (test)
  test.is_nil(lev.print_all_handles())

  test.done()
end
--]]

exports['lev.core:\thandle_type'] = function (test)
  test.equal(lev.handle_type(0), 'FILE') -- stdin
  test.equal(lev.handle_type(1), 'TTY') -- stout

  test.done()
end

-- TODO: should be implemented test for the libuv that supported uv_signal_t version.
-- exports['lev.core:\tactive_signal_handler'] = function (test)
--   test.done()
-- end

exports['lev.core:\tgetuid'] = function (test)
  local uid = lev.getuid()

  test.is_number(uid)
  test.ok(0 <= uid and uid <= (2^32 - 1))

  test.done()
end

exports['lev.core:\tgetgid'] = function (test)
  local gid = lev.getgid()

  test.is_number(gid)
  test.ok(0 <= gid and gid <= (2^32 - 1))

  test.done()
end

exports['lev.core:\tumask: specify number value'] = function (test)
  local mask = tonumber('0644', 8)
  local old = lev.umask(mask)

  test.equal(mask, lev.umask(old))
  test.equal(old, lev.umask())
  test.equal(old, lev.umask())

  test.done()
end

exports['lev.core:\tumask: specify string value'] = function (test)
  local mask = tonumber('0644', 8)
  local old = lev.umask('0644')

  test.equal(mask, lev.umask(old))
  test.equal(old, lev.umask())
  test.equal(old, lev.umask())

  test.done()
end

exports['lev.core:\tumask: specify invalid value'] = function (test)
  test.throws(lev.umask, 'hoge')
  test.throws(lev.umask, 'こんにちは')

  test.done()
end

exports['lev.core:\tplatform'] = function (test)
  test.is_string(lev.platform)
  test.ok(#lev.platform > 0)

  test.done()
end

exports['lev.core:\tarch'] = function (test)
  test.is_string(lev.arch)
  test.ok(#lev.arch > 0)

  test.done()
end

exports['lev.core:\tversion'] = function (test)
  test.is_string(lev.version)
  test.ok(#lev.version > 0)

  test.done()
end

exports['lev.core:\tversions'] = function (test)
  test.is_table(lev.versions)
  test.is_string(lev.versions.lev)
  test.ok(#lev.versions.lev > 0)
  test.is_string(lev.versions.http_perser)
  test.ok(#lev.versions.http_perser > 0)
  test.is_string(lev.versions.libuv)
  test.ok(#lev.versions.libuv > 0)
  test.is_string(lev.versions.luajit)
  test.ok(#lev.versions.luajit > 0)

  test.done()
end

exports['lev.core:\tenviron'] = function (test)
  local found = false
  for k, v in lev.environ() do
    if k == 'LEV_WORKER_ID' then
      found = true
    end
  end
  test.ok(found)

  test.done()
end

exports['lev.core:\tpid'] = function (test)
  test.is_number(lev.pid)
  test.ok(lev.pid > 0)

  test.done()
end

exports['lev.core:\texecpath'] = function (test)
  local path = lev.execpath()

  test.is_string(path)
  test.ok(#path > 0)

  test.done()
end

exports['lev.core:\tmemory_usage'] = function (test)
  local mem = lev.memory_usage()

  test.is_table(mem)
  test.is_number(mem.rss)
  test.ok(mem.rss >= 0)

  test.done()
end

exports['lev.core:\ttitle'] = function (test)
  local title = lev.title

  test.is_string(title)
  test.ok(#title > 0)

  test.done()
end

exports['lev.core:\tcwd'] = function (test)
  local cur = lev.cwd()

  test.is_string(cur)
  test.ok(#cur > 0)

  test.done()
end

exports['lev.core:\tchdir'] = function (test)
  local old = lev.cwd()
  local ret = lev.chdir('../')
  local new = lev.cwd()
  lev.chdir(old)

  test.is_nil(ret)
  test.not_equal(new, old)

  test.done()
end

exports['lev.core:\tchdir: spcify invalid path'] = function (test)
  test.throws(lev.chdir, 1)
  test.throws(lev.chdir, 'hoge')

  test.done()
end

exports['lev.core:\tgetenv'] = function (test)
  lev.setenv('FOO', 'BAR')
  local val = lev.getenv('FOO')

  test.equal('BAR', val)

  test.done()
end

exports['lev.core:\tsetenv'] = function (test)
  local err = lev.setenv('BAZ', 'FOO')
  local val = lev.getenv('BAZ')

  test.is_nil(err)
  test.equal('FOO', val)

  test.done()
end

exports['lev.core:\tsetenv: specify invalid name'] = function (test)
  test.throws(lev.setenv, {}, 'FOO')

  test.done()
end

exports['lev.core:\tsetenv: specify invalid value'] = function (test)
  test.throws(lev.setenv, "FOO", {})

  test.done()
end

exports['lev.core:\tunsetenv'] = function (test)
  local err = lev.unsetenv('FOO')

  test.is_nil(err)
  test.is_nil(lev.getenv('FOO'))

  test.done()
end

exports['lev.core:\t unsetenv: specify invalid name'] = function (test)
  test.throws(lev.unsetenv, {})

  test.done()
end

return exports

