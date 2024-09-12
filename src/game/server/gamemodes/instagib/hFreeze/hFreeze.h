#ifndef GAME_SERVER_GAMEMODES_HFREEZE_H
#define GAME_SERVER_GAMEMODES_HFREEZE_H

#include <game/server/gamemodes/instagib/Freeze.h>

class CGameControllerHFreeze : public CGameControllerFreeze
{
public:
	CGameControllerHFreeze(class CGameContext *pGameServer);
	~CGameControllerHFreeze();

    void OnCharacterSpawn(class CCharacter *pChr) override;
};
#endif // GAME_SERVER_GAMEMODES_GFREEZE_H
