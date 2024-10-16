#ifndef GAME_SERVER_GAMEMODES_GFOOT_H
#define GAME_SERVER_GAMEMODES_GFOOT_H

#include <game/server/gamemodes/instagib/Foot.h>

class CGameControllerGFoot : public CGameControllerInstaFoot
{
public:
	CGameControllerGFoot(class CGameContext *pGameServer);
	~CGameControllerGFoot();

    void OnCharacterSpawn(class CCharacter *pChr) override;
};
#endif // GAME_SERVER_GAMEMODES_GFREEZE_H
