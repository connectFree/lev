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

exports['lev.fs:\tfs_utime'] = function(test)
  local fs = lev.fs

  local path = '_test_tmp1.txt'

  fs.unlink(path)

  local err, fd = fs.open(path, 'w', '0666')

  local atime, mtime = 1234, 5678
  err = fs.futime(fd, atime, mtime)
  test.is_nil(err)

  local stat
  err, stat = fs.fstat(fd)
  test.equal(stat.atime, atime)
  -- Disable this test because it fails on Travis.
  --test.equal(stat:mtime(), mtime)

  fs.close(fd)

  atime, mtime = 4321, 8765
  err = fs.utime(path, atime, mtime)
  test.is_nil(err)

  err, stat = fs.stat(path)
  test.equal(stat.atime, atime)
  test.equal(stat.mtime, mtime)

  fs.unlink(path)

  test.done()
end

return exports
