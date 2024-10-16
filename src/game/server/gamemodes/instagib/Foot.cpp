#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "Foot.h"

CGameControllerInstaFoot::CGameControllerInstaFoot(class CGameContext *pGameServer) :
	CGameControllerInstaTDM(pGameServer)
{
	m_GameFlags = GAMEFLAG_TEAMS;
}

CGameControllerInstaFoot::~CGameControllerInstaFoot() = default;

void CGameControllerInstaFoot::Tick()
{
	CGameControllerInstaBaseDM::Tick();
	
	// check score win condition
	if((m_GameInfo.m_ScoreLimit > 0 && (m_aTeamscore[TEAM_RED] >= m_GameInfo.m_ScoreLimit || m_aTeamscore[TEAM_BLUE] >= m_GameInfo.m_ScoreLimit)) ||
		(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick() - m_GameStartTick) >= m_GameInfo.m_TimeLimit * Server()->TickSpeed() * 60))
	{
		if(m_SuddenDeath)
		{
			if(m_aTeamscore[TEAM_RED] / 100 != m_aTeamscore[TEAM_BLUE] / 100)
			{
				EndRound();
				return;
			}
		}
		else
		{
			if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE])
			{
				EndRound();
				return;
			}
			else
				m_SuddenDeath = 1;
		}
	}
	return;
}

int CGameControllerInstaFoot::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	return CGameControllerPvp::OnCharacterDeath(pVictim, pKiller, WeaponId);
}
