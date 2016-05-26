#!/usr/bin/perl
use Cwd;
use File::Path;
use File::Copy;
$curent_dir = cwd;



$user_home       = $ENV{'HOME'};
$stlink_path     = "$curent_dir/../../reference/3rdParty/stlink";
$arm_path        = "$curent_dir/../../reference/3rdParty/arm/bin";


$ENV{'STLINK'}   = $stlink_path;

$cpath = $ENV{'PATH'};
$ENV{'PATH'}     = "$cpath:$stlink_path:$arm_path";


if ($ARGV[0] eq "b")
{
	system("$stlink_path/st-flash write bin/outp.hex 0x8000000");
	exit();
}
if ($ARGV[0] eq "r")
{
	system("make");
	system("$stlink_path/st-flash write bin/outp.hex 0x8000000");
	exit();
}
system("make");