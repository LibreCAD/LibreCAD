#include "jwwdoc.h"

#define	LINEBUF_SIZE	1024

#ifdef _MSC_VER
void CDataSen::Serialize(std::ofstream& ofstr) const
{
    CData::Serialize(ofstr);
    ofstr	<< (double)m_start.x << (double)m_start.y
        << (double)m_end.x << (double)m_end.y;
}

void CDataEnko::Serialize(std::ofstream& ofstr) const
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

void CDataTen::Serialize(std::ofstream& ofstr) const
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

void CDataMoji::Serialize(std::ofstream& ofstr) const
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

void CDataSolid::Serialize(std::ofstream& ofstr) const
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

void CDataBlock::Serialize(std::ofstream& ofstr) const
{
    CData::Serialize(ofstr);
    ofstr <<(double)m_DPKijunTen.x <<(double)m_DPKijunTen.y
        <<(double)m_dBairitsuX
        <<(double)m_dBairitsuY
        <<(double)m_radKaitenKaku
        <</*(DWORD)m_pDataList->*/m_n_Number;//ポインタでなく通し番号を保存する
}

#endif

void JWWDocument::WriteString(string s){
    int len = s.length();
    if( len == 0 ){
        *ofs << (BYTE)0x0;
        return;
    }else if( len >= 0xFF ){
        *ofs << (BYTE)0xFF;
        *ofs << (WORD)len;
    }else{
        *ofs << (BYTE)len;
    }
    ofs->write(s.c_str(), len);
}

string JWWDocument::ReadData(int n)
{
    //avoid buffer overflow
    if(n>LINEBUF_SIZE){
        //should not happen
        std::cerr<<"JWWDocument::ReadData("<<n<<"): requested data length is larger than "<<LINEBUF_SIZE<<std::endl;
    }
    string	Result;
    Result.resize(n);
    if( n > 0 )
        ifs->read(&(Result[0]), n);
//    Result += '\0'; // may not be necessary, to make sure the string ends with '\0';
    return Result;

//	char cbuf[LINEBUF_SIZE];
//	cbuf[0] = (char)NULL;
//	if( n > 0 )
//		ifs->read(cbuf, n);
//	cbuf[n] = (char)NULL;
//	string	Result(cbuf, n);
//	return	Result;
}

string JWWDocument::ReadString()
{
    BYTE bt;
    WORD wd;
    string	Result("");
    *ifs >> bt;
    if( bt == 0 )
        return Result;
    else if( bt != 0xFF )
        Result = ReadData(bt);
    else
    {
        *ifs >> wd;
        Result = ReadData(wd);
    }
    return	Result;
}

