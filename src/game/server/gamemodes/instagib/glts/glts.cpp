#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "glts.h"

CGameControllerGLTS::CGameControllerGLTS(class CGameContext *pGameServer) :
	CGameControllerInstaLTS(pGameServer)
{
    m_pGameType = "gLTS";
	m_DefaultWeapon = WEAPON_GRENADE;
}

CGameControllerGLTS::~CGameControllerGLTS() = default;

void CGameControllerGLTS::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaLTS::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
