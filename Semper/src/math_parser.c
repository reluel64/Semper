/*Implementation of a math parser
 * using Edsger Dijkstra's shunting yard algorithm
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#undef __STRICT_ANSI__
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include <linked_list.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <diag.h>
#include <math_parser.h>
#define MATH_VECTOR_SIZE 0x0010
#define OP_AS_LE         0x0001
#define OP_AS_UN         0x0002
#define OP_AS_RI         0x0003
#define OP_PRIO(x)      (0xff-(x))
#define string_length(x) ((x)?strlen((x)):0)
#define max(a,b) ((a)>(b))?(a):(b)
#define min(a,b) ((a)>(b))?(b):(a)
#define ALLOW_BASE_2
typedef double (*math_rtn)(double *vec, size_t vec_sz);

typedef struct
{
    char *on;
    unsigned char op_prec;
    unsigned char assoc;
    unsigned char op_cnt;
    math_rtn mr;
} operator_info;

typedef struct
{
    void *inf;
    list_entry current;
} stack_element;

typedef struct
{
    operator_info *oi;
    double value;
    list_entry current;
} output_queue;

//Operators
static double math_add(double *vec)
{
    return(vec[0] + vec[1]);
}
static double math_sub(double *vec, size_t vec_sz)
{
    return((vec_sz == 1) ? -vec[0] : (vec[0] - vec[1]));
}
static double math_pow(double *vec)
{
    return(pow(vec[0], vec[1]));
}
static double math_mul(double *vec)
{
    return(vec[0] * vec[1]);
}
static double math_div(double *vec)
{
    return(vec[1] != 0.0 ? vec[0] / vec[1] : NAN);
}
static double math_mod(double *vec)
{
    return(vec[1] != 0.0 ? fmod(vec[0], vec[1]) : NAN);
}
static double math_and(double *vec)
{
    return((double)((size_t)vec[0] && (size_t)vec[1]));
}
static double math_or(double *vec)
{
    return((double)((size_t)vec[0] || (size_t)vec[1]));
}
static double math_eq(double *vec)
{
    return((double)(vec[0] == vec[1]));
}
static double math_neq(double *vec)
{
    return((double)(vec[0] != vec[1]));
}
static double math_gte(double *vec)
{
    return((double)(vec[0] >= vec[1]));
}
static double math_lte(double *vec)
{
    return((double)(vec[0] <= vec[1]));
}
static double math_le(double *vec)
{
    return((double)(vec[0] < vec[1]));
}
static double math_gt(double *vec)
{
    return((double)(vec[0] > vec[1]));
}
//Bitwise
static double math_bitwand(double *vec)
{
    return((double)((size_t)vec[0] & (size_t)vec[1]));
}
static double math_bitwor(double *vec)
{
    return((double)((size_t)vec[0] | (size_t)vec[1]));
}
static double math_bitshr(double *vec)
{
    return((double)((size_t)vec[0] >> (size_t)vec[1]));
}
static double math_bitshl(double *vec)
{
    return((double)((size_t)vec[0] << (size_t)vec[1]));
}
static double math_bitnot(double *vec)
{
    return((double)~((long long)vec[0]));
}
static double math_bitxor(double *vec)
{
    return((double)((size_t)vec[0] ^ (size_t)vec[1]));
}
//Constants
static double math_true(void)
{
    return(1.0);
}
static double math_false(void)
{
    return(0.0);
}
static double math_e(void)
{
    return(M_E);
}
static double math_pi(void)
{
    return(M_PI);
}
static double math_gold(void)
{
    return(1.61803398874989484820);                      //golden ratio
}
//Functions
static double math_sqrt(double *vec)
{
    return(sqrt(vec[0]));
}
static double math_deg2rad(double *vec)
{
    return(vec[0] * (M_PI / 180.0));
}
static double math_rad2deg(double *vec)
{
    return(vec[0] * (180.0 / M_PI));
}
static double math_max(double *vec)
{
    return(vec[0] > vec[1] ? vec[0] : vec[1]);
}
static double math_min(double *vec)
{
    return(vec[0] > vec[1] ? vec[1] : vec[0]);
}
static double math_acos(double *vec)
{
    return(acos(vec[0]));
}
static double math_asin(double *vec)
{
    return(asin(vec[0]));
}
static double math_atan2(double *vec)
{
    return(atan2(vec[0], vec[1]));
}
static double math_atan(double *vec)
{
    return(atan(vec[0]));
}
static double math_sinh(double *vec)
{
    return(sinh(vec[0]));
}
static double math_sin(double *vec)
{
    return(sin(vec[0]));
}
static double math_cosh(double *vec)
{
    return(cosh(vec[0]));
}
static double math_cos(double *vec)
{
    return(cos(vec[0]));
}
static double math_log10(double *vec)
{
    return(log10(vec[0]));
}
static double math_log(double *vec)
{
    return(log(vec[0]));
}
static double math_tanh(double *vec)
{
    return(tanh(vec[0]));
}
static double math_exp(double *vec)
{
    return(exp(vec[0]));
}
static double math_ldexp(double *vec)
{
    return(ldexp(vec[0], vec[1]));
}
static double math_abs(double *vec)
{
    return(abs(vec[0]));
}
static double math_floor(double *vec)
{
    return(floor(vec[0]));
}
static double math_ceil(double *vec)
{
    return(ceil(vec[0]));
}
static double math_condr(double *vec)
{
    return(vec[0] != 0.0 ? vec[1] : vec[2]);
}
static double math_cond(double *vec)
{
    return(vec[0]);
}
static double math_sign(double *vec)
{
    return(vec[0] < 0.0 ? -1.0 : (vec[0] == 0.0 ? 0.0 : 1.0));
}
static double math_clamp(double *vec)
{
    if(vec[1] < vec[0])  return(vec[0]);

    if(vec[2] < vec[1])  return(vec[2]);

    return(vec[1]);
}

/*Stack Util*/
static inline void math_push_queue(list_entry *head, operator_info *oi, double val)
{
    output_queue *oq = zmalloc(sizeof(output_queue));
    oq->value = val;
    oq->oi = oi;
    list_entry_init(&oq->current);
    linked_list_add_last(&oq->current, head);
}

