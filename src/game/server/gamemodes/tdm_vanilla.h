#ifndef GAME_SERVER_GAMEMODES_TDMVANILLA_H
#define GAME_SERVER_GAMEMODES_TDMVANILLA_H

#include "tdm.h"

class CGameControllerTDMVanilla : public CGameControllerTDM
{
public:
	CGameControllerTDMVanilla(class CGameContext *pGameServer);
	~CGameControllerTDMVanilla();
};
#endif // GAME_SERVER_GAMEMODES_TDM_H
