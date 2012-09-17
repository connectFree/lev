/*
 *  Copyright 2012 The lev Authors. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

/*
** lev frontend Runs commands, scripts, read-eval-print (REPL) etc.
**
** Major portions taken verbatim or adapted from the Lua interpreter.
** Copyright (C) 1994-2008 Lua.org, PUC-Rio. See Copyright Notice in lua.h
** Copyright (C) 2005-2012 Mike Pall. See Copyright Notice in luajit.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define luajit_c

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "lj_arch.h"

/* X:S lev */
#include "lev_new_base.h"
#include "uv.h"
#include "utils.h"
#include "luv_debug.h"
#include "luv_portability.h"
#include "lhttp_parser.h"
/* X:E lev */

#if LJ_TARGET_POSIX
#include <unistd.h>
#define lua_stdin_is_tty()  isatty(0)
#elif LJ_TARGET_WINDOWS
#include <io.h>
#ifdef __BORLANDC__
#define lua_stdin_is_tty()  isatty(_fileno(stdin))
#else
#define lua_stdin_is_tty()  _isatty(_fileno(stdin))
#endif
#else
#define lua_stdin_is_tty()  1
#endif

#if !LJ_TARGET_CONSOLE
#include <signal.h>
#endif

static lua_State *globalL = NULL;
static const char *progname = LUA_PROGNAME;

#define MAX_WORKERS 64

typedef struct ipc_client {
  uv_pipe_t handle;
  struct ipc_client *n;
} ipc_client_t;

ipc_client_t *ipcClientHead = NULL;


typedef struct _lev_worker {
  char uuid[64];
  uv_process_t process;
} lev_worker;

static int core_count = 1;

#if !LJ_TARGET_CONSOLE
static void lstop(lua_State *L, lua_Debug *ar)
{
  (void)ar;  /* unused arg. */
  lua_sethook(L, NULL, 0, 0);
  /* Avoid luaL_error -- a C hook doesn't add an extra frame. */
  luaL_where(L, 0);
  lua_pushfstring(L, "%sinterrupted!", lua_tostring(L, -1));
  lua_error(L);
}

static void laction(int i)
{
  signal(i, SIG_DFL); /* if another SIGINT happens before lstop,
       terminate process (default action) */
  lua_sethook(globalL, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}
#endif

static void print_usage(void)
{
  fprintf(stderr,
  "usage: %s [options]... [script [args]...].\n"
  "Available options are:\n"
  "  -c cores  Use upto these many " LUA_QL("cores") " \n"
  "  -e chunk  Execute string " LUA_QL("chunk") ".\n"
  "  -l name   Require library " LUA_QL("name") ".\n"
  "  -b ...    Save or list bytecode.\n"
  "  -j cmd    Perform LuaJIT control command.\n"
  "  -O[opt]   Control LuaJIT optimizations.\n"
  "  -g        Run a single worker under GDB.\n"
  "  -i        Enter interactive mode after executing " LUA_QL("script") ".\n"
  "  -v        Show version information.\n"
  "  --        Stop handling options.\n"
  "  -         Execute stdin and stop handling options.\n"
  ,
  progname);
  fflush(stderr);
}

static void l_message(const char *pname, const char *msg)
{
  if (pname) fprintf(stderr, "%s: ", pname);
  fprintf(stderr, "%s\n", msg);
  fflush(stderr);
}

static int report(lua_State *L, int status)
{
  if (status && !lua_isnil(L, -1)) {
    const char *msg = lua_tostring(L, -1);
    if (msg == NULL) msg = "(error object is not a string)";
    l_message(progname, msg);
    lua_pop(L, 1);
  }
  return status;
}

static int traceback(lua_State *L)
{
  if (!lua_isstring(L, 1)) { /* Non-string error object? Try metamethod. */
    if (lua_isnoneornil(L, 1) ||
  !luaL_callmeta(L, 1, "__tostring") ||
  !lua_isstring(L, -1))
      return 1;  /* Return non-string error object. */
    lua_remove(L, 1);  /* Replace object by result of __tostring metamethod. */
  }
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* Push error message. */
  lua_pushinteger(L, 2);  /* Skip this function and debug.traceback(). */
  lua_call(L, 2, 1);  /* Call debug.traceback(). */
  return 1;
}

static int docall(lua_State *L, int narg, int clear)
{
  int status;
  int base = lua_gettop(L) - narg;  /* function index */
  lua_pushcfunction(L, traceback);  /* push traceback function */
  lua_insert(L, base);  /* put it under chunk and args */
#if !LJ_TARGET_CONSOLE
  signal(SIGINT, laction);
#endif
  status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
#if !LJ_TARGET_CONSOLE
  signal(SIGINT, SIG_DFL);
#endif
  lua_remove(L, base);  /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
  return status;
}

static void print_version(void)
{
  fputs("welcome to lev -- Lua for Event IO -- http://github.com/connectfree/lev \n", stdout);
}

static void print_jit_status(lua_State *L)
{
  int n;
  const char *s;
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  lua_getfield(L, -1, "jit");  /* Get jit.* module table. */
  lua_remove(L, -2);
  lua_getfield(L, -1, "status");
  lua_remove(L, -2);
  n = lua_gettop(L);
  lua_call(L, 0, LUA_MULTRET);
  fputs(lua_toboolean(L, n) ? "JIT: ON" : "JIT: OFF", stdout);
  for (n++; (s = lua_tostring(L, n)); n++) {
    putc(' ', stdout);
    fputs(s, stdout);
  }
  putc('\n', stdout);
}

static int getargs(lua_State *L, char **argv, int n)
{
  int narg;
  int i;
  int argc = 0;
  while (argv[argc]) argc++;  /* count total number of arguments */
  narg = argc - (n + 1);  /* number of arguments to the script */
  luaL_checkstack(L, narg + 3, "too many arguments to script");
  for (i = n+1; i < argc; i++)
    lua_pushstring(L, argv[i]);
  lua_createtable(L, narg, n + 1);
  for (i = 0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i - n);
  }
  return narg;
}

