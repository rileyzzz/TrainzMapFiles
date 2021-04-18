// TrainzMapFiles.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "GNDFile.h"
#include "MapRenderer.h"

int main(int argc, char* argv[])
{
	//GNDFile testfile("G:/TrainzMapFormat/4baseboard.gnd");
	time_t start = clock();
	//GNDFile testfile("G:/Games/N3V/trs19/build hhl1hrpw1/editing/kuid 217537 100007 Mountains and Small C/mapfile.gnd");
	//GNDFile testfile("G:/Trainz1.1/InstallEnglish/rips/English_countryside/Maps/Britain/Britain.gnd");
	GNDFile testfile("G:/Games/N3V/trs19/build hhl1hrpw1/editing/kuid 414976 101216 map data tests/mapfile.gnd");
	time_t length = clock() - start;

	printf("Loaded in %f seconds\n", (double)length / CLOCKS_PER_SEC);
	//GNDFile testfile("G:/TrainzMapFormat/date.gnd");
	//GNDFile testfile("G:/TrainzMapFormat/sodor.gnd");
	//GNDFile testfile("G:/TrainzMapFormat/thomas' branch.gnd");

	MapRenderer rend(testfile);
	rend.Start();

	return 0;
}
