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

local exports = {}

exports['test-env'] = function(test)
  local lev = require('lev')
  local process = lev.process
  local err = process.setenv('FOO', 'BAR')
  test.is_nil(err)
  test.equal(process.getenv('FOO'), 'BAR')

  local err = process.setenv('FOO', 'BAZ')
  test.is_nil(err)
  test.equal(process.getenv('FOO'), 'BAZ')

  local err = process.unsetenv('FOO')
  test.is_nil(err)
  test.is_nil(process.getenv('FOO'))
  test.done()
end

return exports
