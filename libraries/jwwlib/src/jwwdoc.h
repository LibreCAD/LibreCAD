#ifndef	JWWDOC_H
#define	JWWDOC_H

#include "jwtype.h"
/*
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <list>
#include <iterator>

typedef	double	DOUBLE;
typedef unsigned long DWORD, *LPDWORD;
typedef unsigned short WORD, *LPWORD;
typedef unsigned char BYTE, *LPBYTE;
//typedef	bool	BOOL;
using namespace std;
*/
typedef struct	_DPoint{
	DOUBLE	x;
	DOUBLE	y;
}DPoint;

//属性要素定義
typedef struct	_JWWLay{
	DWORD	m_aanLay;
	DWORD	m_aanLayProtect;
}JWWLay;

typedef struct	_JWWGLay{
	DWORD	m_anGLay;
	DWORD	m_anWriteLay;
	DOUBLE	m_adScale;
	DWORD	m_anGLayProtect;
	_JWWLay	m_nLay[16];
}JWWGLay;

typedef struct	_JWWZoom{
	DOUBLE m_dZoomJumpBairitsu;
	DPoint m_DPZoomJumpGenten;
	DWORD m_nZoomJumpGLay;
}JWWZoom;

typedef struct	_JWWPen{
	DWORD m_m_aPenColor;
	DWORD m_anPenWidth;
}JWWPen;

typedef struct	{
	DWORD m_aPrtpenColor;
	DWORD m_anPrtPenWidth;
	DOUBLE m_adPrtTenHankei;
}JWWPrtPen;

typedef struct	JWWLType1{
	DWORD m_alLtype;
	DWORD m_anTokushusSenUnitDot;
	DWORD m_anTokushuSenPich;
	DWORD m_anPrtTokushuSenPich;
}JWWLType1;

typedef struct	_JWWLType2{
	DWORD m_alLtype;
	DWORD m_anRandSenWide;
	DWORD m_anTokushuSenPich;
	DWORD m_anPrtRandSenWide;
	DWORD m_anPrtTokushuSenPich;
}JWWLType2;

typedef struct	_JWWLType3{
	DWORD m_alLtype;
	DWORD m_anTokushusSenUnitDot;
	DWORD m_anTokushuSenPich;
	DWORD m_anPrtTokushuSenPich;
}JWWLType3;

typedef struct	_JWWaMoji{
	double m_adMojiX;
	double m_adMojiY;
	double m_adMojiD;
	DWORD m_anMojiCol;
}JWWaMoji;

#define	SXCOL_EXT	100
typedef	struct _SXFCOL {
	//画面出力色
	DWORD	m_aPenColor[357];
    DWORD	m_anPenWidth[357];
    //プリンタ出力色
    string	m_astrUDColorName[257];
	DWORD	m_aPrtPenColor[357];
	DWORD  	m_anPrtPenWidth[357];
	DOUBLE	m_adPrtTenHankei[357];
}SXFCOL;

#define	SXLTP_EXT	30
typedef	struct	_SXFLTP {
	DWORD	m_alLType[63];
	DWORD	m_anTokushuSenUintDot[63];
	DWORD	m_anTokushuSenPich[63];
	DWORD	m_anPrtTokushuSenPich[63];
	string	m_astrUDLTypeName[33];                
	DWORD	m_anUDLTypeSegment[33];  
	DOUBLE	m_aadUDLTypePitch[33][11]; 
}SXFLTP;

