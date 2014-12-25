/************************************************************************************************************
 *
 * @file	Scratch.h
 * @brief	
 * @author	Yanagisawa Takahiro
 * @date	
 *
 ************************************************************************************************************/
#ifndef _GAME_H_
#define _GAME_H_

// Marmalade SDK
#include "IwGx.h"

// Project
#include "Main.h"
#include "Sequence.h"

// YanyMarmalade
#include "ymAnim2D.h"

class CHud;

/**
 *
 */
class CTreasure
{
public:
	//---------------------------
	bool isAvailable(){return m_bAvailable;}
	void setAvailable(bool flag){m_bAvailable = flag;}
	//---------------------------
	int getType(){return m_nType;}
	void setType(int type){m_nType = type;}
	//---------------------------
	CIwSVec2 getPosition(){return m_vPosition;}
	int getPotisionX(){return m_vPosition.x;}
	int getPotisionY(){return m_vPosition.y;}
	void setPosition(CIwSVec2 pos){m_vPosition = pos;}
	void setPositionX(int x){m_vPosition.x = x;}
	void setPositionY(int y){m_vPosition.y = y;}
	//---------------------------
	int getScore(){return m_nScore;}
	void setScore(int score){m_nScore = score;}

private:
	bool		m_bAvailable;
	int			m_nType;
	CIwSVec2	m_vPosition;
	int			m_nScore;
};

/**
 *
 */
class CStageMain : public CSequence
{
public:
	CStageMain(){};
	~CStageMain(){};

	void		Initialize();
	void		Terminate();
	SEQUENCE	Proc();
	void		Render();

private:
	enum IMAGE{
		IMAGE_COIN,
		IMAGE_BOX,
		IMAGE_MAX,
	};
	CIwMaterial*	m_GameImage[IMAGE_MAX];

	CIwTexture*		m_ScratchTexture;
	CIwMaterial*	m_ScratchMaterial;

	enum ANIM{
		ANIM_TRASH,
		ANIM_GET_S,
		ANIM_GET_M,
		ANIM_GET_L,
		ANIM_MAX
	};
	YM::CYmAnim2D*		m_Anim[ANIM_MAX];

	bool	m_bTrash;

	static const int SCRATCH_DOT = 10;			// 削るドット数
	int		m_nTouchOldSX,m_nTouchOldSY;		// 以前削った位置情報

	CHud*			m_Hud;

private:
	int				m_nTreasureNum;
	CTreasure*		m_Treasure;

};

#endif // _GAME_H_

