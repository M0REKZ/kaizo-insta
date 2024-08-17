#include "ctf_vanilla.h"

#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

CGameControllerCTFVanilla::CGameControllerCTFVanilla(class CGameContext *pGameServer) :
    CGameControllerCTF(pGameServer)
{
    m_VanillaBehavior = true;

    m_pGameType = "CTF+";
}

CGameControllerCTFVanilla::~CGameControllerCTFVanilla() = default;

void CGameControllerCTFVanilla::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerCTF::OnCharacterSpawn(pChr);
}

bool CGameControllerCTFVanilla::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
	return CGameControllerInstagib::OnCharacterTakeDamage(Force,Dmg,From,Weapon,Character);
}

int CGameControllerCTFVanilla::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
    return CGameControllerCTF::OnCharacterDeath(pVictim, pKiller, WeaponId);
}
