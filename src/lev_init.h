#ifndef LUV_INIT
#define LUV_INIT

#ifdef USE_OPENSSL
int lev_init_ssl();
#endif
int lev_init(lua_State *L, uv_loop_t* loop, int argc, char *argv[]);

#endif
