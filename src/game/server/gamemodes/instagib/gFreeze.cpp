#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "gFreeze.h"

CGameControllerGFreeze::CGameControllerGFreeze(class CGameContext *pGameServer) :
	CGameControllerFreeze(pGameServer)
{
    m_pGameType = "gFreeze+";
}

CGameControllerGFreeze::~CGameControllerGFreeze() = default;

void CGameControllerGFreeze::OnCharacterSpawn(class CCharacter *pChr)
{
    CGameControllerFreeze::OnCharacterSpawn(pChr);

    // give default weapons
    pChr->GiveWeapon(WEAPON_GRENADE, false, -1);
}
