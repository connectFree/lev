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

exports['fs_sync_test'] = function(test)
  local lev = require('lev')
  local fs = lev.fs
  local err, fd
  local is_exists

  err, fd = fs.open('LICENSE.txt', 'r', tonumber('666', 8))
  test.is_nil(err)
  test.ok(fd)
  err = fs.close(fd)
  test.is_nil(err)

  err, fd = fs.open('non_existing_file', 'r', tonumber('666', 8))
  test.equal(err.code, 'ENOENT')
  test.is_nil(fd)

  err = fs.mkdir('_tmp_dir1_', tonumber('750', 8))
  test.is_nil(err)

  err = fs.mkdir('_tmp_dir1_', tonumber('750', 8))
  test.equal(err.code, 'EEXIST')

  is_exists = fs.exists('_tmp_dir1_')
  test.ok(is_exists)

  err = fs.rmdir('_tmp_dir1_')
  test.is_nil(err)

  is_exists = fs.exists('_tmp_dir1_')
  test.ok(not is_exists)

  test.done()
end

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
  test.is_number(stat:atime()[1])
  test.is_number(stat:atime()[2])
  test.is_number(stat:ctime()[1])
  test.is_number(stat:ctime()[2])
  test.is_number(stat:mtime()[1])
  test.is_number(stat:mtime()[2])

  test.done()
end

exports['fs_async_test'] = function(test)
  local lev = require('lev')
  local fs = lev.fs

  fs.open('LICENSE.txt', 'r', tonumber('666', 8), function(err, fd)
    test.is_nil(err)
    test.ok(fd)
    fs.close(fd, function(err)
      test.is_nil(err)
      fs.open('non_existing_file', 'r', tonumber('666', 8), function(err, fd)
        test.equal(err.code, 'ENOENT')
        test.is_nil(fd)

        test.done()
      end)
    end)
  end)
end

return exports
