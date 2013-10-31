require 'std'

ptostring = prettytostring
pprint = function(arg) print(ptostring(arg)) end

require 'websrv'

local server = websrv.server.init{port = 443, file = 'help.log', flags = websrv.flags.USESSL, cert = 'foo-cert.pem'}
websrv.server.addhandler{server = server, mstr = '* *', func = function(session)
	session.write('Content-type: text/plain\r\n\r\n')
	session.write('Hello, World!\r\n')
end}

while true do
	websrv.server.run(server)
end
