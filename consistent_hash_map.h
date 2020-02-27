// 封装对一致性哈希的操作
#ifndef __CONSISTENT_HASH_H__
#define __CONSISTENT_HASH_H__

#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <string>
#include <list>
#include <map>
#include <algorithm>
#include "city.h"

#ifdef _WIN32
	#pragma warning(disable:4996)
#endif

#define temporary_buf_size 128

using uint64 = uint64_t;

template<typename T>
inline void SafeDelete(T*& p)
{
	if (p)
	{
		delete p;
		p = nullptr;
	}
}
inline char* local_strdup(const char* in)
{
	assert(in != nullptr);
	char* out = (char*)malloc(strlen(in) + 1);
	return strcpy(out, in);
}

// hash生成器
template <typename T>
struct city_hasher64
{
	using result_type = uint64;

	city_hasher64() = default;
	~city_hasher64() = default;

	// 计算hash
	result_type value(const char* str, size_t size)
	{
		return CityHash64(str, size);
	}
	result_type operator()(const T& node)
	{
		auto hashText = node->to_str();
		return value(hashText.c_str(), hashText.length());
	}
};

// 一致性哈希
template <typename T, 
	typename Hash, 
	typename Alloc = std::allocator< std::pair<const typename Hash::result_type, T> > >
class consistent_hash_map
{
public:
	using key_type = typename Hash::result_type;
	using map_type = typename std::map<key_type, T, std::less<key_type>, Alloc>;
	using value_type = typename map_type::value_type;
	using reference = value_type&;
	using const_reference = const value_type&;
	using iterator = typename map_type::iterator;
	using reverse_iterator = typename map_type::reverse_iterator;
	using allocator_type = Alloc;

public:
	consistent_hash_map() = default;
	~consistent_hash_map() = default;
	
	// hash环上的节点个数
	std::size_t size() const 
	{
		return m_nodes.size();
	}
	// hash环是否为空
	bool empty() const 
	{
		return m_nodes.empty();
	}
	// 插入元素
	std::pair<iterator, bool> insert(const T& node)
	{
		key_type hash = m_hasher(node);
		return m_nodes.insert(value_type(hash, node));
	}
	std::pair<iterator, bool> insert(T&& node)
	{
		key_type hash = m_hasher(node);
		return m_nodes.insert(value_type(hash, std::forward<T>(node)));
	}
	// 删除元素，返回删除后元素个数
	std::size_t erase(const T& node) 
	{
		key_type hash = m_hasher(node);
		return m_nodes.erase(hash);
	}
	// 查找一致性哈希节点
	iterator find(key_type hash)
	{
		if (m_nodes.empty())
		{
			return m_nodes.end();
		}

		iterator it = m_nodes.lower_bound(hash);
		if (it == m_nodes.end())
		{
			it = m_nodes.begin();
		}

		return it;
	}
	// 开始iter
	iterator begin()
	{
		return m_nodes.begin();
	}
	// 结尾iter
	iterator end()
	{
		return m_nodes.end();
	}
	// 开始riter
	reverse_iterator rbegin() 
	{ 
		return m_nodes.rbegin();
	}
	// 结束riter
	reverse_iterator rend()
	{
		return m_nodes.rend();
	}
	// 计算hash
	key_type hash(const T& value)
	{
		return m_hasher(value);
	}
	key_type hash_value(const char* str, size_t size)
	{
		return m_hasher.value(str, size);
	}
	// hash环是否有该节点
	bool count(key_type hash)
	{
		return m_nodes.find(hash) != m_nodes.end();
	}
	// clear
	void clear()
	{
		m_nodes.clear();
	}

private:
	// hash 生成器
	Hash m_hasher;
	// hash 环
	map_type m_nodes;
};

// 节点
struct BaseNode
{
	// 节点名称
	std::string m_nodeName;
	// 当前节点是否在线
	bool m_nodeOnline;

	BaseNode(const std::string& name):
		m_nodeName(name),
		m_nodeOnline(true)
	{

	}
	virtual ~BaseNode() = default;
	
	// 获取节点名称
	virtual std::string to_str()
	{
		return m_nodeName;
	}
	virtual int QueryNode(std::string& name)
	{
		return -2;
	}
	virtual void Destroy() 
	{

	}
};

