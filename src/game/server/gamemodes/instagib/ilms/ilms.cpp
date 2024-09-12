#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "ilms.h"

CGameControllerILMS::CGameControllerILMS(class CGameContext *pGameServer) :
	CGameControllerInstaLMS(pGameServer)
{
    m_pGameType = "iLMS";
	m_DefaultWeapon = WEAPON_LASER;
}

CGameControllerILMS::~CGameControllerILMS() = default;

void CGameControllerILMS::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaLMS::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
