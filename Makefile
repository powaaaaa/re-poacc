# コンパイルフラグ
# CFLAGS=-std=c13 -g -static

# Docker用のプレフィックス
DOCKER=docker run --rm -v $(POACC):/poacc -w /poacc poa-compiler

# ターゲットと依存関係
poacc: poacc.c
	$(DOCKER) gcc -o $@ $<

test: poacc
	$(DOCKER) ./test.sh

clean:
	$(DOCKER) rm -f poacc *.o *~ tmp*

# 明示的な指定
.PHONY: test clean
