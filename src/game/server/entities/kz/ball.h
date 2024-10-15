/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_BALL_H
#define GAME_SERVER_ENTITIES_BALL_H

#include <game/server/entity.h>

class CBall : public CEntity
{
public:
	CBall(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir);
	void Tick() override;
	void Snap(int SnappingClient) override;
	void Reset() override;
	vec2 GetPos(float Time);
	void GoToStartPos();
	
	virtual int GetOwnerId() const override { return m_Owner; }
	
	unsigned short m_CollisionsByX;
	unsigned short m_CollisionByY;
private:
	vec2 m_Direction;
	int m_Owner;
	//int m_Type;
	int m_SoundImpact;
	int m_StartTick;
	int m_FootPickupDistance; //<-foot grenade diff.
	int m_RespawnTick = 0;
	int m_Team;
	vec2 m_StartPos;
	
};

#endif
