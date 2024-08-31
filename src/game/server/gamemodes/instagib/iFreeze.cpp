#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "iFreeze.h"

CGameControllerIFreeze::CGameControllerIFreeze(class CGameContext *pGameServer) :
	CGameControllerFreeze(pGameServer)
{
    m_pGameType = "iFreeze+";
}

CGameControllerIFreeze::~CGameControllerIFreeze() = default;

void CGameControllerIFreeze::OnCharacterSpawn(class CCharacter *pChr)
{
    CGameControllerFreeze::OnCharacterSpawn(pChr);

    // give default weapons
    pChr->GiveWeapon(WEAPON_LASER, false, -1);
}
