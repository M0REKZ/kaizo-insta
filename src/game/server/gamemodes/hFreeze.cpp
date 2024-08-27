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

bool CGameControllerHFreeze::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
	if((Weapon == WEAPON_HAMMER ? false : (!(Dmg >= g_Config.m_SvDamageNeededForKill))))
		return false;
	Dmg = 0;
	CGameControllerTDM::OnCharacterTakeDamage(Force, Dmg, From, Weapon, Character);
	
	DoFreezing(From, Character);
	
	return false;
}
