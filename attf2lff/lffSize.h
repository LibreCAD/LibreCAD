#include<iostream>
#include<fstream>
#include<algorithm>
#include<vector>
#include<string>
#include<sstream>

class lffFont {
public:
    unsigned int utf8Code;
    std::string lines;
};

class vCodes {
public:
    std::vector<unsigned int> vector;
    void push_back(const unsigned int& i);
    bool find(const unsigned int& i);
};


