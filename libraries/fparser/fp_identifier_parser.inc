/* NOTE:
  Do not include this file in your project. The fparser.cc file #includes
this file internally and thus you don't need to do anything (other than keep
this file in the same directory as fparser.cc).

  Part of this file is generated code (by using the make_function_name_parser
utility, found in the development version of this library). It's not intended
to be modified by hand.
*/

        unsigned nameLength = 0;
        const unsigned maximumNameLength = 0x80000000U-8;
        /*
        Due to the manner the identifier lengths are returned from
        the readOpcode() function, the maximum supported length for
        identifiers is 0x7FFFFFFF bytes. We minus 8 here to add some
        buffer, because of the multibyteness of UTF-8.
        Function names are limited to 0xFFFF bytes instead, but because
        function names that long just are not defined, the point is moot.
        */
        const unsigned char* const uptr = (const unsigned char*) input;
        typedef signed char schar;
        while(likely(nameLength < maximumNameLength))
        {
            unsigned char byte = uptr[nameLength+0];
            /* Handle the common case of A-Za-z first */
            if(byte >= 0x40)
            {
                if(byte < 0x80) // 0x40..0x7F - most common case
                {
                    // Valid characters in 40..7F: A-Za-z_
                    // Valid bitmask for 40..5F: 01111111111111111111111111100001
                    // Valid bitmask for 60..7F: 01111111111111111111111111100000
                    if(sizeof(unsigned long) == 8)
                    {
                        const unsigned n = sizeof(unsigned long)*8-32;
                        // ^ avoids compiler warning when not 64-bit
                        unsigned long masklow6bits = 1UL << (byte & 0x3F);
                        if(masklow6bits & ~((1UL << 0) | (0x0FUL << (0x1B  ))
                                          | (1UL << n) | (0x1FUL << (0x1B+n))))
                            { ++nameLength; continue; }
                    }
                    else
                    {
                        unsigned masklow5bits = 1 << (byte & 0x1F);
                        if((masklow5bits & ~(1 | (0x1F << 0x1B))) || byte == '_')
                            { ++nameLength; continue; }
                    }
                    break;
                }
                if(byte < 0xF0)
                {
                    if(byte < 0xE0)
                    {
                        if(byte < 0xC2) break; // 0x80..0xC1
                        if(byte == 0xC2 && uptr[nameLength+1]==0xA0) break; // skip nbsp
                        // C2-DF - next common case when >= 0x40
                        // Valid sequence: C2-DF 80-BF
                        if(schar(uptr[nameLength+1]) > schar(0xBF)) break;
                        nameLength += 2;
                        continue;
                    }
                    if(byte == 0xE0) // E0
                    {
                        // Valid sequence: E0 A0-BF 80-BF
                        if((unsigned char)(uptr[nameLength+1] - 0xA0) > (0xBF-0xA0)) break;
                    }
                    else
                    {
                        if(byte == 0xED) break; // ED is invalid
                        // Valid sequence: E1-EC 80-BF 80-BF
                        //            And: EE-EF 80-BF 80-BF
                        if(byte == 0xE2)
                        {
                            // break on various space characters
                            if(uptr[nameLength+1] == 0x80
                            && (schar(uptr[nameLength+2]) <= schar(0x8B)
                            || (uptr[nameLength+2] == 0xAF))) break;
                            if(uptr[nameLength+1] == 0x81
                            && uptr[nameLength+2] == 0x9F) break;
                        } else
                        if(byte == 0xE3 && uptr[nameLength+1] == 0x80
                        && uptr[nameLength+2] == 0x80) break; // this too

                        if(schar(uptr[nameLength+1]) > schar(0xBF)) break;
                    }
                    if(schar(uptr[nameLength+2]) > schar(0xBF)) break;
                    nameLength += 3;
                    continue;
                }
                if(byte == 0xF0) // F0
                {
                    // Valid sequence: F0 90-BF 80-BF 80-BF
                    if((unsigned char)(uptr[nameLength+1] - 0x90) > (0xBF-0x90)) break;
                }
                else
                {
                    if(byte > 0xF4) break; // F5-FF are invalid
                    if(byte == 0xF4) // F4
                    {
                        // Valid sequence: F4 80-8F
                        if(schar(uptr[nameLength+1]) > schar(0x8F)) break;
                    }
                    else
                    {
                        // F1-F3
                        // Valid sequence: F1-F3 80-BF 80-BF 80-BF
                        if(schar(uptr[nameLength+1]) > schar(0xBF)) break;
                    }
                }
                if(schar(uptr[nameLength+2]) > schar(0xBF)) break;
                if(schar(uptr[nameLength+3]) > schar(0xBF)) break;
                nameLength += 4;
                continue;
            }
            if(nameLength > 0)
            {
                if(sizeof(unsigned long) == 8)
                {
                    // Valid bitmask for 00..1F: 00000000000000000000000000000000
                    // Valid bitmask for 20..3F: 00000000000000001111111111000000
                    const unsigned n = sizeof(unsigned long)*8-32;
                    // ^ avoids compiler warning when not 64-bit
                    unsigned long masklow6bits = 1UL << byte;
                    if(masklow6bits & (((1UL << 10)-1UL) << (16+n)))
                        { ++nameLength; continue; }
                }
                else
                {
                    if(byte >= '0' && byte <= '9')
                        { ++nameLength; continue; }
                }
            }
            break;
        }

        /* This function generated with make_function_name_parser.cc */
