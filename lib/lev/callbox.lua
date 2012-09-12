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

local table = require('table')

local callbox = {}
callbox.meta = {__index = callbox}

function callbox:create()
  local meta = rawget(self, "meta")
  if not meta then error("Cannot inherit from instance object") end
  return setmetatable({}, meta)
end

function callbox:new(...)
  local obj = self:create()
--  if type(obj.initialize) == "function" then
--    obj:initialize(...)
--  end
  return obj
end

-- By default, and error events that are not listened for should throw errors
function callbox:missingHandlerType(name, ...)
  if name == "error" then
    local args = {...}
    --error(tostring(args[1]))
    -- we define catchall error handler
    if self ~= process then
      -- if process has an error handler
      local handlers = rawget(process, "handlers")
      if handlers and handlers["error"] then
        -- delegate to process error handler
        process:call("error", ..., self)
      else
        debug("UNHANDLED ERROR", ...)
        error("UNHANDLED ERROR. Define process:on('error', handler) to catch such errors")
      end
    else
      debug("UNHANDLED ERROR", ...)
      error("UNHANDLED ERROR. Define process:on('error', handler) to catch such errors")
    end
  end
end

-- Same as `callbox:on` except it de-registers itself after the first event.
function callbox:once(name, callback)
  local function wrapped(...)
    self:removeListener(name, wrapped)
    callback(...)
  end
  self:on(name, wrapped)
  return self
end

-- Adds an event listener (`callback`) for the named event `name`.
function callbox:on(name, callback)
  local handlers = rawget(self, "handlers")
  if not handlers then
    handlers = {}
    rawset(self, "handlers", handlers)
  end
  local handlers_for_type = rawget(handlers, name)
  if not handlers_for_type then
    if self.addHandlerType then
      self:addHandlerType(name)
    end
    handlers_for_type = {}
    rawset(handlers, name, handlers_for_type)
  end
  table.insert(handlers_for_type, callback)
  return self
end

-- callbox a named event to all listeners with optional data argument(s).
function callbox:call(name, ...)
  local handlers = rawget(self, "handlers")
  if not handlers then
    self:missingHandlerType(name, ...)
    return
  end
  local handlers_for_type = rawget(handlers, name)
  if not handlers_for_type then
    self:missingHandlerType(name, ...)
    return
  end
  for i, callback in ipairs(handlers_for_type) do
    callback(...)
  end
  for i = #handlers_for_type, 1, -1 do
    if not handlers_for_type[i] then
      table.remove(handlers_for_type, i)
    end
  end
  return self
end

-- Remove a listener so that it no longer catches events.
function callbox:removeListener(name, callback)
  local handlers = rawget(self, "handlers")
  if not handlers then return end
  local handlers_for_type = rawget(handlers, name)
  if not handlers_for_type then return end
  for i = #handlers_for_type, 1, -1 do
    if handlers_for_type[i] == callback or callback == nil then
      handlers_for_type[i] = nil
    end
  end
end

return callbox

