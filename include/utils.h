#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

__attribute__((__noreturn__)) static inline 
void internal_die_fn(FILE *outfile, const char *fmt, ...) 
{
    va_list va;
    va_start(va, fmt);

    vfprintf(outfile, fmt, va);
    fprintf(outfile, "\n");

    va_end(va);

    exit(EXIT_FAILURE);
}

#define DIE(...)                                      \
    if (doit4me_outfile != NULL)                      \
        internal_die_fn(doit4me_outfile, __VA_ARGS__)

#define LOG(FMT, ...)                                 \
    if (doit4me_outfile != NULL)                      \
        fprintf(doit4me_outfile, "%s[%d]: " FMT "\n", \
                __func__, __LINE__, ##__VA_ARGS__)

#undef LOG
#define LOG(...)

struct doit4me_string {
    size_t size, count;
    char *str;
};

static inline void str_append(struct doit4me_string *a, const char *b)
{
    fprintf(stderr, "Appending '%s' to '%s'\n", b, a->str);
    size_t b_len = strlen(b);
    a->count += b_len;
    if (a->count >= a->size) {
        while (a->count >= a->size) {
            a->size *= 2;
        }
        a->str = realloc(a->str, a->size);
        if (a->str == NULL) {
            abort();
        }
    }

    strcat(a->str, b);
    assert(a->count == strlen(a->str));
}

#endif /* UTILS_H */
