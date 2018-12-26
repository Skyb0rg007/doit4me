#include <stdio.h>
#include <signal.h>

/* const char *doit4me_outfile_name = "log.txt"; */
const char *doit4me_pastebin_apikey = "dd7c93d31880e92c00d489c661687c76";

void baz(void)
{
    raise(SIGSEGV);
}

void bar(void)
{
    baz();
}

void foo(void)
{
    bar();
}

int main(void)
{
    puts("Starting driver!");

    foo();
    
    return 0;
}
