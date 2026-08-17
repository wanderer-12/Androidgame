# Third-party notices

## stb_image

`app/src/main/cpp/third_party/stb_image.h` is from the stb single-file public domain/MIT image
library. Its full license text is included at the end of that header.

## AndroidX Games Activity

The headers and per-ABI static libraries under
`app/src/main/cpp/third_party/game-activity/` are from AndroidX Games Activity 4.0.0 and are
licensed under the Apache License 2.0. They are vendored because the Android Gradle Plugin 9.3.1
Prefab launcher does not execute correctly in this Windows environment. The Java dependency remains
declared as `androidx.games:games-activity:4.0.0`.
