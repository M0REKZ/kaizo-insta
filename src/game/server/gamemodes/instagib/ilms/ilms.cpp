#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "ilms.h"

CGameControllerILMS::CGameControllerILMS(class CGameContext *pGameServer) :
	CGameControllerInstaLMS(pGameServer)
{
    m_pGameType = "iLMSᵏᶻ";
	m_DefaultWeapon = WEAPON_LASER;
	
	m_pStatsTable = "ilms";
	m_pExtraColumns = new CIlmsColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerILMS::~CGameControllerILMS() = default;

void CGameControllerILMS::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaLMS::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
