# -*- coding: utf-8 -*-

#str = "この画像<img src=\"../images/example.png\" alt='example.png' title='<img>タグ例' />は<span>テスト画像</span>です。"
str = File.read("tmphttp.txt", :encoding => Encoding::UTF_8)

#puts "[変換前] #{str}"
#puts "[パターン１（×）] #{str.gsub(/<.*?>/, "")}"
#puts "[パターン２（×）] #{str.gsub(/<\/?[^>]*>/, "")}"
#puts "[パターン３（○）] #{str.gsub(/<("[^"]*"|'[^']*'|[^'">])*>/, "")}"
#puts "[パターン４（○）] #{str.gsub(/<(".*?"|'.*?'|[^'"])*?>/, "")}"


# subは最初にマッチした部分だけを置き換える．gsubはマッチする部分全てを置き換える．
# "<"が出てくるまでを削除
#puts "#{str.sub(/.*</, "")}"
puts "#{str.gsub(/<("[^"]*"|'[^']*'|[^'">])*>/, "")}"
