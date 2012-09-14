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
  test.is_number(t)
  test.ok(t >= 0)

  test.done()
end

exports['lev.core:\texecpath'] = function (test)
  test.is_string(lev.execpath())

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

exports['lev.core:\tget_process_title'] = function (test)
  local title = lev.get_process_title()

  test.is_string(title)

  test.done()
end

-- NOTE: this test pass linux only. because uv_set_process_title do not support all platform. 
exports['lev.core:\tset_process_title'] = function (test)
  test.is_nil(lev.set_process_title('lev_unittest'))
  test.equal(lev.get_process_title(), 'lev_unittest')

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
-- exports['test core active_signal_handler'] = function (test)
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

-- TODO: How do we test?
--exports['lev.core:\tsetuid: specify string value'] = function (test)
--  local old = lev.getuid()
--  local ret = lev.setuid('nobody')
--  local new = lev.getuid()
--  p(old, new)
--
--  test.is_nil(ret)
--  test.not_is_equal(new, old)
--
--  test.done()
--end

-- TODO: How do we test?
--exports['lev.core:\tsetgid: specify string value'] = function (test)
--  local old = lev.getgid()
--  local ret = lev.setuid('nobody')
--  local new = lev.getgid()
--  p(old, new)
--
--  test.is_nil(ret)
--  test.not_is_equal(new, old)
--
--  test.done()
--end

exports['lev.core:\tumask: specify number value'] = function (test)
  local mask = 0644
  local old = lev.umask(mask)

  test.equal(mask, lev.umask(old))
  test.equal(old, lev.umask())
  test.equal(old, lev.umask())

  test.done()
end

exports['lev.core:\tumask: specify string value'] = function (test)
  local mask = 0644
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

exports['test core platform'] = function (test)
  test.is_string(lev.platform)
  test.ok(#lev.platform > 0)

  test.done()
end

exports['test core arch'] = function (test)
  test.is_string(lev.arch)
  test.ok(#lev.arch > 0)

  test.done()
end

return exports

