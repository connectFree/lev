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

exports['fs_sync_truncate'] = function(test)
  local lev = require('lev')
  local fs = lev.fs
  local path = '_tmp_file1.txt'
  assert(not fs.exists(path))

  local err, fd = fs.open(path, 'w+', tonumber('666', 8))
  test.is_nil(err)

  local byte_count_written
  local content = 'lev is everything great about Node.\n'
  local buf = Buffer:new(content)
  err, byte_count_written = fs.write(fd, buf)
  test.is_nil(err)

  local stat
  err, stat = fs.stat(path)
  test.is_nil(err)
  test.equal(stat:size(), #content)

  local new_size = 3
  err = fs.ftruncate(fd, new_size)
  test.is_nil(err)

  err, stat = fs.stat(path)
  test.is_nil(err)
  test.equal(stat:size(), new_size)

  err = fs.close(fd)
  test.is_nil(err)

  err = fs.unlink(path)
  test.is_nil(err)

  test.done()
end

return exports
