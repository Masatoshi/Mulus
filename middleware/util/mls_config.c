/*
 * property configuration.
 */
/*
 * @note XXXX MT-UN-Safe
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include "mls_config.h"
#include "mls_log.h"

#if defined(CDEBUG)
#define CDBG(fmt, ...) do{                   \
        fprintf(stderr, fmt, ##__VA_ARGS__);  \
  }while(0)
#else 
#define CDBG(...) do{}while(0)
#endif

enum prop_type {
    MLS_CONF_EMPTY,
    MLS_CONF_COMMENT,
    MLS_CONF_DATA
};

struct mls_conf_prop
{
    struct mls_conf_prop* p_next;
    enum prop_type p_type;
    char* p_key;
    char* p_val; /* type=COMMENT: one line data */
};

struct mls_conf* mls_conf_ini(char* path)
{
    struct mls_conf* conf;
    conf = malloc(sizeof(*conf));
    if (conf) {
        memset(conf, 0, sizeof(*conf));
        conf->path = strdup(path);
        if (NULL == conf->path) {
            free(conf);
            conf = NULL;
        }
    }
    return conf;
}

void mls_conf_fin(struct mls_conf* conf)
{
    struct mls_conf_prop *pp;
	
    if (NULL == conf) {
        return;
    }
    free(conf->path);
    pp = conf->root;
    while (NULL != pp) {
        struct mls_conf_prop *pnext = pp->p_next;
        switch (pp->p_type) {
        case MLS_CONF_DATA:
            free(pp->p_key);
        case MLS_CONF_COMMENT:
            free(pp->p_val);
        case MLS_CONF_EMPTY:
            free(pp);
            break;
        default:
            MLS_FATAL("Invalid type -> %d", pp->p_type);
        }
        pp = pnext;
    }
    free(conf);
}

void mls_conf_load(struct mls_conf* conf)
{
    FILE* fp;
    char linebuf[MLS_CONF_LEN_LINE];
    int lineno = 0;
    struct mls_conf_prop **ptail = &(conf->root);

    if (NULL == (fp = fopen(conf->path, "r"))) {
        MLS_FATAL_PERROR("Property file '%s' open failed", conf->path);
    }

    while (NULL != fgets(linebuf, sizeof(linebuf)-1, fp)) {
        char* cp = linebuf;
        char* savep;
        struct mls_conf_prop* pp;

        lineno += 1;
        linebuf[sizeof(linebuf)-1] = '\0';

        while ('\0' != *cp && isspace(*cp)) {
            cp += 1; /* skip white-space */
        }
        savep = cp;

        /* empty line -or- comment line */
        if ('\0' == *cp || MLS_CONF_COMMENT_PREFIX == *cp) {
            pp = malloc(sizeof(*pp));
            assert(NULL != pp);
            memset(pp, 0, sizeof(*pp));
            if ('\0' == *cp) {
                pp->p_type = MLS_CONF_EMPTY;
            } else if (MLS_CONF_COMMENT_PREFIX == *cp) {
                int tail = strlen(cp);
                pp->p_type = MLS_CONF_COMMENT;
                pp->p_val = strdup(cp);
                assert(NULL != pp->p_val);
                pp->p_val[tail - 1] = '\0'; /* terminate new-line */
            }
            (*ptail) = pp;
            ptail = &(pp->p_next);
            CDBG("Comment -or- empty '%s', %s@%d\n", savep, path, lineno);
            continue; 
        }

        while ('\0' != *cp && !isspace(*cp) && MLS_CONF_KEYVAL_SEP != (*cp)) {
            cp += 1; /* scan key string */
        }
        if ('\0' == *cp) {
  no_value:
            CDBG("No value for key '%s', %s@%d\n", savep, path, lineno);
            continue;
        }

        if (MLS_CONF_KEYVAL_SEP != *cp) {
            *cp = '\0';
            cp += 1;

            while (MLS_CONF_KEYVAL_SEP != *cp && '\0' != *cp) {
                cp += 1;
            }
            if ('\0' == *cp) {
                CDBG("No '=' for key '%s', %s@%d\n", 
                    savep, path, lineno);
                continue;
            }
        }
        else{
            *cp = '\0';
        }

        cp += 1;
        while ('\0' != *cp && isspace(*cp)) {
            cp += 1; /* skip white-space before value string */
        }
        if ('\0' == *cp) {
            goto no_value;
        }

        pp = malloc(sizeof(*pp));
        assert(NULL != pp);
        memset(pp, 0, sizeof(*pp));
        pp->p_type = MLS_CONF_DATA;
        pp->p_key = strdup(savep);
        assert(NULL != pp->p_key);

        savep = cp;
        cp = savep + strlen(cp) - 1;
        while (cp > savep && isspace(*cp)) {
            cp -= 1;
        }
        cp[1] = '\0';
        pp->p_val = strdup(savep);
        assert(NULL != pp->p_val);

        (*ptail) = pp;
        ptail = &(pp->p_next);
        CDBG("%d: '%s' '%s'\n", lineno, pp->p_key, pp->p_val);
    }

    (void) fclose(fp);
}

