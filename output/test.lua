--require 'std'

--pprint = function(arg) print(prettytostring(arg)) end

--require 'websrv'

--pprint(websrv)

require 'websrv'

local server = websrv.server.init{port = 80, file = 'help.log'}
websrv.server.addhandler{server = server, mstr = '* *', func = function(write)
	write('Content-type: text/plain\r\n\r\n')
	write('Hello, World!\r\n')
end}

while true do
	websrv.server.run{server = server}
end
