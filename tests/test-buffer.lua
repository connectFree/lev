--[[

Copyright 2012 The Luvit Authors. All Rights Reserved.

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

local lev = require("lev")

if lev.process.platform == "win32" then
  print("buffer is broken on win32, need to not ffi into malloc")
  return
end

local exports = {}

exports['dummy_test'] = function(test)
  test.equal(true, true)
  test.done()
end

return exports

--[[
local Buffer = require('cbuffer')


--test union slices/concat
local ubuf1 = Buffer:new("a");
local ubuf2 = Buffer:new("b");
local ubuf3 = Buffer:new("c");
local ubuf4 = Buffer:new("de");
local ubuf5 = Buffer:new("fg");
local ubuf6 = Buffer:new("hij");
local ubuf7 = Buffer:new("lmnop");

local union_buf = ubuf1 .. ubuf2
union_buf = union_buf .. ubuf3
union_buf = union_buf .. ubuf4
union_buf = union_buf .. ubuf5
union_buf = union_buf .. ubuf6
union_buf = union_buf .. ubuf7

-- test buffer concat with lstrings
union_buf = union_buf .. "qrstuv"

assert(tostring(union_buf) == "abcdefghijlmnopqrstuv")


local buf = Buffer:new(4)

buf[1] = 0xFB
buf[2] = 0x04
buf[3] = 0x23
buf[4] = 0x42

assert(buf:readUInt8(1) == 0xFB)
assert(buf:readUInt8(2) == 0x04)
assert(buf:readUInt8(3) == 0x23)
assert(buf:readUInt8(4) == 0x42)
assert(buf:readInt8(1) == -0x05)
assert(buf:readInt8(2) == 0x04)
assert(buf:readInt8(3) == 0x23)
assert(buf:readInt8(4) == 0x42)
assert(buf:readUInt16BE(1) == 0xFB04)
assert(buf:readUInt16LE(1) == 0x04FB)
assert(buf:readUInt16BE(2) == 0x0423)
assert(buf:readUInt16LE(2) == 0x2304)
assert(buf:readUInt16BE(3) == 0x2342)
assert(buf:readUInt16LE(3) == 0x4223)
assert(buf:readUInt32BE(1) == 0xFB042342)
assert(buf:readUInt32LE(1) == 0x422304FB)
assert(buf:readInt32BE(1) == -0x04FBDCBE)
assert(buf:readInt32LE(1) == 0x422304FB)

-- test Buffer:slice
local sliced_buf = buf:slice(3,4)
assert(sliced_buf:readUInt16BE(1) == 0x2342)
local sliced_sliced_buf = sliced_buf:slice(1,1)
assert(sliced_sliced_buf:readUInt8(1) == 0x23)

local buf2 = Buffer:new('abcdefghij')
assert(tostring(buf2) == 'abcdefghij')
assert(buf2:toString(1, 2) == 'ab')
assert(buf2:toString(2, 3) == 'bc')
assert(buf2:toString(3) == 'cdefghij')
assert(buf2:toString() == 'abcdefghij')

-- test Buffer:find
assert(buf2:toString(buf2:find("fgh")) == "fghij")
assert(buf2:find("kristopher") == nil)
assert(buf2:find("") == nil)

-- test Buffer:upUntil
assert(buf2:upUntil("") == '')
assert(buf2:upUntil("d") == 'abc')
assert(buf2:upUntil("d", 4) == '')
assert(buf2:upUntil("d", 5) == 'efghij')

-- test Buffer.isBuffer
assert(Buffer.isBuffer(buf2) == true)
assert(Buffer.isBuffer("buf") == false)

-- test Buffer.length
assert(buf2.length == 10)

-- test Buffer:inspect
assert(buf:inspect() == "<Buffer FB 04 23 42 >")

-- test Buffer.meta:__concat
local concat_buf = buf .. buf2
assert( concat_buf:inspect() == "<Buffer FB 04 23 42 61 62 63 64 65 66 67 68 69 6A >")
-- initial memory for buf and buf2 should not be changed or altered
assert(buf:inspect() == "<Buffer FB 04 23 42 >")
assert(buf2:inspect() == "<Buffer 61 62 63 64 65 66 67 68 69 6A >")

-- test Buffer.fill
concat_buf:fill("", 4, 4)
assert( concat_buf:inspect() == "<Buffer FB 04 23 00 61 62 63 64 65 66 67 68 69 6A >")
concat_buf:fill("", 4, 8)
assert( concat_buf:inspect() == "<Buffer FB 04 23 00 00 00 00 00 65 66 67 68 69 6A >")
concat_buf:fill(0x05, 4, 8)
assert( concat_buf:inspect() == "<Buffer FB 04 23 05 05 05 05 05 65 66 67 68 69 6A >")
concat_buf:fill(0x42, 1)
assert( concat_buf:inspect() == "<Buffer 42 42 42 42 42 42 42 42 42 42 42 42 42 42 >")
concat_buf:fill("\0", 1)
assert( concat_buf:inspect() == "<Buffer 00 00 00 00 00 00 00 00 00 00 00 00 00 00 >")

-- test bitwise write
local writebuf = Buffer:new(4)
writebuf:writeUInt8(0xFB, 1)
writebuf:writeUInt8(0x04, 2)
writebuf:writeUInt8(0x23, 3)
writebuf:writeUInt8(0x42, 4)
writebuf:writeInt8(-0x05, 1)
writebuf:writeInt8(0x04, 2)
writebuf:writeInt8(0x23, 3)
writebuf:writeInt8(0x42, 4)
writebuf:writeUInt16BE(0xFB04, 1)
writebuf:writeUInt16LE(0x04FB, 1)
writebuf:writeUInt16BE(0x0423, 2)
writebuf:writeUInt16LE(0x2304, 2)
writebuf:writeUInt16BE(0x2342, 3)
writebuf:writeUInt16LE(0x4223, 3)
writebuf:writeUInt32BE(0xFB042342, 1)
writebuf:writeUInt32LE(0x422304FB, 1)
writebuf:writeInt32BE(-0x04FBDCBE, 1)
writebuf:writeInt32LE(0x422304FB, 1)
assert( writebuf:inspect() == "<Buffer FB 04 23 42 >")

-- test GC
concat_buf = nil
buf2 = nil
buf = nil
writebuf = nil
sliced_buf = nil
sliced_sliced_buf = nil

ubuf1 = nil
ubuf2 = nil
ubuf3 = nil
ubuf4 = nil
ubuf5 = nil
ubuf6 = nil
ubuf7 = nil
union_buf = nil

collectgarbage()
--]]
