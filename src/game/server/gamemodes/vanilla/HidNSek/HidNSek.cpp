#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "HidNSek.h"

CGameControllerHidNSek::CGameControllerHidNSek(class CGameContext *pGameServer) :
	CGameControllerDM(pGameServer)
{
    m_GameFlags = GAMEFLAG_TEAMS;

    m_pGameType = "HidNSekᵏᶻ";
	//m_AllowSkinChange = false;
    //m_BombTime = g_Config.m_SvBombTime * Server()->TickSpeed();
	m_WinType = WIN_BY_SURVIVAL;
	m_pStatsTable = "hidnsek";
	m_pExtraColumns = new CHidNSekColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerHidNSek::~CGameControllerHidNSek() = default;

void CGameControllerHidNSek::Tick()
{
    CGameControllerDM::Tick();
	
	/*if(g_Config.m_SvBombWeapon == WEAPON_NINJA && (Server()->Tick() % (Server()->TickSpeed() * 5) == 0))
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
			{
				GameServer()->m_apPlayers[i]->GetCharacter()->SetNinjaActivationTick(Server()->Tick());
			}
		}
	}*/
	
	if(m_RoundActive && !(GameServer()->m_World.m_Paused))
	{
		DoWincheckRound();
	}
    //todo: m_World paused when endmatch-- DONE.. i guess
    if(m_RoundPauseTime > 0)
    {
        m_RoundPauseTime--;
        return;
    }
    else if(GameServer()->m_World.m_Paused && m_RoundPauseTime == 0)
    {
        GameServer()->m_World.m_Paused = false;
        m_RoundPauseTime = -1;
        m_GameStartTick = Server()->Tick();
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
		if(Server()->Tick() % Server()->TickSpeed() == 0)
        	GameServer()->SendBroadcast("Waiting for players...", -1);
    }
    if(PlayerAmount == 1 && m_RoundActive)
    {
        m_RoundActive = false;
    }
    if(m_RoundActive && PlayerAmount > 1 && !m_Warmup)
    {  
        if(!SeekersAmount())
        {
            SetSeekers();
        }

    }
    if(!m_RoundActive && PlayerAmount > 1 && !m_Warmup)
    {
        KillEveryone();
        GameServer()->SendBroadcast("Game started", -1);
        m_RoundActive = true;
        m_RoundPauseTime = 2;
    }
}

void CGameControllerHidNSek::Snap(int SnappingClient)
{
    CGameControllerVanilla::Snap(SnappingClient);
}

void CGameControllerHidNSek::OnPlayerConnect(class CPlayer *pPlayer)
{
    CGameControllerDM::OnPlayerConnect(pPlayer);
    /*if(m_RoundActive)
    {
        pPlayer->SetTeamRaw(TEAM_SPECTATORS);
        pPlayer->m_IsDead = true;
    }
    else
    {
        pPlayer->m_IsDead = false;
    }*/ //For now dont, this casues the 0.7 spec bug
    pPlayer->m_IsSeeker = false;
    pPlayer->SetTeamRaw(TEAM_BLUE);
}

void CGameControllerHidNSek::OnCharacterSpawn(class CCharacter *pChr)
{
    OnCharacterConstruct(pChr);

    pChr->SetTeams(&Teams());
    Teams().OnCharacterSpawn(pChr->GetPlayer()->GetCid());
    
    pChr->IncreaseHealth(10);
    pChr->IncreaseArmor(10);
    
    if(pChr->GetPlayer()->m_IsSeeker)
    {
        pChr->GiveWeapon(g_Config.m_SvHnSSeekerWeapon);
        pChr->SetActiveWeapon(g_Config.m_SvHnSSeekerWeapon);
    }
    else
    {
        pChr->GiveWeapon(g_Config.m_SvHnSHiderWeapon);
        pChr->SetActiveWeapon(g_Config.m_SvHnSHiderWeapon);
    }

}

bool CGameControllerHidNSek::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
    return CGameControllerVanilla::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
     
}

bool CGameControllerHidNSek::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
	if(Character.m_TakingNoOwnerDamage || (From >= 0 && From < MAX_CLIENTS && GameServer()->m_apPlayers[From] && !GameServer()->m_apPlayers[From]->m_IsSeeker))
	{
		CGameControllerDM::OnCharacterTakeDamage(Force, Dmg, From, Weapon, Character);
	}
	else
	{
		Dmg = 0;
		CGameControllerPvp::OnCharacterTakeDamage(Force, Dmg, From, Weapon, Character);
	}

	
	if(From < 0 || From > MAX_CLIENTS) //only valid CID
		return false;
    if(GameServer()->m_apPlayers[From] == Character.GetPlayer())
        return false;

    if(GameServer()->m_apPlayers[From] && GameServer()->m_apPlayers[From]->m_IsSeeker)
    {
        Character.GetPlayer()->m_IsDead = true;
        Character.GetPlayer()->SetTeamRaw(TEAM_SPECTATORS);
        Character.Die(From, Weapon, true);
    }
    return false;
}

int CGameControllerHidNSek::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	if(WeaponId == WEAPON_GAME)
		return false;
	
    if(m_RoundActive && pVictim && !pVictim->GetPlayer()->m_IsSeeker)
    {
        pVictim->GetPlayer()->SetTeamRaw(TEAM_SPECTATORS);
        pVictim->GetPlayer()->m_IsDead = true;
    }
	return false;
}

