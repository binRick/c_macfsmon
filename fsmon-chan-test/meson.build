fsmon_chan_test_srcs = [
  'fsmon-chan-test.c',
]
fsmon_include_dirs = [
  '.',
  inc,
]
fsmon_chan_test_deps = [
  fsmon_chan_dep,
  fsevents_dep,
  fsmatch_dep,
  greatest_dep,
  chan_dep,
  logh_dep,
  cargs_dep,
  timestamp_dep,
  c_vector_dep,
  timestamp_dep,
  dbg_dep,
  c_stringfn_dep,
  c_string_buffer_dep,
  c_fsio_dep,
]

if get_option('enable-binaries')
  fsmon_chan_test_exec = executable('fsmon-chan-test',
     fsmon_chan_test_srcs,
     dependencies: fsmon_chan_test_deps,
     include_directories: fsmon_include_dirs,
  )
  test('fsmon-chan-test',fsmon_chan_test_exec, args: ['-v','-a'])
endif
