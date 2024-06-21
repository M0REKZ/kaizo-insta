#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "lts_vanilla.h"

CGameControllerLTSVanilla::CGameControllerLTSVanilla(class CGameContext *pGameServer) :
	CGameControllerLMSVanilla(pGameServer)
{
    m_VanillaBehavior = true;
    
    m_GameFlags = GAMEFLAG_TEAMS;
    m_GameFlags_v7 = protocol7::GAMEFLAG_TEAMS | protocol7::GAMEFLAG_SURVIVAL;

    m_pGameType = "LTS+";
}

CGameControllerLTSVanilla::~CGameControllerLTSVanilla() = default;

void CGameControllerLTSVanilla::Tick()
{
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
	CGameControllerLMSVanilla::Tick();
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
    }
}

void CGameControllerLTSVanilla::OnPlayerConnect(class CPlayer *pPlayer)
{
    CGameControllerLMSVanilla::OnPlayerConnect(pPlayer);
    m_aPlayerTeam[pPlayer->GetCid()] = TEAM_SPECTATORS; //TODO: improve this
    pPlayer->SetTeamRaw(TEAM_SPECTATORS);
    pPlayer->m_IsDead = false;
}

bool CGameControllerLTSVanilla::DoWincheckMatch()
{
    if(!m_RoundActive)
        return false;
    
    // check score win condition
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if(m_GameInfo.m_ScoreLimit > 0 && GameServer()->m_apPlayers[i]->m_Score.value_or(0) >= m_GameInfo.m_ScoreLimit)
            {
                GameServer()->SendBroadcast("Game End", -1);
                EndMatch();
                SetAllUndead();
                return true;
            }
        }
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

void CGameControllerLTSVanilla::SetAllUndead()
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

int CGameControllerLTSVanilla::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
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
