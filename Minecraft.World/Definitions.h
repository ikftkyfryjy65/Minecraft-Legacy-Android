#pragma once

#include <cstdint>
#include <vector>

// الحل النهائي: منع استخدام std::byte القياسية وإجبار الكود على استخدام unsigned char صاف2
#undef byte
#define byte unsigned char

using namespace std;

class AABB;
class Recipy;
class Object;

typedef std::vector<AABB*> AABBList;
typedef std::vector<Recipy *> RecipyList;
typedef std::vector<Object *> ObjectList;

#define MAX_PATH_SIZE 256
#define PI (3.141592654f)
#define HALF_PI (1.570796327f)

enum ByteOrder
{
        BIGENDIAN,
        LITTLEENDIAN,

#if defined(__PS3__) || defined(_XBOX)
        LOCALSYTEM_ENDIAN = BIGENDIAN,
#else
        LOCALSYTEM_ENDIAN = LITTLEENDIAN,
#endif
};

enum EDefaultSkins
{
        eDefaultSkins_ServerSelected,
        eDefaultSkins_Skin0,
        eDefaultSkins_Skin1,
        eDefaultSkins_Skin2,
        eDefaultSkins_Skin3,
        eDefaultSkins_Skin4,
        eDefaultSkins_Skin5,
        eDefaultSkins_Skin6,
        eDefaultSkins_Skin7,

        eDefaultSkins_Count,
};