//
// ヘッダー部
//
class	JWWHead
{
public:
	string head;	//JWWのデータファイルの宣言
	DWORD JW_DATA_VERSION;	//バージョンNo.
	string m_strMemo;	//ファイルメモ
	DWORD m_nZumen;	//図面サイズ
	DWORD m_nWriteGLay;	//レイヤグループ・レイヤ状態
	JWWGLay GLay[16];	//レイヤグループ・レイヤ状態
	DWORD Dummy[14];	//ダミー
	DWORD m_lnSunpou1;	//寸法関係の設定
	DWORD m_lnSunpou2;
	DWORD m_lnSunpou3;
	DWORD m_lnSunpou4;
	DWORD m_lnSunpou5;
	DWORD Dummy1;	//ダミー
	DWORD m_nMaxDrawWid;//線描画の最大幅
	DPoint m_DPPrtGenten;	//プリンタ出力範囲の原点(X,Y)
	DOUBLE m_dPrtBairitsu;	//プリンタ出力倍率
	DWORD m_nPrt90Kaiten;	//プリンタ90°回転出力、プリンタ出力基準点位置
	DWORD m_nMemoriMode;	//目盛設定モード
	DOUBLE m_dMemoriHyoujiMin;	//目盛表示最小間隔ドット
	DOUBLE m_dMemoriX;	//目盛表示間隔X
	DOUBLE m_dMemoriY;	//目盛表示間隔Y
	DPoint m_DpMemoriKijunTen;	//目盛基準点(X,Y)
	string m_aStrLayName[16][16];	//レイヤ名
	string m_aStrGLayName[16];	//レイヤグループ名
	DOUBLE m_dKageLevel;	//日影計算の条件 測定面高さ
	DOUBLE m_dKageIdo;	//緯度
	DWORD m_nKage9_15Flg;	//9縲・5の測定の指定
	DOUBLE m_dKabeKageLevel;	//壁面日影測定面高さ
	DOUBLE m_dTenkuuZuLevel;	//天空図の条件（Ver.3.00以降) 測定面高さ
	DOUBLE m_dTenkuuZuEnkoR;	//天空図の半径＊２
	DWORD m_nMMTani3D;	//2.5Dの計算単位(0以外はmm単位で計算)
	DOUBLE m_dBairitsu;	//保存時の画面倍率(読込むと前画面倍率になる)
	DPoint m_DPGenten;
	DOUBLE m_dHanniBairitsu;	//範囲記憶倍率と基準点(X,Y)
	DPoint m_DPHanniGenten;
	JWWZoom m_dZoom[9];	//マークジャンプ倍率、基準点(X,Y)およびレイヤグループ
    DOUBLE dDm11;   //ダミー
    DOUBLE dDm12;   //ダミー
    DOUBLE dDm13;   //ダミー
    DWORD  lnDm1;   //ダミー
    DOUBLE dDm21;   //ダミー
    DOUBLE dDm22;   //ダミー
    DOUBLE m_dMojiBG;	//(Ver.4.04以前はダミー）文字列範囲を背景色で描画するときの範囲増寸法
	DWORD  m_nMojiBG;
    DOUBLE m_adFukusenSuuchi[10];	//複線間隔
	DOUBLE m_dRyoygawaFukusenTomeDe;	//両側複線の留線出の寸法
	JWWPen m_Pen[10];	//色番号ごとの画面表示色、線幅
	JWWPrtPen m_PrtPen[10];	//色番号ごとのプリンタ出力色、線幅、実点半径
	JWWLType1 m_alLType1[10];//線種番号2から9までのパターン、1ユニットのドット数、ピッチ、プリンタ出力ピッチ
	JWWLType2 m_alLType2[16]; //ランダム線1から5までのパターン、画面表示振幅・ピッチ、プリンタ出力振幅・ピッチ
	JWWLType3 m_alLType3[20]; //倍長線種番号6から9までのパターン、1ユニットのドット数、ピッチ、プリンタ出力ピッチ
	DWORD m_nDrawGamenTen;	//実点を画面描画時の指定半径で描画
	DWORD m_nDrawPrtTen;	//実点をプリンタ出力時、指定半径で書く
	DWORD m_nBitMapFirstDraw;	//BitMap・ソリッドを最初に描画する
	DWORD m_nGyakuDraw;	//逆描画
	DWORD m_nGyakuSearch;	//逆サーチ
	DWORD m_nColorPrint;	//カラー印刷
	DWORD m_nLayJunPrint;	//レイヤ順の印刷
	DWORD m_nColJunPrint;	//色番号順の印刷
	DWORD m_nPrtRenzoku;	//レイヤグループまたはレイヤごとのプリンタ連続出力指定
	DWORD m_nPrtKyoutuuGray;	//プリンタ共通レイヤ(表示のみレイヤ)のグレー出力指定
	DWORD m_nPrtDispOnlyNonDraw;	//プリンタ出力時に表示のみレイヤは出力しない
	DWORD m_lnDrawTime;	//作図時間（Ver.2.23以降）
	DWORD nEyeInit;	//2.5Dの始点位置が設定されている時のフラグ（Ver.2.23以降）
	DWORD m_dEye_H_Ichi_1;	//2.5Dの透視図・鳥瞰図・アイソメ図の視点水平角（Ver.2.23以降）
	DWORD m_dEye_H_Ichi_2;	//
	DWORD m_dEye_H_Ichi_3;	//
	DOUBLE m_dEye_Z_Ichi_1;	//2.5Dの透視図の視点高さ・視点離れ（Ver.2.23以降）
	DOUBLE m_dEye_Y_Ichi_1;	//
	DOUBLE m_dEye_Z_Ichi_2;	//2.5Dの鳥瞰図の視点高さ・視点離れ（Ver.2.23以降）
	DOUBLE m_dEye_Y_Ichi_2;	//
	DOUBLE m_dEye_V_Ichi_3;	//2.5Dのアイソメ図の視点垂直角（Ver.2.23以降）
	DOUBLE m_dSenNagasaSunpou;	//線の長さ指定の最終値（Ver.2.25以降）
	DOUBLE m_dBoxSunpouX;	//矩形寸法横寸法・縦寸法指定の最終値（Ver.2.25以降）
	DOUBLE m_dBoxSunpouY;
	DOUBLE m_dEnHankeySunpou;	//円の半径指定の最終値（Ver.2.25以降）
	DWORD m_nSolidNinniColor;	//ソリッドを任意色で書くフラグ、ソリッドの任意色の既定値（Ver.2.30以降）
	DWORD m_SolidColor;	//RGB
	SXFCOL	m_SxfCol;	//SXF対応拡張線色定義（Ver.4.20以降）
	SXFLTP	m_SxfLtp;	//SXF対応拡張線種定義（Ver.4.20以降）
	JWWaMoji m_Moji[11];//文字種1から10までの文字幅、高さ、間隔、色番号
	DOUBLE m_dMojiSizeX;	//書込み文字の文字幅
	DOUBLE m_dMojiSizeY;	//書込み文字の高さ
	DOUBLE m_dMojiKankaku;	//間隔
	DWORD m_nMojiColor;	//色番号
	DWORD m_nMojiShu;	//文字番号
	DOUBLE m_dMojiSeiriGyouKan;	//文字位置整理の行間
	DOUBLE m_dMojiSeiriSuu;	//文字数
	DWORD m_nMojiKijunZureOn;	//文字基準点のずれ位置使用のフラグ
	DOUBLE m_adMojiKijunZureX[3];	//文字基準点の横方向のずれ位置左、中、右
	DOUBLE m_adMojiKijunZureY[3];	//文字基準点の縦方向のずれ位置下、中、上
	
};

