require 'std'

ptostring = prettytostring
pprint = function(arg) print(ptostring(arg)) end

require 'websrv'

local server = websrv.server.init{port = 80, file = 'help.log'}
websrv.server.addhandler{server = server, mstr = '* *', func = function(session)
	if 'username' ~= session.username or 'password' ~= session.password then
		websrv.client.HTTPdirective{directive = 'HTTP/1.1 401 Authorization Required'}
        session.write('WWW-Authenticate: Basic realm=\"This site info\"\r\n')
        session.write('Content-type: text/html\r\n\r\n')
        session.write('\n')
        session.write('Access denied\n')
        session.write('\n')
	end
    session.write('Content-type: text/html\r\n\r\n')         
    session.write('\n')        
    session.write('You entered in your area\n')
    session.write('\n')
end}

while true do
	websrv.server.run{server = server}
end