void mls_conf_store(struct mls_conf* conf)
{
    FILE* fp;
    struct mls_conf_prop *pp;

    // XXXX Backup & Restore
    if (NULL == (fp = fopen(conf->path, "w"))) {
        MLS_FATAL_PERROR("Property file '%s' open failed", conf->path);
    }

    for (pp = conf->root; NULL != pp; pp = pp->p_next) {
        switch (pp->p_type) {
        case MLS_CONF_DATA:
            fprintf(fp, "%s", pp->p_key);
            fprintf(fp, " %c ", MLS_CONF_KEYVAL_SEP);
        case MLS_CONF_COMMENT:
            fprintf(fp, "%s", pp->p_val);
        case MLS_CONF_EMPTY:
            fprintf(fp, "\n");
            break;
        default:
            MLS_FATAL("Invalid type -> %d", pp->p_type);
        }
    }

    (void) fclose(fp);
}

char* mls_conf_get(struct mls_conf* conf, char* key)
{
    struct mls_conf_prop *pp;
    for (pp = conf->root; NULL != pp; pp = pp->p_next) {
        if ((MLS_CONF_DATA == pp->p_type)
            && (0 == strcmp(pp->p_key, key)))
        {
            return pp->p_val;
        }
    }
    return NULL;
}

int mls_conf_set(struct mls_conf* conf, char* key, char* val)
{
    int ret = 0;
    struct mls_conf_prop **ppp, *pp = NULL;

    for (ppp = &(conf->root); NULL != (*ppp); ppp = &((*ppp)->p_next)) {
        if ((MLS_CONF_DATA == (*ppp)->p_type)
            && (0 == strcmp((*ppp)->p_key, key)))
        {
            /* found key */
            char *newval = strdup(val);
            if (NULL == newval) {
                ret = -errno;
                goto out;
            }
            free((*ppp)->p_val);
            (*ppp)->p_val = newval;
            goto out;
        }
    }
    /* not found key */
    pp = malloc(sizeof(*pp));
    if (NULL == pp) {
        ret = -errno;
        goto out;
    }
    memset(pp, 0, sizeof(*pp));

    pp->p_type = MLS_CONF_DATA;
    pp->p_key = strdup(key);
    pp->p_val = strdup(val);
    if (NULL == pp->p_key || NULL == pp->p_val) {
        ret = -errno;
        goto out;
    }
    *(ppp) = pp;
out:
    if (0 != ret && NULL != pp) {
        if (pp->p_key) {
            free(pp->p_key);
        }
        if (pp->p_val) {
            free(pp->p_val);
        }
        free(pp);
    }
    return ret;
}

#if 0
////////////////////////////////////////////
////////////////////////////////////////////
////////////////////////////////////////////

void
prop_init(char* path)
{
    FILE* fp;
    char linebuf[256];
    int lineno = 0;

    fp = fopen(path, "r");
    if(NULL == fp){
        fatalperror("Property file '%s' open failed", path);
    }

    while(NULL != fgets(linebuf, sizeof(linebuf)-1, fp)){
        char* cp = linebuf;
        char* savep;
        prop_t* pp;

        lineno += 1;
        linebuf[sizeof(linebuf)-1] = '\0';

        while('\0' != *cp && isspace(*cp)){
            cp += 1; /* 先頭の空白文字をスキップする */
        }

        if('\0' == *cp || '#' == *cp){
            continue; /* 空白行もしくはコメント行 */
        }

        savep = cp;
        while('\0' != *cp && !isspace(*cp) && ':' != (*cp)){
            cp += 1; /* キー文字列を走査する */
        }
        if('\0' == *cp){
        no_value:
            fprintf(stderr, "No value for key '%s', %s@%d\n", 
                savep, path, lineno);
            continue;
        }

        if(':' != *cp){
            *cp = '\0';
            cp += 1;

            while(':' != *cp && '\0' != *cp){
                cp += 1;
            }
            if('\0' == *cp){
                fprintf(stderr, "No ':' for key '%s', %s@%d\n", 
                    savep, path, lineno);
                continue;
            }
        }
        else{
            *cp = '\0';
        }

        cp += 1;
        while('\0' != *cp && isspace(*cp)){
            cp += 1; /* 値の前の空白文字をスキップする */
        }
        if('\0' == *cp){
            goto no_value;
        }

        pp = malloc(sizeof(*pp));
        pp->p_next = NULL;
        pp->p_key = strdup(savep);

        savep = cp;
        cp = savep + strlen(cp) - 1;
        while(cp > savep && isspace(*cp)){
            cp -= 1;
        }
        cp[1] = '\0';
        pp->p_value = strdup(savep);

        pp->p_next = root;
        root = pp;
        printf("%d: '%s' '%s'\n", lineno, pp->p_key, pp->p_value);
    }

    (void) fclose(fp);
}
#endif