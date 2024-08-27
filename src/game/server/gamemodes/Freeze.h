#ifndef GAME_SERVER_GAMEMODES_FREEZE_H
#define GAME_SERVER_GAMEMODES_FREEZE_H

#include "tdm.h"

class CGameControllerFreeze : public CGameControllerTDM
{
    
private:
    void ResetFrozenPlayer();
    void DoMelting(class CCharacter *pChr);
    void Melt(int Melted, int Helper);
protected:
	void DoFreezing(int &From, class CCharacter &Character);
public:
	CGameControllerFreeze(class CGameContext *pGameServer);
	~CGameControllerFreeze();

	void OnCharacterSpawn(class CCharacter *pChr) override;
    bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void Tick() override;
};
#endif // GAME_SERVER_GAMEMODES_FREEZE_H
