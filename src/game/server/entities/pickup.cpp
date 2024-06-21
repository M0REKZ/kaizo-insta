/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "pickup.h"
#include "character.h"

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/teamscore.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <engine/shared/config.h>
//dirty:
#include <game/server/gamecontroller.h>

static constexpr int gs_PickupPhysSize = 14;

CPickup::CPickup(CGameWorld *pGameWorld, int Type, int SubType, int Layer, int Number) :
	CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP, vec2(0, 0), gs_PickupPhysSize)
{
	m_Core = vec2(0.0f, 0.0f);
	m_Type = Type;
	m_Subtype = SubType;

	m_Layer = Layer;
	m_Number = Number;
    
    if(GameServer()->m_pController->m_VanillaBehavior) //JSAURUS
    {
        m_SpawnTick = -1;

        if(m_Type == POWERUP_NINJA)
            m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * 90;
    }
    
	GameWorld()->InsertEntity(this);
}

void CPickup::Reset()
{
	m_MarkedForDestroy = true;
}

void CPickup::Tick()
{
    if(GameServer()->m_pController->m_VanillaBehavior)
    {
        if(m_SpawnTick > 0)
        {
            if(Server()->Tick() > m_SpawnTick)
            {
                // respawn
                m_SpawnTick = -1;

                if(m_Type == POWERUP_WEAPON)
                    GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SPAWN);
            }
            else
                return;
        }

        if(!Config()->m_SvSpawnNinja && m_Type == POWERUP_NINJA)
            m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * 90;
    }
    
	Move();

	// Check if a player intersected us
	CEntity *apEnts[MAX_CLIENTS];
	int Num = GameWorld()->FindEntities(m_Pos, GetProximityRadius() + ms_CollisionExtraSize, apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
	for(int i = 0; i < Num; ++i)
	{
		auto *pChr = static_cast<CCharacter *>(apEnts[i]);

        //if(GameServer()->m_pController->m_VanillaBehavior) //JSAURUS
            int RespawnTime = -1;
        
		if(pChr && pChr->IsAlive())
		{
			if(m_Layer == LAYER_SWITCH && m_Number > 0 && !Switchers()[m_Number].m_aStatus[pChr->Team()])
				continue;
            
            //if(!GameServer()->m_pController->m_VanillaBehavior) //JSAURUS
                bool Sound = false;
            
			// player picked us up, is someone was hooking us, let them go
			switch(m_Type)
			{
			case POWERUP_HEALTH:
                if(!GameServer()->m_pController->m_VanillaBehavior)
                {
                    if(pChr->Freeze())
                        GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH, pChr->TeamMask());
                }
                else //JSAURUS..
                {
                    if(pChr->IncreaseHealth(1))
                        {
                            GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
                            RespawnTime = 15; //todo, not hardcode >:(
                        }
                }
				break;

			case POWERUP_ARMOR:
                if(!GameServer()->m_pController->m_VanillaBehavior)
                {
                    if(pChr->Team() == TEAM_SUPER)
                        continue;
                    for(int j = WEAPON_SHOTGUN; j < NUM_WEAPONS; j++)
                    {
                        if(pChr->GetWeaponGot(j))
                        {
                            pChr->SetWeaponGot(j, false);
                            pChr->SetWeaponAmmo(j, 0);
                            Sound = true;
                        }
                    }
                    pChr->SetNinjaActivationDir(vec2(0, 0));
                    pChr->SetNinjaActivationTick(-500);
                    pChr->SetNinjaCurrentMoveTime(0);
                    if(Sound)
                    {
                        pChr->SetLastWeapon(WEAPON_GUN);
                        GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR, pChr->TeamMask());
                    }
                    if(pChr->GetActiveWeapon() >= WEAPON_SHOTGUN)
                        pChr->SetActiveWeapon(WEAPON_HAMMER);
                }
                else //JSAURUS
                {
                    if(pChr->IncreaseArmor(1))
                        {
                            GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
                            RespawnTime = 15; //todo, not hardcode >:(
                        }
                }
				break;

			case POWERUP_ARMOR_SHOTGUN:
				if(pChr->Team() == TEAM_SUPER)
					continue;
				if(pChr->GetWeaponGot(WEAPON_SHOTGUN))
				{
					pChr->SetWeaponGot(WEAPON_SHOTGUN, false);
					pChr->SetWeaponAmmo(WEAPON_SHOTGUN, 0);
					pChr->SetLastWeapon(WEAPON_GUN);
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR, pChr->TeamMask());
				}
				if(pChr->GetActiveWeapon() == WEAPON_SHOTGUN)
					pChr->SetActiveWeapon(WEAPON_HAMMER);
				break;

			case POWERUP_ARMOR_GRENADE:
				if(pChr->Team() == TEAM_SUPER)
					continue;
				if(pChr->GetWeaponGot(WEAPON_GRENADE))
				{
					pChr->SetWeaponGot(WEAPON_GRENADE, false);
					pChr->SetWeaponAmmo(WEAPON_GRENADE, 0);
					pChr->SetLastWeapon(WEAPON_GUN);
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR, pChr->TeamMask());
				}
				if(pChr->GetActiveWeapon() == WEAPON_GRENADE)
					pChr->SetActiveWeapon(WEAPON_HAMMER);
				break;

			case POWERUP_ARMOR_NINJA:
				if(pChr->Team() == TEAM_SUPER)
					continue;
				pChr->SetNinjaActivationDir(vec2(0, 0));
				pChr->SetNinjaActivationTick(-500);
				pChr->SetNinjaCurrentMoveTime(0);
				break;

			case POWERUP_ARMOR_LASER:
				if(pChr->Team() == TEAM_SUPER)
					continue;
				if(pChr->GetWeaponGot(WEAPON_LASER))
				{
					pChr->SetWeaponGot(WEAPON_LASER, false);
					pChr->SetWeaponAmmo(WEAPON_LASER, 0);
					pChr->SetLastWeapon(WEAPON_GUN);
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR, pChr->TeamMask());
				}
				if(pChr->GetActiveWeapon() == WEAPON_LASER)
					pChr->SetActiveWeapon(WEAPON_HAMMER);
				break;

			case POWERUP_WEAPON:
                    
				if(m_Subtype >= 0 && m_Subtype < NUM_WEAPONS && (!pChr->GetWeaponGot(m_Subtype) || (GameServer()->m_pController->m_VanillaBehavior ? (pChr->GetWeaponAmmo(m_Subtype) != -1 && pChr->GetWeaponAmmo(m_Subtype) != 10) : pChr->GetWeaponAmmo(m_Subtype) != -1))) //VanillaBehavior added because JSaurus
				{
                    if(GameServer()->m_pController->m_VanillaBehavior) //JSAURUS
                    {
                        RespawnTime = 15;
                        pChr->GiveWeapon(m_Subtype, false, 10);
                    }
                    else
                    {
                        pChr->GiveWeapon(m_Subtype);
                    }
                    
					if(m_Subtype == WEAPON_GRENADE)
						GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE, pChr->TeamMask());
					else if(m_Subtype == WEAPON_SHOTGUN)
						GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN, pChr->TeamMask());
					else if(m_Subtype == WEAPON_LASER)
						GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN, pChr->TeamMask());

					if(pChr->GetPlayer())
						GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCid(), m_Subtype);
				}
				break;

			case POWERUP_NINJA:
			{
				// activate ninja on target player
				pChr->GiveNinja();
                
                if(GameServer()->m_pController->m_VanillaBehavior) //JSAURUS
                {
                    CClientMask mask;
                    GameServer()->CreateSound(m_Pos, SOUND_PICKUP_NINJA, pChr->TeamMask());

                    RespawnTime = 90;

                    CCharacter *pC = static_cast<CCharacter *>(GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER));
                    for(; pC; pC = (CCharacter *)pC->TypeNext())
                    {
                        if (pC != pChr)
                            pC->SetEmote(EMOTE_SURPRISE, Server()->Tick() + Server()->TickSpeed());
                    }

                    pChr->SetEmote(EMOTE_ANGRY, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
                }
                
				break;
			}
			default:
				break;
			};
        if(GameServer()->m_pController->m_VanillaBehavior) //JSAURUS
        {
            if(RespawnTime >= 0)
            {
                char aBuf[256];
                str_format(aBuf, sizeof(aBuf), "pickup player='%d:%s' item=%d/%d",
                    pChr->GetPlayer()->GetCid(), Server()->ClientName(pChr->GetPlayer()->GetCid()), m_Type, m_Subtype);
                GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
                m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * RespawnTime;
            }
        }
		}
	}
}

