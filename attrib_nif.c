#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "attrib.h"
#include "header.h"

#define MAX_FORMULATE_NUM 1024 /*默认的数量*/
#define MAX_PATH 256
#define BUFF_LEN 64

static struct Expressions *Exps[MAX_FORMULATE_NUM]; /*保存所有公式*/
static ErlNifResourceType *AttribResourceType; 

/*加载attrib_nif*/
static int attrib_load(ErlNifEnv *env, void **priv_data, ERL_NIF_TERM load_info);

/*卸载attrib_nif*/
static void attrib_unload(ErlNifEnv *env, void *priv_data);

static ERL_NIF_TERM attrib_init(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM attrib_new(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM attrib_set(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM attrib_get(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM attrib_add(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM attrib_sub(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM attrib_dumpstring(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM attrib_roll1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM attrib_dump1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);

static int 
openfile(const char *file_name,char **data, size_t *len,  char **error){
	FILE *pFile;
	pFile = fopen(file_name, "r");
	if(pFile == NULL) {
		*error = "fopen attrib.txt file fail!!!\n";
		return 1;
	}else{
		fseek(pFile, 0, SEEK_END);
		size_t size = ftell(pFile);
		char *buff = (char *)MALLOC(sizeof(char) * (size+1));
		rewind(pFile);
		size_t result = fread(buff, 1, size, pFile);
		if(result != size){
			*error = "read attrib.txt file error!!!\n";
			fclose(pFile);
			return 1;
		}
		buff[size] = '\0';
		*data = buff;
		*len = size;
		fclose(pFile);
		return 0;
	}
	return 0;
}

// 剔除注释的语句（由 %% 开始 到 \n结束
static int
remove_comment(char *p_data, size_t len){
    char *p_cursor = p_data;
    while((p_cursor = strchr(p_cursor, '%')) && p_cursor != NULL){
        if(p_cursor - p_data + 1 < len) { // 不是最后一个字符
            // 如果第二个字符也为 %
            if(*(p_cursor + 1) == '%'){
                // 寻找 \n
                char * p_end = strchr(p_cursor, '\n');
                memset(p_cursor, ' ', p_end - p_cursor + 1); //剔除注释 改为空格
            }
        }
    }
    //printf("%s\n",p_data);
    return 0;
}

static int 
parsefile(const char* file_name, char **error){
	char *data;
	size_t len;
	char *p_search, *p_beg;
	if(openfile(file_name, &data, &len, error)){
		return 1;	
	}
    
    remove_comment(data,len);
	
    p_beg = data;
	p_search = strchr(p_beg, ';');
	while(p_search != NULL){
		char * p_colon = strchr(p_beg,':');
		if(p_colon == NULL || p_colon > p_search)
			goto ERROR;
		*p_colon = '\0';
		int index  = atoi(p_beg);
		PRINTF("index = %d\n",index);
        if (index < 0 || index >= MAX_FORMULATE_NUM){
            *error = "parse attrib.txt, INDEX < 0 || INDEX >= MAX_FORMULATE_NUM";
            return 0;
        }
        Exps[index] = exp_create();
		char *p_dot = strchr(p_colon+1,',');
		while(p_dot != NULL && p_dot < p_search){
			*p_dot = '\0';
			PRINTF("formula = %s\n", p_colon+1);
			/*有错误*/
            if(!exp_push(Exps[index],p_colon+1,error)){
                return 0;
            }
            p_colon = p_dot + 1;	
			p_dot = strchr(p_dot+1,',');
				
		}
		*p_search = '\0';
		PRINTF("formula = %s\n", p_colon+1);
		/*有错误*/
        if(!exp_push(Exps[index],p_colon+1,error)){
            return 0;
        }
		p_beg = p_search + 1;
		p_search = strchr(p_beg, ';');
		
	}
	FREE(data);
	return 0;
	
ERROR:
	*error = "parser attrib.txt file error!!!\n";
	FREE(data);
	return 1;	
}

static void
AttribResourceDtor(ErlNifEnv *env, void *obj){
    struct Attrib **attrib = obj;
    
    attrib_release(*attrib);
    PRINTF("Attrib GC%s\n","");
}

static int
attrib_load(ErlNifEnv *env, void **priv_data, ERL_NIF_TERM load_info){
    memset(Exps, 0, sizeof(struct Expressions*) * MAX_FORMULATE_NUM);
    ErlNifResourceFlags tried;
    AttribResourceType = enif_open_resource_type(env, NULL, "attrib", AttribResourceDtor, ERL_NIF_RT_CREATE,&tried);
   return 0;
};


static void 
attrib_unload(ErlNifEnv *env, void *priv_data){
    int i;
    for (i = 0; i < MAX_FORMULATE_NUM; ++i){
        if(Exps[i]){
            exp_release(Exps[i]);
        }
    }
}


static ERL_NIF_TERM 
attrib_init(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]){
    char buf[MAX_PATH];
    size_t buf_size = MAX_PATH;
    if( enif_get_atom(env, argv[0], buf, buf_size, ERL_NIF_LATIN1) <= 0){/*获取失败*/
        strcpy(buf, "attrib.txt");/**读取默认值*/
    }
    char *error;
   if(parsefile(buf, &error)){
        return enif_make_atom(env, error); 
   }
   return enif_make_atom(env, "ok");
}



static ERL_NIF_TERM 
attrib_new(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]){
    int index;
    if(enif_get_int(env, argv[0], &index)){
        if (index < 0 || index >= MAX_FORMULATE_NUM || Exps[index] == NULL){
            return enif_make_atom(env, "in attrib_new, index < 0 || index >= MAX_FORMULATE_NUM || Exps[index] == NULL");
        }
        struct Attrib **attrib = (struct Attrib**)enif_alloc_resource(AttribResourceType, sizeof(*attrib));
        *attrib = attrib_create(Exps[index]);
        ERL_NIF_TERM ret = enif_make_resource(env, (void*)attrib);
        enif_release_resource(attrib);
        return ret;
    }else{
        return enif_make_atom(env, "in attrib_new, arg[0] is not an integer or too big");
    }
}

static ERL_NIF_TERM 
attrib_set(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]){
    struct Attrib **attrib;
    double value;
    if(enif_get_resource(env, argv[0], AttribResourceType, (void**)(uintptr_t)&attrib)){
        char buf[BUFF_LEN];
	if(enif_get_atom(env, argv[1], buf, BUFF_LEN, ERL_NIF_LATIN1) <= 0){
	    return enif_make_atom(env, "in attrib_set, argv[1] is not an atom or string len big than 64");	
	}
	if(enif_get_double(env, argv[2], &value)){
            float f_value = value;
	    char *error;
	    if(!attrib_write(*attrib, buf, f_value, &error)){
	    	return enif_make_atom(env, error);
	    }
	    return enif_make_atom(env, "ok"); 
        }else{
	    return enif_make_atom(env, "in attrib_set, argv[2] is not a float");
	}
    }else{
        return enif_make_atom(env, "in attrib_set, arg[0] is not a handle to a resource object of type Attrib");
    }
}



static ERL_NIF_TERM 
attrib_get(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]){
    struct Attrib **attrib;
    if(enif_get_resource(env, argv[0], AttribResourceType, (void**)(uintptr_t)&attrib)){
        char buf[BUFF_LEN];
	if(enif_get_atom(env, argv[1], buf, BUFF_LEN, ERL_NIF_LATIN1) <= 0){
	    return enif_make_atom(env, "in attrib_get, argv[1] is not an atom or string len big than 64");	
	}
        float value;
	char *error;
	if(!attrib_read(*attrib, buf, &value, &error)){
	    return enif_make_atom(env, error);
	}
	return enif_make_double(env, value); 
    }else{
        return enif_make_atom(env, "in attrib_get, arg[0] is not a handle to a resource object of type Attrib");
    }
}



static ERL_NIF_TERM 
attrib_add(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]){
    struct Attrib **attrib;
    double value;
    if(enif_get_resource(env, argv[0], AttribResourceType, (void**)(uintptr_t)&attrib)){
        char buf[BUFF_LEN];
	if(enif_get_atom(env, argv[1], buf, BUFF_LEN, ERL_NIF_LATIN1) <= 0){
	    return enif_make_atom(env, "in attrib_add, argv[1] is not an atom or string len big than 64");	
	}
	if(enif_get_double(env, argv[2], &value)){
            float f_value;
	    char *error;
	    if(!attrib_read(*attrib, buf, &f_value, &error)){
	    	return enif_make_atom(env, error);
	    }
	    if(!attrib_write(*attrib, buf, f_value + (float)value, &error)){
	    	return enif_make_atom(env, error);
	    }
	    return enif_make_atom(env, "ok"); 
        }else{
	    return enif_make_atom(env, "in attrib_add, argv[2] is not a float");
	}
    }else{
        return enif_make_atom(env, "in attrib_add, arg[0] is not a handle to a resource object of type Attrib");
    }
}


static ERL_NIF_TERM 
attrib_sub(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]){
    struct Attrib **attrib;
    double value;
    if(enif_get_resource(env, argv[0], AttribResourceType, (void**)(uintptr_t)&attrib)){
        char buf[BUFF_LEN];
	if(enif_get_atom(env, argv[1], buf, BUFF_LEN, ERL_NIF_LATIN1) <= 0){
	    return enif_make_atom(env, "in attrib_sub, argv[1] is not an atom or string len big than 64");	
	}
	if(enif_get_double(env, argv[2], &value)){
            float f_value;
	    char *error;
	    if(!attrib_read(*attrib, buf, &f_value, &error)){
	    	return enif_make_atom(env, error);
	    }
	    if(!attrib_write(*attrib, buf, f_value - (float)value, &error)){
	    	return enif_make_atom(env, error);
	    }
	    return enif_make_atom(env, "ok"); 
        }else{
	    return enif_make_atom(env, "in attrib_sub, argv[2] is not a float");
	}
    }else{
        return enif_make_atom(env, "in attrib_sub, arg[0] is not a handle to a resource object of type Attrib");
    }
}


