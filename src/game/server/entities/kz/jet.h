/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_VEHICLE_JET_H
#define GAME_SERVER_ENTITIES_VEHICLE_JET_H

#include <game/server/entity.h>
#include <game/server/entities/character.h>

#include "vehicle_base.h"

class CJet : public CVehicle
{
public:

	CJet(CGameWorld *pGameWorld, vec2 Pos, int Owner = -1, int Layer = 0);
	~CJet();

	virtual void Tick() override;
	virtual void Snap(int SnappingClient) override;

	int m_ExtraLaserIds[3];
	float m_Angle = -1.57f;
	int m_ReloadTimer = 0;
	bool m_Explode = false;

protected:

	virtual void HandleMounterInput(const CNetObj_PlayerInput* Input) override;

};

#endif
