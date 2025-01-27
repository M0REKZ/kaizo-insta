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
    m_RoundPauseTime = 2;
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
	
    if(m_GameState == IGS_END_ROUND)
        return;

	if(m_RoundActive && !(GameServer()->m_World.m_Paused))
	{
		DoWincheckRound();
	}
    if(m_RoundPauseTime > 0)
    {
        m_RoundPauseTime--;
        return;
    }
    else if((GameServer()->m_World.m_Paused && m_RoundPauseTime == 0) || m_EndingRound)
    {
        GameServer()->m_World.m_Paused = false;
        m_RoundPauseTime = -1;
        m_EndingRound = false;
        m_RoundStartTick = Server()->Tick();
        m_GameStartTick = Server()->Tick();
        //RespawnAll();
        if(m_RealEndRound)
        {
            UnSetSeekers();
            m_RealEndRound = false;
        }
        SetAllUndead();
    }

    if(!GameServer()->m_World.m_Paused)
    {
        m_RoundPauseTime = -1;
    }
    //CGameControllerDM::Tick();
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
        {
        	GameServer()->SendBroadcast("Waiting for players...", -1);
            m_RoundStartTick = Server()->Tick();
            m_GameStartTick = Server()->Tick();
        }
    }
    if(PlayerAmount == 1 && m_RoundActive)
    {
        m_RoundActive = false;
    }
    if(m_RoundActive && PlayerAmount > 1 && !m_Warmup)
    {  
        if(!SeekersAmount())
        {
            UnSetSeekers();
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

void CGameControllerHidNSek::OnPlayerReadyChange(CPlayer *pPlayer)
{
    if(!pPlayer)
        return;
    
    SendChatTarget(pPlayer->GetCid(),"You can't /pause in this gamemode");
}

void CGameControllerHidNSek::Snap(int SnappingClient)
{
    CGameControllerVanilla::Snap(SnappingClient);

    if(Server()->IsSixup(SnappingClient))
	{
       /* protocol7::CNetObj_GameDataTeam *pGameDataObj = (protocol7::CNetObj_GameDataTeam *)Server()->SnapNewItem(protocol7::NETOBJTYPE_GAMEDATATEAM, 0, sizeof(protocol7::CNetObj_GameDataTeam));
    
        if(!pGameDataObj)
            return;
        
        pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
        pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];
    */
    }
    else
    {
        CNetObj_GameData *pGameDataObj = (CNetObj_GameData *)Server()->SnapNewItem(NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData));
        if(!pGameDataObj)
            return;

        pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
        pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];

        pGameDataObj->m_FlagCarrierRed = 0;
        pGameDataObj->m_FlagCarrierBlue = 0;
    }
}

void CGameControllerHidNSek::OnPlayerConnect(class CPlayer *pPlayer)
{
    CGameControllerDM::OnPlayerConnect(pPlayer);
    if(m_RoundActive)
    {
        pPlayer->SetTeamRaw(TEAM_SPECTATORS);
        pPlayer->m_IsDead = true;
    }
    else
    {
        pPlayer->m_IsDead = false;
        pPlayer->SetTeamRaw(TEAM_BLUE);
    } //For now dont, this casues the 0.7 spec bug //for now enable it ill discover a solution later

    pPlayer->m_IsSeeker = false;
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
	if(Character.m_TakingNoOwnerDamage || (From >= 0 && From < MAX_CLIENTS && GameServer()->m_apPlayers[From] && !GameServer()->m_apPlayers[From]->m_IsSeeker && !(Character.GetPlayer()->GetTeam() == GameServer()->m_apPlayers[From]->GetTeam())))
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
        Character.Die(From, Weapon, true, false);
    }
    return false;
}

int CGameControllerHidNSek::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	CGameControllerDDRace::OnCharacterDeath(pVictim, pKiller, WeaponId);
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
    if(m_EndingRound)
        return false;

    if(m_RoundPauseTime >= 0)
        return false;

    //printf("DoWincheckRound\n");

    bool HiderFound = false;;

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i] && !GameServer()->m_apPlayers[i]->m_IsDead && !GameServer()->m_apPlayers[i]->m_IsSeeker && !(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && !GameServer()->m_apPlayers[i]->m_IsDead))
        {
            HiderFound = true;
            break;
        }
    }
    if(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60)
    {
        {
            /*for(int i = 0; i < MAX_CLIENTS; i++)
            {
                if(GameServer()->m_apPlayers[i] && !GameServer()->m_apPlayers[i]->m_IsSeeker)
                {
                    GameServer()->m_apPlayers[i]->IncrementScore();
                }
            }*/
            m_aTeamscore[TEAM_BLUE]++;
            //m_RoundStartTick = Server()->Tick();
            //m_GameStartTick = Server()->Tick();
            //FakeEndRound();
            //KillEveryone();
           // SetAllUndead();
            m_EndingRound = true;
            //return true;
        }
    }
    else
    {
        if(HiderFound)
            return false;
        else
        {
            /*for(int i = 0; i < MAX_CLIENTS; i++)
            {
                if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_IsSeeker)
                {
                    GameServer()->m_apPlayers[i]->IncrementScore();
                }
            }*/
            m_aTeamscore[TEAM_RED]++;
           // m_RoundStartTick = Server()->Tick();
            //m_GameStartTick = Server()->Tick();
            //FakeEndRound();
           // KillEveryone();
            //SetAllUndead();
            m_EndingRound = true;
            //return true;
        }
    }

    if((m_GameInfo.m_ScoreLimit > 0 && (m_aTeamscore[TEAM_RED] >= m_GameInfo.m_ScoreLimit || m_aTeamscore[TEAM_BLUE] >= m_GameInfo.m_ScoreLimit)))
    {
        //EndRound();
        //UnSetSeekers();
        //SetAllUndead();
        m_EndingRound = true;
        m_RealEndRound = true;
        //return true;
    }

    if(m_EndingRound)
    {
        m_RoundStartTick = Server()->Tick();
        m_GameStartTick = Server()->Tick();
        if(m_RealEndRound)
        {
            MarkPlayersForRespawn();
            EndRound();
            m_RoundPauseTime = 2;
        }
        else
        {
            FakeEndRound();
            KillEveryone();
        }
        return true;
    }


    return false;
}

