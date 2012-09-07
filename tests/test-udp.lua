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
  local r = server:bind("127.0.0.1", SERVER_PORT)
  -- TODO: we must change bind() to follow common lua error pattern.
  test.ok(r, 0)
  p("server bind ok")
  r = server:recv_start(function(s, address, port, nread, buf)
    p("server recv callback")
    test.equal(address, "127.0.0.1")
    test.equal(port, CLIENT_PORT)
    test.equal(nread, #PING_BUF)
    test.equal(tostring(buf), "\0\1\2\3")

    r = s:send("127.0.0.1", CLIENT_PORT, PONG_BUF)
    test.ok(r, 0)
    p("server sent PONG")
  end)
  p("server before on_close", server)
  server:on_close(function(s)
    p("server closed", s)
  end)

  local client = lev.udp.new()
  local r = client:bind("127.0.0.1", CLIENT_PORT)
  r = client:recv_start(function(c, address, port, nread, buf)
    p("client recv callback")
    test.equal(address, "127.0.0.1")
    test.equal(port, SERVER_PORT)
    test.equal(nread, #PONG_BUF)
    test.equal(tostring(buf), "\4\5\6\7")

    c:close()
  end)
  client:on_close(function(c)
    p("client closed", c)
    test.done()
  end)
  r = client:send("127.0.0.1", SERVER_PORT, PING_BUF)
  test.ok(r, 0)
  p("client sent PING")
end

return exports
