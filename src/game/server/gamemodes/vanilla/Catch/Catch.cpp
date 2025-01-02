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
    int Hue = 0x000000;
    int Sat = 0x00FF00;
    int Lht = 0x000000;
    for(int i = 2; i < MAX_CLIENTS; i++)
    {
        
        Colors[i] = Hue + Sat + Lht;
        Hue += 0x1A0000;
        if(Hue >= 0xEF0000)
        {
            if(Lht < 0x0000EF)
                Lht += 0x000050;
            Hue = 0x000000;
        }
        if(Lht >= 0x0000EF)
        {
            if(Sat > 0x000000)
                Sat -= 0x005000;
            Lht = 0x000000;
        }
        if(Sat <= 0x001F00)
        {
            Hue = 0x000000;
            Sat = 0x00FF00;
            Lht = 0x000000;
        }
    }
}

CGameControllerCatch::~CGameControllerCatch() = default;

void CGameControllerCatch::Tick()
{
    CGameControllerDM::Tick();

    if(g_Config.m_SvCatchWeapon == WEAPON_NINJA && (Server()->Tick() % (Server()->TickSpeed() * 5) == 0))
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
			{
				GameServer()->m_apPlayers[i]->GetCharacter()->SetNinjaActivationTick(Server()->Tick());
			}
		}
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
    if(!(Dmg >= g_Config.m_SvDamageNeededForKill || Weapon == WEAPON_LASER || Weapon == WEAPON_HAMMER || Weapon == WEAPON_GUN || Weapon == WEAPON_SHOTGUN))
        return false;

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
    if(!GameServer()->m_apPlayers[From])
        return false;
    if(GameServer()->m_apPlayers[From] == Character.GetPlayer())
        return false;
    //if(Character.GetPlayer()->m_IsBomb)
     //   return false;
    Character.GetPlayer()->m_CatchColor = GameServer()->m_apPlayers[From]->m_CatchColor;
    str_copy(Character.GetPlayer()->m_CatchSkin, GameServer()->m_apPlayers[From]->m_CatchSkin, MAX_SKIN_LENGTH);
    UpdateSkins();

    if(Character.GetPlayer()->m_CatchColor != GameServer()->m_apPlayers[From]->m_CatchColor)
    {
        CNetMsg_Sv_KillMsg Msg;
        Msg.m_Killer = From;
        Msg.m_Victim = Character.GetPlayer()->GetCid();
        Msg.m_Weapon = Weapon;
        Msg.m_ModeSpecial = 0;
        Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
    }

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
    

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(!GameServer()->m_apPlayers[i])
            continue;

        if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS)
            continue;

        if(WinColor != GameServer()->m_apPlayers[i]->m_CatchColor)
            return false;
    }

    //search winner

    for(int i = 0; i < MAX_CLIENTS; i++)
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
    if(!pPlayer->m_CatchOrigColorSet && pPlayer->m_TeeInfos.m_aSkinName[0])
    {
        pPlayer->m_CatchColor = Colors[pPlayer->GetCid()];
        pPlayer->m_CatchOrigColor = pPlayer->m_CatchColor;
        str_copy(pPlayer->m_CatchOrigSkin, pPlayer->m_TeeInfos.m_aSkinName, MAX_SKIN_LENGTH);
        str_copy(pPlayer->m_CatchSkin, pPlayer->m_CatchOrigSkin, MAX_SKIN_LENGTH);
        pPlayer->m_CatchOrigColorSet = true;
        for(int a = 0; a < 6; a++)
        {
            str_copy(pPlayer->m_CatchSkinPartNames[a], pPlayer->m_TeeInfos.m_apSkinPartNames[a], 24);
            str_copy(pPlayer->m_CatchOrigSkinPartNames[a], pPlayer->m_TeeInfos.m_apSkinPartNames[a], 24);
        }
    }
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
    
    if(!pChr->GetPlayer()->m_CatchOrigColorSet && pChr->GetPlayer()->m_TeeInfos.m_aSkinName[0])
    {
        pChr->GetPlayer()->m_CatchColor = Colors[pChr->GetPlayer()->GetCid()];
        pChr->GetPlayer()->m_CatchOrigColor = pChr->GetPlayer()->m_CatchColor;
        str_copy(pChr->GetPlayer()->m_CatchOrigSkin, pChr->GetPlayer()->m_TeeInfos.m_aSkinName, MAX_SKIN_LENGTH);
        str_copy(pChr->GetPlayer()->m_CatchSkin, pChr->GetPlayer()->m_CatchOrigSkin, MAX_SKIN_LENGTH);
        pChr->GetPlayer()->m_CatchOrigColorSet = true;
        for(int a = 0; a < 6; a++)
        {
            str_copy(pChr->GetPlayer()->m_CatchSkinPartNames[a], pChr->GetPlayer()->m_TeeInfos.m_apSkinPartNames[a], 24);
            str_copy(pChr->GetPlayer()->m_CatchOrigSkinPartNames[a], pChr->GetPlayer()->m_TeeInfos.m_apSkinPartNames[a], 24);
        }
    }
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

        for(int a = 0; a < 6; a++)
        {
            GameServer()->m_apPlayers[i]->m_TeeInfos.m_aUseCustomColors[a] = true;
            GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinPartColors[a] = ColorHSLA(GameServer()->m_apPlayers[i]->m_CatchColor).UnclampLighting(ColorHSLA::DARKEST_LGT).Pack(ColorHSLA::DARKEST_LGT7);
            str_copy(GameServer()->m_apPlayers[i]->m_TeeInfos.m_apSkinPartNames[a], GameServer()->m_apPlayers[i]->m_CatchSkinPartNames[a], 24);
        }

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
        for(int a = 0; a < 6; a++)
        {
            GameServer()->m_apPlayers[i]->m_TeeInfos.m_aUseCustomColors[a] = true;
            GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinPartColors[a] = ColorHSLA(GameServer()->m_apPlayers[i]->m_CatchOrigColor).UnclampLighting(ColorHSLA::DARKEST_LGT).Pack(ColorHSLA::DARKEST_LGT7);
            str_copy(GameServer()->m_apPlayers[i]->m_TeeInfos.m_apSkinPartNames[a], GameServer()->m_apPlayers[i]->m_CatchOrigSkinPartNames[a], 24);
        }

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
        
        if(!GameServer()->m_apPlayers[i]->GetCharacter())
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