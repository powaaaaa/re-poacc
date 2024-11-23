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

echo OK
