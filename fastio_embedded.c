#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>

#define CHUNK_SIZE (1024 * 1024)

// 读取文件分块函数 - Compatible with embedded Lua environments
static int read_chunk(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        luaL_error(L, "cannot open %s: %s", path, strerror(errno));
    }

    char *buf = (char*)malloc(CHUNK_SIZE);  // 动态分配缓冲区
    if (!buf) {
        fclose(fp);
        luaL_error(L, "memory allocation failed");
    }
    
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    size_t nread;

    while ((nread = fread(buf, 1, CHUNK_SIZE, fp)) > 0) {
        luaL_addlstring(&b, buf, nread);
    }

    free(buf);  // 释放缓冲区
    
    if (ferror(fp)) {
        fclose(fp);
        luaL_error(L, "read error for %s", path);
    }
    fclose(fp);
    
    luaL_pushresult(&b);
    return 1;
}

// Library registration for embedded environments
static const luaL_Reg fastio_lib[] = {
    {"read_chunk", read_chunk},
    {NULL, NULL}
};

// Entry point compatible with both Lua 5.2/5.3 and embedded systems
#if LUA_VERSION_NUM >= 502
LUAMOD_API __attribute__((visibility("default"))) int luaopen_fastio(lua_State *L) {
    luaL_newlib(L, fastio_lib);
    return 1;
}
#else
LUAMOD_API __attribute__((visibility("default"))) int luaopen_fastio(lua_State *L) {
    luaL_register(L, "fastio", fastio_lib);
    return 1;
}
#endif