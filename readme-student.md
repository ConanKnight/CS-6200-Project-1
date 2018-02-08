# Project README file

[-] Issues
[+] Insights

[-] I underestimated how much it would take to manually set up the environment (trying to do it from scratch and having not used pipenv before)

[-] Definitely made multiple submissions where i had "return 1;" at the end of main and couldn't figure out why I was failing tests. Lesson learned.

[+] It took some mental gymnastics, but reading a piazza post (which I can't remember where it is) made me consider and realize that instead of having to send some kind of EOF signal to the client in the "transfer" project, I can just close the connection, and when the client reads 0 bytes, it will know that the transfer is complete.

[-] I now realize as I am trying to do some in-house testing before sending code to Bonnie that the makefile isn't compiling properly, with the error "/usr/bin/ld: i386:x86-64 architecture of input file `handler.o' is incompatible with i386 output". It turns out that I used a 32-bit ISO of Ubuntu 16.04 when I built this VM, so the handler.o binary that was shipped with the code is incompatible.

## Project Description
We will manually review your file looking for:

- A summary description of your project design.  If you wish to use graphics, please simply use a URL to point to a JPG or PNG file that we can review

- Any additional observations that you have about what you've done. Examples:
	- __What created problems for you?__
	- __What tests would you have added to the test suite?__
	- __If you were going to do this project again, how would you improve it?__
	- __If you didn't think something was clear in the documentation, what would you write instead?__

## Known Bugs/Issues/Limitations

__Please tell us what you know doesn't work in this submission__
[-] "ssize_t getfile_handler(gfcontext_t *ctx, char *path, void* arg)" was not in the gfserver.c file that I downloaded from the repository. It's declaration was in the gfserver.h header file though.

## References

[+] Beej's guide to socket programming was a huge help in getting the echo server going. Definitely read that through pretty deliberately before I even started programming. Tried to mirror a lot of his code for the initial functionality, and an in-depth understanding of what it did helped with the edits to make the echo work. Still had trouble understanding exactly how the "(struct sockaddr_in *)" funtion pointer as used to reference the "their_addr" struct allowed him to get the addresss of the connection, but since this functionality isn't required for echo server, I put that as a "to research later" note and moved on.
http://beej.us/guide/bgnet/html/single/bgnet.html 

[+] The C programming tutorial on file I/O was helpful in getting the synatx right for binary file reading and writing, which was new to me.
https://www.cprogramming.com/tutorial/cfileio.html

__Please include references to any external materials that you used in your project__

