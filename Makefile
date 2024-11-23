CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

# Docker用のプレフィックス
DOCKER=docker run --rm --platform linux/amd64 -v $(POACC):/poacc -w /poacc poa-compiler

# ターゲットと依存関係
poacc: $(OBJS)
	$(DOCKER) gcc -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(DOCKER) gcc -c -o $@ $<

test: poacc
	$(DOCKER) ./test.sh

clean:
	$(DOCKER) rm -f poacc *.o *~ tmp*

# 明示的な指定
.PHONY: test clean
