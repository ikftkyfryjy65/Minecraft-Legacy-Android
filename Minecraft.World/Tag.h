#pragma once
#include <ostream>
#include "InputOutputStream.h"
using namespace std;

 
class Tag
{
public:
    static const byte TAG_End = static_cast<unsigned char>(0);
    static const byte TAG_Byte = static_cast<unsigned char>(1);
    static const byte TAG_Short = static_cast<unsigned char>(2);
    static const byte TAG_Int = static_cast<unsigned char>(3);
    static const byte TAG_Long = static_cast<unsigned char>(4);
    static const byte TAG_Float = static_cast<unsigned char>(5);
    static const byte TAG_Double = static_cast<unsigned char>(6);
    static const byte TAG_Byte_Array = static_cast<unsigned char>(7);
    static const byte TAG_String = static_cast<unsigned char>(8);
    static const byte TAG_List = static_cast<unsigned char>(9);
    static const byte TAG_Compound = static_cast<unsigned char>(10);
	static const byte TAG_Int_Array = static_cast<unsigned char>(11);

private:
    wstring name;

protected:
	Tag(const wstring &name);

public:
    virtual void write(DataOutput *dos) = 0;
    virtual void load(DataInput *dis)  = 0;
    virtual wstring toString() = 0;
    virtual byte getId() = 0;
    void print(ostream out);
    void print(char *prefix, wostream out);
    wstring getName();
    Tag *setName(const wstring& name);
    static Tag *readNamedTag(DataInput *dis);
    static void writeNamedTag(Tag *tag, DataOutput *dos);
    static Tag *newTag(byte type, const wstring &name);
    static wchar_t *getTagName(byte type);
	virtual ~Tag() {}
	virtual bool equals(Tag *obj); // 4J Brought forward from 1.2
	virtual Tag *copy() = 0; // 4J Brought foward from 1.2
};
