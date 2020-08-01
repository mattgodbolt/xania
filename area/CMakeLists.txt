configure_file(version.are.in version.are)
install(DIRECTORY . DESTINATION area
        FILES_MATCHING PATTERN "*.are"
        PATTERN "area.lst"
        PATTERN "version.are" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/version.are DESTINATION area/)
