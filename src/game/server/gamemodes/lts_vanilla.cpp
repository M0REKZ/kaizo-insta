#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "lts_vanilla.h"

CGameControllerLTSVanilla::CGameControllerLTSVanilla(class CGameContext *pGameServer) :
	CGameControllerLTS(pGameServer)
{
    m_VanillaBehavior = true;
    
    //m_GameFlags = GAMEFLAG_TEAMS;
    //m_GameFlags_v7 = protocol7::GAMEFLAG_TEAMS | protocol7::GAMEFLAG_SURVIVAL;

    m_pGameType = "LTS+";
}

CGameControllerLTSVanilla::~CGameControllerLTSVanilla() = default;

void CGameControllerLTSVanilla::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerLTS::OnCharacterSpawn(pChr);

	//LMS
	pChr->IncreaseArmor(5);
	pChr->GiveWeapon(WEAPON_SHOTGUN, false, 10);
	pChr->GiveWeapon(WEAPON_GRENADE, false, 10);
	pChr->GiveWeapon(WEAPON_LASER, false, 5);
}
