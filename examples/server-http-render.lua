local fs = lev.fs
local render = require('render')
local cS = require('web').createServer

local count = 0

local compiled_template

local client__onRequest = function(c, req, res)
  c:bottle()

  local err
  if not compiled_template then
    err, compiled_template = render.file("test-render.html", __path__)
    if err then
      res.writeHead(404)
      return res.fin("Could not find template!")
    end
  end

  count = count + 1

  res.writeHead(200)
  res.fin(compiled_template({hello=count}))
end

cS(nil, 8080, client__onRequest, nil) --X:E createServer

p("http server listening on port 8080!")
