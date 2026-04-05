cmake_minimum_required(VERSION 3.19)
project(pawnrest LANGUAGES C CXX VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)
option(PAWNREST_ENABLE_TLS "Enable TLS/HTTPS support via OpenSSL" OFF)
option(PAWNREST_TLS_STATIC_OPENSSL "Link OpenSSL statically when TLS is enabled" OFF)
option(PAWNREST_OPTIMIZE_SIZE "Optimize binary size for Release/MinSizeRel builds" ON)
option(PAWNREST_STRIP_SYMBOLS "Strip symbols in GNU/Clang Release/MinSizeRel builds" ON)

add_subdirectory(deps/omp-sdk)

include_directories(
	deps
	deps/pawn/source
	deps/pawn/source/linux
	deps/pawn-natives
)

add_definitions(
	-DHAVE_STDINT_H=1
	-DPAWN_CELL_SIZE=32
)

add_library(${PROJECT_NAME} SHARED src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE OMP-SDK)

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

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
