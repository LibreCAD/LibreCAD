#ifndef JWTYPE_HEAD
#define JWTYPE_HEAD
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include<list>
#include <iterator>

#ifdef _WINDOWS
#include <windows.h>
#endif

using namespace std;
//typedef	int	INT;
typedef	double	DOUBLE;
#ifndef _WINDOWS
typedef unsigned int DWORD, *LPDWORD;
typedef	bool	BOOL;
#endif
typedef unsigned short WORD, *LPWORD;
typedef unsigned char BYTE, *LPBYTE;
//typedef	void*	Pointer;
typedef	long double	LDouble;

#ifdef USE_INTEGER
/////////////////////////////////////////////////////////
class Integer
{
 private: 
	INT data;
 public:
	Integer(const INT& ival=0)
	{
	data = ival;
	}
	~Integer(){;}
	INT Value(){return data;}
	INT operator=(Integer& val)
		{
			return val.data;
		}
	Integer& operator=(INT val)
		{
			data = val;
			return *this;
		}
//	Integer& operator=(Integer& val)
//		{
//			data = val.data;
//			return *this;
//		}
	friend inline ofstream& operator<<(ofstream&, const Integer&); 
	friend inline ifstream& operator>>(ifstream&, Integer&); 
	friend inline ostream& operator<<(ostream&, const Integer&); 
	friend inline istream& operator>>(istream&, Integer&); 
};
inline ofstream& operator<< (ofstream& ofstr, const Integer& output) 
{
	ofstr.write((char*)&(output.data), sizeof(INT));
	return ofstr;
} 

inline ifstream& operator>> (ifstream& ifstr, Integer& input) 
{
	ifstr.read((char*)&(input.data), sizeof(INT));
	return ifstr;
}

inline ostream& operator<< (ostream& ostr, const Integer& output) 
{
	return ostr << output.data;
} 

inline istream& operator>> (istream& istr, Integer& input) 
{
	return istr >> input.data;
}
/////////////////////////////////////////////////////////
class DWord
{
 private: 
	DWORD data;
 public:
	DWord(const DWORD& ulval=0)
	{
		data = ulval;
	}
	~DWord(){;}
	DWORD Value(){return data;}
	DWORD operator=(DWord& val)
		{
			return val.data;
		}
//	DWord& operator=(DWord& val)
//		{
//			data =  val.data;
//			return *this;
//		}
	DWord& operator=(DWORD val)
		{
			data = val;
			return *this;
		}
	friend inline ofstream& operator<<(ofstream&, const DWord&); 
	friend inline ifstream& operator>>(ifstream&, DWord&); 
	friend inline ostream& operator<<(ostream&, const DWord&); 
	friend inline istream& operator>>(istream&, DWord&); 
};
inline ofstream& operator<< (ofstream& ofstr, const DWord& output) 
{
	ofstr.write((char*)&(output.data), sizeof(DWORD));
	return ofstr;
} 

inline ifstream& operator>> (ifstream& ifstr, DWord& input) 
{
	ifstr.read((char*)&(input.data), sizeof(DWORD));
	return ifstr;
}

inline ostream& operator<< (ostream& ostr, const DWord& output) 
{
	return ostr << output.data;
} 

inline istream& operator>> (istream& istr, DWord& input) 
{
	return istr >> input.data;
}

/////////////////////////////////////////////////////////
class Word
{
 private: 
	WORD data;
 public:
	Word(const WORD& uval=0)
	{
		data = uval;
	}
	~Word(){;}
	WORD Value(){return data;}
	WORD operator=(Word& val)
		{
			return val.data;
		}
	Word& operator=(WORD val)
		{
			data = val;
			return *this;
		}
//	Word& operator=(Word& val)
//		{
//			data = val.data;
//			return *this;
//		}
	friend inline ofstream& operator<<(ofstream&, const Word&); 
	friend inline ifstream& operator>>(ifstream&, Word&); 
	friend inline ostream& operator<<(ostream&, const Word&); 
	friend inline istream& operator>>(istream&, Word&); 
};
inline ofstream& operator<< (ofstream& ofstr, const Word& output) 
{
	ofstr.write((char*)&(output.data), sizeof(WORD));
	return ofstr;
} 