//図形データの基底クラス
class	CData
{
protected:
	DWORD nOldVersionSave;	//バージョンNo.
public:
	DWORD m_lGroup;//曲線属性番号
	mutable BYTE m_nPenStyle;//線種番号
	WORD m_nPenColor;//線色番号
/////////////////////////////////////////////////
	mutable WORD m_nPenWidth;//線色幅 Ver.3.51以降
/////////////////////////////////////////////////
	WORD m_nLayer;//レイヤ番号
	WORD m_nGLayer;//レイヤグループ番号
	WORD m_sFlg;//属性フラグ
	const char* className(){return "CData";}
	friend inline std::ostream& operator<<(std::ostream&, const CData&); 
	friend inline std::istream& operator>>(std::istream&, CData&); 
	void SetVersion(DWORD ver){ nOldVersionSave = ver; }
	void Serialize(std::ofstream& ofstr) const{
       ofstr << (DWORD)m_lGroup;      //曲線属性番号
       ofstr << (BYTE)m_nPenStyle;   //線種番号
       ofstr << (WORD)m_nPenColor;   //線色番号
       if( nOldVersionSave >= 351 ){   //Ver.3.51以降
            ofstr << (WORD)m_nPenWidth;//線色幅
        }
        ofstr << (WORD)m_nLayer;      //レイヤ番号
        ofstr << (WORD)m_nGLayer;     //レイヤグループ番号
        ofstr << (WORD)m_sFlg;        //属性フラグ
	}
	void Serialize(std::ifstream& ifstr) {
       ifstr >> /*(DWORD)*/m_lGroup;      //曲線属性番号
       ifstr >> /*(BYTE)*/m_nPenStyle;   //線種番号
       ifstr >> /*(WORD)*/m_nPenColor;   //線色番号
       if( nOldVersionSave >= 351 ){   //Ver.3.51以降
            ifstr >> /*(WORD)*/m_nPenWidth;//線色幅
        }
        ifstr >> /*(WORD)*/m_nLayer;      //レイヤ番号
        ifstr >> /*(WORD)*/m_nGLayer;     //レイヤグループ番号
        ifstr >> /*(WORD)*/m_sFlg;        //属性フラグ
	}
};
inline std::ostream& operator<< (std::ostream& ostr, const CData& output) 
{
       ostr << "曲線属性番号:" << (DWORD)output.m_lGroup;      //曲線属性番号
       ostr << "線種番号:" << (BYTE)output.m_nPenStyle;   //線種番号
       ostr << "線色番号:" << (WORD)output.m_nPenColor;   //線色番号
       if( output.nOldVersionSave >= 351 ){   //Ver.3.51以降
            ostr << "線色幅:" << (WORD)output.m_nPenWidth;//線色幅
        }
        ostr << "レイヤ番号:" << (WORD)output.m_nLayer;      //レイヤ番号
        ostr << "レイヤグループ番号:" << (WORD)output.m_nGLayer;     //レイヤグループ番号
        ostr << "属性フラグ:" << (WORD)output.m_sFlg;        //属性フラグ
	return ostr;
} 

inline std::istream& operator>> (std::istream& istr, CData& input) 
{
       istr >> input.m_lGroup;      //曲線属性番号
       istr >> input.m_nPenStyle;   //線種番号
       istr >> input.m_nPenColor;   //線色番号
       if( input.nOldVersionSave >= 351 ){   //Ver.3.51以降
            istr >> input.m_nPenWidth;//線色幅
        }
        istr >> input.m_nLayer;      //レイヤ番号
        istr >> input.m_nGLayer;     //レイヤグループ番号
        istr >> input.m_sFlg;        //属性フラグ
//	ifstr.read((char*)&(input.data), sizeof(int));
	return istr;
}
/*
属性フラグ(m_sFlg)
-------------------------------------------------------------------------------
データ    線      円      実点    文字          ブロック    寸法     ソリッド
-------------------------------------------------------------------------------
(0x0010)          図形    図形    寸法値
(0x0020)ハッチ    ハッチ  ハッチ  縦字                               ハッチ
(0x0040)          寸法    寸法    真北
(0x0080)          建具    建具    日影
(0x0100)                          半径寸法値
(0x0200)                          直径寸法値
(0x0400)                          角度寸法値
(0x0800)図形                      図形属性選択   図形 
(0x1000)建具                      累計寸法値     建具 
(0x2000)寸法                      建具           寸法      寸法
(0x4000)                          寸法
(0x8000)包絡処理対象外の建具      2.5D
-------------------------------------------------------------------------------
*/

