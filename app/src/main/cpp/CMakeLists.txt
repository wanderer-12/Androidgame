# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("androidgame")

add_library(game-activity-static STATIC IMPORTED)
set_target_properties(game-activity-static PROPERTIES
        IMPORTED_LOCATION
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/game-activity/libs/${ANDROID_ABI}/libgame-activity_static.a"
        INTERFACE_INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/game-activity/include")

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(androidgame SHARED
        main.cpp
        AndroidOut.cpp
        Game.cpp
        Renderer.cpp
        TextureAsset.cpp)

# Forces the linker to keep the JNI entry point for GameActivity
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

# Configure libraries CMake uses to link your target library.
target_link_libraries(androidgame
        # The game activity
        game-activity-static

        # EGL and other dependent libraries required for drawing
        # and interacting with Android system
        EGL
        GLESv3
        android
        log)
