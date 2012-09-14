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

exports['lev.fs:\tfs_sync_rename'] = function(test)
  local lev = require('lev')
  local fs = lev.fs
  local path = '_tmp_file1.txt'
  local new_path = '_tmp_file2.txt'
  assert(not fs.exists(path))
  assert(not fs.exists(new_path))

  local err, fd = fs.open(path, 'w+', '0666')
  test.is_nil(err)

  err = fs.close(fd)
  test.is_nil(err)

  err = fs.rename(path, new_path)
  test.is_nil(err)
  test.ok(not fs.exists(path))
  test.ok(fs.exists(new_path))

  err = fs.unlink(new_path)
  test.is_nil(err)

  test.done()
end

return exports
