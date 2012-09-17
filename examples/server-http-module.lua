local cS = require('web').createServer

local client__onRequest = function(c, req, res)
  res.writeHead(200)
  res.fin("Hello World!\n")
end

local client__onData = function(c, chunk)
  p("client__onData", c, chunk)
end

cS(nil, 8080, client__onRequest, client__onData) --X:E createServer

p("http server listening on port 8080!")