//ヘッダー部読みだし(JWW形式とバージョンチェック)
BOOL JWWDocument::ReadHeader()
{
    int i;
    DWORD dw;
    DOUBLE db;
    string s;
//    WORD wd;
//    BYTE bt;
//    BOOL	Result = false;

    if(ifs)
    {
        //JWWのデータファイルの宣言
        s = ReadData(8);
        Header.head = s;
        if(Header.head =="JwwData.")
        {
            //バージョンNo.
            *ifs >> dw;
            Header.JW_DATA_VERSION = dw;
            if(Header.JW_DATA_VERSION == 230 || Header.JW_DATA_VERSION >= 300)
            {
                //ファイルメモ
                Header.m_strMemo = ReadString();
                //図面サイズ
                //0縲・ ：A0縲廣4
                //8    ：2 A
                //9    ：3 A
                //10   ：4 A
                //11   ：5 A
                //12   ：10m
                //13   ：50m
                //14   ：100m
                *ifs >> dw;
                Header.m_nZumen = dw;
                //レイヤグループ・レイヤ状態
                *ifs >> dw;
                Header.m_nWriteGLay = dw;
                for( i = 0; i < 16; i++ ){
                    *ifs >> dw;
                    Header.GLay[i].m_anGLay = dw;
                    *ifs >> dw;
                    Header.GLay[i].m_anWriteLay = dw;
                    *ifs >> db;
                    Header.GLay[i].m_adScale = db;
                    *ifs >> dw;
                    Header.GLay[i].m_anGLayProtect = dw;
                    for(int nLay=0;nLay<16;nLay++){
                        *ifs >> dw;
                        Header.GLay[i].m_nLay[nLay].m_aanLay = dw;
                        *ifs >> dw;
                        Header.GLay[i].m_nLay[nLay].m_aanLayProtect = dw;
                    }
                }
                //ダミー
                for( i = 0; i < 14; i++ )
                {
                    *ifs >> dw;
                    Header.Dummy[i]=dw;
                }
                //寸法関係の設定
                *ifs >> dw;
                Header.m_lnSunpou1 = dw;
                *ifs >> dw;
                Header.m_lnSunpou2 = dw;
                *ifs >> dw;
                Header.m_lnSunpou3 = dw;
                *ifs >> dw;
                Header.m_lnSunpou4 = dw;
                *ifs >> dw;
                Header.m_lnSunpou5 = dw;
                //ダミー
                *ifs >> dw;
                Header.Dummy1 = dw;
                //線描画の最大幅
                //「線幅を1/100mm単位とする」が設定されているときは「-101」
                *ifs >> dw;
                Header.m_nMaxDrawWid = dw;
                //プリンタ出力範囲の原点(X,Y)
                *ifs >> db;
                Header.m_DPPrtGenten.x = db;
                //
                *ifs >> db;
                Header.m_DPPrtGenten.y = db;
                //プリンタ出力倍率
                *ifs >> db;
                Header.m_dPrtBairitsu = db;
                //プリンタ90°回転出力、プリンタ出力基準点位置
                *ifs >> dw;
                Header.m_nPrt90Kaiten = dw;
                //目盛設定モード
                *ifs >> dw;
                Header.m_nMemoriMode = dw;
                //目盛表示最小間隔ドット
                *ifs >> db;
                Header.m_dMemoriHyoujiMin = db;
                //目盛表示間隔(X,Y)
                *ifs >> db;
                Header.m_dMemoriX = db;
                //
                *ifs >> db;
                Header.m_dMemoriY = db;
                //目盛基準点(X,Y)
                *ifs >> db;
                Header.m_DpMemoriKijunTen.x = db;
                //
                *ifs >> db;
                Header.m_DpMemoriKijunTen.y = db;
                //レイヤ名
                for( i=0; i < 16; i++)
                    for( int j = 0; j < 16; j++ )
                        Header.m_aStrLayName[i][j] = ReadString();
                //レイヤグループ名
                for( i=0; i < 16; i++ )
                    Header.m_aStrGLayName[i] = ReadString();
                //日影計算の条件 測定面高さ
                *ifs >> db;
                Header.m_dKageLevel = db;
                //緯度
                *ifs >> db;
                Header.m_dKageIdo = db;
                //9縲・5の測定の指定
                *ifs >> dw;
                Header.m_nKage9_15Flg = dw;
                //壁面日影測定面高さ
                *ifs >> db;
                Header.m_dKabeKageLevel = db;
                //天空図の条件（Ver.3.00以降)
                if(Header.JW_DATA_VERSION >= 300)
                {
                    //測定面高さ
                    *ifs >> db;
                    Header.m_dTenkuuZuLevel = db;
                    //天空図の半径＊２
                    *ifs >> db;
                    Header.m_dTenkuuZuEnkoR = db;
                }
                //2.5Dの計算単位(0以外はmm単位で計算)
                *ifs >> dw;
                Header.m_nMMTani3D = dw;

                //保存時の画面倍率(読込むと前画面倍率になる)
                *ifs >> db;
                Header.m_dBairitsu = db;
                //
                *ifs >> db;
                Header.m_DPGenten.x = db;
                //
                *ifs >> db;
                Header.m_DPGenten.y = db;

                //範囲記憶倍率と基準点(X,Y)
                *ifs >> db;
                Header.m_dHanniBairitsu = db;
                //
                *ifs >> db;
                Header.m_DPHanniGenten.x = db;
                //
                *ifs >> db;
                Header.m_DPHanniGenten.y = db;

                //マークジャンプ倍率、基準点(X,Y)およびレイヤグループ
                if(Header.JW_DATA_VERSION >= 300)
                {
                    //ズーム拡張(v300)
                    for(int n=1; n<=8; n++){
                        *ifs >> db;
                        Header.m_dZoom[n].m_dZoomJumpBairitsu = db;
                        *ifs >> db;
                        Header.m_dZoom[n].m_DPZoomJumpGenten.x = db;
                        *ifs >> db;
                        Header.m_dZoom[n].m_DPZoomJumpGenten.y = db;
                        *ifs >> dw;
                        Header.m_dZoom[n].m_nZoomJumpGLay = dw;
                    }
                } else
                {
                    for( i=1; i <= 4; i++ )
                    {
                        *ifs >> db;
                        Header.m_dZoom[i].m_dZoomJumpBairitsu = db;
                        *ifs >> db;
                        Header.m_dZoom[i].m_DPZoomJumpGenten.x = db;
                        *ifs >> db;
                        Header.m_dZoom[i].m_DPZoomJumpGenten.y = db;
                    }
                }
                //ダミー
                if( Header.JW_DATA_VERSION >= 300 ){   //Ver.3.00以降
                    *ifs >> db;
                    Header.dDm11 = db;
                    *ifs >> db;
                    Header.dDm12 = db;
                    *ifs >> db;
                    Header.dDm13 = db;
                    *ifs >> dw;
                    Header.lnDm1 = dw;
                    *ifs >> db;
                    Header.dDm21 = db;
                    *ifs >> db;
                    Header.dDm22 = db;
                    //(Ver.4.04以前はダミー）文字列範囲を背景色で描画するときの範囲増寸法
                    *ifs >> db;
                    Header.m_dMojiBG = db;
                    //(Ver.4.04以前はダミー）
                    //十位:文字（寸法図形、ブロック図形）を最後に描画
                    //一位:1 :輪郭・範囲を背景色で描画しない
                    //     2 :文字の輪郭を背景色で描画する
                    //     3 :文字列範囲を背景色で描画する
                    *ifs >> dw;
                    Header.m_nMojiBG = dw;
                }
                //複線間隔
                for( i=0; i <= 9; i++ ){
                    *ifs >> db;
                    Header.m_adFukusenSuuchi[i] = db;
                }
                //両側複線の留線出の寸法
                *ifs >> db;
                Header.m_dRyoygawaFukusenTomeDe = db;
                //色番号ごとの画面表示色、線幅
                for( i=0; i <= 9; i++ ){
                    *ifs >> dw;
                    Header.m_Pen[i].m_m_aPenColor = dw;
                    *ifs >> dw;
                    Header.m_Pen[i].m_anPenWidth = dw;
                }
                //色番号ごとのプリンタ出力色、線幅、実点半径
                for( i=0; i <= 9; i++ ){
                    *ifs >> dw;
                    Header.m_PrtPen[i].m_aPrtpenColor = dw;
                    *ifs >> dw;
                    Header.m_PrtPen[i].m_anPrtPenWidth = dw;
                    *ifs >> db;
                    Header.m_PrtPen[i].m_adPrtTenHankei = db;
                }
                //線種番号2から9までのパターン、1ユニットのドット数、ピッチ、プリンタ出力ピッチ
                for( i=2; i <= 9; i++ ){
                    *ifs >> dw;
                    Header.m_alLType1[i].m_alLtype = dw;
                    *ifs >> dw;
                    Header.m_alLType1[i].m_anTokushusSenUnitDot = dw;
                    *ifs >> dw;
                    Header.m_alLType1[i].m_anTokushuSenPich = dw;
                    *ifs >> dw;
                    Header.m_alLType1[i].m_anPrtTokushuSenPich = dw;
                }
                //ランダム線1から5までのパターン、画面表示振幅・ピッチ、プリンタ出力振幅・ピッチ
                for( i=11; i <= 15; i++ ){
                    *ifs >> dw;
                    Header.m_alLType2[i].m_alLtype = dw;
                    *ifs >> dw;
                    Header.m_alLType2[i].m_anRandSenWide = dw;
                    *ifs >> dw;
                    Header.m_alLType2[i].m_anTokushuSenPich = dw;
                    *ifs >> dw;
                    Header.m_alLType2[i].m_anPrtRandSenWide = dw;
                    *ifs >> dw;
                    Header.m_alLType2[i].m_anPrtTokushuSenPich = dw;
                }
                //倍長線種番号6から9までのパターン、1ユニットのドット数、ピッチ、プリンタ出力ピッチ
                for( i=16; i <= 19; i++ ){
                    *ifs >> dw;
                    Header.m_alLType3[i].m_alLtype = dw;
                    *ifs >> dw;
                    Header.m_alLType3[i].m_anTokushusSenUnitDot = dw;
                    *ifs >> dw;
                    Header.m_alLType3[i].m_anTokushuSenPich = dw;
                    *ifs >> dw;
                    Header.m_alLType3[i].m_anPrtTokushuSenPich = dw;
                }
                //実点を画面描画時の指定半径で描画
                *ifs >> dw;
                Header.m_nDrawGamenTen = dw;
                //実点をプリンタ出力時、指定半径で書く
                *ifs >> dw;
                Header.m_nDrawPrtTen = dw;
                //BitMap・ソリッドを最初に描画する
                *ifs >> dw;
                Header.m_nBitMapFirstDraw = dw;
                //逆描画
                *ifs >> dw;
                Header.m_nGyakuDraw = dw;
                //逆サーチ
                *ifs >> dw;
                Header.m_nGyakuSearch = dw;
                //カラー印刷
                *ifs >> dw;
                Header.m_nColorPrint = dw;
                //レイヤ順の印刷
                *ifs >> dw;
                Header.m_nLayJunPrint = dw;
                //色番号順の印刷
                *ifs >> dw;
                Header.m_nColJunPrint = dw;
                //レイヤグループまたはレイヤごとのプリンタ連続出力指定
                *ifs >> dw;
                Header.m_nPrtRenzoku = dw;
                //プリンタ共通レイヤ(表示のみレイヤ)のグレー出力指定
                *ifs >> dw;
                Header.m_nPrtKyoutuuGray = dw;
                //プリンタ出力時に表示のみレイヤは出力しない
                *ifs >> dw;
                Header.m_nPrtDispOnlyNonDraw = dw;
                //作図時間（Ver.2.23以降）
                *ifs >> dw;
                Header.m_lnDrawTime = dw;
                //2.5Dの始点位置が設定されている時のフラグ（Ver.2.23以降）
                *ifs >> dw;
                Header.nEyeInit = dw;
                //2.5Dの透視図・鳥瞰図・アイソメ図の視点水平角（Ver.2.23以降）
                *ifs >> dw;
                Header.m_dEye_H_Ichi_1 = dw;
                //
                *ifs >> dw;
                Header.m_dEye_H_Ichi_2 = dw;
                //
                *ifs >> dw;
                Header.m_dEye_H_Ichi_3 = dw;
                //2.5Dの透視図の視点高さ・視点離れ（Ver.2.23以降）
                *ifs >> db;
                Header.m_dEye_Z_Ichi_1 = db;
                //
                *ifs >> db;
                Header.m_dEye_Y_Ichi_1 = db;
                //2.5Dの鳥瞰図の視点高さ・視点離れ（Ver.2.23以降）
                *ifs >> db;
                Header.m_dEye_Z_Ichi_2 = db;
                //
                *ifs >> db;
                Header.m_dEye_Y_Ichi_2 = db;
                //2.5Dのアイソメ図の視点垂直角（Ver.2.23以降）
                *ifs >> db;
                Header.m_dEye_V_Ichi_3 = db;
                //線の長さ指定の最終値（Ver.2.25以降）
                *ifs >> db;
                Header.m_dSenNagasaSunpou = db;
                //矩形寸法横寸法・縦寸法指定の最終値（Ver.2.25以降）
                *ifs >> db;
                Header.m_dBoxSunpouX = db;
                //
                *ifs >> db;
                Header.m_dBoxSunpouY = db;
                //円の半径指定の最終値（Ver.2.25以降）
                *ifs >> db;
                Header.m_dEnHankeySunpou = db;
                //ソリッドを任意色で書くフラグ、ソリッドの任意色の既定値（Ver.2.30以降）
                *ifs >> dw;
                Header.m_nSolidNinniColor = dw;
                //RGB
                *ifs >> dw;
                Header.m_SolidColor = dw;
                //SXF対応拡張線色定義（Ver.4.20以降）
                if(Header.JW_DATA_VERSION >= 420){
                    int n1;
                    for( int n=0; n <= 256; n++ ){ //////   画面表示色
                        n1 = n + SXCOL_EXT;   //色番号のオフセット = +100
                        *ifs >> dw;
                        Header.m_SxfCol.m_aPenColor[n1] = dw;
                        *ifs >> dw;
                        Header.m_SxfCol.m_anPenWidth[n1] = dw;
                    }
                    for( int n=0; n <= 256; n++ ){ //////   プリンタ出力色
                        Header.m_SxfCol.m_astrUDColorName[n] = ReadString();
                        n1 = n + SXCOL_EXT;   //色番号のオフセット = +100
                        *ifs >> dw;
                        Header.m_SxfCol.m_aPrtPenColor[n1] = dw;
                        *ifs >> dw;
                        Header.m_SxfCol.m_anPrtPenWidth[n1] = dw;
                        *ifs >> db;
                        Header.m_SxfCol.m_adPrtTenHankei[n1] = db;
                    }
                }
                //SXF対応拡張線種定義（Ver.4.20以降）
                if(Header.JW_DATA_VERSION >= 420){
                    int n1;
                    for( int n=0; n<=32; n++){   //////  線種パターン
                        n1 = n + SXLTP_EXT;   //線種番号のオフセット = +30
                        *ifs >> dw;
                        Header.m_SxfLtp.m_alLType[n1] = dw;
                        *ifs >> dw;
                        Header.m_SxfLtp.m_anTokushuSenUintDot[n1] = dw;
                        *ifs >> dw;
                        Header.m_SxfLtp.m_anTokushuSenPich[n1] = dw;
                        *ifs >> dw;
                        Header.m_SxfLtp.m_anPrtTokushuSenPich[n1] = dw;
                    }
                    for( int n=0; n<=32; n++){   //////  線種パラメータ
                        Header.m_SxfLtp.m_astrUDLTypeName[n] = ReadString();
                        *ifs >> dw;
                        Header.m_SxfLtp.m_anUDLTypeSegment[n] = dw;
                        for( int j=1; j<=10; j++){
                            *ifs >> db;
                            Header.m_SxfLtp.m_aadUDLTypePitch[n][j] = db;
                        }
                    }
                }
                //文字種1から10までの文字幅、高さ、間隔、色番号
                for(i=1; i<=10;i++){
                    *ifs >> db;
                    Header.m_Moji[i].m_adMojiX = db;
                    *ifs >> db;
                    Header.m_Moji[i].m_adMojiY = db;
                    *ifs >> db;
                    Header.m_Moji[i].m_adMojiD = db;
                    *ifs >> dw;
                    Header.m_Moji[i].m_anMojiCol = dw;
                }
                //書込み文字の文字幅
                *ifs >> db;
                Header.m_dMojiSizeX = db;
                //高さ
                *ifs >> db;
                Header.m_dMojiSizeY = db;
                //間隔
                *ifs >> db;
                Header.m_dMojiKankaku = db;
                //色番号
                *ifs >> dw;
                Header.m_nMojiColor = dw;
                //文字番号
                *ifs >> dw;
                Header.m_nMojiShu = dw;
                //文字位置整理の行間
                *ifs >> db;
                Header.m_dMojiSeiriGyouKan = db;
                //文字数
                *ifs >> db;
                Header.m_dMojiSeiriSuu = db;
                //文字基準点のずれ位置使用のフラグ
                *ifs >> dw;
                Header.m_nMojiKijunZureOn = dw;
                //文字基準点の横方向のずれ位置左
                *ifs >> db;
                Header.m_adMojiKijunZureX[0] = db;
                //中
                *ifs >> db;
                Header.m_adMojiKijunZureX[1] = db;
                //右
                *ifs >> db;
                Header.m_adMojiKijunZureX[2] = db;
                //文字基準点の縦方向のずれ位置下
                *ifs >> db;
                Header.m_adMojiKijunZureY[0] = db;
                //中
                *ifs >> db;
                Header.m_adMojiKijunZureY[1] = db;
                //上
                *ifs >> db;
                Header.m_adMojiKijunZureY[2] = db;
            }
        }else
            return false;
    }else
        return false;
    return true;
}

