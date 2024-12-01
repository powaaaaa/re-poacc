#!/bin/bash

assert() {
  expected="$1"
  input="$2"

  ./poacc "$input" > tmp.s
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

# step1 数値を返す
assert 0 'return 0;'
assert 42 'return 42;'

# step2 加減算
assert 21 'return 5+20-4;'

# step3 空白文字を許容
assert 41 'return  12 + 34 - 5 ;'

# step5 四則演算
assert 47 'return 5+6*7;'
assert 15 'return 5*(9-6);'
assert 4 'return (3+5)/2;'

# step6 単項+-
assert 10 'return -10+20;'
assert 10 'return - -10;'
assert 10 'return - - +10;'

# step7 比較演算子
assert 0 'return 0==1;'
assert 1 'return 42==42;'
assert 1 'return 0!=1;'
assert 0 'return 42!=42;'

assert 1 'return 0<1;'
assert 0 'return 1<1;'
assert 0 'return 2<1;'
assert 1 'return 0<=1;'
assert 1 'return 1<=1;'
assert 0 'return 2<=1;'

assert 1 'return 1>0;'
assert 0 'return 1>1;'
assert 0 'return 1>2;'
assert 1 'return 1>=0;'
assert 1 'return 1>=1;'
assert 0 'return 1>=2;'
# step9-11 ローカル変数, statement, return
#
assert 3 'a=3; return a;'
assert 8 'a=3; z=5; return a+z;'

assert 1 'return 1; 2; 3;'
assert 2 '1; return 2; 3;'
assert 3 '1; 2; return 3;'

assert 3 'foo=3; return foo;'
assert 8 'foo123=3; bar=5; return foo123+bar;'

##########################################################

# step12-1 if
assert 3 'if (0) return 2; return 3;'
assert 3 'if (1-1) return 2; return 3;'
assert 2 'if (1) return 2; return 3;'
assert 2 'if (2-1) return 2; return 3;'

# step12-2 while
assert 10 'i=0; while(i<10) i=i+1; return i;'

# step12-3 for
assert 55 'i=0; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j;'
assert 3 'for (;;) return 3; return 5;'

# step13 block
assert 3 '{1; {2;} return 3;}'
assert 55 'i=0; j=0; while(i<=10) {j=i+j; i=i+1;} return j;'

echo OK
