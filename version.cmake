# Do not change this value
set(RADAHN_VERSION_MAJOR  0     CACHE NUMBER "Major Version Number")

# Change this if you have added a new feature
set(RADAHN_VERSION_MINOR  0     CACHE NUMBER "Minor Version Number")

# Change this if you have fixed a bug
set(RADAHN_VERSION_PATCH  1     CACHE NUMBER "Patch NUMBER")

execute_process(COMMAND            git rev-list --count HEAD
                WORKING_DIRECTORY  ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE    git_commit_count
                OUTPUT_STRIP_TRAILING_WHITESPACE
)

# This is only changed through the CI system
set(RADAHN_VERSION_BUILD  ${git_commit_count}     CACHE NUMBER "Build version number. This value should be equal to the number of git commits using: git rev-list --count HEAD")
set(RADAHN_VERSION_STRING ${RADAHN_VERSION_MAJOR}.${RADAHN_VERSION_MINOR}.${RADAHN_VERSION_PATCH}.${RADAHN_VERSION_BUILD} CACHE STRING "The final version number")