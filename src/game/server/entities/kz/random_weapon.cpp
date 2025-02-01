/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "random_weapon.h"

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/teamscore.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <engine/shared/config.h>
#include <game/server/gamecontroller.h>
#include <game/server/entities/character.h>

static constexpr int gs_PickupPhysSize = 14;

CRandomWeapon::CRandomWeapon(CGameWorld *pGameWorld, int Layer, int Number) :
CKZPickup(pGameWorld,CGameWorld::CUSTOM_ENTTYPE_RANDOM_WEAPON,vec2(0,0),gs_PickupPhysSize)
{
	m_Type = POWERUP_WEAPON;
	//m_Subtype = SubType;

	m_Layer = Layer;
	m_Number = Number;

	int SpawnDelay = m_Type == POWERUP_NINJA ? 90 : 0;

	m_Id2 = Server()->SnapNewId();

	for(int i=0;i < MAX_CLIENTS;i++)
	{
		if(SpawnDelay > 0)
			m_SpawnTickTeam[i] = Server()->Tick() + Server()->TickSpeed() * SpawnDelay;
		else
			m_SpawnTickTeam[i] = -1;

		int rnd = 0;
		while(rnd == WEAPON_HAMMER || rnd == WEAPON_GUN)
		{
			rnd = rand() % NUM_WEAPONS;
			
		}
		m_Subtype[i] = rnd;
		m_ChangedType[i] = true;
	}
	
	GameWorld()->InsertEntity(this);
}

void CRandomWeapon::Tick()
{

	for(int i=0;i < MAX_CLIENTS;i++)
	{
		if(!m_ChangedType[i] && m_SpawnTickTeam[i] >= 0)
		{
			int rnd = 0;
			while(rnd == WEAPON_HAMMER || rnd == WEAPON_GUN)
			{
				rnd = rand() % NUM_WEAPONS;
				
			}
			m_Subtype[i] = rnd;
			m_ChangedType[i] = true;
		}
		if(m_ChangedType[i] && m_SpawnTickTeam[i] == -1)
		{
			m_ChangedType[i] = false;
		}
	}
	
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

				if(m_Type == POWERUP_WEAPON)
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
			if(m_SpawnTickTeam[pChr->Team()] > 0)
				continue;

			if(distance(m_Pos,pChr->m_Pos) > (GetProximityRadius() + ms_CollisionExtraSize))
				continue;

			if(m_Number > 0 && !Switchers()[m_Number].m_aStatus[pChr->Team()])
				continue;

			bool Picked = false;
			// player picked us up, is someone was hooking us, let them go
			

				if(m_Subtype[pChr->Team()] >= 0 && m_Subtype[pChr->Team()] < NUM_WEAPONS && (!pChr->GetWeaponGot(m_Subtype[pChr->Team()]) || pChr->GetWeaponAmmo(m_Subtype[pChr->Team()]) != -1))
				{
					if(pChr->GetWeaponAmmo(m_Subtype[pChr->Team()]) < 10)
					{
						pChr->GiveWeapon(m_Subtype[pChr->Team()], false, 10);

						if(m_Subtype[pChr->Team()] == WEAPON_GRENADE)
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE, pChr->TeamMask());
						else if(m_Subtype[pChr->Team()] == WEAPON_SHOTGUN)
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN, pChr->TeamMask());
						else if(m_Subtype[pChr->Team()] == WEAPON_LASER)
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN, pChr->TeamMask());

						if(pChr->GetPlayer())
							GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCid(), m_Subtype[pChr->Team()]);
						Picked = true;
					}
				}


			if(Picked)
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "pickup player='%d:%s' item=%d",
					pChr->GetPlayer()->GetCid(), Server()->ClientName(pChr->GetPlayer()->GetCid()), m_Type);
				GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
				int RespawnTime = m_Type == POWERUP_NINJA ? 90 : 15;
				if(RespawnTime >= 0)
					m_SpawnTickTeam[pChr->Team()] = Server()->Tick() + Server()->TickSpeed() * RespawnTime;
			}
		}
	}
}

void CRandomWeapon::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CCharacter *pChar = GameServer()->GetPlayerChar(SnappingClient);

	if(!(pChar && m_SpawnTickTeam[pChar->Team()] == -1))
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

	if ((m_Type == POWERUP_HEALTH || m_Type == POWERUP_ARMOR) && m_Subtype[pChar->Team()] == 1 && m_Id2 != -1)
	{
		vec2 pos1, pos2;
		
		pos1.x = (int)m_Pos.x + 16*sin((float)Server()->Tick() / 25.0);
		pos1.y = (int)m_Pos.y + 16*sin((float)Server()->Tick() / 25.0);
		
		pos2.x = (int)m_Pos.x + 16*cos((float)Server()->Tick() / 25.0);
		pos2.y = (int)m_Pos.y + -16*cos((float)Server()->Tick() / 25.0);
		
		GameServer()->SnapPickup(CSnapContext(SnappingClientVersion, Sixup), GetId(), pos1, m_Type, 0, m_Number);
		GameServer()->SnapPickup(CSnapContext(SnappingClientVersion, Sixup), m_Id2, pos2, m_Type, 0, m_Number);
	}
	else if(m_Subtype[pChar->Team()] >=0 && m_Subtype[pChar->Team()] < NUM_WEAPONS)
	{
		vec2 postemp;
				
		postemp.x = m_Pos.x + 32*sin((float)Server()->Tick() / 25.0);
		postemp.y = m_Pos.y + 32*cos((float)Server()->Tick() / 25.0);

		CNetObj_Projectile *pProj = Server()->SnapNewItem<CNetObj_Projectile>(m_Id2);
		if(!pProj)
		{
			return;
		}
		pProj->m_X = postemp.x;
		pProj->m_Y = postemp.y;
		pProj->m_VelX = 0;
		pProj->m_VelY = 0;
		pProj->m_StartTick = Server()->Tick();
		pProj->m_Type = WEAPON_HAMMER;
		GameServer()->SnapPickup(CSnapContext(SnappingClientVersion, Sixup), GetId(), m_Pos, m_Type, m_Subtype[pChar->Team()], m_Number);
	}
}