//線分データ
class	CDataSen	:	public	CData
{
public:
	DPoint m_start;//始点X座標、Y座標
	DPoint m_end;//終点X座標、Y座標
	const char* className(){return "CDataSen";}
	friend inline std::ostream& operator<<(std::ostream&, const CDataSen&); 
	friend inline std::istream& operator>>(std::istream&, CDataSen&); 
	void Serialize(std::ofstream& ofstr) const
#ifdef _MSC_VER // TODO VC 7.1 gives overload error.
        ;
#else
        {
	    CData::Serialize(ofstr);
		ofstr	<< (double)m_start.x << (double)m_start.y 
				<< (double)m_end.x << (double)m_end.y;
	}
#endif
	void Serialize(std::ifstream& ifstr){
	    CData::Serialize(ifstr);
		ifstr	>> m_start.x >> m_start.y
				>> m_end.x >> m_end.y;
	}
};
inline std::ostream& operator<< (std::ostream& ostr, const CDataSen& output) 
{
	ostr	<< (double)output.m_start.x << " " << (double)output.m_start.y  << endl
			<< (double)output.m_end.x << " " << (double)output.m_end.y << endl;
	return ostr;
} 

inline std::istream& operator>> (std::istream& istr, CDataSen& input) 
{
	istr	>> input.m_start.x >> input.m_start.y
			>> input.m_end.x >> input.m_end.y;
	return istr;
}
typedef	CDataSen*	PCDataSen;

//円弧データ
class	CDataEnko	:	public	CData
{
public:
	DPoint m_start;//中心点X座標、Y座標
	DOUBLE m_dHankei;//半径
	DOUBLE m_radKaishiKaku;//開始角
	DOUBLE m_radEnkoKaku;//円弧角
	DOUBLE m_radKatamukiKaku;//傾き角
	DOUBLE m_dHenpeiRitsu;//扁平率
	DWORD m_bZenEnFlg;//全円フラグ
	const char* className(){return "CDataEnko";}
	friend inline std::ostream& operator<<(std::ostream&, const CDataEnko&); 
	friend inline std::istream& operator>>(std::istream&, CDataEnko&); 
	void Serialize(std::ofstream& ofstr) const
#ifdef _MSC_VER
        ;
#else
        {
	    CData::Serialize(ofstr);
            ofstr	<< (double)m_start.x << (double)m_start.y
				<< (double)m_dHankei
				<< (double)m_radKaishiKaku
				<< (double)m_radEnkoKaku
				<< (double)m_radKatamukiKaku
				<< (double)m_dHenpeiRitsu
				<< (DWORD )m_bZenEnFlg;
	}
#endif
	void Serialize(std::ifstream& ifstr){
	    CData::Serialize(ifstr);
		ifstr >> /*(double)*/m_start.x >> /*(double)*/m_start.y
			>> /*(double)*/m_dHankei
			>> /*(double)*/m_radKaishiKaku
			>> /*(double)*/m_radEnkoKaku
			>> /*(double)*/m_radKatamukiKaku
			>> /*(double)*/m_dHenpeiRitsu
			>> /*(DWORD)*/m_bZenEnFlg;
	}
};
typedef CDataEnko*	PCDataEnko;
inline std::ostream& operator<< (std::ostream& ostr, const CDataEnko& output)
{
        ostr	<< (double)output.m_start.x << " " << (double)output.m_start.y << endl
				<< (double)output.m_dHankei << endl
				<< (double)output.m_radKaishiKaku << endl
				<< (double)output.m_radEnkoKaku << endl
				<< (double)output.m_radKatamukiKaku << endl
				<< (double)output.m_dHenpeiRitsu << endl
				<< (DWORD )output.m_bZenEnFlg << endl;
	return ostr;
} 

inline std::istream& operator>> (std::istream& istr, CDataEnko& input) 
{
		istr >> input.m_start.x >> input.m_start.y
			>> input.m_dHankei
			>> input.m_radKaishiKaku
			>> input.m_radEnkoKaku
			>> input.m_radKatamukiKaku
			>> input.m_dHenpeiRitsu
			>> input.m_bZenEnFlg;
	return istr;
}

