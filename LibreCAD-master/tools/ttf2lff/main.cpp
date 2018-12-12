/****************************************************************************
** $Id: main.cpp $
**
** Copyright (C) 2011 Rallaz - rallazz@gmail.com
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
** This file is part of the ttf2lff project.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.ribbonsoft.com for further details.
**
** Contact info@ribbonsoft.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifdef __APPLE__
    #include <sys/types.h>
#endif
#ifdef __WIN32__
    #include <time.h>
#endif

#include <iostream>
#include <cmath>
#include <cerrno>
#include <cstring>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H

FT_Library library;
FT_Face face;
double prevx;
double prevy;
bool firstpass;
bool startcontour;
float xMin;
int nodes, precision;
double factor;
int yMax;
FILE* fpLff;
char numFormat [8];

int moveTo(FT_Vector* to, void* fp);
int lineTo(FT_Vector* to, void* fp);
int conicTo(FT_Vector* control, FT_Vector* to, void* fp);
int cubicTo(FT_Vector* control1, FT_Vector* control2, FT_Vector* to, void* fp);

static std::string FT_StrError(FT_Error errnum)
{
    #undef __FTERRORS_H__
    #define FT_ERRORDEF( e, v, s )  { e, s },
    #define FT_ERROR_START_LIST     {
    #define FT_ERROR_END_LIST       { 0, 0 } };

    static const struct {
        FT_Error errnum;
        const char * errstr;
    } ft_errtab[] =
    #include FT_ERRORS_H

    const FT_Error errno_max = (FT_Error)((sizeof(ft_errtab) / sizeof(ft_errtab[0])) - 2 /* FT_ERROR_END_LIST */);
    if(errno > errno_max)
    {
        return "Internal error";
    }

    return std::string(ft_errtab[errnum].errstr);
}

static const FT_Outline_Funcs funcs
= {
      (FT_Outline_MoveTo_Func) moveTo,
      (FT_Outline_LineTo_Func) lineTo,
      (FT_Outline_ConicTo_Func)conicTo,
      (FT_Outline_CubicTo_Func)cubicTo,
      0, 0
  };

std::string clearZeros(double num/*, int precision*/){
	std::string numLine(precision+5, '\0');
	snprintf(&(numLine[0]),precision+5, numFormat, num);
    std::string str = numLine;
    int i = str.length()- 1;
    while (str.at(i) == '0' && i>1) {
        --i;
    }
    if (str.at(i) != '.')
            i++;
    return str.substr(0, i);
}


int moveTo(FT_Vector* to, void* fp) {
    if (firstpass) {
        if (to->x < xMin)
            xMin = to->x;
    } else {
        prevx = to->x;
        prevy = to->y;
        if (fp) {
            if (startcontour) {
                startcontour = false;
                fprintf((FILE*)fp, "%s,%s",clearZeros((double)(to->x-xMin)*factor).c_str(), clearZeros((double)to->y*factor).c_str());
            } else {
                fprintf((FILE*)fp, "\n%s,%s",clearZeros((double)(to->x-xMin)*factor).c_str(), clearZeros((double)to->y*factor).c_str());
            }
        }
    }
    return 0;
}



int lineTo(FT_Vector* to, void* fp) {
    if (firstpass) {
        if (to->x < xMin)
            xMin = to->x;
    } else {
        if (fp) {
            if (startcontour) {
                fprintf((FILE*)fp, "%s,%s",clearZeros((double)(to->x-xMin)*factor).c_str(), clearZeros((double)to->y*factor).c_str());
                startcontour = false;
            } else {
                fprintf((FILE*)fp, ";%s,%s",clearZeros((double)(to->x-xMin)*factor).c_str(), clearZeros((double)to->y*factor).c_str());
            }
        }
        prevx = to->x;
        prevy = to->y;

        if (to->y>yMax) {
            yMax = to->y;
        }
    }
    return 0;
}



