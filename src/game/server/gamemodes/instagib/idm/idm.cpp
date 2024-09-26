#include <game/server/entities/character.h>

#include "idm.h"

CGameControllerIDM::CGameControllerIDM(class CGameContext *pGameServer) :
	CGameControllerInstaBaseDM(pGameServer)
{
	m_pGameType = "iDM";
	m_DefaultWeapon = WEAPON_LASER;

	m_pStatsTable = "idm";
	m_pExtraColumns = new CIdmColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerIDM::~CGameControllerIDM() = default;

void CGameControllerIDM::Tick()
{
	CGameControllerInstaBaseDM::Tick();
}

void CGameControllerIDM::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaBaseDM::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
