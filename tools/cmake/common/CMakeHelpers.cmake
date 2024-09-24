#function to get Specification version from flag
function(GetSpecVersion INPUT OUTPUT)
    # Use a regular expression to match the pattern x_x and
    # get version from flag
    string(REGEX MATCH "[0-9]+_[0-9]+" version_part ${INPUT})

    # Replace underscores with dots
    string(REPLACE "_" "." temp ${version_part})

    # Convert to lowercase
    string(TOLOWER ${temp} ${OUTPUT})

    # Set the output variable in the parent scope
    set(${OUTPUT} ${${OUTPUT}} PARENT_SCOPE)
endfunction()


function(ExtractandDefineRmiRsiVersion version_string)
    # Use a regular expression to extract major and minor version numbers
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)$" _ ${version_string})

    # If the match was successful, the captured groups will be stored in CMAKE_MATCH_1 and CMAKE_MATCH_2
    if(CMAKE_MATCH_1 OR CMAKE_MATCH_2)
        set(MAJOR_VERSION ${CMAKE_MATCH_1})
        set(MINOR_VERSION ${CMAKE_MATCH_2})

        # Add definitions for major and minor versions
        add_definitions(-DRMI_ABI_VERSION_MAJOR=${MAJOR_VERSION})
        add_definitions(-DRMI_ABI_VERSION_MINOR=${MINOR_VERSION})

        add_definitions(-DRSI_ABI_VERSION_MAJOR=${MAJOR_VERSION})
        add_definitions(-DRSI_ABI_VERSION_MINOR=${MINOR_VERSION})

    else()
        message(FATAL_ERROR "Invalid version string format: ${version_string}")
    endif()
endfunction()

function(GetLatestVer result_var)
    # Initialize an empty variable to hold the latest version
    set(latest_version "")

    # Iterate over all arguments passed to the function
    foreach(version IN LISTS ARGN)
        GetSpecVersion(${version} spec_ver)
        # Compare current version with the latest version found so far
        if(latest_version STREQUAL "" OR spec_ver VERSION_GREATER latest_version)
            set(latest_version ${version})
        endif()
    endforeach()

    GetSpecVersion(${latest_version} spec_ver)
    # Set the result variable in the parent scope
    set(${result_var} ${spec_ver} PARENT_SCOPE)
endfunction()

# Functionn to check the flag and add definitions accordingly
function(CheckSpecVersionAndAddDefinitions FLAG)
    # List of possible values
    set(POSSIBLE_VALUES "RMM_V_1_0" "RMM_V_1_1") # Add more values as spec evolves

    # If the flag is not defined or set to ALL, add definitions for all values
    if(NOT DEFINED FLAG OR ${FLAG} STREQUAL "ALL")
        message(STATUS "[ACS] : RMM_SPEC_VER Flag not set or set to ALL, defaulting to ALL")
        foreach(VALUE IN LISTS POSSIBLE_VALUES)
            add_definitions(-D${VALUE})
        endforeach()
        GetLatestVer(latest_ver ${POSSIBLE_VALUES})
        ExtractandDefineRmiRsiVersion(${latest_ver})
    else()
        # Check if the flag is one of the possible values
        list(FIND POSSIBLE_VALUES ${FLAG} INDEX)
        if(${INDEX} GREATER -1)
            add_definitions(-D${FLAG})
            GetSpecVersion(${FLAG} SPEC_VER)
            message(STATUS "[ACS] : ACS is being built for RMM v${SPEC_VER} Specification")
            ExtractandDefineRmiRsiVersion(${SPEC_VER})
        else()
            message(FATAL_ERROR "[ACS] Unknown value for RMM_SPEC_VER: ${FLAG}")
        endif()
    endif()
endfunction()

