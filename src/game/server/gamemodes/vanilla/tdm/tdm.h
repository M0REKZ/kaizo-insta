#ifndef GAME_SERVER_GAMEMODES_VANILLA_TDM_H
#define GAME_SERVER_GAMEMODES_VANILLA_TDM_H

#include <game/server/gamemodes/vanilla/dm/dm.h>

class CGameControllerTDM : public CGameControllerDM
{
public:
	CGameControllerTDM(class CGameContext *pGameServer);
	~CGameControllerTDM() override;

	void Snap(int SnappingClient) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void Tick() override;
};
#endif
