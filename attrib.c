
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#include "header.h"

#define bool int
#define false 0
#define true 1 

#define RANDMAX 0x7fff

// operators
// precedence   operators       associativity
// 4            ^  @            left to right 
// 3            * / %           left to right
// 2            + -             left to right
// 1            =               right to left
//
// @ 运算 随机数运算
// 格式为 A@B 表示的为B概率的事件，如果发生则返回A 如果没有发生则返回0
static int 
op_preced(const char c)
{
	switch(c)    {
        case '^': case '@':
            return 4;
		case '*':  case '/': case '%':
			return 3;
		case '+': case '-':
			return 2;
		case '=':
			return 1;
	}
	return 0;
}

static bool 
op_left_assoc(const char c)
{
	switch(c)    {
		// left to right
        case '*': case '/': case '%': case '+': case '-': case '^': case '@': 
			return true;
			// right to left
		case '=':
			return false;
	}
	return false;
}


#define is_operator(c)  (c == '+' || c == '-' || c == '/' || c == '*' || c == '%' || c == '=' || c == '^' || c == '@')
#define is_ident(c)     (c != '(' && c != ')' && !is_operator(c))

static bool 
shunting_yard(const char *input, char *output, char  **err)
{
 	const char *strpos = input, *strend = input + strlen(input);
	char c, *outpos = output;

	char stack[32];       // operator stack
	unsigned int sl = 0;  // stack length
	char     sc;          // used for record stack element

	while(strpos < strend)   {
		// read one token from the input stream
		c = *strpos;
		if(!isspace(c))    {
			// If the token is a number (identifier), then add it to the output queue.
			if(is_ident(c))  {
				while(is_ident(*strpos) && strpos < strend){
					*outpos = *strpos; ++outpos;
					strpos++;
				}
				*outpos = ' '; 
				++outpos;
				--strpos;
			}
			// If the token is an operator, op1, then:
			else if(is_operator(c))  {
				while(sl > 0)    {
					sc = stack[sl - 1];
					// While there is an operator token, op2, at the top of the stack
					// op1 is left-associative and its precedence is less than or equal to that of op2,
					// or op1 has precedence less than that of op2,
					// Let + and ^ be right associative.
					// Correct transformation from 1^2+3 is 12^3+
					// The differing operator priority decides pop / push
					// If 2 operators have equal priority then associativity decides.
					if(is_operator(sc) &&
						((op_left_assoc(c) && (op_preced(c) <= op_preced(sc))) ||
						(op_preced(c) < op_preced(sc))))   {
							// Pop op2 off the stack, onto the output queue;
							*outpos = sc; 
							++outpos;
							*outpos = ' '; 
							++outpos;
							sl--;
						}
					else   {
						break;
					}
				}
				// push op1 onto the stack.
				stack[sl] = c;
				++sl;
			}
			// If the token is a left parenthesis, then push it onto the stack.
			else if(c == '(')   {
				stack[sl] = c;
				++sl;
			}
			// If the token is a right parenthesis:
			else if(c == ')')    {
				bool pe = false;
				// Until the token at the top of the stack is a left parenthesis,
				// pop operators off the stack onto the output queue
				while(sl > 0)     {
					sc = stack[sl - 1];
					if(sc == '(')    {
						pe = true;
						break;
					}
					else  {
						*outpos = sc; 
						++outpos;
						*outpos = ' '; 
						++outpos;						
						sl--;
					}
				}
				// If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
				if(!pe)  {
					*err = "Error: parentheses mismatched\n";
					return false;
				}
				// Pop the left parenthesis from the stack, but not onto the output queue.
				sl--;
			}
			else  {
				*err = "Unknown token \n";
				return false; // Unknown token
			}
		}
		++strpos;
	}
	// When there are no more tokens to read:
	// While there are still operator tokens in the stack:
	while(sl > 0)  {
		sc = stack[sl - 1];
		if(sc == '(' || sc == ')')   {
			*err = "Error: parentheses mismatched\n";
			return false;
		}
		*outpos = sc; 
		++outpos;
		*outpos = ' '; 
		++outpos;
		--sl;
	}
	*outpos = 0; // Null terminator
	return true;
}

enum TYPE{
	NUM,
	OP,
	VALUE,
};

struct Cacul_value{
	enum TYPE type;
	union {
		float num;
		char  operator;
		int	  reg_pos;
	} u;
};

