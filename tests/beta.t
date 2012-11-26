  $ privateer -r $TESTS/reg1 beta
  Plugin:       beta
  Descriptor:   /home/dcreager/git/privateer/tests/reg1/beta.yaml
  Library:      privateer-tests
  Loader:       pvt_beta
  Dependencies: alpha

  $ privateer -r $TESTS/reg2 beta
  No plugin named "beta"

  $ privateer -r $TESTS/reg1 -r $TESTS/reg2 beta
  Plugin:       beta
  Descriptor:   /home/dcreager/git/privateer/tests/reg1/beta.yaml
  Library:      privateer-tests
  Loader:       pvt_beta
  Dependencies: alpha
