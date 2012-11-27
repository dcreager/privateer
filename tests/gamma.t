  $ privateer get -r $ROOT/tests/reg1 gamma
  No plugin named "gamma"

  $ privateer get -r $ROOT/tests/reg2 gamma
  Plugin:       gamma
  Library:      privateer-tests
  Loader:       pvt_gamma
  Dependencies: beta

  $ privateer get -r $ROOT/tests/reg1 -r $ROOT/tests/reg2 gamma
  Plugin:       gamma
  Library:      privateer-tests
  Loader:       pvt_gamma
  Dependencies: beta

  $ privateer load -r $ROOT/tests/reg1 -r $ROOT/tests/reg2 gamma
  Loading alpha.
  Loading beta.
  Loading gamma.
