  $ privateer -r $TESTS/reg1 alpha
  Plugin:       alpha
  Descriptor:   /home/dcreager/git/privateer/tests/reg1/alpha.yaml
  Library:      [default]
  Loader:       pvt_alpha

  $ privateer -r $TESTS/reg2 alpha
  No plugin named "alpha"

  $ privateer -r $TESTS/reg1 -r $TESTS/reg2 alpha
  Plugin:       alpha
  Descriptor:   /home/dcreager/git/privateer/tests/reg1/alpha.yaml
  Library:      [default]
  Loader:       pvt_alpha
