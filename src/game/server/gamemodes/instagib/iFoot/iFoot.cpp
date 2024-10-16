#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "iFoot.h"

CGameControllerIFoot::CGameControllerIFoot(class CGameContext *pGameServer) :
CGameControllerInstaFoot(pGameServer)
{
    m_pGameType = "iFoot";
	m_DefaultWeapon = WEAPON_LASER;
}

CGameControllerIFoot::~CGameControllerIFoot() = default;

void CGameControllerIFoot::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaFoot::OnCharacterSpawn(pChr);

    // give default weapons
    pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
