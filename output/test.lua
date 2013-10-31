require 'std'

ptostring = prettytostring
pprint = function(arg) print(ptostring(arg)) end

require 'websrv'

local server = websrv.server.init{port = 80, file = 'help.log'}
websrv.server.addhandler{server = server, mstr = '* *', func = function(session)
	local username = session.Post('user')
	if username and string.len(username) > 0 then
		websrv.client.setcookie{key = 'username', value = username, timeoffset = '+15V'}
	end
	session.write("Content-type: text/html\r\n\r\n")
	session.write("<form method='POST'>\r\n")
	session.write("<input type='text' name='user' value='"..session.Cookie('username').."'>\r\n<BR>")
    session.write("<input type='submit' name='send' value=' GO! '>\r\n<BR>")
    session.write("</form>\r\n")
end}

while true do
	websrv.server.run(server)
end
