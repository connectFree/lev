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

exports['lev.fs:\tfs_sync_test'] = function(test)
  local lev = require('lev')
  local fs = lev.fs
  local err, fd
  local is_exists

  err, fd = fs.open('LICENSE.txt', 'r', '0666')
  test.is_nil(err)
  test.ok(fd)
  err = fs.close(fd)
  test.is_nil(err)

  err, fd = fs.open('non_existing_file', 'r', '0666')
  test.equal(err, 'ENOENT')
  test.is_nil(fd)

  err = fs.mkdir('_tmp_dir1_', '0750')
  test.is_nil(err)

  err = fs.mkdir('_tmp_dir1_', '0750')
  test.equal(err, 'EEXIST')

  is_exists = fs.exists('_tmp_dir1_')
  test.ok(is_exists)

  err = fs.rmdir('_tmp_dir1_')
  test.is_nil(err)

  is_exists = fs.exists('_tmp_dir1_')
  test.ok(not is_exists)

  test.done()
end

exports['lev.fs:\tfs_async_test'] = function(test)
  local lev = require('lev')
  local fs = lev.fs

  fs.open('LICENSE.txt', 'r', '0666', function(err, fd)
    test.is_nil(err)
    test.ok(fd)
    fs.close(fd, function(err)
      test.is_nil(err)
      fs.open('non_existing_file', 'r', '0666', function(err, fd)
        test.equal(err, 'ENOENT')
        test.is_nil(fd)

        test.done()
      end)
    end)
  end)
end

return exports
