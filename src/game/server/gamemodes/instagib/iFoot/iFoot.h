#ifndef GAME_SERVER_GAMEMODES_IFOOT_H
#define GAME_SERVER_GAMEMODES_IFOOT_H

#include <game/server/gamemodes/instagib/Foot.h>

class CGameControllerIFoot : public CGameControllerInstaFoot
{
public:
	CGameControllerIFoot(class CGameContext *pGameServer);
	~CGameControllerIFoot();

    void OnCharacterSpawn(class CCharacter *pChr) override;
};
#endif // GAME_SERVER_GAMEMODES_GFREEZE_H
