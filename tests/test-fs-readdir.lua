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

exports['lev.fs:\tfs_readdir'] = function(test)
  local lev = require('lev')
  local table = require('table')
  local fs = lev.fs

  local work_dir = '_test_work'
  local file1 = work_dir .. '/' .. 'file1.txt'
  local dir1 = work_dir .. '/' .. 'dir1'

  assert(not fs.exists(work_dir))

  fs.mkdir(work_dir)
  local err, fd = fs.open(file1, 'w')
  fs.close(fd)
  fs.mkdir(dir1)

  local entries
  err, entries = fs.readdir(work_dir)
  test.is_nil(err)
  test.is_array(entries)
  test.equal(#entries, 2)
  table.sort(entries)
  test.equal(entries, {'dir1', 'file1.txt'})

  fs.unlink(file1)
  fs.rmdir(dir1)
  fs.rmdir(work_dir)

  test.done()
end

return exports
