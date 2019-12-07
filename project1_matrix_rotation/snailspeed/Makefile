# Makefile for everybit
# Copyright (C) 2009, 2010, 2011, 2012 the Massachusetts Institute of Technology
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

.DEFAULT_GOAL := rotate

CC=clang
CFLAGS= -Wall -O3 -g -ftree-vectorize  -march=native -flto
DEPS = ../utils/libbmp.h ../utils/tester.h ../utils/utils.h
OBJ = ../utils/libbmp.o ../utils/tester.o ../utils/utils.o ../utils/main.o rotate.o

debug: CFLAGS += -DDEBUG
debug: rotate

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rotate: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f ../utils/*.o
	rm -f *.o rotate

mytests: additional_tests.c rotate.c ../utils/utils.h Makefile
	 $(CC) additional_tests.c $(CFLAGS) -o additional_tests 
