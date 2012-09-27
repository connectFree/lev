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

exports['lev.fs:\tfs_sync_stat'] = function(test)
  local fs = lev.fs
  local err, stat = fs.stat('LICENSE.txt')
  test.is_nil(err)
  test.ok(stat)
  test.ok(stat.isFile)
  test.ok(not stat.isBlockDevice)
  test.ok(not stat.isCharacterDevice)
  test.ok(not stat.isDirectory)
  test.ok(not stat.isFIFO)
  test.ok(not stat.isSocket)
  test.ok(not stat.isSymbolicLink)
  test.is_number(stat.dev)
  test.is_number(stat.ino)
  test.is_number(stat.mode)
  test.is_string(stat.permission)
  test.is_number(stat.nlink)
  test.is_number(stat.uid)
  test.is_number(stat.gid)
  test.is_number(stat.size)
  test.is_number(stat.blksize)
  test.is_number(stat.blocks)
  test.is_number(stat.atime)
  test.is_number(stat.ctime)
  test.is_number(stat.mtime)

  test.done()
end

return exports