int conicTo(FT_Vector* control, FT_Vector* to, void* fp) {
	if (firstpass) {
		if (to->x < xMin)
			xMin = to->x;
		return 0;
	}
	double px, py;
	double ox = prevx;
	double oy = prevy;
	if (fp) {
		if (startcontour) {
			fprintf((FILE*)fp, "%s,%s",clearZeros((ox-xMin)*factor).c_str(), clearZeros(oy*factor).c_str());
			startcontour = false;
		} else {
			fprintf((FILE*)fp, ";%s,%s",clearZeros((ox-xMin)*factor).c_str(), clearZeros(oy*factor).c_str());
		}
		for (double t = 0.0; t<=1.0; t+=1.0/nodes) {
			px = pow(1.0-t, 2)*prevx + 2*t*(1.0-t)*control->x + t*t*to->x;
			py = pow(1.0-t, 2)*prevy + 2*t*(1.0-t)*control->y + t*t*to->y;
			fprintf((FILE*)fp, ";%s,%s",clearZeros((double)(px-xMin)*factor).c_str(), clearZeros((double)py*factor).c_str());

//			ox = px;
//			oy = py;
		}
	}

	prevx = to->x;
	prevy = to->y;

	if (to->y>yMax) yMax = to->y;
	return 0;
}



int cubicTo(FT_Vector* /*control1*/, FT_Vector* /*control2*/, FT_Vector* to, void* fp) {
    if (firstpass) {
        if (to->x < xMin)
            xMin = to->x;
    } else {
        if (fp) {
            if (startcontour) {
                fprintf((FILE*)fp, "%s,%s",clearZeros((double)(to->x-xMin)*factor).c_str(), clearZeros((double)to->y*factor).c_str());
                startcontour = false;
            } else {
                fprintf((FILE*)fp, ";%s,%s",clearZeros((double)(to->x-xMin)*factor).c_str(), clearZeros((double)to->y*factor).c_str());
            }
        }
        prevx = to->x;
        prevy = to->y;

        if (to->y>yMax) {
            yMax = to->y;
        }
    }
    return 0;
}


/**
 * Converts one single glyph (character, sign) into LFF.
 */
FT_Error convertGlyph(FT_ULong charcode) {
    FT_Error error;
    FT_Glyph glyph;

    // load glyph
    error = FT_Load_Glyph(face,
                          FT_Get_Char_Index(face, charcode),
                          FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE);
    if (error) {
        std::cerr << "FT_Load_Glyph: " << FT_StrError(error) << std::endl;
        return error;
    }

    FT_Get_Glyph(face->glyph, &glyph);
    FT_OutlineGlyph og = (FT_OutlineGlyph)glyph;
    if (face->glyph->format != ft_glyph_format_outline) {
        std::cerr << "Not an outline font\n";
    }

    // write glyph header
    if (fpLff) {
        fprintf(fpLff, "\n[#%04X]\n", (unsigned)charcode);
    }

    // trace outline of the glyph
    xMin = 1000.0;
    firstpass = true;
    error = FT_Outline_Decompose(&(og->outline), &funcs, fpLff);
	if (error)
		std::cerr << "FT_Outline_Decompose: first pass: " << FT_StrError(error) << std::endl;

    firstpass = false;
    startcontour = true;
    error = FT_Outline_Decompose(&(og->outline), &funcs, fpLff);
    if (fpLff) {
        fprintf(fpLff, "\n");
    }

	if (error)
		std::cerr << "FT_Outline_Decompose: second pass: " << FT_StrError(error) << std::endl;

    return error;
}

static void usage(int eval) {
    std::cout << "Usage: ttf2lff <options> <ttf file> <lff file>\n";
    std::cout << "  ttf file: An existing True Type Font file\n";
    std::cout << "  lff file: The LFF font file to create\n";
    std::cout << "options are:\n";
    std::cout << "  -n nodes                 Number of nodes for quadratic and cubic splines (int)\n";
    std::cout << "  -a author                Author of the font. Preferably full name and e-mail address\n";
    std::cout << "  -l letter spacing        Letter spacing (float)\n";
    std::cout << "  -w word spacing          Word spacing (float)\n";
    std::cout << "  -f line spacing factor   Default is 1.0 (float)\n";
    std::cout << "  -d precision             Number of decimal digits (int)\n";
    std::cout << "  -L license               license of the font.\n";
    exit(eval);
}


/**
 * Main.
 */
