gcc -fPIC -c pam_reversed_login.c
gcc -shared -o pam_reversed_login.so pam_reversed_login.o -lpam
cp pam_reversed_login.so /lib64/security/
