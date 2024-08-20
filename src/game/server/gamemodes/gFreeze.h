#ifndef GAME_SERVER_GAMEMODES_GFREEZE_H
#define GAME_SERVER_GAMEMODES_GFREEZE_H

#include "Freeze.h"

class CGameControllerGFreeze : public CGameControllerFreeze
{
public:
	CGameControllerGFreeze(class CGameContext *pGameServer);
	~CGameControllerGFreeze();

    void OnCharacterSpawn(class CCharacter *pChr) override;
};
#endif // GAME_SERVER_GAMEMODES_GFREEZE_H
