#include<iostream>
#include<fstream>
#include<algorithm>
#include<vector>
#include<string>
#include<sstream>
#include"lffSize.h"

using namespace std;

void vCodes::push_back(const unsigned int& i) {
    vector.push_back(i);
}

//find the integer from
bool vCodes::find(const unsigned int& n) {
    int low(0);
    int high(vector.size()-1);
    if (high == 0) return false;
    int middle( (low+high) >> 1);
    while( middle != low && middle != high) {
        if ( vector[middle] > n ) {
            high=middle;
        } else if ( vector[middle] < n ) {
            low=middle;
        } else {
            return true;
        }
        middle=(low+high)>>1;
    }
    return vector[low] == n || vector[high] == n;
}

bool operator<(const lffFont& a, const lffFont& b) {
    return a.utf8Code < b.utf8Code ;
}

int main(int argc, char* argv[]) {
    if(argc<3) {
        std::cout<<"usage:: [Options] input_lff output_lff\n";
        std::cout<<"Options:\n";
        std::cout<<"\t-h\treplace font header with WenQuanYi font header. Default is to keep font header\n";
        std::cout<<"\t-w\tQuietly overwrite existing font file. Default not to overwrite existing file"<<std::endl;
        return 0;
    }
    bool replaceHeader(false);
    bool overWrite(false);
    string inputFileName;
    string outputFileName;
    for(int i=1; i<argc; i++) {
        string cmd(argv[i]);
        if(cmd.size()==0) continue;
        switch(cmd[0]) {
        case '-':
            //options
            for(unsigned j=1; j<cmd.size(); j++) {
                switch(cmd[j]) {
                case 'h':
                case 'H':
                    replaceHeader=true;
                    break;
                case 'w':
                case 'W':
                    overWrite=true;
                default:
                    break;
                }
            }
            break;
        default:
            if(inputFileName.size()==0) {
                inputFileName=cmd;
            } else {
                outputFileName=cmd;
            }
        }
    }

    if( inputFileName == outputFileName ) {
        std::cout<<"Output file can not be the input file\n";
        exit(0);
    }


    //  for(auto pi=codes.begin(); pi != codes.end(); pi++) {
    //      std::cout<<*pi<<std::endl;
    //  }
    //int n1(0x4e10),n2(0x4e12);
    //std::cout<<"find(n1)="<<cList.find(n1)<<std::endl;
    //std::cout<<"find(n2)="<<cList.find(n2)<<std::endl;
    std::ifstream in0(inputFileName.c_str());
    if(! in0.is_open()) {
        std::cout<<"can not open the input lff font file: "<<argv[1]<<std::endl;
        return 0;
    }
    if(overWrite==false) {
        std::ifstream in1(outputFileName.c_str());
        if(in1.is_open()) {
            std::cout<<"won't overwrite existing file: "<<argv[2]<<std::endl;
            std::cout<<"please try again use another filename, or move away the existing file\n";
            std::cout<<"To force overwrite existing font file, add option: -w"<<std::endl;
            return 0;
        }
        in1.close();
    }
    std::ofstream out0(outputFileName.c_str());
    if (replaceHeader) {
        //header
        out0<<"# Format:            LibreCAD Font 1\n";
        out0<<"# Creator:           LibreCAD\n";
        out0<<"# Name:              WenQuanYi Micro Hei Mono Light\n";
        out0<<"# Name:              Chinese Simplified\n";
        out0<<"# Encoding:          UTF-8\n";
        out0<<"# LetterSpacing:     3\n";
        out0<<"# WordSpacing:       6.75\n";
        out0<<"# LineSpacingFactor: 1\n";
        out0<<"# Created:           2011-10-27\n";
        out0<<"# Last modified:     2011-10-27\n";
        out0<<"# Author:            WenQuanYi Project Contributors\n";
        out0<<"# License:           Apache2.0 or GPLv3\n";
        out0<<std::endl;
    }

    std::string linebuf;
    std::vector<lffFont> vlf;
    while(! in0.eof()) {
        getline(in0,linebuf);
        if(linebuf.size()==0) {
            //
            continue;
        }
        lffFont lf;
        switch(linebuf[0]) {
        case '#':
            //case '\n':
            if( replaceHeader == false) {
                out0<<linebuf<<std::endl;
            }
            break;
        case '[': //code
        {
            std::ostringstream oss;
            oss.clear();
            size_t iStart=1;
            if(linebuf[1] == '#' ) {
                //ignore '#' as first character for unicode
                iStart=2;
            }
            size_t iEnd=linebuf.find(']');
            if(iEnd == std::string::npos) {
                iEnd=linebuf.size()-1;
            }
            std::istringstream iss(linebuf.substr(iStart,iEnd));
            iss>>std::hex>>lf.utf8Code;
            oss<<'['<<std::hex<<lf.utf8Code<<linebuf.substr(iEnd,linebuf.size()-1)<<std::endl;
            //oss<<linebuf<<std::endl;
            getline(in0,linebuf);
            oss.precision(2);
            while(linebuf.size() >1) {
                iss.clear();
                iss.str(linebuf);
                iss.seekg(0,std::ios::beg);
                float f;
                char c;
                while(iss.good()) {
                    iss>>c;
                    //std::cout<<c<<std::endl;
                    switch(c) {
                    case ',':
                    case ';':
                    case 'A':
                    case 'a':
                        oss<<c;
                        break;
                    case 'C':
                    case 'c':
                        oss<<c;
                        for(int i0=0; i0<4; i0++) {
                            iss>>c;
                            oss<<c;
                        }
                        oss<<std::endl;
                        break;

                    default:
                    {
                        iss.unget();
                        iss>>f;
                        std::ostringstream oss2;
                        oss2<<std::fixed;
                        oss2.precision(2);
                        oss2<<f;
                        std::string str2=oss2.str();
                        size_t ip=str2.find('.');
                        if(ip != std::string::npos) {
                            size_t length=str2.size();
                            while( str2[length - 1] == '0' ) length--;
                            str2=str2.substr(0,length);
                        }
                        if(fabs(f)<0.005) {
                            str2='0';
                        } else if( str2.substr(0,2) == std::string("0.") ) {
                            str2=str2.substr(1);
                        } else if( str2.substr(0,3) == std::string("-0.") )
                            str2='-'+str2.substr(2);
                        if(str2.at(str2.size()-1) == '.' && str2.size() >= 2) {
                            str2.erase(str2.begin() + str2.size() -1 );
                        }
                        oss<<str2;

                    }


                    }
                }
                oss<<std::endl;
                getline(in0,linebuf);
            }
            //std::cout<<std::hex<<lf.utf8Code<<std::endl;
            oss.flush();
            lf.lines=oss.str();
            //if(cList.find(lf.utf8Code)) {
            std::cout<<std::hex<<lf.utf8Code<<std::endl;
            vlf.push_back(lf);
            //}
        }
        break;
        default:
            break;
        }

    }
    in0.close();
    std::cout<<"vlf.size()="<<vlf.size()<<std::endl;
    sort(vlf.begin(),vlf.end());
    for(auto iv=vlf.begin(); iv != vlf.end(); iv++) {
        out0<<iv->lines<<std::endl;
    }
    out0.close();
    return 0;

}
