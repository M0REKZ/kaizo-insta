#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "tdm_vanilla.h"

CGameControllerTDMVanilla::CGameControllerTDMVanilla(class CGameContext *pGameServer) :
	CGameControllerDMVanilla(pGameServer)
{
    m_VanillaBehavior = true;
    
    m_GameFlags = GAMEFLAG_TEAMS;

    m_GameFlags_v7 = protocol7::GAMEFLAG_TEAMS;

    m_pGameType = "TDM+";
}

CGameControllerTDMVanilla::~CGameControllerTDMVanilla() = default;

int CGameControllerTDMVanilla::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	CGameControllerInstagib::OnCharacterDeath(pVictim, pKiller, WeaponId);

	if(pKiller && WeaponId != WEAPON_GAME)
	{
		// do team scoring
		if(pKiller == pVictim->GetPlayer() || pKiller->GetTeam() == pVictim->GetPlayer()->GetTeam())
			m_aTeamscore[pKiller->GetTeam() & 1]--;
		else
			m_aTeamscore[pKiller->GetTeam() & 1]++;
	}

	// check score win condition
	if((m_GameInfo.m_ScoreLimit > 0 && (m_aTeamscore[TEAM_RED] >= m_GameInfo.m_ScoreLimit || m_aTeamscore[TEAM_BLUE] >= m_GameInfo.m_ScoreLimit)) ||
		(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick() - m_GameStartTick) >= m_GameInfo.m_TimeLimit * Server()->TickSpeed() * 60))
	{
		if(m_SuddenDeath)
		{
			if(m_aTeamscore[TEAM_RED] / 100 != m_aTeamscore[TEAM_BLUE] / 100)
			{
				EndMatch();
				return true;
			}
		}
		else
		{
			if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE])
			{
				EndMatch();
				return true;
			}
			else
				m_SuddenDeath = 1;
		}
	}
	return false;
}
