/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "grenade_launcher.h"

#include <game/server/entities/ddnet_pvp/vanilla_projectile.h>
#include <game/server/entities/character.h>

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/teamscore.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <engine/shared/config.h>
//dirty:
#include <game/server/gamecontroller.h>

CGrenadeLauncher::CGrenadeLauncher(CGameWorld *pGameWorld, vec2 Pos, vec2 Dir) :
CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Pos = Pos;
	m_Dir = Dir;
	m_FireTick = 0;
	
	GameWorld()->InsertEntity(this);
}

void CGrenadeLauncher::Tick()
{
	if(GameServer()->m_World.m_Paused)
		return;
	
	if(m_FireTick)
	{
		m_FireTick--;
	}
	else
	{
		new CVanillaProjectile(&GameServer()->m_World, WEAPON_GRENADE, -1, m_Pos, m_Dir, (int)(Server()->TickSpeed() * GameServer()->Tuning()->m_GrenadeLifetime), false, true, SOUND_GRENADE_EXPLODE, m_Dir);
		m_FireTick = 5 * Server()->TickSpeed();
	}
}

void CGrenadeLauncher::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

}

void CGrenadeLauncher::Reset()
{
	m_MarkedForDestroy = true;
}
