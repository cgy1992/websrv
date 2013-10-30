/*
** websrv.cpp by undwad
** simple crossplatform embedded webserver for lua
** https://github.com/undwad/websrv mailto:undwad@mail.ru
** see copyright notice at the end of this file
*/

#if defined( __SYMBIAN32__ )
#	define SYMBIAN
#elif defined( __WIN32__ ) || defined( _WIN32 ) || defined( WIN32 )
#	ifndef WIN32
#		define WIN32
#	endif
#elif defined( __APPLE_CC__)
#   if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 40000 || __IPHONE_OS_VERSION_MIN_REQUIRED >= 40000
#		define IOS
#   else
#		define OSX
#   endif
#elif defined(linux) && defined(__arm__)
#	define TEGRA2
#elif defined(__ANDROID__)
#	define ANDROID
#elif defined( __native_client__ )
#	define NATIVECLIENT
#else
#	define LINUX
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "luaM.h"
#include "libwebserver/web_server.h"

#if defined(WIN32)
#	include <winsock2.h> 
#	pragma comment(lib, "ws2_32.lib")
#	if defined(HAVE_OPENSSL)
#		pragma comment(lib, "libeay32.lib")
#		pragma comment(lib, "ssleay32.lib")
#	endif
#elif defined(LINUX)
#	include <unistd.h>
#	include <sys/socket.h>
#	include <sys/types.h>
#	include <netinet/in.h>
#	include <netdb.h>
#	include <netinet/if_ether.h>
#	include <fcntl.h>
#elif defined(OSX)
#	error incompatible platform
#else
#	error incompatible platform
#endif

#define websrv_reg_flag(NAME) luaM_setfield(-1, integer, NAME, WS_##NAME)
void websrv_reg_flags(lua_State *L)
{
	websrv_reg_flag(LOCAL)
	websrv_reg_flag(USESSL)
	websrv_reg_flag(USEEXTCONF) 
	websrv_reg_flag(DYNVAR) 
	websrv_reg_flag(USELEN) 
}

luaM_func_begin(log)
	luaM_reqd_param(string, message)
	web_log(message);
luaM_func_end

luaM_func_begin(init)
	luaM_opt_param(integer, port, 80)
	luaM_opt_param(string, file, nullptr)
	luaM_opt_param(integer, flags, 0)
	web_server* server = new web_server;
	if(web_server_init(server, port, file, flags))
	{
		luaM_return(lightuserdata, server);
	}
	else
	{
		delete server;
		return luaL_error(L, "web_server_init failed");
	}
luaM_func_end

struct context_t
{
	lua_State* L;
	int func;
	context_t(lua_State* l, int f) : L(l), func(f) {}
}; 

void handler(void* userdata)
{
	context_t* context = (context_t*)userdata;
	lua_rawgeti(context->L, LUA_REGISTRYINDEX, context->func); 
	if(lua_pcall(context->L, 0, 0, 0)) 
		lua_pop(context->L, 1); 
}

luaM_func_begin(addhandler)
	luaM_reqd_param(userdata, server)
	luaM_reqd_param(string, mstr)
	luaM_reqd_param(function, func)
	luaM_opt_param(integer, flags, 0)
	context_t* context = new context_t(L, func);
	if(!web_server_addhandler((web_server*)server, mstr, handler, flags, context))
	{
		delete context;
		return luaL_error(L, "web_server_addhandler failed");
	}
luaM_func_end

luaM_func_begin(aliasdir)
	luaM_reqd_param(userdata, server)
	luaM_reqd_param(string, alias)
	luaM_reqd_param(string, path)
	luaM_opt_param(integer, flags, 0)
	if(!web_server_aliasdir((web_server*)server, alias, (char*)path, flags))
		return luaL_error(L, "web_server_aliasdir failed");
luaM_func_end

luaM_func_begin(run)
	luaM_reqd_param(userdata, server)
	if(!web_server_run((web_server*)server))
		return luaL_error(L, "web_server_run failed");
luaM_func_end

