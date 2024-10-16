#ifndef GAME_SERVER_GAMEMODES_INSTAGIB_FOOT_H
#define GAME_SERVER_GAMEMODES_INSTAGIB_FOOT_H

#include <game/server/gamemodes/instagib/tdm.h>

class CGameControllerInstaFoot : public CGameControllerInstaTDM
{
public:
	CGameControllerInstaFoot(class CGameContext *pGameServer);
	~CGameControllerInstaFoot() override;

	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void Tick() override;
};
#endif