void CGameControllerHidNSek::KillEveryone()
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if(GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->GetCharacter())
            {
                if(GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())
                    GameServer()->m_apPlayers[i]->KillCharacter();
                //GameServer()->m_apPlayers[i]->Respawn();
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
            if(GameServer()->m_apPlayers[i]->m_IsDead || GameServer()->m_apPlayers[i]->m_MarkedForRespawn)
            {
				GameServer()->m_apPlayers[i]->m_IsDead = false;
                GameServer()->m_apPlayers[i]->m_MarkedForRespawn = false;
                if(GameServer()->m_apPlayers[i]->m_IsSeeker)
                    GameServer()->m_apPlayers[i]->SetTeamRaw(TEAM_RED);
                else
                    GameServer()->m_apPlayers[i]->SetTeamRaw(TEAM_BLUE);
                GameServer()->m_apPlayers[i]->Respawn();
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
    else if(pPlayer->GetTeam() == TEAM_SPECTATORS && Team == TEAM_BLUE)
    {
        pPlayer->m_IsSeeker = false;
        return true;
    }
    else if(Team == TEAM_SPECTATORS)
    {
        pPlayer->m_IsSeeker = false;
        return true;
    }

    return false;
}

void CGameControllerHidNSek::SetSeekers()
{
    int bombneed = g_Config.m_SvHnSSeekerAmount;
    
    if(!bombneed && GetPlayerAmount() <= 1)
        return;
    else
        bombneed = 1;
    
    if(bombneed > g_Config.m_SvMaxClients)
    {
        bombneed = g_Config.m_SvMaxClients;
    }
    
    
    int rnd = 0;
    while(bombneed)
    {
        rnd = rand() % MAX_CLIENTS;
        if(GameServer()->m_apPlayers[rnd])
        {
            if(!GameServer()->m_apPlayers[rnd]->m_IsSeeker && !GameServer()->m_apPlayers[rnd]->m_WasSeeker && GameServer()->m_apPlayers[rnd]->GetTeam() != TEAM_SPECTATORS && (!GameServer()->m_apPlayers[rnd]->m_IsDead || (GameServer()->m_apPlayers[rnd]->GetCharacter() && GameServer()->m_apPlayers[rnd]->GetCharacter()->IsAlive())))
            {
                GameServer()->m_apPlayers[rnd]->m_IsSeeker = true;
                GameServer()->m_apPlayers[rnd]->m_WasSeeker = true;
                GameServer()->m_apPlayers[rnd]->SetTeamRaw(TEAM_RED);
                //GameServer()->m_apPlayers[rnd]->Respawn();
                if(GameServer()->m_apPlayers[rnd]->GetCharacter())
                {
                    GameServer()->m_apPlayers[rnd]->GetCharacter()->GiveWeapon(g_Config.m_SvHnSSeekerWeapon);
                    GameServer()->m_apPlayers[rnd]->GetCharacter()->SetActiveWeapon(g_Config.m_SvHnSSeekerWeapon);
                }
                bombneed--;
            }
        }
    }

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[rnd]->m_WasSeeker && !GameServer()->m_apPlayers[rnd]->m_IsSeeker)
        {
            GameServer()->m_apPlayers[rnd]->m_WasSeeker = false;
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

void CGameControllerHidNSek::UnSetSeekers()
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if(GameServer()->m_apPlayers[i]->m_IsSeeker)
            {
                GameServer()->m_apPlayers[i]->m_IsSeeker = false;
                GameServer()->m_apPlayers[i]->SetTeamRaw(TEAM_BLUE);
            }
        }
    }
}

bool CGameControllerHidNSek::OnCharacterSnap(int SnappingClient, int Id)
{
    if(GameServer()->m_apPlayers[SnappingClient] && GameServer()->m_apPlayers[Id] && !GameServer()->m_apPlayers[Id]->m_IsSeeker && GameServer()->m_apPlayers[SnappingClient]->GetTeam() == TEAM_SPECTATORS)
        return true;
    else if(SnappingClient >= 0 && SnappingClient < MAX_CLIENTS && GameServer()->m_apPlayers[SnappingClient] && GameServer()->m_apPlayers[Id]->GetCharacter())
    {
        vec2 pos = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos;
        vec2 target = GameServer()->m_apPlayers[Id]->GetCharacter()->m_Pos;

        int Res = GameServer()->Collision()->IntersectLine(pos, target, 0, 0, 0);

        if(Res) //Res is 0 when it hits nothing
            return true;
    }
    return CGameControllerDM::OnCharacterSnap(SnappingClient, Id);
}

bool CGameControllerHidNSek::CanSpecPlayer(int ClientID)
{
    if(GameServer()->m_apPlayers[ClientID] && !GameServer()->m_apPlayers[ClientID]->m_IsSeeker)
        return false;
    else
        return CGameControllerDM::CanSpecPlayer(ClientID);
}

void CGameControllerHidNSek::MarkPlayersForRespawn()
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_IsDead)
        {
            GameServer()->m_apPlayers[i]->m_MarkedForRespawn = true;
        }
    }
}