luaM_func_begin(getconf)
	luaM_reqd_param(userdata, server)
	luaM_reqd_param(string, topic)
	luaM_reqd_param(string, key)
	char* value = web_server_getconf((web_server*)server, (char*)topic, (char*)key);
	if(value)
	{
		luaM_return(string, value);
	}
luaM_func_end

luaM_func_begin(useSSLcert)
	luaM_reqd_param(userdata, server)
	luaM_reqd_param(string, file)
	web_server_useSSLcert((web_server*)server, file);
luaM_func_end

luaM_func_begin(useMIMEfile)
	luaM_reqd_param(userdata, server)
	luaM_reqd_param(string, file)
	web_server_useMIMEfile((web_server*)server, file);
luaM_func_end

luaM_func_begin(addfile)
	luaM_reqd_param(string, file)
	if(!web_client_addfile((char*)file))
		return luaL_error(L, "web_client_addfile failed");
luaM_func_end

luaM_func_begin(setcookie)
	luaM_reqd_param(string, key)
	luaM_reqd_param(string, value)
	luaM_reqd_param(string, timeoffset)
	luaM_opt_param(string, path, nullptr)
	luaM_opt_param(string, domain, nullptr)
	luaM_opt_param(boolean, secure, false)
	web_client_setcookie((char*)key, (char*)value, (char*)timeoffset, (char*)path, (char*)domain, secure ? 1 : 0);
luaM_func_end

luaM_func_begin(deletecookie)
	luaM_reqd_param(string, key)
	web_client_deletecookie((char*)key);
luaM_func_end

luaM_func_begin(setvar)
	luaM_reqd_param(string, name)
	luaM_reqd_param(string, value)
	web_client_setvar((char*)name, (char*)value);
luaM_func_end

luaM_func_begin(getvar)
	luaM_reqd_param(string, name)
	char* value = web_client_getvar((char*)name);
	if(value)
	{
		luaM_return(string, value);
	}
luaM_func_end

luaM_func_begin(delvar)
	luaM_reqd_param(string, name)
	web_client_delvar((char*)name);
luaM_func_end

luaM_func_begin(HTTPdirective)
	luaM_reqd_param(string, directive)
	web_client_HTTPdirective((char*)directive);
luaM_func_end

luaM_func_begin(contenttype)
	luaM_reqd_param(string, extension)
	web_client_contenttype((char*)extension);
luaM_func_end

static const struct luaL_Reg lib[] =
{
	//{"savestack", luaM_save_stack},
	{"log", log},
    {nullptr, nullptr},
};

static const struct luaL_Reg server[] =
{
	{"init", init},
	{"addhandler", addhandler},
	{"aliasdir", aliasdir},
	{"run", run},
	{"getconf", getconf},
	{"useSSLcert", useSSLcert},
	{"useMIMEfile", useMIMEfile},
    {nullptr, nullptr},
};

static const struct luaL_Reg client[] =
{
	{"addfile", addfile},
	{"setcookie", setcookie},
	{"deletecookie", deletecookie},
	{"setvar", setvar},
	{"getvar", getvar},
	{"delvar", delvar},
	{"HTTPdirective", HTTPdirective},
	{"contenttype", contenttype},
    {nullptr, nullptr},
};

#define websrv_reg_enum(NAME) \
	lua_newtable(L); \
	websrv_reg_##NAME(L); \
	lua_setfield(L, -2, #NAME);

#define websrv_reg_lib(NAME) \
	lua_newtable(L); \
	luaL_setfuncs(L, NAME, 0); \
	lua_setfield(L, -2, #NAME);

extern "C"
{
	LUAMOD_API int luaopen_websrv(lua_State *L)
	{
		luaL_newlib(L, lib);
		websrv_reg_enum(flags)
		websrv_reg_lib(server)
		websrv_reg_lib(client)
		lua_setglobal(L, "websrv");
		return 1;
	}
}

/******************************************************************************
* Copyright (C) 2013 Undwad, Samara, Russia
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/
