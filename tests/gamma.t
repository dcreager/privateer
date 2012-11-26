  $ privateer get -r $TESTS/reg1 gamma
  No plugin named "gamma"

  $ privateer get -r $TESTS/reg2 gamma
  Plugin:       gamma
  Descriptor:   /home/dcreager/git/privateer/tests/reg2/gamma.yaml
  Library:      privateer-tests
  Loader:       pvt_gamma
  Dependencies: beta

  $ privateer get -r $TESTS/reg1 -r $TESTS/reg2 gamma
  Plugin:       gamma
  Descriptor:   /home/dcreager/git/privateer/tests/reg2/gamma.yaml
  Library:      privateer-tests
  Loader:       pvt_gamma
  Dependencies: beta

  $ privateer load -r $TESTS/reg1 -r $TESTS/reg2 gamma
  Loading alpha.
  Loading beta.
  Loading gamma.
