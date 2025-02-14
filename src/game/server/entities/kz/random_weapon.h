/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_RANDOMPICKUP_H
#define GAME_SERVER_ENTITIES_RANDOMPICKUP_H

#include <game/server/entities/kz/kz_pickup.h>

class CRandomWeapon : public CKZPickup
{
public:
	CRandomWeapon(CGameWorld *pGameWorld, int Layer = 0, int Number = 0);
	void Tick() override;
	void Snap(int SnappingClient) override;

private:

	bool m_ChangedType[NUM_DDRACE_TEAMS];
	int m_Subtype[NUM_DDRACE_TEAMS];


};

#endif