static int dofile(lua_State *L, const char *name)
{
  int status = luaL_loadfile(L, name) || docall(L, 0, 1);
  return report(L, status);
}

static int dostring(lua_State *L, const char *s, const char *name)
{
  int status = luaL_loadbuffer(L, s, strlen(s), name) || docall(L, 0, 1);
  return report(L, status);
}

static int dolibrary(lua_State *L, const char *name)
{
  lua_getglobal(L, "require");
  lua_pushstring(L, name);
  return report(L, docall(L, 1, 1));
}

static void write_prompt(lua_State *L, int firstline)
{
  const char *p;
  lua_getfield(L, LUA_GLOBALSINDEX, firstline ? "_PROMPT" : "_PROMPT2");
  p = lua_tostring(L, -1);
  if (p == NULL) p = firstline ? LUA_PROMPT : LUA_PROMPT2;
  fputs(p, stdout);
  fflush(stdout);
  lua_pop(L, 1);  /* remove global */
}

static int incomplete(lua_State *L, int status)
{
  if (status == LUA_ERRSYNTAX) {
    size_t lmsg;
    const char *msg = lua_tolstring(L, -1, &lmsg);
    const char *tp = msg + lmsg - (sizeof(LUA_QL("<eof>")) - 1);
    if (strstr(msg, LUA_QL("<eof>")) == tp) {
      lua_pop(L, 1);
      return 1;
    }
  }
  return 0;  /* else... */
}

static int pushline(lua_State *L, int firstline)
{
  char buf[LUA_MAXINPUT];
  write_prompt(L, firstline);
  if (fgets(buf, LUA_MAXINPUT, stdin)) {
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n')
      buf[len-1] = '\0';
    if (firstline && buf[0] == '=')
      lua_pushfstring(L, "return %s", buf+1);
    else
      lua_pushstring(L, buf);
    return 1;
  }
  return 0;
}

static int loadline(lua_State *L)
{
  int status;
  lua_settop(L, 0);
  if (!pushline(L, 1))
    return -1;  /* no input */
  for (;;) {  /* repeat until gets a complete line */
    status = luaL_loadbuffer(L, lua_tostring(L, 1), lua_strlen(L, 1), "=stdin");
    if (!incomplete(L, status)) break;  /* cannot try to add lines? */
    if (!pushline(L, 0))  /* no more input? */
      return -1;
    lua_pushliteral(L, "\n");  /* add a new line... */
    lua_insert(L, -2);  /* ...between the two lines */
    lua_concat(L, 3);  /* join them */
  }
  lua_remove(L, 1);  /* remove line */
  return status;
}