#define NAME_LEN 32
struct Reg_value{
	char name[NAME_LEN];
	unsigned int name_hash;
	struct Reg_value *next;
};

#define REG_LEN 64
#define INIT_LEN 8

struct Expression{
	char 	*dump_string;
	struct Cacul_value *cacule_queue;
	int queue_used_num;
    int *cacule_index;          /*在这个公式用到的RegValue的index*/
    int total_index_num;            /*index个数*/
    bool b_have_rand;           //  是否含有随机数生成的运算
	struct Expression *next;
};

struct Expressions{
	struct Expression *exp_list_beg;
	struct Expression *exp_list_end;
	struct 	Reg_value *reg_value;
	int		reg_used_num;
	int		reg_free_pos;
	int		reg_cap;
    bool    b_have_rand;           //  是否含有随机数生成的运算
};

enum STATUS{
	S_READ,
	S_WRITE
};

struct Attrib{
	struct Expressions *exps;
	float *reg;      /*两个值 一个保存上次的值*/
	struct Cacul_value* stack;
	enum STATUS status;
    int     roll;       // roll值 -1：表示公式集中没有随机数运算 false:表示不需要重新roll true:表示需要重新roll
    unsigned long _holdrand; //保存随机数变化种子
};

static unsigned int 
hash(const char *string){

	unsigned int l = strlen(string);
	unsigned int h = l;
	size_t step = (l>>5)+1;
	size_t ll;

	for(ll=1; ll>=step; ll-=step){
		h = h ^ ((h<<5)+(h>>2) + (unsigned char)(string[ll-1]));
	}
	return h;
}
// 由于c语言库本身的rand函数不是线程安全的
// 所以这里按照库函数的随机算法实现了一个线程安全函数
static int inline
_rand (struct Attrib *attrib)
{
        return( ((attrib->_holdrand = attrib->_holdrand * 214013L
            + 2531011L) >> 16) & 0x7fff );
}


static void 
_exps_change_reg_pos(struct Expressions *exps, int pre_pos, int now_pos){
	struct Expression *exp = exps->exp_list_beg;
	for(;exp;exp=exp->next){
		int i;
		for(i=0;i<exp->queue_used_num;i++){
			if(exp->cacule_queue[i].type == VALUE && exp->cacule_queue[i].u.reg_pos == pre_pos){
				exp->cacule_queue[i].u.reg_pos = now_pos;
			}
		}
	}
}

static struct Reg_value*
_reg_insert(struct Expressions *exps, unsigned int name_hash){
	int main_position = name_hash % exps->reg_cap;
	struct Reg_value *result_reg_value;
	
	struct Reg_value *temp_reg = &exps->reg_value[main_position];
	//main_postion位置上有元素
	if(temp_reg->name_hash != 0){
		int temp_main_position = temp_reg->name_hash % exps->reg_cap;
		struct Reg_value *free_reg;
		while( exps->reg_value[exps->reg_free_pos].name_hash != 0){
			exps->reg_free_pos ++;
		}
		assert(exps->reg_free_pos < exps->reg_cap);
		free_reg = exps->reg_value + exps->reg_free_pos;
		exps->reg_free_pos ++;
		//当前位置的main_position与插入的元素main_position 插入元素链接在其后
		if(main_position == temp_main_position){
			result_reg_value = free_reg;
			result_reg_value->next = exps->reg_value[main_position].next;
			exps->reg_value[main_position].next = result_reg_value;
		}else{
			temp_reg = exps->reg_value[temp_main_position].next;
			*free_reg = exps->reg_value[main_position];
			free_reg->next = temp_reg;
			exps->reg_value[temp_main_position].next = free_reg;
			result_reg_value = exps->reg_value + main_position;
			// 把以前指向main_position的Reg_pos改为指向新的位置
			_exps_change_reg_pos(exps, main_position, free_reg - exps->reg_value);
			}
	}else{
		result_reg_value = exps->reg_value + main_position;
	}
	return result_reg_value;
}

static bool
_exps_get_reg_pos(struct Expressions *exps, const char *value_name, int *pos){
	unsigned int hash_value = hash(value_name);
	struct Reg_value *temp_reg = &exps->reg_value[hash_value % exps->reg_cap];
	for(;temp_reg != 0 && temp_reg->name_hash != 0; temp_reg = temp_reg->next){
		if(hash_value == temp_reg->name_hash && 
			strcmp(value_name, temp_reg->name) == 0){
			*pos = temp_reg - exps->reg_value;
			return true;
		}
	}
	return false;
}

