#include <string.h>

#include "lily_alloc.h"
#include "lily_parser.h"
#include "lily_seed.h"
#include "lily_cls_list.h"
#include "lily_value.h"

void lily_sys_var_loader(lily_parse_state *parser, lily_var *var)
{
    lily_options *options = parser->options;
    lily_symtab *symtab = parser->symtab;

    lily_list_val *lv = lily_new_list_val();
    lily_value **values = lily_malloc(options->argc * sizeof(lily_value *));

    lv->elems = values;
    lv->num_values = options->argc;

    int i;
    for (i = 0;i < options->argc;i++)
        values[i] = lily_new_string(options->argv[i]);

    lily_value v;
    v.flags = VAL_IS_LIST;
    v.value.list = lv;

    lily_tie_value(symtab, var, &v);
}

static const lily_var_seed argv_seed = {NULL, "argv", dyna_var, "List[String]"};

void lily_pkg_sys_init(lily_parse_state *parser, lily_options *options)
{
    lily_register_import(parser, "sys", &argv_seed, lily_sys_var_loader);
}