int main(int argc, char* argv[]) {
    FT_Error error;
    std::string fTtf;
    std::string fLff;

    // init:
    fpLff = NULL;
    nodes = 4;
    std::string name = "Unknown";
    double letterSpacing = 3.0;
    double wordSpacing = 6.75;
    double lineSpacingFactor = 1.0;
    std::string author = "Unknown";
    std::string license = "Unknown";
    precision = 6;
    int i;
    int ret;
    library = NULL;
    face = NULL;

    // handle arguments:
    if (argc < 3) {
        usage(1);
        /* NOTREACHED */
    }

    for (i=1; i<argc; ++i) {
        if (!strcmp(argv[i], "-n")) {
            ++i;
            nodes = atoi(argv[i]);
        }
        else if (!strcmp(argv[i], "-a")) {
            ++i;
            author = argv[i];
        }
        else if (!strcmp(argv[i], "-l")) {
            ++i;
            letterSpacing = atof(argv[i]);
        }
        else if (!strcmp(argv[i], "-w")) {
            ++i;
            wordSpacing = atof(argv[i]);
        }
        else if (!strcmp(argv[i], "-d")) {
            ++i;
            precision = atoi(argv[i]);
        }
        else if (!strcmp(argv[i], "-f")) {
            ++i;
            lineSpacingFactor = atof(argv[i]);
        }
        else if (!strcmp(argv[i], "-h")) {
            usage(0);
            /* NOTREACHED */
        }
        else if (!strcmp(argv[i], "-L")) {
            ++i;
            license = argv[i];
        }
        else {
            break;
        }
    }

    if((argc - i) != 2) {
        usage(1);
        /* NOTREACHED */
    }

    fTtf = argv[i++];
    fLff = argv[i];

    std::cout << "TTF file: " << fTtf.c_str() << "\n";
    std::cout << "LFF file: " << fLff.c_str() << "\n";

    ret = 0;

    // init freetype
    error = FT_Init_FreeType(&library);
    if (error) {
        std::cerr << "FT_Init_FreeType: " << FT_StrError(error) << std::endl;
        ret = 1;
    }
    else {
        // load ttf font
        error = FT_New_Face(library,
                            fTtf.c_str(),
                            0,
                            &face);
        if (error) {
            std::cerr << "FT_New_Face: " << fTtf << ": " << FT_StrError(error) << std::endl;
            ret = 1;
        }
        else {
            std::cout << "Family:    " << face->family_name << "\n";
            std::cout << "Height:    " << face->height << "\n";
            std::cout << "Ascender:  " << face->ascender << "\n";
            std::cout << "Descender: " << face->descender << "\n";
            name = face->family_name;

            // find out height by tracing 'A'
            yMax = -1000;
            convertGlyph(65);
            factor = 1.0/(1.0/9.0*yMax);

            std::cout << "Factor:    " << factor << "\n";

            // write font file:
            fpLff = fopen(fLff.c_str(), "wt");
            if (fpLff==NULL) {
                std::cerr << "Can not open " << fLff.c_str() << ": " << strerror(errno) << std::endl;
                ret = 2;
            }
            else {
                snprintf(numFormat,8,"%%.%if", precision);

                // write font header
                fprintf(fpLff, "# Format:            LibreCAD Font 1\n");
                fprintf(fpLff, "# Creator:           ttf2lff\n");
                fprintf(fpLff, "# Version:           1\n");
                fprintf(fpLff, "# Name:              %s\n", name.c_str());
                fprintf(fpLff, "# LetterSpacing:     %s\n", clearZeros(letterSpacing).c_str());
                fprintf(fpLff, "# WordSpacing:       %s\n", clearZeros(wordSpacing).c_str());
                fprintf(fpLff, "# LineSpacingFactor: %s\n", clearZeros(lineSpacingFactor).c_str());

                time_t rawtime;
                struct tm * timeinfo;
                char buffer [12];
                time ( &rawtime );
                timeinfo = localtime ( &rawtime );
                strftime (buffer,sizeof(buffer),"%Y-%m-%d",timeinfo);

                fprintf(fpLff, "# Created:           %s\n", buffer);
                fprintf(fpLff, "# Last modified:     %s\n", buffer);
                fprintf(fpLff, "# Author:            %s\n", author.c_str());
                fprintf(fpLff, "# License:           %s\n", license.c_str());
                fprintf(fpLff, "\n");

                unsigned first;
                FT_Get_First_Char(face, &first);

                FT_ULong  charcode;
                FT_UInt   gindex;

                // iterate through glyphs
                charcode = FT_Get_First_Char( face, &gindex );
                while (gindex != 0) {
                    convertGlyph(charcode);
                    charcode = FT_Get_Next_Char(face, charcode, &gindex);
                }
            }
        }
    }

    if (face) {
        FT_Done_Face(face);
    }
    if (library) {
        FT_Done_Library(library);
    }

    return ret;
}
