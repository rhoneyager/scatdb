#pragma once
#include "defs.hpp"
namespace scatdb {
	namespace error {
		enum class error_types {
			xDivByZero,
			xInvalidRange,
			xAssert,
			xBadInput,
			xNullPointer,
			xModelOutOfRange,
			xMissingFrequency,
			xEmptyInputFile,
			xMissingFolder,
			xMissingFile,
			xMissingVariable,
			xMissingConfigurationFile,
			xFileExists,
			xPathExistsWrongType,
			xTypeMismatch,
			xUnknownFileFormat,
			xMissingKey,
			xMissingHash,
			xKeyExists,
			xUnimplementedFunction,
			xFallbackTemplate,
			xArrayOutOfBounds,
			xDimensionMismatch,
			xSingular,
			xDLLversionMismatch,
			xDuplicateHook,
			xHandleInUse,
			xHandleNotOpen,
			xSymbolNotFound,
			xBadFunctionMap,
			xBadFunctionReturn,
			xBlockedHookLoad,
			xBlockedHookUnload,
			xDLLerror,
			xUpcast,
			xCannotFindReference,
			xOtherError,
			xUnsupportedIOaction
		};
		HIDDEN_SDBR const char* stringify(error_types);
	}
}
