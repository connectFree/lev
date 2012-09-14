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

local lev = require("lev")

local exports = {}

exports['lev.buffer:\tBuffer:new'] = function(test)
   test.ok(Buffer:new('a'))

   test.done()
end

exports['lev.buffer:\tunion slices/concat'] = function(test)
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
   test.equal(tostring(union_buf), 'abcdefghijlmnopqrstuv')

   test.done()
end

exports['lev.buffer:\tBuffer:readUInt8'] = function(test)
   local buf = Buffer:new(4)
   buf[1] = 0xFB
   buf[2] = 0x04
   buf[3] = 0x23
   buf[4] = 0x42

   test.equal(buf:readUInt8(1), 0xFB)
   test.equal(buf:readUInt8(2), 0x04)
   test.equal(buf:readUInt8(3), 0x23)
   test.equal(buf:readUInt8(4), 0x42)

   test.done()
end

exports['lev.buffer:\tBuffer:readInt8'] = function(test)
   local buf = Buffer:new(4)
   buf[1] = 0xFB
   buf[2] = 0x04
   buf[3] = 0x23
   buf[4] = 0x42

   test.equal(buf:readInt8(1), -0x05)
   test.equal(buf:readInt8(2), 0x04)
   test.equal(buf:readInt8(3), 0x23)
   test.equal(buf:readInt8(4), 0x42)

   test.done()
end

exports['lev.buffer:\tBuffer:readUInt16BE'] = function(test)
   local buf = Buffer:new(4)
   buf[1] = 0xFB
   buf[2] = 0x04
   buf[3] = 0x23
   buf[4] = 0x42

   test.equal(buf:readUInt16BE(1), 0xFB04)
   test.equal(buf:readUInt16BE(2), 0x0423)
   test.equal(buf:readUInt16BE(3), 0x2342)

   test.done()
end

exports['lev.buffer:\tBuffer:readUInt16LE'] = function(test)
   local buf = Buffer:new(4)
   buf[1] = 0xFB
   buf[2] = 0x04
   buf[3] = 0x23
   buf[4] = 0x42

   test.equal(buf:readUInt16LE(1), 0x04FB)
   test.equal(buf:readUInt16LE(2), 0x2304)
   test.equal(buf:readUInt16LE(3), 0x4223)

   test.done()
end

exports['lev.buffer:\tBuffer:readUInt32BE'] = function(test)
   local buf = Buffer:new(4)
   buf[1] = 0xFB
   buf[2] = 0x04
   buf[3] = 0x23
   buf[4] = 0x42

   test.equal(buf:readUInt32BE(1), 0xFB042342)

   test.done()
end

exports['lev.buffer:\tBuffer:readUInt32LE'] = function(test)
   local buf = Buffer:new(4)
   buf[1] = 0xFB
   buf[2] = 0x04
   buf[3] = 0x23
   buf[4] = 0x42

   test.equal(buf:readUInt32LE(1), 0x422304FB)

   test.done()
end

exports['lev.buffer:\tBuffer:readInt32BE'] = function(test)
   local buf = Buffer:new(4)
   buf[1] = 0xFB
   buf[2] = 0x04
   buf[3] = 0x23
   buf[4] = 0x42

   test.equal(buf:readInt32BE(1), -0x04FBDCBE)

   test.done()
end

exports['lev.buffer:\tBuffer:readInt32LE'] = function(test)
   local buf = Buffer:new(4)
   buf[1] = 0xFB
   buf[2] = 0x04
   buf[3] = 0x23
   buf[4] = 0x42

   test.equal(buf:readInt32LE(1), 0x422304FB)

   test.done()
end

exports['lev.buffer:\tBuffer:slice'] = function(test)
   local buf = Buffer:new(4)
   buf[1] = 0xFB
   buf[2] = 0x04
   buf[3] = 0x23
   buf[4] = 0x42

   local sliced_buf = buf:slice(3, 4)
   test.equal(sliced_buf:readUInt16BE(1), 0x2342)
   local sliced_sliced_buf = sliced_buf:slice(1, 1)
   test.equal(sliced_sliced_buf:readUInt8(1), 0x23)

   local buf2 = Buffer:new('abcdefghij')
   test.equal(tostring(buf2), 'abcdefghij')
   test.equal(buf2:toString(1, 2), 'ab')
   test.equal(buf2:toString(2, 3), 'bc')
   test.equal(buf2:toString(3), 'cdefghij')
   test.equal(buf2:toString(), 'abcdefghij')

   test.done()
end

exports['lev.buffer:\tBuffer:find'] = function(test)
   local buf = Buffer:new('abcdefghij')

   test.equal(buf:toString(buf:find('fgh')), 'fghij')
   test.is_nil(buf:find('kristopher'))
   test.is_nil(buf:find(''))

   test.done()