//ヘッダー部書き出し
BOOL JWWDocument::WriteHeader()
{
    DWORD dw;
//    WORD wd;
    DOUBLE db;
    string s;

    //JWWのデータファイルの宣言
    ofs->write("JwwData.",8);
    //バージョンNo.
    dw=Header.JW_DATA_VERSION;
    *ofs << dw;
    //ファイルメモ
    WriteString(Header.m_strMemo);
    //図面サイズ
    dw=Header.m_nZumen;
    *ofs << dw;
    //レイヤグループ・レイヤ状態
    dw=Header.m_nWriteGLay;
    *ofs << dw;
    for(int i = 0; i < 16; i++ ){
        dw = Header.GLay[i].m_anGLay;
        *ofs << dw;
        dw = Header.GLay[i].m_anWriteLay;
        *ofs << dw;
        db = Header.GLay[i].m_adScale;
        *ofs << db;
        dw = Header.GLay[i].m_anGLayProtect;
        *ofs << dw;
        for(int nLay=0;nLay<16;nLay++){
            dw = Header.GLay[i].m_nLay[nLay].m_aanLay;
            *ofs << dw;
            dw = Header.GLay[i].m_nLay[nLay].m_aanLayProtect;
            *ofs << dw;
        }
    }
    //ダミー
    for(int i = 0; i < 14; i++ )
    {
        dw=Header.Dummy[i];
        *ofs << dw;
    }
    //寸法関係の設定
    dw = Header.m_lnSunpou1;
    *ofs << dw;
    dw = Header.m_lnSunpou2;
    *ofs << dw;
    dw = Header.m_lnSunpou3;
    *ofs << dw;
    dw = Header.m_lnSunpou4;
    *ofs << dw;
    dw = Header.m_lnSunpou5;
    *ofs << dw;
    //ダミー
    dw = Header.Dummy1;
    *ofs << dw;
    //線描画の最大幅
    dw = Header.m_nMaxDrawWid;
    *ofs << dw;
    //プリンタ出力範囲の原点(X,Y)
    db = Header.m_DPPrtGenten.x;
    *ofs << db;
    //
    db = Header.m_DPPrtGenten.y;
    *ofs << db;
    //プリンタ出力倍率
    db = Header.m_dPrtBairitsu;
    *ofs << db;
    //プリンタ90°回転出力、プリンタ出力基準点位置
    dw = Header.m_nPrt90Kaiten;
    *ofs << dw;
    //目盛設定モード
    dw = Header.m_nMemoriMode;
    *ofs << dw;
    //目盛表示最小間隔ドット
    db = Header.m_dMemoriHyoujiMin;
    *ofs << db;
    //目盛表示間隔(X,Y)
    db = Header.m_dMemoriX;
    *ofs << db;
    //
    db = Header.m_dMemoriY;
    *ofs << db;
    //目盛基準点(X,Y)
    db = Header.m_DpMemoriKijunTen.x;
    *ofs << db;
    //
    db = Header.m_DpMemoriKijunTen.y;
    *ofs << db;
    //レイヤ名
    for(int i = 0; i < 16; i++ )
        for( int j = 0; j < 16; j++ )
            WriteString(Header.m_aStrLayName[i][j]);
    //レイヤグループ名
    for(int i = 0; i < 16; i++ )
        WriteString(Header.m_aStrGLayName[i]);
    //日影計算の条件 測定面高さ
    db = Header.m_dKageLevel;
    *ofs << db;
    //緯度
    db = Header.m_dKageIdo;
    *ofs << db;
    //9縲・5の測定の指定
    dw = Header.m_nKage9_15Flg;
    *ofs << dw;
    //壁面日影測定面高さ
    db = Header.m_dKabeKageLevel;
    *ofs << db;
    //天空図の条件（Ver.3.00以降) 測定面高さ
    if(Header.JW_DATA_VERSION >= 300)
    {
        db = Header.m_dTenkuuZuLevel;
        *ofs << db;
        //天空図の半径＊２
        db = Header.m_dTenkuuZuEnkoR;
        *ofs << db;

    }
    //2.5Dの計算単位(0以外はmm単位で計算)
    dw = Header.m_nMMTani3D;
    *ofs << dw;
    //保存時の画面倍率(読込むと前画面倍率になる)
    db = Header.m_dBairitsu;
    *ofs << db;
    //
    db = Header.m_DPGenten.x;
    *ofs << db;
    //
    db = Header.m_DPGenten.y;
    *ofs << db;
    //範囲記憶倍率と基準点(X,Y)
    db = Header.m_dHanniBairitsu;
    *ofs << db;
    //
    db = Header.m_DPHanniGenten.x;
    *ofs << db;
    //
    db = Header.m_DPHanniGenten.y;
    *ofs << db;
    //マークジャンプ倍率、基準点(X,Y)およびレイヤグループ
    if( Header.JW_DATA_VERSION >= 300 ){   //Ver.3.00以降
        for( int i=1; i<=8; i++ ){
            db = Header.m_dZoom[i].m_dZoomJumpBairitsu;
            *ofs << db;
            db = Header.m_dZoom[i].m_DPZoomJumpGenten.x;
            *ofs << db;
            db = Header.m_dZoom[i].m_DPZoomJumpGenten.y;
            *ofs << db;
            dw = Header.m_dZoom[i].m_nZoomJumpGLay;
            *ofs << dw;
        }
    }else{
        for( int i=1; i<=4; i++){
            db = Header.m_dZoom[i].m_dZoomJumpBairitsu;
            *ofs << db;
            db = Header.m_dZoom[i].m_DPZoomJumpGenten.x;
            *ofs << db;
            db = Header.m_dZoom[i].m_DPZoomJumpGenten.y;
            *ofs << db;
        }
    }
    //文字の描画状態(Ver.4.05以降）
    if( Header.JW_DATA_VERSION >= 300 ){   //Ver.3.00以降
        db = Header.dDm11;//ダミー
        *ofs << db;
        db = Header.dDm12;//ダミー
        *ofs << db;
        db = Header.dDm13;//ダミー
        *ofs << db;
        dw = Header.lnDm1;//ダミー
        *ofs << dw;
        db = Header.dDm21;//ダミー
        *ofs << db;
        db = Header.dDm22;//ダミー
        *ofs << db;
        db = Header.m_dMojiBG;//(Ver.4.04以前はダミー）
        *ofs << db;	//文字列範囲を背景色で描画するときの範囲増寸法
        dw = Header.m_nMojiBG;
        *ofs << dw;
    }
    //複線間隔
    for(int i = 0; i <= 9; i++ )
    {
        db = Header.m_adFukusenSuuchi[i];
        *ofs << db;
    }
    //両側複線の留線出の寸法
    db = Header.m_dRyoygawaFukusenTomeDe;
    *ofs << db;
    //色番号ごとの画面表示色、線幅
    for(int i = 0; i <= 9; i++ )
    {
        dw = Header.m_Pen[i].m_m_aPenColor;
        *ofs << dw;
        dw = Header.m_Pen[i].m_anPenWidth;
        *ofs << dw;
    }
    //色番号ごとのプリンタ出力色、線幅、実点半径
    for(int i = 0; i <= 9; i++ )
    {
        dw = Header.m_PrtPen[i].m_aPrtpenColor;
        *ofs << dw;
        dw = Header.m_PrtPen[i].m_anPrtPenWidth;
        *ofs << dw;
        db = Header.m_PrtPen[i].m_adPrtTenHankei;
        *ofs << db;
    }
    //線種番号2から9までのパターン、1ユニットのドット数、ピッチ、プリンタ出力ピッチ
    for(int i = 2; i <= 9; i++ )
    {
        dw = Header.m_alLType1[i].m_alLtype;
        *ofs << dw;
        dw = Header.m_alLType1[i].m_anTokushusSenUnitDot;
        *ofs << dw;
        dw = Header.m_alLType1[i].m_anTokushuSenPich;
        *ofs << dw;
        dw = Header.m_alLType1[i].m_anPrtTokushuSenPich;
        *ofs << dw;
    }
    //ランダム線1から5までのパターン、画面表示振幅・ピッチ、プリンタ出力振幅・ピッチ
    for(int i = 11; i <= 15; i++ )
    {
        dw = Header.m_alLType2[i].m_alLtype;
        *ofs << dw;
        dw = Header.m_alLType2[i].m_anRandSenWide;
        *ofs << dw;
        dw = Header.m_alLType2[i].m_anTokushuSenPich;
        *ofs << dw;
        dw = Header.m_alLType2[i].m_anPrtRandSenWide;
        *ofs << dw;
        dw = Header.m_alLType2[i].m_anPrtTokushuSenPich;
        *ofs << dw;
    }
    //倍長線種番号6から9までのパターン、1ユニットのドット数、ピッチ、プリンタ出力ピッチ
    for(int i = 16; i <= 19; i++ )
    {
        dw = Header.m_alLType3[i].m_alLtype;
        *ofs << dw;
        dw = Header.m_alLType3[i].m_anTokushusSenUnitDot;
        *ofs << dw;
        dw = Header.m_alLType3[i].m_anTokushuSenPich;
        *ofs << dw;
        dw = Header.m_alLType3[i].m_anPrtTokushuSenPich;
        *ofs << dw;
    }
    //実点を画面描画時の指定半径で描画
    dw = Header.m_nDrawGamenTen;
    *ofs << dw;
    //実点をプリンタ出力時、指定半径で書く
    dw = Header.m_nDrawPrtTen;
    *ofs << dw;
    //BitMap・ソリッドを最初に描画する
    dw = Header.m_nBitMapFirstDraw;
    *ofs << dw;
    //逆描画
    dw = Header.m_nGyakuDraw;
    *ofs << dw;
    //逆サーチ
    dw = Header.m_nGyakuSearch;
    *ofs << dw;
    //カラー印刷
    dw = Header.m_nColorPrint;
    *ofs << dw;
    //レイヤ順の印刷
    dw = Header.m_nLayJunPrint;
    *ofs << dw;
    //色番号順の印刷
    dw = Header.m_nColJunPrint;
    *ofs << dw;
    //レイヤグループまたはレイヤごとのプリンタ連続出力指定
    dw = Header.m_nPrtRenzoku;
    *ofs << dw;
    //プリンタ共通レイヤ(表示のみレイヤ)のグレー出力指定
    dw = Header.m_nPrtKyoutuuGray;
    *ofs << dw;
    //プリンタ出力時に表示のみレイヤは出力しない
    dw = Header.m_nPrtDispOnlyNonDraw;
    *ofs << dw;
    //作図時間（Ver.2.23以降）
    dw = Header.m_lnDrawTime;
    *ofs << dw;
    //2.5Dの始点位置が設定されている時のフラグ（Ver.2.23以降）
    dw = Header.nEyeInit;
    *ofs << dw;
    //2.5Dの透視図・鳥瞰図・アイソメ図の視点水平角（Ver.2.23以降）
    dw = Header.m_dEye_H_Ichi_1;
    *ofs << dw;
    //
    dw = Header.m_dEye_H_Ichi_2;
    *ofs << dw;
    //
    dw = Header.m_dEye_H_Ichi_3;
    *ofs << dw;
    //2.5Dの透視図の視点高さ・視点離れ（Ver.2.23以降）
    db = Header.m_dEye_Z_Ichi_1;
    *ofs << db;
    //
    db = Header.m_dEye_Y_Ichi_1;
    *ofs << db;
    //2.5Dの鳥瞰図の視点高さ・視点離れ（Ver.2.23以降）
    db = Header.m_dEye_Z_Ichi_2;
    *ofs << db;
    //
    db = Header.m_dEye_Y_Ichi_2;
    *ofs << db;
    //2.5Dのアイソメ図の視点垂直角（Ver.2.23以降）
    db = Header.m_dEye_V_Ichi_3;
    *ofs << db;
    //線の長さ指定の最終値（Ver.2.25以降）
    db = Header.m_dSenNagasaSunpou;
    *ofs << db;
    //矩形寸法横寸法・縦寸法指定の最終値（Ver.2.25以降）
    db = Header.m_dBoxSunpouX;
    *ofs << db;
    //
    db = Header.m_dBoxSunpouY;
    *ofs << db;
    //円の半径指定の最終値（Ver.2.25以降）
    db = Header.m_dEnHankeySunpou;
    *ofs << db;
    //ソリッドを任意色で書くフラグ、ソリッドの任意色の既定値（Ver.2.30以降）
    dw = Header.m_nSolidNinniColor;
    *ofs << dw;
    //RGB
    dw = Header.m_SolidColor;
    *ofs << dw;
    //SXF対応拡張線色定義（Ver.4.20以降）
    if( Header.JW_DATA_VERSION >= 420 ){
        int n1;
        for( int n=0; n<=256; n++){ //   画面表示色
            n1 = n + SXCOL_EXT;   //色番号のオフセット = +100
            dw = Header.m_SxfCol.m_aPenColor[n1];
            *ofs << dw;
            dw = Header.m_SxfCol.m_anPenWidth[n1];
            *ofs << dw;
        }
        for( int n=0; n<=256; n++){ //   プリンタ出力色
            WriteString(Header.m_SxfCol.m_astrUDColorName[n]);
            n1 = n + SXCOL_EXT;   //色番号のオフセット = +100
            dw = Header.m_SxfCol.m_aPrtPenColor[n1];
            *ofs << dw;
            dw = Header.m_SxfCol.m_anPrtPenWidth[n1];
            *ofs << dw;
            db = Header.m_SxfCol.m_adPrtTenHankei[n1];
            *ofs << db;
        }
    }
    //SXF対応拡張線種定義（Ver.4.20以降）
    if( Header.JW_DATA_VERSION >= 420 ){
        int n1;
        for( int n=0; n<=32; n++){   //  線種パターン
            n1 = n + SXLTP_EXT;   //線種番号のオフセット = +30
            dw = Header.m_SxfLtp.m_alLType[n1];
            *ofs << dw;
            dw = Header.m_SxfLtp.m_anTokushuSenUintDot[n1];
            *ofs << dw;
            dw = Header.m_SxfLtp.m_anTokushuSenPich[n1];
            *ofs << dw;
            dw = Header.m_SxfLtp.m_anPrtTokushuSenPich[n1];
            *ofs << dw;
        }
        for( int n=0; n<=32; n++){   //  線種パラメータ
            WriteString(Header.m_SxfLtp.m_astrUDLTypeName[n]);
            dw = Header.m_SxfLtp.m_anUDLTypeSegment[n];
            *ofs << dw;
            for(int j=1; j<=10; j++){
                db = Header.m_SxfLtp.m_aadUDLTypePitch[n][j];
                *ofs << db;
            }
        }
    }
    //文字種1から10までの文字幅、高さ、間隔、色番号
    for(int i = 1; i <= 10; i++ )
    {
        db = Header.m_Moji[i].m_adMojiX;
        *ofs << db;
        db = Header.m_Moji[i].m_adMojiY;
        *ofs << db;
        db = Header.m_Moji[i].m_adMojiD;
        *ofs << db;
        dw = Header.m_Moji[i].m_anMojiCol;
        *ofs << dw;
    }
    //書込み文字の文字幅、高さ、間隔、色番号、文字番号
    db = Header.m_dMojiSizeX;
    *ofs << db;
    //
    db = Header.m_dMojiSizeY;
    *ofs << db;
    //
    db = Header.m_dMojiKankaku;
    *ofs << db;
    //
    dw = Header.m_nMojiColor;
    *ofs << dw;
    //
    dw = Header.m_nMojiShu;
    *ofs << dw;
    //文字位置整理の行間、文字数
    db = Header.m_dMojiSeiriGyouKan;
    *ofs << db;
    //
    db = Header.m_dMojiSeiriSuu;
    *ofs << db;
    //文字基準点のずれ位置使用のフラグ
    dw = Header.m_nMojiKijunZureOn;
    *ofs << dw;
    //文字基準点の横方向のずれ位置左、中、右
    db = Header.m_adMojiKijunZureX[0];
    *ofs << db;
    //
    db = Header.m_adMojiKijunZureX[1];
    *ofs << db;
    //
    db = Header.m_adMojiKijunZureX[2];
    *ofs << db;
    //文字基準点の縦方向のずれ位置下、中、上
    db = Header.m_adMojiKijunZureY[0];
    *ofs << db;
    //
    db = Header.m_adMojiKijunZureY[1];
    *ofs << db;
    //
    db = Header.m_adMojiKijunZureY[2];
    *ofs << db;

    return true;
}

