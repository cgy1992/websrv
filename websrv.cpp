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

#include <io.h>
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

luaM_func_begin_(log)
	luaM_reqd_param_(1, string, text)
	web_log(text);
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

//int io_fclose (lua_State *L) 
//{ 
//	LStream *p = tolstream(L);
//	int res = fclose(p->f);
//	return 0; 
//}
//	luaL_Stream* output = (luaL_Stream*)lua_newuserdata(L, sizeof(luaL_Stream));
//	output->closef = &io_fclose;  
//	output->f = fdopen(ClientInfo->outfd, "wb+");
//	luaL_setmetatable(L, LUA_FILEHANDLE);
//	lua_setfield(L, -2, "output");

luaM_func_begin_(write)
	size_t size;
	luaM_reqd_param_(1, lstring, buf, &size)
	if(buf && size)
		fwrite(buf, size, 1, stdout);
luaM_func_end

luaM_func_begin_(commonfunc)
	typedef char*(*proc_t)(char*);
	proc_t proc = (proc_t)lua_touserdata(L, lua_upvalueindex(1));
	luaM_opt_param_(1, string, handle, nullptr)
	char* value = proc((char*)handle);
	if(handle && '#' == handle[0])
	{
		luaM_return(integer, (int)value)
	}
	else
	{
		luaM_return(string, value)
	}
luaM_func_end

luaM_func_begin_(conffunc)
	typedef char*(*proc_t)(char*, char*);
	proc_t proc = (proc_t)lua_touserdata(L, lua_upvalueindex(1));
	luaM_reqd_param_(1, string, topic)
	luaM_reqd_param_(2, string, key)
	char* value = proc((char*)topic, (char*)key);
	luaM_return(string, value)
luaM_func_end

luaM_func_begin_(multipartfunc)
	typedef _MultiPart(*proc_t)(char*);
	proc_t proc = (proc_t)lua_touserdata(L, lua_upvalueindex(1));
	luaM_reqd_param_(1, string, handle)
	_MultiPart multipart = proc((char*)handle);
	lua_newtable(L);
	luaM_setfield(-1, string, id, multipart.id);
	luaM_setfield(-1, unsigned, size, multipart.size);
	luaM_setfield(-1, string, filename, multipart.filename);
	luaM_setfield(-1, lstring, data, multipart.data, multipart.size);
	return 1;
luaM_func_end

#define websrv_pushfunc(NAME, FUNC) \
	lua_pushlightuserdata(L, ClientInfo->NAME); \
	lua_pushcclosure(L, FUNC, 1); \
	lua_setfield(L, -2, #NAME);

void handler(void* userdata)
{
	context_t* context = (context_t*)userdata;
	lua_State* L = context->L;
	lua_rawgeti(L, LUA_REGISTRYINDEX, context->func); 

	lua_newtable(L); 
	luaM_setfield(-1, integer, outfd, ClientInfo->outfd);
	luaM_setfield(-1, string, inetname, ClientInfo->inetname);
	luaM_setfield(-1, string, request, ClientInfo->request);
	luaM_setfield(-1, string, method, ClientInfo->method);
	luaM_setfield(-1, string, username, ClientInfo->user);
	luaM_setfield(-1, string, password, ClientInfo->pass);
	
	websrv_pushfunc(Header, commonfunc)
	websrv_pushfunc(Query, commonfunc)
	websrv_pushfunc(Post, commonfunc)
	websrv_pushfunc(Cookie, commonfunc)
	websrv_pushfunc(Conf, conffunc)
	websrv_pushfunc(MultiPart, multipartfunc)

	luaM_setfield(-1, cfunction, write, write);
	if(lua_pcall(L, 1, 0, 0)) 
	{
		printf("Content-type: text/plain\r\n\r\n");
		printf("%s\r\n", lua_tostring(L, -1));
		web_client_HTTPdirective("HTTP/1.1 500 Internal Error");
		lua_pop(L, 1); 
	}
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

luaM_func_begin_(run)
	luaM_reqd_param_(1, userdata, server)
	if(!web_server_run((web_server*)server))
		return luaL_error(L, "web_server_run failed");
luaM_func_end

luaM_func_begin_(getconf)
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

luaM_func_begin_(addfile)
	luaM_reqd_param_(1, string, file)
	if(!web_client_addfile((char*)file))
		return luaL_error(L, "web_client_addfile failed");
luaM_func_end

luaM_func_begin(gifoutput)
	size_t size;
	luaM_reqd_param(lstring, data, &size)
	luaM_reqd_param(integer, width)
	luaM_reqd_param(integer, height)
	luaM_opt_param(integer, transparency, 0)
	if(web_client_gifoutput((char*)data, width, height, transparency))
		return luaL_error(L, "web_client_gifoutput failed");
luaM_func_end

luaM_func_begin_(gifsetpalette)
	luaM_reqd_param_(1, string, file)
	web_client_gifsetpalette(file);
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

luaM_func_begin_(deletecookie)
	luaM_reqd_param_(1, string, key)
	web_client_deletecookie((char*)key);
luaM_func_end

luaM_func_begin_(setvar)
	luaM_reqd_param_(1, string, name)
	luaM_reqd_param_(2, string, value)
	web_client_setvar((char*)name, (char*)value);
luaM_func_end

luaM_func_begin_(getvar)
	luaM_reqd_param_(1, string, name)
	char* value = web_client_getvar((char*)name);
	if(value)
	{
		luaM_return(string, value);
	}
luaM_func_end

luaM_func_begin_(delvar)
	luaM_reqd_param_(1, string, name)
	web_client_delvar((char*)name);
luaM_func_end

luaM_func_begin_(HTTPdirective)
	luaM_reqd_param_(1, string, directive)
	web_client_HTTPdirective((char*)directive);
luaM_func_end

luaM_func_begin_(contenttype)
	luaM_reqd_param_(1, string, extension)
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
	{"gifoutput", gifoutput},
	{"gifsetpalette", gifsetpalette},
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
