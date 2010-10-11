# Berkelium Build Script
#
# Parameters:
#  BERKELIUM_CHROME_ROOT_SEARCH_PATHS - list of paths to search for berkelium in.  Should be absolute paths.
#
# Outputs:
#  BERKELIUM_CHROME_FOUND - TRUE if Chrome is found and berkelium will be built.

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

IF(APPLE)
  SET(CMAKE_CXX_COMPILER g++-4.0)
  SET(CMAKE_C_COMPILER gcc-4.0)
ENDIF()

SET(CMAKE_VERBOSE_MAKEFILE OFF)
SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)
IF(COMMAND CMAKE_POLICY)
  # Use new policy for absolute path libraries.  We have no reason to depend on the
  # old version which adds library search paths unnecessarily
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

SET(BERKELIUM_TOP_LEVEL ${CMAKE_CURRENT_SOURCE_DIR})
GET_FILENAME_COMPONENT(BERKELIUM_TOP_LEVEL ${BERKELIUM_TOP_LEVEL} ABSOLUTE)

SET(CMAKE_MODULE_PATH
  ${BERKELIUM_TOP_LEVEL}
  ${BERKELIUM_TOP_LEVEL}/build/modules/
)


IF(NOT CHROME_ROOT)
  SET(CHROME_ROOT_SEARCH_PATHS
    ${BERKELIUM_CHROME_ROOT_SEARCH_PATHS} # User provided search paths go first
    # Followed by a few reasonable choices within this directory
    ${BERKELIUM_TOP_LEVEL}/chrome
    ${BERKELIUM_TOP_LEVEL}/chromium
    ${BERKELIUM_TOP_LEVEL}/build/chromium
    ${BERKELIUM_TOP_LEVEL}/build/c
    )
  FOREACH(CHROME_ROOT_SEARCH_PATH ${CHROME_ROOT_SEARCH_PATHS})
    IF(EXISTS ${CHROME_ROOT_SEARCH_PATH})
      IF(NOT CHROME_ROOT)
        SET(CHROME_ROOT ${CHROME_ROOT_SEARCH_PATH})
      ENDIF()
    ENDIF()
  ENDFOREACH()
ENDIF()
SET(CHROME_FOUND_QUIETLY TRUE)

FIND_PACKAGE(Chrome)

SET(BERKELIUM_CHROME_FOUND ${CHROME_FOUND})

