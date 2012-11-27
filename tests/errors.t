  $ privateer get -r $ROOT/tests/nonexistent-reg alpha
  No such file or directory
  [1]

  $ privateer get -r $ROOT/tests/reg1 missing
  No plugin named "missing"

  $ privateer load -r $ROOT/tests/circular alpha
  Circular dependency when loading alpha
  [1]
