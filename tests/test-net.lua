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

exports['net_isIPv4'] = function(test)
  local lev = require('lev')
  local net = lev.net
  test.ok(net.isIPv4("127.0.0.1"))
  test.ok(net.isIPv4("192.168.0.1"))
  test.ok(net.isIPv4("207.97.227.239"))
  test.ok(net.isIPv4("255.255.255.255"))
  test.ok(not net.isIPv4("localhost"))
  test.ok(not net.isIPv4("This is not an IPv4 address."))
  test.done()
end

exports['net_isIPv6'] = function(test)
  local lev = require('lev')
  local net = lev.net
  test.ok(net.isIPv6("2001:0db8:bd05:01d2:288a:1fc0:0001:10ee"))
  test.ok(net.isIPv6("2001:db8::9abc"))
  test.ok(net.isIPv6("::"))
  test.ok(net.isIPv6("::1"))
  test.ok(not net.isIPv6("localhost"))
  test.ok(not net.isIPv6("This is not an IPv6 address."))
  test.done()
end

return exports
