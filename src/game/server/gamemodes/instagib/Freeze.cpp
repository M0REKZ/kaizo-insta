#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <engine/shared/config.h>

#include "Freeze.h"

CGameControllerFreeze::CGameControllerFreeze(class CGameContext *pGameServer) :
	CGameControllerInstaTDM(pGameServer)
{
    m_GameFlags = GAMEFLAG_TEAMS;
    
    m_DontSelfKill = true;
}

CGameControllerFreeze::~CGameControllerFreeze() = default;

void CGameControllerFreeze::Tick()
{
	CGameControllerInstaTDM::Tick();
    
    int Red = 0, Blue = 0, RedFr = 0, BlueFr = 0;

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_BLUE)
            {
                Blue++;
                if(GameServer()->GetPlayerChar(i) && GameServer()->GetPlayerChar(i)->GetCore().m_DeepFrozen)
                {
                    BlueFr++;
                    //DoMelting(GameServer()->GetPlayerChar(i));
                }
            }
            else if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_RED)
            {
                Red++;
                if(GameServer()->GetPlayerChar(i) && GameServer()->GetPlayerChar(i)->GetCore().m_DeepFrozen)
                {
                    RedFr++;
                    //DoMelting(GameServer()->GetPlayerChar(i));
                }
            }
			
			if(GameServer()->GetPlayerChar(i) && (GameServer()->GetPlayerChar(i)->m_FreezeTime || GameServer()->GetPlayerChar(i)->GetCore().m_DeepFrozen))
			{
				DoMelting(GameServer()->GetPlayerChar(i));
			}
        }
    }

    bool BlueScored = RedFr >= Red && Red && RedFr;
    if(BlueScored || (BlueFr >= Blue && Blue && BlueFr))
    {
        m_aTeamscore[(BlueScored) ? TEAM_BLUE : TEAM_RED]++;
        ResetFrozenPlayer();
        GameServer()->SendBroadcast((BlueScored) ? "Blue team scores" : "Red team scores", -1);
        GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
		
		
		//DoWinCheck
		if((m_GameInfo.m_ScoreLimit > 0 && (m_aTeamscore[TEAM_RED] >= m_GameInfo.m_ScoreLimit || m_aTeamscore[TEAM_BLUE] >= m_GameInfo.m_ScoreLimit)) ||
			(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick() - m_GameStartTick) >= m_GameInfo.m_TimeLimit * Server()->TickSpeed() * 60))
		{
			if(m_SuddenDeath)
			{
				if(m_aTeamscore[TEAM_RED] / 100 != m_aTeamscore[TEAM_BLUE] / 100)
				{
					EndRound();
					//return true;
				}
			}
			else
			{
				if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE])
				{
					EndRound();
					//return true;
				}
				else
					m_SuddenDeath = 1;
			}
		}
		
    }
}

int CGameControllerFreeze::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	return CGameControllerInstagib::OnCharacterDeath(pVictim, pKiller, WeaponId);
}

bool CGameControllerFreeze::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
	bool noOwner = false;
	
	if(Character.m_IsGodmode)
		return true;
	if(From < 0 || From > MAX_CLIENTS) //only valid CID
		noOwner = true;
	if(!noOwner && GameServer()->m_pController->IsFriendlyFire(Character.GetPlayer()->GetCid(), From))
		return false;
	
	if(g_Config.m_SvOnlyHookKills && !noOwner)
	{
		CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
		if(!pChr || pChr->GetCore().HookedPlayer() != Character.GetPlayer()->GetCid())
			return false;
	}
	
	bool frz = false;
	if(Dmg >= g_Config.m_SvDamageNeededForKill || Weapon == WEAPON_LASER || Weapon == WEAPON_HAMMER)
	{
		frz = true;
		Dmg = g_Config.m_SvDamageNeededForKill - 1; //dont kill;
	}
	Dmg = 0;
		
    //CGameControllerInstaTDM::OnCharacterTakeDamage(Force, Dmg, From, Weapon, Character);
	
	if(frz)
	{
		if(DoFreezing(From, Character))
		{
			// kill message
			CNetMsg_Sv_KillMsg Msg;
			if(noOwner)
				Msg.m_Killer = Character.GetPlayer()->GetCid();
			else
				Msg.m_Killer = From;
			Msg.m_Victim = Character.GetPlayer()->GetCid();
			Msg.m_Weapon = Weapon;
			Msg.m_ModeSpecial = 0;
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
		}
	}
	
    return false;
}

