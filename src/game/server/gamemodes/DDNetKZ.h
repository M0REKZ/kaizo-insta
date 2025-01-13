/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_GAMEMODES_DDNETKZ_H
#define GAME_SERVER_GAMEMODES_DDNETKZ_H

#include <game/server/gamecontroller.h>
#include "DDRace.h"
#include <game/server/entities/kz/flagball.h>

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
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
	void Snap(int SnappingClient) override;
	bool OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos) override;

	//Flag(ball xD)
	class CFlagBall *m_apFlagBalls[2];

	int m_flagstand_temp_i_0; //+KZ from tw_plus
	int m_flagstand_temp_i_1; //same
};
#endif // GAME_SERVER_GAMEMODES_DDRACE_H
