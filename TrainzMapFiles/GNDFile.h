#include "IOArchive.h"
#include <vector>

#pragma once

//https://github.com/SilverGreen93/CDPExplorer/blob/master/kuid-format.md

#pragma pack(1)
struct GND_Color4
{
    float R;
    float G;
    float B;
    float A;
};
#pragma pack()

struct KUID
{
public:
#pragma pack(1)
    union
    {
        struct
        {
            signed userid : 25;
            unsigned version : 7;
        };
        uint32_t _high;
    };
    union
    {
        signed contentid : 32;
        uint32_t _low;
    };
#pragma pack()
public:
    KUID() : _high(0), _low(0) { }
    inline std::string KUIDstr()
    {
        return std::string() + "<kuid" + (version > 0 ? "2:" : ":") + std::to_string(userid) + ":" + std::to_string(contentid) + ((version > 0) ? (":" + std::to_string(version)) : "") + ">";
    }

    KUID(int32_t in_userid, int32_t in_contentid, uint8_t in_revision = 0)
        : userid(in_userid), contentid(in_contentid), version(in_revision) { }
};


struct DiurnalKeyframe
{
    float time;
    GND_Color4 sunLightColor;
    GND_Color4 ambientLightColor;
    float unknown1;
    GND_Color4 fogColor;
    float fogDensity = 0.0f;
    float dayNight;

    GND_Color4 sky0;
    GND_Color4 sky1;
    GND_Color4 sky2;
};

struct DiurnalTable
{
	std::vector<DiurnalKeyframe> keyframes;
    KUID CloudKUID;
    int32_t rainValue;
    float timeOfDay;
    time_t date = 0;

    uint8_t unknown0 = 0;

    float windStrength = 0.0f;
    float snowlineHeight = 2000.0f;
};

struct TextureElement
{
    float usage = 1.0f;
    KUID texture;
};

struct WaterDataElement
{
    int16_t h;
    int16_t v;
    float height;
};


struct GroundVertexData
{
    uint8_t texID;
    union
    {
        struct
        {
            unsigned rotation : 4; // divide by 16
            unsigned scale : 4; // divide by 15
        };
        uint8_t RotSc = 0;
    };
    uint8_t texAmount = 0xFF;
};

struct GroundVertexElement
{
    std::vector<GroundVertexData> data;
    float height = 0.0f;
    //int8_t normal[3] = {(int8_t)0x00, (int8_t)0x00, (int8_t)0x7F};
    bool staleNormals = false;
    float normal[3] = {0.0f, 0.0f, 1.0f};
};

struct GroundSectionData
{
    std::vector<TextureElement> TextureTable;
    std::vector<WaterDataElement> waterData;
    std::vector<GroundVertexElement> vertexTable;

    inline GroundVertexElement& GetVertex(uint32_t x, uint32_t y)
    {
        return vertexTable[x + y * 76];
    }
};

struct GroundSection
{
	int32_t originH;
	int32_t originV;
	char sectionType[5] = { 0x73, 0x63, 0x65, 0x53, 0x00 }; //sceS - 10m, 5ceS - 5m

	//section data
	uint32_t vertexOffset = 0;
	uint32_t minimapOffset = 0;

    uint32_t minimapResoulution = 64;

	uint8_t unknown0 = 0;
	uint32_t unknown1 = 0; //crc?


    GroundSectionData data;
};

class GNDFile
{
public:
	GNDFile(const char* path);
	~GNDFile();

	bool Serialize(IOArchive& Ar);

	std::vector<GroundSection> sections;
	DiurnalTable diurnalTable;
    GND_Color4 WaterColor;
    KUID WaterKUID;

    //version 09
    KUID UnknownKUID;
    KUID GridTex;

private:
    void SerializeTextureTable(IOArchive& Ar, std::vector<TextureElement>& table);
    void SerializeWaterDataTable(IOArchive& Ar, GroundSectionData& data);
    void SerializeClassicVertexTable(IOArchive& Ar, uint8_t version, GroundSectionData& data);

    void SerializeNormal(IOArchive& Ar, float* normal);
    inline void Normalize(float* vec);
};

