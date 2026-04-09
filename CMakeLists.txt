cmake_minimum_required(VERSION 3.19)
project(PawnREST LANGUAGES C CXX VERSION 1.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_SAMP_PLUGIN "Build for SA-MP" OFF)
option(PAWNREST_ENABLE_TLS "Enable TLS/HTTPS support via OpenSSL" OFF)
option(PAWNREST_TLS_STATIC_OPENSSL "Link OpenSSL statically when TLS is enabled" OFF)
option(PAWNREST_OPTIMIZE_SIZE "Optimize binary size for Release/MinSizeRel builds" ON)
option(PAWNREST_STRIP_SYMBOLS "Strip symbols in GNU/Clang Release/MinSizeRel builds" ON)

# SA-MP build
if(BUILD_SAMP_PLUGIN)
	message(STATUS "Building PawnREST for SA-MP")

	set(SAMPSDK_DIR deps/samp-plugin-sdk)

	# getch.c is POSIX-only
	if(WIN32)
		set(SAMPSDK_SOURCES
			${SAMPSDK_DIR}/amxplugin.cpp
			${SAMPSDK_DIR}/amxplugin2.cpp
		)
	else()
		set(SAMPSDK_SOURCES
			${SAMPSDK_DIR}/amxplugin.cpp
			${SAMPSDK_DIR}/amxplugin2.cpp
			${SAMPSDK_DIR}/amx/getch.c
		)
	endif()

	add_library(${PROJECT_NAME} SHARED
		${SAMPSDK_SOURCES}
		src/entry_samp.cpp
	)

	target_include_directories(${PROJECT_NAME} PRIVATE
		${SAMPSDK_DIR}
		${SAMPSDK_DIR}/amx
		deps
		src
		${CMAKE_CURRENT_SOURCE_DIR}
	)

	target_compile_definitions(${PROJECT_NAME} PRIVATE
		PAWNREST_VERSION="${PROJECT_VERSION}"
		HAVE_STDINT_H=1
		PAWN_CELL_SIZE=32
		BUILD_SAMP_PLUGIN=1
	)

	set_target_properties(${PROJECT_NAME} PROPERTIES
		PREFIX ""
		OUTPUT_NAME "PawnREST"
	)

	if(WIN32)
		set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".dll")

		if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugin.def")
			target_sources(${PROJECT_NAME} PRIVATE plugin.def)
		endif()
	endif()

# open.mp build
else()
	message(STATUS "Building PawnREST for open.mp")

	add_subdirectory(deps/omp-sdk)

	add_library(${PROJECT_NAME} SHARED src/main.cpp)

	target_include_directories(${PROJECT_NAME} PRIVATE
		deps
		deps/pawn/source
		deps/pawn-natives
		src
	)

	if(UNIX AND NOT APPLE)
		target_include_directories(${PROJECT_NAME} PRIVATE
			deps/pawn/source/linux
		)
	endif()

	target_compile_definitions(${PROJECT_NAME} PRIVATE
		PAWNREST_VERSION="${PROJECT_VERSION}"
		HAVE_STDINT_H=1
		PAWN_CELL_SIZE=32
	)

	target_link_libraries(${PROJECT_NAME} PRIVATE OMP-SDK)

	set_target_properties(${PROJECT_NAME} PROPERTIES
		PREFIX ""
		OUTPUT_NAME "PawnREST"
	)
endif()

# Size optimization (applies to both builds)
if(PAWNREST_OPTIMIZE_SIZE)
	if(MSVC)
		target_compile_options(${PROJECT_NAME} PRIVATE
			"$<$<CONFIG:Release>:/O1;/Gw;/Gy;/GL>"
			"$<$<CONFIG:MinSizeRel>:/O1;/Gw;/Gy;/GL>"
		)
		target_link_options(${PROJECT_NAME} PRIVATE
			"$<$<CONFIG:Release>:/LTCG;/OPT:REF;/OPT:ICF>"
			"$<$<CONFIG:MinSizeRel>:/LTCG;/OPT:REF;/OPT:ICF>"
		)
	else()
		target_compile_options(${PROJECT_NAME} PRIVATE
			"$<$<CONFIG:Release>:-Os;-ffunction-sections;-fdata-sections>"
			"$<$<CONFIG:MinSizeRel>:-Os;-ffunction-sections;-fdata-sections>"
		)
		target_link_options(${PROJECT_NAME} PRIVATE
			"$<$<CONFIG:Release>:-Wl,--gc-sections>"
			"$<$<CONFIG:MinSizeRel>:-Wl,--gc-sections>"
		)
		if(PAWNREST_STRIP_SYMBOLS)
			target_link_options(${PROJECT_NAME} PRIVATE
				"$<$<CONFIG:Release>:-s>"
				"$<$<CONFIG:MinSizeRel>:-s>"
			)
		endif()
	endif()
endif()

# TLS support (applies to both builds)
if(PAWNREST_ENABLE_TLS)
	if(PAWNREST_TLS_STATIC_OPENSSL)
		set(OPENSSL_USE_STATIC_LIBS TRUE)
	endif()

	find_package(OpenSSL QUIET)
	if(OpenSSL_FOUND)
		target_compile_definitions(${PROJECT_NAME} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT PAWNREST_HAS_SSL=1)
		target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
	else()
		message(WARNING "OpenSSL not found. Building without TLS/HTTPS support.")
		target_compile_definitions(${PROJECT_NAME} PRIVATE PAWNREST_HAS_SSL=0)
	endif()
else()
	target_compile_definitions(${PROJECT_NAME} PRIVATE PAWNREST_HAS_SSL=0)
endif()
