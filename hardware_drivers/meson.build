span_dep = subproject('nonstd-span-lite').get_variable('nonstd_span_lite_dep')

mock_usb_lib = static_library('mock_usb',
    sources: 'src/mock_usb.cpp',
    include_directories: [
        messages_inc,
        'inc',
    ],
    dependencies: [
        span_dep,
    ]
)

mock_serial_dep = declare_dependency(
    include_directories: 'inc',
)

mock_usb_dep = declare_dependency(
    link_with: mock_usb_lib,
    include_directories: 'inc',
    dependencies: [
        span_dep,
    ]
)

test_mock_usb_exe = executable('test-mock-usb',
    sources: 'tests/test-mock-usb.cpp',
    dependencies: [
        catch2_dep,
        mock_usb_dep,
    ],
)

test('Reading 24x frames from mock USB',
    test_mock_usb_exe,
    args: [
        '-r', 'tap',
    ],
    protocol: 'tap',
)