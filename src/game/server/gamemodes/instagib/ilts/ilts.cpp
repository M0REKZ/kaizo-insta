#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "ilts.h"

CGameControllerILTS::CGameControllerILTS(class CGameContext *pGameServer) :
	CGameControllerInstaLTS(pGameServer)
{
    m_pGameType = "iLTS";
	m_DefaultWeapon = WEAPON_LASER;
}

CGameControllerILTS::~CGameControllerILTS() = default;

void CGameControllerILTS::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaLTS::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}
