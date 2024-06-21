#ifndef GAME_SERVER_GAMEMODES_DMVANILLA_H
#define GAME_SERVER_GAMEMODES_DMVANILLA_H

#include "ctf_vanilla.h"

class CGameControllerDMVanilla : public CGameControllerCTFVanilla
{
public:
	CGameControllerDMVanilla(class CGameContext *pGameServer);
	~CGameControllerDMVanilla();

	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
    bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
    bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
	void Tick() override;
};
#endif // GAME_SERVER_GAMEMODES_DM_H
