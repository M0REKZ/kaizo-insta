/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_VEHICLE_BASE_H
#define GAME_SERVER_ENTITIES_VEHICLE_BASE_H

#include <game/server/entity.h>
#include <game/server/entities/character.h>

class CVehicle : public CEntity
{
public:
	CVehicle(CGameWorld *pGameWorld, vec2 Pos, int Owner = -1, int Layer = 0);

	virtual void Reset() override;
	virtual void Tick() override;
	virtual void Snap(int SnappingClient) override;

	void UnMount();
	CCharacter* GetMounter() { return m_pMounter; }

protected:

	virtual void HandleMounterInput(const CNetObj_PlayerInput* Input);

	vec2 m_StartPos;

	vec2 m_Size;
	vec2 m_Vel;
	float m_MountDistance;

	int m_Owner;
	vec2 m_MountOffset;
	CCharacter* m_pMounter = nullptr;

};

#endif
