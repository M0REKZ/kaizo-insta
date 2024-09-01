/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "random_weapon.h"

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/teamscore.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <engine/shared/config.h>
//dirty:
#include <game/server/gamecontroller.h>

CRandomWeapon::CRandomWeapon(CGameWorld *pGameWorld, int Type, int SubType, int Layer, int Number) :
CVanillaPickup(pGameWorld, Type, SubType, Layer, Number)
{
	m_ChangedType = false;
}

void CRandomWeapon::Tick()
{

	if(!m_ChangedType && m_SpawnTick >= 0)
	{
		int rnd = 0;
		while(rnd == WEAPON_HAMMER || rnd == WEAPON_GUN)
		{
			rnd = rand() % NUM_WEAPONS;
			
		}
		m_Subtype = rnd;
		m_ChangedType = true;
	}
	if(m_ChangedType && m_SpawnTick == -1)
	{
		m_ChangedType = false;
	}
	
	CVanillaPickup::Tick();
}
