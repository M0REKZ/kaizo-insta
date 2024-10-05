/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "mine.h"

#include <game/server/entities/character.h>

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/teamscore.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <engine/shared/config.h>
//dirty:
#include <game/server/gamecontroller.h>

CMine::CMine(CGameWorld *pGameWorld, vec2 Pos, int Owner, bool active, bool respawn) :
CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_Active = active;
	m_Respawn = respawn;
	
	GameWorld()->InsertEntity(this);
}

void CMine::Tick()
{
	Move();
	
	if(m_RespawnTick)
	{
		m_RespawnTick--;
	}
	else
	{
		CCharacter *apCloseChars[MAX_CLIENTS];
		CCharacter *pChr;
		 int Num = GameServer()->m_World.FindEntities(m_Pos, 10, (CEntity **)apCloseChars, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		for(int i = 0; i < Num; ++i)
		{
			
			if(apCloseChars[i] && apCloseChars[i]->IsAlive() && apCloseChars[i]->GetPlayer()->GetCid() != m_Owner)
			{
				pChr = apCloseChars[i];
				if(m_Active)
				{
					m_Explode = true;
				}
				else
				{
					pChr->m_Mines++;
					if(m_Respawn)
						m_RespawnTick = Server()->TickSpeed() * 20;
					else
						Reset();
				}
				break;
			}
		}
		
		if(m_Explode)
		{
			if(m_Owner < 0)
				GameServer()->CreateExplosion(m_Pos, pChr->GetPlayer()->GetCid(), WEAPON_GRENADE, true, pChr->Team(), pChr->TeamMask());
			else
				GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_GRENADE, true, pChr->Team(), pChr->TeamMask());
			
			//normal explosion does not make damage on instagib so i need to do this, also people cant put mines on instagib
			if(GameServer()->m_pController->m_IsInstagibKZ)
				pChr->DoKZDamage(vec2(0,0), 3, pChr->GetPlayer()->GetCid(), WEAPON_GRENADE);
			
			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
			m_Explode = false;
			if(m_Respawn)
				m_RespawnTick = Server()->TickSpeed() * 20;
			else
				Reset();
		}
	}
}

void CMine::Snap(int SnappingClient)
{
	if(m_RespawnTick)
		return;
	
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Projectile *pObj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, GetId(), sizeof(CNetObj_Projectile)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	if(m_Active)
		pObj->m_Y = (int)m_Pos.y;
	else
		pObj->m_Y = (int)m_Pos.y + 8*sin((float)Server()->Tick() / 25.0);
	pObj->m_VelX = 1;
	pObj->m_VelY = 1;
	pObj->m_Type = WEAPON_LASER;
}

void CMine::Reset()
{
	m_MarkedForDestroy = true;
}

void CMine::Move()
{
	if(Server()->Tick() % (int)(Server()->TickSpeed() * 0.15f) == 0)
	{
		int Flags;
		int index = GameServer()->Collision()->IsMover(m_Pos.x, m_Pos.y, &Flags);
		if(index)
		{
			m_Core = GameServer()->Collision()->CpSpeed(index, Flags);
		}
		m_Pos += m_Core;
	}
}
