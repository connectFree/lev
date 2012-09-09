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

exports['udp_ping_pong_test'] = function(test)

  local lev = require('lev')

  local PING_BUF = Buffer:new("\0\1\2\3")
  local PONG_BUF = Buffer:new("\4\5\6\7")

  --TODO: Implement function for getting environment variables.
  --local PORT = process.env.PORT or 10082
  local SERVER_PORT = 10082
  local CLIENT_PORT = 10083

  local server = lev.udp.new()
  local err = server:bind("127.0.0.1", SERVER_PORT)
  test.is_nil(err)
  err = server:recv_start(function(s, err, address, port, nread, buf)
    test.is_nil(err)
    test.equal(address, "127.0.0.1")
    test.equal(port, CLIENT_PORT)
    test.equal(nread, #PING_BUF)
    test.equal(tostring(buf), "\0\1\2\3")

    err = s:send("127.0.0.1", CLIENT_PORT, PONG_BUF)
    test.is_nil(err)
  end)
  server:on_close(function(s)
    test.ok(true)
  end)
  test.is_nil(err)


  local client = lev.udp.new()
  err = client:bind("127.0.0.1", CLIENT_PORT)
  err = client:recv_start(function(c, err, address, port, nread, buf)
    test.is_nil(err)
    test.equal(address, "127.0.0.1")
    test.equal(port, SERVER_PORT)
    test.equal(nread, #PONG_BUF)
    test.equal(tostring(buf), "\4\5\6\7")

    c:close()
  end)
  test.is_nil(err)
  client:on_close(function(c)
    test.done()
  end)
  err = client:send("127.0.0.1", SERVER_PORT, PING_BUF)
  test.is_nil(err)
end

exports['udp_bind_error'] = function(test)
  local lev = require('lev')
  local PORT = 10082

  local server = lev.udp.new()
  local err = server:bind("127.0.0.1", PORT)
  test.is_nil(err)

  local err = server:bind("127.0.0.1", PORT)
  test.equal(err.code, 8 --[[EALREADY]])

  server:close()
  test.done()
end

return exports
