--require 'std'

--pprint = function(arg) print(prettytostring(arg)) end

--require 'websrv'

--pprint(websrv)

require 'websrv'

local server = websrv.server.init{port = 80, file = 'help.log'}
websrv.server.addhandler{server = server, mstr = '* *', func = function(write)
	websrv.client.addfile{file = 'help.log'}
	write('End of log\n')
end}

while true do
	websrv.server.run{server = server}
end
