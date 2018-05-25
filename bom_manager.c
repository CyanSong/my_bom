#include "mydb_manager.h"

#define CREATE_NEW 2
#define USE_EXIST 1

int
usage()
{
    fprintf(stderr, "example_database_load [-b <path to data files>]");
    fprintf(stderr, " [-h <database_home_directory>]\n");

    fprintf(stderr, "\tNote: Any path specified must end with your");
    fprintf(stderr, " system's path delimiter (/ or \\)\n");
    return (-1);
}

BOM_DBS my_bom;

int init_bom(unsigned int flags);
int add_random_cpt(unsigned int num);
int add_random_product(unsigned int num);
int update_bom();
int insert_bom();
int remove_bom();
int query_bom();
int print_products(unsigned int limit);
int print_cpts(unsigned int limit);



int main(){
	unit_test();
	return 0;
}


int 
init_bom(unsigned int flags)
{
	int ret,i;
    unsigned int count = 0;
    initialize_stockdbs(&my_bom);
    set_db_filenames(&my_bom);
    /* Open all databases */
    ret = databases_setup(&my_bom, "mydb_manager", stderr);
    if (ret) {
        fprintf(stderr, "Error opening databases\n");
        databases_close(&my_bom);
        return (ret);
    }
}
