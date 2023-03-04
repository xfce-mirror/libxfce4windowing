if enable_wayland

wl_scanner = find_program(
  wl_scanner_dep.get_variable('wayland_scanner'),
  native: true,
  required: true,
)

wl_scanner_src_gen = generator(
  wl_scanner,
  output: '@BASENAME@-client.c',
  arguments: [ 'private-code', '@INPUT@', '@OUTPUT@' ],
)
wl_scanner_hdr_gen = generator(
  wl_scanner,
  output: '@BASENAME@-client.h',
  arguments: [ 'client-header', '@INPUT@', '@OUTPUT@' ],
)

wl_protocols = [
  'ext-workspace-v1-20220919.xml',
  'wlr-foreign-toplevel-management-unstable-v1.xml',
]

wl_proto_src = []
wl_proto_hdr = []

foreach p : wl_protocols
  wl_proto_src += wl_scanner_src_gen.process(p)
  # wl_proto_hdr += wl_scanner_hdr_gen.process(p)
  wl_proto_hdr += custom_target(
    p.underscorify(),
    output: fs.stem(p) + '-client.h',
    input: p,
    command: [
      wl_scanner,
      'client-header',
      '@INPUT@',
      '@OUTPUT@',
    ],
  )
endforeach

xfw_wl_protocol = static_library(
  'xfw-wl-protocol',
  wl_proto_src + wl_proto_hdr,
  include_directories: [ root_inc, include_directories('.') ],
  install: false,
)
xfw_wl_protocol_dep = declare_dependency(
  link_with: xfw_wl_protocol,
  include_directories: include_directories('.')
)
endif
