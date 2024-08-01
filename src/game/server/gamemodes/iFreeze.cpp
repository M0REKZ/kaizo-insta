#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "iFreeze.h"

CGameControllerIFreeze::CGameControllerIFreeze(class CGameContext *pGameServer) :
	CGameControllerTDM(pGameServer)
{
    m_GameFlags = GAMEFLAG_TEAMS;
    m_GameFlags_v7 = protocol7::GAMEFLAG_TEAMS;
    m_pGameType = "iFreeze+";
    
    m_DontSelfKill = true;
}

CGameControllerIFreeze::~CGameControllerIFreeze() = default;

void CGameControllerIFreeze::Tick()
{
	CGameControllerTDM::Tick();
    
    int Red = 0, Blue = 0, RedFr = 0, BlueFr = 0;

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_BLUE)
            {
                Blue++;
                if(GameServer()->GetPlayerChar(i) && GameServer()->GetPlayerChar(i)->GetCore().m_DeepFrozen)
                    BlueFr++;
            }
            else if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_RED)
            {
                Red++;
                if(GameServer()->GetPlayerChar(i) && GameServer()->GetPlayerChar(i)->GetCore().m_DeepFrozen)
                    RedFr++;
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
    }
}

int CGameControllerIFreeze::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	//CGameControllerInstagib::OnCharacterDeath(pVictim, pKiller, WeaponId);

    /*
	if(pKiller && WeaponId != WEAPON_GAME)
	{
		// do team scoring
		if(pKiller == pVictim->GetPlayer() || pKiller->GetTeam() == pVictim->GetPlayer()->GetTeam())
			m_aTeamscore[pKiller->GetTeam() & 1]--;
		else
			m_aTeamscore[pKiller->GetTeam() & 1]++;
	}
     */

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

bool CGameControllerIFreeze::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
    Dmg = 0;
    CGameControllerTDM::OnCharacterTakeDamage(Force, Dmg, From, Weapon, Character);
    if(GameServer()->m_apPlayers[From] == Character.GetPlayer())
        return false;
    if(GameServer()->m_apPlayers[From]->GetTeam() == Character.GetPlayer()->GetTeam())
        return false;
    if(GameServer()->m_apPlayers[From])
    {
        Character.Freeze();
        Character.SetDeepFrozen(true);
    }
    return false;
}

void CGameControllerIFreeze::ResetFrozenPlayer()
{
    for(int i = 0; i < MAX_CLIENTS; i++)
        if(GameServer()->m_apPlayers[i] && GameServer()->GetPlayerChar(i))
        {
            GameServer()->m_apPlayers[i]->KillCharacter(WEAPON_SELF);
            GameServer()->m_apPlayers[i]->Respawn();
        }
}

void CGameControllerIFreeze::OnCharacterSpawn(class CCharacter *pChr)
{
    CGameControllerTDM::OnCharacterSpawn(pChr);

    // give default weapons
    pChr->GiveWeapon(WEAPON_LASER, false, -1);
}
