#include "layer_kzcustom.h"

#include <game/editor/editor.h>

CLayerKZCustom::CLayerKZCustom(CEditor *pEditor, int w, int h) :
	CLayerTiles(pEditor, w, h)
{
	str_copy(m_aName, "KZCustom");
	m_KZCustom = 1;

	m_pKZCustomTile = new CKZCustomTile[w * h];
	mem_zero(m_pKZCustomTile, (size_t)w * h * sizeof(CKZCustomTile));
}

CLayerKZCustom::CLayerKZCustom(const CLayerKZCustom &Other) :
	CLayerTiles(Other)
{
	str_copy(m_aName, "KZCustom copy");
	m_KZCustom = 1;

	m_pKZCustomTile = new CKZCustomTile[m_Width * m_Height];
	mem_copy(m_pKZCustomTile, Other.m_pKZCustomTile, (size_t)m_Width * m_Height * sizeof(CKZCustomTile));
}

CLayerKZCustom::~CLayerKZCustom()
{
	delete[] m_pKZCustomTile;
}

void CLayerKZCustom::Resize(int NewW, int NewH)
{
	// resize speedup data
	CKZCustomTile *pNewSpeedupData = new CKZCustomTile[NewW * NewH];
	mem_zero(pNewSpeedupData, (size_t)NewW * NewH * sizeof(CKZCustomTile));

	// copy old data
	for(int y = 0; y < minimum(NewH, m_Height); y++)
		mem_copy(&pNewSpeedupData[y * NewW], &m_pKZCustomTile[y * m_Width], minimum(m_Width, NewW) * sizeof(CKZCustomTile));

	// replace old
	delete[] m_pKZCustomTile;
	m_pKZCustomTile = pNewSpeedupData;

	// resize tile data
	CLayerTiles::Resize(NewW, NewH);

	// resize gamelayer too
	if(m_pEditor->m_Map.m_pGameLayer->m_Width != NewW || m_pEditor->m_Map.m_pGameLayer->m_Height != NewH)
		m_pEditor->m_Map.m_pGameLayer->Resize(NewW, NewH);
}

void CLayerKZCustom::Shift(int Direction)
{
	CLayerTiles::Shift(Direction);
	ShiftImpl(m_pKZCustomTile, Direction, m_pEditor->m_ShiftBy);
}

bool CLayerKZCustom::IsEmpty(const std::shared_ptr<CLayerTiles> &pLayer)
{
	for(int y = 0; y < pLayer->m_Height; y++)
		for(int x = 0; x < pLayer->m_Width; x++)
			if(m_pEditor->m_AllowPlaceUnusedTiles)
				return false;

	return true;
}

