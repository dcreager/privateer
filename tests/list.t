  $ privateer list -r $ROOT/tests/reg1 | sort
  alpha: /home/dcreager/git/privateer/tests/reg1/alpha.yaml
  beta: /home/dcreager/git/privateer/tests/reg1/beta.yaml

  $ privateer list -r $ROOT/tests/reg1 -r $ROOT/tests/reg2 | sort
  alpha: /home/dcreager/git/privateer/tests/reg1/alpha.yaml
  beta: /home/dcreager/git/privateer/tests/reg1/beta.yaml
  gamma: /home/dcreager/git/privateer/tests/reg2/gamma.yaml
