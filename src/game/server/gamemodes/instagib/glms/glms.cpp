#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "glms.h"

CGameControllerGLMS::CGameControllerGLMS(class CGameContext *pGameServer) :
	CGameControllerInstaLMS(pGameServer)
{
    m_pGameType = "gLMSᵏᶻ";
	m_DefaultWeapon = WEAPON_GRENADE;
	
	m_pStatsTable = "glms";
	m_pExtraColumns = nullptr;
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerGLMS::~CGameControllerGLMS() = default;

void CGameControllerGLMS::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaLMS::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
