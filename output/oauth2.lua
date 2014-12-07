require 'std'
require 'websrv'
require "https"
require 'pgsql'
local url = require "url"
local ltn12 = require "ltn12"
local json = require 'json'

local folder = 'f:/github/websrv/output/'

function string:replace(map)
    local result = self
    for what,with in pairs(map) do
        local i = result:find(what)
        if nil ~= i then 
            result = result:sub(0, i - 1)..with..result:sub(i + what:len()) 
        end
    end
    return result
end

function getNestedValue(object, ...)
    local args = {...}
    if #args > 0 and object[args[1]] then
        object = object[args[1]]
        table.remove(args, 1)
        if object and #args > 0 then 
            return getNestedValue(object, table.unpack(args)) 
        end
        return object
    end
    return nil
end

local token_request_body = 'code={CODE}&client_id={CLIENT_ID}&client_secret={CLIENT_SECRET}&redirect_uri={REDIRECT_URI}&grant_type=authorization_code'
local redirect_uri = 'https://zalupa.org/oauth2?service={SERVICE}'

local connectionString = 'host=localhost dbname=vg user=undwad password=joder connect_timeout=5 keepalives=1 keepalives_idle=5 keepalives_interval=2 keepalives_count=5'

local connection, err = pg.connect(connectionString) 

local function query_row(sql)
	local res, err = connection:exec(sql)
	if err then error(err) end
    return res:fetch(), res
end

local sessions = {}

local server = websrv.server.init{port = 443, file = folder..'test.log', flags = websrv.flags.USESSL, cert = folder..'foo-cert.pem'}

local function return_error(session, code, text)
    websrv.client.HTTPdirective('HTTP/1.1 '..code..' '..tostring(text))
    session.write('Content-type: text/html\r\n\r\n')
    session.write('\n')
    session.write(tostring(text))
    session.write('\n\n')        
end

local function return_redirect(session, to)
    session.write('Content-type: text/html\r\n\r\n')
    session.write('<script language="JavaScript" type="text/javascript">location.href="'..to..'"</script>')
end

local function return_args(session, ...)
    session.write('Content-type: text/html\r\n\r\n')
    for i,arg in ipairs{...} do
        session.write(tostring(arg))
    end
end

local function escaped_redirect_uri_for_service(service) return url.escape(string.replace(redirect_uri, { ['{SERVICE}'] = service } )) end

websrv.server.addhandler{server = server, mstr = '*/login', func = function(session)
    local service = session.Query("service")
    local row = query_row("select redirect_uri, client_id from vg_auths where name = '"..service.."'")
    if row then
        return_redirect(session, string.replace(row.redirect_uri, {
            ['{REDIRECT_URI}'] = escaped_redirect_uri_for_service(service), 
            ['{CLIENT_ID}'] = row.client_id
        }))
    else return_error(session, 400, 'Bad Request') end
end}

websrv.server.addhandler{server = server, mstr = '*/oauth2', func = function(session)
    local service = session.Query("service")
    local code = session.Query("code")
    local row = query_row("select * from vg_auths where name = '"..service.."'")
    if row and string.len(code) > 0 then
        local res, code, headers, status = ssl.https.request(row.token_uri, string.replace(token_request_body, {
            ['{REDIRECT_URI}'] = escaped_redirect_uri_for_service(service), 
            ['{CLIENT_ID}'] = row.client_id,
            ['{CODE}'] = url.escape(code),
            ['{CLIENT_SECRET}'] = row.client_secret,
        }))
        session.write 'Content-type: text/plain\r\n\r\n'
        session.write('res: '..res..'\n\n')
        session.write('code: '..code..'\n\n')
        session.write('headers: '..tostring(headers)..'\n\n')
        session.write('status: '..status..'\n\n')
        
        --[[
        local oauth = json:decode(res)
            
        if 200 == code then
            local response = {} 
            local res, code, headers, status = ssl.https.request({
                url = params.info_uri,
                headers = { Authorization = "Bearer "..oauth.access_token },
                sink = ltn12.sink.table(response),
            })                  
            if 200 == code then
                local info = json:decode(response[1])
                info.expires_in = oauth.expires_in
                oauth2.sessions[oauth.access_token] = info
                return_redirect(session, '/oauth2/ok#service='..service..'&id='..info.id..'&token='..oauth.access_token)
            else return_error(session, 401, 'Unauthorized') end
        else return_error(session, code, res.error_description) end
        --]]
    else return_error(session, 400, 'Bad Request') end
end}

websrv.server.addhandler{server = server, mstr = '*/info', func = function(session)
    local access_token = session.Query("access_token")
    local info = oauth2.sessions[access_token]
    if info then
        session.write 'Content-type: text/html\r\n\r\n'
        session.write(tostring(info))
    else return_error(session, 401, 'Unauthorized') end
end}

websrv.server.addhandler{server = server, mstr = '*/check', func = function(session)
    local state = session.Query("state")
    if string.len(state) > 0 then
        if states[state] then
            session.write 'Content-type: text/plain\r\n\r\n'
            session.write(tostring(states[state])..'\n\n')
            session.write('OK')
        else return_error(session, 401, 'Unauthorized') end
    else return_error(session, 400, 'Bad Request') end
end}


while true do
	websrv.server.run(server)
end

--[[
                session.write 'Content-type: text/plain\r\n\r\n'
                session.write('oauth: '..tostring(oauth)..'\n\n')
                session.write('res: '..res..'\n\n')
                session.write('code: '..code..'\n\n')
                session.write('headers: '..tostring(headers)..'\n\n')
                session.write('status: '..status..'\n\n')
                session.write('response: '..tostring(response)..'\n\n')

--]]                