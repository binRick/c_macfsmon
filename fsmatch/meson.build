fsmatch_srcs = [
  'fsmatch.c',
]
fsmatch_inc_dirs = [
  '.',
  inc,
]
fsmatch_deps = [
]
fsmatch_lib = static_library('fsmatch',
  fsmatch_srcs,
  install: true,
  dependencies: fsmatch_deps,
  include_directories: fsmatch_inc_dirs,
)

fsmatch_dep = declare_dependency(
  include_directories: fsmatch_inc_dirs,
  link_with: fsmatch_lib,
)
