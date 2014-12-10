require 'std'
require 'websrv'

local folder = 'f:/github/websrv/output/'

local function return_args(session, ...)
    session.write('Content-type: text/html\r\n\r\n')
    for i,arg in ipairs{...} do
        session.write('<p>'..i..': '..tostring(arg)..'</p>')
    end
end

local server = websrv.server.init{port = 443, file = folder..'test.log', flags = websrv.flags.USESSL, cert = folder..'foo-cert.pem'}

websrv.server.addhandler{server = server, mstr = '* *', func = function(session)
    return_args
    (
        session,
        session.Header(), 
        session.Query(), 
        session.Post()
    )
end}

while true do
	websrv.server.run(server)
end