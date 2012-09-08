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

exports['fs_sync_write_read'] = function(test)
  local lev = require('lev')
  local fs = lev.fs
  local path = '_tmp_file1.txt'
  local err, fd = fs.open(path, 'w+', tonumber('666', 8))
  test.is_nil(err)
  test.ok(fd)

  local content = 'lev is everything great about Node.\n'
  local write_buf = Buffer:new(content)
  local byte_count
  err, byte_count = fs.write(fd, write_buf)
  test.is_nil(err)
  test.equal(byte_count, #write_buf)
  test.equal(write_buf:toString(1, byte_count), content)

  local read_buf = Buffer:new(80)
  err, byte_count = fs.read(fd, read_buf, 0)
  test.is_nil(err)
  test.equal(byte_count, #write_buf)

  -- FIXME: write_buf seems changed after another fs operation.
  -- test.equal(write_buf:toString(1, byte_count), content)

  test.equal(read_buf:toString(1, byte_count), content)

  err = fs.close(fd)
  test.is_nil(err)

  err = fs.unlink(path)
  test.is_nil(err)

  test.done()
end

return exports
