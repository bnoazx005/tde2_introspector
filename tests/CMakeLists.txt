cmake_minimum_required (VERSION 3.8)

project (tde2-introspector-tests CXX)

option(IS_TESTING_ENABLED "The option turns on/off tests" ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIGURATION>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIGURATION>")

if (NOT DEFINED ${TDE2_INTROSPECTOR_TESTS_NAME})
	set(TDE2_INTROSPECTOR_TESTS_NAME "tests")
endif ()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (IS_TESTING_ENABLED)
	enable_testing()
endif ()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include")

# include Catch2
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/lib/Catch2/contrib/")
add_subdirectory(lib/Catch2)

set(HEADERS 
	"${CMAKE_CURRENT_SOURCE_DIR}/mockInputStream.h")

set(SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/../source/lexer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../source/parser.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../source/tokens.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../source/symTable.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/lexerTests.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/parserTests.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/symTableTests.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/enumsExtractorTests.cpp")

source_group("includes" FILES ${HEADERS})
source_group("sources" FILES ${SOURCES})

if (MSVC) 	#cl.exe compiler's options

	#Debug compiler's options
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /std:c++14 /MDd /W3 /GS /Zc:inline /Od /ZI  /Zc:wchar_t")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /RTC1 /Gd /Oy- /EHsc /nologo /diagnostics:classic /errorReport:prompt /sdl /permissive- /analyze-")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D _DEBUG")

	#Release compiler's options
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /std:c++14 /permissive- /GS /GL /analyze- /W3 /Gy /Zc:wchar_t /ZI /O2 /sdl /Zc:inline")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /fp:precise /D _WINDLL /D _MBCS /errorReport:prompt /WX- /Zc:forScope /Gd /Oy- /Oi /MD /EHsc /nologo /diagnostics:classic")

endif(MSVC)


if (UNIX)
	message(STATUS "UNIX system has detected...")

	include(CheckCXXCompilerFlag)

	CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX14)

	if(COMPILER_SUPPORTS_CXX14)
		message(STATUS "C++14 is enabled")

		set(CMAKE_CXX_STANDARD 14)
		set(CMAKE_CXX_STANDARD_REQUIRED ON)
	else()
		message(ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
	endif()
endif (UNIX)

add_executable(${TDE2_INTROSPECTOR_TESTS_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${TDE2_INTROSPECTOR_TESTS_NAME} Catch2::Catch2)

include(CTest)
include(Catch)

catch_discover_tests(${TDE2_INTROSPECTOR_TESTS_NAME})