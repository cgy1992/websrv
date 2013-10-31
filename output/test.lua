require 'std'

ptostring = prettytostring
pprint = function(arg) print(ptostring(arg)) end

require 'websrv'

local buffer = require 'buffer'

local image = nil
local server = websrv.server.init{port = 80, file = 'help.log'}
websrv.server.addhandler{server = server, mstr = '* /', func = function(session)
	local img = session.Query("img")
	if string.len(img) > 0 then
		if image then
			session.write("Content-type: image/jpeg\r\n\r\n")
			session.write(image:tostring())
		end
		return nil
	end
	session.write("Content-type: text/html\r\n\r\n")
	session.write([[
		<HTML>
		<BODY bgcolor='EFEFEF'>
		<form method='POST' action='/' enctype='multipart/form-data'>
		<input type=file name=image><BR>
		<input type=submit value=upload><BR>
		</form>
	]])
	local mpart = session.MultiPart("image")
	if mpart and mpart.size > 0 then
		session.write(string.format("%s<BR><img src='/?img=%s.jpg'>\n", mpart.filename, mpart.filename))
		image = buffer(mpart.data)
	else 
		image = nil
	end
	session.write("</BODY>\n</HTML>\n")
end}

while true do
	websrv.server.run(server)
end
