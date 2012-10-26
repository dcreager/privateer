  $ privateer -r $TESTS/reg1 beta name
  ---
  !!map {
    ? !!str "name"
    : !!str "beta",
  }

  $ privateer -r $TESTS/reg1 beta options
  Cannot find beta:options in registry
  [1]

  $ privateer -r $TESTS/reg1 -r $TESTS/reg2 beta options
  ---
  !!map {
    ? !!str "verbose"
    : !!str "false",
    ? !!str "dependencies"
    : !!seq [
      !!str "alpha",
    ],
  }

  $ privateer -r $TESTS/reg1 beta missing
  Cannot find beta:missing in registry
  [1]
