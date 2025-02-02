/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_BLACKHOLE_AMMO_H
#define GAME_SERVER_ENTITIES_BLACKHOLE_AMMO_H

#include <game/server/entity.h>
#include <game/server/entities/ddnet_pvp/vanilla_pickup.h>

class CBlackHoleAmmo : public CEntity
{
public:
	static const int ms_CollisionExtraSize = 6;

	CBlackHoleAmmo(CGameWorld *pGameWorld, vec2 Pos, int Layer = 0, int Number = 0);
	//CBlackHoleAmmo(CGameWorld *pGameWorld, int Objtype, vec2 Pos = vec2(0, 0), int ProximityRadius = 0);
	virtual ~CBlackHoleAmmo();

	void Reset() override;
	void Tick() override;
	void TickPaused() override;
	void Snap(int SnappingClient) override;

	int m_SpawnTickTeam[MAX_CLIENTS];
	
	int m_Id2; //+KZ from Pointer tw+

	int GetSpawnTick(int Team = 0) { return m_SpawnTickTeam[Team]; } //+KZ

protected:

	// DDRace

	void Move();
	vec2 m_Core;
};

#endif
