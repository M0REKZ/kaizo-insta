#ifndef GAME_SERVER_GAMEMODES_ILMS_H
#define GAME_SERVER_GAMEMODES_ILMS_H

#include "lms.h"

class CGameControllerILMS : public CGameControllerLMS
{
public:
	CGameControllerILMS(class CGameContext *pGameServer);
	~CGameControllerILMS();

	void OnCharacterSpawn(class CCharacter *pChr) override;

};
#endif // GAME_SERVER_GAMEMODES_DM_H
