  $ cp -R $ROOT/tests/circular .
  $ cp -R $ROOT/tests/reg1 .

  $ privateer get -r ./nonexistent-reg alpha
  No such file or directory
  [1]

  $ privateer get -r ./reg1 missing
  No plugin named "missing"

  $ privateer load -r ./circular alpha
  Circular dependency when loading alpha
  [1]
