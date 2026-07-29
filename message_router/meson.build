fmt_dep = subproject('fmt').get_variable('fmt_dep')

message_router_lib = static_library('message-router',
    sources: 'src/message_router_impl.cpp',
    include_directories: [
        'inc',
        messages_inc,
        common_inc,
    ],
    dependencies: [
        mock_serial_dep,
        fmt_dep,
    ]
)

message_router_dep = declare_dependency(
    link_with: message_router_lib,
    include_directories: [
        'inc',
        messages_inc,
        common_inc,
    ],
    compile_args: [
        '-DUSING_FIBER',
    ],
    dependencies: [
        span_dep,
    ],
)

frame_capture_card_dep = declare_dependency(
    include_directories: [
        'inc',
        common_inc,
        messages_inc,
    ],
    dependencies: span_dep,
)

test_frame_capture_exe = executable('test-frame-capture',
    sources: 'tests/test-frame-capture.cpp',
    include_directories: [
        common_inc,
        messages_inc,
        'inc',
    ],
    cpp_args: [
        # Disable Boost::Fiber's cooperative scheduling.
        '-UUSING_FIBER',
    ],
    dependencies: [
        catch2_dep,
        mock_usb_dep,
        threads_dep,
    ],
)

test('Capture frames from 24 cameras from Mock USB',
    test_frame_capture_exe,
    args: [
        '-r', 'tap',
    ],
    protocol: 'tap',
)
