[低レイヤを知りたい人のための C コンパイラ作成入門](https://www.sigbus.info/compilerbook)

# 環境構築

(付録 3 から引用)

image を build

```
$ docker build ./ -t poa-compiler --platform linux/amd64
```

コンテナ内でコマンド実行例

```
$ docker run --rm poa-compiler ls /
```

## コマンド

コンパイル

```
$ make
```

テスト実行

```
$ make test
```

クリーンアップ

```
$ make clean
```

# 参考

- [Apple Silicon 環境](https://zenn.dev/tok/scraps/51f8ec23ea48e1)
  - 結局`push`が使えなかったので、変更した
- [M1 Mac で chibicc を動かす](https://sbite.hatenablog.com/entry/2021/04/21/222225)
