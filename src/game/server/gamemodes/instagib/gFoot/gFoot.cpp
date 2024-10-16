#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "gFoot.h"

CGameControllerGFoot::CGameControllerGFoot(class CGameContext *pGameServer) :
CGameControllerInstaFoot(pGameServer)
{
    m_pGameType = "gFoot";
	m_DefaultWeapon = WEAPON_GRENADE;
}

CGameControllerGFoot::~CGameControllerGFoot() = default;

void CGameControllerGFoot::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaFoot::OnCharacterSpawn(pChr);

    // give default weapons
    pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
