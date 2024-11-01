#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "ilts.h"

CGameControllerILTS::CGameControllerILTS(class CGameContext *pGameServer) :
	CGameControllerInstaLTS(pGameServer)
{
    m_pGameType = "iLTSᵏᶻ";
	m_DefaultWeapon = WEAPON_LASER;
	
	m_pStatsTable = "ilts";
	m_pExtraColumns = new CIltsColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerILTS::~CGameControllerILTS() = default;

void CGameControllerILTS::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaLTS::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
