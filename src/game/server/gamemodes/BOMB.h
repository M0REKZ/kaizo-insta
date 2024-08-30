#ifndef GAME_SERVER_GAMEMODES_BOMB_H
#define GAME_SERVER_GAMEMODES_BOMB_H

#include "dm_vanilla.h"

class CGameControllerBOMB : public CGameControllerDMVanilla
{
    int BombAmount();
    int CharAmount();
    void SetBombs();
    void SetSkins();
    void TransferBomb(class CPlayer* From, class CPlayer* To);
    void ExplodeBomb(class CPlayer* BombPlayer);
    void BombTick();
    int m_BombTime;
protected:
    virtual bool DoWincheckRound() override;
    virtual void SetAllUndead();
    virtual void FakeEndRound();
    virtual void KillEveryone();
    int m_RoundPauseTime = -1;
public:
	CGameControllerBOMB(class CGameContext *pGameServer);
	~CGameControllerBOMB();

    virtual void Snap(int SnappingClient) override;
    virtual bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
    virtual void OnPlayerConnect(class CPlayer *pPlayer) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
    bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
    bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
	void Tick() override;
    bool m_RoundActive = false;
};
#endif // GAME_SERVER_GAMEMODES_BOMB_H