inline ifstream& operator>> (ifstream& ifstr, Word& input) 
{
	ifstr.read((char*)&(input.data), sizeof(WORD));
	return ifstr;
}

inline ostream& operator<< (ostream& ostr, const Word& output) 
{
	return ostr << output.data;
} 

inline istream& operator>> (istream& istr, Word& input) 
{
	return istr >> input.data;
}

/////////////////////////////////////////////////////////
class Byte
{
 private: 
	BYTE data;
 public:
	Byte(const BYTE& bval=0)
	{
		data = bval;
	}
	~Byte(){;}
	BYTE Value(){return data;}
	BYTE operator=(Byte& val)
		{
			return val.data;
		}
	Byte& operator=(BYTE val)
		{
			data = val;
			return *this;
		}
//	Byte& operator=(Byte& val)
//		{
//			data = val.data;
//			return *this;
//		}
	friend inline ofstream& operator<<(ofstream&, const Byte&); 
	friend inline ifstream& operator>>(ifstream&, Byte&); 
	friend inline ostream& operator<<(ostream&, const Byte&); 
	friend inline istream& operator>>(istream&, Byte&); 
    friend inline bool operator==( char ch, Byte& c );
    friend inline bool operator==( Byte& c, char ch );
    friend inline bool operator==( Byte& c1, Byte& c2 );
    friend inline bool operator!=( Byte& c1, Byte& c2 );
    friend inline bool operator!=( char ch, Byte& c );
    friend inline bool operator!=( Byte& c, char ch );
    friend inline bool operator<=( Byte& c, char ch );
    friend inline bool operator<=( char ch, Byte& c );
    friend inline bool operator<=( Byte& c1, Byte& c2 );

};
inline ofstream& operator<< (ofstream& ofstr, const Byte& output) 
{
	ofstr.write((char*)&(output.data), sizeof(BYTE));
	return ofstr;
} 

inline ifstream& operator>> (ifstream& ifstr, Byte& input) 
{
	ifstr.read((char*)&(input.data), sizeof(BYTE));
	return ifstr;
}

inline ostream& operator<< (ostream& ostr, const Byte& output) 
{
	return ostr << output.data;
} 

inline istream& operator>> (istream& istr, Byte& input) 
{
	return istr >> input.data;
}

inline bool operator==( char ch, Byte& c )
{
    return ((BYTE) ch) == c.data;
}

inline bool operator==( Byte& c, char ch )
{
    return ((BYTE) ch) == c.data;
}

inline bool operator==( Byte& c1, Byte& c2 )
{
    return c1.data == c2.data;
}

inline bool operator!=( Byte& c1, Byte& c2 )
{
    return c1.data != c2.data;
}

inline bool operator!=( char ch, Byte& c )
{
    return ((BYTE)ch) != c.data;
}

inline bool operator!=( Byte& c, char ch )
{
    return ((BYTE) ch) != c.data;
}

inline bool operator<=( Byte& c, char ch )
{
    return c.data <= ((BYTE) ch);
}

inline bool operator<=( char ch, Byte& c )
{
    return ((BYTE) ch) <= c.data;
}

inline bool operator<=( Byte& c1, Byte& c2 )
{
    return c1.data <= c2.data;
}

/////////////////////////////////////////////////////////
class Double
{
 private: 
	DOUBLE data;
 public:
	Double(const DOUBLE& db=0.0)
		{
			data = db;
		}
	DOUBLE Value(){return data;}
	Double& operator+(Double& val)
		{
			data += val.data;
			return *this;
		}
	Double& operator+(LDouble& val)
		{
			data += val;
			return *this;
		}
	Double& operator+=(Double& val)
		{
			data += val.data;
			return *this;
		}
	Double& operator=(Double& val)
		{
			data = val.data;
			return *this;
		}
	Double& operator=(DOUBLE& val)
		{
			data = val;
			return *this;
		}

	friend inline ofstream& operator<<(ofstream&, const Double&); 
	friend inline ifstream& operator>>(ifstream&, Double&); 
	friend inline ostream& operator<<(ostream&, const Double&); 
	friend inline istream& operator>>(istream&, Double&); 
};

inline ofstream& operator<< (ofstream& ofstr, const Double& output) 
{
	ofstr.write((char*)&(output.data), sizeof(DOUBLE));
	return ofstr;
} 

