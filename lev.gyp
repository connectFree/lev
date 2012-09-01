{
  'targets': [
    {'target_name': 'liblev',
     'type': 'static_library',
     'dependencies': [
       'deps/http-parser/http_parser.gyp:http_parser',
       'deps/luajit.gyp:luajit',
       'deps/luajit.gyp:libluajit',
       'deps/yajl.gyp:yajl',
       'deps/yajl.gyp:copy_headers',
       'deps/uv/uv.gyp:uv',
       'deps/zlib/zlib.gyp:zlib',
       'deps/luacrypto.gyp:luacrypto',
     ],
     'export_dependent_settings': [
       'deps/http-parser/http_parser.gyp:http_parser',
       'deps/luajit.gyp:luajit',
       'deps/luajit.gyp:libluajit',
       'deps/yajl.gyp:yajl',
       'deps/uv/uv.gyp:uv',
       'deps/luacrypto.gyp:luacrypto',
      ],
      'conditions': [
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'cflags': [ '--std=c89' ],
          'defines': [ '_GNU_SOURCE' ]
        }],
        ['"<(without_ssl)" == "false"', {
          'sources': [
            'src/luv_tls.c',
            'src/luv_tls_conn.c',
          ],
          'dependencies': [
            'deps/openssl.gyp:openssl'
          ],
          'export_dependent_settings': [
            'deps/openssl.gyp:openssl'
          ],
          'defines': [ 'USE_OPENSSL' ],
        }],
      ],
     'sources': [
       'src/lconstants.c',
       'src/lenv.c',
       'src/lhttp_parser.c',
       'src/lev_slab.c',
       'src/lev_buffer.c',
       'src/lyajl.c',
       'src/los.c',
       'src/luv.c',
       'src/luv_fs.c',
       'src/luv_fs_watcher.c',
       'src/luv_dns.c',
       'src/luv_debug.c',
       'src/luv_handle.c',
       'src/luv_misc.c',
       'src/luv_pipe.c',
       'src/luv_process.c',
       'src/luv_stream.c',
       'src/luv_tcp.c',
       'src/luv_timer.c',
       'src/luv_tty.c',
       'src/luv_udp.c',
       'src/luv_zlib.c',
       'src/lev_init.c',
       'src/lyajl.c',
       'src/utils.c',
       'lib/lev/buffer.lua',
       'lib/lev/childprocess.lua',
       'lib/lev/core.lua',
       'lib/lev/dgram.lua',
       'lib/lev/dns.lua',
       'lib/lev/fiber.lua',
       'lib/lev/fs.lua',
       'lib/lev/http.lua',
       'lib/lev/https.lua',
       'lib/lev/json.lua',
       'lib/lev/lev.lua',
       'lib/lev/mime.lua',
       'lib/lev/module.lua',
       'lib/lev/net.lua',
       'lib/lev/path.lua',
       'lib/lev/querystring.lua',
       'lib/lev/repl.lua',
       'lib/lev/stack.lua',
       'lib/lev/timer.lua',
       'lib/lev/tls.lua',
       'lib/lev/url.lua',
       'lib/lev/utils.lua',
       'lib/lev/uv.lua',
       'lib/lev/zlib.lua',
     ],
     'defines': [
       'LUVIT_OS="<(OS)"',
       # TODO: should be versioning ...
       #'LEV_VERSION="<!(git --git-dir .git describe --tags)"',
       'LEV_VERSION="0.0.1"',
       'HTTP_VERSION="<!(git --git-dir deps/http-parser/.git describe --tags)"',
       'UV_VERSION="<!(git --git-dir deps/uv/.git describe --all --tags --always --long)"',
       'LUAJIT_VERSION="<!(git --git-dir deps/luajit/.git describe --tags)"',
       'YAJL_VERSIONISH="<!(git --git-dir deps/yajl/.git describe --tags)"',
       'BUNDLE=1',
     ],
     'include_dirs': [
       'src',
       'deps/uv/src/ares'
     ],
     'direct_dependent_settings': {
       'include_dirs': [
         'src',
         'deps/uv/src/ares'
       ]
     },
     'rules': [
       {
         'rule_name': 'jit_lua',
         'extension': 'lua',
         'outputs': [
           '<(SHARED_INTERMEDIATE_DIR)/generated/<(RULE_INPUT_ROOT)_jit.c'
         ],
         'action': [
           '<(PRODUCT_DIR)/luajit',
           '-b', '-g', '<(RULE_INPUT_PATH)',
           '<(SHARED_INTERMEDIATE_DIR)/generated/<(RULE_INPUT_ROOT)_jit.c',
         ],
         'process_outputs_as_sources': 1,
         'message': 'luajit <(RULE_INPUT_PATH)'
       }
     ],
    },
    {
      'target_name': 'lev',
      'type': 'executable',
      'dependencies': [
        'liblev',
      ],
      'sources': [
        'src/lev_main.c',
        'src/lev_exports.c',
      ],
      'msvs-settings': {
        'VCLinkerTool': {
          'SubSystem': 1, # /subsystem:console
        },
      },
      'conditions': [
        ['OS == "linux"', {
          'libraries': ['-ldl'],
        }],
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'cflags': [ '--std=c89' ],
          'defines': [ '_GNU_SOURCE' ]
        }],
        ['"<(without_ssl)" == "false"', {
          'defines': [ 'USE_OPENSSL' ],
        }],
      ],
      'defines': [ 'BUNDLE=1' ]
    },
    {
      'target_name': 'vector_lev',
      'product_name': 'vector',
      'product_extension': 'lev',
      'product_prefix': '',
      'type': 'shared_library',
      'dependencies': [
        'liblev',
      ],
      'sources': [
        'examples/native/vector.c'
      ],
      'conditions': [
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'cflags': [ '--std=c89' ],
          'defines': [ '_GNU_SOURCE' ]
        }],
      ],
    },
    {
      'target_name': 'install',
      'type': 'none',
      'copies': [
        {
          'destination': '<(lev_prefix)/bin',
          'files': [
            'out/Debug/lev'
          ]
        },
        {
          'destination': '<(lev_prefix)/lib/lev',
          'files': [
            'lib/lev/buffer.lua',
            'lib/lev/childprocess.lua',
            'lib/lev/core.lua',
            'lib/lev/dns.lua',
            'lib/lev/fiber.lua',
            'lib/lev/fs.lua',
            'lib/lev/http.lua',
            'lib/lev/https.lua',
            'lib/lev/json.lua',
            'lib/lev/lev.lua',
            'lib/lev/mime.lua',
            'lib/lev/module.lua',
            'lib/lev/net.lua',
            'lib/lev/path.lua',
            'lib/lev/querystring.lua',
            'lib/lev/repl.lua',
            'lib/lev/stack.lua',
            'lib/lev/timer.lua',
            'lib/lev/tls.lua',
            'lib/lev/url.lua',
            'lib/lev/utils.lua',
            'lib/lev/uv.lua',
            'lib/lev/zlib.lua'
          ]
        },
        {
          'destination': '<(lev_prefix)/include/lev/luajit',
          'files': [
            'deps/luajit/src/lua.h',
            'deps/luajit/src/lauxlib.h',
            'deps/luajit/src/luaconf.h',
            'deps/luajit/src/luajit.h',
            'deps/luajit/src/lualib.h'
          ]
        },
        {
          'destination': '<(lev_prefix)/include/lev/http_parser',
          'files': [
            'deps/http-parser/http_parser.h'
          ]
        },
        {
          'destination': '<(lev_prefix)/include/lev/uv',
          'files': [
            'deps/uv/include/'
          ]
        },
        {
          'destination': '<(lev_prefix)/include/lev',
          'files': [
            'src/lconstants.h',
            'src/lenv.h',
            'src/lhttp_parser.h',
            'src/lev_buffer.h',
            'src/los.h',
            'src/luv.h',
            'src/luv_debug.h',
            'src/luv_dns.h',
            'src/luv_fs.h',
            'src/luv_fs_watcher.h',
            'src/luv_handle.h',
            'src/luv_misc.h',
            'src/luv_pipe.h',
            'src/luv_portability.h',
            'src/luv_process.h',
            'src/luv_stream.h',
            'src/luv_tcp.h',
            'src/luv_timer.h',
            'src/luv_tls.h',
            'src/luv_tls_root_certs.h',
            'src/luv_tty.h',
            'src/luv_udp.h',
            'src/luv_zlib.h',
            'src/lev.h',
            'src/lev_exports.h',
            'src/lev_init.h',
            'src/lyajl.h',
            'src/utils.h'
          ]
        }
      ]
    }
  ],
}
