## LL-GDB
### Low-Layer GDB : Yet another GUI for GDB, the GNU debugger.
This is a graphic front-end for the GNU debugger GDB, this one is coded in C++ and based on the GTK2 widget library.

Why choose this one?
* If you need a tool for fighting bugs in a project you are developing, you will surely prefer a debugger interface integrated in your favorite IDE, so that you can navigate through your source file and the data structures of your application. A respectable IDE for C++ is [Codeblocks](http://www.codeblocks.org/). 
* If you are on the dark side and need a tool for doing advanced reverse-engineering on programs developed by others, you may be happier with some specialized tool (something like [radare2](https://rada.re/)).
### LL-GDB is just between these extremes.
* you don't need to own the source code of the target application to use LL-GDB
* but if you have it, it will be gently inserted in the disassembly listing
* LLGDB is much simpler to use than those advanced reverse-engineering tool focused on hacking (but of course its capabilities are modest)

### This is a work in progress.
This is where we are for the moment :
* X86 (aka i686) and AMD64 (aka X86-64) architectures are supported
* you need GDB 8.2 or higher
* Windows 7-8-10 host only (Linux is ongoing)
* development based on MinGW, [MSYS2 distribution](https://www.msys2.org/) (GTK2 and GDB are available as optional packages in MSYS2) 
