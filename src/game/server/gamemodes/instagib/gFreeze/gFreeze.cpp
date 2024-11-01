#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "gFreeze.h"

CGameControllerGFreeze::CGameControllerGFreeze(class CGameContext *pGameServer) :
	CGameControllerFreeze(pGameServer)
{
    m_pGameType = "gFreezeᵏᶻ";
	m_DefaultWeapon = WEAPON_GRENADE;
	
	m_pStatsTable = "gfreeze";
	m_pExtraColumns = nullptr;
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerGFreeze::~CGameControllerGFreeze() = default;

void CGameControllerGFreeze::OnCharacterSpawn(class CCharacter *pChr)
{
    CGameControllerFreeze::OnCharacterSpawn(pChr);

    // give default weapons
    pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
