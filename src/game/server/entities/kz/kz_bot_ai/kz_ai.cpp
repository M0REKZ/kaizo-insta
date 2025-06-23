#include "kz_ai.h"
#include <game/server/entities/flag.h>
#include <game/server/entities/ddnet_pvp/vanilla_pickup.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/gamecontroller.h>
#include <engine/shared/config.h>

CKZBotAI::CKZBotAI(CCharacter * pChr) : CBaseKZBotAI(pChr)
{
	m_AIType = KZBOT_AI;
}

void CKZBotAI::HandleInput(CNetObj_PlayerInput &Input)
{
	//Spaghetti yummy
	
	CCharacter *pClosestChar = nullptr;
	CVanillaPickup *pClosestPickup = nullptr;
	CFlag *pEnemyFlag = nullptr;
	CFlag *pTeamFlag = nullptr;
	bool targetisup = false;
	bool dontjump = false;
	bool butjumpifwall = false;
	bool jumpifgoingtofall = false;
	
	vec2 TargetPos = vec2(0,0);
	bool TargetPosSet = false;
	bool DoSmartTargetChase = false;
	
	Input.m_Fire = false;
	
	//if(str_find_nocase(GameServer()->m_pController->m_pGameType, "CTF"))
	{
		CFlag *p = (CFlag *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_FLAG);
		for(; p; p = (CFlag *)p->TypeNext())
		{
			
			if(p->GetTeam() == m_pPlayer->GetTeam())
			{
				pTeamFlag = p;
				continue;
			}
			
			pEnemyFlag = p;
		}
	}
	
	{
		float ClosestRange = 100000.0f;
		CVanillaPickup *pClosest = 0;
		
		CVanillaPickup *p = (CVanillaPickup *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PICKUP);
		for(; p; p = (CVanillaPickup *)p->TypeNext())
		{
			if(Collision()->FastIntersectLine(*m_pPos,p->m_Pos,nullptr,nullptr))
				continue;
			
			if((p->Type() == POWERUP_HEALTH && GetCharacter()->GetHealth() >= 10) || (p->Type() == POWERUP_ARMOR && GetCharacter()->GetArmor() >= 10) || (p->Subtype() == WEAPON_SHOTGUN && m_pCore->m_aWeapons[WEAPON_SHOTGUN].m_Ammo) || (p->Subtype() == WEAPON_LASER && m_pCore->m_aWeapons[WEAPON_LASER].m_Ammo) || (p->Subtype() == WEAPON_GRENADE && m_pCore->m_aWeapons[WEAPON_GRENADE].m_Ammo)  || (p->Type() == POWERUP_NINJA && m_pCore->m_aWeapons[WEAPON_NINJA].m_Got))
				continue;
			
			if(p->GetSpawnTick() > 0)
			{
				//printf("%d",((CVanillaPickup*)p)->GetSpawnTick());
				continue;
			}
			
			
			float Len = distance(*m_pPos, p->m_Pos);
			if(Len < p->GetProximityRadius() + 100000.0f)
			{
				if(Len < ClosestRange)
				{
					ClosestRange = Len;
					pClosest = p;
				}
			}
		}
		
		pClosestPickup = pClosest;
	}
	
	{
		float ClosestRange = 100000.0f;
		CCharacter *pClosest = 0;
		
		CCharacter *p = (CCharacter *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER);
		for(; p; p = (CCharacter *)p->TypeNext())
		{
			if(p == GetCharacter())
				continue;
			
			if(GameServer()->m_pController->IsTeamPlay() && p->GetPlayer()->GetTeam() == m_pPlayer->GetTeam())
				continue;
			
			if(str_find_nocase(GameServer()->m_pController->m_pGameType, "freeze") && p->GetCore().m_DeepFrozen)
				continue;

			if(str_find_nocase(GameServer()->m_pController->m_pGameType, "catch64") && p->GetPlayer()->m_CatchColor == m_pPlayer->m_CatchColor)
				continue;
			
			
			float Len = distance(*m_pPos, p->m_Pos);
			if(Len < p->GetProximityRadius() + 100000.0f)
			{
				if(Len < ClosestRange)
				{
					ClosestRange = Len;
					pClosest = p;
				}
			}
		}
		
		pClosestChar = pClosest;
	}
	
	if(pEnemyFlag)
	{
		if(pTeamFlag && pEnemyFlag->m_pCarrier == GetCharacter())
		{
			//Input.m_Direction = pTeamFlag->m_Pos->x > m_pPos->x ? 1 : -1;
			TargetPos = pTeamFlag->m_Pos;
			TargetPosSet = true;
			DoSmartTargetChase = true;
		}
		else if(!pEnemyFlag->m_pCarrier)
		{
			//Input.m_Direction = pEnemyFlag->m_Pos->x > m_pPos->x ? 1 : -1;
			TargetPos = pEnemyFlag->m_Pos;
			TargetPosSet = true;
			DoSmartTargetChase = true;
		}
	}
	
	if(pClosestPickup && !(pTeamFlag && pEnemyFlag && (pEnemyFlag->m_pCarrier == GetCharacter() || !pEnemyFlag->m_pCarrier)))
	{
		//Input.m_Direction = pClosestPickup->m_Pos->x > m_pPos->x ? 1 : -1;
		TargetPos = pClosestPickup->m_Pos;
		TargetPosSet = true;
		
		/*if((pClosestPickup->m_Pos->y + pClosestPickup->GetProximityRadius() * 2.f) < m_pPos->y && !(Collision()->GetCollisionAt(m_pPos->x , m_pPos->y - GetProximityRadius() / 3.f) == TILE_DEATH))
		{
			targetisup = true;
		}*/
	}
	
	if(pClosestChar)
	{
		Input.m_TargetX = pClosestChar->m_Pos.x - m_pPos->x; // aim
		Input.m_TargetY = pClosestChar->m_Pos.y - m_pPos->y;
		
		if(!pClosestPickup && !(pTeamFlag && pEnemyFlag && (pEnemyFlag->m_pCarrier == GetCharacter() || !pEnemyFlag->m_pCarrier)))
		{
			TargetPos = pClosestChar->m_Pos;
			TargetPosSet = true;
			DoSmartTargetChase = true;
		}
		
		if(m_pCore->m_aWeapons[WEAPON_NINJA].m_Got)
		{
		}
		else if(m_pCore->m_aWeapons[WEAPON_LASER].m_Got && m_pCore->m_aWeapons[WEAPON_LASER].m_Ammo && distance(*m_pPos, pClosestChar->m_Pos) < GameServer()->Tuning()->m_LaserReach)
		{
			GetCharacter()->SetWeapon(WEAPON_LASER);
		}
		else if(m_pCore->m_aWeapons[WEAPON_SHOTGUN].m_Got && m_pCore->m_aWeapons[WEAPON_SHOTGUN].m_Ammo && distance(*m_pPos, pClosestChar->m_Pos) < GameServer()->Tuning()->m_ShotgunLifetime * 3000.0f)
		{
			GetCharacter()->SetWeapon(WEAPON_SHOTGUN);
		}
		else if(m_pCore->m_aWeapons[WEAPON_HAMMER].m_Got && distance(*m_pPos, pClosestChar->m_Pos) < 40.0f)
		{
			GetCharacter()->SetWeapon(WEAPON_HAMMER);
		}
		else if(m_pCore->m_aWeapons[WEAPON_GUN].m_Got)
		{
			GetCharacter()->SetWeapon(WEAPON_GUN);
		}
		
		if((m_pCore->m_ActiveWeapon == WEAPON_LASER ? (!Collision()->FastIntersectLine(*m_pPos,pClosestChar->m_Pos,nullptr,nullptr) && distance(*m_pPos, pClosestChar->m_Pos) < GameServer()->Tuning()->m_LaserReach) : !Collision()->FastIntersectLine(*m_pPos,pClosestChar->m_Pos,nullptr,nullptr)) || m_pCore->m_aWeapons[WEAPON_NINJA].m_Got)
		{
			if(!GetCharacter()->GetLatestInput().m_Fire)
				Input.m_Fire = true;
			else
				Input.m_Fire = false;
			
			if(Server()->Tick() % Server()->TickSpeed() == 0)
			{
				Input.m_Hook = false;
			}
			else if(distance(*m_pPos, pClosestChar->m_Pos) < GameServer()->Tuning()->m_HookLength)
			{
				Input.m_Hook = true;
			}
		}
		
		if(Collision()->FastIntersectLine(*m_pPos,pClosestChar->m_Pos,nullptr,nullptr) && m_pCore->m_ActiveWeapon == WEAPON_LASER)
		{
			bool fire = false;
			
			float halfy = (pClosestChar->m_Pos.y - m_pPos->y) / 2;
			float halfx = (pClosestChar->m_Pos.x - m_pPos->x) / 2;
			
			vec2 BouncePos_Right,BouncePos_Left,BouncePos_Up,BouncePos_Down;
			bool Bounced_Right = false,Bounced_Left = false,Bounced_Up = false,Bounced_Down = false;
			
			vec2 At, tempvar;
			
			//try y first
			if(Collision()->FastIntersectLine(vec2(m_pPos->x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),nullptr,&tempvar))
			{
				Collision()->UnIntersectLineKZ(vec2(m_pPos->x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),&BouncePos_Right,nullptr);
				if(!Collision()->IntersectLine(*m_pPos,BouncePos_Right,nullptr,nullptr) && tempvar.x > BouncePos_Right.x)
					Bounced_Right = true;
			}
			if(Collision()->FastIntersectLine(vec2(m_pPos->x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),nullptr,&tempvar))
			{
				Collision()->UnIntersectLineKZ(vec2(m_pPos->x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),&BouncePos_Left,nullptr);
				if(!Collision()->IntersectLine(*m_pPos,BouncePos_Left,nullptr,nullptr) && tempvar.x < BouncePos_Left.x)
					Bounced_Left = true;
			}
			//try x now
			if(Collision()->FastIntersectLine(vec2(halfx + m_pPos->x,m_pPos->y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),nullptr,&tempvar))
			{
				Collision()->UnIntersectLineKZ(vec2(halfx + m_pPos->x,m_pPos->y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),&BouncePos_Down,nullptr);
				if(!Collision()->IntersectLine(*m_pPos,BouncePos_Down,nullptr,nullptr) && tempvar.y > BouncePos_Down.y)
					Bounced_Down = true;
			}
			if(Collision()->FastIntersectLine(vec2(halfx + m_pPos->x,m_pPos->y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),nullptr,&BouncePos_Up))
			{
				Collision()->UnIntersectLineKZ(vec2(halfx + m_pPos->x,m_pPos->y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),&BouncePos_Up,nullptr);
				if(!Collision()->IntersectLine(*m_pPos,BouncePos_Up,nullptr,nullptr) && tempvar.y < BouncePos_Up.y)
					Bounced_Up = true;
			}
			
			//try y
			if(Bounced_Right && (BouncePos_Right.y > (halfy + m_pPos->y) - 5.f && BouncePos_Right.y < (halfy + m_pPos->y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Right,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Right, vec2(m_pPos->x,m_pPos->y + (BouncePos_Right.y - m_pPos->y)*2), 0.f, At, GetCharacter()))
			{
				Input.m_TargetX = BouncePos_Right.x - m_pPos->x; // aim
				Input.m_TargetY = BouncePos_Right.y - m_pPos->y;
				fire = true;
			}
			else if(Bounced_Left && (BouncePos_Left.y > (halfy + m_pPos->y) - 5.f && BouncePos_Left.y < (halfy + m_pPos->y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Left,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Left, vec2(m_pPos->x,m_pPos->y + (BouncePos_Left.y - m_pPos->y)*2), 0.f, At, GetCharacter()))
			{
				Input.m_TargetX = BouncePos_Left.x - m_pPos->x; // aim
				Input.m_TargetY = BouncePos_Left.y - m_pPos->y;
				fire = true;
			}
				
			//try x
			if(Bounced_Up && (BouncePos_Up.x > (halfx + m_pPos->x) - 5.f && BouncePos_Up.x < (halfx + m_pPos->x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Up,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Up, vec2(m_pPos->x + (BouncePos_Up.x - m_pPos->x)*2,m_pPos->y), 0.f, At, GetCharacter()))
			{
				Input.m_TargetX = BouncePos_Up.x - m_pPos->x; // aim
				Input.m_TargetY = BouncePos_Up.y - m_pPos->y;
				fire = true;
			}
			else if(Bounced_Down && (BouncePos_Down.x > (halfx + m_pPos->x) - 5.f && BouncePos_Down.x < (halfx + m_pPos->x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Down,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Down, vec2(m_pPos->x + (BouncePos_Down.x - m_pPos->x)*2,m_pPos->y), 0.f, At, GetCharacter()))
			{
				Input.m_TargetX = BouncePos_Down.x - m_pPos->x; // aim
				Input.m_TargetY = BouncePos_Down.y - m_pPos->y;
				fire = true;
			}
					
			if(!fire)
			{
				//try again but moving point
				{
					float tempfloat =0.0f;
					
					tempfloat = halfy; //save halfy
					
					//move point
					halfx = halfx - m_pPos->x > 0 ? halfx + (tempfloat/2 + tempfloat/halfx) : halfx - (tempfloat/2 + tempfloat/halfx);
					if(halfy - m_pPos->y > 0)
					{
						halfy += (halfx/2 + halfx/halfy);
						if(halfy - m_pPos->y < 0)
							halfy += (halfy - m_pPos->y)*2;
					}
					else if(halfy - m_pPos->y < 0)
					{
						halfy -= (halfx/2 + halfx/halfy);
						if(halfy - m_pPos->y > 0)
							halfy -= (halfy - m_pPos->y)*2;
					}
					
					if(halfx - m_pPos->x > 0)
					{
						halfx += (tempfloat/2 + tempfloat/halfx);
						if(halfx - m_pPos->x < 0)
						{
							halfx += (halfx - m_pPos->x)*2;
						}
					}
					else if(halfx - m_pPos->x < 0)
					{
						halfx -= (tempfloat/2 + tempfloat/halfx);
						if(halfx - m_pPos->x > 0)
						{
							halfx -= (halfx - m_pPos->x)*2;
						}
					}
				}
				
				//vec2 BouncePos_Right,BouncePos_Left,BouncePos_Up,BouncePos_Down;
				Bounced_Right = false,Bounced_Left = false,Bounced_Up = false,Bounced_Down = false;
				
				//vec2 At, tempvar;
				
				//try y first
				if(Collision()->FastIntersectLine(vec2(m_pPos->x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),nullptr,&tempvar))
				{
					Collision()->UnIntersectLineKZ(vec2(m_pPos->x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),&BouncePos_Right,nullptr);
					if(!Collision()->IntersectLine(*m_pPos,BouncePos_Right,nullptr,nullptr) && tempvar.x > BouncePos_Right.x)
						Bounced_Right = true;
				}
				if(Collision()->FastIntersectLine(vec2(m_pPos->x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),nullptr,&tempvar))
				{
					Collision()->UnIntersectLineKZ(vec2(m_pPos->x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),&BouncePos_Left,nullptr);
					if(!Collision()->IntersectLine(*m_pPos,BouncePos_Left,nullptr,nullptr) && tempvar.x < BouncePos_Left.x)
						Bounced_Left = true;
				}
				//try x now
				if(Collision()->FastIntersectLine(vec2(halfx + m_pPos->x,m_pPos->y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),nullptr,&tempvar))
				{
					Collision()->UnIntersectLineKZ(vec2(halfx + m_pPos->x,m_pPos->y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),&BouncePos_Down,nullptr);
					if(!Collision()->IntersectLine(*m_pPos,BouncePos_Down,nullptr,nullptr) && tempvar.y > BouncePos_Down.y)
						Bounced_Down = true;
				}
				if(Collision()->FastIntersectLine(vec2(halfx + m_pPos->x,m_pPos->y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),nullptr,&BouncePos_Up))
				{
					Collision()->UnIntersectLineKZ(vec2(halfx + m_pPos->x,m_pPos->y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),&BouncePos_Up,nullptr);
					if(!Collision()->IntersectLine(*m_pPos,BouncePos_Up,nullptr,nullptr) && tempvar.y < BouncePos_Up.y)
						Bounced_Up = true;
				}
				
				//try y
				if(Bounced_Right && (BouncePos_Right.y > (halfy + m_pPos->y) - 5.f && BouncePos_Right.y < (halfy + m_pPos->y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Right,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Right, vec2(m_pPos->x,m_pPos->y + (BouncePos_Right.y - m_pPos->y)*2), 0.f, At, GetCharacter()))
				{
					Input.m_TargetX = BouncePos_Right.x - m_pPos->x; // aim
					Input.m_TargetY = BouncePos_Right.y - m_pPos->y;
					fire = true;
				}
				else if(Bounced_Left && (BouncePos_Left.y > (halfy + m_pPos->y) - 5.f && BouncePos_Left.y < (halfy + m_pPos->y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Left,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Left, vec2(m_pPos->x,m_pPos->y + (BouncePos_Left.y - m_pPos->y)*2), 0.f, At, GetCharacter()))
				{
					Input.m_TargetX = BouncePos_Left.x - m_pPos->x; // aim
					Input.m_TargetY = BouncePos_Left.y - m_pPos->y;
					fire = true;
				}
					
				//try x
				if(Bounced_Up && (BouncePos_Up.x > (halfx + m_pPos->x) - 5.f && BouncePos_Up.x < (halfx + m_pPos->x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Up,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Up, vec2(m_pPos->x + (BouncePos_Up.x - m_pPos->x)*2,m_pPos->y), 0.f, At, GetCharacter()))
				{
					Input.m_TargetX = BouncePos_Up.x - m_pPos->x; // aim
					Input.m_TargetY = BouncePos_Up.y - m_pPos->y;
					fire = true;
				}
				else if(Bounced_Down && (BouncePos_Down.x > (halfx + m_pPos->x) - 5.f && BouncePos_Down.x < (halfx + m_pPos->x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Down,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Down, vec2(m_pPos->x + (BouncePos_Down.x - m_pPos->x)*2,m_pPos->y), 0.f, At, GetCharacter()))
				{
					Input.m_TargetX = BouncePos_Down.x - m_pPos->x; // aim
					Input.m_TargetY = BouncePos_Down.y - m_pPos->y;
					fire = true;
				}
			}
				
			
			if(fire)
			{
				Input.m_Fire = true;
			}
				
			
			
		}
	}
	
	
	if(TargetPosSet && !m_StopUntilTouchGround)
	{
		if(DoSmartTargetChase && !m_DontDoSmartTargetChase)
		{
			if(m_TryingDirectionSmart)
			{
				Input.m_Direction = m_TryingDirectionSmart;
				if((TargetPos.y + 56.0f) < m_pPos->y && !(Collision()->GetCollisionAt(m_pPos->x , m_pPos->y -  GetCharacter()->GetProximityRadius() / 3.f) == TILE_DEATH))
				{
					targetisup = true;
				}
				else
				{
					dontjump = true;
					butjumpifwall = true;
				}
				if(!(Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(0.f,1000.f),nullptr,nullptr)))
				{
					//danger no floor, remove directionsmart
					m_TryingOppositeSmart = m_TryingDirectionSmart = 0;
				}
			}
			else if(((TargetPos.x - m_pPos->x < 0 ? (TargetPos.y - m_pPos->y < 0 ? TargetPos.x - m_pPos->x > TargetPos.y - m_pPos->y : (TargetPos.x - m_pPos->x)*-1 < TargetPos.y - m_pPos->y) : (TargetPos.y - m_pPos->y < 0 ? TargetPos.x - m_pPos->x < (TargetPos.y - m_pPos->y)*-1 : TargetPos.x - m_pPos->x < TargetPos.y - m_pPos->y)) && TargetPos.x > m_pPos->x - 500.0f && TargetPos.x < m_pPos->x + 500.0f))
			{
				//is up/down
				
				bool left = false,right = false;
				
				if(TargetPos.y < m_pPos->y)
				{
					left = Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-400.f,-400.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(0,-350.f),nullptr,nullptr);
					right = Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(400.f,-400.f),nullptr,nullptr);
					
					if(left && right && !m_TryingDirectionSmart && !Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(0,-350.f),nullptr,nullptr))//middle
					{
						Input.m_Direction = TargetPos.x > m_pPos->x ? 1 : -1;
						targetisup = true;
					}
					else if(!m_TryingDirectionSmart && !left)//left
					{
						Input.m_Direction = -1;
						targetisup = true;
					}
					else if(!m_TryingDirectionSmart && !right)//right
					{
						Input.m_Direction = 1;
						targetisup = true;
					}
					else
					{
						//bool leftside = false,rightside = false;
						//vec2 leftcol,rightcol;
						
						
						if(!m_TryingDirectionSmart)
						{
							m_TryingDirectionSmart = TargetPos.x > m_pPos->x ? 1 : -1;
							/*leftside = Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-250.f,0),nullptr,&leftcol);
							rightside = Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(250.f,0),nullptr,&rightcol);
							if(!leftside && !rightside)
							{
								m_TryingDirectionSmart = TargetPos.x > m_pPos->x ? 1 : -1;
							}
							else
							{
								if(!leftside)
								{
									m_TryingDirectionSmart = -1;
								}
								else if(!rightside)
								{
									m_TryingDirectionSmart = 1;
								}
								else
								{
									float d1,d2;
									d1 = distance(*m_pPos,leftcol);
									d2 = distance(*m_pPos,rightcol);
									
									if(d1 > d2)
									{
										m_TryingDirectionSmart = 1;
									}
									else
									{
										m_TryingDirectionSmart = -1;
									}
								}
							}*/
						}
						else
						{
							Input.m_Direction = m_TryingDirectionSmart;
						}
					}
				}
				else
				{
					left = Collision()->FastIntersectLine(*m_pPos + vec2(-100.f,0),*m_pPos + vec2(-100.f,-100.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(*m_pPos + vec2(0,0),*m_pPos + vec2(0,-100.f),nullptr,nullptr);
					right = Collision()->FastIntersectLine(*m_pPos + vec2(100.f,0),*m_pPos + vec2(100.f,-100.f),nullptr,nullptr);
					
					if(left && right && !m_TryingDirectionSmart && !Collision()->FastIntersectLine(*m_pPos + vec2(0,0),*m_pPos + vec2(0,-100.f),nullptr,nullptr))//middle
					{
						m_TryingDirectionSmart = TargetPos.x > m_pPos->x ? 1 : -1;
					}
					else if(!left && !m_TryingDirectionSmart)
					{
						m_TryingDirectionSmart = -1;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!right && !m_TryingDirectionSmart)
					{
						m_TryingDirectionSmart = 1;
						dontjump = true;
						butjumpifwall = true;
					}
					else
					{
						bool leftside = false,rightside = false;
						vec2 leftcol,rightcol;
						
						
						if(!m_TryingDirectionSmart)
						{
							if(!(leftside = Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-1000.f,0),nullptr,&leftcol)))
							{
								m_TryingDirectionSmart = -1;
							}
							else if(!(rightside = Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(1000.f,0),nullptr,&rightcol)))
							{
								m_TryingDirectionSmart = 1;
							}
							else
							{
								float d1,d2;
								d1 = distance(*m_pPos,leftcol);
								d2 = distance(*m_pPos,rightcol);
								
								if(d1 > d2)
								{
									m_TryingDirectionSmart = 1;
								}
								else
								{
									m_TryingDirectionSmart = -1;
								}
							}
						}
						else
						{
							Input.m_Direction = m_TryingDirectionSmart;
						}
					}
				}
			}
			else
			{
				//is still away
				
				//bool up = false,middle = false,down = false;
				
				if(TargetPos.x > m_pPos->x)
				{
					//up = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(100.f,-100.f),nullptr,nullptr) || Collision()->IntersectLine(*m_pPos + vec2(0,-100.f),*m_pPos + vec2(50.f,-150.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(150.f,0),nullptr,nullptr);
					//down = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(100.f,100.f),nullptr,nullptr)  || Collision()->IntersectLine(*m_pPos + vec2(0,100.f),*m_pPos + vec2(50.f,150.f),nullptr,nullptr);
					
					if(!Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(100.f,0),nullptr,nullptr))//middle
					{
						Input.m_Direction = 1;
						dontjump = true;
						jumpifgoingtofall = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(50.f,50.f),nullptr,nullptr))  || !(Collision()->IntersectLine(*m_pPos + vec2(0,100.f),*m_pPos + vec2(50.f,150.f),nullptr,nullptr)))//down
					{
						Input.m_Direction = 1;
						jumpifgoingtofall = true;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(50.f,-50.f),nullptr,nullptr)) || !(Collision()->IntersectLine(*m_pPos + vec2(0,-100.f),*m_pPos + vec2(50.f,-150.f),nullptr,nullptr)))//up
					{
						Input.m_Direction = 1;
						targetisup = true;
					}
					else
					{
						if(!m_TryingDirectionSmart)
						{
							bool upside = false, downside = false;
							downside = Collision()->FastIntersectLine(*m_pPos + vec2(GetCharacter()->GetProximityRadius()/2,0),*m_pPos + vec2(0,300.f),nullptr,nullptr);
							upside = Collision()->FastIntersectLine(*m_pPos + vec2(GetCharacter()->GetProximityRadius()/2,0),*m_pPos + vec2(0,-150.f),nullptr,nullptr);
							if(!upside)
							{
								targetisup = true;
							}
							else if(downside)
							{
								m_TryingDirectionSmart = -1;
								targetisup = true;
							}
							
						}
					}
				}
				else
				{
					//up = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(-100.f,-100.f),nullptr,nullptr) || Collision()->IntersectLine(*m_pPos + vec2(0,-100.f),*m_pPos + vec2(-50.f,-150.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(-150.f,0),nullptr,nullptr);
					//down = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(-100.f,100.f),nullptr,nullptr)  || Collision()->IntersectLine(*m_pPos + vec2(0,100.f),*m_pPos + vec2(-50.f,150.f),nullptr,nullptr);
					
					if(!Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-100.f,0),nullptr,nullptr))//middle
					{
						Input.m_Direction = -1;
						jumpifgoingtofall = true;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-50.f,100.f),nullptr,nullptr))  || !(Collision()->FastIntersectLine(*m_pPos + vec2(0,100.f),*m_pPos + vec2(-50.f,150.f),nullptr,nullptr)))//down
					{
						Input.m_Direction = -1;
						jumpifgoingtofall = true;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-50.f,-100.f),nullptr,nullptr)) || !(Collision()->FastIntersectLine(*m_pPos + vec2(0,-100.f),*m_pPos + vec2(-50.f,-150.f),nullptr,nullptr)))//up
					{
						Input.m_Direction = -1;
						targetisup = true;
					}
					else
					{
						if(!m_TryingDirectionSmart)
						{
							bool upside = false, downside = false;
							downside = Collision()->FastIntersectLine(*m_pPos + vec2(GetCharacter()->GetProximityRadius()/-2,0),*m_pPos + vec2(0,300.f),nullptr,nullptr);
							upside = Collision()->FastIntersectLine(*m_pPos + vec2(GetCharacter()->GetProximityRadius()/-2,0),*m_pPos + vec2(0,-150.f),nullptr,nullptr);
							if(!upside)
							{
								targetisup = true;
							}
							else if(upside && downside)
							{
								m_TryingDirectionSmart = 1;
								targetisup = true;
							}
							
						}
					}
				}
			}
			
		}
		else
		{
			Input.m_Direction = TargetPos.x > m_pPos->x ? 1 : -1;
			
			if((TargetPos.y + 56.0f) < m_pPos->y && !(Collision()->GetCollisionAt(m_pPos->x , m_pPos->y - GetCharacter()->GetProximityRadius() / 3.f) == TILE_DEATH))
			{
				targetisup = true;
			}
			else
			{
				dontjump = true;
				butjumpifwall = true;
			}
			if(m_DontDoSmartTargetChase > 0)
				m_DontDoSmartTargetChase--;
		}

		if(m_DoGrenadeJump && ((GetCharacter()->GetWeaponGot(WEAPON_GRENADE) && GetCharacter()->GetWeaponAmmo(WEAPON_GRENADE)) || (g_Config.m_SvLaserJump && GetCharacter()->GetWeaponGot(WEAPON_LASER) && GetCharacter()->GetWeaponAmmo(WEAPON_LASER))))
		{
			if(GetCharacter()->GetWeaponGot(WEAPON_GRENADE) && GetCharacter()->GetWeaponAmmo(WEAPON_GRENADE))
				GetCharacter()->SetActiveWeapon(WEAPON_GRENADE);
			else if(g_Config.m_SvLaserJump && GetCharacter()->GetWeaponGot(WEAPON_LASER) && GetCharacter()->GetWeaponAmmo(WEAPON_LASER))
				GetCharacter()->SetActiveWeapon(WEAPON_LASER);
			Input.m_TargetX = 0;
			Input.m_TargetY = 1;
			Input.m_Fire = 1;
		}

		m_DoGrenadeJump = false;

		if(((GetCharacter()->GetWeaponGot(WEAPON_GRENADE) && GetCharacter()->GetWeaponAmmo(WEAPON_GRENADE)) || (g_Config.m_SvLaserJump && GetCharacter()->GetWeaponGot(WEAPON_LASER) && GetCharacter()->GetWeaponAmmo(WEAPON_LASER))) && (GameServer()->m_pController->m_IsInstagibKZ || GetCharacter()->GetHealth() >= 10 || (GetCharacter()->GetHealth() >= 5 && GetCharacter()->GetArmor() >= 5)))
		{
			if(m_pCore->m_Jumps > 0 && TargetPos.x > m_pPos->x - 300.f && TargetPos.x < m_pPos->x + 300.f && TargetPos.y < m_pPos->y - 150.f && GetCharacter()->IsGrounded() && !Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(0.f,-300.f),nullptr,nullptr))
			{
				m_DoGrenadeJump = true;
				Input.m_Jump = true;
				if(GetCharacter()->GetWeaponGot(WEAPON_GRENADE) && GetCharacter()->GetWeaponAmmo(WEAPON_GRENADE))
					GetCharacter()->SetActiveWeapon(WEAPON_GRENADE);
				else if(g_Config.m_SvLaserJump && GetCharacter()->GetWeaponGot(WEAPON_LASER) && GetCharacter()->GetWeaponAmmo(WEAPON_LASER))
					GetCharacter()->SetActiveWeapon(WEAPON_LASER);
			}
			else if(TargetPos.x < m_pPos->x - 700.f && GetCharacter()->IsGrounded() && !Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-300.f,0.f),nullptr,nullptr))
			{
				Input.m_TargetX = 1;
				Input.m_TargetY = 1;
				Input.m_Fire = 1;
				if(GetCharacter()->GetWeaponGot(WEAPON_GRENADE) && GetCharacter()->GetWeaponAmmo(WEAPON_GRENADE))
					GetCharacter()->SetActiveWeapon(WEAPON_GRENADE);
				else if(g_Config.m_SvLaserJump && GetCharacter()->GetWeaponGot(WEAPON_LASER) && GetCharacter()->GetWeaponAmmo(WEAPON_LASER))
					GetCharacter()->SetActiveWeapon(WEAPON_LASER);
			}
			else if(TargetPos.x > m_pPos->x + 700.f && GetCharacter()->IsGrounded() && !Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(300.f,0.f),nullptr,nullptr))
			{
				Input.m_TargetX = -1;
				Input.m_TargetY = 1;
				Input.m_Fire = 1;
				if(GetCharacter()->GetWeaponGot(WEAPON_GRENADE) && GetCharacter()->GetWeaponAmmo(WEAPON_GRENADE))
					GetCharacter()->SetActiveWeapon(WEAPON_GRENADE);
				else if(g_Config.m_SvLaserJump && GetCharacter()->GetWeaponGot(WEAPON_LASER) && GetCharacter()->GetWeaponAmmo(WEAPON_LASER))
					GetCharacter()->SetActiveWeapon(WEAPON_LASER);
			}
		}
	}
	
	if(m_TryingDirectionSmart && !Collision()->FastIntersectLine(*m_pPos,TargetPos,nullptr,nullptr))
	{
		m_TryingOppositeSmart = m_TryingDirectionSmart = 0;
		m_StopUntilTouchGround = true;
	}
	
	if(m_StopUntilTouchGround && GetCharacter()->IsGrounded())
	{
		m_StopUntilTouchGround = false;
		m_DontDoSmartTargetChase = Server()->TickSpeed() * 2;
	}
	if(m_pCore->m_Colliding && GetCharacter()->IsGrounded())
	{
		if(!m_TryingOppositeSmart)
		{
			m_TryingDirectionSmart *= -1;
			m_TryingOppositeSmart = true;
		}
		else
		{
			m_TryingDirectionSmart = 0;
			m_TryingOppositeSmart = false;
		}
	}

	if(!m_pCore->m_Jumps && !Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(0.f,1000.f),nullptr,nullptr))
	{
		vec2 right,left;

		Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(1000.f,1000.f),&right,nullptr);
		Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-1000.f,1000.f),&left,nullptr);

		float rightlength = right.x - m_pPos->x;
		float leftlength = m_pPos->x - left.x;

		if(rightlength == leftlength)
		{
			Input.m_Direction = 0;
		}
		else if(rightlength > leftlength)
		{
			Input.m_Direction = -1;
		}
		else
		{
			Input.m_Direction = 1;
		}
	}
	
	
	//HELP
	if(!m_DoGrenadeJump && ((jumpifgoingtofall ? !(Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(0.f,1000.f),nullptr,nullptr)) : false) || (butjumpifwall ? m_pCore->m_Colliding : false) || (!dontjump && ((Collision()->GetCollisionAt(m_pPos->x , m_pPos->y + GetCharacter()->GetProximityRadius() / 3.f) == TILE_DEATH) || !Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(0.f,1000.f),nullptr,nullptr) || m_pCore->m_Colliding || (((Collision()->CheckPoint(m_pPos->x + GetCharacter()->GetProximityRadius() / 2, m_pPos->y + GetCharacter()->GetProximityRadius() / 2 + 5)) && !(Collision()->CheckPoint(m_pPos->x - GetCharacter()->GetProximityRadius() / 2, m_pPos->y + GetCharacter()->GetProximityRadius() / 2 + 5)))) || ((!(Collision()->CheckPoint(m_pPos->x + GetCharacter()->GetProximityRadius() / 2, m_pPos->y + GetCharacter()->GetProximityRadius() / 2 + 5)) && (Collision()->CheckPoint(m_pPos->x - GetCharacter()->GetProximityRadius() / 2, m_pPos->y + GetCharacter()->GetProximityRadius() / 2 + 5)))) || targetisup))))
	{
		if(GetCharacter()->IsGrounded() || (m_pCore->m_Jumps > 0 && m_pCore->m_Vel.y > 0))
			Input.m_Jump = true;
		else
			Input.m_Jump = false;
	}
}
