# proxy

# OS
FROM ubuntu

# 作成者
MAINTAINER Takumi Wakasa <cs11100@s.inf.shizuoka.ac.jp>

# アップデート
RUN apt-get update -y
RUN apt-get upgrade -y

# makeコマンドとifconfigに必要なもの
RUN apt-get install -y make gcc g++ net-tools

# カレントディレクトリをコピー
ADD . /src

# ポート10080番を開放
EXPOSE 10080

# 作業ディレクトリを指定
WORKDIR src

# コンパイル(コンテナ上の環境の上でコンパイルしないと実行できない)
RUN make
RUN gcc -o mydistance mydistance.c

# ipaddr.shに実行権限を付加する
RUN chmod +x ipaddr.sh

# シェルスクリプト実行
CMD ./ipaddr.sh
