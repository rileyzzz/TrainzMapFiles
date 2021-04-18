#include "GNDFile.h"
#include <stdio.h>
#include <iostream>
#include <iomanip>

GNDFile::GNDFile(const char* path)
{
	IOArchive Arch(path, IODirection::Import);
	Serialize(Arch);
}

GNDFile::~GNDFile()
{
}

bool GNDFile::Serialize(IOArchive& Ar)
{
	if (Ar.IsSaving())
	{
		Ar.Serialize("GND", 3);
	}
	else
	{
		char ReadHeader[4] = { 0x00 };
		Ar.Serialize(&ReadHeader[0], 3);
		if (strcmp(ReadHeader, "GND") != 0)
		{
			printf("Failed to read chunk GND\n");
			return false;
		}
	}

	uint8_t version = 11;
	Ar << version;

	printf("GND version %u\n", version);

	if (version == 21)
	{
		uint32_t unknown1 = 0;
		uint32_t unknown2 = 0;
		Ar << unknown1;
		Ar << unknown2;
	}

	uint32_t sectionCount = sections.size();
	Ar << sectionCount;

	printf("%u ground sections\n", sectionCount);

	if (Ar.IsLoading()) sections.resize(sectionCount);
	for (uint32_t i = 0; i < sectionCount; i++)
	{
		GroundSection& section = sections[i];

		Ar << section.originH;
		Ar << section.originV;

		if (version > 11)
			Ar.Serialize(section.sectionType, 4);

		if (version < 21)
		{
			//section data
			Ar << section.vertexOffset;
			if (version < 16)
				Ar << section.minimapOffset;
			if (version < 12)
				section.minimapResoulution = 128;
		}
		else
		{
			Ar << section.unknown0;
			Ar << section.unknown1;

			auto& textureTable = section.data.TextureTable;
			uint32_t numTextures = textureTable.size();
			Ar << numTextures;
			if (Ar.IsLoading()) textureTable.resize(numTextures);
			for (uint32_t tex = 0; tex < numTextures; tex++)
			{
				TextureElement& element = textureTable[tex];
				//Ar << element.usage;
				Ar << element.texture._low;
				Ar << element.texture._high;
			}
		}

		printf("Ground section %d %d type %s offset %u, unknowns: %u, %u\n", section.originH, section.originV, section.sectionType, section.vertexOffset, section.unknown0, section.unknown1);
	}

	
	printf("diurnal begin %d\n", (int)Ar.tell());

	//read diurnal table
	if (version >= 7)
	{
		if (version >= 13)
		{
			int32_t NegativeOne = -1;
			Ar << NegativeOne;
			assert(NegativeOne == -1);
		}


		uint32_t diurnalTableVersion = 1;

		if(version >= 13)
			Ar << diurnalTableVersion;

		printf("Diurnal table version %u\n", diurnalTableVersion);

		uint32_t keyframeCount = diurnalTable.keyframes.size();
		Ar << keyframeCount;

		Ar << diurnalTable.CloudKUID._low;
		Ar << diurnalTable.CloudKUID._high;

		printf("Cloud kuid %s\n", diurnalTable.CloudKUID.KUIDstr().c_str());

		Ar << diurnalTable.rainValue;
		printf("time of day offset %d\n", (int)Ar.tell());
		Ar << diurnalTable.timeOfDay;

		printf("Rain %d, time of day %f\n", diurnalTable.rainValue, diurnalTable.timeOfDay);

		printf("begin diurnal table %d\n", (int)Ar.tell());
		if (Ar.IsLoading()) diurnalTable.keyframes.resize(keyframeCount);
		for (uint32_t i = 0; i < keyframeCount; i++)
		{
			DiurnalKeyframe& keyframe = diurnalTable.keyframes[i];
			Ar << keyframe.time;
			Ar << keyframe.sunLightColor;
			Ar << keyframe.ambientLightColor;
			//good
			if (version != 9 && version != 13)
				Ar << keyframe.unknown1;
			Ar << keyframe.fogColor;
			if(version > 13)
				Ar << keyframe.fogDensity;
			Ar << keyframe.dayNight;

			//if (version != 13)
			Ar << keyframe.sky0;
			Ar << keyframe.sky1;
			Ar << keyframe.sky2;
			

			
			printf("Diurnal keyframe time %f, fogdensity %f, daynight %f\n", keyframe.time, keyframe.fogDensity, keyframe.dayNight);
		}
		printf("diurnal end %d\n", (int)Ar.tell());

		if(version == 21)
			Ar << diurnalTable.unknown0;
		if (version > 11)
		{
			Ar << diurnalTable.date;

			Ar << diurnalTable.windStrength;

			Ar << diurnalTable.snowlineHeight;
		}

		printf("Wind strength %f, snowline height %f\n", diurnalTable.windStrength, diurnalTable.snowlineHeight);
	}
	printf("end diurnal table %d\n", (int)Ar.tell());
	//chump data here
	if (version == 21)
	{
		printf("chump begin %d\n", (int)Ar.tell());
		Ar.ignore(0x10);
	}

	if (version >= 11)
		Ar << WaterColor;
	if (version >= 9 && version != 11)
	{
		Ar << WaterKUID._low;
		Ar << WaterKUID._high;
		printf("Water kuid %s\n", WaterKUID.KUIDstr().c_str());
	}
	
	if (version == 9)
	{
		Ar << UnknownKUID._low;
		Ar << UnknownKUID._high;
		printf("Unknown kuid %s\n", UnknownKUID.KUIDstr().c_str());
	}

	if (version >= 8 && version < 14)
	{
		//ruler data

	}

	printf("texture table %d\n", (int)Ar.tell());
	if (version < 13)
	{
		//texture table
		std::vector<TextureElement> TextureTable;
		SerializeTextureTable(Ar, TextureTable);
		
		for (uint32_t i = 0; i < sectionCount; i++)
		{
			uint32_t sectionIndex = sectionCount - 1 - i;
			GroundSection& section = sections[sectionIndex];
			GroundSectionData& data = section.data;

			data.TextureTable = TextureTable;
		}

		Ar << GridTex._low;
		Ar << GridTex._high;
		printf("Grid kuid %s\n", GridTex.KUIDstr().c_str());
	}

	//return true;

	for (uint32_t i = 0; i < sectionCount; i++)
	{
		uint32_t sectionIndex = sectionCount - 1 - i;
		GroundSection& section = sections[sectionIndex];
		GroundSectionData& data = section.data;


		if (version < 21)
		{
			//seek to section data
			Ar.seek(section.vertexOffset, std::ios::beg);

			if (strcmp(section.sectionType, "sceS") == 0)
			{
				//classic section
				if (version >= 13)
					SerializeTextureTable(Ar, data.TextureTable);

				SerializeWaterDataTable(Ar, data);

				SerializeClassicVertexTable(Ar, version, data);
			}
			else if (strcmp(section.sectionType, "5ceS") == 0)
			{
				//quad section
				printf("quad section\n");
			}
			else
				printf("Unknown section type at index %u!\n", sectionIndex);


		}
		else
		{
			std::string dir = Ar.getFilepath();
			dir = dir.substr(0, dir.find_last_of("\\/") + 1);
			IOArchive mapFile(dir + "mapfile_" + std::to_string(section.originH) + "_" + std::to_string(section.originV) + ".gnd", Ar.IsLoading() ? IODirection::Import : IODirection::Export);
			printf("file dir %s\n", mapFile.getFilepath().c_str());
			char mapHeader[4] = { 0x15, 0x00, 0x00, 0x00 };
			mapFile.Serialize(mapHeader, 4);
			if (strcmp(section.sectionType, "sceS") == 0)
			{
				//classic section
				//texture table
				SerializeTextureTable(mapFile, data.TextureTable);

				SerializeWaterDataTable(mapFile, data);

				printf("data start %d\n", (int)mapFile.tell());
				SerializeClassicVertexTable(mapFile, version, data);
			}
			else if (strcmp(section.sectionType, "5ceS") == 0)
			{
				//quad section
				printf("quad section\n");
			}
			else
				printf("Unknown section type at index %u!\n", sectionIndex);
		}

		
		printf("Read section %u\n", sectionIndex);
	}

	printf("data end %d\n", (int)Ar.tell());

	return true;
}

