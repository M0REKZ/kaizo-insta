#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "iFreeze.h"

CGameControllerIFreeze::CGameControllerIFreeze(class CGameContext *pGameServer) :
	CGameControllerFreeze(pGameServer)
{
    m_pGameType = "iFreeze+";
	m_DefaultWeapon = WEAPON_LASER;
	
	m_pStatsTable = "ifreeze";
	m_pExtraColumns = new CIfreezeColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerIFreeze::~CGameControllerIFreeze() = default;

void CGameControllerIFreeze::OnCharacterSpawn(class CCharacter *pChr)
{
    CGameControllerFreeze::OnCharacterSpawn(pChr);

    // give default weapons
    pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