static void dotty(lua_State *L)
{
  int status;
  const char *oldprogname = progname;
  progname = NULL;
  while ((status = loadline(L)) != -1) {
    if (status == 0) status = docall(L, 0, 0);
    report(L, status);
    if (status == 0 && lua_gettop(L) > 0) {  /* any result to print? */
      lua_getglobal(L, "print");
      lua_insert(L, 1);
      if (lua_pcall(L, lua_gettop(L)-1, 0, 0) != 0)
  l_message(progname,
    lua_pushfstring(L, "error calling " LUA_QL("print") " (%s)",
            lua_tostring(L, -1)));
    }
  }
  lua_settop(L, 0);  /* clear stack */
  fputs("\n", stdout);
  fflush(stdout);
  progname = oldprogname;
}

static int handle_script(lua_State *L, char **argv, int n)
{
  int status;
  const char *fname;
  int narg = getargs(L, argv, n);  /* collect arguments */
  lua_setglobal(L, "arg");
  fname = argv[n];
  if (strcmp(fname, "-") == 0 && strcmp(argv[n-1], "--") != 0)
    fname = NULL;  /* stdin */
  status = luaL_loadfile(L, fname);
  lua_insert(L, -(narg+1));
  if (status == 0)
    status = docall(L, narg, 0);
  else
    lua_pop(L, narg);
  return report(L, status);
}

/* Load add-on module. */
static int loadjitmodule(lua_State *L)
{
  lua_getglobal(L, "require");
  lua_pushliteral(L, "jit.");
  lua_pushvalue(L, -3);
  lua_concat(L, 2);
  if (lua_pcall(L, 1, 1, 0)) {
    const char *msg = lua_tostring(L, -1);
    if (msg && !strncmp(msg, "module ", 7)) {
    err:
      l_message(progname,
    "unknown luaJIT command or jit.* modules not installed");
      return 1;
    } else {
      return report(L, 1);
    }
  }
  lua_getfield(L, -1, "start");
  if (lua_isnil(L, -1)) goto err;
  lua_remove(L, -2);  /* Drop module table. */
  return 0;
}

/* Run command with options. */
static int runcmdopt(lua_State *L, const char *opt)
{
  int narg = 0;
  if (opt && *opt) {
    for (;;) {  /* Split arguments. */
      const char *p = strchr(opt, ',');
      narg++;
      if (!p) break;
      if (p == opt)
  lua_pushnil(L);
      else
  lua_pushlstring(L, opt, (size_t)(p - opt));
      opt = p + 1;
    }
    if (*opt)
      lua_pushstring(L, opt);
    else
      lua_pushnil(L);
  }
  return report(L, lua_pcall(L, narg, 0, 0));
}

/* JIT engine control command: try jit library first or load add-on module. */
static int dojitcmd(lua_State *L, const char *cmd)
{
  const char *opt = strchr(cmd, '=');
  lua_pushlstring(L, cmd, opt ? (size_t)(opt - cmd) : strlen(cmd));
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  lua_getfield(L, -1, "jit");  /* Get jit.* module table. */
  lua_remove(L, -2);
  lua_pushvalue(L, -2);
  lua_gettable(L, -2);  /* Lookup library function. */
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);  /* Drop non-function and jit.* table, keep module name. */
    if (loadjitmodule(L))
      return 1;
  } else {
    lua_remove(L, -2);  /* Drop jit.* table. */
  }
  lua_remove(L, -2);  /* Drop module name. */
  return runcmdopt(L, opt ? opt+1 : opt);
}

/* Optimization flags. */
static int dojitopt(lua_State *L, const char *opt)
{
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  lua_getfield(L, -1, "jit.opt");  /* Get jit.opt.* module table. */
  lua_remove(L, -2);
  lua_getfield(L, -1, "start");
  lua_remove(L, -2);
  return runcmdopt(L, opt);
}

/* Save or list bytecode. */
static int dobytecode(lua_State *L, char **argv)
{
  int narg = 0;
  lua_pushliteral(L, "bcsave");
  if (loadjitmodule(L))
    return 1;
  if (argv[0][2]) {
    narg++;
    argv[0][1] = '-';
    lua_pushstring(L, argv[0]+1);
  }
  for (argv++; *argv != NULL; narg++, argv++)
    lua_pushstring(L, *argv);
  return report(L, lua_pcall(L, narg, 0, 0));
}

/* check that argument has no extra characters at the end */
#define notail(x) {if ((x)[2] != '\0') return -1;}

#define FLAGS_INTERACTIVE 1
#define FLAGS_VERSION     2
#define FLAGS_EXEC        4
#define FLAGS_OPTION      8
#define FLAGS_GDB         16

