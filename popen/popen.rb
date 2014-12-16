#!/usr/bin/env ruby
# encoding : utf-8
=begin
File.open('tmp.txt', "r:utf-8" ) do |f|
	while line  = f.gets
		puts(line)
	end
end
=end

=begin
f = open("recommendation.txt")
while line = f.gets
	puts(line)
end
f.close
f = open("tmphttp.txt")
while line = f.gets
	puts(line)
end
f.close
=end

recommendation = File.read("recommendation.txt", :encoding => Encoding::UTF_8)
tmphttp = File.read("tmphttp.txt", :encoding => Encoding::UTF_8)
tmphttp.scan(/<body*>/) do |matched|
	puts tmphttp.sub(/<body*>/, matched + "\n" + recommendation)
end
