/************************************************************************************************************
 *
 * @file	Scratch.cpp
 * @brief	
 * @author	Yanagisawa Takahiro
 * @date	
 *
 ************************************************************************************************************/
// Marmalade SDK
#include "s3eAudio.h"
#include "IwGx.h"
#include "derbh.h"

// YanyMarmalade
#include "ymInputTouch.h"
#include "ymRender2D.h"
#include "ymResolution.h"
#include "ymFileIO.h"

// Project
#include "StageMain.h"
#include "Hud.h"

/**
 * 準備
 */
void CStageMain::Initialize()
{
	IwGetResManager()->LoadGroup("game.group");
	CIwResGroup*	pGroup = IwGetResManager()->GetGroupNamed("game");

	// お宝
	m_GameImage[IMAGE_COIN]		= (CIwMaterial *)pGroup->GetResNamed("coin", IW_GX_RESTYPE_MATERIAL);
	m_GameImage[IMAGE_BOX]		= (CIwMaterial *)pGroup->GetResNamed("box", IW_GX_RESTYPE_MATERIAL);
	// お宝配置
	{
		int		stageNo = 1;
		char	buf[256];
		sprintf(buf,"stage/stage%02d.csv",stageNo);
		if(YM::CYmFileIO::OpenRead(buf,"rb")){
			char*	pStr = strtok(YM::CYmFileIO::getData(),",");
			m_nTreasureNum = atoi(pStr);	pStr = strtok(NULL,",");
			m_Treasure = new CTreasure[m_nTreasureNum];
			for(int i=0;i<m_nTreasureNum;i++){
				m_Treasure[i].setAvailable(true);
				m_Treasure[i].setType(atoi(pStr));			pStr = strtok(NULL,",");
				m_Treasure[i].setPositionX(atoi(pStr));		pStr = strtok(NULL,",");
				m_Treasure[i].setPositionY(atoi(pStr));		pStr = strtok(NULL,",");
				m_Treasure[i].setScore(atoi(pStr));			pStr = strtok(NULL,",");
			}
			YM::CYmFileIO::CloseRead();
		}
	}

	// 削り部分
	{
		dzArchiveAttach("derbh/Scratch.dz");
		char	buf[256];
		memset(buf,0,256);
		snprintf(buf, sizeof(buf),"scratch.png");
		if(s3eFileCheckExists(buf)){
			CIwImage img;
			img.LoadFromFile(buf);
			m_ScratchTexture = new CIwTexture;
			m_ScratchTexture->GetImage().SetFormat(CIwImage::RGBA_5551);
			img.ConvertToImage(&m_ScratchTexture->GetImage());
			//m_ScratchTexture->SetFormatSW(CIwImage::RGBA_5551);
			m_ScratchTexture->SetFormatHW(CIwImage::RGBA_5551);
			m_ScratchTexture->SetModifiable(true);
			m_ScratchTexture->SetMipMapping(false);
			m_ScratchTexture->Upload();
			m_ScratchMaterial = new CIwMaterial;
			m_ScratchMaterial->SetTexture(m_ScratchTexture);
		}
		dzArchiveDetach();
	}
	// アニメ
	{
		dzArchiveAttach("derbh/Anim.dz");

		m_Anim[ANIM_TRASH] = new YM::CYmAnim2D("trash");
		m_Anim[ANIM_GET_S] = new YM::CYmAnim2D("get_S");
		m_Anim[ANIM_GET_M] = new YM::CYmAnim2D("get_M");
		m_Anim[ANIM_GET_L] = new YM::CYmAnim2D("get_H");
		m_Anim[ANIM_GET_S]->setTexture("get");
		m_Anim[ANIM_GET_M]->setTexture("get");
		m_Anim[ANIM_GET_L]->setTexture("get");

		dzArchiveDetach();
	}

	m_bTrash = false;
	m_nTouchOldSX = -1;
	m_nTouchOldSY = -1;

	m_Hud = new CHud;

	//s3eAudioPlay("/sound/bgm.mp3",0);
}
/**
 * 解放
 */
void CStageMain::Terminate()
{
	IwGetResManager()->DestroyGroup("game");
	
	delete [] m_Treasure;

	delete m_ScratchTexture;
	delete m_ScratchMaterial;
	
	for(int i=0;i<ANIM_MAX;i++){
		delete m_Anim[i];
	}
	delete m_Hud;

	//s3eAudioStop();
}
/**
 * 更新
 */
