/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/teamscore.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/gamecontroller.h>

#include "blackhole_ammo.h"
#include <game/kztiles.h>

static constexpr int gs_PickupPhysSize = 14;

CBlackHoleAmmo::CBlackHoleAmmo(CGameWorld *pGameWorld, vec2 Pos, int Layer, int Number) :
CEntity(pGameWorld,CGameWorld::CUSTOM_ENTTYPE_BLACKHOLE_AMMO,vec2(0,0),gs_PickupPhysSize)
{
	m_Pos = Pos;
	m_Layer = Layer;
	m_Number = Number;

	m_Id2 = Server()->SnapNewId();

	for(int i=0;i < MAX_CLIENTS;i++)
	{
			m_SpawnTickTeam[i] = -1;
	}

	GameWorld()->InsertEntity(this);
}

CBlackHoleAmmo::~CBlackHoleAmmo()
{
	if(m_Id2 != -1)
		Server()->SnapFreeId(m_Id2);
}

void CBlackHoleAmmo::Reset()
{
	m_MarkedForDestroy = true;
}

void CBlackHoleAmmo::Tick()
{
	Move();
	
	for(int i=0;i < MAX_CLIENTS;i++)
	{
		// wait for respawn
		if(m_SpawnTickTeam[i] > 0)
		{
			if(Server()->Tick() > m_SpawnTickTeam[i])
			{
				// respawn
				m_SpawnTickTeam[i] = -1;
				GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SPAWN, GameServer()->m_pController->Teams().TeamMask(i));
			}
			else
				continue;
		}
	}

	// Check if a player intersected us
	CCharacter *pChr = (CCharacter*)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER);//GameWorld()->ClosestCharacter(m_Pos, GetProximityRadius() + ms_CollisionExtraSize, 0);
	for(;pChr;pChr = (CCharacter*)pChr->TypeNext())
	{
		if(pChr && pChr->IsAlive())
		{
			if(!pChr->GetWeaponGot(WEAPON_BLACKHOLE))
				continue;

			if(pChr->Team() < 0 || pChr->Team() >= MAX_CLIENTS)
				continue;

			if(m_SpawnTickTeam[pChr->Team()] > 0)
				continue;

			if(distance(m_Pos,pChr->m_Pos) > (GetProximityRadius() + ms_CollisionExtraSize))
				continue;

			if(m_Number > 0 && !Switchers()[m_Number].m_aStatus[pChr->Team()])
				continue;

			bool Picked = false;
			// player picked us up, is someone was hooking us, let them go
			
			pChr->SetWeaponAmmo(WEAPON_BLACKHOLE,pChr->GetWeaponAmmo(WEAPON_BLACKHOLE)+1);
			GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR, pChr->TeamMask());

			
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "pickup player='%d:%s' item=%d",
					pChr->GetPlayer()->GetCid(), Server()->ClientName(pChr->GetPlayer()->GetCid()), WEAPON_BLACKHOLE);
				GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
				int RespawnTime = 15;
				if(RespawnTime >= 0)
					m_SpawnTickTeam[pChr->Team()] = Server()->Tick() + Server()->TickSpeed() * RespawnTime;
		}
	}
}

void CBlackHoleAmmo::TickPaused()
{
	for(int i = 0;i<MAX_CLIENTS;i++)
	{
		if(m_SpawnTickTeam[i] != -1)
			++m_SpawnTickTeam[i];
	}
}

void CBlackHoleAmmo::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CCharacter *pChar = GameServer()->GetPlayerChar(SnappingClient);

	int Team;

	if(!pChar)
		Team = 0;
	else
		Team = pChar->Team();

	if(Team < 0 || Team >= MAX_CLIENTS)
		return;

	if(!(m_SpawnTickTeam[Team] == -1))
		return;

	int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
	bool Sixup = Server()->IsSixup(SnappingClient);

	if(SnappingClientVersion < VERSION_DDNET_ENTITY_NETOBJS)
	{

		if(SnappingClient != SERVER_DEMO_CLIENT && (GameServer()->m_apPlayers[SnappingClient]->GetTeam() == TEAM_SPECTATORS || GameServer()->m_apPlayers[SnappingClient]->IsPaused()) && GameServer()->m_apPlayers[SnappingClient]->m_SpectatorId != SPEC_FREEVIEW)
			pChar = GameServer()->GetPlayerChar(GameServer()->m_apPlayers[SnappingClient]->m_SpectatorId);

		int Tick = (Server()->Tick() % Server()->TickSpeed()) % 11;
		if(pChar && pChar->IsAlive() && m_Number > 0 && !Switchers()[m_Number].m_aStatus[pChar->Team()] && !Tick)
			return;
	}


	{
			vec2 postemp;
			vec2 veltemp;
					
			postemp.x = m_Pos.x + 10*sin((float)Server()->Tick() / 25.0);
			postemp.y = m_Pos.y + 10*cos((float)Server()->Tick() / 25.0);

			veltemp.x = ((m_Pos.x + 10*sin(((float)Server()->Tick()+1) / 25.0)) - postemp.x);
			veltemp.y = ((m_Pos.y + 10*cos(((float)Server()->Tick()+1) / 25.0)) - postemp.y);
			veltemp = normalize(veltemp);

			CNetObj_Projectile *pProj = Server()->SnapNewItem<CNetObj_Projectile>(GetId());
			if(!pProj)
			{
				return;
			}
			pProj->m_X = postemp.x;
			pProj->m_Y = postemp.y;
			pProj->m_VelX = veltemp.x;
			pProj->m_VelY = veltemp.y;
			pProj->m_StartTick = Server()->Tick();
			pProj->m_Type = WEAPON_GRENADE;
	}

	{
			vec2 postemp;
			vec2 veltemp;
					
			postemp.x = m_Pos.x + 10*sin(((float)Server()->Tick() / 25.0)+180.0);
			postemp.y = m_Pos.y + 10*cos(((float)Server()->Tick() / 25.0)+180.0);

			veltemp.x = ((m_Pos.x + 10*sin((((float)Server()->Tick()+1) / 25.0)+180.0)) - postemp.x);
			veltemp.y = ((m_Pos.y + 10*cos((((float)Server()->Tick()+1) / 25.0)+180.0)) - postemp.y);
			veltemp = normalize(veltemp);

			CNetObj_Projectile *pProj = Server()->SnapNewItem<CNetObj_Projectile>(m_Id2);
			if(!pProj)
			{
				return;
			}
			pProj->m_X = postemp.x;
			pProj->m_Y = postemp.y;
			pProj->m_VelX = veltemp.x;
			pProj->m_VelY = veltemp.y;
			pProj->m_StartTick = Server()->Tick();
			pProj->m_Type = WEAPON_GRENADE;
	}


}

void CBlackHoleAmmo::Move()
{
	if(Server()->Tick() % (int)(Server()->TickSpeed() * 0.15f) == 0)
	{
		Collision()->MoverSpeed(m_Pos.x, m_Pos.y, &m_Core);
		m_Pos += m_Core;
	}
}