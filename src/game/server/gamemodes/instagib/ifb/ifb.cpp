#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "ifb.h"

CGameControllerIFB::CGameControllerIFB(class CGameContext *pGameServer) :
	CGameControllerInstaFB(pGameServer)
{
    m_pGameType = "iFBᵏᶻ";
	m_DefaultWeapon = WEAPON_LASER;
	
	m_pStatsTable = "ifb";
	m_pExtraColumns = new CIfbColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerIFB::~CGameControllerIFB() = default;

void CGameControllerIFB::OnCharacterSpawn(class CCharacter *pChr)
{
    CGameControllerInstaFB::OnCharacterSpawn(pChr);

    // give default weapons
    pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
