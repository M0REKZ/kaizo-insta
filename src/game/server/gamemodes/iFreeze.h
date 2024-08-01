#ifndef GAME_SERVER_GAMEMODES_IFREEZE_H
#define GAME_SERVER_GAMEMODES_IFREEZE_H

#include "tdm.h"

class CGameControllerIFreeze : public CGameControllerTDM
{
    
private:
    void ResetFrozenPlayer();
public:
	CGameControllerIFreeze(class CGameContext *pGameServer);
	~CGameControllerIFreeze();

    void OnCharacterSpawn(class CCharacter *pChr) override;
    bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void Tick() override;
};
#endif // GAME_SERVER_GAMEMODES_IFREEZE_H
