#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int main() {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    
    // Setting GC to generational mode
    lua_gc(L, LUA_GCGEN, 0);

    // Running the testbench
    if (luaL_dofile(L, "./testbench.lua")) {
        fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
        return 1;
    }

    lua_getglobal(L, "test");
    lua_pcall(L, 0, 0, 0);

    // Closing the Lua state
    lua_close(L);

    return 0;
}