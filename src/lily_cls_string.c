#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "lily_alloc.h"
#include "lily_value.h"
#include "lily_vm.h"
#include "lily_seed.h"

#include "lily_cls_list.h"
#include "lily_cls_option.h"

void lily_destroy_string(lily_value *v)
{
    lily_string_val *sv = v->value.string;

    if (sv->string)
        lily_free(sv->string);

    lily_free(sv);
}

static lily_string_val *make_sv(lily_vm_state *vm, int size)
{
    lily_string_val *new_sv = lily_malloc(sizeof(lily_string_val));
    char *new_string = lily_malloc(sizeof(char) * size);

    new_sv->string = new_string;
    new_sv->size = size - 1;
    new_sv->refcount = 1;

    return new_sv;
}

#define CTYPE_WRAP(WRAP_NAME, WRAPPED_CALL) \
void WRAP_NAME(lily_vm_state *vm, uint16_t argc, uint16_t *code) \
{ \
    lily_value **vm_regs = vm->vm_regs; \
    lily_value *ret_arg = vm_regs[code[0]]; \
    lily_value *input_arg = vm_regs[code[1]]; \
\
    if (input_arg->value.string->size == 0) { \
        lily_move_integer(ret_arg, 0); \
        return; \
    } \
\
    char *loop_str = input_arg->value.string->string; \
    int i = 0; \
\
    lily_move_integer(ret_arg, 1); \
    for (i = 0;i < input_arg->value.string->size;i++) { \
        if (WRAPPED_CALL(loop_str[i]) == 0) { \
            ret_arg->value.integer = 0; \
            break; \
        } \
    } \
}

CTYPE_WRAP(lily_string_is_digit, isdigit)
CTYPE_WRAP(lily_string_is_alpha, isalpha)
CTYPE_WRAP(lily_string_is_space, isspace)
CTYPE_WRAP(lily_string_is_alnum, isalnum)

/* This table indicates how many more bytes need to be successfully read after
   that particular byte for proper utf-8. -1 = invalid.
   Table copied from lily_lexer.c */
