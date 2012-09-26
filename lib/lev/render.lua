--[[

Template Rendering Engine `render` for lev

Copyright 2012 connectFree k.k. and the lev authors. All Rights Reserved.

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


local fs = lev.fs
local _s = require('string')
local _t = require('table')

local util = {}
local export = {}

util.escape = function (s)
  if s == nil then return '' end

  local esc, i = s:gsub('&', '&amp;'):gsub('<', '&lt;'):gsub('>', '&gt;')
  return esc
end

-- Used in template parsing to figure out what each {} does.
local VIEW_ACTIONS = {
   ['{%'] = function(code)
    return code
   end
  ,['{{'] = function(code)
    return ('_result[#_result+1] = %s'):format(code)
   end
  ,['{('] = function(code)
    return ([[ 
      if not _children[%s] then
        _children[%s] = _util.load(%s)
      end
      _result[#_result+1] = _children[%s](getfenv())
    ]]):format(code, code, code, code)
   end
  ,['{<'] = function(code)
    return ('_result[#_result+1] =  _util.escape(%s)'):format(code)
  end
}

-- Takes a view template and optional name (usually a file) and 
-- returns a function you can call with a table to render the view.
util.compile_view = function(tmpl, name, directory)
  local tmpl = tmpl .. '{}'
  local code = {'local _result, _children = {}, {}\nlocal _t = require(\'table\')\n'}

  for text, block in _s.gmatch(tmpl, "([^{]-)(%b{})") do
    local act = VIEW_ACTIONS[block:sub(1,2)]
    local output = text

    if act then
      code[#code+1] =  '_result[#_result+1] = [==[' .. text .. ']==]'
      code[#code+1] = act(block:sub(3,-3))
    elseif #block > 2 then
      code[#code+1] = '_result[#_result+1] = [==[' .. text .. block .. ']==]'
    else
      code[#code+1] =  '_result[#_result+1] = [==[' .. text .. ']==]'
    end
  end

  code[#code+1] = 'return _t.concat(_result)'

  code = _t.concat(code, '\n')
  local func, err = loadstring(code, name)

  if err then
    assert(func, err)
  end

  return function(context)
    assert(context, "You must always pass in a table for context.")
    context._util = {
       load = function (name)
        local err, compiled = util.file(name, directory)
        return compiled
       end
      ,escape = util.escape
    }
    setmetatable(context, {__index=_G})
    setfenv(func, context)
    return func()
  end
end

util.file = function(name, directory)
  local filename
  if directory then
    filename = ('%s/%s'):format(directory, name)
  else
    filename = name
  end
  local err, buf = fs.readFile(filename)
  if err then return err, nil end
  return nil, util.compile_view(buf:s(), filename, directory)
end

return {
  file = util.file
}