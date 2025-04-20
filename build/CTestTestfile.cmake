# CMake generated Testfile for 
# Source directory: D:/Desktop/leapMotionApp/leapMotionApp
# Build directory: D:/Desktop/leapMotionApp/leapMotionApp/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(test_DataProcessor "D:/Desktop/leapMotionApp/leapMotionApp/build/Debug/test_DataProcessor.exe")
  set_tests_properties(test_DataProcessor PROPERTIES  _BACKTRACE_TRIPLES "D:/Desktop/leapMotionApp/leapMotionApp/CMakeLists.txt;242;add_test;D:/Desktop/leapMotionApp/leapMotionApp/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(test_DataProcessor "D:/Desktop/leapMotionApp/leapMotionApp/build/Release/test_DataProcessor.exe")
  set_tests_properties(test_DataProcessor PROPERTIES  _BACKTRACE_TRIPLES "D:/Desktop/leapMotionApp/leapMotionApp/CMakeLists.txt;242;add_test;D:/Desktop/leapMotionApp/leapMotionApp/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(test_DataProcessor "D:/Desktop/leapMotionApp/leapMotionApp/build/MinSizeRel/test_DataProcessor.exe")
  set_tests_properties(test_DataProcessor PROPERTIES  _BACKTRACE_TRIPLES "D:/Desktop/leapMotionApp/leapMotionApp/CMakeLists.txt;242;add_test;D:/Desktop/leapMotionApp/leapMotionApp/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(test_DataProcessor "D:/Desktop/leapMotionApp/leapMotionApp/build/RelWithDebInfo/test_DataProcessor.exe")
  set_tests_properties(test_DataProcessor PROPERTIES  _BACKTRACE_TRIPLES "D:/Desktop/leapMotionApp/leapMotionApp/CMakeLists.txt;242;add_test;D:/Desktop/leapMotionApp/leapMotionApp/CMakeLists.txt;0;")
else()
  add_test(test_DataProcessor NOT_AVAILABLE)
endif()
subdirs("_deps/glm-build")
subdirs("_deps/catch2-build")
subdirs("_deps/googletest-build")
subdirs("_deps/sdl2-build")
subdirs("_deps/nlohmann_json-build")
