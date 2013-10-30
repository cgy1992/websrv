require 'std' -- load lua standard library for pretty printing etc.

pprint = function(arg) print(prettytostring(arg)) end

require 'macadr' -- load browse function from browse0conf module

pprint(macadr) -- print conf0 module to see what we have

--function executes macadr.read with params i safe mode and prints both request and result
function printmac(params) print(params, pcall(macadr.read, params)) end

printmac{ip='192.168.10.83', port=22}
printmac{ip='192.168.10.97', port=80}
printmac{ip='192.168.10.174', port=80}
printmac{ip='192.168.10.100', port=80}
printmac{ip='192.168.10.101', port=80}
printmac{ip='192.168.10.95', port=80}
printmac{ip='192.168.10.2', port=80}
printmac{ip='192.168.10.9', port=80}
printmac{ip='192.168.10.84', port=80}

-- note that port parameter is necessary on linux but is not used on windows
-- also on linux there are optional parameters attempts and timeout 
-- attempts parameter specifies the number of attempts to read in search of packet with mac address
-- timeout parameter specifies the connect timeout in seconds