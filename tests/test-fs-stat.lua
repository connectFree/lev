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

exports['fs_sync_stat'] = function(test)
  local lev = require('lev')
  local fs = lev.fs
  local err, stat = fs.stat('LICENSE.txt')
  test.is_nil(err)
  test.ok(stat)
  test.ok(stat:is_file())
  test.ok(not stat:is_block_device())
  test.ok(not stat:is_character_device())
  test.ok(not stat:is_directory())
  test.ok(not stat:is_fifo())
  test.ok(not stat:is_socket())
  test.ok(not stat:is_symbolic_link())
  test.is_number(stat:dev())
  test.is_number(stat:ino())
  test.is_number(stat:mode())
  test.is_number(stat:nlink())
  test.is_number(stat:uid())
  test.is_number(stat:gid())
  test.is_number(stat:size())
  test.is_number(stat:blksize())
  test.is_number(stat:blocks())
  test.is_number(stat:atime())
  test.is_number(stat:ctime())
  test.is_number(stat:mtime())

  test.done()
end

return exports
