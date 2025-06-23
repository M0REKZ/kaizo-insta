/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/teamscore.h>

#include <engine/shared/config.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/gamecontroller.h>

#include "blackhole.h"
#include <game/kztiles.h>

static constexpr int gs_PickupPhysSize = 14;

CBlackHole::CBlackHole(CGameWorld *pGameWorld, vec2 Pos, int Owner, int Layer) :
CEntity(pGameWorld,CGameWorld::CUSTOM_ENTTYPE_BLACKHOLE,vec2(0,0),gs_PickupPhysSize)
{
	m_Pos = Pos;
	m_Layer = Layer;
	m_Owner = Owner;
	m_SpawnTick = Server()->Tick();

	GameWorld()->InsertEntity(this);

	if(!GameServer()->GetPlayerChar(m_Owner))
	{
		Reset();
	}
}

void CBlackHole::Reset()
{
	m_MarkedForDestroy = true;
}

void CBlackHole::Tick()
{
	CCharacter* pOwner = GameServer()->GetPlayerChar(m_Owner);

	if(!pOwner)
	{
		Reset();
		return;
	}

	if(g_Config.m_SvBlackholeLife && Server()->Tick()-m_SpawnTick > g_Config.m_SvBlackholeLife * Server()->TickSpeed())
	{
		Reset();
		return;
	}

	// Check if a player intersected us
	CCharacter *pChr = (CCharacter*)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER);//GameWorld()->ClosestCharacter(m_Pos, GetProximityRadius() + ms_CollisionExtraSize, 0);
	for(;pChr;pChr = (CCharacter*)pChr->TypeNext())
	{
		if(pChr && pChr->IsAlive())
		{
			if(pChr->Team() != pOwner->Team())
				continue;
			
			if(distance(pChr->m_Pos,m_Pos) > 500)
				continue;

			pChr->AddVelocity(normalize(m_Pos - pChr->m_Pos));

			if(distance(pChr->m_Pos,m_Pos) < 10)
			{
				pChr->Die(m_Owner,WEAPON_NINJA,true,false);
			}
		}
	}
}

void CBlackHole::Snap(int SnappingClient)
{
	CCharacter* pOwner = GameServer()->GetPlayerChar(m_Owner);

	if(!pOwner)
	{
		return;
	}

	if(Server()->Tick() % 4 == 0)
	{
		if(NetworkClipped(SnappingClient))
			return;

		CCharacter *pChar = GameServer()->GetPlayerChar(SnappingClient);

		int Team = pOwner->Team();

		if(pChar)
		{
			Team = pChar->Team();
		}

		if(pOwner->Team() != Team)
			return;

		GameServer()->CreateDeath(m_Pos,m_Owner,GameServer()->GetPlayerChar(m_Owner)->TeamMask());
	}

}