#ifndef GAME_SERVER_GAMEMODES_GLMS_H
#define GAME_SERVER_GAMEMODES_GLMS_H

#include "lms.h"

class CGameControllerGLMS : public CGameControllerInstaLMS
{
public:
	CGameControllerGLMS(class CGameContext *pGameServer);
	~CGameControllerGLMS();

	void OnCharacterSpawn(class CCharacter *pChr) override;

};
#endif // GAME_SERVER_GAMEMODES_DM_H