struct VirualWorkNode;
// 真实节点
struct WorkNode : public BaseNode
{
	WorkNode(const std::string &name):
		BaseNode(name)
	{

	}
	~WorkNode() = default;

	virtual int QueryNode(std::string &name)
	{
		name = m_nodeName;
		return 1;
	}
	virtual void Destroy()
	{
		m_virtualNodes.clear();
	}

	std::list<VirualWorkNode*> m_virtualNodes;
};

// 虚拟节点
struct VirualWorkNode : public BaseNode
{
	VirualWorkNode(WorkNode* workNode, const std::string &name): 
		BaseNode(name),
		m_Node(workNode)
	{

	}
	~VirualWorkNode() = default;

	virtual int QueryNode(std::string &name)
	{
		if (m_Node == nullptr) 
		{
			return -3;
		}

		name = m_Node->m_nodeName;
		return 1;
	}
	virtual void Destroy()
	{
		m_Node = nullptr;
	}

	WorkNode* m_Node;
};

// 一致性哈希的服务器
struct Server
{
	using ValueType = BaseNode*;
	// 一致性哈希的服务器名称
	std::string m_serverName;
	// 虚拟节点的个数
	int m_virtualNodeCount;
	// 虚拟节点格式化字符串
	std::string m_virtualFormatStr;
	// 一致性哈希结构
	consistent_hash_map<ValueType, city_hasher64<ValueType>> m_hashNodes;
	// 真实节点
	std::list<WorkNode*> m_nodes;

	Server(const std::string& name, int virtualNodeCount, const std::string& format): 
		m_serverName(name),
		m_virtualNodeCount(virtualNodeCount),
		m_virtualFormatStr(format)
	{
	
	}
	~Server() = default;

	// 计算hash
	uint64 HashValue(const char* str, size_t size)
	{
		return m_hashNodes.hash_value(str, size);
	}
	// 增加真实节点
	int AddNode(const std::string& nodeName)
	{
		if (nodeName.empty())
		{
			return -1;
		}

		uint64 hashValue = m_hashNodes.hash_value(nodeName.c_str(), nodeName.length());
		if (m_hashNodes.count(hashValue))
		{
			return -2;
		}

		WorkNode* workNode = new WorkNode(nodeName);
		m_nodes.push_back(workNode);
		m_hashNodes.insert(workNode);

		char virtualNodeName[temporary_buf_size] = {0};
		for (int i = 0; i < m_virtualNodeCount; ++i)
		{
			auto len = snprintf(virtualNodeName, temporary_buf_size, "%s_virtual_node_extend%d", nodeName.c_str(), i);
			if (len == temporary_buf_size)
			{
				return -3;
			}
			uint64 value = m_hashNodes.hash_value(virtualNodeName, len);
			if (m_hashNodes.count(value))
			{
				continue;
			}

			VirualWorkNode *virtualWorkNode = new VirualWorkNode(workNode, virtualNodeName);
			workNode->m_virtualNodes.push_back(virtualWorkNode);
			m_hashNodes.insert(virtualWorkNode);
		}

		return 0;
	}
	// 查找hash节点
	int QueryNode(uint64 onlyId, std::string& nodeName)
	{
		auto iter = m_hashNodes.find(onlyId);
		if (iter == m_hashNodes.end()) {
			return -1;
		}

		BaseNode* node = iter->second;
		return node->QueryNode(nodeName);
	} 
	int QueryNode(const std::string& name, std::string& nodeName)
	{
		uint64 hashValue = HashValue(name.c_str(), name.length());
		return QueryNode(hashValue, nodeName);
	}
	// 销毁
	void Destroy()
	{
		for (const auto& elem : m_hashNodes)
		{
			BaseNode* node = elem.second;
			node->Destroy();
			SafeDelete(node);
		}
		m_nodes.clear();
		m_hashNodes.clear();
	}
};

// 创建server
extern "C" void* new_server(const char* name, const char* format, const int virtualCount);
// 删除server
extern "C" void delete_server(Server* p);
// 注册节点
extern "C" int register_node(Server* p, const char* nodeName);
// 查找节点
extern "C" char* query_node(Server* p, const char* onlyIdName);

#endif