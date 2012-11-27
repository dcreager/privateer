  $ cp -R $ROOT/tests/reg1 .
  $ cp -R $ROOT/tests/reg2 .

  $ privateer load -r ./reg1
  Loading alpha.
  Loading beta.

  $ privateer load -r ./reg1 -r ./reg2
  Loading alpha.
  Loading beta.
  Loading gamma.