//点データ
class	CDataTen	:	public	CData
{
public:
	DPoint m_start;	//点X座標、Y座標
	DWORD m_bKariten;//仮点フラグ
	DWORD m_nCode;	//m_nPenStyleが「100」のとき点コード（矢印・ポイントマーカー）
	DOUBLE m_radKaitenKaku;	//表示角
	DOUBLE m_dBairitsu;	//表示倍率
	const char* className(){return "CDataTen";}
	friend inline std::ostream& operator<<(std::ostream&, const CDataTen&); 
	friend inline std::istream& operator>>(std::istream&, CDataTen&); 
	void Serialize(std::ofstream& ofstr) const
#ifdef _MSC_VER
        ;
#else
        {
            m_nPenStyle = 1;
            if( nOldVersionSave >= 252 ){   //Ver.2.52以降
                if( 0 != m_nCode ){ m_nPenStyle = 100; }
            }
            CData::Serialize(ofstr);

            ofstr << (double)m_start.x << (double)m_start.y;
            ofstr << (DWORD)m_bKariten;
            if( 100 == m_nPenStyle ){
                ofstr << (DWORD )m_nCode;
                ofstr << (double)m_radKaitenKaku;
                ofstr << (double)m_dBairitsu;
            }
	}
#endif
	void Serialize(std::ifstream& ifstr){
	    CData::Serialize(ifstr);
        ifstr >> m_start.x >> m_start.y;
        ifstr >> m_bKariten;
        if( 100 == m_nPenStyle ){
            ifstr >> m_nCode;
            ifstr >> m_radKaitenKaku;
            ifstr >> m_dBairitsu;
       }
	}
};
typedef CDataTen*	PCDataTen;
inline std::ostream& operator<< (std::ostream& ostr, const CDataTen& output) 
{
        ostr << "x:" <<(double)output.m_start.x << " y:" << (double)output.m_start.y << endl;
        ostr << (DWORD)output.m_bKariten << endl;
        if( 100 == output.m_nPenStyle ){
            ostr << (DWORD )output.m_nCode << endl;
            ostr << (double)output.m_radKaitenKaku << endl;
            ostr << (double)output.m_dBairitsu << endl;
        }
	return ostr;
} 

inline std::istream& operator>> (std::istream& istr, CDataTen& /*input*/)
{
	return istr;
}

//文字データ
class	CDataMoji	:	public	CData
{
public:
	DPoint m_start;//始点X座標、Y座標
	DPoint m_end;//終点X座標、Y座標
	mutable DWORD m_nMojiShu;//文字種(斜体文字は20000、ボールド体は10000を加えた数値)
	DOUBLE m_dSizeX;//文字サイズ横
	DOUBLE m_dSizeY;//文字サイズ縦
	DOUBLE m_dKankaku;//文字間隔
	DOUBLE m_degKakudo;//角度
	string m_strFontName;//フォント名
	string m_string;//文字列
	const char* className(){return "CDataEnko";}
	friend inline std::ostream& operator<<(std::ostream&, const CDataMoji&); 
	friend inline std::istream& operator>>(std::istream&, CDataMoji&); 
	void Serialize(std::ofstream& ofstr) const
#ifdef _MSC_VER
        ;
#else
        {
////////////////////////////////////////////
//SKIP        m_nPenWidth = m_nSunpouFlg; //  (寸法値設定のフラグ)ヘッダーメンバー
        CData::Serialize(ofstr);
        m_nPenWidth = 1;            //文字枠幅を1
//SKIP        if( m_sMojiFlg & 0x0001 ){ m_nMojiShu += 10000; }  //斜体文字
//SKIP        if( m_sMojiFlg & 0x0010 ){ m_nMojiShu += 20000; }  //ボールド

        ofstr << (double)m_start.x << (double)m_start.y 
           << (double)m_end.x << (double)m_end.y
           << (DWORD)m_nMojiShu
           << (double)m_dSizeX << (double)m_dSizeY
           << (double)m_dKankaku
           << (double)m_degKakudo;

		int len = m_strFontName.length();
		if( len == 0 ){
			ofstr << (BYTE)0x0;
		}else
		{
			if( len >= 0xFF ){
				ofstr << (BYTE)0xFF;
				ofstr << (WORD)len;
			}else{
				ofstr << (BYTE)len;
			}
			ofstr.write(m_strFontName.c_str(),len);
		}
		len = m_string.length();
		if( len == 0 ){
			ofstr << (BYTE)0x0;
		}else
		{
			if( len >= 0xFF ){
				ofstr << (BYTE)0xFF;
				ofstr << (WORD)len;
			}else{
				ofstr << (BYTE)len;
			}
			ofstr.write(m_string.c_str(),len);
		}
        m_nMojiShu = (m_nMojiShu % 10000);
	}
#endif
	void Serialize(std::ifstream& ifstr) {
        CData::Serialize(ifstr);
        ifstr >> m_start.x >> m_start.y 
           >> m_end.x >> m_end.y
           >> m_nMojiShu
           >> m_dSizeX >> m_dSizeY
           >> m_dKankaku
           >> m_degKakudo;
		BYTE bt;
		WORD wd;
         char buf[512];
			ifstr >> bt;
			if( bt != 0xFF ){
				ifstr.read(buf,bt);
				buf[bt] = '\0';
				m_strFontName = buf;
#ifdef	DATA_DUMP
cout << "MojiData1:" << m_strFontName << endl;
#endif
			}else
			{
				ifstr >> wd;
				ifstr.read(buf,wd);
				buf[wd] = '\0';
				m_strFontName = buf;
#ifdef	DATA_DUMP
cout << "MojiData2:" << m_strFontName << endl;
#endif
			}
			ifstr >> bt;
			if( bt != 0xFF ){
				ifstr.read(buf,bt);
				buf[bt] = '\0';
				m_string = buf;
#ifdef	DATA_DUMP
cout << "MojiData3:"  << m_string << endl;
#endif
			}else
			{
				ifstr >> wd;
				ifstr.read(buf,wd);
				buf[wd] = '\0';
				m_string = buf;
#ifdef	DATA_DUMP
cout << "MojiData4:"  << m_string << endl;
#endif
			}
/*        m_nPenWidth = 1;            //文字枠幅を1
        if( m_sMojiFlg & 0x0001 ){ m_nMojiShu += 10000; }  //斜体文字
        if( m_sMojiFlg & 0x0010 ){ m_nMojiShu += 20000; }  //ボールド
        m_nMojiShu = (m_nMojiShu % 10000);
        m_nPenWidth = m_nSunpouFlg; //  (寸法値設定のフラグ)
*/
	}
};
typedef CDataMoji*	PCDataMoji;
inline std::ostream& operator<< (std::ostream& ostr, const CDataMoji& /*output*/)
{
	return ostr;
} 

