local json = lev.json
local net = require('net')
local table = require('table')
local string = require('string')
local osDate = require('os').date
local newHttpParser = require('http_parser').new
local parseUrl = require('http_parser').parseUrl

local CRLF = '\r\n'
local CHUNK_BUFFER = Buffer:new(1024)
local CHUNKED_NO_TRAILER = Buffer:new('0' .. CRLF .. CRLF)

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
    local currentField, headers, url, request, response, parser
    parser = newHttpParser("request", {
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
        request.upgrade = info.upgrade

        response = {
           headers_sent = false
          ,chunked_encoding = true
          ,writeHead = function(statusCode, headers)
            if response.headers_sent then error("headers already sent") end
            local reasonPhrase = STATUS_CODES[statusCode] or 'unknown'
            if not reasonPhrase then error("Invalid response code " .. tostring(statusCode)) end

            local response_buffer = Buffer:new( 1024 )
            local response_buffer_len = 0

            response_buffer[1] = 'HTTP/1.1 '
            response_buffer_len = response_buffer_len + 9

            local line
            line = string.format("%s %s\r\n", statusCode, reasonPhrase)
            response_buffer[response_buffer_len+1] = line
            response_buffer_len = response_buffer_len + #line

            local _headers = {["server"]="lev", ["connection"] = "close", ["content-type"] = "text/html"}

            --get date header from nginx-esque time slot cache
            _headers['date'] = lev.timeHTTP()

            if headers then
              for k,v in pairs(headers) do _headers[k:lower()] = v end
            end

            if _headers['content-length'] then
              response.chunked_encoding = false
            else
              _headers['transfer-encoding'] = 'chunked'
            end

            for key, value in pairs(_headers) do
              line = string.format("%s: %s\r\n", key, value)
              response_buffer[response_buffer_len+1] = line
              response_buffer_len = response_buffer_len + #line
            end

            -- CRLF into body
            response_buffer[response_buffer_len+1] = CRLF
            response_buffer_len = response_buffer_len + 2

            client:write( response_buffer:slice(1, response_buffer_len) )
            response.headers_sent = true
            response_buffer = nil

          end --X:E res.writeHead
          ,write = function(chunk)
            if not response.headers_sent then response.writeHead(200) end
            if response.chunked_encoding then
              local chunk_buffer_to = CHUNK_BUFFER:writeHexLower(#chunk, 1)
              CHUNK_BUFFER[chunk_buffer_to+1] = CRLF
              CHUNK_BUFFER[chunk_buffer_to+3] = chunk
              chunk_buffer_to = chunk_buffer_to+3+#chunk
              CHUNK_BUFFER[chunk_buffer_to] = CRLF
              client:write( CHUNK_BUFFER:slice(1, chunk_buffer_to+1) )
            else
              client:write( chunk )
            end
            
          end --X:E res.write
          ,reinit = function()
            request.parser:reinitialize("request")
          end
          ,fin = function(chunk)
            if chunk then 
              if response.chunked_encoding then
                chunk[#chunk+1] = CHUNKED_NO_TRAILER
              end
              response.write( chunk )
            elseif response.chunked_encoding then
              client:write( CHUNKED_NO_TRAILER )
            end
            if request.headers.connection and request.headers.connection:lower() == "keep-alive" then
              response.reinit()
            else
              client:close()
            end
          end --X:E res.fin
          ,simpleJSON = function(status, chunk)
            local out_buf = Buffer:new( json.encode( chunk ) or '' )
            response.writeHead(status, {["content-type"] = 'text/json', ["Content-Length"] = tostring(#out_buf)})
            response.fin( out_buf )
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
      if request and request.upgrade and onData then
        onData(c, chunk)
        return
      end
      local nparsed = parser:execute(chunk, 0, nread)
    end)
    client:on_close(function(c)
      parser:finish()
    end)

  end) --X:E Server
  return server
end

return web