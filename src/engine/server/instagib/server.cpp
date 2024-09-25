#include <base/system.h>
#include <engine/shared/protocol.h>

#include "../server.h"

int CServer::NumConnectedIps() const
{
	unsigned char ConnectedIps[MAX_CLIENTS][16] = {{0}};
	int Count = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aClients[i].m_State == CClient::STATE_EMPTY)
			continue;

		int Size = m_NetServer.ClientAddr(i)->type == NETTYPE_IPV4 ? 4 : 16;
		bool Duplicate = false;

		for(int k = 0; k < MAX_CLIENTS; k++)
		{
			if(k == i)
				continue;
			if(m_aClients[k].m_State == CClient::STATE_EMPTY)
				continue;

			if(!mem_comp(ConnectedIps[k], m_NetServer.ClientAddr(i)->ip, Size))
			{
				Duplicate = true;
				break;
			}

			mem_copy(ConnectedIps[i], m_NetServer.ClientAddr(i)->ip, Size);
		}

		if(!Duplicate)
			Count++;
	}
	return Count;
}
