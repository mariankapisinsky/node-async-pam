gcc -fPIC -fno-stack-protector -c pam_reversed_login.c
ld -x --shared -o /lib64/security/pam_reversed_login.so pam_reversed_login.o