//データファイル読み込み
BOOL JWWDocument::Read()
{
    if(!ifs)
        return false;

    DWORD dw;
//    DOUBLE db;
    string s, t;
    WORD wd;
//    BYTE bt;
    int	i,j/*,k*/;

    BOOL ListFlag;
    int ListCount;
    int ListLength;
    CDataSen	DSen;
    CDataEnko	DEnko;
    CDataTen	DTen;
    CDataMoji	DMoji;
    CDataSolid	DSolid;
    CDataSunpou	DSunpou;
    CDataBlock	DBlock;
    CDataList	DList;

    pBlockList->Init();
    ListFlag = false;
    ListLength = 0;
    ListCount = 0;
    if(!ReadHeader())
        return false;
    SenCount = 0;
    EnkoCount = 0;
    TenCount = 0;
    MojiCount = 0;
    SolidCount = 0;
    BlockCount = 0;
    SunpouCount = 0;

    //バージョン毎にデータ読み書きを変える
    DSen.SetVersion(Header.JW_DATA_VERSION);
    DEnko.SetVersion(Header.JW_DATA_VERSION);
    DTen.SetVersion(Header.JW_DATA_VERSION);
    DMoji.SetVersion(Header.JW_DATA_VERSION);
    DSolid.SetVersion(Header.JW_DATA_VERSION);
    DSunpou.SetVersion(Header.JW_DATA_VERSION);
    DBlock.SetVersion(Header.JW_DATA_VERSION);

    *ifs >> wd;
    if( wd == 0xFFFF )
    {
        *ifs >> dw;
//        j = dw;
	}
//	else j = wd;

    i = 1;

    while( !ifs->eof() )
    {
        *ifs >> wd;
        switch(wd){
        case	0x0000:
            continue;//goto exitloop;
            break;
        case	0xFFFF:
            {
                *ifs >> wd;
                objCode = wd;
                *ifs >> wd;
                s = ReadData(wd);
                pList->AddItem(i,s);
                j = i;
                i++;
            }
            break;
        case	0xFF7F:
             {
                *ifs >> dw;
                j = dw & 0x7FFFFFFF;
            }
            break;
        case	0x7FFF:
            {
                *ifs >> dw;
                j = dw & 0x7FFFFFFF;
            }
            break;
        default:
            {
                if(wd & 0x8000)
                    j = wd & 0x7FFF;
                else
                    j = 0;
            }
        }
        s = pList->GetNoByItem(j).CDataString;
#ifdef	DATA_DUMP
cout << s << endl;
#endif
        if( ListCount == ListLength )
            ListFlag = false;
        if(s == "CDataList")
        {
            ListFlag = true;
            ListCount = 0;
            DList.Serialize(*ifs);
#ifdef	DATA_DUMP
cout << DList;
#endif
            pBlockList->AddBlockList(DList);
            ListLength = DList.Count;
        }
        if( s == "CDataSen" )
        {
            DSen.Serialize(*ifs);
#ifdef	DATA_DUMP
cout << DSen;
#endif
            if( ListFlag )
            {
                pBlockList->AddDataListSen(DSen);
                ListCount++;
            } else
            {
                vSen.push_back(DSen);
                SenCount++;
            }
        }
        if( s == "CDataEnko")
        {
            DEnko.Serialize(*ifs);
#ifdef	DATA_DUMP
cout << DEnko;
#endif
            if( ListFlag )
            {
                pBlockList->AddDataListEnko(DEnko);
                ListCount++;
            }
            else
            {
                vEnko.push_back(DEnko);
                EnkoCount++;
            }
        }
        if( s == "CDataTen" )
        {
            DTen.Serialize(*ifs);
#ifdef	DATA_DUMP
cout << DTen;
#endif
            if( ListFlag )
            {
                pBlockList->AddDataListTen(DTen);
                ListCount++;
            } else
            {
                vTen.push_back(DTen);
                TenCount++;
            }
        }
        if( s == "CDataMoji" )
        {
            DMoji.Serialize(*ifs);
#ifdef	DATA_DUMP
cout << DMoji;
#endif
            if( ListFlag )
            {
                pBlockList->AddDataListMoji(DMoji);
                ListCount++;
            } else
            {
                vMoji.push_back(DMoji);
                MojiCount++;
            }
        }
        if( s == "CDataSolid" )
        {
            DSolid.Serialize(*ifs);
#ifdef	DATA_DUMP
cout << DSolid;
#endif
            if( ListFlag )
            {
                pBlockList->AddDataListSolid(DSolid);
                ListCount++;
            } else
            {
                vSolid.push_back(DSolid);
                SolidCount++;
            }
        }
        if( s == "CDataBlock" )
        {
            DBlock.Serialize(*ifs);
#ifdef	DATA_DUMP
cout << DBlock;
#endif
            if(ListFlag)
            {
                pBlockList->AddDataListBlock(DBlock);
                ListCount++;
            } else
            {
                vBlock.push_back(DBlock);
                BlockCount++;
            }
        }
        if( s == "CDataSunpou" )
        {
            DSunpou.Serialize(*ifs);
#ifdef	DATA_DUMP
cout << DSunpou;
#endif
            if( ListFlag )
            {
                pBlockList->AddDataListSunpou(DSunpou);
                ListCount++;
            } else
            {
                vSunpou.push_back(DSunpou);
                SunpouCount++;
            }
        }
        if( !s.empty() )
            i++;
        s = "";
    }
//exitloop:
    return true;
}

