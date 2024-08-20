#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "glms.h"

CGameControllerGLMS::CGameControllerGLMS(class CGameContext *pGameServer) :
	CGameControllerLMS(pGameServer)
{
    //m_VanillaBehavior = true;

    m_pGameType = "gLMS";
}

CGameControllerGLMS::~CGameControllerGLMS() = default;

void CGameControllerGLMS::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerLMS::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_GRENADE, false, -1);
}
