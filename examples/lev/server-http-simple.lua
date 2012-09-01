local levbase = require('levbase')

local server = levbase.tcp.new()
server:bind("0.0.0.0", 8080)
server:listen(function(s, err)
  local client = s:accept()
  client:on_close(function(c)
    client = nil
  end)
  client:read_start(function(c, nread, buf)
    -- we do not have to worry about closing our client here...
    -- that is taken care of automatically! for events, register with client:on_close()
    c:write("HTTP/1.0 200 OK\r\nConnection: Close\r\nContent-Type: text/html\r\nContent-Length: 14\r\n\r\nHello World\n\r\n")
    c:close()
  end)
end)