static const char follower_table[256] =
{
     /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 1 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 2 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 3 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 4 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 5 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 6 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 7 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 8 */-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 9 */-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* A */-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* B */-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* C */-1,-1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/* D */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/* E */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
/* F */ 4, 4, 4, 4, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

void lily_string_ends_with(lily_vm_state *vm, uint16_t argc, uint16_t *code)
{
    lily_value **vm_regs = vm->vm_regs;
    lily_value *input_arg = vm_regs[code[1]];
    lily_value *suffix_arg = vm_regs[code[2]];
    lily_value *result_arg = vm_regs[code[0]];

    char *input_raw_str = input_arg->value.string->string;
    char *suffix_raw_str = suffix_arg->value.string->string;
    int input_size = input_arg->value.string->size;
    int suffix_size = suffix_arg->value.string->size;

    if (suffix_size > input_size) {
        lily_move_boolean(result_arg, 0);
        return;
    }

    int input_i, suffix_i, ok = 1;
    for (input_i = input_size - 1, suffix_i = suffix_size - 1;
         suffix_i >= 0;
         input_i--, suffix_i--) {
        if (input_raw_str[input_i] != suffix_raw_str[suffix_i]) {
            ok = 0;
            break;
        }
    }

    lily_move_boolean(result_arg, ok);
}

void lily_string_find(lily_vm_state *vm, uint16_t argc, uint16_t *code)
{
    lily_value **vm_regs = vm->vm_regs;
    lily_value *input_arg = vm_regs[code[1]];
    lily_value *find_arg = vm_regs[code[2]];
    lily_value *result_arg = vm_regs[code[0]];

    char *input_str = input_arg->value.string->string;
    int input_length = input_arg->value.string->size;

    char *find_str = find_arg->value.string->string;
    int find_length = find_arg->value.string->size;

    if (find_length > input_length ||
        find_length == 0) {
        lily_move_shared_enum(result_arg, lily_get_none(vm));
        return;
    }

    char find_ch;
    int i, j, k, length_diff, match;

    length_diff = input_length - find_length;
    find_ch = find_str[0];
    match = 0;

    /* This stops at length_diff for two reasons:
       * The inner loop won't have to do a boundary check.
       * Search will stop if there isn't enough length left for a match
         (ex: "abcdef".find("defg")) */
    for (i = 0;i <= length_diff;i++) {
        if (input_str[i] == find_ch) {
            match = 1;
            /* j starts at i + 1 to skip the first match.
               k starts at 1 for the same reason. */
            for (j = i + 1, k = 1;k < find_length;j++, k++) {
                if (input_str[j] != find_str[k]) {
                    match = 0;
                    break;
                }
            }
            if (match == 1)
                break;
        }
    }

    lily_instance_val *source;
    if (match) {
        lily_value *v = lily_new_value(VAL_IS_INTEGER,
                (lily_raw_value){.integer = i});
        source = lily_new_some(v);
        lily_move_enum(result_arg, source);
    }
    else
        lily_move_shared_enum(result_arg, lily_get_none(vm));
}

/* Scan through 'input' in search of html characters to encode. If there are
   any, then vm->vm_buffer is updated to contain an html-safe version of the
   input string.
   If no html characters are found, then 0 is returned, and the caller is to use
   the given input buffer directly.
   If html charcters are found, then 1 is returned, and the caller should read
   from vm->vm_buffer->message. */
int lily_maybe_html_encode_to_buffer(lily_vm_state *vm, lily_value *input)
{
    lily_msgbuf *vm_buffer = vm->vm_buffer;
    lily_msgbuf_flush(vm_buffer);
    int start = 0, stop = 0;
    char *input_str = input->value.string->string;
    char *ch = &input_str[0];

    while (1) {
        if (*ch == '&') {
            stop = (ch - input_str);
            lily_msgbuf_add_text_range(vm_buffer, input_str, start, stop);
            lily_msgbuf_add(vm_buffer, "&amp;");
            start = stop + 1;
        }
        else if (*ch == '<') {
            stop = (ch - input_str);
            lily_msgbuf_add_text_range(vm_buffer, input_str, start, stop);
            lily_msgbuf_add(vm_buffer, "&lt;");
            start = stop + 1;
        }
        else if (*ch == '>') {
            stop = (ch - input_str);
            lily_msgbuf_add_text_range(vm_buffer, input_str, start, stop);
            lily_msgbuf_add(vm_buffer, "&gt;");
            start = stop + 1;
        }
        else if (*ch == '\0')
            break;

        ch++;
    }

    if (start != 0) {
        stop = (ch - input_str);
        lily_msgbuf_add_text_range(vm_buffer, input_str, start, stop);
    }

    return start;
}

void lily_string_html_encode(lily_vm_state *vm, uint16_t argc, uint16_t *code)
{
    lily_value **vm_regs = vm->vm_regs;
    lily_value *input_arg = vm_regs[code[1]];
    lily_value *result_arg = vm_regs[code[0]];

    /* If nothing was escaped, output what was input. */
    if (lily_maybe_html_encode_to_buffer(vm, input_arg) == 0)
        lily_assign_value(result_arg, input_arg);
    else {
        char *source = vm->vm_buffer->message;
        lily_move_string(result_arg, lily_new_raw_string(source));
    }
}

/* This is a helper for lstrip wherein input_arg has some utf-8 bits inside. */
static int lstrip_utf8_start(lily_value *input_arg, lily_string_val *strip_sv)
{
    char *input_str = input_arg->value.string->string;
    int input_length = input_arg->value.string->size;

    char *strip_str = strip_sv->string;
    int strip_length = strip_sv->size;
    int i = 0, j = 0, match = 1;

    char ch = strip_str[0];
    if (follower_table[(unsigned char)ch] == strip_length) {
        /* Only a single utf-8 char. This is a bit simpler. */
        char strip_start_ch = ch;
        int char_width = follower_table[(unsigned char)ch];
        while (i < input_length) {
            if (input_str[i] == strip_start_ch) {
                /* j starts at 1 because the first byte was already checked.
                   This compares the inner part of the strip string and the
                   input string to make sure the whole utf-8 chunk matches. */
                for (j = 1;j < char_width;j++) {
                    if (input_str[i + j] != strip_str[j]) {
                        match = 0;
                        break;
                    }
                }
                if (match == 0)
                    break;

                i += char_width;
            }
            else
                break;
        }
    }
    else {
        /* There's at least one utf-8 chunk. There may be ascii bytes to strip
           as well, or more utf-8 chunks. This is the most complicated case. */
        char input_ch;
        int char_width, k;
        while (1) {
            input_ch = input_str[i];
            if (input_ch == strip_str[j]) {
                char_width = follower_table[(unsigned char)strip_str[j]];
                match = 1;
                /* This has to use k, unlike the above loop, because j is being
                   used to hold the current position in strip_str. */
                for (k = 1;k < char_width;k++) {
                    if (input_str[i + k] != strip_str[j + k]) {
                        match = 0;
                        break;
                    }
                }
                if (match == 1) {
                    /* Found a match, so eat the valid utf-8 chunk and start
                       from the beginning of the strip string again. This makes
                       sure that each chunk of the input string is matched to
                       each chunk of the strip string. */
                    i += char_width;
                    if (i >= input_length)
                        break;
                    else {
                        j = 0;
                        continue;
                    }
                }
            }

            /* This assumes that strip_str is valid utf-8. */
            j += follower_table[(unsigned char)strip_str[j]];

            /* If all chunks in the strip str have been checked, then
               everything that can be removed has been removed. */
            if (j == strip_length) {
                match = 0;
                break;
            }
        }
    }

    return i;
}

/* This is a helper for lstrip wherein input_arg does not have utf-8. */
static int lstrip_ascii_start(lily_value *input_arg, lily_string_val *strip_sv)
{
    int i;
    char *input_str = input_arg->value.string->string;
    int input_length = input_arg->value.string->size;

    if (strip_sv->size == 1) {
        /* Strip a single byte really fast. The easiest case. */
        char strip_ch;
        strip_ch = strip_sv->string[0];
        for (i = 0;i < input_length;i++) {
            if (input_str[i] != strip_ch)
                break;
        }
    }
    else {
        /* Strip one of many ascii bytes. A bit tougher, but not much. */
        char *strip_str = strip_sv->string;
        int strip_length = strip_sv->size;
        for (i = 0;i < input_length;i++) {
            char ch = input_str[i];
            int found = 0;
            int j;
            for (j = 0;j < strip_length;j++) {
                if (ch == strip_str[j]) {
                    found = 1;
                    break;
                }
            }
            if (found == 0)
                break;
        }
    }

    return i;
}

void lily_string_lstrip(lily_vm_state *vm, uint16_t argc, uint16_t *code)
{
    lily_value **vm_regs = vm->vm_regs;
    lily_value *input_arg = vm_regs[code[1]];
    lily_value *strip_arg = vm_regs[code[2]];
    lily_value *result_arg = vm_regs[code[0]];

    char *strip_str;
    unsigned char ch;
    int copy_from, i, has_multibyte_char, strip_str_len;
    lily_string_val *strip_sv;

    /* Either there is nothing to strip (1st), or stripping nothing (2nd). */
    if (input_arg->value.string->size == 0 ||
        strip_arg->value.string->size == 0) {
        lily_assign_value(result_arg, input_arg);
        return;
    }

    strip_sv = strip_arg->value.string;
    strip_str = strip_sv->string;
    strip_str_len = strlen(strip_str);
    has_multibyte_char = 0;

    for (i = 0;i < strip_str_len;i++) {
        ch = (unsigned char)strip_str[i];
        if (ch > 127) {
            has_multibyte_char = 1;
            break;
        }
    }

    if (has_multibyte_char == 0)
        copy_from = lstrip_ascii_start(input_arg, strip_sv);
    else
        copy_from = lstrip_utf8_start(input_arg, strip_sv);

    int new_size = (input_arg->value.string->size - copy_from) + 1;
    lily_string_val *new_sv = make_sv(vm, new_size);

    strcpy(new_sv->string, input_arg->value.string->string + copy_from);

    lily_move_string(result_arg, new_sv);
}

void lily_string_lower(lily_vm_state *vm, uint16_t argc, uint16_t *code)
{
    lily_value **vm_regs = vm->vm_regs;
    lily_value *input_arg = vm_regs[code[1]];
    lily_value *result_arg = vm_regs[code[0]];

    int new_size = input_arg->value.string->size + 1;
    lily_string_val *new_sv = make_sv(vm, new_size);

    char *new_str = new_sv->string;
    char *input_str = input_arg->value.string->string;
    int input_length = input_arg->value.string->size;
    int i;

    for (i = 0;i < input_length;i++) {
        char ch = input_str[i];
        if (isupper(ch))
            new_str[i] = tolower(ch);
        else
            new_str[i] = ch;
    }
    new_str[input_length] = '\0';

    lily_move_string(result_arg, new_sv);
}

void lily_string_parse_i(lily_vm_state *vm, uint16_t argc, uint16_t *code)
{
    lily_value **vm_regs = vm->vm_regs;
    lily_value *result_reg = vm_regs[code[0]];
    char *input = vm_regs[code[1]]->value.string->string;
    uint64_t value = 0;
    int is_negative = 0;
    unsigned int rounds = 0;
    int leading_zeroes = 0;

    if (*input == '-') {
        is_negative = 1;
        ++input;
    }
    else if (*input == '+')
        ++input;

    if (*input == '0') {
        ++input;
        leading_zeroes = 1;
        while (*input == '0')
            ++input;
    }

    /* A signed int64 peaks at 9223372036854775807 (or ...808 for negative).
       The maximum number of reasonable digits is therefore 20 for scanning
       decimal. */
    while (*input >= '0' && *input <= '9' && rounds != 20) {
        value = (value * 10) + (*input - '0');
        ++input;
        rounds++;
    }

    /* These cases check for overflow, trailing junk, and just + or just -. */
    if (value > ((uint64_t)INT64_MAX + is_negative) ||
        *input != '\0' ||
        (rounds == 0 && leading_zeroes == 0)) {
        lily_move_shared_enum(result_reg, lily_get_none(vm));
    }
    else {
        int64_t signed_value;

        if (is_negative == 0)
            signed_value = (int64_t)value;
        else
            signed_value = -(int64_t)value;

        lily_value *v = lily_new_value(VAL_IS_INTEGER,
                (lily_raw_value){.integer = signed_value});

        lily_move_enum(result_reg, lily_new_some(v));
    }
}

/* This is a helper for rstrip when there's no utf-8 in input_arg. */
static int rstrip_ascii_stop(lily_value *input_arg, lily_string_val *strip_sv)
{
    int i;
    char *input_str = input_arg->value.string->string;
    int input_length = input_arg->value.string->size;

    if (strip_sv->size == 1) {
        char strip_ch = strip_sv->string[0];
        for (i = input_length - 1;i >= 0;i--) {
            if (input_str[i] != strip_ch)
                break;
        }
    }
    else {
        char *strip_str = strip_sv->string;
        int strip_length = strip_sv->size;
        for (i = input_length - 1;i >= 0;i--) {
            char ch = input_str[i];
            int found = 0;
            int j;
            for (j = 0;j < strip_length;j++) {
                if (ch == strip_str[j]) {
                    found = 1;
                    break;
                }
            }
            if (found == 0)
                break;
        }
    }

    return i + 1;
}

/* Helper for rstrip, for when there is some utf-8. */
static int rstrip_utf8_stop(lily_value *input_arg, lily_string_val *strip_sv)
{
    char *input_str = input_arg->value.string->string;
    int input_length = input_arg->value.string->size;

    char *strip_str = strip_sv->string;
    int strip_length = strip_sv->size;
    int i, j;

    i = input_length - 1;
    j = 0;
    while (i >= 0) {
        /* First find out how many bytes are in the current chunk. */
        int follow_count = follower_table[(unsigned char)strip_str[j]];
        /* Now get the last byte of this chunk. Since the follower table
           includes the total, offset by -1. */
        char last_strip_byte = strip_str[j + (follow_count - 1)];
        /* Input is going from right to left. See if input matches the last
           byte of the current utf-8 chunk. But also check that there are
           enough chars left to protect against underflow. */
        if (input_str[i] == last_strip_byte &&
            i + 1 >= follow_count) {
            int match = 1;
            int input_i, strip_i, k;
            /* input_i starts at i - 1 to skip the last byte.
               strip_i starts at follow_count so it can stop things. */
            for (input_i = i - 1, strip_i = j + (follow_count - 2), k = 1;
                 k < follow_count;
                 input_i--, strip_i--, k++) {
                if (input_str[input_i] != strip_str[strip_i]) {
                    match = 0;
                    break;
                }
            }

            if (match == 1) {
                i -= follow_count;
                j = 0;
                continue;
            }
        }

        /* Either the first byte or one of the inner bytes didn't match.
           Go to the next chunk and try again. */
        j += follow_count;
        if (j == strip_length)
            break;

        continue;
    }

    return i + 1;
}

void lily_string_rstrip(lily_vm_state *vm, uint16_t argc, uint16_t *code)
{
    lily_value **vm_regs = vm->vm_regs;
    lily_value *input_arg = vm_regs[code[1]];
    lily_value *strip_arg = vm_regs[code[2]];
    lily_value *result_arg = vm_regs[code[0]];

    char *strip_str;
    unsigned char ch;
    int copy_to, i, has_multibyte_char, strip_str_len;
    lily_string_val *strip_sv;

    /* Either there is nothing to strip (1st), or stripping nothing (2nd). */
    if (input_arg->value.string->size == 0 ||
        strip_arg->value.string->size == 0) {
        lily_assign_value(result_arg, input_arg);
        return;
    }

    strip_sv = strip_arg->value.string;
    strip_str = strip_sv->string;
    strip_str_len = strlen(strip_str);
    has_multibyte_char = 0;

    for (i = 0;i < strip_str_len;i++) {
        ch = (unsigned char)strip_str[i];
        if (ch > 127) {
            has_multibyte_char = 1;
            break;
        }
    }

    if (has_multibyte_char == 0)
        copy_to = rstrip_ascii_stop(input_arg, strip_sv);
    else
        copy_to = rstrip_utf8_stop(input_arg, strip_sv);

    int new_size = copy_to + 1;
    lily_string_val *new_sv = make_sv(vm, new_size);

    strncpy(new_sv->string, input_arg->value.string->string, copy_to);
    /* This will always copy a partial string, so make sure to add a terminator. */
    new_sv->string[copy_to] = '\0';

    lily_move_string(result_arg, new_sv);
}

void lily_string_starts_with(lily_vm_state *vm, uint16_t argc, uint16_t *code)
{
    lily_value **vm_regs = vm->vm_regs;
    lily_value *input_arg = vm_regs[code[1]];
    lily_value *prefix_arg = vm_regs[code[2]];
    lily_value *result_arg = vm_regs[code[0]];

    char *input_raw_str = input_arg->value.string->string;
    char *prefix_raw_str = prefix_arg->value.string->string;
    int prefix_size = prefix_arg->value.string->size;

    if (input_arg->value.string->size < prefix_size) {
        lily_move_integer(result_arg, 0);
        return;
    }

    int i, ok = 1;
    for (i = 0;i < prefix_size;i++) {
        if (input_raw_str[i] != prefix_raw_str[i]) {
            ok = 0;
            break;
        }
    }

    lily_move_integer(result_arg, ok);
}

void lily_string_strip(lily_vm_state *vm, uint16_t argc, uint16_t *code)
{
    lily_value **vm_regs = vm->vm_regs;
    lily_value *input_arg = vm_regs[code[1]];
    lily_value *strip_arg = vm_regs[code[2]];
    lily_value *result_arg = vm_regs[code[0]];

    /* Either there is nothing to strip (1st), or stripping nothing (2nd). */
    if (input_arg->value.string->size == 0 ||
        strip_arg->value.string->size == 0) {
        lily_assign_value(result_arg, input_arg);
        return;
    }

    char ch;
    lily_string_val *strip_sv = strip_arg->value.string;
    char *strip_str = strip_sv->string;
    int strip_str_len = strlen(strip_str);
    int has_multibyte_char = 0;
    int copy_from, copy_to, i;

    for (i = 0;i < strip_str_len;i++) {
        ch = (unsigned char)strip_str[i];
        if (ch > 127) {
            has_multibyte_char = 1;
            break;
        }
    }

    if (has_multibyte_char == 0)
        copy_from = lstrip_ascii_start(input_arg, strip_sv);
    else
        copy_from = lstrip_utf8_start(input_arg, strip_sv);

    if (copy_from != input_arg->value.string->size) {
        if (has_multibyte_char)
            copy_to = rstrip_ascii_stop(input_arg, strip_sv);
        else
            copy_to = rstrip_utf8_stop(input_arg, strip_sv);
    }
    else
        /* The whole string consists of stuff in strip_str. Do this so the
           result is an empty string. */
        copy_to = copy_from;

    int new_size = (copy_to - copy_from) + 1;
    lily_string_val *new_sv = make_sv(vm, new_size);

    char *new_str = new_sv->string;
    strncpy(new_str, input_arg->value.string->string + copy_from, new_size - 1);
    new_str[new_size - 1] = '\0';

    lily_move_string(result_arg, new_sv);
}

static const char move_table[256] =
{
     /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 1 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 2 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 3 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 4 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 5 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 6 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 7 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 8 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 9 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* A */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* B */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* C */ 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/* D */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/* E */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
/* F */ 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void string_split_by_val(lily_vm_state *vm, char *input, char *splitby,
        lily_list_val *dest)
{
    char *input_ch = &input[0];
    char *splitby_ch = &splitby[0];
    int values_needed = 0;
    lily_value **elems;

    while (move_table[(unsigned char)*input_ch] != 0) {
        if (*input_ch == *splitby_ch) {
            char *restore_ch = input_ch;
            int is_match = 1;
            while (*input_ch == *splitby_ch) {
                splitby_ch++;
                input_ch++;
                if (*splitby_ch == '\0')
                    break;

                if (*input_ch != *splitby_ch) {
                    is_match = 0;
                    input_ch = restore_ch;
                    break;
                }
            }

            splitby_ch = &splitby[0];
            values_needed += is_match;
        }
        else
            input_ch += move_table[(unsigned char)*input_ch];
    }

    values_needed++;
    input_ch = &input[0];
    elems = lily_malloc(sizeof(lily_value *) * values_needed);
    int i = 0;
    char *last_start = input_ch;

    while (1) {
        char *match_start = input_ch;
        int is_match = 0;
        if (*input_ch == *splitby_ch) {
            is_match = 1;
            while (*input_ch == *splitby_ch) {
                splitby_ch++;
                if (*splitby_ch == '\0')
                    break;

                input_ch++;
                if (*input_ch != *splitby_ch) {
                    is_match = 0;
                    input_ch = match_start;
                    break;
                }
            }
            splitby_ch = &splitby[0];
        }

        /* The second check is so that if the last bit of the input string
           matches the split string, an empty string will be made.
           Ex: "1 2 3 ".split(" ") # ["1", "2", "3", ""] */
        if (is_match || *input_ch == '\0') {
            int sv_size = match_start - last_start;

            elems[i] = lily_new_string_ncpy(last_start, sv_size);
            i++;
            if (*input_ch == '\0')
                break;

            last_start = input_ch + 1;
        }
        else if (*input_ch == '\0')
            break;

        input_ch++;
    }

    dest->elems = elems;
    dest->num_values = values_needed;
}

void lily_string_split(lily_vm_state *vm, uint16_t argc, uint16_t *code)
{
    lily_value **vm_regs = vm->vm_regs;
    lily_string_val *input_strval = vm_regs[code[1]]->value.string;
    lily_string_val *split_strval;
    if (argc == 2)
        split_strval = vm_regs[code[2]]->value.string;
    else {
        lily_string_val fake_sv;
        fake_sv.string = " ";
        fake_sv.size = 1;
        split_strval = &fake_sv;
    }

    lily_value *result_reg = vm_regs[code[0]];

    if (split_strval->size == 0)
        lily_raise(vm->raiser, lily_ValueError, "Cannot split by empty string.\n");

    lily_list_val *lv = lily_new_list_val();

    string_split_by_val(vm, input_strval->string, split_strval->string, lv);

    lily_move_list(result_reg, lv);
}

void lily_string_trim(lily_vm_state *vm, uint16_t argc, uint16_t *code)
{
    lily_value **vm_regs = vm->vm_regs;
    lily_value *input_arg = vm_regs[code[1]];
    lily_value *result_arg = vm_regs[code[0]];

    char fake_buffer[5] = " \t\r\n";
    lily_string_val fake_sv;
    fake_sv.string = fake_buffer;
    fake_sv.size = strlen(fake_buffer);

    int copy_from = lstrip_ascii_start(input_arg, &fake_sv);
    lily_string_val *new_sv;

    if (copy_from != input_arg->value.string->size) {
        int copy_to = rstrip_ascii_stop(input_arg, &fake_sv);
        int new_size = (copy_to - copy_from) + 1;
        new_sv = make_sv(vm, new_size);
        char *new_str = new_sv->string;

        strncpy(new_str, input_arg->value.string->string + copy_from, new_size - 1);
        new_str[new_size - 1] = '\0';
    }
    else {
        /* It's all space, so make a new empty string. */
        new_sv = make_sv(vm, 1);
        new_sv->string[0] = '\0';
    }

    lily_move_string(result_arg, new_sv);
}

void lily_string_upper(lily_vm_state *vm, uint16_t argc, uint16_t *code)
{
    lily_value **vm_regs = vm->vm_regs;
    lily_value *input_arg = vm_regs[code[1]];
    lily_value *result_arg = vm_regs[code[0]];

    int new_size = input_arg->value.string->size + 1;
    lily_string_val *new_sv = make_sv(vm, new_size);

    char *new_str = new_sv->string;
    char *input_str = input_arg->value.string->string;
    int input_length = input_arg->value.string->size;
    int i;

    for (i = 0;i < input_length;i++) {
        char ch = input_str[i];
        if (islower(ch))
            new_str[i] = toupper(ch);
        else
            new_str[i] = ch;
    }
    new_str[input_length] = '\0';

    lily_move_string(result_arg, new_sv);
}


/* This handles a string subscript. The subscript may be negative (in which case
   it is an offset against the end). This must check if the index given by
   'index_reg' is a valid one.
   This moves by utf-8 codepoints, not by bytes. The result is sent to
   'result_reg', unless IndexError is raised. */
void lily_string_subscript(lily_vm_state *vm, lily_value *input_reg,
        lily_value *index_reg, lily_value *result_reg)
{
    char *input = input_reg->value.string->string;
    int index = index_reg->value.integer;
    char *ch;

    if (index >= 0) {
        ch = &input[0];
        while (index && move_table[(unsigned char)*ch] != 0) {
            ch += move_table[(unsigned char)*ch];
            index--;
        }
        if (move_table[(unsigned char)*ch] == 0)
            lily_raise(vm->raiser, lily_IndexError, "Index %d is out of range.\n",
                    index_reg->value.integer);
    }
    else {
        char *stop = &input[0];
        ch = &input[input_reg->value.string->size];
        while (stop != ch && index != 0) {
            ch--;
            if (move_table[(unsigned char)*ch] != 0)
                index++;
        }
        if (index != 0)
            lily_raise(vm->raiser, lily_IndexError, "Index %d is out of range.\n",
                    index_reg->value.integer);
    }

    int to_copy = move_table[(unsigned char)*ch];
    lily_string_val *result = make_sv(vm, to_copy + 1);
    char *dest = &result->string[0];
    dest[to_copy] = '\0';

    strncpy(dest, ch, to_copy);

    lily_move_string(result_reg, result);
}

static const lily_func_seed ends_with =
    {NULL, "ends_with", dyna_function, "(String, String):Boolean", lily_string_ends_with};

static const lily_func_seed find =
    {&ends_with, "find", dyna_function, "(String, String):Option[Integer]", lily_string_find};

static const lily_func_seed html_encode =
    {&find, "html_encode", dyna_function, "(String):String", lily_string_html_encode};

static const lily_func_seed is_alpha_fn =
    {&html_encode, "is_alpha", dyna_function, "(String):Boolean", lily_string_is_alpha};

static const lily_func_seed is_digit_fn =
    {&is_alpha_fn, "is_digit", dyna_function, "(String):Boolean", lily_string_is_digit};

static const lily_func_seed is_alnum_fn =
    {&is_digit_fn, "is_alnum", dyna_function, "(String):Boolean", lily_string_is_alnum};

static const lily_func_seed is_space_fn =
    {&is_alnum_fn, "is_space", dyna_function, "(String):Boolean", lily_string_is_space};

static const lily_func_seed lstrip =
    {&is_space_fn, "lstrip", dyna_function, "(String, String):String", lily_string_lstrip};

static const lily_func_seed lower =
    {&lstrip, "lower", dyna_function, "(String):String", lily_string_lower};

static const lily_func_seed parse_i =
    {&lower, "parse_i", dyna_function, "(String):Option[Integer]", lily_string_parse_i};

static const lily_func_seed rstrip =
    {&parse_i, "rstrip", dyna_function, "(String, String):String", lily_string_rstrip};

static const lily_func_seed starts_with =
    {&rstrip, "starts_with", dyna_function, "(String, String):Boolean", lily_string_starts_with};

static const lily_func_seed split =
    {&starts_with, "split", dyna_function, "(String, *String):List[String]", lily_string_split};

static const lily_func_seed strip =
    {&split, "strip", dyna_function, "(String, String):String", lily_string_strip};

static const lily_func_seed trim =
    {&strip, "trim", dyna_function, "(String):String", lily_string_trim};

static const lily_func_seed dynaload_start =
    {&trim, "upper", dyna_function, "(String):String", lily_string_upper};

static const lily_class_seed string_seed =
{
    NULL,               /* next */
    "String",           /* name */
    dyna_class,         /* load_type */
    1,                  /* is_refcounted */
    0,                  /* generic_count */
    &dynaload_start     /* dynaload_table */
};

lily_class *lily_string_init(lily_symtab *symtab)
{
    return lily_new_class_by_seed(symtab, &string_seed);
}
