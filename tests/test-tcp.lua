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

exports['lev.tcp:\ttcp_echo_test'] = function(test)

  local PING_BUF = Buffer:new("\0\1\2\3")
  local PONG_BUF = Buffer:new("\3\2\1\0")

  --TODO: Implement function for getting environment variables.
  --local PORT = process.env.PORT or 10082
  local PORT = 10082

  local TEST_BUF = Buffer:new("\9\8\7\6")

  local server = lev.tcp.new()
  server:bind("127.0.0.1", PORT)
  server:listen(function(s, err)
    p("server:listen", s, err)
    test.is_nil(err)
    local client = s:accept()
    p("accepted client", client)
    client:on_close(function(c)
      p("client closed", c)
      test.done()
    end)
    client:read_start(function(c, nread, buf)
      p("server:received")
      test.equal(nread, 4)
      test.equal(tostring(buf), "\0\1\2\3")

      -- this seems weird to have here, but it helps
      -- test an edge case with cBuffers
      local test_res = buf .. TEST_BUF
      test.equal(test_res:inspect(), "<Buffer 00 01 02 03 09 08 07 06 >")

      c:write(PONG_BUF)
    end)
  end)

  local client = lev.tcp.new()
  client:connect("127.0.0.1", PORT, function(...)
    p("connected")
    client:write(PING_BUF)
    client:read_start(function(c, nread, buf)
      p("client:received")
      test.equal(nread, 4)
      test.equal(tostring(buf), "\3\2\1\0")
      client:close()
    end)
  end)
end

return exports
