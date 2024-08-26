/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_RANDOMPICKUP_H
#define GAME_SERVER_ENTITIES_RANDOMPICKUP_H

#include "pickup.h"

class CRandomWeapon : public CPickup
{
public:
	CRandomWeapon(CGameWorld *pGameWorld, int Type, int SubType = 0, int Layer = 0, int Number = 0);
	void Tick() override;

private:

	bool m_ChangedType;

};

#endif
