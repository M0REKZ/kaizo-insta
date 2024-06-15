#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "dm_vanilla.h"

CGameControllerDMVanilla::CGameControllerDMVanilla(class CGameContext *pGameServer) :
	CGameControllerCTFVanilla(pGameServer)
{
    m_VanillaBehavior = true;
    
    m_GameFlags = 0;
    m_GameFlags_v7 = 0;

    m_pGameType = "DM+";
}

CGameControllerDMVanilla::~CGameControllerDMVanilla() = default;

void CGameControllerDMVanilla::Tick()
{
	CGameControllerCTFVanilla::Tick();
}

void CGameControllerDMVanilla::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerCTFVanilla::OnCharacterSpawn(pChr);
}

bool CGameControllerDMVanilla::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
    return CGameControllerInstagib::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
}

bool CGameControllerDMVanilla::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
    return CGameControllerCTFVanilla::OnCharacterTakeDamage(Force, Dmg, From, Weapon, Character);
}

int CGameControllerDMVanilla::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	CGameControllerCTFVanilla::OnCharacterDeath(pVictim, pKiller, WeaponId);

	// check score win condition
	if((m_GameInfo.m_ScoreLimit > 0 && pKiller->m_Score.value_or(0) >= m_GameInfo.m_ScoreLimit) ||
		(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick() - m_GameStartTick) >= m_GameInfo.m_TimeLimit * Server()->TickSpeed() * 60))
	{
		EndMatch();
		return true;
	}
	return false;
}
