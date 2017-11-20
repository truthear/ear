#!/usr/bin/perl
use Cwd;
use File::Path;
use File::Copy;
$curent_dir = cwd;
$curentOS = $^O;
print $curentOS;

$user_home       = $ENV{'HOME'};


if ($curentOS eq "darwin" or $curentOS eq "linux")
{
    $stlink_path     = "$curent_dir/../../reference/3rdParty/stlinkMacos/bin";
    $arm_path        = "$curent_dir/../../reference/3rdParty/notForGit/arm/bin";
    $ENV{'STLINK'}   = $stlink_path;
    $cpath = $ENV{'PATH'};
    $ENV{'PATH'}     = "$cpath:$stlink_path:$arm_path";
}


if($curentOS eq "MSWin32"){
    $arm_path = "$curent_dir/../../reference/3rdParty/notForGit/arm/bin";
    $winUtils = "$curent_dir/../../reference/3rdParty/winutils/bin";
    $cpath = $ENV{'PATH'};
    $ENV{'PATH'} = "$winUtils;$cpath;$arm_path";
}

#system("find  -L ./lib -name *.h -exec dirname {} \; | uniq");


#exit(0);
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
