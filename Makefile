CFLAGS := -Wall -Wextra -pedantic -std=c99 -Wfloat-equal -Wvla -Werror
CPPFLAGS := -Iinclude
SANI   := -g3 -fsanitize=address -fsanitize=undefined
OPTIM  := -O3
CC := gcc # Try clang too

CSA_SRC := src/csa.c
CSA_HDR := include/csa.h include/mydefs.h

run: csa csa_s fibmemo
	./csa
	./csa_s
	./fibmemo

csa: examples/driver.c $(CSA_SRC) $(CSA_HDR)
	$(CC) $(CPPFLAGS) examples/driver.c $(CSA_SRC) $(CFLAGS) $(OPTIM) -o csa

csa_s: examples/driver.c $(CSA_SRC) $(CSA_HDR)
	$(CC) $(CPPFLAGS) examples/driver.c $(CSA_SRC) $(CFLAGS) $(SANI) -o csa_s

fibmemo: examples/fibmemo.c $(CSA_SRC) $(CSA_HDR)
	$(CC) $(CPPFLAGS) examples/fibmemo.c $(CSA_SRC) $(CFLAGS) $(OPTIM) -o fibmemo

## Extension 1 : foreach()
factorials: examples/isfactorial.c $(CSA_SRC) $(CSA_HDR)
	$(CC) $(CPPFLAGS) -DEXT examples/isfactorial.c $(CSA_SRC) $(CFLAGS) $(OPTIM) -o factorials

## Extension 2
csa_ext: examples/driver.c $(CSA_SRC) $(CSA_HDR)
	$(CC) $(CPPFLAGS) -DEXT examples/driver.c $(CSA_SRC) $(CFLAGS) $(OPTIM) -o csa_ext

primes: examples/sieve.c $(CSA_SRC) $(CSA_HDR)
	$(CC) $(CPPFLAGS) -DEXT examples/sieve.c $(CSA_SRC) $(CFLAGS) $(OPTIM) -o primes

runall: run factorials primes csa_ext
	./factorials
	./primes
	./csa_ext

clean:
	rm -f csa csa_s factorials primes csa_ext fibmemo
