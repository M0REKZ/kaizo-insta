#ifndef GAME_SERVER_GAMEMODES_DMVANILLA_H
#define GAME_SERVER_GAMEMODES_DMVANILLA_H

#include "dm.h"

class CGameControllerDMVanilla : public CGameControllerDM
{
public:
	CGameControllerDMVanilla(class CGameContext *pGameServer);
	~CGameControllerDMVanilla();
};
#endif // GAME_SERVER_GAMEMODES_DM_H
