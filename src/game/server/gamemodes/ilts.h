#ifndef GAME_SERVER_GAMEMODES_ILTS_H
#define GAME_SERVER_GAMEMODES_ILTS_H

#include "lts.h"

class CGameControllerILTS : public CGameControllerLTS
{

public:
	CGameControllerILTS(class CGameContext *pGameServer);
	~CGameControllerILTS();

	void OnCharacterSpawn(class CCharacter *pChr) override;

};
#endif // GAME_SERVER_GAMEMODES_LMS_H
