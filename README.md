this is a c++ app which uses a self-modifying/anti-tampering technique in the .text section to make static analysis harder.

what it does:
keeps the executable code encrypted (XOR) in memory most of the time, and only decrypts individual 4KB pages for a short time (when the CPU needs to execute them).
