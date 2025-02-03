/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/teamscore.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/gamecontroller.h>

#include "vehicle_base.h"
#include <game/kztiles.h>

CVehicle::CVehicle(CGameWorld *pGameWorld, vec2 Pos, int Owner, int Layer) :
CEntity(pGameWorld,CGameWorld::CUSTOM_ENTTYPE_VEHICLE_BASE,vec2(0,0))
{
	m_Owner = Owner;
	m_StartPos = m_Pos = Pos;
	m_Vel = vec2(0,0);
	m_Size = vec2(0,0);
	m_MountOffset = vec2(0,0);
	m_pMounter = nullptr;
	m_MountDistance = 0.f;

	//GameWorld()->InsertEntity(this);

	if(m_Owner != -1 && !GameServer()->GetPlayerChar(m_Owner))
	{
		Reset();
	}
}

void CVehicle::Reset()
{
	UnMount();
	m_MarkedForDestroy = true;
}

void CVehicle::Tick()
{
	if(m_Owner != -1 && !GameServer()->GetPlayerChar(m_Owner))
	{
		Reset();
		return;
	}

	if(m_pMounter)
	{
		if(!m_pMounter->IsAlive())
		{
			UnMount();
			return;
		}
		vec2 pos = m_Pos + m_MountOffset;
		m_pMounter->m_Pos = pos;
		m_pMounter->m_PrevPos = pos;
		((CCharacterCore*)m_pMounter->Core())->m_Pos = pos;
		((CCharacterCore*)m_pMounter->Core())->m_Vel = vec2(0,0);
		((CCharacterCore*)m_pMounter->Core())->SetMoveRestrictions((CANTMOVE_DOWN|CANTMOVE_LEFT|CANTMOVE_RIGHT|CANTMOVE_UP));
		HandleMounterInput(m_pMounter->GetInput());
	}
	else
	{
		for(CCharacter* pChr = (CCharacter*)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER);pChr;pChr = (CCharacter*)pChr->TypeNext())
		{
			if(!pChr->IsAlive())
				continue;

			if(pChr->Core()->m_Mounted)
				continue;

			if(distance(m_Pos,pChr->m_Pos) > m_MountDistance)
				continue;
			
			GameServer()->SendBroadcast("Press 'Fire' to mount",pChr->GetPlayer()->GetCid());

			if(pChr->GetInput()->m_Fire & 1)
			{
				m_pMounter = pChr;
				((CCharacterCore*)m_pMounter->Core())->m_Mounted = true;
				break;
			}
		}
	}

	if(m_Vel.x || m_Vel.y)
	{
		Collision()->MoveBox(&m_Pos,&m_Vel,m_Size,vec2(0.f,0.f));
		Collision()->PushBoxOutsideQuads(&m_Pos,m_Size,&m_CollidedSides);
	}

	if((GameServer()->Collision()->GetCollisionAt(m_Pos.x, m_Pos.y) == TILE_DEATH) || (GameServer()->Collision()->GetFrontCollisionAt(m_Pos.x, m_Pos.y) == TILE_DEATH) || GameLayerClipped(m_Pos))
	{
		if(m_Owner != -1)
			Reset();
		else
			m_Pos = m_StartPos;
	}
}

void CVehicle::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

}

void CVehicle::UnMount()
{
	if(m_pMounter)
	{
		if(m_pMounter->IsAlive())
			((CCharacterCore*)m_pMounter->Core())->m_Mounted = false;
		m_pMounter = nullptr;
	}
}

void CVehicle::HandleMounterInput(const CNetObj_PlayerInput* Input)
{
}
