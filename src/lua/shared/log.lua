local Class = require 'shared.base.Class'
local ffi = require('ffi')
local zlog = ffi.load('/root/ZpQuant/lib/libzlog.so')

local LOG_SYS, LOG_DATA = 'sys', 'hqdata_'

ffi.cdef[[
    void dzlog_fini(void);
    int dzlog_init(const char *confpath, const char *cname);
    int dzlog_set_category(const char *cname);
    void dzlog(const char *file, size_t filelen,
    const char *func, size_t funclen,
    long line, int level,const char *format, ...);
]]


logger = Class:new()

function logger:init()
    local log_filename = '/root/ZpQuant/conf' .. 'zlog.conf'
    zlog.dzlog_init(log_filename, LOG_DATA)
end
    
function logger:log(category, fmt, ...)
    zlog.dzlog_set_category(LOG_DATA .. category)
    zlog.dzlog('',0, '',0, 0,20, fmt, ...)
end

function logger:debug(fmt, ...)
    zlog.dzlog_set_category(LOG_SYS)
    zlog.dzlog('',0, '',0, 0,20, fmt, ...)
end

function logger:info(fmt, ...)
    zlog.dzlog_set_category(LOG_SYS)
    zlog.dzlog('',0, '',0, 0,40, fmt, ...)
end

function logger:notice(fmt, ...)
    zlog.dzlog_set_category(LOG_SYS)
    zlog.dzlog('',0, '',0, 0,60, fmt, ...)
end

function logger:warn(fmt, ...)
    zlog.dzlog_set_category(LOG_SYS)
    zlog.dzlog('',0, '',0, 0,80, fmt, ...)
end

function logger:error(fmt, ...)
    zlog.dzlog_set_category(LOG_SYS)
    zlog.dzlog('',0, '',0, 0,100, fmt, ...)
end

logger:init()

return logger

