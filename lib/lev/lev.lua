--[[

Copyright 2012 The Luvit Authors. All Rights Reserved.

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

-- Bootstrap require system
local levbase = require('levbase')
local utils = require('utils')
_G.getcwd = nil
_G.argv = nil
--require = require('module').require

-- clear some globals
-- This will break lua code written for other lua runtimes
_G.process = io
_G.io = nil
_G.os = nil
_G.math = nil
_G.string = nil
_G.coroutine = nil
_G.jit = nil
_G.bit = nil
_G.debug = nil
_G.table = nil
_G.loadfile = nil
_G.dofile = nil
_G.print = utils.print
_G.p = utils.prettyPrint
_G.debug = utils.debug
_G.Buffer = levbase.buffer

-- -- Move the version variables into a table
-- process.version = VERSION
-- process.versions = {
--   lev = VERSION,
--   uv = native.VERSION_MAJOR .. "." .. native.VERSION_MINOR .. "-" .. UV_VERSION,
--   luajit = LUAJIT_VERSION,
--   yajl = YAJL_VERSION,
--   zlib = ZLIB_VERSION,
--   http_parser = HTTP_VERSION,
--   openssl = OPENSSL_VERSION,
-- }
-- _G.VERSION = nil
-- _G.YAJL_VERSION = nil
-- _G.LUAJIT_VERSION = nil
-- _G.UV_VERSION = nil
-- _G.HTTP_VERSION = nil
-- _G.ZLIB_VERSION = nil
-- _G.OPENSSL_VERSION = nil