bool CGameControllerHidNSek::DoWincheckRound()
{
    //if(GetPlayerAmount() <= 1)
    //    return false;

    if(m_RoundPauseTime >= 0)
        return false;

    if(CGameControllerDM::DoWincheckRound())
    {
        SetAllUndead();
        m_EndingRound = true;
        return true;
    }

    bool HiderFound = false;;

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i] && !GameServer()->m_apPlayers[i]->m_IsDead && !GameServer()->m_apPlayers[i]->m_IsSeeker)
        {
            HiderFound = true;
            break;
        }
    }
    if(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60)
    {
        {
            for(int i = 0; i < MAX_CLIENTS; i++)
            {
                if(GameServer()->m_apPlayers[i] && !GameServer()->m_apPlayers[i]->m_IsSeeker)
                {
                    GameServer()->m_apPlayers[i]->IncrementScore();
                }
            }
            FakeEndRound();
            SetAllUndead();
            m_EndingRound = true;
            return true;
        }
    }
    else
    {
        if(HiderFound)
            return false;
        else
        {
            for(int i = 0; i < MAX_CLIENTS; i++)
            {
                if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_IsSeeker)
                {
                    GameServer()->m_apPlayers[i]->IncrementScore();
                }
            }
            FakeEndRound();
            SetAllUndead();
            m_EndingRound = true;
            return true;
        }
    }
    return false;
}

void CGameControllerHidNSek::KillEveryone()
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->GetCharacter())
            {
                GameServer()->m_apPlayers[i]->GetCharacter()->Destroy();
                GameServer()->m_apPlayers[i]->Respawn();
            }
            
        }
    }
}

void CGameControllerHidNSek::FakeEndRound()
{
    //m_GameOverTick = Server()->Tick();
    GameServer()->m_World.m_Paused = true;
    m_RoundPauseTime = 150;

	GameServer()->SendBroadcast("Round Finish", -1);
    m_GameStartTick = Server()->Tick();
    //SetGameState(IGS_END_MATCH, TIMER_END);
}

int CGameControllerHidNSek::GetPlayerAmount()
{
    int amount = 0;
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(!GameServer()->m_apPlayers[i])
            continue;

        if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS)
            continue;
        
        if(!GameServer()->m_apPlayers[i]->GetCharacter())
            continue;
        
        amount++;
    }
    return amount;
}

void CGameControllerHidNSek::SetAllUndead()
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if((GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_IsDead ) || GameServer()->m_apPlayers[i]->m_IsDead)
            {
				GameServer()->m_apPlayers[i]->m_IsDead = false;
                GameServer()->m_apPlayers[i]->SetTeamRaw(TEAM_BLUE);
                GameServer()->m_apPlayers[i]->Respawn();
            }
			else
			{
				GameServer()->m_apPlayers[i]->m_IsDead = false;
			}
        }
    }
}

bool CGameControllerHidNSek::CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize)
{
    CPlayer *pPlayer = GameServer()->m_apPlayers[NotThisId];
    if(!pPlayer)
        return false;

    if(pPlayer->m_IsDead)
    {
        str_copy(pErrorReason, "Wait until round end", ErrorReasonSize);
        return false;
    }
    return true;
}

void CGameControllerHidNSek::SetSeekers()
{
    int bombneed = GetPlayerAmount() / g_Config.m_SvHnSSeekerAmount;
    
    if(!bombneed && GetPlayerAmount() <= 1)
        return;
    else
        bombneed = 1;
    
    
    
    int rnd = 0;
    while(bombneed)
    {
        rnd = rand() % MAX_CLIENTS;
        if(GameServer()->m_apPlayers[rnd])
        {
            if(GameServer()->m_apPlayers[rnd]->GetTeam() != TEAM_SPECTATORS && (!GameServer()->m_apPlayers[rnd]->m_IsDead || (GameServer()->m_apPlayers[rnd]->GetCharacter() && GameServer()->m_apPlayers[rnd]->GetCharacter()->IsAlive())))
            {
                GameServer()->m_apPlayers[rnd]->m_IsSeeker = true;
                GameServer()->m_apPlayers[rnd]->SetTeamRaw(TEAM_RED);
                GameServer()->m_apPlayers[rnd]->Respawn();
                if(GameServer()->m_apPlayers[rnd]->GetCharacter())
                {
                    GameServer()->m_apPlayers[rnd]->GetCharacter()->GiveWeapon(g_Config.m_SvHnSSeekerWeapon);
                    GameServer()->m_apPlayers[rnd]->GetCharacter()->SetActiveWeapon(g_Config.m_SvHnSSeekerWeapon);
                }
                bombneed--;
            }
        }
    }
    
    //SetSkins();
}

int CGameControllerHidNSek::SeekersAmount()
{
    int amount = 0;
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if(GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && (!GameServer()->m_apPlayers[i]->m_IsDead || (GameServer()->m_apPlayers[i]->GetCharacter() && GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())) && GameServer()->m_apPlayers[i]->m_IsSeeker)
            {
                amount++;
            }
        }
    }
    return amount;
}