void GNDFile::SerializeTextureTable(IOArchive& Ar, std::vector<TextureElement>& table)
{
	uint32_t maxGTextures = table.size();
	Ar << maxGTextures;
	printf("maxgtextures %u\n", maxGTextures);
	printf("archive location %d\n", (int)Ar.tell());
	//assert(false);
	if (Ar.IsLoading()) table.resize(maxGTextures);
	for (uint32_t tex = 0; tex < maxGTextures; tex++)
	{
		TextureElement& element = table[tex];
		Ar << element.usage;
		if (element.usage != 0.0f)
		{
			Ar << element.texture._low;
			Ar << element.texture._high;
		}

		printf("texture kuid %s\n", element.texture.KUIDstr().c_str());
	}
	printf("tex end %d\n", (int)Ar.tell());
}

void GNDFile::SerializeWaterDataTable(IOArchive& Ar, GroundSectionData& data)
{
	uint32_t waterDataCount = data.waterData.size();
	Ar << waterDataCount;
	if (Ar.IsLoading()) data.waterData.resize(waterDataCount);
	printf("water count %u\n", waterDataCount);
	for (uint32_t water = 0; water < waterDataCount; water++)
	{
		WaterDataElement& element = data.waterData[water];
		Ar << element.h;
		Ar << element.v;
		Ar << element.height;

		//printf("water height %f\n", element.height);
	}
}

