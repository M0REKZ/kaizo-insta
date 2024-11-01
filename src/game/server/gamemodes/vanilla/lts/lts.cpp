#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "lts.h"

CGameControllerLTS::CGameControllerLTS(class CGameContext *pGameServer) :
	CGameControllerLMS(pGameServer)
{
	
	m_GameFlags = GAMEFLAG_TEAMS;
	
	m_pGameType = "LTSᵏᶻ";
	
	m_pStatsTable = "lts";
	m_pExtraColumns = new CLtsColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerLTS::~CGameControllerLTS() = default;

void CGameControllerLTS::Tick()
{
	if(m_RoundActive && !(GameServer()->m_World.m_Paused))
	{
		DoWincheckRound();
	}
	if(m_RoundPauseTime > 0)
	{
		m_RoundPauseTime--;
		return;
	}
	else if(GameServer()->m_World.m_Paused && m_RoundPauseTime == 0)
	{
		GameServer()->m_World.m_Paused = false;
		m_RoundPauseTime = -1;
	}
	CGameControllerDM::Tick();
	//kinda ugly loop
	int PlayerAmount=0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(GameServer()->m_apPlayers[i] && (GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS || (GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_IsDead)))
				++PlayerAmount;
	}
	if(PlayerAmount == 0)
	{
		m_RoundActive = false;
	}
	if(PlayerAmount == 1 && !m_RoundActive)
	{
		GameServer()->SendBroadcast("Waiting for players...", -1);
	}
	if(PlayerAmount == 1 && m_RoundActive)
	{
		m_RoundActive = false;
	}
	if(!m_RoundActive && PlayerAmount > 1 && !m_Warmup)
	{
		KillEveryone();
		GameServer()->SendBroadcast("Game started", -1);
		m_RoundActive = true;
		//m_RoundPauseTime = 2;
	}
}

void CGameControllerLTS::OnPlayerConnect(class CPlayer *pPlayer)
{
    CGameControllerLMS::OnPlayerConnect(pPlayer);
    m_aPlayerTeam[pPlayer->GetCid()] = TEAM_SPECTATORS; //TODO: improve this
    pPlayer->SetTeamRaw(TEAM_SPECTATORS);
    pPlayer->m_IsDead = false;
}

bool CGameControllerLTS::DoWincheckRound()
{
    if(!m_RoundActive && m_RoundPauseTime == -1)
        return false;
    
    // check score win condition

	if(m_GameInfo.m_ScoreLimit > 0 && (m_aTeamscore[TEAM_BLUE] >= m_GameInfo.m_ScoreLimit || m_aTeamscore[TEAM_RED] >= m_GameInfo.m_ScoreLimit))
	{
		
		GameServer()->SendBroadcast("Game End", -1);
		EndRound();
		SetAllUndead();
		
	}
    
    int Count[2] = {0};
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && (!GameServer()->m_apPlayers[i]->m_IsDead || (GameServer()->m_apPlayers[i]->GetCharacter() && GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())))
            ++Count[GameServer()->m_apPlayers[i]->GetTeam()];
    }

    if(Count[TEAM_RED]+Count[TEAM_BLUE] == 0 || (m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60))
    {
        ++m_aTeamscore[TEAM_BLUE];
        ++m_aTeamscore[TEAM_RED];
        FakeEndRound();
        SetAllUndead();
        return true;

    }
    else if(Count[TEAM_RED] == 0)
    {
        ++m_aTeamscore[TEAM_BLUE];
        FakeEndRound();
        SetAllUndead();
        return true;
    }
    else if(Count[TEAM_BLUE] == 0)
    {
        ++m_aTeamscore[TEAM_RED];
        FakeEndRound();
        SetAllUndead();
        return true;
    }
    return false;
}

void CGameControllerLTS::SetAllUndead()
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_IsDead)
            {
                GameServer()->m_apPlayers[i]->SetTeamRaw(m_aPlayerTeam[GameServer()->m_apPlayers[i]->GetCid()]);
                GameServer()->m_apPlayers[i]->Respawn();
            }
            GameServer()->m_apPlayers[i]->m_IsDead = false;
        }
    }
}

int CGameControllerLTS::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
    //CGameControllerInstagib::OnCharacterDeath(pVictim, pKiller, WeaponId); //fallback to instagib
    if(pVictim->GetPlayer() && m_RoundActive && m_RoundPauseTime < 0 && WeaponId != -3)
    {
        //LMS also gives score on killing
        if(pKiller)
        {
            if(pVictim->GetPlayer()==pKiller)
                pKiller->DecrementScore();
            else
                pKiller->IncrementScore();
        }
        m_aPlayerTeam[pVictim->GetPlayer()->GetCid()] = pVictim->GetPlayer()->GetTeam();
        pVictim->GetPlayer()->SetTeamRaw(TEAM_SPECTATORS);
        pVictim->GetPlayer()->m_IsDead = true;
    }
    return false;
}

void CGameControllerLTS::Snap(int SnappingClient)
{
	CGameControllerVanilla::Snap(SnappingClient);

	if(Server()->IsSixup(SnappingClient))
		return;

	CNetObj_GameData *pGameDataObj = (CNetObj_GameData *)Server()->SnapNewItem(NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData));
	if(!pGameDataObj)
		return;

	pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
	pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];

	pGameDataObj->m_FlagCarrierRed = 0;
	pGameDataObj->m_FlagCarrierBlue = 0;
}
