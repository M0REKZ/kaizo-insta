#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "iFoot.h"

CGameControllerIFoot::CGameControllerIFoot(class CGameContext *pGameServer) :
CGameControllerInstaFoot(pGameServer)
{
    m_pGameType = "iFootᵏᶻ";
	m_DefaultWeapon = WEAPON_LASER;
	
	m_pStatsTable = "ifoot";
	m_pExtraColumns = new CIfootColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerIFoot::~CGameControllerIFoot() = default;

void CGameControllerIFoot::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaFoot::OnCharacterSpawn(pChr);

    // give default weapons
    pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
