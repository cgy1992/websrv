require 'std'

pprint = function(arg) print(prettytostring(arg)) end

require 'websrv'

pprint(websrv)

local server = websrv.server.init{port = 80, file = 'help.log'}
websrv.server.addhandler{server = server, mstr = '* *', func = function(stream)
	websrv.log{text = 'processing...\n'}
	websrv.log{text = type(io.output())..' '..io.type(io.output())..' '..tostring(io.output)..'\n'}
	websrv.log{text = type(stream)..' '..io.type(stream)..' '..tostring(stream)..'\n'}
	--stream:test()
	--io.output(stream)
	stream:write('Content-type: text/plain\r\n\r\n')
	stream:write('Hello, World!\r\n')
end}

while true do
	websrv.server.run{server = server}
end
