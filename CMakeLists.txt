cmake_minimum_required(VERSION 3.2)

project(boost_asio_read_test_app VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})

include_directories(${PROJECT_SOURCE_DIR})

configure_file(version.h.in version.h)

add_library(boost_asio_read_test_common
        common/communications.h
        common/interface.h
        common/tcp/client_session.cpp
        common/tcp/server.cpp
        common/tcp/client.cpp
        )
target_link_libraries(boost_asio_read_test_common -lboost_system)

add_executable(boost_asio_read_test_app
        boost_asio_read_test_app/main.cpp
        )
target_link_libraries(boost_asio_read_test_app
        boost_asio_read_test_common
        -lpthread
        )

set_target_properties(boost_asio_read_test_common boost_asio_read_test_app PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        COMPILE_OPTIONS -Wpedantic -Wall -Wextra
        )

install(TARGETS boost_asio_read_test_common boost_asio_read_test_app
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT artyom.sorokoumov@gmail.com)

include(CPack)
