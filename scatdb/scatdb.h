#ifndef SDBR_MAIN
#define SDBR_MAIN

#include "defs.h"
#include <stdbool.h>
#include "data.h"
#include <stdint.h>

/// These functions have C-style linkage
#ifdef __cplusplus
extern "C" {
#endif

	/// This is an opaque pointer used throughout the c-style code.
	typedef void* SDBR_HANDLE;

	/// Deallocates a handle
	bool DLEXPORT_SDBR SDBR_free(SDBR_HANDLE);

	/// Returns the length of the last error message
	int DLEXPORT_SDBR SDBR_err_len();

	/// \brief Copies the last error message to the specified character array.
	/// \param maxlen is the maximum number of characters to write (including the
	/// null character).
	/// \param buffer is the output array.
	/// \returns The number of characters written.
	uint64_t DLEXPORT_SDBR SDBR_err_msg(uint64_t maxlen, char* buffer);

	/// Execute library load tasks
	bool DLEXPORT_SDBR SDBR_start(int argc, char** argv);

	/** \brief Load the database
	 * \param dbfile is a null-terminated string that overrides
	 * the path to the file. If NULL, then
	 * the file is automatically detected using the builtin search paths (see 
	 * the log for a list of the paths that are searched).
	 * \returns A handle to the opened database object.
	 **/
	SDBR_HANDLE DLEXPORT_SDBR SDBR_loadDB(const char* dbfile);

	/** \brief Find the database file
	 * \param maxlen is the maximum number of characters to write (including the
	 * null character).
	 * \param buffer is the output array.
	 * \returns The number of characters written. If zero, the search was unsuccessful.
	 **/
	uint64_t DLEXPORT_SDBR SDBR_findDB(uint64_t maxlen, char* buffer);

	/// Write the database
	/// \param handle is the database handle
	/// \param outfile is the null-terminated output filename
	/// \returns indicates success of operation. See error code if return is false.
	bool DLEXPORT_SDBR SDBR_writeDBtext(SDBR_HANDLE handle, const char* outfile);
	//bool DLEXPORT_SDBR SDBR_writeDBHDF5(SDBR_HANDLE handle, const char* outfile);
	//bool DLEXPORT_SDBR SDBR_writeDBHDF5path(SDBR_HANDLE handle, const char* outfile, const char* hdfinternalpath);

	/// Get number of entries in database
	uint64_t DLEXPORT_SDBR SDBR_getNumRows(SDBR_HANDLE handle);

	/// Get the memory requirements to copy the database float table.
	void DLEXPORT_SDBR SDBR_getFloatTableSize(SDBR_HANDLE db, uint64_t *numFloats, uint64_t *numBytes);

	/// Get the database float table.
	/// \param db is the pointer to the loaded database.
	/// \param p is a pointer to the region of memory which will hold the table. The float table
	/// is copied in row-major form (i.e. each record's columns are all together).
	/// \param maxsize is the maximum size (in bytes!) of the holding region.
	/// \note This memory region is managed by the calling program. SDBR_free is unnecessary.
	/// \returns a bool which indicates whether the data are fully copied.
	bool DLEXPORT_SDBR SDBR_getFloatTable(SDBR_HANDLE db, float* p, uint64_t maxsize); 

	/// Get the memory requirements to copy the database int table
	void DLEXPORT_SDBR SDBR_getIntTableSize(SDBR_HANDLE db, uint64_t *numInts, uint64_t *numBytes);

	/// Get the database integer table.
	/// Same options as for the float table.
	bool DLEXPORT_SDBR SDBR_getIntTable(SDBR_HANDLE db, uint64_t* p, uint64_t maxsize);

	/// Sort the database according to an axis
	//SDBR_HANDLE DLEXPORT_SDBR SDBR_sortByFloat(SDBR_HANDLE db, data_entries_floats col_id);

	/// Filter the database according to a floating-point column
	//SDBR_HANDLE DLEXPORT_SDBR SDBR_filterFloatByString(SDBR_HANDLE db, data_entries_floats col_id, const char* strFilter);
	//SDBR_HANDLE DLEXPORT_SDBR SDBR_filterFloatByRange(SDBR_HANDLE db, data_entries_floats col_id, float minVal, float maxVal);

	/// Filter the database according to an integer column
	//SDBR_HANDLE DLEXPORT_SDBR SDBR_filterIntByString(SDBR_HANDLE db, data_entries_ints col_id, const char* strFilter);
	//SDBR_HANDLE DLEXPORT_SDBR SDBR_filterIntByRange(SDBR_HANDLE db, data_entries_ints col_id, uint64_t minVal, uint64_t maxVal);

	/// Get the statistics table
	/// \param db is the pointer to the data being summarized.
	/// \param p is the pointer to the region of memory which will hold the table (floating point),
	/// which is copied over in row-major form.
	/// \param maxsize is the maximum size (in bytes!) of the destination array.
	/// \param count (pointer) returns the number of points over which the statistics were calculated.
	/// \returns a bool which indicates whether the data are fully copied.
	bool DLEXPORT_SDBR SDBR_getStats(SDBR_HANDLE db, float *p, uint64_t maxsize, uint64_t *count);

	/// Get the size of the statistics table (in number of floats and in bytes).
	/// This is a convenience function to help with memory allocations.
	/// \param numFloats is the number of floats that must be allocated (pointer).
	/// \param bytes is the size of the array, in bytes (pointer).
	void DLEXPORT_SDBR SDBR_getStatsTableSize(uint64_t *numFloats, uint64_t *bytes);

	/// Get the size of the given phase function table

	/// Get the phase function table

/// Ensure that C-style linkage is respected
#ifdef __cplusplus
}
#endif

#endif