IF(CHROME_FOUND)
  INCLUDE_DIRECTORIES(${BERKELIUM_TOP_LEVEL}/include ${CHROME_INCLUDE_DIRS})
  LINK_DIRECTORIES(${CHROME_LIBRARY_DIRS} ../lib .)
  SET(BERKELIUM_SOURCE_NAMES src/Berkelium src/Context src/Cursor src/ContextImpl src/ForkedProcessHook src/NavigationController src/RenderWidget src/MemoryRenderViewHost src/Root src/ScriptVariant src/Window src/WindowImpl)


  SET(BERKELIUM_SOURCES)
  FOREACH(BERKELIUM_SOURCE_FILE ${BERKELIUM_SOURCE_NAMES})
    SET(BERKELIUM_SOURCES ${BERKELIUM_SOURCES}   ${BERKELIUM_TOP_LEVEL}/${BERKELIUM_SOURCE_FILE}.cpp)
  ENDFOREACH()

  SET(CHROMIUM_LDFLAGS "")
  FOREACH(CHROME_LDFLAG ${CHROME_LDFLAGS})
    SET(CHROMIUM_LDFLAGS "${CHROMIUM_LDFLAGS} ${CHROME_LDFLAG}")
  ENDFOREACH()
  FOREACH(CHROME_CFLAG ${CHROME_CFLAGS})
    SET(CHROMIUM_CFLAGS "${CHROMIUM_CFLAGS} ${CHROME_CFLAG}")
  ENDFOREACH()
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual")
  IF(APPLE)
    ADD_DEFINITIONS(-isysroot /Developer/SDKs/MacOSX10.5.sdk -arch i386 -mmacosx-version-min=10.5 )
    SET(BERKELIUM_LDFLAGS "-Wl,-syslibroot -Wl,/Developer/SDKs/MacOSX10.5.sdk/")
  ENDIF()

  IF (NOT APPLE)
    ADD_CHROME_SYMLINK_TARGET()
  ENDIF()

  # libberkelium -- core berkelium library
  ADD_LIBRARY(libberkelium SHARED ${BERKELIUM_SOURCES})
  SET_TARGET_PROPERTIES(libberkelium PROPERTIES DEBUG_POSTFIX "_d")
  SET_TARGET_PROPERTIES(libberkelium PROPERTIES LINK_FLAGS "${CHROMIUM_LDFLAGS} ${BERKELIUM_LDFLAGS}")
  SET_TARGET_PROPERTIES(libberkelium PROPERTIES COMPILE_FLAGS "${CHROMIUM_CFLAGS}")
  SET_TARGET_PROPERTIES(libberkelium PROPERTIES DEBUG_POSTFIX "_d")
  TARGET_LINK_LIBRARIES(libberkelium ${CHROME_LIBRARIES})

  IF(APPLE)
    SET( CHROMIUM_DYLIB_EXT .dylib)
  ELSE()
    SET( CHROMIUM_DYLIB_EXT .so)
  ENDIF()

  # berkelium -- renderer binary
  ADD_EXECUTABLE(berkelium ${BERKELIUM_TOP_LEVEL}/subprocess.cpp)
  SET_TARGET_PROPERTIES(berkelium PROPERTIES LINK_FLAGS "${BERKELIUM_LDFLAGS}")
  TARGET_LINK_LIBRARIES(berkelium -L. debug liblibberkelium_d${CHROMIUM_DYLIB_EXT} optimized liblibberkelium${CHROMIUM_DYLIB_EXT} )
  ADD_DEPENDENCIES(berkelium libberkelium)

  # Useful values for setting up demo apps
  SET(BERKELIUM_LINK_LIBS -L. debug liblibberkelium_d${CHROMIUM_DYLIB_EXT} optimized liblibberkelium${CHROMIUM_DYLIB_EXT} )

  # ppmrender -- demo app
  ADD_EXECUTABLE(ppmrender ${BERKELIUM_TOP_LEVEL}/demo/ppmrender/ppmmain.cpp)
  TARGET_LINK_LIBRARIES(ppmrender ${BERKELIUM_LINK_LIBS})
  SET_TARGET_PROPERTIES(ppmrender PROPERTIES LINK_FLAGS "${BERKELIUM_LDFLAGS}")
  ADD_DEPENDENCIES(ppmrender libberkelium)
  IF(APPLE)
    ADD_CHROME_APP(
      APP ppmrender
      DEPENDS ppmrender plugin_carbon_interpose
      LINKS
      ${CMAKE_CURRENT_BINARY_DIR}/berkelium
      )
  ENDIF()

  # demo directory, so we can share some implementation between demos
  SET(DEMO_DIR ${BERKELIUM_TOP_LEVEL}/demo)

  # GL Demos
  FIND_PACKAGE(OpenGL)
  FIND_PACKAGE(GLUT)
  IF(OPENGL_FOUND AND GLUT_FOUND)
    # 2.4 Compatibility requires global include directory instead of target specific
    INCLUDE_DIRECTORIES(AFTER ${OPENGL_INCLUDE_DIR} ${GLUT_INCLUDE_DIR} ${DEMO_DIR})
  ENDIF()

  # glut_demo -- demo app
  IF(OPENGL_FOUND AND GLUT_FOUND)
    ADD_EXECUTABLE(glut_demo ${BERKELIUM_TOP_LEVEL}/demo/glut_demo/glut_demo.cpp)
    TARGET_LINK_LIBRARIES(glut_demo ${BERKELIUM_LINK_LIBS})
    SET_TARGET_PROPERTIES(glut_demo PROPERTIES LINK_FLAGS "${BERKELIUM_LDFLAGS}")
    TARGET_LINK_LIBRARIES(glut_demo ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
    ADD_DEPENDENCIES(glut_demo libberkelium)
    IF(APPLE)
      ADD_CHROME_APP(
        APP glut_demo
        DEPENDS glut_demo plugin_carbon_interpose
        LINKS
        ${CMAKE_CURRENT_BINARY_DIR}/berkelium
        )
    ENDIF()
  ENDIF()

  # glut_input -- glut demo app which also handles input
  IF(OPENGL_FOUND AND GLUT_FOUND)
    ADD_EXECUTABLE(glut_input ${BERKELIUM_TOP_LEVEL}/demo/glut_input/glut_input.cpp)
    TARGET_LINK_LIBRARIES(glut_input ${BERKELIUM_LINK_LIBS})
    SET_TARGET_PROPERTIES(glut_input PROPERTIES LINK_FLAGS "${BERKELIUM_LDFLAGS}")
    TARGET_LINK_LIBRARIES(glut_input ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
    ADD_DEPENDENCIES(glut_input libberkelium)
    IF(APPLE)
      ADD_CHROME_APP(
        APP glut_input
        DEPENDS glut_input plugin_carbon_interpose
        LINKS
        ${CMAKE_CURRENT_BINARY_DIR}/berkelium
        )
    ENDIF()
  ENDIF()

  IF(APPLE)
    ADD_LIBRARY(plugin_carbon_interpose SHARED ${CHROME_ROOT}/src/chrome/browser/plugin_carbon_interpose_mac.cc)
    SET_TARGET_PROPERTIES(plugin_carbon_interpose PROPERTIES COMPILE_FLAGS ${CHROMIUM_CFLAGS})
    SET_TARGET_PROPERTIES(plugin_carbon_interpose PROPERTIES LINK_FLAGS "-framework Carbon ${BERKELIUM_LDFLAGS}")
    TARGET_LINK_LIBRARIES(plugin_carbon_interpose -L. debug liblibberkelium_d.dylib optimized liblibberkelium.dylib )
    ADD_DEPENDENCIES(plugin_carbon_interpose libberkelium)

  ENDIF()

  INSTALL(TARGETS
          libberkelium
          berkelium
          RUNTIME
          DESTINATION bin
          LIBRARY
          DESTINATION lib)
  IF(NOT APPLE)
    INSTALL(FILES
          ${CHROMIUM_DATADIR}/chrome.pak
          ${CHROMIUM_DATADIR}/resources.pak
            DESTINATION bin)
    INSTALL(DIRECTORY ${CHROMIUM_DATADIR}/locales
        DESTINATION bin)
    INSTALL(DIRECTORY ${CHROMIUM_DATADIR}/resources
        DESTINATION bin)
  ENDIF()
ENDIF(CHROME_FOUND)

FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
  ADD_CUSTOM_TARGET(doc ${DOXYGEN_EXECUTABLE} "doc/Doxyfile")
ENDIF()
