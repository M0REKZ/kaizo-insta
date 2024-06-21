#ifndef GAME_SERVER_GAMEMODES_TDMVANILLA_H
#define GAME_SERVER_GAMEMODES_TDMVANILLA_H

#include "dm_vanilla.h"

class CGameControllerTDMVanilla : public CGameControllerDMVanilla
{
public:
	CGameControllerTDMVanilla(class CGameContext *pGameServer);
	~CGameControllerTDMVanilla();

	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void Tick() override;
};
#endif // GAME_SERVER_GAMEMODES_TDM_H
