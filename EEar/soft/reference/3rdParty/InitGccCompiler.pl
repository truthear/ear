#!/usr/bin/perl

use File::Fetch;
$curentOS = $^O;
$temporaryFolderName = "notForGit";
$linkName = "arm";
$fileName = "";
# download correspond package form links below and unpack into arm folder
#https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
#https://developer.arm.com/-/media/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-mac.tar.bz2
print $curentOS;


# 'https://developer.arm.com/-/media/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-mac.tar.bz2'
if ($curentOS eq "darwin")
{
    #system("wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-mac.tar.bz2");
    #system("tar -xvf gcc-arm-none-eabi-6-2017-q2-update-mac.tar.bz2  -C ./arm");
    $fileName = "gcc-arm-none-eabi-6-2017-q2-update-mac.tar.bz2";
   
    if ($ARGV[0] eq "clean")
    {
       system("rm -rf ./$temporaryFolderName");
       exit(0);
    }
    system("rm -rf ./$temporaryFolderName");

    my $url = "https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/6-2017q2/$fileName";
    system("curl -# \"$url\" -o \"$fileName\"");

    system("mkdir ./$temporaryFolderName");
    system("tar -xvf ./$fileName  -C ./$temporaryFolderName");
    chdir("$temporaryFolderName");
    system("ln -s gcc-arm-none-eabi-6-2017-q2-update $linkName");
    system("rm -rf ../$fileName");
}


if ($curentOS eq "linux")
{
    $fileName = "gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2";
    if ($ARGV[0] eq "clean")
    {
        system("rm -rf ./$temporaryFolderName");
        exit(0);
    }
    system("rm -rf ./$temporaryFolderName");
    system("wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/6-2017q2/$fileName");
    system("mkdir ./$temporaryFolderName");
    system("tar -xvf ./$fileName  -C ./$temporaryFolderName");
    chdir("$temporaryFolderName");
    system("ln -s gcc-arm-none-eabi-6-2017-q2-update $linkName");
    system("rm -rf ../$fileName");
}


if ($curentOS eq "MSWin32")
{
    system("rmdir /S /Q $temporaryFolderName");
    if ($ARGV[0] eq "clean")
    {
        system("rmdir /S /Q $temporaryFolderName");
        exit(0);
    }
    system("mkdir $temporaryFolderName");
    chdir("$temporaryFolderName");
    my $url = 'https://developer.arm.com/-/media/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-win32.zip';
    my $ff = File::Fetch->new(uri => $url);
    my $file = $ff->fetch() or die $ff->error;

    system("../winutils/7z.exe x gcc-arm-none-eabi-6-2017-q2-update-win32.zip -o$linkName")
}
