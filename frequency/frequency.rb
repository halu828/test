#!/usr/bin/ruby
# -*- mode:ruby; coding:utf-8 -*-

# https://sites.google.com/site/rubycocoamemo/Home/ruby-guan-lian/tango-hindo-wo-kazoeru


require 'find'

# HTMLのタグを消す
str = File.read("tmphttp.txt", :encoding => Encoding::UTF_8)
File.open("tag_delete.txt","w") do |out|
  out.puts "#{str.gsub(/<("[^"]*"|'[^']*'|[^'">])*>/, "")}"
end

# タグが消されたテキストを分かち書きする(ファイルに保存せずにRubyで文字列として受け取る)
# `mecab -O wakati tag_delete.txt -o wakati.txt`
wakati = `mecab -O wakati tag_delete.txt`

# 助詞を省く(一文字のひらがなは省く)
File.open("wakati.txt","w") do |out|
  out.puts "#{wakati.gsub(/ [あ-ん] /, "")}"
end

# 以下，頻出回数を求めていく
words = Hash.new(0)

Find.find("wakati.txt") do |find|
  if /\.txt/i =~ File.extname(find)
    File.open(find, "r") do |readfile|
      readfile.read.downcase.scan(/\p{Letter}+/) do |word|
        words[word] += 1
      end
    end
  end
end


# ハッシュを一旦配列に入れて，上から10個表示
wordsArr = words.sort_by{|word,count| [-count,word]}.to_a

File.open("output.txt","w") do |output|
  for i in 0..29
    output.puts wordsArr[i][0]
  end
end


distance = Array.new
IO.popen('./mydistance jawikisep.bin').each do |line|
  distance.push(line)
  # puts line
end

puts "ひらがな一文字は省いています"
print "RANK\tTIMES\tWORD\t\tRESULT\n"
for i in 0..29
  #4文字以上だと出力したときにずれてしまう．bytesでとりだして上位ビットが1であるかどうかを見る．
  if wordsArr[i][0].length < 4 || wordsArr[i][0] =~ /^[0-9A-Za-z]+$/ then
    puts "#{i+1}\t#{wordsArr[i][1]}\t#{wordsArr[i][0]}\t\t#{distance[i]}"
  else
    puts "#{i+1}\t#{wordsArr[i][1]}\t#{wordsArr[i][0]}\t#{distance[i]}"
  end
end



# 過去の成功
=begin
count = 1
File.open("output.txt","w") do |output|
  words.sort_by{|word,count| [-count,word]}.each do |word,count|
    # print "#{word}\t#{count}\n"
    output.print "#{word}\n"
    count += 1
    break if count == 5
  end
end

IO.popen('./mydistance jawikisep.bin').each do |line|
  puts line
end
=end
