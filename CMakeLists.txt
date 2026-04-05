cmake_minimum_required(VERSION 3.19)
project(pawnrest LANGUAGES C CXX VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)
option(PAWNREST_ENABLE_TLS "Enable TLS/HTTPS support via OpenSSL" OFF)

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

if(PAWNREST_ENABLE_TLS)
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
