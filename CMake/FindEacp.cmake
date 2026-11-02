include(CPM)

# eyalamirmusic/eacp jp/remove-crash-reporting (stacked on #196) until it merges.
# -DCPM_eacp_SOURCE=<checkout> builds against a local eacp instead.
set(EACP_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(EACP_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(EACP_BUILD_WEBVIEW OFF CACHE BOOL "" FORCE)

CPMAddPackage(
        NAME eacp
        GITHUB_REPOSITORY eyalamirmusic/eacp
        GIT_TAG 82ee51f22205077edca7f2edb85082224da33595)
