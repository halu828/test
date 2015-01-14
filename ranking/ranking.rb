#!/usr/bin/ruby
# https://sites.google.com/site/rubycocoamemo/Home/ruby-guan-lian/tango-hindo-wo-kazoeru

=begin
$KCODE = "UTF-8"
require 'find'

directory = "japanese.txt"
words = Hash.new(0)

Find.find(directory) do |path|
  if /\.txt/i =~ File.extname(path)
    File.open(path,"r") do |file|
      file.read.downcase.split(/\W+/).each do |word|
        words[word] += 1
      end
    end
  end
end

print "WORD\t\tFREQUENCY\n"

words.sort_by{|word,count| [-count,word]}.each do |word,count|
  print "#{word}\t#{count}\n"
end

=end

#!/usr/bin/ruby
# -*- mode:ruby; coding:utf-8 -*-
require 'find'

directory = "japanese.txt"
words = Hash.new(0)

Find.find(directory) do |path|
  if /\.txt/i =~ File.extname(path)
    File.open(path,"r") do |file|
      file.read.downcase.scan(/\p{Letter}+/) do |word|
        words[word] += 1
      end
    end
  end
end
print "WORD\tFREQUENCY\n"

words.sort_by{|word,count| [-count,word]}.each do |word,count|
  print "#{word}\t#{count}\n"
end