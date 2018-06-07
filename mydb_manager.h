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
#define MAX_MODEL_LEN 19
#define SHADOW_UNKNOWN 1
#define PRIMARY_DB	0
#define SECONDARY_DB	1
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
} BOM_DBS;


typedef struct subcpt_store{
  KEY id;
  unsigned int num;
}SUBCPT_DATA;

typedef struct product_store{
    char name[MAX_NAME_LEN + 1];
    unsigned int subcpt_num;
    SUBCPT_DATA * subcpt;
}PRODUCT_DATA;

typedef struct cpt_store{
  char name[MAX_NAME_LEN + 1];
  char model[MAX_MODEL_LEN +1];
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

int	open_database(DB **, const char *, const char *, FILE *, int);
int	databases_setup(BOM_DBS *, const char *, FILE *);
int	databases_close(BOM_DBS *);
void set_db_filenames(BOM_DBS *);
void initialize_stockdbs(BOM_DBS *);

int insert_product(BOM_DBS *,PRODUCT *);
int insert_cpt(BOM_DBS *,CPT *);

int get_product_by_id(BOM_DBS *,PRODUCT *);
int get_product_by_name(BOM_DBS *,PRODUCT *);
int get_cpt_by_id(BOM_DBS *,CPT *,unsigned int *);
int get_cpt_by_name(BOM_DBS *,CPT *,unsigned int *);

int secure_remove_product(BOM_DBS *,KEY);
int secure_remove_cpt(BOM_DBS *,KEY);

void print_cpt(BOM_DBS *,CPT *);
void print_product(BOM_DBS *,PRODUCT *);
void print_cpt_raw(CPT * );
void print_product_raw(PRODUCT *);

int print_products(BOM_DBS *,unsigned int limit);
int print_cpts(BOM_DBS *,int flag,unsigned int limit);

#endif