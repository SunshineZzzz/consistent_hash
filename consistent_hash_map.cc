#include "consistent_hash_map.h"

extern "C" void* new_server(const char* name, const char* format, const int virtualCount)
{
	assert(name != nullptr && format != nullptr && virtualCount > 0);
	Server* p = new Server(name, virtualCount, format);
	assert(p != nullptr);
	return static_cast<void*>(p);
}

extern "C" void delete_server(Server* p)
{
	assert(p != nullptr);
	p->Destroy();
	SafeDelete(p);
}

extern "C" int register_node(Server* p, const char* nodeName)
{
	assert(p != nullptr && nodeName != nullptr);
	return p->AddNode(nodeName);
}

extern "C" char* query_node(Server* p, const char* onlyIdName) 
{
	assert(p != nullptr && onlyIdName != nullptr);
	std::string nodeName;
	auto rst = p->QueryNode(onlyIdName, nodeName);
	if (rst != 1 || nodeName.empty()) 
	{
		return nullptr;
	}
	// 注意这里申请内存了，lua层别忘记释放内存
	return local_strdup(nodeName.c_str());
}