static void
_reg_expand(struct Expressions *exps){
	struct Reg_value *old_reg = exps->reg_value;
	exps->reg_cap *= 2;
	size_t len = sizeof(struct Reg_value) * exps->reg_cap;
	exps->reg_value = (struct Reg_value*)MALLOC( len );
	memset(exps->reg_value, 0, len);
	exps->reg_free_pos = 0;
	
	struct Expression *exp;	
	struct Reg_value *result_reg, *temp_reg;
	struct Cacul_value	*cacule_value;
	int i,pos;
	//先取反
	for(exp = exps->exp_list_beg;exp;exp=exp->next){
		for(i=0;i<exp->queue_used_num;i++){
			if( exp->cacule_queue[i].type == VALUE ){
				exp->cacule_queue[i].u.reg_pos = ~exp->cacule_queue[i].u.reg_pos;
			}
		}		
	}
	//再插入
	for(exp = exps->exp_list_beg;exp;exp=exp->next){
		for(i=0;i<exp->queue_used_num;i++){
			if( exp->cacule_queue[i].type == VALUE ){
				cacule_value = exp->cacule_queue + i;
				temp_reg = old_reg + (~cacule_value->u.reg_pos);
				if(_exps_get_reg_pos(exps, temp_reg->name, &pos)){
					cacule_value->u.reg_pos = pos;
				}else{
					result_reg = _reg_insert(exps, temp_reg->name_hash);
					strcpy(result_reg->name, temp_reg->name);
					result_reg->name_hash = temp_reg->name_hash;
					cacule_value->u.reg_pos = result_reg - exps->reg_value;
				}
			}
		}		
	}	
	FREE(old_reg);
}

static bool 
_compile( struct Expressions *exps, const char *input, char **err){
	struct Expression *exp = (struct Expression*)MALLOC(sizeof(*exp));
	memset( exp, 0, sizeof(*exp) );
	if(exps->exp_list_beg == 0){
		exps->exp_list_beg = exp;
		exps->exp_list_end = exp;
	}else{
		exps->exp_list_end->next = exp;
		exps->exp_list_end = exp;
	}
	char output[1024];
	memset(output,0,sizeof(output));
	if(!shunting_yard(input, output, err)){
		return false;
	}else{
		exp->dump_string = (char*)MALLOC(strlen(output) + 1);
		strcpy(exp->dump_string, output);
		const char *str_pos = exp->dump_string, *end_pos = exp->dump_string + strlen(exp->dump_string);
		size_t queue_cap = INIT_LEN;
		exp->cacule_queue = (struct Cacul_value*)MALLOC(sizeof(struct Cacul_value)*queue_cap);
        bool b_first_name = true;//第一个name value，由于等号左边的为计算所得的值，在判断是否有变化的时候，这个值可以剔除
        size_t index_cap = INIT_LEN;
        exp->cacule_index = (int *)MALLOC(sizeof(int) * index_cap);
		while(str_pos < end_pos){
			if(*str_pos == ' '){		//空格跳过
				str_pos ++;
			}else{
				if(exp->queue_used_num >= queue_cap){
					queue_cap *= 2;
					exp->cacule_queue = (struct Cacul_value*)REALLOC(exp->cacule_queue, sizeof(struct Cacul_value)*queue_cap);
				}			
				struct Cacul_value *p_cacule = 	exp->cacule_queue + exp->queue_used_num;
				if(*str_pos >= '0' && *str_pos <= '9'){
					p_cacule->type = NUM;
					p_cacule->u.num = strtof(str_pos, (char **)&str_pos);
					exp->queue_used_num++;
				}else if( is_operator(*str_pos) ){
                    // 判断是否为随机数运算
                    if( *str_pos == '@'){
                        exp->b_have_rand = true;
                        exps->b_have_rand = true;
                    }
					p_cacule->type = OP;
					p_cacule->u.operator = *str_pos;
					exp->queue_used_num++;
					str_pos++;
				}else {
					int pos = 0;
                    int i;
					char name[NAME_LEN];
					unsigned int hash_value;
					bool b_has_reg = false;
					while(*str_pos != ' '){
						if(pos > NAME_LEN){
							*err = " token name is Too long(>32)\n";
							return false;
						}
						name[pos++] = *str_pos;
						str_pos ++;
					}
					name[pos] = '\0';
					hash_value = hash(name);
					p_cacule->type = VALUE;
					b_has_reg = _exps_get_reg_pos(exps, name, &pos);
					if(b_has_reg){
						p_cacule->u.reg_pos = pos;
					}else{//需要插入这个Reg
						if(exps->reg_used_num >= exps->reg_cap){
							_reg_expand(exps);
						}
						exps->reg_used_num ++;
						struct Reg_value *result_reg_value = _reg_insert(exps, hash_value);
						result_reg_value->name_hash = hash_value;
						strcpy(result_reg_value->name, name);
						p_cacule->u.reg_pos = result_reg_value - exps->reg_value;
					}
					exp->queue_used_num++;
                    
                    //判断是否要加入到cacule_index里面去
                    pos = p_cacule->u.reg_pos;
                    b_has_reg = false;
                    for(i = 0 ; i < exp->total_index_num; ++i){
                        if (exp->cacule_index[i] == pos){
                            b_has_reg = true;
                            break;
                        }
                    }
                    //第一次不加入到cacule_index里面去
                    if(!b_has_reg && b_first_name){
                        b_first_name = false;
                        b_has_reg = true; 
                    }
                    if (!b_has_reg){
                        ++ exp->total_index_num;
                        /*需要重新分配内存空间*/
                        if(exp->total_index_num > index_cap){
                            index_cap *= 2;
                            exp->cacule_index = (int*)REALLOC(exp->cacule_index, sizeof(int) * index_cap);
                        }
                        exp->cacule_index[exp->total_index_num-1] = pos;

                    }
				}
			}
		}
		exp->cacule_queue = (struct Cacul_value*)REALLOC(exp->cacule_queue, sizeof(struct Cacul_value)*exp->queue_used_num);
      exp->cacule_index = (int*)REALLOC(exp->cacule_index, sizeof(int) * exp->total_index_num);
	}
	return true;
}