inline std::istream& operator>> (std::istream& istr, CDataMoji& /*input*/)
{
	return istr;
}


//寸法データ
class	CDataSunpou	:	public	CData
{
public:
	CDataSen m_Sen;	//線分メンバ
	CDataMoji m_Moji;	//文字メンバ
	WORD m_bSxfMode;	//SXFのモード
	CDataSen m_SenHo1;	//補助線1
	CDataSen m_SenHo2;	//補助線2
	CDataTen m_Ten1;	//矢印（点)1
	CDataTen m_Ten2;	//矢印（点)2
	CDataTen m_TenHo1;	//基準点1
	CDataTen m_TenHo2;	//基準点2
	const char* className(){return "CDataSunpou";}
	friend inline std::ostream& operator<<(std::ostream&, const CDataSunpou&); 
	friend inline std::istream& operator>>(std::istream&, CDataSunpou&); 
	void Serialize(std::ofstream& ofstr) const {
	    CData::Serialize(ofstr);
        m_Sen .Serialize(ofstr);
        m_Moji.Serialize(ofstr);
        if( nOldVersionSave >= 420 ){  //Ver.4.20以降
            ofstr << (WORD )m_bSxfMode;
            m_SenHo1 .Serialize(ofstr);
            m_SenHo2 .Serialize(ofstr);
            m_Ten1   .Serialize(ofstr);
            m_Ten2   .Serialize(ofstr);
            m_TenHo1 .Serialize(ofstr);
            m_TenHo2 .Serialize(ofstr);
        }
	}
	void Serialize(std::ifstream& ifstr) {
	    CData::Serialize(ifstr);
        m_Sen .Serialize(ifstr);
        m_Moji.Serialize(ifstr);
        if( nOldVersionSave >= 420 ){  //Ver.4.20以降
            ifstr >> m_bSxfMode;
            m_SenHo1 .Serialize(ifstr);
            m_SenHo2 .Serialize(ifstr);
            m_Ten1   .Serialize(ifstr);
            m_Ten2   .Serialize(ifstr);
            m_TenHo1 .Serialize(ifstr);
            m_TenHo2 .Serialize(ifstr);
        }
	}
};
typedef CDataSunpou* PCDataSunpou;
inline std::ostream& operator<< (std::ostream& ostr, const CDataSunpou& /*output*/)
{
	return ostr;
} 

inline std::istream& operator>> (std::istream& istr, CDataSunpou& /*input*/)
{
	return istr;
}
/*
m_nPenWidth = m_nSunpouFlg; //  (寸法値設定のフラグ)
「寸法値設定のフラグ」は次のようになる
---------------------------------------------
    0x0002  寸法の文字設定有のフラグ
    0x0008  全角文字
    0x0010  寸法単位
    0x0020  寸法単位追加
    0x0040  ３桁毎のカンマ表示
    0x0080  少数点以下の０表示
    0x0100  少数桁以下  切捨（切捨、切上指定以外は四捨五入）
    0x0200  少数桁以下  切上
    0x0400  "φ" 、"R"  前付
    0x0800  "φ" 、"R"  後付
    0x1000  少数点以下の桁数 1
    0x2000  少数点以下の桁数 2
    0x3000  少数点以下の桁数 3
---------------------------------------------
*/

//ソリッドデータ
class	CDataSolid	:	public	CData
{
public:
	DPoint m_start;//第1点X座標、Y座標
	DPoint m_end;//第4点4座標、Y座標
	DPoint m_DPoint2;//第2点X座標、Y座標
	DPoint m_DPoint3;//第3点X座標、Y座標
	DWORD m_Color;//塗潰し色のRGB値(任意色の場合のみ)
	const char* className(){return "CDataSolid";}
	friend inline std::ostream& operator<<(std::ostream&, const CDataSolid&); 
	friend inline std::istream& operator>>(std::istream&, CDataSolid&); 
	void Serialize(std::ofstream& ofstr) const
#ifdef _MSC_VER
        ;
#else
        {
            CData::Serialize(ofstr);
            ofstr << (double)m_start.x << (double)m_start.y 
                << (double)m_end.x << (double)m_end.y
                << (double)m_DPoint2.x << (double)m_DPoint2.y
                << (double)m_DPoint3.x << (double)m_DPoint3.y;
            if( 10 == m_nPenColor ){
                ofstr << (DWORD)m_Color;//RGB
            }
        }
#endif
	void Serialize(std::ifstream& ifstr) {
	    CData::Serialize(ifstr);
        ifstr >> m_start.x >> m_start.y 
           >> m_end.x >> m_end.y
           >> m_DPoint2.x >> m_DPoint2.y
           >> m_DPoint3.x >> m_DPoint3.y;
        if( 10 == m_nPenColor ){
          	ifstr >> m_Color;//RGB
        }
	}
};
typedef CDataSolid*	PCDataSolid;
inline std::ostream& operator<< (std::ostream& ostr, const CDataSolid& /*output*/)
{
	return ostr;
} 

