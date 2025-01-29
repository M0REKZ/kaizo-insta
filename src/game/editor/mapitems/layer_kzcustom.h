#ifndef GAME_EDITOR_MAPITEMS_LAYER_KZCUSTOM_H
#define GAME_EDITOR_MAPITEMS_LAYER_KZCUSTOM_H

#include "layer_tiles.h"

struct SKZCustomTileStateChange
{
	bool m_Changed;
	struct SData
	{
		int m_Val1;
		int m_Type;
		int m_Flags;
		int m_Val2;
		int m_Index;
	} m_Previous, m_Current;
};

class CLayerKZCustom : public CLayerTiles
{
public:
	CLayerKZCustom(CEditor *pEditor, int w, int h);
	CLayerKZCustom(const CLayerKZCustom &Other);
	~CLayerKZCustom();

	CKZCustomTile *m_pKZCustomTile;
	int m_KZCustomVal1;
	int m_KZCustomVal2;
	//int m_KZCustomVal3;

	void Resize(int NewW, int NewH) override;
	void Shift(int Direction) override;
	bool IsEmpty(const std::shared_ptr<CLayerTiles> &pLayer) override;
	void BrushDraw(std::shared_ptr<CLayer> pBrush, vec2 WorldPos) override;
	void BrushFlipX() override;
	void BrushFlipY() override;
	void BrushRotate(float Amount) override;
	void FillSelection(bool Empty, std::shared_ptr<CLayer> pBrush, CUIRect Rect) override;

	EditorTileStateChangeHistory<SKZCustomTileStateChange> m_History;
	void ClearHistory() override
	{
		CLayerTiles::ClearHistory();
		m_History.clear();
	}

	std::shared_ptr<CLayer> Duplicate() const override;
	const char *TypeName() const override;

private:
	void RecordStateChange(int x, int y, SKZCustomTileStateChange::SData Previous, SKZCustomTileStateChange::SData Current);
};

#endif
