/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_GAMEMODES_DDNETKZ_H
#define GAME_SERVER_GAMEMODES_DDNETKZ_H

#include <game/server/gamecontroller.h>
#include "base_pvp/base_pvp.h"
#include <game/server/entities/kz/flagball.h>

class CGameControllerDDNetKZ : public CGameControllerPvp
{
public:
	CGameControllerDDNetKZ(class CGameContext *pGameServer);
	~CGameControllerDDNetKZ();

	void Tick() override;
	void SetArmorProgress(CCharacter *pCharacer, int Progress) override{};
	void SetArmorProgressFull(CCharacter *pCharacer) override{};
	void SetArmorProgressEmpty(CCharacter *pCharacer) override{};
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
	void Snap(int SnappingClient) override;
	bool OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;

	//override unwanted basepvp functions
	virtual bool DoWincheckRound() override { return false; }
	virtual void SetSpawnWeapons(class CCharacter *pChr) override {};
	void OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason) override;
	virtual bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override { return false; };
	int SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags) override;
	void UpdateSpawnWeapons(bool Silent, bool Apply) override {};
	void InitPlayer(class CPlayer *pPlayer) override;
	virtual int SnapPlayerScore(int SnappingClient, CPlayer *pPlayer, int DDRaceScore) override { return CGameControllerDDRace::SnapPlayerScore(SnappingClient, pPlayer, DDRaceScore); };
	bool ForceNetworkClipping(const CEntity *pEntity, int SnappingClient, vec2 CheckPos) override { return false; };
	bool ForceNetworkClippingLine(const CEntity *pEntity, int SnappingClient, vec2 StartPos, vec2 EndPos) override { return false; };
	

	//Flag(ball xD)
	class CFlagBall *m_apFlagBalls[2];

	int m_flagstand_temp_i_0; //+KZ from tw_plus
	int m_flagstand_temp_i_1; //same
};
#endif // GAME_SERVER_GAMEMODES_DDRACE_H
