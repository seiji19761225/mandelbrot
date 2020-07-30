
M     M
MM   MM    aa    n    n  ddddd   eeeeee  l       bbbbb   rrrrr    oooo    ttttt
M M M M   a  a   nn   n  d    d  e       l       b    b  r    r  o    o     t
M  M  M  a    a  n n  n  d    d  eeeee   l       bbbbb   r    r  o    o     t
M     M  aaaaaa  n  n n  d    d  e       l       b    b  rrrrr   o    o     t
M     M  a    a  n   nn  d    d  e       l       b    b  r   r   o    o     t
M     M  a    a  n    n  ddddd   eeeeee  llllll  bbbbb   r    r   oooo      t

.............::::::::::::::::::::::::::::::::::::::::::::::::.................
.........::::::::::::::::::::::::::::::::::::::::::::::::::::::::.............
.....::::::::::::::::::::::::::::::::::-----------:::::::::::::::::::.........
...:::::::::::::::::::::::::::::------------------------:::::::::::::::.......
:::::::::::::::::::::::::::-------------;;;!:H!!;;;--------:::::::::::::::....
::::::::::::::::::::::::-------------;;;;!!/>&*|I !;;;--------::::::::::::::..
::::::::::::::::::::-------------;;;;;;!!/>)|.*#|>/!!;;;;-------::::::::::::::
::::::::::::::::-------------;;;;;;!!!!//>|:    !:|//!!!;;;;-----:::::::::::::
::::::::::::------------;;;;;;;!!/>)I>>)||I#     H&))>////*!;;-----:::::::::::
::::::::----------;;;;;;;;;;!!!//)H:  #|              IH&*I#/;;-----::::::::::
:::::---------;;;;!!!!!!!!!!!//>|.H:                     #I>/!;;-----:::::::::
:----------;;;;!/||>//>>>>//>>)|%                         %|&/!;;----:::::::::
--------;;;;;!!//)& |;I*-H#&||&/                           *)/!;;-----::::::::
-----;;;;;!!!//>)IH:-        ##                            #&!!;;-----::::::::
;;;;!!!!!///>)H%.**           *                            )/!;;;------:::::::
                                                         &)/!!;;;------:::::::
;;;;!!!!!///>)H%.**           *                            )/!;;;------:::::::
-----;;;;;!!!//>)IH:-        ##                            #&!!;;-----::::::::
--------;;;;;!!//)& |;I*-H#&||&/                           *)/!;;-----::::::::
:----------;;;;!/||>//>>>>//>>)|%                         %|&/!;;----:::::::::
:::::---------;;;;!!!!!!!!!!!//>|.H:                     #I>/!;;-----:::::::::
::::::::----------;;;;;;;;;;!!!//)H:  #|              IH&*I#/;;-----::::::::::
::::::::::::------------;;;;;;;!!/>)I>>)||I#     H&))>////*!;;-----:::::::::::
::::::::::::::::-------------;;;;;;!!!!//>|:    !:|//!!!;;;;-----:::::::::::::
::::::::::::::::::::-------------;;;;;;!!/>)|.*#|>/!!;;;;-------::::::::::::::
::::::::::::::::::::::::-------------;;;;!!/>&*|I !;;;--------::::::::::::::..
:::::::::::::::::::::::::::-------------;;;!:H!!;;;--------:::::::::::::::....
...:::::::::::::::::::::::::::::------------------------:::::::::::::::.......
.....::::::::::::::::::::::::::::::::::-----------:::::::::::::::::::.........
.........::::::::::::::::::::::::::::::::::::::::::::::::::::::::.............
.............::::::::::::::::::::::::::::::::::::::::::::::::.................

Copyright (c) 2010-2011,2014-2015,2017
     Seiji Nishimura, All rights reserved.
E-Mail: seiji1976@gmail.com

$Id: README.txt,v 1.1.1.3 2017/07/27 00:00:00 seiji Exp seiji $

THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

1. Introduction

This directory contains program examples of parallel Mandelbrot set
renderer with adaptive anti-aliasing method for escape time. Programs
are written in C language (ISO C99 standard) and parallelized with
OpenCL, MPI and/or OpenMP.

This directory contains followings:
	00.base/	   01.supersampling/		 02.edge/
	03.multisampling/  04.adaptive_mesh_refinement/  05.adaptive_antialiasing/
	Makefile	   README.txt			 doc/
	input/		   make.inc			 utils/

Each subdirectory is:
   doc/		... documents
   input/	... input data sets
   utils/	... common utility library
   [0-9]*/      ... program example

2. Required Environment

Followings are required to compile and run program examples:
   Multi-processor SMP system running UNIX/Linux compatible OS
   C99 programming environment with OpenMP/MPI
   OpenCL library supporting cl_khr_fp64
   GNU make command

3. Customization

Edit config.mk in each subdirectory.

4. Compilation and Execution

   i.) Configuration
      Edit "make.inc" in this directory to configure for targeted system.
         CC	... C99 compiler command
         PFLAGS	... preprocessor macro definition for cpp command
         CFLAGS	... command line option for C99 compiler
         LIBS	... additional libraries
         MPICC	... C99 compiler command for MPI (optional)
         LIBOCL	... OpenCL library supporting cl_khr_fp64 (optional)
      By default, "make.inc" is configured for X86-64 Linux system.

   ii.) Compilation
      Run "make" command in this directory to compile all program examples.
      Executable file, "mandelbrot.exe", is generated in each subdirectory, if
      compilation successfully finished.

   iii.) Execution
      Move into each subdirectory and launch "mandelbrot.exe" executable file.
      Output image file, "output.ppm", is generated in the same directory,
      if execution successfully finished.

5. Graphics API

Each program example uses simple pixmap library, libpixmap lite-edition.
Refer to "API.txt" in doc/ directory for the detail information.

6. License

Refer to "LICENSE.txt" in doc/ directory for the detail information.

EOF.
