#ifndef GAME_SERVER_GAMEMODES_HFREEZE_H
#define GAME_SERVER_GAMEMODES_HFREEZE_H

#include "Freeze.h"

class CGameControllerHFreeze : public CGameControllerFreeze
{
public:
	CGameControllerHFreeze(class CGameContext *pGameServer);
	~CGameControllerHFreeze();

    void OnCharacterSpawn(class CCharacter *pChr) override;
	bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
};
#endif // GAME_SERVER_GAMEMODES_GFREEZE_H
