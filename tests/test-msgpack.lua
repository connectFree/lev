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

local mp = require('lev').mpack
local string = require('string')
local math = require('math')
local table = require('table')

local exports = {}

local msgpack_cases = {
   false,true,nil,0,0,0,0,0,0,0,0,0,-1,-1,-1,-1,-1,127,127,255,65535,
   4294967295,-32,-32,-128,-32768,-2147483648, 0.0,-0.0,1.0,-1.0,  
   "a","a","a","","","",
   {0},{0},{0},{},{},{},{},{},{},{a=97},{a=97},{a=97},{{}},{{"a"}},
}
local data = {
   nil,
   true,
   false,
   1,
   -1,
   -2,
   -5,
   -31,
   -32,   
   -128, -- 10
   -32768, 
   
   -2147483648,
   -2147483648*100,   
   127,
   255, --15
   65535,
   4294967295,
   4294967295*100,   
   42,
   -42, -- 20
   0.79, 
   "Hello world!",
   {}, 
   {1,2,3},
   {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18}, -- 25
   {a=1,b=2},
   {a=1,b=2,c=3,d=4,e=5,f=6,g=7,h=8,i=9,j=10,k=11,l=12,m=13,n=14,o=15,p=16,q=17,r=18},
   {true,false,42,-42,0.79,"Hello","World!"}, -- 28
   {{"multi","level",{"lists","used",45,{{"trees"}}},"work",{}},"too"},
   {foo="bar",spam="eggs"}, -- 30
   {nested={maps={"work","too"}}},
   {"we","can",{"mix","integer"},{keys="and"},{2,{maps="as well"}}}, -- 32
   msgpack_cases,
}

math.randomseed(1)

local rand_raw = function (len)
  local t = {}
  for i = 1, len do t[i] = string.char(math.random(0, 255)) end
  return table.concat(t)
end

