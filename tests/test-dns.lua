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

exports['getaddrinfo_no_hint'] = function(test)
  local lev = require('lev')
  local dns = lev.dns;
  local flags = dns.NI_NUMERICHOST + dns.NI_NUMERICSERV
  dns.getaddrinfo("localhost", "http", nil, flags, function(err, names)
    test.is_nil(err)
    test.is_table(names)
    test.ok(#names > 0)
    test.done()
  end)
end

exports['getaddrinfo_ipv4'] = function(test)
  local lev = require('lev')
  local dns = lev.dns;
  local hints = {
    family = dns.PF_INET,
    socktype = dns.SOCK_STREAM,
    protocol = IPPROTO_TCP
  }
  local flags = dns.NI_NUMERICHOST + dns.NI_NUMERICSERV
  dns.getaddrinfo("localhost", nil, hints, flags, function(err, names)
    test.is_nil(err)
    test.is_table(names)
    if lev.process.platform == 'linux' then
      test.equal(#names, 2)
      test.equal(names[1].host, "127.0.0.1")
      test.equal(names[1].family, dns.PF_INET)
      test.equal(names[2].host, "127.0.0.1")
      test.equal(names[2].family, dns.PF_INET)
    elseif lev.process.platform == 'darwin' then
      test.equal(#names, 1)
      test.equal(names[1].host, "127.0.0.1")
      test.equal(names[1].family, dns.PF_INET)
    end
    test.done()
  end)
end

exports['getaddrinfo_ipv6'] = function(test)
  local lev = require('lev')
  local dns = lev.dns;
  local hints = {
    family = dns.PF_INET6,
    socktype = dns.SOCK_STREAM,
    protocol = IPPROTO_TCP
  }
  local flags = dns.NI_NUMERICHOST + dns.NI_NUMERICSERV
  dns.getaddrinfo("localhost", nil, hints, flags, function(err, names)
    test.is_nil(err)
    test.is_table(names)
    if lev.process.platform == 'linux' then
      test.equal(#names, 1)
      test.equal(names[1].host, "::1")
      test.equal(names[1].family, dns.PF_INET6)
    elseif lev.process.platform == 'darwin' then
      test.equal(#names, 2)
      test.equal(names[1].host, "::1")
      test.equal(names[1].family, dns.PF_INET6)
      test.equal(names[2].host, "fe80::1%lo0")
      test.equal(names[2].family, dns.PF_INET6)
    end
    test.done()
  end)
end

return exports
