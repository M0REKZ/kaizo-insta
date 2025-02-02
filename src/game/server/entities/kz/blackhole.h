/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_BLACKHOLE_H
#define GAME_SERVER_ENTITIES_BLACKHOLE_H

#include <game/server/entity.h>

class CBlackHole : public CEntity
{
public:
	static const int ms_CollisionExtraSize = 6;

	CBlackHole(CGameWorld *pGameWorld, vec2 Pos, int Owner, int Layer = 0);
	//CBlackHoleAmmo(CGameWorld *pGameWorld, int Objtype, vec2 Pos = vec2(0, 0), int ProximityRadius = 0);

	void Reset() override;
	void Tick() override;
	void Snap(int SnappingClient) override;

	int m_SpawnTickTeam[MAX_CLIENTS];
	
	int m_Id2; //+KZ from Pointer tw+

	int GetSpawnTick(int Team = 0) { return m_SpawnTickTeam[Team]; } //+KZ

protected:

	int m_Owner;

	// DDRace
	vec2 m_Core;
};

#endif
