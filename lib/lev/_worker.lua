--[[

Copyright The lev Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

--]]

p(_G.WorkerID, "WORKER START")

-- Bootstrap require system
local lev = require('lev')
local env = require('env')
local utils = require('utils')
local table = require('table')
local pipe = require('lev').pipe
local mp = require('lev').mpack

local ipc_channel = false
local ipc_connected = false

_G.mbox = {}

local queue = {}
local callbacks = {}

local req_id = 1

local ipc__process_packet = function(c, packet, packet_len, buf, fd, type)
  if not packet['p'] then packet['p'] = {} end
  if fd then
    packet['p']['_cmsg'] = {fd=fd, type=type}
  end
  local cmd = packet.cmd:toString()
  if cmd == "broadcast" then
    p(_G.WorkerID, packet)
  elseif cmd == "reply" then
    local idstr = packet['id']:toString()
    -- make sure we have a callback
    if not callbacks[ idstr ] then return end
    callbacks[ idstr ]( packet['p'] ) -- give packet to callback
    callbacks[ idstr ] = nil -- clear callback
  end
end -- X:E ipc__process_packet


local ipc__on_read = function(c, nread, buf, fd, type)
  if buf then
    local at = 0
    local len, packet = mp.unpack( buf )
    at = at + len
    ipc__process_packet(c, packet, len, buf, fd, type)
    while at < nread do
      buf = buf:slice(len+1)
      len, packet = mp.unpack( buf )
      at = at + len
      ipc__process_packet(c, packet, len, buf, fd, type)
    end    
  end
end --ipc__on_read

local ipc_connect = function()
  if ipc_channel then return end
  ipc_channel = pipe.new(1)
  ipc_channel:connect( env.get("LEV_IPC_FILENAME"), function(c, err)
    if err then
      return
    end
    c:read_start(ipc__on_read)
    c:write(mp.pack({cmd="hello", id=_G.WorkerID}))
    local qi = false
    repeat
      qi = table.remove(queue, 1)
      --p("queueitem", qi)
      if qi then c:write(mp.pack(qi)) end
    until not qi
    ipc_connected = true
  end)
end

local send_msg = function( pkt )
  if ipc_connected then
    ipc_channel:write(mp.pack(pkt))
  else
    table.insert(queue, pkt)
    ipc_connect()
  end
  req_id = req_id + 1
end

_G.mbox.sendBroadcast = function( package, key )
  send_msg( {cmd='broadcast', id=tostring(req_id), from=_G.WorkerID, key=key, p=package} )
end -- X:E _G.sendBroadcast

_G.mbox.toMaster = function( cmd, package, callback )
  local idstr = tostring(req_id)
  if callback then
    callbacks[ idstr ] = callback
  end
  send_msg( {cmd=cmd, id=idstr, from=_G.WorkerID, p=package} )
end

p(_G.WorkerID, "WORKER END")