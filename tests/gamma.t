  $ privateer -r $TESTS/reg1 gamma
  No plugin named "gamma"

  $ privateer -r $TESTS/reg2 gamma
  Plugin:       gamma
  Descriptor:   /home/dcreager/git/privateer/tests/reg2/gamma.yaml
  Library:      privateer-tests
  Loader:       pvt_gamma
  Dependencies: beta

  $ privateer -r $TESTS/reg1 -r $TESTS/reg2 gamma
  Plugin:       gamma
  Descriptor:   /home/dcreager/git/privateer/tests/reg2/gamma.yaml
  Library:      privateer-tests
  Loader:       pvt_gamma
  Dependencies: beta
