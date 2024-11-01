#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "glts.h"

CGameControllerGLTS::CGameControllerGLTS(class CGameContext *pGameServer) :
	CGameControllerInstaLTS(pGameServer)
{
    m_pGameType = "gLTSᵏᶻ";
	m_DefaultWeapon = WEAPON_GRENADE;
	
	m_pStatsTable = "glts";
	m_pExtraColumns = nullptr;
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerGLTS::~CGameControllerGLTS() = default;

void CGameControllerGLTS::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaLTS::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
