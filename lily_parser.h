#ifndef LILY_PARSER_H
# define LILY_PARSER_H

# include "lily_error.h"
# include "lily_ast.h"
# include "lily_lexer.h"
# include "lily_emitter.h"
# include "lily_symtab.h"

typedef struct {
    int depth;
    int *num_expected;
    lily_ast_pool *ast_pool;
    lily_ast **saved_trees;
    lily_ast *current_tree;
    int num_args;
    lily_lex_state *lex;
    lily_emit_state *emit;
    lily_symtab *symtab;
    lily_excep_data *error;
} lily_parse_state;

lily_parse_state *lily_new_parse_state(lily_excep_data *);
void lily_parser(lily_parse_state *);

#endif
