#!/usr/bin/env ruby
# encoding : utf-8

# 一時ファイルの中身を取得
addhttp2serverlink = File.read("addhttp2serverlink.txt", :encoding => Encoding::UTF_8)

value = "<a href=\"http://nghttp2.org/\">HTTP/2対応サーバ</a>"

# bodyタグのすぐあとにmydistanceの出力を連結
addhttp2serverlink.scan(/<body.*>/) do |matched|
	puts addhttp2serverlink.sub(/<body.*>/, matched + "\n<p>" + value + "</p>\n")
end