static int collectargs(char **argv, int *flags)
{
  int i;
  for (i = 1; argv[i] != NULL; i++) {
    if (argv[i][0] != '-') { /* Not an option? */
      return i;
    }
    switch (argv[i][1]) {  /* Check option. */
      case '-':
        notail(argv[i]);
        return (argv[i+1] != NULL ? i+1 : 0);
      case '\0':
        return i;
      case 'i':
        notail(argv[i]);
        *flags |= FLAGS_INTERACTIVE;
        /* fallthrough */
      case 'v':
        notail(argv[i]);
        *flags |= FLAGS_VERSION;
        break;
      case 'g':
        *flags |= FLAGS_GDB;
        break;
      case 'e':
        *flags |= FLAGS_EXEC;
      case 'j':  /* LuaJIT extension */
      case 'c':
      case 'l':
        *flags |= FLAGS_OPTION;
        if (argv[i][2] == '\0') {
          i++;
          if (argv[i] == NULL) {
            return -1;
          }
        }
        break;
      case 'O': break;  /* LuaJIT extension */
      case 'b':  /* LuaJIT extension */
        if (*flags) {
          return -1;
        }
        *flags |= FLAGS_EXEC;
        return 0;
      default: return -1;  /* invalid option */
    }
  }
  return 0;
}

static int runargs(lua_State *L, char **argv, int n) {
  int i;
  for (i = 1; i < n; i++) {
    if (argv[i] == NULL) continue;
    lua_assert(argv[i][0] == '-');
    switch (argv[i][1]) {  /* option */
    case 'c': {
      const char *core_number = argv[i] + 2;
      if (*core_number == '\0') core_number = argv[++i];
      lua_assert(core_number != NULL);
      core_count = atoi(core_number);
      break;
      }
    case 'e': {
      const char *chunk = argv[i] + 2;
      if (*chunk == '\0') chunk = argv[++i];
      lua_assert(chunk != NULL);
      if (dostring(L, chunk, "=(command line)") != 0)
  return 1;
      break;
      }
    case 'l': {
      const char *filename = argv[i] + 2;
      if (*filename == '\0') filename = argv[++i];
      lua_assert(filename != NULL);
      if (dolibrary(L, filename))
  return 1;  /* stop if file fails */
      break;
      }
    case 'j': {  /* LuaJIT extension */
      const char *cmd = argv[i] + 2;
      if (*cmd == '\0') cmd = argv[++i];
      lua_assert(cmd != NULL);
      if (dojitcmd(L, cmd))
  return 1;
      break;
      }
    case 'O':  /* LuaJIT extension */
      if (dojitopt(L, argv[i] + 2))
  return 1;
      break;
    case 'b':  /* LuaJIT extension */
      return dobytecode(L, argv+i);
    default: break;
    }
  }
  return 0;
}

static int handle_luainit(lua_State *L, uv_loop_t* loop) {
  int rc;
  ares_channel channel;
  struct ares_options options;

  memset(&options, 0, sizeof(options));

  rc = ares_library_init(ARES_LIB_INIT_ALL);
  assert(rc == ARES_SUCCESS);

  /* Store the loop within the registry */
  lev_set_loop(L, loop);

  /* Pull up the preload table */
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "preload");
  lua_remove(L, -2);

  /* Register debug */
  lua_pushcfunction(L, luaopen_debugger);
  lua_setfield(L, -2, "_debug");
  /* Register http_parser */
  lua_pushcfunction(L, luaopen_http_parser);
  lua_setfield(L, -2, "http_parser");
  /* Register levbase */
  lua_pushcfunction(L, luaopen_levbase);
  lua_setfield(L, -2, "lev");

  /* We're done with preload, put it away */
  lua_pop(L, 1);

  /* Hold a reference to the main thread in the registry */
  rc = lua_pushthread(L);
  assert(rc == 1);
  lua_setfield(L, LUA_REGISTRYINDEX, "main_thread");

  /* Store the ARES Channel */
  uv_ares_init_options(lev_get_loop(L), &channel, &options, 0);
  lev_set_ares_channel(L, channel);


#if LJ_TARGET_CONSOLE
  const char *init = NULL;
#else
  const char *init = getenv(LUA_INIT);
#endif
  if (init == NULL)
    return 0;  /* status OK */
  else if (init[0] == '@')
    return dofile(L, init+1);
  else
    return dostring(L, init, "=" LUA_INIT);
}

