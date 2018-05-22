#ifndef _MYDB_MANAGER_H 
#define _MYDB_MANAGER_H

#include <db.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef _WIN32
extern int getopt(int, char * const *, const char *);
extern char *optarg;
#define snprintf _snprintf
#else
#include <unistd.h>
#endif


#define REDEFINE_CPT -1
#define MAXDATABUF 1024
#define MAX_NAME_LEN 19
#define PRIMARY_DB	0
#define SECONDARY_DB	1
#define PRODUCT_CPT_NUM 4
#define CPTDB "cptDB.db"
#define PRODUCTDB	"productDB.db"
#define PRODUCTNAMEDB	"product_nameDB.db"
#define CPTNAMEDB	"cpt_nameDB.db"
#define DEFAULT_HOMEDIR	"./"

typedef unsigned int KEY;

typedef struct{
	DB *product_dbp;
	DB *cpt_dbp;
	DB *product_name_sdbp;
	DB *cpt_name_sdbp;

	char *db_home_dir;            
    char *product_db_name; 
    char *cpt_db_name; 
    char *product_name_db_name; 
    char *cpt_name_db_name;   
} DOM_DBS;


typedef struct subcpt_store{
  KEY id;
  unsigned int num;
}SUBCPT_DATA;

typedef struct product_store{
    char name[MAX_NAME_LEN + 1];
    SUBCPT_DATA subcpt[4];
}PRODUCT_DATA;

typedef struct cpt_store{
  char name[MAX_NAME_LEN + 1];
  unsigned int subcpt_num;
  SUBCPT_DATA * subcpt;
} CPT_DATA;


typedef struct component {
	KEY id;
	CPT_DATA data; 
} CPT;



typedef struct product{
	KEY id;
	PRODUCT_DATA data;
} PRODUCT;

int unit_test();   
int	open_database(DB **, const char *, const char *, FILE *, int);
int	databases_setup(DOM_DBS *, const char *, FILE *);
int	databases_close(DOM_DBS *);
void set_db_filenames(DOM_DBS *);
void initialize_stockdbs(DOM_DBS *);

int insert_products(DOM_DBS *,PRODUCT *);
int insert_cpt(DOM_DBS *,CPT *);

int get_product_by_id(DOM_DBS *,PRODUCT *);
int get_product_by_name(DOM_DBS *,PRODUCT *);
int get_cpt_by_id(DOM_DBS *,CPT *,unsigned int *);
int get_cpt_by_name(DOM_DBS *,CPT *,unsigned int *);

int secure_remove_product(DOM_DBS *,KEY);
int secure_remove_cpt(DOM_DBS *,KEY);
void print_cpt(CPT * );
void print_product(PRODUCT *);


#endif