void CLayerKZCustom::BrushDraw(std::shared_ptr<CLayer> pBrush, vec2 WorldPos)
{
	if(m_Readonly)
		return;

	std::shared_ptr<CLayerKZCustom> pSpeedupLayer = std::static_pointer_cast<CLayerKZCustom>(pBrush);
	int sx = ConvertX(WorldPos.x);
	int sy = ConvertY(WorldPos.y);
	if(str_comp(pSpeedupLayer->m_aFileName, m_pEditor->m_aFileName))
	{
		//m_pEditor->m_KZCustomVal3 = pSpeedupLayer->m_KZCustomVal3;
		m_pEditor->m_KZCustomVal1 = pSpeedupLayer->m_KZCustomVal1;
		m_pEditor->m_KZCustomVal2 = pSpeedupLayer->m_KZCustomVal2;
	}

	bool Destructive = m_pEditor->m_BrushDrawDestructive || IsEmpty(pSpeedupLayer);

	for(int y = 0; y < pSpeedupLayer->m_Height; y++)
		for(int x = 0; x < pSpeedupLayer->m_Width; x++)
		{
			int fx = x + sx;
			int fy = y + sy;

			if(fx < 0 || fx >= m_Width || fy < 0 || fy >= m_Height)
				continue;

			if(!Destructive && GetTile(fx, fy).m_Index)
				continue;

			int Index = fy * m_Width + fx;
			SKZCustomTileStateChange::SData Previous{
				m_pKZCustomTile[Index].m_Val1,
				m_pKZCustomTile[Index].m_Val2,
				m_pKZCustomTile[Index].m_Flags,
				m_pKZCustomTile[Index].m_Index,
				m_pTiles[Index].m_Index};

			if((m_pEditor->m_AllowPlaceUnusedTiles) && pSpeedupLayer->m_pTiles[y * pSpeedupLayer->m_Width + x].m_Index != TILE_AIR)
			{
				if(m_pEditor->m_KZCustomVal1 != pSpeedupLayer->m_KZCustomVal1 || m_pEditor->m_KZCustomVal2 != pSpeedupLayer->m_KZCustomVal2)
				{
					m_pKZCustomTile[Index].m_Val1 = m_pEditor->m_KZCustomVal1;
					m_pKZCustomTile[Index].m_Val2 = m_pEditor->m_KZCustomVal2;
					m_pKZCustomTile[Index].m_Index = pSpeedupLayer->m_pTiles[y * pSpeedupLayer->m_Width + x].m_Index;
					m_pTiles[Index].m_Index = pSpeedupLayer->m_pTiles[y * pSpeedupLayer->m_Width + x].m_Index;
				}
				else if(pSpeedupLayer->m_pKZCustomTile[y * pSpeedupLayer->m_Width + x].m_Val1)
				{
					m_pKZCustomTile[Index].m_Val1 = pSpeedupLayer->m_pKZCustomTile[y * pSpeedupLayer->m_Width + x].m_Val1;
					m_pKZCustomTile[Index].m_Val2 = pSpeedupLayer->m_pKZCustomTile[y * pSpeedupLayer->m_Width + x].m_Val2;
					m_pKZCustomTile[Index].m_Index = pSpeedupLayer->m_pTiles[y * pSpeedupLayer->m_Width + x].m_Index;
					m_pTiles[Index].m_Index = pSpeedupLayer->m_pTiles[y * pSpeedupLayer->m_Width + x].m_Index;
				}
				else if(m_pEditor->m_KZCustomVal1)
				{
					m_pKZCustomTile[Index].m_Val1 = m_pEditor->m_KZCustomVal1;
					m_pKZCustomTile[Index].m_Val2 = m_pEditor->m_KZCustomVal2;
					m_pKZCustomTile[Index].m_Index = pSpeedupLayer->m_pTiles[y * pSpeedupLayer->m_Width + x].m_Index;
					m_pTiles[Index].m_Index = pSpeedupLayer->m_pTiles[y * pSpeedupLayer->m_Width + x].m_Index;
				}
				else
				{
					m_pKZCustomTile[Index].m_Val1 = m_pEditor->m_KZCustomVal1;
					m_pKZCustomTile[Index].m_Val2 = m_pEditor->m_KZCustomVal2;
					m_pKZCustomTile[Index].m_Index = pSpeedupLayer->m_pTiles[y * pSpeedupLayer->m_Width + x].m_Index;
					m_pTiles[Index].m_Index = pSpeedupLayer->m_pTiles[y * pSpeedupLayer->m_Width + x].m_Index;
				}
			}
			else
			{
				m_pKZCustomTile[Index].m_Val1 = 0;
				m_pKZCustomTile[Index].m_Val2 = 0;
				m_pKZCustomTile[Index].m_Index = 0;
				m_pTiles[Index].m_Index = 0;

				//if(pSpeedupLayer->m_pTiles[y * pSpeedupLayer->m_Width + x].m_Index != TILE_AIR)
				//	ShowPreventUnusedTilesWarning();
			}

			SKZCustomTileStateChange::SData Current{
				m_pKZCustomTile[Index].m_Val1,
				m_pKZCustomTile[Index].m_Val2,
				m_pKZCustomTile[Index].m_Flags,
				m_pKZCustomTile[Index].m_Index,
				m_pTiles[Index].m_Index};

			RecordStateChange(fx, fy, Previous, Current);
		}
	FlagModified(sx, sy, pSpeedupLayer->m_Width, pSpeedupLayer->m_Height);
}

void CLayerKZCustom::RecordStateChange(int x, int y, SKZCustomTileStateChange::SData Previous, SKZCustomTileStateChange::SData Current)
{
	if(!m_History[y][x].m_Changed)
		m_History[y][x] = SKZCustomTileStateChange{true, Previous, Current};
	else
		m_History[y][x].m_Current = Current;
}

void CLayerKZCustom::BrushFlipX()
{
	CLayerTiles::BrushFlipX();
	BrushFlipXImpl(m_pKZCustomTile);
}

void CLayerKZCustom::BrushFlipY()
{
	CLayerTiles::BrushFlipY();
	BrushFlipYImpl(m_pKZCustomTile);
}

