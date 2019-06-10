#!/bin/bash
try() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$expected expected, but got $actual"
        exit 1
    fi
}

try 0 "0;"
try 42 "42;"
try 21 '5+20-4;'
try 41 " 12 + 34 - 5 ;"
try 47 "5+6*7;"
try 15 "5*(9-6);"
try 4 "(3+5)/2;"
try 5 "-10+15;"
try 31 "-3*+5*-2+1;"
try 2 "-10 * -2 / -5 + 6;"

try 1 "14 == 14;"
try 0 "23 != 23;"
try 1 "3<4;"
try 0 "4<4;"
try 1 "5<=6;"
try 1 "6<=6;"
try 1 "8>7;"
try 0 "8>8;"
try 1 "8>=7;"
try 1 "8>=8;"
try 1 "(1+2)*3 > 1+(2*3);"
try 1 "1==1 != 1!=1;"
try 0 "1==1 == 1==0;"
try 1 "a=1;"
try 3 "a=1;b=2;c=a+b;c;"

echo ok