struct Smain {
  char **argv;
  int argc;
  int status;
};

/*static void timer_gc_cb(uv_handle_t* handle) {
  lua_gc((lua_State *)handle->data, LUA_GCCOLLECT, 0);
}
*/
#ifdef _WIN32
  #define SEP "\\\\"
#else
  #define SEP "/"
#endif

static int uv_workers_count = 0;
static int lev_exit_code = 0;

void worker__on_exit(uv_process_t *req, int exit_status, int term_signal) {
    if (exit_status) { /* update exit code */
      lev_exit_code = exit_status;
    }
    fprintf(stderr, "*lev: a core has exited with status %d, signal %d\n", exit_status, term_signal);
    if (SIGSEGV == term_signal) {/* we just segfaulted */
      fprintf(stderr, "\n\n\n");
      fprintf(stderr, "/****************************************************\\\n");
      fprintf(stderr, "|*  A segfault has occurred on one of lev's workers *|\n");
      fprintf(stderr, "|*  <Please diagnose the problem using the -g flag> *|\n");
      fprintf(stderr, "|*  In the event that lev project code has faulted, *|\n");
      fprintf(stderr, "|*  please report the error via the website below:  *|\n");
      fprintf(stderr, "|*                                                  *|\n");
      fprintf(stderr, "|*     https://github.com/connectFree/lev/issues    *|\n");
      fprintf(stderr, "|*                                                  *|\n");
      fprintf(stderr, "\\*******************[Thank-You!]*********************/\n");
      fprintf(stderr, "\n\n\n");
    }
    uv_close((uv_handle_t*) req, NULL);
    free( req->data );
    if (--uv_workers_count == 0) {
      exit( (lev_exit_code ? lev_exit_code : 0));
    }
}

void spawn_helper(const char*pipe_fn
                  ,lev_worker* lworker
                  ,int script_loc
                  ,const char **argv
                  ,int flags
                  ) {
  uv_process_options_t options;
  size_t exepath_size;
  char exepath[1024];
  char* args[256]; /* we shouldn't need more than 256 args */
  int n;
  int r;
  uv_stdio_container_t stdio[3];

  memset(&options, 0, sizeof(options));

  exepath_size = sizeof(exepath);
  r = uv_exepath(exepath, &exepath_size);
  assert(r == 0);

  exepath[exepath_size] = '\0';

  n = 0;
  if ((flags & FLAGS_GDB)) {
    args[n] = "gdb";
    args[++n] = "--args";
    options.file = "gdb";
    n++;
  } else {
    options.file = exepath;
  }
  args[n] = exepath;
  for (;n<256&&argv[script_loc];) {
    args[++n] = (char *)argv[script_loc++];
  }
  args[n+1] = NULL;

  options.exit_cb = worker__on_exit;
  options.args = args;

  char **environ = lev_os_environ();
  char env_temp[1024];
  int env_cnt = 0;
  char **entry;
  for (entry = environ; *entry; entry++, env_cnt++);
  char *worker_env[2 + env_cnt + 1];
  n = 0;
  sprintf(env_temp, "LEV_WORKER_ID=%s", lworker->uuid);
  worker_env[n++] = strdup(env_temp);
  sprintf(env_temp, "LEV_IPC_FILENAME=%s", pipe_fn);
  worker_env[n++] = strdup(env_temp);
  for (entry = environ; *entry; entry++) {
    worker_env[n++] = *entry;
  }
  worker_env[n] = NULL;
  options.env = worker_env; 

  options.stdio = stdio;
  if ((flags & FLAGS_GDB)) {
    options.stdio[0].flags = UV_INHERIT_FD;
    options.stdio[1].data.fd = 0;
  } else {
    options.stdio[0].flags = UV_IGNORE;
  }
  
  options.stdio[1].flags = UV_INHERIT_FD;
  options.stdio[1].data.fd = 1;
  options.stdio[2].flags = UV_INHERIT_FD;
  options.stdio[2].data.fd = 2;
  options.stdio_count = 3;

  r = uv_spawn(uv_default_loop(), &lworker->process, options);
  free( worker_env[0] );
  free( worker_env[1] );
  assert(r == 0);
}

/*
static int runmaster(lua_State *L, uv_loop_t* loop, const char *pipe_fn) {
  return 0;
}
*/

