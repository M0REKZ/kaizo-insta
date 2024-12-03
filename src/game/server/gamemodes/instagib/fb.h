#ifndef GAME_SERVER_GAMEMODES_INSTAGIB_FB_H
#define GAME_SERVER_GAMEMODES_INSTAGIB_FB_H

#include <game/server/gamemodes/instagib/base_instagib.h>
#include <game/server/entities/kz/flagball.h>

class CGameControllerInstaFB : public CGameControllerInstagib
{
public:
	CGameControllerInstaFB(class CGameContext *pGameServer);
	~CGameControllerInstaFB() override;

	void Tick() override;
	void Snap(int SnappingClient) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	bool OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos) override;

	void DoTeamScoreWincheck();
	class CFlagBall *m_apFlagBalls[2];

	int m_flagstand_temp_i_0; //+KZ from tw_plus
	int m_flagstand_temp_i_1; //same

};
#endif
