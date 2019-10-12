# Convert a hex dump to a binary file, via a Perl script.

use warnings;
use strict;

my $in_file = "linload.txt";
my $out_file = "linload";
open(my $fin, "<", $in_file) or die "$!";
open(my $fout, ">", $out_file) or die "$!";

while (<$fin>) {
    chomp;
    my @mybytes = split(/ /, $_);
    my $bchunk = "";
    for my $mybyte (@mybytes) {
	my $ec = substr($mybyte, -1, 1);
	next if ($ec eq '-' or $ec eq ':');
	$bchunk .= chr(hex($mybyte));
    }
    print $fout $bchunk;
}
