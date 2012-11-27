  $ cp -R $ROOT/tests/reg1 .
  $ cp -R $ROOT/tests/reg2 .

  $ privateer get -r ./reg1 beta
  Plugin:       beta
  Descriptor:   ./reg1/beta.yaml
  Library:      privateer-tests
  Loader:       pvt_beta
  Dependencies: alpha

  $ privateer get -r ./reg2 beta
  No plugin named "beta"

  $ privateer get -r ./reg1 -r ./reg2 beta
  Plugin:       beta
  Descriptor:   ./reg1/beta.yaml
  Library:      privateer-tests
  Loader:       pvt_beta
  Dependencies: alpha

  $ privateer load -r ./reg1 beta
  Loading alpha.
  Loading beta.