end

exports['lev.buffer:\tBuffer:upUntil'] = function(test)
   local buf = Buffer:new('abcdefghij')

   test.equal(buf:upUntil(''), '')
   test.equal(buf:upUntil('d'), 'abc')
   test.equal(buf:upUntil('d', 4), '')
   test.equal(buf:upUntil('d', 5), 'efghij')
   
   test.done()
end

exports['lev.buffer:\tBuffer.isBuffer'] = function(test)
   local buf = Buffer:new('abcdefghij')

   test.ok(Buffer.isBuffer(buf))
   test.not_ok(Buffer.isBuffer('buf'))

   test.done()
end

exports['lev.buffer:\tBuffer length'] = function(test)
   local buf = Buffer:new('abcdefghij')

   test.equal(#buf, 10)

   test.done()
end

exports['lev.buffer:\tBuffer:inspect'] = function(test)
   local buf = Buffer:new(4)
   buf[1] = 0xFB
   buf[2] = 0x04
   buf[3] = 0x23
   buf[4] = 0x42

   test.equal(buf:inspect(), '<Buffer FB 04 23 42 >')

   test.done()
end

exports['lev.buffer:\tBuffer:meta:__concat'] = function(test)
   local buf1 = Buffer:new(4)
   buf1[1] = 0xFB
   buf1[2] = 0x04
   buf1[3] = 0x23
   buf1[4] = 0x42
   local buf2 = Buffer:new('abcdefghij')
   local concat_buf = buf1 .. buf2

   test.equal(concat_buf:inspect(), '<Buffer FB 04 23 42 61 62 63 64 65 66 67 68 69 6A >')

   test.done()
end

exports['lev.buffer:\tinitial memory for buf1 and buf2 should not be changed or altered'] = function(test)
   local buf1 = Buffer:new(4)
   buf1[1] = 0xFB
   buf1[2] = 0x04
   buf1[3] = 0x23
   buf1[4] = 0x42
   local buf2 = Buffer:new('abcdefghij')

   test.equal(buf1:inspect(), '<Buffer FB 04 23 42 >')
   test.equal(buf2:inspect(), '<Buffer 61 62 63 64 65 66 67 68 69 6A >')

   test.done()
end

exports['lev.buffer:\tBuffer:fill'] = function(test)
   local buf1 = Buffer:new(4)
   buf1[1] = 0xFB
   buf1[2] = 0x04
   buf1[3] = 0x23
   buf1[4] = 0x42
   local buf2 = Buffer:new('abcdefghij')
   local concat_buf = buf1 .. buf2

   concat_buf:fill('', 4, 4)
   test.equal(concat_buf:inspect(), '<Buffer FB 04 23 00 61 62 63 64 65 66 67 68 69 6A >')
   concat_buf:fill('', 4, 8)
   test.equal(concat_buf:inspect(), '<Buffer FB 04 23 00 00 00 00 00 65 66 67 68 69 6A >')
   concat_buf:fill(0x05, 4, 8)
   test.equal(concat_buf:inspect(), '<Buffer FB 04 23 05 05 05 05 05 65 66 67 68 69 6A >')
   concat_buf:fill(0x42, 1)
   test.equal(concat_buf:inspect(), '<Buffer 42 42 42 42 42 42 42 42 42 42 42 42 42 42 >')
   concat_buf:fill("\0", 1)
   test.equal(concat_buf:inspect(), '<Buffer 00 00 00 00 00 00 00 00 00 00 00 00 00 00 >')

   test.done()
end

exports['lev.buffer:\tbitwise write'] = function(test)
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
   test.equal(writebuf:inspect(), '<Buffer FB 04 23 42 >')

   test.done()
end

exports['lev.buffer:\tBuffer pair'] = function(test)
   local buf1 = Buffer:new(4)
   buf1[1] = 0xFB
   buf1[2] = 0x04
   buf1[3] = 0x23
   buf1[4] = 0x42

   local cnt = 0
   for k, v in pairs(buf1) do
      test.is_number(k)
      test.is_number(v)
      cnt = cnt + 1
   end
   test.equal(cnt, 4)

   test.done()
end

exports['lev.buffer:\tGC'] = function(test)
   local buf1 = Buffer:new(4)
   buf1[1] = 0xFB
   buf1[2] = 0x04
   buf1[3] = 0x23
   buf1[4] = 0x42
   local buf2 = Buffer:new('abcdefghij')
   local concat_buf = buf1 .. buf2

   buf1 = nil
   buf2 = nil
   concat_buf = nil

   collectgarbage()
   test.ok(true)

   test.done();
end

return exports