BOOL JWWDocument::SaveBich16(DWORD id)
{
    DWORD i=((id*2) | 0x0000ffff) >> 16;
    if( i==0 )
        return true;
    return false;
}

//線
BOOL JWWDocument::SaveSen(CDataSen const& DSen)
{
    WORD wd;
//    DWORD dw;
    string s;
    if( SaveSenCount == 0 )
    {
        PSen=Mpoint;
        Mpoint++;
        wd = 0xFFFF;
        *ofs << wd;
        *ofs << objCode;
        s="CDataSen";
        wd=s.length();
        *ofs << wd;
        ofs->write(s.c_str(), wd);
    }else
    {
        wd=PSen | 0x8000;
        *ofs << wd;
    }
    DSen.Serialize(*ofs);
    SaveSenCount++;
    Mpoint++;
    return true;
}

// 円
BOOL JWWDocument::SaveEnko(CDataEnko const& DEnko)
{
    WORD wd;
    DWORD dw;
    string s;
    if( SaveEnkoCount == 0 )
    {
        PEnko=Mpoint;
        Mpoint++;
        wd=0xFFFF;
        *ofs << wd;
        *ofs << objCode;
        s="CDataEnko";
        wd=s.length();
        *ofs << wd;
        ofs->write(s.c_str(), wd);
    }else
    {
        if( SaveBich16(PEnko) )
        {
            wd=PEnko | 0x8000;
            *ofs << wd;
        }
        else
        {
            wd= 0x7FFF;
            dw=PEnko | 0x80000000;
            *ofs << wd;
            *ofs << dw;
        }
    }
    DEnko.Serialize(*ofs);
    SaveEnkoCount++;
    Mpoint++;
    return true;
}