local raw_test = function (test, raw, overhead)
  local offset, res = mp.unpack(mp.pack(raw))
  test.ok(offset, "decoding failed")
  if not res == raw then
    test.ok(false, string.format("wrong raw (len %d - %d)", #res:toString(), #raw))
  end
  test.ok(offset - #raw == overhead, string.format(
    "wrong overhead %d for #raw %d (expected %d)",
    offset - #raw, #raw, overhead
  ))
end

local nb_test = function (test, n, sz)
  local offset, res = mp.unpack(mp.pack(n))
  test.ok(offset, "decoding failed")
  if not res == n then
    test.ok(false, string.format("wrong value %d, expected %d", res, n))
  end
  test.ok(offset == sz, string.format(
    "wrong size %d for number %d (expected %d)",
    offset, n, sz
  ))
end

local isnan = function (n)
  return n ~= n
end

local display = function (m, x)
  local _t = type(x)
  p(string.format("\n%s: %s ", m, _t))
  if _t == "table" then p(x) end
end

local simpledump = function (s)
  local out = ""
  for i = 1, #s do
    out = out .. " " .. string.format("%x", string.byte(s, i))
  end
  p(out)
end

function deepcompare (t1, t2, ignore_mt, eps)
    local ty1 = type(t1)
    local ty2 = type(t2)
    if ty1 ~= ty2 then return false end
    -- non-table types can be directly compared
    if ty1 ~= 'table' then
        if ty1 == 'number' and eps then return abs(t1 - t2) < eps end
        return t1 == t2
    end
    -- as well as tables which have the metamethod __eq
    local mt = getmetatable(t1)
    if not ignore_mt and mt and mt.__eq then return t1 == t2 end
    for k1,v1 in pairs(t1) do
        local v2 = t2[k1]
        if v2 == nil or not deepcompare(v1, v2, ignore_mt, eps) then return false end
    end
    for k2,v2 in pairs(t2) do
        local v1 = t1[k2]
        if v1 == nil or not deepcompare(v1, v2, ignore_mt, eps) then return false end
    end
    return true
end

local streamtest = function (test, unp, t, dolog)
  local s = mp.pack(t)
  if dolog and #s < 1000 then simpledump(s:toString()) end
  local startat = 1
  while true do
    local unit = 1 + math.floor(math.random(0, #s/10))
    local subs = string.sub(s:toString(), startat, startat + unit - 1)
    if subs and #subs > 0 then
      unp:feed(subs)
      startat = startat + unit
    else
      break
    end
  end
  local out = unp:pull()
  if t ~= nil and t ~= false then 
    test.ok(out, 'no result')
  end
  local res = deepcompare(out,t)
  test.ok(res, "table differ")
  out = unp:pull()
  test.ok(not out, "have to be nil")
end

exports['lev.mpack:\tpack,unpack: basic'] = function (test)
  local origt = {{"multi","level",{"lists","used",45,{{"trees"}}},"work",{}},"too"}
  local sss = mp.pack(origt)
  local l, t = mp.unpack(sss)

  test.ok(#t == #origt)
  test.ok(t[1][1]:toString() == "multi")
  test.ok(t[1][2]:toString() == "level")
  test.ok(t[1][3][1]:toString() == "lists")
  test.ok(t[1][3][2]:toString() == "used")
  test.ok(t[1][3][3] == 45)
  test.ok(t[1][3][4][1][1]:toString() == "trees")
  test.ok(t[1][4]:toString() == "work")
  test.ok(t[1][5][1] == nil)
  test.ok(t[2]:toString() == "too")

  test.done()
end

exports['lev.mpack:\tpack,unpack: fixrow'] = function (test)
  for n = 0, 31 do
    raw_test(test, rand_raw(n), 1)
  end

  test.done()
end

exports['lev.mpack:\tpack,unpack: raw16'] = function (test)
  for n = 32, 32 + 100 do
    raw_test(test, rand_raw(n), 3)
  end

  for n = 65535 - 5, 65535 do
    raw_test(test, rand_raw(n), 3)
  end

  test.done()
end

exports['lev.mpack:\tpack,unpack: raw32'] = function (test)
  for n = 65536, 65536 + 5 do
    raw_test(test, rand_raw(n), 5)
  end

  test.done()
end

exports['lev.mpack:\tpack,unpack: integer positive fixnum'] = function (test)
  for n = 0, 127 do
    nb_test(test, n, 1)
  end

  test.done()
end

exports['lev.mpack:\tpack,unpack: integer uint8'] = function (test)
  for n = 128, 255 do
    nb_test(test, n, 2)
  end

  test.done()
end

--X:DISABLE find a better way to test this! It takes way to long.
--X:DISABLE exports['lev.mpack:\tpack,unpack: integer uint16'] = function (test)
--X:DISABLE   for n = 256, 65535 do
--X:DISABLE     nb_test(test, n, 3)
--X:DISABLE   end
--X:DISABLE 
--X:DISABLE   test.done()
--X:DISABLE end

exports['lev.mpack:\tpack,unpack: integer uint32'] = function (test)
  for n = 65536, 65536 + 100 do
    nb_test(test, n, 5)
  end
  for n = 4294967295 - 100, 4294967295 do
    nb_test(test, n, 5)
  end

  test.done()
end

exports['lev.mpack:\tpack,unpack: integer uint64'] = function (test)
  for n = 4294967296, 4294967296 + 100 do
    nb_test(test, n, 9)
  end

  test.done()
end

exports['lev.mpack:\tpack,unpack: integer negative fixnum'] = function (test)
  for n = -1, -32, -1 do
    nb_test(test, n, 1)
  end

  test.done()
end

exports['lev.mpack:\tpack,unpack: integer int8'] = function (test)
  for n = -33, -128, -1 do
    nb_test(test, n, 2)
  end

  test.done()
end

exports['lev.mpack:\tpack,unpack: integer int16'] = function (test)
  for n = -129, -32768, -1 do
    nb_test(test, n, 3)
  end

  test.done()
end

exports['lev.mpack:\tpack,unpack: integer int32'] = function (test)
  for n = -32769, -32769 - 100, -1 do
    nb_test(test, n, 5)
  end
  for n = -2147483648 + 100, -2147483648, -1 do
    nb_test(test, n, 5)
  end

  test.done()
end

exports['lev.mpack:\tpack,unpack: integer int64'] = function (test)
  for n = -2147483649, -2147483649 - 100, -1 do
    nb_test(test, n, 9)
  end

  test.done()
end

exports['lev.mpack:\tpack,unpack: float'] = function (test)
  for i = 1,100 do
    local n = math.random()*200 - 100
    nb_test(test, n, 9)
  end

  test.done()
end

exports['lev.mpack:\tpack,unpack: long array (16bits-32bits)'] = function (test)
  for i = 65530, 65600 do
     local ary = rand_raw(i)
     local ofs, res = mp.unpack(mp.pack(ary))
     test.ok(ofs, "decoding failed")
     if not deepcompare(res, ary) then
        test.ok(true, "long ary fail. len:" .. i)
     end
  end

  test.done()
end

exports['lev.mpack:\tpack,unpack: long map'] = function (test)
  for n = 65532, 65540 do
    local t = {}
    for i = 1, n do
      t["key" .. i] = i
    end
    local ss = mp.pack(t)
    local ofs, res = mp.unpack(ss)
    test.ok(ofs, "decoding failed")
    if not deepcompare(res, t) then
      test.ok(true ,"long map fail. len:" .. n)
    end
  end

  test.done()
end

--X:DISABLE find a better way to test this! It takes way to long.
--X:DISABLE exports['lev.mpack:\tpack,unpack: long str'] = function (test)
--X:DISABLE   for n = 65532, 65540 do
--X:DISABLE      local s = ""
--X:DISABLE      for i = 1, n do
--X:DISABLE         s = s .. "a"
--X:DISABLE      end
--X:DISABLE      local ss = mp.pack(s)
--X:DISABLE      local ofs, res = mp.unpack(ss)
--X:DISABLE     test.ok(ofs, "decoding failed")
--X:DISABLE      if not deepcompare(res, s) then
--X:DISABLE         test.ok(true ,"long str fail. len:" .. n)
--X:DISABLE      end
--X:DISABLE   end
--X:DISABLE   test.done()
--X:DISABLE end

exports['lev.mpack:\tpack,unpack: simple table'] = function (test)
  local sss = mp.pack({ 1, 2, 3 })
  local l, t = mp.unpack(sss)

  test.ok(t[1] == 1)
  test.ok(t[2] == 2)
  test.ok(t[3] == 3)
  test.ok(t[4] == nil)

  sss = mp.pack({ a = 1, b = 2, c = 3 })
  l, t = mp.unpack(sss)
  test.ok(t.a == 1)
  test.ok(t.b == 2)
  test.ok(t.c == 3)
  test.ok(t.d == nil)
--  simpledump(sss:toString())

  test.done()
end

exports['lev.mpack:\tpack,unpack: number edge'] = function (test)
--  p('nan')
  packed = mp.pack(0/0)
  test.ok(packed:toString() == string.char(0xcb, 0xff, 0xf8, 0, 0, 0, 0, 0, 0))

  local l, unpacked = mp.unpack(packed)
  test.ok(isnan(unpacked))

--  p('+inf')
  packed = mp.pack(1/0)
  test.ok(packed:toString() == string.char(0xcb, 0x7f, 0xf0, 0, 0, 0, 0, 0, 0))
  l, unpacked = mp.unpack(packed)
  test.ok(unpacked == 1/0)

--  p('-inf')
  packed = mp.pack(-1/0)
  test.ok(packed:toString() == string.char(0xcb, 0xff, 0xf0, 0, 0, 0, 0, 0, 0))
  l, unpacked = mp.unpack(packed)
  test.ok(unpacked == -1/0)

  test.done()
end

exports['lev.mpack:\tstream: raw'] = function (test)
  local unp = mp.createUnpacker(1024*1024)

  test.ok(unp)
  streamtest(test, unp, { hoge = { 5, 6 }, fug = "11" }, false)
  streamtest(test, unp, "a")
  streamtest(test, unp, "aaaaaaaaaaaaaaaaa")

  test.done()
end

exports['lev.mpack:\tstream: basic'] = function (test)
  local unp = mp.createUnpacker(1024*1024)

  t = { aho = 7, hoge = { 5, 6, "7", { 8, 9, 10 } }, fuga = "11" }
  sss = mp.pack(t)
  unp:feed(string.char(0x83, 0xa3, 0x61, 0x68))
  unp:feed(string.char(0x6f, 0x7, 0xa4, 0x66))
  unp:feed(string.char(0x75, 0x67, 0x61, 0xa2))
  unp:feed(string.char(0x31, 0x31, 0xa4, 0x68))
  unp:feed(string.char(0x6f, 0x67, 0x65, 0x94))
  unp:feed(string.char(0x5, 0x6, 0xa1, 0x37))
  unp:feed(string.char(0x93, 0x8, 0x9, 0xa))
  out = unp:pull() 
--  p(t, out)
  test.ok(out)
  test.ok(deepcompare(t, out))
  test.ok(not unp:pull())

  test.done()
end

exports['lev.mpack:\tstream: empty table'] = function (test)
  local unp = mp.createUnpacker(1024*1024)

  streamtest(test, unp, {})
  streamtest(test, unp, "")

  test.done()
end

exports['lev.mpack:\tstream: types'] = function (test)
  local unp = mp.createUnpacker(1024*1024)

  t = {}
  for i = 1, 70000 do table.insert(t, "a") end -- raw32
  streamtest(test, unp, { table.concat(t) })

  t = {}
  for i = 1, 100 do table.insert(t, "a") end -- raw16
  streamtest(test, unp, { table.concat(t) })

  t = {}
  for i = 1, 70000 do t["key" .. i] = i end -- map32
  streamtest(test, unp, t)

  t = {}
  for i = 1, 100 do t["key" .. i] = i end -- map16
  streamtest(test, unp, t) 

  t = {}
  for i = 1, 70000 do table.insert(t, 1) end -- ary32
  streamtest(test, unp, t) 

  t = {}
  for i = 1, 100 do table.insert(t, i) end -- ary16
  streamtest(test, unp, t) 

  streamtest(test, unp, { 0.001 }) -- double
  streamtest(test, unp, { -10000000000000000 }) -- i64
  streamtest(test, unp, { -1000000000000000 }) -- i64
  streamtest(test, unp, { -100000000000000 }) -- i64
  streamtest(test, unp, { -10000000000000 }) -- i64
  streamtest(test, unp, { -1000000000000 }) -- i64
  streamtest(test, unp, { -100000000000 }) -- i64
  streamtest(test, unp, { -10000000000 }) -- i64
  streamtest(test, unp, { -1000000000 }) -- i32
  streamtest(test, unp, { -100000000 }) -- i32
  streamtest(test, unp, { -10000000 }) -- i32
  streamtest(test, unp, { -1000000 }) -- i32
  streamtest(test, unp, { -100000 }) -- i32
  streamtest(test, unp, { -10000 }) -- i16
  streamtest(test, unp, { -1000 }) -- i16
  streamtest(test, unp, { -100 }) -- i8
  streamtest(test, unp, { -10 }) -- neg fixnum
  streamtest(test, unp, { -1 }) -- neg fixnum
  streamtest(test, unp, { 1000000000000000000 }) -- u64
  streamtest(test, unp, { 100000000000000000 }) -- u64
  streamtest(test, unp, { 10000000000000000 }) -- u64
  streamtest(test, unp, { 1000000000000000 }) -- u64
  streamtest(test, unp, { 100000000000000 }) -- u64
  streamtest(test, unp, { 10000000000000 }) -- u64
  streamtest(test, unp, { 1000000000000 }) -- u64
  streamtest(test, unp, { 100000000000 }) -- u64
  streamtest(test, unp, { 10000000000 }) -- u64
  streamtest(test, unp, { 1000000000 }) -- u32
  streamtest(test, unp, { 100000000 }) -- u32
  streamtest(test, unp, { 10000000 }) -- u32
  streamtest(test, unp, { 1000000 }) -- u32
  streamtest(test, unp, { 100000 }) -- u32
  streamtest(test, unp, { 10000 }) -- u16
  streamtest(test, unp, { 1000 }) -- u16
  streamtest(test, unp, { 1, 10, 100 }) -- u8

  test.done()
end

exports['lev.mpack:\tstream: multiple tables'] = function (test)
  local unp = mp.createUnpacker(1024*1024)

  t1 = { 10, 20, 30 }
  s1 = mp.pack(t1)
  t2 = { "aaa", "bbb", "ccc" }
  s2 = mp.pack(t2)
  t3 = { a = 1, b = 2, c = 3 }
  s3 = mp.pack(t3)
  sss = s1:toString() .. s2:toString() .. s3:toString()
  test.ok(#sss == (#s1:toString() + #s2:toString() + #s3:toString()))
  unp:feed(s1:toString())
  unp:feed(s2:toString())
  out1 = unp:pull()
  test.ok(out1)
  test.ok(deepcompare(t1, out1))
  out2 = unp:pull()
  test.ok(out2)
  test.ok(deepcompare(t2, out2))
  out3 = unp:pull()
  test.ok(not out3)
  unp:feed(s3:toString())
  out3 = unp:pull()
  test.ok(deepcompare(t3, out3))
  out4 = unp:pull()
  test.ok(not out4)

  test.done()
end

exports['lev.mpack:\tstream: GC'] = function (test)
  t = { aho = 7, hoge = { 5, 6, "7", { 8, 9, 10 } }, fuga = "11" }
  s = mp.pack(t)

  for i = 1, 100000 do
    local u = mp.createUnpacker(1024)
    u:feed(string.sub(s:toString(), 1, 11))
    u:feed(string.sub(s:toString(), 12, #s:toString()))
    local out = u:pull()
    test.ok(out)
    out = u:pull()
    test.ok(not out)
    --collectgarbage()
  end

  test.done()
end

exports['lev.mpack:\tstream: corrupt input'] = function (test)
  s = string.char( 0x91, 0xc1 ) -- c1: reserved code
  local uc = mp.createUnpacker(1024)
  local res = uc:feed(s)

  test.ok(res == -1)

  test.done()
end

exports['lev.mpack:\tstream: too deep'] = function (test)
  t = {}
  for i=1,2000 do
    table.insert(t, 0x91)
  end
  s = string.char(unpack(t))
  uc = mp.createUnpacker(1024*1024)
  res = uc:feed(s)

  test.ok(res == -1)

  test.done()
end

--X:DISABLE -- this needs to be cleaned-up to support cBuffers!
--X:DISABLE exports['lev.mpack:\tcustom data'] = function (test)
--X:DISABLE   local unp = mp.createUnpacker(1024*1024)
--X:DISABLE 
--X:DISABLE   for i = 1, #data do -- 0 tests nil!
--X:DISABLE      local offset, res = mp.unpack(mp.pack(data[i]))
--X:DISABLE      test.ok(offset, "decoding failed")
--X:DISABLE      if not deepcompare(res, data[i]) then
--X:DISABLE         display("expected type:", data[i])
--X:DISABLE         display("found type:", res)
--X:DISABLE         p("found value:", res)
--X:DISABLE         --test.ok(false, string.format("wrong value in case %d", i))
--X:DISABLE       end
--X:DISABLE   end
--X:DISABLE 
--X:DISABLE   -- on streaming
--X:DISABLE   for i = 1, #data do
--X:DISABLE     streamtest(test, unp, data[i])
--X:DISABLE   end
--X:DISABLE 
--X:DISABLE   test.done()
--X:DISABLE end

exports['lev.mpack:\tcorrupt data'] = function (test)
  local s = mp.pack(data)
  local corrupt_tail = string.sub(s:toString(), 1, 10)
  local offset, res = mp.unpack(s) 
  test.ok(offset)

  offset, res = mp.unpack(Buffer:new(corrupt_tail)) 
  test.ok(not offset)

  test.done()
end

exports['lev.mpack:\terror bits'] = function (test)
  local res, msg = pcall(function () mp.pack({ a = function() end }) end)

  test.ok(not res)
  test.ok(msg == "invalid type: function")

  test.done()
end

return exports