static inline void math_push(list_entry *head, void *inf)
{
    stack_element *se = zmalloc(sizeof(stack_element));
    se->inf = inf;
    list_entry_init(&se->current);
    linked_list_add_last(&se->current, head);
}

static inline void *math_peek(list_entry *head)
{
    if(linked_list_empty(head))
    {
        return(NULL);
    }

    stack_element *se = element_of(head->prev, se, current);
    return(se->inf);
}

static inline void *math_pop(list_entry *head)
{
    if(linked_list_empty(head))
    {
        return(NULL);
    }

    stack_element *se = element_of(head->prev, se, current);
    void *ret = se->inf;
    linked_list_remove(&se->current);
    sfree((void**)&se);
    return(ret);
}

/*This routine that brings headaches makes
 * sure that we do have everything nice in the output queue
 * The routine uses the Shunting yard algorithm to transform the expression.
 * ------------------------------------------------------------------------------
 * Shunting yard algorithm (pasted here from https://en.wikipedia.org/wiki/Shunting-yard_algorithm):
 * While there are tokens to be read:
 *   Read a token.
 *   If the token is a number, then push it to the output queue.
 *   If the token is a function token, then push it onto the stack.
 *   If the token is a function argument separator (e.g., a comma):
 *       Until the token at the top of the stack is a left parenthesis, pop operators off the stack onto the output queue.
 *       If no left parentheses are encountered, either the separator was misplaced or parentheses were mismatched.
 *
 *   If the token is an operator, o1, then:
 *       while there is an operator token o2, at the top of the operator stack and either
 *           o1 is left-associative and its precedence is less than or equal to that of o2, or
 *           o1 is right associative, and has precedence less than that of o2,
 *           pop o2 off the operator stack, onto the output queue;
 *       at the end of iteration push o1 onto the operator stack.
 *
 *   If the token is a left parenthesis (i.e. "("), then push it onto the stack.
 *   If the token is a right parenthesis (i.e. ")"):
 *       Until the token at the top of the stack is a left parenthesis, pop operators off the stack onto the output queue.
 *       Pop the left parenthesis from the stack, but not onto the output queue.
 *       If the token at the top of the stack is a function token, pop it onto the output queue.
 *       If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
 *
 * When there are no more tokens to read:
 *   While there are still operator tokens in the stack:
 *       If the operator token on the top of the stack is a parenthesis, then there are mismatched parentheses.
 *       Pop the operator onto the output queue.
 * Exit.
 */

