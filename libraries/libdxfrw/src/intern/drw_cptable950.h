#ifndef DRW_CPTABLE950_H
#define DRW_CPTABLE950_H

//Chinese Big5 (cp950) plus the big5-hkscs extension

//first entry in this tables are 0x80

#define CPOFFSET950 0x80
#define CPLENGTH950 18594
#define NOTFOUND950 0x003F

//Table 950 one byte
static const int DRW_Table950[] = {
    0x20AC  //1 #EURO SIGN
};

//pairs of start/end in DRW_DoubleTable950
//leadTable[i] is the first index whose lead byte is 0x81+i, so the
//range for lead c is [leadTable[c-0x81], leadTable[c-0x80]).  The last
//two entries are the total, which both terminates lead 0xFE and keeps
//a stray 0xFF lead byte inside the array.
static const int DRW_LeadTable950[] = {
    0, //1#DBCS LEAD BYTE 0x81
    0, //2#DBCS LEAD BYTE 0x82
    0, //3#DBCS LEAD BYTE 0x83
    0, //4#DBCS LEAD BYTE 0x84
    0, //5#DBCS LEAD BYTE 0x85
    0, //6#DBCS LEAD BYTE 0x86
    0, //7#DBCS LEAD BYTE 0x87
    125, //8#DBCS LEAD BYTE 0x88
    198, //9#DBCS LEAD BYTE 0x89
    341, //10#DBCS LEAD BYTE 0x8A
    487, //11#DBCS LEAD BYTE 0x8B
    641, //12#DBCS LEAD BYTE 0x8C
    792, //13#DBCS LEAD BYTE 0x8D
    948, //14#DBCS LEAD BYTE 0x8E
    1105, //15#DBCS LEAD BYTE 0x8F
    1262, //16#DBCS LEAD BYTE 0x90
    1419, //17#DBCS LEAD BYTE 0x91
    1576, //18#DBCS LEAD BYTE 0x92
    1733, //19#DBCS LEAD BYTE 0x93
    1890, //20#DBCS LEAD BYTE 0x94
    2047, //21#DBCS LEAD BYTE 0x95
    2204, //22#DBCS LEAD BYTE 0x96
    2361, //23#DBCS LEAD BYTE 0x97
    2518, //24#DBCS LEAD BYTE 0x98
    2675, //25#DBCS LEAD BYTE 0x99
    2832, //26#DBCS LEAD BYTE 0x9A
    2989, //27#DBCS LEAD BYTE 0x9B
    3145, //28#DBCS LEAD BYTE 0x9C
    3302, //29#DBCS LEAD BYTE 0x9D
    3459, //30#DBCS LEAD BYTE 0x9E
    3613, //31#DBCS LEAD BYTE 0x9F
    3761, //32#DBCS LEAD BYTE 0xA0
    3908, //33#DBCS LEAD BYTE 0xA1
    4065, //34#DBCS LEAD BYTE 0xA2
    4222, //35#DBCS LEAD BYTE 0xA3
    4350, //36#DBCS LEAD BYTE 0xA4
    4507, //37#DBCS LEAD BYTE 0xA5
    4664, //38#DBCS LEAD BYTE 0xA6
    4821, //39#DBCS LEAD BYTE 0xA7
    4978, //40#DBCS LEAD BYTE 0xA8
    5135, //41#DBCS LEAD BYTE 0xA9
    5292, //42#DBCS LEAD BYTE 0xAA
    5449, //43#DBCS LEAD BYTE 0xAB
    5606, //44#DBCS LEAD BYTE 0xAC
    5763, //45#DBCS LEAD BYTE 0xAD
    5920, //46#DBCS LEAD BYTE 0xAE
    6077, //47#DBCS LEAD BYTE 0xAF
    6234, //48#DBCS LEAD BYTE 0xB0
    6391, //49#DBCS LEAD BYTE 0xB1
    6548, //50#DBCS LEAD BYTE 0xB2
    6705, //51#DBCS LEAD BYTE 0xB3
    6862, //52#DBCS LEAD BYTE 0xB4
    7019, //53#DBCS LEAD BYTE 0xB5
    7176, //54#DBCS LEAD BYTE 0xB6
    7333, //55#DBCS LEAD BYTE 0xB7
    7490, //56#DBCS LEAD BYTE 0xB8
    7647, //57#DBCS LEAD BYTE 0xB9
    7804, //58#DBCS LEAD BYTE 0xBA
    7961, //59#DBCS LEAD BYTE 0xBB
    8118, //60#DBCS LEAD BYTE 0xBC
    8275, //61#DBCS LEAD BYTE 0xBD
    8432, //62#DBCS LEAD BYTE 0xBE
    8589, //63#DBCS LEAD BYTE 0xBF
    8746, //64#DBCS LEAD BYTE 0xC0
    8903, //65#DBCS LEAD BYTE 0xC1
    9060, //66#DBCS LEAD BYTE 0xC2
    9217, //67#DBCS LEAD BYTE 0xC3
    9374, //68#DBCS LEAD BYTE 0xC4
    9531, //69#DBCS LEAD BYTE 0xC5
    9688, //70#DBCS LEAD BYTE 0xC6
    9845, //71#DBCS LEAD BYTE 0xC7
    10002, //72#DBCS LEAD BYTE 0xC8
    10116, //73#DBCS LEAD BYTE 0xC9
    10273, //74#DBCS LEAD BYTE 0xCA
    10430, //75#DBCS LEAD BYTE 0xCB
    10587, //76#DBCS LEAD BYTE 0xCC
    10744, //77#DBCS LEAD BYTE 0xCD
    10901, //78#DBCS LEAD BYTE 0xCE
    11058, //79#DBCS LEAD BYTE 0xCF
    11215, //80#DBCS LEAD BYTE 0xD0
    11372, //81#DBCS LEAD BYTE 0xD1
    11529, //82#DBCS LEAD BYTE 0xD2
    11686, //83#DBCS LEAD BYTE 0xD3
    11843, //84#DBCS LEAD BYTE 0xD4
    12000, //85#DBCS LEAD BYTE 0xD5
    12157, //86#DBCS LEAD BYTE 0xD6
    12314, //87#DBCS LEAD BYTE 0xD7
    12471, //88#DBCS LEAD BYTE 0xD8
    12628, //89#DBCS LEAD BYTE 0xD9
    12785, //90#DBCS LEAD BYTE 0xDA
    12942, //91#DBCS LEAD BYTE 0xDB
    13099, //92#DBCS LEAD BYTE 0xDC
    13256, //93#DBCS LEAD BYTE 0xDD
    13413, //94#DBCS LEAD BYTE 0xDE
    13570, //95#DBCS LEAD BYTE 0xDF
    13727, //96#DBCS LEAD BYTE 0xE0
    13884, //97#DBCS LEAD BYTE 0xE1
    14041, //98#DBCS LEAD BYTE 0xE2
    14198, //99#DBCS LEAD BYTE 0xE3
    14355, //100#DBCS LEAD BYTE 0xE4
    14512, //101#DBCS LEAD BYTE 0xE5
    14669, //102#DBCS LEAD BYTE 0xE6
    14826, //103#DBCS LEAD BYTE 0xE7
    14983, //104#DBCS LEAD BYTE 0xE8
    15140, //105#DBCS LEAD BYTE 0xE9
    15297, //106#DBCS LEAD BYTE 0xEA
    15454, //107#DBCS LEAD BYTE 0xEB
    15611, //108#DBCS LEAD BYTE 0xEC
    15768, //109#DBCS LEAD BYTE 0xED
    15925, //110#DBCS LEAD BYTE 0xEE
    16082, //111#DBCS LEAD BYTE 0xEF
    16239, //112#DBCS LEAD BYTE 0xF0
    16396, //113#DBCS LEAD BYTE 0xF1
    16553, //114#DBCS LEAD BYTE 0xF2
    16710, //115#DBCS LEAD BYTE 0xF3
    16867, //116#DBCS LEAD BYTE 0xF4
    17024, //117#DBCS LEAD BYTE 0xF5
    17181, //118#DBCS LEAD BYTE 0xF6
    17338, //119#DBCS LEAD BYTE 0xF7
    17495, //120#DBCS LEAD BYTE 0xF8
    17652, //121#DBCS LEAD BYTE 0xF9
    17809, //122#DBCS LEAD BYTE 0xFA
    17966, //123#DBCS LEAD BYTE 0xFB
    18123, //124#DBCS LEAD BYTE 0xFC
    18280, //125#DBCS LEAD BYTE 0xFD
    18437, //126#DBCS LEAD BYTE 0xFE
    18594, //127#DBCS LEAD BYTE 0xFF
    18594 //128#DBCS LEAD BYTE 0x100
};

//Table 950
static const int DRW_DoubleTable950[][2] = {
    {0x8740, 0x43F0}, //1 #big5-hkscs
    {0x8741, 0x4C32}, //2 #big5-hkscs
    {0x8742, 0x4603}, //3 #big5-hkscs
    {0x8743, 0x45A6}, //4 #big5-hkscs
    {0x8744, 0x4578}, //5 #big5-hkscs
    {0x8745, 0x27267}, //6 #big5-hkscs
    {0x8746, 0x4D77}, //7 #big5-hkscs
    {0x8747, 0x45B3}, //8 #big5-hkscs
    {0x8748, 0x27CB1}, //9 #big5-hkscs
    {0x8749, 0x4CE2}, //10 #big5-hkscs
    {0x874A, 0x27CC5}, //11 #big5-hkscs
    {0x874B, 0x3B95}, //12 #big5-hkscs
    {0x874C, 0x4736}, //13 #big5-hkscs
    {0x874D, 0x4744}, //14 #big5-hkscs
    {0x874E, 0x4C47}, //15 #big5-hkscs
    {0x874F, 0x4C40}, //16 #big5-hkscs
    {0x8750, 0x242BF}, //17 #big5-hkscs
    {0x8751, 0x23617}, //18 #big5-hkscs
    {0x8752, 0x27352}, //19 #big5-hkscs
    {0x8753, 0x26E8B}, //20 #big5-hkscs
    {0x8754, 0x270D2}, //21 #big5-hkscs
    {0x8755, 0x4C57}, //22 #big5-hkscs
    {0x8756, 0x2A351}, //23 #big5-hkscs
    {0x8757, 0x474F}, //24 #big5-hkscs
    {0x8758, 0x45DA}, //25 #big5-hkscs
    {0x8759, 0x4C85}, //26 #big5-hkscs
    {0x875A, 0x27C6C}, //27 #big5-hkscs
    {0x875B, 0x4D07}, //28 #big5-hkscs
    {0x875C, 0x4AA4}, //29 #big5-hkscs
    {0x875D, 0x46A1}, //30 #big5-hkscs
    {0x875E, 0x26B23}, //31 #big5-hkscs
    {0x875F, 0x7225}, //32 #big5-hkscs
    {0x8760, 0x25A54}, //33 #big5-hkscs
    {0x8761, 0x21A63}, //34 #big5-hkscs
    {0x8762, 0x23E06}, //35 #big5-hkscs
    {0x8763, 0x23F61}, //36 #big5-hkscs
    {0x8764, 0x664D}, //37 #big5-hkscs
    {0x8765, 0x56FB}, //38 #big5-hkscs
    {0x8767, 0x7D95}, //39 #big5-hkscs
    {0x8768, 0x591D}, //40 #big5-hkscs
    {0x8769, 0x28BB9}, //41 #big5-hkscs
    {0x876A, 0x3DF4}, //42 #big5-hkscs
    {0x876B, 0x9734}, //43 #big5-hkscs
    {0x876C, 0x27BEF}, //44 #big5-hkscs
    {0x876D, 0x5BDB}, //45 #big5-hkscs
    {0x876E, 0x21D5E}, //46 #big5-hkscs
    {0x876F, 0x5AA4}, //47 #big5-hkscs
    {0x8770, 0x3625}, //48 #big5-hkscs
    {0x8771, 0x29EB0}, //49 #big5-hkscs
    {0x8772, 0x5AD1}, //50 #big5-hkscs
    {0x8773, 0x5BB7}, //51 #big5-hkscs
    {0x8774, 0x5CFC}, //52 #big5-hkscs
    {0x8775, 0x676E}, //53 #big5-hkscs
    {0x8776, 0x8593}, //54 #big5-hkscs
    {0x8777, 0x29945}, //55 #big5-hkscs
    {0x8778, 0x7461}, //56 #big5-hkscs
    {0x8779, 0x749D}, //57 #big5-hkscs
    {0x877A, 0x3875}, //58 #big5-hkscs
    {0x877B, 0x21D53}, //59 #big5-hkscs
    {0x877C, 0x2369E}, //60 #big5-hkscs
    {0x877D, 0x26021}, //61 #big5-hkscs
    {0x877E, 0x3EEC}, //62 #big5-hkscs
    {0x87A1, 0x258DE}, //63 #big5-hkscs
    {0x87A2, 0x3AF5}, //64 #big5-hkscs
    {0x87A3, 0x7AFC}, //65 #big5-hkscs
    {0x87A4, 0x9F97}, //66 #big5-hkscs
    {0x87A5, 0x24161}, //67 #big5-hkscs
    {0x87A6, 0x2890D}, //68 #big5-hkscs
    {0x87A7, 0x231EA}, //69 #big5-hkscs
    {0x87A8, 0x20A8A}, //70 #big5-hkscs
    {0x87A9, 0x2325E}, //71 #big5-hkscs
    {0x87AA, 0x430A}, //72 #big5-hkscs
    {0x87AB, 0x8484}, //73 #big5-hkscs
    {0x87AC, 0x9F96}, //74 #big5-hkscs
    {0x87AD, 0x942F}, //75 #big5-hkscs
    {0x87AE, 0x4930}, //76 #big5-hkscs
    {0x87AF, 0x8613}, //77 #big5-hkscs
    {0x87B0, 0x5896}, //78 #big5-hkscs
    {0x87B1, 0x974A}, //79 #big5-hkscs
    {0x87B2, 0x9218}, //80 #big5-hkscs
    {0x87B3, 0x79D0}, //81 #big5-hkscs
    {0x87B4, 0x7A32}, //82 #big5-hkscs
    {0x87B5, 0x6660}, //83 #big5-hkscs
    {0x87B6, 0x6A29}, //84 #big5-hkscs
    {0x87B7, 0x889D}, //85 #big5-hkscs
    {0x87B8, 0x744C}, //86 #big5-hkscs
    {0x87B9, 0x7BC5}, //87 #big5-hkscs
    {0x87BA, 0x6782}, //88 #big5-hkscs
    {0x87BB, 0x7A2C}, //89 #big5-hkscs
    {0x87BC, 0x524F}, //90 #big5-hkscs
    {0x87BD, 0x9046}, //91 #big5-hkscs
    {0x87BE, 0x34E6}, //92 #big5-hkscs
    {0x87BF, 0x73C4}, //93 #big5-hkscs
    {0x87C0, 0x25DB9}, //94 #big5-hkscs
    {0x87C1, 0x74C6}, //95 #big5-hkscs
    {0x87C2, 0x9FC7}, //96 #big5-hkscs
    {0x87C3, 0x57B3}, //97 #big5-hkscs
    {0x87C4, 0x492F}, //98 #big5-hkscs
    {0x87C5, 0x544C}, //99 #big5-hkscs
    {0x87C6, 0x4131}, //100 #big5-hkscs
    {0x87C7, 0x2368E}, //101 #big5-hkscs
    {0x87C8, 0x5818}, //102 #big5-hkscs
    {0x87C9, 0x7A72}, //103 #big5-hkscs
    {0x87CA, 0x27B65}, //104 #big5-hkscs
    {0x87CB, 0x8B8F}, //105 #big5-hkscs
    {0x87CC, 0x46AE}, //106 #big5-hkscs
    {0x87CD, 0x26E88}, //107 #big5-hkscs
    {0x87CE, 0x4181}, //108 #big5-hkscs
    {0x87CF, 0x25D99}, //109 #big5-hkscs
    {0x87D0, 0x7BAE}, //110 #big5-hkscs
    {0x87D1, 0x224BC}, //111 #big5-hkscs
    {0x87D2, 0x9FC8}, //112 #big5-hkscs
    {0x87D3, 0x224C1}, //113 #big5-hkscs
    {0x87D4, 0x224C9}, //114 #big5-hkscs
    {0x87D5, 0x224CC}, //115 #big5-hkscs
    {0x87D6, 0x9FC9}, //116 #big5-hkscs
    {0x87D7, 0x8504}, //117 #big5-hkscs
    {0x87D8, 0x235BB}, //118 #big5-hkscs
    {0x87D9, 0x40B4}, //119 #big5-hkscs
    {0x87DA, 0x9FCA}, //120 #big5-hkscs
    {0x87DB, 0x44E1}, //121 #big5-hkscs
    {0x87DC, 0x2ADFF}, //122 #big5-hkscs
    {0x87DD, 0x62C1}, //123 #big5-hkscs
    {0x87DE, 0x706E}, //124 #big5-hkscs
    {0x87DF, 0x9FCB}, //125 #big5-hkscs
    {0x8840, 0x31C0}, //126 #big5-hkscs
    {0x8841, 0x31C1}, //127 #big5-hkscs
    {0x8842, 0x31C2}, //128 #big5-hkscs
    {0x8843, 0x31C3}, //129 #big5-hkscs
    {0x8844, 0x31C4}, //130 #big5-hkscs
    {0x8845, 0x2010C}, //131 #big5-hkscs
    {0x8846, 0x31C5}, //132 #big5-hkscs
    {0x8847, 0x200D1}, //133 #big5-hkscs
    {0x8848, 0x200CD}, //134 #big5-hkscs
    {0x8849, 0x31C6}, //135 #big5-hkscs
    {0x884A, 0x31C7}, //136 #big5-hkscs
    {0x884B, 0x200CB}, //137 #big5-hkscs
    {0x884C, 0x21FE8}, //138 #big5-hkscs
    {0x884D, 0x31C8}, //139 #big5-hkscs
    {0x884E, 0x200CA}, //140 #big5-hkscs
    {0x884F, 0x31C9}, //141 #big5-hkscs
    {0x8850, 0x31CA}, //142 #big5-hkscs
    {0x8851, 0x31CB}, //143 #big5-hkscs
    {0x8852, 0x31CC}, //144 #big5-hkscs
    {0x8853, 0x2010E}, //145 #big5-hkscs
    {0x8854, 0x31CD}, //146 #big5-hkscs
    {0x8855, 0x31CE}, //147 #big5-hkscs
    {0x8856, 0x0100}, //148 #big5-hkscs
    {0x8857, 0x00C1}, //149 #big5-hkscs
    {0x8858, 0x01CD}, //150 #big5-hkscs
    {0x8859, 0x00C0}, //151 #big5-hkscs
    {0x885A, 0x0112}, //152 #big5-hkscs
    {0x885B, 0x00C9}, //153 #big5-hkscs
    {0x885C, 0x011A}, //154 #big5-hkscs
    {0x885D, 0x00C8}, //155 #big5-hkscs
    {0x885E, 0x014C}, //156 #big5-hkscs
    {0x885F, 0x00D3}, //157 #big5-hkscs
    {0x8860, 0x01D1}, //158 #big5-hkscs
    {0x8861, 0x00D2}, //159 #big5-hkscs
    {0x8862, 0xCA0304}, //160 #U+00CA U+0304 big5-hkscs pair
    {0x8863, 0x1EBE}, //161 #big5-hkscs
    {0x8864, 0xCA030C}, //162 #U+00CA U+030C big5-hkscs pair
    {0x8865, 0x1EC0}, //163 #big5-hkscs
    {0x8866, 0x00CA}, //164 #big5-hkscs
    {0x8867, 0x0101}, //165 #big5-hkscs
    {0x8868, 0x00E1}, //166 #big5-hkscs
    {0x8869, 0x01CE}, //167 #big5-hkscs
    {0x886A, 0x00E0}, //168 #big5-hkscs
    {0x886B, 0x0251}, //169 #big5-hkscs
    {0x886C, 0x0113}, //170 #big5-hkscs
    {0x886D, 0x00E9}, //171 #big5-hkscs
    {0x886E, 0x011B}, //172 #big5-hkscs
    {0x886F, 0x00E8}, //173 #big5-hkscs
    {0x8870, 0x012B}, //174 #big5-hkscs
    {0x8871, 0x00ED}, //175 #big5-hkscs
    {0x8872, 0x01D0}, //176 #big5-hkscs
    {0x8873, 0x00EC}, //177 #big5-hkscs
    {0x8874, 0x014D}, //178 #big5-hkscs
    {0x8875, 0x00F3}, //179 #big5-hkscs
    {0x8876, 0x01D2}, //180 #big5-hkscs
    {0x8877, 0x00F2}, //181 #big5-hkscs
    {0x8878, 0x016B}, //182 #big5-hkscs
    {0x8879, 0x00FA}, //183 #big5-hkscs
    {0x887A, 0x01D4}, //184 #big5-hkscs
    {0x887B, 0x00F9}, //185 #big5-hkscs
    {0x887C, 0x01D6}, //186 #big5-hkscs
    {0x887D, 0x01D8}, //187 #big5-hkscs
    {0x887E, 0x01DA}, //188 #big5-hkscs
    {0x88A1, 0x01DC}, //189 #big5-hkscs
    {0x88A2, 0x00FC}, //190 #big5-hkscs
    {0x88A3, 0xEA0304}, //191 #U+00EA U+0304 big5-hkscs pair
    {0x88A4, 0x1EBF}, //192 #big5-hkscs
    {0x88A5, 0xEA030C}, //193 #U+00EA U+030C big5-hkscs pair
    {0x88A6, 0x1EC1}, //194 #big5-hkscs
    {0x88A7, 0x00EA}, //195 #big5-hkscs
    {0x88A8, 0x0261}, //196 #big5-hkscs
    {0x88A9, 0x23DA}, //197 #big5-hkscs
    {0x88AA, 0x23DB}, //198 #big5-hkscs
    {0x8940, 0x2A3A9}, //199 #big5-hkscs
    {0x8941, 0x21145}, //200 #big5-hkscs
    {0x8943, 0x650A}, //201 #big5-hkscs
    {0x8946, 0x4E3D}, //202 #big5-hkscs
    {0x8947, 0x6EDD}, //203 #big5-hkscs
    {0x8948, 0x9D4E}, //204 #big5-hkscs
    {0x8949, 0x91DF}, //205 #big5-hkscs
    {0x894C, 0x27735}, //206 #big5-hkscs
    {0x894D, 0x6491}, //207 #big5-hkscs
    {0x894E, 0x4F1A}, //208 #big5-hkscs
    {0x894F, 0x4F28}, //209 #big5-hkscs
    {0x8950, 0x4FA8}, //210 #big5-hkscs
    {0x8951, 0x5156}, //211 #big5-hkscs
    {0x8952, 0x5174}, //212 #big5-hkscs
    {0x8953, 0x519C}, //213 #big5-hkscs
    {0x8954, 0x51E4}, //214 #big5-hkscs
    {0x8955, 0x52A1}, //215 #big5-hkscs
    {0x8956, 0x52A8}, //216 #big5-hkscs
    {0x8957, 0x533B}, //217 #big5-hkscs
    {0x8958, 0x534E}, //218 #big5-hkscs
    {0x8959, 0x53D1}, //219 #big5-hkscs
    {0x895A, 0x53D8}, //220 #big5-hkscs
    {0x895B, 0x56E2}, //221 #big5-hkscs
    {0x895C, 0x58F0}, //222 #big5-hkscs
    {0x895D, 0x5904}, //223 #big5-hkscs
    {0x895E, 0x5907}, //224 #big5-hkscs
    {0x895F, 0x5932}, //225 #big5-hkscs
    {0x8960, 0x5934}, //226 #big5-hkscs
    {0x8961, 0x5B66}, //227 #big5-hkscs
    {0x8962, 0x5B9E}, //228 #big5-hkscs
    {0x8963, 0x5B9F}, //229 #big5-hkscs
    {0x8964, 0x5C9A}, //230 #big5-hkscs
    {0x8965, 0x5E86}, //231 #big5-hkscs
    {0x8966, 0x603B}, //232 #big5-hkscs
    {0x8967, 0x6589}, //233 #big5-hkscs
    {0x8968, 0x67FE}, //234 #big5-hkscs
    {0x8969, 0x6804}, //235 #big5-hkscs
    {0x896A, 0x6865}, //236 #big5-hkscs
    {0x896B, 0x6D4E}, //237 #big5-hkscs
    {0x896C, 0x70BC}, //238 #big5-hkscs
    {0x896D, 0x7535}, //239 #big5-hkscs
    {0x896E, 0x7EA4}, //240 #big5-hkscs
    {0x896F, 0x7EAC}, //241 #big5-hkscs
    {0x8970, 0x7EBA}, //242 #big5-hkscs
    {0x8971, 0x7EC7}, //243 #big5-hkscs
    {0x8972, 0x7ECF}, //244 #big5-hkscs
    {0x8973, 0x7EDF}, //245 #big5-hkscs
    {0x8974, 0x7F06}, //246 #big5-hkscs
    {0x8975, 0x7F37}, //247 #big5-hkscs
    {0x8976, 0x827A}, //248 #big5-hkscs
    {0x8977, 0x82CF}, //249 #big5-hkscs
    {0x8978, 0x836F}, //250 #big5-hkscs
    {0x8979, 0x89C6}, //251 #big5-hkscs
    {0x897A, 0x8BBE}, //252 #big5-hkscs
    {0x897B, 0x8BE2}, //253 #big5-hkscs
    {0x897C, 0x8F66}, //254 #big5-hkscs
    {0x897D, 0x8F67}, //255 #big5-hkscs
    {0x897E, 0x8F6E}, //256 #big5-hkscs
    {0x89A1, 0x7411}, //257 #big5-hkscs
    {0x89A2, 0x7CFC}, //258 #big5-hkscs
    {0x89A3, 0x7DCD}, //259 #big5-hkscs
    {0x89A4, 0x6946}, //260 #big5-hkscs
    {0x89A5, 0x7AC9}, //261 #big5-hkscs
    {0x89A6, 0x5227}, //262 #big5-hkscs
    {0x89AB, 0x918C}, //263 #big5-hkscs
    {0x89AC, 0x78B8}, //264 #big5-hkscs
    {0x89AD, 0x915E}, //265 #big5-hkscs
    {0x89AE, 0x80BC}, //266 #big5-hkscs
    {0x89B0, 0x8D0B}, //267 #big5-hkscs
    {0x89B1, 0x80F6}, //268 #big5-hkscs
    {0x89B2, 0x209E7}, //269 #big5-hkscs
    {0x89B5, 0x809F}, //270 #big5-hkscs
    {0x89B6, 0x9EC7}, //271 #big5-hkscs
    {0x89B7, 0x4CCD}, //272 #big5-hkscs
    {0x89B8, 0x9DC9}, //273 #big5-hkscs
    {0x89B9, 0x9E0C}, //274 #big5-hkscs
    {0x89BA, 0x4C3E}, //275 #big5-hkscs
    {0x89BB, 0x29DF6}, //276 #big5-hkscs
    {0x89BC, 0x2700E}, //277 #big5-hkscs
    {0x89BD, 0x9E0A}, //278 #big5-hkscs
    {0x89BE, 0x2A133}, //279 #big5-hkscs
    {0x89BF, 0x35C1}, //280 #big5-hkscs
    {0x89C1, 0x6E9A}, //281 #big5-hkscs
    {0x89C2, 0x823E}, //282 #big5-hkscs
    {0x89C3, 0x7519}, //283 #big5-hkscs
    {0x89C5, 0x4911}, //284 #big5-hkscs
    {0x89C6, 0x9A6C}, //285 #big5-hkscs
    {0x89C7, 0x9A8F}, //286 #big5-hkscs
    {0x89C8, 0x9F99}, //287 #big5-hkscs
    {0x89C9, 0x7987}, //288 #big5-hkscs
    {0x89CA, 0x2846C}, //289 #big5-hkscs
    {0x89CB, 0x21DCA}, //290 #big5-hkscs
    {0x89CC, 0x205D0}, //291 #big5-hkscs
    {0x89CD, 0x22AE6}, //292 #big5-hkscs
    {0x89CE, 0x4E24}, //293 #big5-hkscs
    {0x89CF, 0x4E81}, //294 #big5-hkscs
    {0x89D0, 0x4E80}, //295 #big5-hkscs
    {0x89D1, 0x4E87}, //296 #big5-hkscs
    {0x89D2, 0x4EBF}, //297 #big5-hkscs
    {0x89D3, 0x4EEB}, //298 #big5-hkscs
    {0x89D4, 0x4F37}, //299 #big5-hkscs
    {0x89D5, 0x344C}, //300 #big5-hkscs
    {0x89D6, 0x4FBD}, //301 #big5-hkscs
    {0x89D7, 0x3E48}, //302 #big5-hkscs
    {0x89D8, 0x5003}, //303 #big5-hkscs
    {0x89D9, 0x5088}, //304 #big5-hkscs
    {0x89DA, 0x347D}, //305 #big5-hkscs
    {0x89DB, 0x3493}, //306 #big5-hkscs
    {0x89DC, 0x34A5}, //307 #big5-hkscs
    {0x89DD, 0x5186}, //308 #big5-hkscs
    {0x89DE, 0x5905}, //309 #big5-hkscs
    {0x89DF, 0x51DB}, //310 #big5-hkscs
    {0x89E0, 0x51FC}, //311 #big5-hkscs
    {0x89E1, 0x5205}, //312 #big5-hkscs
    {0x89E2, 0x4E89}, //313 #big5-hkscs
    {0x89E3, 0x5279}, //314 #big5-hkscs
    {0x89E4, 0x5290}, //315 #big5-hkscs
    {0x89E5, 0x5327}, //316 #big5-hkscs
    {0x89E6, 0x35C7}, //317 #big5-hkscs
    {0x89E7, 0x53A9}, //318 #big5-hkscs
    {0x89E8, 0x3551}, //319 #big5-hkscs
    {0x89E9, 0x53B0}, //320 #big5-hkscs
    {0x89EA, 0x3553}, //321 #big5-hkscs
    {0x89EB, 0x53C2}, //322 #big5-hkscs
    {0x89EC, 0x5423}, //323 #big5-hkscs
    {0x89ED, 0x356D}, //324 #big5-hkscs
    {0x89EE, 0x3572}, //325 #big5-hkscs
    {0x89EF, 0x3681}, //326 #big5-hkscs
    {0x89F0, 0x5493}, //327 #big5-hkscs
    {0x89F1, 0x54A3}, //328 #big5-hkscs
    {0x89F2, 0x54B4}, //329 #big5-hkscs
    {0x89F3, 0x54B9}, //330 #big5-hkscs
    {0x89F4, 0x54D0}, //331 #big5-hkscs
    {0x89F5, 0x54EF}, //332 #big5-hkscs
    {0x89F6, 0x5518}, //333 #big5-hkscs
    {0x89F7, 0x5523}, //334 #big5-hkscs
    {0x89F8, 0x5528}, //335 #big5-hkscs
    {0x89F9, 0x3598}, //336 #big5-hkscs
    {0x89FA, 0x553F}, //337 #big5-hkscs
    {0x89FB, 0x35A5}, //338 #big5-hkscs
    {0x89FC, 0x35BF}, //339 #big5-hkscs
    {0x89FD, 0x55D7}, //340 #big5-hkscs
    {0x89FE, 0x35C5}, //341 #big5-hkscs
    {0x8A40, 0x27D84}, //342 #big5-hkscs
    {0x8A41, 0x5525}, //343 #big5-hkscs
    {0x8A43, 0x20C42}, //344 #big5-hkscs
    {0x8A44, 0x20D15}, //345 #big5-hkscs
    {0x8A45, 0x2512B}, //346 #big5-hkscs
    {0x8A46, 0x5590}, //347 #big5-hkscs
    {0x8A47, 0x22CC6}, //348 #big5-hkscs
    {0x8A48, 0x39EC}, //349 #big5-hkscs
    {0x8A49, 0x20341}, //350 #big5-hkscs
    {0x8A4A, 0x8E46}, //351 #big5-hkscs
    {0x8A4B, 0x24DB8}, //352 #big5-hkscs
    {0x8A4C, 0x294E5}, //353 #big5-hkscs
    {0x8A4D, 0x4053}, //354 #big5-hkscs
    {0x8A4E, 0x280BE}, //355 #big5-hkscs
    {0x8A4F, 0x777A}, //356 #big5-hkscs
    {0x8A50, 0x22C38}, //357 #big5-hkscs
    {0x8A51, 0x3A34}, //358 #big5-hkscs
    {0x8A52, 0x47D5}, //359 #big5-hkscs
    {0x8A53, 0x2815D}, //360 #big5-hkscs
    {0x8A54, 0x269F2}, //361 #big5-hkscs
    {0x8A55, 0x24DEA}, //362 #big5-hkscs
    {0x8A56, 0x64DD}, //363 #big5-hkscs
    {0x8A57, 0x20D7C}, //364 #big5-hkscs
    {0x8A58, 0x20FB4}, //365 #big5-hkscs
    {0x8A59, 0x20CD5}, //366 #big5-hkscs
    {0x8A5A, 0x210F4}, //367 #big5-hkscs
    {0x8A5B, 0x648D}, //368 #big5-hkscs
    {0x8A5C, 0x8E7E}, //369 #big5-hkscs
    {0x8A5D, 0x20E96}, //370 #big5-hkscs
    {0x8A5E, 0x20C0B}, //371 #big5-hkscs
    {0x8A5F, 0x20F64}, //372 #big5-hkscs
    {0x8A60, 0x22CA9}, //373 #big5-hkscs
    {0x8A61, 0x28256}, //374 #big5-hkscs
    {0x8A62, 0x244D3}, //375 #big5-hkscs
    {0x8A64, 0x20D46}, //376 #big5-hkscs
    {0x8A65, 0x29A4D}, //377 #big5-hkscs
    {0x8A66, 0x280E9}, //378 #big5-hkscs
    {0x8A67, 0x47F4}, //379 #big5-hkscs
    {0x8A68, 0x24EA7}, //380 #big5-hkscs
    {0x8A69, 0x22CC2}, //381 #big5-hkscs
    {0x8A6A, 0x9AB2}, //382 #big5-hkscs
    {0x8A6B, 0x3A67}, //383 #big5-hkscs
    {0x8A6C, 0x295F4}, //384 #big5-hkscs
    {0x8A6D, 0x3FED}, //385 #big5-hkscs
    {0x8A6E, 0x3506}, //386 #big5-hkscs
    {0x8A6F, 0x252C7}, //387 #big5-hkscs
    {0x8A70, 0x297D4}, //388 #big5-hkscs
    {0x8A71, 0x278C8}, //389 #big5-hkscs
    {0x8A72, 0x22D44}, //390 #big5-hkscs
    {0x8A73, 0x9D6E}, //391 #big5-hkscs
    {0x8A74, 0x9815}, //392 #big5-hkscs
    {0x8A76, 0x43D9}, //393 #big5-hkscs
    {0x8A77, 0x260A5}, //394 #big5-hkscs
    {0x8A78, 0x64B4}, //395 #big5-hkscs
    {0x8A79, 0x54E3}, //396 #big5-hkscs
    {0x8A7A, 0x22D4C}, //397 #big5-hkscs
    {0x8A7B, 0x22BCA}, //398 #big5-hkscs
    {0x8A7C, 0x21077}, //399 #big5-hkscs
    {0x8A7D, 0x39FB}, //400 #big5-hkscs
    {0x8A7E, 0x2106F}, //401 #big5-hkscs
    {0x8AA1, 0x266DA}, //402 #big5-hkscs
    {0x8AA2, 0x26716}, //403 #big5-hkscs
    {0x8AA3, 0x279A0}, //404 #big5-hkscs
    {0x8AA4, 0x64EA}, //405 #big5-hkscs
    {0x8AA5, 0x25052}, //406 #big5-hkscs
    {0x8AA6, 0x20C43}, //407 #big5-hkscs
    {0x8AA7, 0x8E68}, //408 #big5-hkscs
    {0x8AA8, 0x221A1}, //409 #big5-hkscs
    {0x8AA9, 0x28B4C}, //410 #big5-hkscs
    {0x8AAA, 0x20731}, //411 #big5-hkscs
    {0x8AAC, 0x480B}, //412 #big5-hkscs
    {0x8AAD, 0x201A9}, //413 #big5-hkscs
    {0x8AAE, 0x3FFA}, //414 #big5-hkscs
    {0x8AAF, 0x5873}, //415 #big5-hkscs
    {0x8AB0, 0x22D8D}, //416 #big5-hkscs
    {0x8AB2, 0x245C8}, //417 #big5-hkscs
    {0x8AB3, 0x204FC}, //418 #big5-hkscs
    {0x8AB4, 0x26097}, //419 #big5-hkscs
    {0x8AB5, 0x20F4C}, //420 #big5-hkscs
    {0x8AB6, 0x20D96}, //421 #big5-hkscs
    {0x8AB7, 0x5579}, //422 #big5-hkscs
    {0x8AB8, 0x40BB}, //423 #big5-hkscs
    {0x8AB9, 0x43BA}, //424 #big5-hkscs
    {0x8ABB, 0x4AB4}, //425 #big5-hkscs
    {0x8ABC, 0x22A66}, //426 #big5-hkscs
    {0x8ABD, 0x2109D}, //427 #big5-hkscs
    {0x8ABE, 0x81AA}, //428 #big5-hkscs
    {0x8ABF, 0x98F5}, //429 #big5-hkscs
    {0x8AC0, 0x20D9C}, //430 #big5-hkscs
    {0x8AC1, 0x6379}, //431 #big5-hkscs
    {0x8AC2, 0x39FE}, //432 #big5-hkscs
    {0x8AC3, 0x22775}, //433 #big5-hkscs
    {0x8AC4, 0x8DC0}, //434 #big5-hkscs
    {0x8AC5, 0x56A1}, //435 #big5-hkscs
    {0x8AC6, 0x647C}, //436 #big5-hkscs
    {0x8AC7, 0x3E43}, //437 #big5-hkscs
    {0x8AC9, 0x2A601}, //438 #big5-hkscs
    {0x8ACA, 0x20E09}, //439 #big5-hkscs
    {0x8ACB, 0x22ACF}, //440 #big5-hkscs
    {0x8ACC, 0x22CC9}, //441 #big5-hkscs
    {0x8ACE, 0x210C8}, //442 #big5-hkscs
    {0x8ACF, 0x239C2}, //443 #big5-hkscs
    {0x8AD0, 0x3992}, //444 #big5-hkscs
    {0x8AD1, 0x3A06}, //445 #big5-hkscs
    {0x8AD2, 0x2829B}, //446 #big5-hkscs
    {0x8AD3, 0x3578}, //447 #big5-hkscs
    {0x8AD4, 0x25E49}, //448 #big5-hkscs
    {0x8AD5, 0x220C7}, //449 #big5-hkscs
    {0x8AD6, 0x5652}, //450 #big5-hkscs
    {0x8AD7, 0x20F31}, //451 #big5-hkscs
    {0x8AD8, 0x22CB2}, //452 #big5-hkscs
    {0x8AD9, 0x29720}, //453 #big5-hkscs
    {0x8ADA, 0x34BC}, //454 #big5-hkscs
    {0x8ADB, 0x6C3D}, //455 #big5-hkscs
    {0x8ADC, 0x24E3B}, //456 #big5-hkscs
    {0x8ADF, 0x27574}, //457 #big5-hkscs
    {0x8AE0, 0x22E8B}, //458 #big5-hkscs
    {0x8AE1, 0x22208}, //459 #big5-hkscs
    {0x8AE2, 0x2A65B}, //460 #big5-hkscs
    {0x8AE3, 0x28CCD}, //461 #big5-hkscs
    {0x8AE4, 0x20E7A}, //462 #big5-hkscs
    {0x8AE5, 0x20C34}, //463 #big5-hkscs
    {0x8AE6, 0x2681C}, //464 #big5-hkscs
    {0x8AE7, 0x7F93}, //465 #big5-hkscs
    {0x8AE8, 0x210CF}, //466 #big5-hkscs
    {0x8AE9, 0x22803}, //467 #big5-hkscs
    {0x8AEA, 0x22939}, //468 #big5-hkscs
    {0x8AEB, 0x35FB}, //469 #big5-hkscs
    {0x8AEC, 0x251E3}, //470 #big5-hkscs
    {0x8AED, 0x20E8C}, //471 #big5-hkscs
    {0x8AEE, 0x20F8D}, //472 #big5-hkscs
    {0x8AEF, 0x20EAA}, //473 #big5-hkscs
    {0x8AF0, 0x3F93}, //474 #big5-hkscs
    {0x8AF1, 0x20F30}, //475 #big5-hkscs
    {0x8AF2, 0x20D47}, //476 #big5-hkscs
    {0x8AF3, 0x2114F}, //477 #big5-hkscs
    {0x8AF4, 0x20E4C}, //478 #big5-hkscs
    {0x8AF6, 0x20EAB}, //479 #big5-hkscs
    {0x8AF7, 0x20BA9}, //480 #big5-hkscs
    {0x8AF8, 0x20D48}, //481 #big5-hkscs
    {0x8AF9, 0x210C0}, //482 #big5-hkscs
    {0x8AFA, 0x2113D}, //483 #big5-hkscs
    {0x8AFB, 0x3FF9}, //484 #big5-hkscs
    {0x8AFC, 0x22696}, //485 #big5-hkscs
    {0x8AFD, 0x6432}, //486 #big5-hkscs
    {0x8AFE, 0x20FAD}, //487 #big5-hkscs
    {0x8B40, 0x233F4}, //488 #big5-hkscs
    {0x8B41, 0x27639}, //489 #big5-hkscs
    {0x8B42, 0x22BCE}, //490 #big5-hkscs
    {0x8B43, 0x20D7E}, //491 #big5-hkscs
    {0x8B44, 0x20D7F}, //492 #big5-hkscs
    {0x8B45, 0x22C51}, //493 #big5-hkscs
    {0x8B46, 0x22C55}, //494 #big5-hkscs
    {0x8B47, 0x3A18}, //495 #big5-hkscs
    {0x8B48, 0x20E98}, //496 #big5-hkscs
    {0x8B49, 0x210C7}, //497 #big5-hkscs
    {0x8B4A, 0x20F2E}, //498 #big5-hkscs
    {0x8B4B, 0x2A632}, //499 #big5-hkscs
    {0x8B4C, 0x26B50}, //500 #big5-hkscs
    {0x8B4D, 0x28CD2}, //501 #big5-hkscs
    {0x8B4E, 0x28D99}, //502 #big5-hkscs
    {0x8B4F, 0x28CCA}, //503 #big5-hkscs
    {0x8B50, 0x95AA}, //504 #big5-hkscs
    {0x8B51, 0x54CC}, //505 #big5-hkscs
    {0x8B52, 0x82C4}, //506 #big5-hkscs
    {0x8B53, 0x55B9}, //507 #big5-hkscs
    {0x8B55, 0x29EC3}, //508 #big5-hkscs
    {0x8B56, 0x9C26}, //509 #big5-hkscs
    {0x8B57, 0x9AB6}, //510 #big5-hkscs
    {0x8B58, 0x2775E}, //511 #big5-hkscs
    {0x8B59, 0x22DEE}, //512 #big5-hkscs
    {0x8B5A, 0x7140}, //513 #big5-hkscs
    {0x8B5B, 0x816D}, //514 #big5-hkscs
    {0x8B5C, 0x80EC}, //515 #big5-hkscs
    {0x8B5D, 0x5C1C}, //516 #big5-hkscs
    {0x8B5E, 0x26572}, //517 #big5-hkscs
    {0x8B5F, 0x8134}, //518 #big5-hkscs
    {0x8B60, 0x3797}, //519 #big5-hkscs
    {0x8B61, 0x535F}, //520 #big5-hkscs
    {0x8B62, 0x280BD}, //521 #big5-hkscs
    {0x8B63, 0x91B6}, //522 #big5-hkscs
    {0x8B64, 0x20EFA}, //523 #big5-hkscs
    {0x8B65, 0x20E0F}, //524 #big5-hkscs
    {0x8B66, 0x20E77}, //525 #big5-hkscs
    {0x8B67, 0x20EFB}, //526 #big5-hkscs
    {0x8B68, 0x35DD}, //527 #big5-hkscs
    {0x8B69, 0x24DEB}, //528 #big5-hkscs
    {0x8B6A, 0x3609}, //529 #big5-hkscs
    {0x8B6B, 0x20CD6}, //530 #big5-hkscs
    {0x8B6C, 0x56AF}, //531 #big5-hkscs
    {0x8B6D, 0x227B5}, //532 #big5-hkscs
    {0x8B6E, 0x210C9}, //533 #big5-hkscs
    {0x8B6F, 0x20E10}, //534 #big5-hkscs
    {0x8B70, 0x20E78}, //535 #big5-hkscs
    {0x8B71, 0x21078}, //536 #big5-hkscs
    {0x8B72, 0x21148}, //537 #big5-hkscs
    {0x8B73, 0x28207}, //538 #big5-hkscs
    {0x8B74, 0x21455}, //539 #big5-hkscs
    {0x8B75, 0x20E79}, //540 #big5-hkscs
    {0x8B76, 0x24E50}, //541 #big5-hkscs
    {0x8B77, 0x22DA4}, //542 #big5-hkscs
    {0x8B78, 0x5A54}, //543 #big5-hkscs
    {0x8B79, 0x2101D}, //544 #big5-hkscs
    {0x8B7A, 0x2101E}, //545 #big5-hkscs
    {0x8B7B, 0x210F5}, //546 #big5-hkscs
    {0x8B7C, 0x210F6}, //547 #big5-hkscs
    {0x8B7D, 0x579C}, //548 #big5-hkscs
    {0x8B7E, 0x20E11}, //549 #big5-hkscs
    {0x8BA1, 0x27694}, //550 #big5-hkscs
    {0x8BA2, 0x282CD}, //551 #big5-hkscs
    {0x8BA3, 0x20FB5}, //552 #big5-hkscs
    {0x8BA4, 0x20E7B}, //553 #big5-hkscs
    {0x8BA5, 0x2517E}, //554 #big5-hkscs
    {0x8BA6, 0x3703}, //555 #big5-hkscs
    {0x8BA7, 0x20FB6}, //556 #big5-hkscs
    {0x8BA8, 0x21180}, //557 #big5-hkscs
    {0x8BA9, 0x252D8}, //558 #big5-hkscs
    {0x8BAA, 0x2A2BD}, //559 #big5-hkscs
    {0x8BAB, 0x249DA}, //560 #big5-hkscs
    {0x8BAC, 0x2183A}, //561 #big5-hkscs
    {0x8BAD, 0x24177}, //562 #big5-hkscs
    {0x8BAE, 0x2827C}, //563 #big5-hkscs
    {0x8BAF, 0x5899}, //564 #big5-hkscs
    {0x8BB0, 0x5268}, //565 #big5-hkscs
    {0x8BB1, 0x361A}, //566 #big5-hkscs
    {0x8BB2, 0x2573D}, //567 #big5-hkscs
    {0x8BB3, 0x7BB2}, //568 #big5-hkscs
    {0x8BB4, 0x5B68}, //569 #big5-hkscs
    {0x8BB5, 0x4800}, //570 #big5-hkscs
    {0x8BB6, 0x4B2C}, //571 #big5-hkscs
    {0x8BB7, 0x9F27}, //572 #big5-hkscs
    {0x8BB8, 0x49E7}, //573 #big5-hkscs
    {0x8BB9, 0x9C1F}, //574 #big5-hkscs
    {0x8BBA, 0x9B8D}, //575 #big5-hkscs
    {0x8BBB, 0x25B74}, //576 #big5-hkscs
    {0x8BBC, 0x2313D}, //577 #big5-hkscs
    {0x8BBD, 0x55FB}, //578 #big5-hkscs
    {0x8BBE, 0x35F2}, //579 #big5-hkscs
    {0x8BBF, 0x5689}, //580 #big5-hkscs
    {0x8BC0, 0x4E28}, //581 #big5-hkscs
    {0x8BC1, 0x5902}, //582 #big5-hkscs
    {0x8BC2, 0x21BC1}, //583 #big5-hkscs
    {0x8BC3, 0x2F878}, //584 #big5-hkscs
    {0x8BC4, 0x9751}, //585 #big5-hkscs
    {0x8BC5, 0x20086}, //586 #big5-hkscs
    {0x8BC6, 0x4E5B}, //587 #big5-hkscs
    {0x8BC7, 0x4EBB}, //588 #big5-hkscs
    {0x8BC8, 0x353E}, //589 #big5-hkscs
    {0x8BC9, 0x5C23}, //590 #big5-hkscs
    {0x8BCA, 0x5F51}, //591 #big5-hkscs
    {0x8BCB, 0x5FC4}, //592 #big5-hkscs
    {0x8BCC, 0x38FA}, //593 #big5-hkscs
    {0x8BCD, 0x624C}, //594 #big5-hkscs
    {0x8BCE, 0x6535}, //595 #big5-hkscs
    {0x8BCF, 0x6B7A}, //596 #big5-hkscs
    {0x8BD0, 0x6C35}, //597 #big5-hkscs
    {0x8BD1, 0x6C3A}, //598 #big5-hkscs
    {0x8BD2, 0x706C}, //599 #big5-hkscs
    {0x8BD3, 0x722B}, //600 #big5-hkscs
    {0x8BD4, 0x4E2C}, //601 #big5-hkscs
    {0x8BD5, 0x72AD}, //602 #big5-hkscs
    {0x8BD6, 0x248E9}, //603 #big5-hkscs
    {0x8BD7, 0x7F52}, //604 #big5-hkscs
    {0x8BD8, 0x793B}, //605 #big5-hkscs
    {0x8BD9, 0x7CF9}, //606 #big5-hkscs
    {0x8BDA, 0x7F53}, //607 #big5-hkscs
    {0x8BDB, 0x2626A}, //608 #big5-hkscs
    {0x8BDC, 0x34C1}, //609 #big5-hkscs
    {0x8BDE, 0x2634B}, //610 #big5-hkscs
    {0x8BDF, 0x8002}, //611 #big5-hkscs
    {0x8BE0, 0x8080}, //612 #big5-hkscs
    {0x8BE1, 0x26612}, //613 #big5-hkscs
    {0x8BE2, 0x26951}, //614 #big5-hkscs
    {0x8BE3, 0x535D}, //615 #big5-hkscs
    {0x8BE4, 0x8864}, //616 #big5-hkscs
    {0x8BE5, 0x89C1}, //617 #big5-hkscs
    {0x8BE6, 0x278B2}, //618 #big5-hkscs
    {0x8BE7, 0x8BA0}, //619 #big5-hkscs
    {0x8BE8, 0x8D1D}, //620 #big5-hkscs
    {0x8BE9, 0x9485}, //621 #big5-hkscs
    {0x8BEA, 0x9578}, //622 #big5-hkscs
    {0x8BEB, 0x957F}, //623 #big5-hkscs
    {0x8BEC, 0x95E8}, //624 #big5-hkscs
    {0x8BED, 0x28E0F}, //625 #big5-hkscs
    {0x8BEE, 0x97E6}, //626 #big5-hkscs
    {0x8BEF, 0x9875}, //627 #big5-hkscs
    {0x8BF0, 0x98CE}, //628 #big5-hkscs
    {0x8BF1, 0x98DE}, //629 #big5-hkscs
    {0x8BF2, 0x9963}, //630 #big5-hkscs
    {0x8BF3, 0x29810}, //631 #big5-hkscs
    {0x8BF4, 0x9C7C}, //632 #big5-hkscs
    {0x8BF5, 0x9E1F}, //633 #big5-hkscs
    {0x8BF6, 0x9EC4}, //634 #big5-hkscs
    {0x8BF7, 0x6B6F}, //635 #big5-hkscs
    {0x8BF8, 0xF907}, //636 #big5-hkscs
    {0x8BF9, 0x4E37}, //637 #big5-hkscs
    {0x8BFA, 0x20087}, //638 #big5-hkscs
    {0x8BFB, 0x961D}, //639 #big5-hkscs
    {0x8BFC, 0x6237}, //640 #big5-hkscs
    {0x8BFD, 0x94A2}, //641 #big5-hkscs
    {0x8C40, 0x503B}, //642 #big5-hkscs
    {0x8C41, 0x6DFE}, //643 #big5-hkscs
    {0x8C42, 0x29C73}, //644 #big5-hkscs
    {0x8C43, 0x9FA6}, //645 #big5-hkscs
    {0x8C44, 0x3DC9}, //646 #big5-hkscs
    {0x8C45, 0x888F}, //647 #big5-hkscs
    {0x8C46, 0x2414E}, //648 #big5-hkscs
    {0x8C47, 0x7077}, //649 #big5-hkscs
    {0x8C48, 0x5CF5}, //650 #big5-hkscs
    {0x8C49, 0x4B20}, //651 #big5-hkscs
    {0x8C4A, 0x251CD}, //652 #big5-hkscs
    {0x8C4B, 0x3559}, //653 #big5-hkscs
    {0x8C4C, 0x25D30}, //654 #big5-hkscs
    {0x8C4D, 0x6122}, //655 #big5-hkscs
    {0x8C4E, 0x28A32}, //656 #big5-hkscs
    {0x8C4F, 0x8FA7}, //657 #big5-hkscs
    {0x8C50, 0x91F6}, //658 #big5-hkscs
    {0x8C51, 0x7191}, //659 #big5-hkscs
    {0x8C52, 0x6719}, //660 #big5-hkscs
    {0x8C53, 0x73BA}, //661 #big5-hkscs
    {0x8C54, 0x23281}, //662 #big5-hkscs
    {0x8C55, 0x2A107}, //663 #big5-hkscs
    {0x8C56, 0x3C8B}, //664 #big5-hkscs
    {0x8C57, 0x21980}, //665 #big5-hkscs
    {0x8C58, 0x4B10}, //666 #big5-hkscs
    {0x8C59, 0x78E4}, //667 #big5-hkscs
    {0x8C5A, 0x7402}, //668 #big5-hkscs
    {0x8C5B, 0x51AE}, //669 #big5-hkscs
    {0x8C5C, 0x2870F}, //670 #big5-hkscs
    {0x8C5D, 0x4009}, //671 #big5-hkscs
    {0x8C5E, 0x6A63}, //672 #big5-hkscs
    {0x8C5F, 0x2A2BA}, //673 #big5-hkscs
    {0x8C60, 0x4223}, //674 #big5-hkscs
    {0x8C61, 0x860F}, //675 #big5-hkscs
    {0x8C62, 0x20A6F}, //676 #big5-hkscs
    {0x8C63, 0x7A2A}, //677 #big5-hkscs
    {0x8C64, 0x29947}, //678 #big5-hkscs
    {0x8C65, 0x28AEA}, //679 #big5-hkscs
    {0x8C66, 0x9755}, //680 #big5-hkscs
    {0x8C67, 0x704D}, //681 #big5-hkscs
    {0x8C68, 0x5324}, //682 #big5-hkscs
    {0x8C69, 0x2207E}, //683 #big5-hkscs
    {0x8C6A, 0x93F4}, //684 #big5-hkscs
    {0x8C6B, 0x76D9}, //685 #big5-hkscs
    {0x8C6C, 0x289E3}, //686 #big5-hkscs
    {0x8C6D, 0x9FA7}, //687 #big5-hkscs
    {0x8C6E, 0x77DD}, //688 #big5-hkscs
    {0x8C6F, 0x4EA3}, //689 #big5-hkscs
    {0x8C70, 0x4FF0}, //690 #big5-hkscs
    {0x8C71, 0x50BC}, //691 #big5-hkscs
    {0x8C72, 0x4E2F}, //692 #big5-hkscs
    {0x8C73, 0x4F17}, //693 #big5-hkscs
    {0x8C74, 0x9FA8}, //694 #big5-hkscs
    {0x8C75, 0x5434}, //695 #big5-hkscs
    {0x8C76, 0x7D8B}, //696 #big5-hkscs
    {0x8C77, 0x5892}, //697 #big5-hkscs
    {0x8C78, 0x58D0}, //698 #big5-hkscs
    {0x8C79, 0x21DB6}, //699 #big5-hkscs
    {0x8C7A, 0x5E92}, //700 #big5-hkscs
    {0x8C7B, 0x5E99}, //701 #big5-hkscs
    {0x8C7C, 0x5FC2}, //702 #big5-hkscs
    {0x8C7D, 0x22712}, //703 #big5-hkscs
    {0x8C7E, 0x658B}, //704 #big5-hkscs
    {0x8CA1, 0x233F9}, //705 #big5-hkscs
    {0x8CA2, 0x6919}, //706 #big5-hkscs
    {0x8CA3, 0x6A43}, //707 #big5-hkscs
    {0x8CA4, 0x23C63}, //708 #big5-hkscs
    {0x8CA5, 0x6CFF}, //709 #big5-hkscs
    {0x8CA7, 0x7200}, //710 #big5-hkscs
    {0x8CA8, 0x24505}, //711 #big5-hkscs
    {0x8CA9, 0x738C}, //712 #big5-hkscs
    {0x8CAA, 0x3EDB}, //713 #big5-hkscs
    {0x8CAB, 0x24A13}, //714 #big5-hkscs
    {0x8CAC, 0x5B15}, //715 #big5-hkscs
    {0x8CAD, 0x74B9}, //716 #big5-hkscs
    {0x8CAE, 0x8B83}, //717 #big5-hkscs
    {0x8CAF, 0x25CA4}, //718 #big5-hkscs
    {0x8CB0, 0x25695}, //719 #big5-hkscs
    {0x8CB1, 0x7A93}, //720 #big5-hkscs
    {0x8CB2, 0x7BEC}, //721 #big5-hkscs
    {0x8CB3, 0x7CC3}, //722 #big5-hkscs
    {0x8CB4, 0x7E6C}, //723 #big5-hkscs
    {0x8CB5, 0x82F8}, //724 #big5-hkscs
    {0x8CB6, 0x8597}, //725 #big5-hkscs
    {0x8CB7, 0x9FA9}, //726 #big5-hkscs
    {0x8CB8, 0x8890}, //727 #big5-hkscs
    {0x8CB9, 0x9FAA}, //728 #big5-hkscs
    {0x8CBA, 0x8EB9}, //729 #big5-hkscs
    {0x8CBB, 0x9FAB}, //730 #big5-hkscs
    {0x8CBC, 0x8FCF}, //731 #big5-hkscs
    {0x8CBD, 0x855F}, //732 #big5-hkscs
    {0x8CBE, 0x99E0}, //733 #big5-hkscs
    {0x8CBF, 0x9221}, //734 #big5-hkscs
    {0x8CC0, 0x9FAC}, //735 #big5-hkscs
    {0x8CC1, 0x28DB9}, //736 #big5-hkscs
    {0x8CC2, 0x2143F}, //737 #big5-hkscs
    {0x8CC3, 0x4071}, //738 #big5-hkscs
    {0x8CC4, 0x42A2}, //739 #big5-hkscs
    {0x8CC5, 0x5A1A}, //740 #big5-hkscs
    {0x8CC9, 0x9868}, //741 #big5-hkscs
    {0x8CCA, 0x676B}, //742 #big5-hkscs
    {0x8CCB, 0x4276}, //743 #big5-hkscs
    {0x8CCC, 0x573D}, //744 #big5-hkscs
    {0x8CCE, 0x85D6}, //745 #big5-hkscs
    {0x8CCF, 0x2497B}, //746 #big5-hkscs
    {0x8CD0, 0x82BF}, //747 #big5-hkscs
    {0x8CD1, 0x2710D}, //748 #big5-hkscs
    {0x8CD2, 0x4C81}, //749 #big5-hkscs
    {0x8CD3, 0x26D74}, //750 #big5-hkscs
    {0x8CD4, 0x5D7B}, //751 #big5-hkscs
    {0x8CD5, 0x26B15}, //752 #big5-hkscs
    {0x8CD6, 0x26FBE}, //753 #big5-hkscs
    {0x8CD7, 0x9FAD}, //754 #big5-hkscs
    {0x8CD8, 0x9FAE}, //755 #big5-hkscs
    {0x8CD9, 0x5B96}, //756 #big5-hkscs
    {0x8CDA, 0x9FAF}, //757 #big5-hkscs
    {0x8CDB, 0x66E7}, //758 #big5-hkscs
    {0x8CDC, 0x7E5B}, //759 #big5-hkscs
    {0x8CDD, 0x6E57}, //760 #big5-hkscs
    {0x8CDE, 0x79CA}, //761 #big5-hkscs
    {0x8CDF, 0x3D88}, //762 #big5-hkscs
    {0x8CE0, 0x44C3}, //763 #big5-hkscs
    {0x8CE1, 0x23256}, //764 #big5-hkscs
    {0x8CE2, 0x22796}, //765 #big5-hkscs
    {0x8CE3, 0x439A}, //766 #big5-hkscs
    {0x8CE4, 0x4536}, //767 #big5-hkscs
    {0x8CE6, 0x5CD5}, //768 #big5-hkscs
    {0x8CE7, 0x23B1A}, //769 #big5-hkscs
    {0x8CE8, 0x8AF9}, //770 #big5-hkscs
    {0x8CE9, 0x5C78}, //771 #big5-hkscs
    {0x8CEA, 0x3D12}, //772 #big5-hkscs
    {0x8CEB, 0x23551}, //773 #big5-hkscs
    {0x8CEC, 0x5D78}, //774 #big5-hkscs
    {0x8CED, 0x9FB2}, //775 #big5-hkscs
    {0x8CEE, 0x7157}, //776 #big5-hkscs
    {0x8CEF, 0x4558}, //777 #big5-hkscs
    {0x8CF0, 0x240EC}, //778 #big5-hkscs
    {0x8CF1, 0x21E23}, //779 #big5-hkscs
    {0x8CF2, 0x4C77}, //780 #big5-hkscs
    {0x8CF3, 0x3978}, //781 #big5-hkscs
    {0x8CF4, 0x344A}, //782 #big5-hkscs
    {0x8CF5, 0x201A4}, //783 #big5-hkscs
    {0x8CF6, 0x26C41}, //784 #big5-hkscs
    {0x8CF7, 0x8ACC}, //785 #big5-hkscs
    {0x8CF8, 0x4FB4}, //786 #big5-hkscs
    {0x8CF9, 0x20239}, //787 #big5-hkscs
    {0x8CFA, 0x59BF}, //788 #big5-hkscs
    {0x8CFB, 0x816C}, //789 #big5-hkscs
    {0x8CFC, 0x9856}, //790 #big5-hkscs
    {0x8CFD, 0x298FA}, //791 #big5-hkscs
    {0x8CFE, 0x5F3B}, //792 #big5-hkscs
    {0x8D40, 0x20B9F}, //793 #big5-hkscs
    {0x8D42, 0x221C1}, //794 #big5-hkscs
    {0x8D43, 0x2896D}, //795 #big5-hkscs
    {0x8D44, 0x4102}, //796 #big5-hkscs
    {0x8D45, 0x46BB}, //797 #big5-hkscs
    {0x8D46, 0x29079}, //798 #big5-hkscs
    {0x8D47, 0x3F07}, //799 #big5-hkscs
    {0x8D48, 0x9FB3}, //800 #big5-hkscs
    {0x8D49, 0x2A1B5}, //801 #big5-hkscs
    {0x8D4A, 0x40F8}, //802 #big5-hkscs
    {0x8D4B, 0x37D6}, //803 #big5-hkscs
    {0x8D4C, 0x46F7}, //804 #big5-hkscs
    {0x8D4D, 0x26C46}, //805 #big5-hkscs
    {0x8D4E, 0x417C}, //806 #big5-hkscs
    {0x8D4F, 0x286B2}, //807 #big5-hkscs
    {0x8D50, 0x273FF}, //808 #big5-hkscs
    {0x8D51, 0x456D}, //809 #big5-hkscs
    {0x8D52, 0x38D4}, //810 #big5-hkscs
    {0x8D53, 0x2549A}, //811 #big5-hkscs
    {0x8D54, 0x4561}, //812 #big5-hkscs
    {0x8D55, 0x451B}, //813 #big5-hkscs
    {0x8D56, 0x4D89}, //814 #big5-hkscs
    {0x8D57, 0x4C7B}, //815 #big5-hkscs
    {0x8D58, 0x4D76}, //816 #big5-hkscs
    {0x8D59, 0x45EA}, //817 #big5-hkscs
    {0x8D5A, 0x3FC8}, //818 #big5-hkscs
    {0x8D5B, 0x24B0F}, //819 #big5-hkscs
    {0x8D5C, 0x3661}, //820 #big5-hkscs
    {0x8D5D, 0x44DE}, //821 #big5-hkscs
    {0x8D5E, 0x44BD}, //822 #big5-hkscs
    {0x8D5F, 0x41ED}, //823 #big5-hkscs
    {0x8D60, 0x5D3E}, //824 #big5-hkscs
    {0x8D61, 0x5D48}, //825 #big5-hkscs
    {0x8D62, 0x5D56}, //826 #big5-hkscs
    {0x8D63, 0x3DFC}, //827 #big5-hkscs
    {0x8D64, 0x380F}, //828 #big5-hkscs
    {0x8D65, 0x5DA4}, //829 #big5-hkscs
    {0x8D66, 0x5DB9}, //830 #big5-hkscs
    {0x8D67, 0x3820}, //831 #big5-hkscs
    {0x8D68, 0x3838}, //832 #big5-hkscs
    {0x8D69, 0x5E42}, //833 #big5-hkscs
    {0x8D6A, 0x5EBD}, //834 #big5-hkscs
    {0x8D6B, 0x5F25}, //835 #big5-hkscs
    {0x8D6C, 0x5F83}, //836 #big5-hkscs
    {0x8D6D, 0x3908}, //837 #big5-hkscs
    {0x8D6E, 0x3914}, //838 #big5-hkscs
    {0x8D6F, 0x393F}, //839 #big5-hkscs
    {0x8D70, 0x394D}, //840 #big5-hkscs
    {0x8D71, 0x60D7}, //841 #big5-hkscs
    {0x8D72, 0x613D}, //842 #big5-hkscs
    {0x8D73, 0x5CE5}, //843 #big5-hkscs
    {0x8D74, 0x3989}, //844 #big5-hkscs
    {0x8D75, 0x61B7}, //845 #big5-hkscs
    {0x8D76, 0x61B9}, //846 #big5-hkscs
    {0x8D77, 0x61CF}, //847 #big5-hkscs
    {0x8D78, 0x39B8}, //848 #big5-hkscs
    {0x8D79, 0x622C}, //849 #big5-hkscs
    {0x8D7A, 0x6290}, //850 #big5-hkscs
    {0x8D7B, 0x62E5}, //851 #big5-hkscs
    {0x8D7C, 0x6318}, //852 #big5-hkscs
    {0x8D7D, 0x39F8}, //853 #big5-hkscs
    {0x8D7E, 0x56B1}, //854 #big5-hkscs
    {0x8DA1, 0x3A03}, //855 #big5-hkscs
    {0x8DA2, 0x63E2}, //856 #big5-hkscs
    {0x8DA3, 0x63FB}, //857 #big5-hkscs
    {0x8DA4, 0x6407}, //858 #big5-hkscs
    {0x8DA5, 0x645A}, //859 #big5-hkscs
    {0x8DA6, 0x3A4B}, //860 #big5-hkscs
    {0x8DA7, 0x64C0}, //861 #big5-hkscs
    {0x8DA8, 0x5D15}, //862 #big5-hkscs
    {0x8DA9, 0x5621}, //863 #big5-hkscs
    {0x8DAA, 0x9F9F}, //864 #big5-hkscs
    {0x8DAB, 0x3A97}, //865 #big5-hkscs
    {0x8DAC, 0x6586}, //866 #big5-hkscs
    {0x8DAD, 0x3ABD}, //867 #big5-hkscs
    {0x8DAE, 0x65FF}, //868 #big5-hkscs
    {0x8DAF, 0x6653}, //869 #big5-hkscs
    {0x8DB0, 0x3AF2}, //870 #big5-hkscs
    {0x8DB1, 0x6692}, //871 #big5-hkscs
    {0x8DB2, 0x3B22}, //872 #big5-hkscs
    {0x8DB3, 0x6716}, //873 #big5-hkscs
    {0x8DB4, 0x3B42}, //874 #big5-hkscs
    {0x8DB5, 0x67A4}, //875 #big5-hkscs
    {0x8DB6, 0x6800}, //876 #big5-hkscs
    {0x8DB7, 0x3B58}, //877 #big5-hkscs
    {0x8DB8, 0x684A}, //878 #big5-hkscs
    {0x8DB9, 0x6884}, //879 #big5-hkscs
    {0x8DBA, 0x3B72}, //880 #big5-hkscs
    {0x8DBB, 0x3B71}, //881 #big5-hkscs
    {0x8DBC, 0x3B7B}, //882 #big5-hkscs
    {0x8DBD, 0x6909}, //883 #big5-hkscs
    {0x8DBE, 0x6943}, //884 #big5-hkscs
    {0x8DBF, 0x725C}, //885 #big5-hkscs
    {0x8DC0, 0x6964}, //886 #big5-hkscs
    {0x8DC1, 0x699F}, //887 #big5-hkscs
    {0x8DC2, 0x6985}, //888 #big5-hkscs
    {0x8DC3, 0x3BBC}, //889 #big5-hkscs
    {0x8DC4, 0x69D6}, //890 #big5-hkscs
    {0x8DC5, 0x3BDD}, //891 #big5-hkscs
    {0x8DC6, 0x6A65}, //892 #big5-hkscs
    {0x8DC7, 0x6A74}, //893 #big5-hkscs
    {0x8DC8, 0x6A71}, //894 #big5-hkscs
    {0x8DC9, 0x6A82}, //895 #big5-hkscs
    {0x8DCA, 0x3BEC}, //896 #big5-hkscs
    {0x8DCB, 0x6A99}, //897 #big5-hkscs
    {0x8DCC, 0x3BF2}, //898 #big5-hkscs
    {0x8DCD, 0x6AAB}, //899 #big5-hkscs
    {0x8DCE, 0x6AB5}, //900 #big5-hkscs
    {0x8DCF, 0x6AD4}, //901 #big5-hkscs
    {0x8DD0, 0x6AF6}, //902 #big5-hkscs
    {0x8DD1, 0x6B81}, //903 #big5-hkscs
    {0x8DD2, 0x6BC1}, //904 #big5-hkscs
    {0x8DD3, 0x6BEA}, //905 #big5-hkscs
    {0x8DD4, 0x6C75}, //906 #big5-hkscs
    {0x8DD5, 0x6CAA}, //907 #big5-hkscs
    {0x8DD6, 0x3CCB}, //908 #big5-hkscs
    {0x8DD7, 0x6D02}, //909 #big5-hkscs
    {0x8DD8, 0x6D06}, //910 #big5-hkscs
    {0x8DD9, 0x6D26}, //911 #big5-hkscs
    {0x8DDA, 0x6D81}, //912 #big5-hkscs
    {0x8DDB, 0x3CEF}, //913 #big5-hkscs
    {0x8DDC, 0x6DA4}, //914 #big5-hkscs
    {0x8DDD, 0x6DB1}, //915 #big5-hkscs
    {0x8DDE, 0x6E15}, //916 #big5-hkscs
    {0x8DDF, 0x6E18}, //917 #big5-hkscs
    {0x8DE0, 0x6E29}, //918 #big5-hkscs
    {0x8DE1, 0x6E86}, //919 #big5-hkscs
    {0x8DE2, 0x289C0}, //920 #big5-hkscs
    {0x8DE3, 0x6EBB}, //921 #big5-hkscs
    {0x8DE4, 0x6EE2}, //922 #big5-hkscs
    {0x8DE5, 0x6EDA}, //923 #big5-hkscs
    {0x8DE6, 0x9F7F}, //924 #big5-hkscs
    {0x8DE7, 0x6EE8}, //925 #big5-hkscs
    {0x8DE8, 0x6EE9}, //926 #big5-hkscs
    {0x8DE9, 0x6F24}, //927 #big5-hkscs
    {0x8DEA, 0x6F34}, //928 #big5-hkscs
    {0x8DEB, 0x3D46}, //929 #big5-hkscs
    {0x8DEC, 0x23F41}, //930 #big5-hkscs
    {0x8DED, 0x6F81}, //931 #big5-hkscs
    {0x8DEE, 0x6FBE}, //932 #big5-hkscs
    {0x8DEF, 0x3D6A}, //933 #big5-hkscs
    {0x8DF0, 0x3D75}, //934 #big5-hkscs
    {0x8DF1, 0x71B7}, //935 #big5-hkscs
    {0x8DF2, 0x5C99}, //936 #big5-hkscs
    {0x8DF3, 0x3D8A}, //937 #big5-hkscs
    {0x8DF4, 0x702C}, //938 #big5-hkscs
    {0x8DF5, 0x3D91}, //939 #big5-hkscs
    {0x8DF6, 0x7050}, //940 #big5-hkscs
    {0x8DF7, 0x7054}, //941 #big5-hkscs
    {0x8DF8, 0x706F}, //942 #big5-hkscs
    {0x8DF9, 0x707F}, //943 #big5-hkscs
    {0x8DFA, 0x7089}, //944 #big5-hkscs
    {0x8DFB, 0x20325}, //945 #big5-hkscs
    {0x8DFC, 0x43C1}, //946 #big5-hkscs
    {0x8DFD, 0x35F1}, //947 #big5-hkscs
    {0x8DFE, 0x20ED8}, //948 #big5-hkscs
    {0x8E40, 0x23ED7}, //949 #big5-hkscs
    {0x8E41, 0x57BE}, //950 #big5-hkscs
    {0x8E42, 0x26ED3}, //951 #big5-hkscs
    {0x8E43, 0x713E}, //952 #big5-hkscs
    {0x8E44, 0x257E0}, //953 #big5-hkscs
    {0x8E45, 0x364E}, //954 #big5-hkscs
    {0x8E46, 0x69A2}, //955 #big5-hkscs
    {0x8E47, 0x28BE9}, //956 #big5-hkscs
    {0x8E48, 0x5B74}, //957 #big5-hkscs
    {0x8E49, 0x7A49}, //958 #big5-hkscs
    {0x8E4A, 0x258E1}, //959 #big5-hkscs
    {0x8E4B, 0x294D9}, //960 #big5-hkscs
    {0x8E4C, 0x7A65}, //961 #big5-hkscs
    {0x8E4D, 0x7A7D}, //962 #big5-hkscs
    {0x8E4E, 0x259AC}, //963 #big5-hkscs
    {0x8E4F, 0x7ABB}, //964 #big5-hkscs
    {0x8E50, 0x7AB0}, //965 #big5-hkscs
    {0x8E51, 0x7AC2}, //966 #big5-hkscs
    {0x8E52, 0x7AC3}, //967 #big5-hkscs
    {0x8E53, 0x71D1}, //968 #big5-hkscs
    {0x8E54, 0x2648D}, //969 #big5-hkscs
    {0x8E55, 0x41CA}, //970 #big5-hkscs
    {0x8E56, 0x7ADA}, //971 #big5-hkscs
    {0x8E57, 0x7ADD}, //972 #big5-hkscs
    {0x8E58, 0x7AEA}, //973 #big5-hkscs
    {0x8E59, 0x41EF}, //974 #big5-hkscs
    {0x8E5A, 0x54B2}, //975 #big5-hkscs
    {0x8E5B, 0x25C01}, //976 #big5-hkscs
    {0x8E5C, 0x7B0B}, //977 #big5-hkscs
    {0x8E5D, 0x7B55}, //978 #big5-hkscs
    {0x8E5E, 0x7B29}, //979 #big5-hkscs
    {0x8E5F, 0x2530E}, //980 #big5-hkscs
    {0x8E60, 0x25CFE}, //981 #big5-hkscs
    {0x8E61, 0x7BA2}, //982 #big5-hkscs
    {0x8E62, 0x7B6F}, //983 #big5-hkscs
    {0x8E63, 0x839C}, //984 #big5-hkscs
    {0x8E64, 0x25BB4}, //985 #big5-hkscs
    {0x8E65, 0x26C7F}, //986 #big5-hkscs
    {0x8E66, 0x7BD0}, //987 #big5-hkscs
    {0x8E67, 0x8421}, //988 #big5-hkscs
    {0x8E68, 0x7B92}, //989 #big5-hkscs
    {0x8E69, 0x7BB8}, //990 #big5-hkscs
    {0x8E6A, 0x25D20}, //991 #big5-hkscs
    {0x8E6B, 0x3DAD}, //992 #big5-hkscs
    {0x8E6C, 0x25C65}, //993 #big5-hkscs
    {0x8E6D, 0x8492}, //994 #big5-hkscs
    {0x8E6E, 0x7BFA}, //995 #big5-hkscs
    {0x8E6F, 0x7C06}, //996 #big5-hkscs
    {0x8E70, 0x7C35}, //997 #big5-hkscs
    {0x8E71, 0x25CC1}, //998 #big5-hkscs
    {0x8E72, 0x7C44}, //999 #big5-hkscs
    {0x8E73, 0x7C83}, //1000 #big5-hkscs
    {0x8E74, 0x24882}, //1001 #big5-hkscs
    {0x8E75, 0x7CA6}, //1002 #big5-hkscs
    {0x8E76, 0x667D}, //1003 #big5-hkscs
    {0x8E77, 0x24578}, //1004 #big5-hkscs
    {0x8E78, 0x7CC9}, //1005 #big5-hkscs
    {0x8E79, 0x7CC7}, //1006 #big5-hkscs
    {0x8E7A, 0x7CE6}, //1007 #big5-hkscs
    {0x8E7B, 0x7C74}, //1008 #big5-hkscs
    {0x8E7C, 0x7CF3}, //1009 #big5-hkscs
    {0x8E7D, 0x7CF5}, //1010 #big5-hkscs
    {0x8E7E, 0x7CCE}, //1011 #big5-hkscs
    {0x8EA1, 0x7E67}, //1012 #big5-hkscs
    {0x8EA2, 0x451D}, //1013 #big5-hkscs
    {0x8EA3, 0x26E44}, //1014 #big5-hkscs
    {0x8EA4, 0x7D5D}, //1015 #big5-hkscs
    {0x8EA5, 0x26ED6}, //1016 #big5-hkscs
    {0x8EA6, 0x748D}, //1017 #big5-hkscs
    {0x8EA7, 0x7D89}, //1018 #big5-hkscs
    {0x8EA8, 0x7DAB}, //1019 #big5-hkscs
    {0x8EA9, 0x7135}, //1020 #big5-hkscs
    {0x8EAA, 0x7DB3}, //1021 #big5-hkscs
    {0x8EAB, 0x7DD2}, //1022 #big5-hkscs
    {0x8EAC, 0x24057}, //1023 #big5-hkscs
    {0x8EAD, 0x26029}, //1024 #big5-hkscs
    {0x8EAE, 0x7DE4}, //1025 #big5-hkscs
    {0x8EAF, 0x3D13}, //1026 #big5-hkscs
    {0x8EB0, 0x7DF5}, //1027 #big5-hkscs
    {0x8EB1, 0x217F9}, //1028 #big5-hkscs
    {0x8EB2, 0x7DE5}, //1029 #big5-hkscs
    {0x8EB3, 0x2836D}, //1030 #big5-hkscs
    {0x8EB4, 0x7E1D}, //1031 #big5-hkscs
    {0x8EB5, 0x26121}, //1032 #big5-hkscs
    {0x8EB6, 0x2615A}, //1033 #big5-hkscs
    {0x8EB7, 0x7E6E}, //1034 #big5-hkscs
    {0x8EB8, 0x7E92}, //1035 #big5-hkscs
    {0x8EB9, 0x432B}, //1036 #big5-hkscs
    {0x8EBA, 0x946C}, //1037 #big5-hkscs
    {0x8EBB, 0x7E27}, //1038 #big5-hkscs
    {0x8EBC, 0x7F40}, //1039 #big5-hkscs
    {0x8EBD, 0x7F41}, //1040 #big5-hkscs
    {0x8EBE, 0x7F47}, //1041 #big5-hkscs
    {0x8EBF, 0x7936}, //1042 #big5-hkscs
    {0x8EC0, 0x262D0}, //1043 #big5-hkscs
    {0x8EC1, 0x99E1}, //1044 #big5-hkscs
    {0x8EC2, 0x7F97}, //1045 #big5-hkscs
    {0x8EC3, 0x26351}, //1046 #big5-hkscs
    {0x8EC4, 0x7FA3}, //1047 #big5-hkscs
    {0x8EC5, 0x21661}, //1048 #big5-hkscs
    {0x8EC6, 0x20068}, //1049 #big5-hkscs
    {0x8EC7, 0x455C}, //1050 #big5-hkscs
    {0x8EC8, 0x23766}, //1051 #big5-hkscs
    {0x8EC9, 0x4503}, //1052 #big5-hkscs
    {0x8ECA, 0x2833A}, //1053 #big5-hkscs
    {0x8ECB, 0x7FFA}, //1054 #big5-hkscs
    {0x8ECC, 0x26489}, //1055 #big5-hkscs
    {0x8ECD, 0x8005}, //1056 #big5-hkscs
    {0x8ECE, 0x8008}, //1057 #big5-hkscs
    {0x8ECF, 0x801D}, //1058 #big5-hkscs
    {0x8ED0, 0x8028}, //1059 #big5-hkscs
    {0x8ED1, 0x802F}, //1060 #big5-hkscs
    {0x8ED2, 0x2A087}, //1061 #big5-hkscs
    {0x8ED3, 0x26CC3}, //1062 #big5-hkscs
    {0x8ED4, 0x803B}, //1063 #big5-hkscs
    {0x8ED5, 0x803C}, //1064 #big5-hkscs
    {0x8ED6, 0x8061}, //1065 #big5-hkscs
    {0x8ED7, 0x22714}, //1066 #big5-hkscs
    {0x8ED8, 0x4989}, //1067 #big5-hkscs
    {0x8ED9, 0x26626}, //1068 #big5-hkscs
    {0x8EDA, 0x23DE3}, //1069 #big5-hkscs
    {0x8EDB, 0x266E8}, //1070 #big5-hkscs
    {0x8EDC, 0x6725}, //1071 #big5-hkscs
    {0x8EDD, 0x80A7}, //1072 #big5-hkscs
    {0x8EDE, 0x28A48}, //1073 #big5-hkscs
    {0x8EDF, 0x8107}, //1074 #big5-hkscs
    {0x8EE0, 0x811A}, //1075 #big5-hkscs
    {0x8EE1, 0x58B0}, //1076 #big5-hkscs
    {0x8EE2, 0x226F6}, //1077 #big5-hkscs
    {0x8EE3, 0x6C7F}, //1078 #big5-hkscs
    {0x8EE4, 0x26498}, //1079 #big5-hkscs
    {0x8EE5, 0x24FB8}, //1080 #big5-hkscs
    {0x8EE6, 0x64E7}, //1081 #big5-hkscs
    {0x8EE7, 0x2148A}, //1082 #big5-hkscs
    {0x8EE8, 0x8218}, //1083 #big5-hkscs
    {0x8EE9, 0x2185E}, //1084 #big5-hkscs
    {0x8EEA, 0x6A53}, //1085 #big5-hkscs
    {0x8EEB, 0x24A65}, //1086 #big5-hkscs
    {0x8EEC, 0x24A95}, //1087 #big5-hkscs
    {0x8EED, 0x447A}, //1088 #big5-hkscs
    {0x8EEE, 0x8229}, //1089 #big5-hkscs
    {0x8EEF, 0x20B0D}, //1090 #big5-hkscs
    {0x8EF0, 0x26A52}, //1091 #big5-hkscs
    {0x8EF1, 0x23D7E}, //1092 #big5-hkscs
    {0x8EF2, 0x4FF9}, //1093 #big5-hkscs
    {0x8EF3, 0x214FD}, //1094 #big5-hkscs
    {0x8EF4, 0x84E2}, //1095 #big5-hkscs
    {0x8EF5, 0x8362}, //1096 #big5-hkscs
    {0x8EF6, 0x26B0A}, //1097 #big5-hkscs
    {0x8EF7, 0x249A7}, //1098 #big5-hkscs
    {0x8EF8, 0x23530}, //1099 #big5-hkscs
    {0x8EF9, 0x21773}, //1100 #big5-hkscs
    {0x8EFA, 0x23DF8}, //1101 #big5-hkscs
    {0x8EFB, 0x82AA}, //1102 #big5-hkscs
    {0x8EFC, 0x691B}, //1103 #big5-hkscs
    {0x8EFD, 0x2F994}, //1104 #big5-hkscs
    {0x8EFE, 0x41DB}, //1105 #big5-hkscs
    {0x8F40, 0x854B}, //1106 #big5-hkscs
    {0x8F41, 0x82D0}, //1107 #big5-hkscs
    {0x8F42, 0x831A}, //1108 #big5-hkscs
    {0x8F43, 0x20E16}, //1109 #big5-hkscs
    {0x8F44, 0x217B4}, //1110 #big5-hkscs
    {0x8F45, 0x36C1}, //1111 #big5-hkscs
    {0x8F46, 0x2317D}, //1112 #big5-hkscs
    {0x8F47, 0x2355A}, //1113 #big5-hkscs
    {0x8F48, 0x827B}, //1114 #big5-hkscs
    {0x8F49, 0x82E2}, //1115 #big5-hkscs
    {0x8F4A, 0x8318}, //1116 #big5-hkscs
    {0x8F4B, 0x23E8B}, //1117 #big5-hkscs
    {0x8F4C, 0x26DA3}, //1118 #big5-hkscs
    {0x8F4D, 0x26B05}, //1119 #big5-hkscs
    {0x8F4E, 0x26B97}, //1120 #big5-hkscs
    {0x8F4F, 0x235CE}, //1121 #big5-hkscs
    {0x8F50, 0x3DBF}, //1122 #big5-hkscs
    {0x8F51, 0x831D}, //1123 #big5-hkscs
    {0x8F52, 0x55EC}, //1124 #big5-hkscs
    {0x8F53, 0x8385}, //1125 #big5-hkscs
    {0x8F54, 0x450B}, //1126 #big5-hkscs
    {0x8F55, 0x26DA5}, //1127 #big5-hkscs
    {0x8F56, 0x83AC}, //1128 #big5-hkscs
    {0x8F57, 0x83C1}, //1129 #big5-hkscs
    {0x8F58, 0x83D3}, //1130 #big5-hkscs
    {0x8F59, 0x347E}, //1131 #big5-hkscs
    {0x8F5A, 0x26ED4}, //1132 #big5-hkscs
    {0x8F5B, 0x6A57}, //1133 #big5-hkscs
    {0x8F5C, 0x855A}, //1134 #big5-hkscs
    {0x8F5D, 0x3496}, //1135 #big5-hkscs
    {0x8F5E, 0x26E42}, //1136 #big5-hkscs
    {0x8F5F, 0x22EEF}, //1137 #big5-hkscs
    {0x8F60, 0x8458}, //1138 #big5-hkscs
    {0x8F61, 0x25BE4}, //1139 #big5-hkscs
    {0x8F62, 0x8471}, //1140 #big5-hkscs
    {0x8F63, 0x3DD3}, //1141 #big5-hkscs
    {0x8F64, 0x44E4}, //1142 #big5-hkscs
    {0x8F65, 0x6AA7}, //1143 #big5-hkscs
    {0x8F66, 0x844A}, //1144 #big5-hkscs
    {0x8F67, 0x23CB5}, //1145 #big5-hkscs
    {0x8F68, 0x7958}, //1146 #big5-hkscs
    {0x8F69, 0x84A8}, //1147 #big5-hkscs
    {0x8F6A, 0x26B96}, //1148 #big5-hkscs
    {0x8F6B, 0x26E77}, //1149 #big5-hkscs
    {0x8F6C, 0x26E43}, //1150 #big5-hkscs
    {0x8F6D, 0x84DE}, //1151 #big5-hkscs
    {0x8F6E, 0x840F}, //1152 #big5-hkscs
    {0x8F6F, 0x8391}, //1153 #big5-hkscs
    {0x8F70, 0x44A0}, //1154 #big5-hkscs
    {0x8F71, 0x8493}, //1155 #big5-hkscs
    {0x8F72, 0x84E4}, //1156 #big5-hkscs
    {0x8F73, 0x25C91}, //1157 #big5-hkscs
    {0x8F74, 0x4240}, //1158 #big5-hkscs
    {0x8F75, 0x25CC0}, //1159 #big5-hkscs
    {0x8F76, 0x4543}, //1160 #big5-hkscs
    {0x8F77, 0x8534}, //1161 #big5-hkscs
    {0x8F78, 0x5AF2}, //1162 #big5-hkscs
    {0x8F79, 0x26E99}, //1163 #big5-hkscs
    {0x8F7A, 0x4527}, //1164 #big5-hkscs
    {0x8F7B, 0x8573}, //1165 #big5-hkscs
    {0x8F7C, 0x4516}, //1166 #big5-hkscs
    {0x8F7D, 0x67BF}, //1167 #big5-hkscs
    {0x8F7E, 0x8616}, //1168 #big5-hkscs
    {0x8FA1, 0x28625}, //1169 #big5-hkscs
    {0x8FA2, 0x2863B}, //1170 #big5-hkscs
    {0x8FA3, 0x85C1}, //1171 #big5-hkscs
    {0x8FA4, 0x27088}, //1172 #big5-hkscs
    {0x8FA5, 0x8602}, //1173 #big5-hkscs
    {0x8FA6, 0x21582}, //1174 #big5-hkscs
    {0x8FA7, 0x270CD}, //1175 #big5-hkscs
    {0x8FA8, 0x2F9B2}, //1176 #big5-hkscs
    {0x8FA9, 0x456A}, //1177 #big5-hkscs
    {0x8FAA, 0x8628}, //1178 #big5-hkscs
    {0x8FAB, 0x3648}, //1179 #big5-hkscs
    {0x8FAC, 0x218A2}, //1180 #big5-hkscs
    {0x8FAD, 0x53F7}, //1181 #big5-hkscs
    {0x8FAE, 0x2739A}, //1182 #big5-hkscs
    {0x8FAF, 0x867E}, //1183 #big5-hkscs
    {0x8FB0, 0x8771}, //1184 #big5-hkscs
    {0x8FB1, 0x2A0F8}, //1185 #big5-hkscs
    {0x8FB2, 0x87EE}, //1186 #big5-hkscs
    {0x8FB3, 0x22C27}, //1187 #big5-hkscs
    {0x8FB4, 0x87B1}, //1188 #big5-hkscs
    {0x8FB5, 0x87DA}, //1189 #big5-hkscs
    {0x8FB6, 0x880F}, //1190 #big5-hkscs
    {0x8FB7, 0x5661}, //1191 #big5-hkscs
    {0x8FB8, 0x866C}, //1192 #big5-hkscs
    {0x8FB9, 0x6856}, //1193 #big5-hkscs
    {0x8FBA, 0x460F}, //1194 #big5-hkscs
    {0x8FBB, 0x8845}, //1195 #big5-hkscs
    {0x8FBC, 0x8846}, //1196 #big5-hkscs
    {0x8FBD, 0x275E0}, //1197 #big5-hkscs
    {0x8FBE, 0x23DB9}, //1198 #big5-hkscs
    {0x8FBF, 0x275E4}, //1199 #big5-hkscs
    {0x8FC0, 0x885E}, //1200 #big5-hkscs
    {0x8FC1, 0x889C}, //1201 #big5-hkscs
    {0x8FC2, 0x465B}, //1202 #big5-hkscs
    {0x8FC3, 0x88B4}, //1203 #big5-hkscs
    {0x8FC4, 0x88B5}, //1204 #big5-hkscs
    {0x8FC5, 0x63C1}, //1205 #big5-hkscs
    {0x8FC6, 0x88C5}, //1206 #big5-hkscs
    {0x8FC7, 0x7777}, //1207 #big5-hkscs
    {0x8FC8, 0x2770F}, //1208 #big5-hkscs
    {0x8FC9, 0x8987}, //1209 #big5-hkscs
    {0x8FCA, 0x898A}, //1210 #big5-hkscs
    {0x8FCB, 0x89A6}, //1211 #big5-hkscs
    {0x8FCC, 0x89A9}, //1212 #big5-hkscs
    {0x8FCD, 0x89A7}, //1213 #big5-hkscs
    {0x8FCE, 0x89BC}, //1214 #big5-hkscs
    {0x8FCF, 0x28A25}, //1215 #big5-hkscs
    {0x8FD0, 0x89E7}, //1216 #big5-hkscs
    {0x8FD1, 0x27924}, //1217 #big5-hkscs
    {0x8FD2, 0x27ABD}, //1218 #big5-hkscs
    {0x8FD3, 0x8A9C}, //1219 #big5-hkscs
    {0x8FD4, 0x7793}, //1220 #big5-hkscs
    {0x8FD5, 0x91FE}, //1221 #big5-hkscs
    {0x8FD6, 0x8A90}, //1222 #big5-hkscs
    {0x8FD7, 0x27A59}, //1223 #big5-hkscs
    {0x8FD8, 0x7AE9}, //1224 #big5-hkscs
    {0x8FD9, 0x27B3A}, //1225 #big5-hkscs
    {0x8FDA, 0x23F8F}, //1226 #big5-hkscs
    {0x8FDB, 0x4713}, //1227 #big5-hkscs
    {0x8FDC, 0x27B38}, //1228 #big5-hkscs
    {0x8FDD, 0x717C}, //1229 #big5-hkscs
    {0x8FDE, 0x8B0C}, //1230 #big5-hkscs
    {0x8FDF, 0x8B1F}, //1231 #big5-hkscs
    {0x8FE0, 0x25430}, //1232 #big5-hkscs
    {0x8FE1, 0x25565}, //1233 #big5-hkscs
    {0x8FE2, 0x8B3F}, //1234 #big5-hkscs
    {0x8FE3, 0x8B4C}, //1235 #big5-hkscs
    {0x8FE4, 0x8B4D}, //1236 #big5-hkscs
    {0x8FE5, 0x8AA9}, //1237 #big5-hkscs
    {0x8FE6, 0x24A7A}, //1238 #big5-hkscs
    {0x8FE7, 0x8B90}, //1239 #big5-hkscs
    {0x8FE8, 0x8B9B}, //1240 #big5-hkscs
    {0x8FE9, 0x8AAF}, //1241 #big5-hkscs
    {0x8FEA, 0x216DF}, //1242 #big5-hkscs
    {0x8FEB, 0x4615}, //1243 #big5-hkscs
    {0x8FEC, 0x884F}, //1244 #big5-hkscs
    {0x8FED, 0x8C9B}, //1245 #big5-hkscs
    {0x8FEE, 0x27D54}, //1246 #big5-hkscs
    {0x8FEF, 0x27D8F}, //1247 #big5-hkscs
    {0x8FF0, 0x2F9D4}, //1248 #big5-hkscs
    {0x8FF1, 0x3725}, //1249 #big5-hkscs
    {0x8FF2, 0x27D53}, //1250 #big5-hkscs
    {0x8FF3, 0x8CD6}, //1251 #big5-hkscs
    {0x8FF4, 0x27D98}, //1252 #big5-hkscs
    {0x8FF5, 0x27DBD}, //1253 #big5-hkscs
    {0x8FF6, 0x8D12}, //1254 #big5-hkscs
    {0x8FF7, 0x8D03}, //1255 #big5-hkscs
    {0x8FF8, 0x21910}, //1256 #big5-hkscs
    {0x8FF9, 0x8CDB}, //1257 #big5-hkscs
    {0x8FFA, 0x705C}, //1258 #big5-hkscs
    {0x8FFB, 0x8D11}, //1259 #big5-hkscs
    {0x8FFC, 0x24CC9}, //1260 #big5-hkscs
    {0x8FFD, 0x3ED0}, //1261 #big5-hkscs
    {0x8FFE, 0x8D77}, //1262 #big5-hkscs
    {0x9040, 0x8DA9}, //1263 #big5-hkscs
    {0x9041, 0x28002}, //1264 #big5-hkscs
    {0x9042, 0x21014}, //1265 #big5-hkscs
    {0x9043, 0x2498A}, //1266 #big5-hkscs
    {0x9044, 0x3B7C}, //1267 #big5-hkscs
    {0x9045, 0x281BC}, //1268 #big5-hkscs
    {0x9046, 0x2710C}, //1269 #big5-hkscs
    {0x9047, 0x7AE7}, //1270 #big5-hkscs
    {0x9048, 0x8EAD}, //1271 #big5-hkscs
    {0x9049, 0x8EB6}, //1272 #big5-hkscs
    {0x904A, 0x8EC3}, //1273 #big5-hkscs
    {0x904B, 0x92D4}, //1274 #big5-hkscs
    {0x904C, 0x8F19}, //1275 #big5-hkscs
    {0x904D, 0x8F2D}, //1276 #big5-hkscs
    {0x904E, 0x28365}, //1277 #big5-hkscs
    {0x904F, 0x28412}, //1278 #big5-hkscs
    {0x9050, 0x8FA5}, //1279 #big5-hkscs
    {0x9051, 0x9303}, //1280 #big5-hkscs
    {0x9052, 0x2A29F}, //1281 #big5-hkscs
    {0x9053, 0x20A50}, //1282 #big5-hkscs
    {0x9054, 0x8FB3}, //1283 #big5-hkscs
    {0x9055, 0x492A}, //1284 #big5-hkscs
    {0x9056, 0x289DE}, //1285 #big5-hkscs
    {0x9057, 0x2853D}, //1286 #big5-hkscs
    {0x9058, 0x23DBB}, //1287 #big5-hkscs
    {0x9059, 0x5EF8}, //1288 #big5-hkscs
    {0x905A, 0x23262}, //1289 #big5-hkscs
    {0x905B, 0x8FF9}, //1290 #big5-hkscs
    {0x905C, 0x2A014}, //1291 #big5-hkscs
    {0x905D, 0x286BC}, //1292 #big5-hkscs
    {0x905E, 0x28501}, //1293 #big5-hkscs
    {0x905F, 0x22325}, //1294 #big5-hkscs
    {0x9060, 0x3980}, //1295 #big5-hkscs
    {0x9061, 0x26ED7}, //1296 #big5-hkscs
    {0x9062, 0x9037}, //1297 #big5-hkscs
    {0x9063, 0x2853C}, //1298 #big5-hkscs
    {0x9064, 0x27ABE}, //1299 #big5-hkscs
    {0x9065, 0x9061}, //1300 #big5-hkscs
    {0x9066, 0x2856C}, //1301 #big5-hkscs
    {0x9067, 0x2860B}, //1302 #big5-hkscs
    {0x9068, 0x90A8}, //1303 #big5-hkscs
    {0x9069, 0x28713}, //1304 #big5-hkscs
    {0x906A, 0x90C4}, //1305 #big5-hkscs
    {0x906B, 0x286E6}, //1306 #big5-hkscs
    {0x906C, 0x90AE}, //1307 #big5-hkscs
    {0x906D, 0x90FD}, //1308 #big5-hkscs
    {0x906E, 0x9167}, //1309 #big5-hkscs
    {0x906F, 0x3AF0}, //1310 #big5-hkscs
    {0x9070, 0x91A9}, //1311 #big5-hkscs
    {0x9071, 0x91C4}, //1312 #big5-hkscs
    {0x9072, 0x7CAC}, //1313 #big5-hkscs
    {0x9073, 0x28933}, //1314 #big5-hkscs
    {0x9074, 0x21E89}, //1315 #big5-hkscs
    {0x9075, 0x920E}, //1316 #big5-hkscs
    {0x9076, 0x6C9F}, //1317 #big5-hkscs
    {0x9077, 0x9241}, //1318 #big5-hkscs
    {0x9078, 0x9262}, //1319 #big5-hkscs
    {0x9079, 0x255B9}, //1320 #big5-hkscs
    {0x907A, 0x92B9}, //1321 #big5-hkscs
    {0x907B, 0x28AC6}, //1322 #big5-hkscs
    {0x907C, 0x23C9B}, //1323 #big5-hkscs
    {0x907D, 0x28B0C}, //1324 #big5-hkscs
    {0x907E, 0x255DB}, //1325 #big5-hkscs
    {0x90A1, 0x20D31}, //1326 #big5-hkscs
    {0x90A2, 0x932C}, //1327 #big5-hkscs
    {0x90A3, 0x936B}, //1328 #big5-hkscs
    {0x90A4, 0x28AE1}, //1329 #big5-hkscs
    {0x90A5, 0x28BEB}, //1330 #big5-hkscs
    {0x90A6, 0x708F}, //1331 #big5-hkscs
    {0x90A7, 0x5AC3}, //1332 #big5-hkscs
    {0x90A8, 0x28AE2}, //1333 #big5-hkscs
    {0x90A9, 0x28AE5}, //1334 #big5-hkscs
    {0x90AA, 0x4965}, //1335 #big5-hkscs
    {0x90AB, 0x9244}, //1336 #big5-hkscs
    {0x90AC, 0x28BEC}, //1337 #big5-hkscs
    {0x90AD, 0x28C39}, //1338 #big5-hkscs
    {0x90AE, 0x28BFF}, //1339 #big5-hkscs
    {0x90AF, 0x9373}, //1340 #big5-hkscs
    {0x90B0, 0x945B}, //1341 #big5-hkscs
    {0x90B1, 0x8EBC}, //1342 #big5-hkscs
    {0x90B2, 0x9585}, //1343 #big5-hkscs
    {0x90B3, 0x95A6}, //1344 #big5-hkscs
    {0x90B4, 0x9426}, //1345 #big5-hkscs
    {0x90B5, 0x95A0}, //1346 #big5-hkscs
    {0x90B6, 0x6FF6}, //1347 #big5-hkscs
    {0x90B7, 0x42B9}, //1348 #big5-hkscs
    {0x90B8, 0x2267A}, //1349 #big5-hkscs
    {0x90B9, 0x286D8}, //1350 #big5-hkscs
    {0x90BA, 0x2127C}, //1351 #big5-hkscs
    {0x90BB, 0x23E2E}, //1352 #big5-hkscs
    {0x90BC, 0x49DF}, //1353 #big5-hkscs
    {0x90BD, 0x6C1C}, //1354 #big5-hkscs
    {0x90BE, 0x967B}, //1355 #big5-hkscs
    {0x90BF, 0x9696}, //1356 #big5-hkscs
    {0x90C0, 0x416C}, //1357 #big5-hkscs
    {0x90C1, 0x96A3}, //1358 #big5-hkscs
    {0x90C2, 0x26ED5}, //1359 #big5-hkscs
    {0x90C3, 0x61DA}, //1360 #big5-hkscs
    {0x90C4, 0x96B6}, //1361 #big5-hkscs
    {0x90C5, 0x78F5}, //1362 #big5-hkscs
    {0x90C6, 0x28AE0}, //1363 #big5-hkscs
    {0x90C7, 0x96BD}, //1364 #big5-hkscs
    {0x90C8, 0x53CC}, //1365 #big5-hkscs
    {0x90C9, 0x49A1}, //1366 #big5-hkscs
    {0x90CA, 0x26CB8}, //1367 #big5-hkscs
    {0x90CB, 0x20274}, //1368 #big5-hkscs
    {0x90CC, 0x26410}, //1369 #big5-hkscs
    {0x90CD, 0x290AF}, //1370 #big5-hkscs
    {0x90CE, 0x290E5}, //1371 #big5-hkscs
    {0x90CF, 0x24AD1}, //1372 #big5-hkscs
    {0x90D0, 0x21915}, //1373 #big5-hkscs
    {0x90D1, 0x2330A}, //1374 #big5-hkscs
    {0x90D2, 0x9731}, //1375 #big5-hkscs
    {0x90D3, 0x8642}, //1376 #big5-hkscs
    {0x90D4, 0x9736}, //1377 #big5-hkscs
    {0x90D5, 0x4A0F}, //1378 #big5-hkscs
    {0x90D6, 0x453D}, //1379 #big5-hkscs
    {0x90D7, 0x4585}, //1380 #big5-hkscs
    {0x90D8, 0x24AE9}, //1381 #big5-hkscs
    {0x90D9, 0x7075}, //1382 #big5-hkscs
    {0x90DA, 0x5B41}, //1383 #big5-hkscs
    {0x90DB, 0x971B}, //1384 #big5-hkscs
    {0x90DC, 0x975C}, //1385 #big5-hkscs
    {0x90DD, 0x291D5}, //1386 #big5-hkscs
    {0x90DE, 0x9757}, //1387 #big5-hkscs
    {0x90DF, 0x5B4A}, //1388 #big5-hkscs
    {0x90E0, 0x291EB}, //1389 #big5-hkscs
    {0x90E1, 0x975F}, //1390 #big5-hkscs
    {0x90E2, 0x9425}, //1391 #big5-hkscs
    {0x90E3, 0x50D0}, //1392 #big5-hkscs
    {0x90E4, 0x230B7}, //1393 #big5-hkscs
    {0x90E5, 0x230BC}, //1394 #big5-hkscs
    {0x90E6, 0x9789}, //1395 #big5-hkscs
    {0x90E7, 0x979F}, //1396 #big5-hkscs
    {0x90E8, 0x97B1}, //1397 #big5-hkscs
    {0x90E9, 0x97BE}, //1398 #big5-hkscs
    {0x90EA, 0x97C0}, //1399 #big5-hkscs
    {0x90EB, 0x97D2}, //1400 #big5-hkscs
    {0x90EC, 0x97E0}, //1401 #big5-hkscs
    {0x90ED, 0x2546C}, //1402 #big5-hkscs
    {0x90EE, 0x97EE}, //1403 #big5-hkscs
    {0x90EF, 0x741C}, //1404 #big5-hkscs
    {0x90F0, 0x29433}, //1405 #big5-hkscs
    {0x90F1, 0x97FF}, //1406 #big5-hkscs
    {0x90F2, 0x97F5}, //1407 #big5-hkscs
    {0x90F3, 0x2941D}, //1408 #big5-hkscs
    {0x90F4, 0x2797A}, //1409 #big5-hkscs
    {0x90F5, 0x4AD1}, //1410 #big5-hkscs
    {0x90F6, 0x9834}, //1411 #big5-hkscs
    {0x90F7, 0x9833}, //1412 #big5-hkscs
    {0x90F8, 0x984B}, //1413 #big5-hkscs
    {0x90F9, 0x9866}, //1414 #big5-hkscs
    {0x90FA, 0x3B0E}, //1415 #big5-hkscs
    {0x90FB, 0x27175}, //1416 #big5-hkscs
    {0x90FC, 0x3D51}, //1417 #big5-hkscs
    {0x90FD, 0x20630}, //1418 #big5-hkscs
    {0x90FE, 0x2415C}, //1419 #big5-hkscs
    {0x9140, 0x25706}, //1420 #big5-hkscs
    {0x9141, 0x98CA}, //1421 #big5-hkscs
    {0x9142, 0x98B7}, //1422 #big5-hkscs
    {0x9143, 0x98C8}, //1423 #big5-hkscs
    {0x9144, 0x98C7}, //1424 #big5-hkscs
    {0x9145, 0x4AFF}, //1425 #big5-hkscs
    {0x9146, 0x26D27}, //1426 #big5-hkscs
    {0x9147, 0x216D3}, //1427 #big5-hkscs
    {0x9148, 0x55B0}, //1428 #big5-hkscs
    {0x9149, 0x98E1}, //1429 #big5-hkscs
    {0x914A, 0x98E6}, //1430 #big5-hkscs
    {0x914B, 0x98EC}, //1431 #big5-hkscs
    {0x914C, 0x9378}, //1432 #big5-hkscs
    {0x914D, 0x9939}, //1433 #big5-hkscs
    {0x914E, 0x24A29}, //1434 #big5-hkscs
    {0x914F, 0x4B72}, //1435 #big5-hkscs
    {0x9150, 0x29857}, //1436 #big5-hkscs
    {0x9151, 0x29905}, //1437 #big5-hkscs
    {0x9152, 0x99F5}, //1438 #big5-hkscs
    {0x9153, 0x9A0C}, //1439 #big5-hkscs
    {0x9154, 0x9A3B}, //1440 #big5-hkscs
    {0x9155, 0x9A10}, //1441 #big5-hkscs
    {0x9156, 0x9A58}, //1442 #big5-hkscs
    {0x9157, 0x25725}, //1443 #big5-hkscs
    {0x9158, 0x36C4}, //1444 #big5-hkscs
    {0x9159, 0x290B1}, //1445 #big5-hkscs
    {0x915A, 0x29BD5}, //1446 #big5-hkscs
    {0x915B, 0x9AE0}, //1447 #big5-hkscs
    {0x915C, 0x9AE2}, //1448 #big5-hkscs
    {0x915D, 0x29B05}, //1449 #big5-hkscs
    {0x915E, 0x9AF4}, //1450 #big5-hkscs
    {0x915F, 0x4C0E}, //1451 #big5-hkscs
    {0x9160, 0x9B14}, //1452 #big5-hkscs
    {0x9161, 0x9B2D}, //1453 #big5-hkscs
    {0x9162, 0x28600}, //1454 #big5-hkscs
    {0x9163, 0x5034}, //1455 #big5-hkscs
    {0x9164, 0x9B34}, //1456 #big5-hkscs
    {0x9165, 0x269A8}, //1457 #big5-hkscs
    {0x9166, 0x38C3}, //1458 #big5-hkscs
    {0x9167, 0x2307D}, //1459 #big5-hkscs
    {0x9168, 0x9B50}, //1460 #big5-hkscs
    {0x9169, 0x9B40}, //1461 #big5-hkscs
    {0x916A, 0x29D3E}, //1462 #big5-hkscs
    {0x916B, 0x5A45}, //1463 #big5-hkscs
    {0x916C, 0x21863}, //1464 #big5-hkscs
    {0x916D, 0x9B8E}, //1465 #big5-hkscs
    {0x916E, 0x2424B}, //1466 #big5-hkscs
    {0x916F, 0x9C02}, //1467 #big5-hkscs
    {0x9170, 0x9BFF}, //1468 #big5-hkscs
    {0x9171, 0x9C0C}, //1469 #big5-hkscs
    {0x9172, 0x29E68}, //1470 #big5-hkscs
    {0x9173, 0x9DD4}, //1471 #big5-hkscs
    {0x9174, 0x29FB7}, //1472 #big5-hkscs
    {0x9175, 0x2A192}, //1473 #big5-hkscs
    {0x9176, 0x2A1AB}, //1474 #big5-hkscs
    {0x9177, 0x2A0E1}, //1475 #big5-hkscs
    {0x9178, 0x2A123}, //1476 #big5-hkscs
    {0x9179, 0x2A1DF}, //1477 #big5-hkscs
    {0x917A, 0x9D7E}, //1478 #big5-hkscs
    {0x917B, 0x9D83}, //1479 #big5-hkscs
    {0x917C, 0x2A134}, //1480 #big5-hkscs
    {0x917D, 0x9E0E}, //1481 #big5-hkscs
    {0x917E, 0x6888}, //1482 #big5-hkscs
    {0x91A1, 0x9DC4}, //1483 #big5-hkscs
    {0x91A2, 0x2215B}, //1484 #big5-hkscs
    {0x91A3, 0x2A193}, //1485 #big5-hkscs
    {0x91A4, 0x2A220}, //1486 #big5-hkscs
    {0x91A5, 0x2193B}, //1487 #big5-hkscs
    {0x91A6, 0x2A233}, //1488 #big5-hkscs
    {0x91A7, 0x9D39}, //1489 #big5-hkscs
    {0x91A8, 0x2A0B9}, //1490 #big5-hkscs
    {0x91A9, 0x2A2B4}, //1491 #big5-hkscs
    {0x91AA, 0x9E90}, //1492 #big5-hkscs
    {0x91AB, 0x9E95}, //1493 #big5-hkscs
    {0x91AC, 0x9E9E}, //1494 #big5-hkscs
    {0x91AD, 0x9EA2}, //1495 #big5-hkscs
    {0x91AE, 0x4D34}, //1496 #big5-hkscs
    {0x91AF, 0x9EAA}, //1497 #big5-hkscs
    {0x91B0, 0x9EAF}, //1498 #big5-hkscs
    {0x91B1, 0x24364}, //1499 #big5-hkscs
    {0x91B2, 0x9EC1}, //1500 #big5-hkscs
    {0x91B3, 0x3B60}, //1501 #big5-hkscs
    {0x91B4, 0x39E5}, //1502 #big5-hkscs
    {0x91B5, 0x3D1D}, //1503 #big5-hkscs
    {0x91B6, 0x4F32}, //1504 #big5-hkscs
    {0x91B7, 0x37BE}, //1505 #big5-hkscs
    {0x91B8, 0x28C2B}, //1506 #big5-hkscs
    {0x91B9, 0x9F02}, //1507 #big5-hkscs
    {0x91BA, 0x9F08}, //1508 #big5-hkscs
    {0x91BB, 0x4B96}, //1509 #big5-hkscs
    {0x91BC, 0x9424}, //1510 #big5-hkscs
    {0x91BD, 0x26DA2}, //1511 #big5-hkscs
    {0x91BE, 0x9F17}, //1512 #big5-hkscs
    {0x91BF, 0x9F16}, //1513 #big5-hkscs
    {0x91C0, 0x9F39}, //1514 #big5-hkscs
    {0x91C1, 0x569F}, //1515 #big5-hkscs
    {0x91C2, 0x568A}, //1516 #big5-hkscs
    {0x91C3, 0x9F45}, //1517 #big5-hkscs
    {0x91C4, 0x99B8}, //1518 #big5-hkscs
    {0x91C5, 0x2908B}, //1519 #big5-hkscs
    {0x91C6, 0x97F2}, //1520 #big5-hkscs
    {0x91C7, 0x847F}, //1521 #big5-hkscs
    {0x91C8, 0x9F62}, //1522 #big5-hkscs
    {0x91C9, 0x9F69}, //1523 #big5-hkscs
    {0x91CA, 0x7ADC}, //1524 #big5-hkscs
    {0x91CB, 0x9F8E}, //1525 #big5-hkscs
    {0x91CC, 0x7216}, //1526 #big5-hkscs
    {0x91CD, 0x4BBE}, //1527 #big5-hkscs
    {0x91CE, 0x24975}, //1528 #big5-hkscs
    {0x91CF, 0x249BB}, //1529 #big5-hkscs
    {0x91D0, 0x7177}, //1530 #big5-hkscs
    {0x91D1, 0x249F8}, //1531 #big5-hkscs
    {0x91D2, 0x24348}, //1532 #big5-hkscs
    {0x91D3, 0x24A51}, //1533 #big5-hkscs
    {0x91D4, 0x739E}, //1534 #big5-hkscs
    {0x91D5, 0x28BDA}, //1535 #big5-hkscs
    {0x91D6, 0x218FA}, //1536 #big5-hkscs
    {0x91D7, 0x799F}, //1537 #big5-hkscs
    {0x91D8, 0x2897E}, //1538 #big5-hkscs
    {0x91D9, 0x28E36}, //1539 #big5-hkscs
    {0x91DA, 0x9369}, //1540 #big5-hkscs
    {0x91DB, 0x93F3}, //1541 #big5-hkscs
    {0x91DC, 0x28A44}, //1542 #big5-hkscs
    {0x91DD, 0x92EC}, //1543 #big5-hkscs
    {0x91DE, 0x9381}, //1544 #big5-hkscs
    {0x91DF, 0x93CB}, //1545 #big5-hkscs
    {0x91E0, 0x2896C}, //1546 #big5-hkscs
    {0x91E1, 0x244B9}, //1547 #big5-hkscs
    {0x91E2, 0x7217}, //1548 #big5-hkscs
    {0x91E3, 0x3EEB}, //1549 #big5-hkscs
    {0x91E4, 0x7772}, //1550 #big5-hkscs
    {0x91E5, 0x7A43}, //1551 #big5-hkscs
    {0x91E6, 0x70D0}, //1552 #big5-hkscs
    {0x91E7, 0x24473}, //1553 #big5-hkscs
    {0x91E8, 0x243F8}, //1554 #big5-hkscs
    {0x91E9, 0x717E}, //1555 #big5-hkscs
    {0x91EA, 0x217EF}, //1556 #big5-hkscs
    {0x91EB, 0x70A3}, //1557 #big5-hkscs
    {0x91EC, 0x218BE}, //1558 #big5-hkscs
    {0x91ED, 0x23599}, //1559 #big5-hkscs
    {0x91EE, 0x3EC7}, //1560 #big5-hkscs
    {0x91EF, 0x21885}, //1561 #big5-hkscs
    {0x91F0, 0x2542F}, //1562 #big5-hkscs
    {0x91F1, 0x217F8}, //1563 #big5-hkscs
    {0x91F2, 0x3722}, //1564 #big5-hkscs
    {0x91F3, 0x216FB}, //1565 #big5-hkscs
    {0x91F4, 0x21839}, //1566 #big5-hkscs
    {0x91F5, 0x36E1}, //1567 #big5-hkscs
    {0x91F6, 0x21774}, //1568 #big5-hkscs
    {0x91F7, 0x218D1}, //1569 #big5-hkscs
    {0x91F8, 0x25F4B}, //1570 #big5-hkscs
    {0x91F9, 0x3723}, //1571 #big5-hkscs
    {0x91FA, 0x216C0}, //1572 #big5-hkscs
    {0x91FB, 0x575B}, //1573 #big5-hkscs
    {0x91FC, 0x24A25}, //1574 #big5-hkscs
    {0x91FD, 0x213FE}, //1575 #big5-hkscs
    {0x91FE, 0x212A8}, //1576 #big5-hkscs
    {0x9240, 0x213C6}, //1577 #big5-hkscs
    {0x9241, 0x214B6}, //1578 #big5-hkscs
    {0x9242, 0x8503}, //1579 #big5-hkscs
    {0x9243, 0x236A6}, //1580 #big5-hkscs
    {0x9244, 0x8503}, //1581 #big5-hkscs
    {0x9245, 0x8455}, //1582 #big5-hkscs
    {0x9246, 0x24994}, //1583 #big5-hkscs
    {0x9247, 0x27165}, //1584 #big5-hkscs
    {0x9248, 0x23E31}, //1585 #big5-hkscs
    {0x9249, 0x2555C}, //1586 #big5-hkscs
    {0x924A, 0x23EFB}, //1587 #big5-hkscs
    {0x924B, 0x27052}, //1588 #big5-hkscs
    {0x924C, 0x44F4}, //1589 #big5-hkscs
    {0x924D, 0x236EE}, //1590 #big5-hkscs
    {0x924E, 0x2999D}, //1591 #big5-hkscs
    {0x924F, 0x26F26}, //1592 #big5-hkscs
    {0x9250, 0x67F9}, //1593 #big5-hkscs
    {0x9251, 0x3733}, //1594 #big5-hkscs
    {0x9252, 0x3C15}, //1595 #big5-hkscs
    {0x9253, 0x3DE7}, //1596 #big5-hkscs
    {0x9254, 0x586C}, //1597 #big5-hkscs
    {0x9255, 0x21922}, //1598 #big5-hkscs
    {0x9256, 0x6810}, //1599 #big5-hkscs
    {0x9257, 0x4057}, //1600 #big5-hkscs
    {0x9258, 0x2373F}, //1601 #big5-hkscs
    {0x9259, 0x240E1}, //1602 #big5-hkscs
    {0x925A, 0x2408B}, //1603 #big5-hkscs
    {0x925B, 0x2410F}, //1604 #big5-hkscs
    {0x925C, 0x26C21}, //1605 #big5-hkscs
    {0x925D, 0x54CB}, //1606 #big5-hkscs
    {0x925E, 0x569E}, //1607 #big5-hkscs
    {0x925F, 0x266B1}, //1608 #big5-hkscs
    {0x9260, 0x5692}, //1609 #big5-hkscs
    {0x9261, 0x20FDF}, //1610 #big5-hkscs
    {0x9262, 0x20BA8}, //1611 #big5-hkscs
    {0x9263, 0x20E0D}, //1612 #big5-hkscs
    {0x9264, 0x93C6}, //1613 #big5-hkscs
    {0x9265, 0x28B13}, //1614 #big5-hkscs
    {0x9266, 0x939C}, //1615 #big5-hkscs
    {0x9267, 0x4EF8}, //1616 #big5-hkscs
    {0x9268, 0x512B}, //1617 #big5-hkscs
    {0x9269, 0x3819}, //1618 #big5-hkscs
    {0x926A, 0x24436}, //1619 #big5-hkscs
    {0x926B, 0x4EBC}, //1620 #big5-hkscs
    {0x926C, 0x20465}, //1621 #big5-hkscs
    {0x926D, 0x2037F}, //1622 #big5-hkscs
    {0x926E, 0x4F4B}, //1623 #big5-hkscs
    {0x926F, 0x4F8A}, //1624 #big5-hkscs
    {0x9270, 0x25651}, //1625 #big5-hkscs
    {0x9271, 0x5A68}, //1626 #big5-hkscs
    {0x9272, 0x201AB}, //1627 #big5-hkscs
    {0x9273, 0x203CB}, //1628 #big5-hkscs
    {0x9274, 0x3999}, //1629 #big5-hkscs
    {0x9275, 0x2030A}, //1630 #big5-hkscs
    {0x9276, 0x20414}, //1631 #big5-hkscs
    {0x9277, 0x3435}, //1632 #big5-hkscs
    {0x9278, 0x4F29}, //1633 #big5-hkscs
    {0x9279, 0x202C0}, //1634 #big5-hkscs
    {0x927A, 0x28EB3}, //1635 #big5-hkscs
    {0x927B, 0x20275}, //1636 #big5-hkscs
    {0x927C, 0x8ADA}, //1637 #big5-hkscs
    {0x927D, 0x2020C}, //1638 #big5-hkscs
    {0x927E, 0x4E98}, //1639 #big5-hkscs
    {0x92A1, 0x50CD}, //1640 #big5-hkscs
    {0x92A2, 0x510D}, //1641 #big5-hkscs
    {0x92A3, 0x4FA2}, //1642 #big5-hkscs
    {0x92A4, 0x4F03}, //1643 #big5-hkscs
    {0x92A5, 0x24A0E}, //1644 #big5-hkscs
    {0x92A6, 0x23E8A}, //1645 #big5-hkscs
    {0x92A7, 0x4F42}, //1646 #big5-hkscs
    {0x92A8, 0x502E}, //1647 #big5-hkscs
    {0x92A9, 0x506C}, //1648 #big5-hkscs
    {0x92AA, 0x5081}, //1649 #big5-hkscs
    {0x92AB, 0x4FCC}, //1650 #big5-hkscs
    {0x92AC, 0x4FE5}, //1651 #big5-hkscs
    {0x92AD, 0x5058}, //1652 #big5-hkscs
    {0x92AE, 0x50FC}, //1653 #big5-hkscs
    {0x92AF, 0x5159}, //1654 #big5-hkscs
    {0x92B0, 0x515B}, //1655 #big5-hkscs
    {0x92B1, 0x515D}, //1656 #big5-hkscs
    {0x92B2, 0x515E}, //1657 #big5-hkscs
    {0x92B3, 0x6E76}, //1658 #big5-hkscs
    {0x92B4, 0x23595}, //1659 #big5-hkscs
    {0x92B5, 0x23E39}, //1660 #big5-hkscs
    {0x92B6, 0x23EBF}, //1661 #big5-hkscs
    {0x92B7, 0x6D72}, //1662 #big5-hkscs
    {0x92B8, 0x21884}, //1663 #big5-hkscs
    {0x92B9, 0x23E89}, //1664 #big5-hkscs
    {0x92BA, 0x51A8}, //1665 #big5-hkscs
    {0x92BB, 0x51C3}, //1666 #big5-hkscs
    {0x92BC, 0x205E0}, //1667 #big5-hkscs
    {0x92BD, 0x44DD}, //1668 #big5-hkscs
    {0x92BE, 0x204A3}, //1669 #big5-hkscs
    {0x92BF, 0x20492}, //1670 #big5-hkscs
    {0x92C0, 0x20491}, //1671 #big5-hkscs
    {0x92C1, 0x8D7A}, //1672 #big5-hkscs
    {0x92C2, 0x28A9C}, //1673 #big5-hkscs
    {0x92C3, 0x2070E}, //1674 #big5-hkscs
    {0x92C4, 0x5259}, //1675 #big5-hkscs
    {0x92C5, 0x52A4}, //1676 #big5-hkscs
    {0x92C6, 0x20873}, //1677 #big5-hkscs
    {0x92C7, 0x52E1}, //1678 #big5-hkscs
    {0x92C8, 0x936E}, //1679 #big5-hkscs
    {0x92C9, 0x467A}, //1680 #big5-hkscs
    {0x92CA, 0x718C}, //1681 #big5-hkscs
    {0x92CB, 0x2438C}, //1682 #big5-hkscs
    {0x92CC, 0x20C20}, //1683 #big5-hkscs
    {0x92CD, 0x249AC}, //1684 #big5-hkscs
    {0x92CE, 0x210E4}, //1685 #big5-hkscs
    {0x92CF, 0x69D1}, //1686 #big5-hkscs
    {0x92D0, 0x20E1D}, //1687 #big5-hkscs
    {0x92D1, 0x7479}, //1688 #big5-hkscs
    {0x92D2, 0x3EDE}, //1689 #big5-hkscs
    {0x92D3, 0x7499}, //1690 #big5-hkscs
    {0x92D4, 0x7414}, //1691 #big5-hkscs
    {0x92D5, 0x7456}, //1692 #big5-hkscs
    {0x92D6, 0x7398}, //1693 #big5-hkscs
    {0x92D7, 0x4B8E}, //1694 #big5-hkscs
    {0x92D8, 0x24ABC}, //1695 #big5-hkscs
    {0x92D9, 0x2408D}, //1696 #big5-hkscs
    {0x92DA, 0x53D0}, //1697 #big5-hkscs
    {0x92DB, 0x3584}, //1698 #big5-hkscs
    {0x92DC, 0x720F}, //1699 #big5-hkscs
    {0x92DD, 0x240C9}, //1700 #big5-hkscs
    {0x92DE, 0x55B4}, //1701 #big5-hkscs
    {0x92DF, 0x20345}, //1702 #big5-hkscs
    {0x92E0, 0x54CD}, //1703 #big5-hkscs
    {0x92E1, 0x20BC6}, //1704 #big5-hkscs
    {0x92E2, 0x571D}, //1705 #big5-hkscs
    {0x92E3, 0x925D}, //1706 #big5-hkscs
    {0x92E4, 0x96F4}, //1707 #big5-hkscs
    {0x92E5, 0x9366}, //1708 #big5-hkscs
    {0x92E6, 0x57DD}, //1709 #big5-hkscs
    {0x92E7, 0x578D}, //1710 #big5-hkscs
    {0x92E8, 0x577F}, //1711 #big5-hkscs
    {0x92E9, 0x363E}, //1712 #big5-hkscs
    {0x92EA, 0x58CB}, //1713 #big5-hkscs
    {0x92EB, 0x5A99}, //1714 #big5-hkscs
    {0x92EC, 0x28A46}, //1715 #big5-hkscs
    {0x92ED, 0x216FA}, //1716 #big5-hkscs
    {0x92EE, 0x2176F}, //1717 #big5-hkscs
    {0x92EF, 0x21710}, //1718 #big5-hkscs
    {0x92F0, 0x5A2C}, //1719 #big5-hkscs
    {0x92F1, 0x59B8}, //1720 #big5-hkscs
    {0x92F2, 0x928F}, //1721 #big5-hkscs
    {0x92F3, 0x5A7E}, //1722 #big5-hkscs
    {0x92F4, 0x5ACF}, //1723 #big5-hkscs
    {0x92F5, 0x5A12}, //1724 #big5-hkscs
    {0x92F6, 0x25946}, //1725 #big5-hkscs
    {0x92F7, 0x219F3}, //1726 #big5-hkscs
    {0x92F8, 0x21861}, //1727 #big5-hkscs
    {0x92F9, 0x24295}, //1728 #big5-hkscs
    {0x92FA, 0x36F5}, //1729 #big5-hkscs
    {0x92FB, 0x6D05}, //1730 #big5-hkscs
    {0x92FC, 0x7443}, //1731 #big5-hkscs
    {0x92FD, 0x5A21}, //1732 #big5-hkscs
    {0x92FE, 0x25E83}, //1733 #big5-hkscs
    {0x9340, 0x5A81}, //1734 #big5-hkscs
    {0x9341, 0x28BD7}, //1735 #big5-hkscs
    {0x9342, 0x20413}, //1736 #big5-hkscs
    {0x9343, 0x93E0}, //1737 #big5-hkscs
    {0x9344, 0x748C}, //1738 #big5-hkscs
    {0x9345, 0x21303}, //1739 #big5-hkscs
    {0x9346, 0x7105}, //1740 #big5-hkscs
    {0x9347, 0x4972}, //1741 #big5-hkscs
    {0x9348, 0x9408}, //1742 #big5-hkscs
    {0x9349, 0x289FB}, //1743 #big5-hkscs
    {0x934A, 0x93BD}, //1744 #big5-hkscs
    {0x934B, 0x37A0}, //1745 #big5-hkscs
    {0x934C, 0x5C1E}, //1746 #big5-hkscs
    {0x934D, 0x5C9E}, //1747 #big5-hkscs
    {0x934E, 0x5E5E}, //1748 #big5-hkscs
    {0x934F, 0x5E48}, //1749 #big5-hkscs
    {0x9350, 0x21996}, //1750 #big5-hkscs
    {0x9351, 0x2197C}, //1751 #big5-hkscs
    {0x9352, 0x23AEE}, //1752 #big5-hkscs
    {0x9353, 0x5ECD}, //1753 #big5-hkscs
    {0x9354, 0x5B4F}, //1754 #big5-hkscs
    {0x9355, 0x21903}, //1755 #big5-hkscs
    {0x9356, 0x21904}, //1756 #big5-hkscs
    {0x9357, 0x3701}, //1757 #big5-hkscs
    {0x9358, 0x218A0}, //1758 #big5-hkscs
    {0x9359, 0x36DD}, //1759 #big5-hkscs
    {0x935A, 0x216FE}, //1760 #big5-hkscs
    {0x935B, 0x36D3}, //1761 #big5-hkscs
    {0x935C, 0x812A}, //1762 #big5-hkscs
    {0x935D, 0x28A47}, //1763 #big5-hkscs
    {0x935E, 0x21DBA}, //1764 #big5-hkscs
    {0x935F, 0x23472}, //1765 #big5-hkscs
    {0x9360, 0x289A8}, //1766 #big5-hkscs
    {0x9361, 0x5F0C}, //1767 #big5-hkscs
    {0x9362, 0x5F0E}, //1768 #big5-hkscs
    {0x9363, 0x21927}, //1769 #big5-hkscs
    {0x9364, 0x217AB}, //1770 #big5-hkscs
    {0x9365, 0x5A6B}, //1771 #big5-hkscs
    {0x9366, 0x2173B}, //1772 #big5-hkscs
    {0x9367, 0x5B44}, //1773 #big5-hkscs
    {0x9368, 0x8614}, //1774 #big5-hkscs
    {0x9369, 0x275FD}, //1775 #big5-hkscs
    {0x936A, 0x8860}, //1776 #big5-hkscs
    {0x936B, 0x607E}, //1777 #big5-hkscs
    {0x936C, 0x22860}, //1778 #big5-hkscs
    {0x936D, 0x2262B}, //1779 #big5-hkscs
    {0x936E, 0x5FDB}, //1780 #big5-hkscs
    {0x936F, 0x3EB8}, //1781 #big5-hkscs
    {0x9370, 0x225AF}, //1782 #big5-hkscs
    {0x9371, 0x225BE}, //1783 #big5-hkscs
    {0x9372, 0x29088}, //1784 #big5-hkscs
    {0x9373, 0x26F73}, //1785 #big5-hkscs
    {0x9374, 0x61C0}, //1786 #big5-hkscs
    {0x9375, 0x2003E}, //1787 #big5-hkscs
    {0x9376, 0x20046}, //1788 #big5-hkscs
    {0x9377, 0x2261B}, //1789 #big5-hkscs
    {0x9378, 0x6199}, //1790 #big5-hkscs
    {0x9379, 0x6198}, //1791 #big5-hkscs
    {0x937A, 0x6075}, //1792 #big5-hkscs
    {0x937B, 0x22C9B}, //1793 #big5-hkscs
    {0x937C, 0x22D07}, //1794 #big5-hkscs
    {0x937D, 0x246D4}, //1795 #big5-hkscs
    {0x937E, 0x2914D}, //1796 #big5-hkscs
    {0x93A1, 0x6471}, //1797 #big5-hkscs
    {0x93A2, 0x24665}, //1798 #big5-hkscs
    {0x93A3, 0x22B6A}, //1799 #big5-hkscs
    {0x93A4, 0x3A29}, //1800 #big5-hkscs
    {0x93A5, 0x22B22}, //1801 #big5-hkscs
    {0x93A6, 0x23450}, //1802 #big5-hkscs
    {0x93A7, 0x298EA}, //1803 #big5-hkscs
    {0x93A8, 0x22E78}, //1804 #big5-hkscs
    {0x93A9, 0x6337}, //1805 #big5-hkscs
    {0x93AA, 0x2A45B}, //1806 #big5-hkscs
    {0x93AB, 0x64B6}, //1807 #big5-hkscs
    {0x93AC, 0x6331}, //1808 #big5-hkscs
    {0x93AD, 0x63D1}, //1809 #big5-hkscs
    {0x93AE, 0x249E3}, //1810 #big5-hkscs
    {0x93AF, 0x22D67}, //1811 #big5-hkscs
    {0x93B0, 0x62A4}, //1812 #big5-hkscs
    {0x93B1, 0x22CA1}, //1813 #big5-hkscs
    {0x93B2, 0x643B}, //1814 #big5-hkscs
    {0x93B3, 0x656B}, //1815 #big5-hkscs
    {0x93B4, 0x6972}, //1816 #big5-hkscs
    {0x93B5, 0x3BF4}, //1817 #big5-hkscs
    {0x93B6, 0x2308E}, //1818 #big5-hkscs
    {0x93B7, 0x232AD}, //1819 #big5-hkscs
    {0x93B8, 0x24989}, //1820 #big5-hkscs
    {0x93B9, 0x232AB}, //1821 #big5-hkscs
    {0x93BA, 0x550D}, //1822 #big5-hkscs
    {0x93BB, 0x232E0}, //1823 #big5-hkscs
    {0x93BC, 0x218D9}, //1824 #big5-hkscs
    {0x93BD, 0x2943F}, //1825 #big5-hkscs
    {0x93BE, 0x66CE}, //1826 #big5-hkscs
    {0x93BF, 0x23289}, //1827 #big5-hkscs
    {0x93C0, 0x231B3}, //1828 #big5-hkscs
    {0x93C1, 0x3AE0}, //1829 #big5-hkscs
    {0x93C2, 0x4190}, //1830 #big5-hkscs
    {0x93C3, 0x25584}, //1831 #big5-hkscs
    {0x93C4, 0x28B22}, //1832 #big5-hkscs
    {0x93C5, 0x2558F}, //1833 #big5-hkscs
    {0x93C6, 0x216FC}, //1834 #big5-hkscs
    {0x93C7, 0x2555B}, //1835 #big5-hkscs
    {0x93C8, 0x25425}, //1836 #big5-hkscs
    {0x93C9, 0x78EE}, //1837 #big5-hkscs
    {0x93CA, 0x23103}, //1838 #big5-hkscs
    {0x93CB, 0x2182A}, //1839 #big5-hkscs
    {0x93CC, 0x23234}, //1840 #big5-hkscs
    {0x93CD, 0x3464}, //1841 #big5-hkscs
    {0x93CE, 0x2320F}, //1842 #big5-hkscs
    {0x93CF, 0x23182}, //1843 #big5-hkscs
    {0x93D0, 0x242C9}, //1844 #big5-hkscs
    {0x93D1, 0x668E}, //1845 #big5-hkscs
    {0x93D2, 0x26D24}, //1846 #big5-hkscs
    {0x93D3, 0x666B}, //1847 #big5-hkscs
    {0x93D4, 0x4B93}, //1848 #big5-hkscs
    {0x93D5, 0x6630}, //1849 #big5-hkscs
    {0x93D6, 0x27870}, //1850 #big5-hkscs
    {0x93D7, 0x21DEB}, //1851 #big5-hkscs
    {0x93D8, 0x6663}, //1852 #big5-hkscs
    {0x93D9, 0x232D2}, //1853 #big5-hkscs
    {0x93DA, 0x232E1}, //1854 #big5-hkscs
    {0x93DB, 0x661E}, //1855 #big5-hkscs
    {0x93DC, 0x25872}, //1856 #big5-hkscs
    {0x93DD, 0x38D1}, //1857 #big5-hkscs
    {0x93DE, 0x2383A}, //1858 #big5-hkscs
    {0x93DF, 0x237BC}, //1859 #big5-hkscs
    {0x93E0, 0x3B99}, //1860 #big5-hkscs
    {0x93E1, 0x237A2}, //1861 #big5-hkscs
    {0x93E2, 0x233FE}, //1862 #big5-hkscs
    {0x93E3, 0x74D0}, //1863 #big5-hkscs
    {0x93E4, 0x3B96}, //1864 #big5-hkscs
    {0x93E5, 0x678F}, //1865 #big5-hkscs
    {0x93E6, 0x2462A}, //1866 #big5-hkscs
    {0x93E7, 0x68B6}, //1867 #big5-hkscs
    {0x93E8, 0x681E}, //1868 #big5-hkscs
    {0x93E9, 0x3BC4}, //1869 #big5-hkscs
    {0x93EA, 0x6ABE}, //1870 #big5-hkscs
    {0x93EB, 0x3863}, //1871 #big5-hkscs
    {0x93EC, 0x237D5}, //1872 #big5-hkscs
    {0x93ED, 0x24487}, //1873 #big5-hkscs
    {0x93EE, 0x6A33}, //1874 #big5-hkscs
    {0x93EF, 0x6A52}, //1875 #big5-hkscs
    {0x93F0, 0x6AC9}, //1876 #big5-hkscs
    {0x93F1, 0x6B05}, //1877 #big5-hkscs
    {0x93F2, 0x21912}, //1878 #big5-hkscs
    {0x93F3, 0x6511}, //1879 #big5-hkscs
    {0x93F4, 0x6898}, //1880 #big5-hkscs
    {0x93F5, 0x6A4C}, //1881 #big5-hkscs
    {0x93F6, 0x3BD7}, //1882 #big5-hkscs
    {0x93F7, 0x6A7A}, //1883 #big5-hkscs
    {0x93F8, 0x6B57}, //1884 #big5-hkscs
    {0x93F9, 0x23FC0}, //1885 #big5-hkscs
    {0x93FA, 0x23C9A}, //1886 #big5-hkscs
    {0x93FB, 0x93A0}, //1887 #big5-hkscs
    {0x93FC, 0x92F2}, //1888 #big5-hkscs
    {0x93FD, 0x28BEA}, //1889 #big5-hkscs
    {0x93FE, 0x28ACB}, //1890 #big5-hkscs
    {0x9440, 0x9289}, //1891 #big5-hkscs
    {0x9441, 0x2801E}, //1892 #big5-hkscs
    {0x9442, 0x289DC}, //1893 #big5-hkscs
    {0x9443, 0x9467}, //1894 #big5-hkscs
    {0x9444, 0x6DA5}, //1895 #big5-hkscs
    {0x9445, 0x6F0B}, //1896 #big5-hkscs
    {0x9446, 0x249EC}, //1897 #big5-hkscs
    {0x9447, 0x6D67}, //1898 #big5-hkscs
    {0x9448, 0x23F7F}, //1899 #big5-hkscs
    {0x9449, 0x3D8F}, //1900 #big5-hkscs
    {0x944A, 0x6E04}, //1901 #big5-hkscs
    {0x944B, 0x2403C}, //1902 #big5-hkscs
    {0x944C, 0x5A3D}, //1903 #big5-hkscs
    {0x944D, 0x6E0A}, //1904 #big5-hkscs
    {0x944E, 0x5847}, //1905 #big5-hkscs
    {0x944F, 0x6D24}, //1906 #big5-hkscs
    {0x9450, 0x7842}, //1907 #big5-hkscs
    {0x9451, 0x713B}, //1908 #big5-hkscs
    {0x9452, 0x2431A}, //1909 #big5-hkscs
    {0x9453, 0x24276}, //1910 #big5-hkscs
    {0x9454, 0x70F1}, //1911 #big5-hkscs
    {0x9455, 0x7250}, //1912 #big5-hkscs
    {0x9456, 0x7287}, //1913 #big5-hkscs
    {0x9457, 0x7294}, //1914 #big5-hkscs
    {0x9458, 0x2478F}, //1915 #big5-hkscs
    {0x9459, 0x24725}, //1916 #big5-hkscs
    {0x945A, 0x5179}, //1917 #big5-hkscs
    {0x945B, 0x24AA4}, //1918 #big5-hkscs
    {0x945C, 0x205EB}, //1919 #big5-hkscs
    {0x945D, 0x747A}, //1920 #big5-hkscs
    {0x945E, 0x23EF8}, //1921 #big5-hkscs
    {0x945F, 0x2365F}, //1922 #big5-hkscs
    {0x9460, 0x24A4A}, //1923 #big5-hkscs
    {0x9461, 0x24917}, //1924 #big5-hkscs
    {0x9462, 0x25FE1}, //1925 #big5-hkscs
    {0x9463, 0x3F06}, //1926 #big5-hkscs
    {0x9464, 0x3EB1}, //1927 #big5-hkscs
    {0x9465, 0x24ADF}, //1928 #big5-hkscs
    {0x9466, 0x28C23}, //1929 #big5-hkscs
    {0x9467, 0x23F35}, //1930 #big5-hkscs
    {0x9468, 0x60A7}, //1931 #big5-hkscs
    {0x9469, 0x3EF3}, //1932 #big5-hkscs
    {0x946A, 0x74CC}, //1933 #big5-hkscs
    {0x946B, 0x743C}, //1934 #big5-hkscs
    {0x946C, 0x9387}, //1935 #big5-hkscs
    {0x946D, 0x7437}, //1936 #big5-hkscs
    {0x946E, 0x449F}, //1937 #big5-hkscs
    {0x946F, 0x26DEA}, //1938 #big5-hkscs
    {0x9470, 0x4551}, //1939 #big5-hkscs
    {0x9471, 0x7583}, //1940 #big5-hkscs
    {0x9472, 0x3F63}, //1941 #big5-hkscs
    {0x9473, 0x24CD9}, //1942 #big5-hkscs
    {0x9474, 0x24D06}, //1943 #big5-hkscs
    {0x9475, 0x3F58}, //1944 #big5-hkscs
    {0x9476, 0x7555}, //1945 #big5-hkscs
    {0x9477, 0x7673}, //1946 #big5-hkscs
    {0x9478, 0x2A5C6}, //1947 #big5-hkscs
    {0x9479, 0x3B19}, //1948 #big5-hkscs
    {0x947A, 0x7468}, //1949 #big5-hkscs
    {0x947B, 0x28ACC}, //1950 #big5-hkscs
    {0x947C, 0x249AB}, //1951 #big5-hkscs
    {0x947D, 0x2498E}, //1952 #big5-hkscs
    {0x947E, 0x3AFB}, //1953 #big5-hkscs
    {0x94A1, 0x3DCD}, //1954 #big5-hkscs
    {0x94A2, 0x24A4E}, //1955 #big5-hkscs
    {0x94A3, 0x3EFF}, //1956 #big5-hkscs
    {0x94A4, 0x249C5}, //1957 #big5-hkscs
    {0x94A5, 0x248F3}, //1958 #big5-hkscs
    {0x94A6, 0x91FA}, //1959 #big5-hkscs
    {0x94A7, 0x5732}, //1960 #big5-hkscs
    {0x94A8, 0x9342}, //1961 #big5-hkscs
    {0x94A9, 0x28AE3}, //1962 #big5-hkscs
    {0x94AA, 0x21864}, //1963 #big5-hkscs
    {0x94AB, 0x50DF}, //1964 #big5-hkscs
    {0x94AC, 0x25221}, //1965 #big5-hkscs
    {0x94AD, 0x251E7}, //1966 #big5-hkscs
    {0x94AE, 0x7778}, //1967 #big5-hkscs
    {0x94AF, 0x23232}, //1968 #big5-hkscs
    {0x94B0, 0x770E}, //1969 #big5-hkscs
    {0x94B1, 0x770F}, //1970 #big5-hkscs
    {0x94B2, 0x777B}, //1971 #big5-hkscs
    {0x94B3, 0x24697}, //1972 #big5-hkscs
    {0x94B4, 0x23781}, //1973 #big5-hkscs
    {0x94B5, 0x3A5E}, //1974 #big5-hkscs
    {0x94B6, 0x248F0}, //1975 #big5-hkscs
    {0x94B7, 0x7438}, //1976 #big5-hkscs
    {0x94B8, 0x749B}, //1977 #big5-hkscs
    {0x94B9, 0x3EBF}, //1978 #big5-hkscs
    {0x94BA, 0x24ABA}, //1979 #big5-hkscs
    {0x94BB, 0x24AC7}, //1980 #big5-hkscs
    {0x94BC, 0x40C8}, //1981 #big5-hkscs
    {0x94BD, 0x24A96}, //1982 #big5-hkscs
    {0x94BE, 0x261AE}, //1983 #big5-hkscs
    {0x94BF, 0x9307}, //1984 #big5-hkscs
    {0x94C0, 0x25581}, //1985 #big5-hkscs
    {0x94C1, 0x781E}, //1986 #big5-hkscs
    {0x94C2, 0x788D}, //1987 #big5-hkscs
    {0x94C3, 0x7888}, //1988 #big5-hkscs
    {0x94C4, 0x78D2}, //1989 #big5-hkscs
    {0x94C5, 0x73D0}, //1990 #big5-hkscs
    {0x94C6, 0x7959}, //1991 #big5-hkscs
    {0x94C7, 0x27741}, //1992 #big5-hkscs
    {0x94C8, 0x256E3}, //1993 #big5-hkscs
    {0x94C9, 0x410E}, //1994 #big5-hkscs
    {0x94CA, 0x799B}, //1995 #big5-hkscs
    {0x94CB, 0x8496}, //1996 #big5-hkscs
    {0x94CC, 0x79A5}, //1997 #big5-hkscs
    {0x94CD, 0x6A2D}, //1998 #big5-hkscs
    {0x94CE, 0x23EFA}, //1999 #big5-hkscs
    {0x94CF, 0x7A3A}, //2000 #big5-hkscs
    {0x94D0, 0x79F4}, //2001 #big5-hkscs
    {0x94D1, 0x416E}, //2002 #big5-hkscs
    {0x94D2, 0x216E6}, //2003 #big5-hkscs
    {0x94D3, 0x4132}, //2004 #big5-hkscs
    {0x94D4, 0x9235}, //2005 #big5-hkscs
    {0x94D5, 0x79F1}, //2006 #big5-hkscs
    {0x94D6, 0x20D4C}, //2007 #big5-hkscs
    {0x94D7, 0x2498C}, //2008 #big5-hkscs
    {0x94D8, 0x20299}, //2009 #big5-hkscs
    {0x94D9, 0x23DBA}, //2010 #big5-hkscs
    {0x94DA, 0x2176E}, //2011 #big5-hkscs
    {0x94DB, 0x3597}, //2012 #big5-hkscs
    {0x94DC, 0x556B}, //2013 #big5-hkscs
    {0x94DD, 0x3570}, //2014 #big5-hkscs
    {0x94DE, 0x36AA}, //2015 #big5-hkscs
    {0x94DF, 0x201D4}, //2016 #big5-hkscs
    {0x94E0, 0x20C0D}, //2017 #big5-hkscs
    {0x94E1, 0x7AE2}, //2018 #big5-hkscs
    {0x94E2, 0x5A59}, //2019 #big5-hkscs
    {0x94E3, 0x226F5}, //2020 #big5-hkscs
    {0x94E4, 0x25AAF}, //2021 #big5-hkscs
    {0x94E5, 0x25A9C}, //2022 #big5-hkscs
    {0x94E6, 0x5A0D}, //2023 #big5-hkscs
    {0x94E7, 0x2025B}, //2024 #big5-hkscs
    {0x94E8, 0x78F0}, //2025 #big5-hkscs
    {0x94E9, 0x5A2A}, //2026 #big5-hkscs
    {0x94EA, 0x25BC6}, //2027 #big5-hkscs
    {0x94EB, 0x7AFE}, //2028 #big5-hkscs
    {0x94EC, 0x41F9}, //2029 #big5-hkscs
    {0x94ED, 0x7C5D}, //2030 #big5-hkscs
    {0x94EE, 0x7C6D}, //2031 #big5-hkscs
    {0x94EF, 0x4211}, //2032 #big5-hkscs
    {0x94F0, 0x25BB3}, //2033 #big5-hkscs
    {0x94F1, 0x25EBC}, //2034 #big5-hkscs
    {0x94F2, 0x25EA6}, //2035 #big5-hkscs
    {0x94F3, 0x7CCD}, //2036 #big5-hkscs
    {0x94F4, 0x249F9}, //2037 #big5-hkscs
    {0x94F5, 0x217B0}, //2038 #big5-hkscs
    {0x94F6, 0x7C8E}, //2039 #big5-hkscs
    {0x94F7, 0x7C7C}, //2040 #big5-hkscs
    {0x94F8, 0x7CAE}, //2041 #big5-hkscs
    {0x94F9, 0x6AB2}, //2042 #big5-hkscs
    {0x94FA, 0x7DDC}, //2043 #big5-hkscs
    {0x94FB, 0x7E07}, //2044 #big5-hkscs
    {0x94FC, 0x7DD3}, //2045 #big5-hkscs
    {0x94FD, 0x7F4E}, //2046 #big5-hkscs
    {0x94FE, 0x26261}, //2047 #big5-hkscs
    {0x9540, 0x2615C}, //2048 #big5-hkscs
    {0x9541, 0x27B48}, //2049 #big5-hkscs
    {0x9542, 0x7D97}, //2050 #big5-hkscs
    {0x9543, 0x25E82}, //2051 #big5-hkscs
    {0x9544, 0x426A}, //2052 #big5-hkscs
    {0x9545, 0x26B75}, //2053 #big5-hkscs
    {0x9546, 0x20916}, //2054 #big5-hkscs
    {0x9547, 0x67D6}, //2055 #big5-hkscs
    {0x9548, 0x2004E}, //2056 #big5-hkscs
    {0x9549, 0x235CF}, //2057 #big5-hkscs
    {0x954A, 0x57C4}, //2058 #big5-hkscs
    {0x954B, 0x26412}, //2059 #big5-hkscs
    {0x954C, 0x263F8}, //2060 #big5-hkscs
    {0x954D, 0x24962}, //2061 #big5-hkscs
    {0x954E, 0x7FDD}, //2062 #big5-hkscs
    {0x954F, 0x7B27}, //2063 #big5-hkscs
    {0x9550, 0x2082C}, //2064 #big5-hkscs
    {0x9551, 0x25AE9}, //2065 #big5-hkscs
    {0x9552, 0x25D43}, //2066 #big5-hkscs
    {0x9553, 0x7B0C}, //2067 #big5-hkscs
    {0x9554, 0x25E0E}, //2068 #big5-hkscs
    {0x9555, 0x99E6}, //2069 #big5-hkscs
    {0x9556, 0x8645}, //2070 #big5-hkscs
    {0x9557, 0x9A63}, //2071 #big5-hkscs
    {0x9558, 0x6A1C}, //2072 #big5-hkscs
    {0x9559, 0x2343F}, //2073 #big5-hkscs
    {0x955A, 0x39E2}, //2074 #big5-hkscs
    {0x955B, 0x249F7}, //2075 #big5-hkscs
    {0x955C, 0x265AD}, //2076 #big5-hkscs
    {0x955D, 0x9A1F}, //2077 #big5-hkscs
    {0x955E, 0x265A0}, //2078 #big5-hkscs
    {0x955F, 0x8480}, //2079 #big5-hkscs
    {0x9560, 0x27127}, //2080 #big5-hkscs
    {0x9561, 0x26CD1}, //2081 #big5-hkscs
    {0x9562, 0x44EA}, //2082 #big5-hkscs
    {0x9563, 0x8137}, //2083 #big5-hkscs
    {0x9564, 0x4402}, //2084 #big5-hkscs
    {0x9565, 0x80C6}, //2085 #big5-hkscs
    {0x9566, 0x8109}, //2086 #big5-hkscs
    {0x9567, 0x8142}, //2087 #big5-hkscs
    {0x9568, 0x267B4}, //2088 #big5-hkscs
    {0x9569, 0x98C3}, //2089 #big5-hkscs
    {0x956A, 0x26A42}, //2090 #big5-hkscs
    {0x956B, 0x8262}, //2091 #big5-hkscs
    {0x956C, 0x8265}, //2092 #big5-hkscs
    {0x956D, 0x26A51}, //2093 #big5-hkscs
    {0x956E, 0x8453}, //2094 #big5-hkscs
    {0x956F, 0x26DA7}, //2095 #big5-hkscs
    {0x9570, 0x8610}, //2096 #big5-hkscs
    {0x9571, 0x2721B}, //2097 #big5-hkscs
    {0x9572, 0x5A86}, //2098 #big5-hkscs
    {0x9573, 0x417F}, //2099 #big5-hkscs
    {0x9574, 0x21840}, //2100 #big5-hkscs
    {0x9575, 0x5B2B}, //2101 #big5-hkscs
    {0x9576, 0x218A1}, //2102 #big5-hkscs
    {0x9577, 0x5AE4}, //2103 #big5-hkscs
    {0x9578, 0x218D8}, //2104 #big5-hkscs
    {0x9579, 0x86A0}, //2105 #big5-hkscs
    {0x957A, 0x2F9BC}, //2106 #big5-hkscs
    {0x957B, 0x23D8F}, //2107 #big5-hkscs
    {0x957C, 0x882D}, //2108 #big5-hkscs
    {0x957D, 0x27422}, //2109 #big5-hkscs
    {0x957E, 0x5A02}, //2110 #big5-hkscs
    {0x95A1, 0x886E}, //2111 #big5-hkscs
    {0x95A2, 0x4F45}, //2112 #big5-hkscs
    {0x95A3, 0x8887}, //2113 #big5-hkscs
    {0x95A4, 0x88BF}, //2114 #big5-hkscs
    {0x95A5, 0x88E6}, //2115 #big5-hkscs
    {0x95A6, 0x8965}, //2116 #big5-hkscs
    {0x95A7, 0x894D}, //2117 #big5-hkscs
    {0x95A8, 0x25683}, //2118 #big5-hkscs
    {0x95A9, 0x8954}, //2119 #big5-hkscs
    {0x95AA, 0x27785}, //2120 #big5-hkscs
    {0x95AB, 0x27784}, //2121 #big5-hkscs
    {0x95AC, 0x28BF5}, //2122 #big5-hkscs
    {0x95AD, 0x28BD9}, //2123 #big5-hkscs
    {0x95AE, 0x28B9C}, //2124 #big5-hkscs
    {0x95AF, 0x289F9}, //2125 #big5-hkscs
    {0x95B0, 0x3EAD}, //2126 #big5-hkscs
    {0x95B1, 0x84A3}, //2127 #big5-hkscs
    {0x95B2, 0x46F5}, //2128 #big5-hkscs
    {0x95B3, 0x46CF}, //2129 #big5-hkscs
    {0x95B4, 0x37F2}, //2130 #big5-hkscs
    {0x95B5, 0x8A3D}, //2131 #big5-hkscs
    {0x95B6, 0x8A1C}, //2132 #big5-hkscs
    {0x95B7, 0x29448}, //2133 #big5-hkscs
    {0x95B8, 0x5F4D}, //2134 #big5-hkscs
    {0x95B9, 0x922B}, //2135 #big5-hkscs
    {0x95BA, 0x24284}, //2136 #big5-hkscs
    {0x95BB, 0x65D4}, //2137 #big5-hkscs
    {0x95BC, 0x7129}, //2138 #big5-hkscs
    {0x95BD, 0x70C4}, //2139 #big5-hkscs
    {0x95BE, 0x21845}, //2140 #big5-hkscs
    {0x95BF, 0x9D6D}, //2141 #big5-hkscs
    {0x95C0, 0x8C9F}, //2142 #big5-hkscs
    {0x95C1, 0x8CE9}, //2143 #big5-hkscs
    {0x95C2, 0x27DDC}, //2144 #big5-hkscs
    {0x95C3, 0x599A}, //2145 #big5-hkscs
    {0x95C4, 0x77C3}, //2146 #big5-hkscs
    {0x95C5, 0x59F0}, //2147 #big5-hkscs
    {0x95C6, 0x436E}, //2148 #big5-hkscs
    {0x95C7, 0x36D4}, //2149 #big5-hkscs
    {0x95C8, 0x8E2A}, //2150 #big5-hkscs
    {0x95C9, 0x8EA7}, //2151 #big5-hkscs
    {0x95CA, 0x24C09}, //2152 #big5-hkscs
    {0x95CB, 0x8F30}, //2153 #big5-hkscs
    {0x95CC, 0x8F4A}, //2154 #big5-hkscs
    {0x95CD, 0x42F4}, //2155 #big5-hkscs
    {0x95CE, 0x6C58}, //2156 #big5-hkscs
    {0x95CF, 0x6FBB}, //2157 #big5-hkscs
    {0x95D0, 0x22321}, //2158 #big5-hkscs
    {0x95D1, 0x489B}, //2159 #big5-hkscs
    {0x95D2, 0x6F79}, //2160 #big5-hkscs
    {0x95D3, 0x6E8B}, //2161 #big5-hkscs
    {0x95D4, 0x217DA}, //2162 #big5-hkscs
    {0x95D5, 0x9BE9}, //2163 #big5-hkscs
    {0x95D6, 0x36B5}, //2164 #big5-hkscs
    {0x95D7, 0x2492F}, //2165 #big5-hkscs
    {0x95D8, 0x90BB}, //2166 #big5-hkscs
    {0x95D9, 0x9097}, //2167 #big5-hkscs
    {0x95DA, 0x5571}, //2168 #big5-hkscs
    {0x95DB, 0x4906}, //2169 #big5-hkscs
    {0x95DC, 0x91BB}, //2170 #big5-hkscs
    {0x95DD, 0x9404}, //2171 #big5-hkscs
    {0x95DE, 0x28A4B}, //2172 #big5-hkscs
    {0x95DF, 0x4062}, //2173 #big5-hkscs
    {0x95E0, 0x28AFC}, //2174 #big5-hkscs
    {0x95E1, 0x9427}, //2175 #big5-hkscs
    {0x95E2, 0x28C1D}, //2176 #big5-hkscs
    {0x95E3, 0x28C3B}, //2177 #big5-hkscs
    {0x95E4, 0x84E5}, //2178 #big5-hkscs
    {0x95E5, 0x8A2B}, //2179 #big5-hkscs
    {0x95E6, 0x9599}, //2180 #big5-hkscs
    {0x95E7, 0x95A7}, //2181 #big5-hkscs
    {0x95E8, 0x9597}, //2182 #big5-hkscs
    {0x95E9, 0x9596}, //2183 #big5-hkscs
    {0x95EA, 0x28D34}, //2184 #big5-hkscs
    {0x95EB, 0x7445}, //2185 #big5-hkscs
    {0x95EC, 0x3EC2}, //2186 #big5-hkscs
    {0x95ED, 0x248FF}, //2187 #big5-hkscs
    {0x95EE, 0x24A42}, //2188 #big5-hkscs
    {0x95EF, 0x243EA}, //2189 #big5-hkscs
    {0x95F0, 0x3EE7}, //2190 #big5-hkscs
    {0x95F1, 0x23225}, //2191 #big5-hkscs
    {0x95F2, 0x968F}, //2192 #big5-hkscs
    {0x95F3, 0x28EE7}, //2193 #big5-hkscs
    {0x95F4, 0x28E66}, //2194 #big5-hkscs
    {0x95F5, 0x28E65}, //2195 #big5-hkscs
    {0x95F6, 0x3ECC}, //2196 #big5-hkscs
    {0x95F7, 0x249ED}, //2197 #big5-hkscs
    {0x95F8, 0x24A78}, //2198 #big5-hkscs
    {0x95F9, 0x23FEE}, //2199 #big5-hkscs
    {0x95FA, 0x7412}, //2200 #big5-hkscs
    {0x95FB, 0x746B}, //2201 #big5-hkscs
    {0x95FC, 0x3EFC}, //2202 #big5-hkscs
    {0x95FD, 0x9741}, //2203 #big5-hkscs
    {0x95FE, 0x290B0}, //2204 #big5-hkscs
    {0x9640, 0x6847}, //2205 #big5-hkscs
    {0x9641, 0x4A1D}, //2206 #big5-hkscs
    {0x9642, 0x29093}, //2207 #big5-hkscs
    {0x9643, 0x257DF}, //2208 #big5-hkscs
    {0x9644, 0x975D}, //2209 #big5-hkscs
    {0x9645, 0x9368}, //2210 #big5-hkscs
    {0x9646, 0x28989}, //2211 #big5-hkscs
    {0x9647, 0x28C26}, //2212 #big5-hkscs
    {0x9648, 0x28B2F}, //2213 #big5-hkscs
    {0x9649, 0x263BE}, //2214 #big5-hkscs
    {0x964A, 0x92BA}, //2215 #big5-hkscs
    {0x964B, 0x5B11}, //2216 #big5-hkscs
    {0x964C, 0x8B69}, //2217 #big5-hkscs
    {0x964D, 0x493C}, //2218 #big5-hkscs
    {0x964E, 0x73F9}, //2219 #big5-hkscs
    {0x964F, 0x2421B}, //2220 #big5-hkscs
    {0x9650, 0x979B}, //2221 #big5-hkscs
    {0x9651, 0x9771}, //2222 #big5-hkscs
    {0x9652, 0x9938}, //2223 #big5-hkscs
    {0x9653, 0x20F26}, //2224 #big5-hkscs
    {0x9654, 0x5DC1}, //2225 #big5-hkscs
    {0x9655, 0x28BC5}, //2226 #big5-hkscs
    {0x9656, 0x24AB2}, //2227 #big5-hkscs
    {0x9657, 0x981F}, //2228 #big5-hkscs
    {0x9658, 0x294DA}, //2229 #big5-hkscs
    {0x9659, 0x92F6}, //2230 #big5-hkscs
    {0x965A, 0x295D7}, //2231 #big5-hkscs
    {0x965B, 0x91E5}, //2232 #big5-hkscs
    {0x965C, 0x44C0}, //2233 #big5-hkscs
    {0x965D, 0x28B50}, //2234 #big5-hkscs
    {0x965E, 0x24A67}, //2235 #big5-hkscs
    {0x965F, 0x28B64}, //2236 #big5-hkscs
    {0x9660, 0x98DC}, //2237 #big5-hkscs
    {0x9661, 0x28A45}, //2238 #big5-hkscs
    {0x9662, 0x3F00}, //2239 #big5-hkscs
    {0x9663, 0x922A}, //2240 #big5-hkscs
    {0x9664, 0x4925}, //2241 #big5-hkscs
    {0x9665, 0x8414}, //2242 #big5-hkscs
    {0x9666, 0x993B}, //2243 #big5-hkscs
    {0x9667, 0x994D}, //2244 #big5-hkscs
    {0x9668, 0x27B06}, //2245 #big5-hkscs
    {0x9669, 0x3DFD}, //2246 #big5-hkscs
    {0x966A, 0x999B}, //2247 #big5-hkscs
    {0x966B, 0x4B6F}, //2248 #big5-hkscs
    {0x966C, 0x99AA}, //2249 #big5-hkscs
    {0x966D, 0x9A5C}, //2250 #big5-hkscs
    {0x966E, 0x28B65}, //2251 #big5-hkscs
    {0x966F, 0x258C8}, //2252 #big5-hkscs
    {0x9670, 0x6A8F}, //2253 #big5-hkscs
    {0x9671, 0x9A21}, //2254 #big5-hkscs
    {0x9672, 0x5AFE}, //2255 #big5-hkscs
    {0x9673, 0x9A2F}, //2256 #big5-hkscs
    {0x9674, 0x298F1}, //2257 #big5-hkscs
    {0x9675, 0x4B90}, //2258 #big5-hkscs
    {0x9676, 0x29948}, //2259 #big5-hkscs
    {0x9677, 0x99BC}, //2260 #big5-hkscs
    {0x9678, 0x4BBD}, //2261 #big5-hkscs
    {0x9679, 0x4B97}, //2262 #big5-hkscs
    {0x967A, 0x937D}, //2263 #big5-hkscs
    {0x967B, 0x5872}, //2264 #big5-hkscs
    {0x967C, 0x21302}, //2265 #big5-hkscs
    {0x967D, 0x5822}, //2266 #big5-hkscs
    {0x967E, 0x249B8}, //2267 #big5-hkscs
    {0x96A1, 0x214E8}, //2268 #big5-hkscs
    {0x96A2, 0x7844}, //2269 #big5-hkscs
    {0x96A3, 0x2271F}, //2270 #big5-hkscs
    {0x96A4, 0x23DB8}, //2271 #big5-hkscs
    {0x96A5, 0x68C5}, //2272 #big5-hkscs
    {0x96A6, 0x3D7D}, //2273 #big5-hkscs
    {0x96A7, 0x9458}, //2274 #big5-hkscs
    {0x96A8, 0x3927}, //2275 #big5-hkscs
    {0x96A9, 0x6150}, //2276 #big5-hkscs
    {0x96AA, 0x22781}, //2277 #big5-hkscs
    {0x96AB, 0x2296B}, //2278 #big5-hkscs
    {0x96AC, 0x6107}, //2279 #big5-hkscs
    {0x96AD, 0x9C4F}, //2280 #big5-hkscs
    {0x96AE, 0x9C53}, //2281 #big5-hkscs
    {0x96AF, 0x9C7B}, //2282 #big5-hkscs
    {0x96B0, 0x9C35}, //2283 #big5-hkscs
    {0x96B1, 0x9C10}, //2284 #big5-hkscs
    {0x96B2, 0x9B7F}, //2285 #big5-hkscs
    {0x96B3, 0x9BCF}, //2286 #big5-hkscs
    {0x96B4, 0x29E2D}, //2287 #big5-hkscs
    {0x96B5, 0x9B9F}, //2288 #big5-hkscs
    {0x96B6, 0x2A1F5}, //2289 #big5-hkscs
    {0x96B7, 0x2A0FE}, //2290 #big5-hkscs
    {0x96B8, 0x9D21}, //2291 #big5-hkscs
    {0x96B9, 0x4CAE}, //2292 #big5-hkscs
    {0x96BA, 0x24104}, //2293 #big5-hkscs
    {0x96BB, 0x9E18}, //2294 #big5-hkscs
    {0x96BC, 0x4CB0}, //2295 #big5-hkscs
    {0x96BD, 0x9D0C}, //2296 #big5-hkscs
    {0x96BE, 0x2A1B4}, //2297 #big5-hkscs
    {0x96BF, 0x2A0ED}, //2298 #big5-hkscs
    {0x96C0, 0x2A0F3}, //2299 #big5-hkscs
    {0x96C1, 0x2992F}, //2300 #big5-hkscs
    {0x96C2, 0x9DA5}, //2301 #big5-hkscs
    {0x96C3, 0x84BD}, //2302 #big5-hkscs
    {0x96C4, 0x26E12}, //2303 #big5-hkscs
    {0x96C5, 0x26FDF}, //2304 #big5-hkscs
    {0x96C6, 0x26B82}, //2305 #big5-hkscs
    {0x96C7, 0x85FC}, //2306 #big5-hkscs
    {0x96C8, 0x4533}, //2307 #big5-hkscs
    {0x96C9, 0x26DA4}, //2308 #big5-hkscs
    {0x96CA, 0x26E84}, //2309 #big5-hkscs
    {0x96CB, 0x26DF0}, //2310 #big5-hkscs
    {0x96CC, 0x8420}, //2311 #big5-hkscs
    {0x96CD, 0x85EE}, //2312 #big5-hkscs
    {0x96CE, 0x26E00}, //2313 #big5-hkscs
    {0x96CF, 0x237D7}, //2314 #big5-hkscs
    {0x96D0, 0x26064}, //2315 #big5-hkscs
    {0x96D1, 0x79E2}, //2316 #big5-hkscs
    {0x96D2, 0x2359C}, //2317 #big5-hkscs
    {0x96D3, 0x23640}, //2318 #big5-hkscs
    {0x96D4, 0x492D}, //2319 #big5-hkscs
    {0x96D5, 0x249DE}, //2320 #big5-hkscs
    {0x96D6, 0x3D62}, //2321 #big5-hkscs
    {0x96D7, 0x93DB}, //2322 #big5-hkscs
    {0x96D8, 0x92BE}, //2323 #big5-hkscs
    {0x96D9, 0x9348}, //2324 #big5-hkscs
    {0x96DA, 0x202BF}, //2325 #big5-hkscs
    {0x96DB, 0x78B9}, //2326 #big5-hkscs
    {0x96DC, 0x9277}, //2327 #big5-hkscs
    {0x96DD, 0x944D}, //2328 #big5-hkscs
    {0x96DE, 0x4FE4}, //2329 #big5-hkscs
    {0x96DF, 0x3440}, //2330 #big5-hkscs
    {0x96E0, 0x9064}, //2331 #big5-hkscs
    {0x96E1, 0x2555D}, //2332 #big5-hkscs
    {0x96E2, 0x783D}, //2333 #big5-hkscs
    {0x96E3, 0x7854}, //2334 #big5-hkscs
    {0x96E4, 0x78B6}, //2335 #big5-hkscs
    {0x96E5, 0x784B}, //2336 #big5-hkscs
    {0x96E6, 0x21757}, //2337 #big5-hkscs
    {0x96E7, 0x231C9}, //2338 #big5-hkscs
    {0x96E8, 0x24941}, //2339 #big5-hkscs
    {0x96E9, 0x369A}, //2340 #big5-hkscs
    {0x96EA, 0x4F72}, //2341 #big5-hkscs
    {0x96EB, 0x6FDA}, //2342 #big5-hkscs
    {0x96EC, 0x6FD9}, //2343 #big5-hkscs
    {0x96ED, 0x701E}, //2344 #big5-hkscs
    {0x96EE, 0x701E}, //2345 #big5-hkscs
    {0x96EF, 0x5414}, //2346 #big5-hkscs
    {0x96F0, 0x241B5}, //2347 #big5-hkscs
    {0x96F1, 0x57BB}, //2348 #big5-hkscs
    {0x96F2, 0x58F3}, //2349 #big5-hkscs
    {0x96F3, 0x578A}, //2350 #big5-hkscs
    {0x96F4, 0x9D16}, //2351 #big5-hkscs
    {0x96F5, 0x57D7}, //2352 #big5-hkscs
    {0x96F6, 0x7134}, //2353 #big5-hkscs
    {0x96F7, 0x34AF}, //2354 #big5-hkscs
    {0x96F8, 0x241AC}, //2355 #big5-hkscs
    {0x96F9, 0x71EB}, //2356 #big5-hkscs
    {0x96FA, 0x26C40}, //2357 #big5-hkscs
    {0x96FB, 0x24F97}, //2358 #big5-hkscs
    {0x96FC, 0x5B28}, //2359 #big5-hkscs
    {0x96FD, 0x217B5}, //2360 #big5-hkscs
    {0x96FE, 0x28A49}, //2361 #big5-hkscs
    {0x9740, 0x610C}, //2362 #big5-hkscs
    {0x9741, 0x5ACE}, //2363 #big5-hkscs
    {0x9742, 0x5A0B}, //2364 #big5-hkscs
    {0x9743, 0x42BC}, //2365 #big5-hkscs
    {0x9744, 0x24488}, //2366 #big5-hkscs
    {0x9745, 0x372C}, //2367 #big5-hkscs
    {0x9746, 0x4B7B}, //2368 #big5-hkscs
    {0x9747, 0x289FC}, //2369 #big5-hkscs
    {0x9748, 0x93BB}, //2370 #big5-hkscs
    {0x9749, 0x93B8}, //2371 #big5-hkscs
    {0x974A, 0x218D6}, //2372 #big5-hkscs
    {0x974B, 0x20F1D}, //2373 #big5-hkscs
    {0x974C, 0x8472}, //2374 #big5-hkscs
    {0x974D, 0x26CC0}, //2375 #big5-hkscs
    {0x974E, 0x21413}, //2376 #big5-hkscs
    {0x974F, 0x242FA}, //2377 #big5-hkscs
    {0x9750, 0x22C26}, //2378 #big5-hkscs
    {0x9751, 0x243C1}, //2379 #big5-hkscs
    {0x9752, 0x5994}, //2380 #big5-hkscs
    {0x9753, 0x23DB7}, //2381 #big5-hkscs
    {0x9754, 0x26741}, //2382 #big5-hkscs
    {0x9755, 0x7DA8}, //2383 #big5-hkscs
    {0x9756, 0x2615B}, //2384 #big5-hkscs
    {0x9757, 0x260A4}, //2385 #big5-hkscs
    {0x9758, 0x249B9}, //2386 #big5-hkscs
    {0x9759, 0x2498B}, //2387 #big5-hkscs
    {0x975A, 0x289FA}, //2388 #big5-hkscs
    {0x975B, 0x92E5}, //2389 #big5-hkscs
    {0x975C, 0x73E2}, //2390 #big5-hkscs
    {0x975D, 0x3EE9}, //2391 #big5-hkscs
    {0x975E, 0x74B4}, //2392 #big5-hkscs
    {0x975F, 0x28B63}, //2393 #big5-hkscs
    {0x9760, 0x2189F}, //2394 #big5-hkscs
    {0x9761, 0x3EE1}, //2395 #big5-hkscs
    {0x9762, 0x24AB3}, //2396 #big5-hkscs
    {0x9763, 0x6AD8}, //2397 #big5-hkscs
    {0x9764, 0x73F3}, //2398 #big5-hkscs
    {0x9765, 0x73FB}, //2399 #big5-hkscs
    {0x9766, 0x3ED6}, //2400 #big5-hkscs
    {0x9767, 0x24A3E}, //2401 #big5-hkscs
    {0x9768, 0x24A94}, //2402 #big5-hkscs
    {0x9769, 0x217D9}, //2403 #big5-hkscs
    {0x976A, 0x24A66}, //2404 #big5-hkscs
    {0x976B, 0x203A7}, //2405 #big5-hkscs
    {0x976C, 0x21424}, //2406 #big5-hkscs
    {0x976D, 0x249E5}, //2407 #big5-hkscs
    {0x976E, 0x7448}, //2408 #big5-hkscs
    {0x976F, 0x24916}, //2409 #big5-hkscs
    {0x9770, 0x70A5}, //2410 #big5-hkscs
    {0x9771, 0x24976}, //2411 #big5-hkscs
    {0x9772, 0x9284}, //2412 #big5-hkscs
    {0x9773, 0x73E6}, //2413 #big5-hkscs
    {0x9774, 0x935F}, //2414 #big5-hkscs
    {0x9775, 0x204FE}, //2415 #big5-hkscs
    {0x9776, 0x9331}, //2416 #big5-hkscs
    {0x9777, 0x28ACE}, //2417 #big5-hkscs
    {0x9778, 0x28A16}, //2418 #big5-hkscs
    {0x9779, 0x9386}, //2419 #big5-hkscs
    {0x977A, 0x28BE7}, //2420 #big5-hkscs
    {0x977B, 0x255D5}, //2421 #big5-hkscs
    {0x977C, 0x4935}, //2422 #big5-hkscs
    {0x977D, 0x28A82}, //2423 #big5-hkscs
    {0x977E, 0x716B}, //2424 #big5-hkscs
    {0x97A1, 0x24943}, //2425 #big5-hkscs
    {0x97A2, 0x20CFF}, //2426 #big5-hkscs
    {0x97A3, 0x56A4}, //2427 #big5-hkscs
    {0x97A4, 0x2061A}, //2428 #big5-hkscs
    {0x97A5, 0x20BEB}, //2429 #big5-hkscs
    {0x97A6, 0x20CB8}, //2430 #big5-hkscs
    {0x97A7, 0x5502}, //2431 #big5-hkscs
    {0x97A8, 0x79C4}, //2432 #big5-hkscs
    {0x97A9, 0x217FA}, //2433 #big5-hkscs
    {0x97AA, 0x7DFE}, //2434 #big5-hkscs
    {0x97AB, 0x216C2}, //2435 #big5-hkscs
    {0x97AC, 0x24A50}, //2436 #big5-hkscs
    {0x97AD, 0x21852}, //2437 #big5-hkscs
    {0x97AE, 0x452E}, //2438 #big5-hkscs
    {0x97AF, 0x9401}, //2439 #big5-hkscs
    {0x97B0, 0x370A}, //2440 #big5-hkscs
    {0x97B1, 0x28AC0}, //2441 #big5-hkscs
    {0x97B2, 0x249AD}, //2442 #big5-hkscs
    {0x97B3, 0x59B0}, //2443 #big5-hkscs
    {0x97B4, 0x218BF}, //2444 #big5-hkscs
    {0x97B5, 0x21883}, //2445 #big5-hkscs
    {0x97B6, 0x27484}, //2446 #big5-hkscs
    {0x97B7, 0x5AA1}, //2447 #big5-hkscs
    {0x97B8, 0x36E2}, //2448 #big5-hkscs
    {0x97B9, 0x23D5B}, //2449 #big5-hkscs
    {0x97BA, 0x36B0}, //2450 #big5-hkscs
    {0x97BB, 0x925F}, //2451 #big5-hkscs
    {0x97BC, 0x5A79}, //2452 #big5-hkscs
    {0x97BD, 0x28A81}, //2453 #big5-hkscs
    {0x97BE, 0x21862}, //2454 #big5-hkscs
    {0x97BF, 0x9374}, //2455 #big5-hkscs
    {0x97C0, 0x3CCD}, //2456 #big5-hkscs
    {0x97C1, 0x20AB4}, //2457 #big5-hkscs
    {0x97C2, 0x4A96}, //2458 #big5-hkscs
    {0x97C3, 0x398A}, //2459 #big5-hkscs
    {0x97C4, 0x50F4}, //2460 #big5-hkscs
    {0x97C5, 0x3D69}, //2461 #big5-hkscs
    {0x97C6, 0x3D4C}, //2462 #big5-hkscs
    {0x97C7, 0x2139C}, //2463 #big5-hkscs
    {0x97C8, 0x7175}, //2464 #big5-hkscs
    {0x97C9, 0x42FB}, //2465 #big5-hkscs
    {0x97CA, 0x28218}, //2466 #big5-hkscs
    {0x97CB, 0x6E0F}, //2467 #big5-hkscs
    {0x97CC, 0x290E4}, //2468 #big5-hkscs
    {0x97CD, 0x44EB}, //2469 #big5-hkscs
    {0x97CE, 0x6D57}, //2470 #big5-hkscs
    {0x97CF, 0x27E4F}, //2471 #big5-hkscs
    {0x97D0, 0x7067}, //2472 #big5-hkscs
    {0x97D1, 0x6CAF}, //2473 #big5-hkscs
    {0x97D2, 0x3CD6}, //2474 #big5-hkscs
    {0x97D3, 0x23FED}, //2475 #big5-hkscs
    {0x97D4, 0x23E2D}, //2476 #big5-hkscs
    {0x97D5, 0x6E02}, //2477 #big5-hkscs
    {0x97D6, 0x6F0C}, //2478 #big5-hkscs
    {0x97D7, 0x3D6F}, //2479 #big5-hkscs
    {0x97D8, 0x203F5}, //2480 #big5-hkscs
    {0x97D9, 0x7551}, //2481 #big5-hkscs
    {0x97DA, 0x36BC}, //2482 #big5-hkscs
    {0x97DB, 0x34C8}, //2483 #big5-hkscs
    {0x97DC, 0x4680}, //2484 #big5-hkscs
    {0x97DD, 0x3EDA}, //2485 #big5-hkscs
    {0x97DE, 0x4871}, //2486 #big5-hkscs
    {0x97DF, 0x59C4}, //2487 #big5-hkscs
    {0x97E0, 0x926E}, //2488 #big5-hkscs
    {0x97E1, 0x493E}, //2489 #big5-hkscs
    {0x97E2, 0x8F41}, //2490 #big5-hkscs
    {0x97E3, 0x28C1C}, //2491 #big5-hkscs
    {0x97E4, 0x26BC0}, //2492 #big5-hkscs
    {0x97E5, 0x5812}, //2493 #big5-hkscs
    {0x97E6, 0x57C8}, //2494 #big5-hkscs
    {0x97E7, 0x36D6}, //2495 #big5-hkscs
    {0x97E8, 0x21452}, //2496 #big5-hkscs
    {0x97E9, 0x70FE}, //2497 #big5-hkscs
    {0x97EA, 0x24362}, //2498 #big5-hkscs
    {0x97EB, 0x24A71}, //2499 #big5-hkscs
    {0x97EC, 0x22FE3}, //2500 #big5-hkscs
    {0x97ED, 0x212B0}, //2501 #big5-hkscs
    {0x97EE, 0x223BD}, //2502 #big5-hkscs
    {0x97EF, 0x68B9}, //2503 #big5-hkscs
    {0x97F0, 0x6967}, //2504 #big5-hkscs
    {0x97F1, 0x21398}, //2505 #big5-hkscs
    {0x97F2, 0x234E5}, //2506 #big5-hkscs
    {0x97F3, 0x27BF4}, //2507 #big5-hkscs
    {0x97F4, 0x236DF}, //2508 #big5-hkscs
    {0x97F5, 0x28A83}, //2509 #big5-hkscs
    {0x97F6, 0x237D6}, //2510 #big5-hkscs
    {0x97F7, 0x233FA}, //2511 #big5-hkscs
    {0x97F8, 0x24C9F}, //2512 #big5-hkscs
    {0x97F9, 0x6A1A}, //2513 #big5-hkscs
    {0x97FA, 0x236AD}, //2514 #big5-hkscs
    {0x97FB, 0x26CB7}, //2515 #big5-hkscs
    {0x97FC, 0x843E}, //2516 #big5-hkscs
    {0x97FD, 0x44DF}, //2517 #big5-hkscs
    {0x97FE, 0x44CE}, //2518 #big5-hkscs
    {0x9840, 0x26D26}, //2519 #big5-hkscs
    {0x9841, 0x26D51}, //2520 #big5-hkscs
    {0x9842, 0x26C82}, //2521 #big5-hkscs
    {0x9843, 0x26FDE}, //2522 #big5-hkscs
    {0x9844, 0x6F17}, //2523 #big5-hkscs
    {0x9845, 0x27109}, //2524 #big5-hkscs
    {0x9846, 0x833D}, //2525 #big5-hkscs
    {0x9847, 0x2173A}, //2526 #big5-hkscs
    {0x9848, 0x83ED}, //2527 #big5-hkscs
    {0x9849, 0x26C80}, //2528 #big5-hkscs
    {0x984A, 0x27053}, //2529 #big5-hkscs
    {0x984B, 0x217DB}, //2530 #big5-hkscs
    {0x984C, 0x5989}, //2531 #big5-hkscs
    {0x984D, 0x5A82}, //2532 #big5-hkscs
    {0x984E, 0x217B3}, //2533 #big5-hkscs
    {0x984F, 0x5A61}, //2534 #big5-hkscs
    {0x9850, 0x5A71}, //2535 #big5-hkscs
    {0x9851, 0x21905}, //2536 #big5-hkscs
    {0x9852, 0x241FC}, //2537 #big5-hkscs
    {0x9853, 0x372D}, //2538 #big5-hkscs
    {0x9854, 0x59EF}, //2539 #big5-hkscs
    {0x9855, 0x2173C}, //2540 #big5-hkscs
    {0x9856, 0x36C7}, //2541 #big5-hkscs
    {0x9857, 0x718E}, //2542 #big5-hkscs
    {0x9858, 0x9390}, //2543 #big5-hkscs
    {0x9859, 0x669A}, //2544 #big5-hkscs
    {0x985A, 0x242A5}, //2545 #big5-hkscs
    {0x985B, 0x5A6E}, //2546 #big5-hkscs
    {0x985C, 0x5A2B}, //2547 #big5-hkscs
    {0x985D, 0x24293}, //2548 #big5-hkscs
    {0x985E, 0x6A2B}, //2549 #big5-hkscs
    {0x985F, 0x23EF9}, //2550 #big5-hkscs
    {0x9860, 0x27736}, //2551 #big5-hkscs
    {0x9861, 0x2445B}, //2552 #big5-hkscs
    {0x9862, 0x242CA}, //2553 #big5-hkscs
    {0x9863, 0x711D}, //2554 #big5-hkscs
    {0x9864, 0x24259}, //2555 #big5-hkscs
    {0x9865, 0x289E1}, //2556 #big5-hkscs
    {0x9866, 0x4FB0}, //2557 #big5-hkscs
    {0x9867, 0x26D28}, //2558 #big5-hkscs
    {0x9868, 0x5CC2}, //2559 #big5-hkscs
    {0x9869, 0x244CE}, //2560 #big5-hkscs
    {0x986A, 0x27E4D}, //2561 #big5-hkscs
    {0x986B, 0x243BD}, //2562 #big5-hkscs
    {0x986C, 0x6A0C}, //2563 #big5-hkscs
    {0x986D, 0x24256}, //2564 #big5-hkscs
    {0x986E, 0x21304}, //2565 #big5-hkscs
    {0x986F, 0x70A6}, //2566 #big5-hkscs
    {0x9870, 0x7133}, //2567 #big5-hkscs
    {0x9871, 0x243E9}, //2568 #big5-hkscs
    {0x9872, 0x3DA5}, //2569 #big5-hkscs
    {0x9873, 0x6CDF}, //2570 #big5-hkscs
    {0x9874, 0x2F825}, //2571 #big5-hkscs
    {0x9875, 0x24A4F}, //2572 #big5-hkscs
    {0x9876, 0x7E65}, //2573 #big5-hkscs
    {0x9877, 0x59EB}, //2574 #big5-hkscs
    {0x9878, 0x5D2F}, //2575 #big5-hkscs
    {0x9879, 0x3DF3}, //2576 #big5-hkscs
    {0x987A, 0x5F5C}, //2577 #big5-hkscs
    {0x987B, 0x24A5D}, //2578 #big5-hkscs
    {0x987C, 0x217DF}, //2579 #big5-hkscs
    {0x987D, 0x7DA4}, //2580 #big5-hkscs
    {0x987E, 0x8426}, //2581 #big5-hkscs
    {0x98A1, 0x5485}, //2582 #big5-hkscs
    {0x98A2, 0x23AFA}, //2583 #big5-hkscs
    {0x98A3, 0x23300}, //2584 #big5-hkscs
    {0x98A4, 0x20214}, //2585 #big5-hkscs
    {0x98A5, 0x577E}, //2586 #big5-hkscs
    {0x98A6, 0x208D5}, //2587 #big5-hkscs
    {0x98A7, 0x20619}, //2588 #big5-hkscs
    {0x98A8, 0x3FE5}, //2589 #big5-hkscs
    {0x98A9, 0x21F9E}, //2590 #big5-hkscs
    {0x98AA, 0x2A2B6}, //2591 #big5-hkscs
    {0x98AB, 0x7003}, //2592 #big5-hkscs
    {0x98AC, 0x2915B}, //2593 #big5-hkscs
    {0x98AD, 0x5D70}, //2594 #big5-hkscs
    {0x98AE, 0x738F}, //2595 #big5-hkscs
    {0x98AF, 0x7CD3}, //2596 #big5-hkscs
    {0x98B0, 0x28A59}, //2597 #big5-hkscs
    {0x98B1, 0x29420}, //2598 #big5-hkscs
    {0x98B2, 0x4FC8}, //2599 #big5-hkscs
    {0x98B3, 0x7FE7}, //2600 #big5-hkscs
    {0x98B4, 0x72CD}, //2601 #big5-hkscs
    {0x98B5, 0x7310}, //2602 #big5-hkscs
    {0x98B6, 0x27AF4}, //2603 #big5-hkscs
    {0x98B7, 0x7338}, //2604 #big5-hkscs
    {0x98B8, 0x7339}, //2605 #big5-hkscs
    {0x98B9, 0x256F6}, //2606 #big5-hkscs
    {0x98BA, 0x7341}, //2607 #big5-hkscs
    {0x98BB, 0x7348}, //2608 #big5-hkscs
    {0x98BC, 0x3EA9}, //2609 #big5-hkscs
    {0x98BD, 0x27B18}, //2610 #big5-hkscs
    {0x98BE, 0x906C}, //2611 #big5-hkscs
    {0x98BF, 0x71F5}, //2612 #big5-hkscs
    {0x98C0, 0x248F2}, //2613 #big5-hkscs
    {0x98C1, 0x73E1}, //2614 #big5-hkscs
    {0x98C2, 0x81F6}, //2615 #big5-hkscs
    {0x98C3, 0x3ECA}, //2616 #big5-hkscs
    {0x98C4, 0x770C}, //2617 #big5-hkscs
    {0x98C5, 0x3ED1}, //2618 #big5-hkscs
    {0x98C6, 0x6CA2}, //2619 #big5-hkscs
    {0x98C7, 0x56FD}, //2620 #big5-hkscs
    {0x98C8, 0x7419}, //2621 #big5-hkscs
    {0x98C9, 0x741E}, //2622 #big5-hkscs
    {0x98CA, 0x741F}, //2623 #big5-hkscs
    {0x98CB, 0x3EE2}, //2624 #big5-hkscs
    {0x98CC, 0x3EF0}, //2625 #big5-hkscs
    {0x98CD, 0x3EF4}, //2626 #big5-hkscs
    {0x98CE, 0x3EFA}, //2627 #big5-hkscs
    {0x98CF, 0x74D3}, //2628 #big5-hkscs
    {0x98D0, 0x3F0E}, //2629 #big5-hkscs
    {0x98D1, 0x3F53}, //2630 #big5-hkscs
    {0x98D2, 0x7542}, //2631 #big5-hkscs
    {0x98D3, 0x756D}, //2632 #big5-hkscs
    {0x98D4, 0x7572}, //2633 #big5-hkscs
    {0x98D5, 0x758D}, //2634 #big5-hkscs
    {0x98D6, 0x3F7C}, //2635 #big5-hkscs
    {0x98D7, 0x75C8}, //2636 #big5-hkscs
    {0x98D8, 0x75DC}, //2637 #big5-hkscs
    {0x98D9, 0x3FC0}, //2638 #big5-hkscs
    {0x98DA, 0x764D}, //2639 #big5-hkscs
    {0x98DB, 0x3FD7}, //2640 #big5-hkscs
    {0x98DC, 0x7674}, //2641 #big5-hkscs
    {0x98DD, 0x3FDC}, //2642 #big5-hkscs
    {0x98DE, 0x767A}, //2643 #big5-hkscs
    {0x98DF, 0x24F5C}, //2644 #big5-hkscs
    {0x98E0, 0x7188}, //2645 #big5-hkscs
    {0x98E1, 0x5623}, //2646 #big5-hkscs
    {0x98E2, 0x8980}, //2647 #big5-hkscs
    {0x98E3, 0x5869}, //2648 #big5-hkscs
    {0x98E4, 0x401D}, //2649 #big5-hkscs
    {0x98E5, 0x7743}, //2650 #big5-hkscs
    {0x98E6, 0x4039}, //2651 #big5-hkscs
    {0x98E7, 0x6761}, //2652 #big5-hkscs
    {0x98E8, 0x4045}, //2653 #big5-hkscs
    {0x98E9, 0x35DB}, //2654 #big5-hkscs
    {0x98EA, 0x7798}, //2655 #big5-hkscs
    {0x98EB, 0x406A}, //2656 #big5-hkscs
    {0x98EC, 0x406F}, //2657 #big5-hkscs
    {0x98ED, 0x5C5E}, //2658 #big5-hkscs
    {0x98EE, 0x77BE}, //2659 #big5-hkscs
    {0x98EF, 0x77CB}, //2660 #big5-hkscs
    {0x98F0, 0x58F2}, //2661 #big5-hkscs
    {0x98F1, 0x7818}, //2662 #big5-hkscs
    {0x98F2, 0x70B9}, //2663 #big5-hkscs
    {0x98F3, 0x781C}, //2664 #big5-hkscs
    {0x98F4, 0x40A8}, //2665 #big5-hkscs
    {0x98F5, 0x7839}, //2666 #big5-hkscs
    {0x98F6, 0x7847}, //2667 #big5-hkscs
    {0x98F7, 0x7851}, //2668 #big5-hkscs
    {0x98F8, 0x7866}, //2669 #big5-hkscs
    {0x98F9, 0x8448}, //2670 #big5-hkscs
    {0x98FA, 0x25535}, //2671 #big5-hkscs
    {0x98FB, 0x7933}, //2672 #big5-hkscs
    {0x98FC, 0x6803}, //2673 #big5-hkscs
    {0x98FD, 0x7932}, //2674 #big5-hkscs
    {0x98FE, 0x4103}, //2675 #big5-hkscs
    {0x9940, 0x4109}, //2676 #big5-hkscs
    {0x9941, 0x7991}, //2677 #big5-hkscs
    {0x9942, 0x7999}, //2678 #big5-hkscs
    {0x9943, 0x8FBB}, //2679 #big5-hkscs
    {0x9944, 0x7A06}, //2680 #big5-hkscs
    {0x9945, 0x8FBC}, //2681 #big5-hkscs
    {0x9946, 0x4167}, //2682 #big5-hkscs
    {0x9947, 0x7A91}, //2683 #big5-hkscs
    {0x9948, 0x41B2}, //2684 #big5-hkscs
    {0x9949, 0x7ABC}, //2685 #big5-hkscs
    {0x994A, 0x8279}, //2686 #big5-hkscs
    {0x994B, 0x41C4}, //2687 #big5-hkscs
    {0x994C, 0x7ACF}, //2688 #big5-hkscs
    {0x994D, 0x7ADB}, //2689 #big5-hkscs
    {0x994E, 0x41CF}, //2690 #big5-hkscs
    {0x994F, 0x4E21}, //2691 #big5-hkscs
    {0x9950, 0x7B62}, //2692 #big5-hkscs
    {0x9951, 0x7B6C}, //2693 #big5-hkscs
    {0x9952, 0x7B7B}, //2694 #big5-hkscs
    {0x9953, 0x7C12}, //2695 #big5-hkscs
    {0x9954, 0x7C1B}, //2696 #big5-hkscs
    {0x9955, 0x4260}, //2697 #big5-hkscs
    {0x9956, 0x427A}, //2698 #big5-hkscs
    {0x9957, 0x7C7B}, //2699 #big5-hkscs
    {0x9958, 0x7C9C}, //2700 #big5-hkscs
    {0x9959, 0x428C}, //2701 #big5-hkscs
    {0x995A, 0x7CB8}, //2702 #big5-hkscs
    {0x995B, 0x4294}, //2703 #big5-hkscs
    {0x995C, 0x7CED}, //2704 #big5-hkscs
    {0x995D, 0x8F93}, //2705 #big5-hkscs
    {0x995E, 0x70C0}, //2706 #big5-hkscs
    {0x995F, 0x20CCF}, //2707 #big5-hkscs
    {0x9960, 0x7DCF}, //2708 #big5-hkscs
    {0x9961, 0x7DD4}, //2709 #big5-hkscs
    {0x9962, 0x7DD0}, //2710 #big5-hkscs
    {0x9963, 0x7DFD}, //2711 #big5-hkscs
    {0x9964, 0x7FAE}, //2712 #big5-hkscs
    {0x9965, 0x7FB4}, //2713 #big5-hkscs
    {0x9966, 0x729F}, //2714 #big5-hkscs
    {0x9967, 0x4397}, //2715 #big5-hkscs
    {0x9968, 0x8020}, //2716 #big5-hkscs
    {0x9969, 0x8025}, //2717 #big5-hkscs
    {0x996A, 0x7B39}, //2718 #big5-hkscs
    {0x996B, 0x802E}, //2719 #big5-hkscs
    {0x996C, 0x8031}, //2720 #big5-hkscs
    {0x996D, 0x8054}, //2721 #big5-hkscs
    {0x996E, 0x3DCC}, //2722 #big5-hkscs
    {0x996F, 0x57B4}, //2723 #big5-hkscs
    {0x9970, 0x70A0}, //2724 #big5-hkscs
    {0x9971, 0x80B7}, //2725 #big5-hkscs
    {0x9972, 0x80E9}, //2726 #big5-hkscs
    {0x9973, 0x43ED}, //2727 #big5-hkscs
    {0x9974, 0x810C}, //2728 #big5-hkscs
    {0x9975, 0x732A}, //2729 #big5-hkscs
    {0x9976, 0x810E}, //2730 #big5-hkscs
    {0x9977, 0x8112}, //2731 #big5-hkscs
    {0x9978, 0x7560}, //2732 #big5-hkscs
    {0x9979, 0x8114}, //2733 #big5-hkscs
    {0x997A, 0x4401}, //2734 #big5-hkscs
    {0x997B, 0x3B39}, //2735 #big5-hkscs
    {0x997C, 0x8156}, //2736 #big5-hkscs
    {0x997D, 0x8159}, //2737 #big5-hkscs
    {0x997E, 0x815A}, //2738 #big5-hkscs
    {0x99A1, 0x4413}, //2739 #big5-hkscs
    {0x99A2, 0x583A}, //2740 #big5-hkscs
    {0x99A3, 0x817C}, //2741 #big5-hkscs
    {0x99A4, 0x8184}, //2742 #big5-hkscs
    {0x99A5, 0x4425}, //2743 #big5-hkscs
    {0x99A6, 0x8193}, //2744 #big5-hkscs
    {0x99A7, 0x442D}, //2745 #big5-hkscs
    {0x99A8, 0x81A5}, //2746 #big5-hkscs
    {0x99A9, 0x57EF}, //2747 #big5-hkscs
    {0x99AA, 0x81C1}, //2748 #big5-hkscs
    {0x99AB, 0x81E4}, //2749 #big5-hkscs
    {0x99AC, 0x8254}, //2750 #big5-hkscs
    {0x99AD, 0x448F}, //2751 #big5-hkscs
    {0x99AE, 0x82A6}, //2752 #big5-hkscs
    {0x99AF, 0x8276}, //2753 #big5-hkscs
    {0x99B0, 0x82CA}, //2754 #big5-hkscs
    {0x99B1, 0x82D8}, //2755 #big5-hkscs
    {0x99B2, 0x82FF}, //2756 #big5-hkscs
    {0x99B3, 0x44B0}, //2757 #big5-hkscs
    {0x99B4, 0x8357}, //2758 #big5-hkscs
    {0x99B5, 0x9669}, //2759 #big5-hkscs
    {0x99B6, 0x698A}, //2760 #big5-hkscs
    {0x99B7, 0x8405}, //2761 #big5-hkscs
    {0x99B8, 0x70F5}, //2762 #big5-hkscs
    {0x99B9, 0x8464}, //2763 #big5-hkscs
    {0x99BA, 0x60E3}, //2764 #big5-hkscs
    {0x99BB, 0x8488}, //2765 #big5-hkscs
    {0x99BC, 0x4504}, //2766 #big5-hkscs
    {0x99BD, 0x84BE}, //2767 #big5-hkscs
    {0x99BE, 0x84E1}, //2768 #big5-hkscs
    {0x99BF, 0x84F8}, //2769 #big5-hkscs
    {0x99C0, 0x8510}, //2770 #big5-hkscs
    {0x99C1, 0x8538}, //2771 #big5-hkscs
    {0x99C2, 0x8552}, //2772 #big5-hkscs
    {0x99C3, 0x453B}, //2773 #big5-hkscs
    {0x99C4, 0x856F}, //2774 #big5-hkscs
    {0x99C5, 0x8570}, //2775 #big5-hkscs
    {0x99C6, 0x85E0}, //2776 #big5-hkscs
    {0x99C7, 0x4577}, //2777 #big5-hkscs
    {0x99C8, 0x8672}, //2778 #big5-hkscs
    {0x99C9, 0x8692}, //2779 #big5-hkscs
    {0x99CA, 0x86B2}, //2780 #big5-hkscs
    {0x99CB, 0x86EF}, //2781 #big5-hkscs
    {0x99CC, 0x9645}, //2782 #big5-hkscs
    {0x99CD, 0x878B}, //2783 #big5-hkscs
    {0x99CE, 0x4606}, //2784 #big5-hkscs
    {0x99CF, 0x4617}, //2785 #big5-hkscs
    {0x99D0, 0x88AE}, //2786 #big5-hkscs
    {0x99D1, 0x88FF}, //2787 #big5-hkscs
    {0x99D2, 0x8924}, //2788 #big5-hkscs
    {0x99D3, 0x8947}, //2789 #big5-hkscs
    {0x99D4, 0x8991}, //2790 #big5-hkscs
    {0x99D5, 0x27967}, //2791 #big5-hkscs
    {0x99D6, 0x8A29}, //2792 #big5-hkscs
    {0x99D7, 0x8A38}, //2793 #big5-hkscs
    {0x99D8, 0x8A94}, //2794 #big5-hkscs
    {0x99D9, 0x8AB4}, //2795 #big5-hkscs
    {0x99DA, 0x8C51}, //2796 #big5-hkscs
    {0x99DB, 0x8CD4}, //2797 #big5-hkscs
    {0x99DC, 0x8CF2}, //2798 #big5-hkscs
    {0x99DD, 0x8D1C}, //2799 #big5-hkscs
    {0x99DE, 0x4798}, //2800 #big5-hkscs
    {0x99DF, 0x585F}, //2801 #big5-hkscs
    {0x99E0, 0x8DC3}, //2802 #big5-hkscs
    {0x99E1, 0x47ED}, //2803 #big5-hkscs
    {0x99E2, 0x4EEE}, //2804 #big5-hkscs
    {0x99E3, 0x8E3A}, //2805 #big5-hkscs
    {0x99E4, 0x55D8}, //2806 #big5-hkscs
    {0x99E5, 0x5754}, //2807 #big5-hkscs
    {0x99E6, 0x8E71}, //2808 #big5-hkscs
    {0x99E7, 0x55F5}, //2809 #big5-hkscs
    {0x99E8, 0x8EB0}, //2810 #big5-hkscs
    {0x99E9, 0x4837}, //2811 #big5-hkscs
    {0x99EA, 0x8ECE}, //2812 #big5-hkscs
    {0x99EB, 0x8EE2}, //2813 #big5-hkscs
    {0x99EC, 0x8EE4}, //2814 #big5-hkscs
    {0x99ED, 0x8EED}, //2815 #big5-hkscs
    {0x99EE, 0x8EF2}, //2816 #big5-hkscs
    {0x99EF, 0x8FB7}, //2817 #big5-hkscs
    {0x99F0, 0x8FC1}, //2818 #big5-hkscs
    {0x99F1, 0x8FCA}, //2819 #big5-hkscs
    {0x99F2, 0x8FCC}, //2820 #big5-hkscs
    {0x99F3, 0x9033}, //2821 #big5-hkscs
    {0x99F4, 0x99C4}, //2822 #big5-hkscs
    {0x99F5, 0x48AD}, //2823 #big5-hkscs
    {0x99F6, 0x98E0}, //2824 #big5-hkscs
    {0x99F7, 0x9213}, //2825 #big5-hkscs
    {0x99F8, 0x491E}, //2826 #big5-hkscs
    {0x99F9, 0x9228}, //2827 #big5-hkscs
    {0x99FA, 0x9258}, //2828 #big5-hkscs
    {0x99FB, 0x926B}, //2829 #big5-hkscs
    {0x99FC, 0x92B1}, //2830 #big5-hkscs
    {0x99FD, 0x92AE}, //2831 #big5-hkscs
    {0x99FE, 0x92BF}, //2832 #big5-hkscs
    {0x9A40, 0x92E3}, //2833 #big5-hkscs
    {0x9A41, 0x92EB}, //2834 #big5-hkscs
    {0x9A42, 0x92F3}, //2835 #big5-hkscs
    {0x9A43, 0x92F4}, //2836 #big5-hkscs
    {0x9A44, 0x92FD}, //2837 #big5-hkscs
    {0x9A45, 0x9343}, //2838 #big5-hkscs
    {0x9A46, 0x9384}, //2839 #big5-hkscs
    {0x9A47, 0x93AD}, //2840 #big5-hkscs
    {0x9A48, 0x4945}, //2841 #big5-hkscs
    {0x9A49, 0x4951}, //2842 #big5-hkscs
    {0x9A4A, 0x9EBF}, //2843 #big5-hkscs
    {0x9A4B, 0x9417}, //2844 #big5-hkscs
    {0x9A4C, 0x5301}, //2845 #big5-hkscs
    {0x9A4D, 0x941D}, //2846 #big5-hkscs
    {0x9A4E, 0x942D}, //2847 #big5-hkscs
    {0x9A4F, 0x943E}, //2848 #big5-hkscs
    {0x9A50, 0x496A}, //2849 #big5-hkscs
    {0x9A51, 0x9454}, //2850 #big5-hkscs
    {0x9A52, 0x9479}, //2851 #big5-hkscs
    {0x9A53, 0x952D}, //2852 #big5-hkscs
    {0x9A54, 0x95A2}, //2853 #big5-hkscs
    {0x9A55, 0x49A7}, //2854 #big5-hkscs
    {0x9A56, 0x95F4}, //2855 #big5-hkscs
    {0x9A57, 0x9633}, //2856 #big5-hkscs
    {0x9A58, 0x49E5}, //2857 #big5-hkscs
    {0x9A59, 0x67A0}, //2858 #big5-hkscs
    {0x9A5A, 0x4A24}, //2859 #big5-hkscs
    {0x9A5B, 0x9740}, //2860 #big5-hkscs
    {0x9A5C, 0x4A35}, //2861 #big5-hkscs
    {0x9A5D, 0x97B2}, //2862 #big5-hkscs
    {0x9A5E, 0x97C2}, //2863 #big5-hkscs
    {0x9A5F, 0x5654}, //2864 #big5-hkscs
    {0x9A60, 0x4AE4}, //2865 #big5-hkscs
    {0x9A61, 0x60E8}, //2866 #big5-hkscs
    {0x9A62, 0x98B9}, //2867 #big5-hkscs
    {0x9A63, 0x4B19}, //2868 #big5-hkscs
    {0x9A64, 0x98F1}, //2869 #big5-hkscs
    {0x9A65, 0x5844}, //2870 #big5-hkscs
    {0x9A66, 0x990E}, //2871 #big5-hkscs
    {0x9A67, 0x9919}, //2872 #big5-hkscs
    {0x9A68, 0x51B4}, //2873 #big5-hkscs
    {0x9A69, 0x991C}, //2874 #big5-hkscs
    {0x9A6A, 0x9937}, //2875 #big5-hkscs
    {0x9A6B, 0x9942}, //2876 #big5-hkscs
    {0x9A6C, 0x995D}, //2877 #big5-hkscs
    {0x9A6D, 0x9962}, //2878 #big5-hkscs
    {0x9A6E, 0x4B70}, //2879 #big5-hkscs
    {0x9A6F, 0x99C5}, //2880 #big5-hkscs
    {0x9A70, 0x4B9D}, //2881 #big5-hkscs
    {0x9A71, 0x9A3C}, //2882 #big5-hkscs
    {0x9A72, 0x9B0F}, //2883 #big5-hkscs
    {0x9A73, 0x7A83}, //2884 #big5-hkscs
    {0x9A74, 0x9B69}, //2885 #big5-hkscs
    {0x9A75, 0x9B81}, //2886 #big5-hkscs
    {0x9A76, 0x9BDD}, //2887 #big5-hkscs
    {0x9A77, 0x9BF1}, //2888 #big5-hkscs
    {0x9A78, 0x9BF4}, //2889 #big5-hkscs
    {0x9A79, 0x4C6D}, //2890 #big5-hkscs
    {0x9A7A, 0x9C20}, //2891 #big5-hkscs
    {0x9A7B, 0x376F}, //2892 #big5-hkscs
    {0x9A7C, 0x21BC2}, //2893 #big5-hkscs
    {0x9A7D, 0x9D49}, //2894 #big5-hkscs
    {0x9A7E, 0x9C3A}, //2895 #big5-hkscs
    {0x9AA1, 0x9EFE}, //2896 #big5-hkscs
    {0x9AA2, 0x5650}, //2897 #big5-hkscs
    {0x9AA3, 0x9D93}, //2898 #big5-hkscs
    {0x9AA4, 0x9DBD}, //2899 #big5-hkscs
    {0x9AA5, 0x9DC0}, //2900 #big5-hkscs
    {0x9AA6, 0x9DFC}, //2901 #big5-hkscs
    {0x9AA7, 0x94F6}, //2902 #big5-hkscs
    {0x9AA8, 0x8FB6}, //2903 #big5-hkscs
    {0x9AA9, 0x9E7B}, //2904 #big5-hkscs
    {0x9AAA, 0x9EAC}, //2905 #big5-hkscs
    {0x9AAB, 0x9EB1}, //2906 #big5-hkscs
    {0x9AAC, 0x9EBD}, //2907 #big5-hkscs
    {0x9AAD, 0x9EC6}, //2908 #big5-hkscs
    {0x9AAE, 0x94DC}, //2909 #big5-hkscs
    {0x9AAF, 0x9EE2}, //2910 #big5-hkscs
    {0x9AB0, 0x9EF1}, //2911 #big5-hkscs
    {0x9AB1, 0x9EF8}, //2912 #big5-hkscs
    {0x9AB2, 0x7AC8}, //2913 #big5-hkscs
    {0x9AB3, 0x9F44}, //2914 #big5-hkscs
    {0x9AB4, 0x20094}, //2915 #big5-hkscs
    {0x9AB5, 0x202B7}, //2916 #big5-hkscs
    {0x9AB6, 0x203A0}, //2917 #big5-hkscs
    {0x9AB7, 0x691A}, //2918 #big5-hkscs
    {0x9AB8, 0x94C3}, //2919 #big5-hkscs
    {0x9AB9, 0x59AC}, //2920 #big5-hkscs
    {0x9ABA, 0x204D7}, //2921 #big5-hkscs
    {0x9ABB, 0x5840}, //2922 #big5-hkscs
    {0x9ABC, 0x94C1}, //2923 #big5-hkscs
    {0x9ABD, 0x37B9}, //2924 #big5-hkscs
    {0x9ABE, 0x205D5}, //2925 #big5-hkscs
    {0x9ABF, 0x20615}, //2926 #big5-hkscs
    {0x9AC0, 0x20676}, //2927 #big5-hkscs
    {0x9AC1, 0x216BA}, //2928 #big5-hkscs
    {0x9AC2, 0x5757}, //2929 #big5-hkscs
    {0x9AC3, 0x7173}, //2930 #big5-hkscs
    {0x9AC4, 0x20AC2}, //2931 #big5-hkscs
    {0x9AC5, 0x20ACD}, //2932 #big5-hkscs
    {0x9AC6, 0x20BBF}, //2933 #big5-hkscs
    {0x9AC7, 0x546A}, //2934 #big5-hkscs
    {0x9AC8, 0x2F83B}, //2935 #big5-hkscs
    {0x9AC9, 0x20BCB}, //2936 #big5-hkscs
    {0x9ACA, 0x549E}, //2937 #big5-hkscs
    {0x9ACB, 0x20BFB}, //2938 #big5-hkscs
    {0x9ACC, 0x20C3B}, //2939 #big5-hkscs
    {0x9ACD, 0x20C53}, //2940 #big5-hkscs
    {0x9ACE, 0x20C65}, //2941 #big5-hkscs
    {0x9ACF, 0x20C7C}, //2942 #big5-hkscs
    {0x9AD0, 0x60E7}, //2943 #big5-hkscs
    {0x9AD1, 0x20C8D}, //2944 #big5-hkscs
    {0x9AD2, 0x567A}, //2945 #big5-hkscs
    {0x9AD3, 0x20CB5}, //2946 #big5-hkscs
    {0x9AD4, 0x20CDD}, //2947 #big5-hkscs
    {0x9AD5, 0x20CED}, //2948 #big5-hkscs
    {0x9AD6, 0x20D6F}, //2949 #big5-hkscs
    {0x9AD7, 0x20DB2}, //2950 #big5-hkscs
    {0x9AD8, 0x20DC8}, //2951 #big5-hkscs
    {0x9AD9, 0x6955}, //2952 #big5-hkscs
    {0x9ADA, 0x9C2F}, //2953 #big5-hkscs
    {0x9ADB, 0x87A5}, //2954 #big5-hkscs
    {0x9ADC, 0x20E04}, //2955 #big5-hkscs
    {0x9ADD, 0x20E0E}, //2956 #big5-hkscs
    {0x9ADE, 0x20ED7}, //2957 #big5-hkscs
    {0x9ADF, 0x20F90}, //2958 #big5-hkscs
    {0x9AE0, 0x20F2D}, //2959 #big5-hkscs
    {0x9AE1, 0x20E73}, //2960 #big5-hkscs
    {0x9AE2, 0x5C20}, //2961 #big5-hkscs
    {0x9AE3, 0x20FBC}, //2962 #big5-hkscs
    {0x9AE4, 0x5E0B}, //2963 #big5-hkscs
    {0x9AE5, 0x2105C}, //2964 #big5-hkscs
    {0x9AE6, 0x2104F}, //2965 #big5-hkscs
    {0x9AE7, 0x21076}, //2966 #big5-hkscs
    {0x9AE8, 0x671E}, //2967 #big5-hkscs
    {0x9AE9, 0x2107B}, //2968 #big5-hkscs
    {0x9AEA, 0x21088}, //2969 #big5-hkscs
    {0x9AEB, 0x21096}, //2970 #big5-hkscs
    {0x9AEC, 0x3647}, //2971 #big5-hkscs
    {0x9AED, 0x210BF}, //2972 #big5-hkscs
    {0x9AEE, 0x210D3}, //2973 #big5-hkscs
    {0x9AEF, 0x2112F}, //2974 #big5-hkscs
    {0x9AF0, 0x2113B}, //2975 #big5-hkscs
    {0x9AF1, 0x5364}, //2976 #big5-hkscs
    {0x9AF2, 0x84AD}, //2977 #big5-hkscs
    {0x9AF3, 0x212E3}, //2978 #big5-hkscs
    {0x9AF4, 0x21375}, //2979 #big5-hkscs
    {0x9AF5, 0x21336}, //2980 #big5-hkscs
    {0x9AF6, 0x8B81}, //2981 #big5-hkscs
    {0x9AF7, 0x21577}, //2982 #big5-hkscs
    {0x9AF8, 0x21619}, //2983 #big5-hkscs
    {0x9AF9, 0x217C3}, //2984 #big5-hkscs
    {0x9AFA, 0x217C7}, //2985 #big5-hkscs
    {0x9AFB, 0x4E78}, //2986 #big5-hkscs
    {0x9AFC, 0x70BB}, //2987 #big5-hkscs
    {0x9AFD, 0x2182D}, //2988 #big5-hkscs
    {0x9AFE, 0x2196A}, //2989 #big5-hkscs
    {0x9B40, 0x21A2D}, //2990 #big5-hkscs
    {0x9B41, 0x21A45}, //2991 #big5-hkscs
    {0x9B42, 0x21C2A}, //2992 #big5-hkscs
    {0x9B43, 0x21C70}, //2993 #big5-hkscs
    {0x9B44, 0x21CAC}, //2994 #big5-hkscs
    {0x9B45, 0x21EC8}, //2995 #big5-hkscs
    {0x9B46, 0x62C3}, //2996 #big5-hkscs
    {0x9B47, 0x21ED5}, //2997 #big5-hkscs
    {0x9B48, 0x21F15}, //2998 #big5-hkscs
    {0x9B49, 0x7198}, //2999 #big5-hkscs
    {0x9B4A, 0x6855}, //3000 #big5-hkscs
    {0x9B4B, 0x22045}, //3001 #big5-hkscs
    {0x9B4C, 0x69E9}, //3002 #big5-hkscs
    {0x9B4D, 0x36C8}, //3003 #big5-hkscs
    {0x9B4E, 0x2227C}, //3004 #big5-hkscs
    {0x9B4F, 0x223D7}, //3005 #big5-hkscs
    {0x9B50, 0x223FA}, //3006 #big5-hkscs
    {0x9B51, 0x2272A}, //3007 #big5-hkscs
    {0x9B52, 0x22871}, //3008 #big5-hkscs
    {0x9B53, 0x2294F}, //3009 #big5-hkscs
    {0x9B54, 0x82FD}, //3010 #big5-hkscs
    {0x9B55, 0x22967}, //3011 #big5-hkscs
    {0x9B56, 0x22993}, //3012 #big5-hkscs
    {0x9B57, 0x22AD5}, //3013 #big5-hkscs
    {0x9B58, 0x89A5}, //3014 #big5-hkscs
    {0x9B59, 0x22AE8}, //3015 #big5-hkscs
    {0x9B5A, 0x8FA0}, //3016 #big5-hkscs
    {0x9B5B, 0x22B0E}, //3017 #big5-hkscs
    {0x9B5C, 0x97B8}, //3018 #big5-hkscs
    {0x9B5D, 0x22B3F}, //3019 #big5-hkscs
    {0x9B5E, 0x9847}, //3020 #big5-hkscs
    {0x9B5F, 0x9ABD}, //3021 #big5-hkscs
    {0x9B60, 0x22C4C}, //3022 #big5-hkscs
    {0x9B62, 0x22C88}, //3023 #big5-hkscs
    {0x9B63, 0x22CB7}, //3024 #big5-hkscs
    {0x9B64, 0x25BE8}, //3025 #big5-hkscs
    {0x9B65, 0x22D08}, //3026 #big5-hkscs
    {0x9B66, 0x22D12}, //3027 #big5-hkscs
    {0x9B67, 0x22DB7}, //3028 #big5-hkscs
    {0x9B68, 0x22D95}, //3029 #big5-hkscs
    {0x9B69, 0x22E42}, //3030 #big5-hkscs
    {0x9B6A, 0x22F74}, //3031 #big5-hkscs
    {0x9B6B, 0x22FCC}, //3032 #big5-hkscs
    {0x9B6C, 0x23033}, //3033 #big5-hkscs
    {0x9B6D, 0x23066}, //3034 #big5-hkscs
    {0x9B6E, 0x2331F}, //3035 #big5-hkscs
    {0x9B6F, 0x233DE}, //3036 #big5-hkscs
    {0x9B70, 0x5FB1}, //3037 #big5-hkscs
    {0x9B71, 0x6648}, //3038 #big5-hkscs
    {0x9B72, 0x66BF}, //3039 #big5-hkscs
    {0x9B73, 0x27A79}, //3040 #big5-hkscs
    {0x9B74, 0x23567}, //3041 #big5-hkscs
    {0x9B75, 0x235F3}, //3042 #big5-hkscs
    {0x9B76, 0x7201}, //3043 #big5-hkscs
    {0x9B77, 0x249BA}, //3044 #big5-hkscs
    {0x9B78, 0x77D7}, //3045 #big5-hkscs
    {0x9B79, 0x2361A}, //3046 #big5-hkscs
    {0x9B7A, 0x23716}, //3047 #big5-hkscs
    {0x9B7B, 0x7E87}, //3048 #big5-hkscs
    {0x9B7C, 0x20346}, //3049 #big5-hkscs
    {0x9B7D, 0x58B5}, //3050 #big5-hkscs
    {0x9B7E, 0x670E}, //3051 #big5-hkscs
    {0x9BA1, 0x6918}, //3052 #big5-hkscs
    {0x9BA2, 0x23AA7}, //3053 #big5-hkscs
    {0x9BA3, 0x27657}, //3054 #big5-hkscs
    {0x9BA4, 0x25FE2}, //3055 #big5-hkscs
    {0x9BA5, 0x23E11}, //3056 #big5-hkscs
    {0x9BA6, 0x23EB9}, //3057 #big5-hkscs
    {0x9BA7, 0x275FE}, //3058 #big5-hkscs
    {0x9BA8, 0x2209A}, //3059 #big5-hkscs
    {0x9BA9, 0x48D0}, //3060 #big5-hkscs
    {0x9BAA, 0x4AB8}, //3061 #big5-hkscs
    {0x9BAB, 0x24119}, //3062 #big5-hkscs
    {0x9BAC, 0x28A9A}, //3063 #big5-hkscs
    {0x9BAD, 0x242EE}, //3064 #big5-hkscs
    {0x9BAE, 0x2430D}, //3065 #big5-hkscs
    {0x9BAF, 0x2403B}, //3066 #big5-hkscs
    {0x9BB0, 0x24334}, //3067 #big5-hkscs
    {0x9BB1, 0x24396}, //3068 #big5-hkscs
    {0x9BB2, 0x24A45}, //3069 #big5-hkscs
    {0x9BB3, 0x205CA}, //3070 #big5-hkscs
    {0x9BB4, 0x51D2}, //3071 #big5-hkscs
    {0x9BB5, 0x20611}, //3072 #big5-hkscs
    {0x9BB6, 0x599F}, //3073 #big5-hkscs
    {0x9BB7, 0x21EA8}, //3074 #big5-hkscs
    {0x9BB8, 0x3BBE}, //3075 #big5-hkscs
    {0x9BB9, 0x23CFF}, //3076 #big5-hkscs
    {0x9BBA, 0x24404}, //3077 #big5-hkscs
    {0x9BBB, 0x244D6}, //3078 #big5-hkscs
    {0x9BBC, 0x5788}, //3079 #big5-hkscs
    {0x9BBD, 0x24674}, //3080 #big5-hkscs
    {0x9BBE, 0x399B}, //3081 #big5-hkscs
    {0x9BBF, 0x2472F}, //3082 #big5-hkscs
    {0x9BC0, 0x285E8}, //3083 #big5-hkscs
    {0x9BC1, 0x299C9}, //3084 #big5-hkscs
    {0x9BC2, 0x3762}, //3085 #big5-hkscs
    {0x9BC3, 0x221C3}, //3086 #big5-hkscs
    {0x9BC4, 0x8B5E}, //3087 #big5-hkscs
    {0x9BC5, 0x28B4E}, //3088 #big5-hkscs
    {0x9BC6, 0x99D6}, //3089 #big5-hkscs
    {0x9BC7, 0x24812}, //3090 #big5-hkscs
    {0x9BC8, 0x248FB}, //3091 #big5-hkscs
    {0x9BC9, 0x24A15}, //3092 #big5-hkscs
    {0x9BCA, 0x7209}, //3093 #big5-hkscs
    {0x9BCB, 0x24AC0}, //3094 #big5-hkscs
    {0x9BCC, 0x20C78}, //3095 #big5-hkscs
    {0x9BCD, 0x5965}, //3096 #big5-hkscs
    {0x9BCE, 0x24EA5}, //3097 #big5-hkscs
    {0x9BCF, 0x24F86}, //3098 #big5-hkscs
    {0x9BD0, 0x20779}, //3099 #big5-hkscs
    {0x9BD1, 0x8EDA}, //3100 #big5-hkscs
    {0x9BD2, 0x2502C}, //3101 #big5-hkscs
    {0x9BD3, 0x528F}, //3102 #big5-hkscs
    {0x9BD4, 0x573F}, //3103 #big5-hkscs
    {0x9BD5, 0x7171}, //3104 #big5-hkscs
    {0x9BD6, 0x25299}, //3105 #big5-hkscs
    {0x9BD7, 0x25419}, //3106 #big5-hkscs
    {0x9BD8, 0x23F4A}, //3107 #big5-hkscs
    {0x9BD9, 0x24AA7}, //3108 #big5-hkscs
    {0x9BDA, 0x55BC}, //3109 #big5-hkscs
    {0x9BDB, 0x25446}, //3110 #big5-hkscs
    {0x9BDC, 0x2546E}, //3111 #big5-hkscs
    {0x9BDD, 0x26B52}, //3112 #big5-hkscs
    {0x9BDE, 0x91D4}, //3113 #big5-hkscs
    {0x9BDF, 0x3473}, //3114 #big5-hkscs
    {0x9BE0, 0x2553F}, //3115 #big5-hkscs
    {0x9BE1, 0x27632}, //3116 #big5-hkscs
    {0x9BE2, 0x2555E}, //3117 #big5-hkscs
    {0x9BE3, 0x4718}, //3118 #big5-hkscs
    {0x9BE4, 0x25562}, //3119 #big5-hkscs
    {0x9BE5, 0x25566}, //3120 #big5-hkscs
    {0x9BE6, 0x257C7}, //3121 #big5-hkscs
    {0x9BE7, 0x2493F}, //3122 #big5-hkscs
    {0x9BE8, 0x2585D}, //3123 #big5-hkscs
    {0x9BE9, 0x5066}, //3124 #big5-hkscs
    {0x9BEA, 0x34FB}, //3125 #big5-hkscs
    {0x9BEB, 0x233CC}, //3126 #big5-hkscs
    {0x9BEC, 0x60DE}, //3127 #big5-hkscs
    {0x9BED, 0x25903}, //3128 #big5-hkscs
    {0x9BEE, 0x477C}, //3129 #big5-hkscs
    {0x9BEF, 0x28948}, //3130 #big5-hkscs
    {0x9BF0, 0x25AAE}, //3131 #big5-hkscs
    {0x9BF1, 0x25B89}, //3132 #big5-hkscs
    {0x9BF2, 0x25C06}, //3133 #big5-hkscs
    {0x9BF3, 0x21D90}, //3134 #big5-hkscs
    {0x9BF4, 0x57A1}, //3135 #big5-hkscs
    {0x9BF5, 0x7151}, //3136 #big5-hkscs
    {0x9BF6, 0x6FB6}, //3137 #big5-hkscs
    {0x9BF7, 0x26102}, //3138 #big5-hkscs
    {0x9BF8, 0x27C12}, //3139 #big5-hkscs
    {0x9BF9, 0x9056}, //3140 #big5-hkscs
    {0x9BFA, 0x261B2}, //3141 #big5-hkscs
    {0x9BFB, 0x24F9A}, //3142 #big5-hkscs
    {0x9BFC, 0x8B62}, //3143 #big5-hkscs
    {0x9BFD, 0x26402}, //3144 #big5-hkscs
    {0x9BFE, 0x2644A}, //3145 #big5-hkscs
    {0x9C40, 0x5D5B}, //3146 #big5-hkscs
    {0x9C41, 0x26BF7}, //3147 #big5-hkscs
    {0x9C42, 0x8F36}, //3148 #big5-hkscs
    {0x9C43, 0x26484}, //3149 #big5-hkscs
    {0x9C44, 0x2191C}, //3150 #big5-hkscs
    {0x9C45, 0x8AEA}, //3151 #big5-hkscs
    {0x9C46, 0x249F6}, //3152 #big5-hkscs
    {0x9C47, 0x26488}, //3153 #big5-hkscs
    {0x9C48, 0x23FEF}, //3154 #big5-hkscs
    {0x9C49, 0x26512}, //3155 #big5-hkscs
    {0x9C4A, 0x4BC0}, //3156 #big5-hkscs
    {0x9C4B, 0x265BF}, //3157 #big5-hkscs
    {0x9C4C, 0x266B5}, //3158 #big5-hkscs
    {0x9C4D, 0x2271B}, //3159 #big5-hkscs
    {0x9C4E, 0x9465}, //3160 #big5-hkscs
    {0x9C4F, 0x257E1}, //3161 #big5-hkscs
    {0x9C50, 0x6195}, //3162 #big5-hkscs
    {0x9C51, 0x5A27}, //3163 #big5-hkscs
    {0x9C52, 0x2F8CD}, //3164 #big5-hkscs
    {0x9C53, 0x4FBB}, //3165 #big5-hkscs
    {0x9C54, 0x56B9}, //3166 #big5-hkscs
    {0x9C55, 0x24521}, //3167 #big5-hkscs
    {0x9C56, 0x266FC}, //3168 #big5-hkscs
    {0x9C57, 0x4E6A}, //3169 #big5-hkscs
    {0x9C58, 0x24934}, //3170 #big5-hkscs
    {0x9C59, 0x9656}, //3171 #big5-hkscs
    {0x9C5A, 0x6D8F}, //3172 #big5-hkscs
    {0x9C5B, 0x26CBD}, //3173 #big5-hkscs
    {0x9C5C, 0x3618}, //3174 #big5-hkscs
    {0x9C5D, 0x8977}, //3175 #big5-hkscs
    {0x9C5E, 0x26799}, //3176 #big5-hkscs
    {0x9C5F, 0x2686E}, //3177 #big5-hkscs
    {0x9C60, 0x26411}, //3178 #big5-hkscs
    {0x9C61, 0x2685E}, //3179 #big5-hkscs
    {0x9C62, 0x71DF}, //3180 #big5-hkscs
    {0x9C63, 0x268C7}, //3181 #big5-hkscs
    {0x9C64, 0x7B42}, //3182 #big5-hkscs
    {0x9C65, 0x290C0}, //3183 #big5-hkscs
    {0x9C66, 0x20A11}, //3184 #big5-hkscs
    {0x9C67, 0x26926}, //3185 #big5-hkscs
    {0x9C68, 0x9104}, //3186 #big5-hkscs
    {0x9C69, 0x26939}, //3187 #big5-hkscs
    {0x9C6A, 0x7A45}, //3188 #big5-hkscs
    {0x9C6B, 0x9DF0}, //3189 #big5-hkscs
    {0x9C6C, 0x269FA}, //3190 #big5-hkscs
    {0x9C6D, 0x9A26}, //3191 #big5-hkscs
    {0x9C6E, 0x26A2D}, //3192 #big5-hkscs
    {0x9C6F, 0x365F}, //3193 #big5-hkscs
    {0x9C70, 0x26469}, //3194 #big5-hkscs
    {0x9C71, 0x20021}, //3195 #big5-hkscs
    {0x9C72, 0x7983}, //3196 #big5-hkscs
    {0x9C73, 0x26A34}, //3197 #big5-hkscs
    {0x9C74, 0x26B5B}, //3198 #big5-hkscs
    {0x9C75, 0x5D2C}, //3199 #big5-hkscs
    {0x9C76, 0x23519}, //3200 #big5-hkscs
    {0x9C77, 0x83CF}, //3201 #big5-hkscs
    {0x9C78, 0x26B9D}, //3202 #big5-hkscs
    {0x9C79, 0x46D0}, //3203 #big5-hkscs
    {0x9C7A, 0x26CA4}, //3204 #big5-hkscs
    {0x9C7B, 0x753B}, //3205 #big5-hkscs
    {0x9C7C, 0x8865}, //3206 #big5-hkscs
    {0x9C7D, 0x26DAE}, //3207 #big5-hkscs
    {0x9C7E, 0x58B6}, //3208 #big5-hkscs
    {0x9CA1, 0x371C}, //3209 #big5-hkscs
    {0x9CA2, 0x2258D}, //3210 #big5-hkscs
    {0x9CA3, 0x2704B}, //3211 #big5-hkscs
    {0x9CA4, 0x271CD}, //3212 #big5-hkscs
    {0x9CA5, 0x3C54}, //3213 #big5-hkscs
    {0x9CA6, 0x27280}, //3214 #big5-hkscs
    {0x9CA7, 0x27285}, //3215 #big5-hkscs
    {0x9CA8, 0x9281}, //3216 #big5-hkscs
    {0x9CA9, 0x2217A}, //3217 #big5-hkscs
    {0x9CAA, 0x2728B}, //3218 #big5-hkscs
    {0x9CAB, 0x9330}, //3219 #big5-hkscs
    {0x9CAC, 0x272E6}, //3220 #big5-hkscs
    {0x9CAD, 0x249D0}, //3221 #big5-hkscs
    {0x9CAE, 0x6C39}, //3222 #big5-hkscs
    {0x9CAF, 0x949F}, //3223 #big5-hkscs
    {0x9CB0, 0x27450}, //3224 #big5-hkscs
    {0x9CB1, 0x20EF8}, //3225 #big5-hkscs
    {0x9CB2, 0x8827}, //3226 #big5-hkscs
    {0x9CB3, 0x88F5}, //3227 #big5-hkscs
    {0x9CB4, 0x22926}, //3228 #big5-hkscs
    {0x9CB5, 0x28473}, //3229 #big5-hkscs
    {0x9CB6, 0x217B1}, //3230 #big5-hkscs
    {0x9CB7, 0x6EB8}, //3231 #big5-hkscs
    {0x9CB8, 0x24A2A}, //3232 #big5-hkscs
    {0x9CB9, 0x21820}, //3233 #big5-hkscs
    {0x9CBA, 0x39A4}, //3234 #big5-hkscs
    {0x9CBB, 0x36B9}, //3235 #big5-hkscs
    {0x9CBC, 0x5C10}, //3236 #big5-hkscs
    {0x9CBD, 0x79E3}, //3237 #big5-hkscs
    {0x9CBE, 0x453F}, //3238 #big5-hkscs
    {0x9CBF, 0x66B6}, //3239 #big5-hkscs
    {0x9CC0, 0x29CAD}, //3240 #big5-hkscs
    {0x9CC1, 0x298A4}, //3241 #big5-hkscs
    {0x9CC2, 0x8943}, //3242 #big5-hkscs
    {0x9CC3, 0x277CC}, //3243 #big5-hkscs
    {0x9CC4, 0x27858}, //3244 #big5-hkscs
    {0x9CC5, 0x56D6}, //3245 #big5-hkscs
    {0x9CC6, 0x40DF}, //3246 #big5-hkscs
    {0x9CC7, 0x2160A}, //3247 #big5-hkscs
    {0x9CC8, 0x39A1}, //3248 #big5-hkscs
    {0x9CC9, 0x2372F}, //3249 #big5-hkscs
    {0x9CCA, 0x280E8}, //3250 #big5-hkscs
    {0x9CCB, 0x213C5}, //3251 #big5-hkscs
    {0x9CCC, 0x71AD}, //3252 #big5-hkscs
    {0x9CCD, 0x8366}, //3253 #big5-hkscs
    {0x9CCE, 0x279DD}, //3254 #big5-hkscs
    {0x9CCF, 0x291A8}, //3255 #big5-hkscs
    {0x9CD0, 0x5A67}, //3256 #big5-hkscs
    {0x9CD1, 0x4CB7}, //3257 #big5-hkscs
    {0x9CD2, 0x270AF}, //3258 #big5-hkscs
    {0x9CD3, 0x289AB}, //3259 #big5-hkscs
    {0x9CD4, 0x279FD}, //3260 #big5-hkscs
    {0x9CD5, 0x27A0A}, //3261 #big5-hkscs
    {0x9CD6, 0x27B0B}, //3262 #big5-hkscs
    {0x9CD7, 0x27D66}, //3263 #big5-hkscs
    {0x9CD8, 0x2417A}, //3264 #big5-hkscs
    {0x9CD9, 0x7B43}, //3265 #big5-hkscs
    {0x9CDA, 0x797E}, //3266 #big5-hkscs
    {0x9CDB, 0x28009}, //3267 #big5-hkscs
    {0x9CDC, 0x6FB5}, //3268 #big5-hkscs
    {0x9CDD, 0x2A2DF}, //3269 #big5-hkscs
    {0x9CDE, 0x6A03}, //3270 #big5-hkscs
    {0x9CDF, 0x28318}, //3271 #big5-hkscs
    {0x9CE0, 0x53A2}, //3272 #big5-hkscs
    {0x9CE1, 0x26E07}, //3273 #big5-hkscs
    {0x9CE2, 0x93BF}, //3274 #big5-hkscs
    {0x9CE3, 0x6836}, //3275 #big5-hkscs
    {0x9CE4, 0x975D}, //3276 #big5-hkscs
    {0x9CE5, 0x2816F}, //3277 #big5-hkscs
    {0x9CE6, 0x28023}, //3278 #big5-hkscs
    {0x9CE7, 0x269B5}, //3279 #big5-hkscs
    {0x9CE8, 0x213ED}, //3280 #big5-hkscs
    {0x9CE9, 0x2322F}, //3281 #big5-hkscs
    {0x9CEA, 0x28048}, //3282 #big5-hkscs
    {0x9CEB, 0x5D85}, //3283 #big5-hkscs
    {0x9CEC, 0x28C30}, //3284 #big5-hkscs
    {0x9CED, 0x28083}, //3285 #big5-hkscs
    {0x9CEE, 0x5715}, //3286 #big5-hkscs
    {0x9CEF, 0x9823}, //3287 #big5-hkscs
    {0x9CF0, 0x28949}, //3288 #big5-hkscs
    {0x9CF1, 0x5DAB}, //3289 #big5-hkscs
    {0x9CF2, 0x24988}, //3290 #big5-hkscs
    {0x9CF3, 0x65BE}, //3291 #big5-hkscs
    {0x9CF4, 0x69D5}, //3292 #big5-hkscs
    {0x9CF5, 0x53D2}, //3293 #big5-hkscs
    {0x9CF6, 0x24AA5}, //3294 #big5-hkscs
    {0x9CF7, 0x23F81}, //3295 #big5-hkscs
    {0x9CF8, 0x3C11}, //3296 #big5-hkscs
    {0x9CF9, 0x6736}, //3297 #big5-hkscs
    {0x9CFA, 0x28090}, //3298 #big5-hkscs
    {0x9CFB, 0x280F4}, //3299 #big5-hkscs
    {0x9CFC, 0x2812E}, //3300 #big5-hkscs
    {0x9CFD, 0x21FA1}, //3301 #big5-hkscs
    {0x9CFE, 0x2814F}, //3302 #big5-hkscs
    {0x9D40, 0x28189}, //3303 #big5-hkscs
    {0x9D41, 0x281AF}, //3304 #big5-hkscs
    {0x9D42, 0x2821A}, //3305 #big5-hkscs
    {0x9D43, 0x28306}, //3306 #big5-hkscs
    {0x9D44, 0x2832F}, //3307 #big5-hkscs
    {0x9D45, 0x2838A}, //3308 #big5-hkscs
    {0x9D46, 0x35CA}, //3309 #big5-hkscs
    {0x9D47, 0x28468}, //3310 #big5-hkscs
    {0x9D48, 0x286AA}, //3311 #big5-hkscs
    {0x9D49, 0x48FA}, //3312 #big5-hkscs
    {0x9D4A, 0x63E6}, //3313 #big5-hkscs
    {0x9D4B, 0x28956}, //3314 #big5-hkscs
    {0x9D4C, 0x7808}, //3315 #big5-hkscs
    {0x9D4D, 0x9255}, //3316 #big5-hkscs
    {0x9D4E, 0x289B8}, //3317 #big5-hkscs
    {0x9D4F, 0x43F2}, //3318 #big5-hkscs
    {0x9D50, 0x289E7}, //3319 #big5-hkscs
    {0x9D51, 0x43DF}, //3320 #big5-hkscs
    {0x9D52, 0x289E8}, //3321 #big5-hkscs
    {0x9D53, 0x28B46}, //3322 #big5-hkscs
    {0x9D54, 0x28BD4}, //3323 #big5-hkscs
    {0x9D55, 0x59F8}, //3324 #big5-hkscs
    {0x9D56, 0x28C09}, //3325 #big5-hkscs
    {0x9D57, 0x8F0B}, //3326 #big5-hkscs
    {0x9D58, 0x28FC5}, //3327 #big5-hkscs
    {0x9D59, 0x290EC}, //3328 #big5-hkscs
    {0x9D5A, 0x7B51}, //3329 #big5-hkscs
    {0x9D5B, 0x29110}, //3330 #big5-hkscs
    {0x9D5C, 0x2913C}, //3331 #big5-hkscs
    {0x9D5D, 0x3DF7}, //3332 #big5-hkscs
    {0x9D5E, 0x2915E}, //3333 #big5-hkscs
    {0x9D5F, 0x24ACA}, //3334 #big5-hkscs
    {0x9D60, 0x8FD0}, //3335 #big5-hkscs
    {0x9D61, 0x728F}, //3336 #big5-hkscs
    {0x9D62, 0x568B}, //3337 #big5-hkscs
    {0x9D63, 0x294E7}, //3338 #big5-hkscs
    {0x9D64, 0x295E9}, //3339 #big5-hkscs
    {0x9D65, 0x295B0}, //3340 #big5-hkscs
    {0x9D66, 0x295B8}, //3341 #big5-hkscs
    {0x9D67, 0x29732}, //3342 #big5-hkscs
    {0x9D68, 0x298D1}, //3343 #big5-hkscs
    {0x9D69, 0x29949}, //3344 #big5-hkscs
    {0x9D6A, 0x2996A}, //3345 #big5-hkscs
    {0x9D6B, 0x299C3}, //3346 #big5-hkscs
    {0x9D6C, 0x29A28}, //3347 #big5-hkscs
    {0x9D6D, 0x29B0E}, //3348 #big5-hkscs
    {0x9D6E, 0x29D5A}, //3349 #big5-hkscs
    {0x9D6F, 0x29D9B}, //3350 #big5-hkscs
    {0x9D70, 0x7E9F}, //3351 #big5-hkscs
    {0x9D71, 0x29EF8}, //3352 #big5-hkscs
    {0x9D72, 0x29F23}, //3353 #big5-hkscs
    {0x9D73, 0x4CA4}, //3354 #big5-hkscs
    {0x9D74, 0x9547}, //3355 #big5-hkscs
    {0x9D75, 0x2A293}, //3356 #big5-hkscs
    {0x9D76, 0x71A2}, //3357 #big5-hkscs
    {0x9D77, 0x2A2FF}, //3358 #big5-hkscs
    {0x9D78, 0x4D91}, //3359 #big5-hkscs
    {0x9D79, 0x9012}, //3360 #big5-hkscs
    {0x9D7A, 0x2A5CB}, //3361 #big5-hkscs
    {0x9D7B, 0x4D9C}, //3362 #big5-hkscs
    {0x9D7C, 0x20C9C}, //3363 #big5-hkscs
    {0x9D7D, 0x8FBE}, //3364 #big5-hkscs
    {0x9D7E, 0x55C1}, //3365 #big5-hkscs
    {0x9DA1, 0x8FBA}, //3366 #big5-hkscs
    {0x9DA2, 0x224B0}, //3367 #big5-hkscs
    {0x9DA3, 0x8FB9}, //3368 #big5-hkscs
    {0x9DA4, 0x24A93}, //3369 #big5-hkscs
    {0x9DA5, 0x4509}, //3370 #big5-hkscs
    {0x9DA6, 0x7E7F}, //3371 #big5-hkscs
    {0x9DA7, 0x6F56}, //3372 #big5-hkscs
    {0x9DA8, 0x6AB1}, //3373 #big5-hkscs
    {0x9DA9, 0x4EEA}, //3374 #big5-hkscs
    {0x9DAA, 0x34E4}, //3375 #big5-hkscs
    {0x9DAB, 0x28B2C}, //3376 #big5-hkscs
    {0x9DAC, 0x2789D}, //3377 #big5-hkscs
    {0x9DAD, 0x373A}, //3378 #big5-hkscs
    {0x9DAE, 0x8E80}, //3379 #big5-hkscs
    {0x9DAF, 0x217F5}, //3380 #big5-hkscs
    {0x9DB0, 0x28024}, //3381 #big5-hkscs
    {0x9DB1, 0x28B6C}, //3382 #big5-hkscs
    {0x9DB2, 0x28B99}, //3383 #big5-hkscs
    {0x9DB3, 0x27A3E}, //3384 #big5-hkscs
    {0x9DB4, 0x266AF}, //3385 #big5-hkscs
    {0x9DB5, 0x3DEB}, //3386 #big5-hkscs
    {0x9DB6, 0x27655}, //3387 #big5-hkscs
    {0x9DB7, 0x23CB7}, //3388 #big5-hkscs
    {0x9DB8, 0x25635}, //3389 #big5-hkscs
    {0x9DB9, 0x25956}, //3390 #big5-hkscs
    {0x9DBA, 0x4E9A}, //3391 #big5-hkscs
    {0x9DBB, 0x25E81}, //3392 #big5-hkscs
    {0x9DBC, 0x26258}, //3393 #big5-hkscs
    {0x9DBD, 0x56BF}, //3394 #big5-hkscs
    {0x9DBE, 0x20E6D}, //3395 #big5-hkscs
    {0x9DBF, 0x8E0E}, //3396 #big5-hkscs
    {0x9DC0, 0x5B6D}, //3397 #big5-hkscs
    {0x9DC1, 0x23E88}, //3398 #big5-hkscs
    {0x9DC2, 0x24C9E}, //3399 #big5-hkscs
    {0x9DC3, 0x63DE}, //3400 #big5-hkscs
    {0x9DC4, 0x62D0}, //3401 #big5-hkscs
    {0x9DC5, 0x217F6}, //3402 #big5-hkscs
    {0x9DC6, 0x2187B}, //3403 #big5-hkscs
    {0x9DC7, 0x6530}, //3404 #big5-hkscs
    {0x9DC8, 0x562D}, //3405 #big5-hkscs
    {0x9DC9, 0x25C4A}, //3406 #big5-hkscs
    {0x9DCA, 0x541A}, //3407 #big5-hkscs
    {0x9DCB, 0x25311}, //3408 #big5-hkscs
    {0x9DCC, 0x3DC6}, //3409 #big5-hkscs
    {0x9DCD, 0x29D98}, //3410 #big5-hkscs
    {0x9DCE, 0x4C7D}, //3411 #big5-hkscs
    {0x9DCF, 0x5622}, //3412 #big5-hkscs
    {0x9DD0, 0x561E}, //3413 #big5-hkscs
    {0x9DD1, 0x7F49}, //3414 #big5-hkscs
    {0x9DD2, 0x25ED8}, //3415 #big5-hkscs
    {0x9DD3, 0x5975}, //3416 #big5-hkscs
    {0x9DD4, 0x23D40}, //3417 #big5-hkscs
    {0x9DD5, 0x8770}, //3418 #big5-hkscs
    {0x9DD6, 0x4E1C}, //3419 #big5-hkscs
    {0x9DD7, 0x20FEA}, //3420 #big5-hkscs
    {0x9DD8, 0x20D49}, //3421 #big5-hkscs
    {0x9DD9, 0x236BA}, //3422 #big5-hkscs
    {0x9DDA, 0x8117}, //3423 #big5-hkscs
    {0x9DDB, 0x9D5E}, //3424 #big5-hkscs
    {0x9DDC, 0x8D18}, //3425 #big5-hkscs
    {0x9DDD, 0x763B}, //3426 #big5-hkscs
    {0x9DDE, 0x9C45}, //3427 #big5-hkscs
    {0x9DDF, 0x764E}, //3428 #big5-hkscs
    {0x9DE0, 0x77B9}, //3429 #big5-hkscs
    {0x9DE1, 0x9345}, //3430 #big5-hkscs
    {0x9DE2, 0x5432}, //3431 #big5-hkscs
    {0x9DE3, 0x8148}, //3432 #big5-hkscs
    {0x9DE4, 0x82F7}, //3433 #big5-hkscs
    {0x9DE5, 0x5625}, //3434 #big5-hkscs
    {0x9DE6, 0x8132}, //3435 #big5-hkscs
    {0x9DE7, 0x8418}, //3436 #big5-hkscs
    {0x9DE8, 0x80BD}, //3437 #big5-hkscs
    {0x9DE9, 0x55EA}, //3438 #big5-hkscs
    {0x9DEA, 0x7962}, //3439 #big5-hkscs
    {0x9DEB, 0x5643}, //3440 #big5-hkscs
    {0x9DEC, 0x5416}, //3441 #big5-hkscs
    {0x9DED, 0x20E9D}, //3442 #big5-hkscs
    {0x9DEE, 0x35CE}, //3443 #big5-hkscs
    {0x9DEF, 0x5605}, //3444 #big5-hkscs
    {0x9DF0, 0x55F1}, //3445 #big5-hkscs
    {0x9DF1, 0x66F1}, //3446 #big5-hkscs
    {0x9DF2, 0x282E2}, //3447 #big5-hkscs
    {0x9DF3, 0x362D}, //3448 #big5-hkscs
    {0x9DF4, 0x7534}, //3449 #big5-hkscs
    {0x9DF5, 0x55F0}, //3450 #big5-hkscs
    {0x9DF6, 0x55BA}, //3451 #big5-hkscs
    {0x9DF7, 0x5497}, //3452 #big5-hkscs
    {0x9DF8, 0x5572}, //3453 #big5-hkscs
    {0x9DF9, 0x20C41}, //3454 #big5-hkscs
    {0x9DFA, 0x20C96}, //3455 #big5-hkscs
    {0x9DFB, 0x5ED0}, //3456 #big5-hkscs
    {0x9DFC, 0x25148}, //3457 #big5-hkscs
    {0x9DFD, 0x20E76}, //3458 #big5-hkscs
    {0x9DFE, 0x22C62}, //3459 #big5-hkscs
    {0x9E40, 0x20EA2}, //3460 #big5-hkscs
    {0x9E41, 0x9EAB}, //3461 #big5-hkscs
    {0x9E42, 0x7D5A}, //3462 #big5-hkscs
    {0x9E43, 0x55DE}, //3463 #big5-hkscs
    {0x9E44, 0x21075}, //3464 #big5-hkscs
    {0x9E45, 0x629D}, //3465 #big5-hkscs
    {0x9E46, 0x976D}, //3466 #big5-hkscs
    {0x9E47, 0x5494}, //3467 #big5-hkscs
    {0x9E48, 0x8CCD}, //3468 #big5-hkscs
    {0x9E49, 0x71F6}, //3469 #big5-hkscs
    {0x9E4A, 0x9176}, //3470 #big5-hkscs
    {0x9E4B, 0x63FC}, //3471 #big5-hkscs
    {0x9E4C, 0x63B9}, //3472 #big5-hkscs
    {0x9E4D, 0x63FE}, //3473 #big5-hkscs
    {0x9E4E, 0x5569}, //3474 #big5-hkscs
    {0x9E4F, 0x22B43}, //3475 #big5-hkscs
    {0x9E50, 0x9C72}, //3476 #big5-hkscs
    {0x9E51, 0x22EB3}, //3477 #big5-hkscs
    {0x9E52, 0x519A}, //3478 #big5-hkscs
    {0x9E53, 0x34DF}, //3479 #big5-hkscs
    {0x9E54, 0x20DA7}, //3480 #big5-hkscs
    {0x9E55, 0x51A7}, //3481 #big5-hkscs
    {0x9E56, 0x544D}, //3482 #big5-hkscs
    {0x9E57, 0x551E}, //3483 #big5-hkscs
    {0x9E58, 0x5513}, //3484 #big5-hkscs
    {0x9E59, 0x7666}, //3485 #big5-hkscs
    {0x9E5A, 0x8E2D}, //3486 #big5-hkscs
    {0x9E5B, 0x2688A}, //3487 #big5-hkscs
    {0x9E5C, 0x75B1}, //3488 #big5-hkscs
    {0x9E5D, 0x80B6}, //3489 #big5-hkscs
    {0x9E5E, 0x8804}, //3490 #big5-hkscs
    {0x9E5F, 0x8786}, //3491 #big5-hkscs
    {0x9E60, 0x88C7}, //3492 #big5-hkscs
    {0x9E61, 0x81B6}, //3493 #big5-hkscs
    {0x9E62, 0x841C}, //3494 #big5-hkscs
    {0x9E63, 0x210C1}, //3495 #big5-hkscs
    {0x9E64, 0x44EC}, //3496 #big5-hkscs
    {0x9E65, 0x7304}, //3497 #big5-hkscs
    {0x9E66, 0x24706}, //3498 #big5-hkscs
    {0x9E67, 0x5B90}, //3499 #big5-hkscs
    {0x9E68, 0x830B}, //3500 #big5-hkscs
    {0x9E69, 0x26893}, //3501 #big5-hkscs
    {0x9E6A, 0x567B}, //3502 #big5-hkscs
    {0x9E6B, 0x226F4}, //3503 #big5-hkscs
    {0x9E6C, 0x27D2F}, //3504 #big5-hkscs
    {0x9E6D, 0x241A3}, //3505 #big5-hkscs
    {0x9E6E, 0x27D73}, //3506 #big5-hkscs
    {0x9E6F, 0x26ED0}, //3507 #big5-hkscs
    {0x9E70, 0x272B6}, //3508 #big5-hkscs
    {0x9E71, 0x9170}, //3509 #big5-hkscs
    {0x9E72, 0x211D9}, //3510 #big5-hkscs
    {0x9E73, 0x9208}, //3511 #big5-hkscs
    {0x9E74, 0x23CFC}, //3512 #big5-hkscs
    {0x9E75, 0x2A6A9}, //3513 #big5-hkscs
    {0x9E76, 0x20EAC}, //3514 #big5-hkscs
    {0x9E77, 0x20EF9}, //3515 #big5-hkscs
    {0x9E78, 0x7266}, //3516 #big5-hkscs
    {0x9E79, 0x21CA2}, //3517 #big5-hkscs
    {0x9E7A, 0x474E}, //3518 #big5-hkscs
    {0x9E7B, 0x24FC2}, //3519 #big5-hkscs
    {0x9E7C, 0x27FF9}, //3520 #big5-hkscs
    {0x9E7D, 0x20FEB}, //3521 #big5-hkscs
    {0x9E7E, 0x40FA}, //3522 #big5-hkscs
    {0x9EA1, 0x9C5D}, //3523 #big5-hkscs
    {0x9EA2, 0x651F}, //3524 #big5-hkscs
    {0x9EA3, 0x22DA0}, //3525 #big5-hkscs
    {0x9EA4, 0x48F3}, //3526 #big5-hkscs
    {0x9EA5, 0x247E0}, //3527 #big5-hkscs
    {0x9EA6, 0x29D7C}, //3528 #big5-hkscs
    {0x9EA7, 0x20FEC}, //3529 #big5-hkscs
    {0x9EA8, 0x20E0A}, //3530 #big5-hkscs
    {0x9EA9, 0x6062}, //3531 #big5-hkscs
    {0x9EAA, 0x275A3}, //3532 #big5-hkscs
    {0x9EAB, 0x20FED}, //3533 #big5-hkscs
    {0x9EAD, 0x26048}, //3534 #big5-hkscs
    {0x9EAE, 0x21187}, //3535 #big5-hkscs
    {0x9EAF, 0x71A3}, //3536 #big5-hkscs
    {0x9EB0, 0x7E8E}, //3537 #big5-hkscs
    {0x9EB1, 0x9D50}, //3538 #big5-hkscs
    {0x9EB2, 0x4E1A}, //3539 #big5-hkscs
    {0x9EB3, 0x4E04}, //3540 #big5-hkscs
    {0x9EB4, 0x3577}, //3541 #big5-hkscs
    {0x9EB5, 0x5B0D}, //3542 #big5-hkscs
    {0x9EB6, 0x6CB2}, //3543 #big5-hkscs
    {0x9EB7, 0x5367}, //3544 #big5-hkscs
    {0x9EB8, 0x36AC}, //3545 #big5-hkscs
    {0x9EB9, 0x39DC}, //3546 #big5-hkscs
    {0x9EBA, 0x537D}, //3547 #big5-hkscs
    {0x9EBB, 0x36A5}, //3548 #big5-hkscs
    {0x9EBC, 0x24618}, //3549 #big5-hkscs
    {0x9EBD, 0x589A}, //3550 #big5-hkscs
    {0x9EBE, 0x24B6E}, //3551 #big5-hkscs
    {0x9EBF, 0x822D}, //3552 #big5-hkscs
    {0x9EC0, 0x544B}, //3553 #big5-hkscs
    {0x9EC1, 0x57AA}, //3554 #big5-hkscs
    {0x9EC2, 0x25A95}, //3555 #big5-hkscs
    {0x9EC3, 0x20979}, //3556 #big5-hkscs
    {0x9EC5, 0x3A52}, //3557 #big5-hkscs
    {0x9EC6, 0x22465}, //3558 #big5-hkscs
    {0x9EC7, 0x7374}, //3559 #big5-hkscs
    {0x9EC8, 0x29EAC}, //3560 #big5-hkscs
    {0x9EC9, 0x4D09}, //3561 #big5-hkscs
    {0x9ECA, 0x9BED}, //3562 #big5-hkscs
    {0x9ECB, 0x23CFE}, //3563 #big5-hkscs
    {0x9ECC, 0x29F30}, //3564 #big5-hkscs
    {0x9ECD, 0x4C5B}, //3565 #big5-hkscs
    {0x9ECE, 0x24FA9}, //3566 #big5-hkscs
    {0x9ECF, 0x2959E}, //3567 #big5-hkscs
    {0x9ED0, 0x29FDE}, //3568 #big5-hkscs
    {0x9ED1, 0x845C}, //3569 #big5-hkscs
    {0x9ED2, 0x23DB6}, //3570 #big5-hkscs
    {0x9ED3, 0x272B2}, //3571 #big5-hkscs
    {0x9ED4, 0x267B3}, //3572 #big5-hkscs
    {0x9ED5, 0x23720}, //3573 #big5-hkscs
    {0x9ED6, 0x632E}, //3574 #big5-hkscs
    {0x9ED7, 0x7D25}, //3575 #big5-hkscs
    {0x9ED8, 0x23EF7}, //3576 #big5-hkscs
    {0x9ED9, 0x23E2C}, //3577 #big5-hkscs
    {0x9EDA, 0x3A2A}, //3578 #big5-hkscs
    {0x9EDB, 0x9008}, //3579 #big5-hkscs
    {0x9EDC, 0x52CC}, //3580 #big5-hkscs
    {0x9EDD, 0x3E74}, //3581 #big5-hkscs
    {0x9EDE, 0x367A}, //3582 #big5-hkscs
    {0x9EDF, 0x45E9}, //3583 #big5-hkscs
    {0x9EE0, 0x2048E}, //3584 #big5-hkscs
    {0x9EE1, 0x7640}, //3585 #big5-hkscs
    {0x9EE2, 0x5AF0}, //3586 #big5-hkscs
    {0x9EE3, 0x20EB6}, //3587 #big5-hkscs
    {0x9EE4, 0x787A}, //3588 #big5-hkscs
    {0x9EE5, 0x27F2E}, //3589 #big5-hkscs
    {0x9EE6, 0x58A7}, //3590 #big5-hkscs
    {0x9EE7, 0x40BF}, //3591 #big5-hkscs
    {0x9EE8, 0x567C}, //3592 #big5-hkscs
    {0x9EE9, 0x9B8B}, //3593 #big5-hkscs
    {0x9EEA, 0x5D74}, //3594 #big5-hkscs
    {0x9EEB, 0x7654}, //3595 #big5-hkscs
    {0x9EEC, 0x2A434}, //3596 #big5-hkscs
    {0x9EED, 0x9E85}, //3597 #big5-hkscs
    {0x9EEE, 0x4CE1}, //3598 #big5-hkscs
    {0x9EEF, 0x75F9}, //3599 #big5-hkscs
    {0x9EF0, 0x37FB}, //3600 #big5-hkscs
    {0x9EF1, 0x6119}, //3601 #big5-hkscs
    {0x9EF2, 0x230DA}, //3602 #big5-hkscs
    {0x9EF3, 0x243F2}, //3603 #big5-hkscs
    {0x9EF5, 0x565D}, //3604 #big5-hkscs
    {0x9EF6, 0x212A9}, //3605 #big5-hkscs
    {0x9EF7, 0x57A7}, //3606 #big5-hkscs
    {0x9EF8, 0x24963}, //3607 #big5-hkscs
    {0x9EF9, 0x29E06}, //3608 #big5-hkscs
    {0x9EFA, 0x5234}, //3609 #big5-hkscs
    {0x9EFB, 0x270AE}, //3610 #big5-hkscs
    {0x9EFC, 0x35AD}, //3611 #big5-hkscs
    {0x9EFD, 0x6C4A}, //3612 #big5-hkscs
    {0x9EFE, 0x9D7C}, //3613 #big5-hkscs
    {0x9F40, 0x7C56}, //3614 #big5-hkscs
    {0x9F41, 0x9B39}, //3615 #big5-hkscs
    {0x9F42, 0x57DE}, //3616 #big5-hkscs
    {0x9F43, 0x2176C}, //3617 #big5-hkscs
    {0x9F44, 0x5C53}, //3618 #big5-hkscs
    {0x9F45, 0x64D3}, //3619 #big5-hkscs
    {0x9F46, 0x294D0}, //3620 #big5-hkscs
    {0x9F47, 0x26335}, //3621 #big5-hkscs
    {0x9F48, 0x27164}, //3622 #big5-hkscs
    {0x9F49, 0x86AD}, //3623 #big5-hkscs
    {0x9F4A, 0x20D28}, //3624 #big5-hkscs
    {0x9F4B, 0x26D22}, //3625 #big5-hkscs
    {0x9F4C, 0x24AE2}, //3626 #big5-hkscs
    {0x9F4D, 0x20D71}, //3627 #big5-hkscs
    {0x9F4F, 0x51FE}, //3628 #big5-hkscs
    {0x9F50, 0x21F0F}, //3629 #big5-hkscs
    {0x9F51, 0x5D8E}, //3630 #big5-hkscs
    {0x9F52, 0x9703}, //3631 #big5-hkscs
    {0x9F53, 0x21DD1}, //3632 #big5-hkscs
    {0x9F54, 0x9E81}, //3633 #big5-hkscs
    {0x9F55, 0x904C}, //3634 #big5-hkscs
    {0x9F56, 0x7B1F}, //3635 #big5-hkscs
    {0x9F57, 0x9B02}, //3636 #big5-hkscs
    {0x9F58, 0x5CD1}, //3637 #big5-hkscs
    {0x9F59, 0x7BA3}, //3638 #big5-hkscs
    {0x9F5A, 0x6268}, //3639 #big5-hkscs
    {0x9F5B, 0x6335}, //3640 #big5-hkscs
    {0x9F5C, 0x9AFF}, //3641 #big5-hkscs
    {0x9F5D, 0x7BCF}, //3642 #big5-hkscs
    {0x9F5E, 0x9B2A}, //3643 #big5-hkscs
    {0x9F5F, 0x7C7E}, //3644 #big5-hkscs
    {0x9F60, 0x9B2E}, //3645 #big5-hkscs
    {0x9F61, 0x7C42}, //3646 #big5-hkscs
    {0x9F62, 0x7C86}, //3647 #big5-hkscs
    {0x9F63, 0x9C15}, //3648 #big5-hkscs
    {0x9F64, 0x7BFC}, //3649 #big5-hkscs
    {0x9F65, 0x9B09}, //3650 #big5-hkscs
    {0x9F66, 0x9F17}, //3651 #big5-hkscs
    {0x9F67, 0x9C1B}, //3652 #big5-hkscs
    {0x9F68, 0x2493E}, //3653 #big5-hkscs
    {0x9F69, 0x9F5A}, //3654 #big5-hkscs
    {0x9F6A, 0x5573}, //3655 #big5-hkscs
    {0x9F6B, 0x5BC3}, //3656 #big5-hkscs
    {0x9F6C, 0x4FFD}, //3657 #big5-hkscs
    {0x9F6D, 0x9E98}, //3658 #big5-hkscs
    {0x9F6E, 0x4FF2}, //3659 #big5-hkscs
    {0x9F6F, 0x5260}, //3660 #big5-hkscs
    {0x9F70, 0x3E06}, //3661 #big5-hkscs
    {0x9F71, 0x52D1}, //3662 #big5-hkscs
    {0x9F72, 0x5767}, //3663 #big5-hkscs
    {0x9F73, 0x5056}, //3664 #big5-hkscs
    {0x9F74, 0x59B7}, //3665 #big5-hkscs
    {0x9F75, 0x5E12}, //3666 #big5-hkscs
    {0x9F76, 0x97C8}, //3667 #big5-hkscs
    {0x9F77, 0x9DAB}, //3668 #big5-hkscs
    {0x9F78, 0x8F5C}, //3669 #big5-hkscs
    {0x9F79, 0x5469}, //3670 #big5-hkscs
    {0x9F7A, 0x97B4}, //3671 #big5-hkscs
    {0x9F7B, 0x9940}, //3672 #big5-hkscs
    {0x9F7C, 0x97BA}, //3673 #big5-hkscs
    {0x9F7D, 0x532C}, //3674 #big5-hkscs
    {0x9F7E, 0x6130}, //3675 #big5-hkscs
    {0x9FA1, 0x692C}, //3676 #big5-hkscs
    {0x9FA2, 0x53DA}, //3677 #big5-hkscs
    {0x9FA3, 0x9C0A}, //3678 #big5-hkscs
    {0x9FA4, 0x9D02}, //3679 #big5-hkscs
    {0x9FA5, 0x4C3B}, //3680 #big5-hkscs
    {0x9FA6, 0x9641}, //3681 #big5-hkscs
    {0x9FA7, 0x6980}, //3682 #big5-hkscs
    {0x9FA8, 0x50A6}, //3683 #big5-hkscs
    {0x9FA9, 0x7546}, //3684 #big5-hkscs
    {0x9FAA, 0x2176D}, //3685 #big5-hkscs
    {0x9FAB, 0x99DA}, //3686 #big5-hkscs
    {0x9FAC, 0x5273}, //3687 #big5-hkscs
    {0x9FAE, 0x9159}, //3688 #big5-hkscs
    {0x9FAF, 0x9681}, //3689 #big5-hkscs
    {0x9FB0, 0x915C}, //3690 #big5-hkscs
    {0x9FB2, 0x9151}, //3691 #big5-hkscs
    {0x9FB3, 0x28E97}, //3692 #big5-hkscs
    {0x9FB4, 0x637F}, //3693 #big5-hkscs
    {0x9FB5, 0x26D23}, //3694 #big5-hkscs
    {0x9FB6, 0x6ACA}, //3695 #big5-hkscs
    {0x9FB7, 0x5611}, //3696 #big5-hkscs
    {0x9FB8, 0x918E}, //3697 #big5-hkscs
    {0x9FB9, 0x757A}, //3698 #big5-hkscs
    {0x9FBA, 0x6285}, //3699 #big5-hkscs
    {0x9FBB, 0x203FC}, //3700 #big5-hkscs
    {0x9FBC, 0x734F}, //3701 #big5-hkscs
    {0x9FBD, 0x7C70}, //3702 #big5-hkscs
    {0x9FBE, 0x25C21}, //3703 #big5-hkscs
    {0x9FBF, 0x23CFD}, //3704 #big5-hkscs
    {0x9FC1, 0x24919}, //3705 #big5-hkscs
    {0x9FC2, 0x76D6}, //3706 #big5-hkscs
    {0x9FC3, 0x9B9D}, //3707 #big5-hkscs
    {0x9FC4, 0x4E2A}, //3708 #big5-hkscs
    {0x9FC5, 0x20CD4}, //3709 #big5-hkscs
    {0x9FC6, 0x83BE}, //3710 #big5-hkscs
    {0x9FC7, 0x8842}, //3711 #big5-hkscs
    {0x9FC9, 0x5C4A}, //3712 #big5-hkscs
    {0x9FCA, 0x69C0}, //3713 #big5-hkscs
    {0x9FCB, 0x50ED}, //3714 #big5-hkscs
    {0x9FCC, 0x577A}, //3715 #big5-hkscs
    {0x9FCD, 0x521F}, //3716 #big5-hkscs
    {0x9FCE, 0x5DF5}, //3717 #big5-hkscs
    {0x9FCF, 0x4ECE}, //3718 #big5-hkscs
    {0x9FD0, 0x6C31}, //3719 #big5-hkscs
    {0x9FD1, 0x201F2}, //3720 #big5-hkscs
    {0x9FD2, 0x4F39}, //3721 #big5-hkscs
    {0x9FD3, 0x549C}, //3722 #big5-hkscs
    {0x9FD4, 0x54DA}, //3723 #big5-hkscs
    {0x9FD5, 0x529A}, //3724 #big5-hkscs
    {0x9FD6, 0x8D82}, //3725 #big5-hkscs
    {0x9FD7, 0x35FE}, //3726 #big5-hkscs
    {0x9FD8, 0x5F0C}, //3727 #big5-hkscs
    {0x9FD9, 0x35F3}, //3728 #big5-hkscs
    {0x9FDB, 0x6B52}, //3729 #big5-hkscs
    {0x9FDC, 0x917C}, //3730 #big5-hkscs
    {0x9FDD, 0x9FA5}, //3731 #big5-hkscs
    {0x9FDE, 0x9B97}, //3732 #big5-hkscs
    {0x9FDF, 0x982E}, //3733 #big5-hkscs
    {0x9FE0, 0x98B4}, //3734 #big5-hkscs
    {0x9FE1, 0x9ABA}, //3735 #big5-hkscs
    {0x9FE2, 0x9EA8}, //3736 #big5-hkscs
    {0x9FE3, 0x9E84}, //3737 #big5-hkscs
    {0x9FE4, 0x717A}, //3738 #big5-hkscs
    {0x9FE5, 0x7B14}, //3739 #big5-hkscs
    {0x9FE7, 0x6BFA}, //3740 #big5-hkscs
    {0x9FE8, 0x8818}, //3741 #big5-hkscs
    {0x9FE9, 0x7F78}, //3742 #big5-hkscs
    {0x9FEB, 0x5620}, //3743 #big5-hkscs
    {0x9FEC, 0x2A64A}, //3744 #big5-hkscs
    {0x9FED, 0x8E77}, //3745 #big5-hkscs
    {0x9FEE, 0x9F53}, //3746 #big5-hkscs
    {0x9FF0, 0x8DD4}, //3747 #big5-hkscs
    {0x9FF1, 0x8E4F}, //3748 #big5-hkscs
    {0x9FF2, 0x9E1C}, //3749 #big5-hkscs
    {0x9FF3, 0x8E01}, //3750 #big5-hkscs
    {0x9FF4, 0x6282}, //3751 #big5-hkscs
    {0x9FF5, 0x2837D}, //3752 #big5-hkscs
    {0x9FF6, 0x8E28}, //3753 #big5-hkscs
    {0x9FF7, 0x8E75}, //3754 #big5-hkscs
    {0x9FF8, 0x7AD3}, //3755 #big5-hkscs
    {0x9FF9, 0x24A77}, //3756 #big5-hkscs
    {0x9FFA, 0x7A3E}, //3757 #big5-hkscs
    {0x9FFB, 0x78D8}, //3758 #big5-hkscs
    {0x9FFC, 0x6CEA}, //3759 #big5-hkscs
    {0x9FFD, 0x8A67}, //3760 #big5-hkscs
    {0x9FFE, 0x7607}, //3761 #big5-hkscs
    {0xA040, 0x28A5A}, //3762 #big5-hkscs
    {0xA041, 0x9F26}, //3763 #big5-hkscs
    {0xA042, 0x6CCE}, //3764 #big5-hkscs
    {0xA043, 0x87D6}, //3765 #big5-hkscs
    {0xA044, 0x75C3}, //3766 #big5-hkscs
    {0xA045, 0x2A2B2}, //3767 #big5-hkscs
    {0xA046, 0x7853}, //3768 #big5-hkscs
    {0xA047, 0x2F840}, //3769 #big5-hkscs
    {0xA048, 0x8D0C}, //3770 #big5-hkscs
    {0xA049, 0x72E2}, //3771 #big5-hkscs
    {0xA04A, 0x7371}, //3772 #big5-hkscs
    {0xA04B, 0x8B2D}, //3773 #big5-hkscs
    {0xA04C, 0x7302}, //3774 #big5-hkscs
    {0xA04D, 0x74F1}, //3775 #big5-hkscs
    {0xA04E, 0x8CEB}, //3776 #big5-hkscs
    {0xA04F, 0x24ABB}, //3777 #big5-hkscs
    {0xA050, 0x862F}, //3778 #big5-hkscs
    {0xA051, 0x5FBA}, //3779 #big5-hkscs
    {0xA052, 0x88A0}, //3780 #big5-hkscs
    {0xA053, 0x44B7}, //3781 #big5-hkscs
    {0xA055, 0x2183B}, //3782 #big5-hkscs
    {0xA056, 0x26E05}, //3783 #big5-hkscs
    {0xA058, 0x8A7E}, //3784 #big5-hkscs
    {0xA059, 0x2251B}, //3785 #big5-hkscs
    {0xA05B, 0x60FD}, //3786 #big5-hkscs
    {0xA05C, 0x7667}, //3787 #big5-hkscs
    {0xA05D, 0x9AD7}, //3788 #big5-hkscs
    {0xA05E, 0x9D44}, //3789 #big5-hkscs
    {0xA05F, 0x936E}, //3790 #big5-hkscs
    {0xA060, 0x9B8F}, //3791 #big5-hkscs
    {0xA061, 0x87F5}, //3792 #big5-hkscs
    {0xA063, 0x880F}, //3793 #big5-hkscs
    {0xA064, 0x8CF7}, //3794 #big5-hkscs
    {0xA065, 0x732C}, //3795 #big5-hkscs
    {0xA066, 0x9721}, //3796 #big5-hkscs
    {0xA067, 0x9BB0}, //3797 #big5-hkscs
    {0xA068, 0x35D6}, //3798 #big5-hkscs
    {0xA069, 0x72B2}, //3799 #big5-hkscs
    {0xA06A, 0x4C07}, //3800 #big5-hkscs
    {0xA06B, 0x7C51}, //3801 #big5-hkscs
    {0xA06C, 0x994A}, //3802 #big5-hkscs
    {0xA06D, 0x26159}, //3803 #big5-hkscs
    {0xA06E, 0x6159}, //3804 #big5-hkscs
    {0xA06F, 0x4C04}, //3805 #big5-hkscs
    {0xA070, 0x9E96}, //3806 #big5-hkscs
    {0xA071, 0x617D}, //3807 #big5-hkscs
    {0xA073, 0x575F}, //3808 #big5-hkscs
    {0xA074, 0x616F}, //3809 #big5-hkscs
    {0xA075, 0x62A6}, //3810 #big5-hkscs
    {0xA076, 0x6239}, //3811 #big5-hkscs
    {0xA077, 0x62CE}, //3812 #big5-hkscs
    {0xA078, 0x3A5C}, //3813 #big5-hkscs
    {0xA079, 0x61E2}, //3814 #big5-hkscs
    {0xA07A, 0x53AA}, //3815 #big5-hkscs
    {0xA07B, 0x233F5}, //3816 #big5-hkscs
    {0xA07C, 0x6364}, //3817 #big5-hkscs
    {0xA07D, 0x6802}, //3818 #big5-hkscs
    {0xA07E, 0x35D2}, //3819 #big5-hkscs
    {0xA0A1, 0x5D57}, //3820 #big5-hkscs
    {0xA0A2, 0x28BC2}, //3821 #big5-hkscs
    {0xA0A3, 0x8FDA}, //3822 #big5-hkscs
    {0xA0A4, 0x28E39}, //3823 #big5-hkscs
    {0xA0A6, 0x50D9}, //3824 #big5-hkscs
    {0xA0A7, 0x21D46}, //3825 #big5-hkscs
    {0xA0A8, 0x7906}, //3826 #big5-hkscs
    {0xA0A9, 0x5332}, //3827 #big5-hkscs
    {0xA0AA, 0x9638}, //3828 #big5-hkscs
    {0xA0AB, 0x20F3B}, //3829 #big5-hkscs
    {0xA0AC, 0x4065}, //3830 #big5-hkscs
    {0xA0AE, 0x77FE}, //3831 #big5-hkscs
    {0xA0B0, 0x7CC2}, //3832 #big5-hkscs
    {0xA0B1, 0x25F1A}, //3833 #big5-hkscs
    {0xA0B2, 0x7CDA}, //3834 #big5-hkscs
    {0xA0B3, 0x7A2D}, //3835 #big5-hkscs
    {0xA0B4, 0x8066}, //3836 #big5-hkscs
    {0xA0B5, 0x8063}, //3837 #big5-hkscs
    {0xA0B6, 0x7D4D}, //3838 #big5-hkscs
    {0xA0B7, 0x7505}, //3839 #big5-hkscs
    {0xA0B8, 0x74F2}, //3840 #big5-hkscs
    {0xA0B9, 0x8994}, //3841 #big5-hkscs
    {0xA0BA, 0x821A}, //3842 #big5-hkscs
    {0xA0BB, 0x670C}, //3843 #big5-hkscs
    {0xA0BC, 0x8062}, //3844 #big5-hkscs
    {0xA0BD, 0x27486}, //3845 #big5-hkscs
    {0xA0BE, 0x805B}, //3846 #big5-hkscs
    {0xA0BF, 0x74F0}, //3847 #big5-hkscs
    {0xA0C0, 0x8103}, //3848 #big5-hkscs
    {0xA0C1, 0x7724}, //3849 #big5-hkscs
    {0xA0C2, 0x8989}, //3850 #big5-hkscs
    {0xA0C3, 0x267CC}, //3851 #big5-hkscs
    {0xA0C4, 0x7553}, //3852 #big5-hkscs
    {0xA0C5, 0x26ED1}, //3853 #big5-hkscs
    {0xA0C6, 0x87A9}, //3854 #big5-hkscs
    {0xA0C7, 0x87CE}, //3855 #big5-hkscs
    {0xA0C8, 0x81C8}, //3856 #big5-hkscs
    {0xA0C9, 0x878C}, //3857 #big5-hkscs
    {0xA0CA, 0x8A49}, //3858 #big5-hkscs
    {0xA0CB, 0x8CAD}, //3859 #big5-hkscs
    {0xA0CC, 0x8B43}, //3860 #big5-hkscs
    {0xA0CD, 0x772B}, //3861 #big5-hkscs
    {0xA0CE, 0x74F8}, //3862 #big5-hkscs
    {0xA0CF, 0x84DA}, //3863 #big5-hkscs
    {0xA0D0, 0x3635}, //3864 #big5-hkscs
    {0xA0D1, 0x69B2}, //3865 #big5-hkscs
    {0xA0D2, 0x8DA6}, //3866 #big5-hkscs
    {0xA0D4, 0x89A9}, //3867 #big5-hkscs
    {0xA0D5, 0x7468}, //3868 #big5-hkscs
    {0xA0D6, 0x6DB9}, //3869 #big5-hkscs
    {0xA0D7, 0x87C1}, //3870 #big5-hkscs
    {0xA0D8, 0x24011}, //3871 #big5-hkscs
    {0xA0D9, 0x74E7}, //3872 #big5-hkscs
    {0xA0DA, 0x3DDB}, //3873 #big5-hkscs
    {0xA0DB, 0x7176}, //3874 #big5-hkscs
    {0xA0DC, 0x60A4}, //3875 #big5-hkscs
    {0xA0DD, 0x619C}, //3876 #big5-hkscs
    {0xA0DE, 0x3CD1}, //3877 #big5-hkscs
    {0xA0DF, 0x7162}, //3878 #big5-hkscs
    {0xA0E0, 0x6077}, //3879 #big5-hkscs
    {0xA0E2, 0x7F71}, //3880 #big5-hkscs
    {0xA0E3, 0x28B2D}, //3881 #big5-hkscs
    {0xA0E4, 0x7250}, //3882 #big5-hkscs
    {0xA0E5, 0x60E9}, //3883 #big5-hkscs
    {0xA0E6, 0x4B7E}, //3884 #big5-hkscs
    {0xA0E7, 0x5220}, //3885 #big5-hkscs
    {0xA0E8, 0x3C18}, //3886 #big5-hkscs
    {0xA0E9, 0x23CC7}, //3887 #big5-hkscs
    {0xA0EA, 0x25ED7}, //3888 #big5-hkscs
    {0xA0EB, 0x27656}, //3889 #big5-hkscs
    {0xA0EC, 0x25531}, //3890 #big5-hkscs
    {0xA0ED, 0x21944}, //3891 #big5-hkscs
    {0xA0EE, 0x212FE}, //3892 #big5-hkscs
    {0xA0EF, 0x29903}, //3893 #big5-hkscs
    {0xA0F0, 0x26DDC}, //3894 #big5-hkscs
    {0xA0F1, 0x270AD}, //3895 #big5-hkscs
    {0xA0F2, 0x5CC1}, //3896 #big5-hkscs
    {0xA0F3, 0x261AD}, //3897 #big5-hkscs
    {0xA0F4, 0x28A0F}, //3898 #big5-hkscs
    {0xA0F5, 0x23677}, //3899 #big5-hkscs
    {0xA0F6, 0x200EE}, //3900 #big5-hkscs
    {0xA0F7, 0x26846}, //3901 #big5-hkscs
    {0xA0F8, 0x24F0E}, //3902 #big5-hkscs
    {0xA0F9, 0x4562}, //3903 #big5-hkscs
    {0xA0FA, 0x5B1F}, //3904 #big5-hkscs
    {0xA0FB, 0x2634C}, //3905 #big5-hkscs
    {0xA0FC, 0x9F50}, //3906 #big5-hkscs
    {0xA0FD, 0x9EA6}, //3907 #big5-hkscs
    {0xA0FE, 0x2626B}, //3908 #big5-hkscs
    {0xA140, 0x3000}, //3909 #IDEOGRAPHIC SPACE
    {0xA141, 0xFF0C}, //3910 #FULLWIDTH COMMA
    {0xA142, 0x3001}, //3911 #IDEOGRAPHIC COMMA
    {0xA143, 0x3002}, //3912 #IDEOGRAPHIC FULL STOP
    {0xA144, 0xFF0E}, //3913 #FULLWIDTH FULL STOP
    {0xA145, 0x2027}, //3914 #HYPHENATION POINT
    {0xA146, 0xFF1B}, //3915 #FULLWIDTH SEMICOLON
    {0xA147, 0xFF1A}, //3916 #FULLWIDTH COLON
    {0xA148, 0xFF1F}, //3917 #FULLWIDTH QUESTION MARK
    {0xA149, 0xFF01}, //3918 #FULLWIDTH EXCLAMATION MARK
    {0xA14A, 0xFE30}, //3919 #PRESENTATION FORM FOR VERTICAL TWO DOT LEADER
    {0xA14B, 0x2026}, //3920 #HORIZONTAL ELLIPSIS
    {0xA14C, 0x2025}, //3921 #TWO DOT LEADER
    {0xA14D, 0xFE50}, //3922 #SMALL COMMA
    {0xA14E, 0xFE51}, //3923 #SMALL IDEOGRAPHIC COMMA
    {0xA14F, 0xFE52}, //3924 #SMALL FULL STOP
    {0xA150, 0x00B7}, //3925 #MIDDLE DOT
    {0xA151, 0xFE54}, //3926 #SMALL SEMICOLON
    {0xA152, 0xFE55}, //3927 #SMALL COLON
    {0xA153, 0xFE56}, //3928 #SMALL QUESTION MARK
    {0xA154, 0xFE57}, //3929 #SMALL EXCLAMATION MARK
    {0xA155, 0xFF5C}, //3930 #FULLWIDTH VERTICAL LINE
    {0xA156, 0x2013}, //3931 #EN DASH
    {0xA157, 0xFE31}, //3932 #PRESENTATION FORM FOR VERTICAL EM DASH
    {0xA158, 0x2014}, //3933 #EM DASH
    {0xA159, 0xFE33}, //3934 #PRESENTATION FORM FOR VERTICAL LOW LINE
    {0xA15A, 0x2574}, //3935 #BOX DRAWINGS LIGHT LEFT
    {0xA15B, 0xFE34}, //3936 #PRESENTATION FORM FOR VERTICAL WAVY LOW LINE
    {0xA15C, 0xFE4F}, //3937 #WAVY LOW LINE
    {0xA15D, 0xFF08}, //3938 #FULLWIDTH LEFT PARENTHESIS
    {0xA15E, 0xFF09}, //3939 #FULLWIDTH RIGHT PARENTHESIS
    {0xA15F, 0xFE35}, //3940 #PRESENTATION FORM FOR VERTICAL LEFT PARENTHESIS
    {0xA160, 0xFE36}, //3941 #PRESENTATION FORM FOR VERTICAL RIGHT PARENTHESIS
    {0xA161, 0xFF5B}, //3942 #FULLWIDTH LEFT CURLY BRACKET
    {0xA162, 0xFF5D}, //3943 #FULLWIDTH RIGHT CURLY BRACKET
    {0xA163, 0xFE37}, //3944 #PRESENTATION FORM FOR VERTICAL LEFT CURLY BRACKET
    {0xA164, 0xFE38}, //3945 #PRESENTATION FORM FOR VERTICAL RIGHT CURLY BRACKET
    {0xA165, 0x3014}, //3946 #LEFT TORTOISE SHELL BRACKET
    {0xA166, 0x3015}, //3947 #RIGHT TORTOISE SHELL BRACKET
    {0xA167, 0xFE39}, //3948 #PRESENTATION FORM FOR VERTICAL LEFT TORTOISE SHELL BRACKET
    {0xA168, 0xFE3A}, //3949 #PRESENTATION FORM FOR VERTICAL RIGHT TORTOISE SHELL BRACKET
    {0xA169, 0x3010}, //3950 #LEFT BLACK LENTICULAR BRACKET
    {0xA16A, 0x3011}, //3951 #RIGHT BLACK LENTICULAR BRACKET
    {0xA16B, 0xFE3B}, //3952 #PRESENTATION FORM FOR VERTICAL LEFT BLACK LENTICULAR BRACKET
    {0xA16C, 0xFE3C}, //3953 #PRESENTATION FORM FOR VERTICAL RIGHT BLACK LENTICULAR BRACKET
    {0xA16D, 0x300A}, //3954 #LEFT DOUBLE ANGLE BRACKET
    {0xA16E, 0x300B}, //3955 #RIGHT DOUBLE ANGLE BRACKET
    {0xA16F, 0xFE3D}, //3956 #PRESENTATION FORM FOR VERTICAL LEFT DOUBLE ANGLE BRACKET
    {0xA170, 0xFE3E}, //3957 #PRESENTATION FORM FOR VERTICAL RIGHT DOUBLE ANGLE BRACKET
    {0xA171, 0x3008}, //3958 #LEFT ANGLE BRACKET
    {0xA172, 0x3009}, //3959 #RIGHT ANGLE BRACKET
    {0xA173, 0xFE3F}, //3960 #PRESENTATION FORM FOR VERTICAL LEFT ANGLE BRACKET
    {0xA174, 0xFE40}, //3961 #PRESENTATION FORM FOR VERTICAL RIGHT ANGLE BRACKET
    {0xA175, 0x300C}, //3962 #LEFT CORNER BRACKET
    {0xA176, 0x300D}, //3963 #RIGHT CORNER BRACKET
    {0xA177, 0xFE41}, //3964 #PRESENTATION FORM FOR VERTICAL LEFT CORNER BRACKET
    {0xA178, 0xFE42}, //3965 #PRESENTATION FORM FOR VERTICAL RIGHT CORNER BRACKET
    {0xA179, 0x300E}, //3966 #LEFT WHITE CORNER BRACKET
    {0xA17A, 0x300F}, //3967 #RIGHT WHITE CORNER BRACKET
    {0xA17B, 0xFE43}, //3968 #PRESENTATION FORM FOR VERTICAL LEFT WHITE CORNER BRACKET
    {0xA17C, 0xFE44}, //3969 #PRESENTATION FORM FOR VERTICAL RIGHT WHITE CORNER BRACKET
    {0xA17D, 0xFE59}, //3970 #SMALL LEFT PARENTHESIS
    {0xA17E, 0xFE5A}, //3971 #SMALL RIGHT PARENTHESIS
    {0xA1A1, 0xFE5B}, //3972 #SMALL LEFT CURLY BRACKET
    {0xA1A2, 0xFE5C}, //3973 #SMALL RIGHT CURLY BRACKET
    {0xA1A3, 0xFE5D}, //3974 #SMALL LEFT TORTOISE SHELL BRACKET
    {0xA1A4, 0xFE5E}, //3975 #SMALL RIGHT TORTOISE SHELL BRACKET
    {0xA1A5, 0x2018}, //3976 #LEFT SINGLE QUOTATION MARK
    {0xA1A6, 0x2019}, //3977 #RIGHT SINGLE QUOTATION MARK
    {0xA1A7, 0x201C}, //3978 #LEFT DOUBLE QUOTATION MARK
    {0xA1A8, 0x201D}, //3979 #RIGHT DOUBLE QUOTATION MARK
    {0xA1A9, 0x301D}, //3980 #REVERSED DOUBLE PRIME QUOTATION MARK
    {0xA1AA, 0x301E}, //3981 #DOUBLE PRIME QUOTATION MARK
    {0xA1AB, 0x2035}, //3982 #REVERSED PRIME
    {0xA1AC, 0x2032}, //3983 #PRIME
    {0xA1AD, 0xFF03}, //3984 #FULLWIDTH NUMBER SIGN
    {0xA1AE, 0xFF06}, //3985 #FULLWIDTH AMPERSAND
    {0xA1AF, 0xFF0A}, //3986 #FULLWIDTH ASTERISK
    {0xA1B0, 0x203B}, //3987 #REFERENCE MARK
    {0xA1B1, 0x00A7}, //3988 #SECTION SIGN
    {0xA1B2, 0x3003}, //3989 #DITTO MARK
    {0xA1B3, 0x25CB}, //3990 #WHITE CIRCLE
    {0xA1B4, 0x25CF}, //3991 #BLACK CIRCLE
    {0xA1B5, 0x25B3}, //3992 #WHITE UP-POINTING TRIANGLE
    {0xA1B6, 0x25B2}, //3993 #BLACK UP-POINTING TRIANGLE
    {0xA1B7, 0x25CE}, //3994 #BULLSEYE
    {0xA1B8, 0x2606}, //3995 #WHITE STAR
    {0xA1B9, 0x2605}, //3996 #BLACK STAR
    {0xA1BA, 0x25C7}, //3997 #WHITE DIAMOND
    {0xA1BB, 0x25C6}, //3998 #BLACK DIAMOND
    {0xA1BC, 0x25A1}, //3999 #WHITE SQUARE
    {0xA1BD, 0x25A0}, //4000 #BLACK SQUARE
    {0xA1BE, 0x25BD}, //4001 #WHITE DOWN-POINTING TRIANGLE
    {0xA1BF, 0x25BC}, //4002 #BLACK DOWN-POINTING TRIANGLE
    {0xA1C0, 0x32A3}, //4003 #CIRCLED IDEOGRAPH CORRECT
    {0xA1C1, 0x2105}, //4004 #CARE OF
    {0xA1C2, 0x00AF}, //4005 #MACRON
    {0xA1C3, 0xFFE3}, //4006 #FULLWIDTH MACRON
    {0xA1C4, 0xFF3F}, //4007 #FULLWIDTH LOW LINE
    {0xA1C5, 0x02CD}, //4008 #MODIFIER LETTER LOW MACRON
    {0xA1C6, 0xFE49}, //4009 #DASHED OVERLINE
    {0xA1C7, 0xFE4A}, //4010 #CENTRELINE OVERLINE
    {0xA1C8, 0xFE4D}, //4011 #DASHED LOW LINE
    {0xA1C9, 0xFE4E}, //4012 #CENTRELINE LOW LINE
    {0xA1CA, 0xFE4B}, //4013 #WAVY OVERLINE
    {0xA1CB, 0xFE4C}, //4014 #DOUBLE WAVY OVERLINE
    {0xA1CC, 0xFE5F}, //4015 #SMALL NUMBER SIGN
    {0xA1CD, 0xFE60}, //4016 #SMALL AMPERSAND
    {0xA1CE, 0xFE61}, //4017 #SMALL ASTERISK
    {0xA1CF, 0xFF0B}, //4018 #FULLWIDTH PLUS SIGN
    {0xA1D0, 0xFF0D}, //4019 #FULLWIDTH HYPHEN-MINUS
    {0xA1D1, 0x00D7}, //4020 #MULTIPLICATION SIGN
    {0xA1D2, 0x00F7}, //4021 #DIVISION SIGN
    {0xA1D3, 0x00B1}, //4022 #PLUS-MINUS SIGN
    {0xA1D4, 0x221A}, //4023 #SQUARE ROOT
    {0xA1D5, 0xFF1C}, //4024 #FULLWIDTH LESS-THAN SIGN
    {0xA1D6, 0xFF1E}, //4025 #FULLWIDTH GREATER-THAN SIGN
    {0xA1D7, 0xFF1D}, //4026 #FULLWIDTH EQUALS SIGN
    {0xA1D8, 0x2266}, //4027 #LESS-THAN OVER EQUAL TO
    {0xA1D9, 0x2267}, //4028 #GREATER-THAN OVER EQUAL TO
    {0xA1DA, 0x2260}, //4029 #NOT EQUAL TO
    {0xA1DB, 0x221E}, //4030 #INFINITY
    {0xA1DC, 0x2252}, //4031 #APPROXIMATELY EQUAL TO OR THE IMAGE OF
    {0xA1DD, 0x2261}, //4032 #IDENTICAL TO
    {0xA1DE, 0xFE62}, //4033 #SMALL PLUS SIGN
    {0xA1DF, 0xFE63}, //4034 #SMALL HYPHEN-MINUS
    {0xA1E0, 0xFE64}, //4035 #SMALL LESS-THAN SIGN
    {0xA1E1, 0xFE65}, //4036 #SMALL GREATER-THAN SIGN
    {0xA1E2, 0xFE66}, //4037 #SMALL EQUALS SIGN
    {0xA1E3, 0xFF5E}, //4038 #FULLWIDTH TILDE
    {0xA1E4, 0x2229}, //4039 #INTERSECTION
    {0xA1E5, 0x222A}, //4040 #UNION
    {0xA1E6, 0x22A5}, //4041 #UP TACK
    {0xA1E7, 0x2220}, //4042 #ANGLE
    {0xA1E8, 0x221F}, //4043 #RIGHT ANGLE
    {0xA1E9, 0x22BF}, //4044 #RIGHT TRIANGLE
    {0xA1EA, 0x33D2}, //4045 #SQUARE LOG
    {0xA1EB, 0x33D1}, //4046 #SQUARE LN
    {0xA1EC, 0x222B}, //4047 #INTEGRAL
    {0xA1ED, 0x222E}, //4048 #CONTOUR INTEGRAL
    {0xA1EE, 0x2235}, //4049 #BECAUSE
    {0xA1EF, 0x2234}, //4050 #THEREFORE
    {0xA1F0, 0x2640}, //4051 #FEMALE SIGN
    {0xA1F1, 0x2642}, //4052 #MALE SIGN
    {0xA1F2, 0x2295}, //4053 #CIRCLED PLUS
    {0xA1F3, 0x2299}, //4054 #CIRCLED DOT OPERATOR
    {0xA1F4, 0x2191}, //4055 #UPWARDS ARROW
    {0xA1F5, 0x2193}, //4056 #DOWNWARDS ARROW
    {0xA1F6, 0x2190}, //4057 #LEFTWARDS ARROW
    {0xA1F7, 0x2192}, //4058 #RIGHTWARDS ARROW
    {0xA1F8, 0x2196}, //4059 #NORTH WEST ARROW
    {0xA1F9, 0x2197}, //4060 #NORTH EAST ARROW
    {0xA1FA, 0x2199}, //4061 #SOUTH WEST ARROW
    {0xA1FB, 0x2198}, //4062 #SOUTH EAST ARROW
    {0xA1FC, 0x2225}, //4063 #PARALLEL TO
    {0xA1FD, 0x2223}, //4064 #DIVIDES
    {0xA1FE, 0xFF0F}, //4065 #FULLWIDTH SOLIDUS
    {0xA240, 0xFF3C}, //4066 #FULLWIDTH REVERSE SOLIDUS
    {0xA241, 0x2215}, //4067 #DIVISION SLASH
    {0xA242, 0xFE68}, //4068 #SMALL REVERSE SOLIDUS
    {0xA243, 0xFF04}, //4069 #FULLWIDTH DOLLAR SIGN
    {0xA244, 0xFFE5}, //4070 #FULLWIDTH YEN SIGN
    {0xA245, 0x3012}, //4071 #POSTAL MARK
    {0xA246, 0xFFE0}, //4072 #FULLWIDTH CENT SIGN
    {0xA247, 0xFFE1}, //4073 #FULLWIDTH POUND SIGN
    {0xA248, 0xFF05}, //4074 #FULLWIDTH PERCENT SIGN
    {0xA249, 0xFF20}, //4075 #FULLWIDTH COMMERCIAL AT
    {0xA24A, 0x2103}, //4076 #DEGREE CELSIUS
    {0xA24B, 0x2109}, //4077 #DEGREE FAHRENHEIT
    {0xA24C, 0xFE69}, //4078 #SMALL DOLLAR SIGN
    {0xA24D, 0xFE6A}, //4079 #SMALL PERCENT SIGN
    {0xA24E, 0xFE6B}, //4080 #SMALL COMMERCIAL AT
    {0xA24F, 0x33D5}, //4081 #SQUARE MIL
    {0xA250, 0x339C}, //4082 #SQUARE MM
    {0xA251, 0x339D}, //4083 #SQUARE CM
    {0xA252, 0x339E}, //4084 #SQUARE KM
    {0xA253, 0x33CE}, //4085 #SQUARE KM CAPITAL
    {0xA254, 0x33A1}, //4086 #SQUARE M SQUARED
    {0xA255, 0x338E}, //4087 #SQUARE MG
    {0xA256, 0x338F}, //4088 #SQUARE KG
    {0xA257, 0x33C4}, //4089 #SQUARE CC
    {0xA258, 0x00B0}, //4090 #DEGREE SIGN
    {0xA259, 0x5159}, //4091 #CJK UNIFIED IDEOGRAPH
    {0xA25A, 0x515B}, //4092 #CJK UNIFIED IDEOGRAPH
    {0xA25B, 0x515E}, //4093 #CJK UNIFIED IDEOGRAPH
    {0xA25C, 0x515D}, //4094 #CJK UNIFIED IDEOGRAPH
    {0xA25D, 0x5161}, //4095 #CJK UNIFIED IDEOGRAPH
    {0xA25E, 0x5163}, //4096 #CJK UNIFIED IDEOGRAPH
    {0xA25F, 0x55E7}, //4097 #CJK UNIFIED IDEOGRAPH
    {0xA260, 0x74E9}, //4098 #CJK UNIFIED IDEOGRAPH
    {0xA261, 0x7CCE}, //4099 #CJK UNIFIED IDEOGRAPH
    {0xA262, 0x2581}, //4100 #LOWER ONE EIGHTH BLOCK
    {0xA263, 0x2582}, //4101 #LOWER ONE QUARTER BLOCK
    {0xA264, 0x2583}, //4102 #LOWER THREE EIGHTHS BLOCK
    {0xA265, 0x2584}, //4103 #LOWER HALF BLOCK
    {0xA266, 0x2585}, //4104 #LOWER FIVE EIGHTHS BLOCK
    {0xA267, 0x2586}, //4105 #LOWER THREE QUARTERS BLOCK
    {0xA268, 0x2587}, //4106 #LOWER SEVEN EIGHTHS BLOCK
    {0xA269, 0x2588}, //4107 #FULL BLOCK
    {0xA26A, 0x258F}, //4108 #LEFT ONE EIGHTH BLOCK
    {0xA26B, 0x258E}, //4109 #LEFT ONE QUARTER BLOCK
    {0xA26C, 0x258D}, //4110 #LEFT THREE EIGHTHS BLOCK
    {0xA26D, 0x258C}, //4111 #LEFT HALF BLOCK
    {0xA26E, 0x258B}, //4112 #LEFT FIVE EIGHTHS BLOCK
    {0xA26F, 0x258A}, //4113 #LEFT THREE QUARTERS BLOCK
    {0xA270, 0x2589}, //4114 #LEFT SEVEN EIGHTHS BLOCK
    {0xA271, 0x253C}, //4115 #BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
    {0xA272, 0x2534}, //4116 #BOX DRAWINGS LIGHT UP AND HORIZONTAL
    {0xA273, 0x252C}, //4117 #BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
    {0xA274, 0x2524}, //4118 #BOX DRAWINGS LIGHT VERTICAL AND LEFT
    {0xA275, 0x251C}, //4119 #BOX DRAWINGS LIGHT VERTICAL AND RIGHT
    {0xA276, 0x2594}, //4120 #UPPER ONE EIGHTH BLOCK
    {0xA277, 0x2500}, //4121 #BOX DRAWINGS LIGHT HORIZONTAL
    {0xA278, 0x2502}, //4122 #BOX DRAWINGS LIGHT VERTICAL
    {0xA279, 0x2595}, //4123 #RIGHT ONE EIGHTH BLOCK
    {0xA27A, 0x250C}, //4124 #BOX DRAWINGS LIGHT DOWN AND RIGHT
    {0xA27B, 0x2510}, //4125 #BOX DRAWINGS LIGHT DOWN AND LEFT
    {0xA27C, 0x2514}, //4126 #BOX DRAWINGS LIGHT UP AND RIGHT
    {0xA27D, 0x2518}, //4127 #BOX DRAWINGS LIGHT UP AND LEFT
    {0xA27E, 0x256D}, //4128 #BOX DRAWINGS LIGHT ARC DOWN AND RIGHT
    {0xA2A1, 0x256E}, //4129 #BOX DRAWINGS LIGHT ARC DOWN AND LEFT
    {0xA2A2, 0x2570}, //4130 #BOX DRAWINGS LIGHT ARC UP AND RIGHT
    {0xA2A3, 0x256F}, //4131 #BOX DRAWINGS LIGHT ARC UP AND LEFT
    {0xA2A4, 0x2550}, //4132 #BOX DRAWINGS DOUBLE HORIZONTAL
    {0xA2A5, 0x255E}, //4133 #BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE
    {0xA2A6, 0x256A}, //4134 #BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE
    {0xA2A7, 0x2561}, //4135 #BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE
    {0xA2A8, 0x25E2}, //4136 #BLACK LOWER RIGHT TRIANGLE
    {0xA2A9, 0x25E3}, //4137 #BLACK LOWER LEFT TRIANGLE
    {0xA2AA, 0x25E5}, //4138 #BLACK UPPER RIGHT TRIANGLE
    {0xA2AB, 0x25E4}, //4139 #BLACK UPPER LEFT TRIANGLE
    {0xA2AC, 0x2571}, //4140 #BOX DRAWINGS LIGHT DIAGONAL UPPER RIGHT TO LOWER LEFT
    {0xA2AD, 0x2572}, //4141 #BOX DRAWINGS LIGHT DIAGONAL UPPER LEFT TO LOWER RIGHT
    {0xA2AE, 0x2573}, //4142 #BOX DRAWINGS LIGHT DIAGONAL CROSS
    {0xA2AF, 0xFF10}, //4143 #FULLWIDTH DIGIT ZERO
    {0xA2B0, 0xFF11}, //4144 #FULLWIDTH DIGIT ONE
    {0xA2B1, 0xFF12}, //4145 #FULLWIDTH DIGIT TWO
    {0xA2B2, 0xFF13}, //4146 #FULLWIDTH DIGIT THREE
    {0xA2B3, 0xFF14}, //4147 #FULLWIDTH DIGIT FOUR
    {0xA2B4, 0xFF15}, //4148 #FULLWIDTH DIGIT FIVE
    {0xA2B5, 0xFF16}, //4149 #FULLWIDTH DIGIT SIX
    {0xA2B6, 0xFF17}, //4150 #FULLWIDTH DIGIT SEVEN
    {0xA2B7, 0xFF18}, //4151 #FULLWIDTH DIGIT EIGHT
    {0xA2B8, 0xFF19}, //4152 #FULLWIDTH DIGIT NINE
    {0xA2B9, 0x2160}, //4153 #ROMAN NUMERAL ONE
    {0xA2BA, 0x2161}, //4154 #ROMAN NUMERAL TWO
    {0xA2BB, 0x2162}, //4155 #ROMAN NUMERAL THREE
    {0xA2BC, 0x2163}, //4156 #ROMAN NUMERAL FOUR
    {0xA2BD, 0x2164}, //4157 #ROMAN NUMERAL FIVE
    {0xA2BE, 0x2165}, //4158 #ROMAN NUMERAL SIX
    {0xA2BF, 0x2166}, //4159 #ROMAN NUMERAL SEVEN
    {0xA2C0, 0x2167}, //4160 #ROMAN NUMERAL EIGHT
    {0xA2C1, 0x2168}, //4161 #ROMAN NUMERAL NINE
    {0xA2C2, 0x2169}, //4162 #ROMAN NUMERAL TEN
    {0xA2C3, 0x3021}, //4163 #HANGZHOU NUMERAL ONE
    {0xA2C4, 0x3022}, //4164 #HANGZHOU NUMERAL TWO
    {0xA2C5, 0x3023}, //4165 #HANGZHOU NUMERAL THREE
    {0xA2C6, 0x3024}, //4166 #HANGZHOU NUMERAL FOUR
    {0xA2C7, 0x3025}, //4167 #HANGZHOU NUMERAL FIVE
    {0xA2C8, 0x3026}, //4168 #HANGZHOU NUMERAL SIX
    {0xA2C9, 0x3027}, //4169 #HANGZHOU NUMERAL SEVEN
    {0xA2CA, 0x3028}, //4170 #HANGZHOU NUMERAL EIGHT
    {0xA2CB, 0x3029}, //4171 #HANGZHOU NUMERAL NINE
    {0xA2CC, 0x5341}, //4172 #CJK UNIFIED IDEOGRAPH
    {0xA2CD, 0x5344}, //4173 #CJK UNIFIED IDEOGRAPH
    {0xA2CE, 0x5345}, //4174 #CJK UNIFIED IDEOGRAPH
    {0xA2CF, 0xFF21}, //4175 #FULLWIDTH LATIN CAPITAL LETTER A
    {0xA2D0, 0xFF22}, //4176 #FULLWIDTH LATIN CAPITAL LETTER B
    {0xA2D1, 0xFF23}, //4177 #FULLWIDTH LATIN CAPITAL LETTER C
    {0xA2D2, 0xFF24}, //4178 #FULLWIDTH LATIN CAPITAL LETTER D
    {0xA2D3, 0xFF25}, //4179 #FULLWIDTH LATIN CAPITAL LETTER E
    {0xA2D4, 0xFF26}, //4180 #FULLWIDTH LATIN CAPITAL LETTER F
    {0xA2D5, 0xFF27}, //4181 #FULLWIDTH LATIN CAPITAL LETTER G
    {0xA2D6, 0xFF28}, //4182 #FULLWIDTH LATIN CAPITAL LETTER H
    {0xA2D7, 0xFF29}, //4183 #FULLWIDTH LATIN CAPITAL LETTER I
    {0xA2D8, 0xFF2A}, //4184 #FULLWIDTH LATIN CAPITAL LETTER J
    {0xA2D9, 0xFF2B}, //4185 #FULLWIDTH LATIN CAPITAL LETTER K
    {0xA2DA, 0xFF2C}, //4186 #FULLWIDTH LATIN CAPITAL LETTER L
    {0xA2DB, 0xFF2D}, //4187 #FULLWIDTH LATIN CAPITAL LETTER M
    {0xA2DC, 0xFF2E}, //4188 #FULLWIDTH LATIN CAPITAL LETTER N
    {0xA2DD, 0xFF2F}, //4189 #FULLWIDTH LATIN CAPITAL LETTER O
    {0xA2DE, 0xFF30}, //4190 #FULLWIDTH LATIN CAPITAL LETTER P
    {0xA2DF, 0xFF31}, //4191 #FULLWIDTH LATIN CAPITAL LETTER Q
    {0xA2E0, 0xFF32}, //4192 #FULLWIDTH LATIN CAPITAL LETTER R
    {0xA2E1, 0xFF33}, //4193 #FULLWIDTH LATIN CAPITAL LETTER S
    {0xA2E2, 0xFF34}, //4194 #FULLWIDTH LATIN CAPITAL LETTER T
    {0xA2E3, 0xFF35}, //4195 #FULLWIDTH LATIN CAPITAL LETTER U
    {0xA2E4, 0xFF36}, //4196 #FULLWIDTH LATIN CAPITAL LETTER V
    {0xA2E5, 0xFF37}, //4197 #FULLWIDTH LATIN CAPITAL LETTER W
    {0xA2E6, 0xFF38}, //4198 #FULLWIDTH LATIN CAPITAL LETTER X
    {0xA2E7, 0xFF39}, //4199 #FULLWIDTH LATIN CAPITAL LETTER Y
    {0xA2E8, 0xFF3A}, //4200 #FULLWIDTH LATIN CAPITAL LETTER Z
    {0xA2E9, 0xFF41}, //4201 #FULLWIDTH LATIN SMALL LETTER A
    {0xA2EA, 0xFF42}, //4202 #FULLWIDTH LATIN SMALL LETTER B
    {0xA2EB, 0xFF43}, //4203 #FULLWIDTH LATIN SMALL LETTER C
    {0xA2EC, 0xFF44}, //4204 #FULLWIDTH LATIN SMALL LETTER D
    {0xA2ED, 0xFF45}, //4205 #FULLWIDTH LATIN SMALL LETTER E
    {0xA2EE, 0xFF46}, //4206 #FULLWIDTH LATIN SMALL LETTER F
    {0xA2EF, 0xFF47}, //4207 #FULLWIDTH LATIN SMALL LETTER G
    {0xA2F0, 0xFF48}, //4208 #FULLWIDTH LATIN SMALL LETTER H
    {0xA2F1, 0xFF49}, //4209 #FULLWIDTH LATIN SMALL LETTER I
    {0xA2F2, 0xFF4A}, //4210 #FULLWIDTH LATIN SMALL LETTER J
    {0xA2F3, 0xFF4B}, //4211 #FULLWIDTH LATIN SMALL LETTER K
    {0xA2F4, 0xFF4C}, //4212 #FULLWIDTH LATIN SMALL LETTER L
    {0xA2F5, 0xFF4D}, //4213 #FULLWIDTH LATIN SMALL LETTER M
    {0xA2F6, 0xFF4E}, //4214 #FULLWIDTH LATIN SMALL LETTER N
    {0xA2F7, 0xFF4F}, //4215 #FULLWIDTH LATIN SMALL LETTER O
    {0xA2F8, 0xFF50}, //4216 #FULLWIDTH LATIN SMALL LETTER P
    {0xA2F9, 0xFF51}, //4217 #FULLWIDTH LATIN SMALL LETTER Q
    {0xA2FA, 0xFF52}, //4218 #FULLWIDTH LATIN SMALL LETTER R
    {0xA2FB, 0xFF53}, //4219 #FULLWIDTH LATIN SMALL LETTER S
    {0xA2FC, 0xFF54}, //4220 #FULLWIDTH LATIN SMALL LETTER T
    {0xA2FD, 0xFF55}, //4221 #FULLWIDTH LATIN SMALL LETTER U
    {0xA2FE, 0xFF56}, //4222 #FULLWIDTH LATIN SMALL LETTER V
    {0xA340, 0xFF57}, //4223 #FULLWIDTH LATIN SMALL LETTER W
    {0xA341, 0xFF58}, //4224 #FULLWIDTH LATIN SMALL LETTER X
    {0xA342, 0xFF59}, //4225 #FULLWIDTH LATIN SMALL LETTER Y
    {0xA343, 0xFF5A}, //4226 #FULLWIDTH LATIN SMALL LETTER Z
    {0xA344, 0x0391}, //4227 #GREEK CAPITAL LETTER ALPHA
    {0xA345, 0x0392}, //4228 #GREEK CAPITAL LETTER BETA
    {0xA346, 0x0393}, //4229 #GREEK CAPITAL LETTER GAMMA
    {0xA347, 0x0394}, //4230 #GREEK CAPITAL LETTER DELTA
    {0xA348, 0x0395}, //4231 #GREEK CAPITAL LETTER EPSILON
    {0xA349, 0x0396}, //4232 #GREEK CAPITAL LETTER ZETA
    {0xA34A, 0x0397}, //4233 #GREEK CAPITAL LETTER ETA
    {0xA34B, 0x0398}, //4234 #GREEK CAPITAL LETTER THETA
    {0xA34C, 0x0399}, //4235 #GREEK CAPITAL LETTER IOTA
    {0xA34D, 0x039A}, //4236 #GREEK CAPITAL LETTER KAPPA
    {0xA34E, 0x039B}, //4237 #GREEK CAPITAL LETTER LAMDA
    {0xA34F, 0x039C}, //4238 #GREEK CAPITAL LETTER MU
    {0xA350, 0x039D}, //4239 #GREEK CAPITAL LETTER NU
    {0xA351, 0x039E}, //4240 #GREEK CAPITAL LETTER XI
    {0xA352, 0x039F}, //4241 #GREEK CAPITAL LETTER OMICRON
    {0xA353, 0x03A0}, //4242 #GREEK CAPITAL LETTER PI
    {0xA354, 0x03A1}, //4243 #GREEK CAPITAL LETTER RHO
    {0xA355, 0x03A3}, //4244 #GREEK CAPITAL LETTER SIGMA
    {0xA356, 0x03A4}, //4245 #GREEK CAPITAL LETTER TAU
    {0xA357, 0x03A5}, //4246 #GREEK CAPITAL LETTER UPSILON
    {0xA358, 0x03A6}, //4247 #GREEK CAPITAL LETTER PHI
    {0xA359, 0x03A7}, //4248 #GREEK CAPITAL LETTER CHI
    {0xA35A, 0x03A8}, //4249 #GREEK CAPITAL LETTER PSI
    {0xA35B, 0x03A9}, //4250 #GREEK CAPITAL LETTER OMEGA
    {0xA35C, 0x03B1}, //4251 #GREEK SMALL LETTER ALPHA
    {0xA35D, 0x03B2}, //4252 #GREEK SMALL LETTER BETA
    {0xA35E, 0x03B3}, //4253 #GREEK SMALL LETTER GAMMA
    {0xA35F, 0x03B4}, //4254 #GREEK SMALL LETTER DELTA
    {0xA360, 0x03B5}, //4255 #GREEK SMALL LETTER EPSILON
    {0xA361, 0x03B6}, //4256 #GREEK SMALL LETTER ZETA
    {0xA362, 0x03B7}, //4257 #GREEK SMALL LETTER ETA
    {0xA363, 0x03B8}, //4258 #GREEK SMALL LETTER THETA
    {0xA364, 0x03B9}, //4259 #GREEK SMALL LETTER IOTA
    {0xA365, 0x03BA}, //4260 #GREEK SMALL LETTER KAPPA
    {0xA366, 0x03BB}, //4261 #GREEK SMALL LETTER LAMDA
    {0xA367, 0x03BC}, //4262 #GREEK SMALL LETTER MU
    {0xA368, 0x03BD}, //4263 #GREEK SMALL LETTER NU
    {0xA369, 0x03BE}, //4264 #GREEK SMALL LETTER XI
    {0xA36A, 0x03BF}, //4265 #GREEK SMALL LETTER OMICRON
    {0xA36B, 0x03C0}, //4266 #GREEK SMALL LETTER PI
    {0xA36C, 0x03C1}, //4267 #GREEK SMALL LETTER RHO
    {0xA36D, 0x03C3}, //4268 #GREEK SMALL LETTER SIGMA
    {0xA36E, 0x03C4}, //4269 #GREEK SMALL LETTER TAU
    {0xA36F, 0x03C5}, //4270 #GREEK SMALL LETTER UPSILON
    {0xA370, 0x03C6}, //4271 #GREEK SMALL LETTER PHI
    {0xA371, 0x03C7}, //4272 #GREEK SMALL LETTER CHI
    {0xA372, 0x03C8}, //4273 #GREEK SMALL LETTER PSI
    {0xA373, 0x03C9}, //4274 #GREEK SMALL LETTER OMEGA
    {0xA374, 0x3105}, //4275 #BOPOMOFO LETTER B
    {0xA375, 0x3106}, //4276 #BOPOMOFO LETTER P
    {0xA376, 0x3107}, //4277 #BOPOMOFO LETTER M
    {0xA377, 0x3108}, //4278 #BOPOMOFO LETTER F
    {0xA378, 0x3109}, //4279 #BOPOMOFO LETTER D
    {0xA379, 0x310A}, //4280 #BOPOMOFO LETTER T
    {0xA37A, 0x310B}, //4281 #BOPOMOFO LETTER N
    {0xA37B, 0x310C}, //4282 #BOPOMOFO LETTER L
    {0xA37C, 0x310D}, //4283 #BOPOMOFO LETTER G
    {0xA37D, 0x310E}, //4284 #BOPOMOFO LETTER K
    {0xA37E, 0x310F}, //4285 #BOPOMOFO LETTER H
    {0xA3A1, 0x3110}, //4286 #BOPOMOFO LETTER J
    {0xA3A2, 0x3111}, //4287 #BOPOMOFO LETTER Q
    {0xA3A3, 0x3112}, //4288 #BOPOMOFO LETTER X
    {0xA3A4, 0x3113}, //4289 #BOPOMOFO LETTER ZH
    {0xA3A5, 0x3114}, //4290 #BOPOMOFO LETTER CH
    {0xA3A6, 0x3115}, //4291 #BOPOMOFO LETTER SH
    {0xA3A7, 0x3116}, //4292 #BOPOMOFO LETTER R
    {0xA3A8, 0x3117}, //4293 #BOPOMOFO LETTER Z
    {0xA3A9, 0x3118}, //4294 #BOPOMOFO LETTER C
    {0xA3AA, 0x3119}, //4295 #BOPOMOFO LETTER S
    {0xA3AB, 0x311A}, //4296 #BOPOMOFO LETTER A
    {0xA3AC, 0x311B}, //4297 #BOPOMOFO LETTER O
    {0xA3AD, 0x311C}, //4298 #BOPOMOFO LETTER E
    {0xA3AE, 0x311D}, //4299 #BOPOMOFO LETTER EH
    {0xA3AF, 0x311E}, //4300 #BOPOMOFO LETTER AI
    {0xA3B0, 0x311F}, //4301 #BOPOMOFO LETTER EI
    {0xA3B1, 0x3120}, //4302 #BOPOMOFO LETTER AU
    {0xA3B2, 0x3121}, //4303 #BOPOMOFO LETTER OU
    {0xA3B3, 0x3122}, //4304 #BOPOMOFO LETTER AN
    {0xA3B4, 0x3123}, //4305 #BOPOMOFO LETTER EN
    {0xA3B5, 0x3124}, //4306 #BOPOMOFO LETTER ANG
    {0xA3B6, 0x3125}, //4307 #BOPOMOFO LETTER ENG
    {0xA3B7, 0x3126}, //4308 #BOPOMOFO LETTER ER
    {0xA3B8, 0x3127}, //4309 #BOPOMOFO LETTER I
    {0xA3B9, 0x3128}, //4310 #BOPOMOFO LETTER U
    {0xA3BA, 0x3129}, //4311 #BOPOMOFO LETTER IU
    {0xA3BB, 0x02D9}, //4312 #DOT ABOVE
    {0xA3BC, 0x02C9}, //4313 #MODIFIER LETTER MACRON
    {0xA3BD, 0x02CA}, //4314 #MODIFIER LETTER ACUTE ACCENT
    {0xA3BE, 0x02C7}, //4315 #CARON
    {0xA3BF, 0x02CB}, //4316 #MODIFIER LETTER GRAVE ACCENT
    {0xA3C0, 0x2400}, //4317 #big5-hkscs
    {0xA3C1, 0x2401}, //4318 #big5-hkscs
    {0xA3C2, 0x2402}, //4319 #big5-hkscs
    {0xA3C3, 0x2403}, //4320 #big5-hkscs
    {0xA3C4, 0x2404}, //4321 #big5-hkscs
    {0xA3C5, 0x2405}, //4322 #big5-hkscs
    {0xA3C6, 0x2406}, //4323 #big5-hkscs
    {0xA3C7, 0x2407}, //4324 #big5-hkscs
    {0xA3C8, 0x2408}, //4325 #big5-hkscs
    {0xA3C9, 0x2409}, //4326 #big5-hkscs
    {0xA3CA, 0x240A}, //4327 #big5-hkscs
    {0xA3CB, 0x240B}, //4328 #big5-hkscs
    {0xA3CC, 0x240C}, //4329 #big5-hkscs
    {0xA3CD, 0x240D}, //4330 #big5-hkscs
    {0xA3CE, 0x240E}, //4331 #big5-hkscs
    {0xA3CF, 0x240F}, //4332 #big5-hkscs
    {0xA3D0, 0x2410}, //4333 #big5-hkscs
    {0xA3D1, 0x2411}, //4334 #big5-hkscs
    {0xA3D2, 0x2412}, //4335 #big5-hkscs
    {0xA3D3, 0x2413}, //4336 #big5-hkscs
    {0xA3D4, 0x2414}, //4337 #big5-hkscs
    {0xA3D5, 0x2415}, //4338 #big5-hkscs
    {0xA3D6, 0x2416}, //4339 #big5-hkscs
    {0xA3D7, 0x2417}, //4340 #big5-hkscs
    {0xA3D8, 0x2418}, //4341 #big5-hkscs
    {0xA3D9, 0x2419}, //4342 #big5-hkscs
    {0xA3DA, 0x241A}, //4343 #big5-hkscs
    {0xA3DB, 0x241B}, //4344 #big5-hkscs
    {0xA3DC, 0x241C}, //4345 #big5-hkscs
    {0xA3DD, 0x241D}, //4346 #big5-hkscs
    {0xA3DE, 0x241E}, //4347 #big5-hkscs
    {0xA3DF, 0x241F}, //4348 #big5-hkscs
    {0xA3E0, 0x2421}, //4349 #big5-hkscs
    {0xA3E1, 0x20AC}, //4350 #EURO SIGN
    {0xA440, 0x4E00}, //4351 #CJK UNIFIED IDEOGRAPH
    {0xA441, 0x4E59}, //4352 #CJK UNIFIED IDEOGRAPH
    {0xA442, 0x4E01}, //4353 #CJK UNIFIED IDEOGRAPH
    {0xA443, 0x4E03}, //4354 #CJK UNIFIED IDEOGRAPH
    {0xA444, 0x4E43}, //4355 #CJK UNIFIED IDEOGRAPH
    {0xA445, 0x4E5D}, //4356 #CJK UNIFIED IDEOGRAPH
    {0xA446, 0x4E86}, //4357 #CJK UNIFIED IDEOGRAPH
    {0xA447, 0x4E8C}, //4358 #CJK UNIFIED IDEOGRAPH
    {0xA448, 0x4EBA}, //4359 #CJK UNIFIED IDEOGRAPH
    {0xA449, 0x513F}, //4360 #CJK UNIFIED IDEOGRAPH
    {0xA44A, 0x5165}, //4361 #CJK UNIFIED IDEOGRAPH
    {0xA44B, 0x516B}, //4362 #CJK UNIFIED IDEOGRAPH
    {0xA44C, 0x51E0}, //4363 #CJK UNIFIED IDEOGRAPH
    {0xA44D, 0x5200}, //4364 #CJK UNIFIED IDEOGRAPH
    {0xA44E, 0x5201}, //4365 #CJK UNIFIED IDEOGRAPH
    {0xA44F, 0x529B}, //4366 #CJK UNIFIED IDEOGRAPH
    {0xA450, 0x5315}, //4367 #CJK UNIFIED IDEOGRAPH
    {0xA451, 0x5341}, //4368 #CJK UNIFIED IDEOGRAPH
    {0xA452, 0x535C}, //4369 #CJK UNIFIED IDEOGRAPH
    {0xA453, 0x53C8}, //4370 #CJK UNIFIED IDEOGRAPH
    {0xA454, 0x4E09}, //4371 #CJK UNIFIED IDEOGRAPH
    {0xA455, 0x4E0B}, //4372 #CJK UNIFIED IDEOGRAPH
    {0xA456, 0x4E08}, //4373 #CJK UNIFIED IDEOGRAPH
    {0xA457, 0x4E0A}, //4374 #CJK UNIFIED IDEOGRAPH
    {0xA458, 0x4E2B}, //4375 #CJK UNIFIED IDEOGRAPH
    {0xA459, 0x4E38}, //4376 #CJK UNIFIED IDEOGRAPH
    {0xA45A, 0x51E1}, //4377 #CJK UNIFIED IDEOGRAPH
    {0xA45B, 0x4E45}, //4378 #CJK UNIFIED IDEOGRAPH
    {0xA45C, 0x4E48}, //4379 #CJK UNIFIED IDEOGRAPH
    {0xA45D, 0x4E5F}, //4380 #CJK UNIFIED IDEOGRAPH
    {0xA45E, 0x4E5E}, //4381 #CJK UNIFIED IDEOGRAPH
    {0xA45F, 0x4E8E}, //4382 #CJK UNIFIED IDEOGRAPH
    {0xA460, 0x4EA1}, //4383 #CJK UNIFIED IDEOGRAPH
    {0xA461, 0x5140}, //4384 #CJK UNIFIED IDEOGRAPH
    {0xA462, 0x5203}, //4385 #CJK UNIFIED IDEOGRAPH
    {0xA463, 0x52FA}, //4386 #CJK UNIFIED IDEOGRAPH
    {0xA464, 0x5343}, //4387 #CJK UNIFIED IDEOGRAPH
    {0xA465, 0x53C9}, //4388 #CJK UNIFIED IDEOGRAPH
    {0xA466, 0x53E3}, //4389 #CJK UNIFIED IDEOGRAPH
    {0xA467, 0x571F}, //4390 #CJK UNIFIED IDEOGRAPH
    {0xA468, 0x58EB}, //4391 #CJK UNIFIED IDEOGRAPH
    {0xA469, 0x5915}, //4392 #CJK UNIFIED IDEOGRAPH
    {0xA46A, 0x5927}, //4393 #CJK UNIFIED IDEOGRAPH
    {0xA46B, 0x5973}, //4394 #CJK UNIFIED IDEOGRAPH
    {0xA46C, 0x5B50}, //4395 #CJK UNIFIED IDEOGRAPH
    {0xA46D, 0x5B51}, //4396 #CJK UNIFIED IDEOGRAPH
    {0xA46E, 0x5B53}, //4397 #CJK UNIFIED IDEOGRAPH
    {0xA46F, 0x5BF8}, //4398 #CJK UNIFIED IDEOGRAPH
    {0xA470, 0x5C0F}, //4399 #CJK UNIFIED IDEOGRAPH
    {0xA471, 0x5C22}, //4400 #CJK UNIFIED IDEOGRAPH
    {0xA472, 0x5C38}, //4401 #CJK UNIFIED IDEOGRAPH
    {0xA473, 0x5C71}, //4402 #CJK UNIFIED IDEOGRAPH
    {0xA474, 0x5DDD}, //4403 #CJK UNIFIED IDEOGRAPH
    {0xA475, 0x5DE5}, //4404 #CJK UNIFIED IDEOGRAPH
    {0xA476, 0x5DF1}, //4405 #CJK UNIFIED IDEOGRAPH
    {0xA477, 0x5DF2}, //4406 #CJK UNIFIED IDEOGRAPH
    {0xA478, 0x5DF3}, //4407 #CJK UNIFIED IDEOGRAPH
    {0xA479, 0x5DFE}, //4408 #CJK UNIFIED IDEOGRAPH
    {0xA47A, 0x5E72}, //4409 #CJK UNIFIED IDEOGRAPH
    {0xA47B, 0x5EFE}, //4410 #CJK UNIFIED IDEOGRAPH
    {0xA47C, 0x5F0B}, //4411 #CJK UNIFIED IDEOGRAPH
    {0xA47D, 0x5F13}, //4412 #CJK UNIFIED IDEOGRAPH
    {0xA47E, 0x624D}, //4413 #CJK UNIFIED IDEOGRAPH
    {0xA4A1, 0x4E11}, //4414 #CJK UNIFIED IDEOGRAPH
    {0xA4A2, 0x4E10}, //4415 #CJK UNIFIED IDEOGRAPH
    {0xA4A3, 0x4E0D}, //4416 #CJK UNIFIED IDEOGRAPH
    {0xA4A4, 0x4E2D}, //4417 #CJK UNIFIED IDEOGRAPH
    {0xA4A5, 0x4E30}, //4418 #CJK UNIFIED IDEOGRAPH
    {0xA4A6, 0x4E39}, //4419 #CJK UNIFIED IDEOGRAPH
    {0xA4A7, 0x4E4B}, //4420 #CJK UNIFIED IDEOGRAPH
    {0xA4A8, 0x5C39}, //4421 #CJK UNIFIED IDEOGRAPH
    {0xA4A9, 0x4E88}, //4422 #CJK UNIFIED IDEOGRAPH
    {0xA4AA, 0x4E91}, //4423 #CJK UNIFIED IDEOGRAPH
    {0xA4AB, 0x4E95}, //4424 #CJK UNIFIED IDEOGRAPH
    {0xA4AC, 0x4E92}, //4425 #CJK UNIFIED IDEOGRAPH
    {0xA4AD, 0x4E94}, //4426 #CJK UNIFIED IDEOGRAPH
    {0xA4AE, 0x4EA2}, //4427 #CJK UNIFIED IDEOGRAPH
    {0xA4AF, 0x4EC1}, //4428 #CJK UNIFIED IDEOGRAPH
    {0xA4B0, 0x4EC0}, //4429 #CJK UNIFIED IDEOGRAPH
    {0xA4B1, 0x4EC3}, //4430 #CJK UNIFIED IDEOGRAPH
    {0xA4B2, 0x4EC6}, //4431 #CJK UNIFIED IDEOGRAPH
    {0xA4B3, 0x4EC7}, //4432 #CJK UNIFIED IDEOGRAPH
    {0xA4B4, 0x4ECD}, //4433 #CJK UNIFIED IDEOGRAPH
    {0xA4B5, 0x4ECA}, //4434 #CJK UNIFIED IDEOGRAPH
    {0xA4B6, 0x4ECB}, //4435 #CJK UNIFIED IDEOGRAPH
    {0xA4B7, 0x4EC4}, //4436 #CJK UNIFIED IDEOGRAPH
    {0xA4B8, 0x5143}, //4437 #CJK UNIFIED IDEOGRAPH
    {0xA4B9, 0x5141}, //4438 #CJK UNIFIED IDEOGRAPH
    {0xA4BA, 0x5167}, //4439 #CJK UNIFIED IDEOGRAPH
    {0xA4BB, 0x516D}, //4440 #CJK UNIFIED IDEOGRAPH
    {0xA4BC, 0x516E}, //4441 #CJK UNIFIED IDEOGRAPH
    {0xA4BD, 0x516C}, //4442 #CJK UNIFIED IDEOGRAPH
    {0xA4BE, 0x5197}, //4443 #CJK UNIFIED IDEOGRAPH
    {0xA4BF, 0x51F6}, //4444 #CJK UNIFIED IDEOGRAPH
    {0xA4C0, 0x5206}, //4445 #CJK UNIFIED IDEOGRAPH
    {0xA4C1, 0x5207}, //4446 #CJK UNIFIED IDEOGRAPH
    {0xA4C2, 0x5208}, //4447 #CJK UNIFIED IDEOGRAPH
    {0xA4C3, 0x52FB}, //4448 #CJK UNIFIED IDEOGRAPH
    {0xA4C4, 0x52FE}, //4449 #CJK UNIFIED IDEOGRAPH
    {0xA4C5, 0x52FF}, //4450 #CJK UNIFIED IDEOGRAPH
    {0xA4C6, 0x5316}, //4451 #CJK UNIFIED IDEOGRAPH
    {0xA4C7, 0x5339}, //4452 #CJK UNIFIED IDEOGRAPH
    {0xA4C8, 0x5348}, //4453 #CJK UNIFIED IDEOGRAPH
    {0xA4C9, 0x5347}, //4454 #CJK UNIFIED IDEOGRAPH
    {0xA4CA, 0x5345}, //4455 #CJK UNIFIED IDEOGRAPH
    {0xA4CB, 0x535E}, //4456 #CJK UNIFIED IDEOGRAPH
    {0xA4CC, 0x5384}, //4457 #CJK UNIFIED IDEOGRAPH
    {0xA4CD, 0x53CB}, //4458 #CJK UNIFIED IDEOGRAPH
    {0xA4CE, 0x53CA}, //4459 #CJK UNIFIED IDEOGRAPH
    {0xA4CF, 0x53CD}, //4460 #CJK UNIFIED IDEOGRAPH
    {0xA4D0, 0x58EC}, //4461 #CJK UNIFIED IDEOGRAPH
    {0xA4D1, 0x5929}, //4462 #CJK UNIFIED IDEOGRAPH
    {0xA4D2, 0x592B}, //4463 #CJK UNIFIED IDEOGRAPH
    {0xA4D3, 0x592A}, //4464 #CJK UNIFIED IDEOGRAPH
    {0xA4D4, 0x592D}, //4465 #CJK UNIFIED IDEOGRAPH
    {0xA4D5, 0x5B54}, //4466 #CJK UNIFIED IDEOGRAPH
    {0xA4D6, 0x5C11}, //4467 #CJK UNIFIED IDEOGRAPH
    {0xA4D7, 0x5C24}, //4468 #CJK UNIFIED IDEOGRAPH
    {0xA4D8, 0x5C3A}, //4469 #CJK UNIFIED IDEOGRAPH
    {0xA4D9, 0x5C6F}, //4470 #CJK UNIFIED IDEOGRAPH
    {0xA4DA, 0x5DF4}, //4471 #CJK UNIFIED IDEOGRAPH
    {0xA4DB, 0x5E7B}, //4472 #CJK UNIFIED IDEOGRAPH
    {0xA4DC, 0x5EFF}, //4473 #CJK UNIFIED IDEOGRAPH
    {0xA4DD, 0x5F14}, //4474 #CJK UNIFIED IDEOGRAPH
    {0xA4DE, 0x5F15}, //4475 #CJK UNIFIED IDEOGRAPH
    {0xA4DF, 0x5FC3}, //4476 #CJK UNIFIED IDEOGRAPH
    {0xA4E0, 0x6208}, //4477 #CJK UNIFIED IDEOGRAPH
    {0xA4E1, 0x6236}, //4478 #CJK UNIFIED IDEOGRAPH
    {0xA4E2, 0x624B}, //4479 #CJK UNIFIED IDEOGRAPH
    {0xA4E3, 0x624E}, //4480 #CJK UNIFIED IDEOGRAPH
    {0xA4E4, 0x652F}, //4481 #CJK UNIFIED IDEOGRAPH
    {0xA4E5, 0x6587}, //4482 #CJK UNIFIED IDEOGRAPH
    {0xA4E6, 0x6597}, //4483 #CJK UNIFIED IDEOGRAPH
    {0xA4E7, 0x65A4}, //4484 #CJK UNIFIED IDEOGRAPH
    {0xA4E8, 0x65B9}, //4485 #CJK UNIFIED IDEOGRAPH
    {0xA4E9, 0x65E5}, //4486 #CJK UNIFIED IDEOGRAPH
    {0xA4EA, 0x66F0}, //4487 #CJK UNIFIED IDEOGRAPH
    {0xA4EB, 0x6708}, //4488 #CJK UNIFIED IDEOGRAPH
    {0xA4EC, 0x6728}, //4489 #CJK UNIFIED IDEOGRAPH
    {0xA4ED, 0x6B20}, //4490 #CJK UNIFIED IDEOGRAPH
    {0xA4EE, 0x6B62}, //4491 #CJK UNIFIED IDEOGRAPH
    {0xA4EF, 0x6B79}, //4492 #CJK UNIFIED IDEOGRAPH
    {0xA4F0, 0x6BCB}, //4493 #CJK UNIFIED IDEOGRAPH
    {0xA4F1, 0x6BD4}, //4494 #CJK UNIFIED IDEOGRAPH
    {0xA4F2, 0x6BDB}, //4495 #CJK UNIFIED IDEOGRAPH
    {0xA4F3, 0x6C0F}, //4496 #CJK UNIFIED IDEOGRAPH
    {0xA4F4, 0x6C34}, //4497 #CJK UNIFIED IDEOGRAPH
    {0xA4F5, 0x706B}, //4498 #CJK UNIFIED IDEOGRAPH
    {0xA4F6, 0x722A}, //4499 #CJK UNIFIED IDEOGRAPH
    {0xA4F7, 0x7236}, //4500 #CJK UNIFIED IDEOGRAPH
    {0xA4F8, 0x723B}, //4501 #CJK UNIFIED IDEOGRAPH
    {0xA4F9, 0x7247}, //4502 #CJK UNIFIED IDEOGRAPH
    {0xA4FA, 0x7259}, //4503 #CJK UNIFIED IDEOGRAPH
    {0xA4FB, 0x725B}, //4504 #CJK UNIFIED IDEOGRAPH
    {0xA4FC, 0x72AC}, //4505 #CJK UNIFIED IDEOGRAPH
    {0xA4FD, 0x738B}, //4506 #CJK UNIFIED IDEOGRAPH
    {0xA4FE, 0x4E19}, //4507 #CJK UNIFIED IDEOGRAPH
    {0xA540, 0x4E16}, //4508 #CJK UNIFIED IDEOGRAPH
    {0xA541, 0x4E15}, //4509 #CJK UNIFIED IDEOGRAPH
    {0xA542, 0x4E14}, //4510 #CJK UNIFIED IDEOGRAPH
    {0xA543, 0x4E18}, //4511 #CJK UNIFIED IDEOGRAPH
    {0xA544, 0x4E3B}, //4512 #CJK UNIFIED IDEOGRAPH
    {0xA545, 0x4E4D}, //4513 #CJK UNIFIED IDEOGRAPH
    {0xA546, 0x4E4F}, //4514 #CJK UNIFIED IDEOGRAPH
    {0xA547, 0x4E4E}, //4515 #CJK UNIFIED IDEOGRAPH
    {0xA548, 0x4EE5}, //4516 #CJK UNIFIED IDEOGRAPH
    {0xA549, 0x4ED8}, //4517 #CJK UNIFIED IDEOGRAPH
    {0xA54A, 0x4ED4}, //4518 #CJK UNIFIED IDEOGRAPH
    {0xA54B, 0x4ED5}, //4519 #CJK UNIFIED IDEOGRAPH
    {0xA54C, 0x4ED6}, //4520 #CJK UNIFIED IDEOGRAPH
    {0xA54D, 0x4ED7}, //4521 #CJK UNIFIED IDEOGRAPH
    {0xA54E, 0x4EE3}, //4522 #CJK UNIFIED IDEOGRAPH
    {0xA54F, 0x4EE4}, //4523 #CJK UNIFIED IDEOGRAPH
    {0xA550, 0x4ED9}, //4524 #CJK UNIFIED IDEOGRAPH
    {0xA551, 0x4EDE}, //4525 #CJK UNIFIED IDEOGRAPH
    {0xA552, 0x5145}, //4526 #CJK UNIFIED IDEOGRAPH
    {0xA553, 0x5144}, //4527 #CJK UNIFIED IDEOGRAPH
    {0xA554, 0x5189}, //4528 #CJK UNIFIED IDEOGRAPH
    {0xA555, 0x518A}, //4529 #CJK UNIFIED IDEOGRAPH
    {0xA556, 0x51AC}, //4530 #CJK UNIFIED IDEOGRAPH
    {0xA557, 0x51F9}, //4531 #CJK UNIFIED IDEOGRAPH
    {0xA558, 0x51FA}, //4532 #CJK UNIFIED IDEOGRAPH
    {0xA559, 0x51F8}, //4533 #CJK UNIFIED IDEOGRAPH
    {0xA55A, 0x520A}, //4534 #CJK UNIFIED IDEOGRAPH
    {0xA55B, 0x52A0}, //4535 #CJK UNIFIED IDEOGRAPH
    {0xA55C, 0x529F}, //4536 #CJK UNIFIED IDEOGRAPH
    {0xA55D, 0x5305}, //4537 #CJK UNIFIED IDEOGRAPH
    {0xA55E, 0x5306}, //4538 #CJK UNIFIED IDEOGRAPH
    {0xA55F, 0x5317}, //4539 #CJK UNIFIED IDEOGRAPH
    {0xA560, 0x531D}, //4540 #CJK UNIFIED IDEOGRAPH
    {0xA561, 0x4EDF}, //4541 #CJK UNIFIED IDEOGRAPH
    {0xA562, 0x534A}, //4542 #CJK UNIFIED IDEOGRAPH
    {0xA563, 0x5349}, //4543 #CJK UNIFIED IDEOGRAPH
    {0xA564, 0x5361}, //4544 #CJK UNIFIED IDEOGRAPH
    {0xA565, 0x5360}, //4545 #CJK UNIFIED IDEOGRAPH
    {0xA566, 0x536F}, //4546 #CJK UNIFIED IDEOGRAPH
    {0xA567, 0x536E}, //4547 #CJK UNIFIED IDEOGRAPH
    {0xA568, 0x53BB}, //4548 #CJK UNIFIED IDEOGRAPH
    {0xA569, 0x53EF}, //4549 #CJK UNIFIED IDEOGRAPH
    {0xA56A, 0x53E4}, //4550 #CJK UNIFIED IDEOGRAPH
    {0xA56B, 0x53F3}, //4551 #CJK UNIFIED IDEOGRAPH
    {0xA56C, 0x53EC}, //4552 #CJK UNIFIED IDEOGRAPH
    {0xA56D, 0x53EE}, //4553 #CJK UNIFIED IDEOGRAPH
    {0xA56E, 0x53E9}, //4554 #CJK UNIFIED IDEOGRAPH
    {0xA56F, 0x53E8}, //4555 #CJK UNIFIED IDEOGRAPH
    {0xA570, 0x53FC}, //4556 #CJK UNIFIED IDEOGRAPH
    {0xA571, 0x53F8}, //4557 #CJK UNIFIED IDEOGRAPH
    {0xA572, 0x53F5}, //4558 #CJK UNIFIED IDEOGRAPH
    {0xA573, 0x53EB}, //4559 #CJK UNIFIED IDEOGRAPH
    {0xA574, 0x53E6}, //4560 #CJK UNIFIED IDEOGRAPH
    {0xA575, 0x53EA}, //4561 #CJK UNIFIED IDEOGRAPH
    {0xA576, 0x53F2}, //4562 #CJK UNIFIED IDEOGRAPH
    {0xA577, 0x53F1}, //4563 #CJK UNIFIED IDEOGRAPH
    {0xA578, 0x53F0}, //4564 #CJK UNIFIED IDEOGRAPH
    {0xA579, 0x53E5}, //4565 #CJK UNIFIED IDEOGRAPH
    {0xA57A, 0x53ED}, //4566 #CJK UNIFIED IDEOGRAPH
    {0xA57B, 0x53FB}, //4567 #CJK UNIFIED IDEOGRAPH
    {0xA57C, 0x56DB}, //4568 #CJK UNIFIED IDEOGRAPH
    {0xA57D, 0x56DA}, //4569 #CJK UNIFIED IDEOGRAPH
    {0xA57E, 0x5916}, //4570 #CJK UNIFIED IDEOGRAPH
    {0xA5A1, 0x592E}, //4571 #CJK UNIFIED IDEOGRAPH
    {0xA5A2, 0x5931}, //4572 #CJK UNIFIED IDEOGRAPH
    {0xA5A3, 0x5974}, //4573 #CJK UNIFIED IDEOGRAPH
    {0xA5A4, 0x5976}, //4574 #CJK UNIFIED IDEOGRAPH
    {0xA5A5, 0x5B55}, //4575 #CJK UNIFIED IDEOGRAPH
    {0xA5A6, 0x5B83}, //4576 #CJK UNIFIED IDEOGRAPH
    {0xA5A7, 0x5C3C}, //4577 #CJK UNIFIED IDEOGRAPH
    {0xA5A8, 0x5DE8}, //4578 #CJK UNIFIED IDEOGRAPH
    {0xA5A9, 0x5DE7}, //4579 #CJK UNIFIED IDEOGRAPH
    {0xA5AA, 0x5DE6}, //4580 #CJK UNIFIED IDEOGRAPH
    {0xA5AB, 0x5E02}, //4581 #CJK UNIFIED IDEOGRAPH
    {0xA5AC, 0x5E03}, //4582 #CJK UNIFIED IDEOGRAPH
    {0xA5AD, 0x5E73}, //4583 #CJK UNIFIED IDEOGRAPH
    {0xA5AE, 0x5E7C}, //4584 #CJK UNIFIED IDEOGRAPH
    {0xA5AF, 0x5F01}, //4585 #CJK UNIFIED IDEOGRAPH
    {0xA5B0, 0x5F18}, //4586 #CJK UNIFIED IDEOGRAPH
    {0xA5B1, 0x5F17}, //4587 #CJK UNIFIED IDEOGRAPH
    {0xA5B2, 0x5FC5}, //4588 #CJK UNIFIED IDEOGRAPH
    {0xA5B3, 0x620A}, //4589 #CJK UNIFIED IDEOGRAPH
    {0xA5B4, 0x6253}, //4590 #CJK UNIFIED IDEOGRAPH
    {0xA5B5, 0x6254}, //4591 #CJK UNIFIED IDEOGRAPH
    {0xA5B6, 0x6252}, //4592 #CJK UNIFIED IDEOGRAPH
    {0xA5B7, 0x6251}, //4593 #CJK UNIFIED IDEOGRAPH
    {0xA5B8, 0x65A5}, //4594 #CJK UNIFIED IDEOGRAPH
    {0xA5B9, 0x65E6}, //4595 #CJK UNIFIED IDEOGRAPH
    {0xA5BA, 0x672E}, //4596 #CJK UNIFIED IDEOGRAPH
    {0xA5BB, 0x672C}, //4597 #CJK UNIFIED IDEOGRAPH
    {0xA5BC, 0x672A}, //4598 #CJK UNIFIED IDEOGRAPH
    {0xA5BD, 0x672B}, //4599 #CJK UNIFIED IDEOGRAPH
    {0xA5BE, 0x672D}, //4600 #CJK UNIFIED IDEOGRAPH
    {0xA5BF, 0x6B63}, //4601 #CJK UNIFIED IDEOGRAPH
    {0xA5C0, 0x6BCD}, //4602 #CJK UNIFIED IDEOGRAPH
    {0xA5C1, 0x6C11}, //4603 #CJK UNIFIED IDEOGRAPH
    {0xA5C2, 0x6C10}, //4604 #CJK UNIFIED IDEOGRAPH
    {0xA5C3, 0x6C38}, //4605 #CJK UNIFIED IDEOGRAPH
    {0xA5C4, 0x6C41}, //4606 #CJK UNIFIED IDEOGRAPH
    {0xA5C5, 0x6C40}, //4607 #CJK UNIFIED IDEOGRAPH
    {0xA5C6, 0x6C3E}, //4608 #CJK UNIFIED IDEOGRAPH
    {0xA5C7, 0x72AF}, //4609 #CJK UNIFIED IDEOGRAPH
    {0xA5C8, 0x7384}, //4610 #CJK UNIFIED IDEOGRAPH
    {0xA5C9, 0x7389}, //4611 #CJK UNIFIED IDEOGRAPH
    {0xA5CA, 0x74DC}, //4612 #CJK UNIFIED IDEOGRAPH
    {0xA5CB, 0x74E6}, //4613 #CJK UNIFIED IDEOGRAPH
    {0xA5CC, 0x7518}, //4614 #CJK UNIFIED IDEOGRAPH
    {0xA5CD, 0x751F}, //4615 #CJK UNIFIED IDEOGRAPH
    {0xA5CE, 0x7528}, //4616 #CJK UNIFIED IDEOGRAPH
    {0xA5CF, 0x7529}, //4617 #CJK UNIFIED IDEOGRAPH
    {0xA5D0, 0x7530}, //4618 #CJK UNIFIED IDEOGRAPH
    {0xA5D1, 0x7531}, //4619 #CJK UNIFIED IDEOGRAPH
    {0xA5D2, 0x7532}, //4620 #CJK UNIFIED IDEOGRAPH
    {0xA5D3, 0x7533}, //4621 #CJK UNIFIED IDEOGRAPH
    {0xA5D4, 0x758B}, //4622 #CJK UNIFIED IDEOGRAPH
    {0xA5D5, 0x767D}, //4623 #CJK UNIFIED IDEOGRAPH
    {0xA5D6, 0x76AE}, //4624 #CJK UNIFIED IDEOGRAPH
    {0xA5D7, 0x76BF}, //4625 #CJK UNIFIED IDEOGRAPH
    {0xA5D8, 0x76EE}, //4626 #CJK UNIFIED IDEOGRAPH
    {0xA5D9, 0x77DB}, //4627 #CJK UNIFIED IDEOGRAPH
    {0xA5DA, 0x77E2}, //4628 #CJK UNIFIED IDEOGRAPH
    {0xA5DB, 0x77F3}, //4629 #CJK UNIFIED IDEOGRAPH
    {0xA5DC, 0x793A}, //4630 #CJK UNIFIED IDEOGRAPH
    {0xA5DD, 0x79BE}, //4631 #CJK UNIFIED IDEOGRAPH
    {0xA5DE, 0x7A74}, //4632 #CJK UNIFIED IDEOGRAPH
    {0xA5DF, 0x7ACB}, //4633 #CJK UNIFIED IDEOGRAPH
    {0xA5E0, 0x4E1E}, //4634 #CJK UNIFIED IDEOGRAPH
    {0xA5E1, 0x4E1F}, //4635 #CJK UNIFIED IDEOGRAPH
    {0xA5E2, 0x4E52}, //4636 #CJK UNIFIED IDEOGRAPH
    {0xA5E3, 0x4E53}, //4637 #CJK UNIFIED IDEOGRAPH
    {0xA5E4, 0x4E69}, //4638 #CJK UNIFIED IDEOGRAPH
    {0xA5E5, 0x4E99}, //4639 #CJK UNIFIED IDEOGRAPH
    {0xA5E6, 0x4EA4}, //4640 #CJK UNIFIED IDEOGRAPH
    {0xA5E7, 0x4EA6}, //4641 #CJK UNIFIED IDEOGRAPH
    {0xA5E8, 0x4EA5}, //4642 #CJK UNIFIED IDEOGRAPH
    {0xA5E9, 0x4EFF}, //4643 #CJK UNIFIED IDEOGRAPH
    {0xA5EA, 0x4F09}, //4644 #CJK UNIFIED IDEOGRAPH
    {0xA5EB, 0x4F19}, //4645 #CJK UNIFIED IDEOGRAPH
    {0xA5EC, 0x4F0A}, //4646 #CJK UNIFIED IDEOGRAPH
    {0xA5ED, 0x4F15}, //4647 #CJK UNIFIED IDEOGRAPH
    {0xA5EE, 0x4F0D}, //4648 #CJK UNIFIED IDEOGRAPH
    {0xA5EF, 0x4F10}, //4649 #CJK UNIFIED IDEOGRAPH
    {0xA5F0, 0x4F11}, //4650 #CJK UNIFIED IDEOGRAPH
    {0xA5F1, 0x4F0F}, //4651 #CJK UNIFIED IDEOGRAPH
    {0xA5F2, 0x4EF2}, //4652 #CJK UNIFIED IDEOGRAPH
    {0xA5F3, 0x4EF6}, //4653 #CJK UNIFIED IDEOGRAPH
    {0xA5F4, 0x4EFB}, //4654 #CJK UNIFIED IDEOGRAPH
    {0xA5F5, 0x4EF0}, //4655 #CJK UNIFIED IDEOGRAPH
    {0xA5F6, 0x4EF3}, //4656 #CJK UNIFIED IDEOGRAPH
    {0xA5F7, 0x4EFD}, //4657 #CJK UNIFIED IDEOGRAPH
    {0xA5F8, 0x4F01}, //4658 #CJK UNIFIED IDEOGRAPH
    {0xA5F9, 0x4F0B}, //4659 #CJK UNIFIED IDEOGRAPH
    {0xA5FA, 0x5149}, //4660 #CJK UNIFIED IDEOGRAPH
    {0xA5FB, 0x5147}, //4661 #CJK UNIFIED IDEOGRAPH
    {0xA5FC, 0x5146}, //4662 #CJK UNIFIED IDEOGRAPH
    {0xA5FD, 0x5148}, //4663 #CJK UNIFIED IDEOGRAPH
    {0xA5FE, 0x5168}, //4664 #CJK UNIFIED IDEOGRAPH
    {0xA640, 0x5171}, //4665 #CJK UNIFIED IDEOGRAPH
    {0xA641, 0x518D}, //4666 #CJK UNIFIED IDEOGRAPH
    {0xA642, 0x51B0}, //4667 #CJK UNIFIED IDEOGRAPH
    {0xA643, 0x5217}, //4668 #CJK UNIFIED IDEOGRAPH
    {0xA644, 0x5211}, //4669 #CJK UNIFIED IDEOGRAPH
    {0xA645, 0x5212}, //4670 #CJK UNIFIED IDEOGRAPH
    {0xA646, 0x520E}, //4671 #CJK UNIFIED IDEOGRAPH
    {0xA647, 0x5216}, //4672 #CJK UNIFIED IDEOGRAPH
    {0xA648, 0x52A3}, //4673 #CJK UNIFIED IDEOGRAPH
    {0xA649, 0x5308}, //4674 #CJK UNIFIED IDEOGRAPH
    {0xA64A, 0x5321}, //4675 #CJK UNIFIED IDEOGRAPH
    {0xA64B, 0x5320}, //4676 #CJK UNIFIED IDEOGRAPH
    {0xA64C, 0x5370}, //4677 #CJK UNIFIED IDEOGRAPH
    {0xA64D, 0x5371}, //4678 #CJK UNIFIED IDEOGRAPH
    {0xA64E, 0x5409}, //4679 #CJK UNIFIED IDEOGRAPH
    {0xA64F, 0x540F}, //4680 #CJK UNIFIED IDEOGRAPH
    {0xA650, 0x540C}, //4681 #CJK UNIFIED IDEOGRAPH
    {0xA651, 0x540A}, //4682 #CJK UNIFIED IDEOGRAPH
    {0xA652, 0x5410}, //4683 #CJK UNIFIED IDEOGRAPH
    {0xA653, 0x5401}, //4684 #CJK UNIFIED IDEOGRAPH
    {0xA654, 0x540B}, //4685 #CJK UNIFIED IDEOGRAPH
    {0xA655, 0x5404}, //4686 #CJK UNIFIED IDEOGRAPH
    {0xA656, 0x5411}, //4687 #CJK UNIFIED IDEOGRAPH
    {0xA657, 0x540D}, //4688 #CJK UNIFIED IDEOGRAPH
    {0xA658, 0x5408}, //4689 #CJK UNIFIED IDEOGRAPH
    {0xA659, 0x5403}, //4690 #CJK UNIFIED IDEOGRAPH
    {0xA65A, 0x540E}, //4691 #CJK UNIFIED IDEOGRAPH
    {0xA65B, 0x5406}, //4692 #CJK UNIFIED IDEOGRAPH
    {0xA65C, 0x5412}, //4693 #CJK UNIFIED IDEOGRAPH
    {0xA65D, 0x56E0}, //4694 #CJK UNIFIED IDEOGRAPH
    {0xA65E, 0x56DE}, //4695 #CJK UNIFIED IDEOGRAPH
    {0xA65F, 0x56DD}, //4696 #CJK UNIFIED IDEOGRAPH
    {0xA660, 0x5733}, //4697 #CJK UNIFIED IDEOGRAPH
    {0xA661, 0x5730}, //4698 #CJK UNIFIED IDEOGRAPH
    {0xA662, 0x5728}, //4699 #CJK UNIFIED IDEOGRAPH
    {0xA663, 0x572D}, //4700 #CJK UNIFIED IDEOGRAPH
    {0xA664, 0x572C}, //4701 #CJK UNIFIED IDEOGRAPH
    {0xA665, 0x572F}, //4702 #CJK UNIFIED IDEOGRAPH
    {0xA666, 0x5729}, //4703 #CJK UNIFIED IDEOGRAPH
    {0xA667, 0x5919}, //4704 #CJK UNIFIED IDEOGRAPH
    {0xA668, 0x591A}, //4705 #CJK UNIFIED IDEOGRAPH
    {0xA669, 0x5937}, //4706 #CJK UNIFIED IDEOGRAPH
    {0xA66A, 0x5938}, //4707 #CJK UNIFIED IDEOGRAPH
    {0xA66B, 0x5984}, //4708 #CJK UNIFIED IDEOGRAPH
    {0xA66C, 0x5978}, //4709 #CJK UNIFIED IDEOGRAPH
    {0xA66D, 0x5983}, //4710 #CJK UNIFIED IDEOGRAPH
    {0xA66E, 0x597D}, //4711 #CJK UNIFIED IDEOGRAPH
    {0xA66F, 0x5979}, //4712 #CJK UNIFIED IDEOGRAPH
    {0xA670, 0x5982}, //4713 #CJK UNIFIED IDEOGRAPH
    {0xA671, 0x5981}, //4714 #CJK UNIFIED IDEOGRAPH
    {0xA672, 0x5B57}, //4715 #CJK UNIFIED IDEOGRAPH
    {0xA673, 0x5B58}, //4716 #CJK UNIFIED IDEOGRAPH
    {0xA674, 0x5B87}, //4717 #CJK UNIFIED IDEOGRAPH
    {0xA675, 0x5B88}, //4718 #CJK UNIFIED IDEOGRAPH
    {0xA676, 0x5B85}, //4719 #CJK UNIFIED IDEOGRAPH
    {0xA677, 0x5B89}, //4720 #CJK UNIFIED IDEOGRAPH
    {0xA678, 0x5BFA}, //4721 #CJK UNIFIED IDEOGRAPH
    {0xA679, 0x5C16}, //4722 #CJK UNIFIED IDEOGRAPH
    {0xA67A, 0x5C79}, //4723 #CJK UNIFIED IDEOGRAPH
    {0xA67B, 0x5DDE}, //4724 #CJK UNIFIED IDEOGRAPH
    {0xA67C, 0x5E06}, //4725 #CJK UNIFIED IDEOGRAPH
    {0xA67D, 0x5E76}, //4726 #CJK UNIFIED IDEOGRAPH
    {0xA67E, 0x5E74}, //4727 #CJK UNIFIED IDEOGRAPH
    {0xA6A1, 0x5F0F}, //4728 #CJK UNIFIED IDEOGRAPH
    {0xA6A2, 0x5F1B}, //4729 #CJK UNIFIED IDEOGRAPH
    {0xA6A3, 0x5FD9}, //4730 #CJK UNIFIED IDEOGRAPH
    {0xA6A4, 0x5FD6}, //4731 #CJK UNIFIED IDEOGRAPH
    {0xA6A5, 0x620E}, //4732 #CJK UNIFIED IDEOGRAPH
    {0xA6A6, 0x620C}, //4733 #CJK UNIFIED IDEOGRAPH
    {0xA6A7, 0x620D}, //4734 #CJK UNIFIED IDEOGRAPH
    {0xA6A8, 0x6210}, //4735 #CJK UNIFIED IDEOGRAPH
    {0xA6A9, 0x6263}, //4736 #CJK UNIFIED IDEOGRAPH
    {0xA6AA, 0x625B}, //4737 #CJK UNIFIED IDEOGRAPH
    {0xA6AB, 0x6258}, //4738 #CJK UNIFIED IDEOGRAPH
    {0xA6AC, 0x6536}, //4739 #CJK UNIFIED IDEOGRAPH
    {0xA6AD, 0x65E9}, //4740 #CJK UNIFIED IDEOGRAPH
    {0xA6AE, 0x65E8}, //4741 #CJK UNIFIED IDEOGRAPH
    {0xA6AF, 0x65EC}, //4742 #CJK UNIFIED IDEOGRAPH
    {0xA6B0, 0x65ED}, //4743 #CJK UNIFIED IDEOGRAPH
    {0xA6B1, 0x66F2}, //4744 #CJK UNIFIED IDEOGRAPH
    {0xA6B2, 0x66F3}, //4745 #CJK UNIFIED IDEOGRAPH
    {0xA6B3, 0x6709}, //4746 #CJK UNIFIED IDEOGRAPH
    {0xA6B4, 0x673D}, //4747 #CJK UNIFIED IDEOGRAPH
    {0xA6B5, 0x6734}, //4748 #CJK UNIFIED IDEOGRAPH
    {0xA6B6, 0x6731}, //4749 #CJK UNIFIED IDEOGRAPH
    {0xA6B7, 0x6735}, //4750 #CJK UNIFIED IDEOGRAPH
    {0xA6B8, 0x6B21}, //4751 #CJK UNIFIED IDEOGRAPH
    {0xA6B9, 0x6B64}, //4752 #CJK UNIFIED IDEOGRAPH
    {0xA6BA, 0x6B7B}, //4753 #CJK UNIFIED IDEOGRAPH
    {0xA6BB, 0x6C16}, //4754 #CJK UNIFIED IDEOGRAPH
    {0xA6BC, 0x6C5D}, //4755 #CJK UNIFIED IDEOGRAPH
    {0xA6BD, 0x6C57}, //4756 #CJK UNIFIED IDEOGRAPH
    {0xA6BE, 0x6C59}, //4757 #CJK UNIFIED IDEOGRAPH
    {0xA6BF, 0x6C5F}, //4758 #CJK UNIFIED IDEOGRAPH
    {0xA6C0, 0x6C60}, //4759 #CJK UNIFIED IDEOGRAPH
    {0xA6C1, 0x6C50}, //4760 #CJK UNIFIED IDEOGRAPH
    {0xA6C2, 0x6C55}, //4761 #CJK UNIFIED IDEOGRAPH
    {0xA6C3, 0x6C61}, //4762 #CJK UNIFIED IDEOGRAPH
    {0xA6C4, 0x6C5B}, //4763 #CJK UNIFIED IDEOGRAPH
    {0xA6C5, 0x6C4D}, //4764 #CJK UNIFIED IDEOGRAPH
    {0xA6C6, 0x6C4E}, //4765 #CJK UNIFIED IDEOGRAPH
    {0xA6C7, 0x7070}, //4766 #CJK UNIFIED IDEOGRAPH
    {0xA6C8, 0x725F}, //4767 #CJK UNIFIED IDEOGRAPH
    {0xA6C9, 0x725D}, //4768 #CJK UNIFIED IDEOGRAPH
    {0xA6CA, 0x767E}, //4769 #CJK UNIFIED IDEOGRAPH
    {0xA6CB, 0x7AF9}, //4770 #CJK UNIFIED IDEOGRAPH
    {0xA6CC, 0x7C73}, //4771 #CJK UNIFIED IDEOGRAPH
    {0xA6CD, 0x7CF8}, //4772 #CJK UNIFIED IDEOGRAPH
    {0xA6CE, 0x7F36}, //4773 #CJK UNIFIED IDEOGRAPH
    {0xA6CF, 0x7F8A}, //4774 #CJK UNIFIED IDEOGRAPH
    {0xA6D0, 0x7FBD}, //4775 #CJK UNIFIED IDEOGRAPH
    {0xA6D1, 0x8001}, //4776 #CJK UNIFIED IDEOGRAPH
    {0xA6D2, 0x8003}, //4777 #CJK UNIFIED IDEOGRAPH
    {0xA6D3, 0x800C}, //4778 #CJK UNIFIED IDEOGRAPH
    {0xA6D4, 0x8012}, //4779 #CJK UNIFIED IDEOGRAPH
    {0xA6D5, 0x8033}, //4780 #CJK UNIFIED IDEOGRAPH
    {0xA6D6, 0x807F}, //4781 #CJK UNIFIED IDEOGRAPH
    {0xA6D7, 0x8089}, //4782 #CJK UNIFIED IDEOGRAPH
    {0xA6D8, 0x808B}, //4783 #CJK UNIFIED IDEOGRAPH
    {0xA6D9, 0x808C}, //4784 #CJK UNIFIED IDEOGRAPH
    {0xA6DA, 0x81E3}, //4785 #CJK UNIFIED IDEOGRAPH
    {0xA6DB, 0x81EA}, //4786 #CJK UNIFIED IDEOGRAPH
    {0xA6DC, 0x81F3}, //4787 #CJK UNIFIED IDEOGRAPH
    {0xA6DD, 0x81FC}, //4788 #CJK UNIFIED IDEOGRAPH
    {0xA6DE, 0x820C}, //4789 #CJK UNIFIED IDEOGRAPH
    {0xA6DF, 0x821B}, //4790 #CJK UNIFIED IDEOGRAPH
    {0xA6E0, 0x821F}, //4791 #CJK UNIFIED IDEOGRAPH
    {0xA6E1, 0x826E}, //4792 #CJK UNIFIED IDEOGRAPH
    {0xA6E2, 0x8272}, //4793 #CJK UNIFIED IDEOGRAPH
    {0xA6E3, 0x827E}, //4794 #CJK UNIFIED IDEOGRAPH
    {0xA6E4, 0x866B}, //4795 #CJK UNIFIED IDEOGRAPH
    {0xA6E5, 0x8840}, //4796 #CJK UNIFIED IDEOGRAPH
    {0xA6E6, 0x884C}, //4797 #CJK UNIFIED IDEOGRAPH
    {0xA6E7, 0x8863}, //4798 #CJK UNIFIED IDEOGRAPH
    {0xA6E8, 0x897F}, //4799 #CJK UNIFIED IDEOGRAPH
    {0xA6E9, 0x9621}, //4800 #CJK UNIFIED IDEOGRAPH
    {0xA6EA, 0x4E32}, //4801 #CJK UNIFIED IDEOGRAPH
    {0xA6EB, 0x4EA8}, //4802 #CJK UNIFIED IDEOGRAPH
    {0xA6EC, 0x4F4D}, //4803 #CJK UNIFIED IDEOGRAPH
    {0xA6ED, 0x4F4F}, //4804 #CJK UNIFIED IDEOGRAPH
    {0xA6EE, 0x4F47}, //4805 #CJK UNIFIED IDEOGRAPH
    {0xA6EF, 0x4F57}, //4806 #CJK UNIFIED IDEOGRAPH
    {0xA6F0, 0x4F5E}, //4807 #CJK UNIFIED IDEOGRAPH
    {0xA6F1, 0x4F34}, //4808 #CJK UNIFIED IDEOGRAPH
    {0xA6F2, 0x4F5B}, //4809 #CJK UNIFIED IDEOGRAPH
    {0xA6F3, 0x4F55}, //4810 #CJK UNIFIED IDEOGRAPH
    {0xA6F4, 0x4F30}, //4811 #CJK UNIFIED IDEOGRAPH
    {0xA6F5, 0x4F50}, //4812 #CJK UNIFIED IDEOGRAPH
    {0xA6F6, 0x4F51}, //4813 #CJK UNIFIED IDEOGRAPH
    {0xA6F7, 0x4F3D}, //4814 #CJK UNIFIED IDEOGRAPH
    {0xA6F8, 0x4F3A}, //4815 #CJK UNIFIED IDEOGRAPH
    {0xA6F9, 0x4F38}, //4816 #CJK UNIFIED IDEOGRAPH
    {0xA6FA, 0x4F43}, //4817 #CJK UNIFIED IDEOGRAPH
    {0xA6FB, 0x4F54}, //4818 #CJK UNIFIED IDEOGRAPH
    {0xA6FC, 0x4F3C}, //4819 #CJK UNIFIED IDEOGRAPH
    {0xA6FD, 0x4F46}, //4820 #CJK UNIFIED IDEOGRAPH
    {0xA6FE, 0x4F63}, //4821 #CJK UNIFIED IDEOGRAPH
    {0xA740, 0x4F5C}, //4822 #CJK UNIFIED IDEOGRAPH
    {0xA741, 0x4F60}, //4823 #CJK UNIFIED IDEOGRAPH
    {0xA742, 0x4F2F}, //4824 #CJK UNIFIED IDEOGRAPH
    {0xA743, 0x4F4E}, //4825 #CJK UNIFIED IDEOGRAPH
    {0xA744, 0x4F36}, //4826 #CJK UNIFIED IDEOGRAPH
    {0xA745, 0x4F59}, //4827 #CJK UNIFIED IDEOGRAPH
    {0xA746, 0x4F5D}, //4828 #CJK UNIFIED IDEOGRAPH
    {0xA747, 0x4F48}, //4829 #CJK UNIFIED IDEOGRAPH
    {0xA748, 0x4F5A}, //4830 #CJK UNIFIED IDEOGRAPH
    {0xA749, 0x514C}, //4831 #CJK UNIFIED IDEOGRAPH
    {0xA74A, 0x514B}, //4832 #CJK UNIFIED IDEOGRAPH
    {0xA74B, 0x514D}, //4833 #CJK UNIFIED IDEOGRAPH
    {0xA74C, 0x5175}, //4834 #CJK UNIFIED IDEOGRAPH
    {0xA74D, 0x51B6}, //4835 #CJK UNIFIED IDEOGRAPH
    {0xA74E, 0x51B7}, //4836 #CJK UNIFIED IDEOGRAPH
    {0xA74F, 0x5225}, //4837 #CJK UNIFIED IDEOGRAPH
    {0xA750, 0x5224}, //4838 #CJK UNIFIED IDEOGRAPH
    {0xA751, 0x5229}, //4839 #CJK UNIFIED IDEOGRAPH
    {0xA752, 0x522A}, //4840 #CJK UNIFIED IDEOGRAPH
    {0xA753, 0x5228}, //4841 #CJK UNIFIED IDEOGRAPH
    {0xA754, 0x52AB}, //4842 #CJK UNIFIED IDEOGRAPH
    {0xA755, 0x52A9}, //4843 #CJK UNIFIED IDEOGRAPH
    {0xA756, 0x52AA}, //4844 #CJK UNIFIED IDEOGRAPH
    {0xA757, 0x52AC}, //4845 #CJK UNIFIED IDEOGRAPH
    {0xA758, 0x5323}, //4846 #CJK UNIFIED IDEOGRAPH
    {0xA759, 0x5373}, //4847 #CJK UNIFIED IDEOGRAPH
    {0xA75A, 0x5375}, //4848 #CJK UNIFIED IDEOGRAPH
    {0xA75B, 0x541D}, //4849 #CJK UNIFIED IDEOGRAPH
    {0xA75C, 0x542D}, //4850 #CJK UNIFIED IDEOGRAPH
    {0xA75D, 0x541E}, //4851 #CJK UNIFIED IDEOGRAPH
    {0xA75E, 0x543E}, //4852 #CJK UNIFIED IDEOGRAPH
    {0xA75F, 0x5426}, //4853 #CJK UNIFIED IDEOGRAPH
    {0xA760, 0x544E}, //4854 #CJK UNIFIED IDEOGRAPH
    {0xA761, 0x5427}, //4855 #CJK UNIFIED IDEOGRAPH
    {0xA762, 0x5446}, //4856 #CJK UNIFIED IDEOGRAPH
    {0xA763, 0x5443}, //4857 #CJK UNIFIED IDEOGRAPH
    {0xA764, 0x5433}, //4858 #CJK UNIFIED IDEOGRAPH
    {0xA765, 0x5448}, //4859 #CJK UNIFIED IDEOGRAPH
    {0xA766, 0x5442}, //4860 #CJK UNIFIED IDEOGRAPH
    {0xA767, 0x541B}, //4861 #CJK UNIFIED IDEOGRAPH
    {0xA768, 0x5429}, //4862 #CJK UNIFIED IDEOGRAPH
    {0xA769, 0x544A}, //4863 #CJK UNIFIED IDEOGRAPH
    {0xA76A, 0x5439}, //4864 #CJK UNIFIED IDEOGRAPH
    {0xA76B, 0x543B}, //4865 #CJK UNIFIED IDEOGRAPH
    {0xA76C, 0x5438}, //4866 #CJK UNIFIED IDEOGRAPH
    {0xA76D, 0x542E}, //4867 #CJK UNIFIED IDEOGRAPH
    {0xA76E, 0x5435}, //4868 #CJK UNIFIED IDEOGRAPH
    {0xA76F, 0x5436}, //4869 #CJK UNIFIED IDEOGRAPH
    {0xA770, 0x5420}, //4870 #CJK UNIFIED IDEOGRAPH
    {0xA771, 0x543C}, //4871 #CJK UNIFIED IDEOGRAPH
    {0xA772, 0x5440}, //4872 #CJK UNIFIED IDEOGRAPH
    {0xA773, 0x5431}, //4873 #CJK UNIFIED IDEOGRAPH
    {0xA774, 0x542B}, //4874 #CJK UNIFIED IDEOGRAPH
    {0xA775, 0x541F}, //4875 #CJK UNIFIED IDEOGRAPH
    {0xA776, 0x542C}, //4876 #CJK UNIFIED IDEOGRAPH
    {0xA777, 0x56EA}, //4877 #CJK UNIFIED IDEOGRAPH
    {0xA778, 0x56F0}, //4878 #CJK UNIFIED IDEOGRAPH
    {0xA779, 0x56E4}, //4879 #CJK UNIFIED IDEOGRAPH
    {0xA77A, 0x56EB}, //4880 #CJK UNIFIED IDEOGRAPH
    {0xA77B, 0x574A}, //4881 #CJK UNIFIED IDEOGRAPH
    {0xA77C, 0x5751}, //4882 #CJK UNIFIED IDEOGRAPH
    {0xA77D, 0x5740}, //4883 #CJK UNIFIED IDEOGRAPH
    {0xA77E, 0x574D}, //4884 #CJK UNIFIED IDEOGRAPH
    {0xA7A1, 0x5747}, //4885 #CJK UNIFIED IDEOGRAPH
    {0xA7A2, 0x574E}, //4886 #CJK UNIFIED IDEOGRAPH
    {0xA7A3, 0x573E}, //4887 #CJK UNIFIED IDEOGRAPH
    {0xA7A4, 0x5750}, //4888 #CJK UNIFIED IDEOGRAPH
    {0xA7A5, 0x574F}, //4889 #CJK UNIFIED IDEOGRAPH
    {0xA7A6, 0x573B}, //4890 #CJK UNIFIED IDEOGRAPH
    {0xA7A7, 0x58EF}, //4891 #CJK UNIFIED IDEOGRAPH
    {0xA7A8, 0x593E}, //4892 #CJK UNIFIED IDEOGRAPH
    {0xA7A9, 0x599D}, //4893 #CJK UNIFIED IDEOGRAPH
    {0xA7AA, 0x5992}, //4894 #CJK UNIFIED IDEOGRAPH
    {0xA7AB, 0x59A8}, //4895 #CJK UNIFIED IDEOGRAPH
    {0xA7AC, 0x599E}, //4896 #CJK UNIFIED IDEOGRAPH
    {0xA7AD, 0x59A3}, //4897 #CJK UNIFIED IDEOGRAPH
    {0xA7AE, 0x5999}, //4898 #CJK UNIFIED IDEOGRAPH
    {0xA7AF, 0x5996}, //4899 #CJK UNIFIED IDEOGRAPH
    {0xA7B0, 0x598D}, //4900 #CJK UNIFIED IDEOGRAPH
    {0xA7B1, 0x59A4}, //4901 #CJK UNIFIED IDEOGRAPH
    {0xA7B2, 0x5993}, //4902 #CJK UNIFIED IDEOGRAPH
    {0xA7B3, 0x598A}, //4903 #CJK UNIFIED IDEOGRAPH
    {0xA7B4, 0x59A5}, //4904 #CJK UNIFIED IDEOGRAPH
    {0xA7B5, 0x5B5D}, //4905 #CJK UNIFIED IDEOGRAPH
    {0xA7B6, 0x5B5C}, //4906 #CJK UNIFIED IDEOGRAPH
    {0xA7B7, 0x5B5A}, //4907 #CJK UNIFIED IDEOGRAPH
    {0xA7B8, 0x5B5B}, //4908 #CJK UNIFIED IDEOGRAPH
    {0xA7B9, 0x5B8C}, //4909 #CJK UNIFIED IDEOGRAPH
    {0xA7BA, 0x5B8B}, //4910 #CJK UNIFIED IDEOGRAPH
    {0xA7BB, 0x5B8F}, //4911 #CJK UNIFIED IDEOGRAPH
    {0xA7BC, 0x5C2C}, //4912 #CJK UNIFIED IDEOGRAPH
    {0xA7BD, 0x5C40}, //4913 #CJK UNIFIED IDEOGRAPH
    {0xA7BE, 0x5C41}, //4914 #CJK UNIFIED IDEOGRAPH
    {0xA7BF, 0x5C3F}, //4915 #CJK UNIFIED IDEOGRAPH
    {0xA7C0, 0x5C3E}, //4916 #CJK UNIFIED IDEOGRAPH
    {0xA7C1, 0x5C90}, //4917 #CJK UNIFIED IDEOGRAPH
    {0xA7C2, 0x5C91}, //4918 #CJK UNIFIED IDEOGRAPH
    {0xA7C3, 0x5C94}, //4919 #CJK UNIFIED IDEOGRAPH
    {0xA7C4, 0x5C8C}, //4920 #CJK UNIFIED IDEOGRAPH
    {0xA7C5, 0x5DEB}, //4921 #CJK UNIFIED IDEOGRAPH
    {0xA7C6, 0x5E0C}, //4922 #CJK UNIFIED IDEOGRAPH
    {0xA7C7, 0x5E8F}, //4923 #CJK UNIFIED IDEOGRAPH
    {0xA7C8, 0x5E87}, //4924 #CJK UNIFIED IDEOGRAPH
    {0xA7C9, 0x5E8A}, //4925 #CJK UNIFIED IDEOGRAPH
    {0xA7CA, 0x5EF7}, //4926 #CJK UNIFIED IDEOGRAPH
    {0xA7CB, 0x5F04}, //4927 #CJK UNIFIED IDEOGRAPH
    {0xA7CC, 0x5F1F}, //4928 #CJK UNIFIED IDEOGRAPH
    {0xA7CD, 0x5F64}, //4929 #CJK UNIFIED IDEOGRAPH
    {0xA7CE, 0x5F62}, //4930 #CJK UNIFIED IDEOGRAPH
    {0xA7CF, 0x5F77}, //4931 #CJK UNIFIED IDEOGRAPH
    {0xA7D0, 0x5F79}, //4932 #CJK UNIFIED IDEOGRAPH
    {0xA7D1, 0x5FD8}, //4933 #CJK UNIFIED IDEOGRAPH
    {0xA7D2, 0x5FCC}, //4934 #CJK UNIFIED IDEOGRAPH
    {0xA7D3, 0x5FD7}, //4935 #CJK UNIFIED IDEOGRAPH
    {0xA7D4, 0x5FCD}, //4936 #CJK UNIFIED IDEOGRAPH
    {0xA7D5, 0x5FF1}, //4937 #CJK UNIFIED IDEOGRAPH
    {0xA7D6, 0x5FEB}, //4938 #CJK UNIFIED IDEOGRAPH
    {0xA7D7, 0x5FF8}, //4939 #CJK UNIFIED IDEOGRAPH
    {0xA7D8, 0x5FEA}, //4940 #CJK UNIFIED IDEOGRAPH
    {0xA7D9, 0x6212}, //4941 #CJK UNIFIED IDEOGRAPH
    {0xA7DA, 0x6211}, //4942 #CJK UNIFIED IDEOGRAPH
    {0xA7DB, 0x6284}, //4943 #CJK UNIFIED IDEOGRAPH
    {0xA7DC, 0x6297}, //4944 #CJK UNIFIED IDEOGRAPH
    {0xA7DD, 0x6296}, //4945 #CJK UNIFIED IDEOGRAPH
    {0xA7DE, 0x6280}, //4946 #CJK UNIFIED IDEOGRAPH
    {0xA7DF, 0x6276}, //4947 #CJK UNIFIED IDEOGRAPH
    {0xA7E0, 0x6289}, //4948 #CJK UNIFIED IDEOGRAPH
    {0xA7E1, 0x626D}, //4949 #CJK UNIFIED IDEOGRAPH
    {0xA7E2, 0x628A}, //4950 #CJK UNIFIED IDEOGRAPH
    {0xA7E3, 0x627C}, //4951 #CJK UNIFIED IDEOGRAPH
    {0xA7E4, 0x627E}, //4952 #CJK UNIFIED IDEOGRAPH
    {0xA7E5, 0x6279}, //4953 #CJK UNIFIED IDEOGRAPH
    {0xA7E6, 0x6273}, //4954 #CJK UNIFIED IDEOGRAPH
    {0xA7E7, 0x6292}, //4955 #CJK UNIFIED IDEOGRAPH
    {0xA7E8, 0x626F}, //4956 #CJK UNIFIED IDEOGRAPH
    {0xA7E9, 0x6298}, //4957 #CJK UNIFIED IDEOGRAPH
    {0xA7EA, 0x626E}, //4958 #CJK UNIFIED IDEOGRAPH
    {0xA7EB, 0x6295}, //4959 #CJK UNIFIED IDEOGRAPH
    {0xA7EC, 0x6293}, //4960 #CJK UNIFIED IDEOGRAPH
    {0xA7ED, 0x6291}, //4961 #CJK UNIFIED IDEOGRAPH
    {0xA7EE, 0x6286}, //4962 #CJK UNIFIED IDEOGRAPH
    {0xA7EF, 0x6539}, //4963 #CJK UNIFIED IDEOGRAPH
    {0xA7F0, 0x653B}, //4964 #CJK UNIFIED IDEOGRAPH
    {0xA7F1, 0x6538}, //4965 #CJK UNIFIED IDEOGRAPH
    {0xA7F2, 0x65F1}, //4966 #CJK UNIFIED IDEOGRAPH
    {0xA7F3, 0x66F4}, //4967 #CJK UNIFIED IDEOGRAPH
    {0xA7F4, 0x675F}, //4968 #CJK UNIFIED IDEOGRAPH
    {0xA7F5, 0x674E}, //4969 #CJK UNIFIED IDEOGRAPH
    {0xA7F6, 0x674F}, //4970 #CJK UNIFIED IDEOGRAPH
    {0xA7F7, 0x6750}, //4971 #CJK UNIFIED IDEOGRAPH
    {0xA7F8, 0x6751}, //4972 #CJK UNIFIED IDEOGRAPH
    {0xA7F9, 0x675C}, //4973 #CJK UNIFIED IDEOGRAPH
    {0xA7FA, 0x6756}, //4974 #CJK UNIFIED IDEOGRAPH
    {0xA7FB, 0x675E}, //4975 #CJK UNIFIED IDEOGRAPH
    {0xA7FC, 0x6749}, //4976 #CJK UNIFIED IDEOGRAPH
    {0xA7FD, 0x6746}, //4977 #CJK UNIFIED IDEOGRAPH
    {0xA7FE, 0x6760}, //4978 #CJK UNIFIED IDEOGRAPH
    {0xA840, 0x6753}, //4979 #CJK UNIFIED IDEOGRAPH
    {0xA841, 0x6757}, //4980 #CJK UNIFIED IDEOGRAPH
    {0xA842, 0x6B65}, //4981 #CJK UNIFIED IDEOGRAPH
    {0xA843, 0x6BCF}, //4982 #CJK UNIFIED IDEOGRAPH
    {0xA844, 0x6C42}, //4983 #CJK UNIFIED IDEOGRAPH
    {0xA845, 0x6C5E}, //4984 #CJK UNIFIED IDEOGRAPH
    {0xA846, 0x6C99}, //4985 #CJK UNIFIED IDEOGRAPH
    {0xA847, 0x6C81}, //4986 #CJK UNIFIED IDEOGRAPH
    {0xA848, 0x6C88}, //4987 #CJK UNIFIED IDEOGRAPH
    {0xA849, 0x6C89}, //4988 #CJK UNIFIED IDEOGRAPH
    {0xA84A, 0x6C85}, //4989 #CJK UNIFIED IDEOGRAPH
    {0xA84B, 0x6C9B}, //4990 #CJK UNIFIED IDEOGRAPH
    {0xA84C, 0x6C6A}, //4991 #CJK UNIFIED IDEOGRAPH
    {0xA84D, 0x6C7A}, //4992 #CJK UNIFIED IDEOGRAPH
    {0xA84E, 0x6C90}, //4993 #CJK UNIFIED IDEOGRAPH
    {0xA84F, 0x6C70}, //4994 #CJK UNIFIED IDEOGRAPH
    {0xA850, 0x6C8C}, //4995 #CJK UNIFIED IDEOGRAPH
    {0xA851, 0x6C68}, //4996 #CJK UNIFIED IDEOGRAPH
    {0xA852, 0x6C96}, //4997 #CJK UNIFIED IDEOGRAPH
    {0xA853, 0x6C92}, //4998 #CJK UNIFIED IDEOGRAPH
    {0xA854, 0x6C7D}, //4999 #CJK UNIFIED IDEOGRAPH
    {0xA855, 0x6C83}, //5000 #CJK UNIFIED IDEOGRAPH
    {0xA856, 0x6C72}, //5001 #CJK UNIFIED IDEOGRAPH
    {0xA857, 0x6C7E}, //5002 #CJK UNIFIED IDEOGRAPH
    {0xA858, 0x6C74}, //5003 #CJK UNIFIED IDEOGRAPH
    {0xA859, 0x6C86}, //5004 #CJK UNIFIED IDEOGRAPH
    {0xA85A, 0x6C76}, //5005 #CJK UNIFIED IDEOGRAPH
    {0xA85B, 0x6C8D}, //5006 #CJK UNIFIED IDEOGRAPH
    {0xA85C, 0x6C94}, //5007 #CJK UNIFIED IDEOGRAPH
    {0xA85D, 0x6C98}, //5008 #CJK UNIFIED IDEOGRAPH
    {0xA85E, 0x6C82}, //5009 #CJK UNIFIED IDEOGRAPH
    {0xA85F, 0x7076}, //5010 #CJK UNIFIED IDEOGRAPH
    {0xA860, 0x707C}, //5011 #CJK UNIFIED IDEOGRAPH
    {0xA861, 0x707D}, //5012 #CJK UNIFIED IDEOGRAPH
    {0xA862, 0x7078}, //5013 #CJK UNIFIED IDEOGRAPH
    {0xA863, 0x7262}, //5014 #CJK UNIFIED IDEOGRAPH
    {0xA864, 0x7261}, //5015 #CJK UNIFIED IDEOGRAPH
    {0xA865, 0x7260}, //5016 #CJK UNIFIED IDEOGRAPH
    {0xA866, 0x72C4}, //5017 #CJK UNIFIED IDEOGRAPH
    {0xA867, 0x72C2}, //5018 #CJK UNIFIED IDEOGRAPH
    {0xA868, 0x7396}, //5019 #CJK UNIFIED IDEOGRAPH
    {0xA869, 0x752C}, //5020 #CJK UNIFIED IDEOGRAPH
    {0xA86A, 0x752B}, //5021 #CJK UNIFIED IDEOGRAPH
    {0xA86B, 0x7537}, //5022 #CJK UNIFIED IDEOGRAPH
    {0xA86C, 0x7538}, //5023 #CJK UNIFIED IDEOGRAPH
    {0xA86D, 0x7682}, //5024 #CJK UNIFIED IDEOGRAPH
    {0xA86E, 0x76EF}, //5025 #CJK UNIFIED IDEOGRAPH
    {0xA86F, 0x77E3}, //5026 #CJK UNIFIED IDEOGRAPH
    {0xA870, 0x79C1}, //5027 #CJK UNIFIED IDEOGRAPH
    {0xA871, 0x79C0}, //5028 #CJK UNIFIED IDEOGRAPH
    {0xA872, 0x79BF}, //5029 #CJK UNIFIED IDEOGRAPH
    {0xA873, 0x7A76}, //5030 #CJK UNIFIED IDEOGRAPH
    {0xA874, 0x7CFB}, //5031 #CJK UNIFIED IDEOGRAPH
    {0xA875, 0x7F55}, //5032 #CJK UNIFIED IDEOGRAPH
    {0xA876, 0x8096}, //5033 #CJK UNIFIED IDEOGRAPH
    {0xA877, 0x8093}, //5034 #CJK UNIFIED IDEOGRAPH
    {0xA878, 0x809D}, //5035 #CJK UNIFIED IDEOGRAPH
    {0xA879, 0x8098}, //5036 #CJK UNIFIED IDEOGRAPH
    {0xA87A, 0x809B}, //5037 #CJK UNIFIED IDEOGRAPH
    {0xA87B, 0x809A}, //5038 #CJK UNIFIED IDEOGRAPH
    {0xA87C, 0x80B2}, //5039 #CJK UNIFIED IDEOGRAPH
    {0xA87D, 0x826F}, //5040 #CJK UNIFIED IDEOGRAPH
    {0xA87E, 0x8292}, //5041 #CJK UNIFIED IDEOGRAPH
    {0xA8A1, 0x828B}, //5042 #CJK UNIFIED IDEOGRAPH
    {0xA8A2, 0x828D}, //5043 #CJK UNIFIED IDEOGRAPH
    {0xA8A3, 0x898B}, //5044 #CJK UNIFIED IDEOGRAPH
    {0xA8A4, 0x89D2}, //5045 #CJK UNIFIED IDEOGRAPH
    {0xA8A5, 0x8A00}, //5046 #CJK UNIFIED IDEOGRAPH
    {0xA8A6, 0x8C37}, //5047 #CJK UNIFIED IDEOGRAPH
    {0xA8A7, 0x8C46}, //5048 #CJK UNIFIED IDEOGRAPH
    {0xA8A8, 0x8C55}, //5049 #CJK UNIFIED IDEOGRAPH
    {0xA8A9, 0x8C9D}, //5050 #CJK UNIFIED IDEOGRAPH
    {0xA8AA, 0x8D64}, //5051 #CJK UNIFIED IDEOGRAPH
    {0xA8AB, 0x8D70}, //5052 #CJK UNIFIED IDEOGRAPH
    {0xA8AC, 0x8DB3}, //5053 #CJK UNIFIED IDEOGRAPH
    {0xA8AD, 0x8EAB}, //5054 #CJK UNIFIED IDEOGRAPH
    {0xA8AE, 0x8ECA}, //5055 #CJK UNIFIED IDEOGRAPH
    {0xA8AF, 0x8F9B}, //5056 #CJK UNIFIED IDEOGRAPH
    {0xA8B0, 0x8FB0}, //5057 #CJK UNIFIED IDEOGRAPH
    {0xA8B1, 0x8FC2}, //5058 #CJK UNIFIED IDEOGRAPH
    {0xA8B2, 0x8FC6}, //5059 #CJK UNIFIED IDEOGRAPH
    {0xA8B3, 0x8FC5}, //5060 #CJK UNIFIED IDEOGRAPH
    {0xA8B4, 0x8FC4}, //5061 #CJK UNIFIED IDEOGRAPH
    {0xA8B5, 0x5DE1}, //5062 #CJK UNIFIED IDEOGRAPH
    {0xA8B6, 0x9091}, //5063 #CJK UNIFIED IDEOGRAPH
    {0xA8B7, 0x90A2}, //5064 #CJK UNIFIED IDEOGRAPH
    {0xA8B8, 0x90AA}, //5065 #CJK UNIFIED IDEOGRAPH
    {0xA8B9, 0x90A6}, //5066 #CJK UNIFIED IDEOGRAPH
    {0xA8BA, 0x90A3}, //5067 #CJK UNIFIED IDEOGRAPH
    {0xA8BB, 0x9149}, //5068 #CJK UNIFIED IDEOGRAPH
    {0xA8BC, 0x91C6}, //5069 #CJK UNIFIED IDEOGRAPH
    {0xA8BD, 0x91CC}, //5070 #CJK UNIFIED IDEOGRAPH
    {0xA8BE, 0x9632}, //5071 #CJK UNIFIED IDEOGRAPH
    {0xA8BF, 0x962E}, //5072 #CJK UNIFIED IDEOGRAPH
    {0xA8C0, 0x9631}, //5073 #CJK UNIFIED IDEOGRAPH
    {0xA8C1, 0x962A}, //5074 #CJK UNIFIED IDEOGRAPH
    {0xA8C2, 0x962C}, //5075 #CJK UNIFIED IDEOGRAPH
    {0xA8C3, 0x4E26}, //5076 #CJK UNIFIED IDEOGRAPH
    {0xA8C4, 0x4E56}, //5077 #CJK UNIFIED IDEOGRAPH
    {0xA8C5, 0x4E73}, //5078 #CJK UNIFIED IDEOGRAPH
    {0xA8C6, 0x4E8B}, //5079 #CJK UNIFIED IDEOGRAPH
    {0xA8C7, 0x4E9B}, //5080 #CJK UNIFIED IDEOGRAPH
    {0xA8C8, 0x4E9E}, //5081 #CJK UNIFIED IDEOGRAPH
    {0xA8C9, 0x4EAB}, //5082 #CJK UNIFIED IDEOGRAPH
    {0xA8CA, 0x4EAC}, //5083 #CJK UNIFIED IDEOGRAPH
    {0xA8CB, 0x4F6F}, //5084 #CJK UNIFIED IDEOGRAPH
    {0xA8CC, 0x4F9D}, //5085 #CJK UNIFIED IDEOGRAPH
    {0xA8CD, 0x4F8D}, //5086 #CJK UNIFIED IDEOGRAPH
    {0xA8CE, 0x4F73}, //5087 #CJK UNIFIED IDEOGRAPH
    {0xA8CF, 0x4F7F}, //5088 #CJK UNIFIED IDEOGRAPH
    {0xA8D0, 0x4F6C}, //5089 #CJK UNIFIED IDEOGRAPH
    {0xA8D1, 0x4F9B}, //5090 #CJK UNIFIED IDEOGRAPH
    {0xA8D2, 0x4F8B}, //5091 #CJK UNIFIED IDEOGRAPH
    {0xA8D3, 0x4F86}, //5092 #CJK UNIFIED IDEOGRAPH
    {0xA8D4, 0x4F83}, //5093 #CJK UNIFIED IDEOGRAPH
    {0xA8D5, 0x4F70}, //5094 #CJK UNIFIED IDEOGRAPH
    {0xA8D6, 0x4F75}, //5095 #CJK UNIFIED IDEOGRAPH
    {0xA8D7, 0x4F88}, //5096 #CJK UNIFIED IDEOGRAPH
    {0xA8D8, 0x4F69}, //5097 #CJK UNIFIED IDEOGRAPH
    {0xA8D9, 0x4F7B}, //5098 #CJK UNIFIED IDEOGRAPH
    {0xA8DA, 0x4F96}, //5099 #CJK UNIFIED IDEOGRAPH
    {0xA8DB, 0x4F7E}, //5100 #CJK UNIFIED IDEOGRAPH
    {0xA8DC, 0x4F8F}, //5101 #CJK UNIFIED IDEOGRAPH
    {0xA8DD, 0x4F91}, //5102 #CJK UNIFIED IDEOGRAPH
    {0xA8DE, 0x4F7A}, //5103 #CJK UNIFIED IDEOGRAPH
    {0xA8DF, 0x5154}, //5104 #CJK UNIFIED IDEOGRAPH
    {0xA8E0, 0x5152}, //5105 #CJK UNIFIED IDEOGRAPH
    {0xA8E1, 0x5155}, //5106 #CJK UNIFIED IDEOGRAPH
    {0xA8E2, 0x5169}, //5107 #CJK UNIFIED IDEOGRAPH
    {0xA8E3, 0x5177}, //5108 #CJK UNIFIED IDEOGRAPH
    {0xA8E4, 0x5176}, //5109 #CJK UNIFIED IDEOGRAPH
    {0xA8E5, 0x5178}, //5110 #CJK UNIFIED IDEOGRAPH
    {0xA8E6, 0x51BD}, //5111 #CJK UNIFIED IDEOGRAPH
    {0xA8E7, 0x51FD}, //5112 #CJK UNIFIED IDEOGRAPH
    {0xA8E8, 0x523B}, //5113 #CJK UNIFIED IDEOGRAPH
    {0xA8E9, 0x5238}, //5114 #CJK UNIFIED IDEOGRAPH
    {0xA8EA, 0x5237}, //5115 #CJK UNIFIED IDEOGRAPH
    {0xA8EB, 0x523A}, //5116 #CJK UNIFIED IDEOGRAPH
    {0xA8EC, 0x5230}, //5117 #CJK UNIFIED IDEOGRAPH
    {0xA8ED, 0x522E}, //5118 #CJK UNIFIED IDEOGRAPH
    {0xA8EE, 0x5236}, //5119 #CJK UNIFIED IDEOGRAPH
    {0xA8EF, 0x5241}, //5120 #CJK UNIFIED IDEOGRAPH
    {0xA8F0, 0x52BE}, //5121 #CJK UNIFIED IDEOGRAPH
    {0xA8F1, 0x52BB}, //5122 #CJK UNIFIED IDEOGRAPH
    {0xA8F2, 0x5352}, //5123 #CJK UNIFIED IDEOGRAPH
    {0xA8F3, 0x5354}, //5124 #CJK UNIFIED IDEOGRAPH
    {0xA8F4, 0x5353}, //5125 #CJK UNIFIED IDEOGRAPH
    {0xA8F5, 0x5351}, //5126 #CJK UNIFIED IDEOGRAPH
    {0xA8F6, 0x5366}, //5127 #CJK UNIFIED IDEOGRAPH
    {0xA8F7, 0x5377}, //5128 #CJK UNIFIED IDEOGRAPH
    {0xA8F8, 0x5378}, //5129 #CJK UNIFIED IDEOGRAPH
    {0xA8F9, 0x5379}, //5130 #CJK UNIFIED IDEOGRAPH
    {0xA8FA, 0x53D6}, //5131 #CJK UNIFIED IDEOGRAPH
    {0xA8FB, 0x53D4}, //5132 #CJK UNIFIED IDEOGRAPH
    {0xA8FC, 0x53D7}, //5133 #CJK UNIFIED IDEOGRAPH
    {0xA8FD, 0x5473}, //5134 #CJK UNIFIED IDEOGRAPH
    {0xA8FE, 0x5475}, //5135 #CJK UNIFIED IDEOGRAPH
    {0xA940, 0x5496}, //5136 #CJK UNIFIED IDEOGRAPH
    {0xA941, 0x5478}, //5137 #CJK UNIFIED IDEOGRAPH
    {0xA942, 0x5495}, //5138 #CJK UNIFIED IDEOGRAPH
    {0xA943, 0x5480}, //5139 #CJK UNIFIED IDEOGRAPH
    {0xA944, 0x547B}, //5140 #CJK UNIFIED IDEOGRAPH
    {0xA945, 0x5477}, //5141 #CJK UNIFIED IDEOGRAPH
    {0xA946, 0x5484}, //5142 #CJK UNIFIED IDEOGRAPH
    {0xA947, 0x5492}, //5143 #CJK UNIFIED IDEOGRAPH
    {0xA948, 0x5486}, //5144 #CJK UNIFIED IDEOGRAPH
    {0xA949, 0x547C}, //5145 #CJK UNIFIED IDEOGRAPH
    {0xA94A, 0x5490}, //5146 #CJK UNIFIED IDEOGRAPH
    {0xA94B, 0x5471}, //5147 #CJK UNIFIED IDEOGRAPH
    {0xA94C, 0x5476}, //5148 #CJK UNIFIED IDEOGRAPH
    {0xA94D, 0x548C}, //5149 #CJK UNIFIED IDEOGRAPH
    {0xA94E, 0x549A}, //5150 #CJK UNIFIED IDEOGRAPH
    {0xA94F, 0x5462}, //5151 #CJK UNIFIED IDEOGRAPH
    {0xA950, 0x5468}, //5152 #CJK UNIFIED IDEOGRAPH
    {0xA951, 0x548B}, //5153 #CJK UNIFIED IDEOGRAPH
    {0xA952, 0x547D}, //5154 #CJK UNIFIED IDEOGRAPH
    {0xA953, 0x548E}, //5155 #CJK UNIFIED IDEOGRAPH
    {0xA954, 0x56FA}, //5156 #CJK UNIFIED IDEOGRAPH
    {0xA955, 0x5783}, //5157 #CJK UNIFIED IDEOGRAPH
    {0xA956, 0x5777}, //5158 #CJK UNIFIED IDEOGRAPH
    {0xA957, 0x576A}, //5159 #CJK UNIFIED IDEOGRAPH
    {0xA958, 0x5769}, //5160 #CJK UNIFIED IDEOGRAPH
    {0xA959, 0x5761}, //5161 #CJK UNIFIED IDEOGRAPH
    {0xA95A, 0x5766}, //5162 #CJK UNIFIED IDEOGRAPH
    {0xA95B, 0x5764}, //5163 #CJK UNIFIED IDEOGRAPH
    {0xA95C, 0x577C}, //5164 #CJK UNIFIED IDEOGRAPH
    {0xA95D, 0x591C}, //5165 #CJK UNIFIED IDEOGRAPH
    {0xA95E, 0x5949}, //5166 #CJK UNIFIED IDEOGRAPH
    {0xA95F, 0x5947}, //5167 #CJK UNIFIED IDEOGRAPH
    {0xA960, 0x5948}, //5168 #CJK UNIFIED IDEOGRAPH
    {0xA961, 0x5944}, //5169 #CJK UNIFIED IDEOGRAPH
    {0xA962, 0x5954}, //5170 #CJK UNIFIED IDEOGRAPH
    {0xA963, 0x59BE}, //5171 #CJK UNIFIED IDEOGRAPH
    {0xA964, 0x59BB}, //5172 #CJK UNIFIED IDEOGRAPH
    {0xA965, 0x59D4}, //5173 #CJK UNIFIED IDEOGRAPH
    {0xA966, 0x59B9}, //5174 #CJK UNIFIED IDEOGRAPH
    {0xA967, 0x59AE}, //5175 #CJK UNIFIED IDEOGRAPH
    {0xA968, 0x59D1}, //5176 #CJK UNIFIED IDEOGRAPH
    {0xA969, 0x59C6}, //5177 #CJK UNIFIED IDEOGRAPH
    {0xA96A, 0x59D0}, //5178 #CJK UNIFIED IDEOGRAPH
    {0xA96B, 0x59CD}, //5179 #CJK UNIFIED IDEOGRAPH
    {0xA96C, 0x59CB}, //5180 #CJK UNIFIED IDEOGRAPH
    {0xA96D, 0x59D3}, //5181 #CJK UNIFIED IDEOGRAPH
    {0xA96E, 0x59CA}, //5182 #CJK UNIFIED IDEOGRAPH
    {0xA96F, 0x59AF}, //5183 #CJK UNIFIED IDEOGRAPH
    {0xA970, 0x59B3}, //5184 #CJK UNIFIED IDEOGRAPH
    {0xA971, 0x59D2}, //5185 #CJK UNIFIED IDEOGRAPH
    {0xA972, 0x59C5}, //5186 #CJK UNIFIED IDEOGRAPH
    {0xA973, 0x5B5F}, //5187 #CJK UNIFIED IDEOGRAPH
    {0xA974, 0x5B64}, //5188 #CJK UNIFIED IDEOGRAPH
    {0xA975, 0x5B63}, //5189 #CJK UNIFIED IDEOGRAPH
    {0xA976, 0x5B97}, //5190 #CJK UNIFIED IDEOGRAPH
    {0xA977, 0x5B9A}, //5191 #CJK UNIFIED IDEOGRAPH
    {0xA978, 0x5B98}, //5192 #CJK UNIFIED IDEOGRAPH
    {0xA979, 0x5B9C}, //5193 #CJK UNIFIED IDEOGRAPH
    {0xA97A, 0x5B99}, //5194 #CJK UNIFIED IDEOGRAPH
    {0xA97B, 0x5B9B}, //5195 #CJK UNIFIED IDEOGRAPH
    {0xA97C, 0x5C1A}, //5196 #CJK UNIFIED IDEOGRAPH
    {0xA97D, 0x5C48}, //5197 #CJK UNIFIED IDEOGRAPH
    {0xA97E, 0x5C45}, //5198 #CJK UNIFIED IDEOGRAPH
    {0xA9A1, 0x5C46}, //5199 #CJK UNIFIED IDEOGRAPH
    {0xA9A2, 0x5CB7}, //5200 #CJK UNIFIED IDEOGRAPH
    {0xA9A3, 0x5CA1}, //5201 #CJK UNIFIED IDEOGRAPH
    {0xA9A4, 0x5CB8}, //5202 #CJK UNIFIED IDEOGRAPH
    {0xA9A5, 0x5CA9}, //5203 #CJK UNIFIED IDEOGRAPH
    {0xA9A6, 0x5CAB}, //5204 #CJK UNIFIED IDEOGRAPH
    {0xA9A7, 0x5CB1}, //5205 #CJK UNIFIED IDEOGRAPH
    {0xA9A8, 0x5CB3}, //5206 #CJK UNIFIED IDEOGRAPH
    {0xA9A9, 0x5E18}, //5207 #CJK UNIFIED IDEOGRAPH
    {0xA9AA, 0x5E1A}, //5208 #CJK UNIFIED IDEOGRAPH
    {0xA9AB, 0x5E16}, //5209 #CJK UNIFIED IDEOGRAPH
    {0xA9AC, 0x5E15}, //5210 #CJK UNIFIED IDEOGRAPH
    {0xA9AD, 0x5E1B}, //5211 #CJK UNIFIED IDEOGRAPH
    {0xA9AE, 0x5E11}, //5212 #CJK UNIFIED IDEOGRAPH
    {0xA9AF, 0x5E78}, //5213 #CJK UNIFIED IDEOGRAPH
    {0xA9B0, 0x5E9A}, //5214 #CJK UNIFIED IDEOGRAPH
    {0xA9B1, 0x5E97}, //5215 #CJK UNIFIED IDEOGRAPH
    {0xA9B2, 0x5E9C}, //5216 #CJK UNIFIED IDEOGRAPH
    {0xA9B3, 0x5E95}, //5217 #CJK UNIFIED IDEOGRAPH
    {0xA9B4, 0x5E96}, //5218 #CJK UNIFIED IDEOGRAPH
    {0xA9B5, 0x5EF6}, //5219 #CJK UNIFIED IDEOGRAPH
    {0xA9B6, 0x5F26}, //5220 #CJK UNIFIED IDEOGRAPH
    {0xA9B7, 0x5F27}, //5221 #CJK UNIFIED IDEOGRAPH
    {0xA9B8, 0x5F29}, //5222 #CJK UNIFIED IDEOGRAPH
    {0xA9B9, 0x5F80}, //5223 #CJK UNIFIED IDEOGRAPH
    {0xA9BA, 0x5F81}, //5224 #CJK UNIFIED IDEOGRAPH
    {0xA9BB, 0x5F7F}, //5225 #CJK UNIFIED IDEOGRAPH
    {0xA9BC, 0x5F7C}, //5226 #CJK UNIFIED IDEOGRAPH
    {0xA9BD, 0x5FDD}, //5227 #CJK UNIFIED IDEOGRAPH
    {0xA9BE, 0x5FE0}, //5228 #CJK UNIFIED IDEOGRAPH
    {0xA9BF, 0x5FFD}, //5229 #CJK UNIFIED IDEOGRAPH
    {0xA9C0, 0x5FF5}, //5230 #CJK UNIFIED IDEOGRAPH
    {0xA9C1, 0x5FFF}, //5231 #CJK UNIFIED IDEOGRAPH
    {0xA9C2, 0x600F}, //5232 #CJK UNIFIED IDEOGRAPH
    {0xA9C3, 0x6014}, //5233 #CJK UNIFIED IDEOGRAPH
    {0xA9C4, 0x602F}, //5234 #CJK UNIFIED IDEOGRAPH
    {0xA9C5, 0x6035}, //5235 #CJK UNIFIED IDEOGRAPH
    {0xA9C6, 0x6016}, //5236 #CJK UNIFIED IDEOGRAPH
    {0xA9C7, 0x602A}, //5237 #CJK UNIFIED IDEOGRAPH
    {0xA9C8, 0x6015}, //5238 #CJK UNIFIED IDEOGRAPH
    {0xA9C9, 0x6021}, //5239 #CJK UNIFIED IDEOGRAPH
    {0xA9CA, 0x6027}, //5240 #CJK UNIFIED IDEOGRAPH
    {0xA9CB, 0x6029}, //5241 #CJK UNIFIED IDEOGRAPH
    {0xA9CC, 0x602B}, //5242 #CJK UNIFIED IDEOGRAPH
    {0xA9CD, 0x601B}, //5243 #CJK UNIFIED IDEOGRAPH
    {0xA9CE, 0x6216}, //5244 #CJK UNIFIED IDEOGRAPH
    {0xA9CF, 0x6215}, //5245 #CJK UNIFIED IDEOGRAPH
    {0xA9D0, 0x623F}, //5246 #CJK UNIFIED IDEOGRAPH
    {0xA9D1, 0x623E}, //5247 #CJK UNIFIED IDEOGRAPH
    {0xA9D2, 0x6240}, //5248 #CJK UNIFIED IDEOGRAPH
    {0xA9D3, 0x627F}, //5249 #CJK UNIFIED IDEOGRAPH
    {0xA9D4, 0x62C9}, //5250 #CJK UNIFIED IDEOGRAPH
    {0xA9D5, 0x62CC}, //5251 #CJK UNIFIED IDEOGRAPH
    {0xA9D6, 0x62C4}, //5252 #CJK UNIFIED IDEOGRAPH
    {0xA9D7, 0x62BF}, //5253 #CJK UNIFIED IDEOGRAPH
    {0xA9D8, 0x62C2}, //5254 #CJK UNIFIED IDEOGRAPH
    {0xA9D9, 0x62B9}, //5255 #CJK UNIFIED IDEOGRAPH
    {0xA9DA, 0x62D2}, //5256 #CJK UNIFIED IDEOGRAPH
    {0xA9DB, 0x62DB}, //5257 #CJK UNIFIED IDEOGRAPH
    {0xA9DC, 0x62AB}, //5258 #CJK UNIFIED IDEOGRAPH
    {0xA9DD, 0x62D3}, //5259 #CJK UNIFIED IDEOGRAPH
    {0xA9DE, 0x62D4}, //5260 #CJK UNIFIED IDEOGRAPH
    {0xA9DF, 0x62CB}, //5261 #CJK UNIFIED IDEOGRAPH
    {0xA9E0, 0x62C8}, //5262 #CJK UNIFIED IDEOGRAPH
    {0xA9E1, 0x62A8}, //5263 #CJK UNIFIED IDEOGRAPH
    {0xA9E2, 0x62BD}, //5264 #CJK UNIFIED IDEOGRAPH
    {0xA9E3, 0x62BC}, //5265 #CJK UNIFIED IDEOGRAPH
    {0xA9E4, 0x62D0}, //5266 #CJK UNIFIED IDEOGRAPH
    {0xA9E5, 0x62D9}, //5267 #CJK UNIFIED IDEOGRAPH
    {0xA9E6, 0x62C7}, //5268 #CJK UNIFIED IDEOGRAPH
    {0xA9E7, 0x62CD}, //5269 #CJK UNIFIED IDEOGRAPH
    {0xA9E8, 0x62B5}, //5270 #CJK UNIFIED IDEOGRAPH
    {0xA9E9, 0x62DA}, //5271 #CJK UNIFIED IDEOGRAPH
    {0xA9EA, 0x62B1}, //5272 #CJK UNIFIED IDEOGRAPH
    {0xA9EB, 0x62D8}, //5273 #CJK UNIFIED IDEOGRAPH
    {0xA9EC, 0x62D6}, //5274 #CJK UNIFIED IDEOGRAPH
    {0xA9ED, 0x62D7}, //5275 #CJK UNIFIED IDEOGRAPH
    {0xA9EE, 0x62C6}, //5276 #CJK UNIFIED IDEOGRAPH
    {0xA9EF, 0x62AC}, //5277 #CJK UNIFIED IDEOGRAPH
    {0xA9F0, 0x62CE}, //5278 #CJK UNIFIED IDEOGRAPH
    {0xA9F1, 0x653E}, //5279 #CJK UNIFIED IDEOGRAPH
    {0xA9F2, 0x65A7}, //5280 #CJK UNIFIED IDEOGRAPH
    {0xA9F3, 0x65BC}, //5281 #CJK UNIFIED IDEOGRAPH
    {0xA9F4, 0x65FA}, //5282 #CJK UNIFIED IDEOGRAPH
    {0xA9F5, 0x6614}, //5283 #CJK UNIFIED IDEOGRAPH
    {0xA9F6, 0x6613}, //5284 #CJK UNIFIED IDEOGRAPH
    {0xA9F7, 0x660C}, //5285 #CJK UNIFIED IDEOGRAPH
    {0xA9F8, 0x6606}, //5286 #CJK UNIFIED IDEOGRAPH
    {0xA9F9, 0x6602}, //5287 #CJK UNIFIED IDEOGRAPH
    {0xA9FA, 0x660E}, //5288 #CJK UNIFIED IDEOGRAPH
    {0xA9FB, 0x6600}, //5289 #CJK UNIFIED IDEOGRAPH
    {0xA9FC, 0x660F}, //5290 #CJK UNIFIED IDEOGRAPH
    {0xA9FD, 0x6615}, //5291 #CJK UNIFIED IDEOGRAPH
    {0xA9FE, 0x660A}, //5292 #CJK UNIFIED IDEOGRAPH
    {0xAA40, 0x6607}, //5293 #CJK UNIFIED IDEOGRAPH
    {0xAA41, 0x670D}, //5294 #CJK UNIFIED IDEOGRAPH
    {0xAA42, 0x670B}, //5295 #CJK UNIFIED IDEOGRAPH
    {0xAA43, 0x676D}, //5296 #CJK UNIFIED IDEOGRAPH
    {0xAA44, 0x678B}, //5297 #CJK UNIFIED IDEOGRAPH
    {0xAA45, 0x6795}, //5298 #CJK UNIFIED IDEOGRAPH
    {0xAA46, 0x6771}, //5299 #CJK UNIFIED IDEOGRAPH
    {0xAA47, 0x679C}, //5300 #CJK UNIFIED IDEOGRAPH
    {0xAA48, 0x6773}, //5301 #CJK UNIFIED IDEOGRAPH
    {0xAA49, 0x6777}, //5302 #CJK UNIFIED IDEOGRAPH
    {0xAA4A, 0x6787}, //5303 #CJK UNIFIED IDEOGRAPH
    {0xAA4B, 0x679D}, //5304 #CJK UNIFIED IDEOGRAPH
    {0xAA4C, 0x6797}, //5305 #CJK UNIFIED IDEOGRAPH
    {0xAA4D, 0x676F}, //5306 #CJK UNIFIED IDEOGRAPH
    {0xAA4E, 0x6770}, //5307 #CJK UNIFIED IDEOGRAPH
    {0xAA4F, 0x677F}, //5308 #CJK UNIFIED IDEOGRAPH
    {0xAA50, 0x6789}, //5309 #CJK UNIFIED IDEOGRAPH
    {0xAA51, 0x677E}, //5310 #CJK UNIFIED IDEOGRAPH
    {0xAA52, 0x6790}, //5311 #CJK UNIFIED IDEOGRAPH
    {0xAA53, 0x6775}, //5312 #CJK UNIFIED IDEOGRAPH
    {0xAA54, 0x679A}, //5313 #CJK UNIFIED IDEOGRAPH
    {0xAA55, 0x6793}, //5314 #CJK UNIFIED IDEOGRAPH
    {0xAA56, 0x677C}, //5315 #CJK UNIFIED IDEOGRAPH
    {0xAA57, 0x676A}, //5316 #CJK UNIFIED IDEOGRAPH
    {0xAA58, 0x6772}, //5317 #CJK UNIFIED IDEOGRAPH
    {0xAA59, 0x6B23}, //5318 #CJK UNIFIED IDEOGRAPH
    {0xAA5A, 0x6B66}, //5319 #CJK UNIFIED IDEOGRAPH
    {0xAA5B, 0x6B67}, //5320 #CJK UNIFIED IDEOGRAPH
    {0xAA5C, 0x6B7F}, //5321 #CJK UNIFIED IDEOGRAPH
    {0xAA5D, 0x6C13}, //5322 #CJK UNIFIED IDEOGRAPH
    {0xAA5E, 0x6C1B}, //5323 #CJK UNIFIED IDEOGRAPH
    {0xAA5F, 0x6CE3}, //5324 #CJK UNIFIED IDEOGRAPH
    {0xAA60, 0x6CE8}, //5325 #CJK UNIFIED IDEOGRAPH
    {0xAA61, 0x6CF3}, //5326 #CJK UNIFIED IDEOGRAPH
    {0xAA62, 0x6CB1}, //5327 #CJK UNIFIED IDEOGRAPH
    {0xAA63, 0x6CCC}, //5328 #CJK UNIFIED IDEOGRAPH
    {0xAA64, 0x6CE5}, //5329 #CJK UNIFIED IDEOGRAPH
    {0xAA65, 0x6CB3}, //5330 #CJK UNIFIED IDEOGRAPH
    {0xAA66, 0x6CBD}, //5331 #CJK UNIFIED IDEOGRAPH
    {0xAA67, 0x6CBE}, //5332 #CJK UNIFIED IDEOGRAPH
    {0xAA68, 0x6CBC}, //5333 #CJK UNIFIED IDEOGRAPH
    {0xAA69, 0x6CE2}, //5334 #CJK UNIFIED IDEOGRAPH
    {0xAA6A, 0x6CAB}, //5335 #CJK UNIFIED IDEOGRAPH
    {0xAA6B, 0x6CD5}, //5336 #CJK UNIFIED IDEOGRAPH
    {0xAA6C, 0x6CD3}, //5337 #CJK UNIFIED IDEOGRAPH
    {0xAA6D, 0x6CB8}, //5338 #CJK UNIFIED IDEOGRAPH
    {0xAA6E, 0x6CC4}, //5339 #CJK UNIFIED IDEOGRAPH
    {0xAA6F, 0x6CB9}, //5340 #CJK UNIFIED IDEOGRAPH
    {0xAA70, 0x6CC1}, //5341 #CJK UNIFIED IDEOGRAPH
    {0xAA71, 0x6CAE}, //5342 #CJK UNIFIED IDEOGRAPH
    {0xAA72, 0x6CD7}, //5343 #CJK UNIFIED IDEOGRAPH
    {0xAA73, 0x6CC5}, //5344 #CJK UNIFIED IDEOGRAPH
    {0xAA74, 0x6CF1}, //5345 #CJK UNIFIED IDEOGRAPH
    {0xAA75, 0x6CBF}, //5346 #CJK UNIFIED IDEOGRAPH
    {0xAA76, 0x6CBB}, //5347 #CJK UNIFIED IDEOGRAPH
    {0xAA77, 0x6CE1}, //5348 #CJK UNIFIED IDEOGRAPH
    {0xAA78, 0x6CDB}, //5349 #CJK UNIFIED IDEOGRAPH
    {0xAA79, 0x6CCA}, //5350 #CJK UNIFIED IDEOGRAPH
    {0xAA7A, 0x6CAC}, //5351 #CJK UNIFIED IDEOGRAPH
    {0xAA7B, 0x6CEF}, //5352 #CJK UNIFIED IDEOGRAPH
    {0xAA7C, 0x6CDC}, //5353 #CJK UNIFIED IDEOGRAPH
    {0xAA7D, 0x6CD6}, //5354 #CJK UNIFIED IDEOGRAPH
    {0xAA7E, 0x6CE0}, //5355 #CJK UNIFIED IDEOGRAPH
    {0xAAA1, 0x7095}, //5356 #CJK UNIFIED IDEOGRAPH
    {0xAAA2, 0x708E}, //5357 #CJK UNIFIED IDEOGRAPH
    {0xAAA3, 0x7092}, //5358 #CJK UNIFIED IDEOGRAPH
    {0xAAA4, 0x708A}, //5359 #CJK UNIFIED IDEOGRAPH
    {0xAAA5, 0x7099}, //5360 #CJK UNIFIED IDEOGRAPH
    {0xAAA6, 0x722C}, //5361 #CJK UNIFIED IDEOGRAPH
    {0xAAA7, 0x722D}, //5362 #CJK UNIFIED IDEOGRAPH
    {0xAAA8, 0x7238}, //5363 #CJK UNIFIED IDEOGRAPH
    {0xAAA9, 0x7248}, //5364 #CJK UNIFIED IDEOGRAPH
    {0xAAAA, 0x7267}, //5365 #CJK UNIFIED IDEOGRAPH
    {0xAAAB, 0x7269}, //5366 #CJK UNIFIED IDEOGRAPH
    {0xAAAC, 0x72C0}, //5367 #CJK UNIFIED IDEOGRAPH
    {0xAAAD, 0x72CE}, //5368 #CJK UNIFIED IDEOGRAPH
    {0xAAAE, 0x72D9}, //5369 #CJK UNIFIED IDEOGRAPH
    {0xAAAF, 0x72D7}, //5370 #CJK UNIFIED IDEOGRAPH
    {0xAAB0, 0x72D0}, //5371 #CJK UNIFIED IDEOGRAPH
    {0xAAB1, 0x73A9}, //5372 #CJK UNIFIED IDEOGRAPH
    {0xAAB2, 0x73A8}, //5373 #CJK UNIFIED IDEOGRAPH
    {0xAAB3, 0x739F}, //5374 #CJK UNIFIED IDEOGRAPH
    {0xAAB4, 0x73AB}, //5375 #CJK UNIFIED IDEOGRAPH
    {0xAAB5, 0x73A5}, //5376 #CJK UNIFIED IDEOGRAPH
    {0xAAB6, 0x753D}, //5377 #CJK UNIFIED IDEOGRAPH
    {0xAAB7, 0x759D}, //5378 #CJK UNIFIED IDEOGRAPH
    {0xAAB8, 0x7599}, //5379 #CJK UNIFIED IDEOGRAPH
    {0xAAB9, 0x759A}, //5380 #CJK UNIFIED IDEOGRAPH
    {0xAABA, 0x7684}, //5381 #CJK UNIFIED IDEOGRAPH
    {0xAABB, 0x76C2}, //5382 #CJK UNIFIED IDEOGRAPH
    {0xAABC, 0x76F2}, //5383 #CJK UNIFIED IDEOGRAPH
    {0xAABD, 0x76F4}, //5384 #CJK UNIFIED IDEOGRAPH
    {0xAABE, 0x77E5}, //5385 #CJK UNIFIED IDEOGRAPH
    {0xAABF, 0x77FD}, //5386 #CJK UNIFIED IDEOGRAPH
    {0xAAC0, 0x793E}, //5387 #CJK UNIFIED IDEOGRAPH
    {0xAAC1, 0x7940}, //5388 #CJK UNIFIED IDEOGRAPH
    {0xAAC2, 0x7941}, //5389 #CJK UNIFIED IDEOGRAPH
    {0xAAC3, 0x79C9}, //5390 #CJK UNIFIED IDEOGRAPH
    {0xAAC4, 0x79C8}, //5391 #CJK UNIFIED IDEOGRAPH
    {0xAAC5, 0x7A7A}, //5392 #CJK UNIFIED IDEOGRAPH
    {0xAAC6, 0x7A79}, //5393 #CJK UNIFIED IDEOGRAPH
    {0xAAC7, 0x7AFA}, //5394 #CJK UNIFIED IDEOGRAPH
    {0xAAC8, 0x7CFE}, //5395 #CJK UNIFIED IDEOGRAPH
    {0xAAC9, 0x7F54}, //5396 #CJK UNIFIED IDEOGRAPH
    {0xAACA, 0x7F8C}, //5397 #CJK UNIFIED IDEOGRAPH
    {0xAACB, 0x7F8B}, //5398 #CJK UNIFIED IDEOGRAPH
    {0xAACC, 0x8005}, //5399 #CJK UNIFIED IDEOGRAPH
    {0xAACD, 0x80BA}, //5400 #CJK UNIFIED IDEOGRAPH
    {0xAACE, 0x80A5}, //5401 #CJK UNIFIED IDEOGRAPH
    {0xAACF, 0x80A2}, //5402 #CJK UNIFIED IDEOGRAPH
    {0xAAD0, 0x80B1}, //5403 #CJK UNIFIED IDEOGRAPH
    {0xAAD1, 0x80A1}, //5404 #CJK UNIFIED IDEOGRAPH
    {0xAAD2, 0x80AB}, //5405 #CJK UNIFIED IDEOGRAPH
    {0xAAD3, 0x80A9}, //5406 #CJK UNIFIED IDEOGRAPH
    {0xAAD4, 0x80B4}, //5407 #CJK UNIFIED IDEOGRAPH
    {0xAAD5, 0x80AA}, //5408 #CJK UNIFIED IDEOGRAPH
    {0xAAD6, 0x80AF}, //5409 #CJK UNIFIED IDEOGRAPH
    {0xAAD7, 0x81E5}, //5410 #CJK UNIFIED IDEOGRAPH
    {0xAAD8, 0x81FE}, //5411 #CJK UNIFIED IDEOGRAPH
    {0xAAD9, 0x820D}, //5412 #CJK UNIFIED IDEOGRAPH
    {0xAADA, 0x82B3}, //5413 #CJK UNIFIED IDEOGRAPH
    {0xAADB, 0x829D}, //5414 #CJK UNIFIED IDEOGRAPH
    {0xAADC, 0x8299}, //5415 #CJK UNIFIED IDEOGRAPH
    {0xAADD, 0x82AD}, //5416 #CJK UNIFIED IDEOGRAPH
    {0xAADE, 0x82BD}, //5417 #CJK UNIFIED IDEOGRAPH
    {0xAADF, 0x829F}, //5418 #CJK UNIFIED IDEOGRAPH
    {0xAAE0, 0x82B9}, //5419 #CJK UNIFIED IDEOGRAPH
    {0xAAE1, 0x82B1}, //5420 #CJK UNIFIED IDEOGRAPH
    {0xAAE2, 0x82AC}, //5421 #CJK UNIFIED IDEOGRAPH
    {0xAAE3, 0x82A5}, //5422 #CJK UNIFIED IDEOGRAPH
    {0xAAE4, 0x82AF}, //5423 #CJK UNIFIED IDEOGRAPH
    {0xAAE5, 0x82B8}, //5424 #CJK UNIFIED IDEOGRAPH
    {0xAAE6, 0x82A3}, //5425 #CJK UNIFIED IDEOGRAPH
    {0xAAE7, 0x82B0}, //5426 #CJK UNIFIED IDEOGRAPH
    {0xAAE8, 0x82BE}, //5427 #CJK UNIFIED IDEOGRAPH
    {0xAAE9, 0x82B7}, //5428 #CJK UNIFIED IDEOGRAPH
    {0xAAEA, 0x864E}, //5429 #CJK UNIFIED IDEOGRAPH
    {0xAAEB, 0x8671}, //5430 #CJK UNIFIED IDEOGRAPH
    {0xAAEC, 0x521D}, //5431 #CJK UNIFIED IDEOGRAPH
    {0xAAED, 0x8868}, //5432 #CJK UNIFIED IDEOGRAPH
    {0xAAEE, 0x8ECB}, //5433 #CJK UNIFIED IDEOGRAPH
    {0xAAEF, 0x8FCE}, //5434 #CJK UNIFIED IDEOGRAPH
    {0xAAF0, 0x8FD4}, //5435 #CJK UNIFIED IDEOGRAPH
    {0xAAF1, 0x8FD1}, //5436 #CJK UNIFIED IDEOGRAPH
    {0xAAF2, 0x90B5}, //5437 #CJK UNIFIED IDEOGRAPH
    {0xAAF3, 0x90B8}, //5438 #CJK UNIFIED IDEOGRAPH
    {0xAAF4, 0x90B1}, //5439 #CJK UNIFIED IDEOGRAPH
    {0xAAF5, 0x90B6}, //5440 #CJK UNIFIED IDEOGRAPH
    {0xAAF6, 0x91C7}, //5441 #CJK UNIFIED IDEOGRAPH
    {0xAAF7, 0x91D1}, //5442 #CJK UNIFIED IDEOGRAPH
    {0xAAF8, 0x9577}, //5443 #CJK UNIFIED IDEOGRAPH
    {0xAAF9, 0x9580}, //5444 #CJK UNIFIED IDEOGRAPH
    {0xAAFA, 0x961C}, //5445 #CJK UNIFIED IDEOGRAPH
    {0xAAFB, 0x9640}, //5446 #CJK UNIFIED IDEOGRAPH
    {0xAAFC, 0x963F}, //5447 #CJK UNIFIED IDEOGRAPH
    {0xAAFD, 0x963B}, //5448 #CJK UNIFIED IDEOGRAPH
    {0xAAFE, 0x9644}, //5449 #CJK UNIFIED IDEOGRAPH
    {0xAB40, 0x9642}, //5450 #CJK UNIFIED IDEOGRAPH
    {0xAB41, 0x96B9}, //5451 #CJK UNIFIED IDEOGRAPH
    {0xAB42, 0x96E8}, //5452 #CJK UNIFIED IDEOGRAPH
    {0xAB43, 0x9752}, //5453 #CJK UNIFIED IDEOGRAPH
    {0xAB44, 0x975E}, //5454 #CJK UNIFIED IDEOGRAPH
    {0xAB45, 0x4E9F}, //5455 #CJK UNIFIED IDEOGRAPH
    {0xAB46, 0x4EAD}, //5456 #CJK UNIFIED IDEOGRAPH
    {0xAB47, 0x4EAE}, //5457 #CJK UNIFIED IDEOGRAPH
    {0xAB48, 0x4FE1}, //5458 #CJK UNIFIED IDEOGRAPH
    {0xAB49, 0x4FB5}, //5459 #CJK UNIFIED IDEOGRAPH
    {0xAB4A, 0x4FAF}, //5460 #CJK UNIFIED IDEOGRAPH
    {0xAB4B, 0x4FBF}, //5461 #CJK UNIFIED IDEOGRAPH
    {0xAB4C, 0x4FE0}, //5462 #CJK UNIFIED IDEOGRAPH
    {0xAB4D, 0x4FD1}, //5463 #CJK UNIFIED IDEOGRAPH
    {0xAB4E, 0x4FCF}, //5464 #CJK UNIFIED IDEOGRAPH
    {0xAB4F, 0x4FDD}, //5465 #CJK UNIFIED IDEOGRAPH
    {0xAB50, 0x4FC3}, //5466 #CJK UNIFIED IDEOGRAPH
    {0xAB51, 0x4FB6}, //5467 #CJK UNIFIED IDEOGRAPH
    {0xAB52, 0x4FD8}, //5468 #CJK UNIFIED IDEOGRAPH
    {0xAB53, 0x4FDF}, //5469 #CJK UNIFIED IDEOGRAPH
    {0xAB54, 0x4FCA}, //5470 #CJK UNIFIED IDEOGRAPH
    {0xAB55, 0x4FD7}, //5471 #CJK UNIFIED IDEOGRAPH
    {0xAB56, 0x4FAE}, //5472 #CJK UNIFIED IDEOGRAPH
    {0xAB57, 0x4FD0}, //5473 #CJK UNIFIED IDEOGRAPH
    {0xAB58, 0x4FC4}, //5474 #CJK UNIFIED IDEOGRAPH
    {0xAB59, 0x4FC2}, //5475 #CJK UNIFIED IDEOGRAPH
    {0xAB5A, 0x4FDA}, //5476 #CJK UNIFIED IDEOGRAPH
    {0xAB5B, 0x4FCE}, //5477 #CJK UNIFIED IDEOGRAPH
    {0xAB5C, 0x4FDE}, //5478 #CJK UNIFIED IDEOGRAPH
    {0xAB5D, 0x4FB7}, //5479 #CJK UNIFIED IDEOGRAPH
    {0xAB5E, 0x5157}, //5480 #CJK UNIFIED IDEOGRAPH
    {0xAB5F, 0x5192}, //5481 #CJK UNIFIED IDEOGRAPH
    {0xAB60, 0x5191}, //5482 #CJK UNIFIED IDEOGRAPH
    {0xAB61, 0x51A0}, //5483 #CJK UNIFIED IDEOGRAPH
    {0xAB62, 0x524E}, //5484 #CJK UNIFIED IDEOGRAPH
    {0xAB63, 0x5243}, //5485 #CJK UNIFIED IDEOGRAPH
    {0xAB64, 0x524A}, //5486 #CJK UNIFIED IDEOGRAPH
    {0xAB65, 0x524D}, //5487 #CJK UNIFIED IDEOGRAPH
    {0xAB66, 0x524C}, //5488 #CJK UNIFIED IDEOGRAPH
    {0xAB67, 0x524B}, //5489 #CJK UNIFIED IDEOGRAPH
    {0xAB68, 0x5247}, //5490 #CJK UNIFIED IDEOGRAPH
    {0xAB69, 0x52C7}, //5491 #CJK UNIFIED IDEOGRAPH
    {0xAB6A, 0x52C9}, //5492 #CJK UNIFIED IDEOGRAPH
    {0xAB6B, 0x52C3}, //5493 #CJK UNIFIED IDEOGRAPH
    {0xAB6C, 0x52C1}, //5494 #CJK UNIFIED IDEOGRAPH
    {0xAB6D, 0x530D}, //5495 #CJK UNIFIED IDEOGRAPH
    {0xAB6E, 0x5357}, //5496 #CJK UNIFIED IDEOGRAPH
    {0xAB6F, 0x537B}, //5497 #CJK UNIFIED IDEOGRAPH
    {0xAB70, 0x539A}, //5498 #CJK UNIFIED IDEOGRAPH
    {0xAB71, 0x53DB}, //5499 #CJK UNIFIED IDEOGRAPH
    {0xAB72, 0x54AC}, //5500 #CJK UNIFIED IDEOGRAPH
    {0xAB73, 0x54C0}, //5501 #CJK UNIFIED IDEOGRAPH
    {0xAB74, 0x54A8}, //5502 #CJK UNIFIED IDEOGRAPH
    {0xAB75, 0x54CE}, //5503 #CJK UNIFIED IDEOGRAPH
    {0xAB76, 0x54C9}, //5504 #CJK UNIFIED IDEOGRAPH
    {0xAB77, 0x54B8}, //5505 #CJK UNIFIED IDEOGRAPH
    {0xAB78, 0x54A6}, //5506 #CJK UNIFIED IDEOGRAPH
    {0xAB79, 0x54B3}, //5507 #CJK UNIFIED IDEOGRAPH
    {0xAB7A, 0x54C7}, //5508 #CJK UNIFIED IDEOGRAPH
    {0xAB7B, 0x54C2}, //5509 #CJK UNIFIED IDEOGRAPH
    {0xAB7C, 0x54BD}, //5510 #CJK UNIFIED IDEOGRAPH
    {0xAB7D, 0x54AA}, //5511 #CJK UNIFIED IDEOGRAPH
    {0xAB7E, 0x54C1}, //5512 #CJK UNIFIED IDEOGRAPH
    {0xABA1, 0x54C4}, //5513 #CJK UNIFIED IDEOGRAPH
    {0xABA2, 0x54C8}, //5514 #CJK UNIFIED IDEOGRAPH
    {0xABA3, 0x54AF}, //5515 #CJK UNIFIED IDEOGRAPH
    {0xABA4, 0x54AB}, //5516 #CJK UNIFIED IDEOGRAPH
    {0xABA5, 0x54B1}, //5517 #CJK UNIFIED IDEOGRAPH
    {0xABA6, 0x54BB}, //5518 #CJK UNIFIED IDEOGRAPH
    {0xABA7, 0x54A9}, //5519 #CJK UNIFIED IDEOGRAPH
    {0xABA8, 0x54A7}, //5520 #CJK UNIFIED IDEOGRAPH
    {0xABA9, 0x54BF}, //5521 #CJK UNIFIED IDEOGRAPH
    {0xABAA, 0x56FF}, //5522 #CJK UNIFIED IDEOGRAPH
    {0xABAB, 0x5782}, //5523 #CJK UNIFIED IDEOGRAPH
    {0xABAC, 0x578B}, //5524 #CJK UNIFIED IDEOGRAPH
    {0xABAD, 0x57A0}, //5525 #CJK UNIFIED IDEOGRAPH
    {0xABAE, 0x57A3}, //5526 #CJK UNIFIED IDEOGRAPH
    {0xABAF, 0x57A2}, //5527 #CJK UNIFIED IDEOGRAPH
    {0xABB0, 0x57CE}, //5528 #CJK UNIFIED IDEOGRAPH
    {0xABB1, 0x57AE}, //5529 #CJK UNIFIED IDEOGRAPH
    {0xABB2, 0x5793}, //5530 #CJK UNIFIED IDEOGRAPH
    {0xABB3, 0x5955}, //5531 #CJK UNIFIED IDEOGRAPH
    {0xABB4, 0x5951}, //5532 #CJK UNIFIED IDEOGRAPH
    {0xABB5, 0x594F}, //5533 #CJK UNIFIED IDEOGRAPH
    {0xABB6, 0x594E}, //5534 #CJK UNIFIED IDEOGRAPH
    {0xABB7, 0x5950}, //5535 #CJK UNIFIED IDEOGRAPH
    {0xABB8, 0x59DC}, //5536 #CJK UNIFIED IDEOGRAPH
    {0xABB9, 0x59D8}, //5537 #CJK UNIFIED IDEOGRAPH
    {0xABBA, 0x59FF}, //5538 #CJK UNIFIED IDEOGRAPH
    {0xABBB, 0x59E3}, //5539 #CJK UNIFIED IDEOGRAPH
    {0xABBC, 0x59E8}, //5540 #CJK UNIFIED IDEOGRAPH
    {0xABBD, 0x5A03}, //5541 #CJK UNIFIED IDEOGRAPH
    {0xABBE, 0x59E5}, //5542 #CJK UNIFIED IDEOGRAPH
    {0xABBF, 0x59EA}, //5543 #CJK UNIFIED IDEOGRAPH
    {0xABC0, 0x59DA}, //5544 #CJK UNIFIED IDEOGRAPH
    {0xABC1, 0x59E6}, //5545 #CJK UNIFIED IDEOGRAPH
    {0xABC2, 0x5A01}, //5546 #CJK UNIFIED IDEOGRAPH
    {0xABC3, 0x59FB}, //5547 #CJK UNIFIED IDEOGRAPH
    {0xABC4, 0x5B69}, //5548 #CJK UNIFIED IDEOGRAPH
    {0xABC5, 0x5BA3}, //5549 #CJK UNIFIED IDEOGRAPH
    {0xABC6, 0x5BA6}, //5550 #CJK UNIFIED IDEOGRAPH
    {0xABC7, 0x5BA4}, //5551 #CJK UNIFIED IDEOGRAPH
    {0xABC8, 0x5BA2}, //5552 #CJK UNIFIED IDEOGRAPH
    {0xABC9, 0x5BA5}, //5553 #CJK UNIFIED IDEOGRAPH
    {0xABCA, 0x5C01}, //5554 #CJK UNIFIED IDEOGRAPH
    {0xABCB, 0x5C4E}, //5555 #CJK UNIFIED IDEOGRAPH
    {0xABCC, 0x5C4F}, //5556 #CJK UNIFIED IDEOGRAPH
    {0xABCD, 0x5C4D}, //5557 #CJK UNIFIED IDEOGRAPH
    {0xABCE, 0x5C4B}, //5558 #CJK UNIFIED IDEOGRAPH
    {0xABCF, 0x5CD9}, //5559 #CJK UNIFIED IDEOGRAPH
    {0xABD0, 0x5CD2}, //5560 #CJK UNIFIED IDEOGRAPH
    {0xABD1, 0x5DF7}, //5561 #CJK UNIFIED IDEOGRAPH
    {0xABD2, 0x5E1D}, //5562 #CJK UNIFIED IDEOGRAPH
    {0xABD3, 0x5E25}, //5563 #CJK UNIFIED IDEOGRAPH
    {0xABD4, 0x5E1F}, //5564 #CJK UNIFIED IDEOGRAPH
    {0xABD5, 0x5E7D}, //5565 #CJK UNIFIED IDEOGRAPH
    {0xABD6, 0x5EA0}, //5566 #CJK UNIFIED IDEOGRAPH
    {0xABD7, 0x5EA6}, //5567 #CJK UNIFIED IDEOGRAPH
    {0xABD8, 0x5EFA}, //5568 #CJK UNIFIED IDEOGRAPH
    {0xABD9, 0x5F08}, //5569 #CJK UNIFIED IDEOGRAPH
    {0xABDA, 0x5F2D}, //5570 #CJK UNIFIED IDEOGRAPH
    {0xABDB, 0x5F65}, //5571 #CJK UNIFIED IDEOGRAPH
    {0xABDC, 0x5F88}, //5572 #CJK UNIFIED IDEOGRAPH
    {0xABDD, 0x5F85}, //5573 #CJK UNIFIED IDEOGRAPH
    {0xABDE, 0x5F8A}, //5574 #CJK UNIFIED IDEOGRAPH
    {0xABDF, 0x5F8B}, //5575 #CJK UNIFIED IDEOGRAPH
    {0xABE0, 0x5F87}, //5576 #CJK UNIFIED IDEOGRAPH
    {0xABE1, 0x5F8C}, //5577 #CJK UNIFIED IDEOGRAPH
    {0xABE2, 0x5F89}, //5578 #CJK UNIFIED IDEOGRAPH
    {0xABE3, 0x6012}, //5579 #CJK UNIFIED IDEOGRAPH
    {0xABE4, 0x601D}, //5580 #CJK UNIFIED IDEOGRAPH
    {0xABE5, 0x6020}, //5581 #CJK UNIFIED IDEOGRAPH
    {0xABE6, 0x6025}, //5582 #CJK UNIFIED IDEOGRAPH
    {0xABE7, 0x600E}, //5583 #CJK UNIFIED IDEOGRAPH
    {0xABE8, 0x6028}, //5584 #CJK UNIFIED IDEOGRAPH
    {0xABE9, 0x604D}, //5585 #CJK UNIFIED IDEOGRAPH
    {0xABEA, 0x6070}, //5586 #CJK UNIFIED IDEOGRAPH
    {0xABEB, 0x6068}, //5587 #CJK UNIFIED IDEOGRAPH
    {0xABEC, 0x6062}, //5588 #CJK UNIFIED IDEOGRAPH
    {0xABED, 0x6046}, //5589 #CJK UNIFIED IDEOGRAPH
    {0xABEE, 0x6043}, //5590 #CJK UNIFIED IDEOGRAPH
    {0xABEF, 0x606C}, //5591 #CJK UNIFIED IDEOGRAPH
    {0xABF0, 0x606B}, //5592 #CJK UNIFIED IDEOGRAPH
    {0xABF1, 0x606A}, //5593 #CJK UNIFIED IDEOGRAPH
    {0xABF2, 0x6064}, //5594 #CJK UNIFIED IDEOGRAPH
    {0xABF3, 0x6241}, //5595 #CJK UNIFIED IDEOGRAPH
    {0xABF4, 0x62DC}, //5596 #CJK UNIFIED IDEOGRAPH
    {0xABF5, 0x6316}, //5597 #CJK UNIFIED IDEOGRAPH
    {0xABF6, 0x6309}, //5598 #CJK UNIFIED IDEOGRAPH
    {0xABF7, 0x62FC}, //5599 #CJK UNIFIED IDEOGRAPH
    {0xABF8, 0x62ED}, //5600 #CJK UNIFIED IDEOGRAPH
    {0xABF9, 0x6301}, //5601 #CJK UNIFIED IDEOGRAPH
    {0xABFA, 0x62EE}, //5602 #CJK UNIFIED IDEOGRAPH
    {0xABFB, 0x62FD}, //5603 #CJK UNIFIED IDEOGRAPH
    {0xABFC, 0x6307}, //5604 #CJK UNIFIED IDEOGRAPH
    {0xABFD, 0x62F1}, //5605 #CJK UNIFIED IDEOGRAPH
    {0xABFE, 0x62F7}, //5606 #CJK UNIFIED IDEOGRAPH
    {0xAC40, 0x62EF}, //5607 #CJK UNIFIED IDEOGRAPH
    {0xAC41, 0x62EC}, //5608 #CJK UNIFIED IDEOGRAPH
    {0xAC42, 0x62FE}, //5609 #CJK UNIFIED IDEOGRAPH
    {0xAC43, 0x62F4}, //5610 #CJK UNIFIED IDEOGRAPH
    {0xAC44, 0x6311}, //5611 #CJK UNIFIED IDEOGRAPH
    {0xAC45, 0x6302}, //5612 #CJK UNIFIED IDEOGRAPH
    {0xAC46, 0x653F}, //5613 #CJK UNIFIED IDEOGRAPH
    {0xAC47, 0x6545}, //5614 #CJK UNIFIED IDEOGRAPH
    {0xAC48, 0x65AB}, //5615 #CJK UNIFIED IDEOGRAPH
    {0xAC49, 0x65BD}, //5616 #CJK UNIFIED IDEOGRAPH
    {0xAC4A, 0x65E2}, //5617 #CJK UNIFIED IDEOGRAPH
    {0xAC4B, 0x6625}, //5618 #CJK UNIFIED IDEOGRAPH
    {0xAC4C, 0x662D}, //5619 #CJK UNIFIED IDEOGRAPH
    {0xAC4D, 0x6620}, //5620 #CJK UNIFIED IDEOGRAPH
    {0xAC4E, 0x6627}, //5621 #CJK UNIFIED IDEOGRAPH
    {0xAC4F, 0x662F}, //5622 #CJK UNIFIED IDEOGRAPH
    {0xAC50, 0x661F}, //5623 #CJK UNIFIED IDEOGRAPH
    {0xAC51, 0x6628}, //5624 #CJK UNIFIED IDEOGRAPH
    {0xAC52, 0x6631}, //5625 #CJK UNIFIED IDEOGRAPH
    {0xAC53, 0x6624}, //5626 #CJK UNIFIED IDEOGRAPH
    {0xAC54, 0x66F7}, //5627 #CJK UNIFIED IDEOGRAPH
    {0xAC55, 0x67FF}, //5628 #CJK UNIFIED IDEOGRAPH
    {0xAC56, 0x67D3}, //5629 #CJK UNIFIED IDEOGRAPH
    {0xAC57, 0x67F1}, //5630 #CJK UNIFIED IDEOGRAPH
    {0xAC58, 0x67D4}, //5631 #CJK UNIFIED IDEOGRAPH
    {0xAC59, 0x67D0}, //5632 #CJK UNIFIED IDEOGRAPH
    {0xAC5A, 0x67EC}, //5633 #CJK UNIFIED IDEOGRAPH
    {0xAC5B, 0x67B6}, //5634 #CJK UNIFIED IDEOGRAPH
    {0xAC5C, 0x67AF}, //5635 #CJK UNIFIED IDEOGRAPH
    {0xAC5D, 0x67F5}, //5636 #CJK UNIFIED IDEOGRAPH
    {0xAC5E, 0x67E9}, //5637 #CJK UNIFIED IDEOGRAPH
    {0xAC5F, 0x67EF}, //5638 #CJK UNIFIED IDEOGRAPH
    {0xAC60, 0x67C4}, //5639 #CJK UNIFIED IDEOGRAPH
    {0xAC61, 0x67D1}, //5640 #CJK UNIFIED IDEOGRAPH
    {0xAC62, 0x67B4}, //5641 #CJK UNIFIED IDEOGRAPH
    {0xAC63, 0x67DA}, //5642 #CJK UNIFIED IDEOGRAPH
    {0xAC64, 0x67E5}, //5643 #CJK UNIFIED IDEOGRAPH
    {0xAC65, 0x67B8}, //5644 #CJK UNIFIED IDEOGRAPH
    {0xAC66, 0x67CF}, //5645 #CJK UNIFIED IDEOGRAPH
    {0xAC67, 0x67DE}, //5646 #CJK UNIFIED IDEOGRAPH
    {0xAC68, 0x67F3}, //5647 #CJK UNIFIED IDEOGRAPH
    {0xAC69, 0x67B0}, //5648 #CJK UNIFIED IDEOGRAPH
    {0xAC6A, 0x67D9}, //5649 #CJK UNIFIED IDEOGRAPH
    {0xAC6B, 0x67E2}, //5650 #CJK UNIFIED IDEOGRAPH
    {0xAC6C, 0x67DD}, //5651 #CJK UNIFIED IDEOGRAPH
    {0xAC6D, 0x67D2}, //5652 #CJK UNIFIED IDEOGRAPH
    {0xAC6E, 0x6B6A}, //5653 #CJK UNIFIED IDEOGRAPH
    {0xAC6F, 0x6B83}, //5654 #CJK UNIFIED IDEOGRAPH
    {0xAC70, 0x6B86}, //5655 #CJK UNIFIED IDEOGRAPH
    {0xAC71, 0x6BB5}, //5656 #CJK UNIFIED IDEOGRAPH
    {0xAC72, 0x6BD2}, //5657 #CJK UNIFIED IDEOGRAPH
    {0xAC73, 0x6BD7}, //5658 #CJK UNIFIED IDEOGRAPH
    {0xAC74, 0x6C1F}, //5659 #CJK UNIFIED IDEOGRAPH
    {0xAC75, 0x6CC9}, //5660 #CJK UNIFIED IDEOGRAPH
    {0xAC76, 0x6D0B}, //5661 #CJK UNIFIED IDEOGRAPH
    {0xAC77, 0x6D32}, //5662 #CJK UNIFIED IDEOGRAPH
    {0xAC78, 0x6D2A}, //5663 #CJK UNIFIED IDEOGRAPH
    {0xAC79, 0x6D41}, //5664 #CJK UNIFIED IDEOGRAPH
    {0xAC7A, 0x6D25}, //5665 #CJK UNIFIED IDEOGRAPH
    {0xAC7B, 0x6D0C}, //5666 #CJK UNIFIED IDEOGRAPH
    {0xAC7C, 0x6D31}, //5667 #CJK UNIFIED IDEOGRAPH
    {0xAC7D, 0x6D1E}, //5668 #CJK UNIFIED IDEOGRAPH
    {0xAC7E, 0x6D17}, //5669 #CJK UNIFIED IDEOGRAPH
    {0xACA1, 0x6D3B}, //5670 #CJK UNIFIED IDEOGRAPH
    {0xACA2, 0x6D3D}, //5671 #CJK UNIFIED IDEOGRAPH
    {0xACA3, 0x6D3E}, //5672 #CJK UNIFIED IDEOGRAPH
    {0xACA4, 0x6D36}, //5673 #CJK UNIFIED IDEOGRAPH
    {0xACA5, 0x6D1B}, //5674 #CJK UNIFIED IDEOGRAPH
    {0xACA6, 0x6CF5}, //5675 #CJK UNIFIED IDEOGRAPH
    {0xACA7, 0x6D39}, //5676 #CJK UNIFIED IDEOGRAPH
    {0xACA8, 0x6D27}, //5677 #CJK UNIFIED IDEOGRAPH
    {0xACA9, 0x6D38}, //5678 #CJK UNIFIED IDEOGRAPH
    {0xACAA, 0x6D29}, //5679 #CJK UNIFIED IDEOGRAPH
    {0xACAB, 0x6D2E}, //5680 #CJK UNIFIED IDEOGRAPH
    {0xACAC, 0x6D35}, //5681 #CJK UNIFIED IDEOGRAPH
    {0xACAD, 0x6D0E}, //5682 #CJK UNIFIED IDEOGRAPH
    {0xACAE, 0x6D2B}, //5683 #CJK UNIFIED IDEOGRAPH
    {0xACAF, 0x70AB}, //5684 #CJK UNIFIED IDEOGRAPH
    {0xACB0, 0x70BA}, //5685 #CJK UNIFIED IDEOGRAPH
    {0xACB1, 0x70B3}, //5686 #CJK UNIFIED IDEOGRAPH
    {0xACB2, 0x70AC}, //5687 #CJK UNIFIED IDEOGRAPH
    {0xACB3, 0x70AF}, //5688 #CJK UNIFIED IDEOGRAPH
    {0xACB4, 0x70AD}, //5689 #CJK UNIFIED IDEOGRAPH
    {0xACB5, 0x70B8}, //5690 #CJK UNIFIED IDEOGRAPH
    {0xACB6, 0x70AE}, //5691 #CJK UNIFIED IDEOGRAPH
    {0xACB7, 0x70A4}, //5692 #CJK UNIFIED IDEOGRAPH
    {0xACB8, 0x7230}, //5693 #CJK UNIFIED IDEOGRAPH
    {0xACB9, 0x7272}, //5694 #CJK UNIFIED IDEOGRAPH
    {0xACBA, 0x726F}, //5695 #CJK UNIFIED IDEOGRAPH
    {0xACBB, 0x7274}, //5696 #CJK UNIFIED IDEOGRAPH
    {0xACBC, 0x72E9}, //5697 #CJK UNIFIED IDEOGRAPH
    {0xACBD, 0x72E0}, //5698 #CJK UNIFIED IDEOGRAPH
    {0xACBE, 0x72E1}, //5699 #CJK UNIFIED IDEOGRAPH
    {0xACBF, 0x73B7}, //5700 #CJK UNIFIED IDEOGRAPH
    {0xACC0, 0x73CA}, //5701 #CJK UNIFIED IDEOGRAPH
    {0xACC1, 0x73BB}, //5702 #CJK UNIFIED IDEOGRAPH
    {0xACC2, 0x73B2}, //5703 #CJK UNIFIED IDEOGRAPH
    {0xACC3, 0x73CD}, //5704 #CJK UNIFIED IDEOGRAPH
    {0xACC4, 0x73C0}, //5705 #CJK UNIFIED IDEOGRAPH
    {0xACC5, 0x73B3}, //5706 #CJK UNIFIED IDEOGRAPH
    {0xACC6, 0x751A}, //5707 #CJK UNIFIED IDEOGRAPH
    {0xACC7, 0x752D}, //5708 #CJK UNIFIED IDEOGRAPH
    {0xACC8, 0x754F}, //5709 #CJK UNIFIED IDEOGRAPH
    {0xACC9, 0x754C}, //5710 #CJK UNIFIED IDEOGRAPH
    {0xACCA, 0x754E}, //5711 #CJK UNIFIED IDEOGRAPH
    {0xACCB, 0x754B}, //5712 #CJK UNIFIED IDEOGRAPH
    {0xACCC, 0x75AB}, //5713 #CJK UNIFIED IDEOGRAPH
    {0xACCD, 0x75A4}, //5714 #CJK UNIFIED IDEOGRAPH
    {0xACCE, 0x75A5}, //5715 #CJK UNIFIED IDEOGRAPH
    {0xACCF, 0x75A2}, //5716 #CJK UNIFIED IDEOGRAPH
    {0xACD0, 0x75A3}, //5717 #CJK UNIFIED IDEOGRAPH
    {0xACD1, 0x7678}, //5718 #CJK UNIFIED IDEOGRAPH
    {0xACD2, 0x7686}, //5719 #CJK UNIFIED IDEOGRAPH
    {0xACD3, 0x7687}, //5720 #CJK UNIFIED IDEOGRAPH
    {0xACD4, 0x7688}, //5721 #CJK UNIFIED IDEOGRAPH
    {0xACD5, 0x76C8}, //5722 #CJK UNIFIED IDEOGRAPH
    {0xACD6, 0x76C6}, //5723 #CJK UNIFIED IDEOGRAPH
    {0xACD7, 0x76C3}, //5724 #CJK UNIFIED IDEOGRAPH
    {0xACD8, 0x76C5}, //5725 #CJK UNIFIED IDEOGRAPH
    {0xACD9, 0x7701}, //5726 #CJK UNIFIED IDEOGRAPH
    {0xACDA, 0x76F9}, //5727 #CJK UNIFIED IDEOGRAPH
    {0xACDB, 0x76F8}, //5728 #CJK UNIFIED IDEOGRAPH
    {0xACDC, 0x7709}, //5729 #CJK UNIFIED IDEOGRAPH
    {0xACDD, 0x770B}, //5730 #CJK UNIFIED IDEOGRAPH
    {0xACDE, 0x76FE}, //5731 #CJK UNIFIED IDEOGRAPH
    {0xACDF, 0x76FC}, //5732 #CJK UNIFIED IDEOGRAPH
    {0xACE0, 0x7707}, //5733 #CJK UNIFIED IDEOGRAPH
    {0xACE1, 0x77DC}, //5734 #CJK UNIFIED IDEOGRAPH
    {0xACE2, 0x7802}, //5735 #CJK UNIFIED IDEOGRAPH
    {0xACE3, 0x7814}, //5736 #CJK UNIFIED IDEOGRAPH
    {0xACE4, 0x780C}, //5737 #CJK UNIFIED IDEOGRAPH
    {0xACE5, 0x780D}, //5738 #CJK UNIFIED IDEOGRAPH
    {0xACE6, 0x7946}, //5739 #CJK UNIFIED IDEOGRAPH
    {0xACE7, 0x7949}, //5740 #CJK UNIFIED IDEOGRAPH
    {0xACE8, 0x7948}, //5741 #CJK UNIFIED IDEOGRAPH
    {0xACE9, 0x7947}, //5742 #CJK UNIFIED IDEOGRAPH
    {0xACEA, 0x79B9}, //5743 #CJK UNIFIED IDEOGRAPH
    {0xACEB, 0x79BA}, //5744 #CJK UNIFIED IDEOGRAPH
    {0xACEC, 0x79D1}, //5745 #CJK UNIFIED IDEOGRAPH
    {0xACED, 0x79D2}, //5746 #CJK UNIFIED IDEOGRAPH
    {0xACEE, 0x79CB}, //5747 #CJK UNIFIED IDEOGRAPH
    {0xACEF, 0x7A7F}, //5748 #CJK UNIFIED IDEOGRAPH
    {0xACF0, 0x7A81}, //5749 #CJK UNIFIED IDEOGRAPH
    {0xACF1, 0x7AFF}, //5750 #CJK UNIFIED IDEOGRAPH
    {0xACF2, 0x7AFD}, //5751 #CJK UNIFIED IDEOGRAPH
    {0xACF3, 0x7C7D}, //5752 #CJK UNIFIED IDEOGRAPH
    {0xACF4, 0x7D02}, //5753 #CJK UNIFIED IDEOGRAPH
    {0xACF5, 0x7D05}, //5754 #CJK UNIFIED IDEOGRAPH
    {0xACF6, 0x7D00}, //5755 #CJK UNIFIED IDEOGRAPH
    {0xACF7, 0x7D09}, //5756 #CJK UNIFIED IDEOGRAPH
    {0xACF8, 0x7D07}, //5757 #CJK UNIFIED IDEOGRAPH
    {0xACF9, 0x7D04}, //5758 #CJK UNIFIED IDEOGRAPH
    {0xACFA, 0x7D06}, //5759 #CJK UNIFIED IDEOGRAPH
    {0xACFB, 0x7F38}, //5760 #CJK UNIFIED IDEOGRAPH
    {0xACFC, 0x7F8E}, //5761 #CJK UNIFIED IDEOGRAPH
    {0xACFD, 0x7FBF}, //5762 #CJK UNIFIED IDEOGRAPH
    {0xACFE, 0x8004}, //5763 #CJK UNIFIED IDEOGRAPH
    {0xAD40, 0x8010}, //5764 #CJK UNIFIED IDEOGRAPH
    {0xAD41, 0x800D}, //5765 #CJK UNIFIED IDEOGRAPH
    {0xAD42, 0x8011}, //5766 #CJK UNIFIED IDEOGRAPH
    {0xAD43, 0x8036}, //5767 #CJK UNIFIED IDEOGRAPH
    {0xAD44, 0x80D6}, //5768 #CJK UNIFIED IDEOGRAPH
    {0xAD45, 0x80E5}, //5769 #CJK UNIFIED IDEOGRAPH
    {0xAD46, 0x80DA}, //5770 #CJK UNIFIED IDEOGRAPH
    {0xAD47, 0x80C3}, //5771 #CJK UNIFIED IDEOGRAPH
    {0xAD48, 0x80C4}, //5772 #CJK UNIFIED IDEOGRAPH
    {0xAD49, 0x80CC}, //5773 #CJK UNIFIED IDEOGRAPH
    {0xAD4A, 0x80E1}, //5774 #CJK UNIFIED IDEOGRAPH
    {0xAD4B, 0x80DB}, //5775 #CJK UNIFIED IDEOGRAPH
    {0xAD4C, 0x80CE}, //5776 #CJK UNIFIED IDEOGRAPH
    {0xAD4D, 0x80DE}, //5777 #CJK UNIFIED IDEOGRAPH
    {0xAD4E, 0x80E4}, //5778 #CJK UNIFIED IDEOGRAPH
    {0xAD4F, 0x80DD}, //5779 #CJK UNIFIED IDEOGRAPH
    {0xAD50, 0x81F4}, //5780 #CJK UNIFIED IDEOGRAPH
    {0xAD51, 0x8222}, //5781 #CJK UNIFIED IDEOGRAPH
    {0xAD52, 0x82E7}, //5782 #CJK UNIFIED IDEOGRAPH
    {0xAD53, 0x8303}, //5783 #CJK UNIFIED IDEOGRAPH
    {0xAD54, 0x8305}, //5784 #CJK UNIFIED IDEOGRAPH
    {0xAD55, 0x82E3}, //5785 #CJK UNIFIED IDEOGRAPH
    {0xAD56, 0x82DB}, //5786 #CJK UNIFIED IDEOGRAPH
    {0xAD57, 0x82E6}, //5787 #CJK UNIFIED IDEOGRAPH
    {0xAD58, 0x8304}, //5788 #CJK UNIFIED IDEOGRAPH
    {0xAD59, 0x82E5}, //5789 #CJK UNIFIED IDEOGRAPH
    {0xAD5A, 0x8302}, //5790 #CJK UNIFIED IDEOGRAPH
    {0xAD5B, 0x8309}, //5791 #CJK UNIFIED IDEOGRAPH
    {0xAD5C, 0x82D2}, //5792 #CJK UNIFIED IDEOGRAPH
    {0xAD5D, 0x82D7}, //5793 #CJK UNIFIED IDEOGRAPH
    {0xAD5E, 0x82F1}, //5794 #CJK UNIFIED IDEOGRAPH
    {0xAD5F, 0x8301}, //5795 #CJK UNIFIED IDEOGRAPH
    {0xAD60, 0x82DC}, //5796 #CJK UNIFIED IDEOGRAPH
    {0xAD61, 0x82D4}, //5797 #CJK UNIFIED IDEOGRAPH
    {0xAD62, 0x82D1}, //5798 #CJK UNIFIED IDEOGRAPH
    {0xAD63, 0x82DE}, //5799 #CJK UNIFIED IDEOGRAPH
    {0xAD64, 0x82D3}, //5800 #CJK UNIFIED IDEOGRAPH
    {0xAD65, 0x82DF}, //5801 #CJK UNIFIED IDEOGRAPH
    {0xAD66, 0x82EF}, //5802 #CJK UNIFIED IDEOGRAPH
    {0xAD67, 0x8306}, //5803 #CJK UNIFIED IDEOGRAPH
    {0xAD68, 0x8650}, //5804 #CJK UNIFIED IDEOGRAPH
    {0xAD69, 0x8679}, //5805 #CJK UNIFIED IDEOGRAPH
    {0xAD6A, 0x867B}, //5806 #CJK UNIFIED IDEOGRAPH
    {0xAD6B, 0x867A}, //5807 #CJK UNIFIED IDEOGRAPH
    {0xAD6C, 0x884D}, //5808 #CJK UNIFIED IDEOGRAPH
    {0xAD6D, 0x886B}, //5809 #CJK UNIFIED IDEOGRAPH
    {0xAD6E, 0x8981}, //5810 #CJK UNIFIED IDEOGRAPH
    {0xAD6F, 0x89D4}, //5811 #CJK UNIFIED IDEOGRAPH
    {0xAD70, 0x8A08}, //5812 #CJK UNIFIED IDEOGRAPH
    {0xAD71, 0x8A02}, //5813 #CJK UNIFIED IDEOGRAPH
    {0xAD72, 0x8A03}, //5814 #CJK UNIFIED IDEOGRAPH
    {0xAD73, 0x8C9E}, //5815 #CJK UNIFIED IDEOGRAPH
    {0xAD74, 0x8CA0}, //5816 #CJK UNIFIED IDEOGRAPH
    {0xAD75, 0x8D74}, //5817 #CJK UNIFIED IDEOGRAPH
    {0xAD76, 0x8D73}, //5818 #CJK UNIFIED IDEOGRAPH
    {0xAD77, 0x8DB4}, //5819 #CJK UNIFIED IDEOGRAPH
    {0xAD78, 0x8ECD}, //5820 #CJK UNIFIED IDEOGRAPH
    {0xAD79, 0x8ECC}, //5821 #CJK UNIFIED IDEOGRAPH
    {0xAD7A, 0x8FF0}, //5822 #CJK UNIFIED IDEOGRAPH
    {0xAD7B, 0x8FE6}, //5823 #CJK UNIFIED IDEOGRAPH
    {0xAD7C, 0x8FE2}, //5824 #CJK UNIFIED IDEOGRAPH
    {0xAD7D, 0x8FEA}, //5825 #CJK UNIFIED IDEOGRAPH
    {0xAD7E, 0x8FE5}, //5826 #CJK UNIFIED IDEOGRAPH
    {0xADA1, 0x8FED}, //5827 #CJK UNIFIED IDEOGRAPH
    {0xADA2, 0x8FEB}, //5828 #CJK UNIFIED IDEOGRAPH
    {0xADA3, 0x8FE4}, //5829 #CJK UNIFIED IDEOGRAPH
    {0xADA4, 0x8FE8}, //5830 #CJK UNIFIED IDEOGRAPH
    {0xADA5, 0x90CA}, //5831 #CJK UNIFIED IDEOGRAPH
    {0xADA6, 0x90CE}, //5832 #CJK UNIFIED IDEOGRAPH
    {0xADA7, 0x90C1}, //5833 #CJK UNIFIED IDEOGRAPH
    {0xADA8, 0x90C3}, //5834 #CJK UNIFIED IDEOGRAPH
    {0xADA9, 0x914B}, //5835 #CJK UNIFIED IDEOGRAPH
    {0xADAA, 0x914A}, //5836 #CJK UNIFIED IDEOGRAPH
    {0xADAB, 0x91CD}, //5837 #CJK UNIFIED IDEOGRAPH
    {0xADAC, 0x9582}, //5838 #CJK UNIFIED IDEOGRAPH
    {0xADAD, 0x9650}, //5839 #CJK UNIFIED IDEOGRAPH
    {0xADAE, 0x964B}, //5840 #CJK UNIFIED IDEOGRAPH
    {0xADAF, 0x964C}, //5841 #CJK UNIFIED IDEOGRAPH
    {0xADB0, 0x964D}, //5842 #CJK UNIFIED IDEOGRAPH
    {0xADB1, 0x9762}, //5843 #CJK UNIFIED IDEOGRAPH
    {0xADB2, 0x9769}, //5844 #CJK UNIFIED IDEOGRAPH
    {0xADB3, 0x97CB}, //5845 #CJK UNIFIED IDEOGRAPH
    {0xADB4, 0x97ED}, //5846 #CJK UNIFIED IDEOGRAPH
    {0xADB5, 0x97F3}, //5847 #CJK UNIFIED IDEOGRAPH
    {0xADB6, 0x9801}, //5848 #CJK UNIFIED IDEOGRAPH
    {0xADB7, 0x98A8}, //5849 #CJK UNIFIED IDEOGRAPH
    {0xADB8, 0x98DB}, //5850 #CJK UNIFIED IDEOGRAPH
    {0xADB9, 0x98DF}, //5851 #CJK UNIFIED IDEOGRAPH
    {0xADBA, 0x9996}, //5852 #CJK UNIFIED IDEOGRAPH
    {0xADBB, 0x9999}, //5853 #CJK UNIFIED IDEOGRAPH
    {0xADBC, 0x4E58}, //5854 #CJK UNIFIED IDEOGRAPH
    {0xADBD, 0x4EB3}, //5855 #CJK UNIFIED IDEOGRAPH
    {0xADBE, 0x500C}, //5856 #CJK UNIFIED IDEOGRAPH
    {0xADBF, 0x500D}, //5857 #CJK UNIFIED IDEOGRAPH
    {0xADC0, 0x5023}, //5858 #CJK UNIFIED IDEOGRAPH
    {0xADC1, 0x4FEF}, //5859 #CJK UNIFIED IDEOGRAPH
    {0xADC2, 0x5026}, //5860 #CJK UNIFIED IDEOGRAPH
    {0xADC3, 0x5025}, //5861 #CJK UNIFIED IDEOGRAPH
    {0xADC4, 0x4FF8}, //5862 #CJK UNIFIED IDEOGRAPH
    {0xADC5, 0x5029}, //5863 #CJK UNIFIED IDEOGRAPH
    {0xADC6, 0x5016}, //5864 #CJK UNIFIED IDEOGRAPH
    {0xADC7, 0x5006}, //5865 #CJK UNIFIED IDEOGRAPH
    {0xADC8, 0x503C}, //5866 #CJK UNIFIED IDEOGRAPH
    {0xADC9, 0x501F}, //5867 #CJK UNIFIED IDEOGRAPH
    {0xADCA, 0x501A}, //5868 #CJK UNIFIED IDEOGRAPH
    {0xADCB, 0x5012}, //5869 #CJK UNIFIED IDEOGRAPH
    {0xADCC, 0x5011}, //5870 #CJK UNIFIED IDEOGRAPH
    {0xADCD, 0x4FFA}, //5871 #CJK UNIFIED IDEOGRAPH
    {0xADCE, 0x5000}, //5872 #CJK UNIFIED IDEOGRAPH
    {0xADCF, 0x5014}, //5873 #CJK UNIFIED IDEOGRAPH
    {0xADD0, 0x5028}, //5874 #CJK UNIFIED IDEOGRAPH
    {0xADD1, 0x4FF1}, //5875 #CJK UNIFIED IDEOGRAPH
    {0xADD2, 0x5021}, //5876 #CJK UNIFIED IDEOGRAPH
    {0xADD3, 0x500B}, //5877 #CJK UNIFIED IDEOGRAPH
    {0xADD4, 0x5019}, //5878 #CJK UNIFIED IDEOGRAPH
    {0xADD5, 0x5018}, //5879 #CJK UNIFIED IDEOGRAPH
    {0xADD6, 0x4FF3}, //5880 #CJK UNIFIED IDEOGRAPH
    {0xADD7, 0x4FEE}, //5881 #CJK UNIFIED IDEOGRAPH
    {0xADD8, 0x502D}, //5882 #CJK UNIFIED IDEOGRAPH
    {0xADD9, 0x502A}, //5883 #CJK UNIFIED IDEOGRAPH
    {0xADDA, 0x4FFE}, //5884 #CJK UNIFIED IDEOGRAPH
    {0xADDB, 0x502B}, //5885 #CJK UNIFIED IDEOGRAPH
    {0xADDC, 0x5009}, //5886 #CJK UNIFIED IDEOGRAPH
    {0xADDD, 0x517C}, //5887 #CJK UNIFIED IDEOGRAPH
    {0xADDE, 0x51A4}, //5888 #CJK UNIFIED IDEOGRAPH
    {0xADDF, 0x51A5}, //5889 #CJK UNIFIED IDEOGRAPH
    {0xADE0, 0x51A2}, //5890 #CJK UNIFIED IDEOGRAPH
    {0xADE1, 0x51CD}, //5891 #CJK UNIFIED IDEOGRAPH
    {0xADE2, 0x51CC}, //5892 #CJK UNIFIED IDEOGRAPH
    {0xADE3, 0x51C6}, //5893 #CJK UNIFIED IDEOGRAPH
    {0xADE4, 0x51CB}, //5894 #CJK UNIFIED IDEOGRAPH
    {0xADE5, 0x5256}, //5895 #CJK UNIFIED IDEOGRAPH
    {0xADE6, 0x525C}, //5896 #CJK UNIFIED IDEOGRAPH
    {0xADE7, 0x5254}, //5897 #CJK UNIFIED IDEOGRAPH
    {0xADE8, 0x525B}, //5898 #CJK UNIFIED IDEOGRAPH
    {0xADE9, 0x525D}, //5899 #CJK UNIFIED IDEOGRAPH
    {0xADEA, 0x532A}, //5900 #CJK UNIFIED IDEOGRAPH
    {0xADEB, 0x537F}, //5901 #CJK UNIFIED IDEOGRAPH
    {0xADEC, 0x539F}, //5902 #CJK UNIFIED IDEOGRAPH
    {0xADED, 0x539D}, //5903 #CJK UNIFIED IDEOGRAPH
    {0xADEE, 0x53DF}, //5904 #CJK UNIFIED IDEOGRAPH
    {0xADEF, 0x54E8}, //5905 #CJK UNIFIED IDEOGRAPH
    {0xADF0, 0x5510}, //5906 #CJK UNIFIED IDEOGRAPH
    {0xADF1, 0x5501}, //5907 #CJK UNIFIED IDEOGRAPH
    {0xADF2, 0x5537}, //5908 #CJK UNIFIED IDEOGRAPH
    {0xADF3, 0x54FC}, //5909 #CJK UNIFIED IDEOGRAPH
    {0xADF4, 0x54E5}, //5910 #CJK UNIFIED IDEOGRAPH
    {0xADF5, 0x54F2}, //5911 #CJK UNIFIED IDEOGRAPH
    {0xADF6, 0x5506}, //5912 #CJK UNIFIED IDEOGRAPH
    {0xADF7, 0x54FA}, //5913 #CJK UNIFIED IDEOGRAPH
    {0xADF8, 0x5514}, //5914 #CJK UNIFIED IDEOGRAPH
    {0xADF9, 0x54E9}, //5915 #CJK UNIFIED IDEOGRAPH
    {0xADFA, 0x54ED}, //5916 #CJK UNIFIED IDEOGRAPH
    {0xADFB, 0x54E1}, //5917 #CJK UNIFIED IDEOGRAPH
    {0xADFC, 0x5509}, //5918 #CJK UNIFIED IDEOGRAPH
    {0xADFD, 0x54EE}, //5919 #CJK UNIFIED IDEOGRAPH
    {0xADFE, 0x54EA}, //5920 #CJK UNIFIED IDEOGRAPH
    {0xAE40, 0x54E6}, //5921 #CJK UNIFIED IDEOGRAPH
    {0xAE41, 0x5527}, //5922 #CJK UNIFIED IDEOGRAPH
    {0xAE42, 0x5507}, //5923 #CJK UNIFIED IDEOGRAPH
    {0xAE43, 0x54FD}, //5924 #CJK UNIFIED IDEOGRAPH
    {0xAE44, 0x550F}, //5925 #CJK UNIFIED IDEOGRAPH
    {0xAE45, 0x5703}, //5926 #CJK UNIFIED IDEOGRAPH
    {0xAE46, 0x5704}, //5927 #CJK UNIFIED IDEOGRAPH
    {0xAE47, 0x57C2}, //5928 #CJK UNIFIED IDEOGRAPH
    {0xAE48, 0x57D4}, //5929 #CJK UNIFIED IDEOGRAPH
    {0xAE49, 0x57CB}, //5930 #CJK UNIFIED IDEOGRAPH
    {0xAE4A, 0x57C3}, //5931 #CJK UNIFIED IDEOGRAPH
    {0xAE4B, 0x5809}, //5932 #CJK UNIFIED IDEOGRAPH
    {0xAE4C, 0x590F}, //5933 #CJK UNIFIED IDEOGRAPH
    {0xAE4D, 0x5957}, //5934 #CJK UNIFIED IDEOGRAPH
    {0xAE4E, 0x5958}, //5935 #CJK UNIFIED IDEOGRAPH
    {0xAE4F, 0x595A}, //5936 #CJK UNIFIED IDEOGRAPH
    {0xAE50, 0x5A11}, //5937 #CJK UNIFIED IDEOGRAPH
    {0xAE51, 0x5A18}, //5938 #CJK UNIFIED IDEOGRAPH
    {0xAE52, 0x5A1C}, //5939 #CJK UNIFIED IDEOGRAPH
    {0xAE53, 0x5A1F}, //5940 #CJK UNIFIED IDEOGRAPH
    {0xAE54, 0x5A1B}, //5941 #CJK UNIFIED IDEOGRAPH
    {0xAE55, 0x5A13}, //5942 #CJK UNIFIED IDEOGRAPH
    {0xAE56, 0x59EC}, //5943 #CJK UNIFIED IDEOGRAPH
    {0xAE57, 0x5A20}, //5944 #CJK UNIFIED IDEOGRAPH
    {0xAE58, 0x5A23}, //5945 #CJK UNIFIED IDEOGRAPH
    {0xAE59, 0x5A29}, //5946 #CJK UNIFIED IDEOGRAPH
    {0xAE5A, 0x5A25}, //5947 #CJK UNIFIED IDEOGRAPH
    {0xAE5B, 0x5A0C}, //5948 #CJK UNIFIED IDEOGRAPH
    {0xAE5C, 0x5A09}, //5949 #CJK UNIFIED IDEOGRAPH
    {0xAE5D, 0x5B6B}, //5950 #CJK UNIFIED IDEOGRAPH
    {0xAE5E, 0x5C58}, //5951 #CJK UNIFIED IDEOGRAPH
    {0xAE5F, 0x5BB0}, //5952 #CJK UNIFIED IDEOGRAPH
    {0xAE60, 0x5BB3}, //5953 #CJK UNIFIED IDEOGRAPH
    {0xAE61, 0x5BB6}, //5954 #CJK UNIFIED IDEOGRAPH
    {0xAE62, 0x5BB4}, //5955 #CJK UNIFIED IDEOGRAPH
    {0xAE63, 0x5BAE}, //5956 #CJK UNIFIED IDEOGRAPH
    {0xAE64, 0x5BB5}, //5957 #CJK UNIFIED IDEOGRAPH
    {0xAE65, 0x5BB9}, //5958 #CJK UNIFIED IDEOGRAPH
    {0xAE66, 0x5BB8}, //5959 #CJK UNIFIED IDEOGRAPH
    {0xAE67, 0x5C04}, //5960 #CJK UNIFIED IDEOGRAPH
    {0xAE68, 0x5C51}, //5961 #CJK UNIFIED IDEOGRAPH
    {0xAE69, 0x5C55}, //5962 #CJK UNIFIED IDEOGRAPH
    {0xAE6A, 0x5C50}, //5963 #CJK UNIFIED IDEOGRAPH
    {0xAE6B, 0x5CED}, //5964 #CJK UNIFIED IDEOGRAPH
    {0xAE6C, 0x5CFD}, //5965 #CJK UNIFIED IDEOGRAPH
    {0xAE6D, 0x5CFB}, //5966 #CJK UNIFIED IDEOGRAPH
    {0xAE6E, 0x5CEA}, //5967 #CJK UNIFIED IDEOGRAPH
    {0xAE6F, 0x5CE8}, //5968 #CJK UNIFIED IDEOGRAPH
    {0xAE70, 0x5CF0}, //5969 #CJK UNIFIED IDEOGRAPH
    {0xAE71, 0x5CF6}, //5970 #CJK UNIFIED IDEOGRAPH
    {0xAE72, 0x5D01}, //5971 #CJK UNIFIED IDEOGRAPH
    {0xAE73, 0x5CF4}, //5972 #CJK UNIFIED IDEOGRAPH
    {0xAE74, 0x5DEE}, //5973 #CJK UNIFIED IDEOGRAPH
    {0xAE75, 0x5E2D}, //5974 #CJK UNIFIED IDEOGRAPH
    {0xAE76, 0x5E2B}, //5975 #CJK UNIFIED IDEOGRAPH
    {0xAE77, 0x5EAB}, //5976 #CJK UNIFIED IDEOGRAPH
    {0xAE78, 0x5EAD}, //5977 #CJK UNIFIED IDEOGRAPH
    {0xAE79, 0x5EA7}, //5978 #CJK UNIFIED IDEOGRAPH
    {0xAE7A, 0x5F31}, //5979 #CJK UNIFIED IDEOGRAPH
    {0xAE7B, 0x5F92}, //5980 #CJK UNIFIED IDEOGRAPH
    {0xAE7C, 0x5F91}, //5981 #CJK UNIFIED IDEOGRAPH
    {0xAE7D, 0x5F90}, //5982 #CJK UNIFIED IDEOGRAPH
    {0xAE7E, 0x6059}, //5983 #CJK UNIFIED IDEOGRAPH
    {0xAEA1, 0x6063}, //5984 #CJK UNIFIED IDEOGRAPH
    {0xAEA2, 0x6065}, //5985 #CJK UNIFIED IDEOGRAPH
    {0xAEA3, 0x6050}, //5986 #CJK UNIFIED IDEOGRAPH
    {0xAEA4, 0x6055}, //5987 #CJK UNIFIED IDEOGRAPH
    {0xAEA5, 0x606D}, //5988 #CJK UNIFIED IDEOGRAPH
    {0xAEA6, 0x6069}, //5989 #CJK UNIFIED IDEOGRAPH
    {0xAEA7, 0x606F}, //5990 #CJK UNIFIED IDEOGRAPH
    {0xAEA8, 0x6084}, //5991 #CJK UNIFIED IDEOGRAPH
    {0xAEA9, 0x609F}, //5992 #CJK UNIFIED IDEOGRAPH
    {0xAEAA, 0x609A}, //5993 #CJK UNIFIED IDEOGRAPH
    {0xAEAB, 0x608D}, //5994 #CJK UNIFIED IDEOGRAPH
    {0xAEAC, 0x6094}, //5995 #CJK UNIFIED IDEOGRAPH
    {0xAEAD, 0x608C}, //5996 #CJK UNIFIED IDEOGRAPH
    {0xAEAE, 0x6085}, //5997 #CJK UNIFIED IDEOGRAPH
    {0xAEAF, 0x6096}, //5998 #CJK UNIFIED IDEOGRAPH
    {0xAEB0, 0x6247}, //5999 #CJK UNIFIED IDEOGRAPH
    {0xAEB1, 0x62F3}, //6000 #CJK UNIFIED IDEOGRAPH
    {0xAEB2, 0x6308}, //6001 #CJK UNIFIED IDEOGRAPH
    {0xAEB3, 0x62FF}, //6002 #CJK UNIFIED IDEOGRAPH
    {0xAEB4, 0x634E}, //6003 #CJK UNIFIED IDEOGRAPH
    {0xAEB5, 0x633E}, //6004 #CJK UNIFIED IDEOGRAPH
    {0xAEB6, 0x632F}, //6005 #CJK UNIFIED IDEOGRAPH
    {0xAEB7, 0x6355}, //6006 #CJK UNIFIED IDEOGRAPH
    {0xAEB8, 0x6342}, //6007 #CJK UNIFIED IDEOGRAPH
    {0xAEB9, 0x6346}, //6008 #CJK UNIFIED IDEOGRAPH
    {0xAEBA, 0x634F}, //6009 #CJK UNIFIED IDEOGRAPH
    {0xAEBB, 0x6349}, //6010 #CJK UNIFIED IDEOGRAPH
    {0xAEBC, 0x633A}, //6011 #CJK UNIFIED IDEOGRAPH
    {0xAEBD, 0x6350}, //6012 #CJK UNIFIED IDEOGRAPH
    {0xAEBE, 0x633D}, //6013 #CJK UNIFIED IDEOGRAPH
    {0xAEBF, 0x632A}, //6014 #CJK UNIFIED IDEOGRAPH
    {0xAEC0, 0x632B}, //6015 #CJK UNIFIED IDEOGRAPH
    {0xAEC1, 0x6328}, //6016 #CJK UNIFIED IDEOGRAPH
    {0xAEC2, 0x634D}, //6017 #CJK UNIFIED IDEOGRAPH
    {0xAEC3, 0x634C}, //6018 #CJK UNIFIED IDEOGRAPH
    {0xAEC4, 0x6548}, //6019 #CJK UNIFIED IDEOGRAPH
    {0xAEC5, 0x6549}, //6020 #CJK UNIFIED IDEOGRAPH
    {0xAEC6, 0x6599}, //6021 #CJK UNIFIED IDEOGRAPH
    {0xAEC7, 0x65C1}, //6022 #CJK UNIFIED IDEOGRAPH
    {0xAEC8, 0x65C5}, //6023 #CJK UNIFIED IDEOGRAPH
    {0xAEC9, 0x6642}, //6024 #CJK UNIFIED IDEOGRAPH
    {0xAECA, 0x6649}, //6025 #CJK UNIFIED IDEOGRAPH
    {0xAECB, 0x664F}, //6026 #CJK UNIFIED IDEOGRAPH
    {0xAECC, 0x6643}, //6027 #CJK UNIFIED IDEOGRAPH
    {0xAECD, 0x6652}, //6028 #CJK UNIFIED IDEOGRAPH
    {0xAECE, 0x664C}, //6029 #CJK UNIFIED IDEOGRAPH
    {0xAECF, 0x6645}, //6030 #CJK UNIFIED IDEOGRAPH
    {0xAED0, 0x6641}, //6031 #CJK UNIFIED IDEOGRAPH
    {0xAED1, 0x66F8}, //6032 #CJK UNIFIED IDEOGRAPH
    {0xAED2, 0x6714}, //6033 #CJK UNIFIED IDEOGRAPH
    {0xAED3, 0x6715}, //6034 #CJK UNIFIED IDEOGRAPH
    {0xAED4, 0x6717}, //6035 #CJK UNIFIED IDEOGRAPH
    {0xAED5, 0x6821}, //6036 #CJK UNIFIED IDEOGRAPH
    {0xAED6, 0x6838}, //6037 #CJK UNIFIED IDEOGRAPH
    {0xAED7, 0x6848}, //6038 #CJK UNIFIED IDEOGRAPH
    {0xAED8, 0x6846}, //6039 #CJK UNIFIED IDEOGRAPH
    {0xAED9, 0x6853}, //6040 #CJK UNIFIED IDEOGRAPH
    {0xAEDA, 0x6839}, //6041 #CJK UNIFIED IDEOGRAPH
    {0xAEDB, 0x6842}, //6042 #CJK UNIFIED IDEOGRAPH
    {0xAEDC, 0x6854}, //6043 #CJK UNIFIED IDEOGRAPH
    {0xAEDD, 0x6829}, //6044 #CJK UNIFIED IDEOGRAPH
    {0xAEDE, 0x68B3}, //6045 #CJK UNIFIED IDEOGRAPH
    {0xAEDF, 0x6817}, //6046 #CJK UNIFIED IDEOGRAPH
    {0xAEE0, 0x684C}, //6047 #CJK UNIFIED IDEOGRAPH
    {0xAEE1, 0x6851}, //6048 #CJK UNIFIED IDEOGRAPH
    {0xAEE2, 0x683D}, //6049 #CJK UNIFIED IDEOGRAPH
    {0xAEE3, 0x67F4}, //6050 #CJK UNIFIED IDEOGRAPH
    {0xAEE4, 0x6850}, //6051 #CJK UNIFIED IDEOGRAPH
    {0xAEE5, 0x6840}, //6052 #CJK UNIFIED IDEOGRAPH
    {0xAEE6, 0x683C}, //6053 #CJK UNIFIED IDEOGRAPH
    {0xAEE7, 0x6843}, //6054 #CJK UNIFIED IDEOGRAPH
    {0xAEE8, 0x682A}, //6055 #CJK UNIFIED IDEOGRAPH
    {0xAEE9, 0x6845}, //6056 #CJK UNIFIED IDEOGRAPH
    {0xAEEA, 0x6813}, //6057 #CJK UNIFIED IDEOGRAPH
    {0xAEEB, 0x6818}, //6058 #CJK UNIFIED IDEOGRAPH
    {0xAEEC, 0x6841}, //6059 #CJK UNIFIED IDEOGRAPH
    {0xAEED, 0x6B8A}, //6060 #CJK UNIFIED IDEOGRAPH
    {0xAEEE, 0x6B89}, //6061 #CJK UNIFIED IDEOGRAPH
    {0xAEEF, 0x6BB7}, //6062 #CJK UNIFIED IDEOGRAPH
    {0xAEF0, 0x6C23}, //6063 #CJK UNIFIED IDEOGRAPH
    {0xAEF1, 0x6C27}, //6064 #CJK UNIFIED IDEOGRAPH
    {0xAEF2, 0x6C28}, //6065 #CJK UNIFIED IDEOGRAPH
    {0xAEF3, 0x6C26}, //6066 #CJK UNIFIED IDEOGRAPH
    {0xAEF4, 0x6C24}, //6067 #CJK UNIFIED IDEOGRAPH
    {0xAEF5, 0x6CF0}, //6068 #CJK UNIFIED IDEOGRAPH
    {0xAEF6, 0x6D6A}, //6069 #CJK UNIFIED IDEOGRAPH
    {0xAEF7, 0x6D95}, //6070 #CJK UNIFIED IDEOGRAPH
    {0xAEF8, 0x6D88}, //6071 #CJK UNIFIED IDEOGRAPH
    {0xAEF9, 0x6D87}, //6072 #CJK UNIFIED IDEOGRAPH
    {0xAEFA, 0x6D66}, //6073 #CJK UNIFIED IDEOGRAPH
    {0xAEFB, 0x6D78}, //6074 #CJK UNIFIED IDEOGRAPH
    {0xAEFC, 0x6D77}, //6075 #CJK UNIFIED IDEOGRAPH
    {0xAEFD, 0x6D59}, //6076 #CJK UNIFIED IDEOGRAPH
    {0xAEFE, 0x6D93}, //6077 #CJK UNIFIED IDEOGRAPH
    {0xAF40, 0x6D6C}, //6078 #CJK UNIFIED IDEOGRAPH
    {0xAF41, 0x6D89}, //6079 #CJK UNIFIED IDEOGRAPH
    {0xAF42, 0x6D6E}, //6080 #CJK UNIFIED IDEOGRAPH
    {0xAF43, 0x6D5A}, //6081 #CJK UNIFIED IDEOGRAPH
    {0xAF44, 0x6D74}, //6082 #CJK UNIFIED IDEOGRAPH
    {0xAF45, 0x6D69}, //6083 #CJK UNIFIED IDEOGRAPH
    {0xAF46, 0x6D8C}, //6084 #CJK UNIFIED IDEOGRAPH
    {0xAF47, 0x6D8A}, //6085 #CJK UNIFIED IDEOGRAPH
    {0xAF48, 0x6D79}, //6086 #CJK UNIFIED IDEOGRAPH
    {0xAF49, 0x6D85}, //6087 #CJK UNIFIED IDEOGRAPH
    {0xAF4A, 0x6D65}, //6088 #CJK UNIFIED IDEOGRAPH
    {0xAF4B, 0x6D94}, //6089 #CJK UNIFIED IDEOGRAPH
    {0xAF4C, 0x70CA}, //6090 #CJK UNIFIED IDEOGRAPH
    {0xAF4D, 0x70D8}, //6091 #CJK UNIFIED IDEOGRAPH
    {0xAF4E, 0x70E4}, //6092 #CJK UNIFIED IDEOGRAPH
    {0xAF4F, 0x70D9}, //6093 #CJK UNIFIED IDEOGRAPH
    {0xAF50, 0x70C8}, //6094 #CJK UNIFIED IDEOGRAPH
    {0xAF51, 0x70CF}, //6095 #CJK UNIFIED IDEOGRAPH
    {0xAF52, 0x7239}, //6096 #CJK UNIFIED IDEOGRAPH
    {0xAF53, 0x7279}, //6097 #CJK UNIFIED IDEOGRAPH
    {0xAF54, 0x72FC}, //6098 #CJK UNIFIED IDEOGRAPH
    {0xAF55, 0x72F9}, //6099 #CJK UNIFIED IDEOGRAPH
    {0xAF56, 0x72FD}, //6100 #CJK UNIFIED IDEOGRAPH
    {0xAF57, 0x72F8}, //6101 #CJK UNIFIED IDEOGRAPH
    {0xAF58, 0x72F7}, //6102 #CJK UNIFIED IDEOGRAPH
    {0xAF59, 0x7386}, //6103 #CJK UNIFIED IDEOGRAPH
    {0xAF5A, 0x73ED}, //6104 #CJK UNIFIED IDEOGRAPH
    {0xAF5B, 0x7409}, //6105 #CJK UNIFIED IDEOGRAPH
    {0xAF5C, 0x73EE}, //6106 #CJK UNIFIED IDEOGRAPH
    {0xAF5D, 0x73E0}, //6107 #CJK UNIFIED IDEOGRAPH
    {0xAF5E, 0x73EA}, //6108 #CJK UNIFIED IDEOGRAPH
    {0xAF5F, 0x73DE}, //6109 #CJK UNIFIED IDEOGRAPH
    {0xAF60, 0x7554}, //6110 #CJK UNIFIED IDEOGRAPH
    {0xAF61, 0x755D}, //6111 #CJK UNIFIED IDEOGRAPH
    {0xAF62, 0x755C}, //6112 #CJK UNIFIED IDEOGRAPH
    {0xAF63, 0x755A}, //6113 #CJK UNIFIED IDEOGRAPH
    {0xAF64, 0x7559}, //6114 #CJK UNIFIED IDEOGRAPH
    {0xAF65, 0x75BE}, //6115 #CJK UNIFIED IDEOGRAPH
    {0xAF66, 0x75C5}, //6116 #CJK UNIFIED IDEOGRAPH
    {0xAF67, 0x75C7}, //6117 #CJK UNIFIED IDEOGRAPH
    {0xAF68, 0x75B2}, //6118 #CJK UNIFIED IDEOGRAPH
    {0xAF69, 0x75B3}, //6119 #CJK UNIFIED IDEOGRAPH
    {0xAF6A, 0x75BD}, //6120 #CJK UNIFIED IDEOGRAPH
    {0xAF6B, 0x75BC}, //6121 #CJK UNIFIED IDEOGRAPH
    {0xAF6C, 0x75B9}, //6122 #CJK UNIFIED IDEOGRAPH
    {0xAF6D, 0x75C2}, //6123 #CJK UNIFIED IDEOGRAPH
    {0xAF6E, 0x75B8}, //6124 #CJK UNIFIED IDEOGRAPH
    {0xAF6F, 0x768B}, //6125 #CJK UNIFIED IDEOGRAPH
    {0xAF70, 0x76B0}, //6126 #CJK UNIFIED IDEOGRAPH
    {0xAF71, 0x76CA}, //6127 #CJK UNIFIED IDEOGRAPH
    {0xAF72, 0x76CD}, //6128 #CJK UNIFIED IDEOGRAPH
    {0xAF73, 0x76CE}, //6129 #CJK UNIFIED IDEOGRAPH
    {0xAF74, 0x7729}, //6130 #CJK UNIFIED IDEOGRAPH
    {0xAF75, 0x771F}, //6131 #CJK UNIFIED IDEOGRAPH
    {0xAF76, 0x7720}, //6132 #CJK UNIFIED IDEOGRAPH
    {0xAF77, 0x7728}, //6133 #CJK UNIFIED IDEOGRAPH
    {0xAF78, 0x77E9}, //6134 #CJK UNIFIED IDEOGRAPH
    {0xAF79, 0x7830}, //6135 #CJK UNIFIED IDEOGRAPH
    {0xAF7A, 0x7827}, //6136 #CJK UNIFIED IDEOGRAPH
    {0xAF7B, 0x7838}, //6137 #CJK UNIFIED IDEOGRAPH
    {0xAF7C, 0x781D}, //6138 #CJK UNIFIED IDEOGRAPH
    {0xAF7D, 0x7834}, //6139 #CJK UNIFIED IDEOGRAPH
    {0xAF7E, 0x7837}, //6140 #CJK UNIFIED IDEOGRAPH
    {0xAFA1, 0x7825}, //6141 #CJK UNIFIED IDEOGRAPH
    {0xAFA2, 0x782D}, //6142 #CJK UNIFIED IDEOGRAPH
    {0xAFA3, 0x7820}, //6143 #CJK UNIFIED IDEOGRAPH
    {0xAFA4, 0x781F}, //6144 #CJK UNIFIED IDEOGRAPH
    {0xAFA5, 0x7832}, //6145 #CJK UNIFIED IDEOGRAPH
    {0xAFA6, 0x7955}, //6146 #CJK UNIFIED IDEOGRAPH
    {0xAFA7, 0x7950}, //6147 #CJK UNIFIED IDEOGRAPH
    {0xAFA8, 0x7960}, //6148 #CJK UNIFIED IDEOGRAPH
    {0xAFA9, 0x795F}, //6149 #CJK UNIFIED IDEOGRAPH
    {0xAFAA, 0x7956}, //6150 #CJK UNIFIED IDEOGRAPH
    {0xAFAB, 0x795E}, //6151 #CJK UNIFIED IDEOGRAPH
    {0xAFAC, 0x795D}, //6152 #CJK UNIFIED IDEOGRAPH
    {0xAFAD, 0x7957}, //6153 #CJK UNIFIED IDEOGRAPH
    {0xAFAE, 0x795A}, //6154 #CJK UNIFIED IDEOGRAPH
    {0xAFAF, 0x79E4}, //6155 #CJK UNIFIED IDEOGRAPH
    {0xAFB0, 0x79E3}, //6156 #CJK UNIFIED IDEOGRAPH
    {0xAFB1, 0x79E7}, //6157 #CJK UNIFIED IDEOGRAPH
    {0xAFB2, 0x79DF}, //6158 #CJK UNIFIED IDEOGRAPH
    {0xAFB3, 0x79E6}, //6159 #CJK UNIFIED IDEOGRAPH
    {0xAFB4, 0x79E9}, //6160 #CJK UNIFIED IDEOGRAPH
    {0xAFB5, 0x79D8}, //6161 #CJK UNIFIED IDEOGRAPH
    {0xAFB6, 0x7A84}, //6162 #CJK UNIFIED IDEOGRAPH
    {0xAFB7, 0x7A88}, //6163 #CJK UNIFIED IDEOGRAPH
    {0xAFB8, 0x7AD9}, //6164 #CJK UNIFIED IDEOGRAPH
    {0xAFB9, 0x7B06}, //6165 #CJK UNIFIED IDEOGRAPH
    {0xAFBA, 0x7B11}, //6166 #CJK UNIFIED IDEOGRAPH
    {0xAFBB, 0x7C89}, //6167 #CJK UNIFIED IDEOGRAPH
    {0xAFBC, 0x7D21}, //6168 #CJK UNIFIED IDEOGRAPH
    {0xAFBD, 0x7D17}, //6169 #CJK UNIFIED IDEOGRAPH
    {0xAFBE, 0x7D0B}, //6170 #CJK UNIFIED IDEOGRAPH
    {0xAFBF, 0x7D0A}, //6171 #CJK UNIFIED IDEOGRAPH
    {0xAFC0, 0x7D20}, //6172 #CJK UNIFIED IDEOGRAPH
    {0xAFC1, 0x7D22}, //6173 #CJK UNIFIED IDEOGRAPH
    {0xAFC2, 0x7D14}, //6174 #CJK UNIFIED IDEOGRAPH
    {0xAFC3, 0x7D10}, //6175 #CJK UNIFIED IDEOGRAPH
    {0xAFC4, 0x7D15}, //6176 #CJK UNIFIED IDEOGRAPH
    {0xAFC5, 0x7D1A}, //6177 #CJK UNIFIED IDEOGRAPH
    {0xAFC6, 0x7D1C}, //6178 #CJK UNIFIED IDEOGRAPH
    {0xAFC7, 0x7D0D}, //6179 #CJK UNIFIED IDEOGRAPH
    {0xAFC8, 0x7D19}, //6180 #CJK UNIFIED IDEOGRAPH
    {0xAFC9, 0x7D1B}, //6181 #CJK UNIFIED IDEOGRAPH
    {0xAFCA, 0x7F3A}, //6182 #CJK UNIFIED IDEOGRAPH
    {0xAFCB, 0x7F5F}, //6183 #CJK UNIFIED IDEOGRAPH
    {0xAFCC, 0x7F94}, //6184 #CJK UNIFIED IDEOGRAPH
    {0xAFCD, 0x7FC5}, //6185 #CJK UNIFIED IDEOGRAPH
    {0xAFCE, 0x7FC1}, //6186 #CJK UNIFIED IDEOGRAPH
    {0xAFCF, 0x8006}, //6187 #CJK UNIFIED IDEOGRAPH
    {0xAFD0, 0x8018}, //6188 #CJK UNIFIED IDEOGRAPH
    {0xAFD1, 0x8015}, //6189 #CJK UNIFIED IDEOGRAPH
    {0xAFD2, 0x8019}, //6190 #CJK UNIFIED IDEOGRAPH
    {0xAFD3, 0x8017}, //6191 #CJK UNIFIED IDEOGRAPH
    {0xAFD4, 0x803D}, //6192 #CJK UNIFIED IDEOGRAPH
    {0xAFD5, 0x803F}, //6193 #CJK UNIFIED IDEOGRAPH
    {0xAFD6, 0x80F1}, //6194 #CJK UNIFIED IDEOGRAPH
    {0xAFD7, 0x8102}, //6195 #CJK UNIFIED IDEOGRAPH
    {0xAFD8, 0x80F0}, //6196 #CJK UNIFIED IDEOGRAPH
    {0xAFD9, 0x8105}, //6197 #CJK UNIFIED IDEOGRAPH
    {0xAFDA, 0x80ED}, //6198 #CJK UNIFIED IDEOGRAPH
    {0xAFDB, 0x80F4}, //6199 #CJK UNIFIED IDEOGRAPH
    {0xAFDC, 0x8106}, //6200 #CJK UNIFIED IDEOGRAPH
    {0xAFDD, 0x80F8}, //6201 #CJK UNIFIED IDEOGRAPH
    {0xAFDE, 0x80F3}, //6202 #CJK UNIFIED IDEOGRAPH
    {0xAFDF, 0x8108}, //6203 #CJK UNIFIED IDEOGRAPH
    {0xAFE0, 0x80FD}, //6204 #CJK UNIFIED IDEOGRAPH
    {0xAFE1, 0x810A}, //6205 #CJK UNIFIED IDEOGRAPH
    {0xAFE2, 0x80FC}, //6206 #CJK UNIFIED IDEOGRAPH
    {0xAFE3, 0x80EF}, //6207 #CJK UNIFIED IDEOGRAPH
    {0xAFE4, 0x81ED}, //6208 #CJK UNIFIED IDEOGRAPH
    {0xAFE5, 0x81EC}, //6209 #CJK UNIFIED IDEOGRAPH
    {0xAFE6, 0x8200}, //6210 #CJK UNIFIED IDEOGRAPH
    {0xAFE7, 0x8210}, //6211 #CJK UNIFIED IDEOGRAPH
    {0xAFE8, 0x822A}, //6212 #CJK UNIFIED IDEOGRAPH
    {0xAFE9, 0x822B}, //6213 #CJK UNIFIED IDEOGRAPH
    {0xAFEA, 0x8228}, //6214 #CJK UNIFIED IDEOGRAPH
    {0xAFEB, 0x822C}, //6215 #CJK UNIFIED IDEOGRAPH
    {0xAFEC, 0x82BB}, //6216 #CJK UNIFIED IDEOGRAPH
    {0xAFED, 0x832B}, //6217 #CJK UNIFIED IDEOGRAPH
    {0xAFEE, 0x8352}, //6218 #CJK UNIFIED IDEOGRAPH
    {0xAFEF, 0x8354}, //6219 #CJK UNIFIED IDEOGRAPH
    {0xAFF0, 0x834A}, //6220 #CJK UNIFIED IDEOGRAPH
    {0xAFF1, 0x8338}, //6221 #CJK UNIFIED IDEOGRAPH
    {0xAFF2, 0x8350}, //6222 #CJK UNIFIED IDEOGRAPH
    {0xAFF3, 0x8349}, //6223 #CJK UNIFIED IDEOGRAPH
    {0xAFF4, 0x8335}, //6224 #CJK UNIFIED IDEOGRAPH
    {0xAFF5, 0x8334}, //6225 #CJK UNIFIED IDEOGRAPH
    {0xAFF6, 0x834F}, //6226 #CJK UNIFIED IDEOGRAPH
    {0xAFF7, 0x8332}, //6227 #CJK UNIFIED IDEOGRAPH
    {0xAFF8, 0x8339}, //6228 #CJK UNIFIED IDEOGRAPH
    {0xAFF9, 0x8336}, //6229 #CJK UNIFIED IDEOGRAPH
    {0xAFFA, 0x8317}, //6230 #CJK UNIFIED IDEOGRAPH
    {0xAFFB, 0x8340}, //6231 #CJK UNIFIED IDEOGRAPH
    {0xAFFC, 0x8331}, //6232 #CJK UNIFIED IDEOGRAPH
    {0xAFFD, 0x8328}, //6233 #CJK UNIFIED IDEOGRAPH
    {0xAFFE, 0x8343}, //6234 #CJK UNIFIED IDEOGRAPH
    {0xB040, 0x8654}, //6235 #CJK UNIFIED IDEOGRAPH
    {0xB041, 0x868A}, //6236 #CJK UNIFIED IDEOGRAPH
    {0xB042, 0x86AA}, //6237 #CJK UNIFIED IDEOGRAPH
    {0xB043, 0x8693}, //6238 #CJK UNIFIED IDEOGRAPH
    {0xB044, 0x86A4}, //6239 #CJK UNIFIED IDEOGRAPH
    {0xB045, 0x86A9}, //6240 #CJK UNIFIED IDEOGRAPH
    {0xB046, 0x868C}, //6241 #CJK UNIFIED IDEOGRAPH
    {0xB047, 0x86A3}, //6242 #CJK UNIFIED IDEOGRAPH
    {0xB048, 0x869C}, //6243 #CJK UNIFIED IDEOGRAPH
    {0xB049, 0x8870}, //6244 #CJK UNIFIED IDEOGRAPH
    {0xB04A, 0x8877}, //6245 #CJK UNIFIED IDEOGRAPH
    {0xB04B, 0x8881}, //6246 #CJK UNIFIED IDEOGRAPH
    {0xB04C, 0x8882}, //6247 #CJK UNIFIED IDEOGRAPH
    {0xB04D, 0x887D}, //6248 #CJK UNIFIED IDEOGRAPH
    {0xB04E, 0x8879}, //6249 #CJK UNIFIED IDEOGRAPH
    {0xB04F, 0x8A18}, //6250 #CJK UNIFIED IDEOGRAPH
    {0xB050, 0x8A10}, //6251 #CJK UNIFIED IDEOGRAPH
    {0xB051, 0x8A0E}, //6252 #CJK UNIFIED IDEOGRAPH
    {0xB052, 0x8A0C}, //6253 #CJK UNIFIED IDEOGRAPH
    {0xB053, 0x8A15}, //6254 #CJK UNIFIED IDEOGRAPH
    {0xB054, 0x8A0A}, //6255 #CJK UNIFIED IDEOGRAPH
    {0xB055, 0x8A17}, //6256 #CJK UNIFIED IDEOGRAPH
    {0xB056, 0x8A13}, //6257 #CJK UNIFIED IDEOGRAPH
    {0xB057, 0x8A16}, //6258 #CJK UNIFIED IDEOGRAPH
    {0xB058, 0x8A0F}, //6259 #CJK UNIFIED IDEOGRAPH
    {0xB059, 0x8A11}, //6260 #CJK UNIFIED IDEOGRAPH
    {0xB05A, 0x8C48}, //6261 #CJK UNIFIED IDEOGRAPH
    {0xB05B, 0x8C7A}, //6262 #CJK UNIFIED IDEOGRAPH
    {0xB05C, 0x8C79}, //6263 #CJK UNIFIED IDEOGRAPH
    {0xB05D, 0x8CA1}, //6264 #CJK UNIFIED IDEOGRAPH
    {0xB05E, 0x8CA2}, //6265 #CJK UNIFIED IDEOGRAPH
    {0xB05F, 0x8D77}, //6266 #CJK UNIFIED IDEOGRAPH
    {0xB060, 0x8EAC}, //6267 #CJK UNIFIED IDEOGRAPH
    {0xB061, 0x8ED2}, //6268 #CJK UNIFIED IDEOGRAPH
    {0xB062, 0x8ED4}, //6269 #CJK UNIFIED IDEOGRAPH
    {0xB063, 0x8ECF}, //6270 #CJK UNIFIED IDEOGRAPH
    {0xB064, 0x8FB1}, //6271 #CJK UNIFIED IDEOGRAPH
    {0xB065, 0x9001}, //6272 #CJK UNIFIED IDEOGRAPH
    {0xB066, 0x9006}, //6273 #CJK UNIFIED IDEOGRAPH
    {0xB067, 0x8FF7}, //6274 #CJK UNIFIED IDEOGRAPH
    {0xB068, 0x9000}, //6275 #CJK UNIFIED IDEOGRAPH
    {0xB069, 0x8FFA}, //6276 #CJK UNIFIED IDEOGRAPH
    {0xB06A, 0x8FF4}, //6277 #CJK UNIFIED IDEOGRAPH
    {0xB06B, 0x9003}, //6278 #CJK UNIFIED IDEOGRAPH
    {0xB06C, 0x8FFD}, //6279 #CJK UNIFIED IDEOGRAPH
    {0xB06D, 0x9005}, //6280 #CJK UNIFIED IDEOGRAPH
    {0xB06E, 0x8FF8}, //6281 #CJK UNIFIED IDEOGRAPH
    {0xB06F, 0x9095}, //6282 #CJK UNIFIED IDEOGRAPH
    {0xB070, 0x90E1}, //6283 #CJK UNIFIED IDEOGRAPH
    {0xB071, 0x90DD}, //6284 #CJK UNIFIED IDEOGRAPH
    {0xB072, 0x90E2}, //6285 #CJK UNIFIED IDEOGRAPH
    {0xB073, 0x9152}, //6286 #CJK UNIFIED IDEOGRAPH
    {0xB074, 0x914D}, //6287 #CJK UNIFIED IDEOGRAPH
    {0xB075, 0x914C}, //6288 #CJK UNIFIED IDEOGRAPH
    {0xB076, 0x91D8}, //6289 #CJK UNIFIED IDEOGRAPH
    {0xB077, 0x91DD}, //6290 #CJK UNIFIED IDEOGRAPH
    {0xB078, 0x91D7}, //6291 #CJK UNIFIED IDEOGRAPH
    {0xB079, 0x91DC}, //6292 #CJK UNIFIED IDEOGRAPH
    {0xB07A, 0x91D9}, //6293 #CJK UNIFIED IDEOGRAPH
    {0xB07B, 0x9583}, //6294 #CJK UNIFIED IDEOGRAPH
    {0xB07C, 0x9662}, //6295 #CJK UNIFIED IDEOGRAPH
    {0xB07D, 0x9663}, //6296 #CJK UNIFIED IDEOGRAPH
    {0xB07E, 0x9661}, //6297 #CJK UNIFIED IDEOGRAPH
    {0xB0A1, 0x965B}, //6298 #CJK UNIFIED IDEOGRAPH
    {0xB0A2, 0x965D}, //6299 #CJK UNIFIED IDEOGRAPH
    {0xB0A3, 0x9664}, //6300 #CJK UNIFIED IDEOGRAPH
    {0xB0A4, 0x9658}, //6301 #CJK UNIFIED IDEOGRAPH
    {0xB0A5, 0x965E}, //6302 #CJK UNIFIED IDEOGRAPH
    {0xB0A6, 0x96BB}, //6303 #CJK UNIFIED IDEOGRAPH
    {0xB0A7, 0x98E2}, //6304 #CJK UNIFIED IDEOGRAPH
    {0xB0A8, 0x99AC}, //6305 #CJK UNIFIED IDEOGRAPH
    {0xB0A9, 0x9AA8}, //6306 #CJK UNIFIED IDEOGRAPH
    {0xB0AA, 0x9AD8}, //6307 #CJK UNIFIED IDEOGRAPH
    {0xB0AB, 0x9B25}, //6308 #CJK UNIFIED IDEOGRAPH
    {0xB0AC, 0x9B32}, //6309 #CJK UNIFIED IDEOGRAPH
    {0xB0AD, 0x9B3C}, //6310 #CJK UNIFIED IDEOGRAPH
    {0xB0AE, 0x4E7E}, //6311 #CJK UNIFIED IDEOGRAPH
    {0xB0AF, 0x507A}, //6312 #CJK UNIFIED IDEOGRAPH
    {0xB0B0, 0x507D}, //6313 #CJK UNIFIED IDEOGRAPH
    {0xB0B1, 0x505C}, //6314 #CJK UNIFIED IDEOGRAPH
    {0xB0B2, 0x5047}, //6315 #CJK UNIFIED IDEOGRAPH
    {0xB0B3, 0x5043}, //6316 #CJK UNIFIED IDEOGRAPH
    {0xB0B4, 0x504C}, //6317 #CJK UNIFIED IDEOGRAPH
    {0xB0B5, 0x505A}, //6318 #CJK UNIFIED IDEOGRAPH
    {0xB0B6, 0x5049}, //6319 #CJK UNIFIED IDEOGRAPH
    {0xB0B7, 0x5065}, //6320 #CJK UNIFIED IDEOGRAPH
    {0xB0B8, 0x5076}, //6321 #CJK UNIFIED IDEOGRAPH
    {0xB0B9, 0x504E}, //6322 #CJK UNIFIED IDEOGRAPH
    {0xB0BA, 0x5055}, //6323 #CJK UNIFIED IDEOGRAPH
    {0xB0BB, 0x5075}, //6324 #CJK UNIFIED IDEOGRAPH
    {0xB0BC, 0x5074}, //6325 #CJK UNIFIED IDEOGRAPH
    {0xB0BD, 0x5077}, //6326 #CJK UNIFIED IDEOGRAPH
    {0xB0BE, 0x504F}, //6327 #CJK UNIFIED IDEOGRAPH
    {0xB0BF, 0x500F}, //6328 #CJK UNIFIED IDEOGRAPH
    {0xB0C0, 0x506F}, //6329 #CJK UNIFIED IDEOGRAPH
    {0xB0C1, 0x506D}, //6330 #CJK UNIFIED IDEOGRAPH
    {0xB0C2, 0x515C}, //6331 #CJK UNIFIED IDEOGRAPH
    {0xB0C3, 0x5195}, //6332 #CJK UNIFIED IDEOGRAPH
    {0xB0C4, 0x51F0}, //6333 #CJK UNIFIED IDEOGRAPH
    {0xB0C5, 0x526A}, //6334 #CJK UNIFIED IDEOGRAPH
    {0xB0C6, 0x526F}, //6335 #CJK UNIFIED IDEOGRAPH
    {0xB0C7, 0x52D2}, //6336 #CJK UNIFIED IDEOGRAPH
    {0xB0C8, 0x52D9}, //6337 #CJK UNIFIED IDEOGRAPH
    {0xB0C9, 0x52D8}, //6338 #CJK UNIFIED IDEOGRAPH
    {0xB0CA, 0x52D5}, //6339 #CJK UNIFIED IDEOGRAPH
    {0xB0CB, 0x5310}, //6340 #CJK UNIFIED IDEOGRAPH
    {0xB0CC, 0x530F}, //6341 #CJK UNIFIED IDEOGRAPH
    {0xB0CD, 0x5319}, //6342 #CJK UNIFIED IDEOGRAPH
    {0xB0CE, 0x533F}, //6343 #CJK UNIFIED IDEOGRAPH
    {0xB0CF, 0x5340}, //6344 #CJK UNIFIED IDEOGRAPH
    {0xB0D0, 0x533E}, //6345 #CJK UNIFIED IDEOGRAPH
    {0xB0D1, 0x53C3}, //6346 #CJK UNIFIED IDEOGRAPH
    {0xB0D2, 0x66FC}, //6347 #CJK UNIFIED IDEOGRAPH
    {0xB0D3, 0x5546}, //6348 #CJK UNIFIED IDEOGRAPH
    {0xB0D4, 0x556A}, //6349 #CJK UNIFIED IDEOGRAPH
    {0xB0D5, 0x5566}, //6350 #CJK UNIFIED IDEOGRAPH
    {0xB0D6, 0x5544}, //6351 #CJK UNIFIED IDEOGRAPH
    {0xB0D7, 0x555E}, //6352 #CJK UNIFIED IDEOGRAPH
    {0xB0D8, 0x5561}, //6353 #CJK UNIFIED IDEOGRAPH
    {0xB0D9, 0x5543}, //6354 #CJK UNIFIED IDEOGRAPH
    {0xB0DA, 0x554A}, //6355 #CJK UNIFIED IDEOGRAPH
    {0xB0DB, 0x5531}, //6356 #CJK UNIFIED IDEOGRAPH
    {0xB0DC, 0x5556}, //6357 #CJK UNIFIED IDEOGRAPH
    {0xB0DD, 0x554F}, //6358 #CJK UNIFIED IDEOGRAPH
    {0xB0DE, 0x5555}, //6359 #CJK UNIFIED IDEOGRAPH
    {0xB0DF, 0x552F}, //6360 #CJK UNIFIED IDEOGRAPH
    {0xB0E0, 0x5564}, //6361 #CJK UNIFIED IDEOGRAPH
    {0xB0E1, 0x5538}, //6362 #CJK UNIFIED IDEOGRAPH
    {0xB0E2, 0x552E}, //6363 #CJK UNIFIED IDEOGRAPH
    {0xB0E3, 0x555C}, //6364 #CJK UNIFIED IDEOGRAPH
    {0xB0E4, 0x552C}, //6365 #CJK UNIFIED IDEOGRAPH
    {0xB0E5, 0x5563}, //6366 #CJK UNIFIED IDEOGRAPH
    {0xB0E6, 0x5533}, //6367 #CJK UNIFIED IDEOGRAPH
    {0xB0E7, 0x5541}, //6368 #CJK UNIFIED IDEOGRAPH
    {0xB0E8, 0x5557}, //6369 #CJK UNIFIED IDEOGRAPH
    {0xB0E9, 0x5708}, //6370 #CJK UNIFIED IDEOGRAPH
    {0xB0EA, 0x570B}, //6371 #CJK UNIFIED IDEOGRAPH
    {0xB0EB, 0x5709}, //6372 #CJK UNIFIED IDEOGRAPH
    {0xB0EC, 0x57DF}, //6373 #CJK UNIFIED IDEOGRAPH
    {0xB0ED, 0x5805}, //6374 #CJK UNIFIED IDEOGRAPH
    {0xB0EE, 0x580A}, //6375 #CJK UNIFIED IDEOGRAPH
    {0xB0EF, 0x5806}, //6376 #CJK UNIFIED IDEOGRAPH
    {0xB0F0, 0x57E0}, //6377 #CJK UNIFIED IDEOGRAPH
    {0xB0F1, 0x57E4}, //6378 #CJK UNIFIED IDEOGRAPH
    {0xB0F2, 0x57FA}, //6379 #CJK UNIFIED IDEOGRAPH
    {0xB0F3, 0x5802}, //6380 #CJK UNIFIED IDEOGRAPH
    {0xB0F4, 0x5835}, //6381 #CJK UNIFIED IDEOGRAPH
    {0xB0F5, 0x57F7}, //6382 #CJK UNIFIED IDEOGRAPH
    {0xB0F6, 0x57F9}, //6383 #CJK UNIFIED IDEOGRAPH
    {0xB0F7, 0x5920}, //6384 #CJK UNIFIED IDEOGRAPH
    {0xB0F8, 0x5962}, //6385 #CJK UNIFIED IDEOGRAPH
    {0xB0F9, 0x5A36}, //6386 #CJK UNIFIED IDEOGRAPH
    {0xB0FA, 0x5A41}, //6387 #CJK UNIFIED IDEOGRAPH
    {0xB0FB, 0x5A49}, //6388 #CJK UNIFIED IDEOGRAPH
    {0xB0FC, 0x5A66}, //6389 #CJK UNIFIED IDEOGRAPH
    {0xB0FD, 0x5A6A}, //6390 #CJK UNIFIED IDEOGRAPH
    {0xB0FE, 0x5A40}, //6391 #CJK UNIFIED IDEOGRAPH
    {0xB140, 0x5A3C}, //6392 #CJK UNIFIED IDEOGRAPH
    {0xB141, 0x5A62}, //6393 #CJK UNIFIED IDEOGRAPH
    {0xB142, 0x5A5A}, //6394 #CJK UNIFIED IDEOGRAPH
    {0xB143, 0x5A46}, //6395 #CJK UNIFIED IDEOGRAPH
    {0xB144, 0x5A4A}, //6396 #CJK UNIFIED IDEOGRAPH
    {0xB145, 0x5B70}, //6397 #CJK UNIFIED IDEOGRAPH
    {0xB146, 0x5BC7}, //6398 #CJK UNIFIED IDEOGRAPH
    {0xB147, 0x5BC5}, //6399 #CJK UNIFIED IDEOGRAPH
    {0xB148, 0x5BC4}, //6400 #CJK UNIFIED IDEOGRAPH
    {0xB149, 0x5BC2}, //6401 #CJK UNIFIED IDEOGRAPH
    {0xB14A, 0x5BBF}, //6402 #CJK UNIFIED IDEOGRAPH
    {0xB14B, 0x5BC6}, //6403 #CJK UNIFIED IDEOGRAPH
    {0xB14C, 0x5C09}, //6404 #CJK UNIFIED IDEOGRAPH
    {0xB14D, 0x5C08}, //6405 #CJK UNIFIED IDEOGRAPH
    {0xB14E, 0x5C07}, //6406 #CJK UNIFIED IDEOGRAPH
    {0xB14F, 0x5C60}, //6407 #CJK UNIFIED IDEOGRAPH
    {0xB150, 0x5C5C}, //6408 #CJK UNIFIED IDEOGRAPH
    {0xB151, 0x5C5D}, //6409 #CJK UNIFIED IDEOGRAPH
    {0xB152, 0x5D07}, //6410 #CJK UNIFIED IDEOGRAPH
    {0xB153, 0x5D06}, //6411 #CJK UNIFIED IDEOGRAPH
    {0xB154, 0x5D0E}, //6412 #CJK UNIFIED IDEOGRAPH
    {0xB155, 0x5D1B}, //6413 #CJK UNIFIED IDEOGRAPH
    {0xB156, 0x5D16}, //6414 #CJK UNIFIED IDEOGRAPH
    {0xB157, 0x5D22}, //6415 #CJK UNIFIED IDEOGRAPH
    {0xB158, 0x5D11}, //6416 #CJK UNIFIED IDEOGRAPH
    {0xB159, 0x5D29}, //6417 #CJK UNIFIED IDEOGRAPH
    {0xB15A, 0x5D14}, //6418 #CJK UNIFIED IDEOGRAPH
    {0xB15B, 0x5D19}, //6419 #CJK UNIFIED IDEOGRAPH
    {0xB15C, 0x5D24}, //6420 #CJK UNIFIED IDEOGRAPH
    {0xB15D, 0x5D27}, //6421 #CJK UNIFIED IDEOGRAPH
    {0xB15E, 0x5D17}, //6422 #CJK UNIFIED IDEOGRAPH
    {0xB15F, 0x5DE2}, //6423 #CJK UNIFIED IDEOGRAPH
    {0xB160, 0x5E38}, //6424 #CJK UNIFIED IDEOGRAPH
    {0xB161, 0x5E36}, //6425 #CJK UNIFIED IDEOGRAPH
    {0xB162, 0x5E33}, //6426 #CJK UNIFIED IDEOGRAPH
    {0xB163, 0x5E37}, //6427 #CJK UNIFIED IDEOGRAPH
    {0xB164, 0x5EB7}, //6428 #CJK UNIFIED IDEOGRAPH
    {0xB165, 0x5EB8}, //6429 #CJK UNIFIED IDEOGRAPH
    {0xB166, 0x5EB6}, //6430 #CJK UNIFIED IDEOGRAPH
    {0xB167, 0x5EB5}, //6431 #CJK UNIFIED IDEOGRAPH
    {0xB168, 0x5EBE}, //6432 #CJK UNIFIED IDEOGRAPH
    {0xB169, 0x5F35}, //6433 #CJK UNIFIED IDEOGRAPH
    {0xB16A, 0x5F37}, //6434 #CJK UNIFIED IDEOGRAPH
    {0xB16B, 0x5F57}, //6435 #CJK UNIFIED IDEOGRAPH
    {0xB16C, 0x5F6C}, //6436 #CJK UNIFIED IDEOGRAPH
    {0xB16D, 0x5F69}, //6437 #CJK UNIFIED IDEOGRAPH
    {0xB16E, 0x5F6B}, //6438 #CJK UNIFIED IDEOGRAPH
    {0xB16F, 0x5F97}, //6439 #CJK UNIFIED IDEOGRAPH
    {0xB170, 0x5F99}, //6440 #CJK UNIFIED IDEOGRAPH
    {0xB171, 0x5F9E}, //6441 #CJK UNIFIED IDEOGRAPH
    {0xB172, 0x5F98}, //6442 #CJK UNIFIED IDEOGRAPH
    {0xB173, 0x5FA1}, //6443 #CJK UNIFIED IDEOGRAPH
    {0xB174, 0x5FA0}, //6444 #CJK UNIFIED IDEOGRAPH
    {0xB175, 0x5F9C}, //6445 #CJK UNIFIED IDEOGRAPH
    {0xB176, 0x607F}, //6446 #CJK UNIFIED IDEOGRAPH
    {0xB177, 0x60A3}, //6447 #CJK UNIFIED IDEOGRAPH
    {0xB178, 0x6089}, //6448 #CJK UNIFIED IDEOGRAPH
    {0xB179, 0x60A0}, //6449 #CJK UNIFIED IDEOGRAPH
    {0xB17A, 0x60A8}, //6450 #CJK UNIFIED IDEOGRAPH
    {0xB17B, 0x60CB}, //6451 #CJK UNIFIED IDEOGRAPH
    {0xB17C, 0x60B4}, //6452 #CJK UNIFIED IDEOGRAPH
    {0xB17D, 0x60E6}, //6453 #CJK UNIFIED IDEOGRAPH
    {0xB17E, 0x60BD}, //6454 #CJK UNIFIED IDEOGRAPH
    {0xB1A1, 0x60C5}, //6455 #CJK UNIFIED IDEOGRAPH
    {0xB1A2, 0x60BB}, //6456 #CJK UNIFIED IDEOGRAPH
    {0xB1A3, 0x60B5}, //6457 #CJK UNIFIED IDEOGRAPH
    {0xB1A4, 0x60DC}, //6458 #CJK UNIFIED IDEOGRAPH
    {0xB1A5, 0x60BC}, //6459 #CJK UNIFIED IDEOGRAPH
    {0xB1A6, 0x60D8}, //6460 #CJK UNIFIED IDEOGRAPH
    {0xB1A7, 0x60D5}, //6461 #CJK UNIFIED IDEOGRAPH
    {0xB1A8, 0x60C6}, //6462 #CJK UNIFIED IDEOGRAPH
    {0xB1A9, 0x60DF}, //6463 #CJK UNIFIED IDEOGRAPH
    {0xB1AA, 0x60B8}, //6464 #CJK UNIFIED IDEOGRAPH
    {0xB1AB, 0x60DA}, //6465 #CJK UNIFIED IDEOGRAPH
    {0xB1AC, 0x60C7}, //6466 #CJK UNIFIED IDEOGRAPH
    {0xB1AD, 0x621A}, //6467 #CJK UNIFIED IDEOGRAPH
    {0xB1AE, 0x621B}, //6468 #CJK UNIFIED IDEOGRAPH
    {0xB1AF, 0x6248}, //6469 #CJK UNIFIED IDEOGRAPH
    {0xB1B0, 0x63A0}, //6470 #CJK UNIFIED IDEOGRAPH
    {0xB1B1, 0x63A7}, //6471 #CJK UNIFIED IDEOGRAPH
    {0xB1B2, 0x6372}, //6472 #CJK UNIFIED IDEOGRAPH
    {0xB1B3, 0x6396}, //6473 #CJK UNIFIED IDEOGRAPH
    {0xB1B4, 0x63A2}, //6474 #CJK UNIFIED IDEOGRAPH
    {0xB1B5, 0x63A5}, //6475 #CJK UNIFIED IDEOGRAPH
    {0xB1B6, 0x6377}, //6476 #CJK UNIFIED IDEOGRAPH
    {0xB1B7, 0x6367}, //6477 #CJK UNIFIED IDEOGRAPH
    {0xB1B8, 0x6398}, //6478 #CJK UNIFIED IDEOGRAPH
    {0xB1B9, 0x63AA}, //6479 #CJK UNIFIED IDEOGRAPH
    {0xB1BA, 0x6371}, //6480 #CJK UNIFIED IDEOGRAPH
    {0xB1BB, 0x63A9}, //6481 #CJK UNIFIED IDEOGRAPH
    {0xB1BC, 0x6389}, //6482 #CJK UNIFIED IDEOGRAPH
    {0xB1BD, 0x6383}, //6483 #CJK UNIFIED IDEOGRAPH
    {0xB1BE, 0x639B}, //6484 #CJK UNIFIED IDEOGRAPH
    {0xB1BF, 0x636B}, //6485 #CJK UNIFIED IDEOGRAPH
    {0xB1C0, 0x63A8}, //6486 #CJK UNIFIED IDEOGRAPH
    {0xB1C1, 0x6384}, //6487 #CJK UNIFIED IDEOGRAPH
    {0xB1C2, 0x6388}, //6488 #CJK UNIFIED IDEOGRAPH
    {0xB1C3, 0x6399}, //6489 #CJK UNIFIED IDEOGRAPH
    {0xB1C4, 0x63A1}, //6490 #CJK UNIFIED IDEOGRAPH
    {0xB1C5, 0x63AC}, //6491 #CJK UNIFIED IDEOGRAPH
    {0xB1C6, 0x6392}, //6492 #CJK UNIFIED IDEOGRAPH
    {0xB1C7, 0x638F}, //6493 #CJK UNIFIED IDEOGRAPH
    {0xB1C8, 0x6380}, //6494 #CJK UNIFIED IDEOGRAPH
    {0xB1C9, 0x637B}, //6495 #CJK UNIFIED IDEOGRAPH
    {0xB1CA, 0x6369}, //6496 #CJK UNIFIED IDEOGRAPH
    {0xB1CB, 0x6368}, //6497 #CJK UNIFIED IDEOGRAPH
    {0xB1CC, 0x637A}, //6498 #CJK UNIFIED IDEOGRAPH
    {0xB1CD, 0x655D}, //6499 #CJK UNIFIED IDEOGRAPH
    {0xB1CE, 0x6556}, //6500 #CJK UNIFIED IDEOGRAPH
    {0xB1CF, 0x6551}, //6501 #CJK UNIFIED IDEOGRAPH
    {0xB1D0, 0x6559}, //6502 #CJK UNIFIED IDEOGRAPH
    {0xB1D1, 0x6557}, //6503 #CJK UNIFIED IDEOGRAPH
    {0xB1D2, 0x555F}, //6504 #CJK UNIFIED IDEOGRAPH
    {0xB1D3, 0x654F}, //6505 #CJK UNIFIED IDEOGRAPH
    {0xB1D4, 0x6558}, //6506 #CJK UNIFIED IDEOGRAPH
    {0xB1D5, 0x6555}, //6507 #CJK UNIFIED IDEOGRAPH
    {0xB1D6, 0x6554}, //6508 #CJK UNIFIED IDEOGRAPH
    {0xB1D7, 0x659C}, //6509 #CJK UNIFIED IDEOGRAPH
    {0xB1D8, 0x659B}, //6510 #CJK UNIFIED IDEOGRAPH
    {0xB1D9, 0x65AC}, //6511 #CJK UNIFIED IDEOGRAPH
    {0xB1DA, 0x65CF}, //6512 #CJK UNIFIED IDEOGRAPH
    {0xB1DB, 0x65CB}, //6513 #CJK UNIFIED IDEOGRAPH
    {0xB1DC, 0x65CC}, //6514 #CJK UNIFIED IDEOGRAPH
    {0xB1DD, 0x65CE}, //6515 #CJK UNIFIED IDEOGRAPH
    {0xB1DE, 0x665D}, //6516 #CJK UNIFIED IDEOGRAPH
    {0xB1DF, 0x665A}, //6517 #CJK UNIFIED IDEOGRAPH
    {0xB1E0, 0x6664}, //6518 #CJK UNIFIED IDEOGRAPH
    {0xB1E1, 0x6668}, //6519 #CJK UNIFIED IDEOGRAPH
    {0xB1E2, 0x6666}, //6520 #CJK UNIFIED IDEOGRAPH
    {0xB1E3, 0x665E}, //6521 #CJK UNIFIED IDEOGRAPH
    {0xB1E4, 0x66F9}, //6522 #CJK UNIFIED IDEOGRAPH
    {0xB1E5, 0x52D7}, //6523 #CJK UNIFIED IDEOGRAPH
    {0xB1E6, 0x671B}, //6524 #CJK UNIFIED IDEOGRAPH
    {0xB1E7, 0x6881}, //6525 #CJK UNIFIED IDEOGRAPH
    {0xB1E8, 0x68AF}, //6526 #CJK UNIFIED IDEOGRAPH
    {0xB1E9, 0x68A2}, //6527 #CJK UNIFIED IDEOGRAPH
    {0xB1EA, 0x6893}, //6528 #CJK UNIFIED IDEOGRAPH
    {0xB1EB, 0x68B5}, //6529 #CJK UNIFIED IDEOGRAPH
    {0xB1EC, 0x687F}, //6530 #CJK UNIFIED IDEOGRAPH
    {0xB1ED, 0x6876}, //6531 #CJK UNIFIED IDEOGRAPH
    {0xB1EE, 0x68B1}, //6532 #CJK UNIFIED IDEOGRAPH
    {0xB1EF, 0x68A7}, //6533 #CJK UNIFIED IDEOGRAPH
    {0xB1F0, 0x6897}, //6534 #CJK UNIFIED IDEOGRAPH
    {0xB1F1, 0x68B0}, //6535 #CJK UNIFIED IDEOGRAPH
    {0xB1F2, 0x6883}, //6536 #CJK UNIFIED IDEOGRAPH
    {0xB1F3, 0x68C4}, //6537 #CJK UNIFIED IDEOGRAPH
    {0xB1F4, 0x68AD}, //6538 #CJK UNIFIED IDEOGRAPH
    {0xB1F5, 0x6886}, //6539 #CJK UNIFIED IDEOGRAPH
    {0xB1F6, 0x6885}, //6540 #CJK UNIFIED IDEOGRAPH
    {0xB1F7, 0x6894}, //6541 #CJK UNIFIED IDEOGRAPH
    {0xB1F8, 0x689D}, //6542 #CJK UNIFIED IDEOGRAPH
    {0xB1F9, 0x68A8}, //6543 #CJK UNIFIED IDEOGRAPH
    {0xB1FA, 0x689F}, //6544 #CJK UNIFIED IDEOGRAPH
    {0xB1FB, 0x68A1}, //6545 #CJK UNIFIED IDEOGRAPH
    {0xB1FC, 0x6882}, //6546 #CJK UNIFIED IDEOGRAPH
    {0xB1FD, 0x6B32}, //6547 #CJK UNIFIED IDEOGRAPH
    {0xB1FE, 0x6BBA}, //6548 #CJK UNIFIED IDEOGRAPH
    {0xB240, 0x6BEB}, //6549 #CJK UNIFIED IDEOGRAPH
    {0xB241, 0x6BEC}, //6550 #CJK UNIFIED IDEOGRAPH
    {0xB242, 0x6C2B}, //6551 #CJK UNIFIED IDEOGRAPH
    {0xB243, 0x6D8E}, //6552 #CJK UNIFIED IDEOGRAPH
    {0xB244, 0x6DBC}, //6553 #CJK UNIFIED IDEOGRAPH
    {0xB245, 0x6DF3}, //6554 #CJK UNIFIED IDEOGRAPH
    {0xB246, 0x6DD9}, //6555 #CJK UNIFIED IDEOGRAPH
    {0xB247, 0x6DB2}, //6556 #CJK UNIFIED IDEOGRAPH
    {0xB248, 0x6DE1}, //6557 #CJK UNIFIED IDEOGRAPH
    {0xB249, 0x6DCC}, //6558 #CJK UNIFIED IDEOGRAPH
    {0xB24A, 0x6DE4}, //6559 #CJK UNIFIED IDEOGRAPH
    {0xB24B, 0x6DFB}, //6560 #CJK UNIFIED IDEOGRAPH
    {0xB24C, 0x6DFA}, //6561 #CJK UNIFIED IDEOGRAPH
    {0xB24D, 0x6E05}, //6562 #CJK UNIFIED IDEOGRAPH
    {0xB24E, 0x6DC7}, //6563 #CJK UNIFIED IDEOGRAPH
    {0xB24F, 0x6DCB}, //6564 #CJK UNIFIED IDEOGRAPH
    {0xB250, 0x6DAF}, //6565 #CJK UNIFIED IDEOGRAPH
    {0xB251, 0x6DD1}, //6566 #CJK UNIFIED IDEOGRAPH
    {0xB252, 0x6DAE}, //6567 #CJK UNIFIED IDEOGRAPH
    {0xB253, 0x6DDE}, //6568 #CJK UNIFIED IDEOGRAPH
    {0xB254, 0x6DF9}, //6569 #CJK UNIFIED IDEOGRAPH
    {0xB255, 0x6DB8}, //6570 #CJK UNIFIED IDEOGRAPH
    {0xB256, 0x6DF7}, //6571 #CJK UNIFIED IDEOGRAPH
    {0xB257, 0x6DF5}, //6572 #CJK UNIFIED IDEOGRAPH
    {0xB258, 0x6DC5}, //6573 #CJK UNIFIED IDEOGRAPH
    {0xB259, 0x6DD2}, //6574 #CJK UNIFIED IDEOGRAPH
    {0xB25A, 0x6E1A}, //6575 #CJK UNIFIED IDEOGRAPH
    {0xB25B, 0x6DB5}, //6576 #CJK UNIFIED IDEOGRAPH
    {0xB25C, 0x6DDA}, //6577 #CJK UNIFIED IDEOGRAPH
    {0xB25D, 0x6DEB}, //6578 #CJK UNIFIED IDEOGRAPH
    {0xB25E, 0x6DD8}, //6579 #CJK UNIFIED IDEOGRAPH
    {0xB25F, 0x6DEA}, //6580 #CJK UNIFIED IDEOGRAPH
    {0xB260, 0x6DF1}, //6581 #CJK UNIFIED IDEOGRAPH
    {0xB261, 0x6DEE}, //6582 #CJK UNIFIED IDEOGRAPH
    {0xB262, 0x6DE8}, //6583 #CJK UNIFIED IDEOGRAPH
    {0xB263, 0x6DC6}, //6584 #CJK UNIFIED IDEOGRAPH
    {0xB264, 0x6DC4}, //6585 #CJK UNIFIED IDEOGRAPH
    {0xB265, 0x6DAA}, //6586 #CJK UNIFIED IDEOGRAPH
    {0xB266, 0x6DEC}, //6587 #CJK UNIFIED IDEOGRAPH
    {0xB267, 0x6DBF}, //6588 #CJK UNIFIED IDEOGRAPH
    {0xB268, 0x6DE6}, //6589 #CJK UNIFIED IDEOGRAPH
    {0xB269, 0x70F9}, //6590 #CJK UNIFIED IDEOGRAPH
    {0xB26A, 0x7109}, //6591 #CJK UNIFIED IDEOGRAPH
    {0xB26B, 0x710A}, //6592 #CJK UNIFIED IDEOGRAPH
    {0xB26C, 0x70FD}, //6593 #CJK UNIFIED IDEOGRAPH
    {0xB26D, 0x70EF}, //6594 #CJK UNIFIED IDEOGRAPH
    {0xB26E, 0x723D}, //6595 #CJK UNIFIED IDEOGRAPH
    {0xB26F, 0x727D}, //6596 #CJK UNIFIED IDEOGRAPH
    {0xB270, 0x7281}, //6597 #CJK UNIFIED IDEOGRAPH
    {0xB271, 0x731C}, //6598 #CJK UNIFIED IDEOGRAPH
    {0xB272, 0x731B}, //6599 #CJK UNIFIED IDEOGRAPH
    {0xB273, 0x7316}, //6600 #CJK UNIFIED IDEOGRAPH
    {0xB274, 0x7313}, //6601 #CJK UNIFIED IDEOGRAPH
    {0xB275, 0x7319}, //6602 #CJK UNIFIED IDEOGRAPH
    {0xB276, 0x7387}, //6603 #CJK UNIFIED IDEOGRAPH
    {0xB277, 0x7405}, //6604 #CJK UNIFIED IDEOGRAPH
    {0xB278, 0x740A}, //6605 #CJK UNIFIED IDEOGRAPH
    {0xB279, 0x7403}, //6606 #CJK UNIFIED IDEOGRAPH
    {0xB27A, 0x7406}, //6607 #CJK UNIFIED IDEOGRAPH
    {0xB27B, 0x73FE}, //6608 #CJK UNIFIED IDEOGRAPH
    {0xB27C, 0x740D}, //6609 #CJK UNIFIED IDEOGRAPH
    {0xB27D, 0x74E0}, //6610 #CJK UNIFIED IDEOGRAPH
    {0xB27E, 0x74F6}, //6611 #CJK UNIFIED IDEOGRAPH
    {0xB2A1, 0x74F7}, //6612 #CJK UNIFIED IDEOGRAPH
    {0xB2A2, 0x751C}, //6613 #CJK UNIFIED IDEOGRAPH
    {0xB2A3, 0x7522}, //6614 #CJK UNIFIED IDEOGRAPH
    {0xB2A4, 0x7565}, //6615 #CJK UNIFIED IDEOGRAPH
    {0xB2A5, 0x7566}, //6616 #CJK UNIFIED IDEOGRAPH
    {0xB2A6, 0x7562}, //6617 #CJK UNIFIED IDEOGRAPH
    {0xB2A7, 0x7570}, //6618 #CJK UNIFIED IDEOGRAPH
    {0xB2A8, 0x758F}, //6619 #CJK UNIFIED IDEOGRAPH
    {0xB2A9, 0x75D4}, //6620 #CJK UNIFIED IDEOGRAPH
    {0xB2AA, 0x75D5}, //6621 #CJK UNIFIED IDEOGRAPH
    {0xB2AB, 0x75B5}, //6622 #CJK UNIFIED IDEOGRAPH
    {0xB2AC, 0x75CA}, //6623 #CJK UNIFIED IDEOGRAPH
    {0xB2AD, 0x75CD}, //6624 #CJK UNIFIED IDEOGRAPH
    {0xB2AE, 0x768E}, //6625 #CJK UNIFIED IDEOGRAPH
    {0xB2AF, 0x76D4}, //6626 #CJK UNIFIED IDEOGRAPH
    {0xB2B0, 0x76D2}, //6627 #CJK UNIFIED IDEOGRAPH
    {0xB2B1, 0x76DB}, //6628 #CJK UNIFIED IDEOGRAPH
    {0xB2B2, 0x7737}, //6629 #CJK UNIFIED IDEOGRAPH
    {0xB2B3, 0x773E}, //6630 #CJK UNIFIED IDEOGRAPH
    {0xB2B4, 0x773C}, //6631 #CJK UNIFIED IDEOGRAPH
    {0xB2B5, 0x7736}, //6632 #CJK UNIFIED IDEOGRAPH
    {0xB2B6, 0x7738}, //6633 #CJK UNIFIED IDEOGRAPH
    {0xB2B7, 0x773A}, //6634 #CJK UNIFIED IDEOGRAPH
    {0xB2B8, 0x786B}, //6635 #CJK UNIFIED IDEOGRAPH
    {0xB2B9, 0x7843}, //6636 #CJK UNIFIED IDEOGRAPH
    {0xB2BA, 0x784E}, //6637 #CJK UNIFIED IDEOGRAPH
    {0xB2BB, 0x7965}, //6638 #CJK UNIFIED IDEOGRAPH
    {0xB2BC, 0x7968}, //6639 #CJK UNIFIED IDEOGRAPH
    {0xB2BD, 0x796D}, //6640 #CJK UNIFIED IDEOGRAPH
    {0xB2BE, 0x79FB}, //6641 #CJK UNIFIED IDEOGRAPH
    {0xB2BF, 0x7A92}, //6642 #CJK UNIFIED IDEOGRAPH
    {0xB2C0, 0x7A95}, //6643 #CJK UNIFIED IDEOGRAPH
    {0xB2C1, 0x7B20}, //6644 #CJK UNIFIED IDEOGRAPH
    {0xB2C2, 0x7B28}, //6645 #CJK UNIFIED IDEOGRAPH
    {0xB2C3, 0x7B1B}, //6646 #CJK UNIFIED IDEOGRAPH
    {0xB2C4, 0x7B2C}, //6647 #CJK UNIFIED IDEOGRAPH
    {0xB2C5, 0x7B26}, //6648 #CJK UNIFIED IDEOGRAPH
    {0xB2C6, 0x7B19}, //6649 #CJK UNIFIED IDEOGRAPH
    {0xB2C7, 0x7B1E}, //6650 #CJK UNIFIED IDEOGRAPH
    {0xB2C8, 0x7B2E}, //6651 #CJK UNIFIED IDEOGRAPH
    {0xB2C9, 0x7C92}, //6652 #CJK UNIFIED IDEOGRAPH
    {0xB2CA, 0x7C97}, //6653 #CJK UNIFIED IDEOGRAPH
    {0xB2CB, 0x7C95}, //6654 #CJK UNIFIED IDEOGRAPH
    {0xB2CC, 0x7D46}, //6655 #CJK UNIFIED IDEOGRAPH
    {0xB2CD, 0x7D43}, //6656 #CJK UNIFIED IDEOGRAPH
    {0xB2CE, 0x7D71}, //6657 #CJK UNIFIED IDEOGRAPH
    {0xB2CF, 0x7D2E}, //6658 #CJK UNIFIED IDEOGRAPH
    {0xB2D0, 0x7D39}, //6659 #CJK UNIFIED IDEOGRAPH
    {0xB2D1, 0x7D3C}, //6660 #CJK UNIFIED IDEOGRAPH
    {0xB2D2, 0x7D40}, //6661 #CJK UNIFIED IDEOGRAPH
    {0xB2D3, 0x7D30}, //6662 #CJK UNIFIED IDEOGRAPH
    {0xB2D4, 0x7D33}, //6663 #CJK UNIFIED IDEOGRAPH
    {0xB2D5, 0x7D44}, //6664 #CJK UNIFIED IDEOGRAPH
    {0xB2D6, 0x7D2F}, //6665 #CJK UNIFIED IDEOGRAPH
    {0xB2D7, 0x7D42}, //6666 #CJK UNIFIED IDEOGRAPH
    {0xB2D8, 0x7D32}, //6667 #CJK UNIFIED IDEOGRAPH
    {0xB2D9, 0x7D31}, //6668 #CJK UNIFIED IDEOGRAPH
    {0xB2DA, 0x7F3D}, //6669 #CJK UNIFIED IDEOGRAPH
    {0xB2DB, 0x7F9E}, //6670 #CJK UNIFIED IDEOGRAPH
    {0xB2DC, 0x7F9A}, //6671 #CJK UNIFIED IDEOGRAPH
    {0xB2DD, 0x7FCC}, //6672 #CJK UNIFIED IDEOGRAPH
    {0xB2DE, 0x7FCE}, //6673 #CJK UNIFIED IDEOGRAPH
    {0xB2DF, 0x7FD2}, //6674 #CJK UNIFIED IDEOGRAPH
    {0xB2E0, 0x801C}, //6675 #CJK UNIFIED IDEOGRAPH
    {0xB2E1, 0x804A}, //6676 #CJK UNIFIED IDEOGRAPH
    {0xB2E2, 0x8046}, //6677 #CJK UNIFIED IDEOGRAPH
    {0xB2E3, 0x812F}, //6678 #CJK UNIFIED IDEOGRAPH
    {0xB2E4, 0x8116}, //6679 #CJK UNIFIED IDEOGRAPH
    {0xB2E5, 0x8123}, //6680 #CJK UNIFIED IDEOGRAPH
    {0xB2E6, 0x812B}, //6681 #CJK UNIFIED IDEOGRAPH
    {0xB2E7, 0x8129}, //6682 #CJK UNIFIED IDEOGRAPH
    {0xB2E8, 0x8130}, //6683 #CJK UNIFIED IDEOGRAPH
    {0xB2E9, 0x8124}, //6684 #CJK UNIFIED IDEOGRAPH
    {0xB2EA, 0x8202}, //6685 #CJK UNIFIED IDEOGRAPH
    {0xB2EB, 0x8235}, //6686 #CJK UNIFIED IDEOGRAPH
    {0xB2EC, 0x8237}, //6687 #CJK UNIFIED IDEOGRAPH
    {0xB2ED, 0x8236}, //6688 #CJK UNIFIED IDEOGRAPH
    {0xB2EE, 0x8239}, //6689 #CJK UNIFIED IDEOGRAPH
    {0xB2EF, 0x838E}, //6690 #CJK UNIFIED IDEOGRAPH
    {0xB2F0, 0x839E}, //6691 #CJK UNIFIED IDEOGRAPH
    {0xB2F1, 0x8398}, //6692 #CJK UNIFIED IDEOGRAPH
    {0xB2F2, 0x8378}, //6693 #CJK UNIFIED IDEOGRAPH
    {0xB2F3, 0x83A2}, //6694 #CJK UNIFIED IDEOGRAPH
    {0xB2F4, 0x8396}, //6695 #CJK UNIFIED IDEOGRAPH
    {0xB2F5, 0x83BD}, //6696 #CJK UNIFIED IDEOGRAPH
    {0xB2F6, 0x83AB}, //6697 #CJK UNIFIED IDEOGRAPH
    {0xB2F7, 0x8392}, //6698 #CJK UNIFIED IDEOGRAPH
    {0xB2F8, 0x838A}, //6699 #CJK UNIFIED IDEOGRAPH
    {0xB2F9, 0x8393}, //6700 #CJK UNIFIED IDEOGRAPH
    {0xB2FA, 0x8389}, //6701 #CJK UNIFIED IDEOGRAPH
    {0xB2FB, 0x83A0}, //6702 #CJK UNIFIED IDEOGRAPH
    {0xB2FC, 0x8377}, //6703 #CJK UNIFIED IDEOGRAPH
    {0xB2FD, 0x837B}, //6704 #CJK UNIFIED IDEOGRAPH
    {0xB2FE, 0x837C}, //6705 #CJK UNIFIED IDEOGRAPH
    {0xB340, 0x8386}, //6706 #CJK UNIFIED IDEOGRAPH
    {0xB341, 0x83A7}, //6707 #CJK UNIFIED IDEOGRAPH
    {0xB342, 0x8655}, //6708 #CJK UNIFIED IDEOGRAPH
    {0xB343, 0x5F6A}, //6709 #CJK UNIFIED IDEOGRAPH
    {0xB344, 0x86C7}, //6710 #CJK UNIFIED IDEOGRAPH
    {0xB345, 0x86C0}, //6711 #CJK UNIFIED IDEOGRAPH
    {0xB346, 0x86B6}, //6712 #CJK UNIFIED IDEOGRAPH
    {0xB347, 0x86C4}, //6713 #CJK UNIFIED IDEOGRAPH
    {0xB348, 0x86B5}, //6714 #CJK UNIFIED IDEOGRAPH
    {0xB349, 0x86C6}, //6715 #CJK UNIFIED IDEOGRAPH
    {0xB34A, 0x86CB}, //6716 #CJK UNIFIED IDEOGRAPH
    {0xB34B, 0x86B1}, //6717 #CJK UNIFIED IDEOGRAPH
    {0xB34C, 0x86AF}, //6718 #CJK UNIFIED IDEOGRAPH
    {0xB34D, 0x86C9}, //6719 #CJK UNIFIED IDEOGRAPH
    {0xB34E, 0x8853}, //6720 #CJK UNIFIED IDEOGRAPH
    {0xB34F, 0x889E}, //6721 #CJK UNIFIED IDEOGRAPH
    {0xB350, 0x8888}, //6722 #CJK UNIFIED IDEOGRAPH
    {0xB351, 0x88AB}, //6723 #CJK UNIFIED IDEOGRAPH
    {0xB352, 0x8892}, //6724 #CJK UNIFIED IDEOGRAPH
    {0xB353, 0x8896}, //6725 #CJK UNIFIED IDEOGRAPH
    {0xB354, 0x888D}, //6726 #CJK UNIFIED IDEOGRAPH
    {0xB355, 0x888B}, //6727 #CJK UNIFIED IDEOGRAPH
    {0xB356, 0x8993}, //6728 #CJK UNIFIED IDEOGRAPH
    {0xB357, 0x898F}, //6729 #CJK UNIFIED IDEOGRAPH
    {0xB358, 0x8A2A}, //6730 #CJK UNIFIED IDEOGRAPH
    {0xB359, 0x8A1D}, //6731 #CJK UNIFIED IDEOGRAPH
    {0xB35A, 0x8A23}, //6732 #CJK UNIFIED IDEOGRAPH
    {0xB35B, 0x8A25}, //6733 #CJK UNIFIED IDEOGRAPH
    {0xB35C, 0x8A31}, //6734 #CJK UNIFIED IDEOGRAPH
    {0xB35D, 0x8A2D}, //6735 #CJK UNIFIED IDEOGRAPH
    {0xB35E, 0x8A1F}, //6736 #CJK UNIFIED IDEOGRAPH
    {0xB35F, 0x8A1B}, //6737 #CJK UNIFIED IDEOGRAPH
    {0xB360, 0x8A22}, //6738 #CJK UNIFIED IDEOGRAPH
    {0xB361, 0x8C49}, //6739 #CJK UNIFIED IDEOGRAPH
    {0xB362, 0x8C5A}, //6740 #CJK UNIFIED IDEOGRAPH
    {0xB363, 0x8CA9}, //6741 #CJK UNIFIED IDEOGRAPH
    {0xB364, 0x8CAC}, //6742 #CJK UNIFIED IDEOGRAPH
    {0xB365, 0x8CAB}, //6743 #CJK UNIFIED IDEOGRAPH
    {0xB366, 0x8CA8}, //6744 #CJK UNIFIED IDEOGRAPH
    {0xB367, 0x8CAA}, //6745 #CJK UNIFIED IDEOGRAPH
    {0xB368, 0x8CA7}, //6746 #CJK UNIFIED IDEOGRAPH
    {0xB369, 0x8D67}, //6747 #CJK UNIFIED IDEOGRAPH
    {0xB36A, 0x8D66}, //6748 #CJK UNIFIED IDEOGRAPH
    {0xB36B, 0x8DBE}, //6749 #CJK UNIFIED IDEOGRAPH
    {0xB36C, 0x8DBA}, //6750 #CJK UNIFIED IDEOGRAPH
    {0xB36D, 0x8EDB}, //6751 #CJK UNIFIED IDEOGRAPH
    {0xB36E, 0x8EDF}, //6752 #CJK UNIFIED IDEOGRAPH
    {0xB36F, 0x9019}, //6753 #CJK UNIFIED IDEOGRAPH
    {0xB370, 0x900D}, //6754 #CJK UNIFIED IDEOGRAPH
    {0xB371, 0x901A}, //6755 #CJK UNIFIED IDEOGRAPH
    {0xB372, 0x9017}, //6756 #CJK UNIFIED IDEOGRAPH
    {0xB373, 0x9023}, //6757 #CJK UNIFIED IDEOGRAPH
    {0xB374, 0x901F}, //6758 #CJK UNIFIED IDEOGRAPH
    {0xB375, 0x901D}, //6759 #CJK UNIFIED IDEOGRAPH
    {0xB376, 0x9010}, //6760 #CJK UNIFIED IDEOGRAPH
    {0xB377, 0x9015}, //6761 #CJK UNIFIED IDEOGRAPH
    {0xB378, 0x901E}, //6762 #CJK UNIFIED IDEOGRAPH
    {0xB379, 0x9020}, //6763 #CJK UNIFIED IDEOGRAPH
    {0xB37A, 0x900F}, //6764 #CJK UNIFIED IDEOGRAPH
    {0xB37B, 0x9022}, //6765 #CJK UNIFIED IDEOGRAPH
    {0xB37C, 0x9016}, //6766 #CJK UNIFIED IDEOGRAPH
    {0xB37D, 0x901B}, //6767 #CJK UNIFIED IDEOGRAPH
    {0xB37E, 0x9014}, //6768 #CJK UNIFIED IDEOGRAPH
    {0xB3A1, 0x90E8}, //6769 #CJK UNIFIED IDEOGRAPH
    {0xB3A2, 0x90ED}, //6770 #CJK UNIFIED IDEOGRAPH
    {0xB3A3, 0x90FD}, //6771 #CJK UNIFIED IDEOGRAPH
    {0xB3A4, 0x9157}, //6772 #CJK UNIFIED IDEOGRAPH
    {0xB3A5, 0x91CE}, //6773 #CJK UNIFIED IDEOGRAPH
    {0xB3A6, 0x91F5}, //6774 #CJK UNIFIED IDEOGRAPH
    {0xB3A7, 0x91E6}, //6775 #CJK UNIFIED IDEOGRAPH
    {0xB3A8, 0x91E3}, //6776 #CJK UNIFIED IDEOGRAPH
    {0xB3A9, 0x91E7}, //6777 #CJK UNIFIED IDEOGRAPH
    {0xB3AA, 0x91ED}, //6778 #CJK UNIFIED IDEOGRAPH
    {0xB3AB, 0x91E9}, //6779 #CJK UNIFIED IDEOGRAPH
    {0xB3AC, 0x9589}, //6780 #CJK UNIFIED IDEOGRAPH
    {0xB3AD, 0x966A}, //6781 #CJK UNIFIED IDEOGRAPH
    {0xB3AE, 0x9675}, //6782 #CJK UNIFIED IDEOGRAPH
    {0xB3AF, 0x9673}, //6783 #CJK UNIFIED IDEOGRAPH
    {0xB3B0, 0x9678}, //6784 #CJK UNIFIED IDEOGRAPH
    {0xB3B1, 0x9670}, //6785 #CJK UNIFIED IDEOGRAPH
    {0xB3B2, 0x9674}, //6786 #CJK UNIFIED IDEOGRAPH
    {0xB3B3, 0x9676}, //6787 #CJK UNIFIED IDEOGRAPH
    {0xB3B4, 0x9677}, //6788 #CJK UNIFIED IDEOGRAPH
    {0xB3B5, 0x966C}, //6789 #CJK UNIFIED IDEOGRAPH
    {0xB3B6, 0x96C0}, //6790 #CJK UNIFIED IDEOGRAPH
    {0xB3B7, 0x96EA}, //6791 #CJK UNIFIED IDEOGRAPH
    {0xB3B8, 0x96E9}, //6792 #CJK UNIFIED IDEOGRAPH
    {0xB3B9, 0x7AE0}, //6793 #CJK UNIFIED IDEOGRAPH
    {0xB3BA, 0x7ADF}, //6794 #CJK UNIFIED IDEOGRAPH
    {0xB3BB, 0x9802}, //6795 #CJK UNIFIED IDEOGRAPH
    {0xB3BC, 0x9803}, //6796 #CJK UNIFIED IDEOGRAPH
    {0xB3BD, 0x9B5A}, //6797 #CJK UNIFIED IDEOGRAPH
    {0xB3BE, 0x9CE5}, //6798 #CJK UNIFIED IDEOGRAPH
    {0xB3BF, 0x9E75}, //6799 #CJK UNIFIED IDEOGRAPH
    {0xB3C0, 0x9E7F}, //6800 #CJK UNIFIED IDEOGRAPH
    {0xB3C1, 0x9EA5}, //6801 #CJK UNIFIED IDEOGRAPH
    {0xB3C2, 0x9EBB}, //6802 #CJK UNIFIED IDEOGRAPH
    {0xB3C3, 0x50A2}, //6803 #CJK UNIFIED IDEOGRAPH
    {0xB3C4, 0x508D}, //6804 #CJK UNIFIED IDEOGRAPH
    {0xB3C5, 0x5085}, //6805 #CJK UNIFIED IDEOGRAPH
    {0xB3C6, 0x5099}, //6806 #CJK UNIFIED IDEOGRAPH
    {0xB3C7, 0x5091}, //6807 #CJK UNIFIED IDEOGRAPH
    {0xB3C8, 0x5080}, //6808 #CJK UNIFIED IDEOGRAPH
    {0xB3C9, 0x5096}, //6809 #CJK UNIFIED IDEOGRAPH
    {0xB3CA, 0x5098}, //6810 #CJK UNIFIED IDEOGRAPH
    {0xB3CB, 0x509A}, //6811 #CJK UNIFIED IDEOGRAPH
    {0xB3CC, 0x6700}, //6812 #CJK UNIFIED IDEOGRAPH
    {0xB3CD, 0x51F1}, //6813 #CJK UNIFIED IDEOGRAPH
    {0xB3CE, 0x5272}, //6814 #CJK UNIFIED IDEOGRAPH
    {0xB3CF, 0x5274}, //6815 #CJK UNIFIED IDEOGRAPH
    {0xB3D0, 0x5275}, //6816 #CJK UNIFIED IDEOGRAPH
    {0xB3D1, 0x5269}, //6817 #CJK UNIFIED IDEOGRAPH
    {0xB3D2, 0x52DE}, //6818 #CJK UNIFIED IDEOGRAPH
    {0xB3D3, 0x52DD}, //6819 #CJK UNIFIED IDEOGRAPH
    {0xB3D4, 0x52DB}, //6820 #CJK UNIFIED IDEOGRAPH
    {0xB3D5, 0x535A}, //6821 #CJK UNIFIED IDEOGRAPH
    {0xB3D6, 0x53A5}, //6822 #CJK UNIFIED IDEOGRAPH
    {0xB3D7, 0x557B}, //6823 #CJK UNIFIED IDEOGRAPH
    {0xB3D8, 0x5580}, //6824 #CJK UNIFIED IDEOGRAPH
    {0xB3D9, 0x55A7}, //6825 #CJK UNIFIED IDEOGRAPH
    {0xB3DA, 0x557C}, //6826 #CJK UNIFIED IDEOGRAPH
    {0xB3DB, 0x558A}, //6827 #CJK UNIFIED IDEOGRAPH
    {0xB3DC, 0x559D}, //6828 #CJK UNIFIED IDEOGRAPH
    {0xB3DD, 0x5598}, //6829 #CJK UNIFIED IDEOGRAPH
    {0xB3DE, 0x5582}, //6830 #CJK UNIFIED IDEOGRAPH
    {0xB3DF, 0x559C}, //6831 #CJK UNIFIED IDEOGRAPH
    {0xB3E0, 0x55AA}, //6832 #CJK UNIFIED IDEOGRAPH
    {0xB3E1, 0x5594}, //6833 #CJK UNIFIED IDEOGRAPH
    {0xB3E2, 0x5587}, //6834 #CJK UNIFIED IDEOGRAPH
    {0xB3E3, 0x558B}, //6835 #CJK UNIFIED IDEOGRAPH
    {0xB3E4, 0x5583}, //6836 #CJK UNIFIED IDEOGRAPH
    {0xB3E5, 0x55B3}, //6837 #CJK UNIFIED IDEOGRAPH
    {0xB3E6, 0x55AE}, //6838 #CJK UNIFIED IDEOGRAPH
    {0xB3E7, 0x559F}, //6839 #CJK UNIFIED IDEOGRAPH
    {0xB3E8, 0x553E}, //6840 #CJK UNIFIED IDEOGRAPH
    {0xB3E9, 0x55B2}, //6841 #CJK UNIFIED IDEOGRAPH
    {0xB3EA, 0x559A}, //6842 #CJK UNIFIED IDEOGRAPH
    {0xB3EB, 0x55BB}, //6843 #CJK UNIFIED IDEOGRAPH
    {0xB3EC, 0x55AC}, //6844 #CJK UNIFIED IDEOGRAPH
    {0xB3ED, 0x55B1}, //6845 #CJK UNIFIED IDEOGRAPH
    {0xB3EE, 0x557E}, //6846 #CJK UNIFIED IDEOGRAPH
    {0xB3EF, 0x5589}, //6847 #CJK UNIFIED IDEOGRAPH
    {0xB3F0, 0x55AB}, //6848 #CJK UNIFIED IDEOGRAPH
    {0xB3F1, 0x5599}, //6849 #CJK UNIFIED IDEOGRAPH
    {0xB3F2, 0x570D}, //6850 #CJK UNIFIED IDEOGRAPH
    {0xB3F3, 0x582F}, //6851 #CJK UNIFIED IDEOGRAPH
    {0xB3F4, 0x582A}, //6852 #CJK UNIFIED IDEOGRAPH
    {0xB3F5, 0x5834}, //6853 #CJK UNIFIED IDEOGRAPH
    {0xB3F6, 0x5824}, //6854 #CJK UNIFIED IDEOGRAPH
    {0xB3F7, 0x5830}, //6855 #CJK UNIFIED IDEOGRAPH
    {0xB3F8, 0x5831}, //6856 #CJK UNIFIED IDEOGRAPH
    {0xB3F9, 0x5821}, //6857 #CJK UNIFIED IDEOGRAPH
    {0xB3FA, 0x581D}, //6858 #CJK UNIFIED IDEOGRAPH
    {0xB3FB, 0x5820}, //6859 #CJK UNIFIED IDEOGRAPH
    {0xB3FC, 0x58F9}, //6860 #CJK UNIFIED IDEOGRAPH
    {0xB3FD, 0x58FA}, //6861 #CJK UNIFIED IDEOGRAPH
    {0xB3FE, 0x5960}, //6862 #CJK UNIFIED IDEOGRAPH
    {0xB440, 0x5A77}, //6863 #CJK UNIFIED IDEOGRAPH
    {0xB441, 0x5A9A}, //6864 #CJK UNIFIED IDEOGRAPH
    {0xB442, 0x5A7F}, //6865 #CJK UNIFIED IDEOGRAPH
    {0xB443, 0x5A92}, //6866 #CJK UNIFIED IDEOGRAPH
    {0xB444, 0x5A9B}, //6867 #CJK UNIFIED IDEOGRAPH
    {0xB445, 0x5AA7}, //6868 #CJK UNIFIED IDEOGRAPH
    {0xB446, 0x5B73}, //6869 #CJK UNIFIED IDEOGRAPH
    {0xB447, 0x5B71}, //6870 #CJK UNIFIED IDEOGRAPH
    {0xB448, 0x5BD2}, //6871 #CJK UNIFIED IDEOGRAPH
    {0xB449, 0x5BCC}, //6872 #CJK UNIFIED IDEOGRAPH
    {0xB44A, 0x5BD3}, //6873 #CJK UNIFIED IDEOGRAPH
    {0xB44B, 0x5BD0}, //6874 #CJK UNIFIED IDEOGRAPH
    {0xB44C, 0x5C0A}, //6875 #CJK UNIFIED IDEOGRAPH
    {0xB44D, 0x5C0B}, //6876 #CJK UNIFIED IDEOGRAPH
    {0xB44E, 0x5C31}, //6877 #CJK UNIFIED IDEOGRAPH
    {0xB44F, 0x5D4C}, //6878 #CJK UNIFIED IDEOGRAPH
    {0xB450, 0x5D50}, //6879 #CJK UNIFIED IDEOGRAPH
    {0xB451, 0x5D34}, //6880 #CJK UNIFIED IDEOGRAPH
    {0xB452, 0x5D47}, //6881 #CJK UNIFIED IDEOGRAPH
    {0xB453, 0x5DFD}, //6882 #CJK UNIFIED IDEOGRAPH
    {0xB454, 0x5E45}, //6883 #CJK UNIFIED IDEOGRAPH
    {0xB455, 0x5E3D}, //6884 #CJK UNIFIED IDEOGRAPH
    {0xB456, 0x5E40}, //6885 #CJK UNIFIED IDEOGRAPH
    {0xB457, 0x5E43}, //6886 #CJK UNIFIED IDEOGRAPH
    {0xB458, 0x5E7E}, //6887 #CJK UNIFIED IDEOGRAPH
    {0xB459, 0x5ECA}, //6888 #CJK UNIFIED IDEOGRAPH
    {0xB45A, 0x5EC1}, //6889 #CJK UNIFIED IDEOGRAPH
    {0xB45B, 0x5EC2}, //6890 #CJK UNIFIED IDEOGRAPH
    {0xB45C, 0x5EC4}, //6891 #CJK UNIFIED IDEOGRAPH
    {0xB45D, 0x5F3C}, //6892 #CJK UNIFIED IDEOGRAPH
    {0xB45E, 0x5F6D}, //6893 #CJK UNIFIED IDEOGRAPH
    {0xB45F, 0x5FA9}, //6894 #CJK UNIFIED IDEOGRAPH
    {0xB460, 0x5FAA}, //6895 #CJK UNIFIED IDEOGRAPH
    {0xB461, 0x5FA8}, //6896 #CJK UNIFIED IDEOGRAPH
    {0xB462, 0x60D1}, //6897 #CJK UNIFIED IDEOGRAPH
    {0xB463, 0x60E1}, //6898 #CJK UNIFIED IDEOGRAPH
    {0xB464, 0x60B2}, //6899 #CJK UNIFIED IDEOGRAPH
    {0xB465, 0x60B6}, //6900 #CJK UNIFIED IDEOGRAPH
    {0xB466, 0x60E0}, //6901 #CJK UNIFIED IDEOGRAPH
    {0xB467, 0x611C}, //6902 #CJK UNIFIED IDEOGRAPH
    {0xB468, 0x6123}, //6903 #CJK UNIFIED IDEOGRAPH
    {0xB469, 0x60FA}, //6904 #CJK UNIFIED IDEOGRAPH
    {0xB46A, 0x6115}, //6905 #CJK UNIFIED IDEOGRAPH
    {0xB46B, 0x60F0}, //6906 #CJK UNIFIED IDEOGRAPH
    {0xB46C, 0x60FB}, //6907 #CJK UNIFIED IDEOGRAPH
    {0xB46D, 0x60F4}, //6908 #CJK UNIFIED IDEOGRAPH
    {0xB46E, 0x6168}, //6909 #CJK UNIFIED IDEOGRAPH
    {0xB46F, 0x60F1}, //6910 #CJK UNIFIED IDEOGRAPH
    {0xB470, 0x610E}, //6911 #CJK UNIFIED IDEOGRAPH
    {0xB471, 0x60F6}, //6912 #CJK UNIFIED IDEOGRAPH
    {0xB472, 0x6109}, //6913 #CJK UNIFIED IDEOGRAPH
    {0xB473, 0x6100}, //6914 #CJK UNIFIED IDEOGRAPH
    {0xB474, 0x6112}, //6915 #CJK UNIFIED IDEOGRAPH
    {0xB475, 0x621F}, //6916 #CJK UNIFIED IDEOGRAPH
    {0xB476, 0x6249}, //6917 #CJK UNIFIED IDEOGRAPH
    {0xB477, 0x63A3}, //6918 #CJK UNIFIED IDEOGRAPH
    {0xB478, 0x638C}, //6919 #CJK UNIFIED IDEOGRAPH
    {0xB479, 0x63CF}, //6920 #CJK UNIFIED IDEOGRAPH
    {0xB47A, 0x63C0}, //6921 #CJK UNIFIED IDEOGRAPH
    {0xB47B, 0x63E9}, //6922 #CJK UNIFIED IDEOGRAPH
    {0xB47C, 0x63C9}, //6923 #CJK UNIFIED IDEOGRAPH
    {0xB47D, 0x63C6}, //6924 #CJK UNIFIED IDEOGRAPH
    {0xB47E, 0x63CD}, //6925 #CJK UNIFIED IDEOGRAPH
    {0xB4A1, 0x63D2}, //6926 #CJK UNIFIED IDEOGRAPH
    {0xB4A2, 0x63E3}, //6927 #CJK UNIFIED IDEOGRAPH
    {0xB4A3, 0x63D0}, //6928 #CJK UNIFIED IDEOGRAPH
    {0xB4A4, 0x63E1}, //6929 #CJK UNIFIED IDEOGRAPH
    {0xB4A5, 0x63D6}, //6930 #CJK UNIFIED IDEOGRAPH
    {0xB4A6, 0x63ED}, //6931 #CJK UNIFIED IDEOGRAPH
    {0xB4A7, 0x63EE}, //6932 #CJK UNIFIED IDEOGRAPH
    {0xB4A8, 0x6376}, //6933 #CJK UNIFIED IDEOGRAPH
    {0xB4A9, 0x63F4}, //6934 #CJK UNIFIED IDEOGRAPH
    {0xB4AA, 0x63EA}, //6935 #CJK UNIFIED IDEOGRAPH
    {0xB4AB, 0x63DB}, //6936 #CJK UNIFIED IDEOGRAPH
    {0xB4AC, 0x6452}, //6937 #CJK UNIFIED IDEOGRAPH
    {0xB4AD, 0x63DA}, //6938 #CJK UNIFIED IDEOGRAPH
    {0xB4AE, 0x63F9}, //6939 #CJK UNIFIED IDEOGRAPH
    {0xB4AF, 0x655E}, //6940 #CJK UNIFIED IDEOGRAPH
    {0xB4B0, 0x6566}, //6941 #CJK UNIFIED IDEOGRAPH
    {0xB4B1, 0x6562}, //6942 #CJK UNIFIED IDEOGRAPH
    {0xB4B2, 0x6563}, //6943 #CJK UNIFIED IDEOGRAPH
    {0xB4B3, 0x6591}, //6944 #CJK UNIFIED IDEOGRAPH
    {0xB4B4, 0x6590}, //6945 #CJK UNIFIED IDEOGRAPH
    {0xB4B5, 0x65AF}, //6946 #CJK UNIFIED IDEOGRAPH
    {0xB4B6, 0x666E}, //6947 #CJK UNIFIED IDEOGRAPH
    {0xB4B7, 0x6670}, //6948 #CJK UNIFIED IDEOGRAPH
    {0xB4B8, 0x6674}, //6949 #CJK UNIFIED IDEOGRAPH
    {0xB4B9, 0x6676}, //6950 #CJK UNIFIED IDEOGRAPH
    {0xB4BA, 0x666F}, //6951 #CJK UNIFIED IDEOGRAPH
    {0xB4BB, 0x6691}, //6952 #CJK UNIFIED IDEOGRAPH
    {0xB4BC, 0x667A}, //6953 #CJK UNIFIED IDEOGRAPH
    {0xB4BD, 0x667E}, //6954 #CJK UNIFIED IDEOGRAPH
    {0xB4BE, 0x6677}, //6955 #CJK UNIFIED IDEOGRAPH
    {0xB4BF, 0x66FE}, //6956 #CJK UNIFIED IDEOGRAPH
    {0xB4C0, 0x66FF}, //6957 #CJK UNIFIED IDEOGRAPH
    {0xB4C1, 0x671F}, //6958 #CJK UNIFIED IDEOGRAPH
    {0xB4C2, 0x671D}, //6959 #CJK UNIFIED IDEOGRAPH
    {0xB4C3, 0x68FA}, //6960 #CJK UNIFIED IDEOGRAPH
    {0xB4C4, 0x68D5}, //6961 #CJK UNIFIED IDEOGRAPH
    {0xB4C5, 0x68E0}, //6962 #CJK UNIFIED IDEOGRAPH
    {0xB4C6, 0x68D8}, //6963 #CJK UNIFIED IDEOGRAPH
    {0xB4C7, 0x68D7}, //6964 #CJK UNIFIED IDEOGRAPH
    {0xB4C8, 0x6905}, //6965 #CJK UNIFIED IDEOGRAPH
    {0xB4C9, 0x68DF}, //6966 #CJK UNIFIED IDEOGRAPH
    {0xB4CA, 0x68F5}, //6967 #CJK UNIFIED IDEOGRAPH
    {0xB4CB, 0x68EE}, //6968 #CJK UNIFIED IDEOGRAPH
    {0xB4CC, 0x68E7}, //6969 #CJK UNIFIED IDEOGRAPH
    {0xB4CD, 0x68F9}, //6970 #CJK UNIFIED IDEOGRAPH
    {0xB4CE, 0x68D2}, //6971 #CJK UNIFIED IDEOGRAPH
    {0xB4CF, 0x68F2}, //6972 #CJK UNIFIED IDEOGRAPH
    {0xB4D0, 0x68E3}, //6973 #CJK UNIFIED IDEOGRAPH
    {0xB4D1, 0x68CB}, //6974 #CJK UNIFIED IDEOGRAPH
    {0xB4D2, 0x68CD}, //6975 #CJK UNIFIED IDEOGRAPH
    {0xB4D3, 0x690D}, //6976 #CJK UNIFIED IDEOGRAPH
    {0xB4D4, 0x6912}, //6977 #CJK UNIFIED IDEOGRAPH
    {0xB4D5, 0x690E}, //6978 #CJK UNIFIED IDEOGRAPH
    {0xB4D6, 0x68C9}, //6979 #CJK UNIFIED IDEOGRAPH
    {0xB4D7, 0x68DA}, //6980 #CJK UNIFIED IDEOGRAPH
    {0xB4D8, 0x696E}, //6981 #CJK UNIFIED IDEOGRAPH
    {0xB4D9, 0x68FB}, //6982 #CJK UNIFIED IDEOGRAPH
    {0xB4DA, 0x6B3E}, //6983 #CJK UNIFIED IDEOGRAPH
    {0xB4DB, 0x6B3A}, //6984 #CJK UNIFIED IDEOGRAPH
    {0xB4DC, 0x6B3D}, //6985 #CJK UNIFIED IDEOGRAPH
    {0xB4DD, 0x6B98}, //6986 #CJK UNIFIED IDEOGRAPH
    {0xB4DE, 0x6B96}, //6987 #CJK UNIFIED IDEOGRAPH
    {0xB4DF, 0x6BBC}, //6988 #CJK UNIFIED IDEOGRAPH
    {0xB4E0, 0x6BEF}, //6989 #CJK UNIFIED IDEOGRAPH
    {0xB4E1, 0x6C2E}, //6990 #CJK UNIFIED IDEOGRAPH
    {0xB4E2, 0x6C2F}, //6991 #CJK UNIFIED IDEOGRAPH
    {0xB4E3, 0x6C2C}, //6992 #CJK UNIFIED IDEOGRAPH
    {0xB4E4, 0x6E2F}, //6993 #CJK UNIFIED IDEOGRAPH
    {0xB4E5, 0x6E38}, //6994 #CJK UNIFIED IDEOGRAPH
    {0xB4E6, 0x6E54}, //6995 #CJK UNIFIED IDEOGRAPH
    {0xB4E7, 0x6E21}, //6996 #CJK UNIFIED IDEOGRAPH
    {0xB4E8, 0x6E32}, //6997 #CJK UNIFIED IDEOGRAPH
    {0xB4E9, 0x6E67}, //6998 #CJK UNIFIED IDEOGRAPH
    {0xB4EA, 0x6E4A}, //6999 #CJK UNIFIED IDEOGRAPH
    {0xB4EB, 0x6E20}, //7000 #CJK UNIFIED IDEOGRAPH
    {0xB4EC, 0x6E25}, //7001 #CJK UNIFIED IDEOGRAPH
    {0xB4ED, 0x6E23}, //7002 #CJK UNIFIED IDEOGRAPH
    {0xB4EE, 0x6E1B}, //7003 #CJK UNIFIED IDEOGRAPH
    {0xB4EF, 0x6E5B}, //7004 #CJK UNIFIED IDEOGRAPH
    {0xB4F0, 0x6E58}, //7005 #CJK UNIFIED IDEOGRAPH
    {0xB4F1, 0x6E24}, //7006 #CJK UNIFIED IDEOGRAPH
    {0xB4F2, 0x6E56}, //7007 #CJK UNIFIED IDEOGRAPH
    {0xB4F3, 0x6E6E}, //7008 #CJK UNIFIED IDEOGRAPH
    {0xB4F4, 0x6E2D}, //7009 #CJK UNIFIED IDEOGRAPH
    {0xB4F5, 0x6E26}, //7010 #CJK UNIFIED IDEOGRAPH
    {0xB4F6, 0x6E6F}, //7011 #CJK UNIFIED IDEOGRAPH
    {0xB4F7, 0x6E34}, //7012 #CJK UNIFIED IDEOGRAPH
    {0xB4F8, 0x6E4D}, //7013 #CJK UNIFIED IDEOGRAPH
    {0xB4F9, 0x6E3A}, //7014 #CJK UNIFIED IDEOGRAPH
    {0xB4FA, 0x6E2C}, //7015 #CJK UNIFIED IDEOGRAPH
    {0xB4FB, 0x6E43}, //7016 #CJK UNIFIED IDEOGRAPH
    {0xB4FC, 0x6E1D}, //7017 #CJK UNIFIED IDEOGRAPH
    {0xB4FD, 0x6E3E}, //7018 #CJK UNIFIED IDEOGRAPH
    {0xB4FE, 0x6ECB}, //7019 #CJK UNIFIED IDEOGRAPH
    {0xB540, 0x6E89}, //7020 #CJK UNIFIED IDEOGRAPH
    {0xB541, 0x6E19}, //7021 #CJK UNIFIED IDEOGRAPH
    {0xB542, 0x6E4E}, //7022 #CJK UNIFIED IDEOGRAPH
    {0xB543, 0x6E63}, //7023 #CJK UNIFIED IDEOGRAPH
    {0xB544, 0x6E44}, //7024 #CJK UNIFIED IDEOGRAPH
    {0xB545, 0x6E72}, //7025 #CJK UNIFIED IDEOGRAPH
    {0xB546, 0x6E69}, //7026 #CJK UNIFIED IDEOGRAPH
    {0xB547, 0x6E5F}, //7027 #CJK UNIFIED IDEOGRAPH
    {0xB548, 0x7119}, //7028 #CJK UNIFIED IDEOGRAPH
    {0xB549, 0x711A}, //7029 #CJK UNIFIED IDEOGRAPH
    {0xB54A, 0x7126}, //7030 #CJK UNIFIED IDEOGRAPH
    {0xB54B, 0x7130}, //7031 #CJK UNIFIED IDEOGRAPH
    {0xB54C, 0x7121}, //7032 #CJK UNIFIED IDEOGRAPH
    {0xB54D, 0x7136}, //7033 #CJK UNIFIED IDEOGRAPH
    {0xB54E, 0x716E}, //7034 #CJK UNIFIED IDEOGRAPH
    {0xB54F, 0x711C}, //7035 #CJK UNIFIED IDEOGRAPH
    {0xB550, 0x724C}, //7036 #CJK UNIFIED IDEOGRAPH
    {0xB551, 0x7284}, //7037 #CJK UNIFIED IDEOGRAPH
    {0xB552, 0x7280}, //7038 #CJK UNIFIED IDEOGRAPH
    {0xB553, 0x7336}, //7039 #CJK UNIFIED IDEOGRAPH
    {0xB554, 0x7325}, //7040 #CJK UNIFIED IDEOGRAPH
    {0xB555, 0x7334}, //7041 #CJK UNIFIED IDEOGRAPH
    {0xB556, 0x7329}, //7042 #CJK UNIFIED IDEOGRAPH
    {0xB557, 0x743A}, //7043 #CJK UNIFIED IDEOGRAPH
    {0xB558, 0x742A}, //7044 #CJK UNIFIED IDEOGRAPH
    {0xB559, 0x7433}, //7045 #CJK UNIFIED IDEOGRAPH
    {0xB55A, 0x7422}, //7046 #CJK UNIFIED IDEOGRAPH
    {0xB55B, 0x7425}, //7047 #CJK UNIFIED IDEOGRAPH
    {0xB55C, 0x7435}, //7048 #CJK UNIFIED IDEOGRAPH
    {0xB55D, 0x7436}, //7049 #CJK UNIFIED IDEOGRAPH
    {0xB55E, 0x7434}, //7050 #CJK UNIFIED IDEOGRAPH
    {0xB55F, 0x742F}, //7051 #CJK UNIFIED IDEOGRAPH
    {0xB560, 0x741B}, //7052 #CJK UNIFIED IDEOGRAPH
    {0xB561, 0x7426}, //7053 #CJK UNIFIED IDEOGRAPH
    {0xB562, 0x7428}, //7054 #CJK UNIFIED IDEOGRAPH
    {0xB563, 0x7525}, //7055 #CJK UNIFIED IDEOGRAPH
    {0xB564, 0x7526}, //7056 #CJK UNIFIED IDEOGRAPH
    {0xB565, 0x756B}, //7057 #CJK UNIFIED IDEOGRAPH
    {0xB566, 0x756A}, //7058 #CJK UNIFIED IDEOGRAPH
    {0xB567, 0x75E2}, //7059 #CJK UNIFIED IDEOGRAPH
    {0xB568, 0x75DB}, //7060 #CJK UNIFIED IDEOGRAPH
    {0xB569, 0x75E3}, //7061 #CJK UNIFIED IDEOGRAPH
    {0xB56A, 0x75D9}, //7062 #CJK UNIFIED IDEOGRAPH
    {0xB56B, 0x75D8}, //7063 #CJK UNIFIED IDEOGRAPH
    {0xB56C, 0x75DE}, //7064 #CJK UNIFIED IDEOGRAPH
    {0xB56D, 0x75E0}, //7065 #CJK UNIFIED IDEOGRAPH
    {0xB56E, 0x767B}, //7066 #CJK UNIFIED IDEOGRAPH
    {0xB56F, 0x767C}, //7067 #CJK UNIFIED IDEOGRAPH
    {0xB570, 0x7696}, //7068 #CJK UNIFIED IDEOGRAPH
    {0xB571, 0x7693}, //7069 #CJK UNIFIED IDEOGRAPH
    {0xB572, 0x76B4}, //7070 #CJK UNIFIED IDEOGRAPH
    {0xB573, 0x76DC}, //7071 #CJK UNIFIED IDEOGRAPH
    {0xB574, 0x774F}, //7072 #CJK UNIFIED IDEOGRAPH
    {0xB575, 0x77ED}, //7073 #CJK UNIFIED IDEOGRAPH
    {0xB576, 0x785D}, //7074 #CJK UNIFIED IDEOGRAPH
    {0xB577, 0x786C}, //7075 #CJK UNIFIED IDEOGRAPH
    {0xB578, 0x786F}, //7076 #CJK UNIFIED IDEOGRAPH
    {0xB579, 0x7A0D}, //7077 #CJK UNIFIED IDEOGRAPH
    {0xB57A, 0x7A08}, //7078 #CJK UNIFIED IDEOGRAPH
    {0xB57B, 0x7A0B}, //7079 #CJK UNIFIED IDEOGRAPH
    {0xB57C, 0x7A05}, //7080 #CJK UNIFIED IDEOGRAPH
    {0xB57D, 0x7A00}, //7081 #CJK UNIFIED IDEOGRAPH
    {0xB57E, 0x7A98}, //7082 #CJK UNIFIED IDEOGRAPH
    {0xB5A1, 0x7A97}, //7083 #CJK UNIFIED IDEOGRAPH
    {0xB5A2, 0x7A96}, //7084 #CJK UNIFIED IDEOGRAPH
    {0xB5A3, 0x7AE5}, //7085 #CJK UNIFIED IDEOGRAPH
    {0xB5A4, 0x7AE3}, //7086 #CJK UNIFIED IDEOGRAPH
    {0xB5A5, 0x7B49}, //7087 #CJK UNIFIED IDEOGRAPH
    {0xB5A6, 0x7B56}, //7088 #CJK UNIFIED IDEOGRAPH
    {0xB5A7, 0x7B46}, //7089 #CJK UNIFIED IDEOGRAPH
    {0xB5A8, 0x7B50}, //7090 #CJK UNIFIED IDEOGRAPH
    {0xB5A9, 0x7B52}, //7091 #CJK UNIFIED IDEOGRAPH
    {0xB5AA, 0x7B54}, //7092 #CJK UNIFIED IDEOGRAPH
    {0xB5AB, 0x7B4D}, //7093 #CJK UNIFIED IDEOGRAPH
    {0xB5AC, 0x7B4B}, //7094 #CJK UNIFIED IDEOGRAPH
    {0xB5AD, 0x7B4F}, //7095 #CJK UNIFIED IDEOGRAPH
    {0xB5AE, 0x7B51}, //7096 #CJK UNIFIED IDEOGRAPH
    {0xB5AF, 0x7C9F}, //7097 #CJK UNIFIED IDEOGRAPH
    {0xB5B0, 0x7CA5}, //7098 #CJK UNIFIED IDEOGRAPH
    {0xB5B1, 0x7D5E}, //7099 #CJK UNIFIED IDEOGRAPH
    {0xB5B2, 0x7D50}, //7100 #CJK UNIFIED IDEOGRAPH
    {0xB5B3, 0x7D68}, //7101 #CJK UNIFIED IDEOGRAPH
    {0xB5B4, 0x7D55}, //7102 #CJK UNIFIED IDEOGRAPH
    {0xB5B5, 0x7D2B}, //7103 #CJK UNIFIED IDEOGRAPH
    {0xB5B6, 0x7D6E}, //7104 #CJK UNIFIED IDEOGRAPH
    {0xB5B7, 0x7D72}, //7105 #CJK UNIFIED IDEOGRAPH
    {0xB5B8, 0x7D61}, //7106 #CJK UNIFIED IDEOGRAPH
    {0xB5B9, 0x7D66}, //7107 #CJK UNIFIED IDEOGRAPH
    {0xB5BA, 0x7D62}, //7108 #CJK UNIFIED IDEOGRAPH
    {0xB5BB, 0x7D70}, //7109 #CJK UNIFIED IDEOGRAPH
    {0xB5BC, 0x7D73}, //7110 #CJK UNIFIED IDEOGRAPH
    {0xB5BD, 0x5584}, //7111 #CJK UNIFIED IDEOGRAPH
    {0xB5BE, 0x7FD4}, //7112 #CJK UNIFIED IDEOGRAPH
    {0xB5BF, 0x7FD5}, //7113 #CJK UNIFIED IDEOGRAPH
    {0xB5C0, 0x800B}, //7114 #CJK UNIFIED IDEOGRAPH
    {0xB5C1, 0x8052}, //7115 #CJK UNIFIED IDEOGRAPH
    {0xB5C2, 0x8085}, //7116 #CJK UNIFIED IDEOGRAPH
    {0xB5C3, 0x8155}, //7117 #CJK UNIFIED IDEOGRAPH
    {0xB5C4, 0x8154}, //7118 #CJK UNIFIED IDEOGRAPH
    {0xB5C5, 0x814B}, //7119 #CJK UNIFIED IDEOGRAPH
    {0xB5C6, 0x8151}, //7120 #CJK UNIFIED IDEOGRAPH
    {0xB5C7, 0x814E}, //7121 #CJK UNIFIED IDEOGRAPH
    {0xB5C8, 0x8139}, //7122 #CJK UNIFIED IDEOGRAPH
    {0xB5C9, 0x8146}, //7123 #CJK UNIFIED IDEOGRAPH
    {0xB5CA, 0x813E}, //7124 #CJK UNIFIED IDEOGRAPH
    {0xB5CB, 0x814C}, //7125 #CJK UNIFIED IDEOGRAPH
    {0xB5CC, 0x8153}, //7126 #CJK UNIFIED IDEOGRAPH
    {0xB5CD, 0x8174}, //7127 #CJK UNIFIED IDEOGRAPH
    {0xB5CE, 0x8212}, //7128 #CJK UNIFIED IDEOGRAPH
    {0xB5CF, 0x821C}, //7129 #CJK UNIFIED IDEOGRAPH
    {0xB5D0, 0x83E9}, //7130 #CJK UNIFIED IDEOGRAPH
    {0xB5D1, 0x8403}, //7131 #CJK UNIFIED IDEOGRAPH
    {0xB5D2, 0x83F8}, //7132 #CJK UNIFIED IDEOGRAPH
    {0xB5D3, 0x840D}, //7133 #CJK UNIFIED IDEOGRAPH
    {0xB5D4, 0x83E0}, //7134 #CJK UNIFIED IDEOGRAPH
    {0xB5D5, 0x83C5}, //7135 #CJK UNIFIED IDEOGRAPH
    {0xB5D6, 0x840B}, //7136 #CJK UNIFIED IDEOGRAPH
    {0xB5D7, 0x83C1}, //7137 #CJK UNIFIED IDEOGRAPH
    {0xB5D8, 0x83EF}, //7138 #CJK UNIFIED IDEOGRAPH
    {0xB5D9, 0x83F1}, //7139 #CJK UNIFIED IDEOGRAPH
    {0xB5DA, 0x83F4}, //7140 #CJK UNIFIED IDEOGRAPH
    {0xB5DB, 0x8457}, //7141 #CJK UNIFIED IDEOGRAPH
    {0xB5DC, 0x840A}, //7142 #CJK UNIFIED IDEOGRAPH
    {0xB5DD, 0x83F0}, //7143 #CJK UNIFIED IDEOGRAPH
    {0xB5DE, 0x840C}, //7144 #CJK UNIFIED IDEOGRAPH
    {0xB5DF, 0x83CC}, //7145 #CJK UNIFIED IDEOGRAPH
    {0xB5E0, 0x83FD}, //7146 #CJK UNIFIED IDEOGRAPH
    {0xB5E1, 0x83F2}, //7147 #CJK UNIFIED IDEOGRAPH
    {0xB5E2, 0x83CA}, //7148 #CJK UNIFIED IDEOGRAPH
    {0xB5E3, 0x8438}, //7149 #CJK UNIFIED IDEOGRAPH
    {0xB5E4, 0x840E}, //7150 #CJK UNIFIED IDEOGRAPH
    {0xB5E5, 0x8404}, //7151 #CJK UNIFIED IDEOGRAPH
    {0xB5E6, 0x83DC}, //7152 #CJK UNIFIED IDEOGRAPH
    {0xB5E7, 0x8407}, //7153 #CJK UNIFIED IDEOGRAPH
    {0xB5E8, 0x83D4}, //7154 #CJK UNIFIED IDEOGRAPH
    {0xB5E9, 0x83DF}, //7155 #CJK UNIFIED IDEOGRAPH
    {0xB5EA, 0x865B}, //7156 #CJK UNIFIED IDEOGRAPH
    {0xB5EB, 0x86DF}, //7157 #CJK UNIFIED IDEOGRAPH
    {0xB5EC, 0x86D9}, //7158 #CJK UNIFIED IDEOGRAPH
    {0xB5ED, 0x86ED}, //7159 #CJK UNIFIED IDEOGRAPH
    {0xB5EE, 0x86D4}, //7160 #CJK UNIFIED IDEOGRAPH
    {0xB5EF, 0x86DB}, //7161 #CJK UNIFIED IDEOGRAPH
    {0xB5F0, 0x86E4}, //7162 #CJK UNIFIED IDEOGRAPH
    {0xB5F1, 0x86D0}, //7163 #CJK UNIFIED IDEOGRAPH
    {0xB5F2, 0x86DE}, //7164 #CJK UNIFIED IDEOGRAPH
    {0xB5F3, 0x8857}, //7165 #CJK UNIFIED IDEOGRAPH
    {0xB5F4, 0x88C1}, //7166 #CJK UNIFIED IDEOGRAPH
    {0xB5F5, 0x88C2}, //7167 #CJK UNIFIED IDEOGRAPH
    {0xB5F6, 0x88B1}, //7168 #CJK UNIFIED IDEOGRAPH
    {0xB5F7, 0x8983}, //7169 #CJK UNIFIED IDEOGRAPH
    {0xB5F8, 0x8996}, //7170 #CJK UNIFIED IDEOGRAPH
    {0xB5F9, 0x8A3B}, //7171 #CJK UNIFIED IDEOGRAPH
    {0xB5FA, 0x8A60}, //7172 #CJK UNIFIED IDEOGRAPH
    {0xB5FB, 0x8A55}, //7173 #CJK UNIFIED IDEOGRAPH
    {0xB5FC, 0x8A5E}, //7174 #CJK UNIFIED IDEOGRAPH
    {0xB5FD, 0x8A3C}, //7175 #CJK UNIFIED IDEOGRAPH
    {0xB5FE, 0x8A41}, //7176 #CJK UNIFIED IDEOGRAPH
    {0xB640, 0x8A54}, //7177 #CJK UNIFIED IDEOGRAPH
    {0xB641, 0x8A5B}, //7178 #CJK UNIFIED IDEOGRAPH
    {0xB642, 0x8A50}, //7179 #CJK UNIFIED IDEOGRAPH
    {0xB643, 0x8A46}, //7180 #CJK UNIFIED IDEOGRAPH
    {0xB644, 0x8A34}, //7181 #CJK UNIFIED IDEOGRAPH
    {0xB645, 0x8A3A}, //7182 #CJK UNIFIED IDEOGRAPH
    {0xB646, 0x8A36}, //7183 #CJK UNIFIED IDEOGRAPH
    {0xB647, 0x8A56}, //7184 #CJK UNIFIED IDEOGRAPH
    {0xB648, 0x8C61}, //7185 #CJK UNIFIED IDEOGRAPH
    {0xB649, 0x8C82}, //7186 #CJK UNIFIED IDEOGRAPH
    {0xB64A, 0x8CAF}, //7187 #CJK UNIFIED IDEOGRAPH
    {0xB64B, 0x8CBC}, //7188 #CJK UNIFIED IDEOGRAPH
    {0xB64C, 0x8CB3}, //7189 #CJK UNIFIED IDEOGRAPH
    {0xB64D, 0x8CBD}, //7190 #CJK UNIFIED IDEOGRAPH
    {0xB64E, 0x8CC1}, //7191 #CJK UNIFIED IDEOGRAPH
    {0xB64F, 0x8CBB}, //7192 #CJK UNIFIED IDEOGRAPH
    {0xB650, 0x8CC0}, //7193 #CJK UNIFIED IDEOGRAPH
    {0xB651, 0x8CB4}, //7194 #CJK UNIFIED IDEOGRAPH
    {0xB652, 0x8CB7}, //7195 #CJK UNIFIED IDEOGRAPH
    {0xB653, 0x8CB6}, //7196 #CJK UNIFIED IDEOGRAPH
    {0xB654, 0x8CBF}, //7197 #CJK UNIFIED IDEOGRAPH
    {0xB655, 0x8CB8}, //7198 #CJK UNIFIED IDEOGRAPH
    {0xB656, 0x8D8A}, //7199 #CJK UNIFIED IDEOGRAPH
    {0xB657, 0x8D85}, //7200 #CJK UNIFIED IDEOGRAPH
    {0xB658, 0x8D81}, //7201 #CJK UNIFIED IDEOGRAPH
    {0xB659, 0x8DCE}, //7202 #CJK UNIFIED IDEOGRAPH
    {0xB65A, 0x8DDD}, //7203 #CJK UNIFIED IDEOGRAPH
    {0xB65B, 0x8DCB}, //7204 #CJK UNIFIED IDEOGRAPH
    {0xB65C, 0x8DDA}, //7205 #CJK UNIFIED IDEOGRAPH
    {0xB65D, 0x8DD1}, //7206 #CJK UNIFIED IDEOGRAPH
    {0xB65E, 0x8DCC}, //7207 #CJK UNIFIED IDEOGRAPH
    {0xB65F, 0x8DDB}, //7208 #CJK UNIFIED IDEOGRAPH
    {0xB660, 0x8DC6}, //7209 #CJK UNIFIED IDEOGRAPH
    {0xB661, 0x8EFB}, //7210 #CJK UNIFIED IDEOGRAPH
    {0xB662, 0x8EF8}, //7211 #CJK UNIFIED IDEOGRAPH
    {0xB663, 0x8EFC}, //7212 #CJK UNIFIED IDEOGRAPH
    {0xB664, 0x8F9C}, //7213 #CJK UNIFIED IDEOGRAPH
    {0xB665, 0x902E}, //7214 #CJK UNIFIED IDEOGRAPH
    {0xB666, 0x9035}, //7215 #CJK UNIFIED IDEOGRAPH
    {0xB667, 0x9031}, //7216 #CJK UNIFIED IDEOGRAPH
    {0xB668, 0x9038}, //7217 #CJK UNIFIED IDEOGRAPH
    {0xB669, 0x9032}, //7218 #CJK UNIFIED IDEOGRAPH
    {0xB66A, 0x9036}, //7219 #CJK UNIFIED IDEOGRAPH
    {0xB66B, 0x9102}, //7220 #CJK UNIFIED IDEOGRAPH
    {0xB66C, 0x90F5}, //7221 #CJK UNIFIED IDEOGRAPH
    {0xB66D, 0x9109}, //7222 #CJK UNIFIED IDEOGRAPH
    {0xB66E, 0x90FE}, //7223 #CJK UNIFIED IDEOGRAPH
    {0xB66F, 0x9163}, //7224 #CJK UNIFIED IDEOGRAPH
    {0xB670, 0x9165}, //7225 #CJK UNIFIED IDEOGRAPH
    {0xB671, 0x91CF}, //7226 #CJK UNIFIED IDEOGRAPH
    {0xB672, 0x9214}, //7227 #CJK UNIFIED IDEOGRAPH
    {0xB673, 0x9215}, //7228 #CJK UNIFIED IDEOGRAPH
    {0xB674, 0x9223}, //7229 #CJK UNIFIED IDEOGRAPH
    {0xB675, 0x9209}, //7230 #CJK UNIFIED IDEOGRAPH
    {0xB676, 0x921E}, //7231 #CJK UNIFIED IDEOGRAPH
    {0xB677, 0x920D}, //7232 #CJK UNIFIED IDEOGRAPH
    {0xB678, 0x9210}, //7233 #CJK UNIFIED IDEOGRAPH
    {0xB679, 0x9207}, //7234 #CJK UNIFIED IDEOGRAPH
    {0xB67A, 0x9211}, //7235 #CJK UNIFIED IDEOGRAPH
    {0xB67B, 0x9594}, //7236 #CJK UNIFIED IDEOGRAPH
    {0xB67C, 0x958F}, //7237 #CJK UNIFIED IDEOGRAPH
    {0xB67D, 0x958B}, //7238 #CJK UNIFIED IDEOGRAPH
    {0xB67E, 0x9591}, //7239 #CJK UNIFIED IDEOGRAPH
    {0xB6A1, 0x9593}, //7240 #CJK UNIFIED IDEOGRAPH
    {0xB6A2, 0x9592}, //7241 #CJK UNIFIED IDEOGRAPH
    {0xB6A3, 0x958E}, //7242 #CJK UNIFIED IDEOGRAPH
    {0xB6A4, 0x968A}, //7243 #CJK UNIFIED IDEOGRAPH
    {0xB6A5, 0x968E}, //7244 #CJK UNIFIED IDEOGRAPH
    {0xB6A6, 0x968B}, //7245 #CJK UNIFIED IDEOGRAPH
    {0xB6A7, 0x967D}, //7246 #CJK UNIFIED IDEOGRAPH
    {0xB6A8, 0x9685}, //7247 #CJK UNIFIED IDEOGRAPH
    {0xB6A9, 0x9686}, //7248 #CJK UNIFIED IDEOGRAPH
    {0xB6AA, 0x968D}, //7249 #CJK UNIFIED IDEOGRAPH
    {0xB6AB, 0x9672}, //7250 #CJK UNIFIED IDEOGRAPH
    {0xB6AC, 0x9684}, //7251 #CJK UNIFIED IDEOGRAPH
    {0xB6AD, 0x96C1}, //7252 #CJK UNIFIED IDEOGRAPH
    {0xB6AE, 0x96C5}, //7253 #CJK UNIFIED IDEOGRAPH
    {0xB6AF, 0x96C4}, //7254 #CJK UNIFIED IDEOGRAPH
    {0xB6B0, 0x96C6}, //7255 #CJK UNIFIED IDEOGRAPH
    {0xB6B1, 0x96C7}, //7256 #CJK UNIFIED IDEOGRAPH
    {0xB6B2, 0x96EF}, //7257 #CJK UNIFIED IDEOGRAPH
    {0xB6B3, 0x96F2}, //7258 #CJK UNIFIED IDEOGRAPH
    {0xB6B4, 0x97CC}, //7259 #CJK UNIFIED IDEOGRAPH
    {0xB6B5, 0x9805}, //7260 #CJK UNIFIED IDEOGRAPH
    {0xB6B6, 0x9806}, //7261 #CJK UNIFIED IDEOGRAPH
    {0xB6B7, 0x9808}, //7262 #CJK UNIFIED IDEOGRAPH
    {0xB6B8, 0x98E7}, //7263 #CJK UNIFIED IDEOGRAPH
    {0xB6B9, 0x98EA}, //7264 #CJK UNIFIED IDEOGRAPH
    {0xB6BA, 0x98EF}, //7265 #CJK UNIFIED IDEOGRAPH
    {0xB6BB, 0x98E9}, //7266 #CJK UNIFIED IDEOGRAPH
    {0xB6BC, 0x98F2}, //7267 #CJK UNIFIED IDEOGRAPH
    {0xB6BD, 0x98ED}, //7268 #CJK UNIFIED IDEOGRAPH
    {0xB6BE, 0x99AE}, //7269 #CJK UNIFIED IDEOGRAPH
    {0xB6BF, 0x99AD}, //7270 #CJK UNIFIED IDEOGRAPH
    {0xB6C0, 0x9EC3}, //7271 #CJK UNIFIED IDEOGRAPH
    {0xB6C1, 0x9ECD}, //7272 #CJK UNIFIED IDEOGRAPH
    {0xB6C2, 0x9ED1}, //7273 #CJK UNIFIED IDEOGRAPH
    {0xB6C3, 0x4E82}, //7274 #CJK UNIFIED IDEOGRAPH
    {0xB6C4, 0x50AD}, //7275 #CJK UNIFIED IDEOGRAPH
    {0xB6C5, 0x50B5}, //7276 #CJK UNIFIED IDEOGRAPH
    {0xB6C6, 0x50B2}, //7277 #CJK UNIFIED IDEOGRAPH
    {0xB6C7, 0x50B3}, //7278 #CJK UNIFIED IDEOGRAPH
    {0xB6C8, 0x50C5}, //7279 #CJK UNIFIED IDEOGRAPH
    {0xB6C9, 0x50BE}, //7280 #CJK UNIFIED IDEOGRAPH
    {0xB6CA, 0x50AC}, //7281 #CJK UNIFIED IDEOGRAPH
    {0xB6CB, 0x50B7}, //7282 #CJK UNIFIED IDEOGRAPH
    {0xB6CC, 0x50BB}, //7283 #CJK UNIFIED IDEOGRAPH
    {0xB6CD, 0x50AF}, //7284 #CJK UNIFIED IDEOGRAPH
    {0xB6CE, 0x50C7}, //7285 #CJK UNIFIED IDEOGRAPH
    {0xB6CF, 0x527F}, //7286 #CJK UNIFIED IDEOGRAPH
    {0xB6D0, 0x5277}, //7287 #CJK UNIFIED IDEOGRAPH
    {0xB6D1, 0x527D}, //7288 #CJK UNIFIED IDEOGRAPH
    {0xB6D2, 0x52DF}, //7289 #CJK UNIFIED IDEOGRAPH
    {0xB6D3, 0x52E6}, //7290 #CJK UNIFIED IDEOGRAPH
    {0xB6D4, 0x52E4}, //7291 #CJK UNIFIED IDEOGRAPH
    {0xB6D5, 0x52E2}, //7292 #CJK UNIFIED IDEOGRAPH
    {0xB6D6, 0x52E3}, //7293 #CJK UNIFIED IDEOGRAPH
    {0xB6D7, 0x532F}, //7294 #CJK UNIFIED IDEOGRAPH
    {0xB6D8, 0x55DF}, //7295 #CJK UNIFIED IDEOGRAPH
    {0xB6D9, 0x55E8}, //7296 #CJK UNIFIED IDEOGRAPH
    {0xB6DA, 0x55D3}, //7297 #CJK UNIFIED IDEOGRAPH
    {0xB6DB, 0x55E6}, //7298 #CJK UNIFIED IDEOGRAPH
    {0xB6DC, 0x55CE}, //7299 #CJK UNIFIED IDEOGRAPH
    {0xB6DD, 0x55DC}, //7300 #CJK UNIFIED IDEOGRAPH
    {0xB6DE, 0x55C7}, //7301 #CJK UNIFIED IDEOGRAPH
    {0xB6DF, 0x55D1}, //7302 #CJK UNIFIED IDEOGRAPH
    {0xB6E0, 0x55E3}, //7303 #CJK UNIFIED IDEOGRAPH
    {0xB6E1, 0x55E4}, //7304 #CJK UNIFIED IDEOGRAPH
    {0xB6E2, 0x55EF}, //7305 #CJK UNIFIED IDEOGRAPH
    {0xB6E3, 0x55DA}, //7306 #CJK UNIFIED IDEOGRAPH
    {0xB6E4, 0x55E1}, //7307 #CJK UNIFIED IDEOGRAPH
    {0xB6E5, 0x55C5}, //7308 #CJK UNIFIED IDEOGRAPH
    {0xB6E6, 0x55C6}, //7309 #CJK UNIFIED IDEOGRAPH
    {0xB6E7, 0x55E5}, //7310 #CJK UNIFIED IDEOGRAPH
    {0xB6E8, 0x55C9}, //7311 #CJK UNIFIED IDEOGRAPH
    {0xB6E9, 0x5712}, //7312 #CJK UNIFIED IDEOGRAPH
    {0xB6EA, 0x5713}, //7313 #CJK UNIFIED IDEOGRAPH
    {0xB6EB, 0x585E}, //7314 #CJK UNIFIED IDEOGRAPH
    {0xB6EC, 0x5851}, //7315 #CJK UNIFIED IDEOGRAPH
    {0xB6ED, 0x5858}, //7316 #CJK UNIFIED IDEOGRAPH
    {0xB6EE, 0x5857}, //7317 #CJK UNIFIED IDEOGRAPH
    {0xB6EF, 0x585A}, //7318 #CJK UNIFIED IDEOGRAPH
    {0xB6F0, 0x5854}, //7319 #CJK UNIFIED IDEOGRAPH
    {0xB6F1, 0x586B}, //7320 #CJK UNIFIED IDEOGRAPH
    {0xB6F2, 0x584C}, //7321 #CJK UNIFIED IDEOGRAPH
    {0xB6F3, 0x586D}, //7322 #CJK UNIFIED IDEOGRAPH
    {0xB6F4, 0x584A}, //7323 #CJK UNIFIED IDEOGRAPH
    {0xB6F5, 0x5862}, //7324 #CJK UNIFIED IDEOGRAPH
    {0xB6F6, 0x5852}, //7325 #CJK UNIFIED IDEOGRAPH
    {0xB6F7, 0x584B}, //7326 #CJK UNIFIED IDEOGRAPH
    {0xB6F8, 0x5967}, //7327 #CJK UNIFIED IDEOGRAPH
    {0xB6F9, 0x5AC1}, //7328 #CJK UNIFIED IDEOGRAPH
    {0xB6FA, 0x5AC9}, //7329 #CJK UNIFIED IDEOGRAPH
    {0xB6FB, 0x5ACC}, //7330 #CJK UNIFIED IDEOGRAPH
    {0xB6FC, 0x5ABE}, //7331 #CJK UNIFIED IDEOGRAPH
    {0xB6FD, 0x5ABD}, //7332 #CJK UNIFIED IDEOGRAPH
    {0xB6FE, 0x5ABC}, //7333 #CJK UNIFIED IDEOGRAPH
    {0xB740, 0x5AB3}, //7334 #CJK UNIFIED IDEOGRAPH
    {0xB741, 0x5AC2}, //7335 #CJK UNIFIED IDEOGRAPH
    {0xB742, 0x5AB2}, //7336 #CJK UNIFIED IDEOGRAPH
    {0xB743, 0x5D69}, //7337 #CJK UNIFIED IDEOGRAPH
    {0xB744, 0x5D6F}, //7338 #CJK UNIFIED IDEOGRAPH
    {0xB745, 0x5E4C}, //7339 #CJK UNIFIED IDEOGRAPH
    {0xB746, 0x5E79}, //7340 #CJK UNIFIED IDEOGRAPH
    {0xB747, 0x5EC9}, //7341 #CJK UNIFIED IDEOGRAPH
    {0xB748, 0x5EC8}, //7342 #CJK UNIFIED IDEOGRAPH
    {0xB749, 0x5F12}, //7343 #CJK UNIFIED IDEOGRAPH
    {0xB74A, 0x5F59}, //7344 #CJK UNIFIED IDEOGRAPH
    {0xB74B, 0x5FAC}, //7345 #CJK UNIFIED IDEOGRAPH
    {0xB74C, 0x5FAE}, //7346 #CJK UNIFIED IDEOGRAPH
    {0xB74D, 0x611A}, //7347 #CJK UNIFIED IDEOGRAPH
    {0xB74E, 0x610F}, //7348 #CJK UNIFIED IDEOGRAPH
    {0xB74F, 0x6148}, //7349 #CJK UNIFIED IDEOGRAPH
    {0xB750, 0x611F}, //7350 #CJK UNIFIED IDEOGRAPH
    {0xB751, 0x60F3}, //7351 #CJK UNIFIED IDEOGRAPH
    {0xB752, 0x611B}, //7352 #CJK UNIFIED IDEOGRAPH
    {0xB753, 0x60F9}, //7353 #CJK UNIFIED IDEOGRAPH
    {0xB754, 0x6101}, //7354 #CJK UNIFIED IDEOGRAPH
    {0xB755, 0x6108}, //7355 #CJK UNIFIED IDEOGRAPH
    {0xB756, 0x614E}, //7356 #CJK UNIFIED IDEOGRAPH
    {0xB757, 0x614C}, //7357 #CJK UNIFIED IDEOGRAPH
    {0xB758, 0x6144}, //7358 #CJK UNIFIED IDEOGRAPH
    {0xB759, 0x614D}, //7359 #CJK UNIFIED IDEOGRAPH
    {0xB75A, 0x613E}, //7360 #CJK UNIFIED IDEOGRAPH
    {0xB75B, 0x6134}, //7361 #CJK UNIFIED IDEOGRAPH
    {0xB75C, 0x6127}, //7362 #CJK UNIFIED IDEOGRAPH
    {0xB75D, 0x610D}, //7363 #CJK UNIFIED IDEOGRAPH
    {0xB75E, 0x6106}, //7364 #CJK UNIFIED IDEOGRAPH
    {0xB75F, 0x6137}, //7365 #CJK UNIFIED IDEOGRAPH
    {0xB760, 0x6221}, //7366 #CJK UNIFIED IDEOGRAPH
    {0xB761, 0x6222}, //7367 #CJK UNIFIED IDEOGRAPH
    {0xB762, 0x6413}, //7368 #CJK UNIFIED IDEOGRAPH
    {0xB763, 0x643E}, //7369 #CJK UNIFIED IDEOGRAPH
    {0xB764, 0x641E}, //7370 #CJK UNIFIED IDEOGRAPH
    {0xB765, 0x642A}, //7371 #CJK UNIFIED IDEOGRAPH
    {0xB766, 0x642D}, //7372 #CJK UNIFIED IDEOGRAPH
    {0xB767, 0x643D}, //7373 #CJK UNIFIED IDEOGRAPH
    {0xB768, 0x642C}, //7374 #CJK UNIFIED IDEOGRAPH
    {0xB769, 0x640F}, //7375 #CJK UNIFIED IDEOGRAPH
    {0xB76A, 0x641C}, //7376 #CJK UNIFIED IDEOGRAPH
    {0xB76B, 0x6414}, //7377 #CJK UNIFIED IDEOGRAPH
    {0xB76C, 0x640D}, //7378 #CJK UNIFIED IDEOGRAPH
    {0xB76D, 0x6436}, //7379 #CJK UNIFIED IDEOGRAPH
    {0xB76E, 0x6416}, //7380 #CJK UNIFIED IDEOGRAPH
    {0xB76F, 0x6417}, //7381 #CJK UNIFIED IDEOGRAPH
    {0xB770, 0x6406}, //7382 #CJK UNIFIED IDEOGRAPH
    {0xB771, 0x656C}, //7383 #CJK UNIFIED IDEOGRAPH
    {0xB772, 0x659F}, //7384 #CJK UNIFIED IDEOGRAPH
    {0xB773, 0x65B0}, //7385 #CJK UNIFIED IDEOGRAPH
    {0xB774, 0x6697}, //7386 #CJK UNIFIED IDEOGRAPH
    {0xB775, 0x6689}, //7387 #CJK UNIFIED IDEOGRAPH
    {0xB776, 0x6687}, //7388 #CJK UNIFIED IDEOGRAPH
    {0xB777, 0x6688}, //7389 #CJK UNIFIED IDEOGRAPH
    {0xB778, 0x6696}, //7390 #CJK UNIFIED IDEOGRAPH
    {0xB779, 0x6684}, //7391 #CJK UNIFIED IDEOGRAPH
    {0xB77A, 0x6698}, //7392 #CJK UNIFIED IDEOGRAPH
    {0xB77B, 0x668D}, //7393 #CJK UNIFIED IDEOGRAPH
    {0xB77C, 0x6703}, //7394 #CJK UNIFIED IDEOGRAPH
    {0xB77D, 0x6994}, //7395 #CJK UNIFIED IDEOGRAPH
    {0xB77E, 0x696D}, //7396 #CJK UNIFIED IDEOGRAPH
    {0xB7A1, 0x695A}, //7397 #CJK UNIFIED IDEOGRAPH
    {0xB7A2, 0x6977}, //7398 #CJK UNIFIED IDEOGRAPH
    {0xB7A3, 0x6960}, //7399 #CJK UNIFIED IDEOGRAPH
    {0xB7A4, 0x6954}, //7400 #CJK UNIFIED IDEOGRAPH
    {0xB7A5, 0x6975}, //7401 #CJK UNIFIED IDEOGRAPH
    {0xB7A6, 0x6930}, //7402 #CJK UNIFIED IDEOGRAPH
    {0xB7A7, 0x6982}, //7403 #CJK UNIFIED IDEOGRAPH
    {0xB7A8, 0x694A}, //7404 #CJK UNIFIED IDEOGRAPH
    {0xB7A9, 0x6968}, //7405 #CJK UNIFIED IDEOGRAPH
    {0xB7AA, 0x696B}, //7406 #CJK UNIFIED IDEOGRAPH
    {0xB7AB, 0x695E}, //7407 #CJK UNIFIED IDEOGRAPH
    {0xB7AC, 0x6953}, //7408 #CJK UNIFIED IDEOGRAPH
    {0xB7AD, 0x6979}, //7409 #CJK UNIFIED IDEOGRAPH
    {0xB7AE, 0x6986}, //7410 #CJK UNIFIED IDEOGRAPH
    {0xB7AF, 0x695D}, //7411 #CJK UNIFIED IDEOGRAPH
    {0xB7B0, 0x6963}, //7412 #CJK UNIFIED IDEOGRAPH
    {0xB7B1, 0x695B}, //7413 #CJK UNIFIED IDEOGRAPH
    {0xB7B2, 0x6B47}, //7414 #CJK UNIFIED IDEOGRAPH
    {0xB7B3, 0x6B72}, //7415 #CJK UNIFIED IDEOGRAPH
    {0xB7B4, 0x6BC0}, //7416 #CJK UNIFIED IDEOGRAPH
    {0xB7B5, 0x6BBF}, //7417 #CJK UNIFIED IDEOGRAPH
    {0xB7B6, 0x6BD3}, //7418 #CJK UNIFIED IDEOGRAPH
    {0xB7B7, 0x6BFD}, //7419 #CJK UNIFIED IDEOGRAPH
    {0xB7B8, 0x6EA2}, //7420 #CJK UNIFIED IDEOGRAPH
    {0xB7B9, 0x6EAF}, //7421 #CJK UNIFIED IDEOGRAPH
    {0xB7BA, 0x6ED3}, //7422 #CJK UNIFIED IDEOGRAPH
    {0xB7BB, 0x6EB6}, //7423 #CJK UNIFIED IDEOGRAPH
    {0xB7BC, 0x6EC2}, //7424 #CJK UNIFIED IDEOGRAPH
    {0xB7BD, 0x6E90}, //7425 #CJK UNIFIED IDEOGRAPH
    {0xB7BE, 0x6E9D}, //7426 #CJK UNIFIED IDEOGRAPH
    {0xB7BF, 0x6EC7}, //7427 #CJK UNIFIED IDEOGRAPH
    {0xB7C0, 0x6EC5}, //7428 #CJK UNIFIED IDEOGRAPH
    {0xB7C1, 0x6EA5}, //7429 #CJK UNIFIED IDEOGRAPH
    {0xB7C2, 0x6E98}, //7430 #CJK UNIFIED IDEOGRAPH
    {0xB7C3, 0x6EBC}, //7431 #CJK UNIFIED IDEOGRAPH
    {0xB7C4, 0x6EBA}, //7432 #CJK UNIFIED IDEOGRAPH
    {0xB7C5, 0x6EAB}, //7433 #CJK UNIFIED IDEOGRAPH
    {0xB7C6, 0x6ED1}, //7434 #CJK UNIFIED IDEOGRAPH
    {0xB7C7, 0x6E96}, //7435 #CJK UNIFIED IDEOGRAPH
    {0xB7C8, 0x6E9C}, //7436 #CJK UNIFIED IDEOGRAPH
    {0xB7C9, 0x6EC4}, //7437 #CJK UNIFIED IDEOGRAPH
    {0xB7CA, 0x6ED4}, //7438 #CJK UNIFIED IDEOGRAPH
    {0xB7CB, 0x6EAA}, //7439 #CJK UNIFIED IDEOGRAPH
    {0xB7CC, 0x6EA7}, //7440 #CJK UNIFIED IDEOGRAPH
    {0xB7CD, 0x6EB4}, //7441 #CJK UNIFIED IDEOGRAPH
    {0xB7CE, 0x714E}, //7442 #CJK UNIFIED IDEOGRAPH
    {0xB7CF, 0x7159}, //7443 #CJK UNIFIED IDEOGRAPH
    {0xB7D0, 0x7169}, //7444 #CJK UNIFIED IDEOGRAPH
    {0xB7D1, 0x7164}, //7445 #CJK UNIFIED IDEOGRAPH
    {0xB7D2, 0x7149}, //7446 #CJK UNIFIED IDEOGRAPH
    {0xB7D3, 0x7167}, //7447 #CJK UNIFIED IDEOGRAPH
    {0xB7D4, 0x715C}, //7448 #CJK UNIFIED IDEOGRAPH
    {0xB7D5, 0x716C}, //7449 #CJK UNIFIED IDEOGRAPH
    {0xB7D6, 0x7166}, //7450 #CJK UNIFIED IDEOGRAPH
    {0xB7D7, 0x714C}, //7451 #CJK UNIFIED IDEOGRAPH
    {0xB7D8, 0x7165}, //7452 #CJK UNIFIED IDEOGRAPH
    {0xB7D9, 0x715E}, //7453 #CJK UNIFIED IDEOGRAPH
    {0xB7DA, 0x7146}, //7454 #CJK UNIFIED IDEOGRAPH
    {0xB7DB, 0x7168}, //7455 #CJK UNIFIED IDEOGRAPH
    {0xB7DC, 0x7156}, //7456 #CJK UNIFIED IDEOGRAPH
    {0xB7DD, 0x723A}, //7457 #CJK UNIFIED IDEOGRAPH
    {0xB7DE, 0x7252}, //7458 #CJK UNIFIED IDEOGRAPH
    {0xB7DF, 0x7337}, //7459 #CJK UNIFIED IDEOGRAPH
    {0xB7E0, 0x7345}, //7460 #CJK UNIFIED IDEOGRAPH
    {0xB7E1, 0x733F}, //7461 #CJK UNIFIED IDEOGRAPH
    {0xB7E2, 0x733E}, //7462 #CJK UNIFIED IDEOGRAPH
    {0xB7E3, 0x746F}, //7463 #CJK UNIFIED IDEOGRAPH
    {0xB7E4, 0x745A}, //7464 #CJK UNIFIED IDEOGRAPH
    {0xB7E5, 0x7455}, //7465 #CJK UNIFIED IDEOGRAPH
    {0xB7E6, 0x745F}, //7466 #CJK UNIFIED IDEOGRAPH
    {0xB7E7, 0x745E}, //7467 #CJK UNIFIED IDEOGRAPH
    {0xB7E8, 0x7441}, //7468 #CJK UNIFIED IDEOGRAPH
    {0xB7E9, 0x743F}, //7469 #CJK UNIFIED IDEOGRAPH
    {0xB7EA, 0x7459}, //7470 #CJK UNIFIED IDEOGRAPH
    {0xB7EB, 0x745B}, //7471 #CJK UNIFIED IDEOGRAPH
    {0xB7EC, 0x745C}, //7472 #CJK UNIFIED IDEOGRAPH
    {0xB7ED, 0x7576}, //7473 #CJK UNIFIED IDEOGRAPH
    {0xB7EE, 0x7578}, //7474 #CJK UNIFIED IDEOGRAPH
    {0xB7EF, 0x7600}, //7475 #CJK UNIFIED IDEOGRAPH
    {0xB7F0, 0x75F0}, //7476 #CJK UNIFIED IDEOGRAPH
    {0xB7F1, 0x7601}, //7477 #CJK UNIFIED IDEOGRAPH
    {0xB7F2, 0x75F2}, //7478 #CJK UNIFIED IDEOGRAPH
    {0xB7F3, 0x75F1}, //7479 #CJK UNIFIED IDEOGRAPH
    {0xB7F4, 0x75FA}, //7480 #CJK UNIFIED IDEOGRAPH
    {0xB7F5, 0x75FF}, //7481 #CJK UNIFIED IDEOGRAPH
    {0xB7F6, 0x75F4}, //7482 #CJK UNIFIED IDEOGRAPH
    {0xB7F7, 0x75F3}, //7483 #CJK UNIFIED IDEOGRAPH
    {0xB7F8, 0x76DE}, //7484 #CJK UNIFIED IDEOGRAPH
    {0xB7F9, 0x76DF}, //7485 #CJK UNIFIED IDEOGRAPH
    {0xB7FA, 0x775B}, //7486 #CJK UNIFIED IDEOGRAPH
    {0xB7FB, 0x776B}, //7487 #CJK UNIFIED IDEOGRAPH
    {0xB7FC, 0x7766}, //7488 #CJK UNIFIED IDEOGRAPH
    {0xB7FD, 0x775E}, //7489 #CJK UNIFIED IDEOGRAPH
    {0xB7FE, 0x7763}, //7490 #CJK UNIFIED IDEOGRAPH
    {0xB840, 0x7779}, //7491 #CJK UNIFIED IDEOGRAPH
    {0xB841, 0x776A}, //7492 #CJK UNIFIED IDEOGRAPH
    {0xB842, 0x776C}, //7493 #CJK UNIFIED IDEOGRAPH
    {0xB843, 0x775C}, //7494 #CJK UNIFIED IDEOGRAPH
    {0xB844, 0x7765}, //7495 #CJK UNIFIED IDEOGRAPH
    {0xB845, 0x7768}, //7496 #CJK UNIFIED IDEOGRAPH
    {0xB846, 0x7762}, //7497 #CJK UNIFIED IDEOGRAPH
    {0xB847, 0x77EE}, //7498 #CJK UNIFIED IDEOGRAPH
    {0xB848, 0x788E}, //7499 #CJK UNIFIED IDEOGRAPH
    {0xB849, 0x78B0}, //7500 #CJK UNIFIED IDEOGRAPH
    {0xB84A, 0x7897}, //7501 #CJK UNIFIED IDEOGRAPH
    {0xB84B, 0x7898}, //7502 #CJK UNIFIED IDEOGRAPH
    {0xB84C, 0x788C}, //7503 #CJK UNIFIED IDEOGRAPH
    {0xB84D, 0x7889}, //7504 #CJK UNIFIED IDEOGRAPH
    {0xB84E, 0x787C}, //7505 #CJK UNIFIED IDEOGRAPH
    {0xB84F, 0x7891}, //7506 #CJK UNIFIED IDEOGRAPH
    {0xB850, 0x7893}, //7507 #CJK UNIFIED IDEOGRAPH
    {0xB851, 0x787F}, //7508 #CJK UNIFIED IDEOGRAPH
    {0xB852, 0x797A}, //7509 #CJK UNIFIED IDEOGRAPH
    {0xB853, 0x797F}, //7510 #CJK UNIFIED IDEOGRAPH
    {0xB854, 0x7981}, //7511 #CJK UNIFIED IDEOGRAPH
    {0xB855, 0x842C}, //7512 #CJK UNIFIED IDEOGRAPH
    {0xB856, 0x79BD}, //7513 #CJK UNIFIED IDEOGRAPH
    {0xB857, 0x7A1C}, //7514 #CJK UNIFIED IDEOGRAPH
    {0xB858, 0x7A1A}, //7515 #CJK UNIFIED IDEOGRAPH
    {0xB859, 0x7A20}, //7516 #CJK UNIFIED IDEOGRAPH
    {0xB85A, 0x7A14}, //7517 #CJK UNIFIED IDEOGRAPH
    {0xB85B, 0x7A1F}, //7518 #CJK UNIFIED IDEOGRAPH
    {0xB85C, 0x7A1E}, //7519 #CJK UNIFIED IDEOGRAPH
    {0xB85D, 0x7A9F}, //7520 #CJK UNIFIED IDEOGRAPH
    {0xB85E, 0x7AA0}, //7521 #CJK UNIFIED IDEOGRAPH
    {0xB85F, 0x7B77}, //7522 #CJK UNIFIED IDEOGRAPH
    {0xB860, 0x7BC0}, //7523 #CJK UNIFIED IDEOGRAPH
    {0xB861, 0x7B60}, //7524 #CJK UNIFIED IDEOGRAPH
    {0xB862, 0x7B6E}, //7525 #CJK UNIFIED IDEOGRAPH
    {0xB863, 0x7B67}, //7526 #CJK UNIFIED IDEOGRAPH
    {0xB864, 0x7CB1}, //7527 #CJK UNIFIED IDEOGRAPH
    {0xB865, 0x7CB3}, //7528 #CJK UNIFIED IDEOGRAPH
    {0xB866, 0x7CB5}, //7529 #CJK UNIFIED IDEOGRAPH
    {0xB867, 0x7D93}, //7530 #CJK UNIFIED IDEOGRAPH
    {0xB868, 0x7D79}, //7531 #CJK UNIFIED IDEOGRAPH
    {0xB869, 0x7D91}, //7532 #CJK UNIFIED IDEOGRAPH
    {0xB86A, 0x7D81}, //7533 #CJK UNIFIED IDEOGRAPH
    {0xB86B, 0x7D8F}, //7534 #CJK UNIFIED IDEOGRAPH
    {0xB86C, 0x7D5B}, //7535 #CJK UNIFIED IDEOGRAPH
    {0xB86D, 0x7F6E}, //7536 #CJK UNIFIED IDEOGRAPH
    {0xB86E, 0x7F69}, //7537 #CJK UNIFIED IDEOGRAPH
    {0xB86F, 0x7F6A}, //7538 #CJK UNIFIED IDEOGRAPH
    {0xB870, 0x7F72}, //7539 #CJK UNIFIED IDEOGRAPH
    {0xB871, 0x7FA9}, //7540 #CJK UNIFIED IDEOGRAPH
    {0xB872, 0x7FA8}, //7541 #CJK UNIFIED IDEOGRAPH
    {0xB873, 0x7FA4}, //7542 #CJK UNIFIED IDEOGRAPH
    {0xB874, 0x8056}, //7543 #CJK UNIFIED IDEOGRAPH
    {0xB875, 0x8058}, //7544 #CJK UNIFIED IDEOGRAPH
    {0xB876, 0x8086}, //7545 #CJK UNIFIED IDEOGRAPH
    {0xB877, 0x8084}, //7546 #CJK UNIFIED IDEOGRAPH
    {0xB878, 0x8171}, //7547 #CJK UNIFIED IDEOGRAPH
    {0xB879, 0x8170}, //7548 #CJK UNIFIED IDEOGRAPH
    {0xB87A, 0x8178}, //7549 #CJK UNIFIED IDEOGRAPH
    {0xB87B, 0x8165}, //7550 #CJK UNIFIED IDEOGRAPH
    {0xB87C, 0x816E}, //7551 #CJK UNIFIED IDEOGRAPH
    {0xB87D, 0x8173}, //7552 #CJK UNIFIED IDEOGRAPH
    {0xB87E, 0x816B}, //7553 #CJK UNIFIED IDEOGRAPH
    {0xB8A1, 0x8179}, //7554 #CJK UNIFIED IDEOGRAPH
    {0xB8A2, 0x817A}, //7555 #CJK UNIFIED IDEOGRAPH
    {0xB8A3, 0x8166}, //7556 #CJK UNIFIED IDEOGRAPH
    {0xB8A4, 0x8205}, //7557 #CJK UNIFIED IDEOGRAPH
    {0xB8A5, 0x8247}, //7558 #CJK UNIFIED IDEOGRAPH
    {0xB8A6, 0x8482}, //7559 #CJK UNIFIED IDEOGRAPH
    {0xB8A7, 0x8477}, //7560 #CJK UNIFIED IDEOGRAPH
    {0xB8A8, 0x843D}, //7561 #CJK UNIFIED IDEOGRAPH
    {0xB8A9, 0x8431}, //7562 #CJK UNIFIED IDEOGRAPH
    {0xB8AA, 0x8475}, //7563 #CJK UNIFIED IDEOGRAPH
    {0xB8AB, 0x8466}, //7564 #CJK UNIFIED IDEOGRAPH
    {0xB8AC, 0x846B}, //7565 #CJK UNIFIED IDEOGRAPH
    {0xB8AD, 0x8449}, //7566 #CJK UNIFIED IDEOGRAPH
    {0xB8AE, 0x846C}, //7567 #CJK UNIFIED IDEOGRAPH
    {0xB8AF, 0x845B}, //7568 #CJK UNIFIED IDEOGRAPH
    {0xB8B0, 0x843C}, //7569 #CJK UNIFIED IDEOGRAPH
    {0xB8B1, 0x8435}, //7570 #CJK UNIFIED IDEOGRAPH
    {0xB8B2, 0x8461}, //7571 #CJK UNIFIED IDEOGRAPH
    {0xB8B3, 0x8463}, //7572 #CJK UNIFIED IDEOGRAPH
    {0xB8B4, 0x8469}, //7573 #CJK UNIFIED IDEOGRAPH
    {0xB8B5, 0x846D}, //7574 #CJK UNIFIED IDEOGRAPH
    {0xB8B6, 0x8446}, //7575 #CJK UNIFIED IDEOGRAPH
    {0xB8B7, 0x865E}, //7576 #CJK UNIFIED IDEOGRAPH
    {0xB8B8, 0x865C}, //7577 #CJK UNIFIED IDEOGRAPH
    {0xB8B9, 0x865F}, //7578 #CJK UNIFIED IDEOGRAPH
    {0xB8BA, 0x86F9}, //7579 #CJK UNIFIED IDEOGRAPH
    {0xB8BB, 0x8713}, //7580 #CJK UNIFIED IDEOGRAPH
    {0xB8BC, 0x8708}, //7581 #CJK UNIFIED IDEOGRAPH
    {0xB8BD, 0x8707}, //7582 #CJK UNIFIED IDEOGRAPH
    {0xB8BE, 0x8700}, //7583 #CJK UNIFIED IDEOGRAPH
    {0xB8BF, 0x86FE}, //7584 #CJK UNIFIED IDEOGRAPH
    {0xB8C0, 0x86FB}, //7585 #CJK UNIFIED IDEOGRAPH
    {0xB8C1, 0x8702}, //7586 #CJK UNIFIED IDEOGRAPH
    {0xB8C2, 0x8703}, //7587 #CJK UNIFIED IDEOGRAPH
    {0xB8C3, 0x8706}, //7588 #CJK UNIFIED IDEOGRAPH
    {0xB8C4, 0x870A}, //7589 #CJK UNIFIED IDEOGRAPH
    {0xB8C5, 0x8859}, //7590 #CJK UNIFIED IDEOGRAPH
    {0xB8C6, 0x88DF}, //7591 #CJK UNIFIED IDEOGRAPH
    {0xB8C7, 0x88D4}, //7592 #CJK UNIFIED IDEOGRAPH
    {0xB8C8, 0x88D9}, //7593 #CJK UNIFIED IDEOGRAPH
    {0xB8C9, 0x88DC}, //7594 #CJK UNIFIED IDEOGRAPH
    {0xB8CA, 0x88D8}, //7595 #CJK UNIFIED IDEOGRAPH
    {0xB8CB, 0x88DD}, //7596 #CJK UNIFIED IDEOGRAPH
    {0xB8CC, 0x88E1}, //7597 #CJK UNIFIED IDEOGRAPH
    {0xB8CD, 0x88CA}, //7598 #CJK UNIFIED IDEOGRAPH
    {0xB8CE, 0x88D5}, //7599 #CJK UNIFIED IDEOGRAPH
    {0xB8CF, 0x88D2}, //7600 #CJK UNIFIED IDEOGRAPH
    {0xB8D0, 0x899C}, //7601 #CJK UNIFIED IDEOGRAPH
    {0xB8D1, 0x89E3}, //7602 #CJK UNIFIED IDEOGRAPH
    {0xB8D2, 0x8A6B}, //7603 #CJK UNIFIED IDEOGRAPH
    {0xB8D3, 0x8A72}, //7604 #CJK UNIFIED IDEOGRAPH
    {0xB8D4, 0x8A73}, //7605 #CJK UNIFIED IDEOGRAPH
    {0xB8D5, 0x8A66}, //7606 #CJK UNIFIED IDEOGRAPH
    {0xB8D6, 0x8A69}, //7607 #CJK UNIFIED IDEOGRAPH
    {0xB8D7, 0x8A70}, //7608 #CJK UNIFIED IDEOGRAPH
    {0xB8D8, 0x8A87}, //7609 #CJK UNIFIED IDEOGRAPH
    {0xB8D9, 0x8A7C}, //7610 #CJK UNIFIED IDEOGRAPH
    {0xB8DA, 0x8A63}, //7611 #CJK UNIFIED IDEOGRAPH
    {0xB8DB, 0x8AA0}, //7612 #CJK UNIFIED IDEOGRAPH
    {0xB8DC, 0x8A71}, //7613 #CJK UNIFIED IDEOGRAPH
    {0xB8DD, 0x8A85}, //7614 #CJK UNIFIED IDEOGRAPH
    {0xB8DE, 0x8A6D}, //7615 #CJK UNIFIED IDEOGRAPH
    {0xB8DF, 0x8A62}, //7616 #CJK UNIFIED IDEOGRAPH
    {0xB8E0, 0x8A6E}, //7617 #CJK UNIFIED IDEOGRAPH
    {0xB8E1, 0x8A6C}, //7618 #CJK UNIFIED IDEOGRAPH
    {0xB8E2, 0x8A79}, //7619 #CJK UNIFIED IDEOGRAPH
    {0xB8E3, 0x8A7B}, //7620 #CJK UNIFIED IDEOGRAPH
    {0xB8E4, 0x8A3E}, //7621 #CJK UNIFIED IDEOGRAPH
    {0xB8E5, 0x8A68}, //7622 #CJK UNIFIED IDEOGRAPH
    {0xB8E6, 0x8C62}, //7623 #CJK UNIFIED IDEOGRAPH
    {0xB8E7, 0x8C8A}, //7624 #CJK UNIFIED IDEOGRAPH
    {0xB8E8, 0x8C89}, //7625 #CJK UNIFIED IDEOGRAPH
    {0xB8E9, 0x8CCA}, //7626 #CJK UNIFIED IDEOGRAPH
    {0xB8EA, 0x8CC7}, //7627 #CJK UNIFIED IDEOGRAPH
    {0xB8EB, 0x8CC8}, //7628 #CJK UNIFIED IDEOGRAPH
    {0xB8EC, 0x8CC4}, //7629 #CJK UNIFIED IDEOGRAPH
    {0xB8ED, 0x8CB2}, //7630 #CJK UNIFIED IDEOGRAPH
    {0xB8EE, 0x8CC3}, //7631 #CJK UNIFIED IDEOGRAPH
    {0xB8EF, 0x8CC2}, //7632 #CJK UNIFIED IDEOGRAPH
    {0xB8F0, 0x8CC5}, //7633 #CJK UNIFIED IDEOGRAPH
    {0xB8F1, 0x8DE1}, //7634 #CJK UNIFIED IDEOGRAPH
    {0xB8F2, 0x8DDF}, //7635 #CJK UNIFIED IDEOGRAPH
    {0xB8F3, 0x8DE8}, //7636 #CJK UNIFIED IDEOGRAPH
    {0xB8F4, 0x8DEF}, //7637 #CJK UNIFIED IDEOGRAPH
    {0xB8F5, 0x8DF3}, //7638 #CJK UNIFIED IDEOGRAPH
    {0xB8F6, 0x8DFA}, //7639 #CJK UNIFIED IDEOGRAPH
    {0xB8F7, 0x8DEA}, //7640 #CJK UNIFIED IDEOGRAPH
    {0xB8F8, 0x8DE4}, //7641 #CJK UNIFIED IDEOGRAPH
    {0xB8F9, 0x8DE6}, //7642 #CJK UNIFIED IDEOGRAPH
    {0xB8FA, 0x8EB2}, //7643 #CJK UNIFIED IDEOGRAPH
    {0xB8FB, 0x8F03}, //7644 #CJK UNIFIED IDEOGRAPH
    {0xB8FC, 0x8F09}, //7645 #CJK UNIFIED IDEOGRAPH
    {0xB8FD, 0x8EFE}, //7646 #CJK UNIFIED IDEOGRAPH
    {0xB8FE, 0x8F0A}, //7647 #CJK UNIFIED IDEOGRAPH
    {0xB940, 0x8F9F}, //7648 #CJK UNIFIED IDEOGRAPH
    {0xB941, 0x8FB2}, //7649 #CJK UNIFIED IDEOGRAPH
    {0xB942, 0x904B}, //7650 #CJK UNIFIED IDEOGRAPH
    {0xB943, 0x904A}, //7651 #CJK UNIFIED IDEOGRAPH
    {0xB944, 0x9053}, //7652 #CJK UNIFIED IDEOGRAPH
    {0xB945, 0x9042}, //7653 #CJK UNIFIED IDEOGRAPH
    {0xB946, 0x9054}, //7654 #CJK UNIFIED IDEOGRAPH
    {0xB947, 0x903C}, //7655 #CJK UNIFIED IDEOGRAPH
    {0xB948, 0x9055}, //7656 #CJK UNIFIED IDEOGRAPH
    {0xB949, 0x9050}, //7657 #CJK UNIFIED IDEOGRAPH
    {0xB94A, 0x9047}, //7658 #CJK UNIFIED IDEOGRAPH
    {0xB94B, 0x904F}, //7659 #CJK UNIFIED IDEOGRAPH
    {0xB94C, 0x904E}, //7660 #CJK UNIFIED IDEOGRAPH
    {0xB94D, 0x904D}, //7661 #CJK UNIFIED IDEOGRAPH
    {0xB94E, 0x9051}, //7662 #CJK UNIFIED IDEOGRAPH
    {0xB94F, 0x903E}, //7663 #CJK UNIFIED IDEOGRAPH
    {0xB950, 0x9041}, //7664 #CJK UNIFIED IDEOGRAPH
    {0xB951, 0x9112}, //7665 #CJK UNIFIED IDEOGRAPH
    {0xB952, 0x9117}, //7666 #CJK UNIFIED IDEOGRAPH
    {0xB953, 0x916C}, //7667 #CJK UNIFIED IDEOGRAPH
    {0xB954, 0x916A}, //7668 #CJK UNIFIED IDEOGRAPH
    {0xB955, 0x9169}, //7669 #CJK UNIFIED IDEOGRAPH
    {0xB956, 0x91C9}, //7670 #CJK UNIFIED IDEOGRAPH
    {0xB957, 0x9237}, //7671 #CJK UNIFIED IDEOGRAPH
    {0xB958, 0x9257}, //7672 #CJK UNIFIED IDEOGRAPH
    {0xB959, 0x9238}, //7673 #CJK UNIFIED IDEOGRAPH
    {0xB95A, 0x923D}, //7674 #CJK UNIFIED IDEOGRAPH
    {0xB95B, 0x9240}, //7675 #CJK UNIFIED IDEOGRAPH
    {0xB95C, 0x923E}, //7676 #CJK UNIFIED IDEOGRAPH
    {0xB95D, 0x925B}, //7677 #CJK UNIFIED IDEOGRAPH
    {0xB95E, 0x924B}, //7678 #CJK UNIFIED IDEOGRAPH
    {0xB95F, 0x9264}, //7679 #CJK UNIFIED IDEOGRAPH
    {0xB960, 0x9251}, //7680 #CJK UNIFIED IDEOGRAPH
    {0xB961, 0x9234}, //7681 #CJK UNIFIED IDEOGRAPH
    {0xB962, 0x9249}, //7682 #CJK UNIFIED IDEOGRAPH
    {0xB963, 0x924D}, //7683 #CJK UNIFIED IDEOGRAPH
    {0xB964, 0x9245}, //7684 #CJK UNIFIED IDEOGRAPH
    {0xB965, 0x9239}, //7685 #CJK UNIFIED IDEOGRAPH
    {0xB966, 0x923F}, //7686 #CJK UNIFIED IDEOGRAPH
    {0xB967, 0x925A}, //7687 #CJK UNIFIED IDEOGRAPH
    {0xB968, 0x9598}, //7688 #CJK UNIFIED IDEOGRAPH
    {0xB969, 0x9698}, //7689 #CJK UNIFIED IDEOGRAPH
    {0xB96A, 0x9694}, //7690 #CJK UNIFIED IDEOGRAPH
    {0xB96B, 0x9695}, //7691 #CJK UNIFIED IDEOGRAPH
    {0xB96C, 0x96CD}, //7692 #CJK UNIFIED IDEOGRAPH
    {0xB96D, 0x96CB}, //7693 #CJK UNIFIED IDEOGRAPH
    {0xB96E, 0x96C9}, //7694 #CJK UNIFIED IDEOGRAPH
    {0xB96F, 0x96CA}, //7695 #CJK UNIFIED IDEOGRAPH
    {0xB970, 0x96F7}, //7696 #CJK UNIFIED IDEOGRAPH
    {0xB971, 0x96FB}, //7697 #CJK UNIFIED IDEOGRAPH
    {0xB972, 0x96F9}, //7698 #CJK UNIFIED IDEOGRAPH
    {0xB973, 0x96F6}, //7699 #CJK UNIFIED IDEOGRAPH
    {0xB974, 0x9756}, //7700 #CJK UNIFIED IDEOGRAPH
    {0xB975, 0x9774}, //7701 #CJK UNIFIED IDEOGRAPH
    {0xB976, 0x9776}, //7702 #CJK UNIFIED IDEOGRAPH
    {0xB977, 0x9810}, //7703 #CJK UNIFIED IDEOGRAPH
    {0xB978, 0x9811}, //7704 #CJK UNIFIED IDEOGRAPH
    {0xB979, 0x9813}, //7705 #CJK UNIFIED IDEOGRAPH
    {0xB97A, 0x980A}, //7706 #CJK UNIFIED IDEOGRAPH
    {0xB97B, 0x9812}, //7707 #CJK UNIFIED IDEOGRAPH
    {0xB97C, 0x980C}, //7708 #CJK UNIFIED IDEOGRAPH
    {0xB97D, 0x98FC}, //7709 #CJK UNIFIED IDEOGRAPH
    {0xB97E, 0x98F4}, //7710 #CJK UNIFIED IDEOGRAPH
    {0xB9A1, 0x98FD}, //7711 #CJK UNIFIED IDEOGRAPH
    {0xB9A2, 0x98FE}, //7712 #CJK UNIFIED IDEOGRAPH
    {0xB9A3, 0x99B3}, //7713 #CJK UNIFIED IDEOGRAPH
    {0xB9A4, 0x99B1}, //7714 #CJK UNIFIED IDEOGRAPH
    {0xB9A5, 0x99B4}, //7715 #CJK UNIFIED IDEOGRAPH
    {0xB9A6, 0x9AE1}, //7716 #CJK UNIFIED IDEOGRAPH
    {0xB9A7, 0x9CE9}, //7717 #CJK UNIFIED IDEOGRAPH
    {0xB9A8, 0x9E82}, //7718 #CJK UNIFIED IDEOGRAPH
    {0xB9A9, 0x9F0E}, //7719 #CJK UNIFIED IDEOGRAPH
    {0xB9AA, 0x9F13}, //7720 #CJK UNIFIED IDEOGRAPH
    {0xB9AB, 0x9F20}, //7721 #CJK UNIFIED IDEOGRAPH
    {0xB9AC, 0x50E7}, //7722 #CJK UNIFIED IDEOGRAPH
    {0xB9AD, 0x50EE}, //7723 #CJK UNIFIED IDEOGRAPH
    {0xB9AE, 0x50E5}, //7724 #CJK UNIFIED IDEOGRAPH
    {0xB9AF, 0x50D6}, //7725 #CJK UNIFIED IDEOGRAPH
    {0xB9B0, 0x50ED}, //7726 #CJK UNIFIED IDEOGRAPH
    {0xB9B1, 0x50DA}, //7727 #CJK UNIFIED IDEOGRAPH
    {0xB9B2, 0x50D5}, //7728 #CJK UNIFIED IDEOGRAPH
    {0xB9B3, 0x50CF}, //7729 #CJK UNIFIED IDEOGRAPH
    {0xB9B4, 0x50D1}, //7730 #CJK UNIFIED IDEOGRAPH
    {0xB9B5, 0x50F1}, //7731 #CJK UNIFIED IDEOGRAPH
    {0xB9B6, 0x50CE}, //7732 #CJK UNIFIED IDEOGRAPH
    {0xB9B7, 0x50E9}, //7733 #CJK UNIFIED IDEOGRAPH
    {0xB9B8, 0x5162}, //7734 #CJK UNIFIED IDEOGRAPH
    {0xB9B9, 0x51F3}, //7735 #CJK UNIFIED IDEOGRAPH
    {0xB9BA, 0x5283}, //7736 #CJK UNIFIED IDEOGRAPH
    {0xB9BB, 0x5282}, //7737 #CJK UNIFIED IDEOGRAPH
    {0xB9BC, 0x5331}, //7738 #CJK UNIFIED IDEOGRAPH
    {0xB9BD, 0x53AD}, //7739 #CJK UNIFIED IDEOGRAPH
    {0xB9BE, 0x55FE}, //7740 #CJK UNIFIED IDEOGRAPH
    {0xB9BF, 0x5600}, //7741 #CJK UNIFIED IDEOGRAPH
    {0xB9C0, 0x561B}, //7742 #CJK UNIFIED IDEOGRAPH
    {0xB9C1, 0x5617}, //7743 #CJK UNIFIED IDEOGRAPH
    {0xB9C2, 0x55FD}, //7744 #CJK UNIFIED IDEOGRAPH
    {0xB9C3, 0x5614}, //7745 #CJK UNIFIED IDEOGRAPH
    {0xB9C4, 0x5606}, //7746 #CJK UNIFIED IDEOGRAPH
    {0xB9C5, 0x5609}, //7747 #CJK UNIFIED IDEOGRAPH
    {0xB9C6, 0x560D}, //7748 #CJK UNIFIED IDEOGRAPH
    {0xB9C7, 0x560E}, //7749 #CJK UNIFIED IDEOGRAPH
    {0xB9C8, 0x55F7}, //7750 #CJK UNIFIED IDEOGRAPH
    {0xB9C9, 0x5616}, //7751 #CJK UNIFIED IDEOGRAPH
    {0xB9CA, 0x561F}, //7752 #CJK UNIFIED IDEOGRAPH
    {0xB9CB, 0x5608}, //7753 #CJK UNIFIED IDEOGRAPH
    {0xB9CC, 0x5610}, //7754 #CJK UNIFIED IDEOGRAPH
    {0xB9CD, 0x55F6}, //7755 #CJK UNIFIED IDEOGRAPH
    {0xB9CE, 0x5718}, //7756 #CJK UNIFIED IDEOGRAPH
    {0xB9CF, 0x5716}, //7757 #CJK UNIFIED IDEOGRAPH
    {0xB9D0, 0x5875}, //7758 #CJK UNIFIED IDEOGRAPH
    {0xB9D1, 0x587E}, //7759 #CJK UNIFIED IDEOGRAPH
    {0xB9D2, 0x5883}, //7760 #CJK UNIFIED IDEOGRAPH
    {0xB9D3, 0x5893}, //7761 #CJK UNIFIED IDEOGRAPH
    {0xB9D4, 0x588A}, //7762 #CJK UNIFIED IDEOGRAPH
    {0xB9D5, 0x5879}, //7763 #CJK UNIFIED IDEOGRAPH
    {0xB9D6, 0x5885}, //7764 #CJK UNIFIED IDEOGRAPH
    {0xB9D7, 0x587D}, //7765 #CJK UNIFIED IDEOGRAPH
    {0xB9D8, 0x58FD}, //7766 #CJK UNIFIED IDEOGRAPH
    {0xB9D9, 0x5925}, //7767 #CJK UNIFIED IDEOGRAPH
    {0xB9DA, 0x5922}, //7768 #CJK UNIFIED IDEOGRAPH
    {0xB9DB, 0x5924}, //7769 #CJK UNIFIED IDEOGRAPH
    {0xB9DC, 0x596A}, //7770 #CJK UNIFIED IDEOGRAPH
    {0xB9DD, 0x5969}, //7771 #CJK UNIFIED IDEOGRAPH
    {0xB9DE, 0x5AE1}, //7772 #CJK UNIFIED IDEOGRAPH
    {0xB9DF, 0x5AE6}, //7773 #CJK UNIFIED IDEOGRAPH
    {0xB9E0, 0x5AE9}, //7774 #CJK UNIFIED IDEOGRAPH
    {0xB9E1, 0x5AD7}, //7775 #CJK UNIFIED IDEOGRAPH
    {0xB9E2, 0x5AD6}, //7776 #CJK UNIFIED IDEOGRAPH
    {0xB9E3, 0x5AD8}, //7777 #CJK UNIFIED IDEOGRAPH
    {0xB9E4, 0x5AE3}, //7778 #CJK UNIFIED IDEOGRAPH
    {0xB9E5, 0x5B75}, //7779 #CJK UNIFIED IDEOGRAPH
    {0xB9E6, 0x5BDE}, //7780 #CJK UNIFIED IDEOGRAPH
    {0xB9E7, 0x5BE7}, //7781 #CJK UNIFIED IDEOGRAPH
    {0xB9E8, 0x5BE1}, //7782 #CJK UNIFIED IDEOGRAPH
    {0xB9E9, 0x5BE5}, //7783 #CJK UNIFIED IDEOGRAPH
    {0xB9EA, 0x5BE6}, //7784 #CJK UNIFIED IDEOGRAPH
    {0xB9EB, 0x5BE8}, //7785 #CJK UNIFIED IDEOGRAPH
    {0xB9EC, 0x5BE2}, //7786 #CJK UNIFIED IDEOGRAPH
    {0xB9ED, 0x5BE4}, //7787 #CJK UNIFIED IDEOGRAPH
    {0xB9EE, 0x5BDF}, //7788 #CJK UNIFIED IDEOGRAPH
    {0xB9EF, 0x5C0D}, //7789 #CJK UNIFIED IDEOGRAPH
    {0xB9F0, 0x5C62}, //7790 #CJK UNIFIED IDEOGRAPH
    {0xB9F1, 0x5D84}, //7791 #CJK UNIFIED IDEOGRAPH
    {0xB9F2, 0x5D87}, //7792 #CJK UNIFIED IDEOGRAPH
    {0xB9F3, 0x5E5B}, //7793 #CJK UNIFIED IDEOGRAPH
    {0xB9F4, 0x5E63}, //7794 #CJK UNIFIED IDEOGRAPH
    {0xB9F5, 0x5E55}, //7795 #CJK UNIFIED IDEOGRAPH
    {0xB9F6, 0x5E57}, //7796 #CJK UNIFIED IDEOGRAPH
    {0xB9F7, 0x5E54}, //7797 #CJK UNIFIED IDEOGRAPH
    {0xB9F8, 0x5ED3}, //7798 #CJK UNIFIED IDEOGRAPH
    {0xB9F9, 0x5ED6}, //7799 #CJK UNIFIED IDEOGRAPH
    {0xB9FA, 0x5F0A}, //7800 #CJK UNIFIED IDEOGRAPH
    {0xB9FB, 0x5F46}, //7801 #CJK UNIFIED IDEOGRAPH
    {0xB9FC, 0x5F70}, //7802 #CJK UNIFIED IDEOGRAPH
    {0xB9FD, 0x5FB9}, //7803 #CJK UNIFIED IDEOGRAPH
    {0xB9FE, 0x6147}, //7804 #CJK UNIFIED IDEOGRAPH
    {0xBA40, 0x613F}, //7805 #CJK UNIFIED IDEOGRAPH
    {0xBA41, 0x614B}, //7806 #CJK UNIFIED IDEOGRAPH
    {0xBA42, 0x6177}, //7807 #CJK UNIFIED IDEOGRAPH
    {0xBA43, 0x6162}, //7808 #CJK UNIFIED IDEOGRAPH
    {0xBA44, 0x6163}, //7809 #CJK UNIFIED IDEOGRAPH
    {0xBA45, 0x615F}, //7810 #CJK UNIFIED IDEOGRAPH
    {0xBA46, 0x615A}, //7811 #CJK UNIFIED IDEOGRAPH
    {0xBA47, 0x6158}, //7812 #CJK UNIFIED IDEOGRAPH
    {0xBA48, 0x6175}, //7813 #CJK UNIFIED IDEOGRAPH
    {0xBA49, 0x622A}, //7814 #CJK UNIFIED IDEOGRAPH
    {0xBA4A, 0x6487}, //7815 #CJK UNIFIED IDEOGRAPH
    {0xBA4B, 0x6458}, //7816 #CJK UNIFIED IDEOGRAPH
    {0xBA4C, 0x6454}, //7817 #CJK UNIFIED IDEOGRAPH
    {0xBA4D, 0x64A4}, //7818 #CJK UNIFIED IDEOGRAPH
    {0xBA4E, 0x6478}, //7819 #CJK UNIFIED IDEOGRAPH
    {0xBA4F, 0x645F}, //7820 #CJK UNIFIED IDEOGRAPH
    {0xBA50, 0x647A}, //7821 #CJK UNIFIED IDEOGRAPH
    {0xBA51, 0x6451}, //7822 #CJK UNIFIED IDEOGRAPH
    {0xBA52, 0x6467}, //7823 #CJK UNIFIED IDEOGRAPH
    {0xBA53, 0x6434}, //7824 #CJK UNIFIED IDEOGRAPH
    {0xBA54, 0x646D}, //7825 #CJK UNIFIED IDEOGRAPH
    {0xBA55, 0x647B}, //7826 #CJK UNIFIED IDEOGRAPH
    {0xBA56, 0x6572}, //7827 #CJK UNIFIED IDEOGRAPH
    {0xBA57, 0x65A1}, //7828 #CJK UNIFIED IDEOGRAPH
    {0xBA58, 0x65D7}, //7829 #CJK UNIFIED IDEOGRAPH
    {0xBA59, 0x65D6}, //7830 #CJK UNIFIED IDEOGRAPH
    {0xBA5A, 0x66A2}, //7831 #CJK UNIFIED IDEOGRAPH
    {0xBA5B, 0x66A8}, //7832 #CJK UNIFIED IDEOGRAPH
    {0xBA5C, 0x669D}, //7833 #CJK UNIFIED IDEOGRAPH
    {0xBA5D, 0x699C}, //7834 #CJK UNIFIED IDEOGRAPH
    {0xBA5E, 0x69A8}, //7835 #CJK UNIFIED IDEOGRAPH
    {0xBA5F, 0x6995}, //7836 #CJK UNIFIED IDEOGRAPH
    {0xBA60, 0x69C1}, //7837 #CJK UNIFIED IDEOGRAPH
    {0xBA61, 0x69AE}, //7838 #CJK UNIFIED IDEOGRAPH
    {0xBA62, 0x69D3}, //7839 #CJK UNIFIED IDEOGRAPH
    {0xBA63, 0x69CB}, //7840 #CJK UNIFIED IDEOGRAPH
    {0xBA64, 0x699B}, //7841 #CJK UNIFIED IDEOGRAPH
    {0xBA65, 0x69B7}, //7842 #CJK UNIFIED IDEOGRAPH
    {0xBA66, 0x69BB}, //7843 #CJK UNIFIED IDEOGRAPH
    {0xBA67, 0x69AB}, //7844 #CJK UNIFIED IDEOGRAPH
    {0xBA68, 0x69B4}, //7845 #CJK UNIFIED IDEOGRAPH
    {0xBA69, 0x69D0}, //7846 #CJK UNIFIED IDEOGRAPH
    {0xBA6A, 0x69CD}, //7847 #CJK UNIFIED IDEOGRAPH
    {0xBA6B, 0x69AD}, //7848 #CJK UNIFIED IDEOGRAPH
    {0xBA6C, 0x69CC}, //7849 #CJK UNIFIED IDEOGRAPH
    {0xBA6D, 0x69A6}, //7850 #CJK UNIFIED IDEOGRAPH
    {0xBA6E, 0x69C3}, //7851 #CJK UNIFIED IDEOGRAPH
    {0xBA6F, 0x69A3}, //7852 #CJK UNIFIED IDEOGRAPH
    {0xBA70, 0x6B49}, //7853 #CJK UNIFIED IDEOGRAPH
    {0xBA71, 0x6B4C}, //7854 #CJK UNIFIED IDEOGRAPH
    {0xBA72, 0x6C33}, //7855 #CJK UNIFIED IDEOGRAPH
    {0xBA73, 0x6F33}, //7856 #CJK UNIFIED IDEOGRAPH
    {0xBA74, 0x6F14}, //7857 #CJK UNIFIED IDEOGRAPH
    {0xBA75, 0x6EFE}, //7858 #CJK UNIFIED IDEOGRAPH
    {0xBA76, 0x6F13}, //7859 #CJK UNIFIED IDEOGRAPH
    {0xBA77, 0x6EF4}, //7860 #CJK UNIFIED IDEOGRAPH
    {0xBA78, 0x6F29}, //7861 #CJK UNIFIED IDEOGRAPH
    {0xBA79, 0x6F3E}, //7862 #CJK UNIFIED IDEOGRAPH
    {0xBA7A, 0x6F20}, //7863 #CJK UNIFIED IDEOGRAPH
    {0xBA7B, 0x6F2C}, //7864 #CJK UNIFIED IDEOGRAPH
    {0xBA7C, 0x6F0F}, //7865 #CJK UNIFIED IDEOGRAPH
    {0xBA7D, 0x6F02}, //7866 #CJK UNIFIED IDEOGRAPH
    {0xBA7E, 0x6F22}, //7867 #CJK UNIFIED IDEOGRAPH
    {0xBAA1, 0x6EFF}, //7868 #CJK UNIFIED IDEOGRAPH
    {0xBAA2, 0x6EEF}, //7869 #CJK UNIFIED IDEOGRAPH
    {0xBAA3, 0x6F06}, //7870 #CJK UNIFIED IDEOGRAPH
    {0xBAA4, 0x6F31}, //7871 #CJK UNIFIED IDEOGRAPH
    {0xBAA5, 0x6F38}, //7872 #CJK UNIFIED IDEOGRAPH
    {0xBAA6, 0x6F32}, //7873 #CJK UNIFIED IDEOGRAPH
    {0xBAA7, 0x6F23}, //7874 #CJK UNIFIED IDEOGRAPH
    {0xBAA8, 0x6F15}, //7875 #CJK UNIFIED IDEOGRAPH
    {0xBAA9, 0x6F2B}, //7876 #CJK UNIFIED IDEOGRAPH
    {0xBAAA, 0x6F2F}, //7877 #CJK UNIFIED IDEOGRAPH
    {0xBAAB, 0x6F88}, //7878 #CJK UNIFIED IDEOGRAPH
    {0xBAAC, 0x6F2A}, //7879 #CJK UNIFIED IDEOGRAPH
    {0xBAAD, 0x6EEC}, //7880 #CJK UNIFIED IDEOGRAPH
    {0xBAAE, 0x6F01}, //7881 #CJK UNIFIED IDEOGRAPH
    {0xBAAF, 0x6EF2}, //7882 #CJK UNIFIED IDEOGRAPH
    {0xBAB0, 0x6ECC}, //7883 #CJK UNIFIED IDEOGRAPH
    {0xBAB1, 0x6EF7}, //7884 #CJK UNIFIED IDEOGRAPH
    {0xBAB2, 0x7194}, //7885 #CJK UNIFIED IDEOGRAPH
    {0xBAB3, 0x7199}, //7886 #CJK UNIFIED IDEOGRAPH
    {0xBAB4, 0x717D}, //7887 #CJK UNIFIED IDEOGRAPH
    {0xBAB5, 0x718A}, //7888 #CJK UNIFIED IDEOGRAPH
    {0xBAB6, 0x7184}, //7889 #CJK UNIFIED IDEOGRAPH
    {0xBAB7, 0x7192}, //7890 #CJK UNIFIED IDEOGRAPH
    {0xBAB8, 0x723E}, //7891 #CJK UNIFIED IDEOGRAPH
    {0xBAB9, 0x7292}, //7892 #CJK UNIFIED IDEOGRAPH
    {0xBABA, 0x7296}, //7893 #CJK UNIFIED IDEOGRAPH
    {0xBABB, 0x7344}, //7894 #CJK UNIFIED IDEOGRAPH
    {0xBABC, 0x7350}, //7895 #CJK UNIFIED IDEOGRAPH
    {0xBABD, 0x7464}, //7896 #CJK UNIFIED IDEOGRAPH
    {0xBABE, 0x7463}, //7897 #CJK UNIFIED IDEOGRAPH
    {0xBABF, 0x746A}, //7898 #CJK UNIFIED IDEOGRAPH
    {0xBAC0, 0x7470}, //7899 #CJK UNIFIED IDEOGRAPH
    {0xBAC1, 0x746D}, //7900 #CJK UNIFIED IDEOGRAPH
    {0xBAC2, 0x7504}, //7901 #CJK UNIFIED IDEOGRAPH
    {0xBAC3, 0x7591}, //7902 #CJK UNIFIED IDEOGRAPH
    {0xBAC4, 0x7627}, //7903 #CJK UNIFIED IDEOGRAPH
    {0xBAC5, 0x760D}, //7904 #CJK UNIFIED IDEOGRAPH
    {0xBAC6, 0x760B}, //7905 #CJK UNIFIED IDEOGRAPH
    {0xBAC7, 0x7609}, //7906 #CJK UNIFIED IDEOGRAPH
    {0xBAC8, 0x7613}, //7907 #CJK UNIFIED IDEOGRAPH
    {0xBAC9, 0x76E1}, //7908 #CJK UNIFIED IDEOGRAPH
    {0xBACA, 0x76E3}, //7909 #CJK UNIFIED IDEOGRAPH
    {0xBACB, 0x7784}, //7910 #CJK UNIFIED IDEOGRAPH
    {0xBACC, 0x777D}, //7911 #CJK UNIFIED IDEOGRAPH
    {0xBACD, 0x777F}, //7912 #CJK UNIFIED IDEOGRAPH
    {0xBACE, 0x7761}, //7913 #CJK UNIFIED IDEOGRAPH
    {0xBACF, 0x78C1}, //7914 #CJK UNIFIED IDEOGRAPH
    {0xBAD0, 0x789F}, //7915 #CJK UNIFIED IDEOGRAPH
    {0xBAD1, 0x78A7}, //7916 #CJK UNIFIED IDEOGRAPH
    {0xBAD2, 0x78B3}, //7917 #CJK UNIFIED IDEOGRAPH
    {0xBAD3, 0x78A9}, //7918 #CJK UNIFIED IDEOGRAPH
    {0xBAD4, 0x78A3}, //7919 #CJK UNIFIED IDEOGRAPH
    {0xBAD5, 0x798E}, //7920 #CJK UNIFIED IDEOGRAPH
    {0xBAD6, 0x798F}, //7921 #CJK UNIFIED IDEOGRAPH
    {0xBAD7, 0x798D}, //7922 #CJK UNIFIED IDEOGRAPH
    {0xBAD8, 0x7A2E}, //7923 #CJK UNIFIED IDEOGRAPH
    {0xBAD9, 0x7A31}, //7924 #CJK UNIFIED IDEOGRAPH
    {0xBADA, 0x7AAA}, //7925 #CJK UNIFIED IDEOGRAPH
    {0xBADB, 0x7AA9}, //7926 #CJK UNIFIED IDEOGRAPH
    {0xBADC, 0x7AED}, //7927 #CJK UNIFIED IDEOGRAPH
    {0xBADD, 0x7AEF}, //7928 #CJK UNIFIED IDEOGRAPH
    {0xBADE, 0x7BA1}, //7929 #CJK UNIFIED IDEOGRAPH
    {0xBADF, 0x7B95}, //7930 #CJK UNIFIED IDEOGRAPH
    {0xBAE0, 0x7B8B}, //7931 #CJK UNIFIED IDEOGRAPH
    {0xBAE1, 0x7B75}, //7932 #CJK UNIFIED IDEOGRAPH
    {0xBAE2, 0x7B97}, //7933 #CJK UNIFIED IDEOGRAPH
    {0xBAE3, 0x7B9D}, //7934 #CJK UNIFIED IDEOGRAPH
    {0xBAE4, 0x7B94}, //7935 #CJK UNIFIED IDEOGRAPH
    {0xBAE5, 0x7B8F}, //7936 #CJK UNIFIED IDEOGRAPH
    {0xBAE6, 0x7BB8}, //7937 #CJK UNIFIED IDEOGRAPH
    {0xBAE7, 0x7B87}, //7938 #CJK UNIFIED IDEOGRAPH
    {0xBAE8, 0x7B84}, //7939 #CJK UNIFIED IDEOGRAPH
    {0xBAE9, 0x7CB9}, //7940 #CJK UNIFIED IDEOGRAPH
    {0xBAEA, 0x7CBD}, //7941 #CJK UNIFIED IDEOGRAPH
    {0xBAEB, 0x7CBE}, //7942 #CJK UNIFIED IDEOGRAPH
    {0xBAEC, 0x7DBB}, //7943 #CJK UNIFIED IDEOGRAPH
    {0xBAED, 0x7DB0}, //7944 #CJK UNIFIED IDEOGRAPH
    {0xBAEE, 0x7D9C}, //7945 #CJK UNIFIED IDEOGRAPH
    {0xBAEF, 0x7DBD}, //7946 #CJK UNIFIED IDEOGRAPH
    {0xBAF0, 0x7DBE}, //7947 #CJK UNIFIED IDEOGRAPH
    {0xBAF1, 0x7DA0}, //7948 #CJK UNIFIED IDEOGRAPH
    {0xBAF2, 0x7DCA}, //7949 #CJK UNIFIED IDEOGRAPH
    {0xBAF3, 0x7DB4}, //7950 #CJK UNIFIED IDEOGRAPH
    {0xBAF4, 0x7DB2}, //7951 #CJK UNIFIED IDEOGRAPH
    {0xBAF5, 0x7DB1}, //7952 #CJK UNIFIED IDEOGRAPH
    {0xBAF6, 0x7DBA}, //7953 #CJK UNIFIED IDEOGRAPH
    {0xBAF7, 0x7DA2}, //7954 #CJK UNIFIED IDEOGRAPH
    {0xBAF8, 0x7DBF}, //7955 #CJK UNIFIED IDEOGRAPH
    {0xBAF9, 0x7DB5}, //7956 #CJK UNIFIED IDEOGRAPH
    {0xBAFA, 0x7DB8}, //7957 #CJK UNIFIED IDEOGRAPH
    {0xBAFB, 0x7DAD}, //7958 #CJK UNIFIED IDEOGRAPH
    {0xBAFC, 0x7DD2}, //7959 #CJK UNIFIED IDEOGRAPH
    {0xBAFD, 0x7DC7}, //7960 #CJK UNIFIED IDEOGRAPH
    {0xBAFE, 0x7DAC}, //7961 #CJK UNIFIED IDEOGRAPH
    {0xBB40, 0x7F70}, //7962 #CJK UNIFIED IDEOGRAPH
    {0xBB41, 0x7FE0}, //7963 #CJK UNIFIED IDEOGRAPH
    {0xBB42, 0x7FE1}, //7964 #CJK UNIFIED IDEOGRAPH
    {0xBB43, 0x7FDF}, //7965 #CJK UNIFIED IDEOGRAPH
    {0xBB44, 0x805E}, //7966 #CJK UNIFIED IDEOGRAPH
    {0xBB45, 0x805A}, //7967 #CJK UNIFIED IDEOGRAPH
    {0xBB46, 0x8087}, //7968 #CJK UNIFIED IDEOGRAPH
    {0xBB47, 0x8150}, //7969 #CJK UNIFIED IDEOGRAPH
    {0xBB48, 0x8180}, //7970 #CJK UNIFIED IDEOGRAPH
    {0xBB49, 0x818F}, //7971 #CJK UNIFIED IDEOGRAPH
    {0xBB4A, 0x8188}, //7972 #CJK UNIFIED IDEOGRAPH
    {0xBB4B, 0x818A}, //7973 #CJK UNIFIED IDEOGRAPH
    {0xBB4C, 0x817F}, //7974 #CJK UNIFIED IDEOGRAPH
    {0xBB4D, 0x8182}, //7975 #CJK UNIFIED IDEOGRAPH
    {0xBB4E, 0x81E7}, //7976 #CJK UNIFIED IDEOGRAPH
    {0xBB4F, 0x81FA}, //7977 #CJK UNIFIED IDEOGRAPH
    {0xBB50, 0x8207}, //7978 #CJK UNIFIED IDEOGRAPH
    {0xBB51, 0x8214}, //7979 #CJK UNIFIED IDEOGRAPH
    {0xBB52, 0x821E}, //7980 #CJK UNIFIED IDEOGRAPH
    {0xBB53, 0x824B}, //7981 #CJK UNIFIED IDEOGRAPH
    {0xBB54, 0x84C9}, //7982 #CJK UNIFIED IDEOGRAPH
    {0xBB55, 0x84BF}, //7983 #CJK UNIFIED IDEOGRAPH
    {0xBB56, 0x84C6}, //7984 #CJK UNIFIED IDEOGRAPH
    {0xBB57, 0x84C4}, //7985 #CJK UNIFIED IDEOGRAPH
    {0xBB58, 0x8499}, //7986 #CJK UNIFIED IDEOGRAPH
    {0xBB59, 0x849E}, //7987 #CJK UNIFIED IDEOGRAPH
    {0xBB5A, 0x84B2}, //7988 #CJK UNIFIED IDEOGRAPH
    {0xBB5B, 0x849C}, //7989 #CJK UNIFIED IDEOGRAPH
    {0xBB5C, 0x84CB}, //7990 #CJK UNIFIED IDEOGRAPH
    {0xBB5D, 0x84B8}, //7991 #CJK UNIFIED IDEOGRAPH
    {0xBB5E, 0x84C0}, //7992 #CJK UNIFIED IDEOGRAPH
    {0xBB5F, 0x84D3}, //7993 #CJK UNIFIED IDEOGRAPH
    {0xBB60, 0x8490}, //7994 #CJK UNIFIED IDEOGRAPH
    {0xBB61, 0x84BC}, //7995 #CJK UNIFIED IDEOGRAPH
    {0xBB62, 0x84D1}, //7996 #CJK UNIFIED IDEOGRAPH
    {0xBB63, 0x84CA}, //7997 #CJK UNIFIED IDEOGRAPH
    {0xBB64, 0x873F}, //7998 #CJK UNIFIED IDEOGRAPH
    {0xBB65, 0x871C}, //7999 #CJK UNIFIED IDEOGRAPH
    {0xBB66, 0x873B}, //8000 #CJK UNIFIED IDEOGRAPH
    {0xBB67, 0x8722}, //8001 #CJK UNIFIED IDEOGRAPH
    {0xBB68, 0x8725}, //8002 #CJK UNIFIED IDEOGRAPH
    {0xBB69, 0x8734}, //8003 #CJK UNIFIED IDEOGRAPH
    {0xBB6A, 0x8718}, //8004 #CJK UNIFIED IDEOGRAPH
    {0xBB6B, 0x8755}, //8005 #CJK UNIFIED IDEOGRAPH
    {0xBB6C, 0x8737}, //8006 #CJK UNIFIED IDEOGRAPH
    {0xBB6D, 0x8729}, //8007 #CJK UNIFIED IDEOGRAPH
    {0xBB6E, 0x88F3}, //8008 #CJK UNIFIED IDEOGRAPH
    {0xBB6F, 0x8902}, //8009 #CJK UNIFIED IDEOGRAPH
    {0xBB70, 0x88F4}, //8010 #CJK UNIFIED IDEOGRAPH
    {0xBB71, 0x88F9}, //8011 #CJK UNIFIED IDEOGRAPH
    {0xBB72, 0x88F8}, //8012 #CJK UNIFIED IDEOGRAPH
    {0xBB73, 0x88FD}, //8013 #CJK UNIFIED IDEOGRAPH
    {0xBB74, 0x88E8}, //8014 #CJK UNIFIED IDEOGRAPH
    {0xBB75, 0x891A}, //8015 #CJK UNIFIED IDEOGRAPH
    {0xBB76, 0x88EF}, //8016 #CJK UNIFIED IDEOGRAPH
    {0xBB77, 0x8AA6}, //8017 #CJK UNIFIED IDEOGRAPH
    {0xBB78, 0x8A8C}, //8018 #CJK UNIFIED IDEOGRAPH
    {0xBB79, 0x8A9E}, //8019 #CJK UNIFIED IDEOGRAPH
    {0xBB7A, 0x8AA3}, //8020 #CJK UNIFIED IDEOGRAPH
    {0xBB7B, 0x8A8D}, //8021 #CJK UNIFIED IDEOGRAPH
    {0xBB7C, 0x8AA1}, //8022 #CJK UNIFIED IDEOGRAPH
    {0xBB7D, 0x8A93}, //8023 #CJK UNIFIED IDEOGRAPH
    {0xBB7E, 0x8AA4}, //8024 #CJK UNIFIED IDEOGRAPH
    {0xBBA1, 0x8AAA}, //8025 #CJK UNIFIED IDEOGRAPH
    {0xBBA2, 0x8AA5}, //8026 #CJK UNIFIED IDEOGRAPH
    {0xBBA3, 0x8AA8}, //8027 #CJK UNIFIED IDEOGRAPH
    {0xBBA4, 0x8A98}, //8028 #CJK UNIFIED IDEOGRAPH
    {0xBBA5, 0x8A91}, //8029 #CJK UNIFIED IDEOGRAPH
    {0xBBA6, 0x8A9A}, //8030 #CJK UNIFIED IDEOGRAPH
    {0xBBA7, 0x8AA7}, //8031 #CJK UNIFIED IDEOGRAPH
    {0xBBA8, 0x8C6A}, //8032 #CJK UNIFIED IDEOGRAPH
    {0xBBA9, 0x8C8D}, //8033 #CJK UNIFIED IDEOGRAPH
    {0xBBAA, 0x8C8C}, //8034 #CJK UNIFIED IDEOGRAPH
    {0xBBAB, 0x8CD3}, //8035 #CJK UNIFIED IDEOGRAPH
    {0xBBAC, 0x8CD1}, //8036 #CJK UNIFIED IDEOGRAPH
    {0xBBAD, 0x8CD2}, //8037 #CJK UNIFIED IDEOGRAPH
    {0xBBAE, 0x8D6B}, //8038 #CJK UNIFIED IDEOGRAPH
    {0xBBAF, 0x8D99}, //8039 #CJK UNIFIED IDEOGRAPH
    {0xBBB0, 0x8D95}, //8040 #CJK UNIFIED IDEOGRAPH
    {0xBBB1, 0x8DFC}, //8041 #CJK UNIFIED IDEOGRAPH
    {0xBBB2, 0x8F14}, //8042 #CJK UNIFIED IDEOGRAPH
    {0xBBB3, 0x8F12}, //8043 #CJK UNIFIED IDEOGRAPH
    {0xBBB4, 0x8F15}, //8044 #CJK UNIFIED IDEOGRAPH
    {0xBBB5, 0x8F13}, //8045 #CJK UNIFIED IDEOGRAPH
    {0xBBB6, 0x8FA3}, //8046 #CJK UNIFIED IDEOGRAPH
    {0xBBB7, 0x9060}, //8047 #CJK UNIFIED IDEOGRAPH
    {0xBBB8, 0x9058}, //8048 #CJK UNIFIED IDEOGRAPH
    {0xBBB9, 0x905C}, //8049 #CJK UNIFIED IDEOGRAPH
    {0xBBBA, 0x9063}, //8050 #CJK UNIFIED IDEOGRAPH
    {0xBBBB, 0x9059}, //8051 #CJK UNIFIED IDEOGRAPH
    {0xBBBC, 0x905E}, //8052 #CJK UNIFIED IDEOGRAPH
    {0xBBBD, 0x9062}, //8053 #CJK UNIFIED IDEOGRAPH
    {0xBBBE, 0x905D}, //8054 #CJK UNIFIED IDEOGRAPH
    {0xBBBF, 0x905B}, //8055 #CJK UNIFIED IDEOGRAPH
    {0xBBC0, 0x9119}, //8056 #CJK UNIFIED IDEOGRAPH
    {0xBBC1, 0x9118}, //8057 #CJK UNIFIED IDEOGRAPH
    {0xBBC2, 0x911E}, //8058 #CJK UNIFIED IDEOGRAPH
    {0xBBC3, 0x9175}, //8059 #CJK UNIFIED IDEOGRAPH
    {0xBBC4, 0x9178}, //8060 #CJK UNIFIED IDEOGRAPH
    {0xBBC5, 0x9177}, //8061 #CJK UNIFIED IDEOGRAPH
    {0xBBC6, 0x9174}, //8062 #CJK UNIFIED IDEOGRAPH
    {0xBBC7, 0x9278}, //8063 #CJK UNIFIED IDEOGRAPH
    {0xBBC8, 0x9280}, //8064 #CJK UNIFIED IDEOGRAPH
    {0xBBC9, 0x9285}, //8065 #CJK UNIFIED IDEOGRAPH
    {0xBBCA, 0x9298}, //8066 #CJK UNIFIED IDEOGRAPH
    {0xBBCB, 0x9296}, //8067 #CJK UNIFIED IDEOGRAPH
    {0xBBCC, 0x927B}, //8068 #CJK UNIFIED IDEOGRAPH
    {0xBBCD, 0x9293}, //8069 #CJK UNIFIED IDEOGRAPH
    {0xBBCE, 0x929C}, //8070 #CJK UNIFIED IDEOGRAPH
    {0xBBCF, 0x92A8}, //8071 #CJK UNIFIED IDEOGRAPH
    {0xBBD0, 0x927C}, //8072 #CJK UNIFIED IDEOGRAPH
    {0xBBD1, 0x9291}, //8073 #CJK UNIFIED IDEOGRAPH
    {0xBBD2, 0x95A1}, //8074 #CJK UNIFIED IDEOGRAPH
    {0xBBD3, 0x95A8}, //8075 #CJK UNIFIED IDEOGRAPH
    {0xBBD4, 0x95A9}, //8076 #CJK UNIFIED IDEOGRAPH
    {0xBBD5, 0x95A3}, //8077 #CJK UNIFIED IDEOGRAPH
    {0xBBD6, 0x95A5}, //8078 #CJK UNIFIED IDEOGRAPH
    {0xBBD7, 0x95A4}, //8079 #CJK UNIFIED IDEOGRAPH
    {0xBBD8, 0x9699}, //8080 #CJK UNIFIED IDEOGRAPH
    {0xBBD9, 0x969C}, //8081 #CJK UNIFIED IDEOGRAPH
    {0xBBDA, 0x969B}, //8082 #CJK UNIFIED IDEOGRAPH
    {0xBBDB, 0x96CC}, //8083 #CJK UNIFIED IDEOGRAPH
    {0xBBDC, 0x96D2}, //8084 #CJK UNIFIED IDEOGRAPH
    {0xBBDD, 0x9700}, //8085 #CJK UNIFIED IDEOGRAPH
    {0xBBDE, 0x977C}, //8086 #CJK UNIFIED IDEOGRAPH
    {0xBBDF, 0x9785}, //8087 #CJK UNIFIED IDEOGRAPH
    {0xBBE0, 0x97F6}, //8088 #CJK UNIFIED IDEOGRAPH
    {0xBBE1, 0x9817}, //8089 #CJK UNIFIED IDEOGRAPH
    {0xBBE2, 0x9818}, //8090 #CJK UNIFIED IDEOGRAPH
    {0xBBE3, 0x98AF}, //8091 #CJK UNIFIED IDEOGRAPH
    {0xBBE4, 0x98B1}, //8092 #CJK UNIFIED IDEOGRAPH
    {0xBBE5, 0x9903}, //8093 #CJK UNIFIED IDEOGRAPH
    {0xBBE6, 0x9905}, //8094 #CJK UNIFIED IDEOGRAPH
    {0xBBE7, 0x990C}, //8095 #CJK UNIFIED IDEOGRAPH
    {0xBBE8, 0x9909}, //8096 #CJK UNIFIED IDEOGRAPH
    {0xBBE9, 0x99C1}, //8097 #CJK UNIFIED IDEOGRAPH
    {0xBBEA, 0x9AAF}, //8098 #CJK UNIFIED IDEOGRAPH
    {0xBBEB, 0x9AB0}, //8099 #CJK UNIFIED IDEOGRAPH
    {0xBBEC, 0x9AE6}, //8100 #CJK UNIFIED IDEOGRAPH
    {0xBBED, 0x9B41}, //8101 #CJK UNIFIED IDEOGRAPH
    {0xBBEE, 0x9B42}, //8102 #CJK UNIFIED IDEOGRAPH
    {0xBBEF, 0x9CF4}, //8103 #CJK UNIFIED IDEOGRAPH
    {0xBBF0, 0x9CF6}, //8104 #CJK UNIFIED IDEOGRAPH
    {0xBBF1, 0x9CF3}, //8105 #CJK UNIFIED IDEOGRAPH
    {0xBBF2, 0x9EBC}, //8106 #CJK UNIFIED IDEOGRAPH
    {0xBBF3, 0x9F3B}, //8107 #CJK UNIFIED IDEOGRAPH
    {0xBBF4, 0x9F4A}, //8108 #CJK UNIFIED IDEOGRAPH
    {0xBBF5, 0x5104}, //8109 #CJK UNIFIED IDEOGRAPH
    {0xBBF6, 0x5100}, //8110 #CJK UNIFIED IDEOGRAPH
    {0xBBF7, 0x50FB}, //8111 #CJK UNIFIED IDEOGRAPH
    {0xBBF8, 0x50F5}, //8112 #CJK UNIFIED IDEOGRAPH
    {0xBBF9, 0x50F9}, //8113 #CJK UNIFIED IDEOGRAPH
    {0xBBFA, 0x5102}, //8114 #CJK UNIFIED IDEOGRAPH
    {0xBBFB, 0x5108}, //8115 #CJK UNIFIED IDEOGRAPH
    {0xBBFC, 0x5109}, //8116 #CJK UNIFIED IDEOGRAPH
    {0xBBFD, 0x5105}, //8117 #CJK UNIFIED IDEOGRAPH
    {0xBBFE, 0x51DC}, //8118 #CJK UNIFIED IDEOGRAPH
    {0xBC40, 0x5287}, //8119 #CJK UNIFIED IDEOGRAPH
    {0xBC41, 0x5288}, //8120 #CJK UNIFIED IDEOGRAPH
    {0xBC42, 0x5289}, //8121 #CJK UNIFIED IDEOGRAPH
    {0xBC43, 0x528D}, //8122 #CJK UNIFIED IDEOGRAPH
    {0xBC44, 0x528A}, //8123 #CJK UNIFIED IDEOGRAPH
    {0xBC45, 0x52F0}, //8124 #CJK UNIFIED IDEOGRAPH
    {0xBC46, 0x53B2}, //8125 #CJK UNIFIED IDEOGRAPH
    {0xBC47, 0x562E}, //8126 #CJK UNIFIED IDEOGRAPH
    {0xBC48, 0x563B}, //8127 #CJK UNIFIED IDEOGRAPH
    {0xBC49, 0x5639}, //8128 #CJK UNIFIED IDEOGRAPH
    {0xBC4A, 0x5632}, //8129 #CJK UNIFIED IDEOGRAPH
    {0xBC4B, 0x563F}, //8130 #CJK UNIFIED IDEOGRAPH
    {0xBC4C, 0x5634}, //8131 #CJK UNIFIED IDEOGRAPH
    {0xBC4D, 0x5629}, //8132 #CJK UNIFIED IDEOGRAPH
    {0xBC4E, 0x5653}, //8133 #CJK UNIFIED IDEOGRAPH
    {0xBC4F, 0x564E}, //8134 #CJK UNIFIED IDEOGRAPH
    {0xBC50, 0x5657}, //8135 #CJK UNIFIED IDEOGRAPH
    {0xBC51, 0x5674}, //8136 #CJK UNIFIED IDEOGRAPH
    {0xBC52, 0x5636}, //8137 #CJK UNIFIED IDEOGRAPH
    {0xBC53, 0x562F}, //8138 #CJK UNIFIED IDEOGRAPH
    {0xBC54, 0x5630}, //8139 #CJK UNIFIED IDEOGRAPH
    {0xBC55, 0x5880}, //8140 #CJK UNIFIED IDEOGRAPH
    {0xBC56, 0x589F}, //8141 #CJK UNIFIED IDEOGRAPH
    {0xBC57, 0x589E}, //8142 #CJK UNIFIED IDEOGRAPH
    {0xBC58, 0x58B3}, //8143 #CJK UNIFIED IDEOGRAPH
    {0xBC59, 0x589C}, //8144 #CJK UNIFIED IDEOGRAPH
    {0xBC5A, 0x58AE}, //8145 #CJK UNIFIED IDEOGRAPH
    {0xBC5B, 0x58A9}, //8146 #CJK UNIFIED IDEOGRAPH
    {0xBC5C, 0x58A6}, //8147 #CJK UNIFIED IDEOGRAPH
    {0xBC5D, 0x596D}, //8148 #CJK UNIFIED IDEOGRAPH
    {0xBC5E, 0x5B09}, //8149 #CJK UNIFIED IDEOGRAPH
    {0xBC5F, 0x5AFB}, //8150 #CJK UNIFIED IDEOGRAPH
    {0xBC60, 0x5B0B}, //8151 #CJK UNIFIED IDEOGRAPH
    {0xBC61, 0x5AF5}, //8152 #CJK UNIFIED IDEOGRAPH
    {0xBC62, 0x5B0C}, //8153 #CJK UNIFIED IDEOGRAPH
    {0xBC63, 0x5B08}, //8154 #CJK UNIFIED IDEOGRAPH
    {0xBC64, 0x5BEE}, //8155 #CJK UNIFIED IDEOGRAPH
    {0xBC65, 0x5BEC}, //8156 #CJK UNIFIED IDEOGRAPH
    {0xBC66, 0x5BE9}, //8157 #CJK UNIFIED IDEOGRAPH
    {0xBC67, 0x5BEB}, //8158 #CJK UNIFIED IDEOGRAPH
    {0xBC68, 0x5C64}, //8159 #CJK UNIFIED IDEOGRAPH
    {0xBC69, 0x5C65}, //8160 #CJK UNIFIED IDEOGRAPH
    {0xBC6A, 0x5D9D}, //8161 #CJK UNIFIED IDEOGRAPH
    {0xBC6B, 0x5D94}, //8162 #CJK UNIFIED IDEOGRAPH
    {0xBC6C, 0x5E62}, //8163 #CJK UNIFIED IDEOGRAPH
    {0xBC6D, 0x5E5F}, //8164 #CJK UNIFIED IDEOGRAPH
    {0xBC6E, 0x5E61}, //8165 #CJK UNIFIED IDEOGRAPH
    {0xBC6F, 0x5EE2}, //8166 #CJK UNIFIED IDEOGRAPH
    {0xBC70, 0x5EDA}, //8167 #CJK UNIFIED IDEOGRAPH
    {0xBC71, 0x5EDF}, //8168 #CJK UNIFIED IDEOGRAPH
    {0xBC72, 0x5EDD}, //8169 #CJK UNIFIED IDEOGRAPH
    {0xBC73, 0x5EE3}, //8170 #CJK UNIFIED IDEOGRAPH
    {0xBC74, 0x5EE0}, //8171 #CJK UNIFIED IDEOGRAPH
    {0xBC75, 0x5F48}, //8172 #CJK UNIFIED IDEOGRAPH
    {0xBC76, 0x5F71}, //8173 #CJK UNIFIED IDEOGRAPH
    {0xBC77, 0x5FB7}, //8174 #CJK UNIFIED IDEOGRAPH
    {0xBC78, 0x5FB5}, //8175 #CJK UNIFIED IDEOGRAPH
    {0xBC79, 0x6176}, //8176 #CJK UNIFIED IDEOGRAPH
    {0xBC7A, 0x6167}, //8177 #CJK UNIFIED IDEOGRAPH
    {0xBC7B, 0x616E}, //8178 #CJK UNIFIED IDEOGRAPH
    {0xBC7C, 0x615D}, //8179 #CJK UNIFIED IDEOGRAPH
    {0xBC7D, 0x6155}, //8180 #CJK UNIFIED IDEOGRAPH
    {0xBC7E, 0x6182}, //8181 #CJK UNIFIED IDEOGRAPH
    {0xBCA1, 0x617C}, //8182 #CJK UNIFIED IDEOGRAPH
    {0xBCA2, 0x6170}, //8183 #CJK UNIFIED IDEOGRAPH
    {0xBCA3, 0x616B}, //8184 #CJK UNIFIED IDEOGRAPH
    {0xBCA4, 0x617E}, //8185 #CJK UNIFIED IDEOGRAPH
    {0xBCA5, 0x61A7}, //8186 #CJK UNIFIED IDEOGRAPH
    {0xBCA6, 0x6190}, //8187 #CJK UNIFIED IDEOGRAPH
    {0xBCA7, 0x61AB}, //8188 #CJK UNIFIED IDEOGRAPH
    {0xBCA8, 0x618E}, //8189 #CJK UNIFIED IDEOGRAPH
    {0xBCA9, 0x61AC}, //8190 #CJK UNIFIED IDEOGRAPH
    {0xBCAA, 0x619A}, //8191 #CJK UNIFIED IDEOGRAPH
    {0xBCAB, 0x61A4}, //8192 #CJK UNIFIED IDEOGRAPH
    {0xBCAC, 0x6194}, //8193 #CJK UNIFIED IDEOGRAPH
    {0xBCAD, 0x61AE}, //8194 #CJK UNIFIED IDEOGRAPH
    {0xBCAE, 0x622E}, //8195 #CJK UNIFIED IDEOGRAPH
    {0xBCAF, 0x6469}, //8196 #CJK UNIFIED IDEOGRAPH
    {0xBCB0, 0x646F}, //8197 #CJK UNIFIED IDEOGRAPH
    {0xBCB1, 0x6479}, //8198 #CJK UNIFIED IDEOGRAPH
    {0xBCB2, 0x649E}, //8199 #CJK UNIFIED IDEOGRAPH
    {0xBCB3, 0x64B2}, //8200 #CJK UNIFIED IDEOGRAPH
    {0xBCB4, 0x6488}, //8201 #CJK UNIFIED IDEOGRAPH
    {0xBCB5, 0x6490}, //8202 #CJK UNIFIED IDEOGRAPH
    {0xBCB6, 0x64B0}, //8203 #CJK UNIFIED IDEOGRAPH
    {0xBCB7, 0x64A5}, //8204 #CJK UNIFIED IDEOGRAPH
    {0xBCB8, 0x6493}, //8205 #CJK UNIFIED IDEOGRAPH
    {0xBCB9, 0x6495}, //8206 #CJK UNIFIED IDEOGRAPH
    {0xBCBA, 0x64A9}, //8207 #CJK UNIFIED IDEOGRAPH
    {0xBCBB, 0x6492}, //8208 #CJK UNIFIED IDEOGRAPH
    {0xBCBC, 0x64AE}, //8209 #CJK UNIFIED IDEOGRAPH
    {0xBCBD, 0x64AD}, //8210 #CJK UNIFIED IDEOGRAPH
    {0xBCBE, 0x64AB}, //8211 #CJK UNIFIED IDEOGRAPH
    {0xBCBF, 0x649A}, //8212 #CJK UNIFIED IDEOGRAPH
    {0xBCC0, 0x64AC}, //8213 #CJK UNIFIED IDEOGRAPH
    {0xBCC1, 0x6499}, //8214 #CJK UNIFIED IDEOGRAPH
    {0xBCC2, 0x64A2}, //8215 #CJK UNIFIED IDEOGRAPH
    {0xBCC3, 0x64B3}, //8216 #CJK UNIFIED IDEOGRAPH
    {0xBCC4, 0x6575}, //8217 #CJK UNIFIED IDEOGRAPH
    {0xBCC5, 0x6577}, //8218 #CJK UNIFIED IDEOGRAPH
    {0xBCC6, 0x6578}, //8219 #CJK UNIFIED IDEOGRAPH
    {0xBCC7, 0x66AE}, //8220 #CJK UNIFIED IDEOGRAPH
    {0xBCC8, 0x66AB}, //8221 #CJK UNIFIED IDEOGRAPH
    {0xBCC9, 0x66B4}, //8222 #CJK UNIFIED IDEOGRAPH
    {0xBCCA, 0x66B1}, //8223 #CJK UNIFIED IDEOGRAPH
    {0xBCCB, 0x6A23}, //8224 #CJK UNIFIED IDEOGRAPH
    {0xBCCC, 0x6A1F}, //8225 #CJK UNIFIED IDEOGRAPH
    {0xBCCD, 0x69E8}, //8226 #CJK UNIFIED IDEOGRAPH
    {0xBCCE, 0x6A01}, //8227 #CJK UNIFIED IDEOGRAPH
    {0xBCCF, 0x6A1E}, //8228 #CJK UNIFIED IDEOGRAPH
    {0xBCD0, 0x6A19}, //8229 #CJK UNIFIED IDEOGRAPH
    {0xBCD1, 0x69FD}, //8230 #CJK UNIFIED IDEOGRAPH
    {0xBCD2, 0x6A21}, //8231 #CJK UNIFIED IDEOGRAPH
    {0xBCD3, 0x6A13}, //8232 #CJK UNIFIED IDEOGRAPH
    {0xBCD4, 0x6A0A}, //8233 #CJK UNIFIED IDEOGRAPH
    {0xBCD5, 0x69F3}, //8234 #CJK UNIFIED IDEOGRAPH
    {0xBCD6, 0x6A02}, //8235 #CJK UNIFIED IDEOGRAPH
    {0xBCD7, 0x6A05}, //8236 #CJK UNIFIED IDEOGRAPH
    {0xBCD8, 0x69ED}, //8237 #CJK UNIFIED IDEOGRAPH
    {0xBCD9, 0x6A11}, //8238 #CJK UNIFIED IDEOGRAPH
    {0xBCDA, 0x6B50}, //8239 #CJK UNIFIED IDEOGRAPH
    {0xBCDB, 0x6B4E}, //8240 #CJK UNIFIED IDEOGRAPH
    {0xBCDC, 0x6BA4}, //8241 #CJK UNIFIED IDEOGRAPH
    {0xBCDD, 0x6BC5}, //8242 #CJK UNIFIED IDEOGRAPH
    {0xBCDE, 0x6BC6}, //8243 #CJK UNIFIED IDEOGRAPH
    {0xBCDF, 0x6F3F}, //8244 #CJK UNIFIED IDEOGRAPH
    {0xBCE0, 0x6F7C}, //8245 #CJK UNIFIED IDEOGRAPH
    {0xBCE1, 0x6F84}, //8246 #CJK UNIFIED IDEOGRAPH
    {0xBCE2, 0x6F51}, //8247 #CJK UNIFIED IDEOGRAPH
    {0xBCE3, 0x6F66}, //8248 #CJK UNIFIED IDEOGRAPH
    {0xBCE4, 0x6F54}, //8249 #CJK UNIFIED IDEOGRAPH
    {0xBCE5, 0x6F86}, //8250 #CJK UNIFIED IDEOGRAPH
    {0xBCE6, 0x6F6D}, //8251 #CJK UNIFIED IDEOGRAPH
    {0xBCE7, 0x6F5B}, //8252 #CJK UNIFIED IDEOGRAPH
    {0xBCE8, 0x6F78}, //8253 #CJK UNIFIED IDEOGRAPH
    {0xBCE9, 0x6F6E}, //8254 #CJK UNIFIED IDEOGRAPH
    {0xBCEA, 0x6F8E}, //8255 #CJK UNIFIED IDEOGRAPH
    {0xBCEB, 0x6F7A}, //8256 #CJK UNIFIED IDEOGRAPH
    {0xBCEC, 0x6F70}, //8257 #CJK UNIFIED IDEOGRAPH
    {0xBCED, 0x6F64}, //8258 #CJK UNIFIED IDEOGRAPH
    {0xBCEE, 0x6F97}, //8259 #CJK UNIFIED IDEOGRAPH
    {0xBCEF, 0x6F58}, //8260 #CJK UNIFIED IDEOGRAPH
    {0xBCF0, 0x6ED5}, //8261 #CJK UNIFIED IDEOGRAPH
    {0xBCF1, 0x6F6F}, //8262 #CJK UNIFIED IDEOGRAPH
    {0xBCF2, 0x6F60}, //8263 #CJK UNIFIED IDEOGRAPH
    {0xBCF3, 0x6F5F}, //8264 #CJK UNIFIED IDEOGRAPH
    {0xBCF4, 0x719F}, //8265 #CJK UNIFIED IDEOGRAPH
    {0xBCF5, 0x71AC}, //8266 #CJK UNIFIED IDEOGRAPH
    {0xBCF6, 0x71B1}, //8267 #CJK UNIFIED IDEOGRAPH
    {0xBCF7, 0x71A8}, //8268 #CJK UNIFIED IDEOGRAPH
    {0xBCF8, 0x7256}, //8269 #CJK UNIFIED IDEOGRAPH
    {0xBCF9, 0x729B}, //8270 #CJK UNIFIED IDEOGRAPH
    {0xBCFA, 0x734E}, //8271 #CJK UNIFIED IDEOGRAPH
    {0xBCFB, 0x7357}, //8272 #CJK UNIFIED IDEOGRAPH
    {0xBCFC, 0x7469}, //8273 #CJK UNIFIED IDEOGRAPH
    {0xBCFD, 0x748B}, //8274 #CJK UNIFIED IDEOGRAPH
    {0xBCFE, 0x7483}, //8275 #CJK UNIFIED IDEOGRAPH
    {0xBD40, 0x747E}, //8276 #CJK UNIFIED IDEOGRAPH
    {0xBD41, 0x7480}, //8277 #CJK UNIFIED IDEOGRAPH
    {0xBD42, 0x757F}, //8278 #CJK UNIFIED IDEOGRAPH
    {0xBD43, 0x7620}, //8279 #CJK UNIFIED IDEOGRAPH
    {0xBD44, 0x7629}, //8280 #CJK UNIFIED IDEOGRAPH
    {0xBD45, 0x761F}, //8281 #CJK UNIFIED IDEOGRAPH
    {0xBD46, 0x7624}, //8282 #CJK UNIFIED IDEOGRAPH
    {0xBD47, 0x7626}, //8283 #CJK UNIFIED IDEOGRAPH
    {0xBD48, 0x7621}, //8284 #CJK UNIFIED IDEOGRAPH
    {0xBD49, 0x7622}, //8285 #CJK UNIFIED IDEOGRAPH
    {0xBD4A, 0x769A}, //8286 #CJK UNIFIED IDEOGRAPH
    {0xBD4B, 0x76BA}, //8287 #CJK UNIFIED IDEOGRAPH
    {0xBD4C, 0x76E4}, //8288 #CJK UNIFIED IDEOGRAPH
    {0xBD4D, 0x778E}, //8289 #CJK UNIFIED IDEOGRAPH
    {0xBD4E, 0x7787}, //8290 #CJK UNIFIED IDEOGRAPH
    {0xBD4F, 0x778C}, //8291 #CJK UNIFIED IDEOGRAPH
    {0xBD50, 0x7791}, //8292 #CJK UNIFIED IDEOGRAPH
    {0xBD51, 0x778B}, //8293 #CJK UNIFIED IDEOGRAPH
    {0xBD52, 0x78CB}, //8294 #CJK UNIFIED IDEOGRAPH
    {0xBD53, 0x78C5}, //8295 #CJK UNIFIED IDEOGRAPH
    {0xBD54, 0x78BA}, //8296 #CJK UNIFIED IDEOGRAPH
    {0xBD55, 0x78CA}, //8297 #CJK UNIFIED IDEOGRAPH
    {0xBD56, 0x78BE}, //8298 #CJK UNIFIED IDEOGRAPH
    {0xBD57, 0x78D5}, //8299 #CJK UNIFIED IDEOGRAPH
    {0xBD58, 0x78BC}, //8300 #CJK UNIFIED IDEOGRAPH
    {0xBD59, 0x78D0}, //8301 #CJK UNIFIED IDEOGRAPH
    {0xBD5A, 0x7A3F}, //8302 #CJK UNIFIED IDEOGRAPH
    {0xBD5B, 0x7A3C}, //8303 #CJK UNIFIED IDEOGRAPH
    {0xBD5C, 0x7A40}, //8304 #CJK UNIFIED IDEOGRAPH
    {0xBD5D, 0x7A3D}, //8305 #CJK UNIFIED IDEOGRAPH
    {0xBD5E, 0x7A37}, //8306 #CJK UNIFIED IDEOGRAPH
    {0xBD5F, 0x7A3B}, //8307 #CJK UNIFIED IDEOGRAPH
    {0xBD60, 0x7AAF}, //8308 #CJK UNIFIED IDEOGRAPH
    {0xBD61, 0x7AAE}, //8309 #CJK UNIFIED IDEOGRAPH
    {0xBD62, 0x7BAD}, //8310 #CJK UNIFIED IDEOGRAPH
    {0xBD63, 0x7BB1}, //8311 #CJK UNIFIED IDEOGRAPH
    {0xBD64, 0x7BC4}, //8312 #CJK UNIFIED IDEOGRAPH
    {0xBD65, 0x7BB4}, //8313 #CJK UNIFIED IDEOGRAPH
    {0xBD66, 0x7BC6}, //8314 #CJK UNIFIED IDEOGRAPH
    {0xBD67, 0x7BC7}, //8315 #CJK UNIFIED IDEOGRAPH
    {0xBD68, 0x7BC1}, //8316 #CJK UNIFIED IDEOGRAPH
    {0xBD69, 0x7BA0}, //8317 #CJK UNIFIED IDEOGRAPH
    {0xBD6A, 0x7BCC}, //8318 #CJK UNIFIED IDEOGRAPH
    {0xBD6B, 0x7CCA}, //8319 #CJK UNIFIED IDEOGRAPH
    {0xBD6C, 0x7DE0}, //8320 #CJK UNIFIED IDEOGRAPH
    {0xBD6D, 0x7DF4}, //8321 #CJK UNIFIED IDEOGRAPH
    {0xBD6E, 0x7DEF}, //8322 #CJK UNIFIED IDEOGRAPH
    {0xBD6F, 0x7DFB}, //8323 #CJK UNIFIED IDEOGRAPH
    {0xBD70, 0x7DD8}, //8324 #CJK UNIFIED IDEOGRAPH
    {0xBD71, 0x7DEC}, //8325 #CJK UNIFIED IDEOGRAPH
    {0xBD72, 0x7DDD}, //8326 #CJK UNIFIED IDEOGRAPH
    {0xBD73, 0x7DE8}, //8327 #CJK UNIFIED IDEOGRAPH
    {0xBD74, 0x7DE3}, //8328 #CJK UNIFIED IDEOGRAPH
    {0xBD75, 0x7DDA}, //8329 #CJK UNIFIED IDEOGRAPH
    {0xBD76, 0x7DDE}, //8330 #CJK UNIFIED IDEOGRAPH
    {0xBD77, 0x7DE9}, //8331 #CJK UNIFIED IDEOGRAPH
    {0xBD78, 0x7D9E}, //8332 #CJK UNIFIED IDEOGRAPH
    {0xBD79, 0x7DD9}, //8333 #CJK UNIFIED IDEOGRAPH
    {0xBD7A, 0x7DF2}, //8334 #CJK UNIFIED IDEOGRAPH
    {0xBD7B, 0x7DF9}, //8335 #CJK UNIFIED IDEOGRAPH
    {0xBD7C, 0x7F75}, //8336 #CJK UNIFIED IDEOGRAPH
    {0xBD7D, 0x7F77}, //8337 #CJK UNIFIED IDEOGRAPH
    {0xBD7E, 0x7FAF}, //8338 #CJK UNIFIED IDEOGRAPH
    {0xBDA1, 0x7FE9}, //8339 #CJK UNIFIED IDEOGRAPH
    {0xBDA2, 0x8026}, //8340 #CJK UNIFIED IDEOGRAPH
    {0xBDA3, 0x819B}, //8341 #CJK UNIFIED IDEOGRAPH
    {0xBDA4, 0x819C}, //8342 #CJK UNIFIED IDEOGRAPH
    {0xBDA5, 0x819D}, //8343 #CJK UNIFIED IDEOGRAPH
    {0xBDA6, 0x81A0}, //8344 #CJK UNIFIED IDEOGRAPH
    {0xBDA7, 0x819A}, //8345 #CJK UNIFIED IDEOGRAPH
    {0xBDA8, 0x8198}, //8346 #CJK UNIFIED IDEOGRAPH
    {0xBDA9, 0x8517}, //8347 #CJK UNIFIED IDEOGRAPH
    {0xBDAA, 0x853D}, //8348 #CJK UNIFIED IDEOGRAPH
    {0xBDAB, 0x851A}, //8349 #CJK UNIFIED IDEOGRAPH
    {0xBDAC, 0x84EE}, //8350 #CJK UNIFIED IDEOGRAPH
    {0xBDAD, 0x852C}, //8351 #CJK UNIFIED IDEOGRAPH
    {0xBDAE, 0x852D}, //8352 #CJK UNIFIED IDEOGRAPH
    {0xBDAF, 0x8513}, //8353 #CJK UNIFIED IDEOGRAPH
    {0xBDB0, 0x8511}, //8354 #CJK UNIFIED IDEOGRAPH
    {0xBDB1, 0x8523}, //8355 #CJK UNIFIED IDEOGRAPH
    {0xBDB2, 0x8521}, //8356 #CJK UNIFIED IDEOGRAPH
    {0xBDB3, 0x8514}, //8357 #CJK UNIFIED IDEOGRAPH
    {0xBDB4, 0x84EC}, //8358 #CJK UNIFIED IDEOGRAPH
    {0xBDB5, 0x8525}, //8359 #CJK UNIFIED IDEOGRAPH
    {0xBDB6, 0x84FF}, //8360 #CJK UNIFIED IDEOGRAPH
    {0xBDB7, 0x8506}, //8361 #CJK UNIFIED IDEOGRAPH
    {0xBDB8, 0x8782}, //8362 #CJK UNIFIED IDEOGRAPH
    {0xBDB9, 0x8774}, //8363 #CJK UNIFIED IDEOGRAPH
    {0xBDBA, 0x8776}, //8364 #CJK UNIFIED IDEOGRAPH
    {0xBDBB, 0x8760}, //8365 #CJK UNIFIED IDEOGRAPH
    {0xBDBC, 0x8766}, //8366 #CJK UNIFIED IDEOGRAPH
    {0xBDBD, 0x8778}, //8367 #CJK UNIFIED IDEOGRAPH
    {0xBDBE, 0x8768}, //8368 #CJK UNIFIED IDEOGRAPH
    {0xBDBF, 0x8759}, //8369 #CJK UNIFIED IDEOGRAPH
    {0xBDC0, 0x8757}, //8370 #CJK UNIFIED IDEOGRAPH
    {0xBDC1, 0x874C}, //8371 #CJK UNIFIED IDEOGRAPH
    {0xBDC2, 0x8753}, //8372 #CJK UNIFIED IDEOGRAPH
    {0xBDC3, 0x885B}, //8373 #CJK UNIFIED IDEOGRAPH
    {0xBDC4, 0x885D}, //8374 #CJK UNIFIED IDEOGRAPH
    {0xBDC5, 0x8910}, //8375 #CJK UNIFIED IDEOGRAPH
    {0xBDC6, 0x8907}, //8376 #CJK UNIFIED IDEOGRAPH
    {0xBDC7, 0x8912}, //8377 #CJK UNIFIED IDEOGRAPH
    {0xBDC8, 0x8913}, //8378 #CJK UNIFIED IDEOGRAPH
    {0xBDC9, 0x8915}, //8379 #CJK UNIFIED IDEOGRAPH
    {0xBDCA, 0x890A}, //8380 #CJK UNIFIED IDEOGRAPH
    {0xBDCB, 0x8ABC}, //8381 #CJK UNIFIED IDEOGRAPH
    {0xBDCC, 0x8AD2}, //8382 #CJK UNIFIED IDEOGRAPH
    {0xBDCD, 0x8AC7}, //8383 #CJK UNIFIED IDEOGRAPH
    {0xBDCE, 0x8AC4}, //8384 #CJK UNIFIED IDEOGRAPH
    {0xBDCF, 0x8A95}, //8385 #CJK UNIFIED IDEOGRAPH
    {0xBDD0, 0x8ACB}, //8386 #CJK UNIFIED IDEOGRAPH
    {0xBDD1, 0x8AF8}, //8387 #CJK UNIFIED IDEOGRAPH
    {0xBDD2, 0x8AB2}, //8388 #CJK UNIFIED IDEOGRAPH
    {0xBDD3, 0x8AC9}, //8389 #CJK UNIFIED IDEOGRAPH
    {0xBDD4, 0x8AC2}, //8390 #CJK UNIFIED IDEOGRAPH
    {0xBDD5, 0x8ABF}, //8391 #CJK UNIFIED IDEOGRAPH
    {0xBDD6, 0x8AB0}, //8392 #CJK UNIFIED IDEOGRAPH
    {0xBDD7, 0x8AD6}, //8393 #CJK UNIFIED IDEOGRAPH
    {0xBDD8, 0x8ACD}, //8394 #CJK UNIFIED IDEOGRAPH
    {0xBDD9, 0x8AB6}, //8395 #CJK UNIFIED IDEOGRAPH
    {0xBDDA, 0x8AB9}, //8396 #CJK UNIFIED IDEOGRAPH
    {0xBDDB, 0x8ADB}, //8397 #CJK UNIFIED IDEOGRAPH
    {0xBDDC, 0x8C4C}, //8398 #CJK UNIFIED IDEOGRAPH
    {0xBDDD, 0x8C4E}, //8399 #CJK UNIFIED IDEOGRAPH
    {0xBDDE, 0x8C6C}, //8400 #CJK UNIFIED IDEOGRAPH
    {0xBDDF, 0x8CE0}, //8401 #CJK UNIFIED IDEOGRAPH
    {0xBDE0, 0x8CDE}, //8402 #CJK UNIFIED IDEOGRAPH
    {0xBDE1, 0x8CE6}, //8403 #CJK UNIFIED IDEOGRAPH
    {0xBDE2, 0x8CE4}, //8404 #CJK UNIFIED IDEOGRAPH
    {0xBDE3, 0x8CEC}, //8405 #CJK UNIFIED IDEOGRAPH
    {0xBDE4, 0x8CED}, //8406 #CJK UNIFIED IDEOGRAPH
    {0xBDE5, 0x8CE2}, //8407 #CJK UNIFIED IDEOGRAPH
    {0xBDE6, 0x8CE3}, //8408 #CJK UNIFIED IDEOGRAPH
    {0xBDE7, 0x8CDC}, //8409 #CJK UNIFIED IDEOGRAPH
    {0xBDE8, 0x8CEA}, //8410 #CJK UNIFIED IDEOGRAPH
    {0xBDE9, 0x8CE1}, //8411 #CJK UNIFIED IDEOGRAPH
    {0xBDEA, 0x8D6D}, //8412 #CJK UNIFIED IDEOGRAPH
    {0xBDEB, 0x8D9F}, //8413 #CJK UNIFIED IDEOGRAPH
    {0xBDEC, 0x8DA3}, //8414 #CJK UNIFIED IDEOGRAPH
    {0xBDED, 0x8E2B}, //8415 #CJK UNIFIED IDEOGRAPH
    {0xBDEE, 0x8E10}, //8416 #CJK UNIFIED IDEOGRAPH
    {0xBDEF, 0x8E1D}, //8417 #CJK UNIFIED IDEOGRAPH
    {0xBDF0, 0x8E22}, //8418 #CJK UNIFIED IDEOGRAPH
    {0xBDF1, 0x8E0F}, //8419 #CJK UNIFIED IDEOGRAPH
    {0xBDF2, 0x8E29}, //8420 #CJK UNIFIED IDEOGRAPH
    {0xBDF3, 0x8E1F}, //8421 #CJK UNIFIED IDEOGRAPH
    {0xBDF4, 0x8E21}, //8422 #CJK UNIFIED IDEOGRAPH
    {0xBDF5, 0x8E1E}, //8423 #CJK UNIFIED IDEOGRAPH
    {0xBDF6, 0x8EBA}, //8424 #CJK UNIFIED IDEOGRAPH
    {0xBDF7, 0x8F1D}, //8425 #CJK UNIFIED IDEOGRAPH
    {0xBDF8, 0x8F1B}, //8426 #CJK UNIFIED IDEOGRAPH
    {0xBDF9, 0x8F1F}, //8427 #CJK UNIFIED IDEOGRAPH
    {0xBDFA, 0x8F29}, //8428 #CJK UNIFIED IDEOGRAPH
    {0xBDFB, 0x8F26}, //8429 #CJK UNIFIED IDEOGRAPH
    {0xBDFC, 0x8F2A}, //8430 #CJK UNIFIED IDEOGRAPH
    {0xBDFD, 0x8F1C}, //8431 #CJK UNIFIED IDEOGRAPH
    {0xBDFE, 0x8F1E}, //8432 #CJK UNIFIED IDEOGRAPH
    {0xBE40, 0x8F25}, //8433 #CJK UNIFIED IDEOGRAPH
    {0xBE41, 0x9069}, //8434 #CJK UNIFIED IDEOGRAPH
    {0xBE42, 0x906E}, //8435 #CJK UNIFIED IDEOGRAPH
    {0xBE43, 0x9068}, //8436 #CJK UNIFIED IDEOGRAPH
    {0xBE44, 0x906D}, //8437 #CJK UNIFIED IDEOGRAPH
    {0xBE45, 0x9077}, //8438 #CJK UNIFIED IDEOGRAPH
    {0xBE46, 0x9130}, //8439 #CJK UNIFIED IDEOGRAPH
    {0xBE47, 0x912D}, //8440 #CJK UNIFIED IDEOGRAPH
    {0xBE48, 0x9127}, //8441 #CJK UNIFIED IDEOGRAPH
    {0xBE49, 0x9131}, //8442 #CJK UNIFIED IDEOGRAPH
    {0xBE4A, 0x9187}, //8443 #CJK UNIFIED IDEOGRAPH
    {0xBE4B, 0x9189}, //8444 #CJK UNIFIED IDEOGRAPH
    {0xBE4C, 0x918B}, //8445 #CJK UNIFIED IDEOGRAPH
    {0xBE4D, 0x9183}, //8446 #CJK UNIFIED IDEOGRAPH
    {0xBE4E, 0x92C5}, //8447 #CJK UNIFIED IDEOGRAPH
    {0xBE4F, 0x92BB}, //8448 #CJK UNIFIED IDEOGRAPH
    {0xBE50, 0x92B7}, //8449 #CJK UNIFIED IDEOGRAPH
    {0xBE51, 0x92EA}, //8450 #CJK UNIFIED IDEOGRAPH
    {0xBE52, 0x92AC}, //8451 #CJK UNIFIED IDEOGRAPH
    {0xBE53, 0x92E4}, //8452 #CJK UNIFIED IDEOGRAPH
    {0xBE54, 0x92C1}, //8453 #CJK UNIFIED IDEOGRAPH
    {0xBE55, 0x92B3}, //8454 #CJK UNIFIED IDEOGRAPH
    {0xBE56, 0x92BC}, //8455 #CJK UNIFIED IDEOGRAPH
    {0xBE57, 0x92D2}, //8456 #CJK UNIFIED IDEOGRAPH
    {0xBE58, 0x92C7}, //8457 #CJK UNIFIED IDEOGRAPH
    {0xBE59, 0x92F0}, //8458 #CJK UNIFIED IDEOGRAPH
    {0xBE5A, 0x92B2}, //8459 #CJK UNIFIED IDEOGRAPH
    {0xBE5B, 0x95AD}, //8460 #CJK UNIFIED IDEOGRAPH
    {0xBE5C, 0x95B1}, //8461 #CJK UNIFIED IDEOGRAPH
    {0xBE5D, 0x9704}, //8462 #CJK UNIFIED IDEOGRAPH
    {0xBE5E, 0x9706}, //8463 #CJK UNIFIED IDEOGRAPH
    {0xBE5F, 0x9707}, //8464 #CJK UNIFIED IDEOGRAPH
    {0xBE60, 0x9709}, //8465 #CJK UNIFIED IDEOGRAPH
    {0xBE61, 0x9760}, //8466 #CJK UNIFIED IDEOGRAPH
    {0xBE62, 0x978D}, //8467 #CJK UNIFIED IDEOGRAPH
    {0xBE63, 0x978B}, //8468 #CJK UNIFIED IDEOGRAPH
    {0xBE64, 0x978F}, //8469 #CJK UNIFIED IDEOGRAPH
    {0xBE65, 0x9821}, //8470 #CJK UNIFIED IDEOGRAPH
    {0xBE66, 0x982B}, //8471 #CJK UNIFIED IDEOGRAPH
    {0xBE67, 0x981C}, //8472 #CJK UNIFIED IDEOGRAPH
    {0xBE68, 0x98B3}, //8473 #CJK UNIFIED IDEOGRAPH
    {0xBE69, 0x990A}, //8474 #CJK UNIFIED IDEOGRAPH
    {0xBE6A, 0x9913}, //8475 #CJK UNIFIED IDEOGRAPH
    {0xBE6B, 0x9912}, //8476 #CJK UNIFIED IDEOGRAPH
    {0xBE6C, 0x9918}, //8477 #CJK UNIFIED IDEOGRAPH
    {0xBE6D, 0x99DD}, //8478 #CJK UNIFIED IDEOGRAPH
    {0xBE6E, 0x99D0}, //8479 #CJK UNIFIED IDEOGRAPH
    {0xBE6F, 0x99DF}, //8480 #CJK UNIFIED IDEOGRAPH
    {0xBE70, 0x99DB}, //8481 #CJK UNIFIED IDEOGRAPH
    {0xBE71, 0x99D1}, //8482 #CJK UNIFIED IDEOGRAPH
    {0xBE72, 0x99D5}, //8483 #CJK UNIFIED IDEOGRAPH
    {0xBE73, 0x99D2}, //8484 #CJK UNIFIED IDEOGRAPH
    {0xBE74, 0x99D9}, //8485 #CJK UNIFIED IDEOGRAPH
    {0xBE75, 0x9AB7}, //8486 #CJK UNIFIED IDEOGRAPH
    {0xBE76, 0x9AEE}, //8487 #CJK UNIFIED IDEOGRAPH
    {0xBE77, 0x9AEF}, //8488 #CJK UNIFIED IDEOGRAPH
    {0xBE78, 0x9B27}, //8489 #CJK UNIFIED IDEOGRAPH
    {0xBE79, 0x9B45}, //8490 #CJK UNIFIED IDEOGRAPH
    {0xBE7A, 0x9B44}, //8491 #CJK UNIFIED IDEOGRAPH
    {0xBE7B, 0x9B77}, //8492 #CJK UNIFIED IDEOGRAPH
    {0xBE7C, 0x9B6F}, //8493 #CJK UNIFIED IDEOGRAPH
    {0xBE7D, 0x9D06}, //8494 #CJK UNIFIED IDEOGRAPH
    {0xBE7E, 0x9D09}, //8495 #CJK UNIFIED IDEOGRAPH
    {0xBEA1, 0x9D03}, //8496 #CJK UNIFIED IDEOGRAPH
    {0xBEA2, 0x9EA9}, //8497 #CJK UNIFIED IDEOGRAPH
    {0xBEA3, 0x9EBE}, //8498 #CJK UNIFIED IDEOGRAPH
    {0xBEA4, 0x9ECE}, //8499 #CJK UNIFIED IDEOGRAPH
    {0xBEA5, 0x58A8}, //8500 #CJK UNIFIED IDEOGRAPH
    {0xBEA6, 0x9F52}, //8501 #CJK UNIFIED IDEOGRAPH
    {0xBEA7, 0x5112}, //8502 #CJK UNIFIED IDEOGRAPH
    {0xBEA8, 0x5118}, //8503 #CJK UNIFIED IDEOGRAPH
    {0xBEA9, 0x5114}, //8504 #CJK UNIFIED IDEOGRAPH
    {0xBEAA, 0x5110}, //8505 #CJK UNIFIED IDEOGRAPH
    {0xBEAB, 0x5115}, //8506 #CJK UNIFIED IDEOGRAPH
    {0xBEAC, 0x5180}, //8507 #CJK UNIFIED IDEOGRAPH
    {0xBEAD, 0x51AA}, //8508 #CJK UNIFIED IDEOGRAPH
    {0xBEAE, 0x51DD}, //8509 #CJK UNIFIED IDEOGRAPH
    {0xBEAF, 0x5291}, //8510 #CJK UNIFIED IDEOGRAPH
    {0xBEB0, 0x5293}, //8511 #CJK UNIFIED IDEOGRAPH
    {0xBEB1, 0x52F3}, //8512 #CJK UNIFIED IDEOGRAPH
    {0xBEB2, 0x5659}, //8513 #CJK UNIFIED IDEOGRAPH
    {0xBEB3, 0x566B}, //8514 #CJK UNIFIED IDEOGRAPH
    {0xBEB4, 0x5679}, //8515 #CJK UNIFIED IDEOGRAPH
    {0xBEB5, 0x5669}, //8516 #CJK UNIFIED IDEOGRAPH
    {0xBEB6, 0x5664}, //8517 #CJK UNIFIED IDEOGRAPH
    {0xBEB7, 0x5678}, //8518 #CJK UNIFIED IDEOGRAPH
    {0xBEB8, 0x566A}, //8519 #CJK UNIFIED IDEOGRAPH
    {0xBEB9, 0x5668}, //8520 #CJK UNIFIED IDEOGRAPH
    {0xBEBA, 0x5665}, //8521 #CJK UNIFIED IDEOGRAPH
    {0xBEBB, 0x5671}, //8522 #CJK UNIFIED IDEOGRAPH
    {0xBEBC, 0x566F}, //8523 #CJK UNIFIED IDEOGRAPH
    {0xBEBD, 0x566C}, //8524 #CJK UNIFIED IDEOGRAPH
    {0xBEBE, 0x5662}, //8525 #CJK UNIFIED IDEOGRAPH
    {0xBEBF, 0x5676}, //8526 #CJK UNIFIED IDEOGRAPH
    {0xBEC0, 0x58C1}, //8527 #CJK UNIFIED IDEOGRAPH
    {0xBEC1, 0x58BE}, //8528 #CJK UNIFIED IDEOGRAPH
    {0xBEC2, 0x58C7}, //8529 #CJK UNIFIED IDEOGRAPH
    {0xBEC3, 0x58C5}, //8530 #CJK UNIFIED IDEOGRAPH
    {0xBEC4, 0x596E}, //8531 #CJK UNIFIED IDEOGRAPH
    {0xBEC5, 0x5B1D}, //8532 #CJK UNIFIED IDEOGRAPH
    {0xBEC6, 0x5B34}, //8533 #CJK UNIFIED IDEOGRAPH
    {0xBEC7, 0x5B78}, //8534 #CJK UNIFIED IDEOGRAPH
    {0xBEC8, 0x5BF0}, //8535 #CJK UNIFIED IDEOGRAPH
    {0xBEC9, 0x5C0E}, //8536 #CJK UNIFIED IDEOGRAPH
    {0xBECA, 0x5F4A}, //8537 #CJK UNIFIED IDEOGRAPH
    {0xBECB, 0x61B2}, //8538 #CJK UNIFIED IDEOGRAPH
    {0xBECC, 0x6191}, //8539 #CJK UNIFIED IDEOGRAPH
    {0xBECD, 0x61A9}, //8540 #CJK UNIFIED IDEOGRAPH
    {0xBECE, 0x618A}, //8541 #CJK UNIFIED IDEOGRAPH
    {0xBECF, 0x61CD}, //8542 #CJK UNIFIED IDEOGRAPH
    {0xBED0, 0x61B6}, //8543 #CJK UNIFIED IDEOGRAPH
    {0xBED1, 0x61BE}, //8544 #CJK UNIFIED IDEOGRAPH
    {0xBED2, 0x61CA}, //8545 #CJK UNIFIED IDEOGRAPH
    {0xBED3, 0x61C8}, //8546 #CJK UNIFIED IDEOGRAPH
    {0xBED4, 0x6230}, //8547 #CJK UNIFIED IDEOGRAPH
    {0xBED5, 0x64C5}, //8548 #CJK UNIFIED IDEOGRAPH
    {0xBED6, 0x64C1}, //8549 #CJK UNIFIED IDEOGRAPH
    {0xBED7, 0x64CB}, //8550 #CJK UNIFIED IDEOGRAPH
    {0xBED8, 0x64BB}, //8551 #CJK UNIFIED IDEOGRAPH
    {0xBED9, 0x64BC}, //8552 #CJK UNIFIED IDEOGRAPH
    {0xBEDA, 0x64DA}, //8553 #CJK UNIFIED IDEOGRAPH
    {0xBEDB, 0x64C4}, //8554 #CJK UNIFIED IDEOGRAPH
    {0xBEDC, 0x64C7}, //8555 #CJK UNIFIED IDEOGRAPH
    {0xBEDD, 0x64C2}, //8556 #CJK UNIFIED IDEOGRAPH
    {0xBEDE, 0x64CD}, //8557 #CJK UNIFIED IDEOGRAPH
    {0xBEDF, 0x64BF}, //8558 #CJK UNIFIED IDEOGRAPH
    {0xBEE0, 0x64D2}, //8559 #CJK UNIFIED IDEOGRAPH
    {0xBEE1, 0x64D4}, //8560 #CJK UNIFIED IDEOGRAPH
    {0xBEE2, 0x64BE}, //8561 #CJK UNIFIED IDEOGRAPH
    {0xBEE3, 0x6574}, //8562 #CJK UNIFIED IDEOGRAPH
    {0xBEE4, 0x66C6}, //8563 #CJK UNIFIED IDEOGRAPH
    {0xBEE5, 0x66C9}, //8564 #CJK UNIFIED IDEOGRAPH
    {0xBEE6, 0x66B9}, //8565 #CJK UNIFIED IDEOGRAPH
    {0xBEE7, 0x66C4}, //8566 #CJK UNIFIED IDEOGRAPH
    {0xBEE8, 0x66C7}, //8567 #CJK UNIFIED IDEOGRAPH
    {0xBEE9, 0x66B8}, //8568 #CJK UNIFIED IDEOGRAPH
    {0xBEEA, 0x6A3D}, //8569 #CJK UNIFIED IDEOGRAPH
    {0xBEEB, 0x6A38}, //8570 #CJK UNIFIED IDEOGRAPH
    {0xBEEC, 0x6A3A}, //8571 #CJK UNIFIED IDEOGRAPH
    {0xBEED, 0x6A59}, //8572 #CJK UNIFIED IDEOGRAPH
    {0xBEEE, 0x6A6B}, //8573 #CJK UNIFIED IDEOGRAPH
    {0xBEEF, 0x6A58}, //8574 #CJK UNIFIED IDEOGRAPH
    {0xBEF0, 0x6A39}, //8575 #CJK UNIFIED IDEOGRAPH
    {0xBEF1, 0x6A44}, //8576 #CJK UNIFIED IDEOGRAPH
    {0xBEF2, 0x6A62}, //8577 #CJK UNIFIED IDEOGRAPH
    {0xBEF3, 0x6A61}, //8578 #CJK UNIFIED IDEOGRAPH
    {0xBEF4, 0x6A4B}, //8579 #CJK UNIFIED IDEOGRAPH
    {0xBEF5, 0x6A47}, //8580 #CJK UNIFIED IDEOGRAPH
    {0xBEF6, 0x6A35}, //8581 #CJK UNIFIED IDEOGRAPH
    {0xBEF7, 0x6A5F}, //8582 #CJK UNIFIED IDEOGRAPH
    {0xBEF8, 0x6A48}, //8583 #CJK UNIFIED IDEOGRAPH
    {0xBEF9, 0x6B59}, //8584 #CJK UNIFIED IDEOGRAPH
    {0xBEFA, 0x6B77}, //8585 #CJK UNIFIED IDEOGRAPH
    {0xBEFB, 0x6C05}, //8586 #CJK UNIFIED IDEOGRAPH
    {0xBEFC, 0x6FC2}, //8587 #CJK UNIFIED IDEOGRAPH
    {0xBEFD, 0x6FB1}, //8588 #CJK UNIFIED IDEOGRAPH
    {0xBEFE, 0x6FA1}, //8589 #CJK UNIFIED IDEOGRAPH
    {0xBF40, 0x6FC3}, //8590 #CJK UNIFIED IDEOGRAPH
    {0xBF41, 0x6FA4}, //8591 #CJK UNIFIED IDEOGRAPH
    {0xBF42, 0x6FC1}, //8592 #CJK UNIFIED IDEOGRAPH
    {0xBF43, 0x6FA7}, //8593 #CJK UNIFIED IDEOGRAPH
    {0xBF44, 0x6FB3}, //8594 #CJK UNIFIED IDEOGRAPH
    {0xBF45, 0x6FC0}, //8595 #CJK UNIFIED IDEOGRAPH
    {0xBF46, 0x6FB9}, //8596 #CJK UNIFIED IDEOGRAPH
    {0xBF47, 0x6FB6}, //8597 #CJK UNIFIED IDEOGRAPH
    {0xBF48, 0x6FA6}, //8598 #CJK UNIFIED IDEOGRAPH
    {0xBF49, 0x6FA0}, //8599 #CJK UNIFIED IDEOGRAPH
    {0xBF4A, 0x6FB4}, //8600 #CJK UNIFIED IDEOGRAPH
    {0xBF4B, 0x71BE}, //8601 #CJK UNIFIED IDEOGRAPH
    {0xBF4C, 0x71C9}, //8602 #CJK UNIFIED IDEOGRAPH
    {0xBF4D, 0x71D0}, //8603 #CJK UNIFIED IDEOGRAPH
    {0xBF4E, 0x71D2}, //8604 #CJK UNIFIED IDEOGRAPH
    {0xBF4F, 0x71C8}, //8605 #CJK UNIFIED IDEOGRAPH
    {0xBF50, 0x71D5}, //8606 #CJK UNIFIED IDEOGRAPH
    {0xBF51, 0x71B9}, //8607 #CJK UNIFIED IDEOGRAPH
    {0xBF52, 0x71CE}, //8608 #CJK UNIFIED IDEOGRAPH
    {0xBF53, 0x71D9}, //8609 #CJK UNIFIED IDEOGRAPH
    {0xBF54, 0x71DC}, //8610 #CJK UNIFIED IDEOGRAPH
    {0xBF55, 0x71C3}, //8611 #CJK UNIFIED IDEOGRAPH
    {0xBF56, 0x71C4}, //8612 #CJK UNIFIED IDEOGRAPH
    {0xBF57, 0x7368}, //8613 #CJK UNIFIED IDEOGRAPH
    {0xBF58, 0x749C}, //8614 #CJK UNIFIED IDEOGRAPH
    {0xBF59, 0x74A3}, //8615 #CJK UNIFIED IDEOGRAPH
    {0xBF5A, 0x7498}, //8616 #CJK UNIFIED IDEOGRAPH
    {0xBF5B, 0x749F}, //8617 #CJK UNIFIED IDEOGRAPH
    {0xBF5C, 0x749E}, //8618 #CJK UNIFIED IDEOGRAPH
    {0xBF5D, 0x74E2}, //8619 #CJK UNIFIED IDEOGRAPH
    {0xBF5E, 0x750C}, //8620 #CJK UNIFIED IDEOGRAPH
    {0xBF5F, 0x750D}, //8621 #CJK UNIFIED IDEOGRAPH
    {0xBF60, 0x7634}, //8622 #CJK UNIFIED IDEOGRAPH
    {0xBF61, 0x7638}, //8623 #CJK UNIFIED IDEOGRAPH
    {0xBF62, 0x763A}, //8624 #CJK UNIFIED IDEOGRAPH
    {0xBF63, 0x76E7}, //8625 #CJK UNIFIED IDEOGRAPH
    {0xBF64, 0x76E5}, //8626 #CJK UNIFIED IDEOGRAPH
    {0xBF65, 0x77A0}, //8627 #CJK UNIFIED IDEOGRAPH
    {0xBF66, 0x779E}, //8628 #CJK UNIFIED IDEOGRAPH
    {0xBF67, 0x779F}, //8629 #CJK UNIFIED IDEOGRAPH
    {0xBF68, 0x77A5}, //8630 #CJK UNIFIED IDEOGRAPH
    {0xBF69, 0x78E8}, //8631 #CJK UNIFIED IDEOGRAPH
    {0xBF6A, 0x78DA}, //8632 #CJK UNIFIED IDEOGRAPH
    {0xBF6B, 0x78EC}, //8633 #CJK UNIFIED IDEOGRAPH
    {0xBF6C, 0x78E7}, //8634 #CJK UNIFIED IDEOGRAPH
    {0xBF6D, 0x79A6}, //8635 #CJK UNIFIED IDEOGRAPH
    {0xBF6E, 0x7A4D}, //8636 #CJK UNIFIED IDEOGRAPH
    {0xBF6F, 0x7A4E}, //8637 #CJK UNIFIED IDEOGRAPH
    {0xBF70, 0x7A46}, //8638 #CJK UNIFIED IDEOGRAPH
    {0xBF71, 0x7A4C}, //8639 #CJK UNIFIED IDEOGRAPH
    {0xBF72, 0x7A4B}, //8640 #CJK UNIFIED IDEOGRAPH
    {0xBF73, 0x7ABA}, //8641 #CJK UNIFIED IDEOGRAPH
    {0xBF74, 0x7BD9}, //8642 #CJK UNIFIED IDEOGRAPH
    {0xBF75, 0x7C11}, //8643 #CJK UNIFIED IDEOGRAPH
    {0xBF76, 0x7BC9}, //8644 #CJK UNIFIED IDEOGRAPH
    {0xBF77, 0x7BE4}, //8645 #CJK UNIFIED IDEOGRAPH
    {0xBF78, 0x7BDB}, //8646 #CJK UNIFIED IDEOGRAPH
    {0xBF79, 0x7BE1}, //8647 #CJK UNIFIED IDEOGRAPH
    {0xBF7A, 0x7BE9}, //8648 #CJK UNIFIED IDEOGRAPH
    {0xBF7B, 0x7BE6}, //8649 #CJK UNIFIED IDEOGRAPH
    {0xBF7C, 0x7CD5}, //8650 #CJK UNIFIED IDEOGRAPH
    {0xBF7D, 0x7CD6}, //8651 #CJK UNIFIED IDEOGRAPH
    {0xBF7E, 0x7E0A}, //8652 #CJK UNIFIED IDEOGRAPH
    {0xBFA1, 0x7E11}, //8653 #CJK UNIFIED IDEOGRAPH
    {0xBFA2, 0x7E08}, //8654 #CJK UNIFIED IDEOGRAPH
    {0xBFA3, 0x7E1B}, //8655 #CJK UNIFIED IDEOGRAPH
    {0xBFA4, 0x7E23}, //8656 #CJK UNIFIED IDEOGRAPH
    {0xBFA5, 0x7E1E}, //8657 #CJK UNIFIED IDEOGRAPH
    {0xBFA6, 0x7E1D}, //8658 #CJK UNIFIED IDEOGRAPH
    {0xBFA7, 0x7E09}, //8659 #CJK UNIFIED IDEOGRAPH
    {0xBFA8, 0x7E10}, //8660 #CJK UNIFIED IDEOGRAPH
    {0xBFA9, 0x7F79}, //8661 #CJK UNIFIED IDEOGRAPH
    {0xBFAA, 0x7FB2}, //8662 #CJK UNIFIED IDEOGRAPH
    {0xBFAB, 0x7FF0}, //8663 #CJK UNIFIED IDEOGRAPH
    {0xBFAC, 0x7FF1}, //8664 #CJK UNIFIED IDEOGRAPH
    {0xBFAD, 0x7FEE}, //8665 #CJK UNIFIED IDEOGRAPH
    {0xBFAE, 0x8028}, //8666 #CJK UNIFIED IDEOGRAPH
    {0xBFAF, 0x81B3}, //8667 #CJK UNIFIED IDEOGRAPH
    {0xBFB0, 0x81A9}, //8668 #CJK UNIFIED IDEOGRAPH
    {0xBFB1, 0x81A8}, //8669 #CJK UNIFIED IDEOGRAPH
    {0xBFB2, 0x81FB}, //8670 #CJK UNIFIED IDEOGRAPH
    {0xBFB3, 0x8208}, //8671 #CJK UNIFIED IDEOGRAPH
    {0xBFB4, 0x8258}, //8672 #CJK UNIFIED IDEOGRAPH
    {0xBFB5, 0x8259}, //8673 #CJK UNIFIED IDEOGRAPH
    {0xBFB6, 0x854A}, //8674 #CJK UNIFIED IDEOGRAPH
    {0xBFB7, 0x8559}, //8675 #CJK UNIFIED IDEOGRAPH
    {0xBFB8, 0x8548}, //8676 #CJK UNIFIED IDEOGRAPH
    {0xBFB9, 0x8568}, //8677 #CJK UNIFIED IDEOGRAPH
    {0xBFBA, 0x8569}, //8678 #CJK UNIFIED IDEOGRAPH
    {0xBFBB, 0x8543}, //8679 #CJK UNIFIED IDEOGRAPH
    {0xBFBC, 0x8549}, //8680 #CJK UNIFIED IDEOGRAPH
    {0xBFBD, 0x856D}, //8681 #CJK UNIFIED IDEOGRAPH
    {0xBFBE, 0x856A}, //8682 #CJK UNIFIED IDEOGRAPH
    {0xBFBF, 0x855E}, //8683 #CJK UNIFIED IDEOGRAPH
    {0xBFC0, 0x8783}, //8684 #CJK UNIFIED IDEOGRAPH
    {0xBFC1, 0x879F}, //8685 #CJK UNIFIED IDEOGRAPH
    {0xBFC2, 0x879E}, //8686 #CJK UNIFIED IDEOGRAPH
    {0xBFC3, 0x87A2}, //8687 #CJK UNIFIED IDEOGRAPH
    {0xBFC4, 0x878D}, //8688 #CJK UNIFIED IDEOGRAPH
    {0xBFC5, 0x8861}, //8689 #CJK UNIFIED IDEOGRAPH
    {0xBFC6, 0x892A}, //8690 #CJK UNIFIED IDEOGRAPH
    {0xBFC7, 0x8932}, //8691 #CJK UNIFIED IDEOGRAPH
    {0xBFC8, 0x8925}, //8692 #CJK UNIFIED IDEOGRAPH
    {0xBFC9, 0x892B}, //8693 #CJK UNIFIED IDEOGRAPH
    {0xBFCA, 0x8921}, //8694 #CJK UNIFIED IDEOGRAPH
    {0xBFCB, 0x89AA}, //8695 #CJK UNIFIED IDEOGRAPH
    {0xBFCC, 0x89A6}, //8696 #CJK UNIFIED IDEOGRAPH
    {0xBFCD, 0x8AE6}, //8697 #CJK UNIFIED IDEOGRAPH
    {0xBFCE, 0x8AFA}, //8698 #CJK UNIFIED IDEOGRAPH
    {0xBFCF, 0x8AEB}, //8699 #CJK UNIFIED IDEOGRAPH
    {0xBFD0, 0x8AF1}, //8700 #CJK UNIFIED IDEOGRAPH
    {0xBFD1, 0x8B00}, //8701 #CJK UNIFIED IDEOGRAPH
    {0xBFD2, 0x8ADC}, //8702 #CJK UNIFIED IDEOGRAPH
    {0xBFD3, 0x8AE7}, //8703 #CJK UNIFIED IDEOGRAPH
    {0xBFD4, 0x8AEE}, //8704 #CJK UNIFIED IDEOGRAPH
    {0xBFD5, 0x8AFE}, //8705 #CJK UNIFIED IDEOGRAPH
    {0xBFD6, 0x8B01}, //8706 #CJK UNIFIED IDEOGRAPH
    {0xBFD7, 0x8B02}, //8707 #CJK UNIFIED IDEOGRAPH
    {0xBFD8, 0x8AF7}, //8708 #CJK UNIFIED IDEOGRAPH
    {0xBFD9, 0x8AED}, //8709 #CJK UNIFIED IDEOGRAPH
    {0xBFDA, 0x8AF3}, //8710 #CJK UNIFIED IDEOGRAPH
    {0xBFDB, 0x8AF6}, //8711 #CJK UNIFIED IDEOGRAPH
    {0xBFDC, 0x8AFC}, //8712 #CJK UNIFIED IDEOGRAPH
    {0xBFDD, 0x8C6B}, //8713 #CJK UNIFIED IDEOGRAPH
    {0xBFDE, 0x8C6D}, //8714 #CJK UNIFIED IDEOGRAPH
    {0xBFDF, 0x8C93}, //8715 #CJK UNIFIED IDEOGRAPH
    {0xBFE0, 0x8CF4}, //8716 #CJK UNIFIED IDEOGRAPH
    {0xBFE1, 0x8E44}, //8717 #CJK UNIFIED IDEOGRAPH
    {0xBFE2, 0x8E31}, //8718 #CJK UNIFIED IDEOGRAPH
    {0xBFE3, 0x8E34}, //8719 #CJK UNIFIED IDEOGRAPH
    {0xBFE4, 0x8E42}, //8720 #CJK UNIFIED IDEOGRAPH
    {0xBFE5, 0x8E39}, //8721 #CJK UNIFIED IDEOGRAPH
    {0xBFE6, 0x8E35}, //8722 #CJK UNIFIED IDEOGRAPH
    {0xBFE7, 0x8F3B}, //8723 #CJK UNIFIED IDEOGRAPH
    {0xBFE8, 0x8F2F}, //8724 #CJK UNIFIED IDEOGRAPH
    {0xBFE9, 0x8F38}, //8725 #CJK UNIFIED IDEOGRAPH
    {0xBFEA, 0x8F33}, //8726 #CJK UNIFIED IDEOGRAPH
    {0xBFEB, 0x8FA8}, //8727 #CJK UNIFIED IDEOGRAPH
    {0xBFEC, 0x8FA6}, //8728 #CJK UNIFIED IDEOGRAPH
    {0xBFED, 0x9075}, //8729 #CJK UNIFIED IDEOGRAPH
    {0xBFEE, 0x9074}, //8730 #CJK UNIFIED IDEOGRAPH
    {0xBFEF, 0x9078}, //8731 #CJK UNIFIED IDEOGRAPH
    {0xBFF0, 0x9072}, //8732 #CJK UNIFIED IDEOGRAPH
    {0xBFF1, 0x907C}, //8733 #CJK UNIFIED IDEOGRAPH
    {0xBFF2, 0x907A}, //8734 #CJK UNIFIED IDEOGRAPH
    {0xBFF3, 0x9134}, //8735 #CJK UNIFIED IDEOGRAPH
    {0xBFF4, 0x9192}, //8736 #CJK UNIFIED IDEOGRAPH
    {0xBFF5, 0x9320}, //8737 #CJK UNIFIED IDEOGRAPH
    {0xBFF6, 0x9336}, //8738 #CJK UNIFIED IDEOGRAPH
    {0xBFF7, 0x92F8}, //8739 #CJK UNIFIED IDEOGRAPH
    {0xBFF8, 0x9333}, //8740 #CJK UNIFIED IDEOGRAPH
    {0xBFF9, 0x932F}, //8741 #CJK UNIFIED IDEOGRAPH
    {0xBFFA, 0x9322}, //8742 #CJK UNIFIED IDEOGRAPH
    {0xBFFB, 0x92FC}, //8743 #CJK UNIFIED IDEOGRAPH
    {0xBFFC, 0x932B}, //8744 #CJK UNIFIED IDEOGRAPH
    {0xBFFD, 0x9304}, //8745 #CJK UNIFIED IDEOGRAPH
    {0xBFFE, 0x931A}, //8746 #CJK UNIFIED IDEOGRAPH
    {0xC040, 0x9310}, //8747 #CJK UNIFIED IDEOGRAPH
    {0xC041, 0x9326}, //8748 #CJK UNIFIED IDEOGRAPH
    {0xC042, 0x9321}, //8749 #CJK UNIFIED IDEOGRAPH
    {0xC043, 0x9315}, //8750 #CJK UNIFIED IDEOGRAPH
    {0xC044, 0x932E}, //8751 #CJK UNIFIED IDEOGRAPH
    {0xC045, 0x9319}, //8752 #CJK UNIFIED IDEOGRAPH
    {0xC046, 0x95BB}, //8753 #CJK UNIFIED IDEOGRAPH
    {0xC047, 0x96A7}, //8754 #CJK UNIFIED IDEOGRAPH
    {0xC048, 0x96A8}, //8755 #CJK UNIFIED IDEOGRAPH
    {0xC049, 0x96AA}, //8756 #CJK UNIFIED IDEOGRAPH
    {0xC04A, 0x96D5}, //8757 #CJK UNIFIED IDEOGRAPH
    {0xC04B, 0x970E}, //8758 #CJK UNIFIED IDEOGRAPH
    {0xC04C, 0x9711}, //8759 #CJK UNIFIED IDEOGRAPH
    {0xC04D, 0x9716}, //8760 #CJK UNIFIED IDEOGRAPH
    {0xC04E, 0x970D}, //8761 #CJK UNIFIED IDEOGRAPH
    {0xC04F, 0x9713}, //8762 #CJK UNIFIED IDEOGRAPH
    {0xC050, 0x970F}, //8763 #CJK UNIFIED IDEOGRAPH
    {0xC051, 0x975B}, //8764 #CJK UNIFIED IDEOGRAPH
    {0xC052, 0x975C}, //8765 #CJK UNIFIED IDEOGRAPH
    {0xC053, 0x9766}, //8766 #CJK UNIFIED IDEOGRAPH
    {0xC054, 0x9798}, //8767 #CJK UNIFIED IDEOGRAPH
    {0xC055, 0x9830}, //8768 #CJK UNIFIED IDEOGRAPH
    {0xC056, 0x9838}, //8769 #CJK UNIFIED IDEOGRAPH
    {0xC057, 0x983B}, //8770 #CJK UNIFIED IDEOGRAPH
    {0xC058, 0x9837}, //8771 #CJK UNIFIED IDEOGRAPH
    {0xC059, 0x982D}, //8772 #CJK UNIFIED IDEOGRAPH
    {0xC05A, 0x9839}, //8773 #CJK UNIFIED IDEOGRAPH
    {0xC05B, 0x9824}, //8774 #CJK UNIFIED IDEOGRAPH
    {0xC05C, 0x9910}, //8775 #CJK UNIFIED IDEOGRAPH
    {0xC05D, 0x9928}, //8776 #CJK UNIFIED IDEOGRAPH
    {0xC05E, 0x991E}, //8777 #CJK UNIFIED IDEOGRAPH
    {0xC05F, 0x991B}, //8778 #CJK UNIFIED IDEOGRAPH
    {0xC060, 0x9921}, //8779 #CJK UNIFIED IDEOGRAPH
    {0xC061, 0x991A}, //8780 #CJK UNIFIED IDEOGRAPH
    {0xC062, 0x99ED}, //8781 #CJK UNIFIED IDEOGRAPH
    {0xC063, 0x99E2}, //8782 #CJK UNIFIED IDEOGRAPH
    {0xC064, 0x99F1}, //8783 #CJK UNIFIED IDEOGRAPH
    {0xC065, 0x9AB8}, //8784 #CJK UNIFIED IDEOGRAPH
    {0xC066, 0x9ABC}, //8785 #CJK UNIFIED IDEOGRAPH
    {0xC067, 0x9AFB}, //8786 #CJK UNIFIED IDEOGRAPH
    {0xC068, 0x9AED}, //8787 #CJK UNIFIED IDEOGRAPH
    {0xC069, 0x9B28}, //8788 #CJK UNIFIED IDEOGRAPH
    {0xC06A, 0x9B91}, //8789 #CJK UNIFIED IDEOGRAPH
    {0xC06B, 0x9D15}, //8790 #CJK UNIFIED IDEOGRAPH
    {0xC06C, 0x9D23}, //8791 #CJK UNIFIED IDEOGRAPH
    {0xC06D, 0x9D26}, //8792 #CJK UNIFIED IDEOGRAPH
    {0xC06E, 0x9D28}, //8793 #CJK UNIFIED IDEOGRAPH
    {0xC06F, 0x9D12}, //8794 #CJK UNIFIED IDEOGRAPH
    {0xC070, 0x9D1B}, //8795 #CJK UNIFIED IDEOGRAPH
    {0xC071, 0x9ED8}, //8796 #CJK UNIFIED IDEOGRAPH
    {0xC072, 0x9ED4}, //8797 #CJK UNIFIED IDEOGRAPH
    {0xC073, 0x9F8D}, //8798 #CJK UNIFIED IDEOGRAPH
    {0xC074, 0x9F9C}, //8799 #CJK UNIFIED IDEOGRAPH
    {0xC075, 0x512A}, //8800 #CJK UNIFIED IDEOGRAPH
    {0xC076, 0x511F}, //8801 #CJK UNIFIED IDEOGRAPH
    {0xC077, 0x5121}, //8802 #CJK UNIFIED IDEOGRAPH
    {0xC078, 0x5132}, //8803 #CJK UNIFIED IDEOGRAPH
    {0xC079, 0x52F5}, //8804 #CJK UNIFIED IDEOGRAPH
    {0xC07A, 0x568E}, //8805 #CJK UNIFIED IDEOGRAPH
    {0xC07B, 0x5680}, //8806 #CJK UNIFIED IDEOGRAPH
    {0xC07C, 0x5690}, //8807 #CJK UNIFIED IDEOGRAPH
    {0xC07D, 0x5685}, //8808 #CJK UNIFIED IDEOGRAPH
    {0xC07E, 0x5687}, //8809 #CJK UNIFIED IDEOGRAPH
    {0xC0A1, 0x568F}, //8810 #CJK UNIFIED IDEOGRAPH
    {0xC0A2, 0x58D5}, //8811 #CJK UNIFIED IDEOGRAPH
    {0xC0A3, 0x58D3}, //8812 #CJK UNIFIED IDEOGRAPH
    {0xC0A4, 0x58D1}, //8813 #CJK UNIFIED IDEOGRAPH
    {0xC0A5, 0x58CE}, //8814 #CJK UNIFIED IDEOGRAPH
    {0xC0A6, 0x5B30}, //8815 #CJK UNIFIED IDEOGRAPH
    {0xC0A7, 0x5B2A}, //8816 #CJK UNIFIED IDEOGRAPH
    {0xC0A8, 0x5B24}, //8817 #CJK UNIFIED IDEOGRAPH
    {0xC0A9, 0x5B7A}, //8818 #CJK UNIFIED IDEOGRAPH
    {0xC0AA, 0x5C37}, //8819 #CJK UNIFIED IDEOGRAPH
    {0xC0AB, 0x5C68}, //8820 #CJK UNIFIED IDEOGRAPH
    {0xC0AC, 0x5DBC}, //8821 #CJK UNIFIED IDEOGRAPH
    {0xC0AD, 0x5DBA}, //8822 #CJK UNIFIED IDEOGRAPH
    {0xC0AE, 0x5DBD}, //8823 #CJK UNIFIED IDEOGRAPH
    {0xC0AF, 0x5DB8}, //8824 #CJK UNIFIED IDEOGRAPH
    {0xC0B0, 0x5E6B}, //8825 #CJK UNIFIED IDEOGRAPH
    {0xC0B1, 0x5F4C}, //8826 #CJK UNIFIED IDEOGRAPH
    {0xC0B2, 0x5FBD}, //8827 #CJK UNIFIED IDEOGRAPH
    {0xC0B3, 0x61C9}, //8828 #CJK UNIFIED IDEOGRAPH
    {0xC0B4, 0x61C2}, //8829 #CJK UNIFIED IDEOGRAPH
    {0xC0B5, 0x61C7}, //8830 #CJK UNIFIED IDEOGRAPH
    {0xC0B6, 0x61E6}, //8831 #CJK UNIFIED IDEOGRAPH
    {0xC0B7, 0x61CB}, //8832 #CJK UNIFIED IDEOGRAPH
    {0xC0B8, 0x6232}, //8833 #CJK UNIFIED IDEOGRAPH
    {0xC0B9, 0x6234}, //8834 #CJK UNIFIED IDEOGRAPH
    {0xC0BA, 0x64CE}, //8835 #CJK UNIFIED IDEOGRAPH
    {0xC0BB, 0x64CA}, //8836 #CJK UNIFIED IDEOGRAPH
    {0xC0BC, 0x64D8}, //8837 #CJK UNIFIED IDEOGRAPH
    {0xC0BD, 0x64E0}, //8838 #CJK UNIFIED IDEOGRAPH
    {0xC0BE, 0x64F0}, //8839 #CJK UNIFIED IDEOGRAPH
    {0xC0BF, 0x64E6}, //8840 #CJK UNIFIED IDEOGRAPH
    {0xC0C0, 0x64EC}, //8841 #CJK UNIFIED IDEOGRAPH
    {0xC0C1, 0x64F1}, //8842 #CJK UNIFIED IDEOGRAPH
    {0xC0C2, 0x64E2}, //8843 #CJK UNIFIED IDEOGRAPH
    {0xC0C3, 0x64ED}, //8844 #CJK UNIFIED IDEOGRAPH
    {0xC0C4, 0x6582}, //8845 #CJK UNIFIED IDEOGRAPH
    {0xC0C5, 0x6583}, //8846 #CJK UNIFIED IDEOGRAPH
    {0xC0C6, 0x66D9}, //8847 #CJK UNIFIED IDEOGRAPH
    {0xC0C7, 0x66D6}, //8848 #CJK UNIFIED IDEOGRAPH
    {0xC0C8, 0x6A80}, //8849 #CJK UNIFIED IDEOGRAPH
    {0xC0C9, 0x6A94}, //8850 #CJK UNIFIED IDEOGRAPH
    {0xC0CA, 0x6A84}, //8851 #CJK UNIFIED IDEOGRAPH
    {0xC0CB, 0x6AA2}, //8852 #CJK UNIFIED IDEOGRAPH
    {0xC0CC, 0x6A9C}, //8853 #CJK UNIFIED IDEOGRAPH
    {0xC0CD, 0x6ADB}, //8854 #CJK UNIFIED IDEOGRAPH
    {0xC0CE, 0x6AA3}, //8855 #CJK UNIFIED IDEOGRAPH
    {0xC0CF, 0x6A7E}, //8856 #CJK UNIFIED IDEOGRAPH
    {0xC0D0, 0x6A97}, //8857 #CJK UNIFIED IDEOGRAPH
    {0xC0D1, 0x6A90}, //8858 #CJK UNIFIED IDEOGRAPH
    {0xC0D2, 0x6AA0}, //8859 #CJK UNIFIED IDEOGRAPH
    {0xC0D3, 0x6B5C}, //8860 #CJK UNIFIED IDEOGRAPH
    {0xC0D4, 0x6BAE}, //8861 #CJK UNIFIED IDEOGRAPH
    {0xC0D5, 0x6BDA}, //8862 #CJK UNIFIED IDEOGRAPH
    {0xC0D6, 0x6C08}, //8863 #CJK UNIFIED IDEOGRAPH
    {0xC0D7, 0x6FD8}, //8864 #CJK UNIFIED IDEOGRAPH
    {0xC0D8, 0x6FF1}, //8865 #CJK UNIFIED IDEOGRAPH
    {0xC0D9, 0x6FDF}, //8866 #CJK UNIFIED IDEOGRAPH
    {0xC0DA, 0x6FE0}, //8867 #CJK UNIFIED IDEOGRAPH
    {0xC0DB, 0x6FDB}, //8868 #CJK UNIFIED IDEOGRAPH
    {0xC0DC, 0x6FE4}, //8869 #CJK UNIFIED IDEOGRAPH
    {0xC0DD, 0x6FEB}, //8870 #CJK UNIFIED IDEOGRAPH
    {0xC0DE, 0x6FEF}, //8871 #CJK UNIFIED IDEOGRAPH
    {0xC0DF, 0x6F80}, //8872 #CJK UNIFIED IDEOGRAPH
    {0xC0E0, 0x6FEC}, //8873 #CJK UNIFIED IDEOGRAPH
    {0xC0E1, 0x6FE1}, //8874 #CJK UNIFIED IDEOGRAPH
    {0xC0E2, 0x6FE9}, //8875 #CJK UNIFIED IDEOGRAPH
    {0xC0E3, 0x6FD5}, //8876 #CJK UNIFIED IDEOGRAPH
    {0xC0E4, 0x6FEE}, //8877 #CJK UNIFIED IDEOGRAPH
    {0xC0E5, 0x6FF0}, //8878 #CJK UNIFIED IDEOGRAPH
    {0xC0E6, 0x71E7}, //8879 #CJK UNIFIED IDEOGRAPH
    {0xC0E7, 0x71DF}, //8880 #CJK UNIFIED IDEOGRAPH
    {0xC0E8, 0x71EE}, //8881 #CJK UNIFIED IDEOGRAPH
    {0xC0E9, 0x71E6}, //8882 #CJK UNIFIED IDEOGRAPH
    {0xC0EA, 0x71E5}, //8883 #CJK UNIFIED IDEOGRAPH
    {0xC0EB, 0x71ED}, //8884 #CJK UNIFIED IDEOGRAPH
    {0xC0EC, 0x71EC}, //8885 #CJK UNIFIED IDEOGRAPH
    {0xC0ED, 0x71F4}, //8886 #CJK UNIFIED IDEOGRAPH
    {0xC0EE, 0x71E0}, //8887 #CJK UNIFIED IDEOGRAPH
    {0xC0EF, 0x7235}, //8888 #CJK UNIFIED IDEOGRAPH
    {0xC0F0, 0x7246}, //8889 #CJK UNIFIED IDEOGRAPH
    {0xC0F1, 0x7370}, //8890 #CJK UNIFIED IDEOGRAPH
    {0xC0F2, 0x7372}, //8891 #CJK UNIFIED IDEOGRAPH
    {0xC0F3, 0x74A9}, //8892 #CJK UNIFIED IDEOGRAPH
    {0xC0F4, 0x74B0}, //8893 #CJK UNIFIED IDEOGRAPH
    {0xC0F5, 0x74A6}, //8894 #CJK UNIFIED IDEOGRAPH
    {0xC0F6, 0x74A8}, //8895 #CJK UNIFIED IDEOGRAPH
    {0xC0F7, 0x7646}, //8896 #CJK UNIFIED IDEOGRAPH
    {0xC0F8, 0x7642}, //8897 #CJK UNIFIED IDEOGRAPH
    {0xC0F9, 0x764C}, //8898 #CJK UNIFIED IDEOGRAPH
    {0xC0FA, 0x76EA}, //8899 #CJK UNIFIED IDEOGRAPH
    {0xC0FB, 0x77B3}, //8900 #CJK UNIFIED IDEOGRAPH
    {0xC0FC, 0x77AA}, //8901 #CJK UNIFIED IDEOGRAPH
    {0xC0FD, 0x77B0}, //8902 #CJK UNIFIED IDEOGRAPH
    {0xC0FE, 0x77AC}, //8903 #CJK UNIFIED IDEOGRAPH
    {0xC140, 0x77A7}, //8904 #CJK UNIFIED IDEOGRAPH
    {0xC141, 0x77AD}, //8905 #CJK UNIFIED IDEOGRAPH
    {0xC142, 0x77EF}, //8906 #CJK UNIFIED IDEOGRAPH
    {0xC143, 0x78F7}, //8907 #CJK UNIFIED IDEOGRAPH
    {0xC144, 0x78FA}, //8908 #CJK UNIFIED IDEOGRAPH
    {0xC145, 0x78F4}, //8909 #CJK UNIFIED IDEOGRAPH
    {0xC146, 0x78EF}, //8910 #CJK UNIFIED IDEOGRAPH
    {0xC147, 0x7901}, //8911 #CJK UNIFIED IDEOGRAPH
    {0xC148, 0x79A7}, //8912 #CJK UNIFIED IDEOGRAPH
    {0xC149, 0x79AA}, //8913 #CJK UNIFIED IDEOGRAPH
    {0xC14A, 0x7A57}, //8914 #CJK UNIFIED IDEOGRAPH
    {0xC14B, 0x7ABF}, //8915 #CJK UNIFIED IDEOGRAPH
    {0xC14C, 0x7C07}, //8916 #CJK UNIFIED IDEOGRAPH
    {0xC14D, 0x7C0D}, //8917 #CJK UNIFIED IDEOGRAPH
    {0xC14E, 0x7BFE}, //8918 #CJK UNIFIED IDEOGRAPH
    {0xC14F, 0x7BF7}, //8919 #CJK UNIFIED IDEOGRAPH
    {0xC150, 0x7C0C}, //8920 #CJK UNIFIED IDEOGRAPH
    {0xC151, 0x7BE0}, //8921 #CJK UNIFIED IDEOGRAPH
    {0xC152, 0x7CE0}, //8922 #CJK UNIFIED IDEOGRAPH
    {0xC153, 0x7CDC}, //8923 #CJK UNIFIED IDEOGRAPH
    {0xC154, 0x7CDE}, //8924 #CJK UNIFIED IDEOGRAPH
    {0xC155, 0x7CE2}, //8925 #CJK UNIFIED IDEOGRAPH
    {0xC156, 0x7CDF}, //8926 #CJK UNIFIED IDEOGRAPH
    {0xC157, 0x7CD9}, //8927 #CJK UNIFIED IDEOGRAPH
    {0xC158, 0x7CDD}, //8928 #CJK UNIFIED IDEOGRAPH
    {0xC159, 0x7E2E}, //8929 #CJK UNIFIED IDEOGRAPH
    {0xC15A, 0x7E3E}, //8930 #CJK UNIFIED IDEOGRAPH
    {0xC15B, 0x7E46}, //8931 #CJK UNIFIED IDEOGRAPH
    {0xC15C, 0x7E37}, //8932 #CJK UNIFIED IDEOGRAPH
    {0xC15D, 0x7E32}, //8933 #CJK UNIFIED IDEOGRAPH
    {0xC15E, 0x7E43}, //8934 #CJK UNIFIED IDEOGRAPH
    {0xC15F, 0x7E2B}, //8935 #CJK UNIFIED IDEOGRAPH
    {0xC160, 0x7E3D}, //8936 #CJK UNIFIED IDEOGRAPH
    {0xC161, 0x7E31}, //8937 #CJK UNIFIED IDEOGRAPH
    {0xC162, 0x7E45}, //8938 #CJK UNIFIED IDEOGRAPH
    {0xC163, 0x7E41}, //8939 #CJK UNIFIED IDEOGRAPH
    {0xC164, 0x7E34}, //8940 #CJK UNIFIED IDEOGRAPH
    {0xC165, 0x7E39}, //8941 #CJK UNIFIED IDEOGRAPH
    {0xC166, 0x7E48}, //8942 #CJK UNIFIED IDEOGRAPH
    {0xC167, 0x7E35}, //8943 #CJK UNIFIED IDEOGRAPH
    {0xC168, 0x7E3F}, //8944 #CJK UNIFIED IDEOGRAPH
    {0xC169, 0x7E2F}, //8945 #CJK UNIFIED IDEOGRAPH
    {0xC16A, 0x7F44}, //8946 #CJK UNIFIED IDEOGRAPH
    {0xC16B, 0x7FF3}, //8947 #CJK UNIFIED IDEOGRAPH
    {0xC16C, 0x7FFC}, //8948 #CJK UNIFIED IDEOGRAPH
    {0xC16D, 0x8071}, //8949 #CJK UNIFIED IDEOGRAPH
    {0xC16E, 0x8072}, //8950 #CJK UNIFIED IDEOGRAPH
    {0xC16F, 0x8070}, //8951 #CJK UNIFIED IDEOGRAPH
    {0xC170, 0x806F}, //8952 #CJK UNIFIED IDEOGRAPH
    {0xC171, 0x8073}, //8953 #CJK UNIFIED IDEOGRAPH
    {0xC172, 0x81C6}, //8954 #CJK UNIFIED IDEOGRAPH
    {0xC173, 0x81C3}, //8955 #CJK UNIFIED IDEOGRAPH
    {0xC174, 0x81BA}, //8956 #CJK UNIFIED IDEOGRAPH
    {0xC175, 0x81C2}, //8957 #CJK UNIFIED IDEOGRAPH
    {0xC176, 0x81C0}, //8958 #CJK UNIFIED IDEOGRAPH
    {0xC177, 0x81BF}, //8959 #CJK UNIFIED IDEOGRAPH
    {0xC178, 0x81BD}, //8960 #CJK UNIFIED IDEOGRAPH
    {0xC179, 0x81C9}, //8961 #CJK UNIFIED IDEOGRAPH
    {0xC17A, 0x81BE}, //8962 #CJK UNIFIED IDEOGRAPH
    {0xC17B, 0x81E8}, //8963 #CJK UNIFIED IDEOGRAPH
    {0xC17C, 0x8209}, //8964 #CJK UNIFIED IDEOGRAPH
    {0xC17D, 0x8271}, //8965 #CJK UNIFIED IDEOGRAPH
    {0xC17E, 0x85AA}, //8966 #CJK UNIFIED IDEOGRAPH
    {0xC1A1, 0x8584}, //8967 #CJK UNIFIED IDEOGRAPH
    {0xC1A2, 0x857E}, //8968 #CJK UNIFIED IDEOGRAPH
    {0xC1A3, 0x859C}, //8969 #CJK UNIFIED IDEOGRAPH
    {0xC1A4, 0x8591}, //8970 #CJK UNIFIED IDEOGRAPH
    {0xC1A5, 0x8594}, //8971 #CJK UNIFIED IDEOGRAPH
    {0xC1A6, 0x85AF}, //8972 #CJK UNIFIED IDEOGRAPH
    {0xC1A7, 0x859B}, //8973 #CJK UNIFIED IDEOGRAPH
    {0xC1A8, 0x8587}, //8974 #CJK UNIFIED IDEOGRAPH
    {0xC1A9, 0x85A8}, //8975 #CJK UNIFIED IDEOGRAPH
    {0xC1AA, 0x858A}, //8976 #CJK UNIFIED IDEOGRAPH
    {0xC1AB, 0x8667}, //8977 #CJK UNIFIED IDEOGRAPH
    {0xC1AC, 0x87C0}, //8978 #CJK UNIFIED IDEOGRAPH
    {0xC1AD, 0x87D1}, //8979 #CJK UNIFIED IDEOGRAPH
    {0xC1AE, 0x87B3}, //8980 #CJK UNIFIED IDEOGRAPH
    {0xC1AF, 0x87D2}, //8981 #CJK UNIFIED IDEOGRAPH
    {0xC1B0, 0x87C6}, //8982 #CJK UNIFIED IDEOGRAPH
    {0xC1B1, 0x87AB}, //8983 #CJK UNIFIED IDEOGRAPH
    {0xC1B2, 0x87BB}, //8984 #CJK UNIFIED IDEOGRAPH
    {0xC1B3, 0x87BA}, //8985 #CJK UNIFIED IDEOGRAPH
    {0xC1B4, 0x87C8}, //8986 #CJK UNIFIED IDEOGRAPH
    {0xC1B5, 0x87CB}, //8987 #CJK UNIFIED IDEOGRAPH
    {0xC1B6, 0x893B}, //8988 #CJK UNIFIED IDEOGRAPH
    {0xC1B7, 0x8936}, //8989 #CJK UNIFIED IDEOGRAPH
    {0xC1B8, 0x8944}, //8990 #CJK UNIFIED IDEOGRAPH
    {0xC1B9, 0x8938}, //8991 #CJK UNIFIED IDEOGRAPH
    {0xC1BA, 0x893D}, //8992 #CJK UNIFIED IDEOGRAPH
    {0xC1BB, 0x89AC}, //8993 #CJK UNIFIED IDEOGRAPH
    {0xC1BC, 0x8B0E}, //8994 #CJK UNIFIED IDEOGRAPH
    {0xC1BD, 0x8B17}, //8995 #CJK UNIFIED IDEOGRAPH
    {0xC1BE, 0x8B19}, //8996 #CJK UNIFIED IDEOGRAPH
    {0xC1BF, 0x8B1B}, //8997 #CJK UNIFIED IDEOGRAPH
    {0xC1C0, 0x8B0A}, //8998 #CJK UNIFIED IDEOGRAPH
    {0xC1C1, 0x8B20}, //8999 #CJK UNIFIED IDEOGRAPH
    {0xC1C2, 0x8B1D}, //9000 #CJK UNIFIED IDEOGRAPH
    {0xC1C3, 0x8B04}, //9001 #CJK UNIFIED IDEOGRAPH
    {0xC1C4, 0x8B10}, //9002 #CJK UNIFIED IDEOGRAPH
    {0xC1C5, 0x8C41}, //9003 #CJK UNIFIED IDEOGRAPH
    {0xC1C6, 0x8C3F}, //9004 #CJK UNIFIED IDEOGRAPH
    {0xC1C7, 0x8C73}, //9005 #CJK UNIFIED IDEOGRAPH
    {0xC1C8, 0x8CFA}, //9006 #CJK UNIFIED IDEOGRAPH
    {0xC1C9, 0x8CFD}, //9007 #CJK UNIFIED IDEOGRAPH
    {0xC1CA, 0x8CFC}, //9008 #CJK UNIFIED IDEOGRAPH
    {0xC1CB, 0x8CF8}, //9009 #CJK UNIFIED IDEOGRAPH
    {0xC1CC, 0x8CFB}, //9010 #CJK UNIFIED IDEOGRAPH
    {0xC1CD, 0x8DA8}, //9011 #CJK UNIFIED IDEOGRAPH
    {0xC1CE, 0x8E49}, //9012 #CJK UNIFIED IDEOGRAPH
    {0xC1CF, 0x8E4B}, //9013 #CJK UNIFIED IDEOGRAPH
    {0xC1D0, 0x8E48}, //9014 #CJK UNIFIED IDEOGRAPH
    {0xC1D1, 0x8E4A}, //9015 #CJK UNIFIED IDEOGRAPH
    {0xC1D2, 0x8F44}, //9016 #CJK UNIFIED IDEOGRAPH
    {0xC1D3, 0x8F3E}, //9017 #CJK UNIFIED IDEOGRAPH
    {0xC1D4, 0x8F42}, //9018 #CJK UNIFIED IDEOGRAPH
    {0xC1D5, 0x8F45}, //9019 #CJK UNIFIED IDEOGRAPH
    {0xC1D6, 0x8F3F}, //9020 #CJK UNIFIED IDEOGRAPH
    {0xC1D7, 0x907F}, //9021 #CJK UNIFIED IDEOGRAPH
    {0xC1D8, 0x907D}, //9022 #CJK UNIFIED IDEOGRAPH
    {0xC1D9, 0x9084}, //9023 #CJK UNIFIED IDEOGRAPH
    {0xC1DA, 0x9081}, //9024 #CJK UNIFIED IDEOGRAPH
    {0xC1DB, 0x9082}, //9025 #CJK UNIFIED IDEOGRAPH
    {0xC1DC, 0x9080}, //9026 #CJK UNIFIED IDEOGRAPH
    {0xC1DD, 0x9139}, //9027 #CJK UNIFIED IDEOGRAPH
    {0xC1DE, 0x91A3}, //9028 #CJK UNIFIED IDEOGRAPH
    {0xC1DF, 0x919E}, //9029 #CJK UNIFIED IDEOGRAPH
    {0xC1E0, 0x919C}, //9030 #CJK UNIFIED IDEOGRAPH
    {0xC1E1, 0x934D}, //9031 #CJK UNIFIED IDEOGRAPH
    {0xC1E2, 0x9382}, //9032 #CJK UNIFIED IDEOGRAPH
    {0xC1E3, 0x9328}, //9033 #CJK UNIFIED IDEOGRAPH
    {0xC1E4, 0x9375}, //9034 #CJK UNIFIED IDEOGRAPH
    {0xC1E5, 0x934A}, //9035 #CJK UNIFIED IDEOGRAPH
    {0xC1E6, 0x9365}, //9036 #CJK UNIFIED IDEOGRAPH
    {0xC1E7, 0x934B}, //9037 #CJK UNIFIED IDEOGRAPH
    {0xC1E8, 0x9318}, //9038 #CJK UNIFIED IDEOGRAPH
    {0xC1E9, 0x937E}, //9039 #CJK UNIFIED IDEOGRAPH
    {0xC1EA, 0x936C}, //9040 #CJK UNIFIED IDEOGRAPH
    {0xC1EB, 0x935B}, //9041 #CJK UNIFIED IDEOGRAPH
    {0xC1EC, 0x9370}, //9042 #CJK UNIFIED IDEOGRAPH
    {0xC1ED, 0x935A}, //9043 #CJK UNIFIED IDEOGRAPH
    {0xC1EE, 0x9354}, //9044 #CJK UNIFIED IDEOGRAPH
    {0xC1EF, 0x95CA}, //9045 #CJK UNIFIED IDEOGRAPH
    {0xC1F0, 0x95CB}, //9046 #CJK UNIFIED IDEOGRAPH
    {0xC1F1, 0x95CC}, //9047 #CJK UNIFIED IDEOGRAPH
    {0xC1F2, 0x95C8}, //9048 #CJK UNIFIED IDEOGRAPH
    {0xC1F3, 0x95C6}, //9049 #CJK UNIFIED IDEOGRAPH
    {0xC1F4, 0x96B1}, //9050 #CJK UNIFIED IDEOGRAPH
    {0xC1F5, 0x96B8}, //9051 #CJK UNIFIED IDEOGRAPH
    {0xC1F6, 0x96D6}, //9052 #CJK UNIFIED IDEOGRAPH
    {0xC1F7, 0x971C}, //9053 #CJK UNIFIED IDEOGRAPH
    {0xC1F8, 0x971E}, //9054 #CJK UNIFIED IDEOGRAPH
    {0xC1F9, 0x97A0}, //9055 #CJK UNIFIED IDEOGRAPH
    {0xC1FA, 0x97D3}, //9056 #CJK UNIFIED IDEOGRAPH
    {0xC1FB, 0x9846}, //9057 #CJK UNIFIED IDEOGRAPH
    {0xC1FC, 0x98B6}, //9058 #CJK UNIFIED IDEOGRAPH
    {0xC1FD, 0x9935}, //9059 #CJK UNIFIED IDEOGRAPH
    {0xC1FE, 0x9A01}, //9060 #CJK UNIFIED IDEOGRAPH
    {0xC240, 0x99FF}, //9061 #CJK UNIFIED IDEOGRAPH
    {0xC241, 0x9BAE}, //9062 #CJK UNIFIED IDEOGRAPH
    {0xC242, 0x9BAB}, //9063 #CJK UNIFIED IDEOGRAPH
    {0xC243, 0x9BAA}, //9064 #CJK UNIFIED IDEOGRAPH
    {0xC244, 0x9BAD}, //9065 #CJK UNIFIED IDEOGRAPH
    {0xC245, 0x9D3B}, //9066 #CJK UNIFIED IDEOGRAPH
    {0xC246, 0x9D3F}, //9067 #CJK UNIFIED IDEOGRAPH
    {0xC247, 0x9E8B}, //9068 #CJK UNIFIED IDEOGRAPH
    {0xC248, 0x9ECF}, //9069 #CJK UNIFIED IDEOGRAPH
    {0xC249, 0x9EDE}, //9070 #CJK UNIFIED IDEOGRAPH
    {0xC24A, 0x9EDC}, //9071 #CJK UNIFIED IDEOGRAPH
    {0xC24B, 0x9EDD}, //9072 #CJK UNIFIED IDEOGRAPH
    {0xC24C, 0x9EDB}, //9073 #CJK UNIFIED IDEOGRAPH
    {0xC24D, 0x9F3E}, //9074 #CJK UNIFIED IDEOGRAPH
    {0xC24E, 0x9F4B}, //9075 #CJK UNIFIED IDEOGRAPH
    {0xC24F, 0x53E2}, //9076 #CJK UNIFIED IDEOGRAPH
    {0xC250, 0x5695}, //9077 #CJK UNIFIED IDEOGRAPH
    {0xC251, 0x56AE}, //9078 #CJK UNIFIED IDEOGRAPH
    {0xC252, 0x58D9}, //9079 #CJK UNIFIED IDEOGRAPH
    {0xC253, 0x58D8}, //9080 #CJK UNIFIED IDEOGRAPH
    {0xC254, 0x5B38}, //9081 #CJK UNIFIED IDEOGRAPH
    {0xC255, 0x5F5D}, //9082 #CJK UNIFIED IDEOGRAPH
    {0xC256, 0x61E3}, //9083 #CJK UNIFIED IDEOGRAPH
    {0xC257, 0x6233}, //9084 #CJK UNIFIED IDEOGRAPH
    {0xC258, 0x64F4}, //9085 #CJK UNIFIED IDEOGRAPH
    {0xC259, 0x64F2}, //9086 #CJK UNIFIED IDEOGRAPH
    {0xC25A, 0x64FE}, //9087 #CJK UNIFIED IDEOGRAPH
    {0xC25B, 0x6506}, //9088 #CJK UNIFIED IDEOGRAPH
    {0xC25C, 0x64FA}, //9089 #CJK UNIFIED IDEOGRAPH
    {0xC25D, 0x64FB}, //9090 #CJK UNIFIED IDEOGRAPH
    {0xC25E, 0x64F7}, //9091 #CJK UNIFIED IDEOGRAPH
    {0xC25F, 0x65B7}, //9092 #CJK UNIFIED IDEOGRAPH
    {0xC260, 0x66DC}, //9093 #CJK UNIFIED IDEOGRAPH
    {0xC261, 0x6726}, //9094 #CJK UNIFIED IDEOGRAPH
    {0xC262, 0x6AB3}, //9095 #CJK UNIFIED IDEOGRAPH
    {0xC263, 0x6AAC}, //9096 #CJK UNIFIED IDEOGRAPH
    {0xC264, 0x6AC3}, //9097 #CJK UNIFIED IDEOGRAPH
    {0xC265, 0x6ABB}, //9098 #CJK UNIFIED IDEOGRAPH
    {0xC266, 0x6AB8}, //9099 #CJK UNIFIED IDEOGRAPH
    {0xC267, 0x6AC2}, //9100 #CJK UNIFIED IDEOGRAPH
    {0xC268, 0x6AAE}, //9101 #CJK UNIFIED IDEOGRAPH
    {0xC269, 0x6AAF}, //9102 #CJK UNIFIED IDEOGRAPH
    {0xC26A, 0x6B5F}, //9103 #CJK UNIFIED IDEOGRAPH
    {0xC26B, 0x6B78}, //9104 #CJK UNIFIED IDEOGRAPH
    {0xC26C, 0x6BAF}, //9105 #CJK UNIFIED IDEOGRAPH
    {0xC26D, 0x7009}, //9106 #CJK UNIFIED IDEOGRAPH
    {0xC26E, 0x700B}, //9107 #CJK UNIFIED IDEOGRAPH
    {0xC26F, 0x6FFE}, //9108 #CJK UNIFIED IDEOGRAPH
    {0xC270, 0x7006}, //9109 #CJK UNIFIED IDEOGRAPH
    {0xC271, 0x6FFA}, //9110 #CJK UNIFIED IDEOGRAPH
    {0xC272, 0x7011}, //9111 #CJK UNIFIED IDEOGRAPH
    {0xC273, 0x700F}, //9112 #CJK UNIFIED IDEOGRAPH
    {0xC274, 0x71FB}, //9113 #CJK UNIFIED IDEOGRAPH
    {0xC275, 0x71FC}, //9114 #CJK UNIFIED IDEOGRAPH
    {0xC276, 0x71FE}, //9115 #CJK UNIFIED IDEOGRAPH
    {0xC277, 0x71F8}, //9116 #CJK UNIFIED IDEOGRAPH
    {0xC278, 0x7377}, //9117 #CJK UNIFIED IDEOGRAPH
    {0xC279, 0x7375}, //9118 #CJK UNIFIED IDEOGRAPH
    {0xC27A, 0x74A7}, //9119 #CJK UNIFIED IDEOGRAPH
    {0xC27B, 0x74BF}, //9120 #CJK UNIFIED IDEOGRAPH
    {0xC27C, 0x7515}, //9121 #CJK UNIFIED IDEOGRAPH
    {0xC27D, 0x7656}, //9122 #CJK UNIFIED IDEOGRAPH
    {0xC27E, 0x7658}, //9123 #CJK UNIFIED IDEOGRAPH
    {0xC2A1, 0x7652}, //9124 #CJK UNIFIED IDEOGRAPH
    {0xC2A2, 0x77BD}, //9125 #CJK UNIFIED IDEOGRAPH
    {0xC2A3, 0x77BF}, //9126 #CJK UNIFIED IDEOGRAPH
    {0xC2A4, 0x77BB}, //9127 #CJK UNIFIED IDEOGRAPH
    {0xC2A5, 0x77BC}, //9128 #CJK UNIFIED IDEOGRAPH
    {0xC2A6, 0x790E}, //9129 #CJK UNIFIED IDEOGRAPH
    {0xC2A7, 0x79AE}, //9130 #CJK UNIFIED IDEOGRAPH
    {0xC2A8, 0x7A61}, //9131 #CJK UNIFIED IDEOGRAPH
    {0xC2A9, 0x7A62}, //9132 #CJK UNIFIED IDEOGRAPH
    {0xC2AA, 0x7A60}, //9133 #CJK UNIFIED IDEOGRAPH
    {0xC2AB, 0x7AC4}, //9134 #CJK UNIFIED IDEOGRAPH
    {0xC2AC, 0x7AC5}, //9135 #CJK UNIFIED IDEOGRAPH
    {0xC2AD, 0x7C2B}, //9136 #CJK UNIFIED IDEOGRAPH
    {0xC2AE, 0x7C27}, //9137 #CJK UNIFIED IDEOGRAPH
    {0xC2AF, 0x7C2A}, //9138 #CJK UNIFIED IDEOGRAPH
    {0xC2B0, 0x7C1E}, //9139 #CJK UNIFIED IDEOGRAPH
    {0xC2B1, 0x7C23}, //9140 #CJK UNIFIED IDEOGRAPH
    {0xC2B2, 0x7C21}, //9141 #CJK UNIFIED IDEOGRAPH
    {0xC2B3, 0x7CE7}, //9142 #CJK UNIFIED IDEOGRAPH
    {0xC2B4, 0x7E54}, //9143 #CJK UNIFIED IDEOGRAPH
    {0xC2B5, 0x7E55}, //9144 #CJK UNIFIED IDEOGRAPH
    {0xC2B6, 0x7E5E}, //9145 #CJK UNIFIED IDEOGRAPH
    {0xC2B7, 0x7E5A}, //9146 #CJK UNIFIED IDEOGRAPH
    {0xC2B8, 0x7E61}, //9147 #CJK UNIFIED IDEOGRAPH
    {0xC2B9, 0x7E52}, //9148 #CJK UNIFIED IDEOGRAPH
    {0xC2BA, 0x7E59}, //9149 #CJK UNIFIED IDEOGRAPH
    {0xC2BB, 0x7F48}, //9150 #CJK UNIFIED IDEOGRAPH
    {0xC2BC, 0x7FF9}, //9151 #CJK UNIFIED IDEOGRAPH
    {0xC2BD, 0x7FFB}, //9152 #CJK UNIFIED IDEOGRAPH
    {0xC2BE, 0x8077}, //9153 #CJK UNIFIED IDEOGRAPH
    {0xC2BF, 0x8076}, //9154 #CJK UNIFIED IDEOGRAPH
    {0xC2C0, 0x81CD}, //9155 #CJK UNIFIED IDEOGRAPH
    {0xC2C1, 0x81CF}, //9156 #CJK UNIFIED IDEOGRAPH
    {0xC2C2, 0x820A}, //9157 #CJK UNIFIED IDEOGRAPH
    {0xC2C3, 0x85CF}, //9158 #CJK UNIFIED IDEOGRAPH
    {0xC2C4, 0x85A9}, //9159 #CJK UNIFIED IDEOGRAPH
    {0xC2C5, 0x85CD}, //9160 #CJK UNIFIED IDEOGRAPH
    {0xC2C6, 0x85D0}, //9161 #CJK UNIFIED IDEOGRAPH
    {0xC2C7, 0x85C9}, //9162 #CJK UNIFIED IDEOGRAPH
    {0xC2C8, 0x85B0}, //9163 #CJK UNIFIED IDEOGRAPH
    {0xC2C9, 0x85BA}, //9164 #CJK UNIFIED IDEOGRAPH
    {0xC2CA, 0x85B9}, //9165 #CJK UNIFIED IDEOGRAPH
    {0xC2CB, 0x85A6}, //9166 #CJK UNIFIED IDEOGRAPH
    {0xC2CC, 0x87EF}, //9167 #CJK UNIFIED IDEOGRAPH
    {0xC2CD, 0x87EC}, //9168 #CJK UNIFIED IDEOGRAPH
    {0xC2CE, 0x87F2}, //9169 #CJK UNIFIED IDEOGRAPH
    {0xC2CF, 0x87E0}, //9170 #CJK UNIFIED IDEOGRAPH
    {0xC2D0, 0x8986}, //9171 #CJK UNIFIED IDEOGRAPH
    {0xC2D1, 0x89B2}, //9172 #CJK UNIFIED IDEOGRAPH
    {0xC2D2, 0x89F4}, //9173 #CJK UNIFIED IDEOGRAPH
    {0xC2D3, 0x8B28}, //9174 #CJK UNIFIED IDEOGRAPH
    {0xC2D4, 0x8B39}, //9175 #CJK UNIFIED IDEOGRAPH
    {0xC2D5, 0x8B2C}, //9176 #CJK UNIFIED IDEOGRAPH
    {0xC2D6, 0x8B2B}, //9177 #CJK UNIFIED IDEOGRAPH
    {0xC2D7, 0x8C50}, //9178 #CJK UNIFIED IDEOGRAPH
    {0xC2D8, 0x8D05}, //9179 #CJK UNIFIED IDEOGRAPH
    {0xC2D9, 0x8E59}, //9180 #CJK UNIFIED IDEOGRAPH
    {0xC2DA, 0x8E63}, //9181 #CJK UNIFIED IDEOGRAPH
    {0xC2DB, 0x8E66}, //9182 #CJK UNIFIED IDEOGRAPH
    {0xC2DC, 0x8E64}, //9183 #CJK UNIFIED IDEOGRAPH
    {0xC2DD, 0x8E5F}, //9184 #CJK UNIFIED IDEOGRAPH
    {0xC2DE, 0x8E55}, //9185 #CJK UNIFIED IDEOGRAPH
    {0xC2DF, 0x8EC0}, //9186 #CJK UNIFIED IDEOGRAPH
    {0xC2E0, 0x8F49}, //9187 #CJK UNIFIED IDEOGRAPH
    {0xC2E1, 0x8F4D}, //9188 #CJK UNIFIED IDEOGRAPH
    {0xC2E2, 0x9087}, //9189 #CJK UNIFIED IDEOGRAPH
    {0xC2E3, 0x9083}, //9190 #CJK UNIFIED IDEOGRAPH
    {0xC2E4, 0x9088}, //9191 #CJK UNIFIED IDEOGRAPH
    {0xC2E5, 0x91AB}, //9192 #CJK UNIFIED IDEOGRAPH
    {0xC2E6, 0x91AC}, //9193 #CJK UNIFIED IDEOGRAPH
    {0xC2E7, 0x91D0}, //9194 #CJK UNIFIED IDEOGRAPH
    {0xC2E8, 0x9394}, //9195 #CJK UNIFIED IDEOGRAPH
    {0xC2E9, 0x938A}, //9196 #CJK UNIFIED IDEOGRAPH
    {0xC2EA, 0x9396}, //9197 #CJK UNIFIED IDEOGRAPH
    {0xC2EB, 0x93A2}, //9198 #CJK UNIFIED IDEOGRAPH
    {0xC2EC, 0x93B3}, //9199 #CJK UNIFIED IDEOGRAPH
    {0xC2ED, 0x93AE}, //9200 #CJK UNIFIED IDEOGRAPH
    {0xC2EE, 0x93AC}, //9201 #CJK UNIFIED IDEOGRAPH
    {0xC2EF, 0x93B0}, //9202 #CJK UNIFIED IDEOGRAPH
    {0xC2F0, 0x9398}, //9203 #CJK UNIFIED IDEOGRAPH
    {0xC2F1, 0x939A}, //9204 #CJK UNIFIED IDEOGRAPH
    {0xC2F2, 0x9397}, //9205 #CJK UNIFIED IDEOGRAPH
    {0xC2F3, 0x95D4}, //9206 #CJK UNIFIED IDEOGRAPH
    {0xC2F4, 0x95D6}, //9207 #CJK UNIFIED IDEOGRAPH
    {0xC2F5, 0x95D0}, //9208 #CJK UNIFIED IDEOGRAPH
    {0xC2F6, 0x95D5}, //9209 #CJK UNIFIED IDEOGRAPH
    {0xC2F7, 0x96E2}, //9210 #CJK UNIFIED IDEOGRAPH
    {0xC2F8, 0x96DC}, //9211 #CJK UNIFIED IDEOGRAPH
    {0xC2F9, 0x96D9}, //9212 #CJK UNIFIED IDEOGRAPH
    {0xC2FA, 0x96DB}, //9213 #CJK UNIFIED IDEOGRAPH
    {0xC2FB, 0x96DE}, //9214 #CJK UNIFIED IDEOGRAPH
    {0xC2FC, 0x9724}, //9215 #CJK UNIFIED IDEOGRAPH
    {0xC2FD, 0x97A3}, //9216 #CJK UNIFIED IDEOGRAPH
    {0xC2FE, 0x97A6}, //9217 #CJK UNIFIED IDEOGRAPH
    {0xC340, 0x97AD}, //9218 #CJK UNIFIED IDEOGRAPH
    {0xC341, 0x97F9}, //9219 #CJK UNIFIED IDEOGRAPH
    {0xC342, 0x984D}, //9220 #CJK UNIFIED IDEOGRAPH
    {0xC343, 0x984F}, //9221 #CJK UNIFIED IDEOGRAPH
    {0xC344, 0x984C}, //9222 #CJK UNIFIED IDEOGRAPH
    {0xC345, 0x984E}, //9223 #CJK UNIFIED IDEOGRAPH
    {0xC346, 0x9853}, //9224 #CJK UNIFIED IDEOGRAPH
    {0xC347, 0x98BA}, //9225 #CJK UNIFIED IDEOGRAPH
    {0xC348, 0x993E}, //9226 #CJK UNIFIED IDEOGRAPH
    {0xC349, 0x993F}, //9227 #CJK UNIFIED IDEOGRAPH
    {0xC34A, 0x993D}, //9228 #CJK UNIFIED IDEOGRAPH
    {0xC34B, 0x992E}, //9229 #CJK UNIFIED IDEOGRAPH
    {0xC34C, 0x99A5}, //9230 #CJK UNIFIED IDEOGRAPH
    {0xC34D, 0x9A0E}, //9231 #CJK UNIFIED IDEOGRAPH
    {0xC34E, 0x9AC1}, //9232 #CJK UNIFIED IDEOGRAPH
    {0xC34F, 0x9B03}, //9233 #CJK UNIFIED IDEOGRAPH
    {0xC350, 0x9B06}, //9234 #CJK UNIFIED IDEOGRAPH
    {0xC351, 0x9B4F}, //9235 #CJK UNIFIED IDEOGRAPH
    {0xC352, 0x9B4E}, //9236 #CJK UNIFIED IDEOGRAPH
    {0xC353, 0x9B4D}, //9237 #CJK UNIFIED IDEOGRAPH
    {0xC354, 0x9BCA}, //9238 #CJK UNIFIED IDEOGRAPH
    {0xC355, 0x9BC9}, //9239 #CJK UNIFIED IDEOGRAPH
    {0xC356, 0x9BFD}, //9240 #CJK UNIFIED IDEOGRAPH
    {0xC357, 0x9BC8}, //9241 #CJK UNIFIED IDEOGRAPH
    {0xC358, 0x9BC0}, //9242 #CJK UNIFIED IDEOGRAPH
    {0xC359, 0x9D51}, //9243 #CJK UNIFIED IDEOGRAPH
    {0xC35A, 0x9D5D}, //9244 #CJK UNIFIED IDEOGRAPH
    {0xC35B, 0x9D60}, //9245 #CJK UNIFIED IDEOGRAPH
    {0xC35C, 0x9EE0}, //9246 #CJK UNIFIED IDEOGRAPH
    {0xC35D, 0x9F15}, //9247 #CJK UNIFIED IDEOGRAPH
    {0xC35E, 0x9F2C}, //9248 #CJK UNIFIED IDEOGRAPH
    {0xC35F, 0x5133}, //9249 #CJK UNIFIED IDEOGRAPH
    {0xC360, 0x56A5}, //9250 #CJK UNIFIED IDEOGRAPH
    {0xC361, 0x58DE}, //9251 #CJK UNIFIED IDEOGRAPH
    {0xC362, 0x58DF}, //9252 #CJK UNIFIED IDEOGRAPH
    {0xC363, 0x58E2}, //9253 #CJK UNIFIED IDEOGRAPH
    {0xC364, 0x5BF5}, //9254 #CJK UNIFIED IDEOGRAPH
    {0xC365, 0x9F90}, //9255 #CJK UNIFIED IDEOGRAPH
    {0xC366, 0x5EEC}, //9256 #CJK UNIFIED IDEOGRAPH
    {0xC367, 0x61F2}, //9257 #CJK UNIFIED IDEOGRAPH
    {0xC368, 0x61F7}, //9258 #CJK UNIFIED IDEOGRAPH
    {0xC369, 0x61F6}, //9259 #CJK UNIFIED IDEOGRAPH
    {0xC36A, 0x61F5}, //9260 #CJK UNIFIED IDEOGRAPH
    {0xC36B, 0x6500}, //9261 #CJK UNIFIED IDEOGRAPH
    {0xC36C, 0x650F}, //9262 #CJK UNIFIED IDEOGRAPH
    {0xC36D, 0x66E0}, //9263 #CJK UNIFIED IDEOGRAPH
    {0xC36E, 0x66DD}, //9264 #CJK UNIFIED IDEOGRAPH
    {0xC36F, 0x6AE5}, //9265 #CJK UNIFIED IDEOGRAPH
    {0xC370, 0x6ADD}, //9266 #CJK UNIFIED IDEOGRAPH
    {0xC371, 0x6ADA}, //9267 #CJK UNIFIED IDEOGRAPH
    {0xC372, 0x6AD3}, //9268 #CJK UNIFIED IDEOGRAPH
    {0xC373, 0x701B}, //9269 #CJK UNIFIED IDEOGRAPH
    {0xC374, 0x701F}, //9270 #CJK UNIFIED IDEOGRAPH
    {0xC375, 0x7028}, //9271 #CJK UNIFIED IDEOGRAPH
    {0xC376, 0x701A}, //9272 #CJK UNIFIED IDEOGRAPH
    {0xC377, 0x701D}, //9273 #CJK UNIFIED IDEOGRAPH
    {0xC378, 0x7015}, //9274 #CJK UNIFIED IDEOGRAPH
    {0xC379, 0x7018}, //9275 #CJK UNIFIED IDEOGRAPH
    {0xC37A, 0x7206}, //9276 #CJK UNIFIED IDEOGRAPH
    {0xC37B, 0x720D}, //9277 #CJK UNIFIED IDEOGRAPH
    {0xC37C, 0x7258}, //9278 #CJK UNIFIED IDEOGRAPH
    {0xC37D, 0x72A2}, //9279 #CJK UNIFIED IDEOGRAPH
    {0xC37E, 0x7378}, //9280 #CJK UNIFIED IDEOGRAPH
    {0xC3A1, 0x737A}, //9281 #CJK UNIFIED IDEOGRAPH
    {0xC3A2, 0x74BD}, //9282 #CJK UNIFIED IDEOGRAPH
    {0xC3A3, 0x74CA}, //9283 #CJK UNIFIED IDEOGRAPH
    {0xC3A4, 0x74E3}, //9284 #CJK UNIFIED IDEOGRAPH
    {0xC3A5, 0x7587}, //9285 #CJK UNIFIED IDEOGRAPH
    {0xC3A6, 0x7586}, //9286 #CJK UNIFIED IDEOGRAPH
    {0xC3A7, 0x765F}, //9287 #CJK UNIFIED IDEOGRAPH
    {0xC3A8, 0x7661}, //9288 #CJK UNIFIED IDEOGRAPH
    {0xC3A9, 0x77C7}, //9289 #CJK UNIFIED IDEOGRAPH
    {0xC3AA, 0x7919}, //9290 #CJK UNIFIED IDEOGRAPH
    {0xC3AB, 0x79B1}, //9291 #CJK UNIFIED IDEOGRAPH
    {0xC3AC, 0x7A6B}, //9292 #CJK UNIFIED IDEOGRAPH
    {0xC3AD, 0x7A69}, //9293 #CJK UNIFIED IDEOGRAPH
    {0xC3AE, 0x7C3E}, //9294 #CJK UNIFIED IDEOGRAPH
    {0xC3AF, 0x7C3F}, //9295 #CJK UNIFIED IDEOGRAPH
    {0xC3B0, 0x7C38}, //9296 #CJK UNIFIED IDEOGRAPH
    {0xC3B1, 0x7C3D}, //9297 #CJK UNIFIED IDEOGRAPH
    {0xC3B2, 0x7C37}, //9298 #CJK UNIFIED IDEOGRAPH
    {0xC3B3, 0x7C40}, //9299 #CJK UNIFIED IDEOGRAPH
    {0xC3B4, 0x7E6B}, //9300 #CJK UNIFIED IDEOGRAPH
    {0xC3B5, 0x7E6D}, //9301 #CJK UNIFIED IDEOGRAPH
    {0xC3B6, 0x7E79}, //9302 #CJK UNIFIED IDEOGRAPH
    {0xC3B7, 0x7E69}, //9303 #CJK UNIFIED IDEOGRAPH
    {0xC3B8, 0x7E6A}, //9304 #CJK UNIFIED IDEOGRAPH
    {0xC3B9, 0x7F85}, //9305 #CJK UNIFIED IDEOGRAPH
    {0xC3BA, 0x7E73}, //9306 #CJK UNIFIED IDEOGRAPH
    {0xC3BB, 0x7FB6}, //9307 #CJK UNIFIED IDEOGRAPH
    {0xC3BC, 0x7FB9}, //9308 #CJK UNIFIED IDEOGRAPH
    {0xC3BD, 0x7FB8}, //9309 #CJK UNIFIED IDEOGRAPH
    {0xC3BE, 0x81D8}, //9310 #CJK UNIFIED IDEOGRAPH
    {0xC3BF, 0x85E9}, //9311 #CJK UNIFIED IDEOGRAPH
    {0xC3C0, 0x85DD}, //9312 #CJK UNIFIED IDEOGRAPH
    {0xC3C1, 0x85EA}, //9313 #CJK UNIFIED IDEOGRAPH
    {0xC3C2, 0x85D5}, //9314 #CJK UNIFIED IDEOGRAPH
    {0xC3C3, 0x85E4}, //9315 #CJK UNIFIED IDEOGRAPH
    {0xC3C4, 0x85E5}, //9316 #CJK UNIFIED IDEOGRAPH
    {0xC3C5, 0x85F7}, //9317 #CJK UNIFIED IDEOGRAPH
    {0xC3C6, 0x87FB}, //9318 #CJK UNIFIED IDEOGRAPH
    {0xC3C7, 0x8805}, //9319 #CJK UNIFIED IDEOGRAPH
    {0xC3C8, 0x880D}, //9320 #CJK UNIFIED IDEOGRAPH
    {0xC3C9, 0x87F9}, //9321 #CJK UNIFIED IDEOGRAPH
    {0xC3CA, 0x87FE}, //9322 #CJK UNIFIED IDEOGRAPH
    {0xC3CB, 0x8960}, //9323 #CJK UNIFIED IDEOGRAPH
    {0xC3CC, 0x895F}, //9324 #CJK UNIFIED IDEOGRAPH
    {0xC3CD, 0x8956}, //9325 #CJK UNIFIED IDEOGRAPH
    {0xC3CE, 0x895E}, //9326 #CJK UNIFIED IDEOGRAPH
    {0xC3CF, 0x8B41}, //9327 #CJK UNIFIED IDEOGRAPH
    {0xC3D0, 0x8B5C}, //9328 #CJK UNIFIED IDEOGRAPH
    {0xC3D1, 0x8B58}, //9329 #CJK UNIFIED IDEOGRAPH
    {0xC3D2, 0x8B49}, //9330 #CJK UNIFIED IDEOGRAPH
    {0xC3D3, 0x8B5A}, //9331 #CJK UNIFIED IDEOGRAPH
    {0xC3D4, 0x8B4E}, //9332 #CJK UNIFIED IDEOGRAPH
    {0xC3D5, 0x8B4F}, //9333 #CJK UNIFIED IDEOGRAPH
    {0xC3D6, 0x8B46}, //9334 #CJK UNIFIED IDEOGRAPH
    {0xC3D7, 0x8B59}, //9335 #CJK UNIFIED IDEOGRAPH
    {0xC3D8, 0x8D08}, //9336 #CJK UNIFIED IDEOGRAPH
    {0xC3D9, 0x8D0A}, //9337 #CJK UNIFIED IDEOGRAPH
    {0xC3DA, 0x8E7C}, //9338 #CJK UNIFIED IDEOGRAPH
    {0xC3DB, 0x8E72}, //9339 #CJK UNIFIED IDEOGRAPH
    {0xC3DC, 0x8E87}, //9340 #CJK UNIFIED IDEOGRAPH
    {0xC3DD, 0x8E76}, //9341 #CJK UNIFIED IDEOGRAPH
    {0xC3DE, 0x8E6C}, //9342 #CJK UNIFIED IDEOGRAPH
    {0xC3DF, 0x8E7A}, //9343 #CJK UNIFIED IDEOGRAPH
    {0xC3E0, 0x8E74}, //9344 #CJK UNIFIED IDEOGRAPH
    {0xC3E1, 0x8F54}, //9345 #CJK UNIFIED IDEOGRAPH
    {0xC3E2, 0x8F4E}, //9346 #CJK UNIFIED IDEOGRAPH
    {0xC3E3, 0x8FAD}, //9347 #CJK UNIFIED IDEOGRAPH
    {0xC3E4, 0x908A}, //9348 #CJK UNIFIED IDEOGRAPH
    {0xC3E5, 0x908B}, //9349 #CJK UNIFIED IDEOGRAPH
    {0xC3E6, 0x91B1}, //9350 #CJK UNIFIED IDEOGRAPH
    {0xC3E7, 0x91AE}, //9351 #CJK UNIFIED IDEOGRAPH
    {0xC3E8, 0x93E1}, //9352 #CJK UNIFIED IDEOGRAPH
    {0xC3E9, 0x93D1}, //9353 #CJK UNIFIED IDEOGRAPH
    {0xC3EA, 0x93DF}, //9354 #CJK UNIFIED IDEOGRAPH
    {0xC3EB, 0x93C3}, //9355 #CJK UNIFIED IDEOGRAPH
    {0xC3EC, 0x93C8}, //9356 #CJK UNIFIED IDEOGRAPH
    {0xC3ED, 0x93DC}, //9357 #CJK UNIFIED IDEOGRAPH
    {0xC3EE, 0x93DD}, //9358 #CJK UNIFIED IDEOGRAPH
    {0xC3EF, 0x93D6}, //9359 #CJK UNIFIED IDEOGRAPH
    {0xC3F0, 0x93E2}, //9360 #CJK UNIFIED IDEOGRAPH
    {0xC3F1, 0x93CD}, //9361 #CJK UNIFIED IDEOGRAPH
    {0xC3F2, 0x93D8}, //9362 #CJK UNIFIED IDEOGRAPH
    {0xC3F3, 0x93E4}, //9363 #CJK UNIFIED IDEOGRAPH
    {0xC3F4, 0x93D7}, //9364 #CJK UNIFIED IDEOGRAPH
    {0xC3F5, 0x93E8}, //9365 #CJK UNIFIED IDEOGRAPH
    {0xC3F6, 0x95DC}, //9366 #CJK UNIFIED IDEOGRAPH
    {0xC3F7, 0x96B4}, //9367 #CJK UNIFIED IDEOGRAPH
    {0xC3F8, 0x96E3}, //9368 #CJK UNIFIED IDEOGRAPH
    {0xC3F9, 0x972A}, //9369 #CJK UNIFIED IDEOGRAPH
    {0xC3FA, 0x9727}, //9370 #CJK UNIFIED IDEOGRAPH
    {0xC3FB, 0x9761}, //9371 #CJK UNIFIED IDEOGRAPH
    {0xC3FC, 0x97DC}, //9372 #CJK UNIFIED IDEOGRAPH
    {0xC3FD, 0x97FB}, //9373 #CJK UNIFIED IDEOGRAPH
    {0xC3FE, 0x985E}, //9374 #CJK UNIFIED IDEOGRAPH
    {0xC440, 0x9858}, //9375 #CJK UNIFIED IDEOGRAPH
    {0xC441, 0x985B}, //9376 #CJK UNIFIED IDEOGRAPH
    {0xC442, 0x98BC}, //9377 #CJK UNIFIED IDEOGRAPH
    {0xC443, 0x9945}, //9378 #CJK UNIFIED IDEOGRAPH
    {0xC444, 0x9949}, //9379 #CJK UNIFIED IDEOGRAPH
    {0xC445, 0x9A16}, //9380 #CJK UNIFIED IDEOGRAPH
    {0xC446, 0x9A19}, //9381 #CJK UNIFIED IDEOGRAPH
    {0xC447, 0x9B0D}, //9382 #CJK UNIFIED IDEOGRAPH
    {0xC448, 0x9BE8}, //9383 #CJK UNIFIED IDEOGRAPH
    {0xC449, 0x9BE7}, //9384 #CJK UNIFIED IDEOGRAPH
    {0xC44A, 0x9BD6}, //9385 #CJK UNIFIED IDEOGRAPH
    {0xC44B, 0x9BDB}, //9386 #CJK UNIFIED IDEOGRAPH
    {0xC44C, 0x9D89}, //9387 #CJK UNIFIED IDEOGRAPH
    {0xC44D, 0x9D61}, //9388 #CJK UNIFIED IDEOGRAPH
    {0xC44E, 0x9D72}, //9389 #CJK UNIFIED IDEOGRAPH
    {0xC44F, 0x9D6A}, //9390 #CJK UNIFIED IDEOGRAPH
    {0xC450, 0x9D6C}, //9391 #CJK UNIFIED IDEOGRAPH
    {0xC451, 0x9E92}, //9392 #CJK UNIFIED IDEOGRAPH
    {0xC452, 0x9E97}, //9393 #CJK UNIFIED IDEOGRAPH
    {0xC453, 0x9E93}, //9394 #CJK UNIFIED IDEOGRAPH
    {0xC454, 0x9EB4}, //9395 #CJK UNIFIED IDEOGRAPH
    {0xC455, 0x52F8}, //9396 #CJK UNIFIED IDEOGRAPH
    {0xC456, 0x56A8}, //9397 #CJK UNIFIED IDEOGRAPH
    {0xC457, 0x56B7}, //9398 #CJK UNIFIED IDEOGRAPH
    {0xC458, 0x56B6}, //9399 #CJK UNIFIED IDEOGRAPH
    {0xC459, 0x56B4}, //9400 #CJK UNIFIED IDEOGRAPH
    {0xC45A, 0x56BC}, //9401 #CJK UNIFIED IDEOGRAPH
    {0xC45B, 0x58E4}, //9402 #CJK UNIFIED IDEOGRAPH
    {0xC45C, 0x5B40}, //9403 #CJK UNIFIED IDEOGRAPH
    {0xC45D, 0x5B43}, //9404 #CJK UNIFIED IDEOGRAPH
    {0xC45E, 0x5B7D}, //9405 #CJK UNIFIED IDEOGRAPH
    {0xC45F, 0x5BF6}, //9406 #CJK UNIFIED IDEOGRAPH
    {0xC460, 0x5DC9}, //9407 #CJK UNIFIED IDEOGRAPH
    {0xC461, 0x61F8}, //9408 #CJK UNIFIED IDEOGRAPH
    {0xC462, 0x61FA}, //9409 #CJK UNIFIED IDEOGRAPH
    {0xC463, 0x6518}, //9410 #CJK UNIFIED IDEOGRAPH
    {0xC464, 0x6514}, //9411 #CJK UNIFIED IDEOGRAPH
    {0xC465, 0x6519}, //9412 #CJK UNIFIED IDEOGRAPH
    {0xC466, 0x66E6}, //9413 #CJK UNIFIED IDEOGRAPH
    {0xC467, 0x6727}, //9414 #CJK UNIFIED IDEOGRAPH
    {0xC468, 0x6AEC}, //9415 #CJK UNIFIED IDEOGRAPH
    {0xC469, 0x703E}, //9416 #CJK UNIFIED IDEOGRAPH
    {0xC46A, 0x7030}, //9417 #CJK UNIFIED IDEOGRAPH
    {0xC46B, 0x7032}, //9418 #CJK UNIFIED IDEOGRAPH
    {0xC46C, 0x7210}, //9419 #CJK UNIFIED IDEOGRAPH
    {0xC46D, 0x737B}, //9420 #CJK UNIFIED IDEOGRAPH
    {0xC46E, 0x74CF}, //9421 #CJK UNIFIED IDEOGRAPH
    {0xC46F, 0x7662}, //9422 #CJK UNIFIED IDEOGRAPH
    {0xC470, 0x7665}, //9423 #CJK UNIFIED IDEOGRAPH
    {0xC471, 0x7926}, //9424 #CJK UNIFIED IDEOGRAPH
    {0xC472, 0x792A}, //9425 #CJK UNIFIED IDEOGRAPH
    {0xC473, 0x792C}, //9426 #CJK UNIFIED IDEOGRAPH
    {0xC474, 0x792B}, //9427 #CJK UNIFIED IDEOGRAPH
    {0xC475, 0x7AC7}, //9428 #CJK UNIFIED IDEOGRAPH
    {0xC476, 0x7AF6}, //9429 #CJK UNIFIED IDEOGRAPH
    {0xC477, 0x7C4C}, //9430 #CJK UNIFIED IDEOGRAPH
    {0xC478, 0x7C43}, //9431 #CJK UNIFIED IDEOGRAPH
    {0xC479, 0x7C4D}, //9432 #CJK UNIFIED IDEOGRAPH
    {0xC47A, 0x7CEF}, //9433 #CJK UNIFIED IDEOGRAPH
    {0xC47B, 0x7CF0}, //9434 #CJK UNIFIED IDEOGRAPH
    {0xC47C, 0x8FAE}, //9435 #CJK UNIFIED IDEOGRAPH
    {0xC47D, 0x7E7D}, //9436 #CJK UNIFIED IDEOGRAPH
    {0xC47E, 0x7E7C}, //9437 #CJK UNIFIED IDEOGRAPH
    {0xC4A1, 0x7E82}, //9438 #CJK UNIFIED IDEOGRAPH
    {0xC4A2, 0x7F4C}, //9439 #CJK UNIFIED IDEOGRAPH
    {0xC4A3, 0x8000}, //9440 #CJK UNIFIED IDEOGRAPH
    {0xC4A4, 0x81DA}, //9441 #CJK UNIFIED IDEOGRAPH
    {0xC4A5, 0x8266}, //9442 #CJK UNIFIED IDEOGRAPH
    {0xC4A6, 0x85FB}, //9443 #CJK UNIFIED IDEOGRAPH
    {0xC4A7, 0x85F9}, //9444 #CJK UNIFIED IDEOGRAPH
    {0xC4A8, 0x8611}, //9445 #CJK UNIFIED IDEOGRAPH
    {0xC4A9, 0x85FA}, //9446 #CJK UNIFIED IDEOGRAPH
    {0xC4AA, 0x8606}, //9447 #CJK UNIFIED IDEOGRAPH
    {0xC4AB, 0x860B}, //9448 #CJK UNIFIED IDEOGRAPH
    {0xC4AC, 0x8607}, //9449 #CJK UNIFIED IDEOGRAPH
    {0xC4AD, 0x860A}, //9450 #CJK UNIFIED IDEOGRAPH
    {0xC4AE, 0x8814}, //9451 #CJK UNIFIED IDEOGRAPH
    {0xC4AF, 0x8815}, //9452 #CJK UNIFIED IDEOGRAPH
    {0xC4B0, 0x8964}, //9453 #CJK UNIFIED IDEOGRAPH
    {0xC4B1, 0x89BA}, //9454 #CJK UNIFIED IDEOGRAPH
    {0xC4B2, 0x89F8}, //9455 #CJK UNIFIED IDEOGRAPH
    {0xC4B3, 0x8B70}, //9456 #CJK UNIFIED IDEOGRAPH
    {0xC4B4, 0x8B6C}, //9457 #CJK UNIFIED IDEOGRAPH
    {0xC4B5, 0x8B66}, //9458 #CJK UNIFIED IDEOGRAPH
    {0xC4B6, 0x8B6F}, //9459 #CJK UNIFIED IDEOGRAPH
    {0xC4B7, 0x8B5F}, //9460 #CJK UNIFIED IDEOGRAPH
    {0xC4B8, 0x8B6B}, //9461 #CJK UNIFIED IDEOGRAPH
    {0xC4B9, 0x8D0F}, //9462 #CJK UNIFIED IDEOGRAPH
    {0xC4BA, 0x8D0D}, //9463 #CJK UNIFIED IDEOGRAPH
    {0xC4BB, 0x8E89}, //9464 #CJK UNIFIED IDEOGRAPH
    {0xC4BC, 0x8E81}, //9465 #CJK UNIFIED IDEOGRAPH
    {0xC4BD, 0x8E85}, //9466 #CJK UNIFIED IDEOGRAPH
    {0xC4BE, 0x8E82}, //9467 #CJK UNIFIED IDEOGRAPH
    {0xC4BF, 0x91B4}, //9468 #CJK UNIFIED IDEOGRAPH
    {0xC4C0, 0x91CB}, //9469 #CJK UNIFIED IDEOGRAPH
    {0xC4C1, 0x9418}, //9470 #CJK UNIFIED IDEOGRAPH
    {0xC4C2, 0x9403}, //9471 #CJK UNIFIED IDEOGRAPH
    {0xC4C3, 0x93FD}, //9472 #CJK UNIFIED IDEOGRAPH
    {0xC4C4, 0x95E1}, //9473 #CJK UNIFIED IDEOGRAPH
    {0xC4C5, 0x9730}, //9474 #CJK UNIFIED IDEOGRAPH
    {0xC4C6, 0x98C4}, //9475 #CJK UNIFIED IDEOGRAPH
    {0xC4C7, 0x9952}, //9476 #CJK UNIFIED IDEOGRAPH
    {0xC4C8, 0x9951}, //9477 #CJK UNIFIED IDEOGRAPH
    {0xC4C9, 0x99A8}, //9478 #CJK UNIFIED IDEOGRAPH
    {0xC4CA, 0x9A2B}, //9479 #CJK UNIFIED IDEOGRAPH
    {0xC4CB, 0x9A30}, //9480 #CJK UNIFIED IDEOGRAPH
    {0xC4CC, 0x9A37}, //9481 #CJK UNIFIED IDEOGRAPH
    {0xC4CD, 0x9A35}, //9482 #CJK UNIFIED IDEOGRAPH
    {0xC4CE, 0x9C13}, //9483 #CJK UNIFIED IDEOGRAPH
    {0xC4CF, 0x9C0D}, //9484 #CJK UNIFIED IDEOGRAPH
    {0xC4D0, 0x9E79}, //9485 #CJK UNIFIED IDEOGRAPH
    {0xC4D1, 0x9EB5}, //9486 #CJK UNIFIED IDEOGRAPH
    {0xC4D2, 0x9EE8}, //9487 #CJK UNIFIED IDEOGRAPH
    {0xC4D3, 0x9F2F}, //9488 #CJK UNIFIED IDEOGRAPH
    {0xC4D4, 0x9F5F}, //9489 #CJK UNIFIED IDEOGRAPH
    {0xC4D5, 0x9F63}, //9490 #CJK UNIFIED IDEOGRAPH
    {0xC4D6, 0x9F61}, //9491 #CJK UNIFIED IDEOGRAPH
    {0xC4D7, 0x5137}, //9492 #CJK UNIFIED IDEOGRAPH
    {0xC4D8, 0x5138}, //9493 #CJK UNIFIED IDEOGRAPH
    {0xC4D9, 0x56C1}, //9494 #CJK UNIFIED IDEOGRAPH
    {0xC4DA, 0x56C0}, //9495 #CJK UNIFIED IDEOGRAPH
    {0xC4DB, 0x56C2}, //9496 #CJK UNIFIED IDEOGRAPH
    {0xC4DC, 0x5914}, //9497 #CJK UNIFIED IDEOGRAPH
    {0xC4DD, 0x5C6C}, //9498 #CJK UNIFIED IDEOGRAPH
    {0xC4DE, 0x5DCD}, //9499 #CJK UNIFIED IDEOGRAPH
    {0xC4DF, 0x61FC}, //9500 #CJK UNIFIED IDEOGRAPH
    {0xC4E0, 0x61FE}, //9501 #CJK UNIFIED IDEOGRAPH
    {0xC4E1, 0x651D}, //9502 #CJK UNIFIED IDEOGRAPH
    {0xC4E2, 0x651C}, //9503 #CJK UNIFIED IDEOGRAPH
    {0xC4E3, 0x6595}, //9504 #CJK UNIFIED IDEOGRAPH
    {0xC4E4, 0x66E9}, //9505 #CJK UNIFIED IDEOGRAPH
    {0xC4E5, 0x6AFB}, //9506 #CJK UNIFIED IDEOGRAPH
    {0xC4E6, 0x6B04}, //9507 #CJK UNIFIED IDEOGRAPH
    {0xC4E7, 0x6AFA}, //9508 #CJK UNIFIED IDEOGRAPH
    {0xC4E8, 0x6BB2}, //9509 #CJK UNIFIED IDEOGRAPH
    {0xC4E9, 0x704C}, //9510 #CJK UNIFIED IDEOGRAPH
    {0xC4EA, 0x721B}, //9511 #CJK UNIFIED IDEOGRAPH
    {0xC4EB, 0x72A7}, //9512 #CJK UNIFIED IDEOGRAPH
    {0xC4EC, 0x74D6}, //9513 #CJK UNIFIED IDEOGRAPH
    {0xC4ED, 0x74D4}, //9514 #CJK UNIFIED IDEOGRAPH
    {0xC4EE, 0x7669}, //9515 #CJK UNIFIED IDEOGRAPH
    {0xC4EF, 0x77D3}, //9516 #CJK UNIFIED IDEOGRAPH
    {0xC4F0, 0x7C50}, //9517 #CJK UNIFIED IDEOGRAPH
    {0xC4F1, 0x7E8F}, //9518 #CJK UNIFIED IDEOGRAPH
    {0xC4F2, 0x7E8C}, //9519 #CJK UNIFIED IDEOGRAPH
    {0xC4F3, 0x7FBC}, //9520 #CJK UNIFIED IDEOGRAPH
    {0xC4F4, 0x8617}, //9521 #CJK UNIFIED IDEOGRAPH
    {0xC4F5, 0x862D}, //9522 #CJK UNIFIED IDEOGRAPH
    {0xC4F6, 0x861A}, //9523 #CJK UNIFIED IDEOGRAPH
    {0xC4F7, 0x8823}, //9524 #CJK UNIFIED IDEOGRAPH
    {0xC4F8, 0x8822}, //9525 #CJK UNIFIED IDEOGRAPH
    {0xC4F9, 0x8821}, //9526 #CJK UNIFIED IDEOGRAPH
    {0xC4FA, 0x881F}, //9527 #CJK UNIFIED IDEOGRAPH
    {0xC4FB, 0x896A}, //9528 #CJK UNIFIED IDEOGRAPH
    {0xC4FC, 0x896C}, //9529 #CJK UNIFIED IDEOGRAPH
    {0xC4FD, 0x89BD}, //9530 #CJK UNIFIED IDEOGRAPH
    {0xC4FE, 0x8B74}, //9531 #CJK UNIFIED IDEOGRAPH
    {0xC540, 0x8B77}, //9532 #CJK UNIFIED IDEOGRAPH
    {0xC541, 0x8B7D}, //9533 #CJK UNIFIED IDEOGRAPH
    {0xC542, 0x8D13}, //9534 #CJK UNIFIED IDEOGRAPH
    {0xC543, 0x8E8A}, //9535 #CJK UNIFIED IDEOGRAPH
    {0xC544, 0x8E8D}, //9536 #CJK UNIFIED IDEOGRAPH
    {0xC545, 0x8E8B}, //9537 #CJK UNIFIED IDEOGRAPH
    {0xC546, 0x8F5F}, //9538 #CJK UNIFIED IDEOGRAPH
    {0xC547, 0x8FAF}, //9539 #CJK UNIFIED IDEOGRAPH
    {0xC548, 0x91BA}, //9540 #CJK UNIFIED IDEOGRAPH
    {0xC549, 0x942E}, //9541 #CJK UNIFIED IDEOGRAPH
    {0xC54A, 0x9433}, //9542 #CJK UNIFIED IDEOGRAPH
    {0xC54B, 0x9435}, //9543 #CJK UNIFIED IDEOGRAPH
    {0xC54C, 0x943A}, //9544 #CJK UNIFIED IDEOGRAPH
    {0xC54D, 0x9438}, //9545 #CJK UNIFIED IDEOGRAPH
    {0xC54E, 0x9432}, //9546 #CJK UNIFIED IDEOGRAPH
    {0xC54F, 0x942B}, //9547 #CJK UNIFIED IDEOGRAPH
    {0xC550, 0x95E2}, //9548 #CJK UNIFIED IDEOGRAPH
    {0xC551, 0x9738}, //9549 #CJK UNIFIED IDEOGRAPH
    {0xC552, 0x9739}, //9550 #CJK UNIFIED IDEOGRAPH
    {0xC553, 0x9732}, //9551 #CJK UNIFIED IDEOGRAPH
    {0xC554, 0x97FF}, //9552 #CJK UNIFIED IDEOGRAPH
    {0xC555, 0x9867}, //9553 #CJK UNIFIED IDEOGRAPH
    {0xC556, 0x9865}, //9554 #CJK UNIFIED IDEOGRAPH
    {0xC557, 0x9957}, //9555 #CJK UNIFIED IDEOGRAPH
    {0xC558, 0x9A45}, //9556 #CJK UNIFIED IDEOGRAPH
    {0xC559, 0x9A43}, //9557 #CJK UNIFIED IDEOGRAPH
    {0xC55A, 0x9A40}, //9558 #CJK UNIFIED IDEOGRAPH
    {0xC55B, 0x9A3E}, //9559 #CJK UNIFIED IDEOGRAPH
    {0xC55C, 0x9ACF}, //9560 #CJK UNIFIED IDEOGRAPH
    {0xC55D, 0x9B54}, //9561 #CJK UNIFIED IDEOGRAPH
    {0xC55E, 0x9B51}, //9562 #CJK UNIFIED IDEOGRAPH
    {0xC55F, 0x9C2D}, //9563 #CJK UNIFIED IDEOGRAPH
    {0xC560, 0x9C25}, //9564 #CJK UNIFIED IDEOGRAPH
    {0xC561, 0x9DAF}, //9565 #CJK UNIFIED IDEOGRAPH
    {0xC562, 0x9DB4}, //9566 #CJK UNIFIED IDEOGRAPH
    {0xC563, 0x9DC2}, //9567 #CJK UNIFIED IDEOGRAPH
    {0xC564, 0x9DB8}, //9568 #CJK UNIFIED IDEOGRAPH
    {0xC565, 0x9E9D}, //9569 #CJK UNIFIED IDEOGRAPH
    {0xC566, 0x9EEF}, //9570 #CJK UNIFIED IDEOGRAPH
    {0xC567, 0x9F19}, //9571 #CJK UNIFIED IDEOGRAPH
    {0xC568, 0x9F5C}, //9572 #CJK UNIFIED IDEOGRAPH
    {0xC569, 0x9F66}, //9573 #CJK UNIFIED IDEOGRAPH
    {0xC56A, 0x9F67}, //9574 #CJK UNIFIED IDEOGRAPH
    {0xC56B, 0x513C}, //9575 #CJK UNIFIED IDEOGRAPH
    {0xC56C, 0x513B}, //9576 #CJK UNIFIED IDEOGRAPH
    {0xC56D, 0x56C8}, //9577 #CJK UNIFIED IDEOGRAPH
    {0xC56E, 0x56CA}, //9578 #CJK UNIFIED IDEOGRAPH
    {0xC56F, 0x56C9}, //9579 #CJK UNIFIED IDEOGRAPH
    {0xC570, 0x5B7F}, //9580 #CJK UNIFIED IDEOGRAPH
    {0xC571, 0x5DD4}, //9581 #CJK UNIFIED IDEOGRAPH
    {0xC572, 0x5DD2}, //9582 #CJK UNIFIED IDEOGRAPH
    {0xC573, 0x5F4E}, //9583 #CJK UNIFIED IDEOGRAPH
    {0xC574, 0x61FF}, //9584 #CJK UNIFIED IDEOGRAPH
    {0xC575, 0x6524}, //9585 #CJK UNIFIED IDEOGRAPH
    {0xC576, 0x6B0A}, //9586 #CJK UNIFIED IDEOGRAPH
    {0xC577, 0x6B61}, //9587 #CJK UNIFIED IDEOGRAPH
    {0xC578, 0x7051}, //9588 #CJK UNIFIED IDEOGRAPH
    {0xC579, 0x7058}, //9589 #CJK UNIFIED IDEOGRAPH
    {0xC57A, 0x7380}, //9590 #CJK UNIFIED IDEOGRAPH
    {0xC57B, 0x74E4}, //9591 #CJK UNIFIED IDEOGRAPH
    {0xC57C, 0x758A}, //9592 #CJK UNIFIED IDEOGRAPH
    {0xC57D, 0x766E}, //9593 #CJK UNIFIED IDEOGRAPH
    {0xC57E, 0x766C}, //9594 #CJK UNIFIED IDEOGRAPH
    {0xC5A1, 0x79B3}, //9595 #CJK UNIFIED IDEOGRAPH
    {0xC5A2, 0x7C60}, //9596 #CJK UNIFIED IDEOGRAPH
    {0xC5A3, 0x7C5F}, //9597 #CJK UNIFIED IDEOGRAPH
    {0xC5A4, 0x807E}, //9598 #CJK UNIFIED IDEOGRAPH
    {0xC5A5, 0x807D}, //9599 #CJK UNIFIED IDEOGRAPH
    {0xC5A6, 0x81DF}, //9600 #CJK UNIFIED IDEOGRAPH
    {0xC5A7, 0x8972}, //9601 #CJK UNIFIED IDEOGRAPH
    {0xC5A8, 0x896F}, //9602 #CJK UNIFIED IDEOGRAPH
    {0xC5A9, 0x89FC}, //9603 #CJK UNIFIED IDEOGRAPH
    {0xC5AA, 0x8B80}, //9604 #CJK UNIFIED IDEOGRAPH
    {0xC5AB, 0x8D16}, //9605 #CJK UNIFIED IDEOGRAPH
    {0xC5AC, 0x8D17}, //9606 #CJK UNIFIED IDEOGRAPH
    {0xC5AD, 0x8E91}, //9607 #CJK UNIFIED IDEOGRAPH
    {0xC5AE, 0x8E93}, //9608 #CJK UNIFIED IDEOGRAPH
    {0xC5AF, 0x8F61}, //9609 #CJK UNIFIED IDEOGRAPH
    {0xC5B0, 0x9148}, //9610 #CJK UNIFIED IDEOGRAPH
    {0xC5B1, 0x9444}, //9611 #CJK UNIFIED IDEOGRAPH
    {0xC5B2, 0x9451}, //9612 #CJK UNIFIED IDEOGRAPH
    {0xC5B3, 0x9452}, //9613 #CJK UNIFIED IDEOGRAPH
    {0xC5B4, 0x973D}, //9614 #CJK UNIFIED IDEOGRAPH
    {0xC5B5, 0x973E}, //9615 #CJK UNIFIED IDEOGRAPH
    {0xC5B6, 0x97C3}, //9616 #CJK UNIFIED IDEOGRAPH
    {0xC5B7, 0x97C1}, //9617 #CJK UNIFIED IDEOGRAPH
    {0xC5B8, 0x986B}, //9618 #CJK UNIFIED IDEOGRAPH
    {0xC5B9, 0x9955}, //9619 #CJK UNIFIED IDEOGRAPH
    {0xC5BA, 0x9A55}, //9620 #CJK UNIFIED IDEOGRAPH
    {0xC5BB, 0x9A4D}, //9621 #CJK UNIFIED IDEOGRAPH
    {0xC5BC, 0x9AD2}, //9622 #CJK UNIFIED IDEOGRAPH
    {0xC5BD, 0x9B1A}, //9623 #CJK UNIFIED IDEOGRAPH
    {0xC5BE, 0x9C49}, //9624 #CJK UNIFIED IDEOGRAPH
    {0xC5BF, 0x9C31}, //9625 #CJK UNIFIED IDEOGRAPH
    {0xC5C0, 0x9C3E}, //9626 #CJK UNIFIED IDEOGRAPH
    {0xC5C1, 0x9C3B}, //9627 #CJK UNIFIED IDEOGRAPH
    {0xC5C2, 0x9DD3}, //9628 #CJK UNIFIED IDEOGRAPH
    {0xC5C3, 0x9DD7}, //9629 #CJK UNIFIED IDEOGRAPH
    {0xC5C4, 0x9F34}, //9630 #CJK UNIFIED IDEOGRAPH
    {0xC5C5, 0x9F6C}, //9631 #CJK UNIFIED IDEOGRAPH
    {0xC5C6, 0x9F6A}, //9632 #CJK UNIFIED IDEOGRAPH
    {0xC5C7, 0x9F94}, //9633 #CJK UNIFIED IDEOGRAPH
    {0xC5C8, 0x56CC}, //9634 #CJK UNIFIED IDEOGRAPH
    {0xC5C9, 0x5DD6}, //9635 #CJK UNIFIED IDEOGRAPH
    {0xC5CA, 0x6200}, //9636 #CJK UNIFIED IDEOGRAPH
    {0xC5CB, 0x6523}, //9637 #CJK UNIFIED IDEOGRAPH
    {0xC5CC, 0x652B}, //9638 #CJK UNIFIED IDEOGRAPH
    {0xC5CD, 0x652A}, //9639 #CJK UNIFIED IDEOGRAPH
    {0xC5CE, 0x66EC}, //9640 #CJK UNIFIED IDEOGRAPH
    {0xC5CF, 0x6B10}, //9641 #CJK UNIFIED IDEOGRAPH
    {0xC5D0, 0x74DA}, //9642 #CJK UNIFIED IDEOGRAPH
    {0xC5D1, 0x7ACA}, //9643 #CJK UNIFIED IDEOGRAPH
    {0xC5D2, 0x7C64}, //9644 #CJK UNIFIED IDEOGRAPH
    {0xC5D3, 0x7C63}, //9645 #CJK UNIFIED IDEOGRAPH
    {0xC5D4, 0x7C65}, //9646 #CJK UNIFIED IDEOGRAPH
    {0xC5D5, 0x7E93}, //9647 #CJK UNIFIED IDEOGRAPH
    {0xC5D6, 0x7E96}, //9648 #CJK UNIFIED IDEOGRAPH
    {0xC5D7, 0x7E94}, //9649 #CJK UNIFIED IDEOGRAPH
    {0xC5D8, 0x81E2}, //9650 #CJK UNIFIED IDEOGRAPH
    {0xC5D9, 0x8638}, //9651 #CJK UNIFIED IDEOGRAPH
    {0xC5DA, 0x863F}, //9652 #CJK UNIFIED IDEOGRAPH
    {0xC5DB, 0x8831}, //9653 #CJK UNIFIED IDEOGRAPH
    {0xC5DC, 0x8B8A}, //9654 #CJK UNIFIED IDEOGRAPH
    {0xC5DD, 0x9090}, //9655 #CJK UNIFIED IDEOGRAPH
    {0xC5DE, 0x908F}, //9656 #CJK UNIFIED IDEOGRAPH
    {0xC5DF, 0x9463}, //9657 #CJK UNIFIED IDEOGRAPH
    {0xC5E0, 0x9460}, //9658 #CJK UNIFIED IDEOGRAPH
    {0xC5E1, 0x9464}, //9659 #CJK UNIFIED IDEOGRAPH
    {0xC5E2, 0x9768}, //9660 #CJK UNIFIED IDEOGRAPH
    {0xC5E3, 0x986F}, //9661 #CJK UNIFIED IDEOGRAPH
    {0xC5E4, 0x995C}, //9662 #CJK UNIFIED IDEOGRAPH
    {0xC5E5, 0x9A5A}, //9663 #CJK UNIFIED IDEOGRAPH
    {0xC5E6, 0x9A5B}, //9664 #CJK UNIFIED IDEOGRAPH
    {0xC5E7, 0x9A57}, //9665 #CJK UNIFIED IDEOGRAPH
    {0xC5E8, 0x9AD3}, //9666 #CJK UNIFIED IDEOGRAPH
    {0xC5E9, 0x9AD4}, //9667 #CJK UNIFIED IDEOGRAPH
    {0xC5EA, 0x9AD1}, //9668 #CJK UNIFIED IDEOGRAPH
    {0xC5EB, 0x9C54}, //9669 #CJK UNIFIED IDEOGRAPH
    {0xC5EC, 0x9C57}, //9670 #CJK UNIFIED IDEOGRAPH
    {0xC5ED, 0x9C56}, //9671 #CJK UNIFIED IDEOGRAPH
    {0xC5EE, 0x9DE5}, //9672 #CJK UNIFIED IDEOGRAPH
    {0xC5EF, 0x9E9F}, //9673 #CJK UNIFIED IDEOGRAPH
    {0xC5F0, 0x9EF4}, //9674 #CJK UNIFIED IDEOGRAPH
    {0xC5F1, 0x56D1}, //9675 #CJK UNIFIED IDEOGRAPH
    {0xC5F2, 0x58E9}, //9676 #CJK UNIFIED IDEOGRAPH
    {0xC5F3, 0x652C}, //9677 #CJK UNIFIED IDEOGRAPH
    {0xC5F4, 0x705E}, //9678 #CJK UNIFIED IDEOGRAPH
    {0xC5F5, 0x7671}, //9679 #CJK UNIFIED IDEOGRAPH
    {0xC5F6, 0x7672}, //9680 #CJK UNIFIED IDEOGRAPH
    {0xC5F7, 0x77D7}, //9681 #CJK UNIFIED IDEOGRAPH
    {0xC5F8, 0x7F50}, //9682 #CJK UNIFIED IDEOGRAPH
    {0xC5F9, 0x7F88}, //9683 #CJK UNIFIED IDEOGRAPH
    {0xC5FA, 0x8836}, //9684 #CJK UNIFIED IDEOGRAPH
    {0xC5FB, 0x8839}, //9685 #CJK UNIFIED IDEOGRAPH
    {0xC5FC, 0x8862}, //9686 #CJK UNIFIED IDEOGRAPH
    {0xC5FD, 0x8B93}, //9687 #CJK UNIFIED IDEOGRAPH
    {0xC5FE, 0x8B92}, //9688 #CJK UNIFIED IDEOGRAPH
    {0xC640, 0x8B96}, //9689 #CJK UNIFIED IDEOGRAPH
    {0xC641, 0x8277}, //9690 #CJK UNIFIED IDEOGRAPH
    {0xC642, 0x8D1B}, //9691 #CJK UNIFIED IDEOGRAPH
    {0xC643, 0x91C0}, //9692 #CJK UNIFIED IDEOGRAPH
    {0xC644, 0x946A}, //9693 #CJK UNIFIED IDEOGRAPH
    {0xC645, 0x9742}, //9694 #CJK UNIFIED IDEOGRAPH
    {0xC646, 0x9748}, //9695 #CJK UNIFIED IDEOGRAPH
    {0xC647, 0x9744}, //9696 #CJK UNIFIED IDEOGRAPH
    {0xC648, 0x97C6}, //9697 #CJK UNIFIED IDEOGRAPH
    {0xC649, 0x9870}, //9698 #CJK UNIFIED IDEOGRAPH
    {0xC64A, 0x9A5F}, //9699 #CJK UNIFIED IDEOGRAPH
    {0xC64B, 0x9B22}, //9700 #CJK UNIFIED IDEOGRAPH
    {0xC64C, 0x9B58}, //9701 #CJK UNIFIED IDEOGRAPH
    {0xC64D, 0x9C5F}, //9702 #CJK UNIFIED IDEOGRAPH
    {0xC64E, 0x9DF9}, //9703 #CJK UNIFIED IDEOGRAPH
    {0xC64F, 0x9DFA}, //9704 #CJK UNIFIED IDEOGRAPH
    {0xC650, 0x9E7C}, //9705 #CJK UNIFIED IDEOGRAPH
    {0xC651, 0x9E7D}, //9706 #CJK UNIFIED IDEOGRAPH
    {0xC652, 0x9F07}, //9707 #CJK UNIFIED IDEOGRAPH
    {0xC653, 0x9F77}, //9708 #CJK UNIFIED IDEOGRAPH
    {0xC654, 0x9F72}, //9709 #CJK UNIFIED IDEOGRAPH
    {0xC655, 0x5EF3}, //9710 #CJK UNIFIED IDEOGRAPH
    {0xC656, 0x6B16}, //9711 #CJK UNIFIED IDEOGRAPH
    {0xC657, 0x7063}, //9712 #CJK UNIFIED IDEOGRAPH
    {0xC658, 0x7C6C}, //9713 #CJK UNIFIED IDEOGRAPH
    {0xC659, 0x7C6E}, //9714 #CJK UNIFIED IDEOGRAPH
    {0xC65A, 0x883B}, //9715 #CJK UNIFIED IDEOGRAPH
    {0xC65B, 0x89C0}, //9716 #CJK UNIFIED IDEOGRAPH
    {0xC65C, 0x8EA1}, //9717 #CJK UNIFIED IDEOGRAPH
    {0xC65D, 0x91C1}, //9718 #CJK UNIFIED IDEOGRAPH
    {0xC65E, 0x9472}, //9719 #CJK UNIFIED IDEOGRAPH
    {0xC65F, 0x9470}, //9720 #CJK UNIFIED IDEOGRAPH
    {0xC660, 0x9871}, //9721 #CJK UNIFIED IDEOGRAPH
    {0xC661, 0x995E}, //9722 #CJK UNIFIED IDEOGRAPH
    {0xC662, 0x9AD6}, //9723 #CJK UNIFIED IDEOGRAPH
    {0xC663, 0x9B23}, //9724 #CJK UNIFIED IDEOGRAPH
    {0xC664, 0x9ECC}, //9725 #CJK UNIFIED IDEOGRAPH
    {0xC665, 0x7064}, //9726 #CJK UNIFIED IDEOGRAPH
    {0xC666, 0x77DA}, //9727 #CJK UNIFIED IDEOGRAPH
    {0xC667, 0x8B9A}, //9728 #CJK UNIFIED IDEOGRAPH
    {0xC668, 0x9477}, //9729 #CJK UNIFIED IDEOGRAPH
    {0xC669, 0x97C9}, //9730 #CJK UNIFIED IDEOGRAPH
    {0xC66A, 0x9A62}, //9731 #CJK UNIFIED IDEOGRAPH
    {0xC66B, 0x9A65}, //9732 #CJK UNIFIED IDEOGRAPH
    {0xC66C, 0x7E9C}, //9733 #CJK UNIFIED IDEOGRAPH
    {0xC66D, 0x8B9C}, //9734 #CJK UNIFIED IDEOGRAPH
    {0xC66E, 0x8EAA}, //9735 #CJK UNIFIED IDEOGRAPH
    {0xC66F, 0x91C5}, //9736 #CJK UNIFIED IDEOGRAPH
    {0xC670, 0x947D}, //9737 #CJK UNIFIED IDEOGRAPH
    {0xC671, 0x947E}, //9738 #CJK UNIFIED IDEOGRAPH
    {0xC672, 0x947C}, //9739 #CJK UNIFIED IDEOGRAPH
    {0xC673, 0x9C77}, //9740 #CJK UNIFIED IDEOGRAPH
    {0xC674, 0x9C78}, //9741 #CJK UNIFIED IDEOGRAPH
    {0xC675, 0x9EF7}, //9742 #CJK UNIFIED IDEOGRAPH
    {0xC676, 0x8C54}, //9743 #CJK UNIFIED IDEOGRAPH
    {0xC677, 0x947F}, //9744 #CJK UNIFIED IDEOGRAPH
    {0xC678, 0x9E1A}, //9745 #CJK UNIFIED IDEOGRAPH
    {0xC679, 0x7228}, //9746 #CJK UNIFIED IDEOGRAPH
    {0xC67A, 0x9A6A}, //9747 #CJK UNIFIED IDEOGRAPH
    {0xC67B, 0x9B31}, //9748 #CJK UNIFIED IDEOGRAPH
    {0xC67C, 0x9E1B}, //9749 #CJK UNIFIED IDEOGRAPH
    {0xC67D, 0x9E1E}, //9750 #CJK UNIFIED IDEOGRAPH
    {0xC67E, 0x7C72}, //9751 #CJK UNIFIED IDEOGRAPH
    {0xC6A1, 0x2460}, //9752 #big5-hkscs
    {0xC6A2, 0x2461}, //9753 #big5-hkscs
    {0xC6A3, 0x2462}, //9754 #big5-hkscs
    {0xC6A4, 0x2463}, //9755 #big5-hkscs
    {0xC6A5, 0x2464}, //9756 #big5-hkscs
    {0xC6A6, 0x2465}, //9757 #big5-hkscs
    {0xC6A7, 0x2466}, //9758 #big5-hkscs
    {0xC6A8, 0x2467}, //9759 #big5-hkscs
    {0xC6A9, 0x2468}, //9760 #big5-hkscs
    {0xC6AA, 0x2469}, //9761 #big5-hkscs
    {0xC6AB, 0x2474}, //9762 #big5-hkscs
    {0xC6AC, 0x2475}, //9763 #big5-hkscs
    {0xC6AD, 0x2476}, //9764 #big5-hkscs
    {0xC6AE, 0x2477}, //9765 #big5-hkscs
    {0xC6AF, 0x2478}, //9766 #big5-hkscs
    {0xC6B0, 0x2479}, //9767 #big5-hkscs
    {0xC6B1, 0x247A}, //9768 #big5-hkscs
    {0xC6B2, 0x247B}, //9769 #big5-hkscs
    {0xC6B3, 0x247C}, //9770 #big5-hkscs
    {0xC6B4, 0x247D}, //9771 #big5-hkscs
    {0xC6B5, 0x2170}, //9772 #big5-hkscs
    {0xC6B6, 0x2171}, //9773 #big5-hkscs
    {0xC6B7, 0x2172}, //9774 #big5-hkscs
    {0xC6B8, 0x2173}, //9775 #big5-hkscs
    {0xC6B9, 0x2174}, //9776 #big5-hkscs
    {0xC6BA, 0x2175}, //9777 #big5-hkscs
    {0xC6BB, 0x2176}, //9778 #big5-hkscs
    {0xC6BC, 0x2177}, //9779 #big5-hkscs
    {0xC6BD, 0x2178}, //9780 #big5-hkscs
    {0xC6BE, 0x2179}, //9781 #big5-hkscs
    {0xC6BF, 0x4E36}, //9782 #big5-hkscs
    {0xC6C0, 0x4E3F}, //9783 #big5-hkscs
    {0xC6C1, 0x4E85}, //9784 #big5-hkscs
    {0xC6C2, 0x4EA0}, //9785 #big5-hkscs
    {0xC6C3, 0x5182}, //9786 #big5-hkscs
    {0xC6C4, 0x5196}, //9787 #big5-hkscs
    {0xC6C5, 0x51AB}, //9788 #big5-hkscs
    {0xC6C6, 0x52F9}, //9789 #big5-hkscs
    {0xC6C7, 0x5338}, //9790 #big5-hkscs
    {0xC6C8, 0x5369}, //9791 #big5-hkscs
    {0xC6C9, 0x53B6}, //9792 #big5-hkscs
    {0xC6CA, 0x590A}, //9793 #big5-hkscs
    {0xC6CB, 0x5B80}, //9794 #big5-hkscs
    {0xC6CC, 0x5DDB}, //9795 #big5-hkscs
    {0xC6CD, 0x2F33}, //9796 #big5-hkscs
    {0xC6CE, 0x5E7F}, //9797 #big5-hkscs
    {0xC6CF, 0x5EF4}, //9798 #big5-hkscs
    {0xC6D0, 0x5F50}, //9799 #big5-hkscs
    {0xC6D1, 0x5F61}, //9800 #big5-hkscs
    {0xC6D2, 0x6534}, //9801 #big5-hkscs
    {0xC6D3, 0x65E0}, //9802 #big5-hkscs
    {0xC6D4, 0x7592}, //9803 #big5-hkscs
    {0xC6D5, 0x7676}, //9804 #big5-hkscs
    {0xC6D6, 0x8FB5}, //9805 #big5-hkscs
    {0xC6D7, 0x96B6}, //9806 #big5-hkscs
    {0xC6D8, 0x00A8}, //9807 #big5-hkscs
    {0xC6D9, 0x02C6}, //9808 #big5-hkscs
    {0xC6DA, 0x30FD}, //9809 #big5-hkscs
    {0xC6DB, 0x30FE}, //9810 #big5-hkscs
    {0xC6DC, 0x309D}, //9811 #big5-hkscs
    {0xC6DD, 0x309E}, //9812 #big5-hkscs
    {0xC6DE, 0x3003}, //9813 #big5-hkscs
    {0xC6DF, 0x4EDD}, //9814 #big5-hkscs
    {0xC6E0, 0x3005}, //9815 #big5-hkscs
    {0xC6E1, 0x3006}, //9816 #big5-hkscs
    {0xC6E2, 0x3007}, //9817 #big5-hkscs
    {0xC6E3, 0x30FC}, //9818 #big5-hkscs
    {0xC6E4, 0xFF3B}, //9819 #big5-hkscs
    {0xC6E5, 0xFF3D}, //9820 #big5-hkscs
    {0xC6E6, 0x273D}, //9821 #big5-hkscs
    {0xC6E7, 0x3041}, //9822 #big5-hkscs
    {0xC6E8, 0x3042}, //9823 #big5-hkscs
    {0xC6E9, 0x3043}, //9824 #big5-hkscs
    {0xC6EA, 0x3044}, //9825 #big5-hkscs
    {0xC6EB, 0x3045}, //9826 #big5-hkscs
    {0xC6EC, 0x3046}, //9827 #big5-hkscs
    {0xC6ED, 0x3047}, //9828 #big5-hkscs
    {0xC6EE, 0x3048}, //9829 #big5-hkscs
    {0xC6EF, 0x3049}, //9830 #big5-hkscs
    {0xC6F0, 0x304A}, //9831 #big5-hkscs
    {0xC6F1, 0x304B}, //9832 #big5-hkscs
    {0xC6F2, 0x304C}, //9833 #big5-hkscs
    {0xC6F3, 0x304D}, //9834 #big5-hkscs
    {0xC6F4, 0x304E}, //9835 #big5-hkscs
    {0xC6F5, 0x304F}, //9836 #big5-hkscs
    {0xC6F6, 0x3050}, //9837 #big5-hkscs
    {0xC6F7, 0x3051}, //9838 #big5-hkscs
    {0xC6F8, 0x3052}, //9839 #big5-hkscs
    {0xC6F9, 0x3053}, //9840 #big5-hkscs
    {0xC6FA, 0x3054}, //9841 #big5-hkscs
    {0xC6FB, 0x3055}, //9842 #big5-hkscs
    {0xC6FC, 0x3056}, //9843 #big5-hkscs
    {0xC6FD, 0x3057}, //9844 #big5-hkscs
    {0xC6FE, 0x3058}, //9845 #big5-hkscs
    {0xC740, 0x3059}, //9846 #big5-hkscs
    {0xC741, 0x305A}, //9847 #big5-hkscs
    {0xC742, 0x305B}, //9848 #big5-hkscs
    {0xC743, 0x305C}, //9849 #big5-hkscs
    {0xC744, 0x305D}, //9850 #big5-hkscs
    {0xC745, 0x305E}, //9851 #big5-hkscs
    {0xC746, 0x305F}, //9852 #big5-hkscs
    {0xC747, 0x3060}, //9853 #big5-hkscs
    {0xC748, 0x3061}, //9854 #big5-hkscs
    {0xC749, 0x3062}, //9855 #big5-hkscs
    {0xC74A, 0x3063}, //9856 #big5-hkscs
    {0xC74B, 0x3064}, //9857 #big5-hkscs
    {0xC74C, 0x3065}, //9858 #big5-hkscs
    {0xC74D, 0x3066}, //9859 #big5-hkscs
    {0xC74E, 0x3067}, //9860 #big5-hkscs
    {0xC74F, 0x3068}, //9861 #big5-hkscs
    {0xC750, 0x3069}, //9862 #big5-hkscs
    {0xC751, 0x306A}, //9863 #big5-hkscs
    {0xC752, 0x306B}, //9864 #big5-hkscs
    {0xC753, 0x306C}, //9865 #big5-hkscs
    {0xC754, 0x306D}, //9866 #big5-hkscs
    {0xC755, 0x306E}, //9867 #big5-hkscs
    {0xC756, 0x306F}, //9868 #big5-hkscs
    {0xC757, 0x3070}, //9869 #big5-hkscs
    {0xC758, 0x3071}, //9870 #big5-hkscs
    {0xC759, 0x3072}, //9871 #big5-hkscs
    {0xC75A, 0x3073}, //9872 #big5-hkscs
    {0xC75B, 0x3074}, //9873 #big5-hkscs
    {0xC75C, 0x3075}, //9874 #big5-hkscs
    {0xC75D, 0x3076}, //9875 #big5-hkscs
    {0xC75E, 0x3077}, //9876 #big5-hkscs
    {0xC75F, 0x3078}, //9877 #big5-hkscs
    {0xC760, 0x3079}, //9878 #big5-hkscs
    {0xC761, 0x307A}, //9879 #big5-hkscs
    {0xC762, 0x307B}, //9880 #big5-hkscs
    {0xC763, 0x307C}, //9881 #big5-hkscs
    {0xC764, 0x307D}, //9882 #big5-hkscs
    {0xC765, 0x307E}, //9883 #big5-hkscs
    {0xC766, 0x307F}, //9884 #big5-hkscs
    {0xC767, 0x3080}, //9885 #big5-hkscs
    {0xC768, 0x3081}, //9886 #big5-hkscs
    {0xC769, 0x3082}, //9887 #big5-hkscs
    {0xC76A, 0x3083}, //9888 #big5-hkscs
    {0xC76B, 0x3084}, //9889 #big5-hkscs
    {0xC76C, 0x3085}, //9890 #big5-hkscs
    {0xC76D, 0x3086}, //9891 #big5-hkscs
    {0xC76E, 0x3087}, //9892 #big5-hkscs
    {0xC76F, 0x3088}, //9893 #big5-hkscs
    {0xC770, 0x3089}, //9894 #big5-hkscs
    {0xC771, 0x308A}, //9895 #big5-hkscs
    {0xC772, 0x308B}, //9896 #big5-hkscs
    {0xC773, 0x308C}, //9897 #big5-hkscs
    {0xC774, 0x308D}, //9898 #big5-hkscs
    {0xC775, 0x308E}, //9899 #big5-hkscs
    {0xC776, 0x308F}, //9900 #big5-hkscs
    {0xC777, 0x3090}, //9901 #big5-hkscs
    {0xC778, 0x3091}, //9902 #big5-hkscs
    {0xC779, 0x3092}, //9903 #big5-hkscs
    {0xC77A, 0x3093}, //9904 #big5-hkscs
    {0xC77B, 0x30A1}, //9905 #big5-hkscs
    {0xC77C, 0x30A2}, //9906 #big5-hkscs
    {0xC77D, 0x30A3}, //9907 #big5-hkscs
    {0xC77E, 0x30A4}, //9908 #big5-hkscs
    {0xC7A1, 0x30A5}, //9909 #big5-hkscs
    {0xC7A2, 0x30A6}, //9910 #big5-hkscs
    {0xC7A3, 0x30A7}, //9911 #big5-hkscs
    {0xC7A4, 0x30A8}, //9912 #big5-hkscs
    {0xC7A5, 0x30A9}, //9913 #big5-hkscs
    {0xC7A6, 0x30AA}, //9914 #big5-hkscs
    {0xC7A7, 0x30AB}, //9915 #big5-hkscs
    {0xC7A8, 0x30AC}, //9916 #big5-hkscs
    {0xC7A9, 0x30AD}, //9917 #big5-hkscs
    {0xC7AA, 0x30AE}, //9918 #big5-hkscs
    {0xC7AB, 0x30AF}, //9919 #big5-hkscs
    {0xC7AC, 0x30B0}, //9920 #big5-hkscs
    {0xC7AD, 0x30B1}, //9921 #big5-hkscs
    {0xC7AE, 0x30B2}, //9922 #big5-hkscs
    {0xC7AF, 0x30B3}, //9923 #big5-hkscs
    {0xC7B0, 0x30B4}, //9924 #big5-hkscs
    {0xC7B1, 0x30B5}, //9925 #big5-hkscs
    {0xC7B2, 0x30B6}, //9926 #big5-hkscs
    {0xC7B3, 0x30B7}, //9927 #big5-hkscs
    {0xC7B4, 0x30B8}, //9928 #big5-hkscs
    {0xC7B5, 0x30B9}, //9929 #big5-hkscs
    {0xC7B6, 0x30BA}, //9930 #big5-hkscs
    {0xC7B7, 0x30BB}, //9931 #big5-hkscs
    {0xC7B8, 0x30BC}, //9932 #big5-hkscs
    {0xC7B9, 0x30BD}, //9933 #big5-hkscs
    {0xC7BA, 0x30BE}, //9934 #big5-hkscs
    {0xC7BB, 0x30BF}, //9935 #big5-hkscs
    {0xC7BC, 0x30C0}, //9936 #big5-hkscs
    {0xC7BD, 0x30C1}, //9937 #big5-hkscs
    {0xC7BE, 0x30C2}, //9938 #big5-hkscs
    {0xC7BF, 0x30C3}, //9939 #big5-hkscs
    {0xC7C0, 0x30C4}, //9940 #big5-hkscs
    {0xC7C1, 0x30C5}, //9941 #big5-hkscs
    {0xC7C2, 0x30C6}, //9942 #big5-hkscs
    {0xC7C3, 0x30C7}, //9943 #big5-hkscs
    {0xC7C4, 0x30C8}, //9944 #big5-hkscs
    {0xC7C5, 0x30C9}, //9945 #big5-hkscs
    {0xC7C6, 0x30CA}, //9946 #big5-hkscs
    {0xC7C7, 0x30CB}, //9947 #big5-hkscs
    {0xC7C8, 0x30CC}, //9948 #big5-hkscs
    {0xC7C9, 0x30CD}, //9949 #big5-hkscs
    {0xC7CA, 0x30CE}, //9950 #big5-hkscs
    {0xC7CB, 0x30CF}, //9951 #big5-hkscs
    {0xC7CC, 0x30D0}, //9952 #big5-hkscs
    {0xC7CD, 0x30D1}, //9953 #big5-hkscs
    {0xC7CE, 0x30D2}, //9954 #big5-hkscs
    {0xC7CF, 0x30D3}, //9955 #big5-hkscs
    {0xC7D0, 0x30D4}, //9956 #big5-hkscs
    {0xC7D1, 0x30D5}, //9957 #big5-hkscs
    {0xC7D2, 0x30D6}, //9958 #big5-hkscs
    {0xC7D3, 0x30D7}, //9959 #big5-hkscs
    {0xC7D4, 0x30D8}, //9960 #big5-hkscs
    {0xC7D5, 0x30D9}, //9961 #big5-hkscs
    {0xC7D6, 0x30DA}, //9962 #big5-hkscs
    {0xC7D7, 0x30DB}, //9963 #big5-hkscs
    {0xC7D8, 0x30DC}, //9964 #big5-hkscs
    {0xC7D9, 0x30DD}, //9965 #big5-hkscs
    {0xC7DA, 0x30DE}, //9966 #big5-hkscs
    {0xC7DB, 0x30DF}, //9967 #big5-hkscs
    {0xC7DC, 0x30E0}, //9968 #big5-hkscs
    {0xC7DD, 0x30E1}, //9969 #big5-hkscs
    {0xC7DE, 0x30E2}, //9970 #big5-hkscs
    {0xC7DF, 0x30E3}, //9971 #big5-hkscs
    {0xC7E0, 0x30E4}, //9972 #big5-hkscs
    {0xC7E1, 0x30E5}, //9973 #big5-hkscs
    {0xC7E2, 0x30E6}, //9974 #big5-hkscs
    {0xC7E3, 0x30E7}, //9975 #big5-hkscs
    {0xC7E4, 0x30E8}, //9976 #big5-hkscs
    {0xC7E5, 0x30E9}, //9977 #big5-hkscs
    {0xC7E6, 0x30EA}, //9978 #big5-hkscs
    {0xC7E7, 0x30EB}, //9979 #big5-hkscs
    {0xC7E8, 0x30EC}, //9980 #big5-hkscs
    {0xC7E9, 0x30ED}, //9981 #big5-hkscs
    {0xC7EA, 0x30EE}, //9982 #big5-hkscs
    {0xC7EB, 0x30EF}, //9983 #big5-hkscs
    {0xC7EC, 0x30F0}, //9984 #big5-hkscs
    {0xC7ED, 0x30F1}, //9985 #big5-hkscs
    {0xC7EE, 0x30F2}, //9986 #big5-hkscs
    {0xC7EF, 0x30F3}, //9987 #big5-hkscs
    {0xC7F0, 0x30F4}, //9988 #big5-hkscs
    {0xC7F1, 0x30F5}, //9989 #big5-hkscs
    {0xC7F2, 0x30F6}, //9990 #big5-hkscs
    {0xC7F3, 0x0410}, //9991 #big5-hkscs
    {0xC7F4, 0x0411}, //9992 #big5-hkscs
    {0xC7F5, 0x0412}, //9993 #big5-hkscs
    {0xC7F6, 0x0413}, //9994 #big5-hkscs
    {0xC7F7, 0x0414}, //9995 #big5-hkscs
    {0xC7F8, 0x0415}, //9996 #big5-hkscs
    {0xC7F9, 0x0401}, //9997 #big5-hkscs
    {0xC7FA, 0x0416}, //9998 #big5-hkscs
    {0xC7FB, 0x0417}, //9999 #big5-hkscs
    {0xC7FC, 0x0418}, //10000 #big5-hkscs
    {0xC7FD, 0x0419}, //10001 #big5-hkscs
    {0xC7FE, 0x041A}, //10002 #big5-hkscs
    {0xC840, 0x041B}, //10003 #big5-hkscs
    {0xC841, 0x041C}, //10004 #big5-hkscs
    {0xC842, 0x041D}, //10005 #big5-hkscs
    {0xC843, 0x041E}, //10006 #big5-hkscs
    {0xC844, 0x041F}, //10007 #big5-hkscs
    {0xC845, 0x0420}, //10008 #big5-hkscs
    {0xC846, 0x0421}, //10009 #big5-hkscs
    {0xC847, 0x0422}, //10010 #big5-hkscs
    {0xC848, 0x0423}, //10011 #big5-hkscs
    {0xC849, 0x0424}, //10012 #big5-hkscs
    {0xC84A, 0x0425}, //10013 #big5-hkscs
    {0xC84B, 0x0426}, //10014 #big5-hkscs
    {0xC84C, 0x0427}, //10015 #big5-hkscs
    {0xC84D, 0x0428}, //10016 #big5-hkscs
    {0xC84E, 0x0429}, //10017 #big5-hkscs
    {0xC84F, 0x042A}, //10018 #big5-hkscs
    {0xC850, 0x042B}, //10019 #big5-hkscs
    {0xC851, 0x042C}, //10020 #big5-hkscs
    {0xC852, 0x042D}, //10021 #big5-hkscs
    {0xC853, 0x042E}, //10022 #big5-hkscs
    {0xC854, 0x042F}, //10023 #big5-hkscs
    {0xC855, 0x0430}, //10024 #big5-hkscs
    {0xC856, 0x0431}, //10025 #big5-hkscs
    {0xC857, 0x0432}, //10026 #big5-hkscs
    {0xC858, 0x0433}, //10027 #big5-hkscs
    {0xC859, 0x0434}, //10028 #big5-hkscs
    {0xC85A, 0x0435}, //10029 #big5-hkscs
    {0xC85B, 0x0451}, //10030 #big5-hkscs
    {0xC85C, 0x0436}, //10031 #big5-hkscs
    {0xC85D, 0x0437}, //10032 #big5-hkscs
    {0xC85E, 0x0438}, //10033 #big5-hkscs
    {0xC85F, 0x0439}, //10034 #big5-hkscs
    {0xC860, 0x043A}, //10035 #big5-hkscs
    {0xC861, 0x043B}, //10036 #big5-hkscs
    {0xC862, 0x043C}, //10037 #big5-hkscs
    {0xC863, 0x043D}, //10038 #big5-hkscs
    {0xC864, 0x043E}, //10039 #big5-hkscs
    {0xC865, 0x043F}, //10040 #big5-hkscs
    {0xC866, 0x0440}, //10041 #big5-hkscs
    {0xC867, 0x0441}, //10042 #big5-hkscs
    {0xC868, 0x0442}, //10043 #big5-hkscs
    {0xC869, 0x0443}, //10044 #big5-hkscs
    {0xC86A, 0x0444}, //10045 #big5-hkscs
    {0xC86B, 0x0445}, //10046 #big5-hkscs
    {0xC86C, 0x0446}, //10047 #big5-hkscs
    {0xC86D, 0x0447}, //10048 #big5-hkscs
    {0xC86E, 0x0448}, //10049 #big5-hkscs
    {0xC86F, 0x0449}, //10050 #big5-hkscs
    {0xC870, 0x044A}, //10051 #big5-hkscs
    {0xC871, 0x044B}, //10052 #big5-hkscs
    {0xC872, 0x044C}, //10053 #big5-hkscs
    {0xC873, 0x044D}, //10054 #big5-hkscs
    {0xC874, 0x044E}, //10055 #big5-hkscs
    {0xC875, 0x044F}, //10056 #big5-hkscs
    {0xC876, 0x21E7}, //10057 #big5-hkscs
    {0xC877, 0x21B8}, //10058 #big5-hkscs
    {0xC878, 0x21B9}, //10059 #big5-hkscs
    {0xC879, 0x31CF}, //10060 #big5-hkscs
    {0xC87A, 0x200CC}, //10061 #big5-hkscs
    {0xC87B, 0x4E5A}, //10062 #big5-hkscs
    {0xC87C, 0x2008A}, //10063 #big5-hkscs
    {0xC87D, 0x5202}, //10064 #big5-hkscs
    {0xC87E, 0x4491}, //10065 #big5-hkscs
    {0xC8A1, 0x9FB0}, //10066 #big5-hkscs
    {0xC8A2, 0x5188}, //10067 #big5-hkscs
    {0xC8A3, 0x9FB1}, //10068 #big5-hkscs
    {0xC8A4, 0x27607}, //10069 #big5-hkscs
    {0xC8CD, 0xFFE2}, //10070 #big5-hkscs
    {0xC8CE, 0xFFE4}, //10071 #big5-hkscs
    {0xC8CF, 0xFF07}, //10072 #big5-hkscs
    {0xC8D0, 0xFF02}, //10073 #big5-hkscs
    {0xC8D1, 0x3231}, //10074 #big5-hkscs
    {0xC8D2, 0x2116}, //10075 #big5-hkscs
    {0xC8D3, 0x2121}, //10076 #big5-hkscs
    {0xC8D4, 0x309B}, //10077 #big5-hkscs
    {0xC8D5, 0x309C}, //10078 #big5-hkscs
    {0xC8D6, 0x2E80}, //10079 #big5-hkscs
    {0xC8D7, 0x2E84}, //10080 #big5-hkscs
    {0xC8D8, 0x2E86}, //10081 #big5-hkscs
    {0xC8D9, 0x2E87}, //10082 #big5-hkscs
    {0xC8DA, 0x2E88}, //10083 #big5-hkscs
    {0xC8DB, 0x2E8A}, //10084 #big5-hkscs
    {0xC8DC, 0x2E8C}, //10085 #big5-hkscs
    {0xC8DD, 0x2E8D}, //10086 #big5-hkscs
    {0xC8DE, 0x2E95}, //10087 #big5-hkscs
    {0xC8DF, 0x2E9C}, //10088 #big5-hkscs
    {0xC8E0, 0x2E9D}, //10089 #big5-hkscs
    {0xC8E1, 0x2EA5}, //10090 #big5-hkscs
    {0xC8E2, 0x2EA7}, //10091 #big5-hkscs
    {0xC8E3, 0x2EAA}, //10092 #big5-hkscs
    {0xC8E4, 0x2EAC}, //10093 #big5-hkscs
    {0xC8E5, 0x2EAE}, //10094 #big5-hkscs
    {0xC8E6, 0x2EB6}, //10095 #big5-hkscs
    {0xC8E7, 0x2EBC}, //10096 #big5-hkscs
    {0xC8E8, 0x2EBE}, //10097 #big5-hkscs
    {0xC8E9, 0x2EC6}, //10098 #big5-hkscs
    {0xC8EA, 0x2ECA}, //10099 #big5-hkscs
    {0xC8EB, 0x2ECC}, //10100 #big5-hkscs
    {0xC8EC, 0x2ECD}, //10101 #big5-hkscs
    {0xC8ED, 0x2ECF}, //10102 #big5-hkscs
    {0xC8EE, 0x2ED6}, //10103 #big5-hkscs
    {0xC8EF, 0x2ED7}, //10104 #big5-hkscs
    {0xC8F0, 0x2EDE}, //10105 #big5-hkscs
    {0xC8F1, 0x2EE3}, //10106 #big5-hkscs
    {0xC8F5, 0x0283}, //10107 #big5-hkscs
    {0xC8F6, 0x0250}, //10108 #big5-hkscs
    {0xC8F7, 0x025B}, //10109 #big5-hkscs
    {0xC8F8, 0x0254}, //10110 #big5-hkscs
    {0xC8F9, 0x0275}, //10111 #big5-hkscs
    {0xC8FA, 0x0153}, //10112 #big5-hkscs
    {0xC8FB, 0x00F8}, //10113 #big5-hkscs
    {0xC8FC, 0x014B}, //10114 #big5-hkscs
    {0xC8FD, 0x028A}, //10115 #big5-hkscs
    {0xC8FE, 0x026A}, //10116 #big5-hkscs
    {0xC940, 0x4E42}, //10117 #CJK UNIFIED IDEOGRAPH
    {0xC941, 0x4E5C}, //10118 #CJK UNIFIED IDEOGRAPH
    {0xC942, 0x51F5}, //10119 #CJK UNIFIED IDEOGRAPH
    {0xC943, 0x531A}, //10120 #CJK UNIFIED IDEOGRAPH
    {0xC944, 0x5382}, //10121 #CJK UNIFIED IDEOGRAPH
    {0xC945, 0x4E07}, //10122 #CJK UNIFIED IDEOGRAPH
    {0xC946, 0x4E0C}, //10123 #CJK UNIFIED IDEOGRAPH
    {0xC947, 0x4E47}, //10124 #CJK UNIFIED IDEOGRAPH
    {0xC948, 0x4E8D}, //10125 #CJK UNIFIED IDEOGRAPH
    {0xC949, 0x56D7}, //10126 #CJK UNIFIED IDEOGRAPH
    {0xC94A, 0xFA0C}, //10127 #CJK COMPATIBILITY IDEOGRAPH
    {0xC94B, 0x5C6E}, //10128 #CJK UNIFIED IDEOGRAPH
    {0xC94C, 0x5F73}, //10129 #CJK UNIFIED IDEOGRAPH
    {0xC94D, 0x4E0F}, //10130 #CJK UNIFIED IDEOGRAPH
    {0xC94E, 0x5187}, //10131 #CJK UNIFIED IDEOGRAPH
    {0xC94F, 0x4E0E}, //10132 #CJK UNIFIED IDEOGRAPH
    {0xC950, 0x4E2E}, //10133 #CJK UNIFIED IDEOGRAPH
    {0xC951, 0x4E93}, //10134 #CJK UNIFIED IDEOGRAPH
    {0xC952, 0x4EC2}, //10135 #CJK UNIFIED IDEOGRAPH
    {0xC953, 0x4EC9}, //10136 #CJK UNIFIED IDEOGRAPH
    {0xC954, 0x4EC8}, //10137 #CJK UNIFIED IDEOGRAPH
    {0xC955, 0x5198}, //10138 #CJK UNIFIED IDEOGRAPH
    {0xC956, 0x52FC}, //10139 #CJK UNIFIED IDEOGRAPH
    {0xC957, 0x536C}, //10140 #CJK UNIFIED IDEOGRAPH
    {0xC958, 0x53B9}, //10141 #CJK UNIFIED IDEOGRAPH
    {0xC959, 0x5720}, //10142 #CJK UNIFIED IDEOGRAPH
    {0xC95A, 0x5903}, //10143 #CJK UNIFIED IDEOGRAPH
    {0xC95B, 0x592C}, //10144 #CJK UNIFIED IDEOGRAPH
    {0xC95C, 0x5C10}, //10145 #CJK UNIFIED IDEOGRAPH
    {0xC95D, 0x5DFF}, //10146 #CJK UNIFIED IDEOGRAPH
    {0xC95E, 0x65E1}, //10147 #CJK UNIFIED IDEOGRAPH
    {0xC95F, 0x6BB3}, //10148 #CJK UNIFIED IDEOGRAPH
    {0xC960, 0x6BCC}, //10149 #CJK UNIFIED IDEOGRAPH
    {0xC961, 0x6C14}, //10150 #CJK UNIFIED IDEOGRAPH
    {0xC962, 0x723F}, //10151 #CJK UNIFIED IDEOGRAPH
    {0xC963, 0x4E31}, //10152 #CJK UNIFIED IDEOGRAPH
    {0xC964, 0x4E3C}, //10153 #CJK UNIFIED IDEOGRAPH
    {0xC965, 0x4EE8}, //10154 #CJK UNIFIED IDEOGRAPH
    {0xC966, 0x4EDC}, //10155 #CJK UNIFIED IDEOGRAPH
    {0xC967, 0x4EE9}, //10156 #CJK UNIFIED IDEOGRAPH
    {0xC968, 0x4EE1}, //10157 #CJK UNIFIED IDEOGRAPH
    {0xC969, 0x4EDD}, //10158 #CJK UNIFIED IDEOGRAPH
    {0xC96A, 0x4EDA}, //10159 #CJK UNIFIED IDEOGRAPH
    {0xC96B, 0x520C}, //10160 #CJK UNIFIED IDEOGRAPH
    {0xC96C, 0x531C}, //10161 #CJK UNIFIED IDEOGRAPH
    {0xC96D, 0x534C}, //10162 #CJK UNIFIED IDEOGRAPH
    {0xC96E, 0x5722}, //10163 #CJK UNIFIED IDEOGRAPH
    {0xC96F, 0x5723}, //10164 #CJK UNIFIED IDEOGRAPH
    {0xC970, 0x5917}, //10165 #CJK UNIFIED IDEOGRAPH
    {0xC971, 0x592F}, //10166 #CJK UNIFIED IDEOGRAPH
    {0xC972, 0x5B81}, //10167 #CJK UNIFIED IDEOGRAPH
    {0xC973, 0x5B84}, //10168 #CJK UNIFIED IDEOGRAPH
    {0xC974, 0x5C12}, //10169 #CJK UNIFIED IDEOGRAPH
    {0xC975, 0x5C3B}, //10170 #CJK UNIFIED IDEOGRAPH
    {0xC976, 0x5C74}, //10171 #CJK UNIFIED IDEOGRAPH
    {0xC977, 0x5C73}, //10172 #CJK UNIFIED IDEOGRAPH
    {0xC978, 0x5E04}, //10173 #CJK UNIFIED IDEOGRAPH
    {0xC979, 0x5E80}, //10174 #CJK UNIFIED IDEOGRAPH
    {0xC97A, 0x5E82}, //10175 #CJK UNIFIED IDEOGRAPH
    {0xC97B, 0x5FC9}, //10176 #CJK UNIFIED IDEOGRAPH
    {0xC97C, 0x6209}, //10177 #CJK UNIFIED IDEOGRAPH
    {0xC97D, 0x6250}, //10178 #CJK UNIFIED IDEOGRAPH
    {0xC97E, 0x6C15}, //10179 #CJK UNIFIED IDEOGRAPH
    {0xC9A1, 0x6C36}, //10180 #CJK UNIFIED IDEOGRAPH
    {0xC9A2, 0x6C43}, //10181 #CJK UNIFIED IDEOGRAPH
    {0xC9A3, 0x6C3F}, //10182 #CJK UNIFIED IDEOGRAPH
    {0xC9A4, 0x6C3B}, //10183 #CJK UNIFIED IDEOGRAPH
    {0xC9A5, 0x72AE}, //10184 #CJK UNIFIED IDEOGRAPH
    {0xC9A6, 0x72B0}, //10185 #CJK UNIFIED IDEOGRAPH
    {0xC9A7, 0x738A}, //10186 #CJK UNIFIED IDEOGRAPH
    {0xC9A8, 0x79B8}, //10187 #CJK UNIFIED IDEOGRAPH
    {0xC9A9, 0x808A}, //10188 #CJK UNIFIED IDEOGRAPH
    {0xC9AA, 0x961E}, //10189 #CJK UNIFIED IDEOGRAPH
    {0xC9AB, 0x4F0E}, //10190 #CJK UNIFIED IDEOGRAPH
    {0xC9AC, 0x4F18}, //10191 #CJK UNIFIED IDEOGRAPH
    {0xC9AD, 0x4F2C}, //10192 #CJK UNIFIED IDEOGRAPH
    {0xC9AE, 0x4EF5}, //10193 #CJK UNIFIED IDEOGRAPH
    {0xC9AF, 0x4F14}, //10194 #CJK UNIFIED IDEOGRAPH
    {0xC9B0, 0x4EF1}, //10195 #CJK UNIFIED IDEOGRAPH
    {0xC9B1, 0x4F00}, //10196 #CJK UNIFIED IDEOGRAPH
    {0xC9B2, 0x4EF7}, //10197 #CJK UNIFIED IDEOGRAPH
    {0xC9B3, 0x4F08}, //10198 #CJK UNIFIED IDEOGRAPH
    {0xC9B4, 0x4F1D}, //10199 #CJK UNIFIED IDEOGRAPH
    {0xC9B5, 0x4F02}, //10200 #CJK UNIFIED IDEOGRAPH
    {0xC9B6, 0x4F05}, //10201 #CJK UNIFIED IDEOGRAPH
    {0xC9B7, 0x4F22}, //10202 #CJK UNIFIED IDEOGRAPH
    {0xC9B8, 0x4F13}, //10203 #CJK UNIFIED IDEOGRAPH
    {0xC9B9, 0x4F04}, //10204 #CJK UNIFIED IDEOGRAPH
    {0xC9BA, 0x4EF4}, //10205 #CJK UNIFIED IDEOGRAPH
    {0xC9BB, 0x4F12}, //10206 #CJK UNIFIED IDEOGRAPH
    {0xC9BC, 0x51B1}, //10207 #CJK UNIFIED IDEOGRAPH
    {0xC9BD, 0x5213}, //10208 #CJK UNIFIED IDEOGRAPH
    {0xC9BE, 0x5209}, //10209 #CJK UNIFIED IDEOGRAPH
    {0xC9BF, 0x5210}, //10210 #CJK UNIFIED IDEOGRAPH
    {0xC9C0, 0x52A6}, //10211 #CJK UNIFIED IDEOGRAPH
    {0xC9C1, 0x5322}, //10212 #CJK UNIFIED IDEOGRAPH
    {0xC9C2, 0x531F}, //10213 #CJK UNIFIED IDEOGRAPH
    {0xC9C3, 0x534D}, //10214 #CJK UNIFIED IDEOGRAPH
    {0xC9C4, 0x538A}, //10215 #CJK UNIFIED IDEOGRAPH
    {0xC9C5, 0x5407}, //10216 #CJK UNIFIED IDEOGRAPH
    {0xC9C6, 0x56E1}, //10217 #CJK UNIFIED IDEOGRAPH
    {0xC9C7, 0x56DF}, //10218 #CJK UNIFIED IDEOGRAPH
    {0xC9C8, 0x572E}, //10219 #CJK UNIFIED IDEOGRAPH
    {0xC9C9, 0x572A}, //10220 #CJK UNIFIED IDEOGRAPH
    {0xC9CA, 0x5734}, //10221 #CJK UNIFIED IDEOGRAPH
    {0xC9CB, 0x593C}, //10222 #CJK UNIFIED IDEOGRAPH
    {0xC9CC, 0x5980}, //10223 #CJK UNIFIED IDEOGRAPH
    {0xC9CD, 0x597C}, //10224 #CJK UNIFIED IDEOGRAPH
    {0xC9CE, 0x5985}, //10225 #CJK UNIFIED IDEOGRAPH
    {0xC9CF, 0x597B}, //10226 #CJK UNIFIED IDEOGRAPH
    {0xC9D0, 0x597E}, //10227 #CJK UNIFIED IDEOGRAPH
    {0xC9D1, 0x5977}, //10228 #CJK UNIFIED IDEOGRAPH
    {0xC9D2, 0x597F}, //10229 #CJK UNIFIED IDEOGRAPH
    {0xC9D3, 0x5B56}, //10230 #CJK UNIFIED IDEOGRAPH
    {0xC9D4, 0x5C15}, //10231 #CJK UNIFIED IDEOGRAPH
    {0xC9D5, 0x5C25}, //10232 #CJK UNIFIED IDEOGRAPH
    {0xC9D6, 0x5C7C}, //10233 #CJK UNIFIED IDEOGRAPH
    {0xC9D7, 0x5C7A}, //10234 #CJK UNIFIED IDEOGRAPH
    {0xC9D8, 0x5C7B}, //10235 #CJK UNIFIED IDEOGRAPH
    {0xC9D9, 0x5C7E}, //10236 #CJK UNIFIED IDEOGRAPH
    {0xC9DA, 0x5DDF}, //10237 #CJK UNIFIED IDEOGRAPH
    {0xC9DB, 0x5E75}, //10238 #CJK UNIFIED IDEOGRAPH
    {0xC9DC, 0x5E84}, //10239 #CJK UNIFIED IDEOGRAPH
    {0xC9DD, 0x5F02}, //10240 #CJK UNIFIED IDEOGRAPH
    {0xC9DE, 0x5F1A}, //10241 #CJK UNIFIED IDEOGRAPH
    {0xC9DF, 0x5F74}, //10242 #CJK UNIFIED IDEOGRAPH
    {0xC9E0, 0x5FD5}, //10243 #CJK UNIFIED IDEOGRAPH
    {0xC9E1, 0x5FD4}, //10244 #CJK UNIFIED IDEOGRAPH
    {0xC9E2, 0x5FCF}, //10245 #CJK UNIFIED IDEOGRAPH
    {0xC9E3, 0x625C}, //10246 #CJK UNIFIED IDEOGRAPH
    {0xC9E4, 0x625E}, //10247 #CJK UNIFIED IDEOGRAPH
    {0xC9E5, 0x6264}, //10248 #CJK UNIFIED IDEOGRAPH
    {0xC9E6, 0x6261}, //10249 #CJK UNIFIED IDEOGRAPH
    {0xC9E7, 0x6266}, //10250 #CJK UNIFIED IDEOGRAPH
    {0xC9E8, 0x6262}, //10251 #CJK UNIFIED IDEOGRAPH
    {0xC9E9, 0x6259}, //10252 #CJK UNIFIED IDEOGRAPH
    {0xC9EA, 0x6260}, //10253 #CJK UNIFIED IDEOGRAPH
    {0xC9EB, 0x625A}, //10254 #CJK UNIFIED IDEOGRAPH
    {0xC9EC, 0x6265}, //10255 #CJK UNIFIED IDEOGRAPH
    {0xC9ED, 0x65EF}, //10256 #CJK UNIFIED IDEOGRAPH
    {0xC9EE, 0x65EE}, //10257 #CJK UNIFIED IDEOGRAPH
    {0xC9EF, 0x673E}, //10258 #CJK UNIFIED IDEOGRAPH
    {0xC9F0, 0x6739}, //10259 #CJK UNIFIED IDEOGRAPH
    {0xC9F1, 0x6738}, //10260 #CJK UNIFIED IDEOGRAPH
    {0xC9F2, 0x673B}, //10261 #CJK UNIFIED IDEOGRAPH
    {0xC9F3, 0x673A}, //10262 #CJK UNIFIED IDEOGRAPH
    {0xC9F4, 0x673F}, //10263 #CJK UNIFIED IDEOGRAPH
    {0xC9F5, 0x673C}, //10264 #CJK UNIFIED IDEOGRAPH
    {0xC9F6, 0x6733}, //10265 #CJK UNIFIED IDEOGRAPH
    {0xC9F7, 0x6C18}, //10266 #CJK UNIFIED IDEOGRAPH
    {0xC9F8, 0x6C46}, //10267 #CJK UNIFIED IDEOGRAPH
    {0xC9F9, 0x6C52}, //10268 #CJK UNIFIED IDEOGRAPH
    {0xC9FA, 0x6C5C}, //10269 #CJK UNIFIED IDEOGRAPH
    {0xC9FB, 0x6C4F}, //10270 #CJK UNIFIED IDEOGRAPH
    {0xC9FC, 0x6C4A}, //10271 #CJK UNIFIED IDEOGRAPH
    {0xC9FD, 0x6C54}, //10272 #CJK UNIFIED IDEOGRAPH
    {0xC9FE, 0x6C4B}, //10273 #CJK UNIFIED IDEOGRAPH
    {0xCA40, 0x6C4C}, //10274 #CJK UNIFIED IDEOGRAPH
    {0xCA41, 0x7071}, //10275 #CJK UNIFIED IDEOGRAPH
    {0xCA42, 0x725E}, //10276 #CJK UNIFIED IDEOGRAPH
    {0xCA43, 0x72B4}, //10277 #CJK UNIFIED IDEOGRAPH
    {0xCA44, 0x72B5}, //10278 #CJK UNIFIED IDEOGRAPH
    {0xCA45, 0x738E}, //10279 #CJK UNIFIED IDEOGRAPH
    {0xCA46, 0x752A}, //10280 #CJK UNIFIED IDEOGRAPH
    {0xCA47, 0x767F}, //10281 #CJK UNIFIED IDEOGRAPH
    {0xCA48, 0x7A75}, //10282 #CJK UNIFIED IDEOGRAPH
    {0xCA49, 0x7F51}, //10283 #CJK UNIFIED IDEOGRAPH
    {0xCA4A, 0x8278}, //10284 #CJK UNIFIED IDEOGRAPH
    {0xCA4B, 0x827C}, //10285 #CJK UNIFIED IDEOGRAPH
    {0xCA4C, 0x8280}, //10286 #CJK UNIFIED IDEOGRAPH
    {0xCA4D, 0x827D}, //10287 #CJK UNIFIED IDEOGRAPH
    {0xCA4E, 0x827F}, //10288 #CJK UNIFIED IDEOGRAPH
    {0xCA4F, 0x864D}, //10289 #CJK UNIFIED IDEOGRAPH
    {0xCA50, 0x897E}, //10290 #CJK UNIFIED IDEOGRAPH
    {0xCA51, 0x9099}, //10291 #CJK UNIFIED IDEOGRAPH
    {0xCA52, 0x9097}, //10292 #CJK UNIFIED IDEOGRAPH
    {0xCA53, 0x9098}, //10293 #CJK UNIFIED IDEOGRAPH
    {0xCA54, 0x909B}, //10294 #CJK UNIFIED IDEOGRAPH
    {0xCA55, 0x9094}, //10295 #CJK UNIFIED IDEOGRAPH
    {0xCA56, 0x9622}, //10296 #CJK UNIFIED IDEOGRAPH
    {0xCA57, 0x9624}, //10297 #CJK UNIFIED IDEOGRAPH
    {0xCA58, 0x9620}, //10298 #CJK UNIFIED IDEOGRAPH
    {0xCA59, 0x9623}, //10299 #CJK UNIFIED IDEOGRAPH
    {0xCA5A, 0x4F56}, //10300 #CJK UNIFIED IDEOGRAPH
    {0xCA5B, 0x4F3B}, //10301 #CJK UNIFIED IDEOGRAPH
    {0xCA5C, 0x4F62}, //10302 #CJK UNIFIED IDEOGRAPH
    {0xCA5D, 0x4F49}, //10303 #CJK UNIFIED IDEOGRAPH
    {0xCA5E, 0x4F53}, //10304 #CJK UNIFIED IDEOGRAPH
    {0xCA5F, 0x4F64}, //10305 #CJK UNIFIED IDEOGRAPH
    {0xCA60, 0x4F3E}, //10306 #CJK UNIFIED IDEOGRAPH
    {0xCA61, 0x4F67}, //10307 #CJK UNIFIED IDEOGRAPH
    {0xCA62, 0x4F52}, //10308 #CJK UNIFIED IDEOGRAPH
    {0xCA63, 0x4F5F}, //10309 #CJK UNIFIED IDEOGRAPH
    {0xCA64, 0x4F41}, //10310 #CJK UNIFIED IDEOGRAPH
    {0xCA65, 0x4F58}, //10311 #CJK UNIFIED IDEOGRAPH
    {0xCA66, 0x4F2D}, //10312 #CJK UNIFIED IDEOGRAPH
    {0xCA67, 0x4F33}, //10313 #CJK UNIFIED IDEOGRAPH
    {0xCA68, 0x4F3F}, //10314 #CJK UNIFIED IDEOGRAPH
    {0xCA69, 0x4F61}, //10315 #CJK UNIFIED IDEOGRAPH
    {0xCA6A, 0x518F}, //10316 #CJK UNIFIED IDEOGRAPH
    {0xCA6B, 0x51B9}, //10317 #CJK UNIFIED IDEOGRAPH
    {0xCA6C, 0x521C}, //10318 #CJK UNIFIED IDEOGRAPH
    {0xCA6D, 0x521E}, //10319 #CJK UNIFIED IDEOGRAPH
    {0xCA6E, 0x5221}, //10320 #CJK UNIFIED IDEOGRAPH
    {0xCA6F, 0x52AD}, //10321 #CJK UNIFIED IDEOGRAPH
    {0xCA70, 0x52AE}, //10322 #CJK UNIFIED IDEOGRAPH
    {0xCA71, 0x5309}, //10323 #CJK UNIFIED IDEOGRAPH
    {0xCA72, 0x5363}, //10324 #CJK UNIFIED IDEOGRAPH
    {0xCA73, 0x5372}, //10325 #CJK UNIFIED IDEOGRAPH
    {0xCA74, 0x538E}, //10326 #CJK UNIFIED IDEOGRAPH
    {0xCA75, 0x538F}, //10327 #CJK UNIFIED IDEOGRAPH
    {0xCA76, 0x5430}, //10328 #CJK UNIFIED IDEOGRAPH
    {0xCA77, 0x5437}, //10329 #CJK UNIFIED IDEOGRAPH
    {0xCA78, 0x542A}, //10330 #CJK UNIFIED IDEOGRAPH
    {0xCA79, 0x5454}, //10331 #CJK UNIFIED IDEOGRAPH
    {0xCA7A, 0x5445}, //10332 #CJK UNIFIED IDEOGRAPH
    {0xCA7B, 0x5419}, //10333 #CJK UNIFIED IDEOGRAPH
    {0xCA7C, 0x541C}, //10334 #CJK UNIFIED IDEOGRAPH
    {0xCA7D, 0x5425}, //10335 #CJK UNIFIED IDEOGRAPH
    {0xCA7E, 0x5418}, //10336 #CJK UNIFIED IDEOGRAPH
    {0xCAA1, 0x543D}, //10337 #CJK UNIFIED IDEOGRAPH
    {0xCAA2, 0x544F}, //10338 #CJK UNIFIED IDEOGRAPH
    {0xCAA3, 0x5441}, //10339 #CJK UNIFIED IDEOGRAPH
    {0xCAA4, 0x5428}, //10340 #CJK UNIFIED IDEOGRAPH
    {0xCAA5, 0x5424}, //10341 #CJK UNIFIED IDEOGRAPH
    {0xCAA6, 0x5447}, //10342 #CJK UNIFIED IDEOGRAPH
    {0xCAA7, 0x56EE}, //10343 #CJK UNIFIED IDEOGRAPH
    {0xCAA8, 0x56E7}, //10344 #CJK UNIFIED IDEOGRAPH
    {0xCAA9, 0x56E5}, //10345 #CJK UNIFIED IDEOGRAPH
    {0xCAAA, 0x5741}, //10346 #CJK UNIFIED IDEOGRAPH
    {0xCAAB, 0x5745}, //10347 #CJK UNIFIED IDEOGRAPH
    {0xCAAC, 0x574C}, //10348 #CJK UNIFIED IDEOGRAPH
    {0xCAAD, 0x5749}, //10349 #CJK UNIFIED IDEOGRAPH
    {0xCAAE, 0x574B}, //10350 #CJK UNIFIED IDEOGRAPH
    {0xCAAF, 0x5752}, //10351 #CJK UNIFIED IDEOGRAPH
    {0xCAB0, 0x5906}, //10352 #CJK UNIFIED IDEOGRAPH
    {0xCAB1, 0x5940}, //10353 #CJK UNIFIED IDEOGRAPH
    {0xCAB2, 0x59A6}, //10354 #CJK UNIFIED IDEOGRAPH
    {0xCAB3, 0x5998}, //10355 #CJK UNIFIED IDEOGRAPH
    {0xCAB4, 0x59A0}, //10356 #CJK UNIFIED IDEOGRAPH
    {0xCAB5, 0x5997}, //10357 #CJK UNIFIED IDEOGRAPH
    {0xCAB6, 0x598E}, //10358 #CJK UNIFIED IDEOGRAPH
    {0xCAB7, 0x59A2}, //10359 #CJK UNIFIED IDEOGRAPH
    {0xCAB8, 0x5990}, //10360 #CJK UNIFIED IDEOGRAPH
    {0xCAB9, 0x598F}, //10361 #CJK UNIFIED IDEOGRAPH
    {0xCABA, 0x59A7}, //10362 #CJK UNIFIED IDEOGRAPH
    {0xCABB, 0x59A1}, //10363 #CJK UNIFIED IDEOGRAPH
    {0xCABC, 0x5B8E}, //10364 #CJK UNIFIED IDEOGRAPH
    {0xCABD, 0x5B92}, //10365 #CJK UNIFIED IDEOGRAPH
    {0xCABE, 0x5C28}, //10366 #CJK UNIFIED IDEOGRAPH
    {0xCABF, 0x5C2A}, //10367 #CJK UNIFIED IDEOGRAPH
    {0xCAC0, 0x5C8D}, //10368 #CJK UNIFIED IDEOGRAPH
    {0xCAC1, 0x5C8F}, //10369 #CJK UNIFIED IDEOGRAPH
    {0xCAC2, 0x5C88}, //10370 #CJK UNIFIED IDEOGRAPH
    {0xCAC3, 0x5C8B}, //10371 #CJK UNIFIED IDEOGRAPH
    {0xCAC4, 0x5C89}, //10372 #CJK UNIFIED IDEOGRAPH
    {0xCAC5, 0x5C92}, //10373 #CJK UNIFIED IDEOGRAPH
    {0xCAC6, 0x5C8A}, //10374 #CJK UNIFIED IDEOGRAPH
    {0xCAC7, 0x5C86}, //10375 #CJK UNIFIED IDEOGRAPH
    {0xCAC8, 0x5C93}, //10376 #CJK UNIFIED IDEOGRAPH
    {0xCAC9, 0x5C95}, //10377 #CJK UNIFIED IDEOGRAPH
    {0xCACA, 0x5DE0}, //10378 #CJK UNIFIED IDEOGRAPH
    {0xCACB, 0x5E0A}, //10379 #CJK UNIFIED IDEOGRAPH
    {0xCACC, 0x5E0E}, //10380 #CJK UNIFIED IDEOGRAPH
    {0xCACD, 0x5E8B}, //10381 #CJK UNIFIED IDEOGRAPH
    {0xCACE, 0x5E89}, //10382 #CJK UNIFIED IDEOGRAPH
    {0xCACF, 0x5E8C}, //10383 #CJK UNIFIED IDEOGRAPH
    {0xCAD0, 0x5E88}, //10384 #CJK UNIFIED IDEOGRAPH
    {0xCAD1, 0x5E8D}, //10385 #CJK UNIFIED IDEOGRAPH
    {0xCAD2, 0x5F05}, //10386 #CJK UNIFIED IDEOGRAPH
    {0xCAD3, 0x5F1D}, //10387 #CJK UNIFIED IDEOGRAPH
    {0xCAD4, 0x5F78}, //10388 #CJK UNIFIED IDEOGRAPH
    {0xCAD5, 0x5F76}, //10389 #CJK UNIFIED IDEOGRAPH
    {0xCAD6, 0x5FD2}, //10390 #CJK UNIFIED IDEOGRAPH
    {0xCAD7, 0x5FD1}, //10391 #CJK UNIFIED IDEOGRAPH
    {0xCAD8, 0x5FD0}, //10392 #CJK UNIFIED IDEOGRAPH
    {0xCAD9, 0x5FED}, //10393 #CJK UNIFIED IDEOGRAPH
    {0xCADA, 0x5FE8}, //10394 #CJK UNIFIED IDEOGRAPH
    {0xCADB, 0x5FEE}, //10395 #CJK UNIFIED IDEOGRAPH
    {0xCADC, 0x5FF3}, //10396 #CJK UNIFIED IDEOGRAPH
    {0xCADD, 0x5FE1}, //10397 #CJK UNIFIED IDEOGRAPH
    {0xCADE, 0x5FE4}, //10398 #CJK UNIFIED IDEOGRAPH
    {0xCADF, 0x5FE3}, //10399 #CJK UNIFIED IDEOGRAPH
    {0xCAE0, 0x5FFA}, //10400 #CJK UNIFIED IDEOGRAPH
    {0xCAE1, 0x5FEF}, //10401 #CJK UNIFIED IDEOGRAPH
    {0xCAE2, 0x5FF7}, //10402 #CJK UNIFIED IDEOGRAPH
    {0xCAE3, 0x5FFB}, //10403 #CJK UNIFIED IDEOGRAPH
    {0xCAE4, 0x6000}, //10404 #CJK UNIFIED IDEOGRAPH
    {0xCAE5, 0x5FF4}, //10405 #CJK UNIFIED IDEOGRAPH
    {0xCAE6, 0x623A}, //10406 #CJK UNIFIED IDEOGRAPH
    {0xCAE7, 0x6283}, //10407 #CJK UNIFIED IDEOGRAPH
    {0xCAE8, 0x628C}, //10408 #CJK UNIFIED IDEOGRAPH
    {0xCAE9, 0x628E}, //10409 #CJK UNIFIED IDEOGRAPH
    {0xCAEA, 0x628F}, //10410 #CJK UNIFIED IDEOGRAPH
    {0xCAEB, 0x6294}, //10411 #CJK UNIFIED IDEOGRAPH
    {0xCAEC, 0x6287}, //10412 #CJK UNIFIED IDEOGRAPH
    {0xCAED, 0x6271}, //10413 #CJK UNIFIED IDEOGRAPH
    {0xCAEE, 0x627B}, //10414 #CJK UNIFIED IDEOGRAPH
    {0xCAEF, 0x627A}, //10415 #CJK UNIFIED IDEOGRAPH
    {0xCAF0, 0x6270}, //10416 #CJK UNIFIED IDEOGRAPH
    {0xCAF1, 0x6281}, //10417 #CJK UNIFIED IDEOGRAPH
    {0xCAF2, 0x6288}, //10418 #CJK UNIFIED IDEOGRAPH
    {0xCAF3, 0x6277}, //10419 #CJK UNIFIED IDEOGRAPH
    {0xCAF4, 0x627D}, //10420 #CJK UNIFIED IDEOGRAPH
    {0xCAF5, 0x6272}, //10421 #CJK UNIFIED IDEOGRAPH
    {0xCAF6, 0x6274}, //10422 #CJK UNIFIED IDEOGRAPH
    {0xCAF7, 0x6537}, //10423 #CJK UNIFIED IDEOGRAPH
    {0xCAF8, 0x65F0}, //10424 #CJK UNIFIED IDEOGRAPH
    {0xCAF9, 0x65F4}, //10425 #CJK UNIFIED IDEOGRAPH
    {0xCAFA, 0x65F3}, //10426 #CJK UNIFIED IDEOGRAPH
    {0xCAFB, 0x65F2}, //10427 #CJK UNIFIED IDEOGRAPH
    {0xCAFC, 0x65F5}, //10428 #CJK UNIFIED IDEOGRAPH
    {0xCAFD, 0x6745}, //10429 #CJK UNIFIED IDEOGRAPH
    {0xCAFE, 0x6747}, //10430 #CJK UNIFIED IDEOGRAPH
    {0xCB40, 0x6759}, //10431 #CJK UNIFIED IDEOGRAPH
    {0xCB41, 0x6755}, //10432 #CJK UNIFIED IDEOGRAPH
    {0xCB42, 0x674C}, //10433 #CJK UNIFIED IDEOGRAPH
    {0xCB43, 0x6748}, //10434 #CJK UNIFIED IDEOGRAPH
    {0xCB44, 0x675D}, //10435 #CJK UNIFIED IDEOGRAPH
    {0xCB45, 0x674D}, //10436 #CJK UNIFIED IDEOGRAPH
    {0xCB46, 0x675A}, //10437 #CJK UNIFIED IDEOGRAPH
    {0xCB47, 0x674B}, //10438 #CJK UNIFIED IDEOGRAPH
    {0xCB48, 0x6BD0}, //10439 #CJK UNIFIED IDEOGRAPH
    {0xCB49, 0x6C19}, //10440 #CJK UNIFIED IDEOGRAPH
    {0xCB4A, 0x6C1A}, //10441 #CJK UNIFIED IDEOGRAPH
    {0xCB4B, 0x6C78}, //10442 #CJK UNIFIED IDEOGRAPH
    {0xCB4C, 0x6C67}, //10443 #CJK UNIFIED IDEOGRAPH
    {0xCB4D, 0x6C6B}, //10444 #CJK UNIFIED IDEOGRAPH
    {0xCB4E, 0x6C84}, //10445 #CJK UNIFIED IDEOGRAPH
    {0xCB4F, 0x6C8B}, //10446 #CJK UNIFIED IDEOGRAPH
    {0xCB50, 0x6C8F}, //10447 #CJK UNIFIED IDEOGRAPH
    {0xCB51, 0x6C71}, //10448 #CJK UNIFIED IDEOGRAPH
    {0xCB52, 0x6C6F}, //10449 #CJK UNIFIED IDEOGRAPH
    {0xCB53, 0x6C69}, //10450 #CJK UNIFIED IDEOGRAPH
    {0xCB54, 0x6C9A}, //10451 #CJK UNIFIED IDEOGRAPH
    {0xCB55, 0x6C6D}, //10452 #CJK UNIFIED IDEOGRAPH
    {0xCB56, 0x6C87}, //10453 #CJK UNIFIED IDEOGRAPH
    {0xCB57, 0x6C95}, //10454 #CJK UNIFIED IDEOGRAPH
    {0xCB58, 0x6C9C}, //10455 #CJK UNIFIED IDEOGRAPH
    {0xCB59, 0x6C66}, //10456 #CJK UNIFIED IDEOGRAPH
    {0xCB5A, 0x6C73}, //10457 #CJK UNIFIED IDEOGRAPH
    {0xCB5B, 0x6C65}, //10458 #CJK UNIFIED IDEOGRAPH
    {0xCB5C, 0x6C7B}, //10459 #CJK UNIFIED IDEOGRAPH
    {0xCB5D, 0x6C8E}, //10460 #CJK UNIFIED IDEOGRAPH
    {0xCB5E, 0x7074}, //10461 #CJK UNIFIED IDEOGRAPH
    {0xCB5F, 0x707A}, //10462 #CJK UNIFIED IDEOGRAPH
    {0xCB60, 0x7263}, //10463 #CJK UNIFIED IDEOGRAPH
    {0xCB61, 0x72BF}, //10464 #CJK UNIFIED IDEOGRAPH
    {0xCB62, 0x72BD}, //10465 #CJK UNIFIED IDEOGRAPH
    {0xCB63, 0x72C3}, //10466 #CJK UNIFIED IDEOGRAPH
    {0xCB64, 0x72C6}, //10467 #CJK UNIFIED IDEOGRAPH
    {0xCB65, 0x72C1}, //10468 #CJK UNIFIED IDEOGRAPH
    {0xCB66, 0x72BA}, //10469 #CJK UNIFIED IDEOGRAPH
    {0xCB67, 0x72C5}, //10470 #CJK UNIFIED IDEOGRAPH
    {0xCB68, 0x7395}, //10471 #CJK UNIFIED IDEOGRAPH
    {0xCB69, 0x7397}, //10472 #CJK UNIFIED IDEOGRAPH
    {0xCB6A, 0x7393}, //10473 #CJK UNIFIED IDEOGRAPH
    {0xCB6B, 0x7394}, //10474 #CJK UNIFIED IDEOGRAPH
    {0xCB6C, 0x7392}, //10475 #CJK UNIFIED IDEOGRAPH
    {0xCB6D, 0x753A}, //10476 #CJK UNIFIED IDEOGRAPH
    {0xCB6E, 0x7539}, //10477 #CJK UNIFIED IDEOGRAPH
    {0xCB6F, 0x7594}, //10478 #CJK UNIFIED IDEOGRAPH
    {0xCB70, 0x7595}, //10479 #CJK UNIFIED IDEOGRAPH
    {0xCB71, 0x7681}, //10480 #CJK UNIFIED IDEOGRAPH
    {0xCB72, 0x793D}, //10481 #CJK UNIFIED IDEOGRAPH
    {0xCB73, 0x8034}, //10482 #CJK UNIFIED IDEOGRAPH
    {0xCB74, 0x8095}, //10483 #CJK UNIFIED IDEOGRAPH
    {0xCB75, 0x8099}, //10484 #CJK UNIFIED IDEOGRAPH
    {0xCB76, 0x8090}, //10485 #CJK UNIFIED IDEOGRAPH
    {0xCB77, 0x8092}, //10486 #CJK UNIFIED IDEOGRAPH
    {0xCB78, 0x809C}, //10487 #CJK UNIFIED IDEOGRAPH
    {0xCB79, 0x8290}, //10488 #CJK UNIFIED IDEOGRAPH
    {0xCB7A, 0x828F}, //10489 #CJK UNIFIED IDEOGRAPH
    {0xCB7B, 0x8285}, //10490 #CJK UNIFIED IDEOGRAPH
    {0xCB7C, 0x828E}, //10491 #CJK UNIFIED IDEOGRAPH
    {0xCB7D, 0x8291}, //10492 #CJK UNIFIED IDEOGRAPH
    {0xCB7E, 0x8293}, //10493 #CJK UNIFIED IDEOGRAPH
    {0xCBA1, 0x828A}, //10494 #CJK UNIFIED IDEOGRAPH
    {0xCBA2, 0x8283}, //10495 #CJK UNIFIED IDEOGRAPH
    {0xCBA3, 0x8284}, //10496 #CJK UNIFIED IDEOGRAPH
    {0xCBA4, 0x8C78}, //10497 #CJK UNIFIED IDEOGRAPH
    {0xCBA5, 0x8FC9}, //10498 #CJK UNIFIED IDEOGRAPH
    {0xCBA6, 0x8FBF}, //10499 #CJK UNIFIED IDEOGRAPH
    {0xCBA7, 0x909F}, //10500 #CJK UNIFIED IDEOGRAPH
    {0xCBA8, 0x90A1}, //10501 #CJK UNIFIED IDEOGRAPH
    {0xCBA9, 0x90A5}, //10502 #CJK UNIFIED IDEOGRAPH
    {0xCBAA, 0x909E}, //10503 #CJK UNIFIED IDEOGRAPH
    {0xCBAB, 0x90A7}, //10504 #CJK UNIFIED IDEOGRAPH
    {0xCBAC, 0x90A0}, //10505 #CJK UNIFIED IDEOGRAPH
    {0xCBAD, 0x9630}, //10506 #CJK UNIFIED IDEOGRAPH
    {0xCBAE, 0x9628}, //10507 #CJK UNIFIED IDEOGRAPH
    {0xCBAF, 0x962F}, //10508 #CJK UNIFIED IDEOGRAPH
    {0xCBB0, 0x962D}, //10509 #CJK UNIFIED IDEOGRAPH
    {0xCBB1, 0x4E33}, //10510 #CJK UNIFIED IDEOGRAPH
    {0xCBB2, 0x4F98}, //10511 #CJK UNIFIED IDEOGRAPH
    {0xCBB3, 0x4F7C}, //10512 #CJK UNIFIED IDEOGRAPH
    {0xCBB4, 0x4F85}, //10513 #CJK UNIFIED IDEOGRAPH
    {0xCBB5, 0x4F7D}, //10514 #CJK UNIFIED IDEOGRAPH
    {0xCBB6, 0x4F80}, //10515 #CJK UNIFIED IDEOGRAPH
    {0xCBB7, 0x4F87}, //10516 #CJK UNIFIED IDEOGRAPH
    {0xCBB8, 0x4F76}, //10517 #CJK UNIFIED IDEOGRAPH
    {0xCBB9, 0x4F74}, //10518 #CJK UNIFIED IDEOGRAPH
    {0xCBBA, 0x4F89}, //10519 #CJK UNIFIED IDEOGRAPH
    {0xCBBB, 0x4F84}, //10520 #CJK UNIFIED IDEOGRAPH
    {0xCBBC, 0x4F77}, //10521 #CJK UNIFIED IDEOGRAPH
    {0xCBBD, 0x4F4C}, //10522 #CJK UNIFIED IDEOGRAPH
    {0xCBBE, 0x4F97}, //10523 #CJK UNIFIED IDEOGRAPH
    {0xCBBF, 0x4F6A}, //10524 #CJK UNIFIED IDEOGRAPH
    {0xCBC0, 0x4F9A}, //10525 #CJK UNIFIED IDEOGRAPH
    {0xCBC1, 0x4F79}, //10526 #CJK UNIFIED IDEOGRAPH
    {0xCBC2, 0x4F81}, //10527 #CJK UNIFIED IDEOGRAPH
    {0xCBC3, 0x4F78}, //10528 #CJK UNIFIED IDEOGRAPH
    {0xCBC4, 0x4F90}, //10529 #CJK UNIFIED IDEOGRAPH
    {0xCBC5, 0x4F9C}, //10530 #CJK UNIFIED IDEOGRAPH
    {0xCBC6, 0x4F94}, //10531 #CJK UNIFIED IDEOGRAPH
    {0xCBC7, 0x4F9E}, //10532 #CJK UNIFIED IDEOGRAPH
    {0xCBC8, 0x4F92}, //10533 #CJK UNIFIED IDEOGRAPH
    {0xCBC9, 0x4F82}, //10534 #CJK UNIFIED IDEOGRAPH
    {0xCBCA, 0x4F95}, //10535 #CJK UNIFIED IDEOGRAPH
    {0xCBCB, 0x4F6B}, //10536 #CJK UNIFIED IDEOGRAPH
    {0xCBCC, 0x4F6E}, //10537 #CJK UNIFIED IDEOGRAPH
    {0xCBCD, 0x519E}, //10538 #CJK UNIFIED IDEOGRAPH
    {0xCBCE, 0x51BC}, //10539 #CJK UNIFIED IDEOGRAPH
    {0xCBCF, 0x51BE}, //10540 #CJK UNIFIED IDEOGRAPH
    {0xCBD0, 0x5235}, //10541 #CJK UNIFIED IDEOGRAPH
    {0xCBD1, 0x5232}, //10542 #CJK UNIFIED IDEOGRAPH
    {0xCBD2, 0x5233}, //10543 #CJK UNIFIED IDEOGRAPH
    {0xCBD3, 0x5246}, //10544 #CJK UNIFIED IDEOGRAPH
    {0xCBD4, 0x5231}, //10545 #CJK UNIFIED IDEOGRAPH
    {0xCBD5, 0x52BC}, //10546 #CJK UNIFIED IDEOGRAPH
    {0xCBD6, 0x530A}, //10547 #CJK UNIFIED IDEOGRAPH
    {0xCBD7, 0x530B}, //10548 #CJK UNIFIED IDEOGRAPH
    {0xCBD8, 0x533C}, //10549 #CJK UNIFIED IDEOGRAPH
    {0xCBD9, 0x5392}, //10550 #CJK UNIFIED IDEOGRAPH
    {0xCBDA, 0x5394}, //10551 #CJK UNIFIED IDEOGRAPH
    {0xCBDB, 0x5487}, //10552 #CJK UNIFIED IDEOGRAPH
    {0xCBDC, 0x547F}, //10553 #CJK UNIFIED IDEOGRAPH
    {0xCBDD, 0x5481}, //10554 #CJK UNIFIED IDEOGRAPH
    {0xCBDE, 0x5491}, //10555 #CJK UNIFIED IDEOGRAPH
    {0xCBDF, 0x5482}, //10556 #CJK UNIFIED IDEOGRAPH
    {0xCBE0, 0x5488}, //10557 #CJK UNIFIED IDEOGRAPH
    {0xCBE1, 0x546B}, //10558 #CJK UNIFIED IDEOGRAPH
    {0xCBE2, 0x547A}, //10559 #CJK UNIFIED IDEOGRAPH
    {0xCBE3, 0x547E}, //10560 #CJK UNIFIED IDEOGRAPH
    {0xCBE4, 0x5465}, //10561 #CJK UNIFIED IDEOGRAPH
    {0xCBE5, 0x546C}, //10562 #CJK UNIFIED IDEOGRAPH
    {0xCBE6, 0x5474}, //10563 #CJK UNIFIED IDEOGRAPH
    {0xCBE7, 0x5466}, //10564 #CJK UNIFIED IDEOGRAPH
    {0xCBE8, 0x548D}, //10565 #CJK UNIFIED IDEOGRAPH
    {0xCBE9, 0x546F}, //10566 #CJK UNIFIED IDEOGRAPH
    {0xCBEA, 0x5461}, //10567 #CJK UNIFIED IDEOGRAPH
    {0xCBEB, 0x5460}, //10568 #CJK UNIFIED IDEOGRAPH
    {0xCBEC, 0x5498}, //10569 #CJK UNIFIED IDEOGRAPH
    {0xCBED, 0x5463}, //10570 #CJK UNIFIED IDEOGRAPH
    {0xCBEE, 0x5467}, //10571 #CJK UNIFIED IDEOGRAPH
    {0xCBEF, 0x5464}, //10572 #CJK UNIFIED IDEOGRAPH
    {0xCBF0, 0x56F7}, //10573 #CJK UNIFIED IDEOGRAPH
    {0xCBF1, 0x56F9}, //10574 #CJK UNIFIED IDEOGRAPH
    {0xCBF2, 0x576F}, //10575 #CJK UNIFIED IDEOGRAPH
    {0xCBF3, 0x5772}, //10576 #CJK UNIFIED IDEOGRAPH
    {0xCBF4, 0x576D}, //10577 #CJK UNIFIED IDEOGRAPH
    {0xCBF5, 0x576B}, //10578 #CJK UNIFIED IDEOGRAPH
    {0xCBF6, 0x5771}, //10579 #CJK UNIFIED IDEOGRAPH
    {0xCBF7, 0x5770}, //10580 #CJK UNIFIED IDEOGRAPH
    {0xCBF8, 0x5776}, //10581 #CJK UNIFIED IDEOGRAPH
    {0xCBF9, 0x5780}, //10582 #CJK UNIFIED IDEOGRAPH
    {0xCBFA, 0x5775}, //10583 #CJK UNIFIED IDEOGRAPH
    {0xCBFB, 0x577B}, //10584 #CJK UNIFIED IDEOGRAPH
    {0xCBFC, 0x5773}, //10585 #CJK UNIFIED IDEOGRAPH
    {0xCBFD, 0x5774}, //10586 #CJK UNIFIED IDEOGRAPH
    {0xCBFE, 0x5762}, //10587 #CJK UNIFIED IDEOGRAPH
    {0xCC40, 0x5768}, //10588 #CJK UNIFIED IDEOGRAPH
    {0xCC41, 0x577D}, //10589 #CJK UNIFIED IDEOGRAPH
    {0xCC42, 0x590C}, //10590 #CJK UNIFIED IDEOGRAPH
    {0xCC43, 0x5945}, //10591 #CJK UNIFIED IDEOGRAPH
    {0xCC44, 0x59B5}, //10592 #CJK UNIFIED IDEOGRAPH
    {0xCC45, 0x59BA}, //10593 #CJK UNIFIED IDEOGRAPH
    {0xCC46, 0x59CF}, //10594 #CJK UNIFIED IDEOGRAPH
    {0xCC47, 0x59CE}, //10595 #CJK UNIFIED IDEOGRAPH
    {0xCC48, 0x59B2}, //10596 #CJK UNIFIED IDEOGRAPH
    {0xCC49, 0x59CC}, //10597 #CJK UNIFIED IDEOGRAPH
    {0xCC4A, 0x59C1}, //10598 #CJK UNIFIED IDEOGRAPH
    {0xCC4B, 0x59B6}, //10599 #CJK UNIFIED IDEOGRAPH
    {0xCC4C, 0x59BC}, //10600 #CJK UNIFIED IDEOGRAPH
    {0xCC4D, 0x59C3}, //10601 #CJK UNIFIED IDEOGRAPH
    {0xCC4E, 0x59D6}, //10602 #CJK UNIFIED IDEOGRAPH
    {0xCC4F, 0x59B1}, //10603 #CJK UNIFIED IDEOGRAPH
    {0xCC50, 0x59BD}, //10604 #CJK UNIFIED IDEOGRAPH
    {0xCC51, 0x59C0}, //10605 #CJK UNIFIED IDEOGRAPH
    {0xCC52, 0x59C8}, //10606 #CJK UNIFIED IDEOGRAPH
    {0xCC53, 0x59B4}, //10607 #CJK UNIFIED IDEOGRAPH
    {0xCC54, 0x59C7}, //10608 #CJK UNIFIED IDEOGRAPH
    {0xCC55, 0x5B62}, //10609 #CJK UNIFIED IDEOGRAPH
    {0xCC56, 0x5B65}, //10610 #CJK UNIFIED IDEOGRAPH
    {0xCC57, 0x5B93}, //10611 #CJK UNIFIED IDEOGRAPH
    {0xCC58, 0x5B95}, //10612 #CJK UNIFIED IDEOGRAPH
    {0xCC59, 0x5C44}, //10613 #CJK UNIFIED IDEOGRAPH
    {0xCC5A, 0x5C47}, //10614 #CJK UNIFIED IDEOGRAPH
    {0xCC5B, 0x5CAE}, //10615 #CJK UNIFIED IDEOGRAPH
    {0xCC5C, 0x5CA4}, //10616 #CJK UNIFIED IDEOGRAPH
    {0xCC5D, 0x5CA0}, //10617 #CJK UNIFIED IDEOGRAPH
    {0xCC5E, 0x5CB5}, //10618 #CJK UNIFIED IDEOGRAPH
    {0xCC5F, 0x5CAF}, //10619 #CJK UNIFIED IDEOGRAPH
    {0xCC60, 0x5CA8}, //10620 #CJK UNIFIED IDEOGRAPH
    {0xCC61, 0x5CAC}, //10621 #CJK UNIFIED IDEOGRAPH
    {0xCC62, 0x5C9F}, //10622 #CJK UNIFIED IDEOGRAPH
    {0xCC63, 0x5CA3}, //10623 #CJK UNIFIED IDEOGRAPH
    {0xCC64, 0x5CAD}, //10624 #CJK UNIFIED IDEOGRAPH
    {0xCC65, 0x5CA2}, //10625 #CJK UNIFIED IDEOGRAPH
    {0xCC66, 0x5CAA}, //10626 #CJK UNIFIED IDEOGRAPH
    {0xCC67, 0x5CA7}, //10627 #CJK UNIFIED IDEOGRAPH
    {0xCC68, 0x5C9D}, //10628 #CJK UNIFIED IDEOGRAPH
    {0xCC69, 0x5CA5}, //10629 #CJK UNIFIED IDEOGRAPH
    {0xCC6A, 0x5CB6}, //10630 #CJK UNIFIED IDEOGRAPH
    {0xCC6B, 0x5CB0}, //10631 #CJK UNIFIED IDEOGRAPH
    {0xCC6C, 0x5CA6}, //10632 #CJK UNIFIED IDEOGRAPH
    {0xCC6D, 0x5E17}, //10633 #CJK UNIFIED IDEOGRAPH
    {0xCC6E, 0x5E14}, //10634 #CJK UNIFIED IDEOGRAPH
    {0xCC6F, 0x5E19}, //10635 #CJK UNIFIED IDEOGRAPH
    {0xCC70, 0x5F28}, //10636 #CJK UNIFIED IDEOGRAPH
    {0xCC71, 0x5F22}, //10637 #CJK UNIFIED IDEOGRAPH
    {0xCC72, 0x5F23}, //10638 #CJK UNIFIED IDEOGRAPH
    {0xCC73, 0x5F24}, //10639 #CJK UNIFIED IDEOGRAPH
    {0xCC74, 0x5F54}, //10640 #CJK UNIFIED IDEOGRAPH
    {0xCC75, 0x5F82}, //10641 #CJK UNIFIED IDEOGRAPH
    {0xCC76, 0x5F7E}, //10642 #CJK UNIFIED IDEOGRAPH
    {0xCC77, 0x5F7D}, //10643 #CJK UNIFIED IDEOGRAPH
    {0xCC78, 0x5FDE}, //10644 #CJK UNIFIED IDEOGRAPH
    {0xCC79, 0x5FE5}, //10645 #CJK UNIFIED IDEOGRAPH
    {0xCC7A, 0x602D}, //10646 #CJK UNIFIED IDEOGRAPH
    {0xCC7B, 0x6026}, //10647 #CJK UNIFIED IDEOGRAPH
    {0xCC7C, 0x6019}, //10648 #CJK UNIFIED IDEOGRAPH
    {0xCC7D, 0x6032}, //10649 #CJK UNIFIED IDEOGRAPH
    {0xCC7E, 0x600B}, //10650 #CJK UNIFIED IDEOGRAPH
    {0xCCA1, 0x6034}, //10651 #CJK UNIFIED IDEOGRAPH
    {0xCCA2, 0x600A}, //10652 #CJK UNIFIED IDEOGRAPH
    {0xCCA3, 0x6017}, //10653 #CJK UNIFIED IDEOGRAPH
    {0xCCA4, 0x6033}, //10654 #CJK UNIFIED IDEOGRAPH
    {0xCCA5, 0x601A}, //10655 #CJK UNIFIED IDEOGRAPH
    {0xCCA6, 0x601E}, //10656 #CJK UNIFIED IDEOGRAPH
    {0xCCA7, 0x602C}, //10657 #CJK UNIFIED IDEOGRAPH
    {0xCCA8, 0x6022}, //10658 #CJK UNIFIED IDEOGRAPH
    {0xCCA9, 0x600D}, //10659 #CJK UNIFIED IDEOGRAPH
    {0xCCAA, 0x6010}, //10660 #CJK UNIFIED IDEOGRAPH
    {0xCCAB, 0x602E}, //10661 #CJK UNIFIED IDEOGRAPH
    {0xCCAC, 0x6013}, //10662 #CJK UNIFIED IDEOGRAPH
    {0xCCAD, 0x6011}, //10663 #CJK UNIFIED IDEOGRAPH
    {0xCCAE, 0x600C}, //10664 #CJK UNIFIED IDEOGRAPH
    {0xCCAF, 0x6009}, //10665 #CJK UNIFIED IDEOGRAPH
    {0xCCB0, 0x601C}, //10666 #CJK UNIFIED IDEOGRAPH
    {0xCCB1, 0x6214}, //10667 #CJK UNIFIED IDEOGRAPH
    {0xCCB2, 0x623D}, //10668 #CJK UNIFIED IDEOGRAPH
    {0xCCB3, 0x62AD}, //10669 #CJK UNIFIED IDEOGRAPH
    {0xCCB4, 0x62B4}, //10670 #CJK UNIFIED IDEOGRAPH
    {0xCCB5, 0x62D1}, //10671 #CJK UNIFIED IDEOGRAPH
    {0xCCB6, 0x62BE}, //10672 #CJK UNIFIED IDEOGRAPH
    {0xCCB7, 0x62AA}, //10673 #CJK UNIFIED IDEOGRAPH
    {0xCCB8, 0x62B6}, //10674 #CJK UNIFIED IDEOGRAPH
    {0xCCB9, 0x62CA}, //10675 #CJK UNIFIED IDEOGRAPH
    {0xCCBA, 0x62AE}, //10676 #CJK UNIFIED IDEOGRAPH
    {0xCCBB, 0x62B3}, //10677 #CJK UNIFIED IDEOGRAPH
    {0xCCBC, 0x62AF}, //10678 #CJK UNIFIED IDEOGRAPH
    {0xCCBD, 0x62BB}, //10679 #CJK UNIFIED IDEOGRAPH
    {0xCCBE, 0x62A9}, //10680 #CJK UNIFIED IDEOGRAPH
    {0xCCBF, 0x62B0}, //10681 #CJK UNIFIED IDEOGRAPH
    {0xCCC0, 0x62B8}, //10682 #CJK UNIFIED IDEOGRAPH
    {0xCCC1, 0x653D}, //10683 #CJK UNIFIED IDEOGRAPH
    {0xCCC2, 0x65A8}, //10684 #CJK UNIFIED IDEOGRAPH
    {0xCCC3, 0x65BB}, //10685 #CJK UNIFIED IDEOGRAPH
    {0xCCC4, 0x6609}, //10686 #CJK UNIFIED IDEOGRAPH
    {0xCCC5, 0x65FC}, //10687 #CJK UNIFIED IDEOGRAPH
    {0xCCC6, 0x6604}, //10688 #CJK UNIFIED IDEOGRAPH
    {0xCCC7, 0x6612}, //10689 #CJK UNIFIED IDEOGRAPH
    {0xCCC8, 0x6608}, //10690 #CJK UNIFIED IDEOGRAPH
    {0xCCC9, 0x65FB}, //10691 #CJK UNIFIED IDEOGRAPH
    {0xCCCA, 0x6603}, //10692 #CJK UNIFIED IDEOGRAPH
    {0xCCCB, 0x660B}, //10693 #CJK UNIFIED IDEOGRAPH
    {0xCCCC, 0x660D}, //10694 #CJK UNIFIED IDEOGRAPH
    {0xCCCD, 0x6605}, //10695 #CJK UNIFIED IDEOGRAPH
    {0xCCCE, 0x65FD}, //10696 #CJK UNIFIED IDEOGRAPH
    {0xCCCF, 0x6611}, //10697 #CJK UNIFIED IDEOGRAPH
    {0xCCD0, 0x6610}, //10698 #CJK UNIFIED IDEOGRAPH
    {0xCCD1, 0x66F6}, //10699 #CJK UNIFIED IDEOGRAPH
    {0xCCD2, 0x670A}, //10700 #CJK UNIFIED IDEOGRAPH
    {0xCCD3, 0x6785}, //10701 #CJK UNIFIED IDEOGRAPH
    {0xCCD4, 0x676C}, //10702 #CJK UNIFIED IDEOGRAPH
    {0xCCD5, 0x678E}, //10703 #CJK UNIFIED IDEOGRAPH
    {0xCCD6, 0x6792}, //10704 #CJK UNIFIED IDEOGRAPH
    {0xCCD7, 0x6776}, //10705 #CJK UNIFIED IDEOGRAPH
    {0xCCD8, 0x677B}, //10706 #CJK UNIFIED IDEOGRAPH
    {0xCCD9, 0x6798}, //10707 #CJK UNIFIED IDEOGRAPH
    {0xCCDA, 0x6786}, //10708 #CJK UNIFIED IDEOGRAPH
    {0xCCDB, 0x6784}, //10709 #CJK UNIFIED IDEOGRAPH
    {0xCCDC, 0x6774}, //10710 #CJK UNIFIED IDEOGRAPH
    {0xCCDD, 0x678D}, //10711 #CJK UNIFIED IDEOGRAPH
    {0xCCDE, 0x678C}, //10712 #CJK UNIFIED IDEOGRAPH
    {0xCCDF, 0x677A}, //10713 #CJK UNIFIED IDEOGRAPH
    {0xCCE0, 0x679F}, //10714 #CJK UNIFIED IDEOGRAPH
    {0xCCE1, 0x6791}, //10715 #CJK UNIFIED IDEOGRAPH
    {0xCCE2, 0x6799}, //10716 #CJK UNIFIED IDEOGRAPH
    {0xCCE3, 0x6783}, //10717 #CJK UNIFIED IDEOGRAPH
    {0xCCE4, 0x677D}, //10718 #CJK UNIFIED IDEOGRAPH
    {0xCCE5, 0x6781}, //10719 #CJK UNIFIED IDEOGRAPH
    {0xCCE6, 0x6778}, //10720 #CJK UNIFIED IDEOGRAPH
    {0xCCE7, 0x6779}, //10721 #CJK UNIFIED IDEOGRAPH
    {0xCCE8, 0x6794}, //10722 #CJK UNIFIED IDEOGRAPH
    {0xCCE9, 0x6B25}, //10723 #CJK UNIFIED IDEOGRAPH
    {0xCCEA, 0x6B80}, //10724 #CJK UNIFIED IDEOGRAPH
    {0xCCEB, 0x6B7E}, //10725 #CJK UNIFIED IDEOGRAPH
    {0xCCEC, 0x6BDE}, //10726 #CJK UNIFIED IDEOGRAPH
    {0xCCED, 0x6C1D}, //10727 #CJK UNIFIED IDEOGRAPH
    {0xCCEE, 0x6C93}, //10728 #CJK UNIFIED IDEOGRAPH
    {0xCCEF, 0x6CEC}, //10729 #CJK UNIFIED IDEOGRAPH
    {0xCCF0, 0x6CEB}, //10730 #CJK UNIFIED IDEOGRAPH
    {0xCCF1, 0x6CEE}, //10731 #CJK UNIFIED IDEOGRAPH
    {0xCCF2, 0x6CD9}, //10732 #CJK UNIFIED IDEOGRAPH
    {0xCCF3, 0x6CB6}, //10733 #CJK UNIFIED IDEOGRAPH
    {0xCCF4, 0x6CD4}, //10734 #CJK UNIFIED IDEOGRAPH
    {0xCCF5, 0x6CAD}, //10735 #CJK UNIFIED IDEOGRAPH
    {0xCCF6, 0x6CE7}, //10736 #CJK UNIFIED IDEOGRAPH
    {0xCCF7, 0x6CB7}, //10737 #CJK UNIFIED IDEOGRAPH
    {0xCCF8, 0x6CD0}, //10738 #CJK UNIFIED IDEOGRAPH
    {0xCCF9, 0x6CC2}, //10739 #CJK UNIFIED IDEOGRAPH
    {0xCCFA, 0x6CBA}, //10740 #CJK UNIFIED IDEOGRAPH
    {0xCCFB, 0x6CC3}, //10741 #CJK UNIFIED IDEOGRAPH
    {0xCCFC, 0x6CC6}, //10742 #CJK UNIFIED IDEOGRAPH
    {0xCCFD, 0x6CED}, //10743 #CJK UNIFIED IDEOGRAPH
    {0xCCFE, 0x6CF2}, //10744 #CJK UNIFIED IDEOGRAPH
    {0xCD40, 0x6CD2}, //10745 #CJK UNIFIED IDEOGRAPH
    {0xCD41, 0x6CDD}, //10746 #CJK UNIFIED IDEOGRAPH
    {0xCD42, 0x6CB4}, //10747 #CJK UNIFIED IDEOGRAPH
    {0xCD43, 0x6C8A}, //10748 #CJK UNIFIED IDEOGRAPH
    {0xCD44, 0x6C9D}, //10749 #CJK UNIFIED IDEOGRAPH
    {0xCD45, 0x6C80}, //10750 #CJK UNIFIED IDEOGRAPH
    {0xCD46, 0x6CDE}, //10751 #CJK UNIFIED IDEOGRAPH
    {0xCD47, 0x6CC0}, //10752 #CJK UNIFIED IDEOGRAPH
    {0xCD48, 0x6D30}, //10753 #CJK UNIFIED IDEOGRAPH
    {0xCD49, 0x6CCD}, //10754 #CJK UNIFIED IDEOGRAPH
    {0xCD4A, 0x6CC7}, //10755 #CJK UNIFIED IDEOGRAPH
    {0xCD4B, 0x6CB0}, //10756 #CJK UNIFIED IDEOGRAPH
    {0xCD4C, 0x6CF9}, //10757 #CJK UNIFIED IDEOGRAPH
    {0xCD4D, 0x6CCF}, //10758 #CJK UNIFIED IDEOGRAPH
    {0xCD4E, 0x6CE9}, //10759 #CJK UNIFIED IDEOGRAPH
    {0xCD4F, 0x6CD1}, //10760 #CJK UNIFIED IDEOGRAPH
    {0xCD50, 0x7094}, //10761 #CJK UNIFIED IDEOGRAPH
    {0xCD51, 0x7098}, //10762 #CJK UNIFIED IDEOGRAPH
    {0xCD52, 0x7085}, //10763 #CJK UNIFIED IDEOGRAPH
    {0xCD53, 0x7093}, //10764 #CJK UNIFIED IDEOGRAPH
    {0xCD54, 0x7086}, //10765 #CJK UNIFIED IDEOGRAPH
    {0xCD55, 0x7084}, //10766 #CJK UNIFIED IDEOGRAPH
    {0xCD56, 0x7091}, //10767 #CJK UNIFIED IDEOGRAPH
    {0xCD57, 0x7096}, //10768 #CJK UNIFIED IDEOGRAPH
    {0xCD58, 0x7082}, //10769 #CJK UNIFIED IDEOGRAPH
    {0xCD59, 0x709A}, //10770 #CJK UNIFIED IDEOGRAPH
    {0xCD5A, 0x7083}, //10771 #CJK UNIFIED IDEOGRAPH
    {0xCD5B, 0x726A}, //10772 #CJK UNIFIED IDEOGRAPH
    {0xCD5C, 0x72D6}, //10773 #CJK UNIFIED IDEOGRAPH
    {0xCD5D, 0x72CB}, //10774 #CJK UNIFIED IDEOGRAPH
    {0xCD5E, 0x72D8}, //10775 #CJK UNIFIED IDEOGRAPH
    {0xCD5F, 0x72C9}, //10776 #CJK UNIFIED IDEOGRAPH
    {0xCD60, 0x72DC}, //10777 #CJK UNIFIED IDEOGRAPH
    {0xCD61, 0x72D2}, //10778 #CJK UNIFIED IDEOGRAPH
    {0xCD62, 0x72D4}, //10779 #CJK UNIFIED IDEOGRAPH
    {0xCD63, 0x72DA}, //10780 #CJK UNIFIED IDEOGRAPH
    {0xCD64, 0x72CC}, //10781 #CJK UNIFIED IDEOGRAPH
    {0xCD65, 0x72D1}, //10782 #CJK UNIFIED IDEOGRAPH
    {0xCD66, 0x73A4}, //10783 #CJK UNIFIED IDEOGRAPH
    {0xCD67, 0x73A1}, //10784 #CJK UNIFIED IDEOGRAPH
    {0xCD68, 0x73AD}, //10785 #CJK UNIFIED IDEOGRAPH
    {0xCD69, 0x73A6}, //10786 #CJK UNIFIED IDEOGRAPH
    {0xCD6A, 0x73A2}, //10787 #CJK UNIFIED IDEOGRAPH
    {0xCD6B, 0x73A0}, //10788 #CJK UNIFIED IDEOGRAPH
    {0xCD6C, 0x73AC}, //10789 #CJK UNIFIED IDEOGRAPH
    {0xCD6D, 0x739D}, //10790 #CJK UNIFIED IDEOGRAPH
    {0xCD6E, 0x74DD}, //10791 #CJK UNIFIED IDEOGRAPH
    {0xCD6F, 0x74E8}, //10792 #CJK UNIFIED IDEOGRAPH
    {0xCD70, 0x753F}, //10793 #CJK UNIFIED IDEOGRAPH
    {0xCD71, 0x7540}, //10794 #CJK UNIFIED IDEOGRAPH
    {0xCD72, 0x753E}, //10795 #CJK UNIFIED IDEOGRAPH
    {0xCD73, 0x758C}, //10796 #CJK UNIFIED IDEOGRAPH
    {0xCD74, 0x7598}, //10797 #CJK UNIFIED IDEOGRAPH
    {0xCD75, 0x76AF}, //10798 #CJK UNIFIED IDEOGRAPH
    {0xCD76, 0x76F3}, //10799 #CJK UNIFIED IDEOGRAPH
    {0xCD77, 0x76F1}, //10800 #CJK UNIFIED IDEOGRAPH
    {0xCD78, 0x76F0}, //10801 #CJK UNIFIED IDEOGRAPH
    {0xCD79, 0x76F5}, //10802 #CJK UNIFIED IDEOGRAPH
    {0xCD7A, 0x77F8}, //10803 #CJK UNIFIED IDEOGRAPH
    {0xCD7B, 0x77FC}, //10804 #CJK UNIFIED IDEOGRAPH
    {0xCD7C, 0x77F9}, //10805 #CJK UNIFIED IDEOGRAPH
    {0xCD7D, 0x77FB}, //10806 #CJK UNIFIED IDEOGRAPH
    {0xCD7E, 0x77FA}, //10807 #CJK UNIFIED IDEOGRAPH
    {0xCDA1, 0x77F7}, //10808 #CJK UNIFIED IDEOGRAPH
    {0xCDA2, 0x7942}, //10809 #CJK UNIFIED IDEOGRAPH
    {0xCDA3, 0x793F}, //10810 #CJK UNIFIED IDEOGRAPH
    {0xCDA4, 0x79C5}, //10811 #CJK UNIFIED IDEOGRAPH
    {0xCDA5, 0x7A78}, //10812 #CJK UNIFIED IDEOGRAPH
    {0xCDA6, 0x7A7B}, //10813 #CJK UNIFIED IDEOGRAPH
    {0xCDA7, 0x7AFB}, //10814 #CJK UNIFIED IDEOGRAPH
    {0xCDA8, 0x7C75}, //10815 #CJK UNIFIED IDEOGRAPH
    {0xCDA9, 0x7CFD}, //10816 #CJK UNIFIED IDEOGRAPH
    {0xCDAA, 0x8035}, //10817 #CJK UNIFIED IDEOGRAPH
    {0xCDAB, 0x808F}, //10818 #CJK UNIFIED IDEOGRAPH
    {0xCDAC, 0x80AE}, //10819 #CJK UNIFIED IDEOGRAPH
    {0xCDAD, 0x80A3}, //10820 #CJK UNIFIED IDEOGRAPH
    {0xCDAE, 0x80B8}, //10821 #CJK UNIFIED IDEOGRAPH
    {0xCDAF, 0x80B5}, //10822 #CJK UNIFIED IDEOGRAPH
    {0xCDB0, 0x80AD}, //10823 #CJK UNIFIED IDEOGRAPH
    {0xCDB1, 0x8220}, //10824 #CJK UNIFIED IDEOGRAPH
    {0xCDB2, 0x82A0}, //10825 #CJK UNIFIED IDEOGRAPH
    {0xCDB3, 0x82C0}, //10826 #CJK UNIFIED IDEOGRAPH
    {0xCDB4, 0x82AB}, //10827 #CJK UNIFIED IDEOGRAPH
    {0xCDB5, 0x829A}, //10828 #CJK UNIFIED IDEOGRAPH
    {0xCDB6, 0x8298}, //10829 #CJK UNIFIED IDEOGRAPH
    {0xCDB7, 0x829B}, //10830 #CJK UNIFIED IDEOGRAPH
    {0xCDB8, 0x82B5}, //10831 #CJK UNIFIED IDEOGRAPH
    {0xCDB9, 0x82A7}, //10832 #CJK UNIFIED IDEOGRAPH
    {0xCDBA, 0x82AE}, //10833 #CJK UNIFIED IDEOGRAPH
    {0xCDBB, 0x82BC}, //10834 #CJK UNIFIED IDEOGRAPH
    {0xCDBC, 0x829E}, //10835 #CJK UNIFIED IDEOGRAPH
    {0xCDBD, 0x82BA}, //10836 #CJK UNIFIED IDEOGRAPH
    {0xCDBE, 0x82B4}, //10837 #CJK UNIFIED IDEOGRAPH
    {0xCDBF, 0x82A8}, //10838 #CJK UNIFIED IDEOGRAPH
    {0xCDC0, 0x82A1}, //10839 #CJK UNIFIED IDEOGRAPH
    {0xCDC1, 0x82A9}, //10840 #CJK UNIFIED IDEOGRAPH
    {0xCDC2, 0x82C2}, //10841 #CJK UNIFIED IDEOGRAPH
    {0xCDC3, 0x82A4}, //10842 #CJK UNIFIED IDEOGRAPH
    {0xCDC4, 0x82C3}, //10843 #CJK UNIFIED IDEOGRAPH
    {0xCDC5, 0x82B6}, //10844 #CJK UNIFIED IDEOGRAPH
    {0xCDC6, 0x82A2}, //10845 #CJK UNIFIED IDEOGRAPH
    {0xCDC7, 0x8670}, //10846 #CJK UNIFIED IDEOGRAPH
    {0xCDC8, 0x866F}, //10847 #CJK UNIFIED IDEOGRAPH
    {0xCDC9, 0x866D}, //10848 #CJK UNIFIED IDEOGRAPH
    {0xCDCA, 0x866E}, //10849 #CJK UNIFIED IDEOGRAPH
    {0xCDCB, 0x8C56}, //10850 #CJK UNIFIED IDEOGRAPH
    {0xCDCC, 0x8FD2}, //10851 #CJK UNIFIED IDEOGRAPH
    {0xCDCD, 0x8FCB}, //10852 #CJK UNIFIED IDEOGRAPH
    {0xCDCE, 0x8FD3}, //10853 #CJK UNIFIED IDEOGRAPH
    {0xCDCF, 0x8FCD}, //10854 #CJK UNIFIED IDEOGRAPH
    {0xCDD0, 0x8FD6}, //10855 #CJK UNIFIED IDEOGRAPH
    {0xCDD1, 0x8FD5}, //10856 #CJK UNIFIED IDEOGRAPH
    {0xCDD2, 0x8FD7}, //10857 #CJK UNIFIED IDEOGRAPH
    {0xCDD3, 0x90B2}, //10858 #CJK UNIFIED IDEOGRAPH
    {0xCDD4, 0x90B4}, //10859 #CJK UNIFIED IDEOGRAPH
    {0xCDD5, 0x90AF}, //10860 #CJK UNIFIED IDEOGRAPH
    {0xCDD6, 0x90B3}, //10861 #CJK UNIFIED IDEOGRAPH
    {0xCDD7, 0x90B0}, //10862 #CJK UNIFIED IDEOGRAPH
    {0xCDD8, 0x9639}, //10863 #CJK UNIFIED IDEOGRAPH
    {0xCDD9, 0x963D}, //10864 #CJK UNIFIED IDEOGRAPH
    {0xCDDA, 0x963C}, //10865 #CJK UNIFIED IDEOGRAPH
    {0xCDDB, 0x963A}, //10866 #CJK UNIFIED IDEOGRAPH
    {0xCDDC, 0x9643}, //10867 #CJK UNIFIED IDEOGRAPH
    {0xCDDD, 0x4FCD}, //10868 #CJK UNIFIED IDEOGRAPH
    {0xCDDE, 0x4FC5}, //10869 #CJK UNIFIED IDEOGRAPH
    {0xCDDF, 0x4FD3}, //10870 #CJK UNIFIED IDEOGRAPH
    {0xCDE0, 0x4FB2}, //10871 #CJK UNIFIED IDEOGRAPH
    {0xCDE1, 0x4FC9}, //10872 #CJK UNIFIED IDEOGRAPH
    {0xCDE2, 0x4FCB}, //10873 #CJK UNIFIED IDEOGRAPH
    {0xCDE3, 0x4FC1}, //10874 #CJK UNIFIED IDEOGRAPH
    {0xCDE4, 0x4FD4}, //10875 #CJK UNIFIED IDEOGRAPH
    {0xCDE5, 0x4FDC}, //10876 #CJK UNIFIED IDEOGRAPH
    {0xCDE6, 0x4FD9}, //10877 #CJK UNIFIED IDEOGRAPH
    {0xCDE7, 0x4FBB}, //10878 #CJK UNIFIED IDEOGRAPH
    {0xCDE8, 0x4FB3}, //10879 #CJK UNIFIED IDEOGRAPH
    {0xCDE9, 0x4FDB}, //10880 #CJK UNIFIED IDEOGRAPH
    {0xCDEA, 0x4FC7}, //10881 #CJK UNIFIED IDEOGRAPH
    {0xCDEB, 0x4FD6}, //10882 #CJK UNIFIED IDEOGRAPH
    {0xCDEC, 0x4FBA}, //10883 #CJK UNIFIED IDEOGRAPH
    {0xCDED, 0x4FC0}, //10884 #CJK UNIFIED IDEOGRAPH
    {0xCDEE, 0x4FB9}, //10885 #CJK UNIFIED IDEOGRAPH
    {0xCDEF, 0x4FEC}, //10886 #CJK UNIFIED IDEOGRAPH
    {0xCDF0, 0x5244}, //10887 #CJK UNIFIED IDEOGRAPH
    {0xCDF1, 0x5249}, //10888 #CJK UNIFIED IDEOGRAPH
    {0xCDF2, 0x52C0}, //10889 #CJK UNIFIED IDEOGRAPH
    {0xCDF3, 0x52C2}, //10890 #CJK UNIFIED IDEOGRAPH
    {0xCDF4, 0x533D}, //10891 #CJK UNIFIED IDEOGRAPH
    {0xCDF5, 0x537C}, //10892 #CJK UNIFIED IDEOGRAPH
    {0xCDF6, 0x5397}, //10893 #CJK UNIFIED IDEOGRAPH
    {0xCDF7, 0x5396}, //10894 #CJK UNIFIED IDEOGRAPH
    {0xCDF8, 0x5399}, //10895 #CJK UNIFIED IDEOGRAPH
    {0xCDF9, 0x5398}, //10896 #CJK UNIFIED IDEOGRAPH
    {0xCDFA, 0x54BA}, //10897 #CJK UNIFIED IDEOGRAPH
    {0xCDFB, 0x54A1}, //10898 #CJK UNIFIED IDEOGRAPH
    {0xCDFC, 0x54AD}, //10899 #CJK UNIFIED IDEOGRAPH
    {0xCDFD, 0x54A5}, //10900 #CJK UNIFIED IDEOGRAPH
    {0xCDFE, 0x54CF}, //10901 #CJK UNIFIED IDEOGRAPH
    {0xCE40, 0x54C3}, //10902 #CJK UNIFIED IDEOGRAPH
    {0xCE41, 0x830D}, //10903 #CJK UNIFIED IDEOGRAPH
    {0xCE42, 0x54B7}, //10904 #CJK UNIFIED IDEOGRAPH
    {0xCE43, 0x54AE}, //10905 #CJK UNIFIED IDEOGRAPH
    {0xCE44, 0x54D6}, //10906 #CJK UNIFIED IDEOGRAPH
    {0xCE45, 0x54B6}, //10907 #CJK UNIFIED IDEOGRAPH
    {0xCE46, 0x54C5}, //10908 #CJK UNIFIED IDEOGRAPH
    {0xCE47, 0x54C6}, //10909 #CJK UNIFIED IDEOGRAPH
    {0xCE48, 0x54A0}, //10910 #CJK UNIFIED IDEOGRAPH
    {0xCE49, 0x5470}, //10911 #CJK UNIFIED IDEOGRAPH
    {0xCE4A, 0x54BC}, //10912 #CJK UNIFIED IDEOGRAPH
    {0xCE4B, 0x54A2}, //10913 #CJK UNIFIED IDEOGRAPH
    {0xCE4C, 0x54BE}, //10914 #CJK UNIFIED IDEOGRAPH
    {0xCE4D, 0x5472}, //10915 #CJK UNIFIED IDEOGRAPH
    {0xCE4E, 0x54DE}, //10916 #CJK UNIFIED IDEOGRAPH
    {0xCE4F, 0x54B0}, //10917 #CJK UNIFIED IDEOGRAPH
    {0xCE50, 0x57B5}, //10918 #CJK UNIFIED IDEOGRAPH
    {0xCE51, 0x579E}, //10919 #CJK UNIFIED IDEOGRAPH
    {0xCE52, 0x579F}, //10920 #CJK UNIFIED IDEOGRAPH
    {0xCE53, 0x57A4}, //10921 #CJK UNIFIED IDEOGRAPH
    {0xCE54, 0x578C}, //10922 #CJK UNIFIED IDEOGRAPH
    {0xCE55, 0x5797}, //10923 #CJK UNIFIED IDEOGRAPH
    {0xCE56, 0x579D}, //10924 #CJK UNIFIED IDEOGRAPH
    {0xCE57, 0x579B}, //10925 #CJK UNIFIED IDEOGRAPH
    {0xCE58, 0x5794}, //10926 #CJK UNIFIED IDEOGRAPH
    {0xCE59, 0x5798}, //10927 #CJK UNIFIED IDEOGRAPH
    {0xCE5A, 0x578F}, //10928 #CJK UNIFIED IDEOGRAPH
    {0xCE5B, 0x5799}, //10929 #CJK UNIFIED IDEOGRAPH
    {0xCE5C, 0x57A5}, //10930 #CJK UNIFIED IDEOGRAPH
    {0xCE5D, 0x579A}, //10931 #CJK UNIFIED IDEOGRAPH
    {0xCE5E, 0x5795}, //10932 #CJK UNIFIED IDEOGRAPH
    {0xCE5F, 0x58F4}, //10933 #CJK UNIFIED IDEOGRAPH
    {0xCE60, 0x590D}, //10934 #CJK UNIFIED IDEOGRAPH
    {0xCE61, 0x5953}, //10935 #CJK UNIFIED IDEOGRAPH
    {0xCE62, 0x59E1}, //10936 #CJK UNIFIED IDEOGRAPH
    {0xCE63, 0x59DE}, //10937 #CJK UNIFIED IDEOGRAPH
    {0xCE64, 0x59EE}, //10938 #CJK UNIFIED IDEOGRAPH
    {0xCE65, 0x5A00}, //10939 #CJK UNIFIED IDEOGRAPH
    {0xCE66, 0x59F1}, //10940 #CJK UNIFIED IDEOGRAPH
    {0xCE67, 0x59DD}, //10941 #CJK UNIFIED IDEOGRAPH
    {0xCE68, 0x59FA}, //10942 #CJK UNIFIED IDEOGRAPH
    {0xCE69, 0x59FD}, //10943 #CJK UNIFIED IDEOGRAPH
    {0xCE6A, 0x59FC}, //10944 #CJK UNIFIED IDEOGRAPH
    {0xCE6B, 0x59F6}, //10945 #CJK UNIFIED IDEOGRAPH
    {0xCE6C, 0x59E4}, //10946 #CJK UNIFIED IDEOGRAPH
    {0xCE6D, 0x59F2}, //10947 #CJK UNIFIED IDEOGRAPH
    {0xCE6E, 0x59F7}, //10948 #CJK UNIFIED IDEOGRAPH
    {0xCE6F, 0x59DB}, //10949 #CJK UNIFIED IDEOGRAPH
    {0xCE70, 0x59E9}, //10950 #CJK UNIFIED IDEOGRAPH
    {0xCE71, 0x59F3}, //10951 #CJK UNIFIED IDEOGRAPH
    {0xCE72, 0x59F5}, //10952 #CJK UNIFIED IDEOGRAPH
    {0xCE73, 0x59E0}, //10953 #CJK UNIFIED IDEOGRAPH
    {0xCE74, 0x59FE}, //10954 #CJK UNIFIED IDEOGRAPH
    {0xCE75, 0x59F4}, //10955 #CJK UNIFIED IDEOGRAPH
    {0xCE76, 0x59ED}, //10956 #CJK UNIFIED IDEOGRAPH
    {0xCE77, 0x5BA8}, //10957 #CJK UNIFIED IDEOGRAPH
    {0xCE78, 0x5C4C}, //10958 #CJK UNIFIED IDEOGRAPH
    {0xCE79, 0x5CD0}, //10959 #CJK UNIFIED IDEOGRAPH
    {0xCE7A, 0x5CD8}, //10960 #CJK UNIFIED IDEOGRAPH
    {0xCE7B, 0x5CCC}, //10961 #CJK UNIFIED IDEOGRAPH
    {0xCE7C, 0x5CD7}, //10962 #CJK UNIFIED IDEOGRAPH
    {0xCE7D, 0x5CCB}, //10963 #CJK UNIFIED IDEOGRAPH
    {0xCE7E, 0x5CDB}, //10964 #CJK UNIFIED IDEOGRAPH
    {0xCEA1, 0x5CDE}, //10965 #CJK UNIFIED IDEOGRAPH
    {0xCEA2, 0x5CDA}, //10966 #CJK UNIFIED IDEOGRAPH
    {0xCEA3, 0x5CC9}, //10967 #CJK UNIFIED IDEOGRAPH
    {0xCEA4, 0x5CC7}, //10968 #CJK UNIFIED IDEOGRAPH
    {0xCEA5, 0x5CCA}, //10969 #CJK UNIFIED IDEOGRAPH
    {0xCEA6, 0x5CD6}, //10970 #CJK UNIFIED IDEOGRAPH
    {0xCEA7, 0x5CD3}, //10971 #CJK UNIFIED IDEOGRAPH
    {0xCEA8, 0x5CD4}, //10972 #CJK UNIFIED IDEOGRAPH
    {0xCEA9, 0x5CCF}, //10973 #CJK UNIFIED IDEOGRAPH
    {0xCEAA, 0x5CC8}, //10974 #CJK UNIFIED IDEOGRAPH
    {0xCEAB, 0x5CC6}, //10975 #CJK UNIFIED IDEOGRAPH
    {0xCEAC, 0x5CCE}, //10976 #CJK UNIFIED IDEOGRAPH
    {0xCEAD, 0x5CDF}, //10977 #CJK UNIFIED IDEOGRAPH
    {0xCEAE, 0x5CF8}, //10978 #CJK UNIFIED IDEOGRAPH
    {0xCEAF, 0x5DF9}, //10979 #CJK UNIFIED IDEOGRAPH
    {0xCEB0, 0x5E21}, //10980 #CJK UNIFIED IDEOGRAPH
    {0xCEB1, 0x5E22}, //10981 #CJK UNIFIED IDEOGRAPH
    {0xCEB2, 0x5E23}, //10982 #CJK UNIFIED IDEOGRAPH
    {0xCEB3, 0x5E20}, //10983 #CJK UNIFIED IDEOGRAPH
    {0xCEB4, 0x5E24}, //10984 #CJK UNIFIED IDEOGRAPH
    {0xCEB5, 0x5EB0}, //10985 #CJK UNIFIED IDEOGRAPH
    {0xCEB6, 0x5EA4}, //10986 #CJK UNIFIED IDEOGRAPH
    {0xCEB7, 0x5EA2}, //10987 #CJK UNIFIED IDEOGRAPH
    {0xCEB8, 0x5E9B}, //10988 #CJK UNIFIED IDEOGRAPH
    {0xCEB9, 0x5EA3}, //10989 #CJK UNIFIED IDEOGRAPH
    {0xCEBA, 0x5EA5}, //10990 #CJK UNIFIED IDEOGRAPH
    {0xCEBB, 0x5F07}, //10991 #CJK UNIFIED IDEOGRAPH
    {0xCEBC, 0x5F2E}, //10992 #CJK UNIFIED IDEOGRAPH
    {0xCEBD, 0x5F56}, //10993 #CJK UNIFIED IDEOGRAPH
    {0xCEBE, 0x5F86}, //10994 #CJK UNIFIED IDEOGRAPH
    {0xCEBF, 0x6037}, //10995 #CJK UNIFIED IDEOGRAPH
    {0xCEC0, 0x6039}, //10996 #CJK UNIFIED IDEOGRAPH
    {0xCEC1, 0x6054}, //10997 #CJK UNIFIED IDEOGRAPH
    {0xCEC2, 0x6072}, //10998 #CJK UNIFIED IDEOGRAPH
    {0xCEC3, 0x605E}, //10999 #CJK UNIFIED IDEOGRAPH
    {0xCEC4, 0x6045}, //11000 #CJK UNIFIED IDEOGRAPH
    {0xCEC5, 0x6053}, //11001 #CJK UNIFIED IDEOGRAPH
    {0xCEC6, 0x6047}, //11002 #CJK UNIFIED IDEOGRAPH
    {0xCEC7, 0x6049}, //11003 #CJK UNIFIED IDEOGRAPH
    {0xCEC8, 0x605B}, //11004 #CJK UNIFIED IDEOGRAPH
    {0xCEC9, 0x604C}, //11005 #CJK UNIFIED IDEOGRAPH
    {0xCECA, 0x6040}, //11006 #CJK UNIFIED IDEOGRAPH
    {0xCECB, 0x6042}, //11007 #CJK UNIFIED IDEOGRAPH
    {0xCECC, 0x605F}, //11008 #CJK UNIFIED IDEOGRAPH
    {0xCECD, 0x6024}, //11009 #CJK UNIFIED IDEOGRAPH
    {0xCECE, 0x6044}, //11010 #CJK UNIFIED IDEOGRAPH
    {0xCECF, 0x6058}, //11011 #CJK UNIFIED IDEOGRAPH
    {0xCED0, 0x6066}, //11012 #CJK UNIFIED IDEOGRAPH
    {0xCED1, 0x606E}, //11013 #CJK UNIFIED IDEOGRAPH
    {0xCED2, 0x6242}, //11014 #CJK UNIFIED IDEOGRAPH
    {0xCED3, 0x6243}, //11015 #CJK UNIFIED IDEOGRAPH
    {0xCED4, 0x62CF}, //11016 #CJK UNIFIED IDEOGRAPH
    {0xCED5, 0x630D}, //11017 #CJK UNIFIED IDEOGRAPH
    {0xCED6, 0x630B}, //11018 #CJK UNIFIED IDEOGRAPH
    {0xCED7, 0x62F5}, //11019 #CJK UNIFIED IDEOGRAPH
    {0xCED8, 0x630E}, //11020 #CJK UNIFIED IDEOGRAPH
    {0xCED9, 0x6303}, //11021 #CJK UNIFIED IDEOGRAPH
    {0xCEDA, 0x62EB}, //11022 #CJK UNIFIED IDEOGRAPH
    {0xCEDB, 0x62F9}, //11023 #CJK UNIFIED IDEOGRAPH
    {0xCEDC, 0x630F}, //11024 #CJK UNIFIED IDEOGRAPH
    {0xCEDD, 0x630C}, //11025 #CJK UNIFIED IDEOGRAPH
    {0xCEDE, 0x62F8}, //11026 #CJK UNIFIED IDEOGRAPH
    {0xCEDF, 0x62F6}, //11027 #CJK UNIFIED IDEOGRAPH
    {0xCEE0, 0x6300}, //11028 #CJK UNIFIED IDEOGRAPH
    {0xCEE1, 0x6313}, //11029 #CJK UNIFIED IDEOGRAPH
    {0xCEE2, 0x6314}, //11030 #CJK UNIFIED IDEOGRAPH
    {0xCEE3, 0x62FA}, //11031 #CJK UNIFIED IDEOGRAPH
    {0xCEE4, 0x6315}, //11032 #CJK UNIFIED IDEOGRAPH
    {0xCEE5, 0x62FB}, //11033 #CJK UNIFIED IDEOGRAPH
    {0xCEE6, 0x62F0}, //11034 #CJK UNIFIED IDEOGRAPH
    {0xCEE7, 0x6541}, //11035 #CJK UNIFIED IDEOGRAPH
    {0xCEE8, 0x6543}, //11036 #CJK UNIFIED IDEOGRAPH
    {0xCEE9, 0x65AA}, //11037 #CJK UNIFIED IDEOGRAPH
    {0xCEEA, 0x65BF}, //11038 #CJK UNIFIED IDEOGRAPH
    {0xCEEB, 0x6636}, //11039 #CJK UNIFIED IDEOGRAPH
    {0xCEEC, 0x6621}, //11040 #CJK UNIFIED IDEOGRAPH
    {0xCEED, 0x6632}, //11041 #CJK UNIFIED IDEOGRAPH
    {0xCEEE, 0x6635}, //11042 #CJK UNIFIED IDEOGRAPH
    {0xCEEF, 0x661C}, //11043 #CJK UNIFIED IDEOGRAPH
    {0xCEF0, 0x6626}, //11044 #CJK UNIFIED IDEOGRAPH
    {0xCEF1, 0x6622}, //11045 #CJK UNIFIED IDEOGRAPH
    {0xCEF2, 0x6633}, //11046 #CJK UNIFIED IDEOGRAPH
    {0xCEF3, 0x662B}, //11047 #CJK UNIFIED IDEOGRAPH
    {0xCEF4, 0x663A}, //11048 #CJK UNIFIED IDEOGRAPH
    {0xCEF5, 0x661D}, //11049 #CJK UNIFIED IDEOGRAPH
    {0xCEF6, 0x6634}, //11050 #CJK UNIFIED IDEOGRAPH
    {0xCEF7, 0x6639}, //11051 #CJK UNIFIED IDEOGRAPH
    {0xCEF8, 0x662E}, //11052 #CJK UNIFIED IDEOGRAPH
    {0xCEF9, 0x670F}, //11053 #CJK UNIFIED IDEOGRAPH
    {0xCEFA, 0x6710}, //11054 #CJK UNIFIED IDEOGRAPH
    {0xCEFB, 0x67C1}, //11055 #CJK UNIFIED IDEOGRAPH
    {0xCEFC, 0x67F2}, //11056 #CJK UNIFIED IDEOGRAPH
    {0xCEFD, 0x67C8}, //11057 #CJK UNIFIED IDEOGRAPH
    {0xCEFE, 0x67BA}, //11058 #CJK UNIFIED IDEOGRAPH
    {0xCF40, 0x67DC}, //11059 #CJK UNIFIED IDEOGRAPH
    {0xCF41, 0x67BB}, //11060 #CJK UNIFIED IDEOGRAPH
    {0xCF42, 0x67F8}, //11061 #CJK UNIFIED IDEOGRAPH
    {0xCF43, 0x67D8}, //11062 #CJK UNIFIED IDEOGRAPH
    {0xCF44, 0x67C0}, //11063 #CJK UNIFIED IDEOGRAPH
    {0xCF45, 0x67B7}, //11064 #CJK UNIFIED IDEOGRAPH
    {0xCF46, 0x67C5}, //11065 #CJK UNIFIED IDEOGRAPH
    {0xCF47, 0x67EB}, //11066 #CJK UNIFIED IDEOGRAPH
    {0xCF48, 0x67E4}, //11067 #CJK UNIFIED IDEOGRAPH
    {0xCF49, 0x67DF}, //11068 #CJK UNIFIED IDEOGRAPH
    {0xCF4A, 0x67B5}, //11069 #CJK UNIFIED IDEOGRAPH
    {0xCF4B, 0x67CD}, //11070 #CJK UNIFIED IDEOGRAPH
    {0xCF4C, 0x67B3}, //11071 #CJK UNIFIED IDEOGRAPH
    {0xCF4D, 0x67F7}, //11072 #CJK UNIFIED IDEOGRAPH
    {0xCF4E, 0x67F6}, //11073 #CJK UNIFIED IDEOGRAPH
    {0xCF4F, 0x67EE}, //11074 #CJK UNIFIED IDEOGRAPH
    {0xCF50, 0x67E3}, //11075 #CJK UNIFIED IDEOGRAPH
    {0xCF51, 0x67C2}, //11076 #CJK UNIFIED IDEOGRAPH
    {0xCF52, 0x67B9}, //11077 #CJK UNIFIED IDEOGRAPH
    {0xCF53, 0x67CE}, //11078 #CJK UNIFIED IDEOGRAPH
    {0xCF54, 0x67E7}, //11079 #CJK UNIFIED IDEOGRAPH
    {0xCF55, 0x67F0}, //11080 #CJK UNIFIED IDEOGRAPH
    {0xCF56, 0x67B2}, //11081 #CJK UNIFIED IDEOGRAPH
    {0xCF57, 0x67FC}, //11082 #CJK UNIFIED IDEOGRAPH
    {0xCF58, 0x67C6}, //11083 #CJK UNIFIED IDEOGRAPH
    {0xCF59, 0x67ED}, //11084 #CJK UNIFIED IDEOGRAPH
    {0xCF5A, 0x67CC}, //11085 #CJK UNIFIED IDEOGRAPH
    {0xCF5B, 0x67AE}, //11086 #CJK UNIFIED IDEOGRAPH
    {0xCF5C, 0x67E6}, //11087 #CJK UNIFIED IDEOGRAPH
    {0xCF5D, 0x67DB}, //11088 #CJK UNIFIED IDEOGRAPH
    {0xCF5E, 0x67FA}, //11089 #CJK UNIFIED IDEOGRAPH
    {0xCF5F, 0x67C9}, //11090 #CJK UNIFIED IDEOGRAPH
    {0xCF60, 0x67CA}, //11091 #CJK UNIFIED IDEOGRAPH
    {0xCF61, 0x67C3}, //11092 #CJK UNIFIED IDEOGRAPH
    {0xCF62, 0x67EA}, //11093 #CJK UNIFIED IDEOGRAPH
    {0xCF63, 0x67CB}, //11094 #CJK UNIFIED IDEOGRAPH
    {0xCF64, 0x6B28}, //11095 #CJK UNIFIED IDEOGRAPH
    {0xCF65, 0x6B82}, //11096 #CJK UNIFIED IDEOGRAPH
    {0xCF66, 0x6B84}, //11097 #CJK UNIFIED IDEOGRAPH
    {0xCF67, 0x6BB6}, //11098 #CJK UNIFIED IDEOGRAPH
    {0xCF68, 0x6BD6}, //11099 #CJK UNIFIED IDEOGRAPH
    {0xCF69, 0x6BD8}, //11100 #CJK UNIFIED IDEOGRAPH
    {0xCF6A, 0x6BE0}, //11101 #CJK UNIFIED IDEOGRAPH
    {0xCF6B, 0x6C20}, //11102 #CJK UNIFIED IDEOGRAPH
    {0xCF6C, 0x6C21}, //11103 #CJK UNIFIED IDEOGRAPH
    {0xCF6D, 0x6D28}, //11104 #CJK UNIFIED IDEOGRAPH
    {0xCF6E, 0x6D34}, //11105 #CJK UNIFIED IDEOGRAPH
    {0xCF6F, 0x6D2D}, //11106 #CJK UNIFIED IDEOGRAPH
    {0xCF70, 0x6D1F}, //11107 #CJK UNIFIED IDEOGRAPH
    {0xCF71, 0x6D3C}, //11108 #CJK UNIFIED IDEOGRAPH
    {0xCF72, 0x6D3F}, //11109 #CJK UNIFIED IDEOGRAPH
    {0xCF73, 0x6D12}, //11110 #CJK UNIFIED IDEOGRAPH
    {0xCF74, 0x6D0A}, //11111 #CJK UNIFIED IDEOGRAPH
    {0xCF75, 0x6CDA}, //11112 #CJK UNIFIED IDEOGRAPH
    {0xCF76, 0x6D33}, //11113 #CJK UNIFIED IDEOGRAPH
    {0xCF77, 0x6D04}, //11114 #CJK UNIFIED IDEOGRAPH
    {0xCF78, 0x6D19}, //11115 #CJK UNIFIED IDEOGRAPH
    {0xCF79, 0x6D3A}, //11116 #CJK UNIFIED IDEOGRAPH
    {0xCF7A, 0x6D1A}, //11117 #CJK UNIFIED IDEOGRAPH
    {0xCF7B, 0x6D11}, //11118 #CJK UNIFIED IDEOGRAPH
    {0xCF7C, 0x6D00}, //11119 #CJK UNIFIED IDEOGRAPH
    {0xCF7D, 0x6D1D}, //11120 #CJK UNIFIED IDEOGRAPH
    {0xCF7E, 0x6D42}, //11121 #CJK UNIFIED IDEOGRAPH
    {0xCFA1, 0x6D01}, //11122 #CJK UNIFIED IDEOGRAPH
    {0xCFA2, 0x6D18}, //11123 #CJK UNIFIED IDEOGRAPH
    {0xCFA3, 0x6D37}, //11124 #CJK UNIFIED IDEOGRAPH
    {0xCFA4, 0x6D03}, //11125 #CJK UNIFIED IDEOGRAPH
    {0xCFA5, 0x6D0F}, //11126 #CJK UNIFIED IDEOGRAPH
    {0xCFA6, 0x6D40}, //11127 #CJK UNIFIED IDEOGRAPH
    {0xCFA7, 0x6D07}, //11128 #CJK UNIFIED IDEOGRAPH
    {0xCFA8, 0x6D20}, //11129 #CJK UNIFIED IDEOGRAPH
    {0xCFA9, 0x6D2C}, //11130 #CJK UNIFIED IDEOGRAPH
    {0xCFAA, 0x6D08}, //11131 #CJK UNIFIED IDEOGRAPH
    {0xCFAB, 0x6D22}, //11132 #CJK UNIFIED IDEOGRAPH
    {0xCFAC, 0x6D09}, //11133 #CJK UNIFIED IDEOGRAPH
    {0xCFAD, 0x6D10}, //11134 #CJK UNIFIED IDEOGRAPH
    {0xCFAE, 0x70B7}, //11135 #CJK UNIFIED IDEOGRAPH
    {0xCFAF, 0x709F}, //11136 #CJK UNIFIED IDEOGRAPH
    {0xCFB0, 0x70BE}, //11137 #CJK UNIFIED IDEOGRAPH
    {0xCFB1, 0x70B1}, //11138 #CJK UNIFIED IDEOGRAPH
    {0xCFB2, 0x70B0}, //11139 #CJK UNIFIED IDEOGRAPH
    {0xCFB3, 0x70A1}, //11140 #CJK UNIFIED IDEOGRAPH
    {0xCFB4, 0x70B4}, //11141 #CJK UNIFIED IDEOGRAPH
    {0xCFB5, 0x70B5}, //11142 #CJK UNIFIED IDEOGRAPH
    {0xCFB6, 0x70A9}, //11143 #CJK UNIFIED IDEOGRAPH
    {0xCFB7, 0x7241}, //11144 #CJK UNIFIED IDEOGRAPH
    {0xCFB8, 0x7249}, //11145 #CJK UNIFIED IDEOGRAPH
    {0xCFB9, 0x724A}, //11146 #CJK UNIFIED IDEOGRAPH
    {0xCFBA, 0x726C}, //11147 #CJK UNIFIED IDEOGRAPH
    {0xCFBB, 0x7270}, //11148 #CJK UNIFIED IDEOGRAPH
    {0xCFBC, 0x7273}, //11149 #CJK UNIFIED IDEOGRAPH
    {0xCFBD, 0x726E}, //11150 #CJK UNIFIED IDEOGRAPH
    {0xCFBE, 0x72CA}, //11151 #CJK UNIFIED IDEOGRAPH
    {0xCFBF, 0x72E4}, //11152 #CJK UNIFIED IDEOGRAPH
    {0xCFC0, 0x72E8}, //11153 #CJK UNIFIED IDEOGRAPH
    {0xCFC1, 0x72EB}, //11154 #CJK UNIFIED IDEOGRAPH
    {0xCFC2, 0x72DF}, //11155 #CJK UNIFIED IDEOGRAPH
    {0xCFC3, 0x72EA}, //11156 #CJK UNIFIED IDEOGRAPH
    {0xCFC4, 0x72E6}, //11157 #CJK UNIFIED IDEOGRAPH
    {0xCFC5, 0x72E3}, //11158 #CJK UNIFIED IDEOGRAPH
    {0xCFC6, 0x7385}, //11159 #CJK UNIFIED IDEOGRAPH
    {0xCFC7, 0x73CC}, //11160 #CJK UNIFIED IDEOGRAPH
    {0xCFC8, 0x73C2}, //11161 #CJK UNIFIED IDEOGRAPH
    {0xCFC9, 0x73C8}, //11162 #CJK UNIFIED IDEOGRAPH
    {0xCFCA, 0x73C5}, //11163 #CJK UNIFIED IDEOGRAPH
    {0xCFCB, 0x73B9}, //11164 #CJK UNIFIED IDEOGRAPH
    {0xCFCC, 0x73B6}, //11165 #CJK UNIFIED IDEOGRAPH
    {0xCFCD, 0x73B5}, //11166 #CJK UNIFIED IDEOGRAPH
    {0xCFCE, 0x73B4}, //11167 #CJK UNIFIED IDEOGRAPH
    {0xCFCF, 0x73EB}, //11168 #CJK UNIFIED IDEOGRAPH
    {0xCFD0, 0x73BF}, //11169 #CJK UNIFIED IDEOGRAPH
    {0xCFD1, 0x73C7}, //11170 #CJK UNIFIED IDEOGRAPH
    {0xCFD2, 0x73BE}, //11171 #CJK UNIFIED IDEOGRAPH
    {0xCFD3, 0x73C3}, //11172 #CJK UNIFIED IDEOGRAPH
    {0xCFD4, 0x73C6}, //11173 #CJK UNIFIED IDEOGRAPH
    {0xCFD5, 0x73B8}, //11174 #CJK UNIFIED IDEOGRAPH
    {0xCFD6, 0x73CB}, //11175 #CJK UNIFIED IDEOGRAPH
    {0xCFD7, 0x74EC}, //11176 #CJK UNIFIED IDEOGRAPH
    {0xCFD8, 0x74EE}, //11177 #CJK UNIFIED IDEOGRAPH
    {0xCFD9, 0x752E}, //11178 #CJK UNIFIED IDEOGRAPH
    {0xCFDA, 0x7547}, //11179 #CJK UNIFIED IDEOGRAPH
    {0xCFDB, 0x7548}, //11180 #CJK UNIFIED IDEOGRAPH
    {0xCFDC, 0x75A7}, //11181 #CJK UNIFIED IDEOGRAPH
    {0xCFDD, 0x75AA}, //11182 #CJK UNIFIED IDEOGRAPH
    {0xCFDE, 0x7679}, //11183 #CJK UNIFIED IDEOGRAPH
    {0xCFDF, 0x76C4}, //11184 #CJK UNIFIED IDEOGRAPH
    {0xCFE0, 0x7708}, //11185 #CJK UNIFIED IDEOGRAPH
    {0xCFE1, 0x7703}, //11186 #CJK UNIFIED IDEOGRAPH
    {0xCFE2, 0x7704}, //11187 #CJK UNIFIED IDEOGRAPH
    {0xCFE3, 0x7705}, //11188 #CJK UNIFIED IDEOGRAPH
    {0xCFE4, 0x770A}, //11189 #CJK UNIFIED IDEOGRAPH
    {0xCFE5, 0x76F7}, //11190 #CJK UNIFIED IDEOGRAPH
    {0xCFE6, 0x76FB}, //11191 #CJK UNIFIED IDEOGRAPH
    {0xCFE7, 0x76FA}, //11192 #CJK UNIFIED IDEOGRAPH
    {0xCFE8, 0x77E7}, //11193 #CJK UNIFIED IDEOGRAPH
    {0xCFE9, 0x77E8}, //11194 #CJK UNIFIED IDEOGRAPH
    {0xCFEA, 0x7806}, //11195 #CJK UNIFIED IDEOGRAPH
    {0xCFEB, 0x7811}, //11196 #CJK UNIFIED IDEOGRAPH
    {0xCFEC, 0x7812}, //11197 #CJK UNIFIED IDEOGRAPH
    {0xCFED, 0x7805}, //11198 #CJK UNIFIED IDEOGRAPH
    {0xCFEE, 0x7810}, //11199 #CJK UNIFIED IDEOGRAPH
    {0xCFEF, 0x780F}, //11200 #CJK UNIFIED IDEOGRAPH
    {0xCFF0, 0x780E}, //11201 #CJK UNIFIED IDEOGRAPH
    {0xCFF1, 0x7809}, //11202 #CJK UNIFIED IDEOGRAPH
    {0xCFF2, 0x7803}, //11203 #CJK UNIFIED IDEOGRAPH
    {0xCFF3, 0x7813}, //11204 #CJK UNIFIED IDEOGRAPH
    {0xCFF4, 0x794A}, //11205 #CJK UNIFIED IDEOGRAPH
    {0xCFF5, 0x794C}, //11206 #CJK UNIFIED IDEOGRAPH
    {0xCFF6, 0x794B}, //11207 #CJK UNIFIED IDEOGRAPH
    {0xCFF7, 0x7945}, //11208 #CJK UNIFIED IDEOGRAPH
    {0xCFF8, 0x7944}, //11209 #CJK UNIFIED IDEOGRAPH
    {0xCFF9, 0x79D5}, //11210 #CJK UNIFIED IDEOGRAPH
    {0xCFFA, 0x79CD}, //11211 #CJK UNIFIED IDEOGRAPH
    {0xCFFB, 0x79CF}, //11212 #CJK UNIFIED IDEOGRAPH
    {0xCFFC, 0x79D6}, //11213 #CJK UNIFIED IDEOGRAPH
    {0xCFFD, 0x79CE}, //11214 #CJK UNIFIED IDEOGRAPH
    {0xCFFE, 0x7A80}, //11215 #CJK UNIFIED IDEOGRAPH
    {0xD040, 0x7A7E}, //11216 #CJK UNIFIED IDEOGRAPH
    {0xD041, 0x7AD1}, //11217 #CJK UNIFIED IDEOGRAPH
    {0xD042, 0x7B00}, //11218 #CJK UNIFIED IDEOGRAPH
    {0xD043, 0x7B01}, //11219 #CJK UNIFIED IDEOGRAPH
    {0xD044, 0x7C7A}, //11220 #CJK UNIFIED IDEOGRAPH
    {0xD045, 0x7C78}, //11221 #CJK UNIFIED IDEOGRAPH
    {0xD046, 0x7C79}, //11222 #CJK UNIFIED IDEOGRAPH
    {0xD047, 0x7C7F}, //11223 #CJK UNIFIED IDEOGRAPH
    {0xD048, 0x7C80}, //11224 #CJK UNIFIED IDEOGRAPH
    {0xD049, 0x7C81}, //11225 #CJK UNIFIED IDEOGRAPH
    {0xD04A, 0x7D03}, //11226 #CJK UNIFIED IDEOGRAPH
    {0xD04B, 0x7D08}, //11227 #CJK UNIFIED IDEOGRAPH
    {0xD04C, 0x7D01}, //11228 #CJK UNIFIED IDEOGRAPH
    {0xD04D, 0x7F58}, //11229 #CJK UNIFIED IDEOGRAPH
    {0xD04E, 0x7F91}, //11230 #CJK UNIFIED IDEOGRAPH
    {0xD04F, 0x7F8D}, //11231 #CJK UNIFIED IDEOGRAPH
    {0xD050, 0x7FBE}, //11232 #CJK UNIFIED IDEOGRAPH
    {0xD051, 0x8007}, //11233 #CJK UNIFIED IDEOGRAPH
    {0xD052, 0x800E}, //11234 #CJK UNIFIED IDEOGRAPH
    {0xD053, 0x800F}, //11235 #CJK UNIFIED IDEOGRAPH
    {0xD054, 0x8014}, //11236 #CJK UNIFIED IDEOGRAPH
    {0xD055, 0x8037}, //11237 #CJK UNIFIED IDEOGRAPH
    {0xD056, 0x80D8}, //11238 #CJK UNIFIED IDEOGRAPH
    {0xD057, 0x80C7}, //11239 #CJK UNIFIED IDEOGRAPH
    {0xD058, 0x80E0}, //11240 #CJK UNIFIED IDEOGRAPH
    {0xD059, 0x80D1}, //11241 #CJK UNIFIED IDEOGRAPH
    {0xD05A, 0x80C8}, //11242 #CJK UNIFIED IDEOGRAPH
    {0xD05B, 0x80C2}, //11243 #CJK UNIFIED IDEOGRAPH
    {0xD05C, 0x80D0}, //11244 #CJK UNIFIED IDEOGRAPH
    {0xD05D, 0x80C5}, //11245 #CJK UNIFIED IDEOGRAPH
    {0xD05E, 0x80E3}, //11246 #CJK UNIFIED IDEOGRAPH
    {0xD05F, 0x80D9}, //11247 #CJK UNIFIED IDEOGRAPH
    {0xD060, 0x80DC}, //11248 #CJK UNIFIED IDEOGRAPH
    {0xD061, 0x80CA}, //11249 #CJK UNIFIED IDEOGRAPH
    {0xD062, 0x80D5}, //11250 #CJK UNIFIED IDEOGRAPH
    {0xD063, 0x80C9}, //11251 #CJK UNIFIED IDEOGRAPH
    {0xD064, 0x80CF}, //11252 #CJK UNIFIED IDEOGRAPH
    {0xD065, 0x80D7}, //11253 #CJK UNIFIED IDEOGRAPH
    {0xD066, 0x80E6}, //11254 #CJK UNIFIED IDEOGRAPH
    {0xD067, 0x80CD}, //11255 #CJK UNIFIED IDEOGRAPH
    {0xD068, 0x81FF}, //11256 #CJK UNIFIED IDEOGRAPH
    {0xD069, 0x8221}, //11257 #CJK UNIFIED IDEOGRAPH
    {0xD06A, 0x8294}, //11258 #CJK UNIFIED IDEOGRAPH
    {0xD06B, 0x82D9}, //11259 #CJK UNIFIED IDEOGRAPH
    {0xD06C, 0x82FE}, //11260 #CJK UNIFIED IDEOGRAPH
    {0xD06D, 0x82F9}, //11261 #CJK UNIFIED IDEOGRAPH
    {0xD06E, 0x8307}, //11262 #CJK UNIFIED IDEOGRAPH
    {0xD06F, 0x82E8}, //11263 #CJK UNIFIED IDEOGRAPH
    {0xD070, 0x8300}, //11264 #CJK UNIFIED IDEOGRAPH
    {0xD071, 0x82D5}, //11265 #CJK UNIFIED IDEOGRAPH
    {0xD072, 0x833A}, //11266 #CJK UNIFIED IDEOGRAPH
    {0xD073, 0x82EB}, //11267 #CJK UNIFIED IDEOGRAPH
    {0xD074, 0x82D6}, //11268 #CJK UNIFIED IDEOGRAPH
    {0xD075, 0x82F4}, //11269 #CJK UNIFIED IDEOGRAPH
    {0xD076, 0x82EC}, //11270 #CJK UNIFIED IDEOGRAPH
    {0xD077, 0x82E1}, //11271 #CJK UNIFIED IDEOGRAPH
    {0xD078, 0x82F2}, //11272 #CJK UNIFIED IDEOGRAPH
    {0xD079, 0x82F5}, //11273 #CJK UNIFIED IDEOGRAPH
    {0xD07A, 0x830C}, //11274 #CJK UNIFIED IDEOGRAPH
    {0xD07B, 0x82FB}, //11275 #CJK UNIFIED IDEOGRAPH
    {0xD07C, 0x82F6}, //11276 #CJK UNIFIED IDEOGRAPH
    {0xD07D, 0x82F0}, //11277 #CJK UNIFIED IDEOGRAPH
    {0xD07E, 0x82EA}, //11278 #CJK UNIFIED IDEOGRAPH
    {0xD0A1, 0x82E4}, //11279 #CJK UNIFIED IDEOGRAPH
    {0xD0A2, 0x82E0}, //11280 #CJK UNIFIED IDEOGRAPH
    {0xD0A3, 0x82FA}, //11281 #CJK UNIFIED IDEOGRAPH
    {0xD0A4, 0x82F3}, //11282 #CJK UNIFIED IDEOGRAPH
    {0xD0A5, 0x82ED}, //11283 #CJK UNIFIED IDEOGRAPH
    {0xD0A6, 0x8677}, //11284 #CJK UNIFIED IDEOGRAPH
    {0xD0A7, 0x8674}, //11285 #CJK UNIFIED IDEOGRAPH
    {0xD0A8, 0x867C}, //11286 #CJK UNIFIED IDEOGRAPH
    {0xD0A9, 0x8673}, //11287 #CJK UNIFIED IDEOGRAPH
    {0xD0AA, 0x8841}, //11288 #CJK UNIFIED IDEOGRAPH
    {0xD0AB, 0x884E}, //11289 #CJK UNIFIED IDEOGRAPH
    {0xD0AC, 0x8867}, //11290 #CJK UNIFIED IDEOGRAPH
    {0xD0AD, 0x886A}, //11291 #CJK UNIFIED IDEOGRAPH
    {0xD0AE, 0x8869}, //11292 #CJK UNIFIED IDEOGRAPH
    {0xD0AF, 0x89D3}, //11293 #CJK UNIFIED IDEOGRAPH
    {0xD0B0, 0x8A04}, //11294 #CJK UNIFIED IDEOGRAPH
    {0xD0B1, 0x8A07}, //11295 #CJK UNIFIED IDEOGRAPH
    {0xD0B2, 0x8D72}, //11296 #CJK UNIFIED IDEOGRAPH
    {0xD0B3, 0x8FE3}, //11297 #CJK UNIFIED IDEOGRAPH
    {0xD0B4, 0x8FE1}, //11298 #CJK UNIFIED IDEOGRAPH
    {0xD0B5, 0x8FEE}, //11299 #CJK UNIFIED IDEOGRAPH
    {0xD0B6, 0x8FE0}, //11300 #CJK UNIFIED IDEOGRAPH
    {0xD0B7, 0x90F1}, //11301 #CJK UNIFIED IDEOGRAPH
    {0xD0B8, 0x90BD}, //11302 #CJK UNIFIED IDEOGRAPH
    {0xD0B9, 0x90BF}, //11303 #CJK UNIFIED IDEOGRAPH
    {0xD0BA, 0x90D5}, //11304 #CJK UNIFIED IDEOGRAPH
    {0xD0BB, 0x90C5}, //11305 #CJK UNIFIED IDEOGRAPH
    {0xD0BC, 0x90BE}, //11306 #CJK UNIFIED IDEOGRAPH
    {0xD0BD, 0x90C7}, //11307 #CJK UNIFIED IDEOGRAPH
    {0xD0BE, 0x90CB}, //11308 #CJK UNIFIED IDEOGRAPH
    {0xD0BF, 0x90C8}, //11309 #CJK UNIFIED IDEOGRAPH
    {0xD0C0, 0x91D4}, //11310 #CJK UNIFIED IDEOGRAPH
    {0xD0C1, 0x91D3}, //11311 #CJK UNIFIED IDEOGRAPH
    {0xD0C2, 0x9654}, //11312 #CJK UNIFIED IDEOGRAPH
    {0xD0C3, 0x964F}, //11313 #CJK UNIFIED IDEOGRAPH
    {0xD0C4, 0x9651}, //11314 #CJK UNIFIED IDEOGRAPH
    {0xD0C5, 0x9653}, //11315 #CJK UNIFIED IDEOGRAPH
    {0xD0C6, 0x964A}, //11316 #CJK UNIFIED IDEOGRAPH
    {0xD0C7, 0x964E}, //11317 #CJK UNIFIED IDEOGRAPH
    {0xD0C8, 0x501E}, //11318 #CJK UNIFIED IDEOGRAPH
    {0xD0C9, 0x5005}, //11319 #CJK UNIFIED IDEOGRAPH
    {0xD0CA, 0x5007}, //11320 #CJK UNIFIED IDEOGRAPH
    {0xD0CB, 0x5013}, //11321 #CJK UNIFIED IDEOGRAPH
    {0xD0CC, 0x5022}, //11322 #CJK UNIFIED IDEOGRAPH
    {0xD0CD, 0x5030}, //11323 #CJK UNIFIED IDEOGRAPH
    {0xD0CE, 0x501B}, //11324 #CJK UNIFIED IDEOGRAPH
    {0xD0CF, 0x4FF5}, //11325 #CJK UNIFIED IDEOGRAPH
    {0xD0D0, 0x4FF4}, //11326 #CJK UNIFIED IDEOGRAPH
    {0xD0D1, 0x5033}, //11327 #CJK UNIFIED IDEOGRAPH
    {0xD0D2, 0x5037}, //11328 #CJK UNIFIED IDEOGRAPH
    {0xD0D3, 0x502C}, //11329 #CJK UNIFIED IDEOGRAPH
    {0xD0D4, 0x4FF6}, //11330 #CJK UNIFIED IDEOGRAPH
    {0xD0D5, 0x4FF7}, //11331 #CJK UNIFIED IDEOGRAPH
    {0xD0D6, 0x5017}, //11332 #CJK UNIFIED IDEOGRAPH
    {0xD0D7, 0x501C}, //11333 #CJK UNIFIED IDEOGRAPH
    {0xD0D8, 0x5020}, //11334 #CJK UNIFIED IDEOGRAPH
    {0xD0D9, 0x5027}, //11335 #CJK UNIFIED IDEOGRAPH
    {0xD0DA, 0x5035}, //11336 #CJK UNIFIED IDEOGRAPH
    {0xD0DB, 0x502F}, //11337 #CJK UNIFIED IDEOGRAPH
    {0xD0DC, 0x5031}, //11338 #CJK UNIFIED IDEOGRAPH
    {0xD0DD, 0x500E}, //11339 #CJK UNIFIED IDEOGRAPH
    {0xD0DE, 0x515A}, //11340 #CJK UNIFIED IDEOGRAPH
    {0xD0DF, 0x5194}, //11341 #CJK UNIFIED IDEOGRAPH
    {0xD0E0, 0x5193}, //11342 #CJK UNIFIED IDEOGRAPH
    {0xD0E1, 0x51CA}, //11343 #CJK UNIFIED IDEOGRAPH
    {0xD0E2, 0x51C4}, //11344 #CJK UNIFIED IDEOGRAPH
    {0xD0E3, 0x51C5}, //11345 #CJK UNIFIED IDEOGRAPH
    {0xD0E4, 0x51C8}, //11346 #CJK UNIFIED IDEOGRAPH
    {0xD0E5, 0x51CE}, //11347 #CJK UNIFIED IDEOGRAPH
    {0xD0E6, 0x5261}, //11348 #CJK UNIFIED IDEOGRAPH
    {0xD0E7, 0x525A}, //11349 #CJK UNIFIED IDEOGRAPH
    {0xD0E8, 0x5252}, //11350 #CJK UNIFIED IDEOGRAPH
    {0xD0E9, 0x525E}, //11351 #CJK UNIFIED IDEOGRAPH
    {0xD0EA, 0x525F}, //11352 #CJK UNIFIED IDEOGRAPH
    {0xD0EB, 0x5255}, //11353 #CJK UNIFIED IDEOGRAPH
    {0xD0EC, 0x5262}, //11354 #CJK UNIFIED IDEOGRAPH
    {0xD0ED, 0x52CD}, //11355 #CJK UNIFIED IDEOGRAPH
    {0xD0EE, 0x530E}, //11356 #CJK UNIFIED IDEOGRAPH
    {0xD0EF, 0x539E}, //11357 #CJK UNIFIED IDEOGRAPH
    {0xD0F0, 0x5526}, //11358 #CJK UNIFIED IDEOGRAPH
    {0xD0F1, 0x54E2}, //11359 #CJK UNIFIED IDEOGRAPH
    {0xD0F2, 0x5517}, //11360 #CJK UNIFIED IDEOGRAPH
    {0xD0F3, 0x5512}, //11361 #CJK UNIFIED IDEOGRAPH
    {0xD0F4, 0x54E7}, //11362 #CJK UNIFIED IDEOGRAPH
    {0xD0F5, 0x54F3}, //11363 #CJK UNIFIED IDEOGRAPH
    {0xD0F6, 0x54E4}, //11364 #CJK UNIFIED IDEOGRAPH
    {0xD0F7, 0x551A}, //11365 #CJK UNIFIED IDEOGRAPH
    {0xD0F8, 0x54FF}, //11366 #CJK UNIFIED IDEOGRAPH
    {0xD0F9, 0x5504}, //11367 #CJK UNIFIED IDEOGRAPH
    {0xD0FA, 0x5508}, //11368 #CJK UNIFIED IDEOGRAPH
    {0xD0FB, 0x54EB}, //11369 #CJK UNIFIED IDEOGRAPH
    {0xD0FC, 0x5511}, //11370 #CJK UNIFIED IDEOGRAPH
    {0xD0FD, 0x5505}, //11371 #CJK UNIFIED IDEOGRAPH
    {0xD0FE, 0x54F1}, //11372 #CJK UNIFIED IDEOGRAPH
    {0xD140, 0x550A}, //11373 #CJK UNIFIED IDEOGRAPH
    {0xD141, 0x54FB}, //11374 #CJK UNIFIED IDEOGRAPH
    {0xD142, 0x54F7}, //11375 #CJK UNIFIED IDEOGRAPH
    {0xD143, 0x54F8}, //11376 #CJK UNIFIED IDEOGRAPH
    {0xD144, 0x54E0}, //11377 #CJK UNIFIED IDEOGRAPH
    {0xD145, 0x550E}, //11378 #CJK UNIFIED IDEOGRAPH
    {0xD146, 0x5503}, //11379 #CJK UNIFIED IDEOGRAPH
    {0xD147, 0x550B}, //11380 #CJK UNIFIED IDEOGRAPH
    {0xD148, 0x5701}, //11381 #CJK UNIFIED IDEOGRAPH
    {0xD149, 0x5702}, //11382 #CJK UNIFIED IDEOGRAPH
    {0xD14A, 0x57CC}, //11383 #CJK UNIFIED IDEOGRAPH
    {0xD14B, 0x5832}, //11384 #CJK UNIFIED IDEOGRAPH
    {0xD14C, 0x57D5}, //11385 #CJK UNIFIED IDEOGRAPH
    {0xD14D, 0x57D2}, //11386 #CJK UNIFIED IDEOGRAPH
    {0xD14E, 0x57BA}, //11387 #CJK UNIFIED IDEOGRAPH
    {0xD14F, 0x57C6}, //11388 #CJK UNIFIED IDEOGRAPH
    {0xD150, 0x57BD}, //11389 #CJK UNIFIED IDEOGRAPH
    {0xD151, 0x57BC}, //11390 #CJK UNIFIED IDEOGRAPH
    {0xD152, 0x57B8}, //11391 #CJK UNIFIED IDEOGRAPH
    {0xD153, 0x57B6}, //11392 #CJK UNIFIED IDEOGRAPH
    {0xD154, 0x57BF}, //11393 #CJK UNIFIED IDEOGRAPH
    {0xD155, 0x57C7}, //11394 #CJK UNIFIED IDEOGRAPH
    {0xD156, 0x57D0}, //11395 #CJK UNIFIED IDEOGRAPH
    {0xD157, 0x57B9}, //11396 #CJK UNIFIED IDEOGRAPH
    {0xD158, 0x57C1}, //11397 #CJK UNIFIED IDEOGRAPH
    {0xD159, 0x590E}, //11398 #CJK UNIFIED IDEOGRAPH
    {0xD15A, 0x594A}, //11399 #CJK UNIFIED IDEOGRAPH
    {0xD15B, 0x5A19}, //11400 #CJK UNIFIED IDEOGRAPH
    {0xD15C, 0x5A16}, //11401 #CJK UNIFIED IDEOGRAPH
    {0xD15D, 0x5A2D}, //11402 #CJK UNIFIED IDEOGRAPH
    {0xD15E, 0x5A2E}, //11403 #CJK UNIFIED IDEOGRAPH
    {0xD15F, 0x5A15}, //11404 #CJK UNIFIED IDEOGRAPH
    {0xD160, 0x5A0F}, //11405 #CJK UNIFIED IDEOGRAPH
    {0xD161, 0x5A17}, //11406 #CJK UNIFIED IDEOGRAPH
    {0xD162, 0x5A0A}, //11407 #CJK UNIFIED IDEOGRAPH
    {0xD163, 0x5A1E}, //11408 #CJK UNIFIED IDEOGRAPH
    {0xD164, 0x5A33}, //11409 #CJK UNIFIED IDEOGRAPH
    {0xD165, 0x5B6C}, //11410 #CJK UNIFIED IDEOGRAPH
    {0xD166, 0x5BA7}, //11411 #CJK UNIFIED IDEOGRAPH
    {0xD167, 0x5BAD}, //11412 #CJK UNIFIED IDEOGRAPH
    {0xD168, 0x5BAC}, //11413 #CJK UNIFIED IDEOGRAPH
    {0xD169, 0x5C03}, //11414 #CJK UNIFIED IDEOGRAPH
    {0xD16A, 0x5C56}, //11415 #CJK UNIFIED IDEOGRAPH
    {0xD16B, 0x5C54}, //11416 #CJK UNIFIED IDEOGRAPH
    {0xD16C, 0x5CEC}, //11417 #CJK UNIFIED IDEOGRAPH
    {0xD16D, 0x5CFF}, //11418 #CJK UNIFIED IDEOGRAPH
    {0xD16E, 0x5CEE}, //11419 #CJK UNIFIED IDEOGRAPH
    {0xD16F, 0x5CF1}, //11420 #CJK UNIFIED IDEOGRAPH
    {0xD170, 0x5CF7}, //11421 #CJK UNIFIED IDEOGRAPH
    {0xD171, 0x5D00}, //11422 #CJK UNIFIED IDEOGRAPH
    {0xD172, 0x5CF9}, //11423 #CJK UNIFIED IDEOGRAPH
    {0xD173, 0x5E29}, //11424 #CJK UNIFIED IDEOGRAPH
    {0xD174, 0x5E28}, //11425 #CJK UNIFIED IDEOGRAPH
    {0xD175, 0x5EA8}, //11426 #CJK UNIFIED IDEOGRAPH
    {0xD176, 0x5EAE}, //11427 #CJK UNIFIED IDEOGRAPH
    {0xD177, 0x5EAA}, //11428 #CJK UNIFIED IDEOGRAPH
    {0xD178, 0x5EAC}, //11429 #CJK UNIFIED IDEOGRAPH
    {0xD179, 0x5F33}, //11430 #CJK UNIFIED IDEOGRAPH
    {0xD17A, 0x5F30}, //11431 #CJK UNIFIED IDEOGRAPH
    {0xD17B, 0x5F67}, //11432 #CJK UNIFIED IDEOGRAPH
    {0xD17C, 0x605D}, //11433 #CJK UNIFIED IDEOGRAPH
    {0xD17D, 0x605A}, //11434 #CJK UNIFIED IDEOGRAPH
    {0xD17E, 0x6067}, //11435 #CJK UNIFIED IDEOGRAPH
    {0xD1A1, 0x6041}, //11436 #CJK UNIFIED IDEOGRAPH
    {0xD1A2, 0x60A2}, //11437 #CJK UNIFIED IDEOGRAPH
    {0xD1A3, 0x6088}, //11438 #CJK UNIFIED IDEOGRAPH
    {0xD1A4, 0x6080}, //11439 #CJK UNIFIED IDEOGRAPH
    {0xD1A5, 0x6092}, //11440 #CJK UNIFIED IDEOGRAPH
    {0xD1A6, 0x6081}, //11441 #CJK UNIFIED IDEOGRAPH
    {0xD1A7, 0x609D}, //11442 #CJK UNIFIED IDEOGRAPH
    {0xD1A8, 0x6083}, //11443 #CJK UNIFIED IDEOGRAPH
    {0xD1A9, 0x6095}, //11444 #CJK UNIFIED IDEOGRAPH
    {0xD1AA, 0x609B}, //11445 #CJK UNIFIED IDEOGRAPH
    {0xD1AB, 0x6097}, //11446 #CJK UNIFIED IDEOGRAPH
    {0xD1AC, 0x6087}, //11447 #CJK UNIFIED IDEOGRAPH
    {0xD1AD, 0x609C}, //11448 #CJK UNIFIED IDEOGRAPH
    {0xD1AE, 0x608E}, //11449 #CJK UNIFIED IDEOGRAPH
    {0xD1AF, 0x6219}, //11450 #CJK UNIFIED IDEOGRAPH
    {0xD1B0, 0x6246}, //11451 #CJK UNIFIED IDEOGRAPH
    {0xD1B1, 0x62F2}, //11452 #CJK UNIFIED IDEOGRAPH
    {0xD1B2, 0x6310}, //11453 #CJK UNIFIED IDEOGRAPH
    {0xD1B3, 0x6356}, //11454 #CJK UNIFIED IDEOGRAPH
    {0xD1B4, 0x632C}, //11455 #CJK UNIFIED IDEOGRAPH
    {0xD1B5, 0x6344}, //11456 #CJK UNIFIED IDEOGRAPH
    {0xD1B6, 0x6345}, //11457 #CJK UNIFIED IDEOGRAPH
    {0xD1B7, 0x6336}, //11458 #CJK UNIFIED IDEOGRAPH
    {0xD1B8, 0x6343}, //11459 #CJK UNIFIED IDEOGRAPH
    {0xD1B9, 0x63E4}, //11460 #CJK UNIFIED IDEOGRAPH
    {0xD1BA, 0x6339}, //11461 #CJK UNIFIED IDEOGRAPH
    {0xD1BB, 0x634B}, //11462 #CJK UNIFIED IDEOGRAPH
    {0xD1BC, 0x634A}, //11463 #CJK UNIFIED IDEOGRAPH
    {0xD1BD, 0x633C}, //11464 #CJK UNIFIED IDEOGRAPH
    {0xD1BE, 0x6329}, //11465 #CJK UNIFIED IDEOGRAPH
    {0xD1BF, 0x6341}, //11466 #CJK UNIFIED IDEOGRAPH
    {0xD1C0, 0x6334}, //11467 #CJK UNIFIED IDEOGRAPH
    {0xD1C1, 0x6358}, //11468 #CJK UNIFIED IDEOGRAPH
    {0xD1C2, 0x6354}, //11469 #CJK UNIFIED IDEOGRAPH
    {0xD1C3, 0x6359}, //11470 #CJK UNIFIED IDEOGRAPH
    {0xD1C4, 0x632D}, //11471 #CJK UNIFIED IDEOGRAPH
    {0xD1C5, 0x6347}, //11472 #CJK UNIFIED IDEOGRAPH
    {0xD1C6, 0x6333}, //11473 #CJK UNIFIED IDEOGRAPH
    {0xD1C7, 0x635A}, //11474 #CJK UNIFIED IDEOGRAPH
    {0xD1C8, 0x6351}, //11475 #CJK UNIFIED IDEOGRAPH
    {0xD1C9, 0x6338}, //11476 #CJK UNIFIED IDEOGRAPH
    {0xD1CA, 0x6357}, //11477 #CJK UNIFIED IDEOGRAPH
    {0xD1CB, 0x6340}, //11478 #CJK UNIFIED IDEOGRAPH
    {0xD1CC, 0x6348}, //11479 #CJK UNIFIED IDEOGRAPH
    {0xD1CD, 0x654A}, //11480 #CJK UNIFIED IDEOGRAPH
    {0xD1CE, 0x6546}, //11481 #CJK UNIFIED IDEOGRAPH
    {0xD1CF, 0x65C6}, //11482 #CJK UNIFIED IDEOGRAPH
    {0xD1D0, 0x65C3}, //11483 #CJK UNIFIED IDEOGRAPH
    {0xD1D1, 0x65C4}, //11484 #CJK UNIFIED IDEOGRAPH
    {0xD1D2, 0x65C2}, //11485 #CJK UNIFIED IDEOGRAPH
    {0xD1D3, 0x664A}, //11486 #CJK UNIFIED IDEOGRAPH
    {0xD1D4, 0x665F}, //11487 #CJK UNIFIED IDEOGRAPH
    {0xD1D5, 0x6647}, //11488 #CJK UNIFIED IDEOGRAPH
    {0xD1D6, 0x6651}, //11489 #CJK UNIFIED IDEOGRAPH
    {0xD1D7, 0x6712}, //11490 #CJK UNIFIED IDEOGRAPH
    {0xD1D8, 0x6713}, //11491 #CJK UNIFIED IDEOGRAPH
    {0xD1D9, 0x681F}, //11492 #CJK UNIFIED IDEOGRAPH
    {0xD1DA, 0x681A}, //11493 #CJK UNIFIED IDEOGRAPH
    {0xD1DB, 0x6849}, //11494 #CJK UNIFIED IDEOGRAPH
    {0xD1DC, 0x6832}, //11495 #CJK UNIFIED IDEOGRAPH
    {0xD1DD, 0x6833}, //11496 #CJK UNIFIED IDEOGRAPH
    {0xD1DE, 0x683B}, //11497 #CJK UNIFIED IDEOGRAPH
    {0xD1DF, 0x684B}, //11498 #CJK UNIFIED IDEOGRAPH
    {0xD1E0, 0x684F}, //11499 #CJK UNIFIED IDEOGRAPH
    {0xD1E1, 0x6816}, //11500 #CJK UNIFIED IDEOGRAPH
    {0xD1E2, 0x6831}, //11501 #CJK UNIFIED IDEOGRAPH
    {0xD1E3, 0x681C}, //11502 #CJK UNIFIED IDEOGRAPH
    {0xD1E4, 0x6835}, //11503 #CJK UNIFIED IDEOGRAPH
    {0xD1E5, 0x682B}, //11504 #CJK UNIFIED IDEOGRAPH
    {0xD1E6, 0x682D}, //11505 #CJK UNIFIED IDEOGRAPH
    {0xD1E7, 0x682F}, //11506 #CJK UNIFIED IDEOGRAPH
    {0xD1E8, 0x684E}, //11507 #CJK UNIFIED IDEOGRAPH
    {0xD1E9, 0x6844}, //11508 #CJK UNIFIED IDEOGRAPH
    {0xD1EA, 0x6834}, //11509 #CJK UNIFIED IDEOGRAPH
    {0xD1EB, 0x681D}, //11510 #CJK UNIFIED IDEOGRAPH
    {0xD1EC, 0x6812}, //11511 #CJK UNIFIED IDEOGRAPH
    {0xD1ED, 0x6814}, //11512 #CJK UNIFIED IDEOGRAPH
    {0xD1EE, 0x6826}, //11513 #CJK UNIFIED IDEOGRAPH
    {0xD1EF, 0x6828}, //11514 #CJK UNIFIED IDEOGRAPH
    {0xD1F0, 0x682E}, //11515 #CJK UNIFIED IDEOGRAPH
    {0xD1F1, 0x684D}, //11516 #CJK UNIFIED IDEOGRAPH
    {0xD1F2, 0x683A}, //11517 #CJK UNIFIED IDEOGRAPH
    {0xD1F3, 0x6825}, //11518 #CJK UNIFIED IDEOGRAPH
    {0xD1F4, 0x6820}, //11519 #CJK UNIFIED IDEOGRAPH
    {0xD1F5, 0x6B2C}, //11520 #CJK UNIFIED IDEOGRAPH
    {0xD1F6, 0x6B2F}, //11521 #CJK UNIFIED IDEOGRAPH
    {0xD1F7, 0x6B2D}, //11522 #CJK UNIFIED IDEOGRAPH
    {0xD1F8, 0x6B31}, //11523 #CJK UNIFIED IDEOGRAPH
    {0xD1F9, 0x6B34}, //11524 #CJK UNIFIED IDEOGRAPH
    {0xD1FA, 0x6B6D}, //11525 #CJK UNIFIED IDEOGRAPH
    {0xD1FB, 0x8082}, //11526 #CJK UNIFIED IDEOGRAPH
    {0xD1FC, 0x6B88}, //11527 #CJK UNIFIED IDEOGRAPH
    {0xD1FD, 0x6BE6}, //11528 #CJK UNIFIED IDEOGRAPH
    {0xD1FE, 0x6BE4}, //11529 #CJK UNIFIED IDEOGRAPH
    {0xD240, 0x6BE8}, //11530 #CJK UNIFIED IDEOGRAPH
    {0xD241, 0x6BE3}, //11531 #CJK UNIFIED IDEOGRAPH
    {0xD242, 0x6BE2}, //11532 #CJK UNIFIED IDEOGRAPH
    {0xD243, 0x6BE7}, //11533 #CJK UNIFIED IDEOGRAPH
    {0xD244, 0x6C25}, //11534 #CJK UNIFIED IDEOGRAPH
    {0xD245, 0x6D7A}, //11535 #CJK UNIFIED IDEOGRAPH
    {0xD246, 0x6D63}, //11536 #CJK UNIFIED IDEOGRAPH
    {0xD247, 0x6D64}, //11537 #CJK UNIFIED IDEOGRAPH
    {0xD248, 0x6D76}, //11538 #CJK UNIFIED IDEOGRAPH
    {0xD249, 0x6D0D}, //11539 #CJK UNIFIED IDEOGRAPH
    {0xD24A, 0x6D61}, //11540 #CJK UNIFIED IDEOGRAPH
    {0xD24B, 0x6D92}, //11541 #CJK UNIFIED IDEOGRAPH
    {0xD24C, 0x6D58}, //11542 #CJK UNIFIED IDEOGRAPH
    {0xD24D, 0x6D62}, //11543 #CJK UNIFIED IDEOGRAPH
    {0xD24E, 0x6D6D}, //11544 #CJK UNIFIED IDEOGRAPH
    {0xD24F, 0x6D6F}, //11545 #CJK UNIFIED IDEOGRAPH
    {0xD250, 0x6D91}, //11546 #CJK UNIFIED IDEOGRAPH
    {0xD251, 0x6D8D}, //11547 #CJK UNIFIED IDEOGRAPH
    {0xD252, 0x6DEF}, //11548 #CJK UNIFIED IDEOGRAPH
    {0xD253, 0x6D7F}, //11549 #CJK UNIFIED IDEOGRAPH
    {0xD254, 0x6D86}, //11550 #CJK UNIFIED IDEOGRAPH
    {0xD255, 0x6D5E}, //11551 #CJK UNIFIED IDEOGRAPH
    {0xD256, 0x6D67}, //11552 #CJK UNIFIED IDEOGRAPH
    {0xD257, 0x6D60}, //11553 #CJK UNIFIED IDEOGRAPH
    {0xD258, 0x6D97}, //11554 #CJK UNIFIED IDEOGRAPH
    {0xD259, 0x6D70}, //11555 #CJK UNIFIED IDEOGRAPH
    {0xD25A, 0x6D7C}, //11556 #CJK UNIFIED IDEOGRAPH
    {0xD25B, 0x6D5F}, //11557 #CJK UNIFIED IDEOGRAPH
    {0xD25C, 0x6D82}, //11558 #CJK UNIFIED IDEOGRAPH
    {0xD25D, 0x6D98}, //11559 #CJK UNIFIED IDEOGRAPH
    {0xD25E, 0x6D2F}, //11560 #CJK UNIFIED IDEOGRAPH
    {0xD25F, 0x6D68}, //11561 #CJK UNIFIED IDEOGRAPH
    {0xD260, 0x6D8B}, //11562 #CJK UNIFIED IDEOGRAPH
    {0xD261, 0x6D7E}, //11563 #CJK UNIFIED IDEOGRAPH
    {0xD262, 0x6D80}, //11564 #CJK UNIFIED IDEOGRAPH
    {0xD263, 0x6D84}, //11565 #CJK UNIFIED IDEOGRAPH
    {0xD264, 0x6D16}, //11566 #CJK UNIFIED IDEOGRAPH
    {0xD265, 0x6D83}, //11567 #CJK UNIFIED IDEOGRAPH
    {0xD266, 0x6D7B}, //11568 #CJK UNIFIED IDEOGRAPH
    {0xD267, 0x6D7D}, //11569 #CJK UNIFIED IDEOGRAPH
    {0xD268, 0x6D75}, //11570 #CJK UNIFIED IDEOGRAPH
    {0xD269, 0x6D90}, //11571 #CJK UNIFIED IDEOGRAPH
    {0xD26A, 0x70DC}, //11572 #CJK UNIFIED IDEOGRAPH
    {0xD26B, 0x70D3}, //11573 #CJK UNIFIED IDEOGRAPH
    {0xD26C, 0x70D1}, //11574 #CJK UNIFIED IDEOGRAPH
    {0xD26D, 0x70DD}, //11575 #CJK UNIFIED IDEOGRAPH
    {0xD26E, 0x70CB}, //11576 #CJK UNIFIED IDEOGRAPH
    {0xD26F, 0x7F39}, //11577 #CJK UNIFIED IDEOGRAPH
    {0xD270, 0x70E2}, //11578 #CJK UNIFIED IDEOGRAPH
    {0xD271, 0x70D7}, //11579 #CJK UNIFIED IDEOGRAPH
    {0xD272, 0x70D2}, //11580 #CJK UNIFIED IDEOGRAPH
    {0xD273, 0x70DE}, //11581 #CJK UNIFIED IDEOGRAPH
    {0xD274, 0x70E0}, //11582 #CJK UNIFIED IDEOGRAPH
    {0xD275, 0x70D4}, //11583 #CJK UNIFIED IDEOGRAPH
    {0xD276, 0x70CD}, //11584 #CJK UNIFIED IDEOGRAPH
    {0xD277, 0x70C5}, //11585 #CJK UNIFIED IDEOGRAPH
    {0xD278, 0x70C6}, //11586 #CJK UNIFIED IDEOGRAPH
    {0xD279, 0x70C7}, //11587 #CJK UNIFIED IDEOGRAPH
    {0xD27A, 0x70DA}, //11588 #CJK UNIFIED IDEOGRAPH
    {0xD27B, 0x70CE}, //11589 #CJK UNIFIED IDEOGRAPH
    {0xD27C, 0x70E1}, //11590 #CJK UNIFIED IDEOGRAPH
    {0xD27D, 0x7242}, //11591 #CJK UNIFIED IDEOGRAPH
    {0xD27E, 0x7278}, //11592 #CJK UNIFIED IDEOGRAPH
    {0xD2A1, 0x7277}, //11593 #CJK UNIFIED IDEOGRAPH
    {0xD2A2, 0x7276}, //11594 #CJK UNIFIED IDEOGRAPH
    {0xD2A3, 0x7300}, //11595 #CJK UNIFIED IDEOGRAPH
    {0xD2A4, 0x72FA}, //11596 #CJK UNIFIED IDEOGRAPH
    {0xD2A5, 0x72F4}, //11597 #CJK UNIFIED IDEOGRAPH
    {0xD2A6, 0x72FE}, //11598 #CJK UNIFIED IDEOGRAPH
    {0xD2A7, 0x72F6}, //11599 #CJK UNIFIED IDEOGRAPH
    {0xD2A8, 0x72F3}, //11600 #CJK UNIFIED IDEOGRAPH
    {0xD2A9, 0x72FB}, //11601 #CJK UNIFIED IDEOGRAPH
    {0xD2AA, 0x7301}, //11602 #CJK UNIFIED IDEOGRAPH
    {0xD2AB, 0x73D3}, //11603 #CJK UNIFIED IDEOGRAPH
    {0xD2AC, 0x73D9}, //11604 #CJK UNIFIED IDEOGRAPH
    {0xD2AD, 0x73E5}, //11605 #CJK UNIFIED IDEOGRAPH
    {0xD2AE, 0x73D6}, //11606 #CJK UNIFIED IDEOGRAPH
    {0xD2AF, 0x73BC}, //11607 #CJK UNIFIED IDEOGRAPH
    {0xD2B0, 0x73E7}, //11608 #CJK UNIFIED IDEOGRAPH
    {0xD2B1, 0x73E3}, //11609 #CJK UNIFIED IDEOGRAPH
    {0xD2B2, 0x73E9}, //11610 #CJK UNIFIED IDEOGRAPH
    {0xD2B3, 0x73DC}, //11611 #CJK UNIFIED IDEOGRAPH
    {0xD2B4, 0x73D2}, //11612 #CJK UNIFIED IDEOGRAPH
    {0xD2B5, 0x73DB}, //11613 #CJK UNIFIED IDEOGRAPH
    {0xD2B6, 0x73D4}, //11614 #CJK UNIFIED IDEOGRAPH
    {0xD2B7, 0x73DD}, //11615 #CJK UNIFIED IDEOGRAPH
    {0xD2B8, 0x73DA}, //11616 #CJK UNIFIED IDEOGRAPH
    {0xD2B9, 0x73D7}, //11617 #CJK UNIFIED IDEOGRAPH
    {0xD2BA, 0x73D8}, //11618 #CJK UNIFIED IDEOGRAPH
    {0xD2BB, 0x73E8}, //11619 #CJK UNIFIED IDEOGRAPH
    {0xD2BC, 0x74DE}, //11620 #CJK UNIFIED IDEOGRAPH
    {0xD2BD, 0x74DF}, //11621 #CJK UNIFIED IDEOGRAPH
    {0xD2BE, 0x74F4}, //11622 #CJK UNIFIED IDEOGRAPH
    {0xD2BF, 0x74F5}, //11623 #CJK UNIFIED IDEOGRAPH
    {0xD2C0, 0x7521}, //11624 #CJK UNIFIED IDEOGRAPH
    {0xD2C1, 0x755B}, //11625 #CJK UNIFIED IDEOGRAPH
    {0xD2C2, 0x755F}, //11626 #CJK UNIFIED IDEOGRAPH
    {0xD2C3, 0x75B0}, //11627 #CJK UNIFIED IDEOGRAPH
    {0xD2C4, 0x75C1}, //11628 #CJK UNIFIED IDEOGRAPH
    {0xD2C5, 0x75BB}, //11629 #CJK UNIFIED IDEOGRAPH
    {0xD2C6, 0x75C4}, //11630 #CJK UNIFIED IDEOGRAPH
    {0xD2C7, 0x75C0}, //11631 #CJK UNIFIED IDEOGRAPH
    {0xD2C8, 0x75BF}, //11632 #CJK UNIFIED IDEOGRAPH
    {0xD2C9, 0x75B6}, //11633 #CJK UNIFIED IDEOGRAPH
    {0xD2CA, 0x75BA}, //11634 #CJK UNIFIED IDEOGRAPH
    {0xD2CB, 0x768A}, //11635 #CJK UNIFIED IDEOGRAPH
    {0xD2CC, 0x76C9}, //11636 #CJK UNIFIED IDEOGRAPH
    {0xD2CD, 0x771D}, //11637 #CJK UNIFIED IDEOGRAPH
    {0xD2CE, 0x771B}, //11638 #CJK UNIFIED IDEOGRAPH
    {0xD2CF, 0x7710}, //11639 #CJK UNIFIED IDEOGRAPH
    {0xD2D0, 0x7713}, //11640 #CJK UNIFIED IDEOGRAPH
    {0xD2D1, 0x7712}, //11641 #CJK UNIFIED IDEOGRAPH
    {0xD2D2, 0x7723}, //11642 #CJK UNIFIED IDEOGRAPH
    {0xD2D3, 0x7711}, //11643 #CJK UNIFIED IDEOGRAPH
    {0xD2D4, 0x7715}, //11644 #CJK UNIFIED IDEOGRAPH
    {0xD2D5, 0x7719}, //11645 #CJK UNIFIED IDEOGRAPH
    {0xD2D6, 0x771A}, //11646 #CJK UNIFIED IDEOGRAPH
    {0xD2D7, 0x7722}, //11647 #CJK UNIFIED IDEOGRAPH
    {0xD2D8, 0x7727}, //11648 #CJK UNIFIED IDEOGRAPH
    {0xD2D9, 0x7823}, //11649 #CJK UNIFIED IDEOGRAPH
    {0xD2DA, 0x782C}, //11650 #CJK UNIFIED IDEOGRAPH
    {0xD2DB, 0x7822}, //11651 #CJK UNIFIED IDEOGRAPH
    {0xD2DC, 0x7835}, //11652 #CJK UNIFIED IDEOGRAPH
    {0xD2DD, 0x782F}, //11653 #CJK UNIFIED IDEOGRAPH
    {0xD2DE, 0x7828}, //11654 #CJK UNIFIED IDEOGRAPH
    {0xD2DF, 0x782E}, //11655 #CJK UNIFIED IDEOGRAPH
    {0xD2E0, 0x782B}, //11656 #CJK UNIFIED IDEOGRAPH
    {0xD2E1, 0x7821}, //11657 #CJK UNIFIED IDEOGRAPH
    {0xD2E2, 0x7829}, //11658 #CJK UNIFIED IDEOGRAPH
    {0xD2E3, 0x7833}, //11659 #CJK UNIFIED IDEOGRAPH
    {0xD2E4, 0x782A}, //11660 #CJK UNIFIED IDEOGRAPH
    {0xD2E5, 0x7831}, //11661 #CJK UNIFIED IDEOGRAPH
    {0xD2E6, 0x7954}, //11662 #CJK UNIFIED IDEOGRAPH
    {0xD2E7, 0x795B}, //11663 #CJK UNIFIED IDEOGRAPH
    {0xD2E8, 0x794F}, //11664 #CJK UNIFIED IDEOGRAPH
    {0xD2E9, 0x795C}, //11665 #CJK UNIFIED IDEOGRAPH
    {0xD2EA, 0x7953}, //11666 #CJK UNIFIED IDEOGRAPH
    {0xD2EB, 0x7952}, //11667 #CJK UNIFIED IDEOGRAPH
    {0xD2EC, 0x7951}, //11668 #CJK UNIFIED IDEOGRAPH
    {0xD2ED, 0x79EB}, //11669 #CJK UNIFIED IDEOGRAPH
    {0xD2EE, 0x79EC}, //11670 #CJK UNIFIED IDEOGRAPH
    {0xD2EF, 0x79E0}, //11671 #CJK UNIFIED IDEOGRAPH
    {0xD2F0, 0x79EE}, //11672 #CJK UNIFIED IDEOGRAPH
    {0xD2F1, 0x79ED}, //11673 #CJK UNIFIED IDEOGRAPH
    {0xD2F2, 0x79EA}, //11674 #CJK UNIFIED IDEOGRAPH
    {0xD2F3, 0x79DC}, //11675 #CJK UNIFIED IDEOGRAPH
    {0xD2F4, 0x79DE}, //11676 #CJK UNIFIED IDEOGRAPH
    {0xD2F5, 0x79DD}, //11677 #CJK UNIFIED IDEOGRAPH
    {0xD2F6, 0x7A86}, //11678 #CJK UNIFIED IDEOGRAPH
    {0xD2F7, 0x7A89}, //11679 #CJK UNIFIED IDEOGRAPH
    {0xD2F8, 0x7A85}, //11680 #CJK UNIFIED IDEOGRAPH
    {0xD2F9, 0x7A8B}, //11681 #CJK UNIFIED IDEOGRAPH
    {0xD2FA, 0x7A8C}, //11682 #CJK UNIFIED IDEOGRAPH
    {0xD2FB, 0x7A8A}, //11683 #CJK UNIFIED IDEOGRAPH
    {0xD2FC, 0x7A87}, //11684 #CJK UNIFIED IDEOGRAPH
    {0xD2FD, 0x7AD8}, //11685 #CJK UNIFIED IDEOGRAPH
    {0xD2FE, 0x7B10}, //11686 #CJK UNIFIED IDEOGRAPH
    {0xD340, 0x7B04}, //11687 #CJK UNIFIED IDEOGRAPH
    {0xD341, 0x7B13}, //11688 #CJK UNIFIED IDEOGRAPH
    {0xD342, 0x7B05}, //11689 #CJK UNIFIED IDEOGRAPH
    {0xD343, 0x7B0F}, //11690 #CJK UNIFIED IDEOGRAPH
    {0xD344, 0x7B08}, //11691 #CJK UNIFIED IDEOGRAPH
    {0xD345, 0x7B0A}, //11692 #CJK UNIFIED IDEOGRAPH
    {0xD346, 0x7B0E}, //11693 #CJK UNIFIED IDEOGRAPH
    {0xD347, 0x7B09}, //11694 #CJK UNIFIED IDEOGRAPH
    {0xD348, 0x7B12}, //11695 #CJK UNIFIED IDEOGRAPH
    {0xD349, 0x7C84}, //11696 #CJK UNIFIED IDEOGRAPH
    {0xD34A, 0x7C91}, //11697 #CJK UNIFIED IDEOGRAPH
    {0xD34B, 0x7C8A}, //11698 #CJK UNIFIED IDEOGRAPH
    {0xD34C, 0x7C8C}, //11699 #CJK UNIFIED IDEOGRAPH
    {0xD34D, 0x7C88}, //11700 #CJK UNIFIED IDEOGRAPH
    {0xD34E, 0x7C8D}, //11701 #CJK UNIFIED IDEOGRAPH
    {0xD34F, 0x7C85}, //11702 #CJK UNIFIED IDEOGRAPH
    {0xD350, 0x7D1E}, //11703 #CJK UNIFIED IDEOGRAPH
    {0xD351, 0x7D1D}, //11704 #CJK UNIFIED IDEOGRAPH
    {0xD352, 0x7D11}, //11705 #CJK UNIFIED IDEOGRAPH
    {0xD353, 0x7D0E}, //11706 #CJK UNIFIED IDEOGRAPH
    {0xD354, 0x7D18}, //11707 #CJK UNIFIED IDEOGRAPH
    {0xD355, 0x7D16}, //11708 #CJK UNIFIED IDEOGRAPH
    {0xD356, 0x7D13}, //11709 #CJK UNIFIED IDEOGRAPH
    {0xD357, 0x7D1F}, //11710 #CJK UNIFIED IDEOGRAPH
    {0xD358, 0x7D12}, //11711 #CJK UNIFIED IDEOGRAPH
    {0xD359, 0x7D0F}, //11712 #CJK UNIFIED IDEOGRAPH
    {0xD35A, 0x7D0C}, //11713 #CJK UNIFIED IDEOGRAPH
    {0xD35B, 0x7F5C}, //11714 #CJK UNIFIED IDEOGRAPH
    {0xD35C, 0x7F61}, //11715 #CJK UNIFIED IDEOGRAPH
    {0xD35D, 0x7F5E}, //11716 #CJK UNIFIED IDEOGRAPH
    {0xD35E, 0x7F60}, //11717 #CJK UNIFIED IDEOGRAPH
    {0xD35F, 0x7F5D}, //11718 #CJK UNIFIED IDEOGRAPH
    {0xD360, 0x7F5B}, //11719 #CJK UNIFIED IDEOGRAPH
    {0xD361, 0x7F96}, //11720 #CJK UNIFIED IDEOGRAPH
    {0xD362, 0x7F92}, //11721 #CJK UNIFIED IDEOGRAPH
    {0xD363, 0x7FC3}, //11722 #CJK UNIFIED IDEOGRAPH
    {0xD364, 0x7FC2}, //11723 #CJK UNIFIED IDEOGRAPH
    {0xD365, 0x7FC0}, //11724 #CJK UNIFIED IDEOGRAPH
    {0xD366, 0x8016}, //11725 #CJK UNIFIED IDEOGRAPH
    {0xD367, 0x803E}, //11726 #CJK UNIFIED IDEOGRAPH
    {0xD368, 0x8039}, //11727 #CJK UNIFIED IDEOGRAPH
    {0xD369, 0x80FA}, //11728 #CJK UNIFIED IDEOGRAPH
    {0xD36A, 0x80F2}, //11729 #CJK UNIFIED IDEOGRAPH
    {0xD36B, 0x80F9}, //11730 #CJK UNIFIED IDEOGRAPH
    {0xD36C, 0x80F5}, //11731 #CJK UNIFIED IDEOGRAPH
    {0xD36D, 0x8101}, //11732 #CJK UNIFIED IDEOGRAPH
    {0xD36E, 0x80FB}, //11733 #CJK UNIFIED IDEOGRAPH
    {0xD36F, 0x8100}, //11734 #CJK UNIFIED IDEOGRAPH
    {0xD370, 0x8201}, //11735 #CJK UNIFIED IDEOGRAPH
    {0xD371, 0x822F}, //11736 #CJK UNIFIED IDEOGRAPH
    {0xD372, 0x8225}, //11737 #CJK UNIFIED IDEOGRAPH
    {0xD373, 0x8333}, //11738 #CJK UNIFIED IDEOGRAPH
    {0xD374, 0x832D}, //11739 #CJK UNIFIED IDEOGRAPH
    {0xD375, 0x8344}, //11740 #CJK UNIFIED IDEOGRAPH
    {0xD376, 0x8319}, //11741 #CJK UNIFIED IDEOGRAPH
    {0xD377, 0x8351}, //11742 #CJK UNIFIED IDEOGRAPH
    {0xD378, 0x8325}, //11743 #CJK UNIFIED IDEOGRAPH
    {0xD379, 0x8356}, //11744 #CJK UNIFIED IDEOGRAPH
    {0xD37A, 0x833F}, //11745 #CJK UNIFIED IDEOGRAPH
    {0xD37B, 0x8341}, //11746 #CJK UNIFIED IDEOGRAPH
    {0xD37C, 0x8326}, //11747 #CJK UNIFIED IDEOGRAPH
    {0xD37D, 0x831C}, //11748 #CJK UNIFIED IDEOGRAPH
    {0xD37E, 0x8322}, //11749 #CJK UNIFIED IDEOGRAPH
    {0xD3A1, 0x8342}, //11750 #CJK UNIFIED IDEOGRAPH
    {0xD3A2, 0x834E}, //11751 #CJK UNIFIED IDEOGRAPH
    {0xD3A3, 0x831B}, //11752 #CJK UNIFIED IDEOGRAPH
    {0xD3A4, 0x832A}, //11753 #CJK UNIFIED IDEOGRAPH
    {0xD3A5, 0x8308}, //11754 #CJK UNIFIED IDEOGRAPH
    {0xD3A6, 0x833C}, //11755 #CJK UNIFIED IDEOGRAPH
    {0xD3A7, 0x834D}, //11756 #CJK UNIFIED IDEOGRAPH
    {0xD3A8, 0x8316}, //11757 #CJK UNIFIED IDEOGRAPH
    {0xD3A9, 0x8324}, //11758 #CJK UNIFIED IDEOGRAPH
    {0xD3AA, 0x8320}, //11759 #CJK UNIFIED IDEOGRAPH
    {0xD3AB, 0x8337}, //11760 #CJK UNIFIED IDEOGRAPH
    {0xD3AC, 0x832F}, //11761 #CJK UNIFIED IDEOGRAPH
    {0xD3AD, 0x8329}, //11762 #CJK UNIFIED IDEOGRAPH
    {0xD3AE, 0x8347}, //11763 #CJK UNIFIED IDEOGRAPH
    {0xD3AF, 0x8345}, //11764 #CJK UNIFIED IDEOGRAPH
    {0xD3B0, 0x834C}, //11765 #CJK UNIFIED IDEOGRAPH
    {0xD3B1, 0x8353}, //11766 #CJK UNIFIED IDEOGRAPH
    {0xD3B2, 0x831E}, //11767 #CJK UNIFIED IDEOGRAPH
    {0xD3B3, 0x832C}, //11768 #CJK UNIFIED IDEOGRAPH
    {0xD3B4, 0x834B}, //11769 #CJK UNIFIED IDEOGRAPH
    {0xD3B5, 0x8327}, //11770 #CJK UNIFIED IDEOGRAPH
    {0xD3B6, 0x8348}, //11771 #CJK UNIFIED IDEOGRAPH
    {0xD3B7, 0x8653}, //11772 #CJK UNIFIED IDEOGRAPH
    {0xD3B8, 0x8652}, //11773 #CJK UNIFIED IDEOGRAPH
    {0xD3B9, 0x86A2}, //11774 #CJK UNIFIED IDEOGRAPH
    {0xD3BA, 0x86A8}, //11775 #CJK UNIFIED IDEOGRAPH
    {0xD3BB, 0x8696}, //11776 #CJK UNIFIED IDEOGRAPH
    {0xD3BC, 0x868D}, //11777 #CJK UNIFIED IDEOGRAPH
    {0xD3BD, 0x8691}, //11778 #CJK UNIFIED IDEOGRAPH
    {0xD3BE, 0x869E}, //11779 #CJK UNIFIED IDEOGRAPH
    {0xD3BF, 0x8687}, //11780 #CJK UNIFIED IDEOGRAPH
    {0xD3C0, 0x8697}, //11781 #CJK UNIFIED IDEOGRAPH
    {0xD3C1, 0x8686}, //11782 #CJK UNIFIED IDEOGRAPH
    {0xD3C2, 0x868B}, //11783 #CJK UNIFIED IDEOGRAPH
    {0xD3C3, 0x869A}, //11784 #CJK UNIFIED IDEOGRAPH
    {0xD3C4, 0x8685}, //11785 #CJK UNIFIED IDEOGRAPH
    {0xD3C5, 0x86A5}, //11786 #CJK UNIFIED IDEOGRAPH
    {0xD3C6, 0x8699}, //11787 #CJK UNIFIED IDEOGRAPH
    {0xD3C7, 0x86A1}, //11788 #CJK UNIFIED IDEOGRAPH
    {0xD3C8, 0x86A7}, //11789 #CJK UNIFIED IDEOGRAPH
    {0xD3C9, 0x8695}, //11790 #CJK UNIFIED IDEOGRAPH
    {0xD3CA, 0x8698}, //11791 #CJK UNIFIED IDEOGRAPH
    {0xD3CB, 0x868E}, //11792 #CJK UNIFIED IDEOGRAPH
    {0xD3CC, 0x869D}, //11793 #CJK UNIFIED IDEOGRAPH
    {0xD3CD, 0x8690}, //11794 #CJK UNIFIED IDEOGRAPH
    {0xD3CE, 0x8694}, //11795 #CJK UNIFIED IDEOGRAPH
    {0xD3CF, 0x8843}, //11796 #CJK UNIFIED IDEOGRAPH
    {0xD3D0, 0x8844}, //11797 #CJK UNIFIED IDEOGRAPH
    {0xD3D1, 0x886D}, //11798 #CJK UNIFIED IDEOGRAPH
    {0xD3D2, 0x8875}, //11799 #CJK UNIFIED IDEOGRAPH
    {0xD3D3, 0x8876}, //11800 #CJK UNIFIED IDEOGRAPH
    {0xD3D4, 0x8872}, //11801 #CJK UNIFIED IDEOGRAPH
    {0xD3D5, 0x8880}, //11802 #CJK UNIFIED IDEOGRAPH
    {0xD3D6, 0x8871}, //11803 #CJK UNIFIED IDEOGRAPH
    {0xD3D7, 0x887F}, //11804 #CJK UNIFIED IDEOGRAPH
    {0xD3D8, 0x886F}, //11805 #CJK UNIFIED IDEOGRAPH
    {0xD3D9, 0x8883}, //11806 #CJK UNIFIED IDEOGRAPH
    {0xD3DA, 0x887E}, //11807 #CJK UNIFIED IDEOGRAPH
    {0xD3DB, 0x8874}, //11808 #CJK UNIFIED IDEOGRAPH
    {0xD3DC, 0x887C}, //11809 #CJK UNIFIED IDEOGRAPH
    {0xD3DD, 0x8A12}, //11810 #CJK UNIFIED IDEOGRAPH
    {0xD3DE, 0x8C47}, //11811 #CJK UNIFIED IDEOGRAPH
    {0xD3DF, 0x8C57}, //11812 #CJK UNIFIED IDEOGRAPH
    {0xD3E0, 0x8C7B}, //11813 #CJK UNIFIED IDEOGRAPH
    {0xD3E1, 0x8CA4}, //11814 #CJK UNIFIED IDEOGRAPH
    {0xD3E2, 0x8CA3}, //11815 #CJK UNIFIED IDEOGRAPH
    {0xD3E3, 0x8D76}, //11816 #CJK UNIFIED IDEOGRAPH
    {0xD3E4, 0x8D78}, //11817 #CJK UNIFIED IDEOGRAPH
    {0xD3E5, 0x8DB5}, //11818 #CJK UNIFIED IDEOGRAPH
    {0xD3E6, 0x8DB7}, //11819 #CJK UNIFIED IDEOGRAPH
    {0xD3E7, 0x8DB6}, //11820 #CJK UNIFIED IDEOGRAPH
    {0xD3E8, 0x8ED1}, //11821 #CJK UNIFIED IDEOGRAPH
    {0xD3E9, 0x8ED3}, //11822 #CJK UNIFIED IDEOGRAPH
    {0xD3EA, 0x8FFE}, //11823 #CJK UNIFIED IDEOGRAPH
    {0xD3EB, 0x8FF5}, //11824 #CJK UNIFIED IDEOGRAPH
    {0xD3EC, 0x9002}, //11825 #CJK UNIFIED IDEOGRAPH
    {0xD3ED, 0x8FFF}, //11826 #CJK UNIFIED IDEOGRAPH
    {0xD3EE, 0x8FFB}, //11827 #CJK UNIFIED IDEOGRAPH
    {0xD3EF, 0x9004}, //11828 #CJK UNIFIED IDEOGRAPH
    {0xD3F0, 0x8FFC}, //11829 #CJK UNIFIED IDEOGRAPH
    {0xD3F1, 0x8FF6}, //11830 #CJK UNIFIED IDEOGRAPH
    {0xD3F2, 0x90D6}, //11831 #CJK UNIFIED IDEOGRAPH
    {0xD3F3, 0x90E0}, //11832 #CJK UNIFIED IDEOGRAPH
    {0xD3F4, 0x90D9}, //11833 #CJK UNIFIED IDEOGRAPH
    {0xD3F5, 0x90DA}, //11834 #CJK UNIFIED IDEOGRAPH
    {0xD3F6, 0x90E3}, //11835 #CJK UNIFIED IDEOGRAPH
    {0xD3F7, 0x90DF}, //11836 #CJK UNIFIED IDEOGRAPH
    {0xD3F8, 0x90E5}, //11837 #CJK UNIFIED IDEOGRAPH
    {0xD3F9, 0x90D8}, //11838 #CJK UNIFIED IDEOGRAPH
    {0xD3FA, 0x90DB}, //11839 #CJK UNIFIED IDEOGRAPH
    {0xD3FB, 0x90D7}, //11840 #CJK UNIFIED IDEOGRAPH
    {0xD3FC, 0x90DC}, //11841 #CJK UNIFIED IDEOGRAPH
    {0xD3FD, 0x90E4}, //11842 #CJK UNIFIED IDEOGRAPH
    {0xD3FE, 0x9150}, //11843 #CJK UNIFIED IDEOGRAPH
    {0xD440, 0x914E}, //11844 #CJK UNIFIED IDEOGRAPH
    {0xD441, 0x914F}, //11845 #CJK UNIFIED IDEOGRAPH
    {0xD442, 0x91D5}, //11846 #CJK UNIFIED IDEOGRAPH
    {0xD443, 0x91E2}, //11847 #CJK UNIFIED IDEOGRAPH
    {0xD444, 0x91DA}, //11848 #CJK UNIFIED IDEOGRAPH
    {0xD445, 0x965C}, //11849 #CJK UNIFIED IDEOGRAPH
    {0xD446, 0x965F}, //11850 #CJK UNIFIED IDEOGRAPH
    {0xD447, 0x96BC}, //11851 #CJK UNIFIED IDEOGRAPH
    {0xD448, 0x98E3}, //11852 #CJK UNIFIED IDEOGRAPH
    {0xD449, 0x9ADF}, //11853 #CJK UNIFIED IDEOGRAPH
    {0xD44A, 0x9B2F}, //11854 #CJK UNIFIED IDEOGRAPH
    {0xD44B, 0x4E7F}, //11855 #CJK UNIFIED IDEOGRAPH
    {0xD44C, 0x5070}, //11856 #CJK UNIFIED IDEOGRAPH
    {0xD44D, 0x506A}, //11857 #CJK UNIFIED IDEOGRAPH
    {0xD44E, 0x5061}, //11858 #CJK UNIFIED IDEOGRAPH
    {0xD44F, 0x505E}, //11859 #CJK UNIFIED IDEOGRAPH
    {0xD450, 0x5060}, //11860 #CJK UNIFIED IDEOGRAPH
    {0xD451, 0x5053}, //11861 #CJK UNIFIED IDEOGRAPH
    {0xD452, 0x504B}, //11862 #CJK UNIFIED IDEOGRAPH
    {0xD453, 0x505D}, //11863 #CJK UNIFIED IDEOGRAPH
    {0xD454, 0x5072}, //11864 #CJK UNIFIED IDEOGRAPH
    {0xD455, 0x5048}, //11865 #CJK UNIFIED IDEOGRAPH
    {0xD456, 0x504D}, //11866 #CJK UNIFIED IDEOGRAPH
    {0xD457, 0x5041}, //11867 #CJK UNIFIED IDEOGRAPH
    {0xD458, 0x505B}, //11868 #CJK UNIFIED IDEOGRAPH
    {0xD459, 0x504A}, //11869 #CJK UNIFIED IDEOGRAPH
    {0xD45A, 0x5062}, //11870 #CJK UNIFIED IDEOGRAPH
    {0xD45B, 0x5015}, //11871 #CJK UNIFIED IDEOGRAPH
    {0xD45C, 0x5045}, //11872 #CJK UNIFIED IDEOGRAPH
    {0xD45D, 0x505F}, //11873 #CJK UNIFIED IDEOGRAPH
    {0xD45E, 0x5069}, //11874 #CJK UNIFIED IDEOGRAPH
    {0xD45F, 0x506B}, //11875 #CJK UNIFIED IDEOGRAPH
    {0xD460, 0x5063}, //11876 #CJK UNIFIED IDEOGRAPH
    {0xD461, 0x5064}, //11877 #CJK UNIFIED IDEOGRAPH
    {0xD462, 0x5046}, //11878 #CJK UNIFIED IDEOGRAPH
    {0xD463, 0x5040}, //11879 #CJK UNIFIED IDEOGRAPH
    {0xD464, 0x506E}, //11880 #CJK UNIFIED IDEOGRAPH
    {0xD465, 0x5073}, //11881 #CJK UNIFIED IDEOGRAPH
    {0xD466, 0x5057}, //11882 #CJK UNIFIED IDEOGRAPH
    {0xD467, 0x5051}, //11883 #CJK UNIFIED IDEOGRAPH
    {0xD468, 0x51D0}, //11884 #CJK UNIFIED IDEOGRAPH
    {0xD469, 0x526B}, //11885 #CJK UNIFIED IDEOGRAPH
    {0xD46A, 0x526D}, //11886 #CJK UNIFIED IDEOGRAPH
    {0xD46B, 0x526C}, //11887 #CJK UNIFIED IDEOGRAPH
    {0xD46C, 0x526E}, //11888 #CJK UNIFIED IDEOGRAPH
    {0xD46D, 0x52D6}, //11889 #CJK UNIFIED IDEOGRAPH
    {0xD46E, 0x52D3}, //11890 #CJK UNIFIED IDEOGRAPH
    {0xD46F, 0x532D}, //11891 #CJK UNIFIED IDEOGRAPH
    {0xD470, 0x539C}, //11892 #CJK UNIFIED IDEOGRAPH
    {0xD471, 0x5575}, //11893 #CJK UNIFIED IDEOGRAPH
    {0xD472, 0x5576}, //11894 #CJK UNIFIED IDEOGRAPH
    {0xD473, 0x553C}, //11895 #CJK UNIFIED IDEOGRAPH
    {0xD474, 0x554D}, //11896 #CJK UNIFIED IDEOGRAPH
    {0xD475, 0x5550}, //11897 #CJK UNIFIED IDEOGRAPH
    {0xD476, 0x5534}, //11898 #CJK UNIFIED IDEOGRAPH
    {0xD477, 0x552A}, //11899 #CJK UNIFIED IDEOGRAPH
    {0xD478, 0x5551}, //11900 #CJK UNIFIED IDEOGRAPH
    {0xD479, 0x5562}, //11901 #CJK UNIFIED IDEOGRAPH
    {0xD47A, 0x5536}, //11902 #CJK UNIFIED IDEOGRAPH
    {0xD47B, 0x5535}, //11903 #CJK UNIFIED IDEOGRAPH
    {0xD47C, 0x5530}, //11904 #CJK UNIFIED IDEOGRAPH
    {0xD47D, 0x5552}, //11905 #CJK UNIFIED IDEOGRAPH
    {0xD47E, 0x5545}, //11906 #CJK UNIFIED IDEOGRAPH
    {0xD4A1, 0x550C}, //11907 #CJK UNIFIED IDEOGRAPH
    {0xD4A2, 0x5532}, //11908 #CJK UNIFIED IDEOGRAPH
    {0xD4A3, 0x5565}, //11909 #CJK UNIFIED IDEOGRAPH
    {0xD4A4, 0x554E}, //11910 #CJK UNIFIED IDEOGRAPH
    {0xD4A5, 0x5539}, //11911 #CJK UNIFIED IDEOGRAPH
    {0xD4A6, 0x5548}, //11912 #CJK UNIFIED IDEOGRAPH
    {0xD4A7, 0x552D}, //11913 #CJK UNIFIED IDEOGRAPH
    {0xD4A8, 0x553B}, //11914 #CJK UNIFIED IDEOGRAPH
    {0xD4A9, 0x5540}, //11915 #CJK UNIFIED IDEOGRAPH
    {0xD4AA, 0x554B}, //11916 #CJK UNIFIED IDEOGRAPH
    {0xD4AB, 0x570A}, //11917 #CJK UNIFIED IDEOGRAPH
    {0xD4AC, 0x5707}, //11918 #CJK UNIFIED IDEOGRAPH
    {0xD4AD, 0x57FB}, //11919 #CJK UNIFIED IDEOGRAPH
    {0xD4AE, 0x5814}, //11920 #CJK UNIFIED IDEOGRAPH
    {0xD4AF, 0x57E2}, //11921 #CJK UNIFIED IDEOGRAPH
    {0xD4B0, 0x57F6}, //11922 #CJK UNIFIED IDEOGRAPH
    {0xD4B1, 0x57DC}, //11923 #CJK UNIFIED IDEOGRAPH
    {0xD4B2, 0x57F4}, //11924 #CJK UNIFIED IDEOGRAPH
    {0xD4B3, 0x5800}, //11925 #CJK UNIFIED IDEOGRAPH
    {0xD4B4, 0x57ED}, //11926 #CJK UNIFIED IDEOGRAPH
    {0xD4B5, 0x57FD}, //11927 #CJK UNIFIED IDEOGRAPH
    {0xD4B6, 0x5808}, //11928 #CJK UNIFIED IDEOGRAPH
    {0xD4B7, 0x57F8}, //11929 #CJK UNIFIED IDEOGRAPH
    {0xD4B8, 0x580B}, //11930 #CJK UNIFIED IDEOGRAPH
    {0xD4B9, 0x57F3}, //11931 #CJK UNIFIED IDEOGRAPH
    {0xD4BA, 0x57CF}, //11932 #CJK UNIFIED IDEOGRAPH
    {0xD4BB, 0x5807}, //11933 #CJK UNIFIED IDEOGRAPH
    {0xD4BC, 0x57EE}, //11934 #CJK UNIFIED IDEOGRAPH
    {0xD4BD, 0x57E3}, //11935 #CJK UNIFIED IDEOGRAPH
    {0xD4BE, 0x57F2}, //11936 #CJK UNIFIED IDEOGRAPH
    {0xD4BF, 0x57E5}, //11937 #CJK UNIFIED IDEOGRAPH
    {0xD4C0, 0x57EC}, //11938 #CJK UNIFIED IDEOGRAPH
    {0xD4C1, 0x57E1}, //11939 #CJK UNIFIED IDEOGRAPH
    {0xD4C2, 0x580E}, //11940 #CJK UNIFIED IDEOGRAPH
    {0xD4C3, 0x57FC}, //11941 #CJK UNIFIED IDEOGRAPH
    {0xD4C4, 0x5810}, //11942 #CJK UNIFIED IDEOGRAPH
    {0xD4C5, 0x57E7}, //11943 #CJK UNIFIED IDEOGRAPH
    {0xD4C6, 0x5801}, //11944 #CJK UNIFIED IDEOGRAPH
    {0xD4C7, 0x580C}, //11945 #CJK UNIFIED IDEOGRAPH
    {0xD4C8, 0x57F1}, //11946 #CJK UNIFIED IDEOGRAPH
    {0xD4C9, 0x57E9}, //11947 #CJK UNIFIED IDEOGRAPH
    {0xD4CA, 0x57F0}, //11948 #CJK UNIFIED IDEOGRAPH
    {0xD4CB, 0x580D}, //11949 #CJK UNIFIED IDEOGRAPH
    {0xD4CC, 0x5804}, //11950 #CJK UNIFIED IDEOGRAPH
    {0xD4CD, 0x595C}, //11951 #CJK UNIFIED IDEOGRAPH
    {0xD4CE, 0x5A60}, //11952 #CJK UNIFIED IDEOGRAPH
    {0xD4CF, 0x5A58}, //11953 #CJK UNIFIED IDEOGRAPH
    {0xD4D0, 0x5A55}, //11954 #CJK UNIFIED IDEOGRAPH
    {0xD4D1, 0x5A67}, //11955 #CJK UNIFIED IDEOGRAPH
    {0xD4D2, 0x5A5E}, //11956 #CJK UNIFIED IDEOGRAPH
    {0xD4D3, 0x5A38}, //11957 #CJK UNIFIED IDEOGRAPH
    {0xD4D4, 0x5A35}, //11958 #CJK UNIFIED IDEOGRAPH
    {0xD4D5, 0x5A6D}, //11959 #CJK UNIFIED IDEOGRAPH
    {0xD4D6, 0x5A50}, //11960 #CJK UNIFIED IDEOGRAPH
    {0xD4D7, 0x5A5F}, //11961 #CJK UNIFIED IDEOGRAPH
    {0xD4D8, 0x5A65}, //11962 #CJK UNIFIED IDEOGRAPH
    {0xD4D9, 0x5A6C}, //11963 #CJK UNIFIED IDEOGRAPH
    {0xD4DA, 0x5A53}, //11964 #CJK UNIFIED IDEOGRAPH
    {0xD4DB, 0x5A64}, //11965 #CJK UNIFIED IDEOGRAPH
    {0xD4DC, 0x5A57}, //11966 #CJK UNIFIED IDEOGRAPH
    {0xD4DD, 0x5A43}, //11967 #CJK UNIFIED IDEOGRAPH
    {0xD4DE, 0x5A5D}, //11968 #CJK UNIFIED IDEOGRAPH
    {0xD4DF, 0x5A52}, //11969 #CJK UNIFIED IDEOGRAPH
    {0xD4E0, 0x5A44}, //11970 #CJK UNIFIED IDEOGRAPH
    {0xD4E1, 0x5A5B}, //11971 #CJK UNIFIED IDEOGRAPH
    {0xD4E2, 0x5A48}, //11972 #CJK UNIFIED IDEOGRAPH
    {0xD4E3, 0x5A8E}, //11973 #CJK UNIFIED IDEOGRAPH
    {0xD4E4, 0x5A3E}, //11974 #CJK UNIFIED IDEOGRAPH
    {0xD4E5, 0x5A4D}, //11975 #CJK UNIFIED IDEOGRAPH
    {0xD4E6, 0x5A39}, //11976 #CJK UNIFIED IDEOGRAPH
    {0xD4E7, 0x5A4C}, //11977 #CJK UNIFIED IDEOGRAPH
    {0xD4E8, 0x5A70}, //11978 #CJK UNIFIED IDEOGRAPH
    {0xD4E9, 0x5A69}, //11979 #CJK UNIFIED IDEOGRAPH
    {0xD4EA, 0x5A47}, //11980 #CJK UNIFIED IDEOGRAPH
    {0xD4EB, 0x5A51}, //11981 #CJK UNIFIED IDEOGRAPH
    {0xD4EC, 0x5A56}, //11982 #CJK UNIFIED IDEOGRAPH
    {0xD4ED, 0x5A42}, //11983 #CJK UNIFIED IDEOGRAPH
    {0xD4EE, 0x5A5C}, //11984 #CJK UNIFIED IDEOGRAPH
    {0xD4EF, 0x5B72}, //11985 #CJK UNIFIED IDEOGRAPH
    {0xD4F0, 0x5B6E}, //11986 #CJK UNIFIED IDEOGRAPH
    {0xD4F1, 0x5BC1}, //11987 #CJK UNIFIED IDEOGRAPH
    {0xD4F2, 0x5BC0}, //11988 #CJK UNIFIED IDEOGRAPH
    {0xD4F3, 0x5C59}, //11989 #CJK UNIFIED IDEOGRAPH
    {0xD4F4, 0x5D1E}, //11990 #CJK UNIFIED IDEOGRAPH
    {0xD4F5, 0x5D0B}, //11991 #CJK UNIFIED IDEOGRAPH
    {0xD4F6, 0x5D1D}, //11992 #CJK UNIFIED IDEOGRAPH
    {0xD4F7, 0x5D1A}, //11993 #CJK UNIFIED IDEOGRAPH
    {0xD4F8, 0x5D20}, //11994 #CJK UNIFIED IDEOGRAPH
    {0xD4F9, 0x5D0C}, //11995 #CJK UNIFIED IDEOGRAPH
    {0xD4FA, 0x5D28}, //11996 #CJK UNIFIED IDEOGRAPH
    {0xD4FB, 0x5D0D}, //11997 #CJK UNIFIED IDEOGRAPH
    {0xD4FC, 0x5D26}, //11998 #CJK UNIFIED IDEOGRAPH
    {0xD4FD, 0x5D25}, //11999 #CJK UNIFIED IDEOGRAPH
    {0xD4FE, 0x5D0F}, //12000 #CJK UNIFIED IDEOGRAPH
    {0xD540, 0x5D30}, //12001 #CJK UNIFIED IDEOGRAPH
    {0xD541, 0x5D12}, //12002 #CJK UNIFIED IDEOGRAPH
    {0xD542, 0x5D23}, //12003 #CJK UNIFIED IDEOGRAPH
    {0xD543, 0x5D1F}, //12004 #CJK UNIFIED IDEOGRAPH
    {0xD544, 0x5D2E}, //12005 #CJK UNIFIED IDEOGRAPH
    {0xD545, 0x5E3E}, //12006 #CJK UNIFIED IDEOGRAPH
    {0xD546, 0x5E34}, //12007 #CJK UNIFIED IDEOGRAPH
    {0xD547, 0x5EB1}, //12008 #CJK UNIFIED IDEOGRAPH
    {0xD548, 0x5EB4}, //12009 #CJK UNIFIED IDEOGRAPH
    {0xD549, 0x5EB9}, //12010 #CJK UNIFIED IDEOGRAPH
    {0xD54A, 0x5EB2}, //12011 #CJK UNIFIED IDEOGRAPH
    {0xD54B, 0x5EB3}, //12012 #CJK UNIFIED IDEOGRAPH
    {0xD54C, 0x5F36}, //12013 #CJK UNIFIED IDEOGRAPH
    {0xD54D, 0x5F38}, //12014 #CJK UNIFIED IDEOGRAPH
    {0xD54E, 0x5F9B}, //12015 #CJK UNIFIED IDEOGRAPH
    {0xD54F, 0x5F96}, //12016 #CJK UNIFIED IDEOGRAPH
    {0xD550, 0x5F9F}, //12017 #CJK UNIFIED IDEOGRAPH
    {0xD551, 0x608A}, //12018 #CJK UNIFIED IDEOGRAPH
    {0xD552, 0x6090}, //12019 #CJK UNIFIED IDEOGRAPH
    {0xD553, 0x6086}, //12020 #CJK UNIFIED IDEOGRAPH
    {0xD554, 0x60BE}, //12021 #CJK UNIFIED IDEOGRAPH
    {0xD555, 0x60B0}, //12022 #CJK UNIFIED IDEOGRAPH
    {0xD556, 0x60BA}, //12023 #CJK UNIFIED IDEOGRAPH
    {0xD557, 0x60D3}, //12024 #CJK UNIFIED IDEOGRAPH
    {0xD558, 0x60D4}, //12025 #CJK UNIFIED IDEOGRAPH
    {0xD559, 0x60CF}, //12026 #CJK UNIFIED IDEOGRAPH
    {0xD55A, 0x60E4}, //12027 #CJK UNIFIED IDEOGRAPH
    {0xD55B, 0x60D9}, //12028 #CJK UNIFIED IDEOGRAPH
    {0xD55C, 0x60DD}, //12029 #CJK UNIFIED IDEOGRAPH
    {0xD55D, 0x60C8}, //12030 #CJK UNIFIED IDEOGRAPH
    {0xD55E, 0x60B1}, //12031 #CJK UNIFIED IDEOGRAPH
    {0xD55F, 0x60DB}, //12032 #CJK UNIFIED IDEOGRAPH
    {0xD560, 0x60B7}, //12033 #CJK UNIFIED IDEOGRAPH
    {0xD561, 0x60CA}, //12034 #CJK UNIFIED IDEOGRAPH
    {0xD562, 0x60BF}, //12035 #CJK UNIFIED IDEOGRAPH
    {0xD563, 0x60C3}, //12036 #CJK UNIFIED IDEOGRAPH
    {0xD564, 0x60CD}, //12037 #CJK UNIFIED IDEOGRAPH
    {0xD565, 0x60C0}, //12038 #CJK UNIFIED IDEOGRAPH
    {0xD566, 0x6332}, //12039 #CJK UNIFIED IDEOGRAPH
    {0xD567, 0x6365}, //12040 #CJK UNIFIED IDEOGRAPH
    {0xD568, 0x638A}, //12041 #CJK UNIFIED IDEOGRAPH
    {0xD569, 0x6382}, //12042 #CJK UNIFIED IDEOGRAPH
    {0xD56A, 0x637D}, //12043 #CJK UNIFIED IDEOGRAPH
    {0xD56B, 0x63BD}, //12044 #CJK UNIFIED IDEOGRAPH
    {0xD56C, 0x639E}, //12045 #CJK UNIFIED IDEOGRAPH
    {0xD56D, 0x63AD}, //12046 #CJK UNIFIED IDEOGRAPH
    {0xD56E, 0x639D}, //12047 #CJK UNIFIED IDEOGRAPH
    {0xD56F, 0x6397}, //12048 #CJK UNIFIED IDEOGRAPH
    {0xD570, 0x63AB}, //12049 #CJK UNIFIED IDEOGRAPH
    {0xD571, 0x638E}, //12050 #CJK UNIFIED IDEOGRAPH
    {0xD572, 0x636F}, //12051 #CJK UNIFIED IDEOGRAPH
    {0xD573, 0x6387}, //12052 #CJK UNIFIED IDEOGRAPH
    {0xD574, 0x6390}, //12053 #CJK UNIFIED IDEOGRAPH
    {0xD575, 0x636E}, //12054 #CJK UNIFIED IDEOGRAPH
    {0xD576, 0x63AF}, //12055 #CJK UNIFIED IDEOGRAPH
    {0xD577, 0x6375}, //12056 #CJK UNIFIED IDEOGRAPH
    {0xD578, 0x639C}, //12057 #CJK UNIFIED IDEOGRAPH
    {0xD579, 0x636D}, //12058 #CJK UNIFIED IDEOGRAPH
    {0xD57A, 0x63AE}, //12059 #CJK UNIFIED IDEOGRAPH
    {0xD57B, 0x637C}, //12060 #CJK UNIFIED IDEOGRAPH
    {0xD57C, 0x63A4}, //12061 #CJK UNIFIED IDEOGRAPH
    {0xD57D, 0x633B}, //12062 #CJK UNIFIED IDEOGRAPH
    {0xD57E, 0x639F}, //12063 #CJK UNIFIED IDEOGRAPH
    {0xD5A1, 0x6378}, //12064 #CJK UNIFIED IDEOGRAPH
    {0xD5A2, 0x6385}, //12065 #CJK UNIFIED IDEOGRAPH
    {0xD5A3, 0x6381}, //12066 #CJK UNIFIED IDEOGRAPH
    {0xD5A4, 0x6391}, //12067 #CJK UNIFIED IDEOGRAPH
    {0xD5A5, 0x638D}, //12068 #CJK UNIFIED IDEOGRAPH
    {0xD5A6, 0x6370}, //12069 #CJK UNIFIED IDEOGRAPH
    {0xD5A7, 0x6553}, //12070 #CJK UNIFIED IDEOGRAPH
    {0xD5A8, 0x65CD}, //12071 #CJK UNIFIED IDEOGRAPH
    {0xD5A9, 0x6665}, //12072 #CJK UNIFIED IDEOGRAPH
    {0xD5AA, 0x6661}, //12073 #CJK UNIFIED IDEOGRAPH
    {0xD5AB, 0x665B}, //12074 #CJK UNIFIED IDEOGRAPH
    {0xD5AC, 0x6659}, //12075 #CJK UNIFIED IDEOGRAPH
    {0xD5AD, 0x665C}, //12076 #CJK UNIFIED IDEOGRAPH
    {0xD5AE, 0x6662}, //12077 #CJK UNIFIED IDEOGRAPH
    {0xD5AF, 0x6718}, //12078 #CJK UNIFIED IDEOGRAPH
    {0xD5B0, 0x6879}, //12079 #CJK UNIFIED IDEOGRAPH
    {0xD5B1, 0x6887}, //12080 #CJK UNIFIED IDEOGRAPH
    {0xD5B2, 0x6890}, //12081 #CJK UNIFIED IDEOGRAPH
    {0xD5B3, 0x689C}, //12082 #CJK UNIFIED IDEOGRAPH
    {0xD5B4, 0x686D}, //12083 #CJK UNIFIED IDEOGRAPH
    {0xD5B5, 0x686E}, //12084 #CJK UNIFIED IDEOGRAPH
    {0xD5B6, 0x68AE}, //12085 #CJK UNIFIED IDEOGRAPH
    {0xD5B7, 0x68AB}, //12086 #CJK UNIFIED IDEOGRAPH
    {0xD5B8, 0x6956}, //12087 #CJK UNIFIED IDEOGRAPH
    {0xD5B9, 0x686F}, //12088 #CJK UNIFIED IDEOGRAPH
    {0xD5BA, 0x68A3}, //12089 #CJK UNIFIED IDEOGRAPH
    {0xD5BB, 0x68AC}, //12090 #CJK UNIFIED IDEOGRAPH
    {0xD5BC, 0x68A9}, //12091 #CJK UNIFIED IDEOGRAPH
    {0xD5BD, 0x6875}, //12092 #CJK UNIFIED IDEOGRAPH
    {0xD5BE, 0x6874}, //12093 #CJK UNIFIED IDEOGRAPH
    {0xD5BF, 0x68B2}, //12094 #CJK UNIFIED IDEOGRAPH
    {0xD5C0, 0x688F}, //12095 #CJK UNIFIED IDEOGRAPH
    {0xD5C1, 0x6877}, //12096 #CJK UNIFIED IDEOGRAPH
    {0xD5C2, 0x6892}, //12097 #CJK UNIFIED IDEOGRAPH
    {0xD5C3, 0x687C}, //12098 #CJK UNIFIED IDEOGRAPH
    {0xD5C4, 0x686B}, //12099 #CJK UNIFIED IDEOGRAPH
    {0xD5C5, 0x6872}, //12100 #CJK UNIFIED IDEOGRAPH
    {0xD5C6, 0x68AA}, //12101 #CJK UNIFIED IDEOGRAPH
    {0xD5C7, 0x6880}, //12102 #CJK UNIFIED IDEOGRAPH
    {0xD5C8, 0x6871}, //12103 #CJK UNIFIED IDEOGRAPH
    {0xD5C9, 0x687E}, //12104 #CJK UNIFIED IDEOGRAPH
    {0xD5CA, 0x689B}, //12105 #CJK UNIFIED IDEOGRAPH
    {0xD5CB, 0x6896}, //12106 #CJK UNIFIED IDEOGRAPH
    {0xD5CC, 0x688B}, //12107 #CJK UNIFIED IDEOGRAPH
    {0xD5CD, 0x68A0}, //12108 #CJK UNIFIED IDEOGRAPH
    {0xD5CE, 0x6889}, //12109 #CJK UNIFIED IDEOGRAPH
    {0xD5CF, 0x68A4}, //12110 #CJK UNIFIED IDEOGRAPH
    {0xD5D0, 0x6878}, //12111 #CJK UNIFIED IDEOGRAPH
    {0xD5D1, 0x687B}, //12112 #CJK UNIFIED IDEOGRAPH
    {0xD5D2, 0x6891}, //12113 #CJK UNIFIED IDEOGRAPH
    {0xD5D3, 0x688C}, //12114 #CJK UNIFIED IDEOGRAPH
    {0xD5D4, 0x688A}, //12115 #CJK UNIFIED IDEOGRAPH
    {0xD5D5, 0x687D}, //12116 #CJK UNIFIED IDEOGRAPH
    {0xD5D6, 0x6B36}, //12117 #CJK UNIFIED IDEOGRAPH
    {0xD5D7, 0x6B33}, //12118 #CJK UNIFIED IDEOGRAPH
    {0xD5D8, 0x6B37}, //12119 #CJK UNIFIED IDEOGRAPH
    {0xD5D9, 0x6B38}, //12120 #CJK UNIFIED IDEOGRAPH
    {0xD5DA, 0x6B91}, //12121 #CJK UNIFIED IDEOGRAPH
    {0xD5DB, 0x6B8F}, //12122 #CJK UNIFIED IDEOGRAPH
    {0xD5DC, 0x6B8D}, //12123 #CJK UNIFIED IDEOGRAPH
    {0xD5DD, 0x6B8E}, //12124 #CJK UNIFIED IDEOGRAPH
    {0xD5DE, 0x6B8C}, //12125 #CJK UNIFIED IDEOGRAPH
    {0xD5DF, 0x6C2A}, //12126 #CJK UNIFIED IDEOGRAPH
    {0xD5E0, 0x6DC0}, //12127 #CJK UNIFIED IDEOGRAPH
    {0xD5E1, 0x6DAB}, //12128 #CJK UNIFIED IDEOGRAPH
    {0xD5E2, 0x6DB4}, //12129 #CJK UNIFIED IDEOGRAPH
    {0xD5E3, 0x6DB3}, //12130 #CJK UNIFIED IDEOGRAPH
    {0xD5E4, 0x6E74}, //12131 #CJK UNIFIED IDEOGRAPH
    {0xD5E5, 0x6DAC}, //12132 #CJK UNIFIED IDEOGRAPH
    {0xD5E6, 0x6DE9}, //12133 #CJK UNIFIED IDEOGRAPH
    {0xD5E7, 0x6DE2}, //12134 #CJK UNIFIED IDEOGRAPH
    {0xD5E8, 0x6DB7}, //12135 #CJK UNIFIED IDEOGRAPH
    {0xD5E9, 0x6DF6}, //12136 #CJK UNIFIED IDEOGRAPH
    {0xD5EA, 0x6DD4}, //12137 #CJK UNIFIED IDEOGRAPH
    {0xD5EB, 0x6E00}, //12138 #CJK UNIFIED IDEOGRAPH
    {0xD5EC, 0x6DC8}, //12139 #CJK UNIFIED IDEOGRAPH
    {0xD5ED, 0x6DE0}, //12140 #CJK UNIFIED IDEOGRAPH
    {0xD5EE, 0x6DDF}, //12141 #CJK UNIFIED IDEOGRAPH
    {0xD5EF, 0x6DD6}, //12142 #CJK UNIFIED IDEOGRAPH
    {0xD5F0, 0x6DBE}, //12143 #CJK UNIFIED IDEOGRAPH
    {0xD5F1, 0x6DE5}, //12144 #CJK UNIFIED IDEOGRAPH
    {0xD5F2, 0x6DDC}, //12145 #CJK UNIFIED IDEOGRAPH
    {0xD5F3, 0x6DDD}, //12146 #CJK UNIFIED IDEOGRAPH
    {0xD5F4, 0x6DDB}, //12147 #CJK UNIFIED IDEOGRAPH
    {0xD5F5, 0x6DF4}, //12148 #CJK UNIFIED IDEOGRAPH
    {0xD5F6, 0x6DCA}, //12149 #CJK UNIFIED IDEOGRAPH
    {0xD5F7, 0x6DBD}, //12150 #CJK UNIFIED IDEOGRAPH
    {0xD5F8, 0x6DED}, //12151 #CJK UNIFIED IDEOGRAPH
    {0xD5F9, 0x6DF0}, //12152 #CJK UNIFIED IDEOGRAPH
    {0xD5FA, 0x6DBA}, //12153 #CJK UNIFIED IDEOGRAPH
    {0xD5FB, 0x6DD5}, //12154 #CJK UNIFIED IDEOGRAPH
    {0xD5FC, 0x6DC2}, //12155 #CJK UNIFIED IDEOGRAPH
    {0xD5FD, 0x6DCF}, //12156 #CJK UNIFIED IDEOGRAPH
    {0xD5FE, 0x6DC9}, //12157 #CJK UNIFIED IDEOGRAPH
    {0xD640, 0x6DD0}, //12158 #CJK UNIFIED IDEOGRAPH
    {0xD641, 0x6DF2}, //12159 #CJK UNIFIED IDEOGRAPH
    {0xD642, 0x6DD3}, //12160 #CJK UNIFIED IDEOGRAPH
    {0xD643, 0x6DFD}, //12161 #CJK UNIFIED IDEOGRAPH
    {0xD644, 0x6DD7}, //12162 #CJK UNIFIED IDEOGRAPH
    {0xD645, 0x6DCD}, //12163 #CJK UNIFIED IDEOGRAPH
    {0xD646, 0x6DE3}, //12164 #CJK UNIFIED IDEOGRAPH
    {0xD647, 0x6DBB}, //12165 #CJK UNIFIED IDEOGRAPH
    {0xD648, 0x70FA}, //12166 #CJK UNIFIED IDEOGRAPH
    {0xD649, 0x710D}, //12167 #CJK UNIFIED IDEOGRAPH
    {0xD64A, 0x70F7}, //12168 #CJK UNIFIED IDEOGRAPH
    {0xD64B, 0x7117}, //12169 #CJK UNIFIED IDEOGRAPH
    {0xD64C, 0x70F4}, //12170 #CJK UNIFIED IDEOGRAPH
    {0xD64D, 0x710C}, //12171 #CJK UNIFIED IDEOGRAPH
    {0xD64E, 0x70F0}, //12172 #CJK UNIFIED IDEOGRAPH
    {0xD64F, 0x7104}, //12173 #CJK UNIFIED IDEOGRAPH
    {0xD650, 0x70F3}, //12174 #CJK UNIFIED IDEOGRAPH
    {0xD651, 0x7110}, //12175 #CJK UNIFIED IDEOGRAPH
    {0xD652, 0x70FC}, //12176 #CJK UNIFIED IDEOGRAPH
    {0xD653, 0x70FF}, //12177 #CJK UNIFIED IDEOGRAPH
    {0xD654, 0x7106}, //12178 #CJK UNIFIED IDEOGRAPH
    {0xD655, 0x7113}, //12179 #CJK UNIFIED IDEOGRAPH
    {0xD656, 0x7100}, //12180 #CJK UNIFIED IDEOGRAPH
    {0xD657, 0x70F8}, //12181 #CJK UNIFIED IDEOGRAPH
    {0xD658, 0x70F6}, //12182 #CJK UNIFIED IDEOGRAPH
    {0xD659, 0x710B}, //12183 #CJK UNIFIED IDEOGRAPH
    {0xD65A, 0x7102}, //12184 #CJK UNIFIED IDEOGRAPH
    {0xD65B, 0x710E}, //12185 #CJK UNIFIED IDEOGRAPH
    {0xD65C, 0x727E}, //12186 #CJK UNIFIED IDEOGRAPH
    {0xD65D, 0x727B}, //12187 #CJK UNIFIED IDEOGRAPH
    {0xD65E, 0x727C}, //12188 #CJK UNIFIED IDEOGRAPH
    {0xD65F, 0x727F}, //12189 #CJK UNIFIED IDEOGRAPH
    {0xD660, 0x731D}, //12190 #CJK UNIFIED IDEOGRAPH
    {0xD661, 0x7317}, //12191 #CJK UNIFIED IDEOGRAPH
    {0xD662, 0x7307}, //12192 #CJK UNIFIED IDEOGRAPH
    {0xD663, 0x7311}, //12193 #CJK UNIFIED IDEOGRAPH
    {0xD664, 0x7318}, //12194 #CJK UNIFIED IDEOGRAPH
    {0xD665, 0x730A}, //12195 #CJK UNIFIED IDEOGRAPH
    {0xD666, 0x7308}, //12196 #CJK UNIFIED IDEOGRAPH
    {0xD667, 0x72FF}, //12197 #CJK UNIFIED IDEOGRAPH
    {0xD668, 0x730F}, //12198 #CJK UNIFIED IDEOGRAPH
    {0xD669, 0x731E}, //12199 #CJK UNIFIED IDEOGRAPH
    {0xD66A, 0x7388}, //12200 #CJK UNIFIED IDEOGRAPH
    {0xD66B, 0x73F6}, //12201 #CJK UNIFIED IDEOGRAPH
    {0xD66C, 0x73F8}, //12202 #CJK UNIFIED IDEOGRAPH
    {0xD66D, 0x73F5}, //12203 #CJK UNIFIED IDEOGRAPH
    {0xD66E, 0x7404}, //12204 #CJK UNIFIED IDEOGRAPH
    {0xD66F, 0x7401}, //12205 #CJK UNIFIED IDEOGRAPH
    {0xD670, 0x73FD}, //12206 #CJK UNIFIED IDEOGRAPH
    {0xD671, 0x7407}, //12207 #CJK UNIFIED IDEOGRAPH
    {0xD672, 0x7400}, //12208 #CJK UNIFIED IDEOGRAPH
    {0xD673, 0x73FA}, //12209 #CJK UNIFIED IDEOGRAPH
    {0xD674, 0x73FC}, //12210 #CJK UNIFIED IDEOGRAPH
    {0xD675, 0x73FF}, //12211 #CJK UNIFIED IDEOGRAPH
    {0xD676, 0x740C}, //12212 #CJK UNIFIED IDEOGRAPH
    {0xD677, 0x740B}, //12213 #CJK UNIFIED IDEOGRAPH
    {0xD678, 0x73F4}, //12214 #CJK UNIFIED IDEOGRAPH
    {0xD679, 0x7408}, //12215 #CJK UNIFIED IDEOGRAPH
    {0xD67A, 0x7564}, //12216 #CJK UNIFIED IDEOGRAPH
    {0xD67B, 0x7563}, //12217 #CJK UNIFIED IDEOGRAPH
    {0xD67C, 0x75CE}, //12218 #CJK UNIFIED IDEOGRAPH
    {0xD67D, 0x75D2}, //12219 #CJK UNIFIED IDEOGRAPH
    {0xD67E, 0x75CF}, //12220 #CJK UNIFIED IDEOGRAPH
    {0xD6A1, 0x75CB}, //12221 #CJK UNIFIED IDEOGRAPH
    {0xD6A2, 0x75CC}, //12222 #CJK UNIFIED IDEOGRAPH
    {0xD6A3, 0x75D1}, //12223 #CJK UNIFIED IDEOGRAPH
    {0xD6A4, 0x75D0}, //12224 #CJK UNIFIED IDEOGRAPH
    {0xD6A5, 0x768F}, //12225 #CJK UNIFIED IDEOGRAPH
    {0xD6A6, 0x7689}, //12226 #CJK UNIFIED IDEOGRAPH
    {0xD6A7, 0x76D3}, //12227 #CJK UNIFIED IDEOGRAPH
    {0xD6A8, 0x7739}, //12228 #CJK UNIFIED IDEOGRAPH
    {0xD6A9, 0x772F}, //12229 #CJK UNIFIED IDEOGRAPH
    {0xD6AA, 0x772D}, //12230 #CJK UNIFIED IDEOGRAPH
    {0xD6AB, 0x7731}, //12231 #CJK UNIFIED IDEOGRAPH
    {0xD6AC, 0x7732}, //12232 #CJK UNIFIED IDEOGRAPH
    {0xD6AD, 0x7734}, //12233 #CJK UNIFIED IDEOGRAPH
    {0xD6AE, 0x7733}, //12234 #CJK UNIFIED IDEOGRAPH
    {0xD6AF, 0x773D}, //12235 #CJK UNIFIED IDEOGRAPH
    {0xD6B0, 0x7725}, //12236 #CJK UNIFIED IDEOGRAPH
    {0xD6B1, 0x773B}, //12237 #CJK UNIFIED IDEOGRAPH
    {0xD6B2, 0x7735}, //12238 #CJK UNIFIED IDEOGRAPH
    {0xD6B3, 0x7848}, //12239 #CJK UNIFIED IDEOGRAPH
    {0xD6B4, 0x7852}, //12240 #CJK UNIFIED IDEOGRAPH
    {0xD6B5, 0x7849}, //12241 #CJK UNIFIED IDEOGRAPH
    {0xD6B6, 0x784D}, //12242 #CJK UNIFIED IDEOGRAPH
    {0xD6B7, 0x784A}, //12243 #CJK UNIFIED IDEOGRAPH
    {0xD6B8, 0x784C}, //12244 #CJK UNIFIED IDEOGRAPH
    {0xD6B9, 0x7826}, //12245 #CJK UNIFIED IDEOGRAPH
    {0xD6BA, 0x7845}, //12246 #CJK UNIFIED IDEOGRAPH
    {0xD6BB, 0x7850}, //12247 #CJK UNIFIED IDEOGRAPH
    {0xD6BC, 0x7964}, //12248 #CJK UNIFIED IDEOGRAPH
    {0xD6BD, 0x7967}, //12249 #CJK UNIFIED IDEOGRAPH
    {0xD6BE, 0x7969}, //12250 #CJK UNIFIED IDEOGRAPH
    {0xD6BF, 0x796A}, //12251 #CJK UNIFIED IDEOGRAPH
    {0xD6C0, 0x7963}, //12252 #CJK UNIFIED IDEOGRAPH
    {0xD6C1, 0x796B}, //12253 #CJK UNIFIED IDEOGRAPH
    {0xD6C2, 0x7961}, //12254 #CJK UNIFIED IDEOGRAPH
    {0xD6C3, 0x79BB}, //12255 #CJK UNIFIED IDEOGRAPH
    {0xD6C4, 0x79FA}, //12256 #CJK UNIFIED IDEOGRAPH
    {0xD6C5, 0x79F8}, //12257 #CJK UNIFIED IDEOGRAPH
    {0xD6C6, 0x79F6}, //12258 #CJK UNIFIED IDEOGRAPH
    {0xD6C7, 0x79F7}, //12259 #CJK UNIFIED IDEOGRAPH
    {0xD6C8, 0x7A8F}, //12260 #CJK UNIFIED IDEOGRAPH
    {0xD6C9, 0x7A94}, //12261 #CJK UNIFIED IDEOGRAPH
    {0xD6CA, 0x7A90}, //12262 #CJK UNIFIED IDEOGRAPH
    {0xD6CB, 0x7B35}, //12263 #CJK UNIFIED IDEOGRAPH
    {0xD6CC, 0x7B47}, //12264 #CJK UNIFIED IDEOGRAPH
    {0xD6CD, 0x7B34}, //12265 #CJK UNIFIED IDEOGRAPH
    {0xD6CE, 0x7B25}, //12266 #CJK UNIFIED IDEOGRAPH
    {0xD6CF, 0x7B30}, //12267 #CJK UNIFIED IDEOGRAPH
    {0xD6D0, 0x7B22}, //12268 #CJK UNIFIED IDEOGRAPH
    {0xD6D1, 0x7B24}, //12269 #CJK UNIFIED IDEOGRAPH
    {0xD6D2, 0x7B33}, //12270 #CJK UNIFIED IDEOGRAPH
    {0xD6D3, 0x7B18}, //12271 #CJK UNIFIED IDEOGRAPH
    {0xD6D4, 0x7B2A}, //12272 #CJK UNIFIED IDEOGRAPH
    {0xD6D5, 0x7B1D}, //12273 #CJK UNIFIED IDEOGRAPH
    {0xD6D6, 0x7B31}, //12274 #CJK UNIFIED IDEOGRAPH
    {0xD6D7, 0x7B2B}, //12275 #CJK UNIFIED IDEOGRAPH
    {0xD6D8, 0x7B2D}, //12276 #CJK UNIFIED IDEOGRAPH
    {0xD6D9, 0x7B2F}, //12277 #CJK UNIFIED IDEOGRAPH
    {0xD6DA, 0x7B32}, //12278 #CJK UNIFIED IDEOGRAPH
    {0xD6DB, 0x7B38}, //12279 #CJK UNIFIED IDEOGRAPH
    {0xD6DC, 0x7B1A}, //12280 #CJK UNIFIED IDEOGRAPH
    {0xD6DD, 0x7B23}, //12281 #CJK UNIFIED IDEOGRAPH
    {0xD6DE, 0x7C94}, //12282 #CJK UNIFIED IDEOGRAPH
    {0xD6DF, 0x7C98}, //12283 #CJK UNIFIED IDEOGRAPH
    {0xD6E0, 0x7C96}, //12284 #CJK UNIFIED IDEOGRAPH
    {0xD6E1, 0x7CA3}, //12285 #CJK UNIFIED IDEOGRAPH
    {0xD6E2, 0x7D35}, //12286 #CJK UNIFIED IDEOGRAPH
    {0xD6E3, 0x7D3D}, //12287 #CJK UNIFIED IDEOGRAPH
    {0xD6E4, 0x7D38}, //12288 #CJK UNIFIED IDEOGRAPH
    {0xD6E5, 0x7D36}, //12289 #CJK UNIFIED IDEOGRAPH
    {0xD6E6, 0x7D3A}, //12290 #CJK UNIFIED IDEOGRAPH
    {0xD6E7, 0x7D45}, //12291 #CJK UNIFIED IDEOGRAPH
    {0xD6E8, 0x7D2C}, //12292 #CJK UNIFIED IDEOGRAPH
    {0xD6E9, 0x7D29}, //12293 #CJK UNIFIED IDEOGRAPH
    {0xD6EA, 0x7D41}, //12294 #CJK UNIFIED IDEOGRAPH
    {0xD6EB, 0x7D47}, //12295 #CJK UNIFIED IDEOGRAPH
    {0xD6EC, 0x7D3E}, //12296 #CJK UNIFIED IDEOGRAPH
    {0xD6ED, 0x7D3F}, //12297 #CJK UNIFIED IDEOGRAPH
    {0xD6EE, 0x7D4A}, //12298 #CJK UNIFIED IDEOGRAPH
    {0xD6EF, 0x7D3B}, //12299 #CJK UNIFIED IDEOGRAPH
    {0xD6F0, 0x7D28}, //12300 #CJK UNIFIED IDEOGRAPH
    {0xD6F1, 0x7F63}, //12301 #CJK UNIFIED IDEOGRAPH
    {0xD6F2, 0x7F95}, //12302 #CJK UNIFIED IDEOGRAPH
    {0xD6F3, 0x7F9C}, //12303 #CJK UNIFIED IDEOGRAPH
    {0xD6F4, 0x7F9D}, //12304 #CJK UNIFIED IDEOGRAPH
    {0xD6F5, 0x7F9B}, //12305 #CJK UNIFIED IDEOGRAPH
    {0xD6F6, 0x7FCA}, //12306 #CJK UNIFIED IDEOGRAPH
    {0xD6F7, 0x7FCB}, //12307 #CJK UNIFIED IDEOGRAPH
    {0xD6F8, 0x7FCD}, //12308 #CJK UNIFIED IDEOGRAPH
    {0xD6F9, 0x7FD0}, //12309 #CJK UNIFIED IDEOGRAPH
    {0xD6FA, 0x7FD1}, //12310 #CJK UNIFIED IDEOGRAPH
    {0xD6FB, 0x7FC7}, //12311 #CJK UNIFIED IDEOGRAPH
    {0xD6FC, 0x7FCF}, //12312 #CJK UNIFIED IDEOGRAPH
    {0xD6FD, 0x7FC9}, //12313 #CJK UNIFIED IDEOGRAPH
    {0xD6FE, 0x801F}, //12314 #CJK UNIFIED IDEOGRAPH
    {0xD740, 0x801E}, //12315 #CJK UNIFIED IDEOGRAPH
    {0xD741, 0x801B}, //12316 #CJK UNIFIED IDEOGRAPH
    {0xD742, 0x8047}, //12317 #CJK UNIFIED IDEOGRAPH
    {0xD743, 0x8043}, //12318 #CJK UNIFIED IDEOGRAPH
    {0xD744, 0x8048}, //12319 #CJK UNIFIED IDEOGRAPH
    {0xD745, 0x8118}, //12320 #CJK UNIFIED IDEOGRAPH
    {0xD746, 0x8125}, //12321 #CJK UNIFIED IDEOGRAPH
    {0xD747, 0x8119}, //12322 #CJK UNIFIED IDEOGRAPH
    {0xD748, 0x811B}, //12323 #CJK UNIFIED IDEOGRAPH
    {0xD749, 0x812D}, //12324 #CJK UNIFIED IDEOGRAPH
    {0xD74A, 0x811F}, //12325 #CJK UNIFIED IDEOGRAPH
    {0xD74B, 0x812C}, //12326 #CJK UNIFIED IDEOGRAPH
    {0xD74C, 0x811E}, //12327 #CJK UNIFIED IDEOGRAPH
    {0xD74D, 0x8121}, //12328 #CJK UNIFIED IDEOGRAPH
    {0xD74E, 0x8115}, //12329 #CJK UNIFIED IDEOGRAPH
    {0xD74F, 0x8127}, //12330 #CJK UNIFIED IDEOGRAPH
    {0xD750, 0x811D}, //12331 #CJK UNIFIED IDEOGRAPH
    {0xD751, 0x8122}, //12332 #CJK UNIFIED IDEOGRAPH
    {0xD752, 0x8211}, //12333 #CJK UNIFIED IDEOGRAPH
    {0xD753, 0x8238}, //12334 #CJK UNIFIED IDEOGRAPH
    {0xD754, 0x8233}, //12335 #CJK UNIFIED IDEOGRAPH
    {0xD755, 0x823A}, //12336 #CJK UNIFIED IDEOGRAPH
    {0xD756, 0x8234}, //12337 #CJK UNIFIED IDEOGRAPH
    {0xD757, 0x8232}, //12338 #CJK UNIFIED IDEOGRAPH
    {0xD758, 0x8274}, //12339 #CJK UNIFIED IDEOGRAPH
    {0xD759, 0x8390}, //12340 #CJK UNIFIED IDEOGRAPH
    {0xD75A, 0x83A3}, //12341 #CJK UNIFIED IDEOGRAPH
    {0xD75B, 0x83A8}, //12342 #CJK UNIFIED IDEOGRAPH
    {0xD75C, 0x838D}, //12343 #CJK UNIFIED IDEOGRAPH
    {0xD75D, 0x837A}, //12344 #CJK UNIFIED IDEOGRAPH
    {0xD75E, 0x8373}, //12345 #CJK UNIFIED IDEOGRAPH
    {0xD75F, 0x83A4}, //12346 #CJK UNIFIED IDEOGRAPH
    {0xD760, 0x8374}, //12347 #CJK UNIFIED IDEOGRAPH
    {0xD761, 0x838F}, //12348 #CJK UNIFIED IDEOGRAPH
    {0xD762, 0x8381}, //12349 #CJK UNIFIED IDEOGRAPH
    {0xD763, 0x8395}, //12350 #CJK UNIFIED IDEOGRAPH
    {0xD764, 0x8399}, //12351 #CJK UNIFIED IDEOGRAPH
    {0xD765, 0x8375}, //12352 #CJK UNIFIED IDEOGRAPH
    {0xD766, 0x8394}, //12353 #CJK UNIFIED IDEOGRAPH
    {0xD767, 0x83A9}, //12354 #CJK UNIFIED IDEOGRAPH
    {0xD768, 0x837D}, //12355 #CJK UNIFIED IDEOGRAPH
    {0xD769, 0x8383}, //12356 #CJK UNIFIED IDEOGRAPH
    {0xD76A, 0x838C}, //12357 #CJK UNIFIED IDEOGRAPH
    {0xD76B, 0x839D}, //12358 #CJK UNIFIED IDEOGRAPH
    {0xD76C, 0x839B}, //12359 #CJK UNIFIED IDEOGRAPH
    {0xD76D, 0x83AA}, //12360 #CJK UNIFIED IDEOGRAPH
    {0xD76E, 0x838B}, //12361 #CJK UNIFIED IDEOGRAPH
    {0xD76F, 0x837E}, //12362 #CJK UNIFIED IDEOGRAPH
    {0xD770, 0x83A5}, //12363 #CJK UNIFIED IDEOGRAPH
    {0xD771, 0x83AF}, //12364 #CJK UNIFIED IDEOGRAPH
    {0xD772, 0x8388}, //12365 #CJK UNIFIED IDEOGRAPH
    {0xD773, 0x8397}, //12366 #CJK UNIFIED IDEOGRAPH
    {0xD774, 0x83B0}, //12367 #CJK UNIFIED IDEOGRAPH
    {0xD775, 0x837F}, //12368 #CJK UNIFIED IDEOGRAPH
    {0xD776, 0x83A6}, //12369 #CJK UNIFIED IDEOGRAPH
    {0xD777, 0x8387}, //12370 #CJK UNIFIED IDEOGRAPH
    {0xD778, 0x83AE}, //12371 #CJK UNIFIED IDEOGRAPH
    {0xD779, 0x8376}, //12372 #CJK UNIFIED IDEOGRAPH
    {0xD77A, 0x839A}, //12373 #CJK UNIFIED IDEOGRAPH
    {0xD77B, 0x8659}, //12374 #CJK UNIFIED IDEOGRAPH
    {0xD77C, 0x8656}, //12375 #CJK UNIFIED IDEOGRAPH
    {0xD77D, 0x86BF}, //12376 #CJK UNIFIED IDEOGRAPH
    {0xD77E, 0x86B7}, //12377 #CJK UNIFIED IDEOGRAPH
    {0xD7A1, 0x86C2}, //12378 #CJK UNIFIED IDEOGRAPH
    {0xD7A2, 0x86C1}, //12379 #CJK UNIFIED IDEOGRAPH
    {0xD7A3, 0x86C5}, //12380 #CJK UNIFIED IDEOGRAPH
    {0xD7A4, 0x86BA}, //12381 #CJK UNIFIED IDEOGRAPH
    {0xD7A5, 0x86B0}, //12382 #CJK UNIFIED IDEOGRAPH
    {0xD7A6, 0x86C8}, //12383 #CJK UNIFIED IDEOGRAPH
    {0xD7A7, 0x86B9}, //12384 #CJK UNIFIED IDEOGRAPH
    {0xD7A8, 0x86B3}, //12385 #CJK UNIFIED IDEOGRAPH
    {0xD7A9, 0x86B8}, //12386 #CJK UNIFIED IDEOGRAPH
    {0xD7AA, 0x86CC}, //12387 #CJK UNIFIED IDEOGRAPH
    {0xD7AB, 0x86B4}, //12388 #CJK UNIFIED IDEOGRAPH
    {0xD7AC, 0x86BB}, //12389 #CJK UNIFIED IDEOGRAPH
    {0xD7AD, 0x86BC}, //12390 #CJK UNIFIED IDEOGRAPH
    {0xD7AE, 0x86C3}, //12391 #CJK UNIFIED IDEOGRAPH
    {0xD7AF, 0x86BD}, //12392 #CJK UNIFIED IDEOGRAPH
    {0xD7B0, 0x86BE}, //12393 #CJK UNIFIED IDEOGRAPH
    {0xD7B1, 0x8852}, //12394 #CJK UNIFIED IDEOGRAPH
    {0xD7B2, 0x8889}, //12395 #CJK UNIFIED IDEOGRAPH
    {0xD7B3, 0x8895}, //12396 #CJK UNIFIED IDEOGRAPH
    {0xD7B4, 0x88A8}, //12397 #CJK UNIFIED IDEOGRAPH
    {0xD7B5, 0x88A2}, //12398 #CJK UNIFIED IDEOGRAPH
    {0xD7B6, 0x88AA}, //12399 #CJK UNIFIED IDEOGRAPH
    {0xD7B7, 0x889A}, //12400 #CJK UNIFIED IDEOGRAPH
    {0xD7B8, 0x8891}, //12401 #CJK UNIFIED IDEOGRAPH
    {0xD7B9, 0x88A1}, //12402 #CJK UNIFIED IDEOGRAPH
    {0xD7BA, 0x889F}, //12403 #CJK UNIFIED IDEOGRAPH
    {0xD7BB, 0x8898}, //12404 #CJK UNIFIED IDEOGRAPH
    {0xD7BC, 0x88A7}, //12405 #CJK UNIFIED IDEOGRAPH
    {0xD7BD, 0x8899}, //12406 #CJK UNIFIED IDEOGRAPH
    {0xD7BE, 0x889B}, //12407 #CJK UNIFIED IDEOGRAPH
    {0xD7BF, 0x8897}, //12408 #CJK UNIFIED IDEOGRAPH
    {0xD7C0, 0x88A4}, //12409 #CJK UNIFIED IDEOGRAPH
    {0xD7C1, 0x88AC}, //12410 #CJK UNIFIED IDEOGRAPH
    {0xD7C2, 0x888C}, //12411 #CJK UNIFIED IDEOGRAPH
    {0xD7C3, 0x8893}, //12412 #CJK UNIFIED IDEOGRAPH
    {0xD7C4, 0x888E}, //12413 #CJK UNIFIED IDEOGRAPH
    {0xD7C5, 0x8982}, //12414 #CJK UNIFIED IDEOGRAPH
    {0xD7C6, 0x89D6}, //12415 #CJK UNIFIED IDEOGRAPH
    {0xD7C7, 0x89D9}, //12416 #CJK UNIFIED IDEOGRAPH
    {0xD7C8, 0x89D5}, //12417 #CJK UNIFIED IDEOGRAPH
    {0xD7C9, 0x8A30}, //12418 #CJK UNIFIED IDEOGRAPH
    {0xD7CA, 0x8A27}, //12419 #CJK UNIFIED IDEOGRAPH
    {0xD7CB, 0x8A2C}, //12420 #CJK UNIFIED IDEOGRAPH
    {0xD7CC, 0x8A1E}, //12421 #CJK UNIFIED IDEOGRAPH
    {0xD7CD, 0x8C39}, //12422 #CJK UNIFIED IDEOGRAPH
    {0xD7CE, 0x8C3B}, //12423 #CJK UNIFIED IDEOGRAPH
    {0xD7CF, 0x8C5C}, //12424 #CJK UNIFIED IDEOGRAPH
    {0xD7D0, 0x8C5D}, //12425 #CJK UNIFIED IDEOGRAPH
    {0xD7D1, 0x8C7D}, //12426 #CJK UNIFIED IDEOGRAPH
    {0xD7D2, 0x8CA5}, //12427 #CJK UNIFIED IDEOGRAPH
    {0xD7D3, 0x8D7D}, //12428 #CJK UNIFIED IDEOGRAPH
    {0xD7D4, 0x8D7B}, //12429 #CJK UNIFIED IDEOGRAPH
    {0xD7D5, 0x8D79}, //12430 #CJK UNIFIED IDEOGRAPH
    {0xD7D6, 0x8DBC}, //12431 #CJK UNIFIED IDEOGRAPH
    {0xD7D7, 0x8DC2}, //12432 #CJK UNIFIED IDEOGRAPH
    {0xD7D8, 0x8DB9}, //12433 #CJK UNIFIED IDEOGRAPH
    {0xD7D9, 0x8DBF}, //12434 #CJK UNIFIED IDEOGRAPH
    {0xD7DA, 0x8DC1}, //12435 #CJK UNIFIED IDEOGRAPH
    {0xD7DB, 0x8ED8}, //12436 #CJK UNIFIED IDEOGRAPH
    {0xD7DC, 0x8EDE}, //12437 #CJK UNIFIED IDEOGRAPH
    {0xD7DD, 0x8EDD}, //12438 #CJK UNIFIED IDEOGRAPH
    {0xD7DE, 0x8EDC}, //12439 #CJK UNIFIED IDEOGRAPH
    {0xD7DF, 0x8ED7}, //12440 #CJK UNIFIED IDEOGRAPH
    {0xD7E0, 0x8EE0}, //12441 #CJK UNIFIED IDEOGRAPH
    {0xD7E1, 0x8EE1}, //12442 #CJK UNIFIED IDEOGRAPH
    {0xD7E2, 0x9024}, //12443 #CJK UNIFIED IDEOGRAPH
    {0xD7E3, 0x900B}, //12444 #CJK UNIFIED IDEOGRAPH
    {0xD7E4, 0x9011}, //12445 #CJK UNIFIED IDEOGRAPH
    {0xD7E5, 0x901C}, //12446 #CJK UNIFIED IDEOGRAPH
    {0xD7E6, 0x900C}, //12447 #CJK UNIFIED IDEOGRAPH
    {0xD7E7, 0x9021}, //12448 #CJK UNIFIED IDEOGRAPH
    {0xD7E8, 0x90EF}, //12449 #CJK UNIFIED IDEOGRAPH
    {0xD7E9, 0x90EA}, //12450 #CJK UNIFIED IDEOGRAPH
    {0xD7EA, 0x90F0}, //12451 #CJK UNIFIED IDEOGRAPH
    {0xD7EB, 0x90F4}, //12452 #CJK UNIFIED IDEOGRAPH
    {0xD7EC, 0x90F2}, //12453 #CJK UNIFIED IDEOGRAPH
    {0xD7ED, 0x90F3}, //12454 #CJK UNIFIED IDEOGRAPH
    {0xD7EE, 0x90D4}, //12455 #CJK UNIFIED IDEOGRAPH
    {0xD7EF, 0x90EB}, //12456 #CJK UNIFIED IDEOGRAPH
    {0xD7F0, 0x90EC}, //12457 #CJK UNIFIED IDEOGRAPH
    {0xD7F1, 0x90E9}, //12458 #CJK UNIFIED IDEOGRAPH
    {0xD7F2, 0x9156}, //12459 #CJK UNIFIED IDEOGRAPH
    {0xD7F3, 0x9158}, //12460 #CJK UNIFIED IDEOGRAPH
    {0xD7F4, 0x915A}, //12461 #CJK UNIFIED IDEOGRAPH
    {0xD7F5, 0x9153}, //12462 #CJK UNIFIED IDEOGRAPH
    {0xD7F6, 0x9155}, //12463 #CJK UNIFIED IDEOGRAPH
    {0xD7F7, 0x91EC}, //12464 #CJK UNIFIED IDEOGRAPH
    {0xD7F8, 0x91F4}, //12465 #CJK UNIFIED IDEOGRAPH
    {0xD7F9, 0x91F1}, //12466 #CJK UNIFIED IDEOGRAPH
    {0xD7FA, 0x91F3}, //12467 #CJK UNIFIED IDEOGRAPH
    {0xD7FB, 0x91F8}, //12468 #CJK UNIFIED IDEOGRAPH
    {0xD7FC, 0x91E4}, //12469 #CJK UNIFIED IDEOGRAPH
    {0xD7FD, 0x91F9}, //12470 #CJK UNIFIED IDEOGRAPH
    {0xD7FE, 0x91EA}, //12471 #CJK UNIFIED IDEOGRAPH
    {0xD840, 0x91EB}, //12472 #CJK UNIFIED IDEOGRAPH
    {0xD841, 0x91F7}, //12473 #CJK UNIFIED IDEOGRAPH
    {0xD842, 0x91E8}, //12474 #CJK UNIFIED IDEOGRAPH
    {0xD843, 0x91EE}, //12475 #CJK UNIFIED IDEOGRAPH
    {0xD844, 0x957A}, //12476 #CJK UNIFIED IDEOGRAPH
    {0xD845, 0x9586}, //12477 #CJK UNIFIED IDEOGRAPH
    {0xD846, 0x9588}, //12478 #CJK UNIFIED IDEOGRAPH
    {0xD847, 0x967C}, //12479 #CJK UNIFIED IDEOGRAPH
    {0xD848, 0x966D}, //12480 #CJK UNIFIED IDEOGRAPH
    {0xD849, 0x966B}, //12481 #CJK UNIFIED IDEOGRAPH
    {0xD84A, 0x9671}, //12482 #CJK UNIFIED IDEOGRAPH
    {0xD84B, 0x966F}, //12483 #CJK UNIFIED IDEOGRAPH
    {0xD84C, 0x96BF}, //12484 #CJK UNIFIED IDEOGRAPH
    {0xD84D, 0x976A}, //12485 #CJK UNIFIED IDEOGRAPH
    {0xD84E, 0x9804}, //12486 #CJK UNIFIED IDEOGRAPH
    {0xD84F, 0x98E5}, //12487 #CJK UNIFIED IDEOGRAPH
    {0xD850, 0x9997}, //12488 #CJK UNIFIED IDEOGRAPH
    {0xD851, 0x509B}, //12489 #CJK UNIFIED IDEOGRAPH
    {0xD852, 0x5095}, //12490 #CJK UNIFIED IDEOGRAPH
    {0xD853, 0x5094}, //12491 #CJK UNIFIED IDEOGRAPH
    {0xD854, 0x509E}, //12492 #CJK UNIFIED IDEOGRAPH
    {0xD855, 0x508B}, //12493 #CJK UNIFIED IDEOGRAPH
    {0xD856, 0x50A3}, //12494 #CJK UNIFIED IDEOGRAPH
    {0xD857, 0x5083}, //12495 #CJK UNIFIED IDEOGRAPH
    {0xD858, 0x508C}, //12496 #CJK UNIFIED IDEOGRAPH
    {0xD859, 0x508E}, //12497 #CJK UNIFIED IDEOGRAPH
    {0xD85A, 0x509D}, //12498 #CJK UNIFIED IDEOGRAPH
    {0xD85B, 0x5068}, //12499 #CJK UNIFIED IDEOGRAPH
    {0xD85C, 0x509C}, //12500 #CJK UNIFIED IDEOGRAPH
    {0xD85D, 0x5092}, //12501 #CJK UNIFIED IDEOGRAPH
    {0xD85E, 0x5082}, //12502 #CJK UNIFIED IDEOGRAPH
    {0xD85F, 0x5087}, //12503 #CJK UNIFIED IDEOGRAPH
    {0xD860, 0x515F}, //12504 #CJK UNIFIED IDEOGRAPH
    {0xD861, 0x51D4}, //12505 #CJK UNIFIED IDEOGRAPH
    {0xD862, 0x5312}, //12506 #CJK UNIFIED IDEOGRAPH
    {0xD863, 0x5311}, //12507 #CJK UNIFIED IDEOGRAPH
    {0xD864, 0x53A4}, //12508 #CJK UNIFIED IDEOGRAPH
    {0xD865, 0x53A7}, //12509 #CJK UNIFIED IDEOGRAPH
    {0xD866, 0x5591}, //12510 #CJK UNIFIED IDEOGRAPH
    {0xD867, 0x55A8}, //12511 #CJK UNIFIED IDEOGRAPH
    {0xD868, 0x55A5}, //12512 #CJK UNIFIED IDEOGRAPH
    {0xD869, 0x55AD}, //12513 #CJK UNIFIED IDEOGRAPH
    {0xD86A, 0x5577}, //12514 #CJK UNIFIED IDEOGRAPH
    {0xD86B, 0x5645}, //12515 #CJK UNIFIED IDEOGRAPH
    {0xD86C, 0x55A2}, //12516 #CJK UNIFIED IDEOGRAPH
    {0xD86D, 0x5593}, //12517 #CJK UNIFIED IDEOGRAPH
    {0xD86E, 0x5588}, //12518 #CJK UNIFIED IDEOGRAPH
    {0xD86F, 0x558F}, //12519 #CJK UNIFIED IDEOGRAPH
    {0xD870, 0x55B5}, //12520 #CJK UNIFIED IDEOGRAPH
    {0xD871, 0x5581}, //12521 #CJK UNIFIED IDEOGRAPH
    {0xD872, 0x55A3}, //12522 #CJK UNIFIED IDEOGRAPH
    {0xD873, 0x5592}, //12523 #CJK UNIFIED IDEOGRAPH
    {0xD874, 0x55A4}, //12524 #CJK UNIFIED IDEOGRAPH
    {0xD875, 0x557D}, //12525 #CJK UNIFIED IDEOGRAPH
    {0xD876, 0x558C}, //12526 #CJK UNIFIED IDEOGRAPH
    {0xD877, 0x55A6}, //12527 #CJK UNIFIED IDEOGRAPH
    {0xD878, 0x557F}, //12528 #CJK UNIFIED IDEOGRAPH
    {0xD879, 0x5595}, //12529 #CJK UNIFIED IDEOGRAPH
    {0xD87A, 0x55A1}, //12530 #CJK UNIFIED IDEOGRAPH
    {0xD87B, 0x558E}, //12531 #CJK UNIFIED IDEOGRAPH
    {0xD87C, 0x570C}, //12532 #CJK UNIFIED IDEOGRAPH
    {0xD87D, 0x5829}, //12533 #CJK UNIFIED IDEOGRAPH
    {0xD87E, 0x5837}, //12534 #CJK UNIFIED IDEOGRAPH
    {0xD8A1, 0x5819}, //12535 #CJK UNIFIED IDEOGRAPH
    {0xD8A2, 0x581E}, //12536 #CJK UNIFIED IDEOGRAPH
    {0xD8A3, 0x5827}, //12537 #CJK UNIFIED IDEOGRAPH
    {0xD8A4, 0x5823}, //12538 #CJK UNIFIED IDEOGRAPH
    {0xD8A5, 0x5828}, //12539 #CJK UNIFIED IDEOGRAPH
    {0xD8A6, 0x57F5}, //12540 #CJK UNIFIED IDEOGRAPH
    {0xD8A7, 0x5848}, //12541 #CJK UNIFIED IDEOGRAPH
    {0xD8A8, 0x5825}, //12542 #CJK UNIFIED IDEOGRAPH
    {0xD8A9, 0x581C}, //12543 #CJK UNIFIED IDEOGRAPH
    {0xD8AA, 0x581B}, //12544 #CJK UNIFIED IDEOGRAPH
    {0xD8AB, 0x5833}, //12545 #CJK UNIFIED IDEOGRAPH
    {0xD8AC, 0x583F}, //12546 #CJK UNIFIED IDEOGRAPH
    {0xD8AD, 0x5836}, //12547 #CJK UNIFIED IDEOGRAPH
    {0xD8AE, 0x582E}, //12548 #CJK UNIFIED IDEOGRAPH
    {0xD8AF, 0x5839}, //12549 #CJK UNIFIED IDEOGRAPH
    {0xD8B0, 0x5838}, //12550 #CJK UNIFIED IDEOGRAPH
    {0xD8B1, 0x582D}, //12551 #CJK UNIFIED IDEOGRAPH
    {0xD8B2, 0x582C}, //12552 #CJK UNIFIED IDEOGRAPH
    {0xD8B3, 0x583B}, //12553 #CJK UNIFIED IDEOGRAPH
    {0xD8B4, 0x5961}, //12554 #CJK UNIFIED IDEOGRAPH
    {0xD8B5, 0x5AAF}, //12555 #CJK UNIFIED IDEOGRAPH
    {0xD8B6, 0x5A94}, //12556 #CJK UNIFIED IDEOGRAPH
    {0xD8B7, 0x5A9F}, //12557 #CJK UNIFIED IDEOGRAPH
    {0xD8B8, 0x5A7A}, //12558 #CJK UNIFIED IDEOGRAPH
    {0xD8B9, 0x5AA2}, //12559 #CJK UNIFIED IDEOGRAPH
    {0xD8BA, 0x5A9E}, //12560 #CJK UNIFIED IDEOGRAPH
    {0xD8BB, 0x5A78}, //12561 #CJK UNIFIED IDEOGRAPH
    {0xD8BC, 0x5AA6}, //12562 #CJK UNIFIED IDEOGRAPH
    {0xD8BD, 0x5A7C}, //12563 #CJK UNIFIED IDEOGRAPH
    {0xD8BE, 0x5AA5}, //12564 #CJK UNIFIED IDEOGRAPH
    {0xD8BF, 0x5AAC}, //12565 #CJK UNIFIED IDEOGRAPH
    {0xD8C0, 0x5A95}, //12566 #CJK UNIFIED IDEOGRAPH
    {0xD8C1, 0x5AAE}, //12567 #CJK UNIFIED IDEOGRAPH
    {0xD8C2, 0x5A37}, //12568 #CJK UNIFIED IDEOGRAPH
    {0xD8C3, 0x5A84}, //12569 #CJK UNIFIED IDEOGRAPH
    {0xD8C4, 0x5A8A}, //12570 #CJK UNIFIED IDEOGRAPH
    {0xD8C5, 0x5A97}, //12571 #CJK UNIFIED IDEOGRAPH
    {0xD8C6, 0x5A83}, //12572 #CJK UNIFIED IDEOGRAPH
    {0xD8C7, 0x5A8B}, //12573 #CJK UNIFIED IDEOGRAPH
    {0xD8C8, 0x5AA9}, //12574 #CJK UNIFIED IDEOGRAPH
    {0xD8C9, 0x5A7B}, //12575 #CJK UNIFIED IDEOGRAPH
    {0xD8CA, 0x5A7D}, //12576 #CJK UNIFIED IDEOGRAPH
    {0xD8CB, 0x5A8C}, //12577 #CJK UNIFIED IDEOGRAPH
    {0xD8CC, 0x5A9C}, //12578 #CJK UNIFIED IDEOGRAPH
    {0xD8CD, 0x5A8F}, //12579 #CJK UNIFIED IDEOGRAPH
    {0xD8CE, 0x5A93}, //12580 #CJK UNIFIED IDEOGRAPH
    {0xD8CF, 0x5A9D}, //12581 #CJK UNIFIED IDEOGRAPH
    {0xD8D0, 0x5BEA}, //12582 #CJK UNIFIED IDEOGRAPH
    {0xD8D1, 0x5BCD}, //12583 #CJK UNIFIED IDEOGRAPH
    {0xD8D2, 0x5BCB}, //12584 #CJK UNIFIED IDEOGRAPH
    {0xD8D3, 0x5BD4}, //12585 #CJK UNIFIED IDEOGRAPH
    {0xD8D4, 0x5BD1}, //12586 #CJK UNIFIED IDEOGRAPH
    {0xD8D5, 0x5BCA}, //12587 #CJK UNIFIED IDEOGRAPH
    {0xD8D6, 0x5BCE}, //12588 #CJK UNIFIED IDEOGRAPH
    {0xD8D7, 0x5C0C}, //12589 #CJK UNIFIED IDEOGRAPH
    {0xD8D8, 0x5C30}, //12590 #CJK UNIFIED IDEOGRAPH
    {0xD8D9, 0x5D37}, //12591 #CJK UNIFIED IDEOGRAPH
    {0xD8DA, 0x5D43}, //12592 #CJK UNIFIED IDEOGRAPH
    {0xD8DB, 0x5D6B}, //12593 #CJK UNIFIED IDEOGRAPH
    {0xD8DC, 0x5D41}, //12594 #CJK UNIFIED IDEOGRAPH
    {0xD8DD, 0x5D4B}, //12595 #CJK UNIFIED IDEOGRAPH
    {0xD8DE, 0x5D3F}, //12596 #CJK UNIFIED IDEOGRAPH
    {0xD8DF, 0x5D35}, //12597 #CJK UNIFIED IDEOGRAPH
    {0xD8E0, 0x5D51}, //12598 #CJK UNIFIED IDEOGRAPH
    {0xD8E1, 0x5D4E}, //12599 #CJK UNIFIED IDEOGRAPH
    {0xD8E2, 0x5D55}, //12600 #CJK UNIFIED IDEOGRAPH
    {0xD8E3, 0x5D33}, //12601 #CJK UNIFIED IDEOGRAPH
    {0xD8E4, 0x5D3A}, //12602 #CJK UNIFIED IDEOGRAPH
    {0xD8E5, 0x5D52}, //12603 #CJK UNIFIED IDEOGRAPH
    {0xD8E6, 0x5D3D}, //12604 #CJK UNIFIED IDEOGRAPH
    {0xD8E7, 0x5D31}, //12605 #CJK UNIFIED IDEOGRAPH
    {0xD8E8, 0x5D59}, //12606 #CJK UNIFIED IDEOGRAPH
    {0xD8E9, 0x5D42}, //12607 #CJK UNIFIED IDEOGRAPH
    {0xD8EA, 0x5D39}, //12608 #CJK UNIFIED IDEOGRAPH
    {0xD8EB, 0x5D49}, //12609 #CJK UNIFIED IDEOGRAPH
    {0xD8EC, 0x5D38}, //12610 #CJK UNIFIED IDEOGRAPH
    {0xD8ED, 0x5D3C}, //12611 #CJK UNIFIED IDEOGRAPH
    {0xD8EE, 0x5D32}, //12612 #CJK UNIFIED IDEOGRAPH
    {0xD8EF, 0x5D36}, //12613 #CJK UNIFIED IDEOGRAPH
    {0xD8F0, 0x5D40}, //12614 #CJK UNIFIED IDEOGRAPH
    {0xD8F1, 0x5D45}, //12615 #CJK UNIFIED IDEOGRAPH
    {0xD8F2, 0x5E44}, //12616 #CJK UNIFIED IDEOGRAPH
    {0xD8F3, 0x5E41}, //12617 #CJK UNIFIED IDEOGRAPH
    {0xD8F4, 0x5F58}, //12618 #CJK UNIFIED IDEOGRAPH
    {0xD8F5, 0x5FA6}, //12619 #CJK UNIFIED IDEOGRAPH
    {0xD8F6, 0x5FA5}, //12620 #CJK UNIFIED IDEOGRAPH
    {0xD8F7, 0x5FAB}, //12621 #CJK UNIFIED IDEOGRAPH
    {0xD8F8, 0x60C9}, //12622 #CJK UNIFIED IDEOGRAPH
    {0xD8F9, 0x60B9}, //12623 #CJK UNIFIED IDEOGRAPH
    {0xD8FA, 0x60CC}, //12624 #CJK UNIFIED IDEOGRAPH
    {0xD8FB, 0x60E2}, //12625 #CJK UNIFIED IDEOGRAPH
    {0xD8FC, 0x60CE}, //12626 #CJK UNIFIED IDEOGRAPH
    {0xD8FD, 0x60C4}, //12627 #CJK UNIFIED IDEOGRAPH
    {0xD8FE, 0x6114}, //12628 #CJK UNIFIED IDEOGRAPH
    {0xD940, 0x60F2}, //12629 #CJK UNIFIED IDEOGRAPH
    {0xD941, 0x610A}, //12630 #CJK UNIFIED IDEOGRAPH
    {0xD942, 0x6116}, //12631 #CJK UNIFIED IDEOGRAPH
    {0xD943, 0x6105}, //12632 #CJK UNIFIED IDEOGRAPH
    {0xD944, 0x60F5}, //12633 #CJK UNIFIED IDEOGRAPH
    {0xD945, 0x6113}, //12634 #CJK UNIFIED IDEOGRAPH
    {0xD946, 0x60F8}, //12635 #CJK UNIFIED IDEOGRAPH
    {0xD947, 0x60FC}, //12636 #CJK UNIFIED IDEOGRAPH
    {0xD948, 0x60FE}, //12637 #CJK UNIFIED IDEOGRAPH
    {0xD949, 0x60C1}, //12638 #CJK UNIFIED IDEOGRAPH
    {0xD94A, 0x6103}, //12639 #CJK UNIFIED IDEOGRAPH
    {0xD94B, 0x6118}, //12640 #CJK UNIFIED IDEOGRAPH
    {0xD94C, 0x611D}, //12641 #CJK UNIFIED IDEOGRAPH
    {0xD94D, 0x6110}, //12642 #CJK UNIFIED IDEOGRAPH
    {0xD94E, 0x60FF}, //12643 #CJK UNIFIED IDEOGRAPH
    {0xD94F, 0x6104}, //12644 #CJK UNIFIED IDEOGRAPH
    {0xD950, 0x610B}, //12645 #CJK UNIFIED IDEOGRAPH
    {0xD951, 0x624A}, //12646 #CJK UNIFIED IDEOGRAPH
    {0xD952, 0x6394}, //12647 #CJK UNIFIED IDEOGRAPH
    {0xD953, 0x63B1}, //12648 #CJK UNIFIED IDEOGRAPH
    {0xD954, 0x63B0}, //12649 #CJK UNIFIED IDEOGRAPH
    {0xD955, 0x63CE}, //12650 #CJK UNIFIED IDEOGRAPH
    {0xD956, 0x63E5}, //12651 #CJK UNIFIED IDEOGRAPH
    {0xD957, 0x63E8}, //12652 #CJK UNIFIED IDEOGRAPH
    {0xD958, 0x63EF}, //12653 #CJK UNIFIED IDEOGRAPH
    {0xD959, 0x63C3}, //12654 #CJK UNIFIED IDEOGRAPH
    {0xD95A, 0x649D}, //12655 #CJK UNIFIED IDEOGRAPH
    {0xD95B, 0x63F3}, //12656 #CJK UNIFIED IDEOGRAPH
    {0xD95C, 0x63CA}, //12657 #CJK UNIFIED IDEOGRAPH
    {0xD95D, 0x63E0}, //12658 #CJK UNIFIED IDEOGRAPH
    {0xD95E, 0x63F6}, //12659 #CJK UNIFIED IDEOGRAPH
    {0xD95F, 0x63D5}, //12660 #CJK UNIFIED IDEOGRAPH
    {0xD960, 0x63F2}, //12661 #CJK UNIFIED IDEOGRAPH
    {0xD961, 0x63F5}, //12662 #CJK UNIFIED IDEOGRAPH
    {0xD962, 0x6461}, //12663 #CJK UNIFIED IDEOGRAPH
    {0xD963, 0x63DF}, //12664 #CJK UNIFIED IDEOGRAPH
    {0xD964, 0x63BE}, //12665 #CJK UNIFIED IDEOGRAPH
    {0xD965, 0x63DD}, //12666 #CJK UNIFIED IDEOGRAPH
    {0xD966, 0x63DC}, //12667 #CJK UNIFIED IDEOGRAPH
    {0xD967, 0x63C4}, //12668 #CJK UNIFIED IDEOGRAPH
    {0xD968, 0x63D8}, //12669 #CJK UNIFIED IDEOGRAPH
    {0xD969, 0x63D3}, //12670 #CJK UNIFIED IDEOGRAPH
    {0xD96A, 0x63C2}, //12671 #CJK UNIFIED IDEOGRAPH
    {0xD96B, 0x63C7}, //12672 #CJK UNIFIED IDEOGRAPH
    {0xD96C, 0x63CC}, //12673 #CJK UNIFIED IDEOGRAPH
    {0xD96D, 0x63CB}, //12674 #CJK UNIFIED IDEOGRAPH
    {0xD96E, 0x63C8}, //12675 #CJK UNIFIED IDEOGRAPH
    {0xD96F, 0x63F0}, //12676 #CJK UNIFIED IDEOGRAPH
    {0xD970, 0x63D7}, //12677 #CJK UNIFIED IDEOGRAPH
    {0xD971, 0x63D9}, //12678 #CJK UNIFIED IDEOGRAPH
    {0xD972, 0x6532}, //12679 #CJK UNIFIED IDEOGRAPH
    {0xD973, 0x6567}, //12680 #CJK UNIFIED IDEOGRAPH
    {0xD974, 0x656A}, //12681 #CJK UNIFIED IDEOGRAPH
    {0xD975, 0x6564}, //12682 #CJK UNIFIED IDEOGRAPH
    {0xD976, 0x655C}, //12683 #CJK UNIFIED IDEOGRAPH
    {0xD977, 0x6568}, //12684 #CJK UNIFIED IDEOGRAPH
    {0xD978, 0x6565}, //12685 #CJK UNIFIED IDEOGRAPH
    {0xD979, 0x658C}, //12686 #CJK UNIFIED IDEOGRAPH
    {0xD97A, 0x659D}, //12687 #CJK UNIFIED IDEOGRAPH
    {0xD97B, 0x659E}, //12688 #CJK UNIFIED IDEOGRAPH
    {0xD97C, 0x65AE}, //12689 #CJK UNIFIED IDEOGRAPH
    {0xD97D, 0x65D0}, //12690 #CJK UNIFIED IDEOGRAPH
    {0xD97E, 0x65D2}, //12691 #CJK UNIFIED IDEOGRAPH
    {0xD9A1, 0x667C}, //12692 #CJK UNIFIED IDEOGRAPH
    {0xD9A2, 0x666C}, //12693 #CJK UNIFIED IDEOGRAPH
    {0xD9A3, 0x667B}, //12694 #CJK UNIFIED IDEOGRAPH
    {0xD9A4, 0x6680}, //12695 #CJK UNIFIED IDEOGRAPH
    {0xD9A5, 0x6671}, //12696 #CJK UNIFIED IDEOGRAPH
    {0xD9A6, 0x6679}, //12697 #CJK UNIFIED IDEOGRAPH
    {0xD9A7, 0x666A}, //12698 #CJK UNIFIED IDEOGRAPH
    {0xD9A8, 0x6672}, //12699 #CJK UNIFIED IDEOGRAPH
    {0xD9A9, 0x6701}, //12700 #CJK UNIFIED IDEOGRAPH
    {0xD9AA, 0x690C}, //12701 #CJK UNIFIED IDEOGRAPH
    {0xD9AB, 0x68D3}, //12702 #CJK UNIFIED IDEOGRAPH
    {0xD9AC, 0x6904}, //12703 #CJK UNIFIED IDEOGRAPH
    {0xD9AD, 0x68DC}, //12704 #CJK UNIFIED IDEOGRAPH
    {0xD9AE, 0x692A}, //12705 #CJK UNIFIED IDEOGRAPH
    {0xD9AF, 0x68EC}, //12706 #CJK UNIFIED IDEOGRAPH
    {0xD9B0, 0x68EA}, //12707 #CJK UNIFIED IDEOGRAPH
    {0xD9B1, 0x68F1}, //12708 #CJK UNIFIED IDEOGRAPH
    {0xD9B2, 0x690F}, //12709 #CJK UNIFIED IDEOGRAPH
    {0xD9B3, 0x68D6}, //12710 #CJK UNIFIED IDEOGRAPH
    {0xD9B4, 0x68F7}, //12711 #CJK UNIFIED IDEOGRAPH
    {0xD9B5, 0x68EB}, //12712 #CJK UNIFIED IDEOGRAPH
    {0xD9B6, 0x68E4}, //12713 #CJK UNIFIED IDEOGRAPH
    {0xD9B7, 0x68F6}, //12714 #CJK UNIFIED IDEOGRAPH
    {0xD9B8, 0x6913}, //12715 #CJK UNIFIED IDEOGRAPH
    {0xD9B9, 0x6910}, //12716 #CJK UNIFIED IDEOGRAPH
    {0xD9BA, 0x68F3}, //12717 #CJK UNIFIED IDEOGRAPH
    {0xD9BB, 0x68E1}, //12718 #CJK UNIFIED IDEOGRAPH
    {0xD9BC, 0x6907}, //12719 #CJK UNIFIED IDEOGRAPH
    {0xD9BD, 0x68CC}, //12720 #CJK UNIFIED IDEOGRAPH
    {0xD9BE, 0x6908}, //12721 #CJK UNIFIED IDEOGRAPH
    {0xD9BF, 0x6970}, //12722 #CJK UNIFIED IDEOGRAPH
    {0xD9C0, 0x68B4}, //12723 #CJK UNIFIED IDEOGRAPH
    {0xD9C1, 0x6911}, //12724 #CJK UNIFIED IDEOGRAPH
    {0xD9C2, 0x68EF}, //12725 #CJK UNIFIED IDEOGRAPH
    {0xD9C3, 0x68C6}, //12726 #CJK UNIFIED IDEOGRAPH
    {0xD9C4, 0x6914}, //12727 #CJK UNIFIED IDEOGRAPH
    {0xD9C5, 0x68F8}, //12728 #CJK UNIFIED IDEOGRAPH
    {0xD9C6, 0x68D0}, //12729 #CJK UNIFIED IDEOGRAPH
    {0xD9C7, 0x68FD}, //12730 #CJK UNIFIED IDEOGRAPH
    {0xD9C8, 0x68FC}, //12731 #CJK UNIFIED IDEOGRAPH
    {0xD9C9, 0x68E8}, //12732 #CJK UNIFIED IDEOGRAPH
    {0xD9CA, 0x690B}, //12733 #CJK UNIFIED IDEOGRAPH
    {0xD9CB, 0x690A}, //12734 #CJK UNIFIED IDEOGRAPH
    {0xD9CC, 0x6917}, //12735 #CJK UNIFIED IDEOGRAPH
    {0xD9CD, 0x68CE}, //12736 #CJK UNIFIED IDEOGRAPH
    {0xD9CE, 0x68C8}, //12737 #CJK UNIFIED IDEOGRAPH
    {0xD9CF, 0x68DD}, //12738 #CJK UNIFIED IDEOGRAPH
    {0xD9D0, 0x68DE}, //12739 #CJK UNIFIED IDEOGRAPH
    {0xD9D1, 0x68E6}, //12740 #CJK UNIFIED IDEOGRAPH
    {0xD9D2, 0x68F4}, //12741 #CJK UNIFIED IDEOGRAPH
    {0xD9D3, 0x68D1}, //12742 #CJK UNIFIED IDEOGRAPH
    {0xD9D4, 0x6906}, //12743 #CJK UNIFIED IDEOGRAPH
    {0xD9D5, 0x68D4}, //12744 #CJK UNIFIED IDEOGRAPH
    {0xD9D6, 0x68E9}, //12745 #CJK UNIFIED IDEOGRAPH
    {0xD9D7, 0x6915}, //12746 #CJK UNIFIED IDEOGRAPH
    {0xD9D8, 0x6925}, //12747 #CJK UNIFIED IDEOGRAPH
    {0xD9D9, 0x68C7}, //12748 #CJK UNIFIED IDEOGRAPH
    {0xD9DA, 0x6B39}, //12749 #CJK UNIFIED IDEOGRAPH
    {0xD9DB, 0x6B3B}, //12750 #CJK UNIFIED IDEOGRAPH
    {0xD9DC, 0x6B3F}, //12751 #CJK UNIFIED IDEOGRAPH
    {0xD9DD, 0x6B3C}, //12752 #CJK UNIFIED IDEOGRAPH
    {0xD9DE, 0x6B94}, //12753 #CJK UNIFIED IDEOGRAPH
    {0xD9DF, 0x6B97}, //12754 #CJK UNIFIED IDEOGRAPH
    {0xD9E0, 0x6B99}, //12755 #CJK UNIFIED IDEOGRAPH
    {0xD9E1, 0x6B95}, //12756 #CJK UNIFIED IDEOGRAPH
    {0xD9E2, 0x6BBD}, //12757 #CJK UNIFIED IDEOGRAPH
    {0xD9E3, 0x6BF0}, //12758 #CJK UNIFIED IDEOGRAPH
    {0xD9E4, 0x6BF2}, //12759 #CJK UNIFIED IDEOGRAPH
    {0xD9E5, 0x6BF3}, //12760 #CJK UNIFIED IDEOGRAPH
    {0xD9E6, 0x6C30}, //12761 #CJK UNIFIED IDEOGRAPH
    {0xD9E7, 0x6DFC}, //12762 #CJK UNIFIED IDEOGRAPH
    {0xD9E8, 0x6E46}, //12763 #CJK UNIFIED IDEOGRAPH
    {0xD9E9, 0x6E47}, //12764 #CJK UNIFIED IDEOGRAPH
    {0xD9EA, 0x6E1F}, //12765 #CJK UNIFIED IDEOGRAPH
    {0xD9EB, 0x6E49}, //12766 #CJK UNIFIED IDEOGRAPH
    {0xD9EC, 0x6E88}, //12767 #CJK UNIFIED IDEOGRAPH
    {0xD9ED, 0x6E3C}, //12768 #CJK UNIFIED IDEOGRAPH
    {0xD9EE, 0x6E3D}, //12769 #CJK UNIFIED IDEOGRAPH
    {0xD9EF, 0x6E45}, //12770 #CJK UNIFIED IDEOGRAPH
    {0xD9F0, 0x6E62}, //12771 #CJK UNIFIED IDEOGRAPH
    {0xD9F1, 0x6E2B}, //12772 #CJK UNIFIED IDEOGRAPH
    {0xD9F2, 0x6E3F}, //12773 #CJK UNIFIED IDEOGRAPH
    {0xD9F3, 0x6E41}, //12774 #CJK UNIFIED IDEOGRAPH
    {0xD9F4, 0x6E5D}, //12775 #CJK UNIFIED IDEOGRAPH
    {0xD9F5, 0x6E73}, //12776 #CJK UNIFIED IDEOGRAPH
    {0xD9F6, 0x6E1C}, //12777 #CJK UNIFIED IDEOGRAPH
    {0xD9F7, 0x6E33}, //12778 #CJK UNIFIED IDEOGRAPH
    {0xD9F8, 0x6E4B}, //12779 #CJK UNIFIED IDEOGRAPH
    {0xD9F9, 0x6E40}, //12780 #CJK UNIFIED IDEOGRAPH
    {0xD9FA, 0x6E51}, //12781 #CJK UNIFIED IDEOGRAPH
    {0xD9FB, 0x6E3B}, //12782 #CJK UNIFIED IDEOGRAPH
    {0xD9FC, 0x6E03}, //12783 #CJK UNIFIED IDEOGRAPH
    {0xD9FD, 0x6E2E}, //12784 #CJK UNIFIED IDEOGRAPH
    {0xD9FE, 0x6E5E}, //12785 #CJK UNIFIED IDEOGRAPH
    {0xDA40, 0x6E68}, //12786 #CJK UNIFIED IDEOGRAPH
    {0xDA41, 0x6E5C}, //12787 #CJK UNIFIED IDEOGRAPH
    {0xDA42, 0x6E61}, //12788 #CJK UNIFIED IDEOGRAPH
    {0xDA43, 0x6E31}, //12789 #CJK UNIFIED IDEOGRAPH
    {0xDA44, 0x6E28}, //12790 #CJK UNIFIED IDEOGRAPH
    {0xDA45, 0x6E60}, //12791 #CJK UNIFIED IDEOGRAPH
    {0xDA46, 0x6E71}, //12792 #CJK UNIFIED IDEOGRAPH
    {0xDA47, 0x6E6B}, //12793 #CJK UNIFIED IDEOGRAPH
    {0xDA48, 0x6E39}, //12794 #CJK UNIFIED IDEOGRAPH
    {0xDA49, 0x6E22}, //12795 #CJK UNIFIED IDEOGRAPH
    {0xDA4A, 0x6E30}, //12796 #CJK UNIFIED IDEOGRAPH
    {0xDA4B, 0x6E53}, //12797 #CJK UNIFIED IDEOGRAPH
    {0xDA4C, 0x6E65}, //12798 #CJK UNIFIED IDEOGRAPH
    {0xDA4D, 0x6E27}, //12799 #CJK UNIFIED IDEOGRAPH
    {0xDA4E, 0x6E78}, //12800 #CJK UNIFIED IDEOGRAPH
    {0xDA4F, 0x6E64}, //12801 #CJK UNIFIED IDEOGRAPH
    {0xDA50, 0x6E77}, //12802 #CJK UNIFIED IDEOGRAPH
    {0xDA51, 0x6E55}, //12803 #CJK UNIFIED IDEOGRAPH
    {0xDA52, 0x6E79}, //12804 #CJK UNIFIED IDEOGRAPH
    {0xDA53, 0x6E52}, //12805 #CJK UNIFIED IDEOGRAPH
    {0xDA54, 0x6E66}, //12806 #CJK UNIFIED IDEOGRAPH
    {0xDA55, 0x6E35}, //12807 #CJK UNIFIED IDEOGRAPH
    {0xDA56, 0x6E36}, //12808 #CJK UNIFIED IDEOGRAPH
    {0xDA57, 0x6E5A}, //12809 #CJK UNIFIED IDEOGRAPH
    {0xDA58, 0x7120}, //12810 #CJK UNIFIED IDEOGRAPH
    {0xDA59, 0x711E}, //12811 #CJK UNIFIED IDEOGRAPH
    {0xDA5A, 0x712F}, //12812 #CJK UNIFIED IDEOGRAPH
    {0xDA5B, 0x70FB}, //12813 #CJK UNIFIED IDEOGRAPH
    {0xDA5C, 0x712E}, //12814 #CJK UNIFIED IDEOGRAPH
    {0xDA5D, 0x7131}, //12815 #CJK UNIFIED IDEOGRAPH
    {0xDA5E, 0x7123}, //12816 #CJK UNIFIED IDEOGRAPH
    {0xDA5F, 0x7125}, //12817 #CJK UNIFIED IDEOGRAPH
    {0xDA60, 0x7122}, //12818 #CJK UNIFIED IDEOGRAPH
    {0xDA61, 0x7132}, //12819 #CJK UNIFIED IDEOGRAPH
    {0xDA62, 0x711F}, //12820 #CJK UNIFIED IDEOGRAPH
    {0xDA63, 0x7128}, //12821 #CJK UNIFIED IDEOGRAPH
    {0xDA64, 0x713A}, //12822 #CJK UNIFIED IDEOGRAPH
    {0xDA65, 0x711B}, //12823 #CJK UNIFIED IDEOGRAPH
    {0xDA66, 0x724B}, //12824 #CJK UNIFIED IDEOGRAPH
    {0xDA67, 0x725A}, //12825 #CJK UNIFIED IDEOGRAPH
    {0xDA68, 0x7288}, //12826 #CJK UNIFIED IDEOGRAPH
    {0xDA69, 0x7289}, //12827 #CJK UNIFIED IDEOGRAPH
    {0xDA6A, 0x7286}, //12828 #CJK UNIFIED IDEOGRAPH
    {0xDA6B, 0x7285}, //12829 #CJK UNIFIED IDEOGRAPH
    {0xDA6C, 0x728B}, //12830 #CJK UNIFIED IDEOGRAPH
    {0xDA6D, 0x7312}, //12831 #CJK UNIFIED IDEOGRAPH
    {0xDA6E, 0x730B}, //12832 #CJK UNIFIED IDEOGRAPH
    {0xDA6F, 0x7330}, //12833 #CJK UNIFIED IDEOGRAPH
    {0xDA70, 0x7322}, //12834 #CJK UNIFIED IDEOGRAPH
    {0xDA71, 0x7331}, //12835 #CJK UNIFIED IDEOGRAPH
    {0xDA72, 0x7333}, //12836 #CJK UNIFIED IDEOGRAPH
    {0xDA73, 0x7327}, //12837 #CJK UNIFIED IDEOGRAPH
    {0xDA74, 0x7332}, //12838 #CJK UNIFIED IDEOGRAPH
    {0xDA75, 0x732D}, //12839 #CJK UNIFIED IDEOGRAPH
    {0xDA76, 0x7326}, //12840 #CJK UNIFIED IDEOGRAPH
    {0xDA77, 0x7323}, //12841 #CJK UNIFIED IDEOGRAPH
    {0xDA78, 0x7335}, //12842 #CJK UNIFIED IDEOGRAPH
    {0xDA79, 0x730C}, //12843 #CJK UNIFIED IDEOGRAPH
    {0xDA7A, 0x742E}, //12844 #CJK UNIFIED IDEOGRAPH
    {0xDA7B, 0x742C}, //12845 #CJK UNIFIED IDEOGRAPH
    {0xDA7C, 0x7430}, //12846 #CJK UNIFIED IDEOGRAPH
    {0xDA7D, 0x742B}, //12847 #CJK UNIFIED IDEOGRAPH
    {0xDA7E, 0x7416}, //12848 #CJK UNIFIED IDEOGRAPH
    {0xDAA1, 0x741A}, //12849 #CJK UNIFIED IDEOGRAPH
    {0xDAA2, 0x7421}, //12850 #CJK UNIFIED IDEOGRAPH
    {0xDAA3, 0x742D}, //12851 #CJK UNIFIED IDEOGRAPH
    {0xDAA4, 0x7431}, //12852 #CJK UNIFIED IDEOGRAPH
    {0xDAA5, 0x7424}, //12853 #CJK UNIFIED IDEOGRAPH
    {0xDAA6, 0x7423}, //12854 #CJK UNIFIED IDEOGRAPH
    {0xDAA7, 0x741D}, //12855 #CJK UNIFIED IDEOGRAPH
    {0xDAA8, 0x7429}, //12856 #CJK UNIFIED IDEOGRAPH
    {0xDAA9, 0x7420}, //12857 #CJK UNIFIED IDEOGRAPH
    {0xDAAA, 0x7432}, //12858 #CJK UNIFIED IDEOGRAPH
    {0xDAAB, 0x74FB}, //12859 #CJK UNIFIED IDEOGRAPH
    {0xDAAC, 0x752F}, //12860 #CJK UNIFIED IDEOGRAPH
    {0xDAAD, 0x756F}, //12861 #CJK UNIFIED IDEOGRAPH
    {0xDAAE, 0x756C}, //12862 #CJK UNIFIED IDEOGRAPH
    {0xDAAF, 0x75E7}, //12863 #CJK UNIFIED IDEOGRAPH
    {0xDAB0, 0x75DA}, //12864 #CJK UNIFIED IDEOGRAPH
    {0xDAB1, 0x75E1}, //12865 #CJK UNIFIED IDEOGRAPH
    {0xDAB2, 0x75E6}, //12866 #CJK UNIFIED IDEOGRAPH
    {0xDAB3, 0x75DD}, //12867 #CJK UNIFIED IDEOGRAPH
    {0xDAB4, 0x75DF}, //12868 #CJK UNIFIED IDEOGRAPH
    {0xDAB5, 0x75E4}, //12869 #CJK UNIFIED IDEOGRAPH
    {0xDAB6, 0x75D7}, //12870 #CJK UNIFIED IDEOGRAPH
    {0xDAB7, 0x7695}, //12871 #CJK UNIFIED IDEOGRAPH
    {0xDAB8, 0x7692}, //12872 #CJK UNIFIED IDEOGRAPH
    {0xDAB9, 0x76DA}, //12873 #CJK UNIFIED IDEOGRAPH
    {0xDABA, 0x7746}, //12874 #CJK UNIFIED IDEOGRAPH
    {0xDABB, 0x7747}, //12875 #CJK UNIFIED IDEOGRAPH
    {0xDABC, 0x7744}, //12876 #CJK UNIFIED IDEOGRAPH
    {0xDABD, 0x774D}, //12877 #CJK UNIFIED IDEOGRAPH
    {0xDABE, 0x7745}, //12878 #CJK UNIFIED IDEOGRAPH
    {0xDABF, 0x774A}, //12879 #CJK UNIFIED IDEOGRAPH
    {0xDAC0, 0x774E}, //12880 #CJK UNIFIED IDEOGRAPH
    {0xDAC1, 0x774B}, //12881 #CJK UNIFIED IDEOGRAPH
    {0xDAC2, 0x774C}, //12882 #CJK UNIFIED IDEOGRAPH
    {0xDAC3, 0x77DE}, //12883 #CJK UNIFIED IDEOGRAPH
    {0xDAC4, 0x77EC}, //12884 #CJK UNIFIED IDEOGRAPH
    {0xDAC5, 0x7860}, //12885 #CJK UNIFIED IDEOGRAPH
    {0xDAC6, 0x7864}, //12886 #CJK UNIFIED IDEOGRAPH
    {0xDAC7, 0x7865}, //12887 #CJK UNIFIED IDEOGRAPH
    {0xDAC8, 0x785C}, //12888 #CJK UNIFIED IDEOGRAPH
    {0xDAC9, 0x786D}, //12889 #CJK UNIFIED IDEOGRAPH
    {0xDACA, 0x7871}, //12890 #CJK UNIFIED IDEOGRAPH
    {0xDACB, 0x786A}, //12891 #CJK UNIFIED IDEOGRAPH
    {0xDACC, 0x786E}, //12892 #CJK UNIFIED IDEOGRAPH
    {0xDACD, 0x7870}, //12893 #CJK UNIFIED IDEOGRAPH
    {0xDACE, 0x7869}, //12894 #CJK UNIFIED IDEOGRAPH
    {0xDACF, 0x7868}, //12895 #CJK UNIFIED IDEOGRAPH
    {0xDAD0, 0x785E}, //12896 #CJK UNIFIED IDEOGRAPH
    {0xDAD1, 0x7862}, //12897 #CJK UNIFIED IDEOGRAPH
    {0xDAD2, 0x7974}, //12898 #CJK UNIFIED IDEOGRAPH
    {0xDAD3, 0x7973}, //12899 #CJK UNIFIED IDEOGRAPH
    {0xDAD4, 0x7972}, //12900 #CJK UNIFIED IDEOGRAPH
    {0xDAD5, 0x7970}, //12901 #CJK UNIFIED IDEOGRAPH
    {0xDAD6, 0x7A02}, //12902 #CJK UNIFIED IDEOGRAPH
    {0xDAD7, 0x7A0A}, //12903 #CJK UNIFIED IDEOGRAPH
    {0xDAD8, 0x7A03}, //12904 #CJK UNIFIED IDEOGRAPH
    {0xDAD9, 0x7A0C}, //12905 #CJK UNIFIED IDEOGRAPH
    {0xDADA, 0x7A04}, //12906 #CJK UNIFIED IDEOGRAPH
    {0xDADB, 0x7A99}, //12907 #CJK UNIFIED IDEOGRAPH
    {0xDADC, 0x7AE6}, //12908 #CJK UNIFIED IDEOGRAPH
    {0xDADD, 0x7AE4}, //12909 #CJK UNIFIED IDEOGRAPH
    {0xDADE, 0x7B4A}, //12910 #CJK UNIFIED IDEOGRAPH
    {0xDADF, 0x7B3B}, //12911 #CJK UNIFIED IDEOGRAPH
    {0xDAE0, 0x7B44}, //12912 #CJK UNIFIED IDEOGRAPH
    {0xDAE1, 0x7B48}, //12913 #CJK UNIFIED IDEOGRAPH
    {0xDAE2, 0x7B4C}, //12914 #CJK UNIFIED IDEOGRAPH
    {0xDAE3, 0x7B4E}, //12915 #CJK UNIFIED IDEOGRAPH
    {0xDAE4, 0x7B40}, //12916 #CJK UNIFIED IDEOGRAPH
    {0xDAE5, 0x7B58}, //12917 #CJK UNIFIED IDEOGRAPH
    {0xDAE6, 0x7B45}, //12918 #CJK UNIFIED IDEOGRAPH
    {0xDAE7, 0x7CA2}, //12919 #CJK UNIFIED IDEOGRAPH
    {0xDAE8, 0x7C9E}, //12920 #CJK UNIFIED IDEOGRAPH
    {0xDAE9, 0x7CA8}, //12921 #CJK UNIFIED IDEOGRAPH
    {0xDAEA, 0x7CA1}, //12922 #CJK UNIFIED IDEOGRAPH
    {0xDAEB, 0x7D58}, //12923 #CJK UNIFIED IDEOGRAPH
    {0xDAEC, 0x7D6F}, //12924 #CJK UNIFIED IDEOGRAPH
    {0xDAED, 0x7D63}, //12925 #CJK UNIFIED IDEOGRAPH
    {0xDAEE, 0x7D53}, //12926 #CJK UNIFIED IDEOGRAPH
    {0xDAEF, 0x7D56}, //12927 #CJK UNIFIED IDEOGRAPH
    {0xDAF0, 0x7D67}, //12928 #CJK UNIFIED IDEOGRAPH
    {0xDAF1, 0x7D6A}, //12929 #CJK UNIFIED IDEOGRAPH
    {0xDAF2, 0x7D4F}, //12930 #CJK UNIFIED IDEOGRAPH
    {0xDAF3, 0x7D6D}, //12931 #CJK UNIFIED IDEOGRAPH
    {0xDAF4, 0x7D5C}, //12932 #CJK UNIFIED IDEOGRAPH
    {0xDAF5, 0x7D6B}, //12933 #CJK UNIFIED IDEOGRAPH
    {0xDAF6, 0x7D52}, //12934 #CJK UNIFIED IDEOGRAPH
    {0xDAF7, 0x7D54}, //12935 #CJK UNIFIED IDEOGRAPH
    {0xDAF8, 0x7D69}, //12936 #CJK UNIFIED IDEOGRAPH
    {0xDAF9, 0x7D51}, //12937 #CJK UNIFIED IDEOGRAPH
    {0xDAFA, 0x7D5F}, //12938 #CJK UNIFIED IDEOGRAPH
    {0xDAFB, 0x7D4E}, //12939 #CJK UNIFIED IDEOGRAPH
    {0xDAFC, 0x7F3E}, //12940 #CJK UNIFIED IDEOGRAPH
    {0xDAFD, 0x7F3F}, //12941 #CJK UNIFIED IDEOGRAPH
    {0xDAFE, 0x7F65}, //12942 #CJK UNIFIED IDEOGRAPH
    {0xDB40, 0x7F66}, //12943 #CJK UNIFIED IDEOGRAPH
    {0xDB41, 0x7FA2}, //12944 #CJK UNIFIED IDEOGRAPH
    {0xDB42, 0x7FA0}, //12945 #CJK UNIFIED IDEOGRAPH
    {0xDB43, 0x7FA1}, //12946 #CJK UNIFIED IDEOGRAPH
    {0xDB44, 0x7FD7}, //12947 #CJK UNIFIED IDEOGRAPH
    {0xDB45, 0x8051}, //12948 #CJK UNIFIED IDEOGRAPH
    {0xDB46, 0x804F}, //12949 #CJK UNIFIED IDEOGRAPH
    {0xDB47, 0x8050}, //12950 #CJK UNIFIED IDEOGRAPH
    {0xDB48, 0x80FE}, //12951 #CJK UNIFIED IDEOGRAPH
    {0xDB49, 0x80D4}, //12952 #CJK UNIFIED IDEOGRAPH
    {0xDB4A, 0x8143}, //12953 #CJK UNIFIED IDEOGRAPH
    {0xDB4B, 0x814A}, //12954 #CJK UNIFIED IDEOGRAPH
    {0xDB4C, 0x8152}, //12955 #CJK UNIFIED IDEOGRAPH
    {0xDB4D, 0x814F}, //12956 #CJK UNIFIED IDEOGRAPH
    {0xDB4E, 0x8147}, //12957 #CJK UNIFIED IDEOGRAPH
    {0xDB4F, 0x813D}, //12958 #CJK UNIFIED IDEOGRAPH
    {0xDB50, 0x814D}, //12959 #CJK UNIFIED IDEOGRAPH
    {0xDB51, 0x813A}, //12960 #CJK UNIFIED IDEOGRAPH
    {0xDB52, 0x81E6}, //12961 #CJK UNIFIED IDEOGRAPH
    {0xDB53, 0x81EE}, //12962 #CJK UNIFIED IDEOGRAPH
    {0xDB54, 0x81F7}, //12963 #CJK UNIFIED IDEOGRAPH
    {0xDB55, 0x81F8}, //12964 #CJK UNIFIED IDEOGRAPH
    {0xDB56, 0x81F9}, //12965 #CJK UNIFIED IDEOGRAPH
    {0xDB57, 0x8204}, //12966 #CJK UNIFIED IDEOGRAPH
    {0xDB58, 0x823C}, //12967 #CJK UNIFIED IDEOGRAPH
    {0xDB59, 0x823D}, //12968 #CJK UNIFIED IDEOGRAPH
    {0xDB5A, 0x823F}, //12969 #CJK UNIFIED IDEOGRAPH
    {0xDB5B, 0x8275}, //12970 #CJK UNIFIED IDEOGRAPH
    {0xDB5C, 0x833B}, //12971 #CJK UNIFIED IDEOGRAPH
    {0xDB5D, 0x83CF}, //12972 #CJK UNIFIED IDEOGRAPH
    {0xDB5E, 0x83F9}, //12973 #CJK UNIFIED IDEOGRAPH
    {0xDB5F, 0x8423}, //12974 #CJK UNIFIED IDEOGRAPH
    {0xDB60, 0x83C0}, //12975 #CJK UNIFIED IDEOGRAPH
    {0xDB61, 0x83E8}, //12976 #CJK UNIFIED IDEOGRAPH
    {0xDB62, 0x8412}, //12977 #CJK UNIFIED IDEOGRAPH
    {0xDB63, 0x83E7}, //12978 #CJK UNIFIED IDEOGRAPH
    {0xDB64, 0x83E4}, //12979 #CJK UNIFIED IDEOGRAPH
    {0xDB65, 0x83FC}, //12980 #CJK UNIFIED IDEOGRAPH
    {0xDB66, 0x83F6}, //12981 #CJK UNIFIED IDEOGRAPH
    {0xDB67, 0x8410}, //12982 #CJK UNIFIED IDEOGRAPH
    {0xDB68, 0x83C6}, //12983 #CJK UNIFIED IDEOGRAPH
    {0xDB69, 0x83C8}, //12984 #CJK UNIFIED IDEOGRAPH
    {0xDB6A, 0x83EB}, //12985 #CJK UNIFIED IDEOGRAPH
    {0xDB6B, 0x83E3}, //12986 #CJK UNIFIED IDEOGRAPH
    {0xDB6C, 0x83BF}, //12987 #CJK UNIFIED IDEOGRAPH
    {0xDB6D, 0x8401}, //12988 #CJK UNIFIED IDEOGRAPH
    {0xDB6E, 0x83DD}, //12989 #CJK UNIFIED IDEOGRAPH
    {0xDB6F, 0x83E5}, //12990 #CJK UNIFIED IDEOGRAPH
    {0xDB70, 0x83D8}, //12991 #CJK UNIFIED IDEOGRAPH
    {0xDB71, 0x83FF}, //12992 #CJK UNIFIED IDEOGRAPH
    {0xDB72, 0x83E1}, //12993 #CJK UNIFIED IDEOGRAPH
    {0xDB73, 0x83CB}, //12994 #CJK UNIFIED IDEOGRAPH
    {0xDB74, 0x83CE}, //12995 #CJK UNIFIED IDEOGRAPH
    {0xDB75, 0x83D6}, //12996 #CJK UNIFIED IDEOGRAPH
    {0xDB76, 0x83F5}, //12997 #CJK UNIFIED IDEOGRAPH
    {0xDB77, 0x83C9}, //12998 #CJK UNIFIED IDEOGRAPH
    {0xDB78, 0x8409}, //12999 #CJK UNIFIED IDEOGRAPH
    {0xDB79, 0x840F}, //13000 #CJK UNIFIED IDEOGRAPH
    {0xDB7A, 0x83DE}, //13001 #CJK UNIFIED IDEOGRAPH
    {0xDB7B, 0x8411}, //13002 #CJK UNIFIED IDEOGRAPH
    {0xDB7C, 0x8406}, //13003 #CJK UNIFIED IDEOGRAPH
    {0xDB7D, 0x83C2}, //13004 #CJK UNIFIED IDEOGRAPH
    {0xDB7E, 0x83F3}, //13005 #CJK UNIFIED IDEOGRAPH
    {0xDBA1, 0x83D5}, //13006 #CJK UNIFIED IDEOGRAPH
    {0xDBA2, 0x83FA}, //13007 #CJK UNIFIED IDEOGRAPH
    {0xDBA3, 0x83C7}, //13008 #CJK UNIFIED IDEOGRAPH
    {0xDBA4, 0x83D1}, //13009 #CJK UNIFIED IDEOGRAPH
    {0xDBA5, 0x83EA}, //13010 #CJK UNIFIED IDEOGRAPH
    {0xDBA6, 0x8413}, //13011 #CJK UNIFIED IDEOGRAPH
    {0xDBA7, 0x83C3}, //13012 #CJK UNIFIED IDEOGRAPH
    {0xDBA8, 0x83EC}, //13013 #CJK UNIFIED IDEOGRAPH
    {0xDBA9, 0x83EE}, //13014 #CJK UNIFIED IDEOGRAPH
    {0xDBAA, 0x83C4}, //13015 #CJK UNIFIED IDEOGRAPH
    {0xDBAB, 0x83FB}, //13016 #CJK UNIFIED IDEOGRAPH
    {0xDBAC, 0x83D7}, //13017 #CJK UNIFIED IDEOGRAPH
    {0xDBAD, 0x83E2}, //13018 #CJK UNIFIED IDEOGRAPH
    {0xDBAE, 0x841B}, //13019 #CJK UNIFIED IDEOGRAPH
    {0xDBAF, 0x83DB}, //13020 #CJK UNIFIED IDEOGRAPH
    {0xDBB0, 0x83FE}, //13021 #CJK UNIFIED IDEOGRAPH
    {0xDBB1, 0x86D8}, //13022 #CJK UNIFIED IDEOGRAPH
    {0xDBB2, 0x86E2}, //13023 #CJK UNIFIED IDEOGRAPH
    {0xDBB3, 0x86E6}, //13024 #CJK UNIFIED IDEOGRAPH
    {0xDBB4, 0x86D3}, //13025 #CJK UNIFIED IDEOGRAPH
    {0xDBB5, 0x86E3}, //13026 #CJK UNIFIED IDEOGRAPH
    {0xDBB6, 0x86DA}, //13027 #CJK UNIFIED IDEOGRAPH
    {0xDBB7, 0x86EA}, //13028 #CJK UNIFIED IDEOGRAPH
    {0xDBB8, 0x86DD}, //13029 #CJK UNIFIED IDEOGRAPH
    {0xDBB9, 0x86EB}, //13030 #CJK UNIFIED IDEOGRAPH
    {0xDBBA, 0x86DC}, //13031 #CJK UNIFIED IDEOGRAPH
    {0xDBBB, 0x86EC}, //13032 #CJK UNIFIED IDEOGRAPH
    {0xDBBC, 0x86E9}, //13033 #CJK UNIFIED IDEOGRAPH
    {0xDBBD, 0x86D7}, //13034 #CJK UNIFIED IDEOGRAPH
    {0xDBBE, 0x86E8}, //13035 #CJK UNIFIED IDEOGRAPH
    {0xDBBF, 0x86D1}, //13036 #CJK UNIFIED IDEOGRAPH
    {0xDBC0, 0x8848}, //13037 #CJK UNIFIED IDEOGRAPH
    {0xDBC1, 0x8856}, //13038 #CJK UNIFIED IDEOGRAPH
    {0xDBC2, 0x8855}, //13039 #CJK UNIFIED IDEOGRAPH
    {0xDBC3, 0x88BA}, //13040 #CJK UNIFIED IDEOGRAPH
    {0xDBC4, 0x88D7}, //13041 #CJK UNIFIED IDEOGRAPH
    {0xDBC5, 0x88B9}, //13042 #CJK UNIFIED IDEOGRAPH
    {0xDBC6, 0x88B8}, //13043 #CJK UNIFIED IDEOGRAPH
    {0xDBC7, 0x88C0}, //13044 #CJK UNIFIED IDEOGRAPH
    {0xDBC8, 0x88BE}, //13045 #CJK UNIFIED IDEOGRAPH
    {0xDBC9, 0x88B6}, //13046 #CJK UNIFIED IDEOGRAPH
    {0xDBCA, 0x88BC}, //13047 #CJK UNIFIED IDEOGRAPH
    {0xDBCB, 0x88B7}, //13048 #CJK UNIFIED IDEOGRAPH
    {0xDBCC, 0x88BD}, //13049 #CJK UNIFIED IDEOGRAPH
    {0xDBCD, 0x88B2}, //13050 #CJK UNIFIED IDEOGRAPH
    {0xDBCE, 0x8901}, //13051 #CJK UNIFIED IDEOGRAPH
    {0xDBCF, 0x88C9}, //13052 #CJK UNIFIED IDEOGRAPH
    {0xDBD0, 0x8995}, //13053 #CJK UNIFIED IDEOGRAPH
    {0xDBD1, 0x8998}, //13054 #CJK UNIFIED IDEOGRAPH
    {0xDBD2, 0x8997}, //13055 #CJK UNIFIED IDEOGRAPH
    {0xDBD3, 0x89DD}, //13056 #CJK UNIFIED IDEOGRAPH
    {0xDBD4, 0x89DA}, //13057 #CJK UNIFIED IDEOGRAPH
    {0xDBD5, 0x89DB}, //13058 #CJK UNIFIED IDEOGRAPH
    {0xDBD6, 0x8A4E}, //13059 #CJK UNIFIED IDEOGRAPH
    {0xDBD7, 0x8A4D}, //13060 #CJK UNIFIED IDEOGRAPH
    {0xDBD8, 0x8A39}, //13061 #CJK UNIFIED IDEOGRAPH
    {0xDBD9, 0x8A59}, //13062 #CJK UNIFIED IDEOGRAPH
    {0xDBDA, 0x8A40}, //13063 #CJK UNIFIED IDEOGRAPH
    {0xDBDB, 0x8A57}, //13064 #CJK UNIFIED IDEOGRAPH
    {0xDBDC, 0x8A58}, //13065 #CJK UNIFIED IDEOGRAPH
    {0xDBDD, 0x8A44}, //13066 #CJK UNIFIED IDEOGRAPH
    {0xDBDE, 0x8A45}, //13067 #CJK UNIFIED IDEOGRAPH
    {0xDBDF, 0x8A52}, //13068 #CJK UNIFIED IDEOGRAPH
    {0xDBE0, 0x8A48}, //13069 #CJK UNIFIED IDEOGRAPH
    {0xDBE1, 0x8A51}, //13070 #CJK UNIFIED IDEOGRAPH
    {0xDBE2, 0x8A4A}, //13071 #CJK UNIFIED IDEOGRAPH
    {0xDBE3, 0x8A4C}, //13072 #CJK UNIFIED IDEOGRAPH
    {0xDBE4, 0x8A4F}, //13073 #CJK UNIFIED IDEOGRAPH
    {0xDBE5, 0x8C5F}, //13074 #CJK UNIFIED IDEOGRAPH
    {0xDBE6, 0x8C81}, //13075 #CJK UNIFIED IDEOGRAPH
    {0xDBE7, 0x8C80}, //13076 #CJK UNIFIED IDEOGRAPH
    {0xDBE8, 0x8CBA}, //13077 #CJK UNIFIED IDEOGRAPH
    {0xDBE9, 0x8CBE}, //13078 #CJK UNIFIED IDEOGRAPH
    {0xDBEA, 0x8CB0}, //13079 #CJK UNIFIED IDEOGRAPH
    {0xDBEB, 0x8CB9}, //13080 #CJK UNIFIED IDEOGRAPH
    {0xDBEC, 0x8CB5}, //13081 #CJK UNIFIED IDEOGRAPH
    {0xDBED, 0x8D84}, //13082 #CJK UNIFIED IDEOGRAPH
    {0xDBEE, 0x8D80}, //13083 #CJK UNIFIED IDEOGRAPH
    {0xDBEF, 0x8D89}, //13084 #CJK UNIFIED IDEOGRAPH
    {0xDBF0, 0x8DD8}, //13085 #CJK UNIFIED IDEOGRAPH
    {0xDBF1, 0x8DD3}, //13086 #CJK UNIFIED IDEOGRAPH
    {0xDBF2, 0x8DCD}, //13087 #CJK UNIFIED IDEOGRAPH
    {0xDBF3, 0x8DC7}, //13088 #CJK UNIFIED IDEOGRAPH
    {0xDBF4, 0x8DD6}, //13089 #CJK UNIFIED IDEOGRAPH
    {0xDBF5, 0x8DDC}, //13090 #CJK UNIFIED IDEOGRAPH
    {0xDBF6, 0x8DCF}, //13091 #CJK UNIFIED IDEOGRAPH
    {0xDBF7, 0x8DD5}, //13092 #CJK UNIFIED IDEOGRAPH
    {0xDBF8, 0x8DD9}, //13093 #CJK UNIFIED IDEOGRAPH
    {0xDBF9, 0x8DC8}, //13094 #CJK UNIFIED IDEOGRAPH
    {0xDBFA, 0x8DD7}, //13095 #CJK UNIFIED IDEOGRAPH
    {0xDBFB, 0x8DC5}, //13096 #CJK UNIFIED IDEOGRAPH
    {0xDBFC, 0x8EEF}, //13097 #CJK UNIFIED IDEOGRAPH
    {0xDBFD, 0x8EF7}, //13098 #CJK UNIFIED IDEOGRAPH
    {0xDBFE, 0x8EFA}, //13099 #CJK UNIFIED IDEOGRAPH
    {0xDC40, 0x8EF9}, //13100 #CJK UNIFIED IDEOGRAPH
    {0xDC41, 0x8EE6}, //13101 #CJK UNIFIED IDEOGRAPH
    {0xDC42, 0x8EEE}, //13102 #CJK UNIFIED IDEOGRAPH
    {0xDC43, 0x8EE5}, //13103 #CJK UNIFIED IDEOGRAPH
    {0xDC44, 0x8EF5}, //13104 #CJK UNIFIED IDEOGRAPH
    {0xDC45, 0x8EE7}, //13105 #CJK UNIFIED IDEOGRAPH
    {0xDC46, 0x8EE8}, //13106 #CJK UNIFIED IDEOGRAPH
    {0xDC47, 0x8EF6}, //13107 #CJK UNIFIED IDEOGRAPH
    {0xDC48, 0x8EEB}, //13108 #CJK UNIFIED IDEOGRAPH
    {0xDC49, 0x8EF1}, //13109 #CJK UNIFIED IDEOGRAPH
    {0xDC4A, 0x8EEC}, //13110 #CJK UNIFIED IDEOGRAPH
    {0xDC4B, 0x8EF4}, //13111 #CJK UNIFIED IDEOGRAPH
    {0xDC4C, 0x8EE9}, //13112 #CJK UNIFIED IDEOGRAPH
    {0xDC4D, 0x902D}, //13113 #CJK UNIFIED IDEOGRAPH
    {0xDC4E, 0x9034}, //13114 #CJK UNIFIED IDEOGRAPH
    {0xDC4F, 0x902F}, //13115 #CJK UNIFIED IDEOGRAPH
    {0xDC50, 0x9106}, //13116 #CJK UNIFIED IDEOGRAPH
    {0xDC51, 0x912C}, //13117 #CJK UNIFIED IDEOGRAPH
    {0xDC52, 0x9104}, //13118 #CJK UNIFIED IDEOGRAPH
    {0xDC53, 0x90FF}, //13119 #CJK UNIFIED IDEOGRAPH
    {0xDC54, 0x90FC}, //13120 #CJK UNIFIED IDEOGRAPH
    {0xDC55, 0x9108}, //13121 #CJK UNIFIED IDEOGRAPH
    {0xDC56, 0x90F9}, //13122 #CJK UNIFIED IDEOGRAPH
    {0xDC57, 0x90FB}, //13123 #CJK UNIFIED IDEOGRAPH
    {0xDC58, 0x9101}, //13124 #CJK UNIFIED IDEOGRAPH
    {0xDC59, 0x9100}, //13125 #CJK UNIFIED IDEOGRAPH
    {0xDC5A, 0x9107}, //13126 #CJK UNIFIED IDEOGRAPH
    {0xDC5B, 0x9105}, //13127 #CJK UNIFIED IDEOGRAPH
    {0xDC5C, 0x9103}, //13128 #CJK UNIFIED IDEOGRAPH
    {0xDC5D, 0x9161}, //13129 #CJK UNIFIED IDEOGRAPH
    {0xDC5E, 0x9164}, //13130 #CJK UNIFIED IDEOGRAPH
    {0xDC5F, 0x915F}, //13131 #CJK UNIFIED IDEOGRAPH
    {0xDC60, 0x9162}, //13132 #CJK UNIFIED IDEOGRAPH
    {0xDC61, 0x9160}, //13133 #CJK UNIFIED IDEOGRAPH
    {0xDC62, 0x9201}, //13134 #CJK UNIFIED IDEOGRAPH
    {0xDC63, 0x920A}, //13135 #CJK UNIFIED IDEOGRAPH
    {0xDC64, 0x9225}, //13136 #CJK UNIFIED IDEOGRAPH
    {0xDC65, 0x9203}, //13137 #CJK UNIFIED IDEOGRAPH
    {0xDC66, 0x921A}, //13138 #CJK UNIFIED IDEOGRAPH
    {0xDC67, 0x9226}, //13139 #CJK UNIFIED IDEOGRAPH
    {0xDC68, 0x920F}, //13140 #CJK UNIFIED IDEOGRAPH
    {0xDC69, 0x920C}, //13141 #CJK UNIFIED IDEOGRAPH
    {0xDC6A, 0x9200}, //13142 #CJK UNIFIED IDEOGRAPH
    {0xDC6B, 0x9212}, //13143 #CJK UNIFIED IDEOGRAPH
    {0xDC6C, 0x91FF}, //13144 #CJK UNIFIED IDEOGRAPH
    {0xDC6D, 0x91FD}, //13145 #CJK UNIFIED IDEOGRAPH
    {0xDC6E, 0x9206}, //13146 #CJK UNIFIED IDEOGRAPH
    {0xDC6F, 0x9204}, //13147 #CJK UNIFIED IDEOGRAPH
    {0xDC70, 0x9227}, //13148 #CJK UNIFIED IDEOGRAPH
    {0xDC71, 0x9202}, //13149 #CJK UNIFIED IDEOGRAPH
    {0xDC72, 0x921C}, //13150 #CJK UNIFIED IDEOGRAPH
    {0xDC73, 0x9224}, //13151 #CJK UNIFIED IDEOGRAPH
    {0xDC74, 0x9219}, //13152 #CJK UNIFIED IDEOGRAPH
    {0xDC75, 0x9217}, //13153 #CJK UNIFIED IDEOGRAPH
    {0xDC76, 0x9205}, //13154 #CJK UNIFIED IDEOGRAPH
    {0xDC77, 0x9216}, //13155 #CJK UNIFIED IDEOGRAPH
    {0xDC78, 0x957B}, //13156 #CJK UNIFIED IDEOGRAPH
    {0xDC79, 0x958D}, //13157 #CJK UNIFIED IDEOGRAPH
    {0xDC7A, 0x958C}, //13158 #CJK UNIFIED IDEOGRAPH
    {0xDC7B, 0x9590}, //13159 #CJK UNIFIED IDEOGRAPH
    {0xDC7C, 0x9687}, //13160 #CJK UNIFIED IDEOGRAPH
    {0xDC7D, 0x967E}, //13161 #CJK UNIFIED IDEOGRAPH
    {0xDC7E, 0x9688}, //13162 #CJK UNIFIED IDEOGRAPH
    {0xDCA1, 0x9689}, //13163 #CJK UNIFIED IDEOGRAPH
    {0xDCA2, 0x9683}, //13164 #CJK UNIFIED IDEOGRAPH
    {0xDCA3, 0x9680}, //13165 #CJK UNIFIED IDEOGRAPH
    {0xDCA4, 0x96C2}, //13166 #CJK UNIFIED IDEOGRAPH
    {0xDCA5, 0x96C8}, //13167 #CJK UNIFIED IDEOGRAPH
    {0xDCA6, 0x96C3}, //13168 #CJK UNIFIED IDEOGRAPH
    {0xDCA7, 0x96F1}, //13169 #CJK UNIFIED IDEOGRAPH
    {0xDCA8, 0x96F0}, //13170 #CJK UNIFIED IDEOGRAPH
    {0xDCA9, 0x976C}, //13171 #CJK UNIFIED IDEOGRAPH
    {0xDCAA, 0x9770}, //13172 #CJK UNIFIED IDEOGRAPH
    {0xDCAB, 0x976E}, //13173 #CJK UNIFIED IDEOGRAPH
    {0xDCAC, 0x9807}, //13174 #CJK UNIFIED IDEOGRAPH
    {0xDCAD, 0x98A9}, //13175 #CJK UNIFIED IDEOGRAPH
    {0xDCAE, 0x98EB}, //13176 #CJK UNIFIED IDEOGRAPH
    {0xDCAF, 0x9CE6}, //13177 #CJK UNIFIED IDEOGRAPH
    {0xDCB0, 0x9EF9}, //13178 #CJK UNIFIED IDEOGRAPH
    {0xDCB1, 0x4E83}, //13179 #CJK UNIFIED IDEOGRAPH
    {0xDCB2, 0x4E84}, //13180 #CJK UNIFIED IDEOGRAPH
    {0xDCB3, 0x4EB6}, //13181 #CJK UNIFIED IDEOGRAPH
    {0xDCB4, 0x50BD}, //13182 #CJK UNIFIED IDEOGRAPH
    {0xDCB5, 0x50BF}, //13183 #CJK UNIFIED IDEOGRAPH
    {0xDCB6, 0x50C6}, //13184 #CJK UNIFIED IDEOGRAPH
    {0xDCB7, 0x50AE}, //13185 #CJK UNIFIED IDEOGRAPH
    {0xDCB8, 0x50C4}, //13186 #CJK UNIFIED IDEOGRAPH
    {0xDCB9, 0x50CA}, //13187 #CJK UNIFIED IDEOGRAPH
    {0xDCBA, 0x50B4}, //13188 #CJK UNIFIED IDEOGRAPH
    {0xDCBB, 0x50C8}, //13189 #CJK UNIFIED IDEOGRAPH
    {0xDCBC, 0x50C2}, //13190 #CJK UNIFIED IDEOGRAPH
    {0xDCBD, 0x50B0}, //13191 #CJK UNIFIED IDEOGRAPH
    {0xDCBE, 0x50C1}, //13192 #CJK UNIFIED IDEOGRAPH
    {0xDCBF, 0x50BA}, //13193 #CJK UNIFIED IDEOGRAPH
    {0xDCC0, 0x50B1}, //13194 #CJK UNIFIED IDEOGRAPH
    {0xDCC1, 0x50CB}, //13195 #CJK UNIFIED IDEOGRAPH
    {0xDCC2, 0x50C9}, //13196 #CJK UNIFIED IDEOGRAPH
    {0xDCC3, 0x50B6}, //13197 #CJK UNIFIED IDEOGRAPH
    {0xDCC4, 0x50B8}, //13198 #CJK UNIFIED IDEOGRAPH
    {0xDCC5, 0x51D7}, //13199 #CJK UNIFIED IDEOGRAPH
    {0xDCC6, 0x527A}, //13200 #CJK UNIFIED IDEOGRAPH
    {0xDCC7, 0x5278}, //13201 #CJK UNIFIED IDEOGRAPH
    {0xDCC8, 0x527B}, //13202 #CJK UNIFIED IDEOGRAPH
    {0xDCC9, 0x527C}, //13203 #CJK UNIFIED IDEOGRAPH
    {0xDCCA, 0x55C3}, //13204 #CJK UNIFIED IDEOGRAPH
    {0xDCCB, 0x55DB}, //13205 #CJK UNIFIED IDEOGRAPH
    {0xDCCC, 0x55CC}, //13206 #CJK UNIFIED IDEOGRAPH
    {0xDCCD, 0x55D0}, //13207 #CJK UNIFIED IDEOGRAPH
    {0xDCCE, 0x55CB}, //13208 #CJK UNIFIED IDEOGRAPH
    {0xDCCF, 0x55CA}, //13209 #CJK UNIFIED IDEOGRAPH
    {0xDCD0, 0x55DD}, //13210 #CJK UNIFIED IDEOGRAPH
    {0xDCD1, 0x55C0}, //13211 #CJK UNIFIED IDEOGRAPH
    {0xDCD2, 0x55D4}, //13212 #CJK UNIFIED IDEOGRAPH
    {0xDCD3, 0x55C4}, //13213 #CJK UNIFIED IDEOGRAPH
    {0xDCD4, 0x55E9}, //13214 #CJK UNIFIED IDEOGRAPH
    {0xDCD5, 0x55BF}, //13215 #CJK UNIFIED IDEOGRAPH
    {0xDCD6, 0x55D2}, //13216 #CJK UNIFIED IDEOGRAPH
    {0xDCD7, 0x558D}, //13217 #CJK UNIFIED IDEOGRAPH
    {0xDCD8, 0x55CF}, //13218 #CJK UNIFIED IDEOGRAPH
    {0xDCD9, 0x55D5}, //13219 #CJK UNIFIED IDEOGRAPH
    {0xDCDA, 0x55E2}, //13220 #CJK UNIFIED IDEOGRAPH
    {0xDCDB, 0x55D6}, //13221 #CJK UNIFIED IDEOGRAPH
    {0xDCDC, 0x55C8}, //13222 #CJK UNIFIED IDEOGRAPH
    {0xDCDD, 0x55F2}, //13223 #CJK UNIFIED IDEOGRAPH
    {0xDCDE, 0x55CD}, //13224 #CJK UNIFIED IDEOGRAPH
    {0xDCDF, 0x55D9}, //13225 #CJK UNIFIED IDEOGRAPH
    {0xDCE0, 0x55C2}, //13226 #CJK UNIFIED IDEOGRAPH
    {0xDCE1, 0x5714}, //13227 #CJK UNIFIED IDEOGRAPH
    {0xDCE2, 0x5853}, //13228 #CJK UNIFIED IDEOGRAPH
    {0xDCE3, 0x5868}, //13229 #CJK UNIFIED IDEOGRAPH
    {0xDCE4, 0x5864}, //13230 #CJK UNIFIED IDEOGRAPH
    {0xDCE5, 0x584F}, //13231 #CJK UNIFIED IDEOGRAPH
    {0xDCE6, 0x584D}, //13232 #CJK UNIFIED IDEOGRAPH
    {0xDCE7, 0x5849}, //13233 #CJK UNIFIED IDEOGRAPH
    {0xDCE8, 0x586F}, //13234 #CJK UNIFIED IDEOGRAPH
    {0xDCE9, 0x5855}, //13235 #CJK UNIFIED IDEOGRAPH
    {0xDCEA, 0x584E}, //13236 #CJK UNIFIED IDEOGRAPH
    {0xDCEB, 0x585D}, //13237 #CJK UNIFIED IDEOGRAPH
    {0xDCEC, 0x5859}, //13238 #CJK UNIFIED IDEOGRAPH
    {0xDCED, 0x5865}, //13239 #CJK UNIFIED IDEOGRAPH
    {0xDCEE, 0x585B}, //13240 #CJK UNIFIED IDEOGRAPH
    {0xDCEF, 0x583D}, //13241 #CJK UNIFIED IDEOGRAPH
    {0xDCF0, 0x5863}, //13242 #CJK UNIFIED IDEOGRAPH
    {0xDCF1, 0x5871}, //13243 #CJK UNIFIED IDEOGRAPH
    {0xDCF2, 0x58FC}, //13244 #CJK UNIFIED IDEOGRAPH
    {0xDCF3, 0x5AC7}, //13245 #CJK UNIFIED IDEOGRAPH
    {0xDCF4, 0x5AC4}, //13246 #CJK UNIFIED IDEOGRAPH
    {0xDCF5, 0x5ACB}, //13247 #CJK UNIFIED IDEOGRAPH
    {0xDCF6, 0x5ABA}, //13248 #CJK UNIFIED IDEOGRAPH
    {0xDCF7, 0x5AB8}, //13249 #CJK UNIFIED IDEOGRAPH
    {0xDCF8, 0x5AB1}, //13250 #CJK UNIFIED IDEOGRAPH
    {0xDCF9, 0x5AB5}, //13251 #CJK UNIFIED IDEOGRAPH
    {0xDCFA, 0x5AB0}, //13252 #CJK UNIFIED IDEOGRAPH
    {0xDCFB, 0x5ABF}, //13253 #CJK UNIFIED IDEOGRAPH
    {0xDCFC, 0x5AC8}, //13254 #CJK UNIFIED IDEOGRAPH
    {0xDCFD, 0x5ABB}, //13255 #CJK UNIFIED IDEOGRAPH
    {0xDCFE, 0x5AC6}, //13256 #CJK UNIFIED IDEOGRAPH
    {0xDD40, 0x5AB7}, //13257 #CJK UNIFIED IDEOGRAPH
    {0xDD41, 0x5AC0}, //13258 #CJK UNIFIED IDEOGRAPH
    {0xDD42, 0x5ACA}, //13259 #CJK UNIFIED IDEOGRAPH
    {0xDD43, 0x5AB4}, //13260 #CJK UNIFIED IDEOGRAPH
    {0xDD44, 0x5AB6}, //13261 #CJK UNIFIED IDEOGRAPH
    {0xDD45, 0x5ACD}, //13262 #CJK UNIFIED IDEOGRAPH
    {0xDD46, 0x5AB9}, //13263 #CJK UNIFIED IDEOGRAPH
    {0xDD47, 0x5A90}, //13264 #CJK UNIFIED IDEOGRAPH
    {0xDD48, 0x5BD6}, //13265 #CJK UNIFIED IDEOGRAPH
    {0xDD49, 0x5BD8}, //13266 #CJK UNIFIED IDEOGRAPH
    {0xDD4A, 0x5BD9}, //13267 #CJK UNIFIED IDEOGRAPH
    {0xDD4B, 0x5C1F}, //13268 #CJK UNIFIED IDEOGRAPH
    {0xDD4C, 0x5C33}, //13269 #CJK UNIFIED IDEOGRAPH
    {0xDD4D, 0x5D71}, //13270 #CJK UNIFIED IDEOGRAPH
    {0xDD4E, 0x5D63}, //13271 #CJK UNIFIED IDEOGRAPH
    {0xDD4F, 0x5D4A}, //13272 #CJK UNIFIED IDEOGRAPH
    {0xDD50, 0x5D65}, //13273 #CJK UNIFIED IDEOGRAPH
    {0xDD51, 0x5D72}, //13274 #CJK UNIFIED IDEOGRAPH
    {0xDD52, 0x5D6C}, //13275 #CJK UNIFIED IDEOGRAPH
    {0xDD53, 0x5D5E}, //13276 #CJK UNIFIED IDEOGRAPH
    {0xDD54, 0x5D68}, //13277 #CJK UNIFIED IDEOGRAPH
    {0xDD55, 0x5D67}, //13278 #CJK UNIFIED IDEOGRAPH
    {0xDD56, 0x5D62}, //13279 #CJK UNIFIED IDEOGRAPH
    {0xDD57, 0x5DF0}, //13280 #CJK UNIFIED IDEOGRAPH
    {0xDD58, 0x5E4F}, //13281 #CJK UNIFIED IDEOGRAPH
    {0xDD59, 0x5E4E}, //13282 #CJK UNIFIED IDEOGRAPH
    {0xDD5A, 0x5E4A}, //13283 #CJK UNIFIED IDEOGRAPH
    {0xDD5B, 0x5E4D}, //13284 #CJK UNIFIED IDEOGRAPH
    {0xDD5C, 0x5E4B}, //13285 #CJK UNIFIED IDEOGRAPH
    {0xDD5D, 0x5EC5}, //13286 #CJK UNIFIED IDEOGRAPH
    {0xDD5E, 0x5ECC}, //13287 #CJK UNIFIED IDEOGRAPH
    {0xDD5F, 0x5EC6}, //13288 #CJK UNIFIED IDEOGRAPH
    {0xDD60, 0x5ECB}, //13289 #CJK UNIFIED IDEOGRAPH
    {0xDD61, 0x5EC7}, //13290 #CJK UNIFIED IDEOGRAPH
    {0xDD62, 0x5F40}, //13291 #CJK UNIFIED IDEOGRAPH
    {0xDD63, 0x5FAF}, //13292 #CJK UNIFIED IDEOGRAPH
    {0xDD64, 0x5FAD}, //13293 #CJK UNIFIED IDEOGRAPH
    {0xDD65, 0x60F7}, //13294 #CJK UNIFIED IDEOGRAPH
    {0xDD66, 0x6149}, //13295 #CJK UNIFIED IDEOGRAPH
    {0xDD67, 0x614A}, //13296 #CJK UNIFIED IDEOGRAPH
    {0xDD68, 0x612B}, //13297 #CJK UNIFIED IDEOGRAPH
    {0xDD69, 0x6145}, //13298 #CJK UNIFIED IDEOGRAPH
    {0xDD6A, 0x6136}, //13299 #CJK UNIFIED IDEOGRAPH
    {0xDD6B, 0x6132}, //13300 #CJK UNIFIED IDEOGRAPH
    {0xDD6C, 0x612E}, //13301 #CJK UNIFIED IDEOGRAPH
    {0xDD6D, 0x6146}, //13302 #CJK UNIFIED IDEOGRAPH
    {0xDD6E, 0x612F}, //13303 #CJK UNIFIED IDEOGRAPH
    {0xDD6F, 0x614F}, //13304 #CJK UNIFIED IDEOGRAPH
    {0xDD70, 0x6129}, //13305 #CJK UNIFIED IDEOGRAPH
    {0xDD71, 0x6140}, //13306 #CJK UNIFIED IDEOGRAPH
    {0xDD72, 0x6220}, //13307 #CJK UNIFIED IDEOGRAPH
    {0xDD73, 0x9168}, //13308 #CJK UNIFIED IDEOGRAPH
    {0xDD74, 0x6223}, //13309 #CJK UNIFIED IDEOGRAPH
    {0xDD75, 0x6225}, //13310 #CJK UNIFIED IDEOGRAPH
    {0xDD76, 0x6224}, //13311 #CJK UNIFIED IDEOGRAPH
    {0xDD77, 0x63C5}, //13312 #CJK UNIFIED IDEOGRAPH
    {0xDD78, 0x63F1}, //13313 #CJK UNIFIED IDEOGRAPH
    {0xDD79, 0x63EB}, //13314 #CJK UNIFIED IDEOGRAPH
    {0xDD7A, 0x6410}, //13315 #CJK UNIFIED IDEOGRAPH
    {0xDD7B, 0x6412}, //13316 #CJK UNIFIED IDEOGRAPH
    {0xDD7C, 0x6409}, //13317 #CJK UNIFIED IDEOGRAPH
    {0xDD7D, 0x6420}, //13318 #CJK UNIFIED IDEOGRAPH
    {0xDD7E, 0x6424}, //13319 #CJK UNIFIED IDEOGRAPH
    {0xDDA1, 0x6433}, //13320 #CJK UNIFIED IDEOGRAPH
    {0xDDA2, 0x6443}, //13321 #CJK UNIFIED IDEOGRAPH
    {0xDDA3, 0x641F}, //13322 #CJK UNIFIED IDEOGRAPH
    {0xDDA4, 0x6415}, //13323 #CJK UNIFIED IDEOGRAPH
    {0xDDA5, 0x6418}, //13324 #CJK UNIFIED IDEOGRAPH
    {0xDDA6, 0x6439}, //13325 #CJK UNIFIED IDEOGRAPH
    {0xDDA7, 0x6437}, //13326 #CJK UNIFIED IDEOGRAPH
    {0xDDA8, 0x6422}, //13327 #CJK UNIFIED IDEOGRAPH
    {0xDDA9, 0x6423}, //13328 #CJK UNIFIED IDEOGRAPH
    {0xDDAA, 0x640C}, //13329 #CJK UNIFIED IDEOGRAPH
    {0xDDAB, 0x6426}, //13330 #CJK UNIFIED IDEOGRAPH
    {0xDDAC, 0x6430}, //13331 #CJK UNIFIED IDEOGRAPH
    {0xDDAD, 0x6428}, //13332 #CJK UNIFIED IDEOGRAPH
    {0xDDAE, 0x6441}, //13333 #CJK UNIFIED IDEOGRAPH
    {0xDDAF, 0x6435}, //13334 #CJK UNIFIED IDEOGRAPH
    {0xDDB0, 0x642F}, //13335 #CJK UNIFIED IDEOGRAPH
    {0xDDB1, 0x640A}, //13336 #CJK UNIFIED IDEOGRAPH
    {0xDDB2, 0x641A}, //13337 #CJK UNIFIED IDEOGRAPH
    {0xDDB3, 0x6440}, //13338 #CJK UNIFIED IDEOGRAPH
    {0xDDB4, 0x6425}, //13339 #CJK UNIFIED IDEOGRAPH
    {0xDDB5, 0x6427}, //13340 #CJK UNIFIED IDEOGRAPH
    {0xDDB6, 0x640B}, //13341 #CJK UNIFIED IDEOGRAPH
    {0xDDB7, 0x63E7}, //13342 #CJK UNIFIED IDEOGRAPH
    {0xDDB8, 0x641B}, //13343 #CJK UNIFIED IDEOGRAPH
    {0xDDB9, 0x642E}, //13344 #CJK UNIFIED IDEOGRAPH
    {0xDDBA, 0x6421}, //13345 #CJK UNIFIED IDEOGRAPH
    {0xDDBB, 0x640E}, //13346 #CJK UNIFIED IDEOGRAPH
    {0xDDBC, 0x656F}, //13347 #CJK UNIFIED IDEOGRAPH
    {0xDDBD, 0x6592}, //13348 #CJK UNIFIED IDEOGRAPH
    {0xDDBE, 0x65D3}, //13349 #CJK UNIFIED IDEOGRAPH
    {0xDDBF, 0x6686}, //13350 #CJK UNIFIED IDEOGRAPH
    {0xDDC0, 0x668C}, //13351 #CJK UNIFIED IDEOGRAPH
    {0xDDC1, 0x6695}, //13352 #CJK UNIFIED IDEOGRAPH
    {0xDDC2, 0x6690}, //13353 #CJK UNIFIED IDEOGRAPH
    {0xDDC3, 0x668B}, //13354 #CJK UNIFIED IDEOGRAPH
    {0xDDC4, 0x668A}, //13355 #CJK UNIFIED IDEOGRAPH
    {0xDDC5, 0x6699}, //13356 #CJK UNIFIED IDEOGRAPH
    {0xDDC6, 0x6694}, //13357 #CJK UNIFIED IDEOGRAPH
    {0xDDC7, 0x6678}, //13358 #CJK UNIFIED IDEOGRAPH
    {0xDDC8, 0x6720}, //13359 #CJK UNIFIED IDEOGRAPH
    {0xDDC9, 0x6966}, //13360 #CJK UNIFIED IDEOGRAPH
    {0xDDCA, 0x695F}, //13361 #CJK UNIFIED IDEOGRAPH
    {0xDDCB, 0x6938}, //13362 #CJK UNIFIED IDEOGRAPH
    {0xDDCC, 0x694E}, //13363 #CJK UNIFIED IDEOGRAPH
    {0xDDCD, 0x6962}, //13364 #CJK UNIFIED IDEOGRAPH
    {0xDDCE, 0x6971}, //13365 #CJK UNIFIED IDEOGRAPH
    {0xDDCF, 0x693F}, //13366 #CJK UNIFIED IDEOGRAPH
    {0xDDD0, 0x6945}, //13367 #CJK UNIFIED IDEOGRAPH
    {0xDDD1, 0x696A}, //13368 #CJK UNIFIED IDEOGRAPH
    {0xDDD2, 0x6939}, //13369 #CJK UNIFIED IDEOGRAPH
    {0xDDD3, 0x6942}, //13370 #CJK UNIFIED IDEOGRAPH
    {0xDDD4, 0x6957}, //13371 #CJK UNIFIED IDEOGRAPH
    {0xDDD5, 0x6959}, //13372 #CJK UNIFIED IDEOGRAPH
    {0xDDD6, 0x697A}, //13373 #CJK UNIFIED IDEOGRAPH
    {0xDDD7, 0x6948}, //13374 #CJK UNIFIED IDEOGRAPH
    {0xDDD8, 0x6949}, //13375 #CJK UNIFIED IDEOGRAPH
    {0xDDD9, 0x6935}, //13376 #CJK UNIFIED IDEOGRAPH
    {0xDDDA, 0x696C}, //13377 #CJK UNIFIED IDEOGRAPH
    {0xDDDB, 0x6933}, //13378 #CJK UNIFIED IDEOGRAPH
    {0xDDDC, 0x693D}, //13379 #CJK UNIFIED IDEOGRAPH
    {0xDDDD, 0x6965}, //13380 #CJK UNIFIED IDEOGRAPH
    {0xDDDE, 0x68F0}, //13381 #CJK UNIFIED IDEOGRAPH
    {0xDDDF, 0x6978}, //13382 #CJK UNIFIED IDEOGRAPH
    {0xDDE0, 0x6934}, //13383 #CJK UNIFIED IDEOGRAPH
    {0xDDE1, 0x6969}, //13384 #CJK UNIFIED IDEOGRAPH
    {0xDDE2, 0x6940}, //13385 #CJK UNIFIED IDEOGRAPH
    {0xDDE3, 0x696F}, //13386 #CJK UNIFIED IDEOGRAPH
    {0xDDE4, 0x6944}, //13387 #CJK UNIFIED IDEOGRAPH
    {0xDDE5, 0x6976}, //13388 #CJK UNIFIED IDEOGRAPH
    {0xDDE6, 0x6958}, //13389 #CJK UNIFIED IDEOGRAPH
    {0xDDE7, 0x6941}, //13390 #CJK UNIFIED IDEOGRAPH
    {0xDDE8, 0x6974}, //13391 #CJK UNIFIED IDEOGRAPH
    {0xDDE9, 0x694C}, //13392 #CJK UNIFIED IDEOGRAPH
    {0xDDEA, 0x693B}, //13393 #CJK UNIFIED IDEOGRAPH
    {0xDDEB, 0x694B}, //13394 #CJK UNIFIED IDEOGRAPH
    {0xDDEC, 0x6937}, //13395 #CJK UNIFIED IDEOGRAPH
    {0xDDED, 0x695C}, //13396 #CJK UNIFIED IDEOGRAPH
    {0xDDEE, 0x694F}, //13397 #CJK UNIFIED IDEOGRAPH
    {0xDDEF, 0x6951}, //13398 #CJK UNIFIED IDEOGRAPH
    {0xDDF0, 0x6932}, //13399 #CJK UNIFIED IDEOGRAPH
    {0xDDF1, 0x6952}, //13400 #CJK UNIFIED IDEOGRAPH
    {0xDDF2, 0x692F}, //13401 #CJK UNIFIED IDEOGRAPH
    {0xDDF3, 0x697B}, //13402 #CJK UNIFIED IDEOGRAPH
    {0xDDF4, 0x693C}, //13403 #CJK UNIFIED IDEOGRAPH
    {0xDDF5, 0x6B46}, //13404 #CJK UNIFIED IDEOGRAPH
    {0xDDF6, 0x6B45}, //13405 #CJK UNIFIED IDEOGRAPH
    {0xDDF7, 0x6B43}, //13406 #CJK UNIFIED IDEOGRAPH
    {0xDDF8, 0x6B42}, //13407 #CJK UNIFIED IDEOGRAPH
    {0xDDF9, 0x6B48}, //13408 #CJK UNIFIED IDEOGRAPH
    {0xDDFA, 0x6B41}, //13409 #CJK UNIFIED IDEOGRAPH
    {0xDDFB, 0x6B9B}, //13410 #CJK UNIFIED IDEOGRAPH
    {0xDDFC, 0xFA0D}, //13411 #CJK COMPATIBILITY IDEOGRAPH
    {0xDDFD, 0x6BFB}, //13412 #CJK UNIFIED IDEOGRAPH
    {0xDDFE, 0x6BFC}, //13413 #CJK UNIFIED IDEOGRAPH
    {0xDE40, 0x6BF9}, //13414 #CJK UNIFIED IDEOGRAPH
    {0xDE41, 0x6BF7}, //13415 #CJK UNIFIED IDEOGRAPH
    {0xDE42, 0x6BF8}, //13416 #CJK UNIFIED IDEOGRAPH
    {0xDE43, 0x6E9B}, //13417 #CJK UNIFIED IDEOGRAPH
    {0xDE44, 0x6ED6}, //13418 #CJK UNIFIED IDEOGRAPH
    {0xDE45, 0x6EC8}, //13419 #CJK UNIFIED IDEOGRAPH
    {0xDE46, 0x6E8F}, //13420 #CJK UNIFIED IDEOGRAPH
    {0xDE47, 0x6EC0}, //13421 #CJK UNIFIED IDEOGRAPH
    {0xDE48, 0x6E9F}, //13422 #CJK UNIFIED IDEOGRAPH
    {0xDE49, 0x6E93}, //13423 #CJK UNIFIED IDEOGRAPH
    {0xDE4A, 0x6E94}, //13424 #CJK UNIFIED IDEOGRAPH
    {0xDE4B, 0x6EA0}, //13425 #CJK UNIFIED IDEOGRAPH
    {0xDE4C, 0x6EB1}, //13426 #CJK UNIFIED IDEOGRAPH
    {0xDE4D, 0x6EB9}, //13427 #CJK UNIFIED IDEOGRAPH
    {0xDE4E, 0x6EC6}, //13428 #CJK UNIFIED IDEOGRAPH
    {0xDE4F, 0x6ED2}, //13429 #CJK UNIFIED IDEOGRAPH
    {0xDE50, 0x6EBD}, //13430 #CJK UNIFIED IDEOGRAPH
    {0xDE51, 0x6EC1}, //13431 #CJK UNIFIED IDEOGRAPH
    {0xDE52, 0x6E9E}, //13432 #CJK UNIFIED IDEOGRAPH
    {0xDE53, 0x6EC9}, //13433 #CJK UNIFIED IDEOGRAPH
    {0xDE54, 0x6EB7}, //13434 #CJK UNIFIED IDEOGRAPH
    {0xDE55, 0x6EB0}, //13435 #CJK UNIFIED IDEOGRAPH
    {0xDE56, 0x6ECD}, //13436 #CJK UNIFIED IDEOGRAPH
    {0xDE57, 0x6EA6}, //13437 #CJK UNIFIED IDEOGRAPH
    {0xDE58, 0x6ECF}, //13438 #CJK UNIFIED IDEOGRAPH
    {0xDE59, 0x6EB2}, //13439 #CJK UNIFIED IDEOGRAPH
    {0xDE5A, 0x6EBE}, //13440 #CJK UNIFIED IDEOGRAPH
    {0xDE5B, 0x6EC3}, //13441 #CJK UNIFIED IDEOGRAPH
    {0xDE5C, 0x6EDC}, //13442 #CJK UNIFIED IDEOGRAPH
    {0xDE5D, 0x6ED8}, //13443 #CJK UNIFIED IDEOGRAPH
    {0xDE5E, 0x6E99}, //13444 #CJK UNIFIED IDEOGRAPH
    {0xDE5F, 0x6E92}, //13445 #CJK UNIFIED IDEOGRAPH
    {0xDE60, 0x6E8E}, //13446 #CJK UNIFIED IDEOGRAPH
    {0xDE61, 0x6E8D}, //13447 #CJK UNIFIED IDEOGRAPH
    {0xDE62, 0x6EA4}, //13448 #CJK UNIFIED IDEOGRAPH
    {0xDE63, 0x6EA1}, //13449 #CJK UNIFIED IDEOGRAPH
    {0xDE64, 0x6EBF}, //13450 #CJK UNIFIED IDEOGRAPH
    {0xDE65, 0x6EB3}, //13451 #CJK UNIFIED IDEOGRAPH
    {0xDE66, 0x6ED0}, //13452 #CJK UNIFIED IDEOGRAPH
    {0xDE67, 0x6ECA}, //13453 #CJK UNIFIED IDEOGRAPH
    {0xDE68, 0x6E97}, //13454 #CJK UNIFIED IDEOGRAPH
    {0xDE69, 0x6EAE}, //13455 #CJK UNIFIED IDEOGRAPH
    {0xDE6A, 0x6EA3}, //13456 #CJK UNIFIED IDEOGRAPH
    {0xDE6B, 0x7147}, //13457 #CJK UNIFIED IDEOGRAPH
    {0xDE6C, 0x7154}, //13458 #CJK UNIFIED IDEOGRAPH
    {0xDE6D, 0x7152}, //13459 #CJK UNIFIED IDEOGRAPH
    {0xDE6E, 0x7163}, //13460 #CJK UNIFIED IDEOGRAPH
    {0xDE6F, 0x7160}, //13461 #CJK UNIFIED IDEOGRAPH
    {0xDE70, 0x7141}, //13462 #CJK UNIFIED IDEOGRAPH
    {0xDE71, 0x715D}, //13463 #CJK UNIFIED IDEOGRAPH
    {0xDE72, 0x7162}, //13464 #CJK UNIFIED IDEOGRAPH
    {0xDE73, 0x7172}, //13465 #CJK UNIFIED IDEOGRAPH
    {0xDE74, 0x7178}, //13466 #CJK UNIFIED IDEOGRAPH
    {0xDE75, 0x716A}, //13467 #CJK UNIFIED IDEOGRAPH
    {0xDE76, 0x7161}, //13468 #CJK UNIFIED IDEOGRAPH
    {0xDE77, 0x7142}, //13469 #CJK UNIFIED IDEOGRAPH
    {0xDE78, 0x7158}, //13470 #CJK UNIFIED IDEOGRAPH
    {0xDE79, 0x7143}, //13471 #CJK UNIFIED IDEOGRAPH
    {0xDE7A, 0x714B}, //13472 #CJK UNIFIED IDEOGRAPH
    {0xDE7B, 0x7170}, //13473 #CJK UNIFIED IDEOGRAPH
    {0xDE7C, 0x715F}, //13474 #CJK UNIFIED IDEOGRAPH
    {0xDE7D, 0x7150}, //13475 #CJK UNIFIED IDEOGRAPH
    {0xDE7E, 0x7153}, //13476 #CJK UNIFIED IDEOGRAPH
    {0xDEA1, 0x7144}, //13477 #CJK UNIFIED IDEOGRAPH
    {0xDEA2, 0x714D}, //13478 #CJK UNIFIED IDEOGRAPH
    {0xDEA3, 0x715A}, //13479 #CJK UNIFIED IDEOGRAPH
    {0xDEA4, 0x724F}, //13480 #CJK UNIFIED IDEOGRAPH
    {0xDEA5, 0x728D}, //13481 #CJK UNIFIED IDEOGRAPH
    {0xDEA6, 0x728C}, //13482 #CJK UNIFIED IDEOGRAPH
    {0xDEA7, 0x7291}, //13483 #CJK UNIFIED IDEOGRAPH
    {0xDEA8, 0x7290}, //13484 #CJK UNIFIED IDEOGRAPH
    {0xDEA9, 0x728E}, //13485 #CJK UNIFIED IDEOGRAPH
    {0xDEAA, 0x733C}, //13486 #CJK UNIFIED IDEOGRAPH
    {0xDEAB, 0x7342}, //13487 #CJK UNIFIED IDEOGRAPH
    {0xDEAC, 0x733B}, //13488 #CJK UNIFIED IDEOGRAPH
    {0xDEAD, 0x733A}, //13489 #CJK UNIFIED IDEOGRAPH
    {0xDEAE, 0x7340}, //13490 #CJK UNIFIED IDEOGRAPH
    {0xDEAF, 0x734A}, //13491 #CJK UNIFIED IDEOGRAPH
    {0xDEB0, 0x7349}, //13492 #CJK UNIFIED IDEOGRAPH
    {0xDEB1, 0x7444}, //13493 #CJK UNIFIED IDEOGRAPH
    {0xDEB2, 0x744A}, //13494 #CJK UNIFIED IDEOGRAPH
    {0xDEB3, 0x744B}, //13495 #CJK UNIFIED IDEOGRAPH
    {0xDEB4, 0x7452}, //13496 #CJK UNIFIED IDEOGRAPH
    {0xDEB5, 0x7451}, //13497 #CJK UNIFIED IDEOGRAPH
    {0xDEB6, 0x7457}, //13498 #CJK UNIFIED IDEOGRAPH
    {0xDEB7, 0x7440}, //13499 #CJK UNIFIED IDEOGRAPH
    {0xDEB8, 0x744F}, //13500 #CJK UNIFIED IDEOGRAPH
    {0xDEB9, 0x7450}, //13501 #CJK UNIFIED IDEOGRAPH
    {0xDEBA, 0x744E}, //13502 #CJK UNIFIED IDEOGRAPH
    {0xDEBB, 0x7442}, //13503 #CJK UNIFIED IDEOGRAPH
    {0xDEBC, 0x7446}, //13504 #CJK UNIFIED IDEOGRAPH
    {0xDEBD, 0x744D}, //13505 #CJK UNIFIED IDEOGRAPH
    {0xDEBE, 0x7454}, //13506 #CJK UNIFIED IDEOGRAPH
    {0xDEBF, 0x74E1}, //13507 #CJK UNIFIED IDEOGRAPH
    {0xDEC0, 0x74FF}, //13508 #CJK UNIFIED IDEOGRAPH
    {0xDEC1, 0x74FE}, //13509 #CJK UNIFIED IDEOGRAPH
    {0xDEC2, 0x74FD}, //13510 #CJK UNIFIED IDEOGRAPH
    {0xDEC3, 0x751D}, //13511 #CJK UNIFIED IDEOGRAPH
    {0xDEC4, 0x7579}, //13512 #CJK UNIFIED IDEOGRAPH
    {0xDEC5, 0x7577}, //13513 #CJK UNIFIED IDEOGRAPH
    {0xDEC6, 0x6983}, //13514 #CJK UNIFIED IDEOGRAPH
    {0xDEC7, 0x75EF}, //13515 #CJK UNIFIED IDEOGRAPH
    {0xDEC8, 0x760F}, //13516 #CJK UNIFIED IDEOGRAPH
    {0xDEC9, 0x7603}, //13517 #CJK UNIFIED IDEOGRAPH
    {0xDECA, 0x75F7}, //13518 #CJK UNIFIED IDEOGRAPH
    {0xDECB, 0x75FE}, //13519 #CJK UNIFIED IDEOGRAPH
    {0xDECC, 0x75FC}, //13520 #CJK UNIFIED IDEOGRAPH
    {0xDECD, 0x75F9}, //13521 #CJK UNIFIED IDEOGRAPH
    {0xDECE, 0x75F8}, //13522 #CJK UNIFIED IDEOGRAPH
    {0xDECF, 0x7610}, //13523 #CJK UNIFIED IDEOGRAPH
    {0xDED0, 0x75FB}, //13524 #CJK UNIFIED IDEOGRAPH
    {0xDED1, 0x75F6}, //13525 #CJK UNIFIED IDEOGRAPH
    {0xDED2, 0x75ED}, //13526 #CJK UNIFIED IDEOGRAPH
    {0xDED3, 0x75F5}, //13527 #CJK UNIFIED IDEOGRAPH
    {0xDED4, 0x75FD}, //13528 #CJK UNIFIED IDEOGRAPH
    {0xDED5, 0x7699}, //13529 #CJK UNIFIED IDEOGRAPH
    {0xDED6, 0x76B5}, //13530 #CJK UNIFIED IDEOGRAPH
    {0xDED7, 0x76DD}, //13531 #CJK UNIFIED IDEOGRAPH
    {0xDED8, 0x7755}, //13532 #CJK UNIFIED IDEOGRAPH
    {0xDED9, 0x775F}, //13533 #CJK UNIFIED IDEOGRAPH
    {0xDEDA, 0x7760}, //13534 #CJK UNIFIED IDEOGRAPH
    {0xDEDB, 0x7752}, //13535 #CJK UNIFIED IDEOGRAPH
    {0xDEDC, 0x7756}, //13536 #CJK UNIFIED IDEOGRAPH
    {0xDEDD, 0x775A}, //13537 #CJK UNIFIED IDEOGRAPH
    {0xDEDE, 0x7769}, //13538 #CJK UNIFIED IDEOGRAPH
    {0xDEDF, 0x7767}, //13539 #CJK UNIFIED IDEOGRAPH
    {0xDEE0, 0x7754}, //13540 #CJK UNIFIED IDEOGRAPH
    {0xDEE1, 0x7759}, //13541 #CJK UNIFIED IDEOGRAPH
    {0xDEE2, 0x776D}, //13542 #CJK UNIFIED IDEOGRAPH
    {0xDEE3, 0x77E0}, //13543 #CJK UNIFIED IDEOGRAPH
    {0xDEE4, 0x7887}, //13544 #CJK UNIFIED IDEOGRAPH
    {0xDEE5, 0x789A}, //13545 #CJK UNIFIED IDEOGRAPH
    {0xDEE6, 0x7894}, //13546 #CJK UNIFIED IDEOGRAPH
    {0xDEE7, 0x788F}, //13547 #CJK UNIFIED IDEOGRAPH
    {0xDEE8, 0x7884}, //13548 #CJK UNIFIED IDEOGRAPH
    {0xDEE9, 0x7895}, //13549 #CJK UNIFIED IDEOGRAPH
    {0xDEEA, 0x7885}, //13550 #CJK UNIFIED IDEOGRAPH
    {0xDEEB, 0x7886}, //13551 #CJK UNIFIED IDEOGRAPH
    {0xDEEC, 0x78A1}, //13552 #CJK UNIFIED IDEOGRAPH
    {0xDEED, 0x7883}, //13553 #CJK UNIFIED IDEOGRAPH
    {0xDEEE, 0x7879}, //13554 #CJK UNIFIED IDEOGRAPH
    {0xDEEF, 0x7899}, //13555 #CJK UNIFIED IDEOGRAPH
    {0xDEF0, 0x7880}, //13556 #CJK UNIFIED IDEOGRAPH
    {0xDEF1, 0x7896}, //13557 #CJK UNIFIED IDEOGRAPH
    {0xDEF2, 0x787B}, //13558 #CJK UNIFIED IDEOGRAPH
    {0xDEF3, 0x797C}, //13559 #CJK UNIFIED IDEOGRAPH
    {0xDEF4, 0x7982}, //13560 #CJK UNIFIED IDEOGRAPH
    {0xDEF5, 0x797D}, //13561 #CJK UNIFIED IDEOGRAPH
    {0xDEF6, 0x7979}, //13562 #CJK UNIFIED IDEOGRAPH
    {0xDEF7, 0x7A11}, //13563 #CJK UNIFIED IDEOGRAPH
    {0xDEF8, 0x7A18}, //13564 #CJK UNIFIED IDEOGRAPH
    {0xDEF9, 0x7A19}, //13565 #CJK UNIFIED IDEOGRAPH
    {0xDEFA, 0x7A12}, //13566 #CJK UNIFIED IDEOGRAPH
    {0xDEFB, 0x7A17}, //13567 #CJK UNIFIED IDEOGRAPH
    {0xDEFC, 0x7A15}, //13568 #CJK UNIFIED IDEOGRAPH
    {0xDEFD, 0x7A22}, //13569 #CJK UNIFIED IDEOGRAPH
    {0xDEFE, 0x7A13}, //13570 #CJK UNIFIED IDEOGRAPH
    {0xDF40, 0x7A1B}, //13571 #CJK UNIFIED IDEOGRAPH
    {0xDF41, 0x7A10}, //13572 #CJK UNIFIED IDEOGRAPH
    {0xDF42, 0x7AA3}, //13573 #CJK UNIFIED IDEOGRAPH
    {0xDF43, 0x7AA2}, //13574 #CJK UNIFIED IDEOGRAPH
    {0xDF44, 0x7A9E}, //13575 #CJK UNIFIED IDEOGRAPH
    {0xDF45, 0x7AEB}, //13576 #CJK UNIFIED IDEOGRAPH
    {0xDF46, 0x7B66}, //13577 #CJK UNIFIED IDEOGRAPH
    {0xDF47, 0x7B64}, //13578 #CJK UNIFIED IDEOGRAPH
    {0xDF48, 0x7B6D}, //13579 #CJK UNIFIED IDEOGRAPH
    {0xDF49, 0x7B74}, //13580 #CJK UNIFIED IDEOGRAPH
    {0xDF4A, 0x7B69}, //13581 #CJK UNIFIED IDEOGRAPH
    {0xDF4B, 0x7B72}, //13582 #CJK UNIFIED IDEOGRAPH
    {0xDF4C, 0x7B65}, //13583 #CJK UNIFIED IDEOGRAPH
    {0xDF4D, 0x7B73}, //13584 #CJK UNIFIED IDEOGRAPH
    {0xDF4E, 0x7B71}, //13585 #CJK UNIFIED IDEOGRAPH
    {0xDF4F, 0x7B70}, //13586 #CJK UNIFIED IDEOGRAPH
    {0xDF50, 0x7B61}, //13587 #CJK UNIFIED IDEOGRAPH
    {0xDF51, 0x7B78}, //13588 #CJK UNIFIED IDEOGRAPH
    {0xDF52, 0x7B76}, //13589 #CJK UNIFIED IDEOGRAPH
    {0xDF53, 0x7B63}, //13590 #CJK UNIFIED IDEOGRAPH
    {0xDF54, 0x7CB2}, //13591 #CJK UNIFIED IDEOGRAPH
    {0xDF55, 0x7CB4}, //13592 #CJK UNIFIED IDEOGRAPH
    {0xDF56, 0x7CAF}, //13593 #CJK UNIFIED IDEOGRAPH
    {0xDF57, 0x7D88}, //13594 #CJK UNIFIED IDEOGRAPH
    {0xDF58, 0x7D86}, //13595 #CJK UNIFIED IDEOGRAPH
    {0xDF59, 0x7D80}, //13596 #CJK UNIFIED IDEOGRAPH
    {0xDF5A, 0x7D8D}, //13597 #CJK UNIFIED IDEOGRAPH
    {0xDF5B, 0x7D7F}, //13598 #CJK UNIFIED IDEOGRAPH
    {0xDF5C, 0x7D85}, //13599 #CJK UNIFIED IDEOGRAPH
    {0xDF5D, 0x7D7A}, //13600 #CJK UNIFIED IDEOGRAPH
    {0xDF5E, 0x7D8E}, //13601 #CJK UNIFIED IDEOGRAPH
    {0xDF5F, 0x7D7B}, //13602 #CJK UNIFIED IDEOGRAPH
    {0xDF60, 0x7D83}, //13603 #CJK UNIFIED IDEOGRAPH
    {0xDF61, 0x7D7C}, //13604 #CJK UNIFIED IDEOGRAPH
    {0xDF62, 0x7D8C}, //13605 #CJK UNIFIED IDEOGRAPH
    {0xDF63, 0x7D94}, //13606 #CJK UNIFIED IDEOGRAPH
    {0xDF64, 0x7D84}, //13607 #CJK UNIFIED IDEOGRAPH
    {0xDF65, 0x7D7D}, //13608 #CJK UNIFIED IDEOGRAPH
    {0xDF66, 0x7D92}, //13609 #CJK UNIFIED IDEOGRAPH
    {0xDF67, 0x7F6D}, //13610 #CJK UNIFIED IDEOGRAPH
    {0xDF68, 0x7F6B}, //13611 #CJK UNIFIED IDEOGRAPH
    {0xDF69, 0x7F67}, //13612 #CJK UNIFIED IDEOGRAPH
    {0xDF6A, 0x7F68}, //13613 #CJK UNIFIED IDEOGRAPH
    {0xDF6B, 0x7F6C}, //13614 #CJK UNIFIED IDEOGRAPH
    {0xDF6C, 0x7FA6}, //13615 #CJK UNIFIED IDEOGRAPH
    {0xDF6D, 0x7FA5}, //13616 #CJK UNIFIED IDEOGRAPH
    {0xDF6E, 0x7FA7}, //13617 #CJK UNIFIED IDEOGRAPH
    {0xDF6F, 0x7FDB}, //13618 #CJK UNIFIED IDEOGRAPH
    {0xDF70, 0x7FDC}, //13619 #CJK UNIFIED IDEOGRAPH
    {0xDF71, 0x8021}, //13620 #CJK UNIFIED IDEOGRAPH
    {0xDF72, 0x8164}, //13621 #CJK UNIFIED IDEOGRAPH
    {0xDF73, 0x8160}, //13622 #CJK UNIFIED IDEOGRAPH
    {0xDF74, 0x8177}, //13623 #CJK UNIFIED IDEOGRAPH
    {0xDF75, 0x815C}, //13624 #CJK UNIFIED IDEOGRAPH
    {0xDF76, 0x8169}, //13625 #CJK UNIFIED IDEOGRAPH
    {0xDF77, 0x815B}, //13626 #CJK UNIFIED IDEOGRAPH
    {0xDF78, 0x8162}, //13627 #CJK UNIFIED IDEOGRAPH
    {0xDF79, 0x8172}, //13628 #CJK UNIFIED IDEOGRAPH
    {0xDF7A, 0x6721}, //13629 #CJK UNIFIED IDEOGRAPH
    {0xDF7B, 0x815E}, //13630 #CJK UNIFIED IDEOGRAPH
    {0xDF7C, 0x8176}, //13631 #CJK UNIFIED IDEOGRAPH
    {0xDF7D, 0x8167}, //13632 #CJK UNIFIED IDEOGRAPH
    {0xDF7E, 0x816F}, //13633 #CJK UNIFIED IDEOGRAPH
    {0xDFA1, 0x8144}, //13634 #CJK UNIFIED IDEOGRAPH
    {0xDFA2, 0x8161}, //13635 #CJK UNIFIED IDEOGRAPH
    {0xDFA3, 0x821D}, //13636 #CJK UNIFIED IDEOGRAPH
    {0xDFA4, 0x8249}, //13637 #CJK UNIFIED IDEOGRAPH
    {0xDFA5, 0x8244}, //13638 #CJK UNIFIED IDEOGRAPH
    {0xDFA6, 0x8240}, //13639 #CJK UNIFIED IDEOGRAPH
    {0xDFA7, 0x8242}, //13640 #CJK UNIFIED IDEOGRAPH
    {0xDFA8, 0x8245}, //13641 #CJK UNIFIED IDEOGRAPH
    {0xDFA9, 0x84F1}, //13642 #CJK UNIFIED IDEOGRAPH
    {0xDFAA, 0x843F}, //13643 #CJK UNIFIED IDEOGRAPH
    {0xDFAB, 0x8456}, //13644 #CJK UNIFIED IDEOGRAPH
    {0xDFAC, 0x8476}, //13645 #CJK UNIFIED IDEOGRAPH
    {0xDFAD, 0x8479}, //13646 #CJK UNIFIED IDEOGRAPH
    {0xDFAE, 0x848F}, //13647 #CJK UNIFIED IDEOGRAPH
    {0xDFAF, 0x848D}, //13648 #CJK UNIFIED IDEOGRAPH
    {0xDFB0, 0x8465}, //13649 #CJK UNIFIED IDEOGRAPH
    {0xDFB1, 0x8451}, //13650 #CJK UNIFIED IDEOGRAPH
    {0xDFB2, 0x8440}, //13651 #CJK UNIFIED IDEOGRAPH
    {0xDFB3, 0x8486}, //13652 #CJK UNIFIED IDEOGRAPH
    {0xDFB4, 0x8467}, //13653 #CJK UNIFIED IDEOGRAPH
    {0xDFB5, 0x8430}, //13654 #CJK UNIFIED IDEOGRAPH
    {0xDFB6, 0x844D}, //13655 #CJK UNIFIED IDEOGRAPH
    {0xDFB7, 0x847D}, //13656 #CJK UNIFIED IDEOGRAPH
    {0xDFB8, 0x845A}, //13657 #CJK UNIFIED IDEOGRAPH
    {0xDFB9, 0x8459}, //13658 #CJK UNIFIED IDEOGRAPH
    {0xDFBA, 0x8474}, //13659 #CJK UNIFIED IDEOGRAPH
    {0xDFBB, 0x8473}, //13660 #CJK UNIFIED IDEOGRAPH
    {0xDFBC, 0x845D}, //13661 #CJK UNIFIED IDEOGRAPH
    {0xDFBD, 0x8507}, //13662 #CJK UNIFIED IDEOGRAPH
    {0xDFBE, 0x845E}, //13663 #CJK UNIFIED IDEOGRAPH
    {0xDFBF, 0x8437}, //13664 #CJK UNIFIED IDEOGRAPH
    {0xDFC0, 0x843A}, //13665 #CJK UNIFIED IDEOGRAPH
    {0xDFC1, 0x8434}, //13666 #CJK UNIFIED IDEOGRAPH
    {0xDFC2, 0x847A}, //13667 #CJK UNIFIED IDEOGRAPH
    {0xDFC3, 0x8443}, //13668 #CJK UNIFIED IDEOGRAPH
    {0xDFC4, 0x8478}, //13669 #CJK UNIFIED IDEOGRAPH
    {0xDFC5, 0x8432}, //13670 #CJK UNIFIED IDEOGRAPH
    {0xDFC6, 0x8445}, //13671 #CJK UNIFIED IDEOGRAPH
    {0xDFC7, 0x8429}, //13672 #CJK UNIFIED IDEOGRAPH
    {0xDFC8, 0x83D9}, //13673 #CJK UNIFIED IDEOGRAPH
    {0xDFC9, 0x844B}, //13674 #CJK UNIFIED IDEOGRAPH
    {0xDFCA, 0x842F}, //13675 #CJK UNIFIED IDEOGRAPH
    {0xDFCB, 0x8442}, //13676 #CJK UNIFIED IDEOGRAPH
    {0xDFCC, 0x842D}, //13677 #CJK UNIFIED IDEOGRAPH
    {0xDFCD, 0x845F}, //13678 #CJK UNIFIED IDEOGRAPH
    {0xDFCE, 0x8470}, //13679 #CJK UNIFIED IDEOGRAPH
    {0xDFCF, 0x8439}, //13680 #CJK UNIFIED IDEOGRAPH
    {0xDFD0, 0x844E}, //13681 #CJK UNIFIED IDEOGRAPH
    {0xDFD1, 0x844C}, //13682 #CJK UNIFIED IDEOGRAPH
    {0xDFD2, 0x8452}, //13683 #CJK UNIFIED IDEOGRAPH
    {0xDFD3, 0x846F}, //13684 #CJK UNIFIED IDEOGRAPH
    {0xDFD4, 0x84C5}, //13685 #CJK UNIFIED IDEOGRAPH
    {0xDFD5, 0x848E}, //13686 #CJK UNIFIED IDEOGRAPH
    {0xDFD6, 0x843B}, //13687 #CJK UNIFIED IDEOGRAPH
    {0xDFD7, 0x8447}, //13688 #CJK UNIFIED IDEOGRAPH
    {0xDFD8, 0x8436}, //13689 #CJK UNIFIED IDEOGRAPH
    {0xDFD9, 0x8433}, //13690 #CJK UNIFIED IDEOGRAPH
    {0xDFDA, 0x8468}, //13691 #CJK UNIFIED IDEOGRAPH
    {0xDFDB, 0x847E}, //13692 #CJK UNIFIED IDEOGRAPH
    {0xDFDC, 0x8444}, //13693 #CJK UNIFIED IDEOGRAPH
    {0xDFDD, 0x842B}, //13694 #CJK UNIFIED IDEOGRAPH
    {0xDFDE, 0x8460}, //13695 #CJK UNIFIED IDEOGRAPH
    {0xDFDF, 0x8454}, //13696 #CJK UNIFIED IDEOGRAPH
    {0xDFE0, 0x846E}, //13697 #CJK UNIFIED IDEOGRAPH
    {0xDFE1, 0x8450}, //13698 #CJK UNIFIED IDEOGRAPH
    {0xDFE2, 0x870B}, //13699 #CJK UNIFIED IDEOGRAPH
    {0xDFE3, 0x8704}, //13700 #CJK UNIFIED IDEOGRAPH
    {0xDFE4, 0x86F7}, //13701 #CJK UNIFIED IDEOGRAPH
    {0xDFE5, 0x870C}, //13702 #CJK UNIFIED IDEOGRAPH
    {0xDFE6, 0x86FA}, //13703 #CJK UNIFIED IDEOGRAPH
    {0xDFE7, 0x86D6}, //13704 #CJK UNIFIED IDEOGRAPH
    {0xDFE8, 0x86F5}, //13705 #CJK UNIFIED IDEOGRAPH
    {0xDFE9, 0x874D}, //13706 #CJK UNIFIED IDEOGRAPH
    {0xDFEA, 0x86F8}, //13707 #CJK UNIFIED IDEOGRAPH
    {0xDFEB, 0x870E}, //13708 #CJK UNIFIED IDEOGRAPH
    {0xDFEC, 0x8709}, //13709 #CJK UNIFIED IDEOGRAPH
    {0xDFED, 0x8701}, //13710 #CJK UNIFIED IDEOGRAPH
    {0xDFEE, 0x86F6}, //13711 #CJK UNIFIED IDEOGRAPH
    {0xDFEF, 0x870D}, //13712 #CJK UNIFIED IDEOGRAPH
    {0xDFF0, 0x8705}, //13713 #CJK UNIFIED IDEOGRAPH
    {0xDFF1, 0x88D6}, //13714 #CJK UNIFIED IDEOGRAPH
    {0xDFF2, 0x88CB}, //13715 #CJK UNIFIED IDEOGRAPH
    {0xDFF3, 0x88CD}, //13716 #CJK UNIFIED IDEOGRAPH
    {0xDFF4, 0x88CE}, //13717 #CJK UNIFIED IDEOGRAPH
    {0xDFF5, 0x88DE}, //13718 #CJK UNIFIED IDEOGRAPH
    {0xDFF6, 0x88DB}, //13719 #CJK UNIFIED IDEOGRAPH
    {0xDFF7, 0x88DA}, //13720 #CJK UNIFIED IDEOGRAPH
    {0xDFF8, 0x88CC}, //13721 #CJK UNIFIED IDEOGRAPH
    {0xDFF9, 0x88D0}, //13722 #CJK UNIFIED IDEOGRAPH
    {0xDFFA, 0x8985}, //13723 #CJK UNIFIED IDEOGRAPH
    {0xDFFB, 0x899B}, //13724 #CJK UNIFIED IDEOGRAPH
    {0xDFFC, 0x89DF}, //13725 #CJK UNIFIED IDEOGRAPH
    {0xDFFD, 0x89E5}, //13726 #CJK UNIFIED IDEOGRAPH
    {0xDFFE, 0x89E4}, //13727 #CJK UNIFIED IDEOGRAPH
    {0xE040, 0x89E1}, //13728 #CJK UNIFIED IDEOGRAPH
    {0xE041, 0x89E0}, //13729 #CJK UNIFIED IDEOGRAPH
    {0xE042, 0x89E2}, //13730 #CJK UNIFIED IDEOGRAPH
    {0xE043, 0x89DC}, //13731 #CJK UNIFIED IDEOGRAPH
    {0xE044, 0x89E6}, //13732 #CJK UNIFIED IDEOGRAPH
    {0xE045, 0x8A76}, //13733 #CJK UNIFIED IDEOGRAPH
    {0xE046, 0x8A86}, //13734 #CJK UNIFIED IDEOGRAPH
    {0xE047, 0x8A7F}, //13735 #CJK UNIFIED IDEOGRAPH
    {0xE048, 0x8A61}, //13736 #CJK UNIFIED IDEOGRAPH
    {0xE049, 0x8A3F}, //13737 #CJK UNIFIED IDEOGRAPH
    {0xE04A, 0x8A77}, //13738 #CJK UNIFIED IDEOGRAPH
    {0xE04B, 0x8A82}, //13739 #CJK UNIFIED IDEOGRAPH
    {0xE04C, 0x8A84}, //13740 #CJK UNIFIED IDEOGRAPH
    {0xE04D, 0x8A75}, //13741 #CJK UNIFIED IDEOGRAPH
    {0xE04E, 0x8A83}, //13742 #CJK UNIFIED IDEOGRAPH
    {0xE04F, 0x8A81}, //13743 #CJK UNIFIED IDEOGRAPH
    {0xE050, 0x8A74}, //13744 #CJK UNIFIED IDEOGRAPH
    {0xE051, 0x8A7A}, //13745 #CJK UNIFIED IDEOGRAPH
    {0xE052, 0x8C3C}, //13746 #CJK UNIFIED IDEOGRAPH
    {0xE053, 0x8C4B}, //13747 #CJK UNIFIED IDEOGRAPH
    {0xE054, 0x8C4A}, //13748 #CJK UNIFIED IDEOGRAPH
    {0xE055, 0x8C65}, //13749 #CJK UNIFIED IDEOGRAPH
    {0xE056, 0x8C64}, //13750 #CJK UNIFIED IDEOGRAPH
    {0xE057, 0x8C66}, //13751 #CJK UNIFIED IDEOGRAPH
    {0xE058, 0x8C86}, //13752 #CJK UNIFIED IDEOGRAPH
    {0xE059, 0x8C84}, //13753 #CJK UNIFIED IDEOGRAPH
    {0xE05A, 0x8C85}, //13754 #CJK UNIFIED IDEOGRAPH
    {0xE05B, 0x8CCC}, //13755 #CJK UNIFIED IDEOGRAPH
    {0xE05C, 0x8D68}, //13756 #CJK UNIFIED IDEOGRAPH
    {0xE05D, 0x8D69}, //13757 #CJK UNIFIED IDEOGRAPH
    {0xE05E, 0x8D91}, //13758 #CJK UNIFIED IDEOGRAPH
    {0xE05F, 0x8D8C}, //13759 #CJK UNIFIED IDEOGRAPH
    {0xE060, 0x8D8E}, //13760 #CJK UNIFIED IDEOGRAPH
    {0xE061, 0x8D8F}, //13761 #CJK UNIFIED IDEOGRAPH
    {0xE062, 0x8D8D}, //13762 #CJK UNIFIED IDEOGRAPH
    {0xE063, 0x8D93}, //13763 #CJK UNIFIED IDEOGRAPH
    {0xE064, 0x8D94}, //13764 #CJK UNIFIED IDEOGRAPH
    {0xE065, 0x8D90}, //13765 #CJK UNIFIED IDEOGRAPH
    {0xE066, 0x8D92}, //13766 #CJK UNIFIED IDEOGRAPH
    {0xE067, 0x8DF0}, //13767 #CJK UNIFIED IDEOGRAPH
    {0xE068, 0x8DE0}, //13768 #CJK UNIFIED IDEOGRAPH
    {0xE069, 0x8DEC}, //13769 #CJK UNIFIED IDEOGRAPH
    {0xE06A, 0x8DF1}, //13770 #CJK UNIFIED IDEOGRAPH
    {0xE06B, 0x8DEE}, //13771 #CJK UNIFIED IDEOGRAPH
    {0xE06C, 0x8DD0}, //13772 #CJK UNIFIED IDEOGRAPH
    {0xE06D, 0x8DE9}, //13773 #CJK UNIFIED IDEOGRAPH
    {0xE06E, 0x8DE3}, //13774 #CJK UNIFIED IDEOGRAPH
    {0xE06F, 0x8DE2}, //13775 #CJK UNIFIED IDEOGRAPH
    {0xE070, 0x8DE7}, //13776 #CJK UNIFIED IDEOGRAPH
    {0xE071, 0x8DF2}, //13777 #CJK UNIFIED IDEOGRAPH
    {0xE072, 0x8DEB}, //13778 #CJK UNIFIED IDEOGRAPH
    {0xE073, 0x8DF4}, //13779 #CJK UNIFIED IDEOGRAPH
    {0xE074, 0x8F06}, //13780 #CJK UNIFIED IDEOGRAPH
    {0xE075, 0x8EFF}, //13781 #CJK UNIFIED IDEOGRAPH
    {0xE076, 0x8F01}, //13782 #CJK UNIFIED IDEOGRAPH
    {0xE077, 0x8F00}, //13783 #CJK UNIFIED IDEOGRAPH
    {0xE078, 0x8F05}, //13784 #CJK UNIFIED IDEOGRAPH
    {0xE079, 0x8F07}, //13785 #CJK UNIFIED IDEOGRAPH
    {0xE07A, 0x8F08}, //13786 #CJK UNIFIED IDEOGRAPH
    {0xE07B, 0x8F02}, //13787 #CJK UNIFIED IDEOGRAPH
    {0xE07C, 0x8F0B}, //13788 #CJK UNIFIED IDEOGRAPH
    {0xE07D, 0x9052}, //13789 #CJK UNIFIED IDEOGRAPH
    {0xE07E, 0x903F}, //13790 #CJK UNIFIED IDEOGRAPH
    {0xE0A1, 0x9044}, //13791 #CJK UNIFIED IDEOGRAPH
    {0xE0A2, 0x9049}, //13792 #CJK UNIFIED IDEOGRAPH
    {0xE0A3, 0x903D}, //13793 #CJK UNIFIED IDEOGRAPH
    {0xE0A4, 0x9110}, //13794 #CJK UNIFIED IDEOGRAPH
    {0xE0A5, 0x910D}, //13795 #CJK UNIFIED IDEOGRAPH
    {0xE0A6, 0x910F}, //13796 #CJK UNIFIED IDEOGRAPH
    {0xE0A7, 0x9111}, //13797 #CJK UNIFIED IDEOGRAPH
    {0xE0A8, 0x9116}, //13798 #CJK UNIFIED IDEOGRAPH
    {0xE0A9, 0x9114}, //13799 #CJK UNIFIED IDEOGRAPH
    {0xE0AA, 0x910B}, //13800 #CJK UNIFIED IDEOGRAPH
    {0xE0AB, 0x910E}, //13801 #CJK UNIFIED IDEOGRAPH
    {0xE0AC, 0x916E}, //13802 #CJK UNIFIED IDEOGRAPH
    {0xE0AD, 0x916F}, //13803 #CJK UNIFIED IDEOGRAPH
    {0xE0AE, 0x9248}, //13804 #CJK UNIFIED IDEOGRAPH
    {0xE0AF, 0x9252}, //13805 #CJK UNIFIED IDEOGRAPH
    {0xE0B0, 0x9230}, //13806 #CJK UNIFIED IDEOGRAPH
    {0xE0B1, 0x923A}, //13807 #CJK UNIFIED IDEOGRAPH
    {0xE0B2, 0x9266}, //13808 #CJK UNIFIED IDEOGRAPH
    {0xE0B3, 0x9233}, //13809 #CJK UNIFIED IDEOGRAPH
    {0xE0B4, 0x9265}, //13810 #CJK UNIFIED IDEOGRAPH
    {0xE0B5, 0x925E}, //13811 #CJK UNIFIED IDEOGRAPH
    {0xE0B6, 0x9283}, //13812 #CJK UNIFIED IDEOGRAPH
    {0xE0B7, 0x922E}, //13813 #CJK UNIFIED IDEOGRAPH
    {0xE0B8, 0x924A}, //13814 #CJK UNIFIED IDEOGRAPH
    {0xE0B9, 0x9246}, //13815 #CJK UNIFIED IDEOGRAPH
    {0xE0BA, 0x926D}, //13816 #CJK UNIFIED IDEOGRAPH
    {0xE0BB, 0x926C}, //13817 #CJK UNIFIED IDEOGRAPH
    {0xE0BC, 0x924F}, //13818 #CJK UNIFIED IDEOGRAPH
    {0xE0BD, 0x9260}, //13819 #CJK UNIFIED IDEOGRAPH
    {0xE0BE, 0x9267}, //13820 #CJK UNIFIED IDEOGRAPH
    {0xE0BF, 0x926F}, //13821 #CJK UNIFIED IDEOGRAPH
    {0xE0C0, 0x9236}, //13822 #CJK UNIFIED IDEOGRAPH
    {0xE0C1, 0x9261}, //13823 #CJK UNIFIED IDEOGRAPH
    {0xE0C2, 0x9270}, //13824 #CJK UNIFIED IDEOGRAPH
    {0xE0C3, 0x9231}, //13825 #CJK UNIFIED IDEOGRAPH
    {0xE0C4, 0x9254}, //13826 #CJK UNIFIED IDEOGRAPH
    {0xE0C5, 0x9263}, //13827 #CJK UNIFIED IDEOGRAPH
    {0xE0C6, 0x9250}, //13828 #CJK UNIFIED IDEOGRAPH
    {0xE0C7, 0x9272}, //13829 #CJK UNIFIED IDEOGRAPH
    {0xE0C8, 0x924E}, //13830 #CJK UNIFIED IDEOGRAPH
    {0xE0C9, 0x9253}, //13831 #CJK UNIFIED IDEOGRAPH
    {0xE0CA, 0x924C}, //13832 #CJK UNIFIED IDEOGRAPH
    {0xE0CB, 0x9256}, //13833 #CJK UNIFIED IDEOGRAPH
    {0xE0CC, 0x9232}, //13834 #CJK UNIFIED IDEOGRAPH
    {0xE0CD, 0x959F}, //13835 #CJK UNIFIED IDEOGRAPH
    {0xE0CE, 0x959C}, //13836 #CJK UNIFIED IDEOGRAPH
    {0xE0CF, 0x959E}, //13837 #CJK UNIFIED IDEOGRAPH
    {0xE0D0, 0x959B}, //13838 #CJK UNIFIED IDEOGRAPH
    {0xE0D1, 0x9692}, //13839 #CJK UNIFIED IDEOGRAPH
    {0xE0D2, 0x9693}, //13840 #CJK UNIFIED IDEOGRAPH
    {0xE0D3, 0x9691}, //13841 #CJK UNIFIED IDEOGRAPH
    {0xE0D4, 0x9697}, //13842 #CJK UNIFIED IDEOGRAPH
    {0xE0D5, 0x96CE}, //13843 #CJK UNIFIED IDEOGRAPH
    {0xE0D6, 0x96FA}, //13844 #CJK UNIFIED IDEOGRAPH
    {0xE0D7, 0x96FD}, //13845 #CJK UNIFIED IDEOGRAPH
    {0xE0D8, 0x96F8}, //13846 #CJK UNIFIED IDEOGRAPH
    {0xE0D9, 0x96F5}, //13847 #CJK UNIFIED IDEOGRAPH
    {0xE0DA, 0x9773}, //13848 #CJK UNIFIED IDEOGRAPH
    {0xE0DB, 0x9777}, //13849 #CJK UNIFIED IDEOGRAPH
    {0xE0DC, 0x9778}, //13850 #CJK UNIFIED IDEOGRAPH
    {0xE0DD, 0x9772}, //13851 #CJK UNIFIED IDEOGRAPH
    {0xE0DE, 0x980F}, //13852 #CJK UNIFIED IDEOGRAPH
    {0xE0DF, 0x980D}, //13853 #CJK UNIFIED IDEOGRAPH
    {0xE0E0, 0x980E}, //13854 #CJK UNIFIED IDEOGRAPH
    {0xE0E1, 0x98AC}, //13855 #CJK UNIFIED IDEOGRAPH
    {0xE0E2, 0x98F6}, //13856 #CJK UNIFIED IDEOGRAPH
    {0xE0E3, 0x98F9}, //13857 #CJK UNIFIED IDEOGRAPH
    {0xE0E4, 0x99AF}, //13858 #CJK UNIFIED IDEOGRAPH
    {0xE0E5, 0x99B2}, //13859 #CJK UNIFIED IDEOGRAPH
    {0xE0E6, 0x99B0}, //13860 #CJK UNIFIED IDEOGRAPH
    {0xE0E7, 0x99B5}, //13861 #CJK UNIFIED IDEOGRAPH
    {0xE0E8, 0x9AAD}, //13862 #CJK UNIFIED IDEOGRAPH
    {0xE0E9, 0x9AAB}, //13863 #CJK UNIFIED IDEOGRAPH
    {0xE0EA, 0x9B5B}, //13864 #CJK UNIFIED IDEOGRAPH
    {0xE0EB, 0x9CEA}, //13865 #CJK UNIFIED IDEOGRAPH
    {0xE0EC, 0x9CED}, //13866 #CJK UNIFIED IDEOGRAPH
    {0xE0ED, 0x9CE7}, //13867 #CJK UNIFIED IDEOGRAPH
    {0xE0EE, 0x9E80}, //13868 #CJK UNIFIED IDEOGRAPH
    {0xE0EF, 0x9EFD}, //13869 #CJK UNIFIED IDEOGRAPH
    {0xE0F0, 0x50E6}, //13870 #CJK UNIFIED IDEOGRAPH
    {0xE0F1, 0x50D4}, //13871 #CJK UNIFIED IDEOGRAPH
    {0xE0F2, 0x50D7}, //13872 #CJK UNIFIED IDEOGRAPH
    {0xE0F3, 0x50E8}, //13873 #CJK UNIFIED IDEOGRAPH
    {0xE0F4, 0x50F3}, //13874 #CJK UNIFIED IDEOGRAPH
    {0xE0F5, 0x50DB}, //13875 #CJK UNIFIED IDEOGRAPH
    {0xE0F6, 0x50EA}, //13876 #CJK UNIFIED IDEOGRAPH
    {0xE0F7, 0x50DD}, //13877 #CJK UNIFIED IDEOGRAPH
    {0xE0F8, 0x50E4}, //13878 #CJK UNIFIED IDEOGRAPH
    {0xE0F9, 0x50D3}, //13879 #CJK UNIFIED IDEOGRAPH
    {0xE0FA, 0x50EC}, //13880 #CJK UNIFIED IDEOGRAPH
    {0xE0FB, 0x50F0}, //13881 #CJK UNIFIED IDEOGRAPH
    {0xE0FC, 0x50EF}, //13882 #CJK UNIFIED IDEOGRAPH
    {0xE0FD, 0x50E3}, //13883 #CJK UNIFIED IDEOGRAPH
    {0xE0FE, 0x50E0}, //13884 #CJK UNIFIED IDEOGRAPH
    {0xE140, 0x51D8}, //13885 #CJK UNIFIED IDEOGRAPH
    {0xE141, 0x5280}, //13886 #CJK UNIFIED IDEOGRAPH
    {0xE142, 0x5281}, //13887 #CJK UNIFIED IDEOGRAPH
    {0xE143, 0x52E9}, //13888 #CJK UNIFIED IDEOGRAPH
    {0xE144, 0x52EB}, //13889 #CJK UNIFIED IDEOGRAPH
    {0xE145, 0x5330}, //13890 #CJK UNIFIED IDEOGRAPH
    {0xE146, 0x53AC}, //13891 #CJK UNIFIED IDEOGRAPH
    {0xE147, 0x5627}, //13892 #CJK UNIFIED IDEOGRAPH
    {0xE148, 0x5615}, //13893 #CJK UNIFIED IDEOGRAPH
    {0xE149, 0x560C}, //13894 #CJK UNIFIED IDEOGRAPH
    {0xE14A, 0x5612}, //13895 #CJK UNIFIED IDEOGRAPH
    {0xE14B, 0x55FC}, //13896 #CJK UNIFIED IDEOGRAPH
    {0xE14C, 0x560F}, //13897 #CJK UNIFIED IDEOGRAPH
    {0xE14D, 0x561C}, //13898 #CJK UNIFIED IDEOGRAPH
    {0xE14E, 0x5601}, //13899 #CJK UNIFIED IDEOGRAPH
    {0xE14F, 0x5613}, //13900 #CJK UNIFIED IDEOGRAPH
    {0xE150, 0x5602}, //13901 #CJK UNIFIED IDEOGRAPH
    {0xE151, 0x55FA}, //13902 #CJK UNIFIED IDEOGRAPH
    {0xE152, 0x561D}, //13903 #CJK UNIFIED IDEOGRAPH
    {0xE153, 0x5604}, //13904 #CJK UNIFIED IDEOGRAPH
    {0xE154, 0x55FF}, //13905 #CJK UNIFIED IDEOGRAPH
    {0xE155, 0x55F9}, //13906 #CJK UNIFIED IDEOGRAPH
    {0xE156, 0x5889}, //13907 #CJK UNIFIED IDEOGRAPH
    {0xE157, 0x587C}, //13908 #CJK UNIFIED IDEOGRAPH
    {0xE158, 0x5890}, //13909 #CJK UNIFIED IDEOGRAPH
    {0xE159, 0x5898}, //13910 #CJK UNIFIED IDEOGRAPH
    {0xE15A, 0x5886}, //13911 #CJK UNIFIED IDEOGRAPH
    {0xE15B, 0x5881}, //13912 #CJK UNIFIED IDEOGRAPH
    {0xE15C, 0x587F}, //13913 #CJK UNIFIED IDEOGRAPH
    {0xE15D, 0x5874}, //13914 #CJK UNIFIED IDEOGRAPH
    {0xE15E, 0x588B}, //13915 #CJK UNIFIED IDEOGRAPH
    {0xE15F, 0x587A}, //13916 #CJK UNIFIED IDEOGRAPH
    {0xE160, 0x5887}, //13917 #CJK UNIFIED IDEOGRAPH
    {0xE161, 0x5891}, //13918 #CJK UNIFIED IDEOGRAPH
    {0xE162, 0x588E}, //13919 #CJK UNIFIED IDEOGRAPH
    {0xE163, 0x5876}, //13920 #CJK UNIFIED IDEOGRAPH
    {0xE164, 0x5882}, //13921 #CJK UNIFIED IDEOGRAPH
    {0xE165, 0x5888}, //13922 #CJK UNIFIED IDEOGRAPH
    {0xE166, 0x587B}, //13923 #CJK UNIFIED IDEOGRAPH
    {0xE167, 0x5894}, //13924 #CJK UNIFIED IDEOGRAPH
    {0xE168, 0x588F}, //13925 #CJK UNIFIED IDEOGRAPH
    {0xE169, 0x58FE}, //13926 #CJK UNIFIED IDEOGRAPH
    {0xE16A, 0x596B}, //13927 #CJK UNIFIED IDEOGRAPH
    {0xE16B, 0x5ADC}, //13928 #CJK UNIFIED IDEOGRAPH
    {0xE16C, 0x5AEE}, //13929 #CJK UNIFIED IDEOGRAPH
    {0xE16D, 0x5AE5}, //13930 #CJK UNIFIED IDEOGRAPH
    {0xE16E, 0x5AD5}, //13931 #CJK UNIFIED IDEOGRAPH
    {0xE16F, 0x5AEA}, //13932 #CJK UNIFIED IDEOGRAPH
    {0xE170, 0x5ADA}, //13933 #CJK UNIFIED IDEOGRAPH
    {0xE171, 0x5AED}, //13934 #CJK UNIFIED IDEOGRAPH
    {0xE172, 0x5AEB}, //13935 #CJK UNIFIED IDEOGRAPH
    {0xE173, 0x5AF3}, //13936 #CJK UNIFIED IDEOGRAPH
    {0xE174, 0x5AE2}, //13937 #CJK UNIFIED IDEOGRAPH
    {0xE175, 0x5AE0}, //13938 #CJK UNIFIED IDEOGRAPH
    {0xE176, 0x5ADB}, //13939 #CJK UNIFIED IDEOGRAPH
    {0xE177, 0x5AEC}, //13940 #CJK UNIFIED IDEOGRAPH
    {0xE178, 0x5ADE}, //13941 #CJK UNIFIED IDEOGRAPH
    {0xE179, 0x5ADD}, //13942 #CJK UNIFIED IDEOGRAPH
    {0xE17A, 0x5AD9}, //13943 #CJK UNIFIED IDEOGRAPH
    {0xE17B, 0x5AE8}, //13944 #CJK UNIFIED IDEOGRAPH
    {0xE17C, 0x5ADF}, //13945 #CJK UNIFIED IDEOGRAPH
    {0xE17D, 0x5B77}, //13946 #CJK UNIFIED IDEOGRAPH
    {0xE17E, 0x5BE0}, //13947 #CJK UNIFIED IDEOGRAPH
    {0xE1A1, 0x5BE3}, //13948 #CJK UNIFIED IDEOGRAPH
    {0xE1A2, 0x5C63}, //13949 #CJK UNIFIED IDEOGRAPH
    {0xE1A3, 0x5D82}, //13950 #CJK UNIFIED IDEOGRAPH
    {0xE1A4, 0x5D80}, //13951 #CJK UNIFIED IDEOGRAPH
    {0xE1A5, 0x5D7D}, //13952 #CJK UNIFIED IDEOGRAPH
    {0xE1A6, 0x5D86}, //13953 #CJK UNIFIED IDEOGRAPH
    {0xE1A7, 0x5D7A}, //13954 #CJK UNIFIED IDEOGRAPH
    {0xE1A8, 0x5D81}, //13955 #CJK UNIFIED IDEOGRAPH
    {0xE1A9, 0x5D77}, //13956 #CJK UNIFIED IDEOGRAPH
    {0xE1AA, 0x5D8A}, //13957 #CJK UNIFIED IDEOGRAPH
    {0xE1AB, 0x5D89}, //13958 #CJK UNIFIED IDEOGRAPH
    {0xE1AC, 0x5D88}, //13959 #CJK UNIFIED IDEOGRAPH
    {0xE1AD, 0x5D7E}, //13960 #CJK UNIFIED IDEOGRAPH
    {0xE1AE, 0x5D7C}, //13961 #CJK UNIFIED IDEOGRAPH
    {0xE1AF, 0x5D8D}, //13962 #CJK UNIFIED IDEOGRAPH
    {0xE1B0, 0x5D79}, //13963 #CJK UNIFIED IDEOGRAPH
    {0xE1B1, 0x5D7F}, //13964 #CJK UNIFIED IDEOGRAPH
    {0xE1B2, 0x5E58}, //13965 #CJK UNIFIED IDEOGRAPH
    {0xE1B3, 0x5E59}, //13966 #CJK UNIFIED IDEOGRAPH
    {0xE1B4, 0x5E53}, //13967 #CJK UNIFIED IDEOGRAPH
    {0xE1B5, 0x5ED8}, //13968 #CJK UNIFIED IDEOGRAPH
    {0xE1B6, 0x5ED1}, //13969 #CJK UNIFIED IDEOGRAPH
    {0xE1B7, 0x5ED7}, //13970 #CJK UNIFIED IDEOGRAPH
    {0xE1B8, 0x5ECE}, //13971 #CJK UNIFIED IDEOGRAPH
    {0xE1B9, 0x5EDC}, //13972 #CJK UNIFIED IDEOGRAPH
    {0xE1BA, 0x5ED5}, //13973 #CJK UNIFIED IDEOGRAPH
    {0xE1BB, 0x5ED9}, //13974 #CJK UNIFIED IDEOGRAPH
    {0xE1BC, 0x5ED2}, //13975 #CJK UNIFIED IDEOGRAPH
    {0xE1BD, 0x5ED4}, //13976 #CJK UNIFIED IDEOGRAPH
    {0xE1BE, 0x5F44}, //13977 #CJK UNIFIED IDEOGRAPH
    {0xE1BF, 0x5F43}, //13978 #CJK UNIFIED IDEOGRAPH
    {0xE1C0, 0x5F6F}, //13979 #CJK UNIFIED IDEOGRAPH
    {0xE1C1, 0x5FB6}, //13980 #CJK UNIFIED IDEOGRAPH
    {0xE1C2, 0x612C}, //13981 #CJK UNIFIED IDEOGRAPH
    {0xE1C3, 0x6128}, //13982 #CJK UNIFIED IDEOGRAPH
    {0xE1C4, 0x6141}, //13983 #CJK UNIFIED IDEOGRAPH
    {0xE1C5, 0x615E}, //13984 #CJK UNIFIED IDEOGRAPH
    {0xE1C6, 0x6171}, //13985 #CJK UNIFIED IDEOGRAPH
    {0xE1C7, 0x6173}, //13986 #CJK UNIFIED IDEOGRAPH
    {0xE1C8, 0x6152}, //13987 #CJK UNIFIED IDEOGRAPH
    {0xE1C9, 0x6153}, //13988 #CJK UNIFIED IDEOGRAPH
    {0xE1CA, 0x6172}, //13989 #CJK UNIFIED IDEOGRAPH
    {0xE1CB, 0x616C}, //13990 #CJK UNIFIED IDEOGRAPH
    {0xE1CC, 0x6180}, //13991 #CJK UNIFIED IDEOGRAPH
    {0xE1CD, 0x6174}, //13992 #CJK UNIFIED IDEOGRAPH
    {0xE1CE, 0x6154}, //13993 #CJK UNIFIED IDEOGRAPH
    {0xE1CF, 0x617A}, //13994 #CJK UNIFIED IDEOGRAPH
    {0xE1D0, 0x615B}, //13995 #CJK UNIFIED IDEOGRAPH
    {0xE1D1, 0x6165}, //13996 #CJK UNIFIED IDEOGRAPH
    {0xE1D2, 0x613B}, //13997 #CJK UNIFIED IDEOGRAPH
    {0xE1D3, 0x616A}, //13998 #CJK UNIFIED IDEOGRAPH
    {0xE1D4, 0x6161}, //13999 #CJK UNIFIED IDEOGRAPH
    {0xE1D5, 0x6156}, //14000 #CJK UNIFIED IDEOGRAPH
    {0xE1D6, 0x6229}, //14001 #CJK UNIFIED IDEOGRAPH
    {0xE1D7, 0x6227}, //14002 #CJK UNIFIED IDEOGRAPH
    {0xE1D8, 0x622B}, //14003 #CJK UNIFIED IDEOGRAPH
    {0xE1D9, 0x642B}, //14004 #CJK UNIFIED IDEOGRAPH
    {0xE1DA, 0x644D}, //14005 #CJK UNIFIED IDEOGRAPH
    {0xE1DB, 0x645B}, //14006 #CJK UNIFIED IDEOGRAPH
    {0xE1DC, 0x645D}, //14007 #CJK UNIFIED IDEOGRAPH
    {0xE1DD, 0x6474}, //14008 #CJK UNIFIED IDEOGRAPH
    {0xE1DE, 0x6476}, //14009 #CJK UNIFIED IDEOGRAPH
    {0xE1DF, 0x6472}, //14010 #CJK UNIFIED IDEOGRAPH
    {0xE1E0, 0x6473}, //14011 #CJK UNIFIED IDEOGRAPH
    {0xE1E1, 0x647D}, //14012 #CJK UNIFIED IDEOGRAPH
    {0xE1E2, 0x6475}, //14013 #CJK UNIFIED IDEOGRAPH
    {0xE1E3, 0x6466}, //14014 #CJK UNIFIED IDEOGRAPH
    {0xE1E4, 0x64A6}, //14015 #CJK UNIFIED IDEOGRAPH
    {0xE1E5, 0x644E}, //14016 #CJK UNIFIED IDEOGRAPH
    {0xE1E6, 0x6482}, //14017 #CJK UNIFIED IDEOGRAPH
    {0xE1E7, 0x645E}, //14018 #CJK UNIFIED IDEOGRAPH
    {0xE1E8, 0x645C}, //14019 #CJK UNIFIED IDEOGRAPH
    {0xE1E9, 0x644B}, //14020 #CJK UNIFIED IDEOGRAPH
    {0xE1EA, 0x6453}, //14021 #CJK UNIFIED IDEOGRAPH
    {0xE1EB, 0x6460}, //14022 #CJK UNIFIED IDEOGRAPH
    {0xE1EC, 0x6450}, //14023 #CJK UNIFIED IDEOGRAPH
    {0xE1ED, 0x647F}, //14024 #CJK UNIFIED IDEOGRAPH
    {0xE1EE, 0x643F}, //14025 #CJK UNIFIED IDEOGRAPH
    {0xE1EF, 0x646C}, //14026 #CJK UNIFIED IDEOGRAPH
    {0xE1F0, 0x646B}, //14027 #CJK UNIFIED IDEOGRAPH
    {0xE1F1, 0x6459}, //14028 #CJK UNIFIED IDEOGRAPH
    {0xE1F2, 0x6465}, //14029 #CJK UNIFIED IDEOGRAPH
    {0xE1F3, 0x6477}, //14030 #CJK UNIFIED IDEOGRAPH
    {0xE1F4, 0x6573}, //14031 #CJK UNIFIED IDEOGRAPH
    {0xE1F5, 0x65A0}, //14032 #CJK UNIFIED IDEOGRAPH
    {0xE1F6, 0x66A1}, //14033 #CJK UNIFIED IDEOGRAPH
    {0xE1F7, 0x66A0}, //14034 #CJK UNIFIED IDEOGRAPH
    {0xE1F8, 0x669F}, //14035 #CJK UNIFIED IDEOGRAPH
    {0xE1F9, 0x6705}, //14036 #CJK UNIFIED IDEOGRAPH
    {0xE1FA, 0x6704}, //14037 #CJK UNIFIED IDEOGRAPH
    {0xE1FB, 0x6722}, //14038 #CJK UNIFIED IDEOGRAPH
    {0xE1FC, 0x69B1}, //14039 #CJK UNIFIED IDEOGRAPH
    {0xE1FD, 0x69B6}, //14040 #CJK UNIFIED IDEOGRAPH
    {0xE1FE, 0x69C9}, //14041 #CJK UNIFIED IDEOGRAPH
    {0xE240, 0x69A0}, //14042 #CJK UNIFIED IDEOGRAPH
    {0xE241, 0x69CE}, //14043 #CJK UNIFIED IDEOGRAPH
    {0xE242, 0x6996}, //14044 #CJK UNIFIED IDEOGRAPH
    {0xE243, 0x69B0}, //14045 #CJK UNIFIED IDEOGRAPH
    {0xE244, 0x69AC}, //14046 #CJK UNIFIED IDEOGRAPH
    {0xE245, 0x69BC}, //14047 #CJK UNIFIED IDEOGRAPH
    {0xE246, 0x6991}, //14048 #CJK UNIFIED IDEOGRAPH
    {0xE247, 0x6999}, //14049 #CJK UNIFIED IDEOGRAPH
    {0xE248, 0x698E}, //14050 #CJK UNIFIED IDEOGRAPH
    {0xE249, 0x69A7}, //14051 #CJK UNIFIED IDEOGRAPH
    {0xE24A, 0x698D}, //14052 #CJK UNIFIED IDEOGRAPH
    {0xE24B, 0x69A9}, //14053 #CJK UNIFIED IDEOGRAPH
    {0xE24C, 0x69BE}, //14054 #CJK UNIFIED IDEOGRAPH
    {0xE24D, 0x69AF}, //14055 #CJK UNIFIED IDEOGRAPH
    {0xE24E, 0x69BF}, //14056 #CJK UNIFIED IDEOGRAPH
    {0xE24F, 0x69C4}, //14057 #CJK UNIFIED IDEOGRAPH
    {0xE250, 0x69BD}, //14058 #CJK UNIFIED IDEOGRAPH
    {0xE251, 0x69A4}, //14059 #CJK UNIFIED IDEOGRAPH
    {0xE252, 0x69D4}, //14060 #CJK UNIFIED IDEOGRAPH
    {0xE253, 0x69B9}, //14061 #CJK UNIFIED IDEOGRAPH
    {0xE254, 0x69CA}, //14062 #CJK UNIFIED IDEOGRAPH
    {0xE255, 0x699A}, //14063 #CJK UNIFIED IDEOGRAPH
    {0xE256, 0x69CF}, //14064 #CJK UNIFIED IDEOGRAPH
    {0xE257, 0x69B3}, //14065 #CJK UNIFIED IDEOGRAPH
    {0xE258, 0x6993}, //14066 #CJK UNIFIED IDEOGRAPH
    {0xE259, 0x69AA}, //14067 #CJK UNIFIED IDEOGRAPH
    {0xE25A, 0x69A1}, //14068 #CJK UNIFIED IDEOGRAPH
    {0xE25B, 0x699E}, //14069 #CJK UNIFIED IDEOGRAPH
    {0xE25C, 0x69D9}, //14070 #CJK UNIFIED IDEOGRAPH
    {0xE25D, 0x6997}, //14071 #CJK UNIFIED IDEOGRAPH
    {0xE25E, 0x6990}, //14072 #CJK UNIFIED IDEOGRAPH
    {0xE25F, 0x69C2}, //14073 #CJK UNIFIED IDEOGRAPH
    {0xE260, 0x69B5}, //14074 #CJK UNIFIED IDEOGRAPH
    {0xE261, 0x69A5}, //14075 #CJK UNIFIED IDEOGRAPH
    {0xE262, 0x69C6}, //14076 #CJK UNIFIED IDEOGRAPH
    {0xE263, 0x6B4A}, //14077 #CJK UNIFIED IDEOGRAPH
    {0xE264, 0x6B4D}, //14078 #CJK UNIFIED IDEOGRAPH
    {0xE265, 0x6B4B}, //14079 #CJK UNIFIED IDEOGRAPH
    {0xE266, 0x6B9E}, //14080 #CJK UNIFIED IDEOGRAPH
    {0xE267, 0x6B9F}, //14081 #CJK UNIFIED IDEOGRAPH
    {0xE268, 0x6BA0}, //14082 #CJK UNIFIED IDEOGRAPH
    {0xE269, 0x6BC3}, //14083 #CJK UNIFIED IDEOGRAPH
    {0xE26A, 0x6BC4}, //14084 #CJK UNIFIED IDEOGRAPH
    {0xE26B, 0x6BFE}, //14085 #CJK UNIFIED IDEOGRAPH
    {0xE26C, 0x6ECE}, //14086 #CJK UNIFIED IDEOGRAPH
    {0xE26D, 0x6EF5}, //14087 #CJK UNIFIED IDEOGRAPH
    {0xE26E, 0x6EF1}, //14088 #CJK UNIFIED IDEOGRAPH
    {0xE26F, 0x6F03}, //14089 #CJK UNIFIED IDEOGRAPH
    {0xE270, 0x6F25}, //14090 #CJK UNIFIED IDEOGRAPH
    {0xE271, 0x6EF8}, //14091 #CJK UNIFIED IDEOGRAPH
    {0xE272, 0x6F37}, //14092 #CJK UNIFIED IDEOGRAPH
    {0xE273, 0x6EFB}, //14093 #CJK UNIFIED IDEOGRAPH
    {0xE274, 0x6F2E}, //14094 #CJK UNIFIED IDEOGRAPH
    {0xE275, 0x6F09}, //14095 #CJK UNIFIED IDEOGRAPH
    {0xE276, 0x6F4E}, //14096 #CJK UNIFIED IDEOGRAPH
    {0xE277, 0x6F19}, //14097 #CJK UNIFIED IDEOGRAPH
    {0xE278, 0x6F1A}, //14098 #CJK UNIFIED IDEOGRAPH
    {0xE279, 0x6F27}, //14099 #CJK UNIFIED IDEOGRAPH
    {0xE27A, 0x6F18}, //14100 #CJK UNIFIED IDEOGRAPH
    {0xE27B, 0x6F3B}, //14101 #CJK UNIFIED IDEOGRAPH
    {0xE27C, 0x6F12}, //14102 #CJK UNIFIED IDEOGRAPH
    {0xE27D, 0x6EED}, //14103 #CJK UNIFIED IDEOGRAPH
    {0xE27E, 0x6F0A}, //14104 #CJK UNIFIED IDEOGRAPH
    {0xE2A1, 0x6F36}, //14105 #CJK UNIFIED IDEOGRAPH
    {0xE2A2, 0x6F73}, //14106 #CJK UNIFIED IDEOGRAPH
    {0xE2A3, 0x6EF9}, //14107 #CJK UNIFIED IDEOGRAPH
    {0xE2A4, 0x6EEE}, //14108 #CJK UNIFIED IDEOGRAPH
    {0xE2A5, 0x6F2D}, //14109 #CJK UNIFIED IDEOGRAPH
    {0xE2A6, 0x6F40}, //14110 #CJK UNIFIED IDEOGRAPH
    {0xE2A7, 0x6F30}, //14111 #CJK UNIFIED IDEOGRAPH
    {0xE2A8, 0x6F3C}, //14112 #CJK UNIFIED IDEOGRAPH
    {0xE2A9, 0x6F35}, //14113 #CJK UNIFIED IDEOGRAPH
    {0xE2AA, 0x6EEB}, //14114 #CJK UNIFIED IDEOGRAPH
    {0xE2AB, 0x6F07}, //14115 #CJK UNIFIED IDEOGRAPH
    {0xE2AC, 0x6F0E}, //14116 #CJK UNIFIED IDEOGRAPH
    {0xE2AD, 0x6F43}, //14117 #CJK UNIFIED IDEOGRAPH
    {0xE2AE, 0x6F05}, //14118 #CJK UNIFIED IDEOGRAPH
    {0xE2AF, 0x6EFD}, //14119 #CJK UNIFIED IDEOGRAPH
    {0xE2B0, 0x6EF6}, //14120 #CJK UNIFIED IDEOGRAPH
    {0xE2B1, 0x6F39}, //14121 #CJK UNIFIED IDEOGRAPH
    {0xE2B2, 0x6F1C}, //14122 #CJK UNIFIED IDEOGRAPH
    {0xE2B3, 0x6EFC}, //14123 #CJK UNIFIED IDEOGRAPH
    {0xE2B4, 0x6F3A}, //14124 #CJK UNIFIED IDEOGRAPH
    {0xE2B5, 0x6F1F}, //14125 #CJK UNIFIED IDEOGRAPH
    {0xE2B6, 0x6F0D}, //14126 #CJK UNIFIED IDEOGRAPH
    {0xE2B7, 0x6F1E}, //14127 #CJK UNIFIED IDEOGRAPH
    {0xE2B8, 0x6F08}, //14128 #CJK UNIFIED IDEOGRAPH
    {0xE2B9, 0x6F21}, //14129 #CJK UNIFIED IDEOGRAPH
    {0xE2BA, 0x7187}, //14130 #CJK UNIFIED IDEOGRAPH
    {0xE2BB, 0x7190}, //14131 #CJK UNIFIED IDEOGRAPH
    {0xE2BC, 0x7189}, //14132 #CJK UNIFIED IDEOGRAPH
    {0xE2BD, 0x7180}, //14133 #CJK UNIFIED IDEOGRAPH
    {0xE2BE, 0x7185}, //14134 #CJK UNIFIED IDEOGRAPH
    {0xE2BF, 0x7182}, //14135 #CJK UNIFIED IDEOGRAPH
    {0xE2C0, 0x718F}, //14136 #CJK UNIFIED IDEOGRAPH
    {0xE2C1, 0x717B}, //14137 #CJK UNIFIED IDEOGRAPH
    {0xE2C2, 0x7186}, //14138 #CJK UNIFIED IDEOGRAPH
    {0xE2C3, 0x7181}, //14139 #CJK UNIFIED IDEOGRAPH
    {0xE2C4, 0x7197}, //14140 #CJK UNIFIED IDEOGRAPH
    {0xE2C5, 0x7244}, //14141 #CJK UNIFIED IDEOGRAPH
    {0xE2C6, 0x7253}, //14142 #CJK UNIFIED IDEOGRAPH
    {0xE2C7, 0x7297}, //14143 #CJK UNIFIED IDEOGRAPH
    {0xE2C8, 0x7295}, //14144 #CJK UNIFIED IDEOGRAPH
    {0xE2C9, 0x7293}, //14145 #CJK UNIFIED IDEOGRAPH
    {0xE2CA, 0x7343}, //14146 #CJK UNIFIED IDEOGRAPH
    {0xE2CB, 0x734D}, //14147 #CJK UNIFIED IDEOGRAPH
    {0xE2CC, 0x7351}, //14148 #CJK UNIFIED IDEOGRAPH
    {0xE2CD, 0x734C}, //14149 #CJK UNIFIED IDEOGRAPH
    {0xE2CE, 0x7462}, //14150 #CJK UNIFIED IDEOGRAPH
    {0xE2CF, 0x7473}, //14151 #CJK UNIFIED IDEOGRAPH
    {0xE2D0, 0x7471}, //14152 #CJK UNIFIED IDEOGRAPH
    {0xE2D1, 0x7475}, //14153 #CJK UNIFIED IDEOGRAPH
    {0xE2D2, 0x7472}, //14154 #CJK UNIFIED IDEOGRAPH
    {0xE2D3, 0x7467}, //14155 #CJK UNIFIED IDEOGRAPH
    {0xE2D4, 0x746E}, //14156 #CJK UNIFIED IDEOGRAPH
    {0xE2D5, 0x7500}, //14157 #CJK UNIFIED IDEOGRAPH
    {0xE2D6, 0x7502}, //14158 #CJK UNIFIED IDEOGRAPH
    {0xE2D7, 0x7503}, //14159 #CJK UNIFIED IDEOGRAPH
    {0xE2D8, 0x757D}, //14160 #CJK UNIFIED IDEOGRAPH
    {0xE2D9, 0x7590}, //14161 #CJK UNIFIED IDEOGRAPH
    {0xE2DA, 0x7616}, //14162 #CJK UNIFIED IDEOGRAPH
    {0xE2DB, 0x7608}, //14163 #CJK UNIFIED IDEOGRAPH
    {0xE2DC, 0x760C}, //14164 #CJK UNIFIED IDEOGRAPH
    {0xE2DD, 0x7615}, //14165 #CJK UNIFIED IDEOGRAPH
    {0xE2DE, 0x7611}, //14166 #CJK UNIFIED IDEOGRAPH
    {0xE2DF, 0x760A}, //14167 #CJK UNIFIED IDEOGRAPH
    {0xE2E0, 0x7614}, //14168 #CJK UNIFIED IDEOGRAPH
    {0xE2E1, 0x76B8}, //14169 #CJK UNIFIED IDEOGRAPH
    {0xE2E2, 0x7781}, //14170 #CJK UNIFIED IDEOGRAPH
    {0xE2E3, 0x777C}, //14171 #CJK UNIFIED IDEOGRAPH
    {0xE2E4, 0x7785}, //14172 #CJK UNIFIED IDEOGRAPH
    {0xE2E5, 0x7782}, //14173 #CJK UNIFIED IDEOGRAPH
    {0xE2E6, 0x776E}, //14174 #CJK UNIFIED IDEOGRAPH
    {0xE2E7, 0x7780}, //14175 #CJK UNIFIED IDEOGRAPH
    {0xE2E8, 0x776F}, //14176 #CJK UNIFIED IDEOGRAPH
    {0xE2E9, 0x777E}, //14177 #CJK UNIFIED IDEOGRAPH
    {0xE2EA, 0x7783}, //14178 #CJK UNIFIED IDEOGRAPH
    {0xE2EB, 0x78B2}, //14179 #CJK UNIFIED IDEOGRAPH
    {0xE2EC, 0x78AA}, //14180 #CJK UNIFIED IDEOGRAPH
    {0xE2ED, 0x78B4}, //14181 #CJK UNIFIED IDEOGRAPH
    {0xE2EE, 0x78AD}, //14182 #CJK UNIFIED IDEOGRAPH
    {0xE2EF, 0x78A8}, //14183 #CJK UNIFIED IDEOGRAPH
    {0xE2F0, 0x787E}, //14184 #CJK UNIFIED IDEOGRAPH
    {0xE2F1, 0x78AB}, //14185 #CJK UNIFIED IDEOGRAPH
    {0xE2F2, 0x789E}, //14186 #CJK UNIFIED IDEOGRAPH
    {0xE2F3, 0x78A5}, //14187 #CJK UNIFIED IDEOGRAPH
    {0xE2F4, 0x78A0}, //14188 #CJK UNIFIED IDEOGRAPH
    {0xE2F5, 0x78AC}, //14189 #CJK UNIFIED IDEOGRAPH
    {0xE2F6, 0x78A2}, //14190 #CJK UNIFIED IDEOGRAPH
    {0xE2F7, 0x78A4}, //14191 #CJK UNIFIED IDEOGRAPH
    {0xE2F8, 0x7998}, //14192 #CJK UNIFIED IDEOGRAPH
    {0xE2F9, 0x798A}, //14193 #CJK UNIFIED IDEOGRAPH
    {0xE2FA, 0x798B}, //14194 #CJK UNIFIED IDEOGRAPH
    {0xE2FB, 0x7996}, //14195 #CJK UNIFIED IDEOGRAPH
    {0xE2FC, 0x7995}, //14196 #CJK UNIFIED IDEOGRAPH
    {0xE2FD, 0x7994}, //14197 #CJK UNIFIED IDEOGRAPH
    {0xE2FE, 0x7993}, //14198 #CJK UNIFIED IDEOGRAPH
    {0xE340, 0x7997}, //14199 #CJK UNIFIED IDEOGRAPH
    {0xE341, 0x7988}, //14200 #CJK UNIFIED IDEOGRAPH
    {0xE342, 0x7992}, //14201 #CJK UNIFIED IDEOGRAPH
    {0xE343, 0x7990}, //14202 #CJK UNIFIED IDEOGRAPH
    {0xE344, 0x7A2B}, //14203 #CJK UNIFIED IDEOGRAPH
    {0xE345, 0x7A4A}, //14204 #CJK UNIFIED IDEOGRAPH
    {0xE346, 0x7A30}, //14205 #CJK UNIFIED IDEOGRAPH
    {0xE347, 0x7A2F}, //14206 #CJK UNIFIED IDEOGRAPH
    {0xE348, 0x7A28}, //14207 #CJK UNIFIED IDEOGRAPH
    {0xE349, 0x7A26}, //14208 #CJK UNIFIED IDEOGRAPH
    {0xE34A, 0x7AA8}, //14209 #CJK UNIFIED IDEOGRAPH
    {0xE34B, 0x7AAB}, //14210 #CJK UNIFIED IDEOGRAPH
    {0xE34C, 0x7AAC}, //14211 #CJK UNIFIED IDEOGRAPH
    {0xE34D, 0x7AEE}, //14212 #CJK UNIFIED IDEOGRAPH
    {0xE34E, 0x7B88}, //14213 #CJK UNIFIED IDEOGRAPH
    {0xE34F, 0x7B9C}, //14214 #CJK UNIFIED IDEOGRAPH
    {0xE350, 0x7B8A}, //14215 #CJK UNIFIED IDEOGRAPH
    {0xE351, 0x7B91}, //14216 #CJK UNIFIED IDEOGRAPH
    {0xE352, 0x7B90}, //14217 #CJK UNIFIED IDEOGRAPH
    {0xE353, 0x7B96}, //14218 #CJK UNIFIED IDEOGRAPH
    {0xE354, 0x7B8D}, //14219 #CJK UNIFIED IDEOGRAPH
    {0xE355, 0x7B8C}, //14220 #CJK UNIFIED IDEOGRAPH
    {0xE356, 0x7B9B}, //14221 #CJK UNIFIED IDEOGRAPH
    {0xE357, 0x7B8E}, //14222 #CJK UNIFIED IDEOGRAPH
    {0xE358, 0x7B85}, //14223 #CJK UNIFIED IDEOGRAPH
    {0xE359, 0x7B98}, //14224 #CJK UNIFIED IDEOGRAPH
    {0xE35A, 0x5284}, //14225 #CJK UNIFIED IDEOGRAPH
    {0xE35B, 0x7B99}, //14226 #CJK UNIFIED IDEOGRAPH
    {0xE35C, 0x7BA4}, //14227 #CJK UNIFIED IDEOGRAPH
    {0xE35D, 0x7B82}, //14228 #CJK UNIFIED IDEOGRAPH
    {0xE35E, 0x7CBB}, //14229 #CJK UNIFIED IDEOGRAPH
    {0xE35F, 0x7CBF}, //14230 #CJK UNIFIED IDEOGRAPH
    {0xE360, 0x7CBC}, //14231 #CJK UNIFIED IDEOGRAPH
    {0xE361, 0x7CBA}, //14232 #CJK UNIFIED IDEOGRAPH
    {0xE362, 0x7DA7}, //14233 #CJK UNIFIED IDEOGRAPH
    {0xE363, 0x7DB7}, //14234 #CJK UNIFIED IDEOGRAPH
    {0xE364, 0x7DC2}, //14235 #CJK UNIFIED IDEOGRAPH
    {0xE365, 0x7DA3}, //14236 #CJK UNIFIED IDEOGRAPH
    {0xE366, 0x7DAA}, //14237 #CJK UNIFIED IDEOGRAPH
    {0xE367, 0x7DC1}, //14238 #CJK UNIFIED IDEOGRAPH
    {0xE368, 0x7DC0}, //14239 #CJK UNIFIED IDEOGRAPH
    {0xE369, 0x7DC5}, //14240 #CJK UNIFIED IDEOGRAPH
    {0xE36A, 0x7D9D}, //14241 #CJK UNIFIED IDEOGRAPH
    {0xE36B, 0x7DCE}, //14242 #CJK UNIFIED IDEOGRAPH
    {0xE36C, 0x7DC4}, //14243 #CJK UNIFIED IDEOGRAPH
    {0xE36D, 0x7DC6}, //14244 #CJK UNIFIED IDEOGRAPH
    {0xE36E, 0x7DCB}, //14245 #CJK UNIFIED IDEOGRAPH
    {0xE36F, 0x7DCC}, //14246 #CJK UNIFIED IDEOGRAPH
    {0xE370, 0x7DAF}, //14247 #CJK UNIFIED IDEOGRAPH
    {0xE371, 0x7DB9}, //14248 #CJK UNIFIED IDEOGRAPH
    {0xE372, 0x7D96}, //14249 #CJK UNIFIED IDEOGRAPH
    {0xE373, 0x7DBC}, //14250 #CJK UNIFIED IDEOGRAPH
    {0xE374, 0x7D9F}, //14251 #CJK UNIFIED IDEOGRAPH
    {0xE375, 0x7DA6}, //14252 #CJK UNIFIED IDEOGRAPH
    {0xE376, 0x7DAE}, //14253 #CJK UNIFIED IDEOGRAPH
    {0xE377, 0x7DA9}, //14254 #CJK UNIFIED IDEOGRAPH
    {0xE378, 0x7DA1}, //14255 #CJK UNIFIED IDEOGRAPH
    {0xE379, 0x7DC9}, //14256 #CJK UNIFIED IDEOGRAPH
    {0xE37A, 0x7F73}, //14257 #CJK UNIFIED IDEOGRAPH
    {0xE37B, 0x7FE2}, //14258 #CJK UNIFIED IDEOGRAPH
    {0xE37C, 0x7FE3}, //14259 #CJK UNIFIED IDEOGRAPH
    {0xE37D, 0x7FE5}, //14260 #CJK UNIFIED IDEOGRAPH
    {0xE37E, 0x7FDE}, //14261 #CJK UNIFIED IDEOGRAPH
    {0xE3A1, 0x8024}, //14262 #CJK UNIFIED IDEOGRAPH
    {0xE3A2, 0x805D}, //14263 #CJK UNIFIED IDEOGRAPH
    {0xE3A3, 0x805C}, //14264 #CJK UNIFIED IDEOGRAPH
    {0xE3A4, 0x8189}, //14265 #CJK UNIFIED IDEOGRAPH
    {0xE3A5, 0x8186}, //14266 #CJK UNIFIED IDEOGRAPH
    {0xE3A6, 0x8183}, //14267 #CJK UNIFIED IDEOGRAPH
    {0xE3A7, 0x8187}, //14268 #CJK UNIFIED IDEOGRAPH
    {0xE3A8, 0x818D}, //14269 #CJK UNIFIED IDEOGRAPH
    {0xE3A9, 0x818C}, //14270 #CJK UNIFIED IDEOGRAPH
    {0xE3AA, 0x818B}, //14271 #CJK UNIFIED IDEOGRAPH
    {0xE3AB, 0x8215}, //14272 #CJK UNIFIED IDEOGRAPH
    {0xE3AC, 0x8497}, //14273 #CJK UNIFIED IDEOGRAPH
    {0xE3AD, 0x84A4}, //14274 #CJK UNIFIED IDEOGRAPH
    {0xE3AE, 0x84A1}, //14275 #CJK UNIFIED IDEOGRAPH
    {0xE3AF, 0x849F}, //14276 #CJK UNIFIED IDEOGRAPH
    {0xE3B0, 0x84BA}, //14277 #CJK UNIFIED IDEOGRAPH
    {0xE3B1, 0x84CE}, //14278 #CJK UNIFIED IDEOGRAPH
    {0xE3B2, 0x84C2}, //14279 #CJK UNIFIED IDEOGRAPH
    {0xE3B3, 0x84AC}, //14280 #CJK UNIFIED IDEOGRAPH
    {0xE3B4, 0x84AE}, //14281 #CJK UNIFIED IDEOGRAPH
    {0xE3B5, 0x84AB}, //14282 #CJK UNIFIED IDEOGRAPH
    {0xE3B6, 0x84B9}, //14283 #CJK UNIFIED IDEOGRAPH
    {0xE3B7, 0x84B4}, //14284 #CJK UNIFIED IDEOGRAPH
    {0xE3B8, 0x84C1}, //14285 #CJK UNIFIED IDEOGRAPH
    {0xE3B9, 0x84CD}, //14286 #CJK UNIFIED IDEOGRAPH
    {0xE3BA, 0x84AA}, //14287 #CJK UNIFIED IDEOGRAPH
    {0xE3BB, 0x849A}, //14288 #CJK UNIFIED IDEOGRAPH
    {0xE3BC, 0x84B1}, //14289 #CJK UNIFIED IDEOGRAPH
    {0xE3BD, 0x84D0}, //14290 #CJK UNIFIED IDEOGRAPH
    {0xE3BE, 0x849D}, //14291 #CJK UNIFIED IDEOGRAPH
    {0xE3BF, 0x84A7}, //14292 #CJK UNIFIED IDEOGRAPH
    {0xE3C0, 0x84BB}, //14293 #CJK UNIFIED IDEOGRAPH
    {0xE3C1, 0x84A2}, //14294 #CJK UNIFIED IDEOGRAPH
    {0xE3C2, 0x8494}, //14295 #CJK UNIFIED IDEOGRAPH
    {0xE3C3, 0x84C7}, //14296 #CJK UNIFIED IDEOGRAPH
    {0xE3C4, 0x84CC}, //14297 #CJK UNIFIED IDEOGRAPH
    {0xE3C5, 0x849B}, //14298 #CJK UNIFIED IDEOGRAPH
    {0xE3C6, 0x84A9}, //14299 #CJK UNIFIED IDEOGRAPH
    {0xE3C7, 0x84AF}, //14300 #CJK UNIFIED IDEOGRAPH
    {0xE3C8, 0x84A8}, //14301 #CJK UNIFIED IDEOGRAPH
    {0xE3C9, 0x84D6}, //14302 #CJK UNIFIED IDEOGRAPH
    {0xE3CA, 0x8498}, //14303 #CJK UNIFIED IDEOGRAPH
    {0xE3CB, 0x84B6}, //14304 #CJK UNIFIED IDEOGRAPH
    {0xE3CC, 0x84CF}, //14305 #CJK UNIFIED IDEOGRAPH
    {0xE3CD, 0x84A0}, //14306 #CJK UNIFIED IDEOGRAPH
    {0xE3CE, 0x84D7}, //14307 #CJK UNIFIED IDEOGRAPH
    {0xE3CF, 0x84D4}, //14308 #CJK UNIFIED IDEOGRAPH
    {0xE3D0, 0x84D2}, //14309 #CJK UNIFIED IDEOGRAPH
    {0xE3D1, 0x84DB}, //14310 #CJK UNIFIED IDEOGRAPH
    {0xE3D2, 0x84B0}, //14311 #CJK UNIFIED IDEOGRAPH
    {0xE3D3, 0x8491}, //14312 #CJK UNIFIED IDEOGRAPH
    {0xE3D4, 0x8661}, //14313 #CJK UNIFIED IDEOGRAPH
    {0xE3D5, 0x8733}, //14314 #CJK UNIFIED IDEOGRAPH
    {0xE3D6, 0x8723}, //14315 #CJK UNIFIED IDEOGRAPH
    {0xE3D7, 0x8728}, //14316 #CJK UNIFIED IDEOGRAPH
    {0xE3D8, 0x876B}, //14317 #CJK UNIFIED IDEOGRAPH
    {0xE3D9, 0x8740}, //14318 #CJK UNIFIED IDEOGRAPH
    {0xE3DA, 0x872E}, //14319 #CJK UNIFIED IDEOGRAPH
    {0xE3DB, 0x871E}, //14320 #CJK UNIFIED IDEOGRAPH
    {0xE3DC, 0x8721}, //14321 #CJK UNIFIED IDEOGRAPH
    {0xE3DD, 0x8719}, //14322 #CJK UNIFIED IDEOGRAPH
    {0xE3DE, 0x871B}, //14323 #CJK UNIFIED IDEOGRAPH
    {0xE3DF, 0x8743}, //14324 #CJK UNIFIED IDEOGRAPH
    {0xE3E0, 0x872C}, //14325 #CJK UNIFIED IDEOGRAPH
    {0xE3E1, 0x8741}, //14326 #CJK UNIFIED IDEOGRAPH
    {0xE3E2, 0x873E}, //14327 #CJK UNIFIED IDEOGRAPH
    {0xE3E3, 0x8746}, //14328 #CJK UNIFIED IDEOGRAPH
    {0xE3E4, 0x8720}, //14329 #CJK UNIFIED IDEOGRAPH
    {0xE3E5, 0x8732}, //14330 #CJK UNIFIED IDEOGRAPH
    {0xE3E6, 0x872A}, //14331 #CJK UNIFIED IDEOGRAPH
    {0xE3E7, 0x872D}, //14332 #CJK UNIFIED IDEOGRAPH
    {0xE3E8, 0x873C}, //14333 #CJK UNIFIED IDEOGRAPH
    {0xE3E9, 0x8712}, //14334 #CJK UNIFIED IDEOGRAPH
    {0xE3EA, 0x873A}, //14335 #CJK UNIFIED IDEOGRAPH
    {0xE3EB, 0x8731}, //14336 #CJK UNIFIED IDEOGRAPH
    {0xE3EC, 0x8735}, //14337 #CJK UNIFIED IDEOGRAPH
    {0xE3ED, 0x8742}, //14338 #CJK UNIFIED IDEOGRAPH
    {0xE3EE, 0x8726}, //14339 #CJK UNIFIED IDEOGRAPH
    {0xE3EF, 0x8727}, //14340 #CJK UNIFIED IDEOGRAPH
    {0xE3F0, 0x8738}, //14341 #CJK UNIFIED IDEOGRAPH
    {0xE3F1, 0x8724}, //14342 #CJK UNIFIED IDEOGRAPH
    {0xE3F2, 0x871A}, //14343 #CJK UNIFIED IDEOGRAPH
    {0xE3F3, 0x8730}, //14344 #CJK UNIFIED IDEOGRAPH
    {0xE3F4, 0x8711}, //14345 #CJK UNIFIED IDEOGRAPH
    {0xE3F5, 0x88F7}, //14346 #CJK UNIFIED IDEOGRAPH
    {0xE3F6, 0x88E7}, //14347 #CJK UNIFIED IDEOGRAPH
    {0xE3F7, 0x88F1}, //14348 #CJK UNIFIED IDEOGRAPH
    {0xE3F8, 0x88F2}, //14349 #CJK UNIFIED IDEOGRAPH
    {0xE3F9, 0x88FA}, //14350 #CJK UNIFIED IDEOGRAPH
    {0xE3FA, 0x88FE}, //14351 #CJK UNIFIED IDEOGRAPH
    {0xE3FB, 0x88EE}, //14352 #CJK UNIFIED IDEOGRAPH
    {0xE3FC, 0x88FC}, //14353 #CJK UNIFIED IDEOGRAPH
    {0xE3FD, 0x88F6}, //14354 #CJK UNIFIED IDEOGRAPH
    {0xE3FE, 0x88FB}, //14355 #CJK UNIFIED IDEOGRAPH
    {0xE440, 0x88F0}, //14356 #CJK UNIFIED IDEOGRAPH
    {0xE441, 0x88EC}, //14357 #CJK UNIFIED IDEOGRAPH
    {0xE442, 0x88EB}, //14358 #CJK UNIFIED IDEOGRAPH
    {0xE443, 0x899D}, //14359 #CJK UNIFIED IDEOGRAPH
    {0xE444, 0x89A1}, //14360 #CJK UNIFIED IDEOGRAPH
    {0xE445, 0x899F}, //14361 #CJK UNIFIED IDEOGRAPH
    {0xE446, 0x899E}, //14362 #CJK UNIFIED IDEOGRAPH
    {0xE447, 0x89E9}, //14363 #CJK UNIFIED IDEOGRAPH
    {0xE448, 0x89EB}, //14364 #CJK UNIFIED IDEOGRAPH
    {0xE449, 0x89E8}, //14365 #CJK UNIFIED IDEOGRAPH
    {0xE44A, 0x8AAB}, //14366 #CJK UNIFIED IDEOGRAPH
    {0xE44B, 0x8A99}, //14367 #CJK UNIFIED IDEOGRAPH
    {0xE44C, 0x8A8B}, //14368 #CJK UNIFIED IDEOGRAPH
    {0xE44D, 0x8A92}, //14369 #CJK UNIFIED IDEOGRAPH
    {0xE44E, 0x8A8F}, //14370 #CJK UNIFIED IDEOGRAPH
    {0xE44F, 0x8A96}, //14371 #CJK UNIFIED IDEOGRAPH
    {0xE450, 0x8C3D}, //14372 #CJK UNIFIED IDEOGRAPH
    {0xE451, 0x8C68}, //14373 #CJK UNIFIED IDEOGRAPH
    {0xE452, 0x8C69}, //14374 #CJK UNIFIED IDEOGRAPH
    {0xE453, 0x8CD5}, //14375 #CJK UNIFIED IDEOGRAPH
    {0xE454, 0x8CCF}, //14376 #CJK UNIFIED IDEOGRAPH
    {0xE455, 0x8CD7}, //14377 #CJK UNIFIED IDEOGRAPH
    {0xE456, 0x8D96}, //14378 #CJK UNIFIED IDEOGRAPH
    {0xE457, 0x8E09}, //14379 #CJK UNIFIED IDEOGRAPH
    {0xE458, 0x8E02}, //14380 #CJK UNIFIED IDEOGRAPH
    {0xE459, 0x8DFF}, //14381 #CJK UNIFIED IDEOGRAPH
    {0xE45A, 0x8E0D}, //14382 #CJK UNIFIED IDEOGRAPH
    {0xE45B, 0x8DFD}, //14383 #CJK UNIFIED IDEOGRAPH
    {0xE45C, 0x8E0A}, //14384 #CJK UNIFIED IDEOGRAPH
    {0xE45D, 0x8E03}, //14385 #CJK UNIFIED IDEOGRAPH
    {0xE45E, 0x8E07}, //14386 #CJK UNIFIED IDEOGRAPH
    {0xE45F, 0x8E06}, //14387 #CJK UNIFIED IDEOGRAPH
    {0xE460, 0x8E05}, //14388 #CJK UNIFIED IDEOGRAPH
    {0xE461, 0x8DFE}, //14389 #CJK UNIFIED IDEOGRAPH
    {0xE462, 0x8E00}, //14390 #CJK UNIFIED IDEOGRAPH
    {0xE463, 0x8E04}, //14391 #CJK UNIFIED IDEOGRAPH
    {0xE464, 0x8F10}, //14392 #CJK UNIFIED IDEOGRAPH
    {0xE465, 0x8F11}, //14393 #CJK UNIFIED IDEOGRAPH
    {0xE466, 0x8F0E}, //14394 #CJK UNIFIED IDEOGRAPH
    {0xE467, 0x8F0D}, //14395 #CJK UNIFIED IDEOGRAPH
    {0xE468, 0x9123}, //14396 #CJK UNIFIED IDEOGRAPH
    {0xE469, 0x911C}, //14397 #CJK UNIFIED IDEOGRAPH
    {0xE46A, 0x9120}, //14398 #CJK UNIFIED IDEOGRAPH
    {0xE46B, 0x9122}, //14399 #CJK UNIFIED IDEOGRAPH
    {0xE46C, 0x911F}, //14400 #CJK UNIFIED IDEOGRAPH
    {0xE46D, 0x911D}, //14401 #CJK UNIFIED IDEOGRAPH
    {0xE46E, 0x911A}, //14402 #CJK UNIFIED IDEOGRAPH
    {0xE46F, 0x9124}, //14403 #CJK UNIFIED IDEOGRAPH
    {0xE470, 0x9121}, //14404 #CJK UNIFIED IDEOGRAPH
    {0xE471, 0x911B}, //14405 #CJK UNIFIED IDEOGRAPH
    {0xE472, 0x917A}, //14406 #CJK UNIFIED IDEOGRAPH
    {0xE473, 0x9172}, //14407 #CJK UNIFIED IDEOGRAPH
    {0xE474, 0x9179}, //14408 #CJK UNIFIED IDEOGRAPH
    {0xE475, 0x9173}, //14409 #CJK UNIFIED IDEOGRAPH
    {0xE476, 0x92A5}, //14410 #CJK UNIFIED IDEOGRAPH
    {0xE477, 0x92A4}, //14411 #CJK UNIFIED IDEOGRAPH
    {0xE478, 0x9276}, //14412 #CJK UNIFIED IDEOGRAPH
    {0xE479, 0x929B}, //14413 #CJK UNIFIED IDEOGRAPH
    {0xE47A, 0x927A}, //14414 #CJK UNIFIED IDEOGRAPH
    {0xE47B, 0x92A0}, //14415 #CJK UNIFIED IDEOGRAPH
    {0xE47C, 0x9294}, //14416 #CJK UNIFIED IDEOGRAPH
    {0xE47D, 0x92AA}, //14417 #CJK UNIFIED IDEOGRAPH
    {0xE47E, 0x928D}, //14418 #CJK UNIFIED IDEOGRAPH
    {0xE4A1, 0x92A6}, //14419 #CJK UNIFIED IDEOGRAPH
    {0xE4A2, 0x929A}, //14420 #CJK UNIFIED IDEOGRAPH
    {0xE4A3, 0x92AB}, //14421 #CJK UNIFIED IDEOGRAPH
    {0xE4A4, 0x9279}, //14422 #CJK UNIFIED IDEOGRAPH
    {0xE4A5, 0x9297}, //14423 #CJK UNIFIED IDEOGRAPH
    {0xE4A6, 0x927F}, //14424 #CJK UNIFIED IDEOGRAPH
    {0xE4A7, 0x92A3}, //14425 #CJK UNIFIED IDEOGRAPH
    {0xE4A8, 0x92EE}, //14426 #CJK UNIFIED IDEOGRAPH
    {0xE4A9, 0x928E}, //14427 #CJK UNIFIED IDEOGRAPH
    {0xE4AA, 0x9282}, //14428 #CJK UNIFIED IDEOGRAPH
    {0xE4AB, 0x9295}, //14429 #CJK UNIFIED IDEOGRAPH
    {0xE4AC, 0x92A2}, //14430 #CJK UNIFIED IDEOGRAPH
    {0xE4AD, 0x927D}, //14431 #CJK UNIFIED IDEOGRAPH
    {0xE4AE, 0x9288}, //14432 #CJK UNIFIED IDEOGRAPH
    {0xE4AF, 0x92A1}, //14433 #CJK UNIFIED IDEOGRAPH
    {0xE4B0, 0x928A}, //14434 #CJK UNIFIED IDEOGRAPH
    {0xE4B1, 0x9286}, //14435 #CJK UNIFIED IDEOGRAPH
    {0xE4B2, 0x928C}, //14436 #CJK UNIFIED IDEOGRAPH
    {0xE4B3, 0x9299}, //14437 #CJK UNIFIED IDEOGRAPH
    {0xE4B4, 0x92A7}, //14438 #CJK UNIFIED IDEOGRAPH
    {0xE4B5, 0x927E}, //14439 #CJK UNIFIED IDEOGRAPH
    {0xE4B6, 0x9287}, //14440 #CJK UNIFIED IDEOGRAPH
    {0xE4B7, 0x92A9}, //14441 #CJK UNIFIED IDEOGRAPH
    {0xE4B8, 0x929D}, //14442 #CJK UNIFIED IDEOGRAPH
    {0xE4B9, 0x928B}, //14443 #CJK UNIFIED IDEOGRAPH
    {0xE4BA, 0x922D}, //14444 #CJK UNIFIED IDEOGRAPH
    {0xE4BB, 0x969E}, //14445 #CJK UNIFIED IDEOGRAPH
    {0xE4BC, 0x96A1}, //14446 #CJK UNIFIED IDEOGRAPH
    {0xE4BD, 0x96FF}, //14447 #CJK UNIFIED IDEOGRAPH
    {0xE4BE, 0x9758}, //14448 #CJK UNIFIED IDEOGRAPH
    {0xE4BF, 0x977D}, //14449 #CJK UNIFIED IDEOGRAPH
    {0xE4C0, 0x977A}, //14450 #CJK UNIFIED IDEOGRAPH
    {0xE4C1, 0x977E}, //14451 #CJK UNIFIED IDEOGRAPH
    {0xE4C2, 0x9783}, //14452 #CJK UNIFIED IDEOGRAPH
    {0xE4C3, 0x9780}, //14453 #CJK UNIFIED IDEOGRAPH
    {0xE4C4, 0x9782}, //14454 #CJK UNIFIED IDEOGRAPH
    {0xE4C5, 0x977B}, //14455 #CJK UNIFIED IDEOGRAPH
    {0xE4C6, 0x9784}, //14456 #CJK UNIFIED IDEOGRAPH
    {0xE4C7, 0x9781}, //14457 #CJK UNIFIED IDEOGRAPH
    {0xE4C8, 0x977F}, //14458 #CJK UNIFIED IDEOGRAPH
    {0xE4C9, 0x97CE}, //14459 #CJK UNIFIED IDEOGRAPH
    {0xE4CA, 0x97CD}, //14460 #CJK UNIFIED IDEOGRAPH
    {0xE4CB, 0x9816}, //14461 #CJK UNIFIED IDEOGRAPH
    {0xE4CC, 0x98AD}, //14462 #CJK UNIFIED IDEOGRAPH
    {0xE4CD, 0x98AE}, //14463 #CJK UNIFIED IDEOGRAPH
    {0xE4CE, 0x9902}, //14464 #CJK UNIFIED IDEOGRAPH
    {0xE4CF, 0x9900}, //14465 #CJK UNIFIED IDEOGRAPH
    {0xE4D0, 0x9907}, //14466 #CJK UNIFIED IDEOGRAPH
    {0xE4D1, 0x999D}, //14467 #CJK UNIFIED IDEOGRAPH
    {0xE4D2, 0x999C}, //14468 #CJK UNIFIED IDEOGRAPH
    {0xE4D3, 0x99C3}, //14469 #CJK UNIFIED IDEOGRAPH
    {0xE4D4, 0x99B9}, //14470 #CJK UNIFIED IDEOGRAPH
    {0xE4D5, 0x99BB}, //14471 #CJK UNIFIED IDEOGRAPH
    {0xE4D6, 0x99BA}, //14472 #CJK UNIFIED IDEOGRAPH
    {0xE4D7, 0x99C2}, //14473 #CJK UNIFIED IDEOGRAPH
    {0xE4D8, 0x99BD}, //14474 #CJK UNIFIED IDEOGRAPH
    {0xE4D9, 0x99C7}, //14475 #CJK UNIFIED IDEOGRAPH
    {0xE4DA, 0x9AB1}, //14476 #CJK UNIFIED IDEOGRAPH
    {0xE4DB, 0x9AE3}, //14477 #CJK UNIFIED IDEOGRAPH
    {0xE4DC, 0x9AE7}, //14478 #CJK UNIFIED IDEOGRAPH
    {0xE4DD, 0x9B3E}, //14479 #CJK UNIFIED IDEOGRAPH
    {0xE4DE, 0x9B3F}, //14480 #CJK UNIFIED IDEOGRAPH
    {0xE4DF, 0x9B60}, //14481 #CJK UNIFIED IDEOGRAPH
    {0xE4E0, 0x9B61}, //14482 #CJK UNIFIED IDEOGRAPH
    {0xE4E1, 0x9B5F}, //14483 #CJK UNIFIED IDEOGRAPH
    {0xE4E2, 0x9CF1}, //14484 #CJK UNIFIED IDEOGRAPH
    {0xE4E3, 0x9CF2}, //14485 #CJK UNIFIED IDEOGRAPH
    {0xE4E4, 0x9CF5}, //14486 #CJK UNIFIED IDEOGRAPH
    {0xE4E5, 0x9EA7}, //14487 #CJK UNIFIED IDEOGRAPH
    {0xE4E6, 0x50FF}, //14488 #CJK UNIFIED IDEOGRAPH
    {0xE4E7, 0x5103}, //14489 #CJK UNIFIED IDEOGRAPH
    {0xE4E8, 0x5130}, //14490 #CJK UNIFIED IDEOGRAPH
    {0xE4E9, 0x50F8}, //14491 #CJK UNIFIED IDEOGRAPH
    {0xE4EA, 0x5106}, //14492 #CJK UNIFIED IDEOGRAPH
    {0xE4EB, 0x5107}, //14493 #CJK UNIFIED IDEOGRAPH
    {0xE4EC, 0x50F6}, //14494 #CJK UNIFIED IDEOGRAPH
    {0xE4ED, 0x50FE}, //14495 #CJK UNIFIED IDEOGRAPH
    {0xE4EE, 0x510B}, //14496 #CJK UNIFIED IDEOGRAPH
    {0xE4EF, 0x510C}, //14497 #CJK UNIFIED IDEOGRAPH
    {0xE4F0, 0x50FD}, //14498 #CJK UNIFIED IDEOGRAPH
    {0xE4F1, 0x510A}, //14499 #CJK UNIFIED IDEOGRAPH
    {0xE4F2, 0x528B}, //14500 #CJK UNIFIED IDEOGRAPH
    {0xE4F3, 0x528C}, //14501 #CJK UNIFIED IDEOGRAPH
    {0xE4F4, 0x52F1}, //14502 #CJK UNIFIED IDEOGRAPH
    {0xE4F5, 0x52EF}, //14503 #CJK UNIFIED IDEOGRAPH
    {0xE4F6, 0x5648}, //14504 #CJK UNIFIED IDEOGRAPH
    {0xE4F7, 0x5642}, //14505 #CJK UNIFIED IDEOGRAPH
    {0xE4F8, 0x564C}, //14506 #CJK UNIFIED IDEOGRAPH
    {0xE4F9, 0x5635}, //14507 #CJK UNIFIED IDEOGRAPH
    {0xE4FA, 0x5641}, //14508 #CJK UNIFIED IDEOGRAPH
    {0xE4FB, 0x564A}, //14509 #CJK UNIFIED IDEOGRAPH
    {0xE4FC, 0x5649}, //14510 #CJK UNIFIED IDEOGRAPH
    {0xE4FD, 0x5646}, //14511 #CJK UNIFIED IDEOGRAPH
    {0xE4FE, 0x5658}, //14512 #CJK UNIFIED IDEOGRAPH
    {0xE540, 0x565A}, //14513 #CJK UNIFIED IDEOGRAPH
    {0xE541, 0x5640}, //14514 #CJK UNIFIED IDEOGRAPH
    {0xE542, 0x5633}, //14515 #CJK UNIFIED IDEOGRAPH
    {0xE543, 0x563D}, //14516 #CJK UNIFIED IDEOGRAPH
    {0xE544, 0x562C}, //14517 #CJK UNIFIED IDEOGRAPH
    {0xE545, 0x563E}, //14518 #CJK UNIFIED IDEOGRAPH
    {0xE546, 0x5638}, //14519 #CJK UNIFIED IDEOGRAPH
    {0xE547, 0x562A}, //14520 #CJK UNIFIED IDEOGRAPH
    {0xE548, 0x563A}, //14521 #CJK UNIFIED IDEOGRAPH
    {0xE549, 0x571A}, //14522 #CJK UNIFIED IDEOGRAPH
    {0xE54A, 0x58AB}, //14523 #CJK UNIFIED IDEOGRAPH
    {0xE54B, 0x589D}, //14524 #CJK UNIFIED IDEOGRAPH
    {0xE54C, 0x58B1}, //14525 #CJK UNIFIED IDEOGRAPH
    {0xE54D, 0x58A0}, //14526 #CJK UNIFIED IDEOGRAPH
    {0xE54E, 0x58A3}, //14527 #CJK UNIFIED IDEOGRAPH
    {0xE54F, 0x58AF}, //14528 #CJK UNIFIED IDEOGRAPH
    {0xE550, 0x58AC}, //14529 #CJK UNIFIED IDEOGRAPH
    {0xE551, 0x58A5}, //14530 #CJK UNIFIED IDEOGRAPH
    {0xE552, 0x58A1}, //14531 #CJK UNIFIED IDEOGRAPH
    {0xE553, 0x58FF}, //14532 #CJK UNIFIED IDEOGRAPH
    {0xE554, 0x5AFF}, //14533 #CJK UNIFIED IDEOGRAPH
    {0xE555, 0x5AF4}, //14534 #CJK UNIFIED IDEOGRAPH
    {0xE556, 0x5AFD}, //14535 #CJK UNIFIED IDEOGRAPH
    {0xE557, 0x5AF7}, //14536 #CJK UNIFIED IDEOGRAPH
    {0xE558, 0x5AF6}, //14537 #CJK UNIFIED IDEOGRAPH
    {0xE559, 0x5B03}, //14538 #CJK UNIFIED IDEOGRAPH
    {0xE55A, 0x5AF8}, //14539 #CJK UNIFIED IDEOGRAPH
    {0xE55B, 0x5B02}, //14540 #CJK UNIFIED IDEOGRAPH
    {0xE55C, 0x5AF9}, //14541 #CJK UNIFIED IDEOGRAPH
    {0xE55D, 0x5B01}, //14542 #CJK UNIFIED IDEOGRAPH
    {0xE55E, 0x5B07}, //14543 #CJK UNIFIED IDEOGRAPH
    {0xE55F, 0x5B05}, //14544 #CJK UNIFIED IDEOGRAPH
    {0xE560, 0x5B0F}, //14545 #CJK UNIFIED IDEOGRAPH
    {0xE561, 0x5C67}, //14546 #CJK UNIFIED IDEOGRAPH
    {0xE562, 0x5D99}, //14547 #CJK UNIFIED IDEOGRAPH
    {0xE563, 0x5D97}, //14548 #CJK UNIFIED IDEOGRAPH
    {0xE564, 0x5D9F}, //14549 #CJK UNIFIED IDEOGRAPH
    {0xE565, 0x5D92}, //14550 #CJK UNIFIED IDEOGRAPH
    {0xE566, 0x5DA2}, //14551 #CJK UNIFIED IDEOGRAPH
    {0xE567, 0x5D93}, //14552 #CJK UNIFIED IDEOGRAPH
    {0xE568, 0x5D95}, //14553 #CJK UNIFIED IDEOGRAPH
    {0xE569, 0x5DA0}, //14554 #CJK UNIFIED IDEOGRAPH
    {0xE56A, 0x5D9C}, //14555 #CJK UNIFIED IDEOGRAPH
    {0xE56B, 0x5DA1}, //14556 #CJK UNIFIED IDEOGRAPH
    {0xE56C, 0x5D9A}, //14557 #CJK UNIFIED IDEOGRAPH
    {0xE56D, 0x5D9E}, //14558 #CJK UNIFIED IDEOGRAPH
    {0xE56E, 0x5E69}, //14559 #CJK UNIFIED IDEOGRAPH
    {0xE56F, 0x5E5D}, //14560 #CJK UNIFIED IDEOGRAPH
    {0xE570, 0x5E60}, //14561 #CJK UNIFIED IDEOGRAPH
    {0xE571, 0x5E5C}, //14562 #CJK UNIFIED IDEOGRAPH
    {0xE572, 0x7DF3}, //14563 #CJK UNIFIED IDEOGRAPH
    {0xE573, 0x5EDB}, //14564 #CJK UNIFIED IDEOGRAPH
    {0xE574, 0x5EDE}, //14565 #CJK UNIFIED IDEOGRAPH
    {0xE575, 0x5EE1}, //14566 #CJK UNIFIED IDEOGRAPH
    {0xE576, 0x5F49}, //14567 #CJK UNIFIED IDEOGRAPH
    {0xE577, 0x5FB2}, //14568 #CJK UNIFIED IDEOGRAPH
    {0xE578, 0x618B}, //14569 #CJK UNIFIED IDEOGRAPH
    {0xE579, 0x6183}, //14570 #CJK UNIFIED IDEOGRAPH
    {0xE57A, 0x6179}, //14571 #CJK UNIFIED IDEOGRAPH
    {0xE57B, 0x61B1}, //14572 #CJK UNIFIED IDEOGRAPH
    {0xE57C, 0x61B0}, //14573 #CJK UNIFIED IDEOGRAPH
    {0xE57D, 0x61A2}, //14574 #CJK UNIFIED IDEOGRAPH
    {0xE57E, 0x6189}, //14575 #CJK UNIFIED IDEOGRAPH
    {0xE5A1, 0x619B}, //14576 #CJK UNIFIED IDEOGRAPH
    {0xE5A2, 0x6193}, //14577 #CJK UNIFIED IDEOGRAPH
    {0xE5A3, 0x61AF}, //14578 #CJK UNIFIED IDEOGRAPH
    {0xE5A4, 0x61AD}, //14579 #CJK UNIFIED IDEOGRAPH
    {0xE5A5, 0x619F}, //14580 #CJK UNIFIED IDEOGRAPH
    {0xE5A6, 0x6192}, //14581 #CJK UNIFIED IDEOGRAPH
    {0xE5A7, 0x61AA}, //14582 #CJK UNIFIED IDEOGRAPH
    {0xE5A8, 0x61A1}, //14583 #CJK UNIFIED IDEOGRAPH
    {0xE5A9, 0x618D}, //14584 #CJK UNIFIED IDEOGRAPH
    {0xE5AA, 0x6166}, //14585 #CJK UNIFIED IDEOGRAPH
    {0xE5AB, 0x61B3}, //14586 #CJK UNIFIED IDEOGRAPH
    {0xE5AC, 0x622D}, //14587 #CJK UNIFIED IDEOGRAPH
    {0xE5AD, 0x646E}, //14588 #CJK UNIFIED IDEOGRAPH
    {0xE5AE, 0x6470}, //14589 #CJK UNIFIED IDEOGRAPH
    {0xE5AF, 0x6496}, //14590 #CJK UNIFIED IDEOGRAPH
    {0xE5B0, 0x64A0}, //14591 #CJK UNIFIED IDEOGRAPH
    {0xE5B1, 0x6485}, //14592 #CJK UNIFIED IDEOGRAPH
    {0xE5B2, 0x6497}, //14593 #CJK UNIFIED IDEOGRAPH
    {0xE5B3, 0x649C}, //14594 #CJK UNIFIED IDEOGRAPH
    {0xE5B4, 0x648F}, //14595 #CJK UNIFIED IDEOGRAPH
    {0xE5B5, 0x648B}, //14596 #CJK UNIFIED IDEOGRAPH
    {0xE5B6, 0x648A}, //14597 #CJK UNIFIED IDEOGRAPH
    {0xE5B7, 0x648C}, //14598 #CJK UNIFIED IDEOGRAPH
    {0xE5B8, 0x64A3}, //14599 #CJK UNIFIED IDEOGRAPH
    {0xE5B9, 0x649F}, //14600 #CJK UNIFIED IDEOGRAPH
    {0xE5BA, 0x6468}, //14601 #CJK UNIFIED IDEOGRAPH
    {0xE5BB, 0x64B1}, //14602 #CJK UNIFIED IDEOGRAPH
    {0xE5BC, 0x6498}, //14603 #CJK UNIFIED IDEOGRAPH
    {0xE5BD, 0x6576}, //14604 #CJK UNIFIED IDEOGRAPH
    {0xE5BE, 0x657A}, //14605 #CJK UNIFIED IDEOGRAPH
    {0xE5BF, 0x6579}, //14606 #CJK UNIFIED IDEOGRAPH
    {0xE5C0, 0x657B}, //14607 #CJK UNIFIED IDEOGRAPH
    {0xE5C1, 0x65B2}, //14608 #CJK UNIFIED IDEOGRAPH
    {0xE5C2, 0x65B3}, //14609 #CJK UNIFIED IDEOGRAPH
    {0xE5C3, 0x66B5}, //14610 #CJK UNIFIED IDEOGRAPH
    {0xE5C4, 0x66B0}, //14611 #CJK UNIFIED IDEOGRAPH
    {0xE5C5, 0x66A9}, //14612 #CJK UNIFIED IDEOGRAPH
    {0xE5C6, 0x66B2}, //14613 #CJK UNIFIED IDEOGRAPH
    {0xE5C7, 0x66B7}, //14614 #CJK UNIFIED IDEOGRAPH
    {0xE5C8, 0x66AA}, //14615 #CJK UNIFIED IDEOGRAPH
    {0xE5C9, 0x66AF}, //14616 #CJK UNIFIED IDEOGRAPH
    {0xE5CA, 0x6A00}, //14617 #CJK UNIFIED IDEOGRAPH
    {0xE5CB, 0x6A06}, //14618 #CJK UNIFIED IDEOGRAPH
    {0xE5CC, 0x6A17}, //14619 #CJK UNIFIED IDEOGRAPH
    {0xE5CD, 0x69E5}, //14620 #CJK UNIFIED IDEOGRAPH
    {0xE5CE, 0x69F8}, //14621 #CJK UNIFIED IDEOGRAPH
    {0xE5CF, 0x6A15}, //14622 #CJK UNIFIED IDEOGRAPH
    {0xE5D0, 0x69F1}, //14623 #CJK UNIFIED IDEOGRAPH
    {0xE5D1, 0x69E4}, //14624 #CJK UNIFIED IDEOGRAPH
    {0xE5D2, 0x6A20}, //14625 #CJK UNIFIED IDEOGRAPH
    {0xE5D3, 0x69FF}, //14626 #CJK UNIFIED IDEOGRAPH
    {0xE5D4, 0x69EC}, //14627 #CJK UNIFIED IDEOGRAPH
    {0xE5D5, 0x69E2}, //14628 #CJK UNIFIED IDEOGRAPH
    {0xE5D6, 0x6A1B}, //14629 #CJK UNIFIED IDEOGRAPH
    {0xE5D7, 0x6A1D}, //14630 #CJK UNIFIED IDEOGRAPH
    {0xE5D8, 0x69FE}, //14631 #CJK UNIFIED IDEOGRAPH
    {0xE5D9, 0x6A27}, //14632 #CJK UNIFIED IDEOGRAPH
    {0xE5DA, 0x69F2}, //14633 #CJK UNIFIED IDEOGRAPH
    {0xE5DB, 0x69EE}, //14634 #CJK UNIFIED IDEOGRAPH
    {0xE5DC, 0x6A14}, //14635 #CJK UNIFIED IDEOGRAPH
    {0xE5DD, 0x69F7}, //14636 #CJK UNIFIED IDEOGRAPH
    {0xE5DE, 0x69E7}, //14637 #CJK UNIFIED IDEOGRAPH
    {0xE5DF, 0x6A40}, //14638 #CJK UNIFIED IDEOGRAPH
    {0xE5E0, 0x6A08}, //14639 #CJK UNIFIED IDEOGRAPH
    {0xE5E1, 0x69E6}, //14640 #CJK UNIFIED IDEOGRAPH
    {0xE5E2, 0x69FB}, //14641 #CJK UNIFIED IDEOGRAPH
    {0xE5E3, 0x6A0D}, //14642 #CJK UNIFIED IDEOGRAPH
    {0xE5E4, 0x69FC}, //14643 #CJK UNIFIED IDEOGRAPH
    {0xE5E5, 0x69EB}, //14644 #CJK UNIFIED IDEOGRAPH
    {0xE5E6, 0x6A09}, //14645 #CJK UNIFIED IDEOGRAPH
    {0xE5E7, 0x6A04}, //14646 #CJK UNIFIED IDEOGRAPH
    {0xE5E8, 0x6A18}, //14647 #CJK UNIFIED IDEOGRAPH
    {0xE5E9, 0x6A25}, //14648 #CJK UNIFIED IDEOGRAPH
    {0xE5EA, 0x6A0F}, //14649 #CJK UNIFIED IDEOGRAPH
    {0xE5EB, 0x69F6}, //14650 #CJK UNIFIED IDEOGRAPH
    {0xE5EC, 0x6A26}, //14651 #CJK UNIFIED IDEOGRAPH
    {0xE5ED, 0x6A07}, //14652 #CJK UNIFIED IDEOGRAPH
    {0xE5EE, 0x69F4}, //14653 #CJK UNIFIED IDEOGRAPH
    {0xE5EF, 0x6A16}, //14654 #CJK UNIFIED IDEOGRAPH
    {0xE5F0, 0x6B51}, //14655 #CJK UNIFIED IDEOGRAPH
    {0xE5F1, 0x6BA5}, //14656 #CJK UNIFIED IDEOGRAPH
    {0xE5F2, 0x6BA3}, //14657 #CJK UNIFIED IDEOGRAPH
    {0xE5F3, 0x6BA2}, //14658 #CJK UNIFIED IDEOGRAPH
    {0xE5F4, 0x6BA6}, //14659 #CJK UNIFIED IDEOGRAPH
    {0xE5F5, 0x6C01}, //14660 #CJK UNIFIED IDEOGRAPH
    {0xE5F6, 0x6C00}, //14661 #CJK UNIFIED IDEOGRAPH
    {0xE5F7, 0x6BFF}, //14662 #CJK UNIFIED IDEOGRAPH
    {0xE5F8, 0x6C02}, //14663 #CJK UNIFIED IDEOGRAPH
    {0xE5F9, 0x6F41}, //14664 #CJK UNIFIED IDEOGRAPH
    {0xE5FA, 0x6F26}, //14665 #CJK UNIFIED IDEOGRAPH
    {0xE5FB, 0x6F7E}, //14666 #CJK UNIFIED IDEOGRAPH
    {0xE5FC, 0x6F87}, //14667 #CJK UNIFIED IDEOGRAPH
    {0xE5FD, 0x6FC6}, //14668 #CJK UNIFIED IDEOGRAPH
    {0xE5FE, 0x6F92}, //14669 #CJK UNIFIED IDEOGRAPH
    {0xE640, 0x6F8D}, //14670 #CJK UNIFIED IDEOGRAPH
    {0xE641, 0x6F89}, //14671 #CJK UNIFIED IDEOGRAPH
    {0xE642, 0x6F8C}, //14672 #CJK UNIFIED IDEOGRAPH
    {0xE643, 0x6F62}, //14673 #CJK UNIFIED IDEOGRAPH
    {0xE644, 0x6F4F}, //14674 #CJK UNIFIED IDEOGRAPH
    {0xE645, 0x6F85}, //14675 #CJK UNIFIED IDEOGRAPH
    {0xE646, 0x6F5A}, //14676 #CJK UNIFIED IDEOGRAPH
    {0xE647, 0x6F96}, //14677 #CJK UNIFIED IDEOGRAPH
    {0xE648, 0x6F76}, //14678 #CJK UNIFIED IDEOGRAPH
    {0xE649, 0x6F6C}, //14679 #CJK UNIFIED IDEOGRAPH
    {0xE64A, 0x6F82}, //14680 #CJK UNIFIED IDEOGRAPH
    {0xE64B, 0x6F55}, //14681 #CJK UNIFIED IDEOGRAPH
    {0xE64C, 0x6F72}, //14682 #CJK UNIFIED IDEOGRAPH
    {0xE64D, 0x6F52}, //14683 #CJK UNIFIED IDEOGRAPH
    {0xE64E, 0x6F50}, //14684 #CJK UNIFIED IDEOGRAPH
    {0xE64F, 0x6F57}, //14685 #CJK UNIFIED IDEOGRAPH
    {0xE650, 0x6F94}, //14686 #CJK UNIFIED IDEOGRAPH
    {0xE651, 0x6F93}, //14687 #CJK UNIFIED IDEOGRAPH
    {0xE652, 0x6F5D}, //14688 #CJK UNIFIED IDEOGRAPH
    {0xE653, 0x6F00}, //14689 #CJK UNIFIED IDEOGRAPH
    {0xE654, 0x6F61}, //14690 #CJK UNIFIED IDEOGRAPH
    {0xE655, 0x6F6B}, //14691 #CJK UNIFIED IDEOGRAPH
    {0xE656, 0x6F7D}, //14692 #CJK UNIFIED IDEOGRAPH
    {0xE657, 0x6F67}, //14693 #CJK UNIFIED IDEOGRAPH
    {0xE658, 0x6F90}, //14694 #CJK UNIFIED IDEOGRAPH
    {0xE659, 0x6F53}, //14695 #CJK UNIFIED IDEOGRAPH
    {0xE65A, 0x6F8B}, //14696 #CJK UNIFIED IDEOGRAPH
    {0xE65B, 0x6F69}, //14697 #CJK UNIFIED IDEOGRAPH
    {0xE65C, 0x6F7F}, //14698 #CJK UNIFIED IDEOGRAPH
    {0xE65D, 0x6F95}, //14699 #CJK UNIFIED IDEOGRAPH
    {0xE65E, 0x6F63}, //14700 #CJK UNIFIED IDEOGRAPH
    {0xE65F, 0x6F77}, //14701 #CJK UNIFIED IDEOGRAPH
    {0xE660, 0x6F6A}, //14702 #CJK UNIFIED IDEOGRAPH
    {0xE661, 0x6F7B}, //14703 #CJK UNIFIED IDEOGRAPH
    {0xE662, 0x71B2}, //14704 #CJK UNIFIED IDEOGRAPH
    {0xE663, 0x71AF}, //14705 #CJK UNIFIED IDEOGRAPH
    {0xE664, 0x719B}, //14706 #CJK UNIFIED IDEOGRAPH
    {0xE665, 0x71B0}, //14707 #CJK UNIFIED IDEOGRAPH
    {0xE666, 0x71A0}, //14708 #CJK UNIFIED IDEOGRAPH
    {0xE667, 0x719A}, //14709 #CJK UNIFIED IDEOGRAPH
    {0xE668, 0x71A9}, //14710 #CJK UNIFIED IDEOGRAPH
    {0xE669, 0x71B5}, //14711 #CJK UNIFIED IDEOGRAPH
    {0xE66A, 0x719D}, //14712 #CJK UNIFIED IDEOGRAPH
    {0xE66B, 0x71A5}, //14713 #CJK UNIFIED IDEOGRAPH
    {0xE66C, 0x719E}, //14714 #CJK UNIFIED IDEOGRAPH
    {0xE66D, 0x71A4}, //14715 #CJK UNIFIED IDEOGRAPH
    {0xE66E, 0x71A1}, //14716 #CJK UNIFIED IDEOGRAPH
    {0xE66F, 0x71AA}, //14717 #CJK UNIFIED IDEOGRAPH
    {0xE670, 0x719C}, //14718 #CJK UNIFIED IDEOGRAPH
    {0xE671, 0x71A7}, //14719 #CJK UNIFIED IDEOGRAPH
    {0xE672, 0x71B3}, //14720 #CJK UNIFIED IDEOGRAPH
    {0xE673, 0x7298}, //14721 #CJK UNIFIED IDEOGRAPH
    {0xE674, 0x729A}, //14722 #CJK UNIFIED IDEOGRAPH
    {0xE675, 0x7358}, //14723 #CJK UNIFIED IDEOGRAPH
    {0xE676, 0x7352}, //14724 #CJK UNIFIED IDEOGRAPH
    {0xE677, 0x735E}, //14725 #CJK UNIFIED IDEOGRAPH
    {0xE678, 0x735F}, //14726 #CJK UNIFIED IDEOGRAPH
    {0xE679, 0x7360}, //14727 #CJK UNIFIED IDEOGRAPH
    {0xE67A, 0x735D}, //14728 #CJK UNIFIED IDEOGRAPH
    {0xE67B, 0x735B}, //14729 #CJK UNIFIED IDEOGRAPH
    {0xE67C, 0x7361}, //14730 #CJK UNIFIED IDEOGRAPH
    {0xE67D, 0x735A}, //14731 #CJK UNIFIED IDEOGRAPH
    {0xE67E, 0x7359}, //14732 #CJK UNIFIED IDEOGRAPH
    {0xE6A1, 0x7362}, //14733 #CJK UNIFIED IDEOGRAPH
    {0xE6A2, 0x7487}, //14734 #CJK UNIFIED IDEOGRAPH
    {0xE6A3, 0x7489}, //14735 #CJK UNIFIED IDEOGRAPH
    {0xE6A4, 0x748A}, //14736 #CJK UNIFIED IDEOGRAPH
    {0xE6A5, 0x7486}, //14737 #CJK UNIFIED IDEOGRAPH
    {0xE6A6, 0x7481}, //14738 #CJK UNIFIED IDEOGRAPH
    {0xE6A7, 0x747D}, //14739 #CJK UNIFIED IDEOGRAPH
    {0xE6A8, 0x7485}, //14740 #CJK UNIFIED IDEOGRAPH
    {0xE6A9, 0x7488}, //14741 #CJK UNIFIED IDEOGRAPH
    {0xE6AA, 0x747C}, //14742 #CJK UNIFIED IDEOGRAPH
    {0xE6AB, 0x7479}, //14743 #CJK UNIFIED IDEOGRAPH
    {0xE6AC, 0x7508}, //14744 #CJK UNIFIED IDEOGRAPH
    {0xE6AD, 0x7507}, //14745 #CJK UNIFIED IDEOGRAPH
    {0xE6AE, 0x757E}, //14746 #CJK UNIFIED IDEOGRAPH
    {0xE6AF, 0x7625}, //14747 #CJK UNIFIED IDEOGRAPH
    {0xE6B0, 0x761E}, //14748 #CJK UNIFIED IDEOGRAPH
    {0xE6B1, 0x7619}, //14749 #CJK UNIFIED IDEOGRAPH
    {0xE6B2, 0x761D}, //14750 #CJK UNIFIED IDEOGRAPH
    {0xE6B3, 0x761C}, //14751 #CJK UNIFIED IDEOGRAPH
    {0xE6B4, 0x7623}, //14752 #CJK UNIFIED IDEOGRAPH
    {0xE6B5, 0x761A}, //14753 #CJK UNIFIED IDEOGRAPH
    {0xE6B6, 0x7628}, //14754 #CJK UNIFIED IDEOGRAPH
    {0xE6B7, 0x761B}, //14755 #CJK UNIFIED IDEOGRAPH
    {0xE6B8, 0x769C}, //14756 #CJK UNIFIED IDEOGRAPH
    {0xE6B9, 0x769D}, //14757 #CJK UNIFIED IDEOGRAPH
    {0xE6BA, 0x769E}, //14758 #CJK UNIFIED IDEOGRAPH
    {0xE6BB, 0x769B}, //14759 #CJK UNIFIED IDEOGRAPH
    {0xE6BC, 0x778D}, //14760 #CJK UNIFIED IDEOGRAPH
    {0xE6BD, 0x778F}, //14761 #CJK UNIFIED IDEOGRAPH
    {0xE6BE, 0x7789}, //14762 #CJK UNIFIED IDEOGRAPH
    {0xE6BF, 0x7788}, //14763 #CJK UNIFIED IDEOGRAPH
    {0xE6C0, 0x78CD}, //14764 #CJK UNIFIED IDEOGRAPH
    {0xE6C1, 0x78BB}, //14765 #CJK UNIFIED IDEOGRAPH
    {0xE6C2, 0x78CF}, //14766 #CJK UNIFIED IDEOGRAPH
    {0xE6C3, 0x78CC}, //14767 #CJK UNIFIED IDEOGRAPH
    {0xE6C4, 0x78D1}, //14768 #CJK UNIFIED IDEOGRAPH
    {0xE6C5, 0x78CE}, //14769 #CJK UNIFIED IDEOGRAPH
    {0xE6C6, 0x78D4}, //14770 #CJK UNIFIED IDEOGRAPH
    {0xE6C7, 0x78C8}, //14771 #CJK UNIFIED IDEOGRAPH
    {0xE6C8, 0x78C3}, //14772 #CJK UNIFIED IDEOGRAPH
    {0xE6C9, 0x78C4}, //14773 #CJK UNIFIED IDEOGRAPH
    {0xE6CA, 0x78C9}, //14774 #CJK UNIFIED IDEOGRAPH
    {0xE6CB, 0x799A}, //14775 #CJK UNIFIED IDEOGRAPH
    {0xE6CC, 0x79A1}, //14776 #CJK UNIFIED IDEOGRAPH
    {0xE6CD, 0x79A0}, //14777 #CJK UNIFIED IDEOGRAPH
    {0xE6CE, 0x799C}, //14778 #CJK UNIFIED IDEOGRAPH
    {0xE6CF, 0x79A2}, //14779 #CJK UNIFIED IDEOGRAPH
    {0xE6D0, 0x799B}, //14780 #CJK UNIFIED IDEOGRAPH
    {0xE6D1, 0x6B76}, //14781 #CJK UNIFIED IDEOGRAPH
    {0xE6D2, 0x7A39}, //14782 #CJK UNIFIED IDEOGRAPH
    {0xE6D3, 0x7AB2}, //14783 #CJK UNIFIED IDEOGRAPH
    {0xE6D4, 0x7AB4}, //14784 #CJK UNIFIED IDEOGRAPH
    {0xE6D5, 0x7AB3}, //14785 #CJK UNIFIED IDEOGRAPH
    {0xE6D6, 0x7BB7}, //14786 #CJK UNIFIED IDEOGRAPH
    {0xE6D7, 0x7BCB}, //14787 #CJK UNIFIED IDEOGRAPH
    {0xE6D8, 0x7BBE}, //14788 #CJK UNIFIED IDEOGRAPH
    {0xE6D9, 0x7BAC}, //14789 #CJK UNIFIED IDEOGRAPH
    {0xE6DA, 0x7BCE}, //14790 #CJK UNIFIED IDEOGRAPH
    {0xE6DB, 0x7BAF}, //14791 #CJK UNIFIED IDEOGRAPH
    {0xE6DC, 0x7BB9}, //14792 #CJK UNIFIED IDEOGRAPH
    {0xE6DD, 0x7BCA}, //14793 #CJK UNIFIED IDEOGRAPH
    {0xE6DE, 0x7BB5}, //14794 #CJK UNIFIED IDEOGRAPH
    {0xE6DF, 0x7CC5}, //14795 #CJK UNIFIED IDEOGRAPH
    {0xE6E0, 0x7CC8}, //14796 #CJK UNIFIED IDEOGRAPH
    {0xE6E1, 0x7CCC}, //14797 #CJK UNIFIED IDEOGRAPH
    {0xE6E2, 0x7CCB}, //14798 #CJK UNIFIED IDEOGRAPH
    {0xE6E3, 0x7DF7}, //14799 #CJK UNIFIED IDEOGRAPH
    {0xE6E4, 0x7DDB}, //14800 #CJK UNIFIED IDEOGRAPH
    {0xE6E5, 0x7DEA}, //14801 #CJK UNIFIED IDEOGRAPH
    {0xE6E6, 0x7DE7}, //14802 #CJK UNIFIED IDEOGRAPH
    {0xE6E7, 0x7DD7}, //14803 #CJK UNIFIED IDEOGRAPH
    {0xE6E8, 0x7DE1}, //14804 #CJK UNIFIED IDEOGRAPH
    {0xE6E9, 0x7E03}, //14805 #CJK UNIFIED IDEOGRAPH
    {0xE6EA, 0x7DFA}, //14806 #CJK UNIFIED IDEOGRAPH
    {0xE6EB, 0x7DE6}, //14807 #CJK UNIFIED IDEOGRAPH
    {0xE6EC, 0x7DF6}, //14808 #CJK UNIFIED IDEOGRAPH
    {0xE6ED, 0x7DF1}, //14809 #CJK UNIFIED IDEOGRAPH
    {0xE6EE, 0x7DF0}, //14810 #CJK UNIFIED IDEOGRAPH
    {0xE6EF, 0x7DEE}, //14811 #CJK UNIFIED IDEOGRAPH
    {0xE6F0, 0x7DDF}, //14812 #CJK UNIFIED IDEOGRAPH
    {0xE6F1, 0x7F76}, //14813 #CJK UNIFIED IDEOGRAPH
    {0xE6F2, 0x7FAC}, //14814 #CJK UNIFIED IDEOGRAPH
    {0xE6F3, 0x7FB0}, //14815 #CJK UNIFIED IDEOGRAPH
    {0xE6F4, 0x7FAD}, //14816 #CJK UNIFIED IDEOGRAPH
    {0xE6F5, 0x7FED}, //14817 #CJK UNIFIED IDEOGRAPH
    {0xE6F6, 0x7FEB}, //14818 #CJK UNIFIED IDEOGRAPH
    {0xE6F7, 0x7FEA}, //14819 #CJK UNIFIED IDEOGRAPH
    {0xE6F8, 0x7FEC}, //14820 #CJK UNIFIED IDEOGRAPH
    {0xE6F9, 0x7FE6}, //14821 #CJK UNIFIED IDEOGRAPH
    {0xE6FA, 0x7FE8}, //14822 #CJK UNIFIED IDEOGRAPH
    {0xE6FB, 0x8064}, //14823 #CJK UNIFIED IDEOGRAPH
    {0xE6FC, 0x8067}, //14824 #CJK UNIFIED IDEOGRAPH
    {0xE6FD, 0x81A3}, //14825 #CJK UNIFIED IDEOGRAPH
    {0xE6FE, 0x819F}, //14826 #CJK UNIFIED IDEOGRAPH
    {0xE740, 0x819E}, //14827 #CJK UNIFIED IDEOGRAPH
    {0xE741, 0x8195}, //14828 #CJK UNIFIED IDEOGRAPH
    {0xE742, 0x81A2}, //14829 #CJK UNIFIED IDEOGRAPH
    {0xE743, 0x8199}, //14830 #CJK UNIFIED IDEOGRAPH
    {0xE744, 0x8197}, //14831 #CJK UNIFIED IDEOGRAPH
    {0xE745, 0x8216}, //14832 #CJK UNIFIED IDEOGRAPH
    {0xE746, 0x824F}, //14833 #CJK UNIFIED IDEOGRAPH
    {0xE747, 0x8253}, //14834 #CJK UNIFIED IDEOGRAPH
    {0xE748, 0x8252}, //14835 #CJK UNIFIED IDEOGRAPH
    {0xE749, 0x8250}, //14836 #CJK UNIFIED IDEOGRAPH
    {0xE74A, 0x824E}, //14837 #CJK UNIFIED IDEOGRAPH
    {0xE74B, 0x8251}, //14838 #CJK UNIFIED IDEOGRAPH
    {0xE74C, 0x8524}, //14839 #CJK UNIFIED IDEOGRAPH
    {0xE74D, 0x853B}, //14840 #CJK UNIFIED IDEOGRAPH
    {0xE74E, 0x850F}, //14841 #CJK UNIFIED IDEOGRAPH
    {0xE74F, 0x8500}, //14842 #CJK UNIFIED IDEOGRAPH
    {0xE750, 0x8529}, //14843 #CJK UNIFIED IDEOGRAPH
    {0xE751, 0x850E}, //14844 #CJK UNIFIED IDEOGRAPH
    {0xE752, 0x8509}, //14845 #CJK UNIFIED IDEOGRAPH
    {0xE753, 0x850D}, //14846 #CJK UNIFIED IDEOGRAPH
    {0xE754, 0x851F}, //14847 #CJK UNIFIED IDEOGRAPH
    {0xE755, 0x850A}, //14848 #CJK UNIFIED IDEOGRAPH
    {0xE756, 0x8527}, //14849 #CJK UNIFIED IDEOGRAPH
    {0xE757, 0x851C}, //14850 #CJK UNIFIED IDEOGRAPH
    {0xE758, 0x84FB}, //14851 #CJK UNIFIED IDEOGRAPH
    {0xE759, 0x852B}, //14852 #CJK UNIFIED IDEOGRAPH
    {0xE75A, 0x84FA}, //14853 #CJK UNIFIED IDEOGRAPH
    {0xE75B, 0x8508}, //14854 #CJK UNIFIED IDEOGRAPH
    {0xE75C, 0x850C}, //14855 #CJK UNIFIED IDEOGRAPH
    {0xE75D, 0x84F4}, //14856 #CJK UNIFIED IDEOGRAPH
    {0xE75E, 0x852A}, //14857 #CJK UNIFIED IDEOGRAPH
    {0xE75F, 0x84F2}, //14858 #CJK UNIFIED IDEOGRAPH
    {0xE760, 0x8515}, //14859 #CJK UNIFIED IDEOGRAPH
    {0xE761, 0x84F7}, //14860 #CJK UNIFIED IDEOGRAPH
    {0xE762, 0x84EB}, //14861 #CJK UNIFIED IDEOGRAPH
    {0xE763, 0x84F3}, //14862 #CJK UNIFIED IDEOGRAPH
    {0xE764, 0x84FC}, //14863 #CJK UNIFIED IDEOGRAPH
    {0xE765, 0x8512}, //14864 #CJK UNIFIED IDEOGRAPH
    {0xE766, 0x84EA}, //14865 #CJK UNIFIED IDEOGRAPH
    {0xE767, 0x84E9}, //14866 #CJK UNIFIED IDEOGRAPH
    {0xE768, 0x8516}, //14867 #CJK UNIFIED IDEOGRAPH
    {0xE769, 0x84FE}, //14868 #CJK UNIFIED IDEOGRAPH
    {0xE76A, 0x8528}, //14869 #CJK UNIFIED IDEOGRAPH
    {0xE76B, 0x851D}, //14870 #CJK UNIFIED IDEOGRAPH
    {0xE76C, 0x852E}, //14871 #CJK UNIFIED IDEOGRAPH
    {0xE76D, 0x8502}, //14872 #CJK UNIFIED IDEOGRAPH
    {0xE76E, 0x84FD}, //14873 #CJK UNIFIED IDEOGRAPH
    {0xE76F, 0x851E}, //14874 #CJK UNIFIED IDEOGRAPH
    {0xE770, 0x84F6}, //14875 #CJK UNIFIED IDEOGRAPH
    {0xE771, 0x8531}, //14876 #CJK UNIFIED IDEOGRAPH
    {0xE772, 0x8526}, //14877 #CJK UNIFIED IDEOGRAPH
    {0xE773, 0x84E7}, //14878 #CJK UNIFIED IDEOGRAPH
    {0xE774, 0x84E8}, //14879 #CJK UNIFIED IDEOGRAPH
    {0xE775, 0x84F0}, //14880 #CJK UNIFIED IDEOGRAPH
    {0xE776, 0x84EF}, //14881 #CJK UNIFIED IDEOGRAPH
    {0xE777, 0x84F9}, //14882 #CJK UNIFIED IDEOGRAPH
    {0xE778, 0x8518}, //14883 #CJK UNIFIED IDEOGRAPH
    {0xE779, 0x8520}, //14884 #CJK UNIFIED IDEOGRAPH
    {0xE77A, 0x8530}, //14885 #CJK UNIFIED IDEOGRAPH
    {0xE77B, 0x850B}, //14886 #CJK UNIFIED IDEOGRAPH
    {0xE77C, 0x8519}, //14887 #CJK UNIFIED IDEOGRAPH
    {0xE77D, 0x852F}, //14888 #CJK UNIFIED IDEOGRAPH
    {0xE77E, 0x8662}, //14889 #CJK UNIFIED IDEOGRAPH
    {0xE7A1, 0x8756}, //14890 #CJK UNIFIED IDEOGRAPH
    {0xE7A2, 0x8763}, //14891 #CJK UNIFIED IDEOGRAPH
    {0xE7A3, 0x8764}, //14892 #CJK UNIFIED IDEOGRAPH
    {0xE7A4, 0x8777}, //14893 #CJK UNIFIED IDEOGRAPH
    {0xE7A5, 0x87E1}, //14894 #CJK UNIFIED IDEOGRAPH
    {0xE7A6, 0x8773}, //14895 #CJK UNIFIED IDEOGRAPH
    {0xE7A7, 0x8758}, //14896 #CJK UNIFIED IDEOGRAPH
    {0xE7A8, 0x8754}, //14897 #CJK UNIFIED IDEOGRAPH
    {0xE7A9, 0x875B}, //14898 #CJK UNIFIED IDEOGRAPH
    {0xE7AA, 0x8752}, //14899 #CJK UNIFIED IDEOGRAPH
    {0xE7AB, 0x8761}, //14900 #CJK UNIFIED IDEOGRAPH
    {0xE7AC, 0x875A}, //14901 #CJK UNIFIED IDEOGRAPH
    {0xE7AD, 0x8751}, //14902 #CJK UNIFIED IDEOGRAPH
    {0xE7AE, 0x875E}, //14903 #CJK UNIFIED IDEOGRAPH
    {0xE7AF, 0x876D}, //14904 #CJK UNIFIED IDEOGRAPH
    {0xE7B0, 0x876A}, //14905 #CJK UNIFIED IDEOGRAPH
    {0xE7B1, 0x8750}, //14906 #CJK UNIFIED IDEOGRAPH
    {0xE7B2, 0x874E}, //14907 #CJK UNIFIED IDEOGRAPH
    {0xE7B3, 0x875F}, //14908 #CJK UNIFIED IDEOGRAPH
    {0xE7B4, 0x875D}, //14909 #CJK UNIFIED IDEOGRAPH
    {0xE7B5, 0x876F}, //14910 #CJK UNIFIED IDEOGRAPH
    {0xE7B6, 0x876C}, //14911 #CJK UNIFIED IDEOGRAPH
    {0xE7B7, 0x877A}, //14912 #CJK UNIFIED IDEOGRAPH
    {0xE7B8, 0x876E}, //14913 #CJK UNIFIED IDEOGRAPH
    {0xE7B9, 0x875C}, //14914 #CJK UNIFIED IDEOGRAPH
    {0xE7BA, 0x8765}, //14915 #CJK UNIFIED IDEOGRAPH
    {0xE7BB, 0x874F}, //14916 #CJK UNIFIED IDEOGRAPH
    {0xE7BC, 0x877B}, //14917 #CJK UNIFIED IDEOGRAPH
    {0xE7BD, 0x8775}, //14918 #CJK UNIFIED IDEOGRAPH
    {0xE7BE, 0x8762}, //14919 #CJK UNIFIED IDEOGRAPH
    {0xE7BF, 0x8767}, //14920 #CJK UNIFIED IDEOGRAPH
    {0xE7C0, 0x8769}, //14921 #CJK UNIFIED IDEOGRAPH
    {0xE7C1, 0x885A}, //14922 #CJK UNIFIED IDEOGRAPH
    {0xE7C2, 0x8905}, //14923 #CJK UNIFIED IDEOGRAPH
    {0xE7C3, 0x890C}, //14924 #CJK UNIFIED IDEOGRAPH
    {0xE7C4, 0x8914}, //14925 #CJK UNIFIED IDEOGRAPH
    {0xE7C5, 0x890B}, //14926 #CJK UNIFIED IDEOGRAPH
    {0xE7C6, 0x8917}, //14927 #CJK UNIFIED IDEOGRAPH
    {0xE7C7, 0x8918}, //14928 #CJK UNIFIED IDEOGRAPH
    {0xE7C8, 0x8919}, //14929 #CJK UNIFIED IDEOGRAPH
    {0xE7C9, 0x8906}, //14930 #CJK UNIFIED IDEOGRAPH
    {0xE7CA, 0x8916}, //14931 #CJK UNIFIED IDEOGRAPH
    {0xE7CB, 0x8911}, //14932 #CJK UNIFIED IDEOGRAPH
    {0xE7CC, 0x890E}, //14933 #CJK UNIFIED IDEOGRAPH
    {0xE7CD, 0x8909}, //14934 #CJK UNIFIED IDEOGRAPH
    {0xE7CE, 0x89A2}, //14935 #CJK UNIFIED IDEOGRAPH
    {0xE7CF, 0x89A4}, //14936 #CJK UNIFIED IDEOGRAPH
    {0xE7D0, 0x89A3}, //14937 #CJK UNIFIED IDEOGRAPH
    {0xE7D1, 0x89ED}, //14938 #CJK UNIFIED IDEOGRAPH
    {0xE7D2, 0x89F0}, //14939 #CJK UNIFIED IDEOGRAPH
    {0xE7D3, 0x89EC}, //14940 #CJK UNIFIED IDEOGRAPH
    {0xE7D4, 0x8ACF}, //14941 #CJK UNIFIED IDEOGRAPH
    {0xE7D5, 0x8AC6}, //14942 #CJK UNIFIED IDEOGRAPH
    {0xE7D6, 0x8AB8}, //14943 #CJK UNIFIED IDEOGRAPH
    {0xE7D7, 0x8AD3}, //14944 #CJK UNIFIED IDEOGRAPH
    {0xE7D8, 0x8AD1}, //14945 #CJK UNIFIED IDEOGRAPH
    {0xE7D9, 0x8AD4}, //14946 #CJK UNIFIED IDEOGRAPH
    {0xE7DA, 0x8AD5}, //14947 #CJK UNIFIED IDEOGRAPH
    {0xE7DB, 0x8ABB}, //14948 #CJK UNIFIED IDEOGRAPH
    {0xE7DC, 0x8AD7}, //14949 #CJK UNIFIED IDEOGRAPH
    {0xE7DD, 0x8ABE}, //14950 #CJK UNIFIED IDEOGRAPH
    {0xE7DE, 0x8AC0}, //14951 #CJK UNIFIED IDEOGRAPH
    {0xE7DF, 0x8AC5}, //14952 #CJK UNIFIED IDEOGRAPH
    {0xE7E0, 0x8AD8}, //14953 #CJK UNIFIED IDEOGRAPH
    {0xE7E1, 0x8AC3}, //14954 #CJK UNIFIED IDEOGRAPH
    {0xE7E2, 0x8ABA}, //14955 #CJK UNIFIED IDEOGRAPH
    {0xE7E3, 0x8ABD}, //14956 #CJK UNIFIED IDEOGRAPH
    {0xE7E4, 0x8AD9}, //14957 #CJK UNIFIED IDEOGRAPH
    {0xE7E5, 0x8C3E}, //14958 #CJK UNIFIED IDEOGRAPH
    {0xE7E6, 0x8C4D}, //14959 #CJK UNIFIED IDEOGRAPH
    {0xE7E7, 0x8C8F}, //14960 #CJK UNIFIED IDEOGRAPH
    {0xE7E8, 0x8CE5}, //14961 #CJK UNIFIED IDEOGRAPH
    {0xE7E9, 0x8CDF}, //14962 #CJK UNIFIED IDEOGRAPH
    {0xE7EA, 0x8CD9}, //14963 #CJK UNIFIED IDEOGRAPH
    {0xE7EB, 0x8CE8}, //14964 #CJK UNIFIED IDEOGRAPH
    {0xE7EC, 0x8CDA}, //14965 #CJK UNIFIED IDEOGRAPH
    {0xE7ED, 0x8CDD}, //14966 #CJK UNIFIED IDEOGRAPH
    {0xE7EE, 0x8CE7}, //14967 #CJK UNIFIED IDEOGRAPH
    {0xE7EF, 0x8DA0}, //14968 #CJK UNIFIED IDEOGRAPH
    {0xE7F0, 0x8D9C}, //14969 #CJK UNIFIED IDEOGRAPH
    {0xE7F1, 0x8DA1}, //14970 #CJK UNIFIED IDEOGRAPH
    {0xE7F2, 0x8D9B}, //14971 #CJK UNIFIED IDEOGRAPH
    {0xE7F3, 0x8E20}, //14972 #CJK UNIFIED IDEOGRAPH
    {0xE7F4, 0x8E23}, //14973 #CJK UNIFIED IDEOGRAPH
    {0xE7F5, 0x8E25}, //14974 #CJK UNIFIED IDEOGRAPH
    {0xE7F6, 0x8E24}, //14975 #CJK UNIFIED IDEOGRAPH
    {0xE7F7, 0x8E2E}, //14976 #CJK UNIFIED IDEOGRAPH
    {0xE7F8, 0x8E15}, //14977 #CJK UNIFIED IDEOGRAPH
    {0xE7F9, 0x8E1B}, //14978 #CJK UNIFIED IDEOGRAPH
    {0xE7FA, 0x8E16}, //14979 #CJK UNIFIED IDEOGRAPH
    {0xE7FB, 0x8E11}, //14980 #CJK UNIFIED IDEOGRAPH
    {0xE7FC, 0x8E19}, //14981 #CJK UNIFIED IDEOGRAPH
    {0xE7FD, 0x8E26}, //14982 #CJK UNIFIED IDEOGRAPH
    {0xE7FE, 0x8E27}, //14983 #CJK UNIFIED IDEOGRAPH
    {0xE840, 0x8E14}, //14984 #CJK UNIFIED IDEOGRAPH
    {0xE841, 0x8E12}, //14985 #CJK UNIFIED IDEOGRAPH
    {0xE842, 0x8E18}, //14986 #CJK UNIFIED IDEOGRAPH
    {0xE843, 0x8E13}, //14987 #CJK UNIFIED IDEOGRAPH
    {0xE844, 0x8E1C}, //14988 #CJK UNIFIED IDEOGRAPH
    {0xE845, 0x8E17}, //14989 #CJK UNIFIED IDEOGRAPH
    {0xE846, 0x8E1A}, //14990 #CJK UNIFIED IDEOGRAPH
    {0xE847, 0x8F2C}, //14991 #CJK UNIFIED IDEOGRAPH
    {0xE848, 0x8F24}, //14992 #CJK UNIFIED IDEOGRAPH
    {0xE849, 0x8F18}, //14993 #CJK UNIFIED IDEOGRAPH
    {0xE84A, 0x8F1A}, //14994 #CJK UNIFIED IDEOGRAPH
    {0xE84B, 0x8F20}, //14995 #CJK UNIFIED IDEOGRAPH
    {0xE84C, 0x8F23}, //14996 #CJK UNIFIED IDEOGRAPH
    {0xE84D, 0x8F16}, //14997 #CJK UNIFIED IDEOGRAPH
    {0xE84E, 0x8F17}, //14998 #CJK UNIFIED IDEOGRAPH
    {0xE84F, 0x9073}, //14999 #CJK UNIFIED IDEOGRAPH
    {0xE850, 0x9070}, //15000 #CJK UNIFIED IDEOGRAPH
    {0xE851, 0x906F}, //15001 #CJK UNIFIED IDEOGRAPH
    {0xE852, 0x9067}, //15002 #CJK UNIFIED IDEOGRAPH
    {0xE853, 0x906B}, //15003 #CJK UNIFIED IDEOGRAPH
    {0xE854, 0x912F}, //15004 #CJK UNIFIED IDEOGRAPH
    {0xE855, 0x912B}, //15005 #CJK UNIFIED IDEOGRAPH
    {0xE856, 0x9129}, //15006 #CJK UNIFIED IDEOGRAPH
    {0xE857, 0x912A}, //15007 #CJK UNIFIED IDEOGRAPH
    {0xE858, 0x9132}, //15008 #CJK UNIFIED IDEOGRAPH
    {0xE859, 0x9126}, //15009 #CJK UNIFIED IDEOGRAPH
    {0xE85A, 0x912E}, //15010 #CJK UNIFIED IDEOGRAPH
    {0xE85B, 0x9185}, //15011 #CJK UNIFIED IDEOGRAPH
    {0xE85C, 0x9186}, //15012 #CJK UNIFIED IDEOGRAPH
    {0xE85D, 0x918A}, //15013 #CJK UNIFIED IDEOGRAPH
    {0xE85E, 0x9181}, //15014 #CJK UNIFIED IDEOGRAPH
    {0xE85F, 0x9182}, //15015 #CJK UNIFIED IDEOGRAPH
    {0xE860, 0x9184}, //15016 #CJK UNIFIED IDEOGRAPH
    {0xE861, 0x9180}, //15017 #CJK UNIFIED IDEOGRAPH
    {0xE862, 0x92D0}, //15018 #CJK UNIFIED IDEOGRAPH
    {0xE863, 0x92C3}, //15019 #CJK UNIFIED IDEOGRAPH
    {0xE864, 0x92C4}, //15020 #CJK UNIFIED IDEOGRAPH
    {0xE865, 0x92C0}, //15021 #CJK UNIFIED IDEOGRAPH
    {0xE866, 0x92D9}, //15022 #CJK UNIFIED IDEOGRAPH
    {0xE867, 0x92B6}, //15023 #CJK UNIFIED IDEOGRAPH
    {0xE868, 0x92CF}, //15024 #CJK UNIFIED IDEOGRAPH
    {0xE869, 0x92F1}, //15025 #CJK UNIFIED IDEOGRAPH
    {0xE86A, 0x92DF}, //15026 #CJK UNIFIED IDEOGRAPH
    {0xE86B, 0x92D8}, //15027 #CJK UNIFIED IDEOGRAPH
    {0xE86C, 0x92E9}, //15028 #CJK UNIFIED IDEOGRAPH
    {0xE86D, 0x92D7}, //15029 #CJK UNIFIED IDEOGRAPH
    {0xE86E, 0x92DD}, //15030 #CJK UNIFIED IDEOGRAPH
    {0xE86F, 0x92CC}, //15031 #CJK UNIFIED IDEOGRAPH
    {0xE870, 0x92EF}, //15032 #CJK UNIFIED IDEOGRAPH
    {0xE871, 0x92C2}, //15033 #CJK UNIFIED IDEOGRAPH
    {0xE872, 0x92E8}, //15034 #CJK UNIFIED IDEOGRAPH
    {0xE873, 0x92CA}, //15035 #CJK UNIFIED IDEOGRAPH
    {0xE874, 0x92C8}, //15036 #CJK UNIFIED IDEOGRAPH
    {0xE875, 0x92CE}, //15037 #CJK UNIFIED IDEOGRAPH
    {0xE876, 0x92E6}, //15038 #CJK UNIFIED IDEOGRAPH
    {0xE877, 0x92CD}, //15039 #CJK UNIFIED IDEOGRAPH
    {0xE878, 0x92D5}, //15040 #CJK UNIFIED IDEOGRAPH
    {0xE879, 0x92C9}, //15041 #CJK UNIFIED IDEOGRAPH
    {0xE87A, 0x92E0}, //15042 #CJK UNIFIED IDEOGRAPH
    {0xE87B, 0x92DE}, //15043 #CJK UNIFIED IDEOGRAPH
    {0xE87C, 0x92E7}, //15044 #CJK UNIFIED IDEOGRAPH
    {0xE87D, 0x92D1}, //15045 #CJK UNIFIED IDEOGRAPH
    {0xE87E, 0x92D3}, //15046 #CJK UNIFIED IDEOGRAPH
    {0xE8A1, 0x92B5}, //15047 #CJK UNIFIED IDEOGRAPH
    {0xE8A2, 0x92E1}, //15048 #CJK UNIFIED IDEOGRAPH
    {0xE8A3, 0x92C6}, //15049 #CJK UNIFIED IDEOGRAPH
    {0xE8A4, 0x92B4}, //15050 #CJK UNIFIED IDEOGRAPH
    {0xE8A5, 0x957C}, //15051 #CJK UNIFIED IDEOGRAPH
    {0xE8A6, 0x95AC}, //15052 #CJK UNIFIED IDEOGRAPH
    {0xE8A7, 0x95AB}, //15053 #CJK UNIFIED IDEOGRAPH
    {0xE8A8, 0x95AE}, //15054 #CJK UNIFIED IDEOGRAPH
    {0xE8A9, 0x95B0}, //15055 #CJK UNIFIED IDEOGRAPH
    {0xE8AA, 0x96A4}, //15056 #CJK UNIFIED IDEOGRAPH
    {0xE8AB, 0x96A2}, //15057 #CJK UNIFIED IDEOGRAPH
    {0xE8AC, 0x96D3}, //15058 #CJK UNIFIED IDEOGRAPH
    {0xE8AD, 0x9705}, //15059 #CJK UNIFIED IDEOGRAPH
    {0xE8AE, 0x9708}, //15060 #CJK UNIFIED IDEOGRAPH
    {0xE8AF, 0x9702}, //15061 #CJK UNIFIED IDEOGRAPH
    {0xE8B0, 0x975A}, //15062 #CJK UNIFIED IDEOGRAPH
    {0xE8B1, 0x978A}, //15063 #CJK UNIFIED IDEOGRAPH
    {0xE8B2, 0x978E}, //15064 #CJK UNIFIED IDEOGRAPH
    {0xE8B3, 0x9788}, //15065 #CJK UNIFIED IDEOGRAPH
    {0xE8B4, 0x97D0}, //15066 #CJK UNIFIED IDEOGRAPH
    {0xE8B5, 0x97CF}, //15067 #CJK UNIFIED IDEOGRAPH
    {0xE8B6, 0x981E}, //15068 #CJK UNIFIED IDEOGRAPH
    {0xE8B7, 0x981D}, //15069 #CJK UNIFIED IDEOGRAPH
    {0xE8B8, 0x9826}, //15070 #CJK UNIFIED IDEOGRAPH
    {0xE8B9, 0x9829}, //15071 #CJK UNIFIED IDEOGRAPH
    {0xE8BA, 0x9828}, //15072 #CJK UNIFIED IDEOGRAPH
    {0xE8BB, 0x9820}, //15073 #CJK UNIFIED IDEOGRAPH
    {0xE8BC, 0x981B}, //15074 #CJK UNIFIED IDEOGRAPH
    {0xE8BD, 0x9827}, //15075 #CJK UNIFIED IDEOGRAPH
    {0xE8BE, 0x98B2}, //15076 #CJK UNIFIED IDEOGRAPH
    {0xE8BF, 0x9908}, //15077 #CJK UNIFIED IDEOGRAPH
    {0xE8C0, 0x98FA}, //15078 #CJK UNIFIED IDEOGRAPH
    {0xE8C1, 0x9911}, //15079 #CJK UNIFIED IDEOGRAPH
    {0xE8C2, 0x9914}, //15080 #CJK UNIFIED IDEOGRAPH
    {0xE8C3, 0x9916}, //15081 #CJK UNIFIED IDEOGRAPH
    {0xE8C4, 0x9917}, //15082 #CJK UNIFIED IDEOGRAPH
    {0xE8C5, 0x9915}, //15083 #CJK UNIFIED IDEOGRAPH
    {0xE8C6, 0x99DC}, //15084 #CJK UNIFIED IDEOGRAPH
    {0xE8C7, 0x99CD}, //15085 #CJK UNIFIED IDEOGRAPH
    {0xE8C8, 0x99CF}, //15086 #CJK UNIFIED IDEOGRAPH
    {0xE8C9, 0x99D3}, //15087 #CJK UNIFIED IDEOGRAPH
    {0xE8CA, 0x99D4}, //15088 #CJK UNIFIED IDEOGRAPH
    {0xE8CB, 0x99CE}, //15089 #CJK UNIFIED IDEOGRAPH
    {0xE8CC, 0x99C9}, //15090 #CJK UNIFIED IDEOGRAPH
    {0xE8CD, 0x99D6}, //15091 #CJK UNIFIED IDEOGRAPH
    {0xE8CE, 0x99D8}, //15092 #CJK UNIFIED IDEOGRAPH
    {0xE8CF, 0x99CB}, //15093 #CJK UNIFIED IDEOGRAPH
    {0xE8D0, 0x99D7}, //15094 #CJK UNIFIED IDEOGRAPH
    {0xE8D1, 0x99CC}, //15095 #CJK UNIFIED IDEOGRAPH
    {0xE8D2, 0x9AB3}, //15096 #CJK UNIFIED IDEOGRAPH
    {0xE8D3, 0x9AEC}, //15097 #CJK UNIFIED IDEOGRAPH
    {0xE8D4, 0x9AEB}, //15098 #CJK UNIFIED IDEOGRAPH
    {0xE8D5, 0x9AF3}, //15099 #CJK UNIFIED IDEOGRAPH
    {0xE8D6, 0x9AF2}, //15100 #CJK UNIFIED IDEOGRAPH
    {0xE8D7, 0x9AF1}, //15101 #CJK UNIFIED IDEOGRAPH
    {0xE8D8, 0x9B46}, //15102 #CJK UNIFIED IDEOGRAPH
    {0xE8D9, 0x9B43}, //15103 #CJK UNIFIED IDEOGRAPH
    {0xE8DA, 0x9B67}, //15104 #CJK UNIFIED IDEOGRAPH
    {0xE8DB, 0x9B74}, //15105 #CJK UNIFIED IDEOGRAPH
    {0xE8DC, 0x9B71}, //15106 #CJK UNIFIED IDEOGRAPH
    {0xE8DD, 0x9B66}, //15107 #CJK UNIFIED IDEOGRAPH
    {0xE8DE, 0x9B76}, //15108 #CJK UNIFIED IDEOGRAPH
    {0xE8DF, 0x9B75}, //15109 #CJK UNIFIED IDEOGRAPH
    {0xE8E0, 0x9B70}, //15110 #CJK UNIFIED IDEOGRAPH
    {0xE8E1, 0x9B68}, //15111 #CJK UNIFIED IDEOGRAPH
    {0xE8E2, 0x9B64}, //15112 #CJK UNIFIED IDEOGRAPH
    {0xE8E3, 0x9B6C}, //15113 #CJK UNIFIED IDEOGRAPH
    {0xE8E4, 0x9CFC}, //15114 #CJK UNIFIED IDEOGRAPH
    {0xE8E5, 0x9CFA}, //15115 #CJK UNIFIED IDEOGRAPH
    {0xE8E6, 0x9CFD}, //15116 #CJK UNIFIED IDEOGRAPH
    {0xE8E7, 0x9CFF}, //15117 #CJK UNIFIED IDEOGRAPH
    {0xE8E8, 0x9CF7}, //15118 #CJK UNIFIED IDEOGRAPH
    {0xE8E9, 0x9D07}, //15119 #CJK UNIFIED IDEOGRAPH
    {0xE8EA, 0x9D00}, //15120 #CJK UNIFIED IDEOGRAPH
    {0xE8EB, 0x9CF9}, //15121 #CJK UNIFIED IDEOGRAPH
    {0xE8EC, 0x9CFB}, //15122 #CJK UNIFIED IDEOGRAPH
    {0xE8ED, 0x9D08}, //15123 #CJK UNIFIED IDEOGRAPH
    {0xE8EE, 0x9D05}, //15124 #CJK UNIFIED IDEOGRAPH
    {0xE8EF, 0x9D04}, //15125 #CJK UNIFIED IDEOGRAPH
    {0xE8F0, 0x9E83}, //15126 #CJK UNIFIED IDEOGRAPH
    {0xE8F1, 0x9ED3}, //15127 #CJK UNIFIED IDEOGRAPH
    {0xE8F2, 0x9F0F}, //15128 #CJK UNIFIED IDEOGRAPH
    {0xE8F3, 0x9F10}, //15129 #CJK UNIFIED IDEOGRAPH
    {0xE8F4, 0x511C}, //15130 #CJK UNIFIED IDEOGRAPH
    {0xE8F5, 0x5113}, //15131 #CJK UNIFIED IDEOGRAPH
    {0xE8F6, 0x5117}, //15132 #CJK UNIFIED IDEOGRAPH
    {0xE8F7, 0x511A}, //15133 #CJK UNIFIED IDEOGRAPH
    {0xE8F8, 0x5111}, //15134 #CJK UNIFIED IDEOGRAPH
    {0xE8F9, 0x51DE}, //15135 #CJK UNIFIED IDEOGRAPH
    {0xE8FA, 0x5334}, //15136 #CJK UNIFIED IDEOGRAPH
    {0xE8FB, 0x53E1}, //15137 #CJK UNIFIED IDEOGRAPH
    {0xE8FC, 0x5670}, //15138 #CJK UNIFIED IDEOGRAPH
    {0xE8FD, 0x5660}, //15139 #CJK UNIFIED IDEOGRAPH
    {0xE8FE, 0x566E}, //15140 #CJK UNIFIED IDEOGRAPH
    {0xE940, 0x5673}, //15141 #CJK UNIFIED IDEOGRAPH
    {0xE941, 0x5666}, //15142 #CJK UNIFIED IDEOGRAPH
    {0xE942, 0x5663}, //15143 #CJK UNIFIED IDEOGRAPH
    {0xE943, 0x566D}, //15144 #CJK UNIFIED IDEOGRAPH
    {0xE944, 0x5672}, //15145 #CJK UNIFIED IDEOGRAPH
    {0xE945, 0x565E}, //15146 #CJK UNIFIED IDEOGRAPH
    {0xE946, 0x5677}, //15147 #CJK UNIFIED IDEOGRAPH
    {0xE947, 0x571C}, //15148 #CJK UNIFIED IDEOGRAPH
    {0xE948, 0x571B}, //15149 #CJK UNIFIED IDEOGRAPH
    {0xE949, 0x58C8}, //15150 #CJK UNIFIED IDEOGRAPH
    {0xE94A, 0x58BD}, //15151 #CJK UNIFIED IDEOGRAPH
    {0xE94B, 0x58C9}, //15152 #CJK UNIFIED IDEOGRAPH
    {0xE94C, 0x58BF}, //15153 #CJK UNIFIED IDEOGRAPH
    {0xE94D, 0x58BA}, //15154 #CJK UNIFIED IDEOGRAPH
    {0xE94E, 0x58C2}, //15155 #CJK UNIFIED IDEOGRAPH
    {0xE94F, 0x58BC}, //15156 #CJK UNIFIED IDEOGRAPH
    {0xE950, 0x58C6}, //15157 #CJK UNIFIED IDEOGRAPH
    {0xE951, 0x5B17}, //15158 #CJK UNIFIED IDEOGRAPH
    {0xE952, 0x5B19}, //15159 #CJK UNIFIED IDEOGRAPH
    {0xE953, 0x5B1B}, //15160 #CJK UNIFIED IDEOGRAPH
    {0xE954, 0x5B21}, //15161 #CJK UNIFIED IDEOGRAPH
    {0xE955, 0x5B14}, //15162 #CJK UNIFIED IDEOGRAPH
    {0xE956, 0x5B13}, //15163 #CJK UNIFIED IDEOGRAPH
    {0xE957, 0x5B10}, //15164 #CJK UNIFIED IDEOGRAPH
    {0xE958, 0x5B16}, //15165 #CJK UNIFIED IDEOGRAPH
    {0xE959, 0x5B28}, //15166 #CJK UNIFIED IDEOGRAPH
    {0xE95A, 0x5B1A}, //15167 #CJK UNIFIED IDEOGRAPH
    {0xE95B, 0x5B20}, //15168 #CJK UNIFIED IDEOGRAPH
    {0xE95C, 0x5B1E}, //15169 #CJK UNIFIED IDEOGRAPH
    {0xE95D, 0x5BEF}, //15170 #CJK UNIFIED IDEOGRAPH
    {0xE95E, 0x5DAC}, //15171 #CJK UNIFIED IDEOGRAPH
    {0xE95F, 0x5DB1}, //15172 #CJK UNIFIED IDEOGRAPH
    {0xE960, 0x5DA9}, //15173 #CJK UNIFIED IDEOGRAPH
    {0xE961, 0x5DA7}, //15174 #CJK UNIFIED IDEOGRAPH
    {0xE962, 0x5DB5}, //15175 #CJK UNIFIED IDEOGRAPH
    {0xE963, 0x5DB0}, //15176 #CJK UNIFIED IDEOGRAPH
    {0xE964, 0x5DAE}, //15177 #CJK UNIFIED IDEOGRAPH
    {0xE965, 0x5DAA}, //15178 #CJK UNIFIED IDEOGRAPH
    {0xE966, 0x5DA8}, //15179 #CJK UNIFIED IDEOGRAPH
    {0xE967, 0x5DB2}, //15180 #CJK UNIFIED IDEOGRAPH
    {0xE968, 0x5DAD}, //15181 #CJK UNIFIED IDEOGRAPH
    {0xE969, 0x5DAF}, //15182 #CJK UNIFIED IDEOGRAPH
    {0xE96A, 0x5DB4}, //15183 #CJK UNIFIED IDEOGRAPH
    {0xE96B, 0x5E67}, //15184 #CJK UNIFIED IDEOGRAPH
    {0xE96C, 0x5E68}, //15185 #CJK UNIFIED IDEOGRAPH
    {0xE96D, 0x5E66}, //15186 #CJK UNIFIED IDEOGRAPH
    {0xE96E, 0x5E6F}, //15187 #CJK UNIFIED IDEOGRAPH
    {0xE96F, 0x5EE9}, //15188 #CJK UNIFIED IDEOGRAPH
    {0xE970, 0x5EE7}, //15189 #CJK UNIFIED IDEOGRAPH
    {0xE971, 0x5EE6}, //15190 #CJK UNIFIED IDEOGRAPH
    {0xE972, 0x5EE8}, //15191 #CJK UNIFIED IDEOGRAPH
    {0xE973, 0x5EE5}, //15192 #CJK UNIFIED IDEOGRAPH
    {0xE974, 0x5F4B}, //15193 #CJK UNIFIED IDEOGRAPH
    {0xE975, 0x5FBC}, //15194 #CJK UNIFIED IDEOGRAPH
    {0xE976, 0x619D}, //15195 #CJK UNIFIED IDEOGRAPH
    {0xE977, 0x61A8}, //15196 #CJK UNIFIED IDEOGRAPH
    {0xE978, 0x6196}, //15197 #CJK UNIFIED IDEOGRAPH
    {0xE979, 0x61C5}, //15198 #CJK UNIFIED IDEOGRAPH
    {0xE97A, 0x61B4}, //15199 #CJK UNIFIED IDEOGRAPH
    {0xE97B, 0x61C6}, //15200 #CJK UNIFIED IDEOGRAPH
    {0xE97C, 0x61C1}, //15201 #CJK UNIFIED IDEOGRAPH
    {0xE97D, 0x61CC}, //15202 #CJK UNIFIED IDEOGRAPH
    {0xE97E, 0x61BA}, //15203 #CJK UNIFIED IDEOGRAPH
    {0xE9A1, 0x61BF}, //15204 #CJK UNIFIED IDEOGRAPH
    {0xE9A2, 0x61B8}, //15205 #CJK UNIFIED IDEOGRAPH
    {0xE9A3, 0x618C}, //15206 #CJK UNIFIED IDEOGRAPH
    {0xE9A4, 0x64D7}, //15207 #CJK UNIFIED IDEOGRAPH
    {0xE9A5, 0x64D6}, //15208 #CJK UNIFIED IDEOGRAPH
    {0xE9A6, 0x64D0}, //15209 #CJK UNIFIED IDEOGRAPH
    {0xE9A7, 0x64CF}, //15210 #CJK UNIFIED IDEOGRAPH
    {0xE9A8, 0x64C9}, //15211 #CJK UNIFIED IDEOGRAPH
    {0xE9A9, 0x64BD}, //15212 #CJK UNIFIED IDEOGRAPH
    {0xE9AA, 0x6489}, //15213 #CJK UNIFIED IDEOGRAPH
    {0xE9AB, 0x64C3}, //15214 #CJK UNIFIED IDEOGRAPH
    {0xE9AC, 0x64DB}, //15215 #CJK UNIFIED IDEOGRAPH
    {0xE9AD, 0x64F3}, //15216 #CJK UNIFIED IDEOGRAPH
    {0xE9AE, 0x64D9}, //15217 #CJK UNIFIED IDEOGRAPH
    {0xE9AF, 0x6533}, //15218 #CJK UNIFIED IDEOGRAPH
    {0xE9B0, 0x657F}, //15219 #CJK UNIFIED IDEOGRAPH
    {0xE9B1, 0x657C}, //15220 #CJK UNIFIED IDEOGRAPH
    {0xE9B2, 0x65A2}, //15221 #CJK UNIFIED IDEOGRAPH
    {0xE9B3, 0x66C8}, //15222 #CJK UNIFIED IDEOGRAPH
    {0xE9B4, 0x66BE}, //15223 #CJK UNIFIED IDEOGRAPH
    {0xE9B5, 0x66C0}, //15224 #CJK UNIFIED IDEOGRAPH
    {0xE9B6, 0x66CA}, //15225 #CJK UNIFIED IDEOGRAPH
    {0xE9B7, 0x66CB}, //15226 #CJK UNIFIED IDEOGRAPH
    {0xE9B8, 0x66CF}, //15227 #CJK UNIFIED IDEOGRAPH
    {0xE9B9, 0x66BD}, //15228 #CJK UNIFIED IDEOGRAPH
    {0xE9BA, 0x66BB}, //15229 #CJK UNIFIED IDEOGRAPH
    {0xE9BB, 0x66BA}, //15230 #CJK UNIFIED IDEOGRAPH
    {0xE9BC, 0x66CC}, //15231 #CJK UNIFIED IDEOGRAPH
    {0xE9BD, 0x6723}, //15232 #CJK UNIFIED IDEOGRAPH
    {0xE9BE, 0x6A34}, //15233 #CJK UNIFIED IDEOGRAPH
    {0xE9BF, 0x6A66}, //15234 #CJK UNIFIED IDEOGRAPH
    {0xE9C0, 0x6A49}, //15235 #CJK UNIFIED IDEOGRAPH
    {0xE9C1, 0x6A67}, //15236 #CJK UNIFIED IDEOGRAPH
    {0xE9C2, 0x6A32}, //15237 #CJK UNIFIED IDEOGRAPH
    {0xE9C3, 0x6A68}, //15238 #CJK UNIFIED IDEOGRAPH
    {0xE9C4, 0x6A3E}, //15239 #CJK UNIFIED IDEOGRAPH
    {0xE9C5, 0x6A5D}, //15240 #CJK UNIFIED IDEOGRAPH
    {0xE9C6, 0x6A6D}, //15241 #CJK UNIFIED IDEOGRAPH
    {0xE9C7, 0x6A76}, //15242 #CJK UNIFIED IDEOGRAPH
    {0xE9C8, 0x6A5B}, //15243 #CJK UNIFIED IDEOGRAPH
    {0xE9C9, 0x6A51}, //15244 #CJK UNIFIED IDEOGRAPH
    {0xE9CA, 0x6A28}, //15245 #CJK UNIFIED IDEOGRAPH
    {0xE9CB, 0x6A5A}, //15246 #CJK UNIFIED IDEOGRAPH
    {0xE9CC, 0x6A3B}, //15247 #CJK UNIFIED IDEOGRAPH
    {0xE9CD, 0x6A3F}, //15248 #CJK UNIFIED IDEOGRAPH
    {0xE9CE, 0x6A41}, //15249 #CJK UNIFIED IDEOGRAPH
    {0xE9CF, 0x6A6A}, //15250 #CJK UNIFIED IDEOGRAPH
    {0xE9D0, 0x6A64}, //15251 #CJK UNIFIED IDEOGRAPH
    {0xE9D1, 0x6A50}, //15252 #CJK UNIFIED IDEOGRAPH
    {0xE9D2, 0x6A4F}, //15253 #CJK UNIFIED IDEOGRAPH
    {0xE9D3, 0x6A54}, //15254 #CJK UNIFIED IDEOGRAPH
    {0xE9D4, 0x6A6F}, //15255 #CJK UNIFIED IDEOGRAPH
    {0xE9D5, 0x6A69}, //15256 #CJK UNIFIED IDEOGRAPH
    {0xE9D6, 0x6A60}, //15257 #CJK UNIFIED IDEOGRAPH
    {0xE9D7, 0x6A3C}, //15258 #CJK UNIFIED IDEOGRAPH
    {0xE9D8, 0x6A5E}, //15259 #CJK UNIFIED IDEOGRAPH
    {0xE9D9, 0x6A56}, //15260 #CJK UNIFIED IDEOGRAPH
    {0xE9DA, 0x6A55}, //15261 #CJK UNIFIED IDEOGRAPH
    {0xE9DB, 0x6A4D}, //15262 #CJK UNIFIED IDEOGRAPH
    {0xE9DC, 0x6A4E}, //15263 #CJK UNIFIED IDEOGRAPH
    {0xE9DD, 0x6A46}, //15264 #CJK UNIFIED IDEOGRAPH
    {0xE9DE, 0x6B55}, //15265 #CJK UNIFIED IDEOGRAPH
    {0xE9DF, 0x6B54}, //15266 #CJK UNIFIED IDEOGRAPH
    {0xE9E0, 0x6B56}, //15267 #CJK UNIFIED IDEOGRAPH
    {0xE9E1, 0x6BA7}, //15268 #CJK UNIFIED IDEOGRAPH
    {0xE9E2, 0x6BAA}, //15269 #CJK UNIFIED IDEOGRAPH
    {0xE9E3, 0x6BAB}, //15270 #CJK UNIFIED IDEOGRAPH
    {0xE9E4, 0x6BC8}, //15271 #CJK UNIFIED IDEOGRAPH
    {0xE9E5, 0x6BC7}, //15272 #CJK UNIFIED IDEOGRAPH
    {0xE9E6, 0x6C04}, //15273 #CJK UNIFIED IDEOGRAPH
    {0xE9E7, 0x6C03}, //15274 #CJK UNIFIED IDEOGRAPH
    {0xE9E8, 0x6C06}, //15275 #CJK UNIFIED IDEOGRAPH
    {0xE9E9, 0x6FAD}, //15276 #CJK UNIFIED IDEOGRAPH
    {0xE9EA, 0x6FCB}, //15277 #CJK UNIFIED IDEOGRAPH
    {0xE9EB, 0x6FA3}, //15278 #CJK UNIFIED IDEOGRAPH
    {0xE9EC, 0x6FC7}, //15279 #CJK UNIFIED IDEOGRAPH
    {0xE9ED, 0x6FBC}, //15280 #CJK UNIFIED IDEOGRAPH
    {0xE9EE, 0x6FCE}, //15281 #CJK UNIFIED IDEOGRAPH
    {0xE9EF, 0x6FC8}, //15282 #CJK UNIFIED IDEOGRAPH
    {0xE9F0, 0x6F5E}, //15283 #CJK UNIFIED IDEOGRAPH
    {0xE9F1, 0x6FC4}, //15284 #CJK UNIFIED IDEOGRAPH
    {0xE9F2, 0x6FBD}, //15285 #CJK UNIFIED IDEOGRAPH
    {0xE9F3, 0x6F9E}, //15286 #CJK UNIFIED IDEOGRAPH
    {0xE9F4, 0x6FCA}, //15287 #CJK UNIFIED IDEOGRAPH
    {0xE9F5, 0x6FA8}, //15288 #CJK UNIFIED IDEOGRAPH
    {0xE9F6, 0x7004}, //15289 #CJK UNIFIED IDEOGRAPH
    {0xE9F7, 0x6FA5}, //15290 #CJK UNIFIED IDEOGRAPH
    {0xE9F8, 0x6FAE}, //15291 #CJK UNIFIED IDEOGRAPH
    {0xE9F9, 0x6FBA}, //15292 #CJK UNIFIED IDEOGRAPH
    {0xE9FA, 0x6FAC}, //15293 #CJK UNIFIED IDEOGRAPH
    {0xE9FB, 0x6FAA}, //15294 #CJK UNIFIED IDEOGRAPH
    {0xE9FC, 0x6FCF}, //15295 #CJK UNIFIED IDEOGRAPH
    {0xE9FD, 0x6FBF}, //15296 #CJK UNIFIED IDEOGRAPH
    {0xE9FE, 0x6FB8}, //15297 #CJK UNIFIED IDEOGRAPH
    {0xEA40, 0x6FA2}, //15298 #CJK UNIFIED IDEOGRAPH
    {0xEA41, 0x6FC9}, //15299 #CJK UNIFIED IDEOGRAPH
    {0xEA42, 0x6FAB}, //15300 #CJK UNIFIED IDEOGRAPH
    {0xEA43, 0x6FCD}, //15301 #CJK UNIFIED IDEOGRAPH
    {0xEA44, 0x6FAF}, //15302 #CJK UNIFIED IDEOGRAPH
    {0xEA45, 0x6FB2}, //15303 #CJK UNIFIED IDEOGRAPH
    {0xEA46, 0x6FB0}, //15304 #CJK UNIFIED IDEOGRAPH
    {0xEA47, 0x71C5}, //15305 #CJK UNIFIED IDEOGRAPH
    {0xEA48, 0x71C2}, //15306 #CJK UNIFIED IDEOGRAPH
    {0xEA49, 0x71BF}, //15307 #CJK UNIFIED IDEOGRAPH
    {0xEA4A, 0x71B8}, //15308 #CJK UNIFIED IDEOGRAPH
    {0xEA4B, 0x71D6}, //15309 #CJK UNIFIED IDEOGRAPH
    {0xEA4C, 0x71C0}, //15310 #CJK UNIFIED IDEOGRAPH
    {0xEA4D, 0x71C1}, //15311 #CJK UNIFIED IDEOGRAPH
    {0xEA4E, 0x71CB}, //15312 #CJK UNIFIED IDEOGRAPH
    {0xEA4F, 0x71D4}, //15313 #CJK UNIFIED IDEOGRAPH
    {0xEA50, 0x71CA}, //15314 #CJK UNIFIED IDEOGRAPH
    {0xEA51, 0x71C7}, //15315 #CJK UNIFIED IDEOGRAPH
    {0xEA52, 0x71CF}, //15316 #CJK UNIFIED IDEOGRAPH
    {0xEA53, 0x71BD}, //15317 #CJK UNIFIED IDEOGRAPH
    {0xEA54, 0x71D8}, //15318 #CJK UNIFIED IDEOGRAPH
    {0xEA55, 0x71BC}, //15319 #CJK UNIFIED IDEOGRAPH
    {0xEA56, 0x71C6}, //15320 #CJK UNIFIED IDEOGRAPH
    {0xEA57, 0x71DA}, //15321 #CJK UNIFIED IDEOGRAPH
    {0xEA58, 0x71DB}, //15322 #CJK UNIFIED IDEOGRAPH
    {0xEA59, 0x729D}, //15323 #CJK UNIFIED IDEOGRAPH
    {0xEA5A, 0x729E}, //15324 #CJK UNIFIED IDEOGRAPH
    {0xEA5B, 0x7369}, //15325 #CJK UNIFIED IDEOGRAPH
    {0xEA5C, 0x7366}, //15326 #CJK UNIFIED IDEOGRAPH
    {0xEA5D, 0x7367}, //15327 #CJK UNIFIED IDEOGRAPH
    {0xEA5E, 0x736C}, //15328 #CJK UNIFIED IDEOGRAPH
    {0xEA5F, 0x7365}, //15329 #CJK UNIFIED IDEOGRAPH
    {0xEA60, 0x736B}, //15330 #CJK UNIFIED IDEOGRAPH
    {0xEA61, 0x736A}, //15331 #CJK UNIFIED IDEOGRAPH
    {0xEA62, 0x747F}, //15332 #CJK UNIFIED IDEOGRAPH
    {0xEA63, 0x749A}, //15333 #CJK UNIFIED IDEOGRAPH
    {0xEA64, 0x74A0}, //15334 #CJK UNIFIED IDEOGRAPH
    {0xEA65, 0x7494}, //15335 #CJK UNIFIED IDEOGRAPH
    {0xEA66, 0x7492}, //15336 #CJK UNIFIED IDEOGRAPH
    {0xEA67, 0x7495}, //15337 #CJK UNIFIED IDEOGRAPH
    {0xEA68, 0x74A1}, //15338 #CJK UNIFIED IDEOGRAPH
    {0xEA69, 0x750B}, //15339 #CJK UNIFIED IDEOGRAPH
    {0xEA6A, 0x7580}, //15340 #CJK UNIFIED IDEOGRAPH
    {0xEA6B, 0x762F}, //15341 #CJK UNIFIED IDEOGRAPH
    {0xEA6C, 0x762D}, //15342 #CJK UNIFIED IDEOGRAPH
    {0xEA6D, 0x7631}, //15343 #CJK UNIFIED IDEOGRAPH
    {0xEA6E, 0x763D}, //15344 #CJK UNIFIED IDEOGRAPH
    {0xEA6F, 0x7633}, //15345 #CJK UNIFIED IDEOGRAPH
    {0xEA70, 0x763C}, //15346 #CJK UNIFIED IDEOGRAPH
    {0xEA71, 0x7635}, //15347 #CJK UNIFIED IDEOGRAPH
    {0xEA72, 0x7632}, //15348 #CJK UNIFIED IDEOGRAPH
    {0xEA73, 0x7630}, //15349 #CJK UNIFIED IDEOGRAPH
    {0xEA74, 0x76BB}, //15350 #CJK UNIFIED IDEOGRAPH
    {0xEA75, 0x76E6}, //15351 #CJK UNIFIED IDEOGRAPH
    {0xEA76, 0x779A}, //15352 #CJK UNIFIED IDEOGRAPH
    {0xEA77, 0x779D}, //15353 #CJK UNIFIED IDEOGRAPH
    {0xEA78, 0x77A1}, //15354 #CJK UNIFIED IDEOGRAPH
    {0xEA79, 0x779C}, //15355 #CJK UNIFIED IDEOGRAPH
    {0xEA7A, 0x779B}, //15356 #CJK UNIFIED IDEOGRAPH
    {0xEA7B, 0x77A2}, //15357 #CJK UNIFIED IDEOGRAPH
    {0xEA7C, 0x77A3}, //15358 #CJK UNIFIED IDEOGRAPH
    {0xEA7D, 0x7795}, //15359 #CJK UNIFIED IDEOGRAPH
    {0xEA7E, 0x7799}, //15360 #CJK UNIFIED IDEOGRAPH
    {0xEAA1, 0x7797}, //15361 #CJK UNIFIED IDEOGRAPH
    {0xEAA2, 0x78DD}, //15362 #CJK UNIFIED IDEOGRAPH
    {0xEAA3, 0x78E9}, //15363 #CJK UNIFIED IDEOGRAPH
    {0xEAA4, 0x78E5}, //15364 #CJK UNIFIED IDEOGRAPH
    {0xEAA5, 0x78EA}, //15365 #CJK UNIFIED IDEOGRAPH
    {0xEAA6, 0x78DE}, //15366 #CJK UNIFIED IDEOGRAPH
    {0xEAA7, 0x78E3}, //15367 #CJK UNIFIED IDEOGRAPH
    {0xEAA8, 0x78DB}, //15368 #CJK UNIFIED IDEOGRAPH
    {0xEAA9, 0x78E1}, //15369 #CJK UNIFIED IDEOGRAPH
    {0xEAAA, 0x78E2}, //15370 #CJK UNIFIED IDEOGRAPH
    {0xEAAB, 0x78ED}, //15371 #CJK UNIFIED IDEOGRAPH
    {0xEAAC, 0x78DF}, //15372 #CJK UNIFIED IDEOGRAPH
    {0xEAAD, 0x78E0}, //15373 #CJK UNIFIED IDEOGRAPH
    {0xEAAE, 0x79A4}, //15374 #CJK UNIFIED IDEOGRAPH
    {0xEAAF, 0x7A44}, //15375 #CJK UNIFIED IDEOGRAPH
    {0xEAB0, 0x7A48}, //15376 #CJK UNIFIED IDEOGRAPH
    {0xEAB1, 0x7A47}, //15377 #CJK UNIFIED IDEOGRAPH
    {0xEAB2, 0x7AB6}, //15378 #CJK UNIFIED IDEOGRAPH
    {0xEAB3, 0x7AB8}, //15379 #CJK UNIFIED IDEOGRAPH
    {0xEAB4, 0x7AB5}, //15380 #CJK UNIFIED IDEOGRAPH
    {0xEAB5, 0x7AB1}, //15381 #CJK UNIFIED IDEOGRAPH
    {0xEAB6, 0x7AB7}, //15382 #CJK UNIFIED IDEOGRAPH
    {0xEAB7, 0x7BDE}, //15383 #CJK UNIFIED IDEOGRAPH
    {0xEAB8, 0x7BE3}, //15384 #CJK UNIFIED IDEOGRAPH
    {0xEAB9, 0x7BE7}, //15385 #CJK UNIFIED IDEOGRAPH
    {0xEABA, 0x7BDD}, //15386 #CJK UNIFIED IDEOGRAPH
    {0xEABB, 0x7BD5}, //15387 #CJK UNIFIED IDEOGRAPH
    {0xEABC, 0x7BE5}, //15388 #CJK UNIFIED IDEOGRAPH
    {0xEABD, 0x7BDA}, //15389 #CJK UNIFIED IDEOGRAPH
    {0xEABE, 0x7BE8}, //15390 #CJK UNIFIED IDEOGRAPH
    {0xEABF, 0x7BF9}, //15391 #CJK UNIFIED IDEOGRAPH
    {0xEAC0, 0x7BD4}, //15392 #CJK UNIFIED IDEOGRAPH
    {0xEAC1, 0x7BEA}, //15393 #CJK UNIFIED IDEOGRAPH
    {0xEAC2, 0x7BE2}, //15394 #CJK UNIFIED IDEOGRAPH
    {0xEAC3, 0x7BDC}, //15395 #CJK UNIFIED IDEOGRAPH
    {0xEAC4, 0x7BEB}, //15396 #CJK UNIFIED IDEOGRAPH
    {0xEAC5, 0x7BD8}, //15397 #CJK UNIFIED IDEOGRAPH
    {0xEAC6, 0x7BDF}, //15398 #CJK UNIFIED IDEOGRAPH
    {0xEAC7, 0x7CD2}, //15399 #CJK UNIFIED IDEOGRAPH
    {0xEAC8, 0x7CD4}, //15400 #CJK UNIFIED IDEOGRAPH
    {0xEAC9, 0x7CD7}, //15401 #CJK UNIFIED IDEOGRAPH
    {0xEACA, 0x7CD0}, //15402 #CJK UNIFIED IDEOGRAPH
    {0xEACB, 0x7CD1}, //15403 #CJK UNIFIED IDEOGRAPH
    {0xEACC, 0x7E12}, //15404 #CJK UNIFIED IDEOGRAPH
    {0xEACD, 0x7E21}, //15405 #CJK UNIFIED IDEOGRAPH
    {0xEACE, 0x7E17}, //15406 #CJK UNIFIED IDEOGRAPH
    {0xEACF, 0x7E0C}, //15407 #CJK UNIFIED IDEOGRAPH
    {0xEAD0, 0x7E1F}, //15408 #CJK UNIFIED IDEOGRAPH
    {0xEAD1, 0x7E20}, //15409 #CJK UNIFIED IDEOGRAPH
    {0xEAD2, 0x7E13}, //15410 #CJK UNIFIED IDEOGRAPH
    {0xEAD3, 0x7E0E}, //15411 #CJK UNIFIED IDEOGRAPH
    {0xEAD4, 0x7E1C}, //15412 #CJK UNIFIED IDEOGRAPH
    {0xEAD5, 0x7E15}, //15413 #CJK UNIFIED IDEOGRAPH
    {0xEAD6, 0x7E1A}, //15414 #CJK UNIFIED IDEOGRAPH
    {0xEAD7, 0x7E22}, //15415 #CJK UNIFIED IDEOGRAPH
    {0xEAD8, 0x7E0B}, //15416 #CJK UNIFIED IDEOGRAPH
    {0xEAD9, 0x7E0F}, //15417 #CJK UNIFIED IDEOGRAPH
    {0xEADA, 0x7E16}, //15418 #CJK UNIFIED IDEOGRAPH
    {0xEADB, 0x7E0D}, //15419 #CJK UNIFIED IDEOGRAPH
    {0xEADC, 0x7E14}, //15420 #CJK UNIFIED IDEOGRAPH
    {0xEADD, 0x7E25}, //15421 #CJK UNIFIED IDEOGRAPH
    {0xEADE, 0x7E24}, //15422 #CJK UNIFIED IDEOGRAPH
    {0xEADF, 0x7F43}, //15423 #CJK UNIFIED IDEOGRAPH
    {0xEAE0, 0x7F7B}, //15424 #CJK UNIFIED IDEOGRAPH
    {0xEAE1, 0x7F7C}, //15425 #CJK UNIFIED IDEOGRAPH
    {0xEAE2, 0x7F7A}, //15426 #CJK UNIFIED IDEOGRAPH
    {0xEAE3, 0x7FB1}, //15427 #CJK UNIFIED IDEOGRAPH
    {0xEAE4, 0x7FEF}, //15428 #CJK UNIFIED IDEOGRAPH
    {0xEAE5, 0x802A}, //15429 #CJK UNIFIED IDEOGRAPH
    {0xEAE6, 0x8029}, //15430 #CJK UNIFIED IDEOGRAPH
    {0xEAE7, 0x806C}, //15431 #CJK UNIFIED IDEOGRAPH
    {0xEAE8, 0x81B1}, //15432 #CJK UNIFIED IDEOGRAPH
    {0xEAE9, 0x81A6}, //15433 #CJK UNIFIED IDEOGRAPH
    {0xEAEA, 0x81AE}, //15434 #CJK UNIFIED IDEOGRAPH
    {0xEAEB, 0x81B9}, //15435 #CJK UNIFIED IDEOGRAPH
    {0xEAEC, 0x81B5}, //15436 #CJK UNIFIED IDEOGRAPH
    {0xEAED, 0x81AB}, //15437 #CJK UNIFIED IDEOGRAPH
    {0xEAEE, 0x81B0}, //15438 #CJK UNIFIED IDEOGRAPH
    {0xEAEF, 0x81AC}, //15439 #CJK UNIFIED IDEOGRAPH
    {0xEAF0, 0x81B4}, //15440 #CJK UNIFIED IDEOGRAPH
    {0xEAF1, 0x81B2}, //15441 #CJK UNIFIED IDEOGRAPH
    {0xEAF2, 0x81B7}, //15442 #CJK UNIFIED IDEOGRAPH
    {0xEAF3, 0x81A7}, //15443 #CJK UNIFIED IDEOGRAPH
    {0xEAF4, 0x81F2}, //15444 #CJK UNIFIED IDEOGRAPH
    {0xEAF5, 0x8255}, //15445 #CJK UNIFIED IDEOGRAPH
    {0xEAF6, 0x8256}, //15446 #CJK UNIFIED IDEOGRAPH
    {0xEAF7, 0x8257}, //15447 #CJK UNIFIED IDEOGRAPH
    {0xEAF8, 0x8556}, //15448 #CJK UNIFIED IDEOGRAPH
    {0xEAF9, 0x8545}, //15449 #CJK UNIFIED IDEOGRAPH
    {0xEAFA, 0x856B}, //15450 #CJK UNIFIED IDEOGRAPH
    {0xEAFB, 0x854D}, //15451 #CJK UNIFIED IDEOGRAPH
    {0xEAFC, 0x8553}, //15452 #CJK UNIFIED IDEOGRAPH
    {0xEAFD, 0x8561}, //15453 #CJK UNIFIED IDEOGRAPH
    {0xEAFE, 0x8558}, //15454 #CJK UNIFIED IDEOGRAPH
    {0xEB40, 0x8540}, //15455 #CJK UNIFIED IDEOGRAPH
    {0xEB41, 0x8546}, //15456 #CJK UNIFIED IDEOGRAPH
    {0xEB42, 0x8564}, //15457 #CJK UNIFIED IDEOGRAPH
    {0xEB43, 0x8541}, //15458 #CJK UNIFIED IDEOGRAPH
    {0xEB44, 0x8562}, //15459 #CJK UNIFIED IDEOGRAPH
    {0xEB45, 0x8544}, //15460 #CJK UNIFIED IDEOGRAPH
    {0xEB46, 0x8551}, //15461 #CJK UNIFIED IDEOGRAPH
    {0xEB47, 0x8547}, //15462 #CJK UNIFIED IDEOGRAPH
    {0xEB48, 0x8563}, //15463 #CJK UNIFIED IDEOGRAPH
    {0xEB49, 0x853E}, //15464 #CJK UNIFIED IDEOGRAPH
    {0xEB4A, 0x855B}, //15465 #CJK UNIFIED IDEOGRAPH
    {0xEB4B, 0x8571}, //15466 #CJK UNIFIED IDEOGRAPH
    {0xEB4C, 0x854E}, //15467 #CJK UNIFIED IDEOGRAPH
    {0xEB4D, 0x856E}, //15468 #CJK UNIFIED IDEOGRAPH
    {0xEB4E, 0x8575}, //15469 #CJK UNIFIED IDEOGRAPH
    {0xEB4F, 0x8555}, //15470 #CJK UNIFIED IDEOGRAPH
    {0xEB50, 0x8567}, //15471 #CJK UNIFIED IDEOGRAPH
    {0xEB51, 0x8560}, //15472 #CJK UNIFIED IDEOGRAPH
    {0xEB52, 0x858C}, //15473 #CJK UNIFIED IDEOGRAPH
    {0xEB53, 0x8566}, //15474 #CJK UNIFIED IDEOGRAPH
    {0xEB54, 0x855D}, //15475 #CJK UNIFIED IDEOGRAPH
    {0xEB55, 0x8554}, //15476 #CJK UNIFIED IDEOGRAPH
    {0xEB56, 0x8565}, //15477 #CJK UNIFIED IDEOGRAPH
    {0xEB57, 0x856C}, //15478 #CJK UNIFIED IDEOGRAPH
    {0xEB58, 0x8663}, //15479 #CJK UNIFIED IDEOGRAPH
    {0xEB59, 0x8665}, //15480 #CJK UNIFIED IDEOGRAPH
    {0xEB5A, 0x8664}, //15481 #CJK UNIFIED IDEOGRAPH
    {0xEB5B, 0x879B}, //15482 #CJK UNIFIED IDEOGRAPH
    {0xEB5C, 0x878F}, //15483 #CJK UNIFIED IDEOGRAPH
    {0xEB5D, 0x8797}, //15484 #CJK UNIFIED IDEOGRAPH
    {0xEB5E, 0x8793}, //15485 #CJK UNIFIED IDEOGRAPH
    {0xEB5F, 0x8792}, //15486 #CJK UNIFIED IDEOGRAPH
    {0xEB60, 0x8788}, //15487 #CJK UNIFIED IDEOGRAPH
    {0xEB61, 0x8781}, //15488 #CJK UNIFIED IDEOGRAPH
    {0xEB62, 0x8796}, //15489 #CJK UNIFIED IDEOGRAPH
    {0xEB63, 0x8798}, //15490 #CJK UNIFIED IDEOGRAPH
    {0xEB64, 0x8779}, //15491 #CJK UNIFIED IDEOGRAPH
    {0xEB65, 0x8787}, //15492 #CJK UNIFIED IDEOGRAPH
    {0xEB66, 0x87A3}, //15493 #CJK UNIFIED IDEOGRAPH
    {0xEB67, 0x8785}, //15494 #CJK UNIFIED IDEOGRAPH
    {0xEB68, 0x8790}, //15495 #CJK UNIFIED IDEOGRAPH
    {0xEB69, 0x8791}, //15496 #CJK UNIFIED IDEOGRAPH
    {0xEB6A, 0x879D}, //15497 #CJK UNIFIED IDEOGRAPH
    {0xEB6B, 0x8784}, //15498 #CJK UNIFIED IDEOGRAPH
    {0xEB6C, 0x8794}, //15499 #CJK UNIFIED IDEOGRAPH
    {0xEB6D, 0x879C}, //15500 #CJK UNIFIED IDEOGRAPH
    {0xEB6E, 0x879A}, //15501 #CJK UNIFIED IDEOGRAPH
    {0xEB6F, 0x8789}, //15502 #CJK UNIFIED IDEOGRAPH
    {0xEB70, 0x891E}, //15503 #CJK UNIFIED IDEOGRAPH
    {0xEB71, 0x8926}, //15504 #CJK UNIFIED IDEOGRAPH
    {0xEB72, 0x8930}, //15505 #CJK UNIFIED IDEOGRAPH
    {0xEB73, 0x892D}, //15506 #CJK UNIFIED IDEOGRAPH
    {0xEB74, 0x892E}, //15507 #CJK UNIFIED IDEOGRAPH
    {0xEB75, 0x8927}, //15508 #CJK UNIFIED IDEOGRAPH
    {0xEB76, 0x8931}, //15509 #CJK UNIFIED IDEOGRAPH
    {0xEB77, 0x8922}, //15510 #CJK UNIFIED IDEOGRAPH
    {0xEB78, 0x8929}, //15511 #CJK UNIFIED IDEOGRAPH
    {0xEB79, 0x8923}, //15512 #CJK UNIFIED IDEOGRAPH
    {0xEB7A, 0x892F}, //15513 #CJK UNIFIED IDEOGRAPH
    {0xEB7B, 0x892C}, //15514 #CJK UNIFIED IDEOGRAPH
    {0xEB7C, 0x891F}, //15515 #CJK UNIFIED IDEOGRAPH
    {0xEB7D, 0x89F1}, //15516 #CJK UNIFIED IDEOGRAPH
    {0xEB7E, 0x8AE0}, //15517 #CJK UNIFIED IDEOGRAPH
    {0xEBA1, 0x8AE2}, //15518 #CJK UNIFIED IDEOGRAPH
    {0xEBA2, 0x8AF2}, //15519 #CJK UNIFIED IDEOGRAPH
    {0xEBA3, 0x8AF4}, //15520 #CJK UNIFIED IDEOGRAPH
    {0xEBA4, 0x8AF5}, //15521 #CJK UNIFIED IDEOGRAPH
    {0xEBA5, 0x8ADD}, //15522 #CJK UNIFIED IDEOGRAPH
    {0xEBA6, 0x8B14}, //15523 #CJK UNIFIED IDEOGRAPH
    {0xEBA7, 0x8AE4}, //15524 #CJK UNIFIED IDEOGRAPH
    {0xEBA8, 0x8ADF}, //15525 #CJK UNIFIED IDEOGRAPH
    {0xEBA9, 0x8AF0}, //15526 #CJK UNIFIED IDEOGRAPH
    {0xEBAA, 0x8AC8}, //15527 #CJK UNIFIED IDEOGRAPH
    {0xEBAB, 0x8ADE}, //15528 #CJK UNIFIED IDEOGRAPH
    {0xEBAC, 0x8AE1}, //15529 #CJK UNIFIED IDEOGRAPH
    {0xEBAD, 0x8AE8}, //15530 #CJK UNIFIED IDEOGRAPH
    {0xEBAE, 0x8AFF}, //15531 #CJK UNIFIED IDEOGRAPH
    {0xEBAF, 0x8AEF}, //15532 #CJK UNIFIED IDEOGRAPH
    {0xEBB0, 0x8AFB}, //15533 #CJK UNIFIED IDEOGRAPH
    {0xEBB1, 0x8C91}, //15534 #CJK UNIFIED IDEOGRAPH
    {0xEBB2, 0x8C92}, //15535 #CJK UNIFIED IDEOGRAPH
    {0xEBB3, 0x8C90}, //15536 #CJK UNIFIED IDEOGRAPH
    {0xEBB4, 0x8CF5}, //15537 #CJK UNIFIED IDEOGRAPH
    {0xEBB5, 0x8CEE}, //15538 #CJK UNIFIED IDEOGRAPH
    {0xEBB6, 0x8CF1}, //15539 #CJK UNIFIED IDEOGRAPH
    {0xEBB7, 0x8CF0}, //15540 #CJK UNIFIED IDEOGRAPH
    {0xEBB8, 0x8CF3}, //15541 #CJK UNIFIED IDEOGRAPH
    {0xEBB9, 0x8D6C}, //15542 #CJK UNIFIED IDEOGRAPH
    {0xEBBA, 0x8D6E}, //15543 #CJK UNIFIED IDEOGRAPH
    {0xEBBB, 0x8DA5}, //15544 #CJK UNIFIED IDEOGRAPH
    {0xEBBC, 0x8DA7}, //15545 #CJK UNIFIED IDEOGRAPH
    {0xEBBD, 0x8E33}, //15546 #CJK UNIFIED IDEOGRAPH
    {0xEBBE, 0x8E3E}, //15547 #CJK UNIFIED IDEOGRAPH
    {0xEBBF, 0x8E38}, //15548 #CJK UNIFIED IDEOGRAPH
    {0xEBC0, 0x8E40}, //15549 #CJK UNIFIED IDEOGRAPH
    {0xEBC1, 0x8E45}, //15550 #CJK UNIFIED IDEOGRAPH
    {0xEBC2, 0x8E36}, //15551 #CJK UNIFIED IDEOGRAPH
    {0xEBC3, 0x8E3C}, //15552 #CJK UNIFIED IDEOGRAPH
    {0xEBC4, 0x8E3D}, //15553 #CJK UNIFIED IDEOGRAPH
    {0xEBC5, 0x8E41}, //15554 #CJK UNIFIED IDEOGRAPH
    {0xEBC6, 0x8E30}, //15555 #CJK UNIFIED IDEOGRAPH
    {0xEBC7, 0x8E3F}, //15556 #CJK UNIFIED IDEOGRAPH
    {0xEBC8, 0x8EBD}, //15557 #CJK UNIFIED IDEOGRAPH
    {0xEBC9, 0x8F36}, //15558 #CJK UNIFIED IDEOGRAPH
    {0xEBCA, 0x8F2E}, //15559 #CJK UNIFIED IDEOGRAPH
    {0xEBCB, 0x8F35}, //15560 #CJK UNIFIED IDEOGRAPH
    {0xEBCC, 0x8F32}, //15561 #CJK UNIFIED IDEOGRAPH
    {0xEBCD, 0x8F39}, //15562 #CJK UNIFIED IDEOGRAPH
    {0xEBCE, 0x8F37}, //15563 #CJK UNIFIED IDEOGRAPH
    {0xEBCF, 0x8F34}, //15564 #CJK UNIFIED IDEOGRAPH
    {0xEBD0, 0x9076}, //15565 #CJK UNIFIED IDEOGRAPH
    {0xEBD1, 0x9079}, //15566 #CJK UNIFIED IDEOGRAPH
    {0xEBD2, 0x907B}, //15567 #CJK UNIFIED IDEOGRAPH
    {0xEBD3, 0x9086}, //15568 #CJK UNIFIED IDEOGRAPH
    {0xEBD4, 0x90FA}, //15569 #CJK UNIFIED IDEOGRAPH
    {0xEBD5, 0x9133}, //15570 #CJK UNIFIED IDEOGRAPH
    {0xEBD6, 0x9135}, //15571 #CJK UNIFIED IDEOGRAPH
    {0xEBD7, 0x9136}, //15572 #CJK UNIFIED IDEOGRAPH
    {0xEBD8, 0x9193}, //15573 #CJK UNIFIED IDEOGRAPH
    {0xEBD9, 0x9190}, //15574 #CJK UNIFIED IDEOGRAPH
    {0xEBDA, 0x9191}, //15575 #CJK UNIFIED IDEOGRAPH
    {0xEBDB, 0x918D}, //15576 #CJK UNIFIED IDEOGRAPH
    {0xEBDC, 0x918F}, //15577 #CJK UNIFIED IDEOGRAPH
    {0xEBDD, 0x9327}, //15578 #CJK UNIFIED IDEOGRAPH
    {0xEBDE, 0x931E}, //15579 #CJK UNIFIED IDEOGRAPH
    {0xEBDF, 0x9308}, //15580 #CJK UNIFIED IDEOGRAPH
    {0xEBE0, 0x931F}, //15581 #CJK UNIFIED IDEOGRAPH
    {0xEBE1, 0x9306}, //15582 #CJK UNIFIED IDEOGRAPH
    {0xEBE2, 0x930F}, //15583 #CJK UNIFIED IDEOGRAPH
    {0xEBE3, 0x937A}, //15584 #CJK UNIFIED IDEOGRAPH
    {0xEBE4, 0x9338}, //15585 #CJK UNIFIED IDEOGRAPH
    {0xEBE5, 0x933C}, //15586 #CJK UNIFIED IDEOGRAPH
    {0xEBE6, 0x931B}, //15587 #CJK UNIFIED IDEOGRAPH
    {0xEBE7, 0x9323}, //15588 #CJK UNIFIED IDEOGRAPH
    {0xEBE8, 0x9312}, //15589 #CJK UNIFIED IDEOGRAPH
    {0xEBE9, 0x9301}, //15590 #CJK UNIFIED IDEOGRAPH
    {0xEBEA, 0x9346}, //15591 #CJK UNIFIED IDEOGRAPH
    {0xEBEB, 0x932D}, //15592 #CJK UNIFIED IDEOGRAPH
    {0xEBEC, 0x930E}, //15593 #CJK UNIFIED IDEOGRAPH
    {0xEBED, 0x930D}, //15594 #CJK UNIFIED IDEOGRAPH
    {0xEBEE, 0x92CB}, //15595 #CJK UNIFIED IDEOGRAPH
    {0xEBEF, 0x931D}, //15596 #CJK UNIFIED IDEOGRAPH
    {0xEBF0, 0x92FA}, //15597 #CJK UNIFIED IDEOGRAPH
    {0xEBF1, 0x9325}, //15598 #CJK UNIFIED IDEOGRAPH
    {0xEBF2, 0x9313}, //15599 #CJK UNIFIED IDEOGRAPH
    {0xEBF3, 0x92F9}, //15600 #CJK UNIFIED IDEOGRAPH
    {0xEBF4, 0x92F7}, //15601 #CJK UNIFIED IDEOGRAPH
    {0xEBF5, 0x9334}, //15602 #CJK UNIFIED IDEOGRAPH
    {0xEBF6, 0x9302}, //15603 #CJK UNIFIED IDEOGRAPH
    {0xEBF7, 0x9324}, //15604 #CJK UNIFIED IDEOGRAPH
    {0xEBF8, 0x92FF}, //15605 #CJK UNIFIED IDEOGRAPH
    {0xEBF9, 0x9329}, //15606 #CJK UNIFIED IDEOGRAPH
    {0xEBFA, 0x9339}, //15607 #CJK UNIFIED IDEOGRAPH
    {0xEBFB, 0x9335}, //15608 #CJK UNIFIED IDEOGRAPH
    {0xEBFC, 0x932A}, //15609 #CJK UNIFIED IDEOGRAPH
    {0xEBFD, 0x9314}, //15610 #CJK UNIFIED IDEOGRAPH
    {0xEBFE, 0x930C}, //15611 #CJK UNIFIED IDEOGRAPH
    {0xEC40, 0x930B}, //15612 #CJK UNIFIED IDEOGRAPH
    {0xEC41, 0x92FE}, //15613 #CJK UNIFIED IDEOGRAPH
    {0xEC42, 0x9309}, //15614 #CJK UNIFIED IDEOGRAPH
    {0xEC43, 0x9300}, //15615 #CJK UNIFIED IDEOGRAPH
    {0xEC44, 0x92FB}, //15616 #CJK UNIFIED IDEOGRAPH
    {0xEC45, 0x9316}, //15617 #CJK UNIFIED IDEOGRAPH
    {0xEC46, 0x95BC}, //15618 #CJK UNIFIED IDEOGRAPH
    {0xEC47, 0x95CD}, //15619 #CJK UNIFIED IDEOGRAPH
    {0xEC48, 0x95BE}, //15620 #CJK UNIFIED IDEOGRAPH
    {0xEC49, 0x95B9}, //15621 #CJK UNIFIED IDEOGRAPH
    {0xEC4A, 0x95BA}, //15622 #CJK UNIFIED IDEOGRAPH
    {0xEC4B, 0x95B6}, //15623 #CJK UNIFIED IDEOGRAPH
    {0xEC4C, 0x95BF}, //15624 #CJK UNIFIED IDEOGRAPH
    {0xEC4D, 0x95B5}, //15625 #CJK UNIFIED IDEOGRAPH
    {0xEC4E, 0x95BD}, //15626 #CJK UNIFIED IDEOGRAPH
    {0xEC4F, 0x96A9}, //15627 #CJK UNIFIED IDEOGRAPH
    {0xEC50, 0x96D4}, //15628 #CJK UNIFIED IDEOGRAPH
    {0xEC51, 0x970B}, //15629 #CJK UNIFIED IDEOGRAPH
    {0xEC52, 0x9712}, //15630 #CJK UNIFIED IDEOGRAPH
    {0xEC53, 0x9710}, //15631 #CJK UNIFIED IDEOGRAPH
    {0xEC54, 0x9799}, //15632 #CJK UNIFIED IDEOGRAPH
    {0xEC55, 0x9797}, //15633 #CJK UNIFIED IDEOGRAPH
    {0xEC56, 0x9794}, //15634 #CJK UNIFIED IDEOGRAPH
    {0xEC57, 0x97F0}, //15635 #CJK UNIFIED IDEOGRAPH
    {0xEC58, 0x97F8}, //15636 #CJK UNIFIED IDEOGRAPH
    {0xEC59, 0x9835}, //15637 #CJK UNIFIED IDEOGRAPH
    {0xEC5A, 0x982F}, //15638 #CJK UNIFIED IDEOGRAPH
    {0xEC5B, 0x9832}, //15639 #CJK UNIFIED IDEOGRAPH
    {0xEC5C, 0x9924}, //15640 #CJK UNIFIED IDEOGRAPH
    {0xEC5D, 0x991F}, //15641 #CJK UNIFIED IDEOGRAPH
    {0xEC5E, 0x9927}, //15642 #CJK UNIFIED IDEOGRAPH
    {0xEC5F, 0x9929}, //15643 #CJK UNIFIED IDEOGRAPH
    {0xEC60, 0x999E}, //15644 #CJK UNIFIED IDEOGRAPH
    {0xEC61, 0x99EE}, //15645 #CJK UNIFIED IDEOGRAPH
    {0xEC62, 0x99EC}, //15646 #CJK UNIFIED IDEOGRAPH
    {0xEC63, 0x99E5}, //15647 #CJK UNIFIED IDEOGRAPH
    {0xEC64, 0x99E4}, //15648 #CJK UNIFIED IDEOGRAPH
    {0xEC65, 0x99F0}, //15649 #CJK UNIFIED IDEOGRAPH
    {0xEC66, 0x99E3}, //15650 #CJK UNIFIED IDEOGRAPH
    {0xEC67, 0x99EA}, //15651 #CJK UNIFIED IDEOGRAPH
    {0xEC68, 0x99E9}, //15652 #CJK UNIFIED IDEOGRAPH
    {0xEC69, 0x99E7}, //15653 #CJK UNIFIED IDEOGRAPH
    {0xEC6A, 0x9AB9}, //15654 #CJK UNIFIED IDEOGRAPH
    {0xEC6B, 0x9ABF}, //15655 #CJK UNIFIED IDEOGRAPH
    {0xEC6C, 0x9AB4}, //15656 #CJK UNIFIED IDEOGRAPH
    {0xEC6D, 0x9ABB}, //15657 #CJK UNIFIED IDEOGRAPH
    {0xEC6E, 0x9AF6}, //15658 #CJK UNIFIED IDEOGRAPH
    {0xEC6F, 0x9AFA}, //15659 #CJK UNIFIED IDEOGRAPH
    {0xEC70, 0x9AF9}, //15660 #CJK UNIFIED IDEOGRAPH
    {0xEC71, 0x9AF7}, //15661 #CJK UNIFIED IDEOGRAPH
    {0xEC72, 0x9B33}, //15662 #CJK UNIFIED IDEOGRAPH
    {0xEC73, 0x9B80}, //15663 #CJK UNIFIED IDEOGRAPH
    {0xEC74, 0x9B85}, //15664 #CJK UNIFIED IDEOGRAPH
    {0xEC75, 0x9B87}, //15665 #CJK UNIFIED IDEOGRAPH
    {0xEC76, 0x9B7C}, //15666 #CJK UNIFIED IDEOGRAPH
    {0xEC77, 0x9B7E}, //15667 #CJK UNIFIED IDEOGRAPH
    {0xEC78, 0x9B7B}, //15668 #CJK UNIFIED IDEOGRAPH
    {0xEC79, 0x9B82}, //15669 #CJK UNIFIED IDEOGRAPH
    {0xEC7A, 0x9B93}, //15670 #CJK UNIFIED IDEOGRAPH
    {0xEC7B, 0x9B92}, //15671 #CJK UNIFIED IDEOGRAPH
    {0xEC7C, 0x9B90}, //15672 #CJK UNIFIED IDEOGRAPH
    {0xEC7D, 0x9B7A}, //15673 #CJK UNIFIED IDEOGRAPH
    {0xEC7E, 0x9B95}, //15674 #CJK UNIFIED IDEOGRAPH
    {0xECA1, 0x9B7D}, //15675 #CJK UNIFIED IDEOGRAPH
    {0xECA2, 0x9B88}, //15676 #CJK UNIFIED IDEOGRAPH
    {0xECA3, 0x9D25}, //15677 #CJK UNIFIED IDEOGRAPH
    {0xECA4, 0x9D17}, //15678 #CJK UNIFIED IDEOGRAPH
    {0xECA5, 0x9D20}, //15679 #CJK UNIFIED IDEOGRAPH
    {0xECA6, 0x9D1E}, //15680 #CJK UNIFIED IDEOGRAPH
    {0xECA7, 0x9D14}, //15681 #CJK UNIFIED IDEOGRAPH
    {0xECA8, 0x9D29}, //15682 #CJK UNIFIED IDEOGRAPH
    {0xECA9, 0x9D1D}, //15683 #CJK UNIFIED IDEOGRAPH
    {0xECAA, 0x9D18}, //15684 #CJK UNIFIED IDEOGRAPH
    {0xECAB, 0x9D22}, //15685 #CJK UNIFIED IDEOGRAPH
    {0xECAC, 0x9D10}, //15686 #CJK UNIFIED IDEOGRAPH
    {0xECAD, 0x9D19}, //15687 #CJK UNIFIED IDEOGRAPH
    {0xECAE, 0x9D1F}, //15688 #CJK UNIFIED IDEOGRAPH
    {0xECAF, 0x9E88}, //15689 #CJK UNIFIED IDEOGRAPH
    {0xECB0, 0x9E86}, //15690 #CJK UNIFIED IDEOGRAPH
    {0xECB1, 0x9E87}, //15691 #CJK UNIFIED IDEOGRAPH
    {0xECB2, 0x9EAE}, //15692 #CJK UNIFIED IDEOGRAPH
    {0xECB3, 0x9EAD}, //15693 #CJK UNIFIED IDEOGRAPH
    {0xECB4, 0x9ED5}, //15694 #CJK UNIFIED IDEOGRAPH
    {0xECB5, 0x9ED6}, //15695 #CJK UNIFIED IDEOGRAPH
    {0xECB6, 0x9EFA}, //15696 #CJK UNIFIED IDEOGRAPH
    {0xECB7, 0x9F12}, //15697 #CJK UNIFIED IDEOGRAPH
    {0xECB8, 0x9F3D}, //15698 #CJK UNIFIED IDEOGRAPH
    {0xECB9, 0x5126}, //15699 #CJK UNIFIED IDEOGRAPH
    {0xECBA, 0x5125}, //15700 #CJK UNIFIED IDEOGRAPH
    {0xECBB, 0x5122}, //15701 #CJK UNIFIED IDEOGRAPH
    {0xECBC, 0x5124}, //15702 #CJK UNIFIED IDEOGRAPH
    {0xECBD, 0x5120}, //15703 #CJK UNIFIED IDEOGRAPH
    {0xECBE, 0x5129}, //15704 #CJK UNIFIED IDEOGRAPH
    {0xECBF, 0x52F4}, //15705 #CJK UNIFIED IDEOGRAPH
    {0xECC0, 0x5693}, //15706 #CJK UNIFIED IDEOGRAPH
    {0xECC1, 0x568C}, //15707 #CJK UNIFIED IDEOGRAPH
    {0xECC2, 0x568D}, //15708 #CJK UNIFIED IDEOGRAPH
    {0xECC3, 0x5686}, //15709 #CJK UNIFIED IDEOGRAPH
    {0xECC4, 0x5684}, //15710 #CJK UNIFIED IDEOGRAPH
    {0xECC5, 0x5683}, //15711 #CJK UNIFIED IDEOGRAPH
    {0xECC6, 0x567E}, //15712 #CJK UNIFIED IDEOGRAPH
    {0xECC7, 0x5682}, //15713 #CJK UNIFIED IDEOGRAPH
    {0xECC8, 0x567F}, //15714 #CJK UNIFIED IDEOGRAPH
    {0xECC9, 0x5681}, //15715 #CJK UNIFIED IDEOGRAPH
    {0xECCA, 0x58D6}, //15716 #CJK UNIFIED IDEOGRAPH
    {0xECCB, 0x58D4}, //15717 #CJK UNIFIED IDEOGRAPH
    {0xECCC, 0x58CF}, //15718 #CJK UNIFIED IDEOGRAPH
    {0xECCD, 0x58D2}, //15719 #CJK UNIFIED IDEOGRAPH
    {0xECCE, 0x5B2D}, //15720 #CJK UNIFIED IDEOGRAPH
    {0xECCF, 0x5B25}, //15721 #CJK UNIFIED IDEOGRAPH
    {0xECD0, 0x5B32}, //15722 #CJK UNIFIED IDEOGRAPH
    {0xECD1, 0x5B23}, //15723 #CJK UNIFIED IDEOGRAPH
    {0xECD2, 0x5B2C}, //15724 #CJK UNIFIED IDEOGRAPH
    {0xECD3, 0x5B27}, //15725 #CJK UNIFIED IDEOGRAPH
    {0xECD4, 0x5B26}, //15726 #CJK UNIFIED IDEOGRAPH
    {0xECD5, 0x5B2F}, //15727 #CJK UNIFIED IDEOGRAPH
    {0xECD6, 0x5B2E}, //15728 #CJK UNIFIED IDEOGRAPH
    {0xECD7, 0x5B7B}, //15729 #CJK UNIFIED IDEOGRAPH
    {0xECD8, 0x5BF1}, //15730 #CJK UNIFIED IDEOGRAPH
    {0xECD9, 0x5BF2}, //15731 #CJK UNIFIED IDEOGRAPH
    {0xECDA, 0x5DB7}, //15732 #CJK UNIFIED IDEOGRAPH
    {0xECDB, 0x5E6C}, //15733 #CJK UNIFIED IDEOGRAPH
    {0xECDC, 0x5E6A}, //15734 #CJK UNIFIED IDEOGRAPH
    {0xECDD, 0x5FBE}, //15735 #CJK UNIFIED IDEOGRAPH
    {0xECDE, 0x5FBB}, //15736 #CJK UNIFIED IDEOGRAPH
    {0xECDF, 0x61C3}, //15737 #CJK UNIFIED IDEOGRAPH
    {0xECE0, 0x61B5}, //15738 #CJK UNIFIED IDEOGRAPH
    {0xECE1, 0x61BC}, //15739 #CJK UNIFIED IDEOGRAPH
    {0xECE2, 0x61E7}, //15740 #CJK UNIFIED IDEOGRAPH
    {0xECE3, 0x61E0}, //15741 #CJK UNIFIED IDEOGRAPH
    {0xECE4, 0x61E5}, //15742 #CJK UNIFIED IDEOGRAPH
    {0xECE5, 0x61E4}, //15743 #CJK UNIFIED IDEOGRAPH
    {0xECE6, 0x61E8}, //15744 #CJK UNIFIED IDEOGRAPH
    {0xECE7, 0x61DE}, //15745 #CJK UNIFIED IDEOGRAPH
    {0xECE8, 0x64EF}, //15746 #CJK UNIFIED IDEOGRAPH
    {0xECE9, 0x64E9}, //15747 #CJK UNIFIED IDEOGRAPH
    {0xECEA, 0x64E3}, //15748 #CJK UNIFIED IDEOGRAPH
    {0xECEB, 0x64EB}, //15749 #CJK UNIFIED IDEOGRAPH
    {0xECEC, 0x64E4}, //15750 #CJK UNIFIED IDEOGRAPH
    {0xECED, 0x64E8}, //15751 #CJK UNIFIED IDEOGRAPH
    {0xECEE, 0x6581}, //15752 #CJK UNIFIED IDEOGRAPH
    {0xECEF, 0x6580}, //15753 #CJK UNIFIED IDEOGRAPH
    {0xECF0, 0x65B6}, //15754 #CJK UNIFIED IDEOGRAPH
    {0xECF1, 0x65DA}, //15755 #CJK UNIFIED IDEOGRAPH
    {0xECF2, 0x66D2}, //15756 #CJK UNIFIED IDEOGRAPH
    {0xECF3, 0x6A8D}, //15757 #CJK UNIFIED IDEOGRAPH
    {0xECF4, 0x6A96}, //15758 #CJK UNIFIED IDEOGRAPH
    {0xECF5, 0x6A81}, //15759 #CJK UNIFIED IDEOGRAPH
    {0xECF6, 0x6AA5}, //15760 #CJK UNIFIED IDEOGRAPH
    {0xECF7, 0x6A89}, //15761 #CJK UNIFIED IDEOGRAPH
    {0xECF8, 0x6A9F}, //15762 #CJK UNIFIED IDEOGRAPH
    {0xECF9, 0x6A9B}, //15763 #CJK UNIFIED IDEOGRAPH
    {0xECFA, 0x6AA1}, //15764 #CJK UNIFIED IDEOGRAPH
    {0xECFB, 0x6A9E}, //15765 #CJK UNIFIED IDEOGRAPH
    {0xECFC, 0x6A87}, //15766 #CJK UNIFIED IDEOGRAPH
    {0xECFD, 0x6A93}, //15767 #CJK UNIFIED IDEOGRAPH
    {0xECFE, 0x6A8E}, //15768 #CJK UNIFIED IDEOGRAPH
    {0xED40, 0x6A95}, //15769 #CJK UNIFIED IDEOGRAPH
    {0xED41, 0x6A83}, //15770 #CJK UNIFIED IDEOGRAPH
    {0xED42, 0x6AA8}, //15771 #CJK UNIFIED IDEOGRAPH
    {0xED43, 0x6AA4}, //15772 #CJK UNIFIED IDEOGRAPH
    {0xED44, 0x6A91}, //15773 #CJK UNIFIED IDEOGRAPH
    {0xED45, 0x6A7F}, //15774 #CJK UNIFIED IDEOGRAPH
    {0xED46, 0x6AA6}, //15775 #CJK UNIFIED IDEOGRAPH
    {0xED47, 0x6A9A}, //15776 #CJK UNIFIED IDEOGRAPH
    {0xED48, 0x6A85}, //15777 #CJK UNIFIED IDEOGRAPH
    {0xED49, 0x6A8C}, //15778 #CJK UNIFIED IDEOGRAPH
    {0xED4A, 0x6A92}, //15779 #CJK UNIFIED IDEOGRAPH
    {0xED4B, 0x6B5B}, //15780 #CJK UNIFIED IDEOGRAPH
    {0xED4C, 0x6BAD}, //15781 #CJK UNIFIED IDEOGRAPH
    {0xED4D, 0x6C09}, //15782 #CJK UNIFIED IDEOGRAPH
    {0xED4E, 0x6FCC}, //15783 #CJK UNIFIED IDEOGRAPH
    {0xED4F, 0x6FA9}, //15784 #CJK UNIFIED IDEOGRAPH
    {0xED50, 0x6FF4}, //15785 #CJK UNIFIED IDEOGRAPH
    {0xED51, 0x6FD4}, //15786 #CJK UNIFIED IDEOGRAPH
    {0xED52, 0x6FE3}, //15787 #CJK UNIFIED IDEOGRAPH
    {0xED53, 0x6FDC}, //15788 #CJK UNIFIED IDEOGRAPH
    {0xED54, 0x6FED}, //15789 #CJK UNIFIED IDEOGRAPH
    {0xED55, 0x6FE7}, //15790 #CJK UNIFIED IDEOGRAPH
    {0xED56, 0x6FE6}, //15791 #CJK UNIFIED IDEOGRAPH
    {0xED57, 0x6FDE}, //15792 #CJK UNIFIED IDEOGRAPH
    {0xED58, 0x6FF2}, //15793 #CJK UNIFIED IDEOGRAPH
    {0xED59, 0x6FDD}, //15794 #CJK UNIFIED IDEOGRAPH
    {0xED5A, 0x6FE2}, //15795 #CJK UNIFIED IDEOGRAPH
    {0xED5B, 0x6FE8}, //15796 #CJK UNIFIED IDEOGRAPH
    {0xED5C, 0x71E1}, //15797 #CJK UNIFIED IDEOGRAPH
    {0xED5D, 0x71F1}, //15798 #CJK UNIFIED IDEOGRAPH
    {0xED5E, 0x71E8}, //15799 #CJK UNIFIED IDEOGRAPH
    {0xED5F, 0x71F2}, //15800 #CJK UNIFIED IDEOGRAPH
    {0xED60, 0x71E4}, //15801 #CJK UNIFIED IDEOGRAPH
    {0xED61, 0x71F0}, //15802 #CJK UNIFIED IDEOGRAPH
    {0xED62, 0x71E2}, //15803 #CJK UNIFIED IDEOGRAPH
    {0xED63, 0x7373}, //15804 #CJK UNIFIED IDEOGRAPH
    {0xED64, 0x736E}, //15805 #CJK UNIFIED IDEOGRAPH
    {0xED65, 0x736F}, //15806 #CJK UNIFIED IDEOGRAPH
    {0xED66, 0x7497}, //15807 #CJK UNIFIED IDEOGRAPH
    {0xED67, 0x74B2}, //15808 #CJK UNIFIED IDEOGRAPH
    {0xED68, 0x74AB}, //15809 #CJK UNIFIED IDEOGRAPH
    {0xED69, 0x7490}, //15810 #CJK UNIFIED IDEOGRAPH
    {0xED6A, 0x74AA}, //15811 #CJK UNIFIED IDEOGRAPH
    {0xED6B, 0x74AD}, //15812 #CJK UNIFIED IDEOGRAPH
    {0xED6C, 0x74B1}, //15813 #CJK UNIFIED IDEOGRAPH
    {0xED6D, 0x74A5}, //15814 #CJK UNIFIED IDEOGRAPH
    {0xED6E, 0x74AF}, //15815 #CJK UNIFIED IDEOGRAPH
    {0xED6F, 0x7510}, //15816 #CJK UNIFIED IDEOGRAPH
    {0xED70, 0x7511}, //15817 #CJK UNIFIED IDEOGRAPH
    {0xED71, 0x7512}, //15818 #CJK UNIFIED IDEOGRAPH
    {0xED72, 0x750F}, //15819 #CJK UNIFIED IDEOGRAPH
    {0xED73, 0x7584}, //15820 #CJK UNIFIED IDEOGRAPH
    {0xED74, 0x7643}, //15821 #CJK UNIFIED IDEOGRAPH
    {0xED75, 0x7648}, //15822 #CJK UNIFIED IDEOGRAPH
    {0xED76, 0x7649}, //15823 #CJK UNIFIED IDEOGRAPH
    {0xED77, 0x7647}, //15824 #CJK UNIFIED IDEOGRAPH
    {0xED78, 0x76A4}, //15825 #CJK UNIFIED IDEOGRAPH
    {0xED79, 0x76E9}, //15826 #CJK UNIFIED IDEOGRAPH
    {0xED7A, 0x77B5}, //15827 #CJK UNIFIED IDEOGRAPH
    {0xED7B, 0x77AB}, //15828 #CJK UNIFIED IDEOGRAPH
    {0xED7C, 0x77B2}, //15829 #CJK UNIFIED IDEOGRAPH
    {0xED7D, 0x77B7}, //15830 #CJK UNIFIED IDEOGRAPH
    {0xED7E, 0x77B6}, //15831 #CJK UNIFIED IDEOGRAPH
    {0xEDA1, 0x77B4}, //15832 #CJK UNIFIED IDEOGRAPH
    {0xEDA2, 0x77B1}, //15833 #CJK UNIFIED IDEOGRAPH
    {0xEDA3, 0x77A8}, //15834 #CJK UNIFIED IDEOGRAPH
    {0xEDA4, 0x77F0}, //15835 #CJK UNIFIED IDEOGRAPH
    {0xEDA5, 0x78F3}, //15836 #CJK UNIFIED IDEOGRAPH
    {0xEDA6, 0x78FD}, //15837 #CJK UNIFIED IDEOGRAPH
    {0xEDA7, 0x7902}, //15838 #CJK UNIFIED IDEOGRAPH
    {0xEDA8, 0x78FB}, //15839 #CJK UNIFIED IDEOGRAPH
    {0xEDA9, 0x78FC}, //15840 #CJK UNIFIED IDEOGRAPH
    {0xEDAA, 0x78F2}, //15841 #CJK UNIFIED IDEOGRAPH
    {0xEDAB, 0x7905}, //15842 #CJK UNIFIED IDEOGRAPH
    {0xEDAC, 0x78F9}, //15843 #CJK UNIFIED IDEOGRAPH
    {0xEDAD, 0x78FE}, //15844 #CJK UNIFIED IDEOGRAPH
    {0xEDAE, 0x7904}, //15845 #CJK UNIFIED IDEOGRAPH
    {0xEDAF, 0x79AB}, //15846 #CJK UNIFIED IDEOGRAPH
    {0xEDB0, 0x79A8}, //15847 #CJK UNIFIED IDEOGRAPH
    {0xEDB1, 0x7A5C}, //15848 #CJK UNIFIED IDEOGRAPH
    {0xEDB2, 0x7A5B}, //15849 #CJK UNIFIED IDEOGRAPH
    {0xEDB3, 0x7A56}, //15850 #CJK UNIFIED IDEOGRAPH
    {0xEDB4, 0x7A58}, //15851 #CJK UNIFIED IDEOGRAPH
    {0xEDB5, 0x7A54}, //15852 #CJK UNIFIED IDEOGRAPH
    {0xEDB6, 0x7A5A}, //15853 #CJK UNIFIED IDEOGRAPH
    {0xEDB7, 0x7ABE}, //15854 #CJK UNIFIED IDEOGRAPH
    {0xEDB8, 0x7AC0}, //15855 #CJK UNIFIED IDEOGRAPH
    {0xEDB9, 0x7AC1}, //15856 #CJK UNIFIED IDEOGRAPH
    {0xEDBA, 0x7C05}, //15857 #CJK UNIFIED IDEOGRAPH
    {0xEDBB, 0x7C0F}, //15858 #CJK UNIFIED IDEOGRAPH
    {0xEDBC, 0x7BF2}, //15859 #CJK UNIFIED IDEOGRAPH
    {0xEDBD, 0x7C00}, //15860 #CJK UNIFIED IDEOGRAPH
    {0xEDBE, 0x7BFF}, //15861 #CJK UNIFIED IDEOGRAPH
    {0xEDBF, 0x7BFB}, //15862 #CJK UNIFIED IDEOGRAPH
    {0xEDC0, 0x7C0E}, //15863 #CJK UNIFIED IDEOGRAPH
    {0xEDC1, 0x7BF4}, //15864 #CJK UNIFIED IDEOGRAPH
    {0xEDC2, 0x7C0B}, //15865 #CJK UNIFIED IDEOGRAPH
    {0xEDC3, 0x7BF3}, //15866 #CJK UNIFIED IDEOGRAPH
    {0xEDC4, 0x7C02}, //15867 #CJK UNIFIED IDEOGRAPH
    {0xEDC5, 0x7C09}, //15868 #CJK UNIFIED IDEOGRAPH
    {0xEDC6, 0x7C03}, //15869 #CJK UNIFIED IDEOGRAPH
    {0xEDC7, 0x7C01}, //15870 #CJK UNIFIED IDEOGRAPH
    {0xEDC8, 0x7BF8}, //15871 #CJK UNIFIED IDEOGRAPH
    {0xEDC9, 0x7BFD}, //15872 #CJK UNIFIED IDEOGRAPH
    {0xEDCA, 0x7C06}, //15873 #CJK UNIFIED IDEOGRAPH
    {0xEDCB, 0x7BF0}, //15874 #CJK UNIFIED IDEOGRAPH
    {0xEDCC, 0x7BF1}, //15875 #CJK UNIFIED IDEOGRAPH
    {0xEDCD, 0x7C10}, //15876 #CJK UNIFIED IDEOGRAPH
    {0xEDCE, 0x7C0A}, //15877 #CJK UNIFIED IDEOGRAPH
    {0xEDCF, 0x7CE8}, //15878 #CJK UNIFIED IDEOGRAPH
    {0xEDD0, 0x7E2D}, //15879 #CJK UNIFIED IDEOGRAPH
    {0xEDD1, 0x7E3C}, //15880 #CJK UNIFIED IDEOGRAPH
    {0xEDD2, 0x7E42}, //15881 #CJK UNIFIED IDEOGRAPH
    {0xEDD3, 0x7E33}, //15882 #CJK UNIFIED IDEOGRAPH
    {0xEDD4, 0x9848}, //15883 #CJK UNIFIED IDEOGRAPH
    {0xEDD5, 0x7E38}, //15884 #CJK UNIFIED IDEOGRAPH
    {0xEDD6, 0x7E2A}, //15885 #CJK UNIFIED IDEOGRAPH
    {0xEDD7, 0x7E49}, //15886 #CJK UNIFIED IDEOGRAPH
    {0xEDD8, 0x7E40}, //15887 #CJK UNIFIED IDEOGRAPH
    {0xEDD9, 0x7E47}, //15888 #CJK UNIFIED IDEOGRAPH
    {0xEDDA, 0x7E29}, //15889 #CJK UNIFIED IDEOGRAPH
    {0xEDDB, 0x7E4C}, //15890 #CJK UNIFIED IDEOGRAPH
    {0xEDDC, 0x7E30}, //15891 #CJK UNIFIED IDEOGRAPH
    {0xEDDD, 0x7E3B}, //15892 #CJK UNIFIED IDEOGRAPH
    {0xEDDE, 0x7E36}, //15893 #CJK UNIFIED IDEOGRAPH
    {0xEDDF, 0x7E44}, //15894 #CJK UNIFIED IDEOGRAPH
    {0xEDE0, 0x7E3A}, //15895 #CJK UNIFIED IDEOGRAPH
    {0xEDE1, 0x7F45}, //15896 #CJK UNIFIED IDEOGRAPH
    {0xEDE2, 0x7F7F}, //15897 #CJK UNIFIED IDEOGRAPH
    {0xEDE3, 0x7F7E}, //15898 #CJK UNIFIED IDEOGRAPH
    {0xEDE4, 0x7F7D}, //15899 #CJK UNIFIED IDEOGRAPH
    {0xEDE5, 0x7FF4}, //15900 #CJK UNIFIED IDEOGRAPH
    {0xEDE6, 0x7FF2}, //15901 #CJK UNIFIED IDEOGRAPH
    {0xEDE7, 0x802C}, //15902 #CJK UNIFIED IDEOGRAPH
    {0xEDE8, 0x81BB}, //15903 #CJK UNIFIED IDEOGRAPH
    {0xEDE9, 0x81C4}, //15904 #CJK UNIFIED IDEOGRAPH
    {0xEDEA, 0x81CC}, //15905 #CJK UNIFIED IDEOGRAPH
    {0xEDEB, 0x81CA}, //15906 #CJK UNIFIED IDEOGRAPH
    {0xEDEC, 0x81C5}, //15907 #CJK UNIFIED IDEOGRAPH
    {0xEDED, 0x81C7}, //15908 #CJK UNIFIED IDEOGRAPH
    {0xEDEE, 0x81BC}, //15909 #CJK UNIFIED IDEOGRAPH
    {0xEDEF, 0x81E9}, //15910 #CJK UNIFIED IDEOGRAPH
    {0xEDF0, 0x825B}, //15911 #CJK UNIFIED IDEOGRAPH
    {0xEDF1, 0x825A}, //15912 #CJK UNIFIED IDEOGRAPH
    {0xEDF2, 0x825C}, //15913 #CJK UNIFIED IDEOGRAPH
    {0xEDF3, 0x8583}, //15914 #CJK UNIFIED IDEOGRAPH
    {0xEDF4, 0x8580}, //15915 #CJK UNIFIED IDEOGRAPH
    {0xEDF5, 0x858F}, //15916 #CJK UNIFIED IDEOGRAPH
    {0xEDF6, 0x85A7}, //15917 #CJK UNIFIED IDEOGRAPH
    {0xEDF7, 0x8595}, //15918 #CJK UNIFIED IDEOGRAPH
    {0xEDF8, 0x85A0}, //15919 #CJK UNIFIED IDEOGRAPH
    {0xEDF9, 0x858B}, //15920 #CJK UNIFIED IDEOGRAPH
    {0xEDFA, 0x85A3}, //15921 #CJK UNIFIED IDEOGRAPH
    {0xEDFB, 0x857B}, //15922 #CJK UNIFIED IDEOGRAPH
    {0xEDFC, 0x85A4}, //15923 #CJK UNIFIED IDEOGRAPH
    {0xEDFD, 0x859A}, //15924 #CJK UNIFIED IDEOGRAPH
    {0xEDFE, 0x859E}, //15925 #CJK UNIFIED IDEOGRAPH
    {0xEE40, 0x8577}, //15926 #CJK UNIFIED IDEOGRAPH
    {0xEE41, 0x857C}, //15927 #CJK UNIFIED IDEOGRAPH
    {0xEE42, 0x8589}, //15928 #CJK UNIFIED IDEOGRAPH
    {0xEE43, 0x85A1}, //15929 #CJK UNIFIED IDEOGRAPH
    {0xEE44, 0x857A}, //15930 #CJK UNIFIED IDEOGRAPH
    {0xEE45, 0x8578}, //15931 #CJK UNIFIED IDEOGRAPH
    {0xEE46, 0x8557}, //15932 #CJK UNIFIED IDEOGRAPH
    {0xEE47, 0x858E}, //15933 #CJK UNIFIED IDEOGRAPH
    {0xEE48, 0x8596}, //15934 #CJK UNIFIED IDEOGRAPH
    {0xEE49, 0x8586}, //15935 #CJK UNIFIED IDEOGRAPH
    {0xEE4A, 0x858D}, //15936 #CJK UNIFIED IDEOGRAPH
    {0xEE4B, 0x8599}, //15937 #CJK UNIFIED IDEOGRAPH
    {0xEE4C, 0x859D}, //15938 #CJK UNIFIED IDEOGRAPH
    {0xEE4D, 0x8581}, //15939 #CJK UNIFIED IDEOGRAPH
    {0xEE4E, 0x85A2}, //15940 #CJK UNIFIED IDEOGRAPH
    {0xEE4F, 0x8582}, //15941 #CJK UNIFIED IDEOGRAPH
    {0xEE50, 0x8588}, //15942 #CJK UNIFIED IDEOGRAPH
    {0xEE51, 0x8585}, //15943 #CJK UNIFIED IDEOGRAPH
    {0xEE52, 0x8579}, //15944 #CJK UNIFIED IDEOGRAPH
    {0xEE53, 0x8576}, //15945 #CJK UNIFIED IDEOGRAPH
    {0xEE54, 0x8598}, //15946 #CJK UNIFIED IDEOGRAPH
    {0xEE55, 0x8590}, //15947 #CJK UNIFIED IDEOGRAPH
    {0xEE56, 0x859F}, //15948 #CJK UNIFIED IDEOGRAPH
    {0xEE57, 0x8668}, //15949 #CJK UNIFIED IDEOGRAPH
    {0xEE58, 0x87BE}, //15950 #CJK UNIFIED IDEOGRAPH
    {0xEE59, 0x87AA}, //15951 #CJK UNIFIED IDEOGRAPH
    {0xEE5A, 0x87AD}, //15952 #CJK UNIFIED IDEOGRAPH
    {0xEE5B, 0x87C5}, //15953 #CJK UNIFIED IDEOGRAPH
    {0xEE5C, 0x87B0}, //15954 #CJK UNIFIED IDEOGRAPH
    {0xEE5D, 0x87AC}, //15955 #CJK UNIFIED IDEOGRAPH
    {0xEE5E, 0x87B9}, //15956 #CJK UNIFIED IDEOGRAPH
    {0xEE5F, 0x87B5}, //15957 #CJK UNIFIED IDEOGRAPH
    {0xEE60, 0x87BC}, //15958 #CJK UNIFIED IDEOGRAPH
    {0xEE61, 0x87AE}, //15959 #CJK UNIFIED IDEOGRAPH
    {0xEE62, 0x87C9}, //15960 #CJK UNIFIED IDEOGRAPH
    {0xEE63, 0x87C3}, //15961 #CJK UNIFIED IDEOGRAPH
    {0xEE64, 0x87C2}, //15962 #CJK UNIFIED IDEOGRAPH
    {0xEE65, 0x87CC}, //15963 #CJK UNIFIED IDEOGRAPH
    {0xEE66, 0x87B7}, //15964 #CJK UNIFIED IDEOGRAPH
    {0xEE67, 0x87AF}, //15965 #CJK UNIFIED IDEOGRAPH
    {0xEE68, 0x87C4}, //15966 #CJK UNIFIED IDEOGRAPH
    {0xEE69, 0x87CA}, //15967 #CJK UNIFIED IDEOGRAPH
    {0xEE6A, 0x87B4}, //15968 #CJK UNIFIED IDEOGRAPH
    {0xEE6B, 0x87B6}, //15969 #CJK UNIFIED IDEOGRAPH
    {0xEE6C, 0x87BF}, //15970 #CJK UNIFIED IDEOGRAPH
    {0xEE6D, 0x87B8}, //15971 #CJK UNIFIED IDEOGRAPH
    {0xEE6E, 0x87BD}, //15972 #CJK UNIFIED IDEOGRAPH
    {0xEE6F, 0x87DE}, //15973 #CJK UNIFIED IDEOGRAPH
    {0xEE70, 0x87B2}, //15974 #CJK UNIFIED IDEOGRAPH
    {0xEE71, 0x8935}, //15975 #CJK UNIFIED IDEOGRAPH
    {0xEE72, 0x8933}, //15976 #CJK UNIFIED IDEOGRAPH
    {0xEE73, 0x893C}, //15977 #CJK UNIFIED IDEOGRAPH
    {0xEE74, 0x893E}, //15978 #CJK UNIFIED IDEOGRAPH
    {0xEE75, 0x8941}, //15979 #CJK UNIFIED IDEOGRAPH
    {0xEE76, 0x8952}, //15980 #CJK UNIFIED IDEOGRAPH
    {0xEE77, 0x8937}, //15981 #CJK UNIFIED IDEOGRAPH
    {0xEE78, 0x8942}, //15982 #CJK UNIFIED IDEOGRAPH
    {0xEE79, 0x89AD}, //15983 #CJK UNIFIED IDEOGRAPH
    {0xEE7A, 0x89AF}, //15984 #CJK UNIFIED IDEOGRAPH
    {0xEE7B, 0x89AE}, //15985 #CJK UNIFIED IDEOGRAPH
    {0xEE7C, 0x89F2}, //15986 #CJK UNIFIED IDEOGRAPH
    {0xEE7D, 0x89F3}, //15987 #CJK UNIFIED IDEOGRAPH
    {0xEE7E, 0x8B1E}, //15988 #CJK UNIFIED IDEOGRAPH
    {0xEEA1, 0x8B18}, //15989 #CJK UNIFIED IDEOGRAPH
    {0xEEA2, 0x8B16}, //15990 #CJK UNIFIED IDEOGRAPH
    {0xEEA3, 0x8B11}, //15991 #CJK UNIFIED IDEOGRAPH
    {0xEEA4, 0x8B05}, //15992 #CJK UNIFIED IDEOGRAPH
    {0xEEA5, 0x8B0B}, //15993 #CJK UNIFIED IDEOGRAPH
    {0xEEA6, 0x8B22}, //15994 #CJK UNIFIED IDEOGRAPH
    {0xEEA7, 0x8B0F}, //15995 #CJK UNIFIED IDEOGRAPH
    {0xEEA8, 0x8B12}, //15996 #CJK UNIFIED IDEOGRAPH
    {0xEEA9, 0x8B15}, //15997 #CJK UNIFIED IDEOGRAPH
    {0xEEAA, 0x8B07}, //15998 #CJK UNIFIED IDEOGRAPH
    {0xEEAB, 0x8B0D}, //15999 #CJK UNIFIED IDEOGRAPH
    {0xEEAC, 0x8B08}, //16000 #CJK UNIFIED IDEOGRAPH
    {0xEEAD, 0x8B06}, //16001 #CJK UNIFIED IDEOGRAPH
    {0xEEAE, 0x8B1C}, //16002 #CJK UNIFIED IDEOGRAPH
    {0xEEAF, 0x8B13}, //16003 #CJK UNIFIED IDEOGRAPH
    {0xEEB0, 0x8B1A}, //16004 #CJK UNIFIED IDEOGRAPH
    {0xEEB1, 0x8C4F}, //16005 #CJK UNIFIED IDEOGRAPH
    {0xEEB2, 0x8C70}, //16006 #CJK UNIFIED IDEOGRAPH
    {0xEEB3, 0x8C72}, //16007 #CJK UNIFIED IDEOGRAPH
    {0xEEB4, 0x8C71}, //16008 #CJK UNIFIED IDEOGRAPH
    {0xEEB5, 0x8C6F}, //16009 #CJK UNIFIED IDEOGRAPH
    {0xEEB6, 0x8C95}, //16010 #CJK UNIFIED IDEOGRAPH
    {0xEEB7, 0x8C94}, //16011 #CJK UNIFIED IDEOGRAPH
    {0xEEB8, 0x8CF9}, //16012 #CJK UNIFIED IDEOGRAPH
    {0xEEB9, 0x8D6F}, //16013 #CJK UNIFIED IDEOGRAPH
    {0xEEBA, 0x8E4E}, //16014 #CJK UNIFIED IDEOGRAPH
    {0xEEBB, 0x8E4D}, //16015 #CJK UNIFIED IDEOGRAPH
    {0xEEBC, 0x8E53}, //16016 #CJK UNIFIED IDEOGRAPH
    {0xEEBD, 0x8E50}, //16017 #CJK UNIFIED IDEOGRAPH
    {0xEEBE, 0x8E4C}, //16018 #CJK UNIFIED IDEOGRAPH
    {0xEEBF, 0x8E47}, //16019 #CJK UNIFIED IDEOGRAPH
    {0xEEC0, 0x8F43}, //16020 #CJK UNIFIED IDEOGRAPH
    {0xEEC1, 0x8F40}, //16021 #CJK UNIFIED IDEOGRAPH
    {0xEEC2, 0x9085}, //16022 #CJK UNIFIED IDEOGRAPH
    {0xEEC3, 0x907E}, //16023 #CJK UNIFIED IDEOGRAPH
    {0xEEC4, 0x9138}, //16024 #CJK UNIFIED IDEOGRAPH
    {0xEEC5, 0x919A}, //16025 #CJK UNIFIED IDEOGRAPH
    {0xEEC6, 0x91A2}, //16026 #CJK UNIFIED IDEOGRAPH
    {0xEEC7, 0x919B}, //16027 #CJK UNIFIED IDEOGRAPH
    {0xEEC8, 0x9199}, //16028 #CJK UNIFIED IDEOGRAPH
    {0xEEC9, 0x919F}, //16029 #CJK UNIFIED IDEOGRAPH
    {0xEECA, 0x91A1}, //16030 #CJK UNIFIED IDEOGRAPH
    {0xEECB, 0x919D}, //16031 #CJK UNIFIED IDEOGRAPH
    {0xEECC, 0x91A0}, //16032 #CJK UNIFIED IDEOGRAPH
    {0xEECD, 0x93A1}, //16033 #CJK UNIFIED IDEOGRAPH
    {0xEECE, 0x9383}, //16034 #CJK UNIFIED IDEOGRAPH
    {0xEECF, 0x93AF}, //16035 #CJK UNIFIED IDEOGRAPH
    {0xEED0, 0x9364}, //16036 #CJK UNIFIED IDEOGRAPH
    {0xEED1, 0x9356}, //16037 #CJK UNIFIED IDEOGRAPH
    {0xEED2, 0x9347}, //16038 #CJK UNIFIED IDEOGRAPH
    {0xEED3, 0x937C}, //16039 #CJK UNIFIED IDEOGRAPH
    {0xEED4, 0x9358}, //16040 #CJK UNIFIED IDEOGRAPH
    {0xEED5, 0x935C}, //16041 #CJK UNIFIED IDEOGRAPH
    {0xEED6, 0x9376}, //16042 #CJK UNIFIED IDEOGRAPH
    {0xEED7, 0x9349}, //16043 #CJK UNIFIED IDEOGRAPH
    {0xEED8, 0x9350}, //16044 #CJK UNIFIED IDEOGRAPH
    {0xEED9, 0x9351}, //16045 #CJK UNIFIED IDEOGRAPH
    {0xEEDA, 0x9360}, //16046 #CJK UNIFIED IDEOGRAPH
    {0xEEDB, 0x936D}, //16047 #CJK UNIFIED IDEOGRAPH
    {0xEEDC, 0x938F}, //16048 #CJK UNIFIED IDEOGRAPH
    {0xEEDD, 0x934C}, //16049 #CJK UNIFIED IDEOGRAPH
    {0xEEDE, 0x936A}, //16050 #CJK UNIFIED IDEOGRAPH
    {0xEEDF, 0x9379}, //16051 #CJK UNIFIED IDEOGRAPH
    {0xEEE0, 0x9357}, //16052 #CJK UNIFIED IDEOGRAPH
    {0xEEE1, 0x9355}, //16053 #CJK UNIFIED IDEOGRAPH
    {0xEEE2, 0x9352}, //16054 #CJK UNIFIED IDEOGRAPH
    {0xEEE3, 0x934F}, //16055 #CJK UNIFIED IDEOGRAPH
    {0xEEE4, 0x9371}, //16056 #CJK UNIFIED IDEOGRAPH
    {0xEEE5, 0x9377}, //16057 #CJK UNIFIED IDEOGRAPH
    {0xEEE6, 0x937B}, //16058 #CJK UNIFIED IDEOGRAPH
    {0xEEE7, 0x9361}, //16059 #CJK UNIFIED IDEOGRAPH
    {0xEEE8, 0x935E}, //16060 #CJK UNIFIED IDEOGRAPH
    {0xEEE9, 0x9363}, //16061 #CJK UNIFIED IDEOGRAPH
    {0xEEEA, 0x9367}, //16062 #CJK UNIFIED IDEOGRAPH
    {0xEEEB, 0x9380}, //16063 #CJK UNIFIED IDEOGRAPH
    {0xEEEC, 0x934E}, //16064 #CJK UNIFIED IDEOGRAPH
    {0xEEED, 0x9359}, //16065 #CJK UNIFIED IDEOGRAPH
    {0xEEEE, 0x95C7}, //16066 #CJK UNIFIED IDEOGRAPH
    {0xEEEF, 0x95C0}, //16067 #CJK UNIFIED IDEOGRAPH
    {0xEEF0, 0x95C9}, //16068 #CJK UNIFIED IDEOGRAPH
    {0xEEF1, 0x95C3}, //16069 #CJK UNIFIED IDEOGRAPH
    {0xEEF2, 0x95C5}, //16070 #CJK UNIFIED IDEOGRAPH
    {0xEEF3, 0x95B7}, //16071 #CJK UNIFIED IDEOGRAPH
    {0xEEF4, 0x96AE}, //16072 #CJK UNIFIED IDEOGRAPH
    {0xEEF5, 0x96B0}, //16073 #CJK UNIFIED IDEOGRAPH
    {0xEEF6, 0x96AC}, //16074 #CJK UNIFIED IDEOGRAPH
    {0xEEF7, 0x9720}, //16075 #CJK UNIFIED IDEOGRAPH
    {0xEEF8, 0x971F}, //16076 #CJK UNIFIED IDEOGRAPH
    {0xEEF9, 0x9718}, //16077 #CJK UNIFIED IDEOGRAPH
    {0xEEFA, 0x971D}, //16078 #CJK UNIFIED IDEOGRAPH
    {0xEEFB, 0x9719}, //16079 #CJK UNIFIED IDEOGRAPH
    {0xEEFC, 0x979A}, //16080 #CJK UNIFIED IDEOGRAPH
    {0xEEFD, 0x97A1}, //16081 #CJK UNIFIED IDEOGRAPH
    {0xEEFE, 0x979C}, //16082 #CJK UNIFIED IDEOGRAPH
    {0xEF40, 0x979E}, //16083 #CJK UNIFIED IDEOGRAPH
    {0xEF41, 0x979D}, //16084 #CJK UNIFIED IDEOGRAPH
    {0xEF42, 0x97D5}, //16085 #CJK UNIFIED IDEOGRAPH
    {0xEF43, 0x97D4}, //16086 #CJK UNIFIED IDEOGRAPH
    {0xEF44, 0x97F1}, //16087 #CJK UNIFIED IDEOGRAPH
    {0xEF45, 0x9841}, //16088 #CJK UNIFIED IDEOGRAPH
    {0xEF46, 0x9844}, //16089 #CJK UNIFIED IDEOGRAPH
    {0xEF47, 0x984A}, //16090 #CJK UNIFIED IDEOGRAPH
    {0xEF48, 0x9849}, //16091 #CJK UNIFIED IDEOGRAPH
    {0xEF49, 0x9845}, //16092 #CJK UNIFIED IDEOGRAPH
    {0xEF4A, 0x9843}, //16093 #CJK UNIFIED IDEOGRAPH
    {0xEF4B, 0x9925}, //16094 #CJK UNIFIED IDEOGRAPH
    {0xEF4C, 0x992B}, //16095 #CJK UNIFIED IDEOGRAPH
    {0xEF4D, 0x992C}, //16096 #CJK UNIFIED IDEOGRAPH
    {0xEF4E, 0x992A}, //16097 #CJK UNIFIED IDEOGRAPH
    {0xEF4F, 0x9933}, //16098 #CJK UNIFIED IDEOGRAPH
    {0xEF50, 0x9932}, //16099 #CJK UNIFIED IDEOGRAPH
    {0xEF51, 0x992F}, //16100 #CJK UNIFIED IDEOGRAPH
    {0xEF52, 0x992D}, //16101 #CJK UNIFIED IDEOGRAPH
    {0xEF53, 0x9931}, //16102 #CJK UNIFIED IDEOGRAPH
    {0xEF54, 0x9930}, //16103 #CJK UNIFIED IDEOGRAPH
    {0xEF55, 0x9998}, //16104 #CJK UNIFIED IDEOGRAPH
    {0xEF56, 0x99A3}, //16105 #CJK UNIFIED IDEOGRAPH
    {0xEF57, 0x99A1}, //16106 #CJK UNIFIED IDEOGRAPH
    {0xEF58, 0x9A02}, //16107 #CJK UNIFIED IDEOGRAPH
    {0xEF59, 0x99FA}, //16108 #CJK UNIFIED IDEOGRAPH
    {0xEF5A, 0x99F4}, //16109 #CJK UNIFIED IDEOGRAPH
    {0xEF5B, 0x99F7}, //16110 #CJK UNIFIED IDEOGRAPH
    {0xEF5C, 0x99F9}, //16111 #CJK UNIFIED IDEOGRAPH
    {0xEF5D, 0x99F8}, //16112 #CJK UNIFIED IDEOGRAPH
    {0xEF5E, 0x99F6}, //16113 #CJK UNIFIED IDEOGRAPH
    {0xEF5F, 0x99FB}, //16114 #CJK UNIFIED IDEOGRAPH
    {0xEF60, 0x99FD}, //16115 #CJK UNIFIED IDEOGRAPH
    {0xEF61, 0x99FE}, //16116 #CJK UNIFIED IDEOGRAPH
    {0xEF62, 0x99FC}, //16117 #CJK UNIFIED IDEOGRAPH
    {0xEF63, 0x9A03}, //16118 #CJK UNIFIED IDEOGRAPH
    {0xEF64, 0x9ABE}, //16119 #CJK UNIFIED IDEOGRAPH
    {0xEF65, 0x9AFE}, //16120 #CJK UNIFIED IDEOGRAPH
    {0xEF66, 0x9AFD}, //16121 #CJK UNIFIED IDEOGRAPH
    {0xEF67, 0x9B01}, //16122 #CJK UNIFIED IDEOGRAPH
    {0xEF68, 0x9AFC}, //16123 #CJK UNIFIED IDEOGRAPH
    {0xEF69, 0x9B48}, //16124 #CJK UNIFIED IDEOGRAPH
    {0xEF6A, 0x9B9A}, //16125 #CJK UNIFIED IDEOGRAPH
    {0xEF6B, 0x9BA8}, //16126 #CJK UNIFIED IDEOGRAPH
    {0xEF6C, 0x9B9E}, //16127 #CJK UNIFIED IDEOGRAPH
    {0xEF6D, 0x9B9B}, //16128 #CJK UNIFIED IDEOGRAPH
    {0xEF6E, 0x9BA6}, //16129 #CJK UNIFIED IDEOGRAPH
    {0xEF6F, 0x9BA1}, //16130 #CJK UNIFIED IDEOGRAPH
    {0xEF70, 0x9BA5}, //16131 #CJK UNIFIED IDEOGRAPH
    {0xEF71, 0x9BA4}, //16132 #CJK UNIFIED IDEOGRAPH
    {0xEF72, 0x9B86}, //16133 #CJK UNIFIED IDEOGRAPH
    {0xEF73, 0x9BA2}, //16134 #CJK UNIFIED IDEOGRAPH
    {0xEF74, 0x9BA0}, //16135 #CJK UNIFIED IDEOGRAPH
    {0xEF75, 0x9BAF}, //16136 #CJK UNIFIED IDEOGRAPH
    {0xEF76, 0x9D33}, //16137 #CJK UNIFIED IDEOGRAPH
    {0xEF77, 0x9D41}, //16138 #CJK UNIFIED IDEOGRAPH
    {0xEF78, 0x9D67}, //16139 #CJK UNIFIED IDEOGRAPH
    {0xEF79, 0x9D36}, //16140 #CJK UNIFIED IDEOGRAPH
    {0xEF7A, 0x9D2E}, //16141 #CJK UNIFIED IDEOGRAPH
    {0xEF7B, 0x9D2F}, //16142 #CJK UNIFIED IDEOGRAPH
    {0xEF7C, 0x9D31}, //16143 #CJK UNIFIED IDEOGRAPH
    {0xEF7D, 0x9D38}, //16144 #CJK UNIFIED IDEOGRAPH
    {0xEF7E, 0x9D30}, //16145 #CJK UNIFIED IDEOGRAPH
    {0xEFA1, 0x9D45}, //16146 #CJK UNIFIED IDEOGRAPH
    {0xEFA2, 0x9D42}, //16147 #CJK UNIFIED IDEOGRAPH
    {0xEFA3, 0x9D43}, //16148 #CJK UNIFIED IDEOGRAPH
    {0xEFA4, 0x9D3E}, //16149 #CJK UNIFIED IDEOGRAPH
    {0xEFA5, 0x9D37}, //16150 #CJK UNIFIED IDEOGRAPH
    {0xEFA6, 0x9D40}, //16151 #CJK UNIFIED IDEOGRAPH
    {0xEFA7, 0x9D3D}, //16152 #CJK UNIFIED IDEOGRAPH
    {0xEFA8, 0x7FF5}, //16153 #CJK UNIFIED IDEOGRAPH
    {0xEFA9, 0x9D2D}, //16154 #CJK UNIFIED IDEOGRAPH
    {0xEFAA, 0x9E8A}, //16155 #CJK UNIFIED IDEOGRAPH
    {0xEFAB, 0x9E89}, //16156 #CJK UNIFIED IDEOGRAPH
    {0xEFAC, 0x9E8D}, //16157 #CJK UNIFIED IDEOGRAPH
    {0xEFAD, 0x9EB0}, //16158 #CJK UNIFIED IDEOGRAPH
    {0xEFAE, 0x9EC8}, //16159 #CJK UNIFIED IDEOGRAPH
    {0xEFAF, 0x9EDA}, //16160 #CJK UNIFIED IDEOGRAPH
    {0xEFB0, 0x9EFB}, //16161 #CJK UNIFIED IDEOGRAPH
    {0xEFB1, 0x9EFF}, //16162 #CJK UNIFIED IDEOGRAPH
    {0xEFB2, 0x9F24}, //16163 #CJK UNIFIED IDEOGRAPH
    {0xEFB3, 0x9F23}, //16164 #CJK UNIFIED IDEOGRAPH
    {0xEFB4, 0x9F22}, //16165 #CJK UNIFIED IDEOGRAPH
    {0xEFB5, 0x9F54}, //16166 #CJK UNIFIED IDEOGRAPH
    {0xEFB6, 0x9FA0}, //16167 #CJK UNIFIED IDEOGRAPH
    {0xEFB7, 0x5131}, //16168 #CJK UNIFIED IDEOGRAPH
    {0xEFB8, 0x512D}, //16169 #CJK UNIFIED IDEOGRAPH
    {0xEFB9, 0x512E}, //16170 #CJK UNIFIED IDEOGRAPH
    {0xEFBA, 0x5698}, //16171 #CJK UNIFIED IDEOGRAPH
    {0xEFBB, 0x569C}, //16172 #CJK UNIFIED IDEOGRAPH
    {0xEFBC, 0x5697}, //16173 #CJK UNIFIED IDEOGRAPH
    {0xEFBD, 0x569A}, //16174 #CJK UNIFIED IDEOGRAPH
    {0xEFBE, 0x569D}, //16175 #CJK UNIFIED IDEOGRAPH
    {0xEFBF, 0x5699}, //16176 #CJK UNIFIED IDEOGRAPH
    {0xEFC0, 0x5970}, //16177 #CJK UNIFIED IDEOGRAPH
    {0xEFC1, 0x5B3C}, //16178 #CJK UNIFIED IDEOGRAPH
    {0xEFC2, 0x5C69}, //16179 #CJK UNIFIED IDEOGRAPH
    {0xEFC3, 0x5C6A}, //16180 #CJK UNIFIED IDEOGRAPH
    {0xEFC4, 0x5DC0}, //16181 #CJK UNIFIED IDEOGRAPH
    {0xEFC5, 0x5E6D}, //16182 #CJK UNIFIED IDEOGRAPH
    {0xEFC6, 0x5E6E}, //16183 #CJK UNIFIED IDEOGRAPH
    {0xEFC7, 0x61D8}, //16184 #CJK UNIFIED IDEOGRAPH
    {0xEFC8, 0x61DF}, //16185 #CJK UNIFIED IDEOGRAPH
    {0xEFC9, 0x61ED}, //16186 #CJK UNIFIED IDEOGRAPH
    {0xEFCA, 0x61EE}, //16187 #CJK UNIFIED IDEOGRAPH
    {0xEFCB, 0x61F1}, //16188 #CJK UNIFIED IDEOGRAPH
    {0xEFCC, 0x61EA}, //16189 #CJK UNIFIED IDEOGRAPH
    {0xEFCD, 0x61F0}, //16190 #CJK UNIFIED IDEOGRAPH
    {0xEFCE, 0x61EB}, //16191 #CJK UNIFIED IDEOGRAPH
    {0xEFCF, 0x61D6}, //16192 #CJK UNIFIED IDEOGRAPH
    {0xEFD0, 0x61E9}, //16193 #CJK UNIFIED IDEOGRAPH
    {0xEFD1, 0x64FF}, //16194 #CJK UNIFIED IDEOGRAPH
    {0xEFD2, 0x6504}, //16195 #CJK UNIFIED IDEOGRAPH
    {0xEFD3, 0x64FD}, //16196 #CJK UNIFIED IDEOGRAPH
    {0xEFD4, 0x64F8}, //16197 #CJK UNIFIED IDEOGRAPH
    {0xEFD5, 0x6501}, //16198 #CJK UNIFIED IDEOGRAPH
    {0xEFD6, 0x6503}, //16199 #CJK UNIFIED IDEOGRAPH
    {0xEFD7, 0x64FC}, //16200 #CJK UNIFIED IDEOGRAPH
    {0xEFD8, 0x6594}, //16201 #CJK UNIFIED IDEOGRAPH
    {0xEFD9, 0x65DB}, //16202 #CJK UNIFIED IDEOGRAPH
    {0xEFDA, 0x66DA}, //16203 #CJK UNIFIED IDEOGRAPH
    {0xEFDB, 0x66DB}, //16204 #CJK UNIFIED IDEOGRAPH
    {0xEFDC, 0x66D8}, //16205 #CJK UNIFIED IDEOGRAPH
    {0xEFDD, 0x6AC5}, //16206 #CJK UNIFIED IDEOGRAPH
    {0xEFDE, 0x6AB9}, //16207 #CJK UNIFIED IDEOGRAPH
    {0xEFDF, 0x6ABD}, //16208 #CJK UNIFIED IDEOGRAPH
    {0xEFE0, 0x6AE1}, //16209 #CJK UNIFIED IDEOGRAPH
    {0xEFE1, 0x6AC6}, //16210 #CJK UNIFIED IDEOGRAPH
    {0xEFE2, 0x6ABA}, //16211 #CJK UNIFIED IDEOGRAPH
    {0xEFE3, 0x6AB6}, //16212 #CJK UNIFIED IDEOGRAPH
    {0xEFE4, 0x6AB7}, //16213 #CJK UNIFIED IDEOGRAPH
    {0xEFE5, 0x6AC7}, //16214 #CJK UNIFIED IDEOGRAPH
    {0xEFE6, 0x6AB4}, //16215 #CJK UNIFIED IDEOGRAPH
    {0xEFE7, 0x6AAD}, //16216 #CJK UNIFIED IDEOGRAPH
    {0xEFE8, 0x6B5E}, //16217 #CJK UNIFIED IDEOGRAPH
    {0xEFE9, 0x6BC9}, //16218 #CJK UNIFIED IDEOGRAPH
    {0xEFEA, 0x6C0B}, //16219 #CJK UNIFIED IDEOGRAPH
    {0xEFEB, 0x7007}, //16220 #CJK UNIFIED IDEOGRAPH
    {0xEFEC, 0x700C}, //16221 #CJK UNIFIED IDEOGRAPH
    {0xEFED, 0x700D}, //16222 #CJK UNIFIED IDEOGRAPH
    {0xEFEE, 0x7001}, //16223 #CJK UNIFIED IDEOGRAPH
    {0xEFEF, 0x7005}, //16224 #CJK UNIFIED IDEOGRAPH
    {0xEFF0, 0x7014}, //16225 #CJK UNIFIED IDEOGRAPH
    {0xEFF1, 0x700E}, //16226 #CJK UNIFIED IDEOGRAPH
    {0xEFF2, 0x6FFF}, //16227 #CJK UNIFIED IDEOGRAPH
    {0xEFF3, 0x7000}, //16228 #CJK UNIFIED IDEOGRAPH
    {0xEFF4, 0x6FFB}, //16229 #CJK UNIFIED IDEOGRAPH
    {0xEFF5, 0x7026}, //16230 #CJK UNIFIED IDEOGRAPH
    {0xEFF6, 0x6FFC}, //16231 #CJK UNIFIED IDEOGRAPH
    {0xEFF7, 0x6FF7}, //16232 #CJK UNIFIED IDEOGRAPH
    {0xEFF8, 0x700A}, //16233 #CJK UNIFIED IDEOGRAPH
    {0xEFF9, 0x7201}, //16234 #CJK UNIFIED IDEOGRAPH
    {0xEFFA, 0x71FF}, //16235 #CJK UNIFIED IDEOGRAPH
    {0xEFFB, 0x71F9}, //16236 #CJK UNIFIED IDEOGRAPH
    {0xEFFC, 0x7203}, //16237 #CJK UNIFIED IDEOGRAPH
    {0xEFFD, 0x71FD}, //16238 #CJK UNIFIED IDEOGRAPH
    {0xEFFE, 0x7376}, //16239 #CJK UNIFIED IDEOGRAPH
    {0xF040, 0x74B8}, //16240 #CJK UNIFIED IDEOGRAPH
    {0xF041, 0x74C0}, //16241 #CJK UNIFIED IDEOGRAPH
    {0xF042, 0x74B5}, //16242 #CJK UNIFIED IDEOGRAPH
    {0xF043, 0x74C1}, //16243 #CJK UNIFIED IDEOGRAPH
    {0xF044, 0x74BE}, //16244 #CJK UNIFIED IDEOGRAPH
    {0xF045, 0x74B6}, //16245 #CJK UNIFIED IDEOGRAPH
    {0xF046, 0x74BB}, //16246 #CJK UNIFIED IDEOGRAPH
    {0xF047, 0x74C2}, //16247 #CJK UNIFIED IDEOGRAPH
    {0xF048, 0x7514}, //16248 #CJK UNIFIED IDEOGRAPH
    {0xF049, 0x7513}, //16249 #CJK UNIFIED IDEOGRAPH
    {0xF04A, 0x765C}, //16250 #CJK UNIFIED IDEOGRAPH
    {0xF04B, 0x7664}, //16251 #CJK UNIFIED IDEOGRAPH
    {0xF04C, 0x7659}, //16252 #CJK UNIFIED IDEOGRAPH
    {0xF04D, 0x7650}, //16253 #CJK UNIFIED IDEOGRAPH
    {0xF04E, 0x7653}, //16254 #CJK UNIFIED IDEOGRAPH
    {0xF04F, 0x7657}, //16255 #CJK UNIFIED IDEOGRAPH
    {0xF050, 0x765A}, //16256 #CJK UNIFIED IDEOGRAPH
    {0xF051, 0x76A6}, //16257 #CJK UNIFIED IDEOGRAPH
    {0xF052, 0x76BD}, //16258 #CJK UNIFIED IDEOGRAPH
    {0xF053, 0x76EC}, //16259 #CJK UNIFIED IDEOGRAPH
    {0xF054, 0x77C2}, //16260 #CJK UNIFIED IDEOGRAPH
    {0xF055, 0x77BA}, //16261 #CJK UNIFIED IDEOGRAPH
    {0xF056, 0x78FF}, //16262 #CJK UNIFIED IDEOGRAPH
    {0xF057, 0x790C}, //16263 #CJK UNIFIED IDEOGRAPH
    {0xF058, 0x7913}, //16264 #CJK UNIFIED IDEOGRAPH
    {0xF059, 0x7914}, //16265 #CJK UNIFIED IDEOGRAPH
    {0xF05A, 0x7909}, //16266 #CJK UNIFIED IDEOGRAPH
    {0xF05B, 0x7910}, //16267 #CJK UNIFIED IDEOGRAPH
    {0xF05C, 0x7912}, //16268 #CJK UNIFIED IDEOGRAPH
    {0xF05D, 0x7911}, //16269 #CJK UNIFIED IDEOGRAPH
    {0xF05E, 0x79AD}, //16270 #CJK UNIFIED IDEOGRAPH
    {0xF05F, 0x79AC}, //16271 #CJK UNIFIED IDEOGRAPH
    {0xF060, 0x7A5F}, //16272 #CJK UNIFIED IDEOGRAPH
    {0xF061, 0x7C1C}, //16273 #CJK UNIFIED IDEOGRAPH
    {0xF062, 0x7C29}, //16274 #CJK UNIFIED IDEOGRAPH
    {0xF063, 0x7C19}, //16275 #CJK UNIFIED IDEOGRAPH
    {0xF064, 0x7C20}, //16276 #CJK UNIFIED IDEOGRAPH
    {0xF065, 0x7C1F}, //16277 #CJK UNIFIED IDEOGRAPH
    {0xF066, 0x7C2D}, //16278 #CJK UNIFIED IDEOGRAPH
    {0xF067, 0x7C1D}, //16279 #CJK UNIFIED IDEOGRAPH
    {0xF068, 0x7C26}, //16280 #CJK UNIFIED IDEOGRAPH
    {0xF069, 0x7C28}, //16281 #CJK UNIFIED IDEOGRAPH
    {0xF06A, 0x7C22}, //16282 #CJK UNIFIED IDEOGRAPH
    {0xF06B, 0x7C25}, //16283 #CJK UNIFIED IDEOGRAPH
    {0xF06C, 0x7C30}, //16284 #CJK UNIFIED IDEOGRAPH
    {0xF06D, 0x7E5C}, //16285 #CJK UNIFIED IDEOGRAPH
    {0xF06E, 0x7E50}, //16286 #CJK UNIFIED IDEOGRAPH
    {0xF06F, 0x7E56}, //16287 #CJK UNIFIED IDEOGRAPH
    {0xF070, 0x7E63}, //16288 #CJK UNIFIED IDEOGRAPH
    {0xF071, 0x7E58}, //16289 #CJK UNIFIED IDEOGRAPH
    {0xF072, 0x7E62}, //16290 #CJK UNIFIED IDEOGRAPH
    {0xF073, 0x7E5F}, //16291 #CJK UNIFIED IDEOGRAPH
    {0xF074, 0x7E51}, //16292 #CJK UNIFIED IDEOGRAPH
    {0xF075, 0x7E60}, //16293 #CJK UNIFIED IDEOGRAPH
    {0xF076, 0x7E57}, //16294 #CJK UNIFIED IDEOGRAPH
    {0xF077, 0x7E53}, //16295 #CJK UNIFIED IDEOGRAPH
    {0xF078, 0x7FB5}, //16296 #CJK UNIFIED IDEOGRAPH
    {0xF079, 0x7FB3}, //16297 #CJK UNIFIED IDEOGRAPH
    {0xF07A, 0x7FF7}, //16298 #CJK UNIFIED IDEOGRAPH
    {0xF07B, 0x7FF8}, //16299 #CJK UNIFIED IDEOGRAPH
    {0xF07C, 0x8075}, //16300 #CJK UNIFIED IDEOGRAPH
    {0xF07D, 0x81D1}, //16301 #CJK UNIFIED IDEOGRAPH
    {0xF07E, 0x81D2}, //16302 #CJK UNIFIED IDEOGRAPH
    {0xF0A1, 0x81D0}, //16303 #CJK UNIFIED IDEOGRAPH
    {0xF0A2, 0x825F}, //16304 #CJK UNIFIED IDEOGRAPH
    {0xF0A3, 0x825E}, //16305 #CJK UNIFIED IDEOGRAPH
    {0xF0A4, 0x85B4}, //16306 #CJK UNIFIED IDEOGRAPH
    {0xF0A5, 0x85C6}, //16307 #CJK UNIFIED IDEOGRAPH
    {0xF0A6, 0x85C0}, //16308 #CJK UNIFIED IDEOGRAPH
    {0xF0A7, 0x85C3}, //16309 #CJK UNIFIED IDEOGRAPH
    {0xF0A8, 0x85C2}, //16310 #CJK UNIFIED IDEOGRAPH
    {0xF0A9, 0x85B3}, //16311 #CJK UNIFIED IDEOGRAPH
    {0xF0AA, 0x85B5}, //16312 #CJK UNIFIED IDEOGRAPH
    {0xF0AB, 0x85BD}, //16313 #CJK UNIFIED IDEOGRAPH
    {0xF0AC, 0x85C7}, //16314 #CJK UNIFIED IDEOGRAPH
    {0xF0AD, 0x85C4}, //16315 #CJK UNIFIED IDEOGRAPH
    {0xF0AE, 0x85BF}, //16316 #CJK UNIFIED IDEOGRAPH
    {0xF0AF, 0x85CB}, //16317 #CJK UNIFIED IDEOGRAPH
    {0xF0B0, 0x85CE}, //16318 #CJK UNIFIED IDEOGRAPH
    {0xF0B1, 0x85C8}, //16319 #CJK UNIFIED IDEOGRAPH
    {0xF0B2, 0x85C5}, //16320 #CJK UNIFIED IDEOGRAPH
    {0xF0B3, 0x85B1}, //16321 #CJK UNIFIED IDEOGRAPH
    {0xF0B4, 0x85B6}, //16322 #CJK UNIFIED IDEOGRAPH
    {0xF0B5, 0x85D2}, //16323 #CJK UNIFIED IDEOGRAPH
    {0xF0B6, 0x8624}, //16324 #CJK UNIFIED IDEOGRAPH
    {0xF0B7, 0x85B8}, //16325 #CJK UNIFIED IDEOGRAPH
    {0xF0B8, 0x85B7}, //16326 #CJK UNIFIED IDEOGRAPH
    {0xF0B9, 0x85BE}, //16327 #CJK UNIFIED IDEOGRAPH
    {0xF0BA, 0x8669}, //16328 #CJK UNIFIED IDEOGRAPH
    {0xF0BB, 0x87E7}, //16329 #CJK UNIFIED IDEOGRAPH
    {0xF0BC, 0x87E6}, //16330 #CJK UNIFIED IDEOGRAPH
    {0xF0BD, 0x87E2}, //16331 #CJK UNIFIED IDEOGRAPH
    {0xF0BE, 0x87DB}, //16332 #CJK UNIFIED IDEOGRAPH
    {0xF0BF, 0x87EB}, //16333 #CJK UNIFIED IDEOGRAPH
    {0xF0C0, 0x87EA}, //16334 #CJK UNIFIED IDEOGRAPH
    {0xF0C1, 0x87E5}, //16335 #CJK UNIFIED IDEOGRAPH
    {0xF0C2, 0x87DF}, //16336 #CJK UNIFIED IDEOGRAPH
    {0xF0C3, 0x87F3}, //16337 #CJK UNIFIED IDEOGRAPH
    {0xF0C4, 0x87E4}, //16338 #CJK UNIFIED IDEOGRAPH
    {0xF0C5, 0x87D4}, //16339 #CJK UNIFIED IDEOGRAPH
    {0xF0C6, 0x87DC}, //16340 #CJK UNIFIED IDEOGRAPH
    {0xF0C7, 0x87D3}, //16341 #CJK UNIFIED IDEOGRAPH
    {0xF0C8, 0x87ED}, //16342 #CJK UNIFIED IDEOGRAPH
    {0xF0C9, 0x87D8}, //16343 #CJK UNIFIED IDEOGRAPH
    {0xF0CA, 0x87E3}, //16344 #CJK UNIFIED IDEOGRAPH
    {0xF0CB, 0x87A4}, //16345 #CJK UNIFIED IDEOGRAPH
    {0xF0CC, 0x87D7}, //16346 #CJK UNIFIED IDEOGRAPH
    {0xF0CD, 0x87D9}, //16347 #CJK UNIFIED IDEOGRAPH
    {0xF0CE, 0x8801}, //16348 #CJK UNIFIED IDEOGRAPH
    {0xF0CF, 0x87F4}, //16349 #CJK UNIFIED IDEOGRAPH
    {0xF0D0, 0x87E8}, //16350 #CJK UNIFIED IDEOGRAPH
    {0xF0D1, 0x87DD}, //16351 #CJK UNIFIED IDEOGRAPH
    {0xF0D2, 0x8953}, //16352 #CJK UNIFIED IDEOGRAPH
    {0xF0D3, 0x894B}, //16353 #CJK UNIFIED IDEOGRAPH
    {0xF0D4, 0x894F}, //16354 #CJK UNIFIED IDEOGRAPH
    {0xF0D5, 0x894C}, //16355 #CJK UNIFIED IDEOGRAPH
    {0xF0D6, 0x8946}, //16356 #CJK UNIFIED IDEOGRAPH
    {0xF0D7, 0x8950}, //16357 #CJK UNIFIED IDEOGRAPH
    {0xF0D8, 0x8951}, //16358 #CJK UNIFIED IDEOGRAPH
    {0xF0D9, 0x8949}, //16359 #CJK UNIFIED IDEOGRAPH
    {0xF0DA, 0x8B2A}, //16360 #CJK UNIFIED IDEOGRAPH
    {0xF0DB, 0x8B27}, //16361 #CJK UNIFIED IDEOGRAPH
    {0xF0DC, 0x8B23}, //16362 #CJK UNIFIED IDEOGRAPH
    {0xF0DD, 0x8B33}, //16363 #CJK UNIFIED IDEOGRAPH
    {0xF0DE, 0x8B30}, //16364 #CJK UNIFIED IDEOGRAPH
    {0xF0DF, 0x8B35}, //16365 #CJK UNIFIED IDEOGRAPH
    {0xF0E0, 0x8B47}, //16366 #CJK UNIFIED IDEOGRAPH
    {0xF0E1, 0x8B2F}, //16367 #CJK UNIFIED IDEOGRAPH
    {0xF0E2, 0x8B3C}, //16368 #CJK UNIFIED IDEOGRAPH
    {0xF0E3, 0x8B3E}, //16369 #CJK UNIFIED IDEOGRAPH
    {0xF0E4, 0x8B31}, //16370 #CJK UNIFIED IDEOGRAPH
    {0xF0E5, 0x8B25}, //16371 #CJK UNIFIED IDEOGRAPH
    {0xF0E6, 0x8B37}, //16372 #CJK UNIFIED IDEOGRAPH
    {0xF0E7, 0x8B26}, //16373 #CJK UNIFIED IDEOGRAPH
    {0xF0E8, 0x8B36}, //16374 #CJK UNIFIED IDEOGRAPH
    {0xF0E9, 0x8B2E}, //16375 #CJK UNIFIED IDEOGRAPH
    {0xF0EA, 0x8B24}, //16376 #CJK UNIFIED IDEOGRAPH
    {0xF0EB, 0x8B3B}, //16377 #CJK UNIFIED IDEOGRAPH
    {0xF0EC, 0x8B3D}, //16378 #CJK UNIFIED IDEOGRAPH
    {0xF0ED, 0x8B3A}, //16379 #CJK UNIFIED IDEOGRAPH
    {0xF0EE, 0x8C42}, //16380 #CJK UNIFIED IDEOGRAPH
    {0xF0EF, 0x8C75}, //16381 #CJK UNIFIED IDEOGRAPH
    {0xF0F0, 0x8C99}, //16382 #CJK UNIFIED IDEOGRAPH
    {0xF0F1, 0x8C98}, //16383 #CJK UNIFIED IDEOGRAPH
    {0xF0F2, 0x8C97}, //16384 #CJK UNIFIED IDEOGRAPH
    {0xF0F3, 0x8CFE}, //16385 #CJK UNIFIED IDEOGRAPH
    {0xF0F4, 0x8D04}, //16386 #CJK UNIFIED IDEOGRAPH
    {0xF0F5, 0x8D02}, //16387 #CJK UNIFIED IDEOGRAPH
    {0xF0F6, 0x8D00}, //16388 #CJK UNIFIED IDEOGRAPH
    {0xF0F7, 0x8E5C}, //16389 #CJK UNIFIED IDEOGRAPH
    {0xF0F8, 0x8E62}, //16390 #CJK UNIFIED IDEOGRAPH
    {0xF0F9, 0x8E60}, //16391 #CJK UNIFIED IDEOGRAPH
    {0xF0FA, 0x8E57}, //16392 #CJK UNIFIED IDEOGRAPH
    {0xF0FB, 0x8E56}, //16393 #CJK UNIFIED IDEOGRAPH
    {0xF0FC, 0x8E5E}, //16394 #CJK UNIFIED IDEOGRAPH
    {0xF0FD, 0x8E65}, //16395 #CJK UNIFIED IDEOGRAPH
    {0xF0FE, 0x8E67}, //16396 #CJK UNIFIED IDEOGRAPH
    {0xF140, 0x8E5B}, //16397 #CJK UNIFIED IDEOGRAPH
    {0xF141, 0x8E5A}, //16398 #CJK UNIFIED IDEOGRAPH
    {0xF142, 0x8E61}, //16399 #CJK UNIFIED IDEOGRAPH
    {0xF143, 0x8E5D}, //16400 #CJK UNIFIED IDEOGRAPH
    {0xF144, 0x8E69}, //16401 #CJK UNIFIED IDEOGRAPH
    {0xF145, 0x8E54}, //16402 #CJK UNIFIED IDEOGRAPH
    {0xF146, 0x8F46}, //16403 #CJK UNIFIED IDEOGRAPH
    {0xF147, 0x8F47}, //16404 #CJK UNIFIED IDEOGRAPH
    {0xF148, 0x8F48}, //16405 #CJK UNIFIED IDEOGRAPH
    {0xF149, 0x8F4B}, //16406 #CJK UNIFIED IDEOGRAPH
    {0xF14A, 0x9128}, //16407 #CJK UNIFIED IDEOGRAPH
    {0xF14B, 0x913A}, //16408 #CJK UNIFIED IDEOGRAPH
    {0xF14C, 0x913B}, //16409 #CJK UNIFIED IDEOGRAPH
    {0xF14D, 0x913E}, //16410 #CJK UNIFIED IDEOGRAPH
    {0xF14E, 0x91A8}, //16411 #CJK UNIFIED IDEOGRAPH
    {0xF14F, 0x91A5}, //16412 #CJK UNIFIED IDEOGRAPH
    {0xF150, 0x91A7}, //16413 #CJK UNIFIED IDEOGRAPH
    {0xF151, 0x91AF}, //16414 #CJK UNIFIED IDEOGRAPH
    {0xF152, 0x91AA}, //16415 #CJK UNIFIED IDEOGRAPH
    {0xF153, 0x93B5}, //16416 #CJK UNIFIED IDEOGRAPH
    {0xF154, 0x938C}, //16417 #CJK UNIFIED IDEOGRAPH
    {0xF155, 0x9392}, //16418 #CJK UNIFIED IDEOGRAPH
    {0xF156, 0x93B7}, //16419 #CJK UNIFIED IDEOGRAPH
    {0xF157, 0x939B}, //16420 #CJK UNIFIED IDEOGRAPH
    {0xF158, 0x939D}, //16421 #CJK UNIFIED IDEOGRAPH
    {0xF159, 0x9389}, //16422 #CJK UNIFIED IDEOGRAPH
    {0xF15A, 0x93A7}, //16423 #CJK UNIFIED IDEOGRAPH
    {0xF15B, 0x938E}, //16424 #CJK UNIFIED IDEOGRAPH
    {0xF15C, 0x93AA}, //16425 #CJK UNIFIED IDEOGRAPH
    {0xF15D, 0x939E}, //16426 #CJK UNIFIED IDEOGRAPH
    {0xF15E, 0x93A6}, //16427 #CJK UNIFIED IDEOGRAPH
    {0xF15F, 0x9395}, //16428 #CJK UNIFIED IDEOGRAPH
    {0xF160, 0x9388}, //16429 #CJK UNIFIED IDEOGRAPH
    {0xF161, 0x9399}, //16430 #CJK UNIFIED IDEOGRAPH
    {0xF162, 0x939F}, //16431 #CJK UNIFIED IDEOGRAPH
    {0xF163, 0x938D}, //16432 #CJK UNIFIED IDEOGRAPH
    {0xF164, 0x93B1}, //16433 #CJK UNIFIED IDEOGRAPH
    {0xF165, 0x9391}, //16434 #CJK UNIFIED IDEOGRAPH
    {0xF166, 0x93B2}, //16435 #CJK UNIFIED IDEOGRAPH
    {0xF167, 0x93A4}, //16436 #CJK UNIFIED IDEOGRAPH
    {0xF168, 0x93A8}, //16437 #CJK UNIFIED IDEOGRAPH
    {0xF169, 0x93B4}, //16438 #CJK UNIFIED IDEOGRAPH
    {0xF16A, 0x93A3}, //16439 #CJK UNIFIED IDEOGRAPH
    {0xF16B, 0x93A5}, //16440 #CJK UNIFIED IDEOGRAPH
    {0xF16C, 0x95D2}, //16441 #CJK UNIFIED IDEOGRAPH
    {0xF16D, 0x95D3}, //16442 #CJK UNIFIED IDEOGRAPH
    {0xF16E, 0x95D1}, //16443 #CJK UNIFIED IDEOGRAPH
    {0xF16F, 0x96B3}, //16444 #CJK UNIFIED IDEOGRAPH
    {0xF170, 0x96D7}, //16445 #CJK UNIFIED IDEOGRAPH
    {0xF171, 0x96DA}, //16446 #CJK UNIFIED IDEOGRAPH
    {0xF172, 0x5DC2}, //16447 #CJK UNIFIED IDEOGRAPH
    {0xF173, 0x96DF}, //16448 #CJK UNIFIED IDEOGRAPH
    {0xF174, 0x96D8}, //16449 #CJK UNIFIED IDEOGRAPH
    {0xF175, 0x96DD}, //16450 #CJK UNIFIED IDEOGRAPH
    {0xF176, 0x9723}, //16451 #CJK UNIFIED IDEOGRAPH
    {0xF177, 0x9722}, //16452 #CJK UNIFIED IDEOGRAPH
    {0xF178, 0x9725}, //16453 #CJK UNIFIED IDEOGRAPH
    {0xF179, 0x97AC}, //16454 #CJK UNIFIED IDEOGRAPH
    {0xF17A, 0x97AE}, //16455 #CJK UNIFIED IDEOGRAPH
    {0xF17B, 0x97A8}, //16456 #CJK UNIFIED IDEOGRAPH
    {0xF17C, 0x97AB}, //16457 #CJK UNIFIED IDEOGRAPH
    {0xF17D, 0x97A4}, //16458 #CJK UNIFIED IDEOGRAPH
    {0xF17E, 0x97AA}, //16459 #CJK UNIFIED IDEOGRAPH
    {0xF1A1, 0x97A2}, //16460 #CJK UNIFIED IDEOGRAPH
    {0xF1A2, 0x97A5}, //16461 #CJK UNIFIED IDEOGRAPH
    {0xF1A3, 0x97D7}, //16462 #CJK UNIFIED IDEOGRAPH
    {0xF1A4, 0x97D9}, //16463 #CJK UNIFIED IDEOGRAPH
    {0xF1A5, 0x97D6}, //16464 #CJK UNIFIED IDEOGRAPH
    {0xF1A6, 0x97D8}, //16465 #CJK UNIFIED IDEOGRAPH
    {0xF1A7, 0x97FA}, //16466 #CJK UNIFIED IDEOGRAPH
    {0xF1A8, 0x9850}, //16467 #CJK UNIFIED IDEOGRAPH
    {0xF1A9, 0x9851}, //16468 #CJK UNIFIED IDEOGRAPH
    {0xF1AA, 0x9852}, //16469 #CJK UNIFIED IDEOGRAPH
    {0xF1AB, 0x98B8}, //16470 #CJK UNIFIED IDEOGRAPH
    {0xF1AC, 0x9941}, //16471 #CJK UNIFIED IDEOGRAPH
    {0xF1AD, 0x993C}, //16472 #CJK UNIFIED IDEOGRAPH
    {0xF1AE, 0x993A}, //16473 #CJK UNIFIED IDEOGRAPH
    {0xF1AF, 0x9A0F}, //16474 #CJK UNIFIED IDEOGRAPH
    {0xF1B0, 0x9A0B}, //16475 #CJK UNIFIED IDEOGRAPH
    {0xF1B1, 0x9A09}, //16476 #CJK UNIFIED IDEOGRAPH
    {0xF1B2, 0x9A0D}, //16477 #CJK UNIFIED IDEOGRAPH
    {0xF1B3, 0x9A04}, //16478 #CJK UNIFIED IDEOGRAPH
    {0xF1B4, 0x9A11}, //16479 #CJK UNIFIED IDEOGRAPH
    {0xF1B5, 0x9A0A}, //16480 #CJK UNIFIED IDEOGRAPH
    {0xF1B6, 0x9A05}, //16481 #CJK UNIFIED IDEOGRAPH
    {0xF1B7, 0x9A07}, //16482 #CJK UNIFIED IDEOGRAPH
    {0xF1B8, 0x9A06}, //16483 #CJK UNIFIED IDEOGRAPH
    {0xF1B9, 0x9AC0}, //16484 #CJK UNIFIED IDEOGRAPH
    {0xF1BA, 0x9ADC}, //16485 #CJK UNIFIED IDEOGRAPH
    {0xF1BB, 0x9B08}, //16486 #CJK UNIFIED IDEOGRAPH
    {0xF1BC, 0x9B04}, //16487 #CJK UNIFIED IDEOGRAPH
    {0xF1BD, 0x9B05}, //16488 #CJK UNIFIED IDEOGRAPH
    {0xF1BE, 0x9B29}, //16489 #CJK UNIFIED IDEOGRAPH
    {0xF1BF, 0x9B35}, //16490 #CJK UNIFIED IDEOGRAPH
    {0xF1C0, 0x9B4A}, //16491 #CJK UNIFIED IDEOGRAPH
    {0xF1C1, 0x9B4C}, //16492 #CJK UNIFIED IDEOGRAPH
    {0xF1C2, 0x9B4B}, //16493 #CJK UNIFIED IDEOGRAPH
    {0xF1C3, 0x9BC7}, //16494 #CJK UNIFIED IDEOGRAPH
    {0xF1C4, 0x9BC6}, //16495 #CJK UNIFIED IDEOGRAPH
    {0xF1C5, 0x9BC3}, //16496 #CJK UNIFIED IDEOGRAPH
    {0xF1C6, 0x9BBF}, //16497 #CJK UNIFIED IDEOGRAPH
    {0xF1C7, 0x9BC1}, //16498 #CJK UNIFIED IDEOGRAPH
    {0xF1C8, 0x9BB5}, //16499 #CJK UNIFIED IDEOGRAPH
    {0xF1C9, 0x9BB8}, //16500 #CJK UNIFIED IDEOGRAPH
    {0xF1CA, 0x9BD3}, //16501 #CJK UNIFIED IDEOGRAPH
    {0xF1CB, 0x9BB6}, //16502 #CJK UNIFIED IDEOGRAPH
    {0xF1CC, 0x9BC4}, //16503 #CJK UNIFIED IDEOGRAPH
    {0xF1CD, 0x9BB9}, //16504 #CJK UNIFIED IDEOGRAPH
    {0xF1CE, 0x9BBD}, //16505 #CJK UNIFIED IDEOGRAPH
    {0xF1CF, 0x9D5C}, //16506 #CJK UNIFIED IDEOGRAPH
    {0xF1D0, 0x9D53}, //16507 #CJK UNIFIED IDEOGRAPH
    {0xF1D1, 0x9D4F}, //16508 #CJK UNIFIED IDEOGRAPH
    {0xF1D2, 0x9D4A}, //16509 #CJK UNIFIED IDEOGRAPH
    {0xF1D3, 0x9D5B}, //16510 #CJK UNIFIED IDEOGRAPH
    {0xF1D4, 0x9D4B}, //16511 #CJK UNIFIED IDEOGRAPH
    {0xF1D5, 0x9D59}, //16512 #CJK UNIFIED IDEOGRAPH
    {0xF1D6, 0x9D56}, //16513 #CJK UNIFIED IDEOGRAPH
    {0xF1D7, 0x9D4C}, //16514 #CJK UNIFIED IDEOGRAPH
    {0xF1D8, 0x9D57}, //16515 #CJK UNIFIED IDEOGRAPH
    {0xF1D9, 0x9D52}, //16516 #CJK UNIFIED IDEOGRAPH
    {0xF1DA, 0x9D54}, //16517 #CJK UNIFIED IDEOGRAPH
    {0xF1DB, 0x9D5F}, //16518 #CJK UNIFIED IDEOGRAPH
    {0xF1DC, 0x9D58}, //16519 #CJK UNIFIED IDEOGRAPH
    {0xF1DD, 0x9D5A}, //16520 #CJK UNIFIED IDEOGRAPH
    {0xF1DE, 0x9E8E}, //16521 #CJK UNIFIED IDEOGRAPH
    {0xF1DF, 0x9E8C}, //16522 #CJK UNIFIED IDEOGRAPH
    {0xF1E0, 0x9EDF}, //16523 #CJK UNIFIED IDEOGRAPH
    {0xF1E1, 0x9F01}, //16524 #CJK UNIFIED IDEOGRAPH
    {0xF1E2, 0x9F00}, //16525 #CJK UNIFIED IDEOGRAPH
    {0xF1E3, 0x9F16}, //16526 #CJK UNIFIED IDEOGRAPH
    {0xF1E4, 0x9F25}, //16527 #CJK UNIFIED IDEOGRAPH
    {0xF1E5, 0x9F2B}, //16528 #CJK UNIFIED IDEOGRAPH
    {0xF1E6, 0x9F2A}, //16529 #CJK UNIFIED IDEOGRAPH
    {0xF1E7, 0x9F29}, //16530 #CJK UNIFIED IDEOGRAPH
    {0xF1E8, 0x9F28}, //16531 #CJK UNIFIED IDEOGRAPH
    {0xF1E9, 0x9F4C}, //16532 #CJK UNIFIED IDEOGRAPH
    {0xF1EA, 0x9F55}, //16533 #CJK UNIFIED IDEOGRAPH
    {0xF1EB, 0x5134}, //16534 #CJK UNIFIED IDEOGRAPH
    {0xF1EC, 0x5135}, //16535 #CJK UNIFIED IDEOGRAPH
    {0xF1ED, 0x5296}, //16536 #CJK UNIFIED IDEOGRAPH
    {0xF1EE, 0x52F7}, //16537 #CJK UNIFIED IDEOGRAPH
    {0xF1EF, 0x53B4}, //16538 #CJK UNIFIED IDEOGRAPH
    {0xF1F0, 0x56AB}, //16539 #CJK UNIFIED IDEOGRAPH
    {0xF1F1, 0x56AD}, //16540 #CJK UNIFIED IDEOGRAPH
    {0xF1F2, 0x56A6}, //16541 #CJK UNIFIED IDEOGRAPH
    {0xF1F3, 0x56A7}, //16542 #CJK UNIFIED IDEOGRAPH
    {0xF1F4, 0x56AA}, //16543 #CJK UNIFIED IDEOGRAPH
    {0xF1F5, 0x56AC}, //16544 #CJK UNIFIED IDEOGRAPH
    {0xF1F6, 0x58DA}, //16545 #CJK UNIFIED IDEOGRAPH
    {0xF1F7, 0x58DD}, //16546 #CJK UNIFIED IDEOGRAPH
    {0xF1F8, 0x58DB}, //16547 #CJK UNIFIED IDEOGRAPH
    {0xF1F9, 0x5912}, //16548 #CJK UNIFIED IDEOGRAPH
    {0xF1FA, 0x5B3D}, //16549 #CJK UNIFIED IDEOGRAPH
    {0xF1FB, 0x5B3E}, //16550 #CJK UNIFIED IDEOGRAPH
    {0xF1FC, 0x5B3F}, //16551 #CJK UNIFIED IDEOGRAPH
    {0xF1FD, 0x5DC3}, //16552 #CJK UNIFIED IDEOGRAPH
    {0xF1FE, 0x5E70}, //16553 #CJK UNIFIED IDEOGRAPH
    {0xF240, 0x5FBF}, //16554 #CJK UNIFIED IDEOGRAPH
    {0xF241, 0x61FB}, //16555 #CJK UNIFIED IDEOGRAPH
    {0xF242, 0x6507}, //16556 #CJK UNIFIED IDEOGRAPH
    {0xF243, 0x6510}, //16557 #CJK UNIFIED IDEOGRAPH
    {0xF244, 0x650D}, //16558 #CJK UNIFIED IDEOGRAPH
    {0xF245, 0x6509}, //16559 #CJK UNIFIED IDEOGRAPH
    {0xF246, 0x650C}, //16560 #CJK UNIFIED IDEOGRAPH
    {0xF247, 0x650E}, //16561 #CJK UNIFIED IDEOGRAPH
    {0xF248, 0x6584}, //16562 #CJK UNIFIED IDEOGRAPH
    {0xF249, 0x65DE}, //16563 #CJK UNIFIED IDEOGRAPH
    {0xF24A, 0x65DD}, //16564 #CJK UNIFIED IDEOGRAPH
    {0xF24B, 0x66DE}, //16565 #CJK UNIFIED IDEOGRAPH
    {0xF24C, 0x6AE7}, //16566 #CJK UNIFIED IDEOGRAPH
    {0xF24D, 0x6AE0}, //16567 #CJK UNIFIED IDEOGRAPH
    {0xF24E, 0x6ACC}, //16568 #CJK UNIFIED IDEOGRAPH
    {0xF24F, 0x6AD1}, //16569 #CJK UNIFIED IDEOGRAPH
    {0xF250, 0x6AD9}, //16570 #CJK UNIFIED IDEOGRAPH
    {0xF251, 0x6ACB}, //16571 #CJK UNIFIED IDEOGRAPH
    {0xF252, 0x6ADF}, //16572 #CJK UNIFIED IDEOGRAPH
    {0xF253, 0x6ADC}, //16573 #CJK UNIFIED IDEOGRAPH
    {0xF254, 0x6AD0}, //16574 #CJK UNIFIED IDEOGRAPH
    {0xF255, 0x6AEB}, //16575 #CJK UNIFIED IDEOGRAPH
    {0xF256, 0x6ACF}, //16576 #CJK UNIFIED IDEOGRAPH
    {0xF257, 0x6ACD}, //16577 #CJK UNIFIED IDEOGRAPH
    {0xF258, 0x6ADE}, //16578 #CJK UNIFIED IDEOGRAPH
    {0xF259, 0x6B60}, //16579 #CJK UNIFIED IDEOGRAPH
    {0xF25A, 0x6BB0}, //16580 #CJK UNIFIED IDEOGRAPH
    {0xF25B, 0x6C0C}, //16581 #CJK UNIFIED IDEOGRAPH
    {0xF25C, 0x7019}, //16582 #CJK UNIFIED IDEOGRAPH
    {0xF25D, 0x7027}, //16583 #CJK UNIFIED IDEOGRAPH
    {0xF25E, 0x7020}, //16584 #CJK UNIFIED IDEOGRAPH
    {0xF25F, 0x7016}, //16585 #CJK UNIFIED IDEOGRAPH
    {0xF260, 0x702B}, //16586 #CJK UNIFIED IDEOGRAPH
    {0xF261, 0x7021}, //16587 #CJK UNIFIED IDEOGRAPH
    {0xF262, 0x7022}, //16588 #CJK UNIFIED IDEOGRAPH
    {0xF263, 0x7023}, //16589 #CJK UNIFIED IDEOGRAPH
    {0xF264, 0x7029}, //16590 #CJK UNIFIED IDEOGRAPH
    {0xF265, 0x7017}, //16591 #CJK UNIFIED IDEOGRAPH
    {0xF266, 0x7024}, //16592 #CJK UNIFIED IDEOGRAPH
    {0xF267, 0x701C}, //16593 #CJK UNIFIED IDEOGRAPH
    {0xF268, 0x702A}, //16594 #CJK UNIFIED IDEOGRAPH
    {0xF269, 0x720C}, //16595 #CJK UNIFIED IDEOGRAPH
    {0xF26A, 0x720A}, //16596 #CJK UNIFIED IDEOGRAPH
    {0xF26B, 0x7207}, //16597 #CJK UNIFIED IDEOGRAPH
    {0xF26C, 0x7202}, //16598 #CJK UNIFIED IDEOGRAPH
    {0xF26D, 0x7205}, //16599 #CJK UNIFIED IDEOGRAPH
    {0xF26E, 0x72A5}, //16600 #CJK UNIFIED IDEOGRAPH
    {0xF26F, 0x72A6}, //16601 #CJK UNIFIED IDEOGRAPH
    {0xF270, 0x72A4}, //16602 #CJK UNIFIED IDEOGRAPH
    {0xF271, 0x72A3}, //16603 #CJK UNIFIED IDEOGRAPH
    {0xF272, 0x72A1}, //16604 #CJK UNIFIED IDEOGRAPH
    {0xF273, 0x74CB}, //16605 #CJK UNIFIED IDEOGRAPH
    {0xF274, 0x74C5}, //16606 #CJK UNIFIED IDEOGRAPH
    {0xF275, 0x74B7}, //16607 #CJK UNIFIED IDEOGRAPH
    {0xF276, 0x74C3}, //16608 #CJK UNIFIED IDEOGRAPH
    {0xF277, 0x7516}, //16609 #CJK UNIFIED IDEOGRAPH
    {0xF278, 0x7660}, //16610 #CJK UNIFIED IDEOGRAPH
    {0xF279, 0x77C9}, //16611 #CJK UNIFIED IDEOGRAPH
    {0xF27A, 0x77CA}, //16612 #CJK UNIFIED IDEOGRAPH
    {0xF27B, 0x77C4}, //16613 #CJK UNIFIED IDEOGRAPH
    {0xF27C, 0x77F1}, //16614 #CJK UNIFIED IDEOGRAPH
    {0xF27D, 0x791D}, //16615 #CJK UNIFIED IDEOGRAPH
    {0xF27E, 0x791B}, //16616 #CJK UNIFIED IDEOGRAPH
    {0xF2A1, 0x7921}, //16617 #CJK UNIFIED IDEOGRAPH
    {0xF2A2, 0x791C}, //16618 #CJK UNIFIED IDEOGRAPH
    {0xF2A3, 0x7917}, //16619 #CJK UNIFIED IDEOGRAPH
    {0xF2A4, 0x791E}, //16620 #CJK UNIFIED IDEOGRAPH
    {0xF2A5, 0x79B0}, //16621 #CJK UNIFIED IDEOGRAPH
    {0xF2A6, 0x7A67}, //16622 #CJK UNIFIED IDEOGRAPH
    {0xF2A7, 0x7A68}, //16623 #CJK UNIFIED IDEOGRAPH
    {0xF2A8, 0x7C33}, //16624 #CJK UNIFIED IDEOGRAPH
    {0xF2A9, 0x7C3C}, //16625 #CJK UNIFIED IDEOGRAPH
    {0xF2AA, 0x7C39}, //16626 #CJK UNIFIED IDEOGRAPH
    {0xF2AB, 0x7C2C}, //16627 #CJK UNIFIED IDEOGRAPH
    {0xF2AC, 0x7C3B}, //16628 #CJK UNIFIED IDEOGRAPH
    {0xF2AD, 0x7CEC}, //16629 #CJK UNIFIED IDEOGRAPH
    {0xF2AE, 0x7CEA}, //16630 #CJK UNIFIED IDEOGRAPH
    {0xF2AF, 0x7E76}, //16631 #CJK UNIFIED IDEOGRAPH
    {0xF2B0, 0x7E75}, //16632 #CJK UNIFIED IDEOGRAPH
    {0xF2B1, 0x7E78}, //16633 #CJK UNIFIED IDEOGRAPH
    {0xF2B2, 0x7E70}, //16634 #CJK UNIFIED IDEOGRAPH
    {0xF2B3, 0x7E77}, //16635 #CJK UNIFIED IDEOGRAPH
    {0xF2B4, 0x7E6F}, //16636 #CJK UNIFIED IDEOGRAPH
    {0xF2B5, 0x7E7A}, //16637 #CJK UNIFIED IDEOGRAPH
    {0xF2B6, 0x7E72}, //16638 #CJK UNIFIED IDEOGRAPH
    {0xF2B7, 0x7E74}, //16639 #CJK UNIFIED IDEOGRAPH
    {0xF2B8, 0x7E68}, //16640 #CJK UNIFIED IDEOGRAPH
    {0xF2B9, 0x7F4B}, //16641 #CJK UNIFIED IDEOGRAPH
    {0xF2BA, 0x7F4A}, //16642 #CJK UNIFIED IDEOGRAPH
    {0xF2BB, 0x7F83}, //16643 #CJK UNIFIED IDEOGRAPH
    {0xF2BC, 0x7F86}, //16644 #CJK UNIFIED IDEOGRAPH
    {0xF2BD, 0x7FB7}, //16645 #CJK UNIFIED IDEOGRAPH
    {0xF2BE, 0x7FFD}, //16646 #CJK UNIFIED IDEOGRAPH
    {0xF2BF, 0x7FFE}, //16647 #CJK UNIFIED IDEOGRAPH
    {0xF2C0, 0x8078}, //16648 #CJK UNIFIED IDEOGRAPH
    {0xF2C1, 0x81D7}, //16649 #CJK UNIFIED IDEOGRAPH
    {0xF2C2, 0x81D5}, //16650 #CJK UNIFIED IDEOGRAPH
    {0xF2C3, 0x8264}, //16651 #CJK UNIFIED IDEOGRAPH
    {0xF2C4, 0x8261}, //16652 #CJK UNIFIED IDEOGRAPH
    {0xF2C5, 0x8263}, //16653 #CJK UNIFIED IDEOGRAPH
    {0xF2C6, 0x85EB}, //16654 #CJK UNIFIED IDEOGRAPH
    {0xF2C7, 0x85F1}, //16655 #CJK UNIFIED IDEOGRAPH
    {0xF2C8, 0x85ED}, //16656 #CJK UNIFIED IDEOGRAPH
    {0xF2C9, 0x85D9}, //16657 #CJK UNIFIED IDEOGRAPH
    {0xF2CA, 0x85E1}, //16658 #CJK UNIFIED IDEOGRAPH
    {0xF2CB, 0x85E8}, //16659 #CJK UNIFIED IDEOGRAPH
    {0xF2CC, 0x85DA}, //16660 #CJK UNIFIED IDEOGRAPH
    {0xF2CD, 0x85D7}, //16661 #CJK UNIFIED IDEOGRAPH
    {0xF2CE, 0x85EC}, //16662 #CJK UNIFIED IDEOGRAPH
    {0xF2CF, 0x85F2}, //16663 #CJK UNIFIED IDEOGRAPH
    {0xF2D0, 0x85F8}, //16664 #CJK UNIFIED IDEOGRAPH
    {0xF2D1, 0x85D8}, //16665 #CJK UNIFIED IDEOGRAPH
    {0xF2D2, 0x85DF}, //16666 #CJK UNIFIED IDEOGRAPH
    {0xF2D3, 0x85E3}, //16667 #CJK UNIFIED IDEOGRAPH
    {0xF2D4, 0x85DC}, //16668 #CJK UNIFIED IDEOGRAPH
    {0xF2D5, 0x85D1}, //16669 #CJK UNIFIED IDEOGRAPH
    {0xF2D6, 0x85F0}, //16670 #CJK UNIFIED IDEOGRAPH
    {0xF2D7, 0x85E6}, //16671 #CJK UNIFIED IDEOGRAPH
    {0xF2D8, 0x85EF}, //16672 #CJK UNIFIED IDEOGRAPH
    {0xF2D9, 0x85DE}, //16673 #CJK UNIFIED IDEOGRAPH
    {0xF2DA, 0x85E2}, //16674 #CJK UNIFIED IDEOGRAPH
    {0xF2DB, 0x8800}, //16675 #CJK UNIFIED IDEOGRAPH
    {0xF2DC, 0x87FA}, //16676 #CJK UNIFIED IDEOGRAPH
    {0xF2DD, 0x8803}, //16677 #CJK UNIFIED IDEOGRAPH
    {0xF2DE, 0x87F6}, //16678 #CJK UNIFIED IDEOGRAPH
    {0xF2DF, 0x87F7}, //16679 #CJK UNIFIED IDEOGRAPH
    {0xF2E0, 0x8809}, //16680 #CJK UNIFIED IDEOGRAPH
    {0xF2E1, 0x880C}, //16681 #CJK UNIFIED IDEOGRAPH
    {0xF2E2, 0x880B}, //16682 #CJK UNIFIED IDEOGRAPH
    {0xF2E3, 0x8806}, //16683 #CJK UNIFIED IDEOGRAPH
    {0xF2E4, 0x87FC}, //16684 #CJK UNIFIED IDEOGRAPH
    {0xF2E5, 0x8808}, //16685 #CJK UNIFIED IDEOGRAPH
    {0xF2E6, 0x87FF}, //16686 #CJK UNIFIED IDEOGRAPH
    {0xF2E7, 0x880A}, //16687 #CJK UNIFIED IDEOGRAPH
    {0xF2E8, 0x8802}, //16688 #CJK UNIFIED IDEOGRAPH
    {0xF2E9, 0x8962}, //16689 #CJK UNIFIED IDEOGRAPH
    {0xF2EA, 0x895A}, //16690 #CJK UNIFIED IDEOGRAPH
    {0xF2EB, 0x895B}, //16691 #CJK UNIFIED IDEOGRAPH
    {0xF2EC, 0x8957}, //16692 #CJK UNIFIED IDEOGRAPH
    {0xF2ED, 0x8961}, //16693 #CJK UNIFIED IDEOGRAPH
    {0xF2EE, 0x895C}, //16694 #CJK UNIFIED IDEOGRAPH
    {0xF2EF, 0x8958}, //16695 #CJK UNIFIED IDEOGRAPH
    {0xF2F0, 0x895D}, //16696 #CJK UNIFIED IDEOGRAPH
    {0xF2F1, 0x8959}, //16697 #CJK UNIFIED IDEOGRAPH
    {0xF2F2, 0x8988}, //16698 #CJK UNIFIED IDEOGRAPH
    {0xF2F3, 0x89B7}, //16699 #CJK UNIFIED IDEOGRAPH
    {0xF2F4, 0x89B6}, //16700 #CJK UNIFIED IDEOGRAPH
    {0xF2F5, 0x89F6}, //16701 #CJK UNIFIED IDEOGRAPH
    {0xF2F6, 0x8B50}, //16702 #CJK UNIFIED IDEOGRAPH
    {0xF2F7, 0x8B48}, //16703 #CJK UNIFIED IDEOGRAPH
    {0xF2F8, 0x8B4A}, //16704 #CJK UNIFIED IDEOGRAPH
    {0xF2F9, 0x8B40}, //16705 #CJK UNIFIED IDEOGRAPH
    {0xF2FA, 0x8B53}, //16706 #CJK UNIFIED IDEOGRAPH
    {0xF2FB, 0x8B56}, //16707 #CJK UNIFIED IDEOGRAPH
    {0xF2FC, 0x8B54}, //16708 #CJK UNIFIED IDEOGRAPH
    {0xF2FD, 0x8B4B}, //16709 #CJK UNIFIED IDEOGRAPH
    {0xF2FE, 0x8B55}, //16710 #CJK UNIFIED IDEOGRAPH
    {0xF340, 0x8B51}, //16711 #CJK UNIFIED IDEOGRAPH
    {0xF341, 0x8B42}, //16712 #CJK UNIFIED IDEOGRAPH
    {0xF342, 0x8B52}, //16713 #CJK UNIFIED IDEOGRAPH
    {0xF343, 0x8B57}, //16714 #CJK UNIFIED IDEOGRAPH
    {0xF344, 0x8C43}, //16715 #CJK UNIFIED IDEOGRAPH
    {0xF345, 0x8C77}, //16716 #CJK UNIFIED IDEOGRAPH
    {0xF346, 0x8C76}, //16717 #CJK UNIFIED IDEOGRAPH
    {0xF347, 0x8C9A}, //16718 #CJK UNIFIED IDEOGRAPH
    {0xF348, 0x8D06}, //16719 #CJK UNIFIED IDEOGRAPH
    {0xF349, 0x8D07}, //16720 #CJK UNIFIED IDEOGRAPH
    {0xF34A, 0x8D09}, //16721 #CJK UNIFIED IDEOGRAPH
    {0xF34B, 0x8DAC}, //16722 #CJK UNIFIED IDEOGRAPH
    {0xF34C, 0x8DAA}, //16723 #CJK UNIFIED IDEOGRAPH
    {0xF34D, 0x8DAD}, //16724 #CJK UNIFIED IDEOGRAPH
    {0xF34E, 0x8DAB}, //16725 #CJK UNIFIED IDEOGRAPH
    {0xF34F, 0x8E6D}, //16726 #CJK UNIFIED IDEOGRAPH
    {0xF350, 0x8E78}, //16727 #CJK UNIFIED IDEOGRAPH
    {0xF351, 0x8E73}, //16728 #CJK UNIFIED IDEOGRAPH
    {0xF352, 0x8E6A}, //16729 #CJK UNIFIED IDEOGRAPH
    {0xF353, 0x8E6F}, //16730 #CJK UNIFIED IDEOGRAPH
    {0xF354, 0x8E7B}, //16731 #CJK UNIFIED IDEOGRAPH
    {0xF355, 0x8EC2}, //16732 #CJK UNIFIED IDEOGRAPH
    {0xF356, 0x8F52}, //16733 #CJK UNIFIED IDEOGRAPH
    {0xF357, 0x8F51}, //16734 #CJK UNIFIED IDEOGRAPH
    {0xF358, 0x8F4F}, //16735 #CJK UNIFIED IDEOGRAPH
    {0xF359, 0x8F50}, //16736 #CJK UNIFIED IDEOGRAPH
    {0xF35A, 0x8F53}, //16737 #CJK UNIFIED IDEOGRAPH
    {0xF35B, 0x8FB4}, //16738 #CJK UNIFIED IDEOGRAPH
    {0xF35C, 0x9140}, //16739 #CJK UNIFIED IDEOGRAPH
    {0xF35D, 0x913F}, //16740 #CJK UNIFIED IDEOGRAPH
    {0xF35E, 0x91B0}, //16741 #CJK UNIFIED IDEOGRAPH
    {0xF35F, 0x91AD}, //16742 #CJK UNIFIED IDEOGRAPH
    {0xF360, 0x93DE}, //16743 #CJK UNIFIED IDEOGRAPH
    {0xF361, 0x93C7}, //16744 #CJK UNIFIED IDEOGRAPH
    {0xF362, 0x93CF}, //16745 #CJK UNIFIED IDEOGRAPH
    {0xF363, 0x93C2}, //16746 #CJK UNIFIED IDEOGRAPH
    {0xF364, 0x93DA}, //16747 #CJK UNIFIED IDEOGRAPH
    {0xF365, 0x93D0}, //16748 #CJK UNIFIED IDEOGRAPH
    {0xF366, 0x93F9}, //16749 #CJK UNIFIED IDEOGRAPH
    {0xF367, 0x93EC}, //16750 #CJK UNIFIED IDEOGRAPH
    {0xF368, 0x93CC}, //16751 #CJK UNIFIED IDEOGRAPH
    {0xF369, 0x93D9}, //16752 #CJK UNIFIED IDEOGRAPH
    {0xF36A, 0x93A9}, //16753 #CJK UNIFIED IDEOGRAPH
    {0xF36B, 0x93E6}, //16754 #CJK UNIFIED IDEOGRAPH
    {0xF36C, 0x93CA}, //16755 #CJK UNIFIED IDEOGRAPH
    {0xF36D, 0x93D4}, //16756 #CJK UNIFIED IDEOGRAPH
    {0xF36E, 0x93EE}, //16757 #CJK UNIFIED IDEOGRAPH
    {0xF36F, 0x93E3}, //16758 #CJK UNIFIED IDEOGRAPH
    {0xF370, 0x93D5}, //16759 #CJK UNIFIED IDEOGRAPH
    {0xF371, 0x93C4}, //16760 #CJK UNIFIED IDEOGRAPH
    {0xF372, 0x93CE}, //16761 #CJK UNIFIED IDEOGRAPH
    {0xF373, 0x93C0}, //16762 #CJK UNIFIED IDEOGRAPH
    {0xF374, 0x93D2}, //16763 #CJK UNIFIED IDEOGRAPH
    {0xF375, 0x93E7}, //16764 #CJK UNIFIED IDEOGRAPH
    {0xF376, 0x957D}, //16765 #CJK UNIFIED IDEOGRAPH
    {0xF377, 0x95DA}, //16766 #CJK UNIFIED IDEOGRAPH
    {0xF378, 0x95DB}, //16767 #CJK UNIFIED IDEOGRAPH
    {0xF379, 0x96E1}, //16768 #CJK UNIFIED IDEOGRAPH
    {0xF37A, 0x9729}, //16769 #CJK UNIFIED IDEOGRAPH
    {0xF37B, 0x972B}, //16770 #CJK UNIFIED IDEOGRAPH
    {0xF37C, 0x972C}, //16771 #CJK UNIFIED IDEOGRAPH
    {0xF37D, 0x9728}, //16772 #CJK UNIFIED IDEOGRAPH
    {0xF37E, 0x9726}, //16773 #CJK UNIFIED IDEOGRAPH
    {0xF3A1, 0x97B3}, //16774 #CJK UNIFIED IDEOGRAPH
    {0xF3A2, 0x97B7}, //16775 #CJK UNIFIED IDEOGRAPH
    {0xF3A3, 0x97B6}, //16776 #CJK UNIFIED IDEOGRAPH
    {0xF3A4, 0x97DD}, //16777 #CJK UNIFIED IDEOGRAPH
    {0xF3A5, 0x97DE}, //16778 #CJK UNIFIED IDEOGRAPH
    {0xF3A6, 0x97DF}, //16779 #CJK UNIFIED IDEOGRAPH
    {0xF3A7, 0x985C}, //16780 #CJK UNIFIED IDEOGRAPH
    {0xF3A8, 0x9859}, //16781 #CJK UNIFIED IDEOGRAPH
    {0xF3A9, 0x985D}, //16782 #CJK UNIFIED IDEOGRAPH
    {0xF3AA, 0x9857}, //16783 #CJK UNIFIED IDEOGRAPH
    {0xF3AB, 0x98BF}, //16784 #CJK UNIFIED IDEOGRAPH
    {0xF3AC, 0x98BD}, //16785 #CJK UNIFIED IDEOGRAPH
    {0xF3AD, 0x98BB}, //16786 #CJK UNIFIED IDEOGRAPH
    {0xF3AE, 0x98BE}, //16787 #CJK UNIFIED IDEOGRAPH
    {0xF3AF, 0x9948}, //16788 #CJK UNIFIED IDEOGRAPH
    {0xF3B0, 0x9947}, //16789 #CJK UNIFIED IDEOGRAPH
    {0xF3B1, 0x9943}, //16790 #CJK UNIFIED IDEOGRAPH
    {0xF3B2, 0x99A6}, //16791 #CJK UNIFIED IDEOGRAPH
    {0xF3B3, 0x99A7}, //16792 #CJK UNIFIED IDEOGRAPH
    {0xF3B4, 0x9A1A}, //16793 #CJK UNIFIED IDEOGRAPH
    {0xF3B5, 0x9A15}, //16794 #CJK UNIFIED IDEOGRAPH
    {0xF3B6, 0x9A25}, //16795 #CJK UNIFIED IDEOGRAPH
    {0xF3B7, 0x9A1D}, //16796 #CJK UNIFIED IDEOGRAPH
    {0xF3B8, 0x9A24}, //16797 #CJK UNIFIED IDEOGRAPH
    {0xF3B9, 0x9A1B}, //16798 #CJK UNIFIED IDEOGRAPH
    {0xF3BA, 0x9A22}, //16799 #CJK UNIFIED IDEOGRAPH
    {0xF3BB, 0x9A20}, //16800 #CJK UNIFIED IDEOGRAPH
    {0xF3BC, 0x9A27}, //16801 #CJK UNIFIED IDEOGRAPH
    {0xF3BD, 0x9A23}, //16802 #CJK UNIFIED IDEOGRAPH
    {0xF3BE, 0x9A1E}, //16803 #CJK UNIFIED IDEOGRAPH
    {0xF3BF, 0x9A1C}, //16804 #CJK UNIFIED IDEOGRAPH
    {0xF3C0, 0x9A14}, //16805 #CJK UNIFIED IDEOGRAPH
    {0xF3C1, 0x9AC2}, //16806 #CJK UNIFIED IDEOGRAPH
    {0xF3C2, 0x9B0B}, //16807 #CJK UNIFIED IDEOGRAPH
    {0xF3C3, 0x9B0A}, //16808 #CJK UNIFIED IDEOGRAPH
    {0xF3C4, 0x9B0E}, //16809 #CJK UNIFIED IDEOGRAPH
    {0xF3C5, 0x9B0C}, //16810 #CJK UNIFIED IDEOGRAPH
    {0xF3C6, 0x9B37}, //16811 #CJK UNIFIED IDEOGRAPH
    {0xF3C7, 0x9BEA}, //16812 #CJK UNIFIED IDEOGRAPH
    {0xF3C8, 0x9BEB}, //16813 #CJK UNIFIED IDEOGRAPH
    {0xF3C9, 0x9BE0}, //16814 #CJK UNIFIED IDEOGRAPH
    {0xF3CA, 0x9BDE}, //16815 #CJK UNIFIED IDEOGRAPH
    {0xF3CB, 0x9BE4}, //16816 #CJK UNIFIED IDEOGRAPH
    {0xF3CC, 0x9BE6}, //16817 #CJK UNIFIED IDEOGRAPH
    {0xF3CD, 0x9BE2}, //16818 #CJK UNIFIED IDEOGRAPH
    {0xF3CE, 0x9BF0}, //16819 #CJK UNIFIED IDEOGRAPH
    {0xF3CF, 0x9BD4}, //16820 #CJK UNIFIED IDEOGRAPH
    {0xF3D0, 0x9BD7}, //16821 #CJK UNIFIED IDEOGRAPH
    {0xF3D1, 0x9BEC}, //16822 #CJK UNIFIED IDEOGRAPH
    {0xF3D2, 0x9BDC}, //16823 #CJK UNIFIED IDEOGRAPH
    {0xF3D3, 0x9BD9}, //16824 #CJK UNIFIED IDEOGRAPH
    {0xF3D4, 0x9BE5}, //16825 #CJK UNIFIED IDEOGRAPH
    {0xF3D5, 0x9BD5}, //16826 #CJK UNIFIED IDEOGRAPH
    {0xF3D6, 0x9BE1}, //16827 #CJK UNIFIED IDEOGRAPH
    {0xF3D7, 0x9BDA}, //16828 #CJK UNIFIED IDEOGRAPH
    {0xF3D8, 0x9D77}, //16829 #CJK UNIFIED IDEOGRAPH
    {0xF3D9, 0x9D81}, //16830 #CJK UNIFIED IDEOGRAPH
    {0xF3DA, 0x9D8A}, //16831 #CJK UNIFIED IDEOGRAPH
    {0xF3DB, 0x9D84}, //16832 #CJK UNIFIED IDEOGRAPH
    {0xF3DC, 0x9D88}, //16833 #CJK UNIFIED IDEOGRAPH
    {0xF3DD, 0x9D71}, //16834 #CJK UNIFIED IDEOGRAPH
    {0xF3DE, 0x9D80}, //16835 #CJK UNIFIED IDEOGRAPH
    {0xF3DF, 0x9D78}, //16836 #CJK UNIFIED IDEOGRAPH
    {0xF3E0, 0x9D86}, //16837 #CJK UNIFIED IDEOGRAPH
    {0xF3E1, 0x9D8B}, //16838 #CJK UNIFIED IDEOGRAPH
    {0xF3E2, 0x9D8C}, //16839 #CJK UNIFIED IDEOGRAPH
    {0xF3E3, 0x9D7D}, //16840 #CJK UNIFIED IDEOGRAPH
    {0xF3E4, 0x9D6B}, //16841 #CJK UNIFIED IDEOGRAPH
    {0xF3E5, 0x9D74}, //16842 #CJK UNIFIED IDEOGRAPH
    {0xF3E6, 0x9D75}, //16843 #CJK UNIFIED IDEOGRAPH
    {0xF3E7, 0x9D70}, //16844 #CJK UNIFIED IDEOGRAPH
    {0xF3E8, 0x9D69}, //16845 #CJK UNIFIED IDEOGRAPH
    {0xF3E9, 0x9D85}, //16846 #CJK UNIFIED IDEOGRAPH
    {0xF3EA, 0x9D73}, //16847 #CJK UNIFIED IDEOGRAPH
    {0xF3EB, 0x9D7B}, //16848 #CJK UNIFIED IDEOGRAPH
    {0xF3EC, 0x9D82}, //16849 #CJK UNIFIED IDEOGRAPH
    {0xF3ED, 0x9D6F}, //16850 #CJK UNIFIED IDEOGRAPH
    {0xF3EE, 0x9D79}, //16851 #CJK UNIFIED IDEOGRAPH
    {0xF3EF, 0x9D7F}, //16852 #CJK UNIFIED IDEOGRAPH
    {0xF3F0, 0x9D87}, //16853 #CJK UNIFIED IDEOGRAPH
    {0xF3F1, 0x9D68}, //16854 #CJK UNIFIED IDEOGRAPH
    {0xF3F2, 0x9E94}, //16855 #CJK UNIFIED IDEOGRAPH
    {0xF3F3, 0x9E91}, //16856 #CJK UNIFIED IDEOGRAPH
    {0xF3F4, 0x9EC0}, //16857 #CJK UNIFIED IDEOGRAPH
    {0xF3F5, 0x9EFC}, //16858 #CJK UNIFIED IDEOGRAPH
    {0xF3F6, 0x9F2D}, //16859 #CJK UNIFIED IDEOGRAPH
    {0xF3F7, 0x9F40}, //16860 #CJK UNIFIED IDEOGRAPH
    {0xF3F8, 0x9F41}, //16861 #CJK UNIFIED IDEOGRAPH
    {0xF3F9, 0x9F4D}, //16862 #CJK UNIFIED IDEOGRAPH
    {0xF3FA, 0x9F56}, //16863 #CJK UNIFIED IDEOGRAPH
    {0xF3FB, 0x9F57}, //16864 #CJK UNIFIED IDEOGRAPH
    {0xF3FC, 0x9F58}, //16865 #CJK UNIFIED IDEOGRAPH
    {0xF3FD, 0x5337}, //16866 #CJK UNIFIED IDEOGRAPH
    {0xF3FE, 0x56B2}, //16867 #CJK UNIFIED IDEOGRAPH
    {0xF440, 0x56B5}, //16868 #CJK UNIFIED IDEOGRAPH
    {0xF441, 0x56B3}, //16869 #CJK UNIFIED IDEOGRAPH
    {0xF442, 0x58E3}, //16870 #CJK UNIFIED IDEOGRAPH
    {0xF443, 0x5B45}, //16871 #CJK UNIFIED IDEOGRAPH
    {0xF444, 0x5DC6}, //16872 #CJK UNIFIED IDEOGRAPH
    {0xF445, 0x5DC7}, //16873 #CJK UNIFIED IDEOGRAPH
    {0xF446, 0x5EEE}, //16874 #CJK UNIFIED IDEOGRAPH
    {0xF447, 0x5EEF}, //16875 #CJK UNIFIED IDEOGRAPH
    {0xF448, 0x5FC0}, //16876 #CJK UNIFIED IDEOGRAPH
    {0xF449, 0x5FC1}, //16877 #CJK UNIFIED IDEOGRAPH
    {0xF44A, 0x61F9}, //16878 #CJK UNIFIED IDEOGRAPH
    {0xF44B, 0x6517}, //16879 #CJK UNIFIED IDEOGRAPH
    {0xF44C, 0x6516}, //16880 #CJK UNIFIED IDEOGRAPH
    {0xF44D, 0x6515}, //16881 #CJK UNIFIED IDEOGRAPH
    {0xF44E, 0x6513}, //16882 #CJK UNIFIED IDEOGRAPH
    {0xF44F, 0x65DF}, //16883 #CJK UNIFIED IDEOGRAPH
    {0xF450, 0x66E8}, //16884 #CJK UNIFIED IDEOGRAPH
    {0xF451, 0x66E3}, //16885 #CJK UNIFIED IDEOGRAPH
    {0xF452, 0x66E4}, //16886 #CJK UNIFIED IDEOGRAPH
    {0xF453, 0x6AF3}, //16887 #CJK UNIFIED IDEOGRAPH
    {0xF454, 0x6AF0}, //16888 #CJK UNIFIED IDEOGRAPH
    {0xF455, 0x6AEA}, //16889 #CJK UNIFIED IDEOGRAPH
    {0xF456, 0x6AE8}, //16890 #CJK UNIFIED IDEOGRAPH
    {0xF457, 0x6AF9}, //16891 #CJK UNIFIED IDEOGRAPH
    {0xF458, 0x6AF1}, //16892 #CJK UNIFIED IDEOGRAPH
    {0xF459, 0x6AEE}, //16893 #CJK UNIFIED IDEOGRAPH
    {0xF45A, 0x6AEF}, //16894 #CJK UNIFIED IDEOGRAPH
    {0xF45B, 0x703C}, //16895 #CJK UNIFIED IDEOGRAPH
    {0xF45C, 0x7035}, //16896 #CJK UNIFIED IDEOGRAPH
    {0xF45D, 0x702F}, //16897 #CJK UNIFIED IDEOGRAPH
    {0xF45E, 0x7037}, //16898 #CJK UNIFIED IDEOGRAPH
    {0xF45F, 0x7034}, //16899 #CJK UNIFIED IDEOGRAPH
    {0xF460, 0x7031}, //16900 #CJK UNIFIED IDEOGRAPH
    {0xF461, 0x7042}, //16901 #CJK UNIFIED IDEOGRAPH
    {0xF462, 0x7038}, //16902 #CJK UNIFIED IDEOGRAPH
    {0xF463, 0x703F}, //16903 #CJK UNIFIED IDEOGRAPH
    {0xF464, 0x703A}, //16904 #CJK UNIFIED IDEOGRAPH
    {0xF465, 0x7039}, //16905 #CJK UNIFIED IDEOGRAPH
    {0xF466, 0x7040}, //16906 #CJK UNIFIED IDEOGRAPH
    {0xF467, 0x703B}, //16907 #CJK UNIFIED IDEOGRAPH
    {0xF468, 0x7033}, //16908 #CJK UNIFIED IDEOGRAPH
    {0xF469, 0x7041}, //16909 #CJK UNIFIED IDEOGRAPH
    {0xF46A, 0x7213}, //16910 #CJK UNIFIED IDEOGRAPH
    {0xF46B, 0x7214}, //16911 #CJK UNIFIED IDEOGRAPH
    {0xF46C, 0x72A8}, //16912 #CJK UNIFIED IDEOGRAPH
    {0xF46D, 0x737D}, //16913 #CJK UNIFIED IDEOGRAPH
    {0xF46E, 0x737C}, //16914 #CJK UNIFIED IDEOGRAPH
    {0xF46F, 0x74BA}, //16915 #CJK UNIFIED IDEOGRAPH
    {0xF470, 0x76AB}, //16916 #CJK UNIFIED IDEOGRAPH
    {0xF471, 0x76AA}, //16917 #CJK UNIFIED IDEOGRAPH
    {0xF472, 0x76BE}, //16918 #CJK UNIFIED IDEOGRAPH
    {0xF473, 0x76ED}, //16919 #CJK UNIFIED IDEOGRAPH
    {0xF474, 0x77CC}, //16920 #CJK UNIFIED IDEOGRAPH
    {0xF475, 0x77CE}, //16921 #CJK UNIFIED IDEOGRAPH
    {0xF476, 0x77CF}, //16922 #CJK UNIFIED IDEOGRAPH
    {0xF477, 0x77CD}, //16923 #CJK UNIFIED IDEOGRAPH
    {0xF478, 0x77F2}, //16924 #CJK UNIFIED IDEOGRAPH
    {0xF479, 0x7925}, //16925 #CJK UNIFIED IDEOGRAPH
    {0xF47A, 0x7923}, //16926 #CJK UNIFIED IDEOGRAPH
    {0xF47B, 0x7927}, //16927 #CJK UNIFIED IDEOGRAPH
    {0xF47C, 0x7928}, //16928 #CJK UNIFIED IDEOGRAPH
    {0xF47D, 0x7924}, //16929 #CJK UNIFIED IDEOGRAPH
    {0xF47E, 0x7929}, //16930 #CJK UNIFIED IDEOGRAPH
    {0xF4A1, 0x79B2}, //16931 #CJK UNIFIED IDEOGRAPH
    {0xF4A2, 0x7A6E}, //16932 #CJK UNIFIED IDEOGRAPH
    {0xF4A3, 0x7A6C}, //16933 #CJK UNIFIED IDEOGRAPH
    {0xF4A4, 0x7A6D}, //16934 #CJK UNIFIED IDEOGRAPH
    {0xF4A5, 0x7AF7}, //16935 #CJK UNIFIED IDEOGRAPH
    {0xF4A6, 0x7C49}, //16936 #CJK UNIFIED IDEOGRAPH
    {0xF4A7, 0x7C48}, //16937 #CJK UNIFIED IDEOGRAPH
    {0xF4A8, 0x7C4A}, //16938 #CJK UNIFIED IDEOGRAPH
    {0xF4A9, 0x7C47}, //16939 #CJK UNIFIED IDEOGRAPH
    {0xF4AA, 0x7C45}, //16940 #CJK UNIFIED IDEOGRAPH
    {0xF4AB, 0x7CEE}, //16941 #CJK UNIFIED IDEOGRAPH
    {0xF4AC, 0x7E7B}, //16942 #CJK UNIFIED IDEOGRAPH
    {0xF4AD, 0x7E7E}, //16943 #CJK UNIFIED IDEOGRAPH
    {0xF4AE, 0x7E81}, //16944 #CJK UNIFIED IDEOGRAPH
    {0xF4AF, 0x7E80}, //16945 #CJK UNIFIED IDEOGRAPH
    {0xF4B0, 0x7FBA}, //16946 #CJK UNIFIED IDEOGRAPH
    {0xF4B1, 0x7FFF}, //16947 #CJK UNIFIED IDEOGRAPH
    {0xF4B2, 0x8079}, //16948 #CJK UNIFIED IDEOGRAPH
    {0xF4B3, 0x81DB}, //16949 #CJK UNIFIED IDEOGRAPH
    {0xF4B4, 0x81D9}, //16950 #CJK UNIFIED IDEOGRAPH
    {0xF4B5, 0x820B}, //16951 #CJK UNIFIED IDEOGRAPH
    {0xF4B6, 0x8268}, //16952 #CJK UNIFIED IDEOGRAPH
    {0xF4B7, 0x8269}, //16953 #CJK UNIFIED IDEOGRAPH
    {0xF4B8, 0x8622}, //16954 #CJK UNIFIED IDEOGRAPH
    {0xF4B9, 0x85FF}, //16955 #CJK UNIFIED IDEOGRAPH
    {0xF4BA, 0x8601}, //16956 #CJK UNIFIED IDEOGRAPH
    {0xF4BB, 0x85FE}, //16957 #CJK UNIFIED IDEOGRAPH
    {0xF4BC, 0x861B}, //16958 #CJK UNIFIED IDEOGRAPH
    {0xF4BD, 0x8600}, //16959 #CJK UNIFIED IDEOGRAPH
    {0xF4BE, 0x85F6}, //16960 #CJK UNIFIED IDEOGRAPH
    {0xF4BF, 0x8604}, //16961 #CJK UNIFIED IDEOGRAPH
    {0xF4C0, 0x8609}, //16962 #CJK UNIFIED IDEOGRAPH
    {0xF4C1, 0x8605}, //16963 #CJK UNIFIED IDEOGRAPH
    {0xF4C2, 0x860C}, //16964 #CJK UNIFIED IDEOGRAPH
    {0xF4C3, 0x85FD}, //16965 #CJK UNIFIED IDEOGRAPH
    {0xF4C4, 0x8819}, //16966 #CJK UNIFIED IDEOGRAPH
    {0xF4C5, 0x8810}, //16967 #CJK UNIFIED IDEOGRAPH
    {0xF4C6, 0x8811}, //16968 #CJK UNIFIED IDEOGRAPH
    {0xF4C7, 0x8817}, //16969 #CJK UNIFIED IDEOGRAPH
    {0xF4C8, 0x8813}, //16970 #CJK UNIFIED IDEOGRAPH
    {0xF4C9, 0x8816}, //16971 #CJK UNIFIED IDEOGRAPH
    {0xF4CA, 0x8963}, //16972 #CJK UNIFIED IDEOGRAPH
    {0xF4CB, 0x8966}, //16973 #CJK UNIFIED IDEOGRAPH
    {0xF4CC, 0x89B9}, //16974 #CJK UNIFIED IDEOGRAPH
    {0xF4CD, 0x89F7}, //16975 #CJK UNIFIED IDEOGRAPH
    {0xF4CE, 0x8B60}, //16976 #CJK UNIFIED IDEOGRAPH
    {0xF4CF, 0x8B6A}, //16977 #CJK UNIFIED IDEOGRAPH
    {0xF4D0, 0x8B5D}, //16978 #CJK UNIFIED IDEOGRAPH
    {0xF4D1, 0x8B68}, //16979 #CJK UNIFIED IDEOGRAPH
    {0xF4D2, 0x8B63}, //16980 #CJK UNIFIED IDEOGRAPH
    {0xF4D3, 0x8B65}, //16981 #CJK UNIFIED IDEOGRAPH
    {0xF4D4, 0x8B67}, //16982 #CJK UNIFIED IDEOGRAPH
    {0xF4D5, 0x8B6D}, //16983 #CJK UNIFIED IDEOGRAPH
    {0xF4D6, 0x8DAE}, //16984 #CJK UNIFIED IDEOGRAPH
    {0xF4D7, 0x8E86}, //16985 #CJK UNIFIED IDEOGRAPH
    {0xF4D8, 0x8E88}, //16986 #CJK UNIFIED IDEOGRAPH
    {0xF4D9, 0x8E84}, //16987 #CJK UNIFIED IDEOGRAPH
    {0xF4DA, 0x8F59}, //16988 #CJK UNIFIED IDEOGRAPH
    {0xF4DB, 0x8F56}, //16989 #CJK UNIFIED IDEOGRAPH
    {0xF4DC, 0x8F57}, //16990 #CJK UNIFIED IDEOGRAPH
    {0xF4DD, 0x8F55}, //16991 #CJK UNIFIED IDEOGRAPH
    {0xF4DE, 0x8F58}, //16992 #CJK UNIFIED IDEOGRAPH
    {0xF4DF, 0x8F5A}, //16993 #CJK UNIFIED IDEOGRAPH
    {0xF4E0, 0x908D}, //16994 #CJK UNIFIED IDEOGRAPH
    {0xF4E1, 0x9143}, //16995 #CJK UNIFIED IDEOGRAPH
    {0xF4E2, 0x9141}, //16996 #CJK UNIFIED IDEOGRAPH
    {0xF4E3, 0x91B7}, //16997 #CJK UNIFIED IDEOGRAPH
    {0xF4E4, 0x91B5}, //16998 #CJK UNIFIED IDEOGRAPH
    {0xF4E5, 0x91B2}, //16999 #CJK UNIFIED IDEOGRAPH
    {0xF4E6, 0x91B3}, //17000 #CJK UNIFIED IDEOGRAPH
    {0xF4E7, 0x940B}, //17001 #CJK UNIFIED IDEOGRAPH
    {0xF4E8, 0x9413}, //17002 #CJK UNIFIED IDEOGRAPH
    {0xF4E9, 0x93FB}, //17003 #CJK UNIFIED IDEOGRAPH
    {0xF4EA, 0x9420}, //17004 #CJK UNIFIED IDEOGRAPH
    {0xF4EB, 0x940F}, //17005 #CJK UNIFIED IDEOGRAPH
    {0xF4EC, 0x9414}, //17006 #CJK UNIFIED IDEOGRAPH
    {0xF4ED, 0x93FE}, //17007 #CJK UNIFIED IDEOGRAPH
    {0xF4EE, 0x9415}, //17008 #CJK UNIFIED IDEOGRAPH
    {0xF4EF, 0x9410}, //17009 #CJK UNIFIED IDEOGRAPH
    {0xF4F0, 0x9428}, //17010 #CJK UNIFIED IDEOGRAPH
    {0xF4F1, 0x9419}, //17011 #CJK UNIFIED IDEOGRAPH
    {0xF4F2, 0x940D}, //17012 #CJK UNIFIED IDEOGRAPH
    {0xF4F3, 0x93F5}, //17013 #CJK UNIFIED IDEOGRAPH
    {0xF4F4, 0x9400}, //17014 #CJK UNIFIED IDEOGRAPH
    {0xF4F5, 0x93F7}, //17015 #CJK UNIFIED IDEOGRAPH
    {0xF4F6, 0x9407}, //17016 #CJK UNIFIED IDEOGRAPH
    {0xF4F7, 0x940E}, //17017 #CJK UNIFIED IDEOGRAPH
    {0xF4F8, 0x9416}, //17018 #CJK UNIFIED IDEOGRAPH
    {0xF4F9, 0x9412}, //17019 #CJK UNIFIED IDEOGRAPH
    {0xF4FA, 0x93FA}, //17020 #CJK UNIFIED IDEOGRAPH
    {0xF4FB, 0x9409}, //17021 #CJK UNIFIED IDEOGRAPH
    {0xF4FC, 0x93F8}, //17022 #CJK UNIFIED IDEOGRAPH
    {0xF4FD, 0x940A}, //17023 #CJK UNIFIED IDEOGRAPH
    {0xF4FE, 0x93FF}, //17024 #CJK UNIFIED IDEOGRAPH
    {0xF540, 0x93FC}, //17025 #CJK UNIFIED IDEOGRAPH
    {0xF541, 0x940C}, //17026 #CJK UNIFIED IDEOGRAPH
    {0xF542, 0x93F6}, //17027 #CJK UNIFIED IDEOGRAPH
    {0xF543, 0x9411}, //17028 #CJK UNIFIED IDEOGRAPH
    {0xF544, 0x9406}, //17029 #CJK UNIFIED IDEOGRAPH
    {0xF545, 0x95DE}, //17030 #CJK UNIFIED IDEOGRAPH
    {0xF546, 0x95E0}, //17031 #CJK UNIFIED IDEOGRAPH
    {0xF547, 0x95DF}, //17032 #CJK UNIFIED IDEOGRAPH
    {0xF548, 0x972E}, //17033 #CJK UNIFIED IDEOGRAPH
    {0xF549, 0x972F}, //17034 #CJK UNIFIED IDEOGRAPH
    {0xF54A, 0x97B9}, //17035 #CJK UNIFIED IDEOGRAPH
    {0xF54B, 0x97BB}, //17036 #CJK UNIFIED IDEOGRAPH
    {0xF54C, 0x97FD}, //17037 #CJK UNIFIED IDEOGRAPH
    {0xF54D, 0x97FE}, //17038 #CJK UNIFIED IDEOGRAPH
    {0xF54E, 0x9860}, //17039 #CJK UNIFIED IDEOGRAPH
    {0xF54F, 0x9862}, //17040 #CJK UNIFIED IDEOGRAPH
    {0xF550, 0x9863}, //17041 #CJK UNIFIED IDEOGRAPH
    {0xF551, 0x985F}, //17042 #CJK UNIFIED IDEOGRAPH
    {0xF552, 0x98C1}, //17043 #CJK UNIFIED IDEOGRAPH
    {0xF553, 0x98C2}, //17044 #CJK UNIFIED IDEOGRAPH
    {0xF554, 0x9950}, //17045 #CJK UNIFIED IDEOGRAPH
    {0xF555, 0x994E}, //17046 #CJK UNIFIED IDEOGRAPH
    {0xF556, 0x9959}, //17047 #CJK UNIFIED IDEOGRAPH
    {0xF557, 0x994C}, //17048 #CJK UNIFIED IDEOGRAPH
    {0xF558, 0x994B}, //17049 #CJK UNIFIED IDEOGRAPH
    {0xF559, 0x9953}, //17050 #CJK UNIFIED IDEOGRAPH
    {0xF55A, 0x9A32}, //17051 #CJK UNIFIED IDEOGRAPH
    {0xF55B, 0x9A34}, //17052 #CJK UNIFIED IDEOGRAPH
    {0xF55C, 0x9A31}, //17053 #CJK UNIFIED IDEOGRAPH
    {0xF55D, 0x9A2C}, //17054 #CJK UNIFIED IDEOGRAPH
    {0xF55E, 0x9A2A}, //17055 #CJK UNIFIED IDEOGRAPH
    {0xF55F, 0x9A36}, //17056 #CJK UNIFIED IDEOGRAPH
    {0xF560, 0x9A29}, //17057 #CJK UNIFIED IDEOGRAPH
    {0xF561, 0x9A2E}, //17058 #CJK UNIFIED IDEOGRAPH
    {0xF562, 0x9A38}, //17059 #CJK UNIFIED IDEOGRAPH
    {0xF563, 0x9A2D}, //17060 #CJK UNIFIED IDEOGRAPH
    {0xF564, 0x9AC7}, //17061 #CJK UNIFIED IDEOGRAPH
    {0xF565, 0x9ACA}, //17062 #CJK UNIFIED IDEOGRAPH
    {0xF566, 0x9AC6}, //17063 #CJK UNIFIED IDEOGRAPH
    {0xF567, 0x9B10}, //17064 #CJK UNIFIED IDEOGRAPH
    {0xF568, 0x9B12}, //17065 #CJK UNIFIED IDEOGRAPH
    {0xF569, 0x9B11}, //17066 #CJK UNIFIED IDEOGRAPH
    {0xF56A, 0x9C0B}, //17067 #CJK UNIFIED IDEOGRAPH
    {0xF56B, 0x9C08}, //17068 #CJK UNIFIED IDEOGRAPH
    {0xF56C, 0x9BF7}, //17069 #CJK UNIFIED IDEOGRAPH
    {0xF56D, 0x9C05}, //17070 #CJK UNIFIED IDEOGRAPH
    {0xF56E, 0x9C12}, //17071 #CJK UNIFIED IDEOGRAPH
    {0xF56F, 0x9BF8}, //17072 #CJK UNIFIED IDEOGRAPH
    {0xF570, 0x9C40}, //17073 #CJK UNIFIED IDEOGRAPH
    {0xF571, 0x9C07}, //17074 #CJK UNIFIED IDEOGRAPH
    {0xF572, 0x9C0E}, //17075 #CJK UNIFIED IDEOGRAPH
    {0xF573, 0x9C06}, //17076 #CJK UNIFIED IDEOGRAPH
    {0xF574, 0x9C17}, //17077 #CJK UNIFIED IDEOGRAPH
    {0xF575, 0x9C14}, //17078 #CJK UNIFIED IDEOGRAPH
    {0xF576, 0x9C09}, //17079 #CJK UNIFIED IDEOGRAPH
    {0xF577, 0x9D9F}, //17080 #CJK UNIFIED IDEOGRAPH
    {0xF578, 0x9D99}, //17081 #CJK UNIFIED IDEOGRAPH
    {0xF579, 0x9DA4}, //17082 #CJK UNIFIED IDEOGRAPH
    {0xF57A, 0x9D9D}, //17083 #CJK UNIFIED IDEOGRAPH
    {0xF57B, 0x9D92}, //17084 #CJK UNIFIED IDEOGRAPH
    {0xF57C, 0x9D98}, //17085 #CJK UNIFIED IDEOGRAPH
    {0xF57D, 0x9D90}, //17086 #CJK UNIFIED IDEOGRAPH
    {0xF57E, 0x9D9B}, //17087 #CJK UNIFIED IDEOGRAPH
    {0xF5A1, 0x9DA0}, //17088 #CJK UNIFIED IDEOGRAPH
    {0xF5A2, 0x9D94}, //17089 #CJK UNIFIED IDEOGRAPH
    {0xF5A3, 0x9D9C}, //17090 #CJK UNIFIED IDEOGRAPH
    {0xF5A4, 0x9DAA}, //17091 #CJK UNIFIED IDEOGRAPH
    {0xF5A5, 0x9D97}, //17092 #CJK UNIFIED IDEOGRAPH
    {0xF5A6, 0x9DA1}, //17093 #CJK UNIFIED IDEOGRAPH
    {0xF5A7, 0x9D9A}, //17094 #CJK UNIFIED IDEOGRAPH
    {0xF5A8, 0x9DA2}, //17095 #CJK UNIFIED IDEOGRAPH
    {0xF5A9, 0x9DA8}, //17096 #CJK UNIFIED IDEOGRAPH
    {0xF5AA, 0x9D9E}, //17097 #CJK UNIFIED IDEOGRAPH
    {0xF5AB, 0x9DA3}, //17098 #CJK UNIFIED IDEOGRAPH
    {0xF5AC, 0x9DBF}, //17099 #CJK UNIFIED IDEOGRAPH
    {0xF5AD, 0x9DA9}, //17100 #CJK UNIFIED IDEOGRAPH
    {0xF5AE, 0x9D96}, //17101 #CJK UNIFIED IDEOGRAPH
    {0xF5AF, 0x9DA6}, //17102 #CJK UNIFIED IDEOGRAPH
    {0xF5B0, 0x9DA7}, //17103 #CJK UNIFIED IDEOGRAPH
    {0xF5B1, 0x9E99}, //17104 #CJK UNIFIED IDEOGRAPH
    {0xF5B2, 0x9E9B}, //17105 #CJK UNIFIED IDEOGRAPH
    {0xF5B3, 0x9E9A}, //17106 #CJK UNIFIED IDEOGRAPH
    {0xF5B4, 0x9EE5}, //17107 #CJK UNIFIED IDEOGRAPH
    {0xF5B5, 0x9EE4}, //17108 #CJK UNIFIED IDEOGRAPH
    {0xF5B6, 0x9EE7}, //17109 #CJK UNIFIED IDEOGRAPH
    {0xF5B7, 0x9EE6}, //17110 #CJK UNIFIED IDEOGRAPH
    {0xF5B8, 0x9F30}, //17111 #CJK UNIFIED IDEOGRAPH
    {0xF5B9, 0x9F2E}, //17112 #CJK UNIFIED IDEOGRAPH
    {0xF5BA, 0x9F5B}, //17113 #CJK UNIFIED IDEOGRAPH
    {0xF5BB, 0x9F60}, //17114 #CJK UNIFIED IDEOGRAPH
    {0xF5BC, 0x9F5E}, //17115 #CJK UNIFIED IDEOGRAPH
    {0xF5BD, 0x9F5D}, //17116 #CJK UNIFIED IDEOGRAPH
    {0xF5BE, 0x9F59}, //17117 #CJK UNIFIED IDEOGRAPH
    {0xF5BF, 0x9F91}, //17118 #CJK UNIFIED IDEOGRAPH
    {0xF5C0, 0x513A}, //17119 #CJK UNIFIED IDEOGRAPH
    {0xF5C1, 0x5139}, //17120 #CJK UNIFIED IDEOGRAPH
    {0xF5C2, 0x5298}, //17121 #CJK UNIFIED IDEOGRAPH
    {0xF5C3, 0x5297}, //17122 #CJK UNIFIED IDEOGRAPH
    {0xF5C4, 0x56C3}, //17123 #CJK UNIFIED IDEOGRAPH
    {0xF5C5, 0x56BD}, //17124 #CJK UNIFIED IDEOGRAPH
    {0xF5C6, 0x56BE}, //17125 #CJK UNIFIED IDEOGRAPH
    {0xF5C7, 0x5B48}, //17126 #CJK UNIFIED IDEOGRAPH
    {0xF5C8, 0x5B47}, //17127 #CJK UNIFIED IDEOGRAPH
    {0xF5C9, 0x5DCB}, //17128 #CJK UNIFIED IDEOGRAPH
    {0xF5CA, 0x5DCF}, //17129 #CJK UNIFIED IDEOGRAPH
    {0xF5CB, 0x5EF1}, //17130 #CJK UNIFIED IDEOGRAPH
    {0xF5CC, 0x61FD}, //17131 #CJK UNIFIED IDEOGRAPH
    {0xF5CD, 0x651B}, //17132 #CJK UNIFIED IDEOGRAPH
    {0xF5CE, 0x6B02}, //17133 #CJK UNIFIED IDEOGRAPH
    {0xF5CF, 0x6AFC}, //17134 #CJK UNIFIED IDEOGRAPH
    {0xF5D0, 0x6B03}, //17135 #CJK UNIFIED IDEOGRAPH
    {0xF5D1, 0x6AF8}, //17136 #CJK UNIFIED IDEOGRAPH
    {0xF5D2, 0x6B00}, //17137 #CJK UNIFIED IDEOGRAPH
    {0xF5D3, 0x7043}, //17138 #CJK UNIFIED IDEOGRAPH
    {0xF5D4, 0x7044}, //17139 #CJK UNIFIED IDEOGRAPH
    {0xF5D5, 0x704A}, //17140 #CJK UNIFIED IDEOGRAPH
    {0xF5D6, 0x7048}, //17141 #CJK UNIFIED IDEOGRAPH
    {0xF5D7, 0x7049}, //17142 #CJK UNIFIED IDEOGRAPH
    {0xF5D8, 0x7045}, //17143 #CJK UNIFIED IDEOGRAPH
    {0xF5D9, 0x7046}, //17144 #CJK UNIFIED IDEOGRAPH
    {0xF5DA, 0x721D}, //17145 #CJK UNIFIED IDEOGRAPH
    {0xF5DB, 0x721A}, //17146 #CJK UNIFIED IDEOGRAPH
    {0xF5DC, 0x7219}, //17147 #CJK UNIFIED IDEOGRAPH
    {0xF5DD, 0x737E}, //17148 #CJK UNIFIED IDEOGRAPH
    {0xF5DE, 0x7517}, //17149 #CJK UNIFIED IDEOGRAPH
    {0xF5DF, 0x766A}, //17150 #CJK UNIFIED IDEOGRAPH
    {0xF5E0, 0x77D0}, //17151 #CJK UNIFIED IDEOGRAPH
    {0xF5E1, 0x792D}, //17152 #CJK UNIFIED IDEOGRAPH
    {0xF5E2, 0x7931}, //17153 #CJK UNIFIED IDEOGRAPH
    {0xF5E3, 0x792F}, //17154 #CJK UNIFIED IDEOGRAPH
    {0xF5E4, 0x7C54}, //17155 #CJK UNIFIED IDEOGRAPH
    {0xF5E5, 0x7C53}, //17156 #CJK UNIFIED IDEOGRAPH
    {0xF5E6, 0x7CF2}, //17157 #CJK UNIFIED IDEOGRAPH
    {0xF5E7, 0x7E8A}, //17158 #CJK UNIFIED IDEOGRAPH
    {0xF5E8, 0x7E87}, //17159 #CJK UNIFIED IDEOGRAPH
    {0xF5E9, 0x7E88}, //17160 #CJK UNIFIED IDEOGRAPH
    {0xF5EA, 0x7E8B}, //17161 #CJK UNIFIED IDEOGRAPH
    {0xF5EB, 0x7E86}, //17162 #CJK UNIFIED IDEOGRAPH
    {0xF5EC, 0x7E8D}, //17163 #CJK UNIFIED IDEOGRAPH
    {0xF5ED, 0x7F4D}, //17164 #CJK UNIFIED IDEOGRAPH
    {0xF5EE, 0x7FBB}, //17165 #CJK UNIFIED IDEOGRAPH
    {0xF5EF, 0x8030}, //17166 #CJK UNIFIED IDEOGRAPH
    {0xF5F0, 0x81DD}, //17167 #CJK UNIFIED IDEOGRAPH
    {0xF5F1, 0x8618}, //17168 #CJK UNIFIED IDEOGRAPH
    {0xF5F2, 0x862A}, //17169 #CJK UNIFIED IDEOGRAPH
    {0xF5F3, 0x8626}, //17170 #CJK UNIFIED IDEOGRAPH
    {0xF5F4, 0x861F}, //17171 #CJK UNIFIED IDEOGRAPH
    {0xF5F5, 0x8623}, //17172 #CJK UNIFIED IDEOGRAPH
    {0xF5F6, 0x861C}, //17173 #CJK UNIFIED IDEOGRAPH
    {0xF5F7, 0x8619}, //17174 #CJK UNIFIED IDEOGRAPH
    {0xF5F8, 0x8627}, //17175 #CJK UNIFIED IDEOGRAPH
    {0xF5F9, 0x862E}, //17176 #CJK UNIFIED IDEOGRAPH
    {0xF5FA, 0x8621}, //17177 #CJK UNIFIED IDEOGRAPH
    {0xF5FB, 0x8620}, //17178 #CJK UNIFIED IDEOGRAPH
    {0xF5FC, 0x8629}, //17179 #CJK UNIFIED IDEOGRAPH
    {0xF5FD, 0x861E}, //17180 #CJK UNIFIED IDEOGRAPH
    {0xF5FE, 0x8625}, //17181 #CJK UNIFIED IDEOGRAPH
    {0xF640, 0x8829}, //17182 #CJK UNIFIED IDEOGRAPH
    {0xF641, 0x881D}, //17183 #CJK UNIFIED IDEOGRAPH
    {0xF642, 0x881B}, //17184 #CJK UNIFIED IDEOGRAPH
    {0xF643, 0x8820}, //17185 #CJK UNIFIED IDEOGRAPH
    {0xF644, 0x8824}, //17186 #CJK UNIFIED IDEOGRAPH
    {0xF645, 0x881C}, //17187 #CJK UNIFIED IDEOGRAPH
    {0xF646, 0x882B}, //17188 #CJK UNIFIED IDEOGRAPH
    {0xF647, 0x884A}, //17189 #CJK UNIFIED IDEOGRAPH
    {0xF648, 0x896D}, //17190 #CJK UNIFIED IDEOGRAPH
    {0xF649, 0x8969}, //17191 #CJK UNIFIED IDEOGRAPH
    {0xF64A, 0x896E}, //17192 #CJK UNIFIED IDEOGRAPH
    {0xF64B, 0x896B}, //17193 #CJK UNIFIED IDEOGRAPH
    {0xF64C, 0x89FA}, //17194 #CJK UNIFIED IDEOGRAPH
    {0xF64D, 0x8B79}, //17195 #CJK UNIFIED IDEOGRAPH
    {0xF64E, 0x8B78}, //17196 #CJK UNIFIED IDEOGRAPH
    {0xF64F, 0x8B45}, //17197 #CJK UNIFIED IDEOGRAPH
    {0xF650, 0x8B7A}, //17198 #CJK UNIFIED IDEOGRAPH
    {0xF651, 0x8B7B}, //17199 #CJK UNIFIED IDEOGRAPH
    {0xF652, 0x8D10}, //17200 #CJK UNIFIED IDEOGRAPH
    {0xF653, 0x8D14}, //17201 #CJK UNIFIED IDEOGRAPH
    {0xF654, 0x8DAF}, //17202 #CJK UNIFIED IDEOGRAPH
    {0xF655, 0x8E8E}, //17203 #CJK UNIFIED IDEOGRAPH
    {0xF656, 0x8E8C}, //17204 #CJK UNIFIED IDEOGRAPH
    {0xF657, 0x8F5E}, //17205 #CJK UNIFIED IDEOGRAPH
    {0xF658, 0x8F5B}, //17206 #CJK UNIFIED IDEOGRAPH
    {0xF659, 0x8F5D}, //17207 #CJK UNIFIED IDEOGRAPH
    {0xF65A, 0x9146}, //17208 #CJK UNIFIED IDEOGRAPH
    {0xF65B, 0x9144}, //17209 #CJK UNIFIED IDEOGRAPH
    {0xF65C, 0x9145}, //17210 #CJK UNIFIED IDEOGRAPH
    {0xF65D, 0x91B9}, //17211 #CJK UNIFIED IDEOGRAPH
    {0xF65E, 0x943F}, //17212 #CJK UNIFIED IDEOGRAPH
    {0xF65F, 0x943B}, //17213 #CJK UNIFIED IDEOGRAPH
    {0xF660, 0x9436}, //17214 #CJK UNIFIED IDEOGRAPH
    {0xF661, 0x9429}, //17215 #CJK UNIFIED IDEOGRAPH
    {0xF662, 0x943D}, //17216 #CJK UNIFIED IDEOGRAPH
    {0xF663, 0x943C}, //17217 #CJK UNIFIED IDEOGRAPH
    {0xF664, 0x9430}, //17218 #CJK UNIFIED IDEOGRAPH
    {0xF665, 0x9439}, //17219 #CJK UNIFIED IDEOGRAPH
    {0xF666, 0x942A}, //17220 #CJK UNIFIED IDEOGRAPH
    {0xF667, 0x9437}, //17221 #CJK UNIFIED IDEOGRAPH
    {0xF668, 0x942C}, //17222 #CJK UNIFIED IDEOGRAPH
    {0xF669, 0x9440}, //17223 #CJK UNIFIED IDEOGRAPH
    {0xF66A, 0x9431}, //17224 #CJK UNIFIED IDEOGRAPH
    {0xF66B, 0x95E5}, //17225 #CJK UNIFIED IDEOGRAPH
    {0xF66C, 0x95E4}, //17226 #CJK UNIFIED IDEOGRAPH
    {0xF66D, 0x95E3}, //17227 #CJK UNIFIED IDEOGRAPH
    {0xF66E, 0x9735}, //17228 #CJK UNIFIED IDEOGRAPH
    {0xF66F, 0x973A}, //17229 #CJK UNIFIED IDEOGRAPH
    {0xF670, 0x97BF}, //17230 #CJK UNIFIED IDEOGRAPH
    {0xF671, 0x97E1}, //17231 #CJK UNIFIED IDEOGRAPH
    {0xF672, 0x9864}, //17232 #CJK UNIFIED IDEOGRAPH
    {0xF673, 0x98C9}, //17233 #CJK UNIFIED IDEOGRAPH
    {0xF674, 0x98C6}, //17234 #CJK UNIFIED IDEOGRAPH
    {0xF675, 0x98C0}, //17235 #CJK UNIFIED IDEOGRAPH
    {0xF676, 0x9958}, //17236 #CJK UNIFIED IDEOGRAPH
    {0xF677, 0x9956}, //17237 #CJK UNIFIED IDEOGRAPH
    {0xF678, 0x9A39}, //17238 #CJK UNIFIED IDEOGRAPH
    {0xF679, 0x9A3D}, //17239 #CJK UNIFIED IDEOGRAPH
    {0xF67A, 0x9A46}, //17240 #CJK UNIFIED IDEOGRAPH
    {0xF67B, 0x9A44}, //17241 #CJK UNIFIED IDEOGRAPH
    {0xF67C, 0x9A42}, //17242 #CJK UNIFIED IDEOGRAPH
    {0xF67D, 0x9A41}, //17243 #CJK UNIFIED IDEOGRAPH
    {0xF67E, 0x9A3A}, //17244 #CJK UNIFIED IDEOGRAPH
    {0xF6A1, 0x9A3F}, //17245 #CJK UNIFIED IDEOGRAPH
    {0xF6A2, 0x9ACD}, //17246 #CJK UNIFIED IDEOGRAPH
    {0xF6A3, 0x9B15}, //17247 #CJK UNIFIED IDEOGRAPH
    {0xF6A4, 0x9B17}, //17248 #CJK UNIFIED IDEOGRAPH
    {0xF6A5, 0x9B18}, //17249 #CJK UNIFIED IDEOGRAPH
    {0xF6A6, 0x9B16}, //17250 #CJK UNIFIED IDEOGRAPH
    {0xF6A7, 0x9B3A}, //17251 #CJK UNIFIED IDEOGRAPH
    {0xF6A8, 0x9B52}, //17252 #CJK UNIFIED IDEOGRAPH
    {0xF6A9, 0x9C2B}, //17253 #CJK UNIFIED IDEOGRAPH
    {0xF6AA, 0x9C1D}, //17254 #CJK UNIFIED IDEOGRAPH
    {0xF6AB, 0x9C1C}, //17255 #CJK UNIFIED IDEOGRAPH
    {0xF6AC, 0x9C2C}, //17256 #CJK UNIFIED IDEOGRAPH
    {0xF6AD, 0x9C23}, //17257 #CJK UNIFIED IDEOGRAPH
    {0xF6AE, 0x9C28}, //17258 #CJK UNIFIED IDEOGRAPH
    {0xF6AF, 0x9C29}, //17259 #CJK UNIFIED IDEOGRAPH
    {0xF6B0, 0x9C24}, //17260 #CJK UNIFIED IDEOGRAPH
    {0xF6B1, 0x9C21}, //17261 #CJK UNIFIED IDEOGRAPH
    {0xF6B2, 0x9DB7}, //17262 #CJK UNIFIED IDEOGRAPH
    {0xF6B3, 0x9DB6}, //17263 #CJK UNIFIED IDEOGRAPH
    {0xF6B4, 0x9DBC}, //17264 #CJK UNIFIED IDEOGRAPH
    {0xF6B5, 0x9DC1}, //17265 #CJK UNIFIED IDEOGRAPH
    {0xF6B6, 0x9DC7}, //17266 #CJK UNIFIED IDEOGRAPH
    {0xF6B7, 0x9DCA}, //17267 #CJK UNIFIED IDEOGRAPH
    {0xF6B8, 0x9DCF}, //17268 #CJK UNIFIED IDEOGRAPH
    {0xF6B9, 0x9DBE}, //17269 #CJK UNIFIED IDEOGRAPH
    {0xF6BA, 0x9DC5}, //17270 #CJK UNIFIED IDEOGRAPH
    {0xF6BB, 0x9DC3}, //17271 #CJK UNIFIED IDEOGRAPH
    {0xF6BC, 0x9DBB}, //17272 #CJK UNIFIED IDEOGRAPH
    {0xF6BD, 0x9DB5}, //17273 #CJK UNIFIED IDEOGRAPH
    {0xF6BE, 0x9DCE}, //17274 #CJK UNIFIED IDEOGRAPH
    {0xF6BF, 0x9DB9}, //17275 #CJK UNIFIED IDEOGRAPH
    {0xF6C0, 0x9DBA}, //17276 #CJK UNIFIED IDEOGRAPH
    {0xF6C1, 0x9DAC}, //17277 #CJK UNIFIED IDEOGRAPH
    {0xF6C2, 0x9DC8}, //17278 #CJK UNIFIED IDEOGRAPH
    {0xF6C3, 0x9DB1}, //17279 #CJK UNIFIED IDEOGRAPH
    {0xF6C4, 0x9DAD}, //17280 #CJK UNIFIED IDEOGRAPH
    {0xF6C5, 0x9DCC}, //17281 #CJK UNIFIED IDEOGRAPH
    {0xF6C6, 0x9DB3}, //17282 #CJK UNIFIED IDEOGRAPH
    {0xF6C7, 0x9DCD}, //17283 #CJK UNIFIED IDEOGRAPH
    {0xF6C8, 0x9DB2}, //17284 #CJK UNIFIED IDEOGRAPH
    {0xF6C9, 0x9E7A}, //17285 #CJK UNIFIED IDEOGRAPH
    {0xF6CA, 0x9E9C}, //17286 #CJK UNIFIED IDEOGRAPH
    {0xF6CB, 0x9EEB}, //17287 #CJK UNIFIED IDEOGRAPH
    {0xF6CC, 0x9EEE}, //17288 #CJK UNIFIED IDEOGRAPH
    {0xF6CD, 0x9EED}, //17289 #CJK UNIFIED IDEOGRAPH
    {0xF6CE, 0x9F1B}, //17290 #CJK UNIFIED IDEOGRAPH
    {0xF6CF, 0x9F18}, //17291 #CJK UNIFIED IDEOGRAPH
    {0xF6D0, 0x9F1A}, //17292 #CJK UNIFIED IDEOGRAPH
    {0xF6D1, 0x9F31}, //17293 #CJK UNIFIED IDEOGRAPH
    {0xF6D2, 0x9F4E}, //17294 #CJK UNIFIED IDEOGRAPH
    {0xF6D3, 0x9F65}, //17295 #CJK UNIFIED IDEOGRAPH
    {0xF6D4, 0x9F64}, //17296 #CJK UNIFIED IDEOGRAPH
    {0xF6D5, 0x9F92}, //17297 #CJK UNIFIED IDEOGRAPH
    {0xF6D6, 0x4EB9}, //17298 #CJK UNIFIED IDEOGRAPH
    {0xF6D7, 0x56C6}, //17299 #CJK UNIFIED IDEOGRAPH
    {0xF6D8, 0x56C5}, //17300 #CJK UNIFIED IDEOGRAPH
    {0xF6D9, 0x56CB}, //17301 #CJK UNIFIED IDEOGRAPH
    {0xF6DA, 0x5971}, //17302 #CJK UNIFIED IDEOGRAPH
    {0xF6DB, 0x5B4B}, //17303 #CJK UNIFIED IDEOGRAPH
    {0xF6DC, 0x5B4C}, //17304 #CJK UNIFIED IDEOGRAPH
    {0xF6DD, 0x5DD5}, //17305 #CJK UNIFIED IDEOGRAPH
    {0xF6DE, 0x5DD1}, //17306 #CJK UNIFIED IDEOGRAPH
    {0xF6DF, 0x5EF2}, //17307 #CJK UNIFIED IDEOGRAPH
    {0xF6E0, 0x6521}, //17308 #CJK UNIFIED IDEOGRAPH
    {0xF6E1, 0x6520}, //17309 #CJK UNIFIED IDEOGRAPH
    {0xF6E2, 0x6526}, //17310 #CJK UNIFIED IDEOGRAPH
    {0xF6E3, 0x6522}, //17311 #CJK UNIFIED IDEOGRAPH
    {0xF6E4, 0x6B0B}, //17312 #CJK UNIFIED IDEOGRAPH
    {0xF6E5, 0x6B08}, //17313 #CJK UNIFIED IDEOGRAPH
    {0xF6E6, 0x6B09}, //17314 #CJK UNIFIED IDEOGRAPH
    {0xF6E7, 0x6C0D}, //17315 #CJK UNIFIED IDEOGRAPH
    {0xF6E8, 0x7055}, //17316 #CJK UNIFIED IDEOGRAPH
    {0xF6E9, 0x7056}, //17317 #CJK UNIFIED IDEOGRAPH
    {0xF6EA, 0x7057}, //17318 #CJK UNIFIED IDEOGRAPH
    {0xF6EB, 0x7052}, //17319 #CJK UNIFIED IDEOGRAPH
    {0xF6EC, 0x721E}, //17320 #CJK UNIFIED IDEOGRAPH
    {0xF6ED, 0x721F}, //17321 #CJK UNIFIED IDEOGRAPH
    {0xF6EE, 0x72A9}, //17322 #CJK UNIFIED IDEOGRAPH
    {0xF6EF, 0x737F}, //17323 #CJK UNIFIED IDEOGRAPH
    {0xF6F0, 0x74D8}, //17324 #CJK UNIFIED IDEOGRAPH
    {0xF6F1, 0x74D5}, //17325 #CJK UNIFIED IDEOGRAPH
    {0xF6F2, 0x74D9}, //17326 #CJK UNIFIED IDEOGRAPH
    {0xF6F3, 0x74D7}, //17327 #CJK UNIFIED IDEOGRAPH
    {0xF6F4, 0x766D}, //17328 #CJK UNIFIED IDEOGRAPH
    {0xF6F5, 0x76AD}, //17329 #CJK UNIFIED IDEOGRAPH
    {0xF6F6, 0x7935}, //17330 #CJK UNIFIED IDEOGRAPH
    {0xF6F7, 0x79B4}, //17331 #CJK UNIFIED IDEOGRAPH
    {0xF6F8, 0x7A70}, //17332 #CJK UNIFIED IDEOGRAPH
    {0xF6F9, 0x7A71}, //17333 #CJK UNIFIED IDEOGRAPH
    {0xF6FA, 0x7C57}, //17334 #CJK UNIFIED IDEOGRAPH
    {0xF6FB, 0x7C5C}, //17335 #CJK UNIFIED IDEOGRAPH
    {0xF6FC, 0x7C59}, //17336 #CJK UNIFIED IDEOGRAPH
    {0xF6FD, 0x7C5B}, //17337 #CJK UNIFIED IDEOGRAPH
    {0xF6FE, 0x7C5A}, //17338 #CJK UNIFIED IDEOGRAPH
    {0xF740, 0x7CF4}, //17339 #CJK UNIFIED IDEOGRAPH
    {0xF741, 0x7CF1}, //17340 #CJK UNIFIED IDEOGRAPH
    {0xF742, 0x7E91}, //17341 #CJK UNIFIED IDEOGRAPH
    {0xF743, 0x7F4F}, //17342 #CJK UNIFIED IDEOGRAPH
    {0xF744, 0x7F87}, //17343 #CJK UNIFIED IDEOGRAPH
    {0xF745, 0x81DE}, //17344 #CJK UNIFIED IDEOGRAPH
    {0xF746, 0x826B}, //17345 #CJK UNIFIED IDEOGRAPH
    {0xF747, 0x8634}, //17346 #CJK UNIFIED IDEOGRAPH
    {0xF748, 0x8635}, //17347 #CJK UNIFIED IDEOGRAPH
    {0xF749, 0x8633}, //17348 #CJK UNIFIED IDEOGRAPH
    {0xF74A, 0x862C}, //17349 #CJK UNIFIED IDEOGRAPH
    {0xF74B, 0x8632}, //17350 #CJK UNIFIED IDEOGRAPH
    {0xF74C, 0x8636}, //17351 #CJK UNIFIED IDEOGRAPH
    {0xF74D, 0x882C}, //17352 #CJK UNIFIED IDEOGRAPH
    {0xF74E, 0x8828}, //17353 #CJK UNIFIED IDEOGRAPH
    {0xF74F, 0x8826}, //17354 #CJK UNIFIED IDEOGRAPH
    {0xF750, 0x882A}, //17355 #CJK UNIFIED IDEOGRAPH
    {0xF751, 0x8825}, //17356 #CJK UNIFIED IDEOGRAPH
    {0xF752, 0x8971}, //17357 #CJK UNIFIED IDEOGRAPH
    {0xF753, 0x89BF}, //17358 #CJK UNIFIED IDEOGRAPH
    {0xF754, 0x89BE}, //17359 #CJK UNIFIED IDEOGRAPH
    {0xF755, 0x89FB}, //17360 #CJK UNIFIED IDEOGRAPH
    {0xF756, 0x8B7E}, //17361 #CJK UNIFIED IDEOGRAPH
    {0xF757, 0x8B84}, //17362 #CJK UNIFIED IDEOGRAPH
    {0xF758, 0x8B82}, //17363 #CJK UNIFIED IDEOGRAPH
    {0xF759, 0x8B86}, //17364 #CJK UNIFIED IDEOGRAPH
    {0xF75A, 0x8B85}, //17365 #CJK UNIFIED IDEOGRAPH
    {0xF75B, 0x8B7F}, //17366 #CJK UNIFIED IDEOGRAPH
    {0xF75C, 0x8D15}, //17367 #CJK UNIFIED IDEOGRAPH
    {0xF75D, 0x8E95}, //17368 #CJK UNIFIED IDEOGRAPH
    {0xF75E, 0x8E94}, //17369 #CJK UNIFIED IDEOGRAPH
    {0xF75F, 0x8E9A}, //17370 #CJK UNIFIED IDEOGRAPH
    {0xF760, 0x8E92}, //17371 #CJK UNIFIED IDEOGRAPH
    {0xF761, 0x8E90}, //17372 #CJK UNIFIED IDEOGRAPH
    {0xF762, 0x8E96}, //17373 #CJK UNIFIED IDEOGRAPH
    {0xF763, 0x8E97}, //17374 #CJK UNIFIED IDEOGRAPH
    {0xF764, 0x8F60}, //17375 #CJK UNIFIED IDEOGRAPH
    {0xF765, 0x8F62}, //17376 #CJK UNIFIED IDEOGRAPH
    {0xF766, 0x9147}, //17377 #CJK UNIFIED IDEOGRAPH
    {0xF767, 0x944C}, //17378 #CJK UNIFIED IDEOGRAPH
    {0xF768, 0x9450}, //17379 #CJK UNIFIED IDEOGRAPH
    {0xF769, 0x944A}, //17380 #CJK UNIFIED IDEOGRAPH
    {0xF76A, 0x944B}, //17381 #CJK UNIFIED IDEOGRAPH
    {0xF76B, 0x944F}, //17382 #CJK UNIFIED IDEOGRAPH
    {0xF76C, 0x9447}, //17383 #CJK UNIFIED IDEOGRAPH
    {0xF76D, 0x9445}, //17384 #CJK UNIFIED IDEOGRAPH
    {0xF76E, 0x9448}, //17385 #CJK UNIFIED IDEOGRAPH
    {0xF76F, 0x9449}, //17386 #CJK UNIFIED IDEOGRAPH
    {0xF770, 0x9446}, //17387 #CJK UNIFIED IDEOGRAPH
    {0xF771, 0x973F}, //17388 #CJK UNIFIED IDEOGRAPH
    {0xF772, 0x97E3}, //17389 #CJK UNIFIED IDEOGRAPH
    {0xF773, 0x986A}, //17390 #CJK UNIFIED IDEOGRAPH
    {0xF774, 0x9869}, //17391 #CJK UNIFIED IDEOGRAPH
    {0xF775, 0x98CB}, //17392 #CJK UNIFIED IDEOGRAPH
    {0xF776, 0x9954}, //17393 #CJK UNIFIED IDEOGRAPH
    {0xF777, 0x995B}, //17394 #CJK UNIFIED IDEOGRAPH
    {0xF778, 0x9A4E}, //17395 #CJK UNIFIED IDEOGRAPH
    {0xF779, 0x9A53}, //17396 #CJK UNIFIED IDEOGRAPH
    {0xF77A, 0x9A54}, //17397 #CJK UNIFIED IDEOGRAPH
    {0xF77B, 0x9A4C}, //17398 #CJK UNIFIED IDEOGRAPH
    {0xF77C, 0x9A4F}, //17399 #CJK UNIFIED IDEOGRAPH
    {0xF77D, 0x9A48}, //17400 #CJK UNIFIED IDEOGRAPH
    {0xF77E, 0x9A4A}, //17401 #CJK UNIFIED IDEOGRAPH
    {0xF7A1, 0x9A49}, //17402 #CJK UNIFIED IDEOGRAPH
    {0xF7A2, 0x9A52}, //17403 #CJK UNIFIED IDEOGRAPH
    {0xF7A3, 0x9A50}, //17404 #CJK UNIFIED IDEOGRAPH
    {0xF7A4, 0x9AD0}, //17405 #CJK UNIFIED IDEOGRAPH
    {0xF7A5, 0x9B19}, //17406 #CJK UNIFIED IDEOGRAPH
    {0xF7A6, 0x9B2B}, //17407 #CJK UNIFIED IDEOGRAPH
    {0xF7A7, 0x9B3B}, //17408 #CJK UNIFIED IDEOGRAPH
    {0xF7A8, 0x9B56}, //17409 #CJK UNIFIED IDEOGRAPH
    {0xF7A9, 0x9B55}, //17410 #CJK UNIFIED IDEOGRAPH
    {0xF7AA, 0x9C46}, //17411 #CJK UNIFIED IDEOGRAPH
    {0xF7AB, 0x9C48}, //17412 #CJK UNIFIED IDEOGRAPH
    {0xF7AC, 0x9C3F}, //17413 #CJK UNIFIED IDEOGRAPH
    {0xF7AD, 0x9C44}, //17414 #CJK UNIFIED IDEOGRAPH
    {0xF7AE, 0x9C39}, //17415 #CJK UNIFIED IDEOGRAPH
    {0xF7AF, 0x9C33}, //17416 #CJK UNIFIED IDEOGRAPH
    {0xF7B0, 0x9C41}, //17417 #CJK UNIFIED IDEOGRAPH
    {0xF7B1, 0x9C3C}, //17418 #CJK UNIFIED IDEOGRAPH
    {0xF7B2, 0x9C37}, //17419 #CJK UNIFIED IDEOGRAPH
    {0xF7B3, 0x9C34}, //17420 #CJK UNIFIED IDEOGRAPH
    {0xF7B4, 0x9C32}, //17421 #CJK UNIFIED IDEOGRAPH
    {0xF7B5, 0x9C3D}, //17422 #CJK UNIFIED IDEOGRAPH
    {0xF7B6, 0x9C36}, //17423 #CJK UNIFIED IDEOGRAPH
    {0xF7B7, 0x9DDB}, //17424 #CJK UNIFIED IDEOGRAPH
    {0xF7B8, 0x9DD2}, //17425 #CJK UNIFIED IDEOGRAPH
    {0xF7B9, 0x9DDE}, //17426 #CJK UNIFIED IDEOGRAPH
    {0xF7BA, 0x9DDA}, //17427 #CJK UNIFIED IDEOGRAPH
    {0xF7BB, 0x9DCB}, //17428 #CJK UNIFIED IDEOGRAPH
    {0xF7BC, 0x9DD0}, //17429 #CJK UNIFIED IDEOGRAPH
    {0xF7BD, 0x9DDC}, //17430 #CJK UNIFIED IDEOGRAPH
    {0xF7BE, 0x9DD1}, //17431 #CJK UNIFIED IDEOGRAPH
    {0xF7BF, 0x9DDF}, //17432 #CJK UNIFIED IDEOGRAPH
    {0xF7C0, 0x9DE9}, //17433 #CJK UNIFIED IDEOGRAPH
    {0xF7C1, 0x9DD9}, //17434 #CJK UNIFIED IDEOGRAPH
    {0xF7C2, 0x9DD8}, //17435 #CJK UNIFIED IDEOGRAPH
    {0xF7C3, 0x9DD6}, //17436 #CJK UNIFIED IDEOGRAPH
    {0xF7C4, 0x9DF5}, //17437 #CJK UNIFIED IDEOGRAPH
    {0xF7C5, 0x9DD5}, //17438 #CJK UNIFIED IDEOGRAPH
    {0xF7C6, 0x9DDD}, //17439 #CJK UNIFIED IDEOGRAPH
    {0xF7C7, 0x9EB6}, //17440 #CJK UNIFIED IDEOGRAPH
    {0xF7C8, 0x9EF0}, //17441 #CJK UNIFIED IDEOGRAPH
    {0xF7C9, 0x9F35}, //17442 #CJK UNIFIED IDEOGRAPH
    {0xF7CA, 0x9F33}, //17443 #CJK UNIFIED IDEOGRAPH
    {0xF7CB, 0x9F32}, //17444 #CJK UNIFIED IDEOGRAPH
    {0xF7CC, 0x9F42}, //17445 #CJK UNIFIED IDEOGRAPH
    {0xF7CD, 0x9F6B}, //17446 #CJK UNIFIED IDEOGRAPH
    {0xF7CE, 0x9F95}, //17447 #CJK UNIFIED IDEOGRAPH
    {0xF7CF, 0x9FA2}, //17448 #CJK UNIFIED IDEOGRAPH
    {0xF7D0, 0x513D}, //17449 #CJK UNIFIED IDEOGRAPH
    {0xF7D1, 0x5299}, //17450 #CJK UNIFIED IDEOGRAPH
    {0xF7D2, 0x58E8}, //17451 #CJK UNIFIED IDEOGRAPH
    {0xF7D3, 0x58E7}, //17452 #CJK UNIFIED IDEOGRAPH
    {0xF7D4, 0x5972}, //17453 #CJK UNIFIED IDEOGRAPH
    {0xF7D5, 0x5B4D}, //17454 #CJK UNIFIED IDEOGRAPH
    {0xF7D6, 0x5DD8}, //17455 #CJK UNIFIED IDEOGRAPH
    {0xF7D7, 0x882F}, //17456 #CJK UNIFIED IDEOGRAPH
    {0xF7D8, 0x5F4F}, //17457 #CJK UNIFIED IDEOGRAPH
    {0xF7D9, 0x6201}, //17458 #CJK UNIFIED IDEOGRAPH
    {0xF7DA, 0x6203}, //17459 #CJK UNIFIED IDEOGRAPH
    {0xF7DB, 0x6204}, //17460 #CJK UNIFIED IDEOGRAPH
    {0xF7DC, 0x6529}, //17461 #CJK UNIFIED IDEOGRAPH
    {0xF7DD, 0x6525}, //17462 #CJK UNIFIED IDEOGRAPH
    {0xF7DE, 0x6596}, //17463 #CJK UNIFIED IDEOGRAPH
    {0xF7DF, 0x66EB}, //17464 #CJK UNIFIED IDEOGRAPH
    {0xF7E0, 0x6B11}, //17465 #CJK UNIFIED IDEOGRAPH
    {0xF7E1, 0x6B12}, //17466 #CJK UNIFIED IDEOGRAPH
    {0xF7E2, 0x6B0F}, //17467 #CJK UNIFIED IDEOGRAPH
    {0xF7E3, 0x6BCA}, //17468 #CJK UNIFIED IDEOGRAPH
    {0xF7E4, 0x705B}, //17469 #CJK UNIFIED IDEOGRAPH
    {0xF7E5, 0x705A}, //17470 #CJK UNIFIED IDEOGRAPH
    {0xF7E6, 0x7222}, //17471 #CJK UNIFIED IDEOGRAPH
    {0xF7E7, 0x7382}, //17472 #CJK UNIFIED IDEOGRAPH
    {0xF7E8, 0x7381}, //17473 #CJK UNIFIED IDEOGRAPH
    {0xF7E9, 0x7383}, //17474 #CJK UNIFIED IDEOGRAPH
    {0xF7EA, 0x7670}, //17475 #CJK UNIFIED IDEOGRAPH
    {0xF7EB, 0x77D4}, //17476 #CJK UNIFIED IDEOGRAPH
    {0xF7EC, 0x7C67}, //17477 #CJK UNIFIED IDEOGRAPH
    {0xF7ED, 0x7C66}, //17478 #CJK UNIFIED IDEOGRAPH
    {0xF7EE, 0x7E95}, //17479 #CJK UNIFIED IDEOGRAPH
    {0xF7EF, 0x826C}, //17480 #CJK UNIFIED IDEOGRAPH
    {0xF7F0, 0x863A}, //17481 #CJK UNIFIED IDEOGRAPH
    {0xF7F1, 0x8640}, //17482 #CJK UNIFIED IDEOGRAPH
    {0xF7F2, 0x8639}, //17483 #CJK UNIFIED IDEOGRAPH
    {0xF7F3, 0x863C}, //17484 #CJK UNIFIED IDEOGRAPH
    {0xF7F4, 0x8631}, //17485 #CJK UNIFIED IDEOGRAPH
    {0xF7F5, 0x863B}, //17486 #CJK UNIFIED IDEOGRAPH
    {0xF7F6, 0x863E}, //17487 #CJK UNIFIED IDEOGRAPH
    {0xF7F7, 0x8830}, //17488 #CJK UNIFIED IDEOGRAPH
    {0xF7F8, 0x8832}, //17489 #CJK UNIFIED IDEOGRAPH
    {0xF7F9, 0x882E}, //17490 #CJK UNIFIED IDEOGRAPH
    {0xF7FA, 0x8833}, //17491 #CJK UNIFIED IDEOGRAPH
    {0xF7FB, 0x8976}, //17492 #CJK UNIFIED IDEOGRAPH
    {0xF7FC, 0x8974}, //17493 #CJK UNIFIED IDEOGRAPH
    {0xF7FD, 0x8973}, //17494 #CJK UNIFIED IDEOGRAPH
    {0xF7FE, 0x89FE}, //17495 #CJK UNIFIED IDEOGRAPH
    {0xF840, 0x8B8C}, //17496 #CJK UNIFIED IDEOGRAPH
    {0xF841, 0x8B8E}, //17497 #CJK UNIFIED IDEOGRAPH
    {0xF842, 0x8B8B}, //17498 #CJK UNIFIED IDEOGRAPH
    {0xF843, 0x8B88}, //17499 #CJK UNIFIED IDEOGRAPH
    {0xF844, 0x8C45}, //17500 #CJK UNIFIED IDEOGRAPH
    {0xF845, 0x8D19}, //17501 #CJK UNIFIED IDEOGRAPH
    {0xF846, 0x8E98}, //17502 #CJK UNIFIED IDEOGRAPH
    {0xF847, 0x8F64}, //17503 #CJK UNIFIED IDEOGRAPH
    {0xF848, 0x8F63}, //17504 #CJK UNIFIED IDEOGRAPH
    {0xF849, 0x91BC}, //17505 #CJK UNIFIED IDEOGRAPH
    {0xF84A, 0x9462}, //17506 #CJK UNIFIED IDEOGRAPH
    {0xF84B, 0x9455}, //17507 #CJK UNIFIED IDEOGRAPH
    {0xF84C, 0x945D}, //17508 #CJK UNIFIED IDEOGRAPH
    {0xF84D, 0x9457}, //17509 #CJK UNIFIED IDEOGRAPH
    {0xF84E, 0x945E}, //17510 #CJK UNIFIED IDEOGRAPH
    {0xF84F, 0x97C4}, //17511 #CJK UNIFIED IDEOGRAPH
    {0xF850, 0x97C5}, //17512 #CJK UNIFIED IDEOGRAPH
    {0xF851, 0x9800}, //17513 #CJK UNIFIED IDEOGRAPH
    {0xF852, 0x9A56}, //17514 #CJK UNIFIED IDEOGRAPH
    {0xF853, 0x9A59}, //17515 #CJK UNIFIED IDEOGRAPH
    {0xF854, 0x9B1E}, //17516 #CJK UNIFIED IDEOGRAPH
    {0xF855, 0x9B1F}, //17517 #CJK UNIFIED IDEOGRAPH
    {0xF856, 0x9B20}, //17518 #CJK UNIFIED IDEOGRAPH
    {0xF857, 0x9C52}, //17519 #CJK UNIFIED IDEOGRAPH
    {0xF858, 0x9C58}, //17520 #CJK UNIFIED IDEOGRAPH
    {0xF859, 0x9C50}, //17521 #CJK UNIFIED IDEOGRAPH
    {0xF85A, 0x9C4A}, //17522 #CJK UNIFIED IDEOGRAPH
    {0xF85B, 0x9C4D}, //17523 #CJK UNIFIED IDEOGRAPH
    {0xF85C, 0x9C4B}, //17524 #CJK UNIFIED IDEOGRAPH
    {0xF85D, 0x9C55}, //17525 #CJK UNIFIED IDEOGRAPH
    {0xF85E, 0x9C59}, //17526 #CJK UNIFIED IDEOGRAPH
    {0xF85F, 0x9C4C}, //17527 #CJK UNIFIED IDEOGRAPH
    {0xF860, 0x9C4E}, //17528 #CJK UNIFIED IDEOGRAPH
    {0xF861, 0x9DFB}, //17529 #CJK UNIFIED IDEOGRAPH
    {0xF862, 0x9DF7}, //17530 #CJK UNIFIED IDEOGRAPH
    {0xF863, 0x9DEF}, //17531 #CJK UNIFIED IDEOGRAPH
    {0xF864, 0x9DE3}, //17532 #CJK UNIFIED IDEOGRAPH
    {0xF865, 0x9DEB}, //17533 #CJK UNIFIED IDEOGRAPH
    {0xF866, 0x9DF8}, //17534 #CJK UNIFIED IDEOGRAPH
    {0xF867, 0x9DE4}, //17535 #CJK UNIFIED IDEOGRAPH
    {0xF868, 0x9DF6}, //17536 #CJK UNIFIED IDEOGRAPH
    {0xF869, 0x9DE1}, //17537 #CJK UNIFIED IDEOGRAPH
    {0xF86A, 0x9DEE}, //17538 #CJK UNIFIED IDEOGRAPH
    {0xF86B, 0x9DE6}, //17539 #CJK UNIFIED IDEOGRAPH
    {0xF86C, 0x9DF2}, //17540 #CJK UNIFIED IDEOGRAPH
    {0xF86D, 0x9DF0}, //17541 #CJK UNIFIED IDEOGRAPH
    {0xF86E, 0x9DE2}, //17542 #CJK UNIFIED IDEOGRAPH
    {0xF86F, 0x9DEC}, //17543 #CJK UNIFIED IDEOGRAPH
    {0xF870, 0x9DF4}, //17544 #CJK UNIFIED IDEOGRAPH
    {0xF871, 0x9DF3}, //17545 #CJK UNIFIED IDEOGRAPH
    {0xF872, 0x9DE8}, //17546 #CJK UNIFIED IDEOGRAPH
    {0xF873, 0x9DED}, //17547 #CJK UNIFIED IDEOGRAPH
    {0xF874, 0x9EC2}, //17548 #CJK UNIFIED IDEOGRAPH
    {0xF875, 0x9ED0}, //17549 #CJK UNIFIED IDEOGRAPH
    {0xF876, 0x9EF2}, //17550 #CJK UNIFIED IDEOGRAPH
    {0xF877, 0x9EF3}, //17551 #CJK UNIFIED IDEOGRAPH
    {0xF878, 0x9F06}, //17552 #CJK UNIFIED IDEOGRAPH
    {0xF879, 0x9F1C}, //17553 #CJK UNIFIED IDEOGRAPH
    {0xF87A, 0x9F38}, //17554 #CJK UNIFIED IDEOGRAPH
    {0xF87B, 0x9F37}, //17555 #CJK UNIFIED IDEOGRAPH
    {0xF87C, 0x9F36}, //17556 #CJK UNIFIED IDEOGRAPH
    {0xF87D, 0x9F43}, //17557 #CJK UNIFIED IDEOGRAPH
    {0xF87E, 0x9F4F}, //17558 #CJK UNIFIED IDEOGRAPH
    {0xF8A1, 0x9F71}, //17559 #CJK UNIFIED IDEOGRAPH
    {0xF8A2, 0x9F70}, //17560 #CJK UNIFIED IDEOGRAPH
    {0xF8A3, 0x9F6E}, //17561 #CJK UNIFIED IDEOGRAPH
    {0xF8A4, 0x9F6F}, //17562 #CJK UNIFIED IDEOGRAPH
    {0xF8A5, 0x56D3}, //17563 #CJK UNIFIED IDEOGRAPH
    {0xF8A6, 0x56CD}, //17564 #CJK UNIFIED IDEOGRAPH
    {0xF8A7, 0x5B4E}, //17565 #CJK UNIFIED IDEOGRAPH
    {0xF8A8, 0x5C6D}, //17566 #CJK UNIFIED IDEOGRAPH
    {0xF8A9, 0x652D}, //17567 #CJK UNIFIED IDEOGRAPH
    {0xF8AA, 0x66ED}, //17568 #CJK UNIFIED IDEOGRAPH
    {0xF8AB, 0x66EE}, //17569 #CJK UNIFIED IDEOGRAPH
    {0xF8AC, 0x6B13}, //17570 #CJK UNIFIED IDEOGRAPH
    {0xF8AD, 0x705F}, //17571 #CJK UNIFIED IDEOGRAPH
    {0xF8AE, 0x7061}, //17572 #CJK UNIFIED IDEOGRAPH
    {0xF8AF, 0x705D}, //17573 #CJK UNIFIED IDEOGRAPH
    {0xF8B0, 0x7060}, //17574 #CJK UNIFIED IDEOGRAPH
    {0xF8B1, 0x7223}, //17575 #CJK UNIFIED IDEOGRAPH
    {0xF8B2, 0x74DB}, //17576 #CJK UNIFIED IDEOGRAPH
    {0xF8B3, 0x74E5}, //17577 #CJK UNIFIED IDEOGRAPH
    {0xF8B4, 0x77D5}, //17578 #CJK UNIFIED IDEOGRAPH
    {0xF8B5, 0x7938}, //17579 #CJK UNIFIED IDEOGRAPH
    {0xF8B6, 0x79B7}, //17580 #CJK UNIFIED IDEOGRAPH
    {0xF8B7, 0x79B6}, //17581 #CJK UNIFIED IDEOGRAPH
    {0xF8B8, 0x7C6A}, //17582 #CJK UNIFIED IDEOGRAPH
    {0xF8B9, 0x7E97}, //17583 #CJK UNIFIED IDEOGRAPH
    {0xF8BA, 0x7F89}, //17584 #CJK UNIFIED IDEOGRAPH
    {0xF8BB, 0x826D}, //17585 #CJK UNIFIED IDEOGRAPH
    {0xF8BC, 0x8643}, //17586 #CJK UNIFIED IDEOGRAPH
    {0xF8BD, 0x8838}, //17587 #CJK UNIFIED IDEOGRAPH
    {0xF8BE, 0x8837}, //17588 #CJK UNIFIED IDEOGRAPH
    {0xF8BF, 0x8835}, //17589 #CJK UNIFIED IDEOGRAPH
    {0xF8C0, 0x884B}, //17590 #CJK UNIFIED IDEOGRAPH
    {0xF8C1, 0x8B94}, //17591 #CJK UNIFIED IDEOGRAPH
    {0xF8C2, 0x8B95}, //17592 #CJK UNIFIED IDEOGRAPH
    {0xF8C3, 0x8E9E}, //17593 #CJK UNIFIED IDEOGRAPH
    {0xF8C4, 0x8E9F}, //17594 #CJK UNIFIED IDEOGRAPH
    {0xF8C5, 0x8EA0}, //17595 #CJK UNIFIED IDEOGRAPH
    {0xF8C6, 0x8E9D}, //17596 #CJK UNIFIED IDEOGRAPH
    {0xF8C7, 0x91BE}, //17597 #CJK UNIFIED IDEOGRAPH
    {0xF8C8, 0x91BD}, //17598 #CJK UNIFIED IDEOGRAPH
    {0xF8C9, 0x91C2}, //17599 #CJK UNIFIED IDEOGRAPH
    {0xF8CA, 0x946B}, //17600 #CJK UNIFIED IDEOGRAPH
    {0xF8CB, 0x9468}, //17601 #CJK UNIFIED IDEOGRAPH
    {0xF8CC, 0x9469}, //17602 #CJK UNIFIED IDEOGRAPH
    {0xF8CD, 0x96E5}, //17603 #CJK UNIFIED IDEOGRAPH
    {0xF8CE, 0x9746}, //17604 #CJK UNIFIED IDEOGRAPH
    {0xF8CF, 0x9743}, //17605 #CJK UNIFIED IDEOGRAPH
    {0xF8D0, 0x9747}, //17606 #CJK UNIFIED IDEOGRAPH
    {0xF8D1, 0x97C7}, //17607 #CJK UNIFIED IDEOGRAPH
    {0xF8D2, 0x97E5}, //17608 #CJK UNIFIED IDEOGRAPH
    {0xF8D3, 0x9A5E}, //17609 #CJK UNIFIED IDEOGRAPH
    {0xF8D4, 0x9AD5}, //17610 #CJK UNIFIED IDEOGRAPH
    {0xF8D5, 0x9B59}, //17611 #CJK UNIFIED IDEOGRAPH
    {0xF8D6, 0x9C63}, //17612 #CJK UNIFIED IDEOGRAPH
    {0xF8D7, 0x9C67}, //17613 #CJK UNIFIED IDEOGRAPH
    {0xF8D8, 0x9C66}, //17614 #CJK UNIFIED IDEOGRAPH
    {0xF8D9, 0x9C62}, //17615 #CJK UNIFIED IDEOGRAPH
    {0xF8DA, 0x9C5E}, //17616 #CJK UNIFIED IDEOGRAPH
    {0xF8DB, 0x9C60}, //17617 #CJK UNIFIED IDEOGRAPH
    {0xF8DC, 0x9E02}, //17618 #CJK UNIFIED IDEOGRAPH
    {0xF8DD, 0x9DFE}, //17619 #CJK UNIFIED IDEOGRAPH
    {0xF8DE, 0x9E07}, //17620 #CJK UNIFIED IDEOGRAPH
    {0xF8DF, 0x9E03}, //17621 #CJK UNIFIED IDEOGRAPH
    {0xF8E0, 0x9E06}, //17622 #CJK UNIFIED IDEOGRAPH
    {0xF8E1, 0x9E05}, //17623 #CJK UNIFIED IDEOGRAPH
    {0xF8E2, 0x9E00}, //17624 #CJK UNIFIED IDEOGRAPH
    {0xF8E3, 0x9E01}, //17625 #CJK UNIFIED IDEOGRAPH
    {0xF8E4, 0x9E09}, //17626 #CJK UNIFIED IDEOGRAPH
    {0xF8E5, 0x9DFF}, //17627 #CJK UNIFIED IDEOGRAPH
    {0xF8E6, 0x9DFD}, //17628 #CJK UNIFIED IDEOGRAPH
    {0xF8E7, 0x9E04}, //17629 #CJK UNIFIED IDEOGRAPH
    {0xF8E8, 0x9EA0}, //17630 #CJK UNIFIED IDEOGRAPH
    {0xF8E9, 0x9F1E}, //17631 #CJK UNIFIED IDEOGRAPH
    {0xF8EA, 0x9F46}, //17632 #CJK UNIFIED IDEOGRAPH
    {0xF8EB, 0x9F74}, //17633 #CJK UNIFIED IDEOGRAPH
    {0xF8EC, 0x9F75}, //17634 #CJK UNIFIED IDEOGRAPH
    {0xF8ED, 0x9F76}, //17635 #CJK UNIFIED IDEOGRAPH
    {0xF8EE, 0x56D4}, //17636 #CJK UNIFIED IDEOGRAPH
    {0xF8EF, 0x652E}, //17637 #CJK UNIFIED IDEOGRAPH
    {0xF8F0, 0x65B8}, //17638 #CJK UNIFIED IDEOGRAPH
    {0xF8F1, 0x6B18}, //17639 #CJK UNIFIED IDEOGRAPH
    {0xF8F2, 0x6B19}, //17640 #CJK UNIFIED IDEOGRAPH
    {0xF8F3, 0x6B17}, //17641 #CJK UNIFIED IDEOGRAPH
    {0xF8F4, 0x6B1A}, //17642 #CJK UNIFIED IDEOGRAPH
    {0xF8F5, 0x7062}, //17643 #CJK UNIFIED IDEOGRAPH
    {0xF8F6, 0x7226}, //17644 #CJK UNIFIED IDEOGRAPH
    {0xF8F7, 0x72AA}, //17645 #CJK UNIFIED IDEOGRAPH
    {0xF8F8, 0x77D8}, //17646 #CJK UNIFIED IDEOGRAPH
    {0xF8F9, 0x77D9}, //17647 #CJK UNIFIED IDEOGRAPH
    {0xF8FA, 0x7939}, //17648 #CJK UNIFIED IDEOGRAPH
    {0xF8FB, 0x7C69}, //17649 #CJK UNIFIED IDEOGRAPH
    {0xF8FC, 0x7C6B}, //17650 #CJK UNIFIED IDEOGRAPH
    {0xF8FD, 0x7CF6}, //17651 #CJK UNIFIED IDEOGRAPH
    {0xF8FE, 0x7E9A}, //17652 #CJK UNIFIED IDEOGRAPH
    {0xF940, 0x7E98}, //17653 #CJK UNIFIED IDEOGRAPH
    {0xF941, 0x7E9B}, //17654 #CJK UNIFIED IDEOGRAPH
    {0xF942, 0x7E99}, //17655 #CJK UNIFIED IDEOGRAPH
    {0xF943, 0x81E0}, //17656 #CJK UNIFIED IDEOGRAPH
    {0xF944, 0x81E1}, //17657 #CJK UNIFIED IDEOGRAPH
    {0xF945, 0x8646}, //17658 #CJK UNIFIED IDEOGRAPH
    {0xF946, 0x8647}, //17659 #CJK UNIFIED IDEOGRAPH
    {0xF947, 0x8648}, //17660 #CJK UNIFIED IDEOGRAPH
    {0xF948, 0x8979}, //17661 #CJK UNIFIED IDEOGRAPH
    {0xF949, 0x897A}, //17662 #CJK UNIFIED IDEOGRAPH
    {0xF94A, 0x897C}, //17663 #CJK UNIFIED IDEOGRAPH
    {0xF94B, 0x897B}, //17664 #CJK UNIFIED IDEOGRAPH
    {0xF94C, 0x89FF}, //17665 #CJK UNIFIED IDEOGRAPH
    {0xF94D, 0x8B98}, //17666 #CJK UNIFIED IDEOGRAPH
    {0xF94E, 0x8B99}, //17667 #CJK UNIFIED IDEOGRAPH
    {0xF94F, 0x8EA5}, //17668 #CJK UNIFIED IDEOGRAPH
    {0xF950, 0x8EA4}, //17669 #CJK UNIFIED IDEOGRAPH
    {0xF951, 0x8EA3}, //17670 #CJK UNIFIED IDEOGRAPH
    {0xF952, 0x946E}, //17671 #CJK UNIFIED IDEOGRAPH
    {0xF953, 0x946D}, //17672 #CJK UNIFIED IDEOGRAPH
    {0xF954, 0x946F}, //17673 #CJK UNIFIED IDEOGRAPH
    {0xF955, 0x9471}, //17674 #CJK UNIFIED IDEOGRAPH
    {0xF956, 0x9473}, //17675 #CJK UNIFIED IDEOGRAPH
    {0xF957, 0x9749}, //17676 #CJK UNIFIED IDEOGRAPH
    {0xF958, 0x9872}, //17677 #CJK UNIFIED IDEOGRAPH
    {0xF959, 0x995F}, //17678 #CJK UNIFIED IDEOGRAPH
    {0xF95A, 0x9C68}, //17679 #CJK UNIFIED IDEOGRAPH
    {0xF95B, 0x9C6E}, //17680 #CJK UNIFIED IDEOGRAPH
    {0xF95C, 0x9C6D}, //17681 #CJK UNIFIED IDEOGRAPH
    {0xF95D, 0x9E0B}, //17682 #CJK UNIFIED IDEOGRAPH
    {0xF95E, 0x9E0D}, //17683 #CJK UNIFIED IDEOGRAPH
    {0xF95F, 0x9E10}, //17684 #CJK UNIFIED IDEOGRAPH
    {0xF960, 0x9E0F}, //17685 #CJK UNIFIED IDEOGRAPH
    {0xF961, 0x9E12}, //17686 #CJK UNIFIED IDEOGRAPH
    {0xF962, 0x9E11}, //17687 #CJK UNIFIED IDEOGRAPH
    {0xF963, 0x9EA1}, //17688 #CJK UNIFIED IDEOGRAPH
    {0xF964, 0x9EF5}, //17689 #CJK UNIFIED IDEOGRAPH
    {0xF965, 0x9F09}, //17690 #CJK UNIFIED IDEOGRAPH
    {0xF966, 0x9F47}, //17691 #CJK UNIFIED IDEOGRAPH
    {0xF967, 0x9F78}, //17692 #CJK UNIFIED IDEOGRAPH
    {0xF968, 0x9F7B}, //17693 #CJK UNIFIED IDEOGRAPH
    {0xF969, 0x9F7A}, //17694 #CJK UNIFIED IDEOGRAPH
    {0xF96A, 0x9F79}, //17695 #CJK UNIFIED IDEOGRAPH
    {0xF96B, 0x571E}, //17696 #CJK UNIFIED IDEOGRAPH
    {0xF96C, 0x7066}, //17697 #CJK UNIFIED IDEOGRAPH
    {0xF96D, 0x7C6F}, //17698 #CJK UNIFIED IDEOGRAPH
    {0xF96E, 0x883C}, //17699 #CJK UNIFIED IDEOGRAPH
    {0xF96F, 0x8DB2}, //17700 #CJK UNIFIED IDEOGRAPH
    {0xF970, 0x8EA6}, //17701 #CJK UNIFIED IDEOGRAPH
    {0xF971, 0x91C3}, //17702 #CJK UNIFIED IDEOGRAPH
    {0xF972, 0x9474}, //17703 #CJK UNIFIED IDEOGRAPH
    {0xF973, 0x9478}, //17704 #CJK UNIFIED IDEOGRAPH
    {0xF974, 0x9476}, //17705 #CJK UNIFIED IDEOGRAPH
    {0xF975, 0x9475}, //17706 #CJK UNIFIED IDEOGRAPH
    {0xF976, 0x9A60}, //17707 #CJK UNIFIED IDEOGRAPH
    {0xF977, 0x9C74}, //17708 #CJK UNIFIED IDEOGRAPH
    {0xF978, 0x9C73}, //17709 #CJK UNIFIED IDEOGRAPH
    {0xF979, 0x9C71}, //17710 #CJK UNIFIED IDEOGRAPH
    {0xF97A, 0x9C75}, //17711 #CJK UNIFIED IDEOGRAPH
    {0xF97B, 0x9E14}, //17712 #CJK UNIFIED IDEOGRAPH
    {0xF97C, 0x9E13}, //17713 #CJK UNIFIED IDEOGRAPH
    {0xF97D, 0x9EF6}, //17714 #CJK UNIFIED IDEOGRAPH
    {0xF97E, 0x9F0A}, //17715 #CJK UNIFIED IDEOGRAPH
    {0xF9A1, 0x9FA4}, //17716 #CJK UNIFIED IDEOGRAPH
    {0xF9A2, 0x7068}, //17717 #CJK UNIFIED IDEOGRAPH
    {0xF9A3, 0x7065}, //17718 #CJK UNIFIED IDEOGRAPH
    {0xF9A4, 0x7CF7}, //17719 #CJK UNIFIED IDEOGRAPH
    {0xF9A5, 0x866A}, //17720 #CJK UNIFIED IDEOGRAPH
    {0xF9A6, 0x883E}, //17721 #CJK UNIFIED IDEOGRAPH
    {0xF9A7, 0x883D}, //17722 #CJK UNIFIED IDEOGRAPH
    {0xF9A8, 0x883F}, //17723 #CJK UNIFIED IDEOGRAPH
    {0xF9A9, 0x8B9E}, //17724 #CJK UNIFIED IDEOGRAPH
    {0xF9AA, 0x8C9C}, //17725 #CJK UNIFIED IDEOGRAPH
    {0xF9AB, 0x8EA9}, //17726 #CJK UNIFIED IDEOGRAPH
    {0xF9AC, 0x8EC9}, //17727 #CJK UNIFIED IDEOGRAPH
    {0xF9AD, 0x974B}, //17728 #CJK UNIFIED IDEOGRAPH
    {0xF9AE, 0x9873}, //17729 #CJK UNIFIED IDEOGRAPH
    {0xF9AF, 0x9874}, //17730 #CJK UNIFIED IDEOGRAPH
    {0xF9B0, 0x98CC}, //17731 #CJK UNIFIED IDEOGRAPH
    {0xF9B1, 0x9961}, //17732 #CJK UNIFIED IDEOGRAPH
    {0xF9B2, 0x99AB}, //17733 #CJK UNIFIED IDEOGRAPH
    {0xF9B3, 0x9A64}, //17734 #CJK UNIFIED IDEOGRAPH
    {0xF9B4, 0x9A66}, //17735 #CJK UNIFIED IDEOGRAPH
    {0xF9B5, 0x9A67}, //17736 #CJK UNIFIED IDEOGRAPH
    {0xF9B6, 0x9B24}, //17737 #CJK UNIFIED IDEOGRAPH
    {0xF9B7, 0x9E15}, //17738 #CJK UNIFIED IDEOGRAPH
    {0xF9B8, 0x9E17}, //17739 #CJK UNIFIED IDEOGRAPH
    {0xF9B9, 0x9F48}, //17740 #CJK UNIFIED IDEOGRAPH
    {0xF9BA, 0x6207}, //17741 #CJK UNIFIED IDEOGRAPH
    {0xF9BB, 0x6B1E}, //17742 #CJK UNIFIED IDEOGRAPH
    {0xF9BC, 0x7227}, //17743 #CJK UNIFIED IDEOGRAPH
    {0xF9BD, 0x864C}, //17744 #CJK UNIFIED IDEOGRAPH
    {0xF9BE, 0x8EA8}, //17745 #CJK UNIFIED IDEOGRAPH
    {0xF9BF, 0x9482}, //17746 #CJK UNIFIED IDEOGRAPH
    {0xF9C0, 0x9480}, //17747 #CJK UNIFIED IDEOGRAPH
    {0xF9C1, 0x9481}, //17748 #CJK UNIFIED IDEOGRAPH
    {0xF9C2, 0x9A69}, //17749 #CJK UNIFIED IDEOGRAPH
    {0xF9C3, 0x9A68}, //17750 #CJK UNIFIED IDEOGRAPH
    {0xF9C4, 0x9B2E}, //17751 #CJK UNIFIED IDEOGRAPH
    {0xF9C5, 0x9E19}, //17752 #CJK UNIFIED IDEOGRAPH
    {0xF9C6, 0x7229}, //17753 #CJK UNIFIED IDEOGRAPH
    {0xF9C7, 0x864B}, //17754 #CJK UNIFIED IDEOGRAPH
    {0xF9C8, 0x8B9F}, //17755 #CJK UNIFIED IDEOGRAPH
    {0xF9C9, 0x9483}, //17756 #CJK UNIFIED IDEOGRAPH
    {0xF9CA, 0x9C79}, //17757 #CJK UNIFIED IDEOGRAPH
    {0xF9CB, 0x9EB7}, //17758 #CJK UNIFIED IDEOGRAPH
    {0xF9CC, 0x7675}, //17759 #CJK UNIFIED IDEOGRAPH
    {0xF9CD, 0x9A6B}, //17760 #CJK UNIFIED IDEOGRAPH
    {0xF9CE, 0x9C7A}, //17761 #CJK UNIFIED IDEOGRAPH
    {0xF9CF, 0x9E1D}, //17762 #CJK UNIFIED IDEOGRAPH
    {0xF9D0, 0x7069}, //17763 #CJK UNIFIED IDEOGRAPH
    {0xF9D1, 0x706A}, //17764 #CJK UNIFIED IDEOGRAPH
    {0xF9D2, 0x9EA4}, //17765 #CJK UNIFIED IDEOGRAPH
    {0xF9D3, 0x9F7E}, //17766 #CJK UNIFIED IDEOGRAPH
    {0xF9D4, 0x9F49}, //17767 #CJK UNIFIED IDEOGRAPH
    {0xF9D5, 0x9F98}, //17768 #CJK UNIFIED IDEOGRAPH
    {0xF9D6, 0x7881}, //17769 #CJK UNIFIED IDEOGRAPH
    {0xF9D7, 0x92B9}, //17770 #CJK UNIFIED IDEOGRAPH
    {0xF9D8, 0x88CF}, //17771 #CJK UNIFIED IDEOGRAPH
    {0xF9D9, 0x58BB}, //17772 #CJK UNIFIED IDEOGRAPH
    {0xF9DA, 0x6052}, //17773 #CJK UNIFIED IDEOGRAPH
    {0xF9DB, 0x7CA7}, //17774 #CJK UNIFIED IDEOGRAPH
    {0xF9DC, 0x5AFA}, //17775 #CJK UNIFIED IDEOGRAPH
    {0xF9DD, 0x2554}, //17776 #BOX DRAWINGS DOUBLE DOWN AND RIGHT
    {0xF9DE, 0x2566}, //17777 #BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
    {0xF9DF, 0x2557}, //17778 #BOX DRAWINGS DOUBLE DOWN AND LEFT
    {0xF9E0, 0x2560}, //17779 #BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
    {0xF9E1, 0x256C}, //17780 #BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
    {0xF9E2, 0x2563}, //17781 #BOX DRAWINGS DOUBLE VERTICAL AND LEFT
    {0xF9E3, 0x255A}, //17782 #BOX DRAWINGS DOUBLE UP AND RIGHT
    {0xF9E4, 0x2569}, //17783 #BOX DRAWINGS DOUBLE UP AND HORIZONTAL
    {0xF9E5, 0x255D}, //17784 #BOX DRAWINGS DOUBLE UP AND LEFT
    {0xF9E6, 0x2552}, //17785 #BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE
    {0xF9E7, 0x2564}, //17786 #BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE
    {0xF9E8, 0x2555}, //17787 #BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE
    {0xF9E9, 0x255E}, //17788 #BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE
    {0xF9EA, 0x256A}, //17789 #BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE
    {0xF9EB, 0x2561}, //17790 #BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE
    {0xF9EC, 0x2558}, //17791 #BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE
    {0xF9ED, 0x2567}, //17792 #BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE
    {0xF9EE, 0x255B}, //17793 #BOX DRAWINGS UP SINGLE AND LEFT DOUBLE
    {0xF9EF, 0x2553}, //17794 #BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE
    {0xF9F0, 0x2565}, //17795 #BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE
    {0xF9F1, 0x2556}, //17796 #BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE
    {0xF9F2, 0x255F}, //17797 #BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE
    {0xF9F3, 0x256B}, //17798 #BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE
    {0xF9F4, 0x2562}, //17799 #BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE
    {0xF9F5, 0x2559}, //17800 #BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE
    {0xF9F6, 0x2568}, //17801 #BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE
    {0xF9F7, 0x255C}, //17802 #BOX DRAWINGS UP DOUBLE AND LEFT SINGLE
    {0xF9F8, 0x2551}, //17803 #BOX DRAWINGS DOUBLE VERTICAL
    {0xF9F9, 0x2550}, //17804 #BOX DRAWINGS DOUBLE HORIZONTAL
    {0xF9FA, 0x256D}, //17805 #BOX DRAWINGS LIGHT ARC DOWN AND RIGHT
    {0xF9FB, 0x256E}, //17806 #BOX DRAWINGS LIGHT ARC DOWN AND LEFT
    {0xF9FC, 0x2570}, //17807 #BOX DRAWINGS LIGHT ARC UP AND RIGHT
    {0xF9FD, 0x256F}, //17808 #BOX DRAWINGS LIGHT ARC UP AND LEFT
    {0xF9FE, 0x2593}, //17809 #DARK SHADE
    {0xFA40, 0x20547}, //17810 #big5-hkscs
    {0xFA41, 0x92DB}, //17811 #big5-hkscs
    {0xFA42, 0x205DF}, //17812 #big5-hkscs
    {0xFA43, 0x23FC5}, //17813 #big5-hkscs
    {0xFA44, 0x854C}, //17814 #big5-hkscs
    {0xFA45, 0x42B5}, //17815 #big5-hkscs
    {0xFA46, 0x73EF}, //17816 #big5-hkscs
    {0xFA47, 0x51B5}, //17817 #big5-hkscs
    {0xFA48, 0x3649}, //17818 #big5-hkscs
    {0xFA49, 0x24942}, //17819 #big5-hkscs
    {0xFA4A, 0x289E4}, //17820 #big5-hkscs
    {0xFA4B, 0x9344}, //17821 #big5-hkscs
    {0xFA4C, 0x219DB}, //17822 #big5-hkscs
    {0xFA4D, 0x82EE}, //17823 #big5-hkscs
    {0xFA4E, 0x23CC8}, //17824 #big5-hkscs
    {0xFA4F, 0x783C}, //17825 #big5-hkscs
    {0xFA50, 0x6744}, //17826 #big5-hkscs
    {0xFA51, 0x62DF}, //17827 #big5-hkscs
    {0xFA52, 0x24933}, //17828 #big5-hkscs
    {0xFA53, 0x289AA}, //17829 #big5-hkscs
    {0xFA54, 0x202A0}, //17830 #big5-hkscs
    {0xFA55, 0x26BB3}, //17831 #big5-hkscs
    {0xFA56, 0x21305}, //17832 #big5-hkscs
    {0xFA57, 0x4FAB}, //17833 #big5-hkscs
    {0xFA58, 0x224ED}, //17834 #big5-hkscs
    {0xFA59, 0x5008}, //17835 #big5-hkscs
    {0xFA5A, 0x26D29}, //17836 #big5-hkscs
    {0xFA5B, 0x27A84}, //17837 #big5-hkscs
    {0xFA5C, 0x23600}, //17838 #big5-hkscs
    {0xFA5D, 0x24AB1}, //17839 #big5-hkscs
    {0xFA5E, 0x22513}, //17840 #big5-hkscs
    {0xFA5F, 0x5029}, //17841 #big5-hkscs
    {0xFA60, 0x2037E}, //17842 #big5-hkscs
    {0xFA61, 0x5FA4}, //17843 #big5-hkscs
    {0xFA62, 0x20380}, //17844 #big5-hkscs
    {0xFA63, 0x20347}, //17845 #big5-hkscs
    {0xFA64, 0x6EDB}, //17846 #big5-hkscs
    {0xFA65, 0x2041F}, //17847 #big5-hkscs
    {0xFA66, 0x507D}, //17848 #big5-hkscs
    {0xFA67, 0x5101}, //17849 #big5-hkscs
    {0xFA68, 0x347A}, //17850 #big5-hkscs
    {0xFA69, 0x510E}, //17851 #big5-hkscs
    {0xFA6A, 0x986C}, //17852 #big5-hkscs
    {0xFA6B, 0x3743}, //17853 #big5-hkscs
    {0xFA6C, 0x8416}, //17854 #big5-hkscs
    {0xFA6D, 0x249A4}, //17855 #big5-hkscs
    {0xFA6E, 0x20487}, //17856 #big5-hkscs
    {0xFA6F, 0x5160}, //17857 #big5-hkscs
    {0xFA70, 0x233B4}, //17858 #big5-hkscs
    {0xFA71, 0x516A}, //17859 #big5-hkscs
    {0xFA72, 0x20BFF}, //17860 #big5-hkscs
    {0xFA73, 0x220FC}, //17861 #big5-hkscs
    {0xFA74, 0x202E5}, //17862 #big5-hkscs
    {0xFA75, 0x22530}, //17863 #big5-hkscs
    {0xFA76, 0x2058E}, //17864 #big5-hkscs
    {0xFA77, 0x23233}, //17865 #big5-hkscs
    {0xFA78, 0x21983}, //17866 #big5-hkscs
    {0xFA79, 0x5B82}, //17867 #big5-hkscs
    {0xFA7A, 0x877D}, //17868 #big5-hkscs
    {0xFA7B, 0x205B3}, //17869 #big5-hkscs
    {0xFA7C, 0x23C99}, //17870 #big5-hkscs
    {0xFA7D, 0x51B2}, //17871 #big5-hkscs
    {0xFA7E, 0x51B8}, //17872 #big5-hkscs
    {0xFAA1, 0x9D34}, //17873 #big5-hkscs
    {0xFAA2, 0x51C9}, //17874 #big5-hkscs
    {0xFAA3, 0x51CF}, //17875 #big5-hkscs
    {0xFAA4, 0x51D1}, //17876 #big5-hkscs
    {0xFAA5, 0x3CDC}, //17877 #big5-hkscs
    {0xFAA6, 0x51D3}, //17878 #big5-hkscs
    {0xFAA7, 0x24AA6}, //17879 #big5-hkscs
    {0xFAA8, 0x51B3}, //17880 #big5-hkscs
    {0xFAA9, 0x51E2}, //17881 #big5-hkscs
    {0xFAAA, 0x5342}, //17882 #big5-hkscs
    {0xFAAB, 0x51ED}, //17883 #big5-hkscs
    {0xFAAC, 0x83CD}, //17884 #big5-hkscs
    {0xFAAD, 0x693E}, //17885 #big5-hkscs
    {0xFAAE, 0x2372D}, //17886 #big5-hkscs
    {0xFAAF, 0x5F7B}, //17887 #big5-hkscs
    {0xFAB0, 0x520B}, //17888 #big5-hkscs
    {0xFAB1, 0x5226}, //17889 #big5-hkscs
    {0xFAB2, 0x523C}, //17890 #big5-hkscs
    {0xFAB3, 0x52B5}, //17891 #big5-hkscs
    {0xFAB4, 0x5257}, //17892 #big5-hkscs
    {0xFAB5, 0x5294}, //17893 #big5-hkscs
    {0xFAB6, 0x52B9}, //17894 #big5-hkscs
    {0xFAB7, 0x52C5}, //17895 #big5-hkscs
    {0xFAB8, 0x7C15}, //17896 #big5-hkscs
    {0xFAB9, 0x8542}, //17897 #big5-hkscs
    {0xFABA, 0x52E0}, //17898 #big5-hkscs
    {0xFABB, 0x860D}, //17899 #big5-hkscs
    {0xFABC, 0x26B13}, //17900 #big5-hkscs
    {0xFABD, 0x5305}, //17901 #big5-hkscs
    {0xFABE, 0x28ADE}, //17902 #big5-hkscs
    {0xFABF, 0x5549}, //17903 #big5-hkscs
    {0xFAC0, 0x6ED9}, //17904 #big5-hkscs
    {0xFAC1, 0x23F80}, //17905 #big5-hkscs
    {0xFAC2, 0x20954}, //17906 #big5-hkscs
    {0xFAC3, 0x23FEC}, //17907 #big5-hkscs
    {0xFAC4, 0x5333}, //17908 #big5-hkscs
    {0xFAC5, 0x5344}, //17909 #big5-hkscs
    {0xFAC6, 0x20BE2}, //17910 #big5-hkscs
    {0xFAC7, 0x6CCB}, //17911 #big5-hkscs
    {0xFAC8, 0x21726}, //17912 #big5-hkscs
    {0xFAC9, 0x681B}, //17913 #big5-hkscs
    {0xFACA, 0x73D5}, //17914 #big5-hkscs
    {0xFACB, 0x604A}, //17915 #big5-hkscs
    {0xFACC, 0x3EAA}, //17916 #big5-hkscs
    {0xFACD, 0x38CC}, //17917 #big5-hkscs
    {0xFACE, 0x216E8}, //17918 #big5-hkscs
    {0xFACF, 0x71DD}, //17919 #big5-hkscs
    {0xFAD0, 0x44A2}, //17920 #big5-hkscs
    {0xFAD1, 0x536D}, //17921 #big5-hkscs
    {0xFAD2, 0x5374}, //17922 #big5-hkscs
    {0xFAD3, 0x286AB}, //17923 #big5-hkscs
    {0xFAD4, 0x537E}, //17924 #big5-hkscs
    {0xFAD5, 0x537F}, //17925 #big5-hkscs
    {0xFAD6, 0x21596}, //17926 #big5-hkscs
    {0xFAD7, 0x21613}, //17927 #big5-hkscs
    {0xFAD8, 0x77E6}, //17928 #big5-hkscs
    {0xFAD9, 0x5393}, //17929 #big5-hkscs
    {0xFADA, 0x28A9B}, //17930 #big5-hkscs
    {0xFADB, 0x53A0}, //17931 #big5-hkscs
    {0xFADC, 0x53AB}, //17932 #big5-hkscs
    {0xFADD, 0x53AE}, //17933 #big5-hkscs
    {0xFADE, 0x73A7}, //17934 #big5-hkscs
    {0xFADF, 0x25772}, //17935 #big5-hkscs
    {0xFAE0, 0x3F59}, //17936 #big5-hkscs
    {0xFAE1, 0x739C}, //17937 #big5-hkscs
    {0xFAE2, 0x53C1}, //17938 #big5-hkscs
    {0xFAE3, 0x53C5}, //17939 #big5-hkscs
    {0xFAE4, 0x6C49}, //17940 #big5-hkscs
    {0xFAE5, 0x4E49}, //17941 #big5-hkscs
    {0xFAE6, 0x57FE}, //17942 #big5-hkscs
    {0xFAE7, 0x53D9}, //17943 #big5-hkscs
    {0xFAE8, 0x3AAB}, //17944 #big5-hkscs
    {0xFAE9, 0x20B8F}, //17945 #big5-hkscs
    {0xFAEA, 0x53E0}, //17946 #big5-hkscs
    {0xFAEB, 0x23FEB}, //17947 #big5-hkscs
    {0xFAEC, 0x22DA3}, //17948 #big5-hkscs
    {0xFAED, 0x53F6}, //17949 #big5-hkscs
    {0xFAEE, 0x20C77}, //17950 #big5-hkscs
    {0xFAEF, 0x5413}, //17951 #big5-hkscs
    {0xFAF0, 0x7079}, //17952 #big5-hkscs
    {0xFAF1, 0x552B}, //17953 #big5-hkscs
    {0xFAF2, 0x6657}, //17954 #big5-hkscs
    {0xFAF3, 0x6D5B}, //17955 #big5-hkscs
    {0xFAF4, 0x546D}, //17956 #big5-hkscs
    {0xFAF5, 0x26B53}, //17957 #big5-hkscs
    {0xFAF6, 0x20D74}, //17958 #big5-hkscs
    {0xFAF7, 0x555D}, //17959 #big5-hkscs
    {0xFAF8, 0x548F}, //17960 #big5-hkscs
    {0xFAF9, 0x54A4}, //17961 #big5-hkscs
    {0xFAFA, 0x47A6}, //17962 #big5-hkscs
    {0xFAFB, 0x2170D}, //17963 #big5-hkscs
    {0xFAFC, 0x20EDD}, //17964 #big5-hkscs
    {0xFAFD, 0x3DB4}, //17965 #big5-hkscs
    {0xFAFE, 0x20D4D}, //17966 #big5-hkscs
    {0xFB40, 0x289BC}, //17967 #big5-hkscs
    {0xFB41, 0x22698}, //17968 #big5-hkscs
    {0xFB42, 0x5547}, //17969 #big5-hkscs
    {0xFB43, 0x4CED}, //17970 #big5-hkscs
    {0xFB44, 0x542F}, //17971 #big5-hkscs
    {0xFB45, 0x7417}, //17972 #big5-hkscs
    {0xFB46, 0x5586}, //17973 #big5-hkscs
    {0xFB47, 0x55A9}, //17974 #big5-hkscs
    {0xFB48, 0x5605}, //17975 #big5-hkscs
    {0xFB49, 0x218D7}, //17976 #big5-hkscs
    {0xFB4A, 0x2403A}, //17977 #big5-hkscs
    {0xFB4B, 0x4552}, //17978 #big5-hkscs
    {0xFB4C, 0x24435}, //17979 #big5-hkscs
    {0xFB4D, 0x66B3}, //17980 #big5-hkscs
    {0xFB4E, 0x210B4}, //17981 #big5-hkscs
    {0xFB4F, 0x5637}, //17982 #big5-hkscs
    {0xFB50, 0x66CD}, //17983 #big5-hkscs
    {0xFB51, 0x2328A}, //17984 #big5-hkscs
    {0xFB52, 0x66A4}, //17985 #big5-hkscs
    {0xFB53, 0x66AD}, //17986 #big5-hkscs
    {0xFB54, 0x564D}, //17987 #big5-hkscs
    {0xFB55, 0x564F}, //17988 #big5-hkscs
    {0xFB56, 0x78F1}, //17989 #big5-hkscs
    {0xFB57, 0x56F1}, //17990 #big5-hkscs
    {0xFB58, 0x9787}, //17991 #big5-hkscs
    {0xFB59, 0x53FE}, //17992 #big5-hkscs
    {0xFB5A, 0x5700}, //17993 #big5-hkscs
    {0xFB5B, 0x56EF}, //17994 #big5-hkscs
    {0xFB5C, 0x56ED}, //17995 #big5-hkscs
    {0xFB5D, 0x28B66}, //17996 #big5-hkscs
    {0xFB5E, 0x3623}, //17997 #big5-hkscs
    {0xFB5F, 0x2124F}, //17998 #big5-hkscs
    {0xFB60, 0x5746}, //17999 #big5-hkscs
    {0xFB61, 0x241A5}, //18000 #big5-hkscs
    {0xFB62, 0x6C6E}, //18001 #big5-hkscs
    {0xFB63, 0x708B}, //18002 #big5-hkscs
    {0xFB64, 0x5742}, //18003 #big5-hkscs
    {0xFB65, 0x36B1}, //18004 #big5-hkscs
    {0xFB66, 0x26C7E}, //18005 #big5-hkscs
    {0xFB67, 0x57E6}, //18006 #big5-hkscs
    {0xFB68, 0x21416}, //18007 #big5-hkscs
    {0xFB69, 0x5803}, //18008 #big5-hkscs
    {0xFB6A, 0x21454}, //18009 #big5-hkscs
    {0xFB6B, 0x24363}, //18010 #big5-hkscs
    {0xFB6C, 0x5826}, //18011 #big5-hkscs
    {0xFB6D, 0x24BF5}, //18012 #big5-hkscs
    {0xFB6E, 0x585C}, //18013 #big5-hkscs
    {0xFB6F, 0x58AA}, //18014 #big5-hkscs
    {0xFB70, 0x3561}, //18015 #big5-hkscs
    {0xFB71, 0x58E0}, //18016 #big5-hkscs
    {0xFB72, 0x58DC}, //18017 #big5-hkscs
    {0xFB73, 0x2123C}, //18018 #big5-hkscs
    {0xFB74, 0x58FB}, //18019 #big5-hkscs
    {0xFB75, 0x5BFF}, //18020 #big5-hkscs
    {0xFB76, 0x5743}, //18021 #big5-hkscs
    {0xFB77, 0x2A150}, //18022 #big5-hkscs
    {0xFB78, 0x24278}, //18023 #big5-hkscs
    {0xFB79, 0x93D3}, //18024 #big5-hkscs
    {0xFB7A, 0x35A1}, //18025 #big5-hkscs
    {0xFB7B, 0x591F}, //18026 #big5-hkscs
    {0xFB7C, 0x68A6}, //18027 #big5-hkscs
    {0xFB7D, 0x36C3}, //18028 #big5-hkscs
    {0xFB7E, 0x6E59}, //18029 #big5-hkscs
    {0xFBA1, 0x2163E}, //18030 #big5-hkscs
    {0xFBA2, 0x5A24}, //18031 #big5-hkscs
    {0xFBA3, 0x5553}, //18032 #big5-hkscs
    {0xFBA4, 0x21692}, //18033 #big5-hkscs
    {0xFBA5, 0x8505}, //18034 #big5-hkscs
    {0xFBA6, 0x59C9}, //18035 #big5-hkscs
    {0xFBA7, 0x20D4E}, //18036 #big5-hkscs
    {0xFBA8, 0x26C81}, //18037 #big5-hkscs
    {0xFBA9, 0x26D2A}, //18038 #big5-hkscs
    {0xFBAA, 0x217DC}, //18039 #big5-hkscs
    {0xFBAB, 0x59D9}, //18040 #big5-hkscs
    {0xFBAC, 0x217FB}, //18041 #big5-hkscs
    {0xFBAD, 0x217B2}, //18042 #big5-hkscs
    {0xFBAE, 0x26DA6}, //18043 #big5-hkscs
    {0xFBAF, 0x6D71}, //18044 #big5-hkscs
    {0xFBB0, 0x21828}, //18045 #big5-hkscs
    {0xFBB1, 0x216D5}, //18046 #big5-hkscs
    {0xFBB2, 0x59F9}, //18047 #big5-hkscs
    {0xFBB3, 0x26E45}, //18048 #big5-hkscs
    {0xFBB4, 0x5AAB}, //18049 #big5-hkscs
    {0xFBB5, 0x5A63}, //18050 #big5-hkscs
    {0xFBB6, 0x36E6}, //18051 #big5-hkscs
    {0xFBB7, 0x249A9}, //18052 #big5-hkscs
    {0xFBB8, 0x5A77}, //18053 #big5-hkscs
    {0xFBB9, 0x3708}, //18054 #big5-hkscs
    {0xFBBA, 0x5A96}, //18055 #big5-hkscs
    {0xFBBB, 0x7465}, //18056 #big5-hkscs
    {0xFBBC, 0x5AD3}, //18057 #big5-hkscs
    {0xFBBD, 0x26FA1}, //18058 #big5-hkscs
    {0xFBBE, 0x22554}, //18059 #big5-hkscs
    {0xFBBF, 0x3D85}, //18060 #big5-hkscs
    {0xFBC0, 0x21911}, //18061 #big5-hkscs
    {0xFBC1, 0x3732}, //18062 #big5-hkscs
    {0xFBC2, 0x216B8}, //18063 #big5-hkscs
    {0xFBC3, 0x5E83}, //18064 #big5-hkscs
    {0xFBC4, 0x52D0}, //18065 #big5-hkscs
    {0xFBC5, 0x5B76}, //18066 #big5-hkscs
    {0xFBC6, 0x6588}, //18067 #big5-hkscs
    {0xFBC7, 0x5B7C}, //18068 #big5-hkscs
    {0xFBC8, 0x27A0E}, //18069 #big5-hkscs
    {0xFBC9, 0x4004}, //18070 #big5-hkscs
    {0xFBCA, 0x485D}, //18071 #big5-hkscs
    {0xFBCB, 0x20204}, //18072 #big5-hkscs
    {0xFBCC, 0x5BD5}, //18073 #big5-hkscs
    {0xFBCD, 0x6160}, //18074 #big5-hkscs
    {0xFBCE, 0x21A34}, //18075 #big5-hkscs
    {0xFBCF, 0x259CC}, //18076 #big5-hkscs
    {0xFBD0, 0x205A5}, //18077 #big5-hkscs
    {0xFBD1, 0x5BF3}, //18078 #big5-hkscs
    {0xFBD2, 0x5B9D}, //18079 #big5-hkscs
    {0xFBD3, 0x4D10}, //18080 #big5-hkscs
    {0xFBD4, 0x5C05}, //18081 #big5-hkscs
    {0xFBD5, 0x21B44}, //18082 #big5-hkscs
    {0xFBD6, 0x5C13}, //18083 #big5-hkscs
    {0xFBD7, 0x73CE}, //18084 #big5-hkscs
    {0xFBD8, 0x5C14}, //18085 #big5-hkscs
    {0xFBD9, 0x21CA5}, //18086 #big5-hkscs
    {0xFBDA, 0x26B28}, //18087 #big5-hkscs
    {0xFBDB, 0x5C49}, //18088 #big5-hkscs
    {0xFBDC, 0x48DD}, //18089 #big5-hkscs
    {0xFBDD, 0x5C85}, //18090 #big5-hkscs
    {0xFBDE, 0x5CE9}, //18091 #big5-hkscs
    {0xFBDF, 0x5CEF}, //18092 #big5-hkscs
    {0xFBE0, 0x5D8B}, //18093 #big5-hkscs
    {0xFBE1, 0x21DF9}, //18094 #big5-hkscs
    {0xFBE2, 0x21E37}, //18095 #big5-hkscs
    {0xFBE3, 0x5D10}, //18096 #big5-hkscs
    {0xFBE4, 0x5D18}, //18097 #big5-hkscs
    {0xFBE5, 0x5D46}, //18098 #big5-hkscs
    {0xFBE6, 0x21EA4}, //18099 #big5-hkscs
    {0xFBE7, 0x5CBA}, //18100 #big5-hkscs
    {0xFBE8, 0x5DD7}, //18101 #big5-hkscs
    {0xFBE9, 0x82FC}, //18102 #big5-hkscs
    {0xFBEA, 0x382D}, //18103 #big5-hkscs
    {0xFBEB, 0x24901}, //18104 #big5-hkscs
    {0xFBEC, 0x22049}, //18105 #big5-hkscs
    {0xFBED, 0x22173}, //18106 #big5-hkscs
    {0xFBEE, 0x8287}, //18107 #big5-hkscs
    {0xFBEF, 0x3836}, //18108 #big5-hkscs
    {0xFBF0, 0x3BC2}, //18109 #big5-hkscs
    {0xFBF1, 0x5E2E}, //18110 #big5-hkscs
    {0xFBF2, 0x6A8A}, //18111 #big5-hkscs
    {0xFBF3, 0x5E75}, //18112 #big5-hkscs
    {0xFBF4, 0x5E7A}, //18113 #big5-hkscs
    {0xFBF5, 0x244BC}, //18114 #big5-hkscs
    {0xFBF6, 0x20CD3}, //18115 #big5-hkscs
    {0xFBF7, 0x53A6}, //18116 #big5-hkscs
    {0xFBF8, 0x4EB7}, //18117 #big5-hkscs
    {0xFBF9, 0x5ED0}, //18118 #big5-hkscs
    {0xFBFA, 0x53A8}, //18119 #big5-hkscs
    {0xFBFB, 0x21771}, //18120 #big5-hkscs
    {0xFBFC, 0x5E09}, //18121 #big5-hkscs
    {0xFBFD, 0x5EF4}, //18122 #big5-hkscs
    {0xFBFE, 0x28482}, //18123 #big5-hkscs
    {0xFC40, 0x5EF9}, //18124 #big5-hkscs
    {0xFC41, 0x5EFB}, //18125 #big5-hkscs
    {0xFC42, 0x38A0}, //18126 #big5-hkscs
    {0xFC43, 0x5EFC}, //18127 #big5-hkscs
    {0xFC44, 0x683E}, //18128 #big5-hkscs
    {0xFC45, 0x941B}, //18129 #big5-hkscs
    {0xFC46, 0x5F0D}, //18130 #big5-hkscs
    {0xFC47, 0x201C1}, //18131 #big5-hkscs
    {0xFC48, 0x2F894}, //18132 #big5-hkscs
    {0xFC49, 0x3ADE}, //18133 #big5-hkscs
    {0xFC4A, 0x48AE}, //18134 #big5-hkscs
    {0xFC4B, 0x2133A}, //18135 #big5-hkscs
    {0xFC4C, 0x5F3A}, //18136 #big5-hkscs
    {0xFC4D, 0x26888}, //18137 #big5-hkscs
    {0xFC4E, 0x223D0}, //18138 #big5-hkscs
    {0xFC4F, 0x5F58}, //18139 #big5-hkscs
    {0xFC50, 0x22471}, //18140 #big5-hkscs
    {0xFC51, 0x5F63}, //18141 #big5-hkscs
    {0xFC52, 0x97BD}, //18142 #big5-hkscs
    {0xFC53, 0x26E6E}, //18143 #big5-hkscs
    {0xFC54, 0x5F72}, //18144 #big5-hkscs
    {0xFC55, 0x9340}, //18145 #big5-hkscs
    {0xFC56, 0x28A36}, //18146 #big5-hkscs
    {0xFC57, 0x5FA7}, //18147 #big5-hkscs
    {0xFC58, 0x5DB6}, //18148 #big5-hkscs
    {0xFC59, 0x3D5F}, //18149 #big5-hkscs
    {0xFC5A, 0x25250}, //18150 #big5-hkscs
    {0xFC5B, 0x21F6A}, //18151 #big5-hkscs
    {0xFC5C, 0x270F8}, //18152 #big5-hkscs
    {0xFC5D, 0x22668}, //18153 #big5-hkscs
    {0xFC5E, 0x91D6}, //18154 #big5-hkscs
    {0xFC5F, 0x2029E}, //18155 #big5-hkscs
    {0xFC60, 0x28A29}, //18156 #big5-hkscs
    {0xFC61, 0x6031}, //18157 #big5-hkscs
    {0xFC62, 0x6685}, //18158 #big5-hkscs
    {0xFC63, 0x21877}, //18159 #big5-hkscs
    {0xFC64, 0x3963}, //18160 #big5-hkscs
    {0xFC65, 0x3DC7}, //18161 #big5-hkscs
    {0xFC66, 0x3639}, //18162 #big5-hkscs
    {0xFC67, 0x5790}, //18163 #big5-hkscs
    {0xFC68, 0x227B4}, //18164 #big5-hkscs
    {0xFC69, 0x7971}, //18165 #big5-hkscs
    {0xFC6A, 0x3E40}, //18166 #big5-hkscs
    {0xFC6B, 0x609E}, //18167 #big5-hkscs
    {0xFC6C, 0x60A4}, //18168 #big5-hkscs
    {0xFC6D, 0x60B3}, //18169 #big5-hkscs
    {0xFC6E, 0x24982}, //18170 #big5-hkscs
    {0xFC6F, 0x2498F}, //18171 #big5-hkscs
    {0xFC70, 0x27A53}, //18172 #big5-hkscs
    {0xFC71, 0x74A4}, //18173 #big5-hkscs
    {0xFC72, 0x50E1}, //18174 #big5-hkscs
    {0xFC73, 0x5AA0}, //18175 #big5-hkscs
    {0xFC74, 0x6164}, //18176 #big5-hkscs
    {0xFC75, 0x8424}, //18177 #big5-hkscs
    {0xFC76, 0x6142}, //18178 #big5-hkscs
    {0xFC77, 0x2F8A6}, //18179 #big5-hkscs
    {0xFC78, 0x26ED2}, //18180 #big5-hkscs
    {0xFC79, 0x6181}, //18181 #big5-hkscs
    {0xFC7A, 0x51F4}, //18182 #big5-hkscs
    {0xFC7B, 0x20656}, //18183 #big5-hkscs
    {0xFC7C, 0x6187}, //18184 #big5-hkscs
    {0xFC7D, 0x5BAA}, //18185 #big5-hkscs
    {0xFC7E, 0x23FB7}, //18186 #big5-hkscs
    {0xFCA1, 0x2285F}, //18187 #big5-hkscs
    {0xFCA2, 0x61D3}, //18188 #big5-hkscs
    {0xFCA3, 0x28B9D}, //18189 #big5-hkscs
    {0xFCA4, 0x2995D}, //18190 #big5-hkscs
    {0xFCA5, 0x61D0}, //18191 #big5-hkscs
    {0xFCA6, 0x3932}, //18192 #big5-hkscs
    {0xFCA7, 0x22980}, //18193 #big5-hkscs
    {0xFCA8, 0x228C1}, //18194 #big5-hkscs
    {0xFCA9, 0x6023}, //18195 #big5-hkscs
    {0xFCAA, 0x615C}, //18196 #big5-hkscs
    {0xFCAB, 0x651E}, //18197 #big5-hkscs
    {0xFCAC, 0x638B}, //18198 #big5-hkscs
    {0xFCAD, 0x20118}, //18199 #big5-hkscs
    {0xFCAE, 0x62C5}, //18200 #big5-hkscs
    {0xFCAF, 0x21770}, //18201 #big5-hkscs
    {0xFCB0, 0x62D5}, //18202 #big5-hkscs
    {0xFCB1, 0x22E0D}, //18203 #big5-hkscs
    {0xFCB2, 0x636C}, //18204 #big5-hkscs
    {0xFCB3, 0x249DF}, //18205 #big5-hkscs
    {0xFCB4, 0x3A17}, //18206 #big5-hkscs
    {0xFCB5, 0x6438}, //18207 #big5-hkscs
    {0xFCB6, 0x63F8}, //18208 #big5-hkscs
    {0xFCB7, 0x2138E}, //18209 #big5-hkscs
    {0xFCB8, 0x217FC}, //18210 #big5-hkscs
    {0xFCB9, 0x6490}, //18211 #big5-hkscs
    {0xFCBA, 0x6F8A}, //18212 #big5-hkscs
    {0xFCBB, 0x22E36}, //18213 #big5-hkscs
    {0xFCBC, 0x9814}, //18214 #big5-hkscs
    {0xFCBD, 0x2408C}, //18215 #big5-hkscs
    {0xFCBE, 0x2571D}, //18216 #big5-hkscs
    {0xFCBF, 0x64E1}, //18217 #big5-hkscs
    {0xFCC0, 0x64E5}, //18218 #big5-hkscs
    {0xFCC1, 0x947B}, //18219 #big5-hkscs
    {0xFCC2, 0x3A66}, //18220 #big5-hkscs
    {0xFCC3, 0x643A}, //18221 #big5-hkscs
    {0xFCC4, 0x3A57}, //18222 #big5-hkscs
    {0xFCC5, 0x654D}, //18223 #big5-hkscs
    {0xFCC6, 0x6F16}, //18224 #big5-hkscs
    {0xFCC7, 0x24A28}, //18225 #big5-hkscs
    {0xFCC8, 0x24A23}, //18226 #big5-hkscs
    {0xFCC9, 0x6585}, //18227 #big5-hkscs
    {0xFCCA, 0x656D}, //18228 #big5-hkscs
    {0xFCCB, 0x655F}, //18229 #big5-hkscs
    {0xFCCC, 0x2307E}, //18230 #big5-hkscs
    {0xFCCD, 0x65B5}, //18231 #big5-hkscs
    {0xFCCE, 0x24940}, //18232 #big5-hkscs
    {0xFCCF, 0x4B37}, //18233 #big5-hkscs
    {0xFCD0, 0x65D1}, //18234 #big5-hkscs
    {0xFCD1, 0x40D8}, //18235 #big5-hkscs
    {0xFCD2, 0x21829}, //18236 #big5-hkscs
    {0xFCD3, 0x65E0}, //18237 #big5-hkscs
    {0xFCD4, 0x65E3}, //18238 #big5-hkscs
    {0xFCD5, 0x5FDF}, //18239 #big5-hkscs
    {0xFCD6, 0x23400}, //18240 #big5-hkscs
    {0xFCD7, 0x6618}, //18241 #big5-hkscs
    {0xFCD8, 0x231F7}, //18242 #big5-hkscs
    {0xFCD9, 0x231F8}, //18243 #big5-hkscs
    {0xFCDA, 0x6644}, //18244 #big5-hkscs
    {0xFCDB, 0x231A4}, //18245 #big5-hkscs
    {0xFCDC, 0x231A5}, //18246 #big5-hkscs
    {0xFCDD, 0x664B}, //18247 #big5-hkscs
    {0xFCDE, 0x20E75}, //18248 #big5-hkscs
    {0xFCDF, 0x6667}, //18249 #big5-hkscs
    {0xFCE0, 0x251E6}, //18250 #big5-hkscs
    {0xFCE1, 0x6673}, //18251 #big5-hkscs
    {0xFCE2, 0x6674}, //18252 #big5-hkscs
    {0xFCE3, 0x21E3D}, //18253 #big5-hkscs
    {0xFCE4, 0x23231}, //18254 #big5-hkscs
    {0xFCE5, 0x285F4}, //18255 #big5-hkscs
    {0xFCE6, 0x231C8}, //18256 #big5-hkscs
    {0xFCE7, 0x25313}, //18257 #big5-hkscs
    {0xFCE8, 0x77C5}, //18258 #big5-hkscs
    {0xFCE9, 0x228F7}, //18259 #big5-hkscs
    {0xFCEA, 0x99A4}, //18260 #big5-hkscs
    {0xFCEB, 0x6702}, //18261 #big5-hkscs
    {0xFCEC, 0x2439C}, //18262 #big5-hkscs
    {0xFCED, 0x24A21}, //18263 #big5-hkscs
    {0xFCEE, 0x3B2B}, //18264 #big5-hkscs
    {0xFCEF, 0x69FA}, //18265 #big5-hkscs
    {0xFCF0, 0x237C2}, //18266 #big5-hkscs
    {0xFCF1, 0x675E}, //18267 #big5-hkscs
    {0xFCF2, 0x6767}, //18268 #big5-hkscs
    {0xFCF3, 0x6762}, //18269 #big5-hkscs
    {0xFCF4, 0x241CD}, //18270 #big5-hkscs
    {0xFCF5, 0x290ED}, //18271 #big5-hkscs
    {0xFCF6, 0x67D7}, //18272 #big5-hkscs
    {0xFCF7, 0x44E9}, //18273 #big5-hkscs
    {0xFCF8, 0x6822}, //18274 #big5-hkscs
    {0xFCF9, 0x6E50}, //18275 #big5-hkscs
    {0xFCFA, 0x923C}, //18276 #big5-hkscs
    {0xFCFB, 0x6801}, //18277 #big5-hkscs
    {0xFCFC, 0x233E6}, //18278 #big5-hkscs
    {0xFCFD, 0x26DA0}, //18279 #big5-hkscs
    {0xFCFE, 0x685D}, //18280 #big5-hkscs
    {0xFD40, 0x2346F}, //18281 #big5-hkscs
    {0xFD41, 0x69E1}, //18282 #big5-hkscs
    {0xFD42, 0x6A0B}, //18283 #big5-hkscs
    {0xFD43, 0x28ADF}, //18284 #big5-hkscs
    {0xFD44, 0x6973}, //18285 #big5-hkscs
    {0xFD45, 0x68C3}, //18286 #big5-hkscs
    {0xFD46, 0x235CD}, //18287 #big5-hkscs
    {0xFD47, 0x6901}, //18288 #big5-hkscs
    {0xFD48, 0x6900}, //18289 #big5-hkscs
    {0xFD49, 0x3D32}, //18290 #big5-hkscs
    {0xFD4A, 0x3A01}, //18291 #big5-hkscs
    {0xFD4B, 0x2363C}, //18292 #big5-hkscs
    {0xFD4C, 0x3B80}, //18293 #big5-hkscs
    {0xFD4D, 0x67AC}, //18294 #big5-hkscs
    {0xFD4E, 0x6961}, //18295 #big5-hkscs
    {0xFD4F, 0x28A4A}, //18296 #big5-hkscs
    {0xFD50, 0x42FC}, //18297 #big5-hkscs
    {0xFD51, 0x6936}, //18298 #big5-hkscs
    {0xFD52, 0x6998}, //18299 #big5-hkscs
    {0xFD53, 0x3BA1}, //18300 #big5-hkscs
    {0xFD54, 0x203C9}, //18301 #big5-hkscs
    {0xFD55, 0x8363}, //18302 #big5-hkscs
    {0xFD56, 0x5090}, //18303 #big5-hkscs
    {0xFD57, 0x69F9}, //18304 #big5-hkscs
    {0xFD58, 0x23659}, //18305 #big5-hkscs
    {0xFD59, 0x2212A}, //18306 #big5-hkscs
    {0xFD5A, 0x6A45}, //18307 #big5-hkscs
    {0xFD5B, 0x23703}, //18308 #big5-hkscs
    {0xFD5C, 0x6A9D}, //18309 #big5-hkscs
    {0xFD5D, 0x3BF3}, //18310 #big5-hkscs
    {0xFD5E, 0x67B1}, //18311 #big5-hkscs
    {0xFD5F, 0x6AC8}, //18312 #big5-hkscs
    {0xFD60, 0x2919C}, //18313 #big5-hkscs
    {0xFD61, 0x3C0D}, //18314 #big5-hkscs
    {0xFD62, 0x6B1D}, //18315 #big5-hkscs
    {0xFD63, 0x20923}, //18316 #big5-hkscs
    {0xFD64, 0x60DE}, //18317 #big5-hkscs
    {0xFD65, 0x6B35}, //18318 #big5-hkscs
    {0xFD66, 0x6B74}, //18319 #big5-hkscs
    {0xFD67, 0x227CD}, //18320 #big5-hkscs
    {0xFD68, 0x6EB5}, //18321 #big5-hkscs
    {0xFD69, 0x23ADB}, //18322 #big5-hkscs
    {0xFD6A, 0x203B5}, //18323 #big5-hkscs
    {0xFD6B, 0x21958}, //18324 #big5-hkscs
    {0xFD6C, 0x3740}, //18325 #big5-hkscs
    {0xFD6D, 0x5421}, //18326 #big5-hkscs
    {0xFD6E, 0x23B5A}, //18327 #big5-hkscs
    {0xFD6F, 0x6BE1}, //18328 #big5-hkscs
    {0xFD70, 0x23EFC}, //18329 #big5-hkscs
    {0xFD71, 0x6BDC}, //18330 #big5-hkscs
    {0xFD72, 0x6C37}, //18331 #big5-hkscs
    {0xFD73, 0x2248B}, //18332 #big5-hkscs
    {0xFD74, 0x248F1}, //18333 #big5-hkscs
    {0xFD75, 0x26B51}, //18334 #big5-hkscs
    {0xFD76, 0x6C5A}, //18335 #big5-hkscs
    {0xFD77, 0x8226}, //18336 #big5-hkscs
    {0xFD78, 0x6C79}, //18337 #big5-hkscs
    {0xFD79, 0x23DBC}, //18338 #big5-hkscs
    {0xFD7A, 0x44C5}, //18339 #big5-hkscs
    {0xFD7B, 0x23DBD}, //18340 #big5-hkscs
    {0xFD7C, 0x241A4}, //18341 #big5-hkscs
    {0xFD7D, 0x2490C}, //18342 #big5-hkscs
    {0xFD7E, 0x24900}, //18343 #big5-hkscs
    {0xFDA1, 0x23CC9}, //18344 #big5-hkscs
    {0xFDA2, 0x36E5}, //18345 #big5-hkscs
    {0xFDA3, 0x3CEB}, //18346 #big5-hkscs
    {0xFDA4, 0x20D32}, //18347 #big5-hkscs
    {0xFDA5, 0x9B83}, //18348 #big5-hkscs
    {0xFDA6, 0x231F9}, //18349 #big5-hkscs
    {0xFDA7, 0x22491}, //18350 #big5-hkscs
    {0xFDA8, 0x7F8F}, //18351 #big5-hkscs
    {0xFDA9, 0x6837}, //18352 #big5-hkscs
    {0xFDAA, 0x26D25}, //18353 #big5-hkscs
    {0xFDAB, 0x26DA1}, //18354 #big5-hkscs
    {0xFDAC, 0x26DEB}, //18355 #big5-hkscs
    {0xFDAD, 0x6D96}, //18356 #big5-hkscs
    {0xFDAE, 0x6D5C}, //18357 #big5-hkscs
    {0xFDAF, 0x6E7C}, //18358 #big5-hkscs
    {0xFDB0, 0x6F04}, //18359 #big5-hkscs
    {0xFDB1, 0x2497F}, //18360 #big5-hkscs
    {0xFDB2, 0x24085}, //18361 #big5-hkscs
    {0xFDB3, 0x26E72}, //18362 #big5-hkscs
    {0xFDB4, 0x8533}, //18363 #big5-hkscs
    {0xFDB5, 0x26F74}, //18364 #big5-hkscs
    {0xFDB6, 0x51C7}, //18365 #big5-hkscs
    {0xFDB7, 0x6C9C}, //18366 #big5-hkscs
    {0xFDB8, 0x6E1D}, //18367 #big5-hkscs
    {0xFDB9, 0x842E}, //18368 #big5-hkscs
    {0xFDBA, 0x28B21}, //18369 #big5-hkscs
    {0xFDBB, 0x6E2F}, //18370 #big5-hkscs
    {0xFDBC, 0x23E2F}, //18371 #big5-hkscs
    {0xFDBD, 0x7453}, //18372 #big5-hkscs
    {0xFDBE, 0x23F82}, //18373 #big5-hkscs
    {0xFDBF, 0x79CC}, //18374 #big5-hkscs
    {0xFDC0, 0x6E4F}, //18375 #big5-hkscs
    {0xFDC1, 0x5A91}, //18376 #big5-hkscs
    {0xFDC2, 0x2304B}, //18377 #big5-hkscs
    {0xFDC3, 0x6FF8}, //18378 #big5-hkscs
    {0xFDC4, 0x370D}, //18379 #big5-hkscs
    {0xFDC5, 0x6F9D}, //18380 #big5-hkscs
    {0xFDC6, 0x23E30}, //18381 #big5-hkscs
    {0xFDC7, 0x6EFA}, //18382 #big5-hkscs
    {0xFDC8, 0x21497}, //18383 #big5-hkscs
    {0xFDC9, 0x2403D}, //18384 #big5-hkscs
    {0xFDCA, 0x4555}, //18385 #big5-hkscs
    {0xFDCB, 0x93F0}, //18386 #big5-hkscs
    {0xFDCC, 0x6F44}, //18387 #big5-hkscs
    {0xFDCD, 0x6F5C}, //18388 #big5-hkscs
    {0xFDCE, 0x3D4E}, //18389 #big5-hkscs
    {0xFDCF, 0x6F74}, //18390 #big5-hkscs
    {0xFDD0, 0x29170}, //18391 #big5-hkscs
    {0xFDD1, 0x3D3B}, //18392 #big5-hkscs
    {0xFDD2, 0x6F9F}, //18393 #big5-hkscs
    {0xFDD3, 0x24144}, //18394 #big5-hkscs
    {0xFDD4, 0x6FD3}, //18395 #big5-hkscs
    {0xFDD5, 0x24091}, //18396 #big5-hkscs
    {0xFDD6, 0x24155}, //18397 #big5-hkscs
    {0xFDD7, 0x24039}, //18398 #big5-hkscs
    {0xFDD8, 0x23FF0}, //18399 #big5-hkscs
    {0xFDD9, 0x23FB4}, //18400 #big5-hkscs
    {0xFDDA, 0x2413F}, //18401 #big5-hkscs
    {0xFDDB, 0x51DF}, //18402 #big5-hkscs
    {0xFDDC, 0x24156}, //18403 #big5-hkscs
    {0xFDDD, 0x24157}, //18404 #big5-hkscs
    {0xFDDE, 0x24140}, //18405 #big5-hkscs
    {0xFDDF, 0x261DD}, //18406 #big5-hkscs
    {0xFDE0, 0x704B}, //18407 #big5-hkscs
    {0xFDE1, 0x707E}, //18408 #big5-hkscs
    {0xFDE2, 0x70A7}, //18409 #big5-hkscs
    {0xFDE3, 0x7081}, //18410 #big5-hkscs
    {0xFDE4, 0x70CC}, //18411 #big5-hkscs
    {0xFDE5, 0x70D5}, //18412 #big5-hkscs
    {0xFDE6, 0x70D6}, //18413 #big5-hkscs
    {0xFDE7, 0x70DF}, //18414 #big5-hkscs
    {0xFDE8, 0x4104}, //18415 #big5-hkscs
    {0xFDE9, 0x3DE8}, //18416 #big5-hkscs
    {0xFDEA, 0x71B4}, //18417 #big5-hkscs
    {0xFDEB, 0x7196}, //18418 #big5-hkscs
    {0xFDEC, 0x24277}, //18419 #big5-hkscs
    {0xFDED, 0x712B}, //18420 #big5-hkscs
    {0xFDEE, 0x7145}, //18421 #big5-hkscs
    {0xFDEF, 0x5A88}, //18422 #big5-hkscs
    {0xFDF0, 0x714A}, //18423 #big5-hkscs
    {0xFDF1, 0x716E}, //18424 #big5-hkscs
    {0xFDF2, 0x5C9C}, //18425 #big5-hkscs
    {0xFDF3, 0x24365}, //18426 #big5-hkscs
    {0xFDF4, 0x714F}, //18427 #big5-hkscs
    {0xFDF5, 0x9362}, //18428 #big5-hkscs
    {0xFDF6, 0x242C1}, //18429 #big5-hkscs
    {0xFDF7, 0x712C}, //18430 #big5-hkscs
    {0xFDF8, 0x2445A}, //18431 #big5-hkscs
    {0xFDF9, 0x24A27}, //18432 #big5-hkscs
    {0xFDFA, 0x24A22}, //18433 #big5-hkscs
    {0xFDFB, 0x71BA}, //18434 #big5-hkscs
    {0xFDFC, 0x28BE8}, //18435 #big5-hkscs
    {0xFDFD, 0x70BD}, //18436 #big5-hkscs
    {0xFDFE, 0x720E}, //18437 #big5-hkscs
    {0xFE40, 0x9442}, //18438 #big5-hkscs
    {0xFE41, 0x7215}, //18439 #big5-hkscs
    {0xFE42, 0x5911}, //18440 #big5-hkscs
    {0xFE43, 0x9443}, //18441 #big5-hkscs
    {0xFE44, 0x7224}, //18442 #big5-hkscs
    {0xFE45, 0x9341}, //18443 #big5-hkscs
    {0xFE46, 0x25605}, //18444 #big5-hkscs
    {0xFE47, 0x722E}, //18445 #big5-hkscs
    {0xFE48, 0x7240}, //18446 #big5-hkscs
    {0xFE49, 0x24974}, //18447 #big5-hkscs
    {0xFE4A, 0x68BD}, //18448 #big5-hkscs
    {0xFE4B, 0x7255}, //18449 #big5-hkscs
    {0xFE4C, 0x7257}, //18450 #big5-hkscs
    {0xFE4D, 0x3E55}, //18451 #big5-hkscs
    {0xFE4E, 0x23044}, //18452 #big5-hkscs
    {0xFE4F, 0x680D}, //18453 #big5-hkscs
    {0xFE50, 0x6F3D}, //18454 #big5-hkscs
    {0xFE51, 0x7282}, //18455 #big5-hkscs
    {0xFE52, 0x732A}, //18456 #big5-hkscs
    {0xFE53, 0x732B}, //18457 #big5-hkscs
    {0xFE54, 0x24823}, //18458 #big5-hkscs
    {0xFE55, 0x2882B}, //18459 #big5-hkscs
    {0xFE56, 0x48ED}, //18460 #big5-hkscs
    {0xFE57, 0x28804}, //18461 #big5-hkscs
    {0xFE58, 0x7328}, //18462 #big5-hkscs
    {0xFE59, 0x732E}, //18463 #big5-hkscs
    {0xFE5A, 0x73CF}, //18464 #big5-hkscs
    {0xFE5B, 0x73AA}, //18465 #big5-hkscs
    {0xFE5C, 0x20C3A}, //18466 #big5-hkscs
    {0xFE5D, 0x26A2E}, //18467 #big5-hkscs
    {0xFE5E, 0x73C9}, //18468 #big5-hkscs
    {0xFE5F, 0x7449}, //18469 #big5-hkscs
    {0xFE60, 0x241E2}, //18470 #big5-hkscs
    {0xFE61, 0x216E7}, //18471 #big5-hkscs
    {0xFE62, 0x24A24}, //18472 #big5-hkscs
    {0xFE63, 0x6623}, //18473 #big5-hkscs
    {0xFE64, 0x36C5}, //18474 #big5-hkscs
    {0xFE65, 0x249B7}, //18475 #big5-hkscs
    {0xFE66, 0x2498D}, //18476 #big5-hkscs
    {0xFE67, 0x249FB}, //18477 #big5-hkscs
    {0xFE68, 0x73F7}, //18478 #big5-hkscs
    {0xFE69, 0x7415}, //18479 #big5-hkscs
    {0xFE6A, 0x6903}, //18480 #big5-hkscs
    {0xFE6B, 0x24A26}, //18481 #big5-hkscs
    {0xFE6C, 0x7439}, //18482 #big5-hkscs
    {0xFE6D, 0x205C3}, //18483 #big5-hkscs
    {0xFE6E, 0x3ED7}, //18484 #big5-hkscs
    {0xFE6F, 0x745C}, //18485 #big5-hkscs
    {0xFE70, 0x228AD}, //18486 #big5-hkscs
    {0xFE71, 0x7460}, //18487 #big5-hkscs
    {0xFE72, 0x28EB2}, //18488 #big5-hkscs
    {0xFE73, 0x7447}, //18489 #big5-hkscs
    {0xFE74, 0x73E4}, //18490 #big5-hkscs
    {0xFE75, 0x7476}, //18491 #big5-hkscs
    {0xFE76, 0x83B9}, //18492 #big5-hkscs
    {0xFE77, 0x746C}, //18493 #big5-hkscs
    {0xFE78, 0x3730}, //18494 #big5-hkscs
    {0xFE79, 0x7474}, //18495 #big5-hkscs
    {0xFE7A, 0x93F1}, //18496 #big5-hkscs
    {0xFE7B, 0x6A2C}, //18497 #big5-hkscs
    {0xFE7C, 0x7482}, //18498 #big5-hkscs
    {0xFE7D, 0x4953}, //18499 #big5-hkscs
    {0xFE7E, 0x24A8C}, //18500 #big5-hkscs
    {0xFEA1, 0x2415F}, //18501 #big5-hkscs
    {0xFEA2, 0x24A79}, //18502 #big5-hkscs
    {0xFEA3, 0x28B8F}, //18503 #big5-hkscs
    {0xFEA4, 0x5B46}, //18504 #big5-hkscs
    {0xFEA5, 0x28C03}, //18505 #big5-hkscs
    {0xFEA6, 0x2189E}, //18506 #big5-hkscs
    {0xFEA7, 0x74C8}, //18507 #big5-hkscs
    {0xFEA8, 0x21988}, //18508 #big5-hkscs
    {0xFEA9, 0x750E}, //18509 #big5-hkscs
    {0xFEAA, 0x74E9}, //18510 #big5-hkscs
    {0xFEAB, 0x751E}, //18511 #big5-hkscs
    {0xFEAC, 0x28ED9}, //18512 #big5-hkscs
    {0xFEAD, 0x21A4B}, //18513 #big5-hkscs
    {0xFEAE, 0x5BD7}, //18514 #big5-hkscs
    {0xFEAF, 0x28EAC}, //18515 #big5-hkscs
    {0xFEB0, 0x9385}, //18516 #big5-hkscs
    {0xFEB1, 0x754D}, //18517 #big5-hkscs
    {0xFEB2, 0x754A}, //18518 #big5-hkscs
    {0xFEB3, 0x7567}, //18519 #big5-hkscs
    {0xFEB4, 0x756E}, //18520 #big5-hkscs
    {0xFEB5, 0x24F82}, //18521 #big5-hkscs
    {0xFEB6, 0x3F04}, //18522 #big5-hkscs
    {0xFEB7, 0x24D13}, //18523 #big5-hkscs
    {0xFEB8, 0x758E}, //18524 #big5-hkscs
    {0xFEB9, 0x745D}, //18525 #big5-hkscs
    {0xFEBA, 0x759E}, //18526 #big5-hkscs
    {0xFEBB, 0x75B4}, //18527 #big5-hkscs
    {0xFEBC, 0x7602}, //18528 #big5-hkscs
    {0xFEBD, 0x762C}, //18529 #big5-hkscs
    {0xFEBE, 0x7651}, //18530 #big5-hkscs
    {0xFEBF, 0x764F}, //18531 #big5-hkscs
    {0xFEC0, 0x766F}, //18532 #big5-hkscs
    {0xFEC1, 0x7676}, //18533 #big5-hkscs
    {0xFEC2, 0x263F5}, //18534 #big5-hkscs
    {0xFEC3, 0x7690}, //18535 #big5-hkscs
    {0xFEC4, 0x81EF}, //18536 #big5-hkscs
    {0xFEC5, 0x37F8}, //18537 #big5-hkscs
    {0xFEC6, 0x26911}, //18538 #big5-hkscs
    {0xFEC7, 0x2690E}, //18539 #big5-hkscs
    {0xFEC8, 0x76A1}, //18540 #big5-hkscs
    {0xFEC9, 0x76A5}, //18541 #big5-hkscs
    {0xFECA, 0x76B7}, //18542 #big5-hkscs
    {0xFECB, 0x76CC}, //18543 #big5-hkscs
    {0xFECC, 0x26F9F}, //18544 #big5-hkscs
    {0xFECD, 0x8462}, //18545 #big5-hkscs
    {0xFECE, 0x2509D}, //18546 #big5-hkscs
    {0xFECF, 0x2517D}, //18547 #big5-hkscs
    {0xFED0, 0x21E1C}, //18548 #big5-hkscs
    {0xFED1, 0x771E}, //18549 #big5-hkscs
    {0xFED2, 0x7726}, //18550 #big5-hkscs
    {0xFED3, 0x7740}, //18551 #big5-hkscs
    {0xFED4, 0x64AF}, //18552 #big5-hkscs
    {0xFED5, 0x25220}, //18553 #big5-hkscs
    {0xFED6, 0x7758}, //18554 #big5-hkscs
    {0xFED7, 0x232AC}, //18555 #big5-hkscs
    {0xFED8, 0x77AF}, //18556 #big5-hkscs
    {0xFED9, 0x28964}, //18557 #big5-hkscs
    {0xFEDA, 0x28968}, //18558 #big5-hkscs
    {0xFEDB, 0x216C1}, //18559 #big5-hkscs
    {0xFEDC, 0x77F4}, //18560 #big5-hkscs
    {0xFEDD, 0x7809}, //18561 #big5-hkscs
    {0xFEDE, 0x21376}, //18562 #big5-hkscs
    {0xFEDF, 0x24A12}, //18563 #big5-hkscs
    {0xFEE0, 0x68CA}, //18564 #big5-hkscs
    {0xFEE1, 0x78AF}, //18565 #big5-hkscs
    {0xFEE2, 0x78C7}, //18566 #big5-hkscs
    {0xFEE3, 0x78D3}, //18567 #big5-hkscs
    {0xFEE4, 0x96A5}, //18568 #big5-hkscs
    {0xFEE5, 0x792E}, //18569 #big5-hkscs
    {0xFEE6, 0x255E0}, //18570 #big5-hkscs
    {0xFEE7, 0x78D7}, //18571 #big5-hkscs
    {0xFEE8, 0x7934}, //18572 #big5-hkscs
    {0xFEE9, 0x78B1}, //18573 #big5-hkscs
    {0xFEEA, 0x2760C}, //18574 #big5-hkscs
    {0xFEEB, 0x8FB8}, //18575 #big5-hkscs
    {0xFEEC, 0x8884}, //18576 #big5-hkscs
    {0xFEED, 0x28B2B}, //18577 #big5-hkscs
    {0xFEEE, 0x26083}, //18578 #big5-hkscs
    {0xFEEF, 0x2261C}, //18579 #big5-hkscs
    {0xFEF0, 0x7986}, //18580 #big5-hkscs
    {0xFEF1, 0x8900}, //18581 #big5-hkscs
    {0xFEF2, 0x6902}, //18582 #big5-hkscs
    {0xFEF3, 0x7980}, //18583 #big5-hkscs
    {0xFEF4, 0x25857}, //18584 #big5-hkscs
    {0xFEF5, 0x799D}, //18585 #big5-hkscs
    {0xFEF6, 0x27B39}, //18586 #big5-hkscs
    {0xFEF7, 0x793C}, //18587 #big5-hkscs
    {0xFEF8, 0x79A9}, //18588 #big5-hkscs
    {0xFEF9, 0x6E2A}, //18589 #big5-hkscs
    {0xFEFA, 0x27126}, //18590 #big5-hkscs
    {0xFEFB, 0x3EA8}, //18591 #big5-hkscs
    {0xFEFC, 0x79C6}, //18592 #big5-hkscs
    {0xFEFD, 0x2910D}, //18593 #big5-hkscs
    {0xFEFE, 0x79D4} //18594 #big5-hkscs
};

#endif // DRW_CPTABLE950_H
