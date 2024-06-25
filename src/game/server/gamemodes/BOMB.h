#ifndef GAME_SERVER_GAMEMODES_BOMB_H
#define GAME_SERVER_GAMEMODES_BOMB_H

#include "lms_vanilla.h"

class CGameControllerBOMB : public CGameControllerLMSVanilla
{
    int BombAmount();
    int CharAmount();
    void SetBombs();
    void SetSkins();
    void TransferBomb(class CPlayer* From, class CPlayer* To);
    void ExplodeBomb(class CPlayer* BombPlayer);
    int m_BombTime;
public:
	CGameControllerBOMB(class CGameContext *pGameServer);
	~CGameControllerBOMB();

    virtual void Snap(int SnappingClient) override;
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
    bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
    bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
	void Tick() override;
    //bool m_RoundActive = false;
};
#endif // GAME_SERVER_GAMEMODES_BOMB_H
