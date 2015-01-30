# Minimum HTTP/2 client/server implementation

# OS
FROM ubuntu:14.04

# 作成者
MAINTAINER Takumi Wakasa <cs11100@s.inf.shizuoka.ac.jp>

# アップデート
RUN apt-get update && apt-get upgrade -y

# makeコマンド実行に必要なものとifconfigのインストール
RUN apt-get install -y gcc g++ make net-tools libboost1.55

# カレントディレクトリをコピー
ADD . /src

# ポート80番を開放
EXPOSE 80

# 作業ディレクトリを指定
WORKDIR src

# コンパイル(コンテナ上の環境の上でコンパイルしないと実行できない)
RUN make

CMD ./http2_client 106.186.112.116 80
