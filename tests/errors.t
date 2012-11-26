  $ privateer get -r $TESTS/nonexistent-reg alpha
  No such file or directory
  [1]

  $ privateer get -r $TESTS/reg1 missing
  No plugin named "missing"

  $ privateer load -r $TESTS/circular alpha
  Circular dependency when loading alpha
  [1]
