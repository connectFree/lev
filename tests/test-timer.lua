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

exports['lev.timer:\ttcp_timer_once'] = function(test)
  local lev = require('lev')
  local timer = lev.timer.new()
  timer:start(function(t, status)
    test.ok(t)
    test.equal(status, 0)
    timer:close(function(t)
      test.done()
    end)
  end, 100)
end

exports['lev.timer:\ttcp_timer_repeat'] = function(test)
  local lev = require('lev')
  local timer = lev.timer.new()
  local called_count = 0
  timer:start(function(t, status)
    test.ok(t)
    test.equal(status, 0)
    called_count = called_count + 1
    if called_count == 5 then
      timer:close(function(t)
        test.done()
      end)
    end
  end, 200, 100)
end

exports['lev.timer:\ttcp_timer_stop'] = function(test)
  local lev = require('lev')
  local timer = lev.timer.new()
  local called_count = 0
  timer:start(function(t, status)
    test.ok(false)
  end, 2000)
  timer:stop()
  timer:close(function(t)
    test.ok(true)
    test.done()
  end)
end

exports['lev.timer:\ttcp_timer_again'] = function(test)
  local lev = require('lev')
  local timer = lev.timer.new()
  local called_count = 0
  timer:start(function(t, status)
    test.ok(t)
    test.equal(status, 0)
    called_count = called_count + 1
    if called_count == 5 then
      timer:close(function(t)
        test.done()
      end)
    end
  end, 200, 100)
  timer:stop()
  timer:again()
end

exports['lev.timer:\ttcp_timer_again_error'] = function(test)
  local lev = require('lev')
  local constants = require('constants')
  local timer = lev.timer.new()
  local err = timer:again()
  test.equal(err, 'EINVAL')
  test.done()
end

return exports
