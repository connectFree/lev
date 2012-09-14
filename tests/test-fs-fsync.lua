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

exports['lev.fs:\tfs_fsync'] = function(test)
  local lev = require('lev')
  local fs = lev.fs

  local path = '_test_tmp1.txt'
  fs.open(path, 'a', '0666', function(err, fd)
    test.is_nil(err)

    local err = fs.fdatasync(fd)
    test.is_nil(err)

    err = fs.fsync(fd)
    test.is_nil(err)

    fs.fdatasync(fd, function(err)
      test.is_nil(err)

      fs.fsync(fd, function(err)
        test.is_nil(err)

        err = fs.close(fd)
        test.is_nil(err)

        err = fs.unlink(path)
        test.is_nil(err)

        test.done()
      end)
    end)
  end)
end

return exports
