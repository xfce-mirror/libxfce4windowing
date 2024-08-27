if get_option('tests')
  test_bins = [
    'xfw-enum-monitors',
    'xfw-enum-windows',
    'xfw-monitor-offon',
  ]

  foreach bin : test_bins
    executable(
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
  endforeach
endif
