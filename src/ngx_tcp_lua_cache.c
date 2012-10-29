
#include <nginx.h>
#include <ngx_md5.h>
#include "ngx_tcp_lua_common.h"
#include "ngx_tcp_lua_cache.h"
#include "ngx_tcp_lua_clfactory.h"

static void ngx_tcp_lua_clear_package_loaded(lua_State *L);

ngx_int_t
ngx_tcp_lua_cache_loadfile(lua_State *L, const u_char *script,
        const u_char *cache_key, char **err, unsigned enabled)
{
    int              rc;

    //u_char           buf[NGX_TCP_LUA_FILE_KEY_LEN + 1];
    //u_char          *p;


    /*  load closure factory of script file to the top of lua stack, sp++ */
    rc = ngx_tcp_lua_clfactory_loadfile(L, (char *) script);

    if (rc != 0) {
        /*  Oops! error occured when loading Lua script */
        if (rc == LUA_ERRMEM) {
            *err = "memory allocation error";

        } else {
            if (lua_isstring(L, -1)) {
                *err = (char *) lua_tostring(L, -1);
            } else {
                *err = "syntax error";
            }
        }

        return NGX_ERROR;
    }
    
    /*  call closure factory to generate new closure */
    rc = lua_pcall(L, 0, 1, 0);
    if (rc != 0) {
        dd("Error: failed to call closure factory!!");
        return NGX_ERROR;
    }
    
    ngx_tcp_lua_clear_package_loaded(L);


    return NGX_OK;
}


static void
ngx_tcp_lua_clear_package_loaded(lua_State *L)
{
    size_t       len;
    u_char      *p;

    dd("clear out package.loaded.* on the Lua land");
    lua_getglobal(L, "package"); /* package */

    lua_getfield(L, -1, "loaded"); /* package loaded */

    lua_pushnil(L); /* package loaded nil */

    while (lua_next(L, -2)) { /* package loaded key value */
        lua_pop(L, 1);  /* package loaded key */

        p = (u_char *) lua_tolstring(L, -1, &len);

#if 1
        /* XXX work-around the "stack overflow" issue of LuaRocks
         * while unloading and reloading Lua modules */
        if (len >= sizeof("luarocks") - 1 &&
                ngx_strncmp(p, "luarocks", sizeof("luarocks") - 1) == 0)
        {
            goto done;
        }
#endif

        switch (len) {
        case 2:
            if (p[0] == 'o' && p[1] == 's') {
                goto done;
            }

            if (p[0] == 'i' && p[1] == 'o') {
                goto done;
            }

#if 0
            if (ngx_strncmp(p, "_G", sizeof("_G") - 1) == 0) {
                goto done;
            }
#endif

            break;

        case 3:
            if (ngx_strncmp(p, "bit", sizeof("bit") - 1) == 0) {
                goto done;
            }

            if (ngx_strncmp(p, "jit", sizeof("jit") - 1) == 0) {
                goto done;
            }

            if (ngx_strncmp(p, "ngx", sizeof("ngx") - 1) == 0) {
                goto done;
            }

            if (ngx_strncmp(p, "ndk", sizeof("ndk") - 1) == 0) {
                goto done;
            }

            break;

        case 4:
            if (ngx_strncmp(p, "math", sizeof("math") - 1) == 0) {
                goto done;
            }

            break;

        case 5:
            if (ngx_strncmp(p, "table", sizeof("table") - 1) == 0) {
                goto done;
            }

            if (ngx_strncmp(p, "debug", sizeof("table") - 1) == 0) {
                goto done;
            }

            break;

        case 6:
            if (ngx_strncmp(p, "string", sizeof("string") - 1) == 0) {
                goto done;
            }

            break;

        case 7:
            if (ngx_strncmp(p, "package", sizeof("package") - 1) == 0) {
                goto done;
            }

            if (ngx_strncmp(p, "jit.opt", sizeof("jit.opt") - 1) == 0) {
                goto done;
            }

            break;

       case 8:
            if (ngx_strncmp(p, "jit.util", sizeof("jit.util") - 1) == 0) {
                goto done;
            }

            break;

       case 9:
            if (ngx_strncmp(p, "coroutine", sizeof("coroutine") - 1) == 0) {
                goto done;
            }

            break;

        default:
            break;
        }

        dd("clearing package %s", p);

        lua_pushvalue(L, -1);  /* package loaded key key */
        lua_pushnil(L); /* package loaded key key nil */
        lua_settable(L, -4);  /* package loaded key */
done:
        continue;
    }

    /* package loaded */
    lua_pop(L, 2);

    lua_newtable(L);
    lua_setglobal(L, "_G");
}


ngx_int_t
ngx_tcp_lua_cache_loadbuffer(lua_State *L, const u_char *src, size_t src_len,
        const u_char *cache_key, const char *name, char **err,
        unsigned enabled)
{
    int          rc;

    dd("XXX cache key: [%s]", cache_key);

    if (!enabled) {
        ngx_tcp_lua_clear_package_loaded(L);
    }
/*
    if (ngx_tcp_lua_cache_load_code(L, (char *) cache_key)
            == NGX_OK)
    {
        */
        /*  code chunk loaded from cache, sp++ */
        /*
        dd("Code cache hit! cache key='%s', stack top=%d, script='%.*s'",
                cache_key, lua_gettop(L), (int) src_len, src);
        return NGX_OK;
    }
*/
    dd("Code cache missed! cache key='%s', stack top=%d, script='%.*s'",
            cache_key, lua_gettop(L), (int) src_len, src);

    /*  load closure factory of inline script to the top of lua stack, sp++ */
    rc = ngx_tcp_lua_clfactory_loadbuffer(L, (char *) src, src_len, name);

    if (rc != 0) {
        /*  Oops! error occured when loading Lua script */
        if (rc == LUA_ERRMEM) {
            *err = "memory allocation error";

        } else {
            if (lua_isstring(L, -1)) {
                *err = (char *) lua_tostring(L, -1);
            } else {
                *err = "syntax error";
            }
        }

        return NGX_ERROR;
    }

    /*  store closure factory and gen new closure at the top of lua stack to
     *  code cache */
        /*
    rc = ngx_tcp_lua_cache_store_code(L, (char *) cache_key);

    if (rc != NGX_OK) {
        *err = "fail to generate new closure from the closure factory";
        return NGX_ERROR;
    }
*/

    /*  call closure factory to generate new closure */
    rc = lua_pcall(L, 0, 1, 0);
    if (rc != 0) {
        dd("Error: failed to call closure factory!!");
        return NGX_ERROR;
    }
    
    return NGX_OK;
}
