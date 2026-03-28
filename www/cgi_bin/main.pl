#!/usr/bin/perl
use strict;
use warnings;
use POSIX qw(strftime);

# Print HTTP header
print "Content-type: text/html\n\n";

# Get current date/time
my $date = strftime "%Y-%m-%d %H:%M:%S", localtime;

print <<HTML;
<html>
<head><title>Date and Time</title></head>
<body>
<h1>Current server time:</h1>
<p>$date</p>
</body>
</html>
HTML