#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "hFreeze.h"

CGameControllerHFreeze::CGameControllerHFreeze(class CGameContext *pGameServer) :
	CGameControllerFreeze(pGameServer)
{
    m_pGameType = "hFreeze+";
}

CGameControllerHFreeze::~CGameControllerHFreeze() = default;

void CGameControllerHFreeze::OnCharacterSpawn(class CCharacter *pChr)
{
    CGameControllerFreeze::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->ResetPickups();
	pChr->GiveWeapon(WEAPON_HAMMER);
	pChr->SetActiveWeapon(WEAPON_HAMMER);
}
