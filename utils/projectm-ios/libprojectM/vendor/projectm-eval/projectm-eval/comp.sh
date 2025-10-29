#!/bin/sh

/opt/homebrew/opt/bison/bin/bison --defines=Compiler.h -Wcounterexamples --no-lines -o Compiler.c $1

