local levbase = require('levbase')

-- use print till p() is ported
local p = print

local client = levbase.tcp.new()
client:connect("0.0.0.0", 8080, function(...)
  p("connected")
  buf = "hello"
  client:write(buf)
  client:read_start(function(c, nread, buf)
    if buf then
      p("received", buf)
    end
  end)
end)
