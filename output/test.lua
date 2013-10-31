require 'std'

ptostring = prettytostring
pprint = function(arg) print(ptostring(arg)) end

require 'websrv'

local server = websrv.server.init{port = 80, file = 'help.cfg', flags = websrv.flags.USEEXTCONF}
websrv.server.addhandler{server = server, mstr = '* *', func = function(session)
	session.write("Content-type: text/html\r\n\r\n")
	session.write("<PRE>")
	websrv.client.addfile('help.cfg')
	session.write("</PRE>")
	session.write("ClientInfo->Conf(\"PERSONAL_CONF\",\"PORT\")="..session.Conf("PERSONAL_CONF","PORT").."<BR>\n")
	session.write("ClientInfo->Conf(\"PERSONAL_CONF\",\"IP\")="..session.Conf("PERSONAL_CONF","IP").."<BR>\n")
	session.write("ClientInfo->Conf(\"LIBWEBSERVER\",\"PORT\")="..session.Conf("LIBWEBSERVER","PORT").."<BR>\n")
end}

while true do
	websrv.server.run(server)
end
