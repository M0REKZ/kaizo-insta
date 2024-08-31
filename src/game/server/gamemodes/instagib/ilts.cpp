#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "ilts.h"

CGameControllerILTS::CGameControllerILTS(class CGameContext *pGameServer) :
	CGameControllerInstaLTS(pGameServer)
{
    //m_GameFlags = GAMEFLAG_TEAMS;

    m_pGameType = "iLTS";
}

CGameControllerILTS::~CGameControllerILTS() = default;

void CGameControllerILTS::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaLTS::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_LASER, false, -1);
}
