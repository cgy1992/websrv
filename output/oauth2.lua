require 'std'
require 'websrv'
require "https"
require 'pgsql'
local url = require "url"
local ltn12 = require "ltn12"
local json = require 'json'

local folder = 'f:/github/websrv/output/'
local states = {}

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

local oauth2 =
{
    access_token_body_pattern = 'code={CODE}&client_id={CLIENT_ID}&client_secret={CLIENT_SECRET}&redirect_uri={REDIRECT_URI}&grant_type=authorization_code',
    redirect_uri = 'https://zalupa.org/oauth2/',
    services = 
    {
        yandex =
        {
            enabled = true,
            href_pattern = 'https://oauth.yandex.ru/authorize?response_type=code&client_id={CLIENT_ID}&state={STATE}',
            client_id = 'd2f8ddcb159d4da1b26987c686e98409',
            client_secret = 'be22fb5f63ec451caf505020cf42527e',
            access_token_uri = 'https://oauth.yandex.ru/token',
            info_uri = 'https://login.yandex.ru/info?format=json'
        },
        google =
        {
            enabled = true,
            href_pattern = 'https://accounts.google.com/o/oauth2/auth?redirect_uri={REDIRECT_URI}&response_type=code&client_id={CLIENT_ID}&scope=email&state={STATE}',
            client_id = '1003457679327-gu20cis8v037ul2jvdi5rtg71mvf9qsg.apps.googleusercontent.com',
            client_secret = '0322hnJ9OIb8ZwRzyjk35i_W',
            access_token_uri = 'https://accounts.google.com/o/oauth2/token',
            info_uri = 'https://www.googleapis.com/oauth2/v2/userinfo?access_token={ACCESS_TOKEN}'
        },
        facebook =
        {
            enabled = false,
            href_pattern = 'https://www.facebook.com/dialog/oauth?client_id={CLIENT_ID}&redirect_uri={REDIRECT_URI}&response_type=code&state={STATE}',
            client_id = '740409662718765',
            client_secret = 'f8df3185f4c62b719a8acbaffe1d8d26',
            access_token_uri = 'https://accounts.google.com/o/oauth2/token',
        }
    }
}

local server = websrv.server.init{port = 443, file = folder..'test.log', flags = websrv.flags.USESSL, cert = folder..'foo-cert.pem'}

local function return_error(session, code, text)
    websrv.client.HTTPdirective('HTTP/1.1 '..code..' '..text)
    session.write('Content-type: text/html\r\n\r\n')
    session.write('\n')
    session.write(text)
    session.write('\n\n')        
end

websrv.server.addhandler{server = server, mstr = '*/login', func = function(session)
    local state = session.Query("state")
    if string.len(state) > 0 then
        session.write 'Content-type: text/html\r\n\r\n'
        session.write '<HTML><BODY bgcolor="EFEFEF">'
        for service, params in pairs(oauth2.services) do
            if params.enabled then
                local href = string.replace(params.href_pattern, {
                    ['{REDIRECT_URI}'] = url.escape(oauth2.redirect_uri..service), 
                    ['{CLIENT_ID}'] = params.client_id,
                    ['{STATE}'] = state,
                })
                session.write('<a href="'..href..'" title="enter via '..service..'">enter via '..service..'</a><br/>')
            end
        end
        states[state] = {}
    else return_error(session, 400, 'Bad Request') end
end}

websrv.server.addhandler{server = server, mstr = '*/oauth2/*', func = function(session)
    local path = url.parse_path(session.request)
    local service = path[#path]
    local state = session.Query("state")
    local code = session.Query("code")
    if oauth2.services[service] and string.len(state) > 0 and string.len(code) > 0 then
        if states[state] then
            local params = oauth2.services[service]

            local body = string.replace(params.access_token_body_pattern or oauth2.access_token_body_pattern, {
                ['{REDIRECT_URI}'] = url.escape(oauth2.redirect_uri..service), 
                ['{CLIENT_ID}'] = params.client_id,
                ['{CODE}'] = url.escape(code),
                ['{CLIENT_SECRET}'] = params.client_secret,
            })
            
            local res, code, headers, status = ssl.https.request(params.access_token_uri, body)
            
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
                    session.write 'Content-type: text/plain\r\n\r\n'
                    session.write(tostring(info)..'\n\n')
                    session.write(state)
                    states[state] = info
                else return_error(session, 401, 'Unauthorized') end
            else return_error(session, code, res.error_description) end
        else return_error(session, 401, 'Unauthorized') end
    else return_error(session, 400, 'Bad Request') end
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