local assert = assert
local chash = require("consistent_hash")
local chash_ml = chash:new()

chash_ml:init("my_login_svr", "%s_virtual_node_extend%", 50)

local value = 1
for i=0, value-1, 1 do
	local r = chash_ml:register("login_svr" .. (i+1))
	assert(r == 0)
end

local r = chash_ml:query("1159395874189066240")
print(r)
assert(r ~= nil)