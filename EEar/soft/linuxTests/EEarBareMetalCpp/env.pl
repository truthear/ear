#!/usr/bin/perl
use Cwd;
use File::Path;
use File::Copy;
$curent_dir = cwd;
$curentOS = $^O;



$user_home       = $ENV{'HOME'};
if ($curentOS eq "darwin")
{
	$stlink_path     = "$curent_dir/../../reference/3rdParty/stlinkMacos/bin";
}

$ENV{'STLINK'}   = $stlink_path;

$cpath = $ENV{'PATH'};
$ENV{'PATH'}     = "$cpath:$stlink_path";

print("$stlink_path/st-flash write $ARGV[0]  0x8000000");
system("$stlink_path/st-flash write $ARGV[0]  0x8000000");
print "AAAAAAAAAA";