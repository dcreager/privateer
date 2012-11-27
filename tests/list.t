  $ cp -R $ROOT/tests/reg1 .
  $ cp -R $ROOT/tests/reg2 .

  $ privateer list -r ./reg1 | sort
  alpha: ./reg1/alpha.yaml
  beta: ./reg1/beta.yaml

  $ privateer list -r ./reg1 -r ./reg2 | sort
  alpha: ./reg1/alpha.yaml
  beta: ./reg1/beta.yaml
  gamma: ./reg2/gamma.yaml