bool CGameControllerFreeze::DoFreezing(int &From, CCharacter &Character)
{
	if(Character.IsAlive())
	{
		bool noOwner = false;
		if(From < 0 || From > MAX_CLIENTS) //only valid CID
			noOwner = true;
		
		if(!noOwner && GameServer()->m_apPlayers[From] == Character.GetPlayer())
			return false;
		if(!noOwner && GameServer()->m_apPlayers[From]->GetTeam() == Character.GetPlayer()->GetTeam())
			return false;
		Character.Freeze();
		Character.SetDeepFrozen(true);
		Character.GetPlayer()->m_AutoMeltTicks = g_Config.m_SvFreezeAutomeltTime * Server()->TickSpeed();
		
		GameServer()->CreatePlayerSpawn(Character.m_Pos, Character.TeamMask());
		GameServer()->CreateSound(Character.m_Pos, SOUND_NINJA_HIT);
		
		char aBuf[128];
		if(!noOwner)
		{
			str_format(aBuf, sizeof(aBuf), "You froze %s", Server()->ClientName(Character.GetPlayer()->GetCid()));
			GameServer()->SendBroadcast(aBuf, From);
			str_format(aBuf, sizeof(aBuf), "%s froze you", Server()->ClientName(From));
			GameServer()->SendBroadcast(aBuf, Character.GetPlayer()->GetCid());
		}
		else
		{
			GameServer()->SendBroadcast("You froze by yourself", Character.GetPlayer()->GetCid());
		}
		
		return true;
	}
	return false;
}
	

void CGameControllerFreeze::ResetFrozenPlayer()
{
    for(int i = 0; i < MAX_CLIENTS; i++)
        if(GameServer()->m_apPlayers[i] && GameServer()->GetPlayerChar(i))
        {
            GameServer()->m_apPlayers[i]->GetCharacter()->Destroy();
            GameServer()->m_apPlayers[i]->Respawn();
        }
}

void CGameControllerFreeze::DoMelting(class CCharacter *pChr)
{
    
    if(pChr->GetPlayer()->m_AutoMeltTicks > 0)
    {
        pChr->GetPlayer()->m_AutoMeltTicks--;
    }
    else
    {
        pChr->GetPlayer()->m_AutoMeltTicks = 0;
        Melt(pChr->GetPlayer()->GetCid(),0);
        return;
    }
    
    bool FoundMelter = false;
    CCharacter *apCloseChars[MAX_CLIENTS];
    int Num = GameServer()->m_World.FindEntities(pChr->m_Pos, g_Config.m_SvFreezeMeltRange, (CEntity **)apCloseChars, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
    for(int i = 0; i < Num; i++)
    {
        if (!apCloseChars[i])
            continue;
        
        if (!apCloseChars[i]->IsAlive() || apCloseChars[i] == pChr || apCloseChars[i]->GetCore().m_DeepFrozen)
            continue;
        
        if (apCloseChars[i]->GetPlayer()->GetTeam() == pChr->GetPlayer()->GetTeam())
        {
            pChr->GetPlayer()->m_MeltTicks++;
            FoundMelter = true;
            // Send "thawed" on half of melttime
            if (pChr->GetPlayer()->m_MeltTicks == (int)(Server()->TickSpeed() * g_Config.m_SvFreezeMeltTime * 0.0005f))
                GameServer()->SendBroadcast("You are being thawed", pChr->GetPlayer()->GetCid());
            else if (pChr->GetPlayer()->m_MeltTicks >= Server()->TickSpeed() * g_Config.m_SvFreezeMeltTime * 0.001f)
                Melt(pChr->GetPlayer()->GetCid(),apCloseChars[i]->GetPlayer()->GetCid());
            break;
        }
    }
    
    // set counter to 0 if melter went away or no melter found
    if (!FoundMelter)
        pChr->GetPlayer()->m_MeltTicks = 0;
    
}

void CGameControllerFreeze::Melt(int Melted, int Helper)
{
    GameServer()->m_apPlayers[Melted]->GetCharacter()->SetDeepFrozen(false);
    GameServer()->m_apPlayers[Melted]->GetCharacter()->UnFreeze();
    GameServer()->CreateSound(GameServer()->m_apPlayers[Melted]->GetCharacter()->m_Pos, SOUND_GRENADE_EXPLODE);
	GameServer()->CreateDeath(GameServer()->m_apPlayers[Melted]->GetCharacter()->m_Pos, Melted, GameServer()->m_apPlayers[Melted]->GetCharacter()->TeamMask());
    
    if(g_Config.m_SvFreezeMeltRespawn)
    {
        GameServer()->m_apPlayers[Melted]->GetCharacter()->Destroy();
        GameServer()->m_apPlayers[Melted]->Respawn();
    }
    
    if(Helper > 0)
    {
        char aBuf[128];
        str_format(aBuf, sizeof(aBuf), "You melted %s", Server()->ClientName(Melted));
        GameServer()->SendBroadcast(aBuf, Helper);
        str_format(aBuf, sizeof(aBuf), "%s melted you", Server()->ClientName(Helper));
        GameServer()->SendBroadcast(aBuf, Melted);
    }

    
}

void CGameControllerFreeze::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaTDM::OnCharacterSpawn(pChr);

	pChr->GetPlayer()->m_AutoMeltTicks = g_Config.m_SvFreezeAutomeltTime * Server()->TickSpeed();
}