void GNDFile::SerializeClassicVertexTable(IOArchive& Ar, uint8_t version, GroundSectionData& data)
{
	uint32_t vertexTableCount = 76 * 76;

	data.vertexTable.resize(vertexTableCount);

	for (uint32_t vert = 0; vert < vertexTableCount; vert++)
	{
		GroundVertexElement& element = data.vertexTable[vert];

		if (Ar.IsLoading())
		{
			int8_t identifier = Ar.peek();
			//Ar << identifier;
			//printf("begin vertex run %d\n", (int)Ar.tell());
			if (identifier == '\375' || identifier == '\376')
			{
				Ar.ignore(1);
				//onetex
				{
					GroundVertexData vertData;
					Ar << vertData.texID;
					if (version >= 2)
						Ar << vertData.RotSc;

					element.data.push_back(vertData);
				}

				if (identifier == '\375')
				{
					Ar << element.height;
					if (version >= 11 && version < 13) //version >= 12
						SerializeNormal(Ar, element.normal);
					else
						element.staleNormals = true;

					//weird tangent-like data
					if (version == 9)
						Ar.ignore(3);
				}
			}
			else
			{
				//printf("begin vertex run %d\n", (int)Ar.tell());
				//assert(false);
				int iterCount = 0;
				while (identifier != '\377')
				{
					GroundVertexData vertData;
					Ar << vertData.texID;
					Ar << vertData.texAmount;
					if (version >= 2)
						Ar << vertData.RotSc;
					element.data.push_back(vertData);

					identifier = Ar.peek();
					iterCount++;
				}// while (identifier != '\377'); //'\377'
				//printf("finish vertex run %d, %d\n", iterCount, (int)Ar.tell());
				//assert(false);
				Ar.ignore(1);
				Ar << element.height;
				if (version >= 11 && version < 13) //version >= 12 NORMALS ARENT IN 13, 21
					SerializeNormal(Ar, element.normal);
				else
					element.staleNormals = true;

				//weird tangent-like data
				if (version == 9)
					Ar.ignore(3);

				//printf("height %f\n", element.height);

				//assert(false);
			}
		}
		else
		{
			//TODO pick the best structure given the vertex data

		}

		//printf("finish vertex run %d\n", (int)Ar.tell());
		//assert(false);
	}


	if (Ar.IsLoading() && version < 11 || version >= 13)
	{
		constexpr int dx = 1;
		constexpr int dy = 1;

		//generate normal data
		for (int y = 2; y < 75; y++)
		{
			for (int x = 2; x < 75; x++)
			{
				GroundVertexElement& vert = data.GetVertex(x, y);
				if (vert.staleNormals)
				{
					float dfdx = (data.GetVertex(x + dx, y).height - data.GetVertex(x - dx, y).height) / (2.0f * dx * 10.0f);
					float dfdy = (data.GetVertex(x, y + dy).height - data.GetVertex(x, y - dy).height) / (2.0f * dy * 10.0f);

					float normal[3] = { -dfdx, -dfdy, 1.0f };
					//float normal[3] = { -1.0f / dfdx, -1.0f / dfdy, 1.0f };

					//float normal[3] = { 0.0f, 0.0f, 1.0f };
					Normalize(normal);
					memcpy(vert.normal, normal, sizeof(normal));
				}
			}
		}
	}
}

void GNDFile::SerializeNormal(IOArchive& Ar, float* normal)
{
	uint8_t packed[3] = { (int8_t)(normal[0] / 127.0f), (int8_t)(normal[1] / 127.0f), (int8_t)(normal[2] / 127.0f) };
	Ar << packed;

	if (Ar.IsLoading())
	{
		normal[0] = (float)packed[0] / 127.0f;
		normal[1] = (float)packed[1] / 127.0f;
		normal[2] = (float)packed[2] / 127.0f;
	}
}

inline void GNDFile::Normalize(float* vec)
{
	float magnitude = sqrtf(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
	if (magnitude <= 0.0f) return;
	vec[0] /= magnitude;
	vec[1] /= magnitude;
	vec[2] /= magnitude;
}