// 点
BOOL JWWDocument::SaveTen(CDataTen const& DTen)
{
    WORD wd;
    DWORD dw;
    string s;

    if( SaveTenCount == 0 )
    {
        PTen=Mpoint;
        Mpoint++;
        wd=0xFFFF;
        *ofs << wd;
        *ofs << objCode;
        s="CDataTen";
        wd=s.length();
        *ofs << wd;
        ofs->write(s.c_str(), wd);
    }else
    {
        if( SaveBich16(PTen) )
        {
            wd=PTen | 0x8000;
            *ofs << wd;
        }
        else
        {
            wd= 0x7FFF;
            *ofs << wd;
            dw=PTen | 0x80000000;
            *ofs << dw;
        }
    }
    DTen.Serialize(*ofs);
    SaveTenCount++;
    Mpoint++;
    return true;

}

// 文字
BOOL JWWDocument::SaveMoji(CDataMoji const& DMoji)
{
    WORD wd;
    DWORD dw;
    string s;

    if( SaveMojiCount == 0 )
    {
        PMoji=Mpoint;
        Mpoint++;
        wd=0xFFFF;
        *ofs << wd;
        *ofs << objCode;
        s="CDataMoji";
        wd=s.length();
        *ofs << wd;
        ofs->write(s.c_str(), wd);
    }else
    {
        if( SaveBich16(PMoji) )
        {
            wd=PMoji | 0x8000;
            *ofs << wd;
        }
        else
        {
            wd= 0x7FFF;
            dw=PMoji | 0x80000000;
            *ofs << wd;
            *ofs << dw;
        }
    }
    DMoji.Serialize(*ofs);
    SaveMojiCount++;
    Mpoint++;
    return true;
}

