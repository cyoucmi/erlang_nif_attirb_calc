#ifndef __attrib__h__
#define __attrib__h__

struct Expressions;

struct Expressions* exp_create();
int	exp_push(struct Expressions *exps, const char *exp, char **err);
void exp_release( struct Expressions *exps );
int exp_dumpstring(struct Expressions *exps, int which_exp,  char **dump);

struct Attrib;

struct Attrib *attrib_create( struct Expressions *exps );
int	attrib_write( struct Attrib *attrib, const char *value_name, float value, char **err);
int attrib_read( struct Attrib *attrib, const char *value_name, float* value, char **err);
struct Expressions *attrib_get_exps(struct Attrib *attrib);
void attrib_roll(struct Attrib *attrib);
void attrib_dump(struct Attrib *attrib);
void attrib_release(struct Attrib*);

#endif