void CLayerKZCustom::BrushRotate(float Amount)
{
	int Rotation = (round_to_int(360.0f * Amount / (pi * 2)) / 90) % 4; // 0=0°, 1=90°, 2=180°, 3=270°
	if(Rotation < 0)
		Rotation += 4;

	if(Rotation == 1 || Rotation == 3)
	{
		// 90° rotation
		CKZCustomTile *pTempData1 = new CKZCustomTile[m_Width * m_Height];
		CTile *pTempData2 = new CTile[m_Width * m_Height];
		mem_copy(pTempData1, m_pKZCustomTile, (size_t)m_Width * m_Height * sizeof(CKZCustomTile));
		mem_copy(pTempData2, m_pTiles, (size_t)m_Width * m_Height * sizeof(CTile));
		CKZCustomTile *pDst1 = m_pKZCustomTile;
		CTile *pDst2 = m_pTiles;
		for(int x = 0; x < m_Width; ++x)
			for(int y = m_Height - 1; y >= 0; --y, ++pDst1, ++pDst2)
			{
				*pDst1 = pTempData1[y * m_Width + x];
				*pDst2 = pTempData2[y * m_Width + x];
			}

		std::swap(m_Width, m_Height);
		delete[] pTempData1;
		delete[] pTempData2;
	}

	if(Rotation == 2 || Rotation == 3)
	{
		BrushFlipX();
		BrushFlipY();
	}
}

void CLayerKZCustom::FillSelection(bool Empty, std::shared_ptr<CLayer> pBrush, CUIRect Rect)
{
	if(m_Readonly || (!Empty && pBrush->m_Type != LAYERTYPE_TILES))
		return;

	Snap(&Rect); // corrects Rect; no need of <=

	Snap(&Rect);

	int sx = ConvertX(Rect.x);
	int sy = ConvertY(Rect.y);
	int w = ConvertX(Rect.w);
	int h = ConvertY(Rect.h);

	std::shared_ptr<CLayerKZCustom> pLt = std::static_pointer_cast<CLayerKZCustom>(pBrush);

	bool Destructive = m_pEditor->m_BrushDrawDestructive || Empty || IsEmpty(pLt);

	for(int y = 0; y < h; y++)
	{
		for(int x = 0; x < w; x++)
		{
			int fx = x + sx;
			int fy = y + sy;

			if(fx < 0 || fx >= m_Width || fy < 0 || fy >= m_Height)
				continue;

			if(!Destructive && GetTile(fx, fy).m_Index)
				continue;

			const int SrcIndex = Empty ? 0 : (y * pLt->m_Width + x % pLt->m_Width) % (pLt->m_Width * pLt->m_Height);
			const int TgtIndex = fy * m_Width + fx;

			SKZCustomTileStateChange::SData Previous{
				m_pKZCustomTile[TgtIndex].m_Val1,
				m_pKZCustomTile[TgtIndex].m_Val2,
				m_pKZCustomTile[TgtIndex].m_Flags,
				m_pKZCustomTile[TgtIndex].m_Index,
				m_pTiles[TgtIndex].m_Index};

			if(Empty || false) // no speed up tile chosen: reset
			{
				m_pTiles[TgtIndex].m_Index = 0;
				m_pKZCustomTile[TgtIndex].m_Val1 = 0;
				m_pKZCustomTile[TgtIndex].m_Val2 = 0;

				if(!Empty)
					ShowPreventUnusedTilesWarning();
			}
			else
			{
				m_pTiles[TgtIndex] = pLt->m_pTiles[SrcIndex];
				if(pLt->m_KZCustom && m_pTiles[TgtIndex].m_Index > 0)
				{
					m_pKZCustomTile[TgtIndex].m_Index = m_pTiles[TgtIndex].m_Index;

					if((pLt->m_pKZCustomTile[SrcIndex].m_Val1 == 0 && m_pEditor->m_KZCustomVal1) || m_pEditor->m_KZCustomVal1 != pLt->m_KZCustomVal1)
						m_pKZCustomTile[TgtIndex].m_Val1 = m_pEditor->m_KZCustomVal1;
					else
						m_pKZCustomTile[TgtIndex].m_Val1 = pLt->m_pKZCustomTile[SrcIndex].m_Val1;

					if((pLt->m_pKZCustomTile[SrcIndex].m_Val2 == 0 && m_pEditor->m_KZCustomVal2) || m_pEditor->m_KZCustomVal2 != pLt->m_KZCustomVal2)
						m_pKZCustomTile[TgtIndex].m_Val2 = m_pEditor->m_KZCustomVal2;
					else
						m_pKZCustomTile[TgtIndex].m_Val2 = pLt->m_pKZCustomTile[SrcIndex].m_Val2;
				}
			}

			SKZCustomTileStateChange::SData Current{
				m_pKZCustomTile[TgtIndex].m_Val1,
				m_pKZCustomTile[TgtIndex].m_Val2,
				m_pKZCustomTile[TgtIndex].m_Flags,
				m_pKZCustomTile[TgtIndex].m_Index,
				m_pTiles[TgtIndex].m_Index};

			RecordStateChange(fx, fy, Previous, Current);
		}
	}
	FlagModified(sx, sy, w, h);
}

std::shared_ptr<CLayer> CLayerKZCustom::Duplicate() const
{
	return std::make_shared<CLayerKZCustom>(*this);
}

const char *CLayerKZCustom::TypeName() const
{
	return "kzcustom";
}
