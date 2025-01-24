/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_KZ_PICKUP_H
#define GAME_SERVER_ENTITIES_KZ_PICKUP_H

#include <game/server/entity.h>
#include <game/server/entities/ddnet_pvp/vanilla_pickup.h>

class CKZPickup : public CVanillaPickup
{
public:
	CKZPickup(CGameWorld *pGameWorld, int Type, int SubType = 0, int Layer = 0, int Number = 0);
	virtual ~CKZPickup();

	void Reset() override;
	void Tick() override;
	void TickPaused() override;
	void Snap(int SnappingClient) override;

	int m_SpawnTickTeam[MAX_CLIENTS];
	
	int m_Id2; //+KZ from Pointer tw+
};

#endif
