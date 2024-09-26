#include <game/server/entities/character.h>
#include <game/server/gamemodes/instagib/idm/idm.h>

#include "itdm.h"

CGameControllerITDM::CGameControllerITDM(class CGameContext *pGameServer) :
	CGameControllerInstaTDM(pGameServer)
{
	m_pGameType = "iTDM";
	m_DefaultWeapon = WEAPON_LASER;

	m_pStatsTable = "itdm";
	m_pExtraColumns = new CIdmColumns(); // yes itdm and idm have the same db columns
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerITDM::~CGameControllerITDM() = default;

void CGameControllerITDM::Tick()
{
	CGameControllerInstaTDM::Tick();
}

void CGameControllerITDM::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaTDM::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