static bool
_cacul(struct Attrib *attrib, char **err){
	struct Expression *exp = attrib->exps->exp_list_beg;
	int stack_pos, queue_pos, j;
	struct Cacul_value *p_cacul, *value[2];
	float num[2], result = 0;
	
	for(;exp;exp=exp->next){
		stack_pos = 0;
		queue_pos = 0;

		while(queue_pos < exp->queue_used_num){
			p_cacul = exp->cacule_queue + queue_pos;
			if(p_cacul->type == OP){
				if(stack_pos >= 2){
					for(j=0;j<2;j++){
						value[j]= &attrib->stack[stack_pos-1-j];
						if(value[j]->type == OP){
							goto ERROR;
						}else if(value[j]->type == NUM){
							num[j] = value[j]->u.num;
						}else{
							num[j] = attrib->reg[value[j]->u.reg_pos];
						}
					}				
				}else{
					goto ERROR;
				}
				switch(p_cacul->u.operator){
					case '+':
						result = num[1] + num[0];
						break;
					case '-':
						result = num[1] - num[0];
						break;
					case '*':
						result = num[1] * num[0];
						break;
					case '/':
						result = num[1] / num[0];
						break;
					case '%':
						result = (int)num[1] % (int)num[0];
						break;
                    case '^':
                        result = pow(num[1], num[0]);
                        break;
                    case '@':
                        result = (_rand(attrib) <= num[0] * RANDMAX)?num[1]:0; 
                        break;
					case '=':
						if(value[1]->type != VALUE)
							goto ERROR;
						attrib->reg[value[1]->u.reg_pos] = num[0];
						break;
					default:
						goto ERROR;
				}
				stack_pos -= 2;
				attrib->stack[stack_pos].type = NUM;
				attrib->stack[stack_pos].u.num = result;
				stack_pos ++;
			}else{
				attrib->stack[stack_pos++] = *p_cacul;
			}
			queue_pos ++;
		}
	}

    if(attrib->exps->b_have_rand && attrib->roll == true){
        attrib->roll = false;
    }

	return true;
ERROR:
	*err = "in caculate,expression is illegal\n";
	return false;
} 

struct Expressions* 
exp_create(){
	struct Expressions *exps = (struct Expressions*) MALLOC(sizeof(*exps));
	memset( exps, 0, sizeof(*exps) );
	exps->reg_cap = INIT_LEN;
	size_t len = sizeof(struct Reg_value) * exps->reg_cap;
	exps->reg_value = (struct Reg_value*)MALLOC( len );
	memset(exps->reg_value, 0, len);
	return exps;
}

