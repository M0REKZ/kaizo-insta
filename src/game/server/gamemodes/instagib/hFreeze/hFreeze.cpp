#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "hFreeze.h"

CGameControllerHFreeze::CGameControllerHFreeze(class CGameContext *pGameServer) :
	CGameControllerFreeze(pGameServer)
{
    m_pGameType = "hFreeze+";
	m_DefaultWeapon = WEAPON_HAMMER;
	
	m_pStatsTable = "hfreeze";
	m_pExtraColumns = nullptr;
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerHFreeze::~CGameControllerHFreeze() = default;

void CGameControllerHFreeze::OnCharacterSpawn(class CCharacter *pChr)
{
    CGameControllerFreeze::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->ResetPickups();
	pChr->GiveWeapon(m_DefaultWeapon);
	//pChr->SetActiveWeapon(WEAPON_HAMMER);
}