CSequence::SEQUENCE CStageMain::Proc()
{
	m_bTrash = false;

	//+-----------------------------------------------------------------
	//| ライト更新
	//+-----------------------------------------------------------------
	if(m_Hud->UpdateLight() == false){
		int nPattern = -1;
		while(1){
			nPattern = IwRandRange(CHud::LIGHT_PATTERN_MAX);
			if(nPattern != m_Hud->getLightPattern())break;
		}
		m_Hud->setLightPattern(nPattern);
	}

	if(m_Anim[ANIM_GET_M]->isPlaying() == true)return SEQUENCE_STAGE;

	//+-----------------------------------------------------------------
	//| 画面タッチ(削る)
	//+-----------------------------------------------------------------
	if(YM::CYmInputTouch::isInputDown()){
		int touchX = s3ePointerGetX();
		int touchY = s3ePointerGetY();

		int	seatWidth	= m_ScratchTexture->GetWidth();
		int	seatHeight	= m_ScratchTexture->GetHeight();
		int seatX		= YM::CYmResolution::getX(0);
		int seatY		= YM::CYmResolution::getY(0);

		// 削り部分を触っている
		if(touchY >= seatY && touchY < seatY+seatHeight*YM::CYmResolution::fRATE_H){
			if(touchX >= seatX && touchX < seatX+seatWidth*YM::CYmResolution::fRATE_W){
				uint16*		texels =  reinterpret_cast<uint16*>(m_ScratchTexture->GetTexels());
				int			texelsSize = m_ScratchTexture->GetTexelsMemSize()/2;
				
				// 削り終了チェック
				for(int i=0;i<m_nTreasureNum;i++){
					if(m_Treasure[i].isAvailable() == false)continue;

					int nCount = 0;
					int	treasureX = m_Treasure[i].getPotisionX();
					int	treasureY = m_Treasure[i].getPotisionY();
					int treasureWidth = m_GameImage[IMAGE_COIN]->GetTexture()->GetWidth();
					int treasureHeight = m_GameImage[IMAGE_COIN]->GetTexture()->GetHeight();
					for(int y=treasureY;y<treasureY+treasureHeight;y++){
						for(int x=treasureX;x<treasureX+treasureWidth;x++){
							if(texels[x+y*seatWidth] == 0)nCount++;
						}
					}
					// お宝ゲット
					if(nCount > static_cast<int>(treasureWidth*treasureHeight*0.6f)){
						if(m_Treasure[i].isAvailable()){
							m_Anim[ANIM_GET_M]->Reset();
							m_Anim[ANIM_GET_M]->setDispPosX(treasureX+treasureWidth/2);
							m_Anim[ANIM_GET_M]->setDispPosY(treasureY+treasureHeight/2);
							m_Anim[ANIM_GET_M]->Play();
							m_Treasure[i].setAvailable(false);
							m_Hud->addScore(m_Treasure[i].getScore());
						}
					}
				}

				// 削り処理
				int nTouchNowSX = static_cast<int>((touchX-seatX)/YM::CYmResolution::fRATE_W);
				int nTouchNowSY = static_cast<int>((touchY-seatY)/YM::CYmResolution::fRATE_H);
				if(m_nTouchOldSX == -1){
					m_nTouchOldSX = nTouchNowSX;
				}
				if(m_nTouchOldSY == -1){
					m_nTouchOldSY = nTouchNowSY;
				}

				bool	bLoop = true;
				int		nPos;
				do{
					for(int x=-SCRATCH_DOT;x<=SCRATCH_DOT;x++){
						for(int y=-SCRATCH_DOT;y<=SCRATCH_DOT;y++){
							if(abs(x) == SCRATCH_DOT && (abs(y) >= (SCRATCH_DOT-3) && abs(y) <= SCRATCH_DOT))continue;
							if(abs(y) == SCRATCH_DOT && (abs(x) >= (SCRATCH_DOT-3) && abs(x) <= SCRATCH_DOT))continue;
							nPos = (nTouchNowSX+x)+(nTouchNowSY+y)*seatWidth;
							if(nPos > 0 && nPos < texelsSize){
								if(texels[nPos] > 0){
									m_bTrash = true;
									texels[nPos] = 0;
								}
							}
						}
					}
					if(nTouchNowSX < m_nTouchOldSX){nTouchNowSX++;}
					else if(nTouchNowSX > m_nTouchOldSX){nTouchNowSX--;}
					if(nTouchNowSY < m_nTouchOldSY){nTouchNowSY++;}
					else if(nTouchNowSY > m_nTouchOldSY){nTouchNowSY--;}
					if(nTouchNowSX == m_nTouchOldSX && nTouchNowSY == m_nTouchOldSY){
						bLoop = false;
					}
				}while(bLoop);

				m_nTouchOldSX = static_cast<int>((touchX-seatX)/YM::CYmResolution::fRATE_W);
				m_nTouchOldSY = static_cast<int>((touchY-seatY)/YM::CYmResolution::fRATE_H);
			}
		}
		if(m_bTrash){
			m_Anim[ANIM_TRASH]->setDispPosX(static_cast<int>(touchX/YM::CYmResolution::fRATE_W));
			m_Anim[ANIM_TRASH]->setDispPosY(static_cast<int>(touchY/YM::CYmResolution::fRATE_H));
			m_Anim[ANIM_TRASH]->setLoop(true);
			m_Anim[ANIM_TRASH]->Play();
			m_ScratchTexture->ChangeTexels(m_ScratchTexture->GetTexels(),CIwImage::RGBA_5551);
		}
		else{
			m_Anim[ANIM_TRASH]->setLoop(false);
		}
	}
	else{
		m_nTouchOldSX = -1;
		m_nTouchOldSY = -1;
		m_Anim[ANIM_TRASH]->setLoop(false);
	}
	return SEQUENCE_STAGE;
}
/**
 * 描画
 */
void CStageMain::Render()
{
	// お宝
	for(int i=0;i<m_nTreasureNum;i++){
		if(m_Treasure[i].isAvailable() == false)continue;
		YM::CYmRender2D::Render2DImage(m_GameImage[m_Treasure[i].getType()],m_Treasure[i].getPotisionX(),m_Treasure[i].getPotisionY(),0xff);
	}
	// 削り部分
	YM::CYmRender2D::Render2DImage(m_ScratchMaterial,0,0,0xff);

	// 削りアニメ
	m_Anim[ANIM_TRASH]->Render();
	m_Anim[ANIM_GET_M]->Render();

	m_Hud->Render();
}
