# ----------------------------------------------------------------------------- #
# Finishes the standalone macOS .app bundle following the "Shipping on MacOS"
# section of README.md: installs the icon and Info.plist and (optionally) code
# signs the bundle. Run as a POST_BUILD step via `cmake -P`.
#
# Expects: APP_BUNDLE, ASSETS_DIR, and optionally CODESIGN_IDENTITY.
# ----------------------------------------------------------------------------- #

message(STATUS "[WAIVE-FRONT] Packaging app bundle: ${APP_BUNDLE}")

# 1. Icon -> Contents/Resources/Icon.icns
message(STATUS "[WAIVE-FRONT]   - installing Icon.icns into Contents/Resources")
file(MAKE_DIRECTORY "${APP_BUNDLE}/Contents/Resources")
file(COPY "${ASSETS_DIR}/Icon.icns" DESTINATION "${APP_BUNDLE}/Contents/Resources")

# 2. Info.plist -> Contents/Info.plist (overwrites the CMake-generated one)
message(STATUS "[WAIVE-FRONT]   - installing Info.plist into Contents")
configure_file("${ASSETS_DIR}/Info-Standalone.plist" "${APP_BUNDLE}/Contents/Info.plist" COPYONLY)

# 3. Code signing (only when an identity is provided)
if(CODESIGN_IDENTITY)
    message(STATUS "[WAIVE-FRONT]   - code signing with identity: ${CODESIGN_IDENTITY}")
    execute_process(
        COMMAND codesign --deep --force --options=runtime
                --entitlements "${ASSETS_DIR}/entitlements.plist"
                --sign "${CODESIGN_IDENTITY}" --timestamp "${APP_BUNDLE}"
        RESULT_VARIABLE sign_result)

    if(NOT sign_result EQUAL 0)
        message(FATAL_ERROR "[WAIVE-FRONT]   - code signing FAILED (exit ${sign_result})")
    endif()

    message(STATUS "[WAIVE-FRONT]   - verifying signature")
    execute_process(
        COMMAND codesign --verify --deep --strict --verbose=2 "${APP_BUNDLE}"
        RESULT_VARIABLE verify_result)

    if(NOT verify_result EQUAL 0)
        message(FATAL_ERROR "[WAIVE-FRONT]   - signature verification FAILED (exit ${verify_result})")
    endif()

    message(STATUS "[WAIVE-FRONT]   - signed and verified. Next: notarize (see README 'Shipping on MacOS').")
else()
    message(STATUS "[WAIVE-FRONT]   - no MACOS_CODESIGN_IDENTITY set; skipping code signing")
    message(STATUS "[WAIVE-FRONT]     to sign, reconfigure with -DMACOS_CODESIGN_IDENTITY=<hash>")
    message(STATUS "[WAIVE-FRONT]     (find it with: security find-identity -p basic -v)")
endif()

message(STATUS "[WAIVE-FRONT] App bundle ready: ${APP_BUNDLE}")
