#include "pointer_ai.h"
#include <game/server/entities/flag.h>
#include <game/server/entities/ddnet_pvp/vanilla_pickup.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/gamecontroller.h>

CPointerBotAI::CPointerBotAI(CCharacter * pChr) : CBaseKZBotAI(pChr)
{
	m_AIType = POINTERBOT_AI;
}

void CPointerBotAI::HandleInput(CNetObj_PlayerInput &Input)
{
	Input.m_Direction = 0;
	Input.m_TargetX = (rand() % 128) - 64; // look randomly
	Input.m_TargetY = (rand() % 128) - 64;
	Input.m_Jump = false;
	Input.m_Fire = true;
	if (!GameServer()->m_pController->m_IsInstagibKZ) // make non-automatic weapons work, but still have ammo reload work in instagib
		Input.m_Fire = Server()->Tick() % (2) == 1;
	Input.m_Hook = false;
	Input.m_PlayerFlags = PLAYERFLAG_PLAYING;
	Input.m_WantedWeapon = WEAPON_GUN+1;
	if (GameServer()->m_pController->m_IsInstagibKZ)
	{
		if(GameServer()->m_pController->m_pGameType[0] == 'i')
			Input.m_WantedWeapon = WEAPON_LASER+1;
		else if (GameServer()->m_pController->m_pGameType[0] == 'g')
			Input.m_WantedWeapon = WEAPON_GRENADE+1;
	}
	Input.m_NextWeapon = WEAPON_GUN+1;
	Input.m_PrevWeapon = WEAPON_GUN+1;
	// move, and occasionally jump
	if (rand() % (Server()->TickSpeed()*2) == 1)
		m_botDirectionPointer = -m_botDirectionPointer;
	Input.m_Direction = m_botDirectionPointer;
	if (rand() % (Server()->TickSpeed()*2) == 1)
		Input.m_Jump = true;
	
	
	if (GameServer()->Server()->Tick() % (Server()->TickSpeed()) == 1)
	{
		// get a new aggro
		float smallestDistance = 850.0;
		m_botAggroPointer = -1;
		
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (i != m_pPlayer->GetCid() && GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter()) {
				vec2 pos = GameServer()->m_apPlayers[i]->GetCharacter()->m_Pos;
				float d = sqrt((pos.x - m_pPos->x)*(pos.x - m_pPos->x) + (1.35)*(pos.y - m_pPos->y)*(pos.y - m_pPos->y));
				// vertical distance is multiplied by a factor, since screens are larger horizontally
				if (d < smallestDistance && !(GameServer()->m_pController->IsTeamPlay() && GameServer()->m_apPlayers[i]->GetTeam() == m_pPlayer->GetTeam())) {
					smallestDistance = d;
					m_botAggroPointer = i;
				}
			}
		}
	}
	if (m_botAggroPointer == -1)
	{
		m_ticksSinceFirePointer = 0; // reset
		Input.m_Fire = false; // do not shoot by default
	}
	else
	{
		m_ticksSinceFirePointer++;
		if (GameServer()->m_pController->m_pGameType[0] == 'g')
		{
			if (m_ticksSinceFirePointer > 50)
			{
				m_ticksSinceFirePointer = 0; // reset
				Input.m_Fire = true; // fire
			}
			else
				Input.m_Fire = false; // do not shoot by default
		}
		else if(GameServer()->m_pController->m_IsInstagibKZ)
		{
			m_ticksSinceFirePointer = 0; // reset
			Input.m_Fire = true; // fire
		}
		else
		{
			if (m_ticksSinceFirePointer > 10)
			{ // 10 = 0.2s
				m_ticksSinceFirePointer = 0; // reset
				Input.m_Fire = true; // fire
			} else
				Input.m_Fire = false; // do not shoot by default
			Input.m_Fire = GameServer()->Server()->Tick() % (2) == 1;
		}
		
		// aim
		if (GameServer()->m_apPlayers[m_botAggroPointer] && GameServer()->m_apPlayers[m_botAggroPointer]->GetCharacter()) {
			vec2 pos = GameServer()->m_apPlayers[m_botAggroPointer]->GetCharacter()->m_Pos;
			//float d = sqrt((pos.x - m_Pos.x)*(pos.x - m_Pos.x) + (pos.y - m_Pos.y)*(pos.y - m_Pos.y)); unused
			Input.m_TargetX = pos.x - m_pPos->x; // aim
			Input.m_TargetY = pos.y - m_pPos->y;
			if (GameServer()->m_pController->m_pGameType[0] == 'g') // grenade curve correction, somewhat
				Input.m_TargetY = Input.m_TargetY + (-abs(Input.m_TargetX)*0.3);
		}
	}
	
	m_pCore->m_aWeapons[WEAPON_GUN].m_Ammo = 10;
}
