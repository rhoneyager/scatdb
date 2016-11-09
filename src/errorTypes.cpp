#include "../scatdb/errorTypes.hpp"
namespace scatdb {
	namespace error {
		const char* stringify(error_types et) {
			if (et == error_types::xDivByZero) return "Division by zero.";
			else if (et == error_types::xInvalidRange) return "Invalid range.";
			else if (et == error_types::xAssert) return "Assertion failed.";
			else if (et == error_types::xBadInput) return "Bad or nonsensical input.";
			else if (et == error_types::xNullPointer) return "An unexpected null pointer was encountered.";
			else if (et == error_types::xModelOutOfRange) return "Model out of range.";
			else if (et == error_types::xMissingFrequency) return "Missing data for this frequency.";
			else if (et == error_types::xEmptyInputFile) return "Input file is empty.";
			else if (et == error_types::xMissingFolder) return "Missing folder in path.";
			else if (et == error_types::xMissingFile) return "Missing file.";
			else if (et == error_types::xMissingVariable) return "Missing variable in a saved structure.";
			else if (et == error_types::xMissingConfigurationFile) return "Cannot find an appropriate configuration file.";
			else if (et == error_types::xFileExists) return "File to be opened for writing already exists.";
			else if (et == error_types::xPathExistsWrongType) return "Path exists, but wrong type (i.e. file vs. directory).";
			else if (et == error_types::xTypeMismatch) return "Variable does not have the correct type.";
			else if (et == error_types::xUnknownFileFormat) return "Unknown file format.";
			else if (et == error_types::xMissingKey) return "Key not found in map.";
			else if (et == error_types::xMissingHash) return "Cannot find hash to load.";
			else if (et == error_types::xKeyExists) return "Key already exists in map.";
			else if (et == error_types::xUnimplementedFunction) return "Unimplemented function.";
			else if (et == error_types::xFallbackTemplate) return "Reached point in code that is the fallback template. Missing the appropriate template specialization.";
			else if (et == error_types::xArrayOutOfBounds) return "An array went out of bounds.";
			else if (et == error_types::xDimensionMismatch) return "An array went out of bounds.";
			else if (et == error_types::xSingular) return "Singular matrix detected.";
			else if (et == error_types::xDLLversionMismatch) return "Attempting to load a DLL that is compiled against an incompatible library version.";
			else if (et == error_types::xDuplicateHook) return "Attempting to load same DLL twice.";
			else if (et == error_types::xHandleInUse) return "DLL handle is currently in use, but the code wants to overwrite it.";
			else if (et == error_types::xHandleNotOpen) return "Attempting to access unopened DLL handle.";
			else if (et == error_types::xSymbolNotFound) return "Cannot find symbol in DLL.";
			else if (et == error_types::xBadFunctionMap) return "DLL symbol function map table is invalid.";
			else if (et == error_types::xBadFunctionReturn) return "DLL symbol map table is invalid.";
			else if (et == error_types::xBlockedHookLoad) return "Another hook blocked the load operation (for DLLs).";
			else if (et == error_types::xBlockedHookUnload) return "Another DLL depends on the one that you are unloading.";
			else if (et == error_types::xDLLerror) return "Unspecified DLL error.";
			else if (et == error_types::xUpcast) return "Plugin failure: cannot cast base upwards to a derived class provided by a plugin. Usually means that no matching plugin can be found.";
			else if (et == error_types::xCannotFindReference) return "Cannot find reference in file.";
			else if (et == error_types::xOtherError) return "Nonspecific error.";
			else if (et == error_types::xUnsupportedIOaction) return "Unsupported IO action.";
			else return "INVALID - stringify does not have the appropriate error type!";
		}
	}
}
