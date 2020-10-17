cmake_minimum_required (VERSION 3.8)

project (tde2_introspector)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/$<CONFIGURATION>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/$<CONFIGURATION>")

# Global variables are declared here
set(TDE2_INTROSPECTOR_TESTS_NAME "tests")

# Global options are declared here
option(IS_PLUGIN_BUILDING_ENABLED "The option shows whether plugins should be built or not" ON)
option(IS_SAMPLES_BUILDING_ENABLED "The option shows whether sample projects should be built or not" ON)
option(IS_TESTING_ENABLED "The option turns on/off tests" ON)

set(EXECUTABLE_NAME "tde2_introspector")

set(HEADERS 
	"${CMAKE_CURRENT_SOURCE_DIR}/deps/argparse/argparse.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/archive.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/deps/PicoSHA2/picosha2.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/lexer.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/parser.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tokens.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/symtable.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/codegenerator.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/jobmanager.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/common.h")

set(SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/deps/argparse/argparse.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/source/common.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/source/lexer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/source/parser.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/source/tokens.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/source/symtable.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/source/codegenerator.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/source/jobmanager.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp")

source_group("includes" FILES ${HEADERS})
source_group("sources" FILES ${SOURCES})


if (MSVC) 	#cl.exe compiler's options

	#Debug compiler's options
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /std:c++17 /MDd /W3 /GS /Zc:inline /Od /ZI  /Zc:wchar_t")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /RTC1 /Gd /Oy- /EHsc /nologo /diagnostics:classic /errorReport:prompt /sdl /permissive- /analyze-")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D _DEBUG")

	#Release compiler's options
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /std:c++17 /permissive- /GS /GL /analyze- /W3 /Gy /Zc:wchar_t /ZI /Gm- /O2 /sdl /Zc:inline")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /fp:precise /D _WINDLL /D _MBCS /errorReport:prompt /WX- /Zc:forScope /Gd /Oy- /Oi /MD /EHsc /nologo /diagnostics:classic")

endif(MSVC)


if (UNIX)
	message(STATUS "UNIX system has detected...")

	include(CheckCXXCompilerFlag)

	CHECK_CXX_COMPILER_FLAG("-std=c++1z" COMPILER_SUPPORTS_CXX17)

	if(COMPILER_SUPPORTS_CXX17)
		message(STATUS "C++17 is enabled")

		set(CMAKE_CXX_STANDARD 17)
		set(CMAKE_CXX_STANDARD_REQUIRED ON)
	else()
		message(ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
	endif()
endif (UNIX)


add_executable(${EXECUTABLE_NAME} ${SOURCES} ${HEADERS})

if (UNIX)
	target_link_libraries(${EXECUTABLE_NAME} pthread)
endif()


if (IS_TESTING_ENABLED)
	enable_testing()
endif ()

if (IS_TESTING_ENABLED)
	add_subdirectory(tests)
endif ()