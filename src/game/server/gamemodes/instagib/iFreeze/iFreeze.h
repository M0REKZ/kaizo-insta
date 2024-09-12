#ifndef GAME_SERVER_GAMEMODES_IFREEZE_H
#define GAME_SERVER_GAMEMODES_IFREEZE_H

#include <game/server/gamemodes/instagib/Freeze.h>

class CGameControllerIFreeze : public CGameControllerFreeze
{
public:
	CGameControllerIFreeze(class CGameContext *pGameServer);
	~CGameControllerIFreeze();

    void OnCharacterSpawn(class CCharacter *pChr) override;
};
#endif // GAME_SERVER_GAMEMODES_IFREEZE_H
