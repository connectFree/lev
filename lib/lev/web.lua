local net = require('net')
local table = require('table')
local string = require('string')
local osDate = require('os').date
local newHttpParser = require('http_parser').new
local parseUrl = require('http_parser').parseUrl


local web = {}

local STATUS_CODES = {
  [100] = 'Continue',
  [101] = 'Switching Protocols',
  [102] = 'Processing',                 -- RFC 2518, obsoleted by RFC 4918
  [200] = 'OK',
  [201] = 'Created',
  [202] = 'Accepted',
  [203] = 'Non-Authoritative Information',
  [204] = 'No Content',
  [205] = 'Reset Content',
  [206] = 'Partial Content',
  [207] = 'Multi-Status',               -- RFC 4918
  [300] = 'Multiple Choices',
  [301] = 'Moved Permanently',
  [302] = 'Moved Temporarily',
  [303] = 'See Other',
  [304] = 'Not Modified',
  [305] = 'Use Proxy',
  [307] = 'Temporary Redirect',
  [400] = 'Bad Request',
  [401] = 'Unauthorized',
  [402] = 'Payment Required',
  [403] = 'Forbidden',
  [404] = 'Not Found',
  [405] = 'Method Not Allowed',
  [406] = 'Not Acceptable',
  [407] = 'Proxy Authentication Required',
  [408] = 'Request Time-out',
  [409] = 'Conflict',
  [410] = 'Gone',
  [411] = 'Length Required',
  [412] = 'Precondition Failed',
  [413] = 'Request Entity Too Large',
  [414] = 'Request-URI Too Large',
  [415] = 'Unsupported Media Type',
  [416] = 'Requested Range Not Satisfiable',
  [417] = 'Expectation Failed',
  [418] = 'I\'m a teapot',              -- RFC 2324
  [422] = 'Unprocessable Entity',       -- RFC 4918
  [423] = 'Locked',                     -- RFC 4918
  [424] = 'Failed Dependency',          -- RFC 4918
  [425] = 'Unordered Collection',       -- RFC 4918
  [426] = 'Upgrade Required',           -- RFC 2817
  [500] = 'Internal Server Error',
  [501] = 'Not Implemented',
  [502] = 'Bad Gateway',
  [503] = 'Service Unavailable',
  [504] = 'Gateway Time-out',
  [505] = 'HTTP Version not supported',
  [506] = 'Variant Also Negotiates',    -- RFC 2295
  [507] = 'Insufficient Storage',       -- RFC 4918
  [509] = 'Bandwidth Limit Exceeded',
  [510] = 'Not Extended'                -- RFC 2774
}


function web.createServer(host, port, onRequest, onData)
  if not port then error("port is a required parameter") end
  if not onRequest then error("onRequest is a required parameter") end
  net.createServer(host or "0.0.0.0", port, function(s, err)
    local client = s:accept()

--- X:S PARSER
    local currentField, headers, url, request
    local parser = newHttpParser("request", {
      onMessageBegin = function ()
        headers = {}
        request = {}
      end,
      onUrl = function (value)
        url = parseUrl(value)
      end,
      onHeaderField = function (field)
        currentField = field
      end,
      onHeaderValue = function (value)
        headers[currentField:lower()] = value
      end,
      onHeadersComplete = function (info)
        request.url = url
        request.headers = headers
        request.parser = parser

        response = {
          writeHead = function(statusCode, headers)
            local reasonPhrase = STATUS_CODES[statusCode] or 'unknown'
            if not reasonPhrase then error("Invalid response code " .. tostring(statusCode)) end

            local headers_len = 64

            local _headers = {["server"]="lev", ["connection"] = "Close", ["content-type"] = "text/html"}

            _headers['date'] = osDate("!%a, %d %b %Y %H:%M:%S GMT")

            if headers then
              for k,v in pairs(headers) do _headers[k:lower()] = v end
            end

            local head = { string.format("HTTP/1.1 %s %s\r\n", statusCode, reasonPhrase) }

            for key, value in pairs(_headers) do
              headers_len = headers_len + #key + #value
              table.insert(head, string.format("%s: %s\r\n", key, value))
            end
            table.insert(head, "\r\n")
            headers_len = headers_len + 2

            local out_buffer = Buffer:new(headers_len)

            local at = 1
            for i=1,#head do
              local part_len = #head[i]
              out_buffer[ at ] = head[i]
              at = at + part_len
            end

            client:write( out_buffer:slice(1,at-1) )
          end --X:E res.writeHead
          ,write = function(chunk)
            client:write( chunk )
          end --X:E res.write
          ,reinit = function()
            parser:reinitialize("request")
          end
          ,fin = function(chunk)
            client:write( chunk )
            client:close()
          end --X:E res.fin
        } --X:E response
        onRequest(client, request, response)
      end,
      onBody = function (chunk)
        if not onData then return end
        onData(client, chunk)
      end,
      onMessageComplete = function ()
      end
    })
--- X:E PARSER

    client:read_start(function(c, nread, chunk)
      if nread == 0 then return end
      local nparsed = parser:execute(chunk, 0, nread)
    end)
    client:on_close(function(c)
      parser:finish()
    end)

  end) --X:E Server
  return server
end

return web