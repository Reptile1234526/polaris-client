add_executable(PolarisInjector WIN32
    main.cpp
)

set_property(TARGET PolarisInjector PROPERTY CXX_STANDARD 17)
set_property(TARGET PolarisInjector PROPERTY CXX_STANDARD_REQUIRED ON)

target_compile_options(PolarisInjector PRIVATE /W3)

target_link_libraries(PolarisInjector PRIVATE
    comdlg32    # GetOpenFileName
    dwmapi      # DwmSetWindowAttribute (dark title bar)
)
