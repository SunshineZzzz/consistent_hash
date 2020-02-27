-- 封装lua层对一致性哈希的操作

local ffi = require("ffi")
-- This is the default C library namespace
local C = ffi.C
local setmetatable = setmetatable
local ngx_re_gmatch, ngx_re_match = ngx.re.gmatch, ngx.re.match
local io_open, io_close = io.open, io.close
local assert = assert
local ok, new_tab = pcall(require, "table.new")
if not ok then
	new_tab = function (narr, nrec) return {} end
end

-- 服务于ffi.load
local function _load_shared_lib(so_name)
	local cpath = package.cpath
	local tried_paths = new_tab(32, 0)
	local i = 1

	local it, err = ngx_re_gmatch(cpath, "[^;]+")
	if not it then
		return nil, err
	end

	while true do
		local m, err = it()
		if err then
			return nil, err
		end

		if not m then
			break
		end

		local fpath
		local path = m[0]
		local r, err = ngx_re_match(path, "(.*/)")
		if r then
			fpath = r[0] .. so_name
			local f = io_open(fpath)
			if f ~= nil then
				io_close(f)
				return ffi.load(fpath)
			end
			tried_paths[i] = fpath
			i = i + 1
		end
	end

	return nil, tried_paths
end

local libchash, err = _load_shared_lib("libchash.so")
assert(libchash ~= nil)

-- Adds multiple C declarations for types or external 
-- symbols (named variables or functions). def must be a Lua string.
ffi.cdef[[
	/* From our library */
	typedef struct Server Server;
	// 创建server
	Server* new_server(const char* name, const char* format, const int virtualCount);
	// 删除server
	void delete_server(Server* p);
	// 注册节点 & 虚拟节点
	int register_node(Server* p, const char* nodeName);
	// 查找节点
	char* query_node(Server* p, const char* onlyIdName);

	/* From the C library */
	// 释放malloc创建的内存
	void free(void*);
]]

-- 一致性哈希对象
local CHash = {}

-- 创建CHash对象
function CHash:new()
	local instance = {
		super = nil,
	}
	setmetatable(instance, {__index = self,})

	return instance
end

-- 解析配置
-- 服务器名称，虚拟节点format，虚拟节点个数
function CHash:init(name, format, virtualCount)
	self.super = libchash.new_server(name, format, virtualCount)
	ffi.gc(self.super, libchash.delete_server)
end

-- 注册节点
function CHash:register(nodeName)
	return libchash.register_node(self.super, nodeName)
end

-- 查找结点
function CHash:query(onlyIdName)
	local name = libchash.query_node(self.super, onlyIdName)
	ffi.gc(name, C.free)
	-- cdata:NULL
	-- 不可以使用 if not name then xxx end
	if name == nil then
		return nil
	end
	return ffi.string(name)
end

return CHash
