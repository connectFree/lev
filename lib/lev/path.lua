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

local lev = require('lev')
local string = require('string')
local table = require('table')

local path = {}

if lev.os_type() == 'win32' then
  path.sep = '\\'
else
  path.sep = '/'
end

local SEP_BYTE = string.byte(path.sep)

--[[
TODO: implement

-- Split a filename into [root, dir, basename], unix version
-- 'root' is just a slash, or nothing.
function Path:_splitPath(filename)
  local root, dir, basename
  local i, j = filename:find('[^' .. self.sep .. ']*$')
  if filename:sub(1, 1) == self.sep then
    root = self.root
    dir = filename:sub(2, i - 1)
  else
    root = ''
    dir = filename:sub(1, i - 1)
  end
  local basename = filename:sub(i, j)
  return root, dir, basename, ext
end

-- Modifies an array of path parts in place by interpreting '.' and '..' segments
function Path:_normalizeArray(parts)
  local skip = 0
  for i = #parts, 1, -1 do
    local part = parts[i]
    if part == '.' then
      table.remove(parts, i)
    elseif part == '..' then
      table.remove(parts, i)
      skip = skip + 1
    elseif skip > 0 then
      table.remove(parts, i)
      skip = skip - 1
    end
  end
end

function Path:resolve(root, filepath)
  if filepath:sub(1, self.root:len()) == self.root then
    return self:normalize(filepath)
  end
  return self:join(root, filepath)
end
--]]

function path.normalize(filepath)
--[[
TODO: implement
  local is_absolute = filepath:sub(1, 1) == self.sep
  local trailing_slash = filepath:sub(#filepath) == self.sep

  local parts = {}
  for part in filepath:gmatch('[^' .. self.sep .. ']+') do
    parts[#parts + 1] = part
  end
  self:_normalizeArray(parts)
  filepath = table.concat(parts, self.sep)

  if #filepath == 0 then
    if is_absolute then
      return self.sep
    end
    return '.'
  end
  if trailing_slash then
    filepath = filepath .. self.sep
  end
  if is_absolute then
    filepath = self.sep .. filepath
  end
--]]
  return filepath
end

function path.join(...)
  return path.normalize(table.concat({...}, path.sep))
end

local function rfind_byte(haystack, needle_byte)
  for i = #haystack, 1, -1 do
    if string.byte(haystack, i) == needle_byte then
      return i
    end
  end
  return nil
end

function path.dirname(filepath)
  local i = rfind_byte(filepath, SEP_BYTE)
  return i and string.sub(filepath, 1, i - 1) or filepath
end

-- TODO: move this to string util module, and create a native function.
local function ends_with(str, suffix)
  return string.sub(str, #str - #suffix + 1) == suffix
end

function path.basename(filepath, ext)
  local i = rfind_byte(filepath, SEP_BYTE)
  if i then
    if ext and ends_with(filepath, ext) then
      return string.sub(filepath, i + 1, #filepath - #ext)
    else
      return string.sub(filepath, i + 1)
    end
  else
    return filepath
  end
end

local DOT_BYTE = string.byte('.')

function path.extname(filepath)
  local i = rfind_byte(filepath, DOT_BYTE)
  return i and string.sub(filepath, i) or ''
end

return path
