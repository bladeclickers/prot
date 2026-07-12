this is a c++ dll which uses a self-modifying/anti-tampering technique in the .text section to make static analysis harder.

what it does:
keeps the executable code encrypted (XOR) in memory most of the time, and only decrypts individual 4KB pages for a short time (when the CPU needs to execute them).

the code section sits encrypted in memory almost all of the time; each page is decrypted only when execution actually jumps into it (via the page-fault trampoline), then re-encrypted later once the cache fills up.

set tdghjjgyfjtyr() to be the dll entry point

please for the love of god use a better way to store the xor key
