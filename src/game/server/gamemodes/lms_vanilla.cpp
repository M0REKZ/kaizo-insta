#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "lms_vanilla.h"

CGameControllerLMSVanilla::CGameControllerLMSVanilla(class CGameContext *pGameServer) :
	CGameControllerLMS(pGameServer)
{
    m_VanillaBehavior = true;

    m_pGameType = "LMS+";
}

CGameControllerLMSVanilla::~CGameControllerLMSVanilla() = default;

void CGameControllerLMSVanilla::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerLMS::OnCharacterSpawn(pChr);

	//LMS
	pChr->IncreaseArmor(5);
	pChr->GiveWeapon(WEAPON_SHOTGUN, false, 10);
	pChr->GiveWeapon(WEAPON_GRENADE, false, 10);
	pChr->GiveWeapon(WEAPON_LASER, false, 5);
}
