#!/usr/bin/ruby
# -*- mode:ruby; coding:utf-8 -*-

#bodyの部分だけをもってきて，書き換えて，あとでくっつける．
#bodyの部分だけを正規表現使って持ってくる？


# レスポンスが保存されたファイルを読み取る
str = File.read("html.txt", :encoding => Encoding::UTF_8)

# 文字コードが原因でgsubができない問題を解決
str_encode = str.encode("UTF-16BE", "UTF-8", :invalid => :replace, :undef => :replace, :replace => '?').encode("UTF-8")

# ハイライトしたい文字を読み込む
value = File.read("resultWord.txt", :encoding => Encoding::UTF_8)

# 改行文字を取り除く
value.chomp!

# body部分を抜き取る
body = str_encode.scan(/<body.*\/body>/m)

# ハイライト表示するための置換
rep = body[0].gsub(/#{value}/, "<span style=\"background-color: #ffff00\"><b>#{value}</b></span>")
# rep = str_encode.gsub(/#{value}/m, "<span style=\"background-color: #ffff00\"><b>#{value}</b></span>")

# 置換したものをもとのhtmlコードに反映
puts str_encode.sub(/<body.*\/body>/m, rep)
# puts rep
