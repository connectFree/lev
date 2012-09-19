-- WebChat example for lev by @kristate / @kristopher
-- Based on NodeChat by @ry
-- spin-up it up: ./build/lev ./examples/webchat/webchat.lua
-- connect via a brower: http://127.0.0.1:9000/

local fs = lev.fs
local os = require('os')
local _m = require('math')
local _t = require('table')
local _s = require('string')
local qs = require('querystring')
local cS = require('web').createServer

local MESSAGE_BACKLOG = 200
local SESSION_TIMEOUT = 60

local sessions = {}
local mem = {rss = 1} --X:FUTURE

local starttime = os.time()

local channel
channel = {
   messages = {}
  ,callbacks = {}
  ,appendMessage = function(nick, kind, text)
    local m = { nick = nick
               ,type = kind -- "msg", "join", "part"
               ,text = text
               ,timestamp = os.time()
              }
    if kind == "msg" then
      p("msg", "<" .. nick .. "> " .. text)
    elseif kind == "join" then
      p("join", nick)
    elseif kind == "part" then
      p("part", nick)
    end

    _t.insert(channel.messages, m)
    local cb
    while #channel.callbacks > 0 do
      cb = _t.remove(channel.callbacks, 1)
      cb.callback( {m} )
    end

    while #channel.messages > MESSAGE_BACKLOG do
      _t.remove(channel.messages, 1)
    end

   end -- X:E appendMessage
  ,query = function (since, callback)
    local matching = {}
    for noop,message in pairs(channel.messages) do
      if message.timestamp > since then
        _t.insert(matching, message)
      end
    end

    if #matching ~= 0 then
      callback( matching )
    else
      _t.insert(channel.callbacks, { timestamp = os.time(), callback = callback })
    end
  end -- X:E query

} -- X:E channel

function createSession (nick)
  if #nick > 50 then return nil end
  if not _s.gmatch(nick, '[^\\w_\\-^!]') then return nil end

  for noop,session in pairs(sessions) do
    if session and session.nick == nick then return nil end
  end

  local session
  session = { 
     nick = nick 
    ,id = tostring(_m.floor(_m.random()*99999999999))
    ,timestamp = os.time()
    ,poke = function ()
      session.timestamp = os.time()
     end
    ,destroy = function ()
      channel.appendMessage(session.nick, "part")
      sessions[session.id] = nil
    end
  }
  sessions[session.id] = session
  return session
end

local path_handlers = {
  
  ["/who"] = function(c, req, res)
    local nicks = {}
    for noop,session in pairs(sessions) do
      _t.insert(nicks, session.nick)
    end
    res.simpleJSON(200, { nicks = nicks
                          ,rss = mem.rss
                        })
   end -- X:E /who
  ,["/join"] = function(c, req, res)
    local qparsed = qs.parse(req.url.query)
    if not qparsed.nick or #qparsed.nick == 0 then
      res.simpleJSON(400, {error = "Bad nick."})
      return
    end

    local session = createSession( qparsed.nick )
    if not session then
      res.simpleJSON(400, {error = "Nick in use"})
      return
    end

    channel.appendMessage(session.nick, "join")
    res.simpleJSON(200, { id = session.id
                        , nick = session.nick
                        , rss = mem.rss
                        , starttime = starttime
                        })

   end -- X:E /join
  ,["/part"] = function(c, req, res)
    local session, id
    if id and sessions[id] then
      session = sessions[id]
      session.destroy()
    end
    res.simpleJSON(200, { rss = mem.rss })
   end -- X:E /part
  ,["/recv"] = function(c, req, res)
    local qparsed = qs.parse(req.url.query)
    if not qparsed.since then --check for since here
      res.simpleJSON(400, { error = "Must supply since parameter" })
      return
    end

    local session
    if qparsed.id and sessions[qparsed.id] then
      session = sessions[qparsed.id]
      session.poke()
    end

    channel.query(tonumber(qparsed.since), function (messages)
      if session then session.poke() end
      res.simpleJSON(200, { messages = messages, rss = mem.rss })
    end)

   end -- X:E /recv
  ,["/send"] = function(c, req, res)
    local qparsed = qs.parse(req.url.query)
    local session = sessions[ qparsed.id ]
    if not session or not qparsed.text then
      res.simpleJSON(400, { error = "No such session id" })
      return
    end
    
    session.poke()

    channel.appendMessage(session.nick, "msg", qparsed.text)
    res.simpleJSON(200, { rss = mem.rss })

   end -- X:E /send
} --X:E path_handlers

-- clear old callbacks
-- they can hang around for at most 30 seconds.
local timer_callbacks = lev.timer.new()
timer_callbacks:start(function(t, status)
  local now = os.time()
  local cb
  while #channel.callbacks > 0 and now - channel.callbacks[1].timestamp > 30 do
    cb = _t.remove(channel.callbacks, 1)
    if cb then cb.callback( {} ) end
  end
end, 3000, 3000)

-- interval to kill off old sessions
local timer_sessions = lev.timer.new()
timer_sessions:start(function(t, status)
  local now = os.time()
  for noop,session in pairs(sessions) do
    if now - session.timestamp > SESSION_TIMEOUT then
      session.destroy()
    end
  end
end, 1000, 1000)

local STATICS = {
   ["/"] = {name = "/index.html", type = "text/html"}
  ,["/style.css"] = {name = "/style.css", type = "text/css"}
  ,["/client.js"] = {name = "/client.js", type = "application/javascript"}
  ,["/jquery.min.js"] = {name = "/jquery.min.js", type = "application/javascript"}
}

local client__onRequest = function(c, req, res)
  local route = path_handlers[ req.url.path ]
  local static = STATICS[ req.url.path ]
  if route then
    route( c, req, res )
  elseif static then -- oh, you want some static data!
    if not static.buf then
      local err
      err, static.buf = fs.readFile(__path__ .. static.name)
      if err then
        res.writeHead(500)
        res.fin( "File Error!" )
        return
      end
    end
    res.writeHead(200, {["Content-Type"] = static.type, ["Content-Length"] = tostring(#static.buf)})
    res.fin( static.buf )
  else
    res.writeHead(404)
    res.fin( "Bad Route!" )
  end
end

cS(nil, 9000, client__onRequest) --X:E createServer

p("http server listening on port 9000!")