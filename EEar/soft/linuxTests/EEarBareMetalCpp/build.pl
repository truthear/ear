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


$arm_path        = "$curent_dir/../../reference/3rdParty/arm/bin";


$ENV{'STLINK'}   = $stlink_path;

$cpath = $ENV{'PATH'};
$ENV{'PATH'}     = "$cpath:$stlink_path:$arm_path";


if ($ARGV[0] eq "b")
{
	system("$stlink_path/st-flash write bin/outp.bin 0x8000000");
	exit();
}
if ($ARGV[0] eq "r")
{
	system("make");
	system("$stlink_path/st-flash write ./bin/outp.bin 0x8000000");
	exit();
}
system("make");