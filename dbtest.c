#include "mydb_manager.h"

int
usage()
{
    fprintf(stderr, "example_database_load [-b <path to data files>]");
    fprintf(stderr, " [-h <database_home_directory>]\n");

    fprintf(stderr, "\tNote: Any path specified must end with your");
    fprintf(stderr, " system's path delimiter (/ or \\)\n");
    return (-1);
}

int main(){
unit_test();
return 0;
//return show_all_records();
}
