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

exports['dns_resolve4'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  dns.resolve4('www.google.com', function(err, ips)
    test.is_nil(err)
    test.ok(#ips > 0)
    for i = 1, #ips do
      test.ok(net.isIPv4(ips[i]))
    end
    test.done()
  end)
end

--[[ TODO: fix callback never called
exports['dns_resolve4_nonexist'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  dns.resolve4('there_is_no_such_domain', function(err, ips)
    test.ok(err)
    test.done()
  end)
end
--]]

exports['dns_resolve6'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  dns.resolve6('ipv6.google.com', function(err, ips)
    test.is_nil(err)
    test.ok(#ips > 0)
    for i = 1, #ips do
      test.ok(net.isIPv6(ips[i]))
    end
    test.done()
  end)
end

exports['dns_reverse_ipv4'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.reverse('8.8.8.8', function(err, domains)
    test.is_nil(err)
    test.ok(#domains > 0)
    for i = 1, #domains do
      test.ok(type(domains[i]) == 'string')
    end
    test.done()
  end)
  test.is_nil(err)
end

exports['dns_reverse_ipv6'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.reverse('2001:4860:4860::8888', function(err, domains)
    test.is_nil(err)
    test.ok(#domains > 0)
    for i = 1, #domains do
      test.ok(type(domains[i]) == 'string')
    end
    test.done()
  end)
  test.is_nil(err)
end

exports['dns_reverse_bad_ip'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.reverse('this_is_invalid_ip', function(err, domains)
    test.ok(false) -- callback should not be called.
  end)
  test.equal(err, 'ENOTIMP')
  test.done()
end

return exports
