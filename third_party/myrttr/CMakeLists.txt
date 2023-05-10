####################################################################################
#                                                                                  #
#  Copyright (c) 2014 - 2018 Axel Menzel <info@rttr.org>                           #
#                                                                                  #
#  This file is part of RTTR (Run Time Type Reflection)                            #
#  License: MIT License                                                            #
#                                                                                  #
#  Permission is hereby granted, free of charge, to any person obtaining           #
#  a copy of this software and associated documentation files (the "Software"),    #
#  to deal in the Software without restriction, including without limitation       #
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,        #
#  and/or sell copies of the Software, and to permit persons to whom the           #
#  Software is furnished to do so, subject to the following conditions:            #
#                                                                                  #
#  The above copyright notice and this permission notice shall be included in      #
#  all copies or substantial portions of the Software.                             #
#                                                                                  #
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      #
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        #
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     #
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          #
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   #
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   #
#  SOFTWARE.                                                                       #
#                                                                                  #
####################################################################################

project(rttr LANGUAGES CXX)

message(STATUS "Scanning "  ${PROJECT_NAME} " module.")
message(STATUS "===========================")

# loadFolder("rttr" HPP_FILES SRC_FILES BUILD_INSTALLER)

# if (USE_PCH)
#     activate_precompiled_headers("detail/base/pch.h" SRC_FILES)
# endif()

file(GLOB SRC_FILES "**/*.cpp")
file(GLOB HPP_FILES "**/*.hpp")
# set(UnityBuild )

option(BUILD_STATIC "Build RTTR as static library" ON)

if (BUILD_STATIC)
    # aux_source_directory(. SRC)
    file(GLOB_RECURSE SRC ./*.cpp)
    add_library(myrttr_core_lib STATIC ${SRC})
    # add_library(RTTR::Core_Lib ALIAS rttr_core_lib)

    

    # set_target_properties(rttr_core_lib PROPERTIES
    #                     #   VERSION ${RTTR_VERSION} SOVERSION ${RTTR_VERSION}
    #                       EXPORT_NAME Core_Lib
    #                     #   CXX_STANDARD ${MAX_CXX_STANDARD}
    #                     #   DEBUG_POSTFIX ${RTTR_DEBUG_POSTFIX}
    #                       )                      
    target_link_libraries(myrttr_core_lib PRIVATE ${CMAKE_DL_LIBS})
    
    # if (MSVC)
    #     set_target_properties(rttr_core_lib PROPERTIES OUTPUT_NAME librttr_core)
    # elseif (CMAKE_COMPILER_IS_GNUCXX)
    #     set_target_properties(rttr_core_lib PROPERTIES OUTPUT_NAME rttr_core)
    # elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    #     set_target_properties(rttr_core_lib PROPERTIES OUTPUT_NAME rttr_core)
    # else()
    #     message(SEND_ERROR "Do not know how to name the static library.")
    # endif()
    
    target_include_directories(myrttr_core_lib PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../>
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
                                $<INSTALL_INTERFACE:include>
                                ${HOME_SRC}/third_party)
    
    # if(${CMAKE_VERSION} VERSION_GREATER "3.8.0") 
    #     target_compile_features(rttr_core_lib PUBLIC cxx_std_11) # at least c++11 is needed to compile RTTR
    # endif()
    
    # set_compiler_warnings(rttr_core_lib)

    # if (BUILD_INSTALLER)
    #     install(TARGETS rttr_core_lib EXPORT rttr_targets
    #             RUNTIME       DESTINATION ${RTTR_RUNTIME_INSTALL_DIR}
    #             LIBRARY       DESTINATION ${RTTR_LIBRARY_INSTALL_DIR}
    #             ARCHIVE       DESTINATION ${RTTR_ARCHIVE_INSTALL_DIR}
    #             FRAMEWORK     DESTINATION ${RTTR_FRAMEWORK_INSTALL_DIR})
    # endif()
endif()

message(STATUS "Scanning " ${PROJECT_NAME} " module finished!")
message(STATUS "")
