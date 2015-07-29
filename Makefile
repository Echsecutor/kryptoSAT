#############################################################################
# 
# @file Makefile
#
# @section DESCRIPTION
# 
# Builds the referenz implementation of KryptoSAT and runs some test cases.
#
# @author  Sebastian Schmittner <sebastian@schmittner.pw>
#
# @version 1.0.2015-04-09
#
# @section Version number format
#
# The Version number is formatted as "M.S.D" where M is the major
# release branch (backward compatibility to all non-alpha releases of
# the same branch is guaranteed), S is the state of this release (0
# for alpha, 1 for beta, 2 for stable), and D is the date formatted
# as yyyy-mm-dd.)
# 
# 
# @copyright 2015 Sebastian Schmittner
#
# @section LICENSE
# This file is part of KryptoSAT.
#
# KryptoSAT is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# KryptoSAT is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with KryptoSAT.  If not, see <http://www.gnu.org/licenses/>.
#
#############################################################################

#export SHELL = /bin/sh
#MAKEFLAGS+= -w -e

ROOTDIR := $(shell pwd)

SRCDIR = $(ROOTDIR)/src
TESTCASES = $(ROOTDIR)/test-in
TESTDIR = $(ROOTDIR)/test-out
BINDIR = $(ROOTDIR)/bin


CC := g++

DEBUGFLAGS =

CFLAGS = -Wall -ansi -pedantic -std=c++0x 

LDFLAGS =

VPATH = $(SRCDIR):$(TESTDIR)

.PHONY: clean all folders tests

.DELETE_ON_ERROR:

all: DEBUGFLAGS=-O2 
all: folders tests
	@echo 
	@echo "[OK]		All tests Passed! :D"
	@echo 

tests: kryptoSAT.out

binaries: $(BINDIR)/kryptoSAT

debug: DEBUGFLAGS = -g -O0
debug: folders tests

folders: $(BINDIR) $(TESTDIR)

$(BINDIR):
	@-mkdir $(BINDIR)

$(TESTDIR):
	@-mkdir $(TESTDIR)


$(BINDIR)/%: $(BINDIR)/%.o
	@echo
	@echo "[...]		Building $^"
	@echo
	$(CC) $(PREFLAGS) $(CFLAGS) $(DEBUGFLAGS) -o $@ $^ $(LDFLAGS)
	@echo
	@echo "[OK]		Compillation succesfull"
	@echo


kryptoSAT.out: $(BINDIR)/kryptoSAT
	@echo
	@echo "[...]		Test run of $^."
	@echo "[...]		Generating random 8 bit text..."
	@echo
	$^ -b -n 8 -m 1 -o $(TESTDIR)/text > $(TESTDIR)/$@ 2>&1
	@echo "[OK]		"
	@echo "[...]		Generating Key with default parameters"
	time $^ -b -g -o $(TESTDIR)/key >> $(TESTDIR)/$@ 2>&1
	@echo "[OK]		"
	@echo "[...]		Encrypting with default parameters"
	time $^ -b -t $(TESTDIR)/text.priv -k $(TESTDIR)/key.pub -o $(TESTDIR)/cipher >> $(TESTDIR)/$@ 2>&1
	@echo "[OK]		"
	@echo "[...]		Decryption..."
	$^ -b -c $(TESTDIR)/cipher.cipher -k $(TESTDIR)/key.pub -K $(TESTDIR)/key.priv -o $(TESTDIR)/clear >> $(TESTDIR)/$@ 2>&1
	@echo
	@echo "[OK]		test did not crash ;)"
	@echo "[...]		comparing texts..."
	@echo
	@if [ "$(cat $(TESTDIR)/clear.text)" = "$(cat $(TESTDIR)/text.priv)" ];	then echo "[OK]		Decryption matches clear text!"; else echo "[fail]		Decryption does not match clear text!" ; return -1; fi
	@echo


clean:
	@-rm -f $(BINDIR)/* $(TESTDIR)/* *~ $(SRCDIR)/*~ *.o *~


$(BINDIR)/%.o : %.cpp
	$(CC) $(PREFLAGS) $(CFLAGS) $(DEBUGFLAGS) -c -o $@ $<



