#!/usr/bin/perl
use Cwd;
use File::Path;
use File::Copy;
$curent_dir = cwd;


$user_home       = $ENV{'HOME'};
$stlink_path     = "$curent_dir/3rdParty/stlink";

$ENV{'STLINK'}   = $stlink_path;

$cpath = $ENV{'PATH'};
$ENV{'PATH'}     = "$cpath:$stlink_path";


system("$stlink_path/st-flash write $ARGV[0]  0x8000000");
print "AAAAAAAAAA";