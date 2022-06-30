fsmon_chan_srcs = [
  'fsmon-chan.c',
]
fsmon_chan_inc_dirs = [
  '.',
  inc,
]
fsmon_chan_deps = [
  fsmatch_dep,
  fsevents_dep,
  chan_dep,
  fsmon_dep,
  ansi_codes_dep,
  generic_print_dep,
  logh_dep,
  ms_dep,
  timestamp_dep,
  c_timer_dep,
  bytes_dep,  
]
fsmon_chan_lib = static_library('fsmon-chan',
  fsmon_chan_srcs,
  install: true,
  dependencies: fsmon_chan_deps,
  include_directories: fsmon_chan_inc_dirs,
)

fsmon_chan_dep = declare_dependency(
  include_directories: fsmon_chan_inc_dirs,
  link_with: fsmon_chan_lib,
)
