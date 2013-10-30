require 'std'

pprint = function(arg) print(prettytostring(arg)) end

require 'websrv'

pprint(websrv)

local server = websrv.server.init{port = 80, file = 'help.log'}
websrv.server.addhandler{server = server, mstr = '* *', func = function()
	websrv.log{text = 'processing...'}
	io.write('Content-type: text/plain\r\n\r\n')
	io.write('Hello, World!\r\n')
end}

while true do
	websrv.server.run{server = server}
end