static int math_parser_gen_queue(unsigned char *f, list_entry *out_queue, math_parser_callback mpc, void *pv)
{
    static operator_info opt[] =
    {

        { "(",      OP_PRIO(0xff), OP_AS_UN, 0, (math_rtn)              NULL},
        { ")",      OP_PRIO(0xff), OP_AS_UN, 0, (math_rtn)              NULL},
        { ",",      OP_PRIO(0xff), OP_AS_UN, 0, (math_rtn)              NULL},
        //Basic
        { "+",      OP_PRIO(0x04), OP_AS_LE, 2, (math_rtn)          math_add},
        { "-",      OP_PRIO(0x04), OP_AS_LE, 2, (math_rtn)          math_sub},
        /*
         * the power operator has to be declared before
         * multiplication otherwise the expression is not evaluated properly.
         */
        { "**",     OP_PRIO(0x02), OP_AS_RI, 2, (math_rtn)          math_pow},
        { "*",      OP_PRIO(0x03), OP_AS_LE, 2, (math_rtn)          math_mul},
        { "/",      OP_PRIO(0x03), OP_AS_LE, 2, (math_rtn)          math_div},
        {"%",       OP_PRIO(0x03), OP_AS_LE, 2, (math_rtn)          math_mod},
        //Bitwise
        {"&",       OP_PRIO(0x08), OP_AS_LE, 2, (math_rtn)      math_bitwand},
        {"|",       OP_PRIO(0x0a), OP_AS_LE, 2, (math_rtn)       math_bitwor},
        {"<<",      OP_PRIO(0x05), OP_AS_LE, 2, (math_rtn)       math_bitshl},
        {">>",      OP_PRIO(0x05), OP_AS_LE, 2, (math_rtn)       math_bitshr},
        {"^",       OP_PRIO(0x09), OP_AS_LE, 2, (math_rtn)       math_bitxor},
        {"~",       OP_PRIO(0x02), OP_AS_RI, 1, (math_rtn)       math_bitnot},
        //Logical
        {"&&",      OP_PRIO(0x0b), OP_AS_LE, 2, (math_rtn)          math_and},
        {"||",      OP_PRIO(0x0c), OP_AS_LE, 2, (math_rtn)           math_or},
        //Relational
        {"==",      OP_PRIO(0x07), OP_AS_LE, 2, (math_rtn)           math_eq},
        {"<=",      OP_PRIO(0x06), OP_AS_LE, 2, (math_rtn)          math_lte},
        {"<",       OP_PRIO(0x06), OP_AS_LE, 2, (math_rtn)           math_le},
        {">=",      OP_PRIO(0x06), OP_AS_LE, 2, (math_rtn)          math_gte},
        {">",       OP_PRIO(0x06), OP_AS_LE, 2, (math_rtn)           math_gt},
        {"!=",      OP_PRIO(0x07), OP_AS_LE, 2, (math_rtn)          math_neq},
        /*
         * even if the operator is actually "?:", we split it in
         *half and the actual work is done by the ":" operator function
         */
        {"?",       OP_PRIO(0x0e), OP_AS_RI, 1, (math_rtn)         math_cond},
        {":",       OP_PRIO(0x0e), OP_AS_RI, 3, (math_rtn)        math_condr},
        //Constants
        {"pi",      OP_PRIO(0x00), OP_AS_UN, 0, (math_rtn)           math_pi},
        {"e",       OP_PRIO(0x00), OP_AS_UN, 0, (math_rtn)            math_e},
        {"true",    OP_PRIO(0x00), OP_AS_UN, 0, (math_rtn)         math_true},
        {"false",   OP_PRIO(0x00), OP_AS_UN, 0, (math_rtn)        math_false},
        {"gr",      OP_PRIO(0x00), OP_AS_UN, 0, (math_rtn)         math_gold},
        //Function
        {"acos",    OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)         math_acos},
        {"asin",    OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)         math_asin},
        {"atan2",   OP_PRIO(0x00), OP_AS_UN, 2, (math_rtn)        math_atan2},
        {"atan",    OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)         math_atan},
        {"sinh",    OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)         math_sinh},
        {"sin",     OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)          math_sin},
        {"cosh",    OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)         math_cosh},
        {"cos",     OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)          math_cos},
        {"max",     OP_PRIO(0x00), OP_AS_UN, 2, (math_rtn)          math_max},
        {"min",     OP_PRIO(0x00), OP_AS_UN, 2, (math_rtn)          math_min},
        {"log10",   OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)        math_log10},
        {"log",     OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)          math_log},
        {"tanh",    OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)         math_tanh},
        {"exp",     OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)          math_exp},
        {"ldexp",   OP_PRIO(0x00), OP_AS_UN, 2, (math_rtn)        math_ldexp},
        {"sqrt",    OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)         math_sqrt},
        {"abs",     OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)          math_abs},
        {"floor",   OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)        math_floor},
        {"ceil",    OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)         math_ceil},
        {"deg2rad", OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)      math_deg2rad},
        {"rad2deg", OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)      math_rad2deg},
        {"clamp",   OP_PRIO(0x00), OP_AS_UN, 3, (math_rtn)        math_clamp},
        {"sign",    OP_PRIO(0x00), OP_AS_UN, 1, (math_rtn)         math_sign}
    };


    unsigned char err = 0;
    unsigned char was_number = 0; //was the previous token a number? - we need this if we have something like 3-1 where 1 is not negative
    unsigned char clp = 0;      //closed parenthesis (this is to avoid cases like "(x)-3" to be interpreted like 3 being negative because it's not)
    list_entry op_stack = {0};
    list_entry_init(&op_stack);


    for(size_t i = 0; !err && f[i]; i++)
    {
        if(isspace(f[i])) //skip white spaces
        {
            continue;
        }

        //Push the number into the queue
        unsigned char *n = NULL;
        double v = 0.0;

#if defined(ALLOW_BASE_2)
        v = (double)strtoll(f + i, (char**)&n, 2);

        if(toupper(n[0]) != 'B')
        {
            n = NULL;
            v = 0.0;
        }
        else
        {
            n++; //skip 'B'
        }

        if(f[i] == '0' && n == NULL)
        {
            v = (double)strtoll(f + i, (char**)&n, 8);
            unsigned char *n2 = NULL;
            strtod(f + i, (char**)&n2); //we must make sure that the number base was properly detected

            if(isdigit(n[0]) || n[0] == '\0' || n2 > n)
            {
                n = NULL;
                v = 0.0;
            }
        }

#endif

        if(n == NULL)
        {
            v = strtod(f + i, (char**)&n);
        }

        if(clp == 0 && was_number == 0 && n != f + i)
        {
            i = (n - f) - 1;
            was_number = 1;
            math_push_queue(out_queue, NULL, v);
        }
        else //Take care of the non-numeric tokens
        {
            if(clp == 1)
            {
                clp = 0;
            }

            operator_info *foi = NULL; //found operator
            size_t token_len = 0;

            //Lookup through the table to find a supported token

            for(size_t x = 0; x < sizeof(opt) / sizeof(operator_info); x++)
            {
                token_len = string_length(opt[x].on);

                if(!strncasecmp(opt[x].on, f + i, token_len))
                {
                    foi = opt + x; //save the operator info
                    break;    //found something that is supported
                }
                else
                {
                    foi = NULL;
                    token_len = 0; //prevent shit from happening
                }
            }

            if(token_len == 0 || foi == NULL)
            {

                /*
                 * before we are going to declare it a total failure,
                 * we'll try to check the callback and see if we're successful
                 * */
                if(mpc)
                {
                    size_t len = 0;
                    int name = 1;
                    double v = 0.0;
                    /*
                     * Calculate the length and stop when running out of operators
                     *
                     */
                    for(len = 0; f[i + len] && name == 1; len++)
                    {
                        for(size_t x = 0; x < sizeof(opt) / sizeof(operator_info); x++)
                        {
                            token_len = string_length(opt[x].on);

                            /*If we reach an operator we
                             * stop the search and call
                             * the callback*/
                            if(strncasecmp(f + i + len, opt[x].on, token_len) == 0)
                            {
                                name = 0;
                                break;
                            }

                            /*The last operator is ":" so if we reach it, we will
                             * stop and go to the next position in buffer
                             * */
                            if(strncasecmp(":", opt[x].on, token_len) == 0)
                                break;
                        }

                        /*exit before incrementing len*/

                        if(name == 0)
                            break;
                    }

                    if(mpc(f + i, &len, &v, pv) == 0)
                    {
                        math_push_queue(out_queue, NULL, v);
                        i += len - 1;
                        continue;
                    }
                }

                err = 1;
                break; //surprise motherfucker  (this happens if the token or function is not supported)
            }
            else
            {
                i += token_len - 1; //we're good to go (we are not using the full length as "i" is anyway incremented so we subtract 1 from the actual buffer size
            }

            if(foi->on[0] == ',')
            {
                while(1)
                {
                    operator_info *loi = math_peek(&op_stack);

                    if(!loi || loi->on[0] == '(')
                    {
                        break;
                    }

                    loi = math_pop(&op_stack);
                    math_push_queue(out_queue, loi, 0.0);
                }
            }
            else if(foi->on[0] == '(')
            {
                math_push(&op_stack, foi);
            }
            else if(foi->on[0] == ')')
            {
                clp = 1;
                unsigned char closed = 0;

                while(1)
                {
                    operator_info *loi = math_peek(&op_stack);

                    if(loi == NULL)
                    {
                        if(closed == 0)
                        {
                            err = 1;
                        }

                        break;
                    }
                    else if(loi->on[0] == '(')
                    {
                        closed = 1;
                        math_pop(&op_stack); //remove it from the stack
                        continue;
                    }
                    else if(closed == 1 && loi->op_prec != 0xff)
                    {
                        break;
                    }

                    loi = math_pop(&op_stack);
                    math_push_queue(out_queue, loi, 0.0);

                    if(closed == 1 && loi->op_prec == 0xff)
                    {
                        break;
                    }
                }
            }
            else if(foi->op_prec != 0xff && foi->op_prec != 0x00) //this is most likely an operator
            {
                while(1)
                {
                    operator_info *loi = math_peek(&op_stack);

                    if(loi == NULL)
                    {
                        break; //this is not an operator
                    }

                    if((foi->assoc == OP_AS_LE && foi->op_prec <= loi->op_prec) || (foi->assoc == OP_AS_RI && foi->op_prec < loi->op_prec))
                    {
                        loi = math_pop(&op_stack);
                        math_push_queue(out_queue, loi, 0.0);
                    }
                    else
                    {
                        break;
                    }
                }

                math_push(&op_stack, foi);
            }
            else if(foi->op_prec == 0xff)
            {
                math_push(&op_stack, foi);
            }

            else
            {
                math_push(&op_stack, foi);
            }

            was_number = 0;
        }
    }

    if(err) //errors were detected so we are aborting but before doing that we just clean up the mess
    {
        output_queue *oq = NULL;
        output_queue *toq = NULL;
        diag_error("%s expression \"%s\" is malformed", __FUNCTION__, f);

        while(math_pop(&op_stack)); //there's no issue here to pop things like that as the data is statically allocated and we do not risk any memory leak

        list_enum_part_safe(oq, toq, out_queue, current)
        {
            linked_list_remove(&oq->current);
            sfree((void**)&oq);
        }
        return(-1);
    }
    else
    {
        operator_info *tbl = NULL;

        while((tbl = math_pop(&op_stack)) != NULL)
        {
            math_push_queue(out_queue, tbl, 0.0);
        }

#if defined(DEBUG)
        output_queue *oq = NULL;

        diag_verb("%s --------------Out stack--------------", __FUNCTION__);
        list_enum_part(oq, out_queue, current)
        {
            if(oq->oi)
            {
                diag_verb("%s: %s", __FUNCTION__, oq->oi->on);
            }
            else
            {
                diag_verb("%s: %lf", __FUNCTION__, oq->value);
            }
        }
#endif
    }

    return(0);
}


