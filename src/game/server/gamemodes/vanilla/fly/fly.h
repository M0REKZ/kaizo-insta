#ifndef GAME_SERVER_GAMEMODES_VANILLA_FLY_FLY_H
#define GAME_SERVER_GAMEMODES_VANILLA_FLY_FLY_H

#include <game/server/gamemodes/vanilla/ctf/ctf.h>

class CGameControllerFly : public CGameControllerCTF
{
public:
	CGameControllerFly(class CGameContext *pGameServer);
	~CGameControllerFly() override;

	void Tick() override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
};
#endif
