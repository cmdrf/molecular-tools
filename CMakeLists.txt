add_library(stb INTERFACE)
target_include_directories(stb INTERFACE SYSTEM 3rdparty/stb)
add_library(tinyexr INTERFACE)
target_include_directories(tinyexr INTERFACE SYSTEM 3rdparty)

# ETC Texture Compressor
add_executable(etccompress
	EtcCompressMain.cpp
	3rdparty/etc/rg_etc1.cpp
	3rdparty/etc/etc2_encoder.cpp
)

target_link_libraries(etccompress
	stb
	molecular-util
)

set_source_files_properties(
	3rdparty/etc/rg_etc1.cpp
	3rdparty/etc/etc2_encoder.cpp
	PROPERTIES COMPILE_FLAGS -O3 # Also optimize on debug builds
)

# meshinfo
add_executable(molecularmeshinfo
	MeshInfoMain.cpp
)
target_link_libraries(molecularmeshinfo
	molecular-util
	molecular-meshfile
)
