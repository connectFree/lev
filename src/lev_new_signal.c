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

#include "lev_new_base.h"

#include <lua.h>
#include <lauxlib.h>
#include "luv_debug.h"

void luaopen_lev_signal(lua_State *L) {
  lua_newtable(L);

#ifdef SIGHUP
  LEV_SET_FIELD(SIGHUP, number, SIGHUP);
#endif
#ifdef SIGINT
  LEV_SET_FIELD(SIGINT, number, SIGINT);
#endif
#ifdef SIGQUIT
  LEV_SET_FIELD(SIGQUIT, number, SIGQUIT);
#endif
#ifdef SIGILL
  LEV_SET_FIELD(SIGILL, number, SIGILL);
#endif
#ifdef SIGTRAP
  LEV_SET_FIELD(SIGTRAP, number, SIGTRAP);
#endif
#ifdef SIGABRT
  LEV_SET_FIELD(SIGABRT, number, SIGABRT);
#endif
#ifdef SIGIOT
# if SIGABRT != SIGIOT
  LEV_SET_FIELD(SIGIOT, number, SIGIOT);
# endif
#endif
#ifdef SIGBUS
  LEV_SET_FIELD(SIGBUS, number, SIGBUS);
#endif
#ifdef SIGFPE
  LEV_SET_FIELD(SIGFPE, number, SIGFPE);
#endif
#ifdef SIGKILL
  LEV_SET_FIELD(SIGKILL, number, SIGKILL);
#endif
#ifdef SIGUSR1
  LEV_SET_FIELD(SIGUSR1, number, SIGUSR1);
#endif
#ifdef SIGUSR2
  LEV_SET_FIELD(SIGUSR2, number, SIGUSR2);
#endif
#ifdef SIGSEGV
  LEV_SET_FIELD(SIGSEGV, number, SIGSEGV);
#endif
#ifdef SIGPIPE
  LEV_SET_FIELD(SIGPIPE, number, SIGPIPE);
#endif
#ifdef SIGALRM
  LEV_SET_FIELD(SIGALRM, number, SIGALRM);
#endif
#ifdef SIGTERM
  LEV_SET_FIELD(SIGTERM, number, SIGTERM);
#endif
#ifdef SIGCHLD
  LEV_SET_FIELD(SIGCHLD, number, SIGCHLD);
#endif
#ifdef SIGSTKFLT
  LEV_SET_FIELD(SIGSTKFLT, number, SIGSTKFLT);
#endif
#ifdef SIGCONT
  LEV_SET_FIELD(SIGCONT, number, SIGCONT);
#endif
#ifdef SIGSTOP
  LEV_SET_FIELD(SIGSTOP, number, SIGSTOP);
#endif
#ifdef SIGTSTP
  LEV_SET_FIELD(SIGTSTP, number, SIGTSTP);
#endif
#ifdef SIGTTIN
  LEV_SET_FIELD(SIGTTIN, number, SIGTTIN);
#endif
#ifdef SIGTTOU
  LEV_SET_FIELD(SIGTTOU, number, SIGTTOU);
#endif
#ifdef SIGURG
  LEV_SET_FIELD(SIGURG, number, SIGURG);
#endif
#ifdef SIGXCPU
  LEV_SET_FIELD(SIGXCPU, number, SIGXCPU);
#endif
#ifdef SIGXFSZ
  LEV_SET_FIELD(SIGXFSZ, number, SIGXFSZ);
#endif
#ifdef SIGVTALRM
  LEV_SET_FIELD(SIGVTALRM, number, SIGVTALRM);
#endif
#ifdef SIGPROF
  LEV_SET_FIELD(SIGPROF, number, SIGPROF);
#endif
#ifdef SIGWINCH
  LEV_SET_FIELD(SIGWINCH, number, SIGWINCH);
#endif
#ifdef SIGIO
  LEV_SET_FIELD(SIGIO, number, SIGIO);
#endif
#ifdef SIGPOLL
# if SIGPOLL != SIGIO
  LEV_SET_FIELD(SIGPOLL, number, SIGPOLL);
# endif
#endif
#ifdef SIGLOST
  LEV_SET_FIELD(SIGLOST, number, SIGLOST);
#endif
#ifdef SIGPWR
# if SIGPWR != SIGLOST
  LEV_SET_FIELD(SIGPWR, number, SIGPWR);
# endif
#endif
#ifdef SIGSYS
  LEV_SET_FIELD(SIGSYS, number, SIGSYS);
#endif

  lua_setfield(L, -2, "signal");
}
