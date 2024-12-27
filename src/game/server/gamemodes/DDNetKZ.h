/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_GAMEMODES_DDNETKZ_H
#define GAME_SERVER_GAMEMODES_DDNETKZ_H

#include <game/server/gamecontroller.h>
#include "DDRace.h"

class CGameControllerDDNetKZ : public CGameControllerDDRace
{
public:
	CGameControllerDDNetKZ(class CGameContext *pGameServer);
	~CGameControllerDDNetKZ();

	CScore *Score();
	void Tick() override;
	int SnapGameInfoExFlags2(int SnappingClient, int DDRaceFlags) override;
	void SetArmorProgress(CCharacter *pCharacer, int Progress) override{};
	void SetArmorProgressFull(CCharacter *pCharacer) override{};
	void SetArmorProgressEmpty(CCharacter *pCharacer) override{};
};
#endif // GAME_SERVER_GAMEMODES_DDRACE_H
