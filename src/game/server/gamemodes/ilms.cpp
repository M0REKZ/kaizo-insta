#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "ilms.h"

CGameControllerILMS::CGameControllerILMS(class CGameContext *pGameServer) :
	CGameControllerLMS(pGameServer)
{
    //m_VanillaBehavior = true;

    m_pGameType = "iLMS";
}

CGameControllerILMS::~CGameControllerILMS() = default;

void CGameControllerILMS::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerLMS::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_LASER, false, -1);
}
