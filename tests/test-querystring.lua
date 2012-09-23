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

local qs = require('querystring')

local exports = {}

local testcases = { 
   { 'foo=918854443121279438895193', 'foo=918854443121279438895193', { foo = '918854443121279438895193' }}
  ,{ 'foo=bar', 'foo=bar', { foo = 'bar' }}
  -- TODO: should be support duplicate
  --,{ 'foo=bar&foo=quux', 'foo=bar&foo=quux', { foo = { 'bar', 'quux' } }}
  ,{ 'foo=1&bar=2', 'foo=1&bar=2', { foo = '1', bar = '2' }}
  ,{ 'my+weird+field=q1%212%22%27w%245%267%2Fz8%29%3F', 'my%20weird%20field=q1!2%22\'w%245%267%2Fz8)%3F', { ['my weird field'] = 'q1!2"\'w$5&7/z8)?' }}
  ,{ 'foo%3Dbaz=bar', 'foo%3Dbaz=bar', { ['foo=baz'] = 'bar'}}
  ,{ 'foo=baz=bar', 'foo=baz%3Dbar', { foo = 'baz=bar'}}
  -- TODO: should be support duplicate
  --,{ 'str=foo&arr=1&arr=2&arr=3&somenull=&undef=', 'str=foo&arr=1&arr=2&arr=3&somenull=&undef=', { str = 'foo', arr = { '1', '2', '3' }, somenull = '', undef = '' }}
  ,{ ' foo = bar ', '%20foo%20=%20bar%20', { [' foo '] = ' bar ' }}
  ,{ 'foo=%zx', 'foo=%25zx', { foo = '%zx' }}
  --,{ 'foo=%EF%BF%BD', 'foo=%EF%BF%BD', { foo = 'ufffd' }}
}

local colon_testcases = { 
   { 'foo:bar', 'foo:bar', { foo = 'bar' } }
  -- TODO: should be support duplicate
  --,{ 'foo:bar;foo:quux', 'foo:bar;foo:quux', { foo = { 'bar', 'quux' } } }
  ,{ 'foo:1&bar:2;baz:quux', 'foo:1%26bar%3A2;baz:quux', { foo = '1&bar:2', baz = 'quux' } }
  ,{ 'foo%3Abaz:bar', 'foo%3Abaz:bar', { ['foo:baz'] = 'bar' } }
  ,{ 'foo:baz:bar', 'foo:baz%3Abar', { foo = 'baz:bar' } }
}

local weird_objects = {
   { { fn = function () end }, 'fn=', { fn = '' } }
  ,{ { math = require('math') }, 'math=', { math = '' } }
  ,{ { obj = {} }, 'obj=', { obj = '' } }
  ,{ { f = false, t = true }, 'f=false&t=true', { f = 'false', t = 'true' } }
  ,{ { n = nil }, 'n=', { n = '' } }
}


exports['lev.querystring:\tparse: basic parse'] = function (test)
  for _, v in ipairs(testcases) do
    local parsed = qs.parse(v[1])
    test.equal(v[3], parsed)
  end

  test.done()
end

exports['lev.querystring:\tparse: semi colon separate, colon equal'] = function (test)
  for _, v in ipairs(colon_testcases) do
    local parsed = qs.parse(v[1], ';', ':')
    test.equal(v[3], parsed)
  end

  test.done()
end

exports['lev.querystring:\tparse: weired object'] = function (test)
  for _, v in ipairs(weird_objects) do
    local parsed = qs.parse(v[2])
    test.equal(v[3], parsed)
  end

  test.done()
end

exports['lev.querystring:\tparse: nested qs-in-qs'] = function (test)
  local f = qs.parse('a=b&q=x%3Dy%26y%3Dz')
  f.q = qs.parse(f.q)
  test.equal(f, { a = 'b', q = { x = 'y', y = 'z' } })

  test.done()
end

exports['lev.querystring:\tparse: nested in colon'] = function (test)
  local f = qs.parse('a:b;q:x%3Ay%3By%3Az', ';', ':')
  f.q = qs.parse(f.q, ';', ':')
  test.equal(f, { a = 'b', q = { x = 'y', y = 'z' } })

  test.done()
end

exports['lev.querystring:\tparse: spcify illigal data'] = function (test)
  test.not_throws(qs.parse, {})
  test.not_throws(qs.parse, nil)
  test.not_throws(qs.parse, function () end)

  test.done()
end

return exports
