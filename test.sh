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
assert 0 0
assert 42 42
# step2 加減算
assert 21 "5+20-4"
# step3 空白文字を許容
assert 41 " 12 + 34 - 5 "
# step5 四則演算
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 4 '(3+5)/2'
# step6 単項+-
assert 10 '-10+20'
assert 10 '- -10'
assert 10 '- - +10'
# step7 比較演算子
assert 0 '0==1'
assert 1 '42==42'
assert 1 '0!=1'
assert 0 '42!=42'

assert 1 '0<1'
assert 0 '1<1'
assert 0 '2<1'
assert 1 '0<=1'
assert 1 '1<=1'
assert 0 '2<=1'

assert 1 '1>0'
assert 0 '1>1'
assert 0 '1>2'
assert 1 '1>=0'
assert 1 '1>=1'
assert 0 '1>=2'

echo OK
