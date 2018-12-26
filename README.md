The best debugger around

# Usage

* Link libdoit4me.so to your project
* Compile with -g -rdynamic
* Set "const char *doit4me_pastebin_apikey" to a string literal within your program
* Alternatively set PASTEBIN_API_KEY to your pastebin dev key
* ???
* Profit

# Requirements

* libunwind
* curl
* sed
* awk
* indent
* addr2line

# Test run

    $ ./driver/driver

    Starting driver!
    =========================================================
    Programming is hard.
    Do you want someone else to solve your problem? [y/n]y

The code then outputs the following markdown:

#####################

Hey I'm having a problem with my program that I need to finish for ~~homework~~ a personal project.

I'm having an issue in this function here:

    void baz(void)
    {
        raise(SIGSEGV);
    /*** ERROR HERE - LINE 10 ***/
    }



I got the following stacktrace:

    driver.c[10] - baz()
    driver.c[15] - bar()
    driver.c[20] - foo()
    driver.c[28] - main()


You can take a look at the source code [here](https://pastebin.com/s5SYse9z)
Thanks so much for the help!


#######################


By default the pastebin only lasts for 10 minutes - hopefully pastebin doesn't ban me

# Use

If you want to test this out (for some reason), do the following:

    $ mkdir build; cd build
    $ cmake ..
    $ # Make sure awk and addr2line are on your PATH
    $ # Now ./libdoit4me.so is there
    $ # Get a pastebin API dev code on https://pastebin.com/api
    $ export PASTEBIN_API_CODE=<code>
    $ # Now link to ./libdoit4me.so, and run!
    $ # This repo provides an example driver
    $ ./driver/driver