// 寸法
BOOL JWWDocument::SaveSunpou(CDataSunpou const& DSunpou)
{
    WORD wd;
    DWORD dw;
    string s;

    if( SaveSunpouCount == 0 )
    {
        PSunpou=Mpoint;
        Mpoint++;
        wd=0xFFFF;
        *ofs << wd;
        *ofs << objCode;
        s="CDataSunpou";
        wd=s.length();
        *ofs << wd;
        ofs->write(s.c_str(), wd);
    } else
    {
        if( SaveBich16(PSunpou) )
        {
            wd=PSunpou | 0x8000;
            *ofs << wd;
        }
        else
        {
            wd= 0x7FFF;
            dw=PSunpou | 0x80000000;
            *ofs << wd;
            *ofs << dw;
        }
    }
    DSunpou.Serialize(*ofs);
    SaveSunpouCount++;
    Mpoint++;
    return true;
}

// ソリッド
BOOL JWWDocument::SaveSolid(CDataSolid const& DSolid)
{
    WORD wd;
    DWORD dw;
    string s;

    if( SaveSolidCount == 0 )
    {
        PSolid=Mpoint;
        Mpoint++;
        wd=0xFFFF;
        *ofs << wd;
        *ofs << objCode;
        s="CDataSolid";
        wd=s.length();
        *ofs << wd;
        ofs->write(s.c_str(), wd);
    } else
    {
        if( SaveBich16(PSolid) )
        {
            wd=PSolid | 0x8000;
            *ofs << wd;
        }
        else
        {
            wd= 0x7FFF;
            dw=PSolid | 0x80000000;
            *ofs << wd;
            *ofs << dw;
        }
    }
    DSolid.Serialize(*ofs);
    SaveSolidCount++;
    Mpoint++;
    return true;
}

// ブロック
BOOL JWWDocument::SaveBlock(CDataBlock const& DBlock)
{
    WORD wd;
    DWORD dw;
    string s;

    if( SaveBlockCount == 0 )
    {
        PBlock=Mpoint;
        Mpoint++;
        wd=0xFFFF;
        *ofs << wd;
        *ofs << objCode;
        s="CDataBlock";
        wd=s.length();
        *ofs << wd;
        ofs->write(s.c_str(), wd);
    } else
    {
        if( SaveBich16(PBlock) )
        {
            wd=PBlock | 0x8000;
            *ofs << wd;
        }
        else
        {
            wd= 0x7FFF;
            dw=PBlock | 0x80000000;
            *ofs << wd;
            *ofs << dw;
        }
    }
    DBlock.Serialize(*ofs);
    SaveBlockCount++;
    Mpoint++;
    return true;
}

// データリスト
BOOL JWWDocument::SaveDataList(CDataList const& DList)
{
    WORD wd;
    DWORD dw;
    string s;

    if( SaveDataListCount == 0 )
    {
        PList=Mpoint;
        Mpoint++;
        wd=0xFFFF;
        *ofs << wd;
        *ofs << objCode;
        s="CDataList";
        wd=s.length();
        *ofs << wd;
        ofs->write(s.c_str(), wd);
    } else
    {
        if( SaveBich16(PList) )
        {
            wd=PList | 0x8000;
            *ofs << wd;
        }
        else
        {
            wd= 0x7FFF;
            dw=PList | 0x80000000;
            *ofs << wd;
            *ofs << dw;
        }
    }
    DList.Serialize(*ofs);
    SaveDataListCount++;
    Mpoint++;
    return true;
}

//データファイル保存
BOOL JWWDocument::Save()
{
    if(!ofs)
        return false;
    DWORD dw;
    WORD wd;
//    DOUBLE db;
    string s;
    SaveSenCount=0;
    SaveEnkoCount=0;
    SaveTenCount=0;
    SaveMojiCount=0;
    SaveSunpouCount=0;
    SaveSolidCount=0;
    SaveBlockCount=0;
    SaveDataListCount=0;

    WriteHeader();
    //データ出力
    dw = vSen.size() + vEnko.size() + vTen.size() + vMoji.size() + vSunpou.size() + vSolid.size() + vBlock.size();// + 7;
    if( SaveBich16(dw) )
        ofs->write((char*)&dw,2);
    else
    {
        wd= 0xFFFF;
        *ofs << wd;
        *ofs << dw;
    }
    Mpoint=1;
    unsigned int i;
    int j;
    for( i=0 ; i < vSen.size(); i++ )
        SaveSen(vSen[i]);
    for( i=0 ; i < vEnko.size(); i++ )
        SaveEnko(vEnko[i]);
    for( i=0 ; i < vTen.size(); i++ )
        SaveTen(vTen[i]);
    for( i=0 ; i < vMoji.size(); i++ )
        SaveMoji(vMoji[i]);
    for( i=0 ; i < vSunpou.size(); i++ )
        SaveSunpou(vSunpou[i]);
    for( i=0 ; i < vSolid.size(); i++ )
        SaveSolid(vSolid[i]);
    for( i=0 ; i < vBlock.size(); i++)
        SaveBlock(vBlock[i]);
    dw=pBlockList->getBlockListCount();
    *ofs << dw;
    for( i=0; i < dw; i++ )
    {
        SaveDataList(pBlockList->GetBlockList(i));
        int Count=pBlockList->GetDataListCount(i);
        for( j=0 ; j < Count; j++)
        {
            switch(pBlockList->GetDataType(i,j))
            {
            case	Sen :
                SaveSen(pBlockList->GetCDataSen(i,j));
                break;
            case	Enko:
                SaveEnko(pBlockList->GetCDataEnko(i,j));
                break;
            case	Ten:
                SaveTen(pBlockList->GetCDataTen(i,j));
                break;
            case	Moji:
                SaveMoji(pBlockList->GetCDataMoji(i,j));
                break;
            case	Solid:
                SaveSolid(pBlockList->GetCDataSolid(i,j));
                break;
            case	Sunpou:
                SaveSunpou(pBlockList->GetCDataSunpou(i,j));
                break;
            case	Block:
                SaveBlock(pBlockList->GetCDataBlock(i,j));
                break;
            }
        }
    }
    return true;
}

