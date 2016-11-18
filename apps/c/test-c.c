#include <stdio.h>
#include <stdlib.h>

#include "../../scatdb/scatdb.h"

void printError() {
#define BUFLEN 1024
	//const int buflen = 1024;
	char buffer[BUFLEN];
	SDBR_err_msg(BUFLEN, buffer);
	printf("Error message:\n%s", buffer);
#undef BUFLEN
}

int main(int argc, char** argv) {
	int retval = 0;
	SDBR_HANDLE hdb = 0, hdbFiltA = 0, hdbFiltB = 0;
	float *summaryTable = 0, *pfTable = 0;

	printf("Calling SDBR_start\n");
	bool resb= SDBR_start(argc, argv);
	if (!resb) {
		printf("Library startup failed!\n");
		printError();
		retval = 1;
		goto freeObjs;
	}

	// Load the database
	printf("Loading the database.\n");
		hdb = SDBR_loadDB(0);
	if (!hdb) {
		printf("Cannot load database!\n");
		printError();
		retval = 2;
		goto freeObjs;
	}

	// Count the number of lines in the database
	uint64_t numEntries = SDBR_getNumRows(hdb);
	printf("The database has %lld rows.\n", numEntries);

	// Filter the database based on flake type
	const char* fts = "3,4,5,6,20";
	hdbFiltA = SDBR_filterIntByString(hdb, SDBR_FLAKETYPE, fts);
	if (!hdbFiltA) {
		printError();
		retval = 4;
		goto freeObjs;
	}
	// Filter the database based on frequency
	hdbFiltB = SDBR_filterFloatByRange(hdbFiltA, SDBR_FREQUENCY_GHZ, 13.f, 14.f);
	if (!hdbFiltB) {
		printError();
		retval = 4;
		goto freeObjs;
	}

	// Summarize the database
	// Get the size of the summary table
	int64_t statsNumFloats = 0, statsNumBytes = 0, count = 0;
	SDBR_getStatsTableSize(&statsNumFloats, &statsNumBytes);
	summaryTable = malloc((size_t) statsNumBytes);
	if (!summaryTable) {
		printf("Malloc error.\n");
		retval = 5;
		goto freeObjs;
	}
	if (!SDBR_getStats(hdbFiltB, summaryTable, statsNumBytes, &count)) {
		printf("Error retreiving stats.\n");
		printError();
		retval = 6;
		goto freeObjs;
	}
	printf("There are %lld points used to calculate the statistics.\n\n", count);
	for (int i=0; i<SDBR_NUM_DATA_ENTRIES_STATS; ++i) {
		printf("\t%s", SDBR_stringifyStatsColumn(i));
	}
	printf("\n");
	int k=0;
	for (int j=0; j<SDBR_NUM_DATA_ENTRIES_FLOATS; ++j) {
		printf("%s", SDBR_stringifyFloatsColumn(j));
		for (int i=0; i<SDBR_NUM_DATA_ENTRIES_STATS; ++i) {
			printf("\t%f", summaryTable[k]);
			++k;
		}
		printf("\n");
	}

	// Get the phase function of the very first filtered entry.

	// ... and display it


	// Write the database
	printf("Writing the database to outdb.csv\n");
	resb = SDBR_writeDBtext(hdb, "outdb.csv");
	if (!resb) {
		printf("Cannot write database to outdb.csv!\n");
		printError();
		retval = 3;
		goto freeObjs;
	}

freeObjs:
	// Close handles
	printf("Closing handles.\n");
	if (hdb) SDBR_free(hdb);
	if (hdbFiltA) SDBR_free(hdbFiltA);
	if (hdbFiltB) SDBR_free(hdbFiltB);
	if (summaryTable) free(summaryTable);
	if (pfTable) free(pfTable);

	printf("Finished with return code %d.\n", retval);
	return retval;
}

