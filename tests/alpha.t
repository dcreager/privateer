  $ cp -R $ROOT/tests/reg1 .
  $ cp -R $ROOT/tests/reg2 .

  $ privateer get -r ./reg1 alpha
  Plugin:       alpha
  Descriptor:   ./reg1/alpha.yaml
  Library:      [default]
  Loader:       pvt_alpha

  $ privateer get -r ./reg2 alpha
  No plugin named "alpha"

  $ privateer get -r ./reg1 -r ./reg2 alpha
  Plugin:       alpha
  Descriptor:   ./reg1/alpha.yaml
  Library:      [default]
  Loader:       pvt_alpha

  $ privateer load -r ./reg1 alpha
  Loading alpha.