int 
exp_dumpstring(struct Expressions *exps, int which_exp,  char **dump){
	struct Expression *exp = exps->exp_list_beg;
	int which = 0;
	for(;exp;exp=exp->next){
		which++;
		if(which == which_exp){
			*dump = exp->dump_string;
			return true;
		}
	}
	return false;
}



void 
exp_release( struct Expressions *exps ){
	struct Expression *exp = exps->exp_list_beg;
	struct Expression *temp;
	while(exp){
		temp = exp;
		exp = exp->next;
		FREE(temp->cacule_queue);
		FREE(temp->dump_string);
        FREE(temp->cacule_index);
		FREE(temp);
	}
	FREE(exps->reg_value);
	FREE(exps);
	
}

int	
exp_push(struct Expressions *exps, const char *exp, char **err){
	return _compile(exps, exp, err);
}



// 重新roll
void
attrib_roll(struct Attrib *attrib){
   if(attrib->exps->b_have_rand){   //有随机数运算才需要继续处理
       attrib->roll = true;
       attrib->status = S_READ;
   }
}

struct Attrib *
attrib_create( struct Expressions *exps ){
	struct Attrib *attrib = (struct Attrib*)MALLOC(sizeof(*attrib));
	struct Expression *exp;
	int stack_max_len = 0;
	
	memset(attrib, 0, sizeof(*attrib));
	attrib->exps = exps;
	attrib->status = S_WRITE;
	
    attrib->reg = (float*)MALLOC(sizeof(float) * exps->reg_cap);
    memset(attrib->reg, 0, sizeof(float) * exps->reg_cap);

	//MALLOC stack space
	for(exp = exps->exp_list_beg;exp;exp=exp->next){
		if(exp->queue_used_num > stack_max_len){
			stack_max_len = exp->queue_used_num;
		}
	}
	attrib->stack = (struct Cacul_value*)MALLOC(sizeof(struct Cacul_value)*stack_max_len);
	memset(attrib->stack, 0, sizeof(struct Cacul_value)*stack_max_len);

    //初始化随机函数种子
    if(exps->b_have_rand){
        attrib->_holdrand = time(NULL);
        PRINTF("_holdrand = %ld\n", attrib->_holdrand);
        attrib_roll(attrib); 
    }
	
	return attrib;
}

void 
attrib_release(struct Attrib* attrib){
	FREE(attrib->stack);
	FREE(attrib->reg);
	FREE(attrib);
}

static bool 
_attrib_get_reg_pos(struct Attrib *attrib, const char *value_name, int *pos, char **err){
	struct Expressions *exps = attrib->exps;
	if(_exps_get_reg_pos(exps, value_name, pos))
		return true;
	*err = "cannot found this Value\n";
	return false;
}

int	
attrib_write( struct Attrib *attrib, const char *value_name, float value, char **err){
	int pos;
	if(_attrib_get_reg_pos(attrib, value_name, &pos, err)){
		attrib->reg[pos] = value;
		attrib->status = S_READ;
		return true;
	}
	return false;
}

int 
attrib_read( struct Attrib *attrib, const char *value_name, float* value, char **err){
		int pos;
	if(_attrib_get_reg_pos(attrib, value_name, &pos, err)){
		if(attrib->status == S_READ){
			if(!_cacul(attrib, err))
				return false;
		}
		*value = attrib->reg[pos];
		attrib->status = S_WRITE;
		return true;
	}
	return false;
}


struct Expressions *
attrib_get_exps(struct Attrib *attrib){
    return attrib->exps;
}

void 
attrib_dump(struct Attrib *attrib){
    const size_t buffer_size = 2048;
    char buffer[buffer_size];
    size_t rest_size = buffer_size;
    size_t i;
    struct Expressions* exps = attrib->exps;
    char *error;
    float value = 0;
    for(i = 0; i <exps->reg_cap; i++){
        if(strlen(exps->reg_value[i].name) != 0){
            const char *name = exps->reg_value[i].name;
            attrib_read(attrib,name, &value, &error);
            rest_size -= snprintf(buffer + (buffer_size-rest_size), rest_size, "%s : %f\n", name, value);
        }
    }
    printf("dump values = \n%s\n", buffer);
}
