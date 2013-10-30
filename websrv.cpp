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

#if defined(WIN32)
#	include <winsock2.h> 
#	include <iphlpapi.h> 
#	pragma comment(lib, "ws2_32.lib")
#	pragma comment(lib, "iphlpapi.lib")

	luaM_func_begin(read)
		luaM_reqd_param(string, ip)
		IPAddr addr = inet_addr(ip);
		unsigned char mac[6];
		ULONG len = sizeof(mac);
		DWORD err = SendARP(addr, 0, mac, &len);
		if(NO_ERROR == err)
		{
			char str[32] = {0};
			sprintf(str, "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); 
			luaM_return(string, str)
		}
		else
			return luaL_error(L, "SendARP failed with error %d", err);
	luaM_func_end

#elif defined(LINUX)

#	include <unistd.h>
#	include <sys/socket.h>
#	include <sys/types.h>
#	include <netinet/in.h>
#	include <netdb.h>
#	include <netinet/if_ether.h>
#	include <fcntl.h>

	struct auto_socket_closer
	{
		int socket;
		auto_socket_closer(int s) : socket(s) { }
		~auto_socket_closer() 
		{ 
			if(socket >= 0) 
				close(socket); 
		}
	};

	luaM_func_begin(read)
		luaM_reqd_param(string, ip)
		luaM_reqd_param(unsigned, port)
		luaM_opt_param(unsigned, attempts, 1000)
		luaM_opt_param(unsigned, timeout, 5)

		auto_socket_closer rawsock(socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP)));
		if(rawsock.socket < 0)
			return luaL_error(L, "raw socket() failed");

		hostent *host = gethostbyname(ip);
		if(!host)
			return luaL_error(L, "gethostbyname failed");

		auto_socket_closer sock(socket(AF_INET, SOCK_STREAM, 0));
		if(sock.socket < 0)
			return luaL_error(L, "socket() failed");

		fcntl(sock.socket, F_SETFL, O_NONBLOCK);

		sockaddr_in addr = {0};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr = *((in_addr *)host->h_addr);
		//if(connect(sock.socket, (sockaddr*)&addr, sizeof(sockaddr)) < 0)
			//return luaL_error(L, "connect() failed");
		connect(sock.socket, (sockaddr*)&addr, sizeof(sockaddr));

		fd_set fdset;
		timeval tv = {0};
		tv.tv_sec = timeout;
		FD_ZERO(&fdset);
		FD_SET(sock.socket, &fdset);

		if(1 != select(sock.socket + 1, nullptr, &fdset, nullptr, &tv) == 1)
			return luaL_error(L, "select() failed");

		int error = 0;
		socklen_t len = sizeof(error);
		getsockopt(sock.socket, SOL_SOCKET, SO_ERROR, &error, &len);
		if(error)
			return luaL_error(L, "select() failed with error %d", error);

		fcntl(sock.socket, F_SETFL, 0);

		for(int i = 0; i < attempts; i++)
		{
			unsigned char data[64];
			unsigned char* buf = data;

			while(buf - data <= 34)
			{
				int n = recvfrom(rawsock.socket, buf, sizeof(data) - (buf - data), 0, nullptr, nullptr);
				if(n < 0)
					return luaL_error(L, "recvfrom() failed");
				buf += n;
			}

			if(buf - data > 34)
			{
				buf = data + 14;
				if(0x45 == *buf)
				{
					//printf("%X %X %X\n", addr.sin_addr, *(in_addr*)(buf + 12), *(in_addr*)(buf + 16)); 
					int idx = -1;
					if(0 == memcmp(buf + 12, &addr.sin_addr, sizeof(addr.sin_addr)))
						idx = 6;
					else if(0 == memcmp(buf + 16, &addr.sin_addr, sizeof(addr.sin_addr)))
						idx = 0;
					if(idx >= 0)
					{
						//printf("0 %02X-%02X-%02X-%02X-%02X-%02X \n", data[0], data[1], data[2], data[3], data[4], data[5]);
						//printf("6 %02X-%02X-%02X-%02X-%02X-%02X \n", data[6], data[7], data[8], data[9], data[10], data[11]);
						char mac[32] = {0};
						sprintf(mac, "%02X-%02X-%02X-%02X-%02X-%02X", data[idx], data[idx + 1], data[idx + 2], data[idx + 3], data[idx + 4], data[idx + 5]);
						luaM_return(string, mac)
						break;
					}
				}
			}
		}

		if(!result)
			return luaL_error(L, "%d attempts are over", attempts);

	luaM_func_end

#elif defined(OSX)
#	error incompatible platform
#else
#	error incompatible platform
#endif

static const struct luaL_Reg lib[] =
{
	//{"savestack", luaM_save_stack},
	{"read", read},
    {nullptr, nullptr},
};

#define conf0_reg_enum(NAME) \
	lua_newtable(L); \
	conf0_reg_##NAME(L); \
	lua_setfield(L, -2, #NAME);

extern "C"
{
	LUAMOD_API int luaopen_websrv(lua_State *L)
	{
		luaL_newlib (L, lib);
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
