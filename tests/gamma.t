  $ cp -R $ROOT/tests/reg1 .
  $ cp -R $ROOT/tests/reg2 .

  $ privateer get -r ./reg1 gamma
  No plugin named "gamma"

  $ privateer get -r ./reg2 gamma
  Plugin:       gamma
  Descriptor:   ./reg2/gamma.yaml
  Library:      privateer-tests
  Loader:       pvt_gamma
  Dependencies: beta

  $ privateer get -r ./reg1 -r ./reg2 gamma
  Plugin:       gamma
  Descriptor:   ./reg2/gamma.yaml
  Library:      privateer-tests
  Loader:       pvt_gamma
  Dependencies: beta

  $ privateer load -r ./reg1 -r ./reg2 gamma
  Loading alpha.
  Loading beta.
  Loading gamma.
