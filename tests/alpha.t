  $ privateer get -r $ROOT/tests/reg1 alpha
  Plugin:       alpha
  Library:      [default]
  Loader:       pvt_alpha

  $ privateer get -r $ROOT/tests/reg2 alpha
  No plugin named "alpha"

  $ privateer get -r $ROOT/tests/reg1 -r $ROOT/tests/reg2 alpha
  Plugin:       alpha
  Library:      [default]
  Loader:       pvt_alpha

  $ privateer load -r $ROOT/tests/reg1 alpha
  Loading alpha.
