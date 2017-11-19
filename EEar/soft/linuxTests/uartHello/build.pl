#!/usr/bin/perl
use Cwd;
use File::Path;
use File::Copy;
$curent_dir = cwd;


$out_file        = "uartHello";

$user_home       = $ENV{'HOME'};
$stlink_path     = "$curent_dir/../../reference/3rdParty/stlinkMacos/bin";
$arm_path        = "$curent_dir/../../reference/3rdParty/arm/bin";


$ENV{'STLINK'}   = $stlink_path;

$cpath = $ENV{'PATH'};
$ENV{'PATH'}     = "$cpath:$stlink_path:$arm_path";

$CC         = "arm-none-eabi-gcc";
$OBJCOPY    = "arm-none-eabi-objcopy";

$SRCS       = "";
$SRCS       =  $SRCS."main.c"." ";
$SRCS       =  $SRCS."src/printf.c"." ";
$SRCS       =  $SRCS."src/syscalls.c"." ";
$SRCS       =  $SRCS."cmsis/src/misc.c"." ";
$SRCS       =  $SRCS."cmsis/src/stm32f4xx_usart.c"." ";
$SRCS       =  $SRCS."cmsis/src/stm32f4xx_rcc.c"." ";
$SRCS       =  $SRCS."cmsis/src/stm32f4xx_gpio.c"." ";
$SRCS       =  $SRCS."cmsis_boot/startup_stm32f4xx.s"." ";
$SRCS       =  $SRCS."cmsis_boot/system_stm32f4xx.c"." ";


$CFLAGS     = " ";
$CFLAGS     = $CFLAGS."-g -O2 -Wall -Tcmsis_boot/stm32_flash.ld"." ";
$CFLAGS     = $CFLAGS."-mlittle-endian"." ";
$CFLAGS     = $CFLAGS."-mthumb"." ";
$CFLAGS     = $CFLAGS."-mcpu=cortex-m4"." ";
$CFLAGS     = $CFLAGS."-mthumb-interwork"." ";
$CFLAGS     = $CFLAGS."-mfloat-abi=hard"." ";
$CFLAGS     = $CFLAGS."-mfpu=fpv4-sp-d16"." ";
$CFLAGS     = $CFLAGS."-DUSE_STDPERIPH_DRIVER"." ";


$CFLAGS     = $CFLAGS."-I."." ";
$CFLAGS     = $CFLAGS."-Isrc"." ";
$CFLAGS     = $CFLAGS."-Icmsis_boot/"." ";
$CFLAGS     = $CFLAGS."-Icmsis/"." ";
$CFLAGS     = $CFLAGS."-Icmsis/inc"." ";



if ($ARGV[0] eq "b")
{
    system("$stlink_path/st-flash write hexout/$out_file.bin  0x8000000");
}
else
{
    system("$CC      $SRCS $CFLAGS -o hexout/$out_file.elf");
    if ( $? == 0 )
    {
        printf "build succesfull\n";
    }
    else
    {
        die || "compile failed \n";
    }
    system("$OBJCOPY    -O ihex       hexout/$out_file.elf hexout/$out_file.hex");
    system("$OBJCOPY    -O binary     hexout/$out_file.elf hexout/$out_file.bin");

}
