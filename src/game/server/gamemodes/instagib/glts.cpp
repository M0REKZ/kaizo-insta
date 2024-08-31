#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "glts.h"

CGameControllerGLTS::CGameControllerGLTS(class CGameContext *pGameServer) :
	CGameControllerInstaLTS(pGameServer)
{
    //m_VanillaBehavior = true;
    
    //m_GameFlags = GAMEFLAG_TEAMS;
    //m_GameFlags_v7 = protocol7::GAMEFLAG_TEAMS | protocol7::GAMEFLAG_SURVIVAL;

    m_pGameType = "gLTS";
}

CGameControllerGLTS::~CGameControllerGLTS() = default;

void CGameControllerGLTS::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaLTS::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_GRENADE, false, -1);
}
