fsmon_test_srcs = [
  'fsmon-test.c',
]
fsmon_include_dirs = [
  '.',
  inc,
]
fsmon_test_deps = [
  fsmon_dep,
  fsevents_dep,
  fsmatch_dep,
  greatest_dep,
]

if get_option('enable-binaries')
  fsmon_test_exec = executable('fsmon-test',
     fsmon_test_srcs,
     dependencies: fsmon_test_deps,
     include_directories: fsmon_include_dirs,
  )
  #test('fsmon-test',fsmon_test_exec, args: [meson.current_source_dir()])
endif
