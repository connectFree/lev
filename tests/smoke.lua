local exports = { }

exports['asserts_helpers'] = function (test)
  test.equal(test.h.ok(true), true)
  test.equal(test.h.throws(foo), true)
  test.done()
end

exports['asserts_ok'] = function (test)
  test.ok(true)
  test.not_ok(not true)
  test.done()
end

exports['asserts_equal'] = function (test)
  test.equal(1, 1)
  test.not_equal(2, 1)
  test.equal({1,2,3, foo = 'foo', bar = { 'baz' }}, {bar = { 'baz' }, 1,2,3, foo = 'foo'})
  test.not_equal({1,2,3, foo = 'foo', bar = { 'baz' }}, {bar = { 'baz' }, 1,3,2, foo = 'foo'})

  local a = {}
  a[1] = a
  local b = a
  test.equal(a, b)

  test.done()
end

exports['asserts_nil'] = function (test)
  test.is_nil(nil)
  test.not_is_nil(1)
  test.done()
end

exports['asserts_number'] = function (test)
  test.is_number(0)
  test.is_number(1)
  test.is_number(3.1415926)
  test.is_number(1 / 1)
  test.is_number(1 / 0)
  test.not_is_number(nil)
  test.not_is_number('')
  test.not_is_number(true)
  test.not_is_number(false)
  test.not_is_number({})
  test.done()
end

exports['asserts_boolean'] = function (test)
  test.not_is_boolean(0)
  test.not_is_boolean(1)
  test.not_is_boolean(3.1415926)
  test.not_is_boolean(1 / 1)
  test.not_is_boolean(1 / 0)
  test.not_is_boolean(nil)
  test.not_is_boolean('')
  test.is_boolean(true)
  test.is_boolean(not '')
  test.is_boolean(false)
  test.is_boolean(not false)
  test.not_is_boolean({})
  test.done()
end

exports['asserts_string'] = function (test)
  test.not_is_string(0)
  test.not_is_string(1)
  test.not_is_string(3.1415926)
  test.not_is_string(1 / 1)
  test.not_is_string(1 / 0)
  test.not_is_string(nil)
  test.is_string('')
  test.not_is_string(true)
  test.not_is_string(not '')
  test.not_is_string(false)
  test.not_is_string({})
  test.done()
end

exports['asserts_table'] = function (test)
  test.is_table({})
  test.is_table({1,2,3})
  test.is_table({a=1,b=3})
  test.is_table({a=1,0,2,3,b=3})
  test.not_is_table(1)
  test.not_is_table(false)
  test.not_is_table(true)
  test.not_is_table('a')
  test.done()
end

exports['asserts_array'] = function (test)
  test.is_array({})
  test.is_array({1,2,3})
  test.not_is_array({a=1,b=3})
  test.not_is_array({a=1,0,2,3,b=3})
  test.not_is_array(1)
  test.not_is_array(false)
  test.not_is_array(true)
  test.not_is_array('a')
  test.done()
end

exports['asserts_hash'] = function (test)
  test.is_hash({})
  test.not_is_hash({1,2,3})
  test.is_hash({a=1,b=3})
  test.not_is_hash({a=1,0,2,3,b=3})
  test.not_is_hash(1)
  test.not_is_hash(false)
  test.not_is_hash(true)
  test.not_is_hash('a')
  test.done()
end

exports['asserts_callable'] = function (test)
  test.is_callable(print)
  test.not_is_callable({})
  local callable = setmetatable({},{__call = function () return true end})
  test.ok(callable())
  test.is_callable(callable)
  local fake_callable = setmetatable({},{__call = {}})
  test.throws(fake_callable)
  test.not_is_callable(fake_callable)
  test.done()
end

exports['asserts_throws'] = function (test)
  test.throws(assert, nil)
  test.not_throws(assert, 1)
  test.done()
end

return exports
