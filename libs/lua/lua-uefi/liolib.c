/*
** $Id: liolib.c,v 2.112 2013/04/11 18:34:06 roberto Exp $
** Standard I/O (and system) library
** See Copyright Notice in lua.h
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define liolib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"


static int io_type (lua_State *L) {
  return luaL_error(L, "io.type is not allow");
}

static int io_close (lua_State *L) {
  return luaL_error(L, "io.close is not allow");
}

static int io_open (lua_State *L) {
  return luaL_error(L, "io.open is not allow");
}

static int io_popen (lua_State *L) {
  return luaL_error(L, "io.popen is not allow");
}

static int io_tmpfile (lua_State *L) {
  return luaL_error(L, "io.tmpfile is not allow");
}

static int io_input (lua_State *L) {
  return luaL_error(L, "io.input is not allow");
}

static int io_output (lua_State *L) {
  return luaL_error(L, "io.output is not allow");
}

static int io_lines (lua_State *L) {
  return luaL_error(L, "io.lines is not allow");
}

static int io_read (lua_State *L) {
  return luaL_error(L, "io.read is not allow");
}

static int io_write (lua_State *L) {
  return luaL_error(L, "file.flush is not allow");
}

static int io_flush (lua_State *L) {
  return luaL_error(L, "io.flush is not allow");
}

/*
** functions for 'io' library
*/
static const luaL_Reg iolib[] = {
  {"close", io_close},
  {"flush", io_flush},
  {"input", io_input},
  {"lines", io_lines},
  {"open", io_open},
  {"output", io_output},
  {"popen", io_popen},
  {"read", io_read},
  {"tmpfile", io_tmpfile},
  {"type", io_type},
  {"write", io_write},
  {NULL, NULL}
};

LUAMOD_API int luaopen_io (lua_State *L) {
  luaL_newlib(L, iolib);  /* new module */
  return 1;
}
