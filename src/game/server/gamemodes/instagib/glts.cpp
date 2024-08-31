#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "glts.h"

CGameControllerGLTS::CGameControllerGLTS(class CGameContext *pGameServer) :
	CGameControllerInstaLTS(pGameServer)
{

    //m_GameFlags = GAMEFLAG_TEAMS;

    m_pGameType = "gLTS";
}

CGameControllerGLTS::~CGameControllerGLTS() = default;

void CGameControllerGLTS::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaLTS::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_GRENADE, false, -1);
}
