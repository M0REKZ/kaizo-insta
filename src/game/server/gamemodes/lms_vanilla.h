#ifndef GAME_SERVER_GAMEMODES_LMSVANILLA_H
#define GAME_SERVER_GAMEMODES_LMSVANILLA_H

#include "lms.h"

class CGameControllerLMSVanilla : public CGameControllerLMS
{
public:
	CGameControllerLMSVanilla(class CGameContext *pGameServer);
	~CGameControllerLMSVanilla();

	void OnCharacterSpawn(class CCharacter *pChr) override;

};
#endif // GAME_SERVER_GAMEMODES_DM_H
