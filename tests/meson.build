core_test = executable('test_core', 'test_core.c', dependencies: flashux_dep)
test('core', core_test, timeout: 30)
partition_test = executable('test_partition', 'test_partition.c', dependencies: flashux_dep)
test('partition', partition_test)
checksum_test = executable('test_checksum', 'test_checksum.c', dependencies: flashux_dep)
test('checksum', checksum_test)
device_test = executable('test_device', 'test_device.c', dependencies: flashux_dep)
test('device', device_test)
args_test = executable('test_cli_args', 'test_cli_args.c', dependencies: cli_args_dep)
test('cli-args', args_test)
format_test = executable('test_format', 'test_format.c', dependencies: flashux_dep)
test('format', format_test)
flasher_test = executable('test_flasher', 'test_flasher.c', dependencies: raw_dep)
test('flasher', flasher_test)
ipc_test = executable('test_ipc', 'test_ipc.c', dependencies: flashux_dep)
test('ipc', ipc_test)
if gui_available and is_linux
  gui_test = executable('test_gui', 'test_gui.c', dependencies: [gui_dep, dependency('pangoft2', include_type: 'system'), dependency('fontconfig', include_type: 'system')])
  xvfb = find_program('xvfb-run', required: false)
  if xvfb.found()
    test('gui', xvfb, args: ['-a', gui_test], env: ['GTK_A11Y=test', 'GSK_RENDERER=cairo'])
  endif
endif
image_test = executable('test_image', 'test_image.c', dependencies: raw_dep)
test('image', image_test)
batch_test = executable('test_cli_batch', 'test_cli_batch.c', link_with: cli_batch,
  dependencies: cli_args_dep)
test('cli-batch', batch_test)
if is_linux
  # Manual, nondestructive helper smoke probe; requires a running helper and Polkit.
  executable('test_helper', 'test_helper.c', dependencies: flashux_dep)
endif
