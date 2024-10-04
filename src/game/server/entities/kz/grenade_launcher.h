/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_GRENADELAUNCHERKZ_H
#define GAME_SERVER_ENTITIES_GRENADELAUNCHERKZ_H

#include <game/server/entity.h>

class CGrenadeLauncher : public CEntity
{
public:
	CGrenadeLauncher(CGameWorld *pGameWorld, vec2 Pos, vec2 Dir);
	void Tick() override;
	void Snap(int SnappingClient) override;
	void Reset() override;
private:
	int m_FireTick;
	vec2 m_Dir;
};

#endif
