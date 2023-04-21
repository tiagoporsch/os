# os

https://user-images.githubusercontent.com/10602379/233748436-6f452b75-508e-4072-ad80-ad13d96c1738.mp4

My x86-64 hobby operating system. Cooperative multitasking system with no user-mode support, everything runs on ring 0 (for now). Packed with a really simple libc (not compliant with standards at all), uses [my own filesystem design](https://github.com/tiagoporsch/tfs), and has very basic support for programs.

There's nothing more to explain that the source code does a thousand times better. It's a fairly simple read.

## Bootloader
All the bootloader does is:
* use BIOS Extensions to load itself at 0x7E00 (as 0x7C00-0x7CFF is already loaded by the bios)
* get memory information and store it at 0x7000
* enter long mode
* map the complete physical memory to 0xFFFFFFC000000000
* load the file '/boot/kernel.elf' and map it to 0xFFFFFF8000000000
* creates a bitmap that contains information about which memory frames (units of 4096 KB of physical RAM) are being used at 0x100000
* jumps to the kernel's entry point

## Developer notes
### Lower Memory Map
This is the state of the physical memory under 0x100000.

|Start  |End    |Size  |Description |
|-------|-------|------|------------|
|0x00000|0x00FFF|  4 KB|Reserved    |
|0x01000|0x02FFF|  8 KB|Pages       |
|0x03000|0x04FFF|  8 KB|TSS Stack   |
|0x05000|0x06FFF|  8 KB|Kernel Stack|
|0x07000|0x07BFF|  3 KB|Boot info   |
|0x07C00|0x7FFFF|481 KB|Boot code   |
|0x80000|0xFFFFF|512 KB|Reserved    |
