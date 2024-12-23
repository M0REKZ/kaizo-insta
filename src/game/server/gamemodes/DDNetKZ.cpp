/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
/* Based on Race mod stuff and tweaked by GreYFoX@GTi and others to fit our DDRace needs. */
#include "DDNetKZ.h"

#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#define GAME_TYPE_NAME "DDraceNetwork"
#define TEST_TYPE_NAME "TestDDraceNetwork"

CGameControllerDDNetKZ::CGameControllerDDNetKZ(class CGameContext *pGameServer) :
	CGameControllerDDRace(pGameServer)
{
	// game
	m_AllowSkinChange = true;
	//m_pGameType = g_Config.m_SvTestingCommands ? TEST_TYPE_NAME : GAME_TYPE_NAME;
	m_GameFlags = protocol7::GAMEFLAG_RACE;

	m_pGameType =  g_Config.m_SvTestingCommands ? "TestDDNetᵏᶻ" : "DDNetᵏᶻ";
	m_DefaultWeapon = WEAPON_HAMMER;
	m_IsInstagibKZ = false;
	m_IsVanillaGameType = false;
}

CGameControllerDDNetKZ::~CGameControllerDDNetKZ() = default;

void CGameControllerDDNetKZ::Tick()
{
	CGameControllerDDRace::Tick();

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
		{
			if(GameServer()->GetPlayerChar(i)->m_Health <= 0)
			{
				GameServer()->GetPlayerChar(i)->Die(i, WEAPON_WORLD);
			}
		}
	}
}

int CGameControllerDDNetKZ::SnapGameInfoExFlags2(int SnappingClient, int DDRaceFlags)
{
	return GAMEINFOFLAG2_HUD_AMMO | GAMEINFOFLAG2_HUD_HEALTH_ARMOR;
}