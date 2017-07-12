# Allow subfolders in solution file
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


if (CMAKE_VERSION VERSION_LESS "3.1")
        message(WARNING "It is recommended to use at least CMake 3.1")
	set(DEFAULT_TARGET_OPTIONS
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF)
else ()
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CMAKE_CXX_EXTENSIONS OFF)
endif ()
