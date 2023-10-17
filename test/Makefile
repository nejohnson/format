# ***************************************************************************
# * Format - lightweight string formatting library.
# * Copyright (C) 2016-2023, Neil Johnson
# * All rights reserved.
# *
# * Redistribution and use in source and binary forms,
# * with or without modification,
# * are permitted provided that the following conditions are met:
# *
# * * Redistributions of source code must retain the above copyright notice,
# *   this list of conditions and the following disclaimer.
# * * Redistributions in binary form must reproduce the above copyright notice,
# *   this list of conditions and the following disclaimer in the
# *   documentation and/or other materials provided with the distribution.
# * * Neither the name of nor the names of its contributors
# *   may be used to endorse or promote products derived from this software
# *   without specific prior written permission.
# *
# * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
# * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# * ************************************************************************* */

CFLAGS += -I../src -std=c99 -Wall -pedantic -g \
	-Wunused -Wstrict-prototypes -Wmissing-prototypes \
	-Wshadow -Wmaybe-uninitialized -Wtype-limits

LDFLAGS += 

all: testharness perftest libtest
	./testharness
	./perftest
	./libtest

format.o: ../src/format.c
	$(CC) $(CFLAGS) -c $< -o $@

tinyformat.o: ../src/tinyformat.c
	$(CC) $(CFLAGS) -c $< -o $@

microformat.o: ../src/microformat.c
	$(CC) $(CFLAGS) -c $< -o $@

lib.o: ../lib/printf.c
	$(CC) $(CFLAGS) -I../lib -c $< -o $@

libtest.o: libtest.c
	$(CC) $(CFLAGS) -I../lib -c $< -o $@

testharness: testharness.o format.o
	$(CC) $(LDFLAGS) testharness.o format.o -o testharness

tinytestharness: tinytestharness.o tinyformat.o
	$(CC) $(LDFLAGS) tinytestharness.o tinyformat.o -o tinytestharness

microtestharness: microtestharness.o microformat.o
	$(CC) $(LDFLAGS) microtestharness.o microformat.o -o microtestharness

perftest: performance.o format.o
	$(CC) $(LDFLAGS) performance.o format.o -o perftest

libtest: libtest.o format.o lib.o
	$(CC) $(LDFLAGS) libtest.o format.o lib.o -o libtest

clean:
	rm -f testharness
	rm -f tinytestharness
	rm -f microtestharness
	rm -f perftest
	rm -f libtest
	rm -f *.o

what:
	@echo "make targets:"
	@echo "   all              -- all known targets (the default), then runs the test harness"
	@echo "   testharness      -- the format test program"
	@echo "   tinytestharness  -- test harness for tinyformat"
	@echo "   microtestharness -- test harness for microformat"
	@echo "   perftest         -- runs some float performance tests"
	@echo "   libtest          -- library tests"
	@echo "   clean            -- deletes all build artifacts"

