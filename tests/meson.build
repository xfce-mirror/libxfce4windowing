add_test_setup(
  'default',
  exclude_suites: 'interactive',
  is_default: true,
)
add_test_setup(
  'full',
)

test_bins = [
  'xfw-enum-monitors',
]
test_interactive_bins = [
  'xfw-enum-windows',
  'xfw-monitor-offon',
]

foreach bin : test_bins + test_interactive_bins
  e = executable(
    bin,
    sources: [
      '@0@.c'.format(bin),
    ],
    include_directories: [
      include_directories('..'),
    ],
    dependencies: [
      gtk,
    ],
    link_with: [
      libxfce4windowing,
    ],
    install: false,
  )

  if bin in test_interactive_bins
    test(bin, e, suite: 'interactive')
  else
    test(bin, e)
  endif
endforeach