inline ifstream& operator>> (ifstream& ifstr, Double& input) 
{
	ifstr.read((char*)&(input.data), sizeof(DOUBLE));
	return ifstr;
}

inline ostream& operator<< (ostream& ostr, const Double& output) 
{
	return ostr << output.data;
} 

inline istream& operator>> (istream& istr, Double& input) 
{
	return istr >> input.data;
}
#endif

#ifndef _WINDOWS
inline ofstream& operator<< (ofstream& ofstr, const DOUBLE& output) 
{
	ofstr.write((char*)&output, sizeof(DOUBLE));
    return ofstr;
} 

inline ifstream& operator>> (ifstream& ifstr, DOUBLE& input) 
{
	ifstr.read((char*)&input, sizeof(DOUBLE));
    return ifstr;
}
#endif

inline ofstream& operator<< (ofstream& ofstr, const DWORD& output) 
{
	ofstr.write((char*)&output, sizeof(DWORD));
    return ofstr;
} 

inline ifstream& operator>> (ifstream& ifstr, DWORD& input) 
{
	ifstr.read((char*)&input, sizeof(DWORD));
    return ifstr;
}

inline ofstream& operator<< (ofstream& ofstr, const WORD& output) 
{
	ofstr.write((char*)&output, sizeof(WORD));
    return ofstr;
} 

inline ifstream& operator>> (ifstream& ifstr, WORD& input) 
{
	ifstr.read((char*)&input, sizeof(WORD));
    return ifstr;
}

inline ofstream& operator<< (ofstream& ofstr, const BYTE& output) 
{
	ofstr.write((char*)&output, sizeof(BYTE));
    return ofstr;
} 

inline ifstream& operator>> (ifstream& ifstr, BYTE& input) 
{
	ifstr.read((char*)&input, sizeof(BYTE));
    return ifstr;
}

inline ofstream& operator<< (ofstream& ofstr, const int& output) 
{
	ofstr.write((char*)&output, sizeof(int));
    return ofstr;
} 

inline ifstream& operator>> (ifstream& ifstr, int& input) 
{
	ifstr.read((char*)&input, sizeof(int));
    return ifstr;
}

/*
class String { 
 private: 
	char* data; 
	size_t size;
 public:
	String(const char* str = 0)
	{
		if( str ){
			data = new char[strlen(str)+1];
			strcpy(data, str);
			size = strlen(str);
		}else{
			data = NULL;
			size = 0;
		}
	}
	~String()
	{
		if(data)
			delete data;
		size = 0;
	}
	char*	ascii(){return data;}
	int	length(){return size;}
	String& operator=(String& str)
	{
		if(str.length()==0)
			return *this;
		if(data)
			delete data;
		data = new char[str.length()+1];
		strcpy(data, str.ascii());
		size = str.size;
		return *this;
	}
	String& operator=(char *str)
	{
		if(strlen(str)==0)
			return *this;
		if(data)
			delete data;
		size = strlen(str);
		data = new char[size+1];
		strcpy(data, str);
		return *this;
	}
	friend inline ofstream& operator<<(ofstream&, const String&); 
	friend inline ifstream& operator>>(ifstream&, String&); 
	friend inline ostream& operator<<(ostream&, const String&); 
	friend inline istream& operator>>(istream&, String&); 
}; 

inline ofstream& operator<< (ofstream& ofstr, const String& output) 
{
	ofstr.write((char*)&(output.size), sizeof(size_t));
	ofstr.write(output.data, output.size);
	return ofstr;
} 

inline ostream& operator<< (ostream& ostr, const String& output) 
{
	return ostr << output.data;
} 

inline ifstream& operator>> (ifstream& ifstr, String& input) 
{
	ifstr.read((char*)&(input.size), sizeof(size_t));
	input.data = new char[input.size+1]; 
	ifstr.read(input.data, input.size);
	input.data[input.size] = (char)NULL;
	return ifstr;
}

inline istream& operator>> (istream& istr, String& input) 
{
	const int maxline = 512;
	char holder[maxline];
	istr.get(holder, maxline, '\n');
	input = holder;
	return istr;
}
*/
#endif//JWTYPE_HEAD
