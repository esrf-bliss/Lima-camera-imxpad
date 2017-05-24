# CMakeLists.txt for ImXPAD camera

SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--add-needed")

SET(imxpad_srcs src/imXpadCamera.cpp  src/imXpadInterface.cpp
	 src/imXpadDetInfoCtrlObj.cpp src/imXpadSyncCtrlObj.cpp
	 src/imXpadClient.cpp)
	 
FILE(STRINGS "VERSION" imxpad_vers)
ADD_LIBRARY(limaimxpad SHARED ${imxpad_srcs})
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWITH_CONFIG")

#INCLUDES
#TARGET_INCLUDE_DIRECTORIES(limaimxpad PUBLIC "${CMAKE_SOURCE_DIR}/camera/common/espia/include")
TARGET_INCLUDE_DIRECTORIES(limaimxpad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
TARGET_INCLUDE_DIRECTORIES(limaimxpad PUBLIC "${CMAKE_SOURCE_DIR}/third-party/libconfig/lib")


#LIBRARIES
#

TARGET_LINK_LIBRARIES(limaimxpad limacore)

SET_TARGET_PROPERTIES(limaimxpad PROPERTIES VERSION ${imxpad_vers} SOVERSION "1")

INSTALL(TARGETS limaimxpad LIBRARY DESTINATION lib)

IF(COMPILE_SIP)
    SET(NAME "imxpad")
    SET(INCLUDES)
    FILE(GLOB sipfiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/sip" "${CMAKE_CURRENT_SOURCE_DIR}/sip/*.sip")
    FOREACH(sipfile ${sipfiles})
        SET(INCLUDES 
        "${INCLUDES} 
        %Include ${sipfile}")
    ENDFOREACH()

    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/sip/limamodules.sip.in sip/limaimxpad.sip)
    SET(SIP_CONCAT_PARTS 1)
    SET(SIP_INCLUDES ${SIP_INCLUDES}
        "${CMAKE_SOURCE_DIR}/third-party/Processlib/sip"
        "${CMAKE_BINARY_DIR}/sip/core"
        "${CMAKE_SOURCE_DIR}/third-party/Processlib/tasks/sip"
        "${CMAKE_SOURCE_DIR}/common/sip"
        "${CMAKE_SOURCE_DIR}/hardware/sip"
        "${CMAKE_SOURCE_DIR}/control/sip"
        "${CMAKE_SOURCE_DIR}/control/software_operation/sip"
        "${CMAKE_CURRENT_SOURCE_DIR}/sip")
    set(SIP_DISABLE_FEATURES WITH_CONFIG)
    set(SIP_CHECK_EXC ON)
    ADD_SIP_PYTHON_MODULE(limaimxpad ${CMAKE_CURRENT_BINARY_DIR}/sip/limaimxpad.sip)
    TARGET_INCLUDE_DIRECTORIES(python_module_limaimxpad PRIVATE
                               ${PYTHON_INCLUDE_DIRS}
                               "${CMAKE_SOURCE_DIR}/sip"
                               "${CMAKE_SOURCE_DIR}/sip/core")
    TARGET_LINK_LIBRARIES(python_module_limaimxpad limaimxpad)

    INSTALL(FILES python/__init__.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/ImXPAD")
ENDIF()