inline std::istream& operator>> (std::istream& istr, CDataSolid& /*input*/)
{
	return istr;
}

//部品データ
class	CDataBlock	:	public	CData
{
public:
	DPoint m_DPKijunTen;//基準点X座標、Y座標
	DOUBLE m_dBairitsuX;//X方向の倍率
	DOUBLE m_dBairitsuY;//Y方向の倍率
	DOUBLE m_radKaitenKaku;//回転角
	DWORD m_n_Number;//ブロック定義データの通し番号
	const char* className(){return "CDataBlock";}
	friend inline std::ostream& operator<<(std::ostream&, const CDataBlock&); 
	friend inline std::istream& operator>>(std::istream&, CDataBlock&); 
	void Serialize(std::ofstream& ofstr) const
#ifdef _MSC_VER
        ;
#else
        {
	    CData::Serialize(ofstr);
        ofstr <<(double)m_DPKijunTen.x <<(double)m_DPKijunTen.y
           <<(double)m_dBairitsuX
           <<(double)m_dBairitsuY
           <<(double)m_radKaitenKaku
           <</*(DWORD)m_pDataList->*/m_n_Number;//ポインタでなく通し番号を保存する
	}
#endif
	void Serialize(std::ifstream& ifstr){
	    CData::Serialize(ifstr);
        ifstr >> m_DPKijunTen.x >> m_DPKijunTen.y
           >> m_dBairitsuX
           >> m_dBairitsuY
           >> m_radKaitenKaku
           >>/*(DWORD)m_pDataList->*/m_n_Number;//ポインタでなく通し番号を保存する
	}
};
typedef	CDataBlock*	PCDataBlock;
inline std::ostream& operator<< (std::ostream& ostr, const CDataBlock& /*output*/)
{
	return ostr;
} 

inline std::istream& operator>> (std::istream& istr, CDataBlock& /*input*/)
{
	return istr;
}

//ブロック定義データ
class	CDataList	:	public	CData
{
public:
	DWORD m_nNumber;	//定義データの通し番号
	DWORD m_bReffered;	//参照されているかのフラグ
	DWORD m_time;	//定義された時間
	string m_strName;	//定義データの名前
						//Ver.4.10 以降、名前の後ろに
                        //"@@SfigorgFlag@@"に続けて、複合図形種別フラグを付加
                        //1:部分図(数学座標系)、2: 部分図(測地座標系)、
                        //3:作図グループ、4:作図部品
	vector<CData*> m_DataList;	//定義データの実体のリスト
	WORD Count;	//テスト用
	const char* className(){return "CDataList";}
	friend inline std::ostream& operator<<(std::ostream&, const CDataList&); 
	friend inline std::istream& operator>>(std::istream&, CDataList&); 
	void Serialize(std::ofstream& ofstr) const{
	    CData::Serialize(ofstr);
        ofstr <<(DWORD)m_nNumber
        	<<(DWORD)m_bReffered
           <<m_time;
        //Ver.4.10 以降、名前の後ろに
        //"@@SfigorgFlag@@"に続けて、複合図形種別フラグを付加
        //1:部分図(数学座標系)、2: 部分図(測地座標系)、
        //3:作図グループ、4:作図部品
		int len = m_strName.length();
		if( len == 0 ){
			ofstr << (BYTE)0x0;
		}else
		{
			if( len >= 0xFF ){
				ofstr << (BYTE)0xFF;
				ofstr << (WORD)len;
			}else{
				ofstr << (BYTE)len;
			}
			ofstr.write(m_strName.c_str(), len);
		}
	    //SKIP m_DataList.Serialize(ofstr);
	}
	void Serialize(std::ifstream& ifstr) {
	    CData::Serialize(ifstr);
        ifstr >> m_nNumber
           >> m_bReffered
           >> m_time;
		//Ver.4.10 以降、名前の後ろに
		//"@@SfigorgFlag@@"に続けて、複合図形種別フラグを付加
		//1:部分図(数学座標系)、2: 部分図(測地座標系)、
		//3:作図グループ、4:作図部品
		BYTE bt;
		WORD wd;
		char buf[512];
		ifstr >> bt;
		if( bt != 0xFF ){
			ifstr.read(buf,bt);
			buf[bt] = '\0';
			m_strName = buf;
#ifdef	DATA_DUMP
cout << "MojiData1:"  << m_strName << endl;
#endif
		}else
		{
			ifstr >> wd;
			ifstr.read(buf,wd);
			buf[wd] = '\0';
			m_strName = buf;
#ifdef	DATA_DUMP
cout << "MojiData1:"  << m_strName << endl;
#endif
		}
	    //SKIP m_DataList.Serialize(ifstr);
	}
};
typedef	CDataList* PCDataList;
inline std::ostream& operator<< (std::ostream& ostr, const CDataList& /*output*/)
{
	return ostr;
} 