#define lN l7 lB
#define lM l2 lB
#define lL l3 lB
#define lK if('i'l4
#define lJ uptr
#define lI l5 3]={
#define lH 'n'l4
#define lG l6 3;}lB
#define lF return
#define lE 0x80000003U:3;
#define lD 0x80000005U:5;
#define lC std::memcmp(lJ+
#define lB case
#define lA switch(
#define l9 <<16)|
#define l8 lC 1,tmp,
#define l7 lD lF 5;}
#define l6 default:lF
#define l5 static const char tmp[
#define l4 ==lJ[
#define l3 lE lF 3;
#define l2 0x80000004U:4;lF 4;
#define l1 .enabled()?(
#define l0 lF Functions[
lA
nameLength){lB
2:lK
0]&&'f'l4
1])l0
cIf]l1
cIf
l9
0x80000002U:2;lF
2;lB
3:lA
lJ[0]){lB'a':if('b'l4
1]&&'s'l4
2])l0
cAbs]l1
cAbs
l9
lL'c':lA
lJ[1]){lB'o':lA
lJ[2]){lB's':l0
cCos]l1
cCos
l9
lE
lB't':l0
cCot]l1
cCot
l9
lE
lG's':if('c'l4
2])l0
cCsc]l1
cCsc
l9
l3
lG'e':if('x'l4
1]&&'p'l4
2])l0
cExp]l1
cExp
l9
lL'i':if(lH
1]&&'t'l4
2])l0
cInt]l1
cInt
l9
lL'l':if('o'l4
1]&&'g'l4
2])l0
cLog]l1
cLog
l9
lL'm':lA
lJ[1]){lB'a':if('x'l4
2])l0
cMax]l1
cMax
l9
lL'i':if(lH
2])l0
cMin]l1
cMin
l9
l3
lG'p':if('o'l4
1]&&'w'l4
2])l0
cPow]l1
cPow
l9
lL's':lA
lJ[1]){lB'e':if('c'l4
2])l0
cSec]l1
cSec
l9
lL'i':if(lH
2])l0
cSin]l1
cSin
l9
l3
lG't':if('a'l4
1]&&lH
2])l0
cTan]l1
cTan
l9
l3
lG
4:lA
lJ[0]){lB'a':lA
lJ[1]){lB'c':if('o'l4
2]&&'s'l4
3])l0
cAcos]l1
cAcos
l9
lM's':lK
2]&&lH
3])l0
cAsin]l1
cAsin
l9
lM't':if('a'l4
2]&&lH
3])l0
cAtan]l1
cAtan
l9
l2
l6
4;}
lB'c':lA
lJ[1]){lB'b':if('r'l4
2]&&'t'l4
3])l0
cCbrt]l1
cCbrt
l9
lM'e':lK
2]&&'l'l4
3])l0
cCeil]l1
cCeil
l9
lM'o':if('s'l4
2]&&'h'l4
3])l0
cCosh]l1
cCosh
l9
l2
l6
4;}
lB'e':lA
lJ[1]){lB'v':if('a'l4
2]&&'l'l4
3])l0
cEval]l1
cEval
l9
lM'x':if('p'l4
2]&&'2'l4
3])l0
cExp2]l1
cExp2
l9
l2
l6
4;}
lB'l':{lI'o','g','2'}
;if(l8
3)==0)l0
cLog2]l1
cLog2
l9
l2}
lB's':lA
lJ[1]){lB'i':if(lH
2]&&'h'l4
3])l0
cSinh]l1
cSinh
l9
lM'q':if('r'l4
2]&&'t'l4
3])l0
cSqrt]l1
cSqrt
l9
l2
l6
4;}
lB't':{lI'a','n','h'}
;if(l8
3)==0)l0
cTanh]l1
cTanh
l9
l2}
l6
4;}
lB
5:lA
lJ[0]){lB'a':lA
lJ[1]){lB'c':{lI'o','s','h'}
;if(lC
2,tmp,3)==0)l0
cAcosh]l1
cAcosh
l9
lN's':{lI'i','n','h'}
;if(lC
2,tmp,3)==0)l0
cAsinh]l1
cAsinh
l9
lN't':if('a'l4
2]){if(lH
3]){lA
lJ[4]){lB'2':l0
cAtan2]l1
cAtan2
l9
lD
lB'h':l0
cAtanh]l1
cAtanh
l9
lD
l6
5;}
}
lF
5;}
lF
5;l6
5;}
lB'f':{l5
4]={'l','o','o','r'}
;if(l8
4)==0)l0
cFloor]l1
cFloor
l9
lN'h':{l5
4]={'y','p','o','t'}
;if(l8
4)==0)l0
cHypot]l1
cHypot
l9
lN'l':{l5
4]={'o','g','1','0'}
;if(l8
4)==0)l0
cLog10]l1
cLog10
l9
lN't':{l5
4]={'r','u','n','c'}
;if(l8
4)==0)l0
cTrunc]l1
cTrunc
l9
l7
l6
5;}
default:break;}
lF
nameLength;