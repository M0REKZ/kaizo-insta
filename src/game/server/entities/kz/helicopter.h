/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_VEHICLE_HELICOPTER_H
#define GAME_SERVER_ENTITIES_VEHICLE_HELICOPTER_H

#include <game/server/entity.h>
#include <game/server/entities/character.h>

#include "vehicle_base.h"

class CHelicopter : public CVehicle
{
public:
	static const int ms_CollisionExtraSize = 6;

	CHelicopter(CGameWorld *pGameWorld, vec2 Pos, int Owner = -1, int Layer = 0);
	~CHelicopter();

	virtual void Tick() override;
	virtual void Snap(int SnappingClient) override;

	int m_ExtraLaserIds[11];
	bool m_Direction = false;
	int m_ReloadTimer = 0;

protected:

	virtual void HandleMounterInput(const CNetObj_PlayerInput* Input) override;

};

#endif
