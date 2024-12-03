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

run: poacc
	$(DOCKER) ./poacc "$(INPUT)" > tmp.s
	$(DOCKER) gcc -o tmp tmp.s
	$(DOCKER) ./tmp
	$(DOCKER) echo $?

test: poacc
	$(DOCKER) ./poacc tests > tmp.s
	$(DOCKER) gcc -static -o tmp tmp.s
	$(DOCKER) ./tmp

clean:
	$(DOCKER) rm -f poacc *.o *~ tmp*

# 明示的な指定
.PHONY: test clean
