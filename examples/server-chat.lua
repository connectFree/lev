
-- MultiCore Chat example for lev by @kristate
-- uses mbox.* to transfer chat buffers between cores
-- spin-up two cores -> ./build/lev -c 2 ./examples/lev/server-chat.lua
-- connect via nc -> nc 127.0.0.1 8080

local table = require('table')
local net = require('net')

local cli_pool = {}

mbox.recvBroadcast("incoming_chat", function(packet)
  for wc,noop in pairs(cli_pool) do
    if cli_pool[wc]['nickname'] then
      wc:write( packet['p']['msg'] )
    end
  end
end) -- X:E mbox.recvBroadcast

local broadcastLocal = function(from_cli, send_buf)
  for wc,noop in pairs(cli_pool) do
    if cli_pool[wc]['nickname'] and from_cli ~= wc then
      wc:write( send_buf )
    end
  end
end

local client__on_read = function(c, nread, buf)
  local just_joined = false
  if buf then
    if not cli_pool[c]['nickname'] then
      local nickname = buf:upUntil('\n')
      if not nickname or #nickname == 0 then
        c:close()
      end
      cli_pool[c]['nickname'] = Buffer:new( nickname .. ": " )
      just_joined = true
    end

    local send_buf = nil

    if just_joined then
      send_buf = cli_pool[c]['nickname'] .. (" has joined on worker #" .. _G.WorkerID .. "\n")
    else
      send_buf = cli_pool[c]['nickname'] .. buf
    end

    -- first, broadcast message to other nodes
    mbox.sendBroadcast({msg=send_buf}, "incoming_chat")
    -- finally, broadcast to locally connected nodes
    broadcastLocal(c, send_buf)
  end
end

local client__on_close = function(c)
  p(_G.WorkerID, "client closed", c)
  local signoff_buf = cli_pool[c]['nickname'] .. Buffer:new("Has Signed Off!\n")
  broadcastLocal(c, signoff_buf)
  mbox.sendBroadcast({msg=signoff_buf}, "incoming_chat")
  cli_pool[c] = nil
end

net.createServer("0.0.0.0", 8080, function(s, err)
  local client = s:accept()
  p(_G.WorkerID, "accepted client", client)
  client:on_close(client__on_close)
  client:read_start(client__on_read)
  client:write("please enter your nickname:\n")
  -- add client to pool
  cli_pool[client] = {}
end)


