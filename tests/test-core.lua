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

-- emulate sleep function
local sleep = function ()
  local s = ''
  for i = 1, 4096 do
    s = s .. tostring(i)
  end
end

exports['test core now'] = function (test)
  local t = lev.now()

  test.is_number(t)
  test.ok(t >= 0)

  test.done()
end

exports['test core update_time'] = function (test)
  local t1 = lev.now()

  sleep()
  test.is_nil(lev.update_time())
  test.ok((lev.now() - t1) >= 0)

  test.done()
end

exports['test core hrtime'] = function (test)
  local t = lev.hrtime()
  test.is_number(t)
  test.ok(t >= 0)

  test.done()
end

exports['test core uptime'] = function (test)
  local t = lev.uptime()
  test.is_number(t)
  test.ok(t >= 0)

  test.done()
end

exports['test core execpath'] = function (test)
  test.is_string(lev.execpath())

  test.done()
end

exports['test core cpu_info'] = function (test)
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

exports['test core interface_addresses'] = function (test)
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

exports['test core loadavg'] = function (test)
  local avg = lev.loadavg()

  test.ok(#avg > 0)
  for _, v in pairs(avg) do
    test.ok(v >= 0)
  end

  test.done()
end

exports['test core get_free_memory'] = function (test)
  local free_mem = lev.get_free_memory()

  test.ok(free_mem >= 0)

  test.done()
end

exports['test core get_total_memory'] = function (test)
  local total_mem = lev.get_free_memory()

  test.ok(total_mem >= 0)

  test.done()
end

exports['test core get_process_title'] = function (test)
  local title = lev.get_process_title()

  test.is_string(title)

  test.done()
end

-- NOTE: this test pass linux only. because uv_set_process_title do not support all platform. 
exports['test core set_process_title'] = function (test)
  test.is_nil(lev.set_process_title('lev_unittest'))
  test.equal(lev.get_process_title(), 'lev_unittest')

  test.done()
end

exports['test core print_active_handles'] = function (test)
  test.is_nil(lev.print_active_handles())

  test.done()
end

exports['test core print_all_handles'] = function (test)
  test.is_nil(lev.print_all_handles())

  test.done()
end

exports['test core handle_type'] = function (test)
  test.equal(lev.handle_type(0), 'TTY') -- stdin
  test.equal(lev.handle_type(1), 'TTY') -- stdout

  test.done()
end

-- TODO: should be implemented test for the libuv that supported uv_signal_t version.
-- exports['test core active_signal_handler'] = function (test)
--   test.done()
-- end

return exports
