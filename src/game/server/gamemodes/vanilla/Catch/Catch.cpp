#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "Catch.h"

CGameControllerCatch::CGameControllerCatch(class CGameContext *pGameServer) :
	CGameControllerDM(pGameServer)
{
    m_GameFlags = 0;

    m_pGameType = "Catch64ᵏᶻ";
	m_AllowSkinChange = false;
    //m_BombTime = g_Config.m_SvBombTime * Server()->TickSpeed();
	
	m_pStatsTable = "catch";
	m_pExtraColumns = new CCatchColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);


    Colors[0] = 0;
    Colors[1] = 255;
    for(int i = 2; i < MAX_CLIENTS; i++)
    {
        Colors[i] = maximum(0, 160 - i * 10) * 0x010000 + 0xff00;
    }
}

CGameControllerCatch::~CGameControllerCatch() = default;

void CGameControllerCatch::Tick()
{
    CGameControllerDM::Tick();

    if(m_RoundPauseTime > 0)
    {
        m_RoundPauseTime--;
        return;
    }
    else if(GameServer()->m_World.m_Paused && m_RoundPauseTime == 0)
    {
        GameServer()->m_World.m_Paused = false;
        m_RoundPauseTime = -1;
        KillEveryone();
    }

    if(m_EndingRound && !GameServer()->m_World.m_Paused)
    {
       ResetPlayerColors();
       m_EndingRound = false;
    }
   
}

bool CGameControllerCatch::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
    if(g_Config.m_SvCatchDamage || Character.m_TakingNoOwnerDamage)
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
    //if(Character.GetPlayer()->m_IsBomb)
     //   return false;
    Character.GetPlayer()->m_CatchColor = GameServer()->m_apPlayers[From]->m_CatchColor;
    str_copy(Character.GetPlayer()->m_CatchSkin, GameServer()->m_apPlayers[From]->m_CatchSkin, MAX_SKIN_LENGTH);
    UpdateSkins();

    return false;
}

bool CGameControllerCatch::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
    return CGameControllerVanilla::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
}

bool CGameControllerCatch::DoWincheckRound()
{
    if(GetPlayerAmount() <= 1)
        return false;

    if(m_RoundPauseTime >= 0)
        return false;

    if(CGameControllerDM::DoWincheckRound())
    {
        m_EndingRound = true;
        return true;
    }

    int WinColor = -1;

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
        {
            WinColor = GameServer()->m_apPlayers[i]->m_CatchColor;
            break;
        }
    }

    if(WinColor == -1)
        return false;
    

    for(int i = 1; i < MAX_CLIENTS; i++)
    {
        if(!GameServer()->m_apPlayers[i])
            continue;

        if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS)
            continue;

        if(WinColor != GameServer()->m_apPlayers[i]->m_CatchColor)
            return false;
    }

    //search winner

    for(int i = 1; i < MAX_CLIENTS; i++)
    {
        if(!GameServer()->m_apPlayers[i])
            continue;

        if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS)
            continue;

        if(WinColor == GameServer()->m_apPlayers[i]->m_CatchOrigColor)
        {
            GameServer()->m_apPlayers[i]->IncrementScore();
            FakeEndRound();
            m_EndingRound = true;
            return true;
        }
    }

    FakeEndRound();
    m_EndingRound = true;
    return true;
}

void CGameControllerCatch::OnPlayerConnect(class CPlayer *pPlayer)
{
    CGameControllerDM::OnPlayerConnect(pPlayer);
    pPlayer->m_CatchColor = Colors[pPlayer->GetCid()];
    pPlayer->m_CatchOrigColor = pPlayer->m_CatchColor;
    str_copy(pPlayer->m_CatchOrigSkin, pPlayer->m_TeeInfos.m_aSkinName, MAX_SKIN_LENGTH);
    str_copy(pPlayer->m_CatchSkin, pPlayer->m_CatchOrigSkin, MAX_SKIN_LENGTH);
    return;
}

