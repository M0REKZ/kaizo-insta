#ifndef GAME_SERVER_GAMEMODES_GLTS_H
#define GAME_SERVER_GAMEMODES_GLTS_H

#include <game/server/gamemodes/instagib/lts.h>

class CGameControllerGLTS : public CGameControllerInstaLTS
{

public:
	CGameControllerGLTS(class CGameContext *pGameServer);
	~CGameControllerGLTS();

	void OnCharacterSpawn(class CCharacter *pChr) override;

};
#endif // GAME_SERVER_GAMEMODES_LMS_H
