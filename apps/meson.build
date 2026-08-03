capture_images_exe = executable('capture-images',
    sources: [
        'main.cpp',
    ],
    include_directories: [
        'amgen2019-full',
        messages_inc,
    ],
    cpp_args: [
        '-march=native',
        '-ffunction-sections',
        '-fdata-sections',
        '-DMOCK_SERIAL',
    ],
    link_args: [
        '-Wl,-gc-sections',
    ],
    dependencies: [
        message_router_dep,
        workers_dep,
        fmt_dep,
        bioimage_coder_dsl_dep,
        threads_dep,
        mock_serial_dep,
    ],
)

render_to_plantuml_amgen2019_full_exe = executable('export-to-plantuml-amgen2019-full',
    sources: [
        'amgen2019-full/main_protocol.hpp',
    ],
    include_directories: [
        'amgen2019-full',
        messages_inc,
    ],
    dependencies: [
        fmt_dep,
        bioimage_coder_export_plantuml_dep,
        threads_dep,
    ],
)

render_to_plantuml_tie_exe = executable('export-to-plantuml-tie',
    include_directories: [
        'transport-of-intensity-equation',
        messages_inc,
    ],
    dependencies: [
        fmt_dep,
        bioimage_coder_export_plantuml_dep,
        threads_dep,
    ],
)

render_to_plantuml_lucy_richarson_exe = executable('export-to-plantuml-lucy-richardson',
    include_directories: [
        'lucy-richardson-deconvolution',
        messages_inc,
    ],
    dependencies: [
        fmt_dep,
        bioimage_coder_export_plantuml_dep,
        threads_dep,
    ],
)