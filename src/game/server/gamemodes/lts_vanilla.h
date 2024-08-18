#ifndef GAME_SERVER_GAMEMODES_LTSVANILLA_H
#define GAME_SERVER_GAMEMODES_LTSVANILLA_H

#include "lts.h"

class CGameControllerLTSVanilla : public CGameControllerLTS
{

public:
	CGameControllerLTSVanilla(class CGameContext *pGameServer);
	~CGameControllerLTSVanilla();

	void OnCharacterSpawn(class CCharacter *pChr) override;

};
#endif // GAME_SERVER_GAMEMODES_LMS_H