void CGameControllerCatch::OnCharacterSpawn(class CCharacter *pChr)
{
    OnCharacterConstruct(pChr);

    pChr->SetTeams(&Teams());
    Teams().OnCharacterSpawn(pChr->GetPlayer()->GetCid());
    
    pChr->IncreaseHealth(10);
    pChr->IncreaseArmor(10);
    
    pChr->GiveWeapon(g_Config.m_SvCatchWeapon);
    pChr->SetActiveWeapon(g_Config.m_SvCatchWeapon);
    
   // SetSkins();
   UpdateSkins();

}

void CGameControllerCatch::UpdateSkins()
{
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(!GameServer()->m_apPlayers[i])
            continue;

        GameServer()->m_apPlayers[i]->m_TeeInfos.m_UseCustomColor = 1;
        GameServer()->m_apPlayers[i]->m_TeeInfos.m_ColorBody = GameServer()->m_apPlayers[i]->m_CatchColor;
        GameServer()->m_apPlayers[i]->m_TeeInfos.m_ColorFeet = GameServer()->m_apPlayers[i]->m_CatchColor;

        str_copy(GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinName, GameServer()->m_apPlayers[i]->m_CatchSkin, MAX_SKIN_LENGTH);

        GameServer()->m_apPlayers[i]->m_TeeInfos.m_aUseCustomColors[0] = true;
        GameServer()->m_apPlayers[i]->m_TeeInfos.m_aUseCustomColors[1] = true;
        GameServer()->m_apPlayers[i]->m_TeeInfos.m_aUseCustomColors[2] = true;
        GameServer()->m_apPlayers[i]->m_TeeInfos.m_aUseCustomColors[3] = true;
        GameServer()->m_apPlayers[i]->m_TeeInfos.m_aUseCustomColors[4] = true;
        GameServer()->m_apPlayers[i]->m_TeeInfos.m_aUseCustomColors[5] = true;
                    
        GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinPartColors[0] = GameServer()->m_apPlayers[i]->m_CatchColor;
        GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinPartColors[1] = GameServer()->m_apPlayers[i]->m_CatchColor;
        GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinPartColors[2] = GameServer()->m_apPlayers[i]->m_CatchColor;
        GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinPartColors[3] = GameServer()->m_apPlayers[i]->m_CatchColor;
        GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinPartColors[4] = GameServer()->m_apPlayers[i]->m_CatchColor;
        GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinPartColors[5] = GameServer()->m_apPlayers[i]->m_CatchColor;

        protocol7::CNetMsg_Sv_SkinChange Msg;
		Msg.m_ClientId = i;
		for(int p = 0; p < protocol7::NUM_SKINPARTS; p++)
		{
			Msg.m_apSkinPartNames[p] = GameServer()->m_apPlayers[i]->m_TeeInfos.m_apSkinPartNames[p];
			Msg.m_aSkinPartColors[p] = GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinPartColors[p];
			Msg.m_aUseCustomColors[p] = GameServer()->m_apPlayers[i]->m_TeeInfos.m_aUseCustomColors[p];
		}

		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, -1);
    }
}

void CGameControllerCatch::ResetPlayerColors()
{
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(!GameServer()->m_apPlayers[i])
            continue;
        
        GameServer()->m_apPlayers[i]->m_CatchColor = Colors[i];
        GameServer()->m_apPlayers[i]->m_CatchOrigColor = Colors[i];
        str_copy(GameServer()->m_apPlayers[i]->m_CatchSkin, GameServer()->m_apPlayers[i]->m_CatchOrigSkin, MAX_SKIN_LENGTH);
    }

    UpdateSkins();
}

int CGameControllerCatch::GetPlayerAmount()
{
    int amount = 0;
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(!GameServer()->m_apPlayers[i])
            continue;

        if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS)
            continue;
        
        amount++;
    }
    return amount;
}

void CGameControllerCatch::FakeEndRound()
{
    //m_GameOverTick = Server()->Tick();
    GameServer()->m_World.m_Paused = true;
    m_RoundPauseTime = 150;

	GameServer()->SendBroadcast("Round Finish", -1);
    //SetGameState(IGS_END_MATCH, TIMER_END);
}

void CGameControllerCatch::KillEveryone()
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