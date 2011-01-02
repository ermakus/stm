CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET (CMAKE_SYSTEM_NAME Generic)
SET (CMAKE_SYSTEM_VERSION 1)
SET (CMAKE_SYSTEM_PROCESSOR arm)

SET (CMAKE_C_COMPILER arm-apple-darwin9-gcc)
SET (CMAKE_CXX_COMPILER arm-apple-darwin9-g++)

SET (CMAKE_C_FLAGS "-ggdb")
SET (CMAKE_CXX_FLAGS "-ggdb")
SET (CMAKE_EXE_LINKER_FLAGS "-framework Foundation -framework CoreFoundation -framework UIKit -framework QuartzCore -framework CrashReporter -L/home/anton/iphone/lib -w -bind_at_load")

PROJECT (setup)
AUX_SOURCE_DIRECTORY(. SOURCES)
ADD_EXECUTABLE(setup ${SOURCES})
TARGET_LINK_LIBRARIES(setup)
