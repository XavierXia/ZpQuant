local Class = require 'shared.base.Class'
local ngx = package.loaded['ngx']
local cjson = require "cjson"
local logger = require 'shared.log'

KRDataHandle = Class:new()

logger:info('%s','KRDataHandle begin!')

function KRDataHandle:parse_request(data)
	if not data or #data == 0 then return end
	local value,err = cjson.decode(data)

    --[[
    if not value then
        local value = ngx.req.get_post_args()
        ngx.say("body is post data")
    else
        ngx.say("body is post json")
    end
    ]]
end

function KRDataHandle:handle(ngx)
	ngx.req.read_body()
	local body_str = ngx.req.get_body_data()
    if not body_str then
        ngx.exit(ngx.HTTP_BAD_REQUEST)    
    end

	--local requests = self:parse_request(body_str)
    local current_time = string.format(" %s",ngx.now())
    ngx.log(ngx.ERR,tostring(body_str) .. current_time) 
end

return KRDataHandle

