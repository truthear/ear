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

if ( -d $arm_path) {
    system("make -s -f ../common/Makefile");
} else {
    print "\n\n Please init Gcc Compiler by EEar/soft/reference/3rdparty/InitGccCompiler.pl script \n\n"
}


