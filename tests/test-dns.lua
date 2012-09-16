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

exports['dns_resolve_mx'] = function(test)
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

exports['dns_resolve_txt'] = function(test)
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

exports['dns_resolve_srv'] = function(test)
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

exports['dns_resolve_ns'] = function(test)
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

exports['dns_resolve_cname'] = function(test)
  local lev = require('lev')
  local dns = lev.dns
  local err = dns.resolveCname('www.google.com', function(err, names)
    test.is_nil(err)
    test.ok(#names > 0)
    for i = 1, #names do
      local address = names[i]
      test.equal(type(address), 'string')
    end
    test.done()
  end)
  test.is_nil(err)
end

exports['dns_reverse_ipv4'] = function(test)
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

exports['dns_reverse_ipv6'] = function(test)
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

exports['dns_lookup_ipv4_implicit'] = function(test)
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

exports['dns_lookup_ipv6_implicit'] = function(test)
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

exports['dns_lookup_ipv4_explicit'] = function(test)
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

exports['dns_lookup_ipv6_explicit'] = function(test)
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

exports['dns_lookup_ip_ipv4'] = function(test)
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

exports['dns_lookup_ip_ipv6'] = function(test)
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
