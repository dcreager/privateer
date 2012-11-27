  $ privateer get -r $ROOT/tests/reg1 beta
  Plugin:       beta
  Library:      privateer-tests
  Loader:       pvt_beta
  Dependencies: alpha

  $ privateer get -r $ROOT/tests/reg2 beta
  No plugin named "beta"

  $ privateer get -r $ROOT/tests/reg1 -r $ROOT/tests/reg2 beta
  Plugin:       beta
  Library:      privateer-tests
  Loader:       pvt_beta
  Dependencies: alpha

  $ privateer load -r $ROOT/tests/reg1 beta
  Loading alpha.
  Loading beta.