void JWWList::AddItem(int No, string& str)
{
    PNoList	nList = new NoList;
    nList->CDataString = str;
    nList->No = No;
    FList.push_back(nList);
}

JWWList::JWWList()
{
    string str = "";
    //NULLデータ登録
    AddItem(0, str);
}

JWWList::~JWWList()
{
    for( unsigned int i=0; i < FList.size(); i++)
        if(FList[i])
            delete FList[i];
    FList.clear();
}

int JWWList::GetCount()
{
    return FList.size();
}

NoList& JWWList::GetItem(int i)
{
    return *FList[i];
}


NoList& JWWList::GetNoByItem(int No)
{
//	vector<PNoList>::iterator   itr    = vect.begin();
//	vector<PNoList>::iterator   itrEnd = vect.end();
    for( unsigned int i=0; i < FList.size(); i++)
    {
        if(FList[i]->No == No){
            return *FList[i];
        }
    }
    return *FList[0];
}

JWWBlockList::JWWBlockList()
{
}

JWWBlockList::~JWWBlockList()
{
    int sz = FBlockList.size();
    for(int i=0; i < sz; i++)
    {
        if(FBlockList[i])
            delete FBlockList[i];
    }
    FBlockList.clear();

/*
    //2010-02-09  不要な削除
    sz = FDataList.size();
    for(int i=0; i < sz; i++)
    {
        if(FDataList[i])
            delete FDataList[i];
    }
    FDataList.clear();

    FDataType.clear();
*/
}

CDataList JWWBlockList::GetBlockList(unsigned int i)
{
    for(unsigned int k=0; k < FBlockList.size(); k++)
        if(i == FBlockList[k]->m_n_Number)
            return *(PCDataList)FBlockList[k];
    return {};
}

int JWWBlockList::getBlockListCount()
{
    return FBlockList.size();
}

CDataEnko JWWBlockList::GetCDataEnko(int i, int j)
{
    if( GetCDataType(i,j) == Enko )
        return *(PCDataEnko)GetData(i,j);
    return {};
}

CDataMoji JWWBlockList::GetCDataMoji(int i, int j)
{
    if( GetCDataType(i,j) == Moji )
        return *(PCDataMoji)GetData(i,j);
    return {};
}

CDataSen JWWBlockList::GetCDataSen(int i, int j)
{
    if( GetCDataType(i,j) == Sen )
        return *(PCDataSen)GetData(i,j);
    return {};
}

CDataSolid JWWBlockList::GetCDataSolid(int i, int j)
{
    if( GetCDataType(i,j) == Solid )
        return *(PCDataSolid)GetData(i,j);
    return {};
}

CDataSunpou JWWBlockList::GetCDataSunpou(int i, int j)
{
    if( GetCDataType(i,j) == Sunpou )
        return *(PCDataSunpou)GetData(i,j);
    return {};
}

CDataTen JWWBlockList::GetCDataTen(int i, int j)
{
    if( GetCDataType(i,j) == Ten )
        return *(PCDataTen)GetData(i,j);
    return {};
}

CDataType JWWBlockList::GetCDataType(int i, int j)
{
    return GetDataType(i,j);
}

void* JWWBlockList::GetData(unsigned int i, int j)
{
    int l = 0;
    for( unsigned int k=0; k < FBlockList.size(); k++ )
    {
        if( i == PCDataList(FBlockList[k])->m_nNumber )
            return FDataList[l+j];
        l=l + PCDataList(FBlockList[k])->Count;
    }
    return (void *)NULL;
}

int JWWBlockList::GetDataListCount(unsigned int i)
{
    for(unsigned int k=0; k < FBlockList.size(); k++)
    {
        if( i == PCDataList(FBlockList[k])->m_nNumber )
            return PCDataList(FBlockList[k])->Count;
    }
    return 0;
}

CDataType JWWBlockList::GetDataType(unsigned int i, int j)
{
    int l = 0;
    for( unsigned int k=0; k < FBlockList.size(); k++ )
    {
        if( i == PCDataList(FBlockList[k])->m_nNumber )
            return FDataType[l+j];
        l=l + PCDataList(FBlockList[k])->Count;
    }
    return Sen;
}

void JWWBlockList::AddBlockList(CDataList& CData)
{
    PCDataList data = new CDataList;
    *data = CData;
    FBlockList.push_back((PCDataBlock)data);
}

void JWWBlockList::AddDataListEnko(CDataEnko& D)
{
    PCDataEnko data = new CDataEnko;
    *data = D;
    FDataType.push_back(Enko);
    FDataList.push_back((PCDataList)data);
}

void JWWBlockList::AddDataListMoji(CDataMoji& D)
{
    PCDataMoji data = new CDataMoji;
    *data = D;
    FDataType.push_back(Moji);
    FDataList.push_back((PCDataList)data);
}

void JWWBlockList::AddDataListSen(CDataSen& D)
{
    PCDataSen data = new CDataSen;
    *data = D;
    FDataType.push_back(Sen);
    FDataList.push_back((PCDataList)data);
}

void JWWBlockList::AddDataListSolid(CDataSolid& D)
{
    PCDataSolid data = new CDataSolid;
    *data = D;
    FDataType.push_back(Solid);
    FDataList.push_back((PCDataList)data);
}

void JWWBlockList::AddDataListSunpou(CDataSunpou& D)
{
    PCDataSunpou data = new CDataSunpou;
    *data = D;
    FDataType.push_back(Sunpou);
    FDataList.push_back((PCDataList)data);
}

void JWWBlockList::AddDataListTen(CDataTen& D)
{
    PCDataTen data = new CDataTen;
    *data = D;
    FDataType.push_back(Ten);
    FDataList.push_back((PCDataList)data);
}

void JWWBlockList::Init()
{
    for(unsigned int i=0; i < FBlockList.size(); i++)
    {
        if(FBlockList[i])
            delete FBlockList[i];
    }
    FBlockList.clear();

    for(unsigned int i=0; i < FDataList.size(); i++)
    {
        if(FDataList[i])
            delete FDataList[i];
    }
    FDataList.clear();
    FDataType.clear();
}

void JWWBlockList::AddDataListBlock(CDataBlock& D)
{
    PCDataBlock data = new CDataBlock;
    *data = D;
    FDataType.push_back(Block);
    FDataList.push_back((PCDataList)data);
}

CDataBlock JWWBlockList::GetCDataBlock(int i, int j)
{
    if( GetCDataType(i,j) == Block )
        return *PCDataBlock(GetData(i,j));
    return {};
}
