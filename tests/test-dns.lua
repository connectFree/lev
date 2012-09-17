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

local process = lev.process

local exports = {}

exports['lev.dns\tresolve4'] = function(test)
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

exports['lev.dns\tresolve6'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local process = lev.process
  dns.resolve6('ipv6.google.com', function(err, ips)
    test.is_nil(err)
    test.ok(#ips > 0)
    for i = 1, #ips do
      test.ok(net.isIPv6(ips[i]))
    end
    test.done()
  end)
end

exports['lev.dns\tresolve_mx'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local err = dns.resolveMx('gmail.com', function(err, addresses)
    test.is_nil(err)
    test.ok(#addresses > 0)
    for i = 1, #addresses do
      local address = addresses[i]
      test.equal(type(address), 'table')
      test.equal(type(address.priority), 'number')
      test.equal(type(address.exchange), 'string')
    end
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\tresolve_txt'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local err = dns.resolveTxt('gmail.com', function(err, records)
    test.is_nil(err)
    test.ok(#records > 0)
    for i = 1, #records do
      local record = records[i]
      test.equal(type(record), 'string')
    end
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\tresolve_srv'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local err = dns.resolveSrv('_jabber._tcp.google.com', function(err, services)
    test.is_nil(err)
    test.ok(#services > 0)
    for i = 1, #services do
      local service = services[i]
      test.equal(type(service), 'table')
      test.equal(type(service.name), 'string')
      test.equal(type(service.priority), 'number')
      test.equal(type(service.port), 'number')
      test.equal(type(service.weight), 'number')
    end
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\tresolve_ns'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local err = dns.resolveNs('google.com', function(err, addresses)
    test.is_nil(err)
    test.ok(#addresses > 0)
    for i = 1, #addresses do
      local address = addresses[i]
      test.equal(type(address), 'string')
    end
    test.done()
  end)
  test.is_nil(err)
end

if process.getenv('TRAVIS') then
  print("SKIP dns_resolve_cname on TRAVIS")
else
  exports['lev.dns\tresolve_cname'] = function(test)
    local lev = require('lev')
    local dns = lev.dns
    local err = dns.resolveCname('www.google.com', function(err, names)
      test.is_nil(err)
      if names then
        test.ok(#names > 0)
        for i = 1, #names do
          local address = names[i]
          test.equal(type(address), 'string')
        end
      end
      test.done()
    end)
    test.is_nil(err)
  end
end

exports['lev.dns\tgeneric_ipv4'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  dns.resolve('www.google.com', 'A', function(err, ips)
    test.is_nil(err)
    test.ok(#ips > 0)
    for i = 1, #ips do
      test.ok(net.isIPv4(ips[i]))
    end
    test.done()
  end)
end

exports['lev.dns\tgeneric_resolve_ipv6'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  dns.resolve('ipv6.google.com', 'AAAA', function(err, ips)
    test.is_nil(err)
    test.ok(#ips > 0)
    for i = 1, #ips do
      test.ok(net.isIPv6(ips[i]))
    end
    test.done()
  end)
end

exports['lev.dns\tgeneric_resolve_mx'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local err = dns.resolve('gmail.com', 'MX', function(err, addresses)
    test.is_nil(err)
    test.ok(#addresses > 0)
    for i = 1, #addresses do
      local address = addresses[i]
      test.equal(type(address), 'table')
      test.equal(type(address.priority), 'number')
      test.equal(type(address.exchange), 'string')
    end
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\tgeneric_resolve_txt'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local err = dns.resolve('gmail.com', 'TXT', function(err, records)
    test.is_nil(err)
    test.ok(#records > 0)
    for i = 1, #records do
      local record = records[i]
      test.equal(type(record), 'string')
    end
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\tgeneric_resolve_srv'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local err = dns.resolve('_jabber._tcp.google.com', 'SRV',
      function(err, services)
    test.is_nil(err)
    test.ok(#services > 0)
    for i = 1, #services do
      local service = services[i]
      test.equal(type(service), 'table')
      test.equal(type(service.name), 'string')
      test.equal(type(service.priority), 'number')
      test.equal(type(service.port), 'number')
      test.equal(type(service.weight), 'number')
    end
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\tgeneric_resolve_ns'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local err = dns.resolve('google.com', 'NS', function(err, addresses)
    test.is_nil(err)
    test.ok(#addresses > 0)
    for i = 1, #addresses do
      local address = addresses[i]
      test.equal(type(address), 'string')
    end
    test.done()
  end)
  test.is_nil(err)
end

if process.getenv('TRAVIS') then
  print("SKIP dns_generic_resolve_cname on TRAVIS")
else
  -- NOTE: This test sometimes fails on Ubunt 12.04.
  exports['lev.dns\tgeneric_resolve_cname'] = function(test)
    local lev = require('lev')
    local dns = lev.dns
    local err = dns.resolve('www.google.com', 'CNAME', function(err, names)
      test.is_nil(err)
      if names then
        test.ok(#names > 0)
        for i = 1, #names do
          local address = names[i]
          test.equal(type(address), 'string')
        end
      end
      test.done()
    end)
    test.is_nil(err)
  end
end

exports['lev.dns\tgeneric_resolve_reverse_ipv4'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.resolve('8.8.8.8', 'PTR', function(err, domains)
    test.is_nil(err)
    test.ok(#domains > 0)
    for i = 1, #domains do
      test.equal(type(domains[i]), 'string')
    end
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\tgeneric_resolve_reverse_ipv6'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.resolve('2001:4860:4860::8888', 'PTR', function(err, domains)
    test.is_nil(err)
    test.ok(#domains > 0)
    for i = 1, #domains do
      test.equal(type(domains[i]), 'string')
    end
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\treverse_ipv4'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.reverse('8.8.8.8', function(err, domains)
    test.is_nil(err)
    test.ok(#domains > 0)
    for i = 1, #domains do
      test.equal(type(domains[i]), 'string')
    end
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\treverse_ipv6'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.reverse('2001:4860:4860::8888', function(err, domains)
    test.is_nil(err)
    test.ok(#domains > 0)
    for i = 1, #domains do
      test.equal(type(domains[i]), 'string')
    end
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\treverse_bad_ip'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.reverse('this_is_invalid_ip', function(err, domains)
    test.ok(false) -- callback should not be called.
  end)
  test.equal(err, 'ENOTIMP')
  test.done()
end

exports['lev.dns\tlookup_ipv4_implicit'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.lookup('www.google.com', function(err, ip, family)
    test.is_nil(err)
    test.ok(net.isIPv4(ip))
    test.equal(family, 4)
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\tlookup_ipv6_implicit'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.lookup('ipv6.google.com', function(err, ip, family)
    test.is_nil(err)
    test.ok(net.isIPv6(ip))
    test.equal(family, 6)
    test.done()
  end)
  test.is_nil(err)
end

--[[ TODO: fix callback never called
exports['dns_lookup_nonexist'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.lookupFamily('does.not.exist', 4, function(err, ip, family)
    test.equal(err, 'ENOTFOUND')
    test.done()
  end)
  test.is_nil(err)
end
--]]

exports['lev.dns\tlookup_ipv4_explicit'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.lookupFamily('www.google.com', 4, function(err, ip, family)
    test.is_nil(err)
    test.ok(net.isIPv4(ip))
    test.equal(family, 4)
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\tlookup_ipv6_explicit'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.lookupFamily('ipv6.google.com', 6, function(err, ip, family)
    test.is_nil(err)
    test.ok(net.isIPv6(ip))
    test.equal(family, 6)
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\tlookup_ip_ipv4'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.lookupFamily('127.0.0.1', 4, function(err, ip, family)
    test.is_nil(err)
    test.equal(ip, '127.0.0.1')
    test.ok(net.isIPv4(ip))
    test.equal(family, 4)
    test.done()
  end)
  test.is_nil(err)
end

exports['lev.dns\tlookup_ip_ipv6'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local net = lev.net
  local err = dns.lookupFamily('::1', 6, function(err, ip, family)
    test.is_nil(err)
    test.ok(net.isIPv6(ip))
    test.equal(family, 6)
    test.done()
  end)
  test.is_nil(err)
end

return exports
