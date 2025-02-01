#ifndef GAME_SERVER_ENTITIES_MINIGUN_PROJECTILE_H
#define GAME_SERVER_ENTITIES_MINIGUN_PROJECTILE_H

#include <game/server/entity.h>
#include "fdd_stable_projectile.h"

class CMinigunProjectile : public CStableProjectile
{
	vec2 m_Dir;
public:
	CMinigunProjectile(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir, bool HideOnSpec = false, bool OnlyShowOwner = false);

	virtual void Tick() override;
	virtual void Snap(int SnappingClient) override;
	virtual void Reset() override;
};

#endif