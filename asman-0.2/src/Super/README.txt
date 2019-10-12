Compile this software with Turbo C 2.01 for MS-DOS, 16-bit real-mode
binaries are the targets.  Use more modern software such as `bcc' or
DJGPP if you dare, but be forewarned that porting will be required.

N.B.: A very nice but simple extension to add to this software: Send
disk images over the COM serial port.  Via BIOS COM port routines,
this is very easy to implement.  Also, this should be tested and
verified to work with hard disk images too.  At 115200 baud, it would
take 49.5 minutes to transfer a 40 MB hard disk image.  If you cannot
sustain a baud this high, it will take several hours to transfer the
image!  Of course for more modern PCs with larger hard disks, you will
have an Ethernet card at your disposable and should be able to get 1
MB/sec, but for the oldest of the old, COM serial port transfers are
the way to go for "network" data transfers.

Either that, or LPT parallel port transfers.  For the oldest of the
old (older than IBM PS/2), LPT transfers are one way: you can send
disk images away, but you can't transfer them in.  Also, please note
LPT transfers are not necessarily faster than serial data transfers.
If your serial connection's maximum communication rate is slower than
1000 bytes per second, then sure, the parallel port could just as well
give you a faster transfer speed for your one-way ticket off the old
hardware.  Of course, you wouldn't want to spend 11.7 hours to
transfer a 40 MB hard disk image over a parallel port!

TODO: Other nice-to-haves: Read/write size header for stream sources.
Could also enable for files.  Read/write directly to memory, need to
specify a size for this. Decode/encode hexidecimal on read/write.
Execute code after write to memory.  Reboot after finish copying to
disk.  Also, we can support WattTCP and Watt32-TCP, in which case we
essentially bundle a lightweight "netcat".  Heck, we're basically
reinventing for ourselves a stripped down version of U-boot, aren't
we?  That too, and a "monitor" program of a sort.  This compiled
binary should also have a data structure with a magic numbers and
initial parameter values so it is easy to modify the binary image to
bake in certain parameters, without the need for a recompile.  Also,
support for initializing/formatting floppy disks should be built-in.
That was one of my biggest frustrations with Apple II Disk Transfer
ProDOS!  You've got some new floppy disks, but initializing
then... well, that may have been included, but not very elegant.

I could also add support for card image to Unix newlines, newline
conversion, and byte swapping.  Heck, why don't I turn this into a
full miniature dd?  dd for DOS is the name of the game.

Okay, let's be clear on my grand plans.  This is NOT a monitor program
in terms of the extended features and functions.  The extended
features and functions, beyond the concept of "DD for DOS" is mainly a
bootloader.  Some particular programs of interest that you would want
to bootload are a more "Apple Disk Transfer" style client, a monitor
program, and my own Unix.

Please note there is a somewhat clever way to automate more
complicated network boot procedures on a simple serial connection with
a simple bootloader.  The bootloader starts up listening for an image
on the serial connection.  The PC host side doesn't know if there is a
client connected there, so it periodically pings out a very small boot
program simply meant to send a ping back and revert back to the
bootloader.  If the PC receives the ping back, it knows there is a
machine connected, so it sends back the full destination boot program
for that particular machine.  Heck, I might as well just include ping
back directly in the initial bootloader as an option.  Essentially the
analog of a "READY" signal.

Interactive mode is a must if we boot directly off of a boot sector
without MS-DOS.  This would also include a user interface to configure
serial parameters.

Important!  How do we know the byte order is correct?  Make sure we
replicate this good old Unix trick.  Checksum the image on both sides,
and report back.  If the checksum is wrong, restart with the byte
order swapped, try again, and then checksum it.  Optional, of course.
But checksumming is important.

Although I must admit I have to question.  Are full conversions really
worth it for this program?  For low-level early boot and faithful
imaging, we have a different concept and world view.  For
archival-grade copies, there absolutely can't be any issue with
uncertainty of the byte order.  With faithful emulation in the
destination environment, there is absolutely no reason for card image
conversion or newline conversion.

So, I think I have to put things clear for this concept.  This isn't
meant to be a full traditional Unix `dd'.  Archiving and native
booting are the key focus and emphasis here.  The responsibility of a
full Unix-style `dd' lies in a separate program implementation.  That
being said, we do still support decode/encode hexidecimal as it is
sometimes necessary for data transfer and booting.

Wow, this program is pretty fully developed now and the information on
it is growing fast!  I need to have an organized documentation
discipline on this.  Demos, demos, demos.  Learn by example for sure
we must champion.
