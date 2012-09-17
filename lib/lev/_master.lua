--[[

Copyright 2012 The lev Authors. All Rights Reserved.

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

-- Bootstrap require system
local utils = require('utils')

local lev = require('lev')
local pipe = lev.pipe
local mp = lev.mpack
local net_tcp = lev.tcp
local process = lev.process

local bind_pool = {}
local worker_pool = {}
local worker_pool_by_id = {}

local client__process_packet = function(c, packet, packet_len, buf)
  --p(c, packet, packet_len, buf)
  local cmd = packet.cmd:toString()
  if cmd == "hello" then
    worker_pool[ c ]["id"] = packet.id
    worker_pool_by_id[ packet.id ] = c
  elseif cmd == "bind" then
    local bind_id = packet['p']['type']:toString() .. '|' .. packet['p']['address']:toString() .. '|' .. packet['p']['port']
    if not bind_pool[ bind_id ] then
      if packet['p']['type']:toString() == 'tcp' then
        bind_pool[ bind_id ] = net_tcp.new()
        bind_pool[ bind_id ]:bind(packet['p']['address']:toString(), packet['p']['port'])
      else
        p("INVALID TYPE FOR BIND")
        return
      end
    end

    local fd = bind_pool[ bind_id ]:fd_get()
    --send FD as extra argument for SCM_RIGHTS
    c:write(mp.pack( {cmd='reply', id=packet['id'], p={}} ), fd)

  elseif cmd == "broadcast" then
    for wc,noop in pairs(worker_pool) do
      if worker_pool[wc]['id'] and c ~= wc then
        wc:write(buf:slice(1,packet_len)) --we can send the exact same broadcast
      end
    end
  end
end -- X:E client__process_packet

local client__on_read = function(c, nread, buf)
  if buf then
    local at = 0
    local len, packet = mp.unpack( buf )
    at = at + len
    client__process_packet(c, packet, len, buf)
    while at < nread do
      buf = buf:slice(len+1)
      len, packet = mp.unpack( buf )
      at = at + len
      client__process_packet(c, packet, len, buf)
    end    
  end
end -- X:E client__on_read

local client__on_close = function(c)
  if worker_pool[ c ]["id"] then
    worker_pool_by_id[ worker_pool[ c ]["id"] ] = nil
  end
  worker_pool[ c ] = nil
end

local master_ipc = pipe.new(1)
master_ipc:bind( process.getenv("LEV_IPC_FILENAME") )
master_ipc:listen(function(s, err)
  local client = s:accept()
  client:on_close( client__on_close )
  client:read_start(client__on_read)
  worker_pool[ client ] = {}
end)
