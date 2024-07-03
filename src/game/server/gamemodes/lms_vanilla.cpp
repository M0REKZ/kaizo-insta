#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "lms_vanilla.h"

CGameControllerLMSVanilla::CGameControllerLMSVanilla(class CGameContext *pGameServer) :
	CGameControllerDMVanilla(pGameServer)
{
    m_VanillaBehavior = true;
    
    m_GameFlags = 0;
    m_GameFlags_v7 = protocol7::GAMEFLAG_SURVIVAL;

    m_pGameType = "LMS+";
}

CGameControllerLMSVanilla::~CGameControllerLMSVanilla() = default;

void CGameControllerLMSVanilla::Tick()
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
	CGameControllerDMVanilla::Tick();
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

void CGameControllerLMSVanilla::OnPlayerConnect(class CPlayer *pPlayer)
{
    CGameControllerDMVanilla::OnPlayerConnect(pPlayer);
    /*if(m_RoundActive)
    {
        pPlayer->SetTeamRaw(TEAM_SPECTATORS);
        pPlayer->m_IsDead = true;
    }
    else
    {
        pPlayer->m_IsDead = false;
    }*/ //For now dont, this casues the 0.7 spec bug
}

void CGameControllerLMSVanilla::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerDMVanilla::OnCharacterSpawn(pChr);

    //LMS
    pChr->IncreaseArmor(5);
    pChr->GiveWeapon(WEAPON_SHOTGUN, false, 10);
    pChr->GiveWeapon(WEAPON_GRENADE, false, 10);
    pChr->GiveWeapon(WEAPON_LASER, false, 5);
}

bool CGameControllerLMSVanilla::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
    return CGameControllerInstagib::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
}

bool CGameControllerLMSVanilla::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
    return CGameControllerDMVanilla::OnCharacterTakeDamage(Force, Dmg, From, Weapon, Character);
}

int CGameControllerLMSVanilla::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
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
            
        pVictim->GetPlayer()->SetTeamRaw(TEAM_SPECTATORS);
        pVictim->GetPlayer()->m_IsDead = true;
    }
	return false;
}

bool CGameControllerLMSVanilla::DoWincheckMatch()
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
    
    // check for time based win
    if(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60)
    {
        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            if(GameServer()->m_apPlayers[i])
            {
                if(GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && (!GameServer()->m_apPlayers[i]->m_IsDead || (GameServer()->m_apPlayers[i]->GetCharacter() && GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())))
                    GameServer()->m_apPlayers[i]->IncrementScore();
            }
        }

        GameServer()->SendBroadcast("Game End", -1);
        EndMatch();
        SetAllUndead();
        return true;
    }
    else
    {
        // check for survival win
        CPlayer *pAlivePlayer = 0;
        int AlivePlayerCount = 0;
        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            if(GameServer()->m_apPlayers[i])
            {
                if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && (!GameServer()->m_apPlayers[i]->m_IsDead || (GameServer()->m_apPlayers[i]->GetCharacter() && GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())))
                {
                    ++AlivePlayerCount;
                    pAlivePlayer = GameServer()->m_apPlayers[i];
                }
                
            }
        }

        if(AlivePlayerCount == 0)        // no winner
        {
            FakeEndRound();
            SetAllUndead();
            return true;
        }
        else if(AlivePlayerCount == 1)    // 1 winner
        {
            pAlivePlayer->IncrementScore();
            FakeEndRound();
            SetAllUndead();
            return true;
        }
    }
    return false;
}

void CGameControllerLMSVanilla::SetAllUndead()
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_IsDead)
            {
                GameServer()->m_apPlayers[i]->SetTeamRaw(TEAM_RED);
                GameServer()->m_apPlayers[i]->Respawn();
            }
            GameServer()->m_apPlayers[i]->m_IsDead = false;
        }
    }
}

void CGameControllerLMSVanilla::FakeEndRound()
{
    //m_GameOverTick = Server()->Tick();
    GameServer()->m_World.m_Paused = true;
    m_RoundPauseTime = 150;
    GameServer()->SendBroadcast("Round Finish", -1);
    KillEveryone();
    //SetGameState(IGS_END_MATCH, TIMER_END);
}

void CGameControllerLMSVanilla::KillEveryone()
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
            {
                GameServer()->m_apPlayers[i]->KillCharacter(WEAPON_SELF);
                GameServer()->m_apPlayers[i]->Respawn();
            }
            
        }
    }
}

bool CGameControllerLMSVanilla::CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize)
{
    CPlayer *pPlayer = GameServer()->m_apPlayers[NotThisId];
    if(!pPlayer)
        return false;

    if(pPlayer->m_IsDead && Team != TEAM_SPECTATORS)
    {
        str_copy(pErrorReason, "Wait until round end", ErrorReasonSize);
        return false;
    }
    return true;
}
