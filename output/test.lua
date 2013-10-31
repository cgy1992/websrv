require 'std'

ptostring = prettytostring
pprint = function(arg) print(ptostring(arg)) end

require 'websrv'

local buffer = require 'buffer'

local GIFSIDE = 320
local gifdata = buffer(GIFSIDE * GIFSIDE)
local server = websrv.server.init{port = 80, file = 'help.log'}
websrv.server.addhandler{server = server, mstr = '* *', func = function(session)
	websrv.client.gifsetpalette("EGA")
	local img = session.Query("img")
	if img and string.len(img) > 0 then
		if 'circle' == img then
			session.write("Content-type: image/gif\r\n\r\n")
			local xc = tonumber(session.Query('x')) % GIFSIDE
			local yc = tonumber(session.Query('y')) % GIFSIDE
			local color = (math.random() % 15) + 1
			for i = 0, 6.28, 0.01 do
				local x = (GIFSIDE + math.round((xc + math.cos(i) * 10))) % GIFSIDE
				local y = (GIFSIDE + math.round((yc + math.sin(i) * 10))) % GIFSIDE
				gifdata[1 + x + (y * GIFSIDE)] = color
			end
		end
		websrv.client.gifoutput{data = gifdata:tostring(), width = GIFSIDE, height = GIFSIDE}
	end
	session.write('Content-type: text/html\r\n\r\n')
	session.write("<center>Generated a circle (click inside the image)<BR>\n")
	session.write(string.format("Pressed x=%s,y=%s<BR>\n", session.Query("x"), session.Query("y")))
	session.write(string.format("<form><input type=image border=0 src='/gif?img=circle&x=%s&y=%s'></form></center>\n", session.Query("x"), session.Query("y")))
end}

while true do
	websrv.server.run(server)
end
