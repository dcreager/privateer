  $ privateer -r $TESTS/reg1 alpha name
  ---
  !!map {
    ? !!str "name"
    : !!str "alpha",
  }

  $ privateer -r $TESTS/reg1 alpha options
  ---
  !!map {
    ? !!str "verbose"
    : !!str "true",
  }

  $ privateer -r $TESTS/reg1 alpha missing
  Cannot find alpha:missing in registry
  [1]
