require 'std'
require 'websrv'
require 'https'
require 'pgsql'
local url = require 'url'
local ltn12 = require 'ltn12'
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
local redirect_uri = 'https://zalupa.org/oauth2/callback?service={SERVICE}'

local connectionString = 'host=localhost dbname=vg user=undwad password=joder connect_timeout=5 keepalives=1 keepalives_idle=5 keepalives_interval=2 keepalives_count=5'

local connection, err = pg.connect(connectionString) 

local function query_row(sql)
	local res, err = connection:exec(sql)
	if err then error(err) end
    return res:fetch(), res
end

local sessions = {}

local server = websrv.server.init{port = 443, file = folder..'test.log', flags = websrv.flags.USESSL, cert = folder..'foo-cert.pem'}

local function return_error(session, code, text, description)
    text = tostring(text)
    description = description and tostring(description) or text
    websrv.client.HTTPdirective('HTTP/1.1 '..code..' '..text)
    session.write('Content-type: text/html\r\n\r\n')
    session.write[[
        <html>
        <head>
        <style>
        h1 {text-align:center;}
        p {text-align:center;}
        </style>
        </head>
        <body>    
    ]]
    session.write('<h1>'..text..'</h1>')
    session.write('<p>'..description..'</p>')
    session.write[[
        </body>
        </html>
    ]]
end

local function return_text(session, text)
    websrv.client.HTTPdirective('HTTP/1.1 200 OK')
    session.write('Content-type: text/plain\r\n\r\n')
    session.write(tostring(text))
end

local function return_redirect(session, to)
    session.write('Content-type: text/html\r\n\r\n')
    session.write('<script language="JavaScript" type="text/javascript">location.href="'..to..'"</script>')
end

local function return_json(session, object)
    session.write('Content-type: text/json\r\n\r\n')
    session.write(json:encode(object))
end

local function return_rows(session, res)
    local result = {}
    while true do
        local row = res:fetch()
        if not row then break end
        result[#result + 1] = row
    end
    return_json(session, result)
end

local function return_args(session, ...)
    session.write('Content-type: text/plain\r\n\r\n')
    for i,arg in ipairs{...} do
        session.write(i..': '..tostring(arg)..'\n\n')
    end
end

local function escaped_redirect_uri_for_service(service) return url.escape(string.replace(redirect_uri, { ['{SERVICE}'] = service } )) end

websrv.server.addhandler{server = server, mstr = '*/oauth2/login', func = function(session)
    local service = session.Query("service")
    local row = query_row("select redirect_uri, client_id from vg_services where name = '"..service.."'")
    if row then
        return_redirect(session, string.replace(row.redirect_uri, {
            ['{REDIRECT_URI}'] = escaped_redirect_uri_for_service(service), 
            ['{CLIENT_ID}'] = row.client_id
        }))
    else return_error(session, 400, 'Bad Request') end
end}

websrv.server.addhandler{server = server, mstr = '*/oauth2/callback', func = function(session)
    local service = session.Query("service")
    local code = session.Query("code")
    local row = query_row("select * from vg_services where name = '"..service.."'")
    if row and string.len(code) > 0 then
        local res, code, headers, status = ssl.https.request(row.token_uri, string.replace(token_request_body, {
            ['{REDIRECT_URI}'] = escaped_redirect_uri_for_service(service), 
            ['{CLIENT_ID}'] = row.client_id,
            ['{CODE}'] = url.escape(code),
            ['{CLIENT_SECRET}'] = row.client_secret,
        }))
        if 200 == code then
            local token = json:decode(res)
            local info = {} 
            local res, code, headers, status = ssl.https.request({
                url = row.info_uri,
                headers = { Authorization = "Bearer "..token.access_token },
                sink = ltn12.sink.table(info),
            })       
            if 200 == code then
                local info = json:decode(info[1])
                local sql = string.format
                (
                    "select vg_int_register_account_token('%s', '%s', '%s', '%s', '%s', '%s')", 
                    service, 
                    info[row.id_key], 
                    info[row.email_key], 
                    info[row.name_key], 
                    token.access_token, 
                    token.expires_in..' seconds'
                )
                local result = query_row(sql)
                return return_redirect(session, '/oauth2/ok#token='..result[1])
            end
        end
        return return_error(session, 400, 'Bad Request', res)
    end 
    return return_error(session, 400, 'Bad Request')
end}

websrv.server.addhandler{server = server, mstr = '*/oauth2/ok', func = function(session)
    return_error(session, 200, 'OK', 'SUCCESS')
end}

websrv.server.addhandler{server = server, mstr = '*/buildings', func = function(session)
    local limit = session.Query("limit")
	local res, err = connection:exec('select ogc_fid from integra_features order by ogc_fid limit '..limit)
	if err then return return_error(session, 400, 'Bad Request', err) end
    return_rows(session, res)
end}

while true do
	websrv.server.run(server)
end