inline std::istream& operator>> (std::istream& istr, CDataList& /*input*/)
{
	return istr;
}

//図形要素
typedef	enum{
	Sen,
	Enko,
	Ten,
	Moji,
	Sunpou,
	Solid,
	Block
}CDataType;
typedef	CDataType* PCDataType;

//
// ブロックデータの実体のクラス
// DataTypeプロパティーを読んでから振り分けてDataSenなどで読み込む
//
class	JWWBlockList
{
private:
	vector<PCDataBlock> FBlockList;
	vector<PCDataList> FDataList;
	vector<CDataType> FDataType;
protected:
public:
	JWWBlockList();
	~JWWBlockList();
	CDataList GetBlockList(unsigned int i);
	int getBlockListCount();
    int GetDataListCount(unsigned int i);
    void* GetData(unsigned int i, int j );
    CDataType GetDataType(unsigned int i, int j );

	CDataEnko GetCDataEnko(int i, int j );
	CDataMoji GetCDataMoji(int i, int j );
	CDataSen GetCDataSen(int i, int j );
	CDataSolid GetCDataSolid(int i, int j );
	CDataSunpou GetCDataSunpou(int i, int j );
	CDataTen GetCDataTen(int i, int j );
	CDataType GetCDataType(int i, int j );
	CDataBlock GetCDataBlock(int i, int j );

	void AddBlockList(CDataList& CData);
	void AddDataListSen(CDataSen& D);
	void AddDataListEnko(CDataEnko& D);
	void AddDataListTen(CDataTen& D);
	void AddDataListMoji(CDataMoji& D);
	void AddDataListSolid(CDataSolid& D);
	void AddDataListSunpou(CDataSunpou& D);
	void AddDataListBlock(CDataBlock& D);
	void Init();
};

//図形番号リスト
typedef struct	_NoList{
	string CDataString;
	int No;
}NoList;
typedef	NoList*	PNoList;

//データ格納リスト
class	JWWList
{
private:
	vector<PNoList> FList;

public:
	JWWList();
	~JWWList();
	int GetCount();
	NoList& GetItem(int i);
	NoList& GetNoByItem(int i);
	void AddItem(int No,string& str);
};

//JWWファイル入出力クラス
class	JWWDocument
{
private:
	string	InputFName,
			OutputFName;
public:
	JWWDocument(string& iFName, string& oFName){
		InputFName = iFName;
		if(iFName.length()>0)
			ifs = new ifstream(iFName.c_str(),ios::binary);
		else
			ifs = NULL;
		OutputFName = oFName;
		if(oFName.length()>0)
			ofs = new ofstream(oFName.c_str(),ios::binary|ios::trunc);
		else
			ofs = NULL;
		pList = new JWWList();
		pBlockList = new JWWBlockList();
	}
	~JWWDocument(){
		delete pList;
		delete pBlockList;
		if(ifs){
			ifs->close();
			delete ifs;
		}
		if(ofs){
			ofs->close();
			delete ofs;
		}
	}
// 各図形のレコードの実体
	JWWHead	Header;
	ifstream*	ifs;
	ofstream*	ofs;
	WORD objCode;
	DWORD Mpoint;
	DWORD PSen;
	DWORD PEnko;
	DWORD PTen;
	DWORD PMoji;
	DWORD PSunpou;
	DWORD PSolid;
	DWORD PBlock;
	DWORD PList;
	DWORD SenCount;
	DWORD EnkoCount;
	DWORD TenCount;
	DWORD MojiCount;
	DWORD SolidCount;
	DWORD BlockCount;
	DWORD SunpouCount;
    DWORD SaveSenCount;
    DWORD SaveEnkoCount;
    DWORD SaveTenCount;
    DWORD SaveMojiCount;
    DWORD SaveSunpouCount;
    DWORD SaveSolidCount;
    DWORD SaveBlockCount;
    DWORD SaveDataListCount;

	vector<CDataSen>	vSen;//
	vector<CDataEnko>	vEnko;//
	vector<CDataTen>	vTen;//
	vector<CDataMoji>	vMoji;//
	vector<CDataSolid>	vSolid;//
	vector<CDataBlock>	vBlock;//
	vector<CDataSunpou>	vSunpou;//
	JWWList*	pList;//
	JWWBlockList*	pBlockList;//ブロックデータ定義部のリスト
	vector<CData*>   m_DataList;    //図形データのリスト
	vector<CDataList*>	m_DataListList;  //ブロックデータ定義部のリスト
	void WriteString(string s);
	string ReadData(int n);
	string ReadString();
	BOOL ReadHeader();
	BOOL WriteHeader();
	BOOL Read();
	BOOL Save();
	BOOL SaveBich16(DWORD id);
	BOOL SaveSen(CDataSen const& DSen);
	BOOL SaveEnko(CDataEnko const& DEnko);
	BOOL SaveTen(CDataTen const& DTen);
	BOOL SaveMoji(CDataMoji const& DMoji);
	BOOL SaveSunpou(CDataSunpou const& DSunpou);
	BOOL SaveSolid(CDataSolid const& DSolid);
	BOOL SaveBlock(CDataBlock const& DBlock);
	BOOL SaveDataList(CDataList const& DList);
};

#endif //JWWDOC_H