static ERL_NIF_TERM 
attrib_dumpstring(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]){
    struct Attrib **attrib;
    if(enif_get_resource(env, argv[0], AttribResourceType, (void**)(uintptr_t)&attrib)){
        int index;
        if(enif_get_int(env, argv[1], &index)){
            char *dump;
            if(exp_dumpstring(attrib_get_exps(*attrib), index, &dump)){
                printf("DUMPSTRING = %s\n",dump);
                return enif_make_atom(env, dump);
            }else{
                return enif_make_atom(env, "in attrib_dumpstring, DUMP STRING failed");
            }
        }else{
            return enif_make_atom(env, "in attrib_dumpstring, argv[1] is not a integer or too big"); 
        }
    }else{
        return enif_make_atom(env, "in attrib_get, arg[0] is not a handle to a resource object of type Attrib");
    }
}

static ERL_NIF_TERM 
attrib_roll1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]){
    struct Attrib **attrib;
    if(enif_get_resource(env, argv[0], AttribResourceType, (void**)(uintptr_t)&attrib)){
        attrib_roll(*attrib);
        return enif_make_atom(env, "ok");
    }else{
        return enif_make_atom(env, "in attrib_roll, arg[0] is not a handle to a resource object of type Attrib");
    }
}

static ERL_NIF_TERM 
attrib_dump1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]){
    struct Attrib **attrib;
    if(enif_get_resource(env, argv[0], AttribResourceType, (void**)(uintptr_t)&attrib)){
        attrib_dump(*attrib);
        return enif_make_atom(env, "ok");
    }else{
        return enif_make_atom(env, "in attrib_dump, arg[0] is not a handle to a resource object of type Attrib");
    }
}





static ErlNifFunc nif_funcs[] = {
	{"init", 1, attrib_init},
	{"new", 1, attrib_new},
	{"set", 3, attrib_set},
	{"get", 2, attrib_get},
	{"add", 3, attrib_add},
	{"sub", 3, attrib_sub},
    {"dumpstring", 2, attrib_dumpstring},
    {"roll", 1, attrib_roll1},
    {"dump", 1, attrib_dump1},
};

ERL_NIF_INIT(attrib_nif, nif_funcs, attrib_load, NULL, NULL, attrib_unload);
