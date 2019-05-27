# Homework 03

## Introduction

- In this homework, we'll  create a  module  "mtest", which
  accepts three kinds of input: ***"listvma"***,  ***"findpage"***  and
  ***"writeval"***
- ==**"listvma"**==
	- List all virtual  memory  area  blocks of the current process in
	  the format of start-addr end-addr permission.
- ==**"findpage address"**==
	- Find the physical address which the given virtual address translated
	  to.
- ==**"writeval address value"**==
	- Change the value in  the current process' virtual address to the
	  given value.

----

## Usage

- Compile *test.c*: `gcc test.c`.
- Run the executable file just generated.
	- It'll show the value and the virtuall address of  the  variables
	  when the program starts.
	- Enter the commands described in the introduction.  It will  send
	  them to */proc/mtest*.
	- Enter ***"showval"*** if you want to see the value of the variables
	in the program.
- You can see the result by typing `dmesg` in another terminal.
