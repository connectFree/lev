local levbase = require('levbase')

local server = levbase.tcp.new()
server:bind("0.0.0.0", 8080)
server:listen(function(s, err)
  local client = s:accept()
  p("accepted client", client)
  client:on_close(function(c)
    p("client closed", c)
  end)
  client:read_start(function(c, nread, buf)
    if buf then
      p("echoing", buf)
      c:write(buf)
    end
  end)
end)


--local server = require('server')
--local native = require('uv_native')
--
--p(server.new)
--
--server.new(
--   "0.0.0.0"
--  ,8080
--  ,function(...) -- server related events
--    local s = {...}
--    p("server:", s, "end")
--    s = nil
--    end
--  ,function(...) -- client related events
--    local c = {...}
--    if c[2] == "read" then --reply!
--      native.write(c[1], c[2], function()
--        --write call back
--        p("wrote to line")
--      end)
--    end
--
--    p("client:", c, "end")
--    c = nil
--    end
--  )-- end server