int math_parser(unsigned char *formula, double *res, math_parser_callback mpc, void *pv)
{
    double result = 0.0;
    unsigned char error = 0;
    list_entry out_queue = {0};
    list_entry_init(&out_queue);

    if(math_parser_gen_queue(formula, &out_queue, mpc, pv) != 0) /*generate the queue*/
    {
        return(-1);
    }

    while(1)
    {
        double vec[MATH_VECTOR_SIZE] = {0};
        size_t vp = 0;
        size_t operands = 0;
        output_queue *oq = NULL;    //operand holder
        output_queue *op_oq = NULL; //operand holder
        output_queue *top_oq = NULL; //operand holder (safety)
        unsigned char have_operator = 0;

        //find the first operator
        list_enum_part(oq, &out_queue, current)
        {
            if(oq->oi) //found the operator so let's just get out of here
            {
                have_operator = 1;
                break;
            }
            else
            {
                operands++;
            }
        }

        if(have_operator == 0 || error) //we are coming to the end aren't we?
        {
            size_t rcnt = 0;
            /* we expect to be one element left in the queue which is the final result
             * but if we do encounter more than one operand left we consider the expression malformed and we set the result to 0
             * however, we do not exit the loop as we do want to clean up the mess
             */
            list_enum_part_safe(oq, top_oq, &out_queue, current)
            {
                if(rcnt++ == 0 && error == 0)
                {
                    result = oq->value;
                }
                else
                {
                    result = 0.0;
                }

                linked_list_remove(&oq->current);
                sfree((void**)&oq);
            }

            if(rcnt > 1)
            {
                diag_error("%s expression \"%s\" is malformed", __FUNCTION__, formula);
            }

            break;
        }

        operands = (operands >= oq->oi->op_cnt) ?
                   operands - oq->oi->op_cnt : 0; //do some magic and decrease the operand count (see for yourself)

        list_enum_part_safe(op_oq, top_oq, &out_queue, current)
        {
            if(operands)
            {
                operands--;
                continue;
            }
            else if(op_oq->oi == NULL)
            {
                if(vp < MATH_VECTOR_SIZE)
                {
                    vec[vp++] = op_oq->value;
                }
                else
                {
                    diag_verb("%s vector size is not big enough", __FUNCTION__);
                    error = 1;
                    break;
                }

                linked_list_remove(&op_oq->current);
                sfree((void**)&op_oq);
            }
            else
            {
                if(oq->oi&&oq->oi->mr)
                {
                    oq->value = (*oq->oi->mr)(vec, vp);
                }
                else
                {
                    error = 1; 
                }
                if(oq->value == NAN)
                {
                    error = 1;
                }

                oq->oi = NULL;
                break;
            }
        }
    }

    *res = result;
    return(error ? -1 : 0);
}
