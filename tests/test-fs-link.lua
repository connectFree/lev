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

exports['lev.fs:\tfs_link'] = function(test)
  local lev = require('lev')
  local fs = lev.fs

  local path1 = '_test_tmp1.txt'
  local path2 = '_test_tmp2.txt'

  fs.unlink(path1)
  fs.unlink(path2)

  local err, fd = fs.open(path1, 'a', '0666')
  fs.close(fd)

  err = fs.link(path1, path2)
  test.is_nil(err)

  local stat1, stat2
  err, stat1 = fs.stat(path1)
  err, stat2 = fs.stat(path2)
  test.equal(stat2.ino, stat1.ino)

  fs.unlink(path2)

  err = fs.symlink(path1, path2)
  test.is_nil(err)

  local path
  err, path = fs.readlink(path2)
  test.is_nil(err)
  test.equal(path, path1)

  fs.unlink(path1)
  fs.unlink(path2)

  test.done()
end

return exports
