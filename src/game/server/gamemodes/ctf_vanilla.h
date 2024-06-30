#ifndef GAME_SERVER_GAMEMODES_CTFVANILLA_H
#define GAME_SERVER_GAMEMODES_CTFVANILLA_H

#include "ctf.h"

class CGameControllerCTFVanilla : public CGameControllerCTF
{
	class CFlag *m_apFlags[2];
	//virtual bool DoWincheckMatch() override;

public:
	CGameControllerCTFVanilla(class CGameContext *pGameServer);
	~CGameControllerCTFVanilla();

	//void Tick() override;
	//virtual void Snap(int SnappingClient) override;
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	//virtual void OnFlagReturn(class CFlag *pFlag) override;
	//virtual void OnFlagGrab(class CFlag *pFlag) override;
	//virtual void OnFlagCapture(class CFlag *pFlag, float Time) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
    bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
	//bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;

	//void FlagTick();
};
#endif // GAME_SERVER_GAMEMODES_CTF_H
