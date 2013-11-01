websrv
=====
simple crossplatform embedded webserver for lua

libwebserver (http://freecode.com/projects/libwebserver) binding

WINDOWS:

websrv depends on lua 5.2 that can be downloaded from http://luabinaries.sourceforge.net/

also there is visual c++ project file websrv.vcxproj.

if you have incompatible version of visual studio then simply create new dynamic library project, 
add project source and header files, module definition file and libraries.
anyway CMakeLists.txt is provided so it is possible to use cmake.

LINUX:

websrv depends on lua5.2 and liblua5.2 that can be installed with apt-get.

anyway cmake will tell you what's missing.

unfortunately I don't how to configure cmake to produce libraries without lib prefix,
so after building libwebsrv.so module you need either rename it to websrv.so or create appropriate symbolic link,
so that lua could find it. 

COMMON:

websrv = {
        client =
        {
                HTTPdirective = function: 60C89090,
                addfile = function: 60C885B0,
                contenttype = function: 60C89130,
                deletecookie = function: 60C88D90,
                delvar = function: 60C88FF0,
                getvar = function: 60C88F20,
                gifoutput = function: 60C88670,
                gifsetpalette = function: 60C88950,
                setcookie = function: 60C889F0,
                setvar = function: 60C88E30,
        },
        flags =
        {
                DYNVAR = 8,
                LOCAL = 1,
                USEEXTCONF = 4,
                USELEN = 16,
                USESSL = 2,
        },
        log = function: 60C891D0,
        server =
        {
                addhandler = function: 60C895B0,
                aliasdir = function: 60C89930,
                getconf = function: 60C89CA0,
                init = function: 60C89270,
                run = function: 60C89BE0,
        },
}


you can look through examples.html to learn how to use libwebserver from pure C or websrv module from lua.

note that some examples require mutable buffer object that can be downloaded from here https://github.com/starwing/lbuffer

alternatively you can modify the sample code to use another lua mutable buffer module
 
2013.10.30 10.36.28 undwad, samara, russia