static int pmain(lua_State *L) {
  uv_loop_t *loop;
  /*uv_timer_t gc_timer;*/
  struct Smain *s = (struct Smain *)lua_touserdata(L, 1);
  char **argv = s->argv;
  int script;
  int flags = 0;
  globalL = L;
  if (argv[0] && argv[0][0]) progname = argv[0];
  /*LUAJIT_VERSION_SYM();*/  /* linker-enforced version check */
  lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
  luaL_openlibs(L);  /* open libraries */
  loop = uv_default_loop();
/*
  uv_timer_init(uv_default_loop(), &gc_timer);
  gc_timer.data = L;
  uv_timer_start(&gc_timer, (uv_timer_cb)timer_gc_cb, 5000, 5000);
*/
  #ifdef LUV_EXPORTS
  lev__suck_in_symbols();
#endif
  lua_gc(L, LUA_GCRESTART, -1);
  s->status = handle_luainit(L, loop);
  if (s->status != 0) return 0;
  script = collectargs(argv, &flags);
  if (script < 0) {  /* invalid args? */
    print_usage();
    s->status = 1;
    return 0;
  }
  if ((flags & FLAGS_VERSION)) {
    print_version();
  }
  s->status = runargs(L, argv, (script > 0) ? script : s->argc);
  if (s->status != 0) return 0;

  if (!script) {/* we must have a script to run */
    return 0;
  }

  if (NULL == getenv("LEV_WORKER_ID")) {
    /* we will fork here * core_count */

    char *pipe_fn = tempnam("/tmp", "lev_"); /* can we come-up with something better? */
    setenv("LEV_IPC_FILENAME", pipe_fn, 1);

    s->status = luaL_dostring(L, "\
      local path = require('lev').execpath():match('^(.*)"SEP"[^"SEP"]+"SEP"[^"SEP"]+$') .. '"SEP"lib"SEP"lev"SEP"?.lua'\
      package.path = path .. ';' .. package.path\
      assert(require('kickstart'))\
      assert(require('_master'))");
    if (s->status != 0) return 0;

    if ((flags & FLAGS_GDB)) {
      core_count = 1; /* force to one worker */
    }

    while (uv_workers_count < MAX_WORKERS
           && uv_workers_count < core_count
          ) {

      /* X:S pipes; mario would be proud. */
      lev_worker* _worker = (lev_worker*)malloc(sizeof(lev_worker));
      _worker->process.data = _worker;
      memset(_worker, 0, sizeof(lev_worker));

      sprintf(_worker->uuid, "%d", uv_workers_count+1);
      spawn_helper(pipe_fn
                   ,_worker
                   ,script 
                   ,(const char **)argv
                   ,flags
                   );

      /* X:E pipes */

      uv_workers_count++;
    } /* X:E while */

  } else {/* we are a worker */
    fprintf(stderr, "*lev core started: %d\n", getpid());

    s->status = luaL_dostring(L, "\
      local path = require('lev').execpath():match('^(.*)"SEP"[^"SEP"]+"SEP"[^"SEP"]+$') .. '"SEP"lib"SEP"lev"SEP"?.lua'\
      package.path = path .. ';' .. package.path\
      assert(require('kickstart'))\
      assert(require('_worker'))");
    if (s->status != 0) return 0;

    /* start the madness */

    s->status = handle_script(L, argv, script);


  }

  if (0 == s->status) {
    uv_run(loop);
  }
  /* 
     X:TODO: When FLAGS_INTERACTIVE is set,
     default to single-core mode and drop into shell 
  */
  return 0;/* skip interactivity */
  if (s->status != 0) return 0;
  if ((flags & FLAGS_INTERACTIVE)) {
    print_jit_status(L);
    dotty(L);
  } else if (script == 0 && !(flags & (FLAGS_EXEC|FLAGS_VERSION))) {
    if (lua_stdin_is_tty()) {
      print_version();
      print_jit_status(L);
      dotty(L);
    } else {
      dofile(L, NULL); 
    }
  }
  return 0;
}

int main(int argc, char **argv)
{
  argv = uv_setup_args(argc, argv);

  int status;
  struct Smain s;
  lua_State *L = lua_open();  /* create state */
  if (L == NULL) {
    l_message(argv[0], "cannot create state: not enough memory");
    return EXIT_FAILURE;
  }
  s.argc = argc;
  s.argv = argv;
  status = lua_cpcall(L, pmain, &s);
  report(L, status);
  lua_close(L);
  return (status || s.status) ? EXIT_FAILURE : EXIT_SUCCESS;
}

