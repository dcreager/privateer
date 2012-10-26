  $ privateer -r $TESTS/reg1 gamma name
  Cannot find gamma:name in registry
  [1]

  $ privateer -r $TESTS/reg1 gamma options
  Cannot find gamma:options in registry
  [1]

  $ privateer -r $TESTS/reg1 -r $TESTS/reg2 gamma options
  Cannot find gamma:options in registry
  [1]

  $ privateer -r $TESTS/reg1 gamma missing
  Cannot find gamma:missing in registry
  [1]
