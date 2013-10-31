require 'std'

ptostring = prettytostring
pprint = function(arg) print(ptostring(arg)) end

require 'websrv'

local server = websrv.server.init{port = 80, file = 'help.log'}
websrv.server.addhandler{server = server, mstr = '* *', func = function(session)
	local txt = {"one", "two", "three", "four", "five"}
	session.write("Content-type: text/html\r\n\r\n")
	session.write("<form method='QUERY'>\r\n")
	for i = 1, 5 do
		session.write("<input type='checkbox' name='number' value='"..txt[i].."'>\r\n<BR>")
	end
	session.write("<input type='submit' name='send' value=' SEND '>\r\n<BR>")
	session.write("</form>\r\n")
	session.write("You have choosen <font color='FF0000'>"..session.Query("number").."</font> numbers: \r\n")
	for i = 1, session.Query("#number") do
		session.write("<b>"..session.Query("number").."</b>,\r\n\r\n")
	end
	session.write("...<BR>\r\n\r\n")
end}

while true do
	websrv.server.run(server)
end
