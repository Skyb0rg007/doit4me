#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include <curl/curl.h>
#include <libunwind.h>

#include "config.h"
#include "utils.h"

/* Config globals */
__attribute__((__weak__)) const char *doit4me_outfile_name = NULL;
__attribute__((__weak__)) const char *doit4me_pastebin_apikey = NULL;

/* Global variables */

/* The program executable itself */
static char *g_progname; 

/* The list of functions in the stack-trace */
struct doit4me_fn_info {
    char *fn_name, *file_name; long line_no;
};
static struct doit4me_fn_info *g_backtrace = NULL;
static size_t g_backtrace_len = 0;

/* The file to output to */
static FILE *doit4me_outfile = NULL;

/* Static functions */
static void init_progname(void);
static void destroy_progname(void);
static void load_backtrace(int sig);
static void destroy_backtrace(void);
static void add_backtrace(unw_word_t ip, const char *buf);
static void print_backtrace(char *pastebin);
static char *pastebin_it(void);
static void prompt_user(void);

__attribute__((constructor))
void doit4me_init(void)
{
    /* Setup printing first */
    if (doit4me_outfile_name == NULL) {
        doit4me_outfile = stderr;
    } else if (doit4me_outfile_name == (const char *)-1) {
        doit4me_outfile = stdout;
    } else {
        doit4me_outfile = fopen(doit4me_outfile_name, "w");
        if (doit4me_outfile == NULL) {
            abort();
        }
    }

    /* Find the program executable name */
    init_progname();

    /* Enable CURL */
    curl_global_init(CURL_GLOBAL_ALL);

    struct sigaction sa;
    sa.sa_handler = load_backtrace;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND | SA_NODEFER;
    sigaction(SIGSEGV, &sa, NULL);

}

__attribute((destructor))
void doit4me_destroy(void)
{
    destroy_backtrace();

    destroy_progname();

    if (doit4me_outfile != stderr && doit4me_outfile != stdout) {
        fclose(doit4me_outfile);
    }
}

static void init_progname(void)
{
    g_progname = realpath("/proc/self/exe", NULL);
    if (g_progname == NULL) {
        DIE("Could not read program name");
    }
}

static void destroy_progname(void)
{
    free(g_progname);
}

struct doit4me_mem_struct {
    size_t size; char *mem;
};
static size_t pastebin_curl_callback(void *contents, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    struct doit4me_mem_struct *mem = data;

    mem->mem = realloc(mem->mem, mem->size + realsize + 1);
    if (mem->mem == NULL) {
        DIE("Out of memory");
    }

    memcpy(&mem->mem[mem->size], contents, realsize);
    mem->size += realsize;
    mem->mem[mem->size] = '\0';

    return realsize;
}

static char *pastebin_it(void)
{
    static const char *pastebin_url = "https://pastebin.com/api/api_post.php";

    LOG("Starting pastebin_it");
    LOG("API-KEY = %s", doit4me_pastebin_apikey);

    if (g_backtrace_len == 0) {
        DIE("No backtrace to print!");
    }

    char **sourcefiles = calloc(g_backtrace_len, sizeof *sourcefiles);
    size_t n = 0;

    for (size_t i = 0; i < g_backtrace_len; i++) {
        char *filename = g_backtrace[i].file_name;

        /* Ensure no repeats */
        for (size_t j = 0; j < n; j++) {
            if (strcmp(sourcefiles[j], filename) == 0) {
                goto next_backtrace;
            }
        }

        sourcefiles[n++] = filename;
next_backtrace: (void)0;
    }

    struct doit4me_string code = {
        .size = 128, .count = 0,
        .str = calloc(128, sizeof (char))
    };

    for (size_t i = 0; i < n; i++) {
        FILE *f = fopen(sourcefiles[i], "rt");
        if (f == NULL) {
            LOG("Error opening %s: %s", sourcefiles[i], strerror(errno));
            continue;
        }

        fseek(f, 0, SEEK_END);
        long file_len = ftell(f);
        rewind(f);

        char *file_content = calloc(file_len + 1, sizeof (char));
        size_t num_read = fread(file_content, sizeof (char), file_len, f);
        if (num_read != (size_t)file_len) {
            DIE("Error reading %s: %s", sourcefiles[i], strerror(errno));
        }

        LOG("About to append");

        str_append(&code, "\n==== ");
        str_append(&code, sourcefiles[i]);
        str_append(&code, " ====\n");
        str_append(&code, file_content);

        free(file_content);
        fclose(f);
    }
    free(sourcefiles);

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        DIE("Could not initialize CURL");
    }

    char *escaped_code = curl_easy_escape(curl, code.str, code.count);
    size_t escaped_len = strlen(escaped_code);
    free(code.str);

    size_t postfield_len = escaped_len + 256;
    char *postfield = calloc(postfield_len, sizeof *postfield);
    snprintf(postfield, postfield_len,
            "api_option=paste&\
            api_paste_private=1&\
            api_paste_expire_date=%s&\
            api_paste_format=c&\
            api_dev_key=%s&\
            api_paste_code=%s",
            "10M", doit4me_pastebin_apikey, escaped_code);
    curl_free(escaped_code);

    LOG("Postdata: %s", postfield);

    struct doit4me_mem_struct response = {
        .size = 0, .mem = NULL
    };

    curl_easy_setopt(curl, CURLOPT_URL, pastebin_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, pastebin_curl_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "doit4me-agent/1.0");
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfield);
    curl_easy_setopt(curl, CURLOPT_STDERR, doit4me_outfile);

    int err = curl_easy_perform(curl);
    if (err != CURLE_OK) {
        DIE("Error making pastebin request: %s", curl_easy_strerror(err));
    }
    free(postfield);
    curl_easy_cleanup(curl);

    return response.mem;
}