void CPickup::TickPaused()
{
}

void CPickup::Snap(int SnappingClient)
{
    if(GameServer()->m_pController->m_VanillaBehavior) //JSAURUS
    {
        if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
            return;
    }
    else
    {
        if(NetworkClipped(SnappingClient))
            return;
    }

	int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
	bool Sixup = Server()->IsSixup(SnappingClient);

	if(SnappingClientVersion < VERSION_DDNET_ENTITY_NETOBJS)
	{
		CCharacter *pChar = GameServer()->GetPlayerChar(SnappingClient);

		if(SnappingClient != SERVER_DEMO_CLIENT && (GameServer()->m_apPlayers[SnappingClient]->GetTeam() == TEAM_SPECTATORS || GameServer()->m_apPlayers[SnappingClient]->IsPaused()) && GameServer()->m_apPlayers[SnappingClient]->m_SpectatorId != SPEC_FREEVIEW)
			pChar = GameServer()->GetPlayerChar(GameServer()->m_apPlayers[SnappingClient]->m_SpectatorId);

		int Tick = (Server()->Tick() % Server()->TickSpeed()) % 11;
		if(pChar && pChar->IsAlive() && m_Layer == LAYER_SWITCH && m_Number > 0 && !Switchers()[m_Number].m_aStatus[pChar->Team()] && !Tick)
			return;
	}

	GameServer()->SnapPickup(CSnapContext(SnappingClientVersion, Sixup), GetId(), m_Pos, m_Type, m_Subtype, m_Number);
}

void CPickup::Move()
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
