local net = require('net')

local RES_BUFFER = Buffer:new("HTTP/1.0 200 OK\r\nConnection: Close\r\nContent-Type: text/html\r\nContent-Length: 14\r\n\r\nHello World\n\r\n")

local client__on_read = function(c, nread, buf)
  -- we do not have to worry about closing our client here...
  -- that is taken care of automatically! for events, register with client:on_close()
  c:write( RES_BUFFER )
  c:close()
end

net.createServer("0.0.0.0", 8080, function(s, err)
  local client = s:accept()
  client:read_start(client__on_read)
end)