require 'std'
require 'websrv'

local function return_args(session, ...)
    session.write('Content-type: text/plain\r\n\r\n')
    for i,arg in ipairs{...} do
        session.write(i..': '..tostring(arg)..'\n\n')
    end
end

local server = websrv.server.init{port = 80, file = 'help.log'}
websrv.server.addhandler{server = server, mstr = '* *', func = function(session)
    return_args
    (
        session, 
        session.Header(), 
        session.Query(), 
        session.Post(), 
        session
        --session
    )
end}

while true do
	websrv.server.run(server)
end