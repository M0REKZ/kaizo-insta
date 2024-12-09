// #include <base/system.h>
// #include <engine/shared/protocol.h>
//
#include "../server.h"

void CServer::AddMapToRandomPool(const char *pMap)
{
	m_vMapPool.emplace_back(pMap);
}

void CServer::ClearRandomMapPool()
{
	m_vMapPool.clear();
}

const char *CServer::GetRandomMapFromPool()
{
	if(m_vMapPool.empty())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", "map pool is empty add one with 'add_map_to_pool [map name]'");
		return "";
	}

	int RandIdx = secure_rand() % m_vMapPool.size();
	const char *pMap = m_vMapPool[RandIdx].c_str();

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "Chose random map '%s' out of %d maps", pMap, m_vMapPool.size());
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", aBuf);
	return pMap;
}