static void load_backtrace(int sig)
{
    (void)sig;

    unw_context_t uc;
    unw_getcontext(&uc);
    unw_cursor_t cursor;
    unw_init_local(&cursor, &uc);

    while (unw_step(&cursor) > 0) {
        unw_word_t ip;
        unw_get_reg(&cursor, UNW_REG_IP, &ip);

        char buf[256];
        unw_word_t offset;
        unw_get_proc_name(&cursor, buf, sizeof buf, &offset);

        add_backtrace(ip, buf);
    }

    prompt_user();

    exit(12);
}

static void add_backtrace(unw_word_t ip, const char *fn_name)
{
    g_backtrace = realloc(g_backtrace, sizeof (*g_backtrace) * (++g_backtrace_len));
    if (g_backtrace == NULL) {
        DIE("Out of memory!");
    }

    LOG("Adding '%s : %lx' to the backtrace", fn_name, ip);
    g_backtrace[g_backtrace_len - 1].fn_name = strdup(fn_name);

    char cmd[533];
    snprintf(cmd, sizeof cmd, "" ADDR2LINE " -e '%s' -- %lx",
            g_progname, ip);
    FILE *addr_info = popen(cmd, "r");

    char *sourcefilename = NULL;
    size_t sourcefilename_len = 0;
    ssize_t ret = getdelim(&sourcefilename, &sourcefilename_len, ':', addr_info);
    if (ret == -1) {
        DIE("Error reading source file from addr2line");
    }
    sourcefilename_len = strlen(sourcefilename);
    sourcefilename[sourcefilename_len - 1] = '\0';
    g_backtrace[g_backtrace_len - 1].file_name = sourcefilename;

    char *sourcefileno = NULL;
    size_t sourcefileno_len = 0;
    ret = getline(&sourcefileno, &sourcefileno_len, addr_info);
    if (ret == -1) {
        DIE("Error reading file number from addr2line");
    }
    sourcefileno_len = strlen(sourcefileno);
    sourcefileno[sourcefileno_len - 1] = '\0';
    g_backtrace[g_backtrace_len - 1].line_no = strtol(sourcefileno, NULL, 10);

    pclose(addr_info);
    free(sourcefileno);
}

static void destroy_backtrace(void)
{
    while (g_backtrace_len--) {
        free(g_backtrace[g_backtrace_len].file_name);
        free(g_backtrace[g_backtrace_len].fn_name);
    }

    free(g_backtrace);
}

static void print_backtrace(char *pastebin)
{
    FILE *outfile = tmpfile();
    if (outfile == NULL) {
        DIE("Error creating temp file: %s", strerror(errno));
    }

    fputs("Hey I'm having a problem with my program "
            "that I need to finish for ~~homework~~ a personal project.\n\n"
            "I'm having an issue in this function here:\n\n", outfile);
    fprintf(outfile, "    %s\n    %s\n", g_backtrace[2].fn_name, "void todo() { }");
    fputs("\n\nI got the following stacktrace:\n\n", outfile);

    for (size_t i = 2; i < g_backtrace_len; i++) {
        LOG("%zu: %s() - %s : %ld", i, g_backtrace[i].fn_name, 
                g_backtrace[i].file_name, g_backtrace[i].line_no);

        fprintf(outfile, "    %s[%ld] - %s()\n",
                basename(g_backtrace[i].file_name), g_backtrace[i].line_no, 
                g_backtrace[i].fn_name);

    }

    fprintf(outfile, "\n\nYou can take a look at the source code [here](%s)\n",
            pastebin);
    fprintf(outfile, "Thanks so much for the help!\n");

    rewind(outfile);
    char *line = NULL;
    size_t n = 0;
    while (getline(&line, &n, outfile) != -1) {
        printf("%s", line);
    }
    free(line);

    fclose(outfile);
}

static void prompt_user(void)
{
    fprintf(stderr, "=========================================================\n");
    fprintf(stderr, "Programming is hard.\n");
    fprintf(stderr, "Do you want someone else to solve your problem? [y/n]");

    int c;
    do {
        c = getchar();
    } while (c != 'y' && c != 'n' && c != 'Y' && c != 'N' && c != EOF);

    LOG("Got response '%c'", c);

    if (c == 'y' || c == 'Y') {
        char *url = "<url>";// pastebin_it();
        fprintf(stderr, "Go look at %s\n", url);
        /* free(url); */
        print_backtrace(url);
    } else {
        fprintf(stderr, "\nGood luck\n");
    }

    exit(EXIT_SUCCESS);
}
