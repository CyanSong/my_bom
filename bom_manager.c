#include "mydb_manager.h"
#include <time.h>
#include <stdlib.h>
#define QUERY_BY_ID 1
#define QUERY_BY_NAME 2

void
usage()
{
    fprintf(stdout, "Usage:\n\
        a for add random items\n\
        i for insert\n\
        r for remove\n\
        u for update\n\
        q for query\n\
        o for quit\n\
        s for show all records\n\
        EOF for quit\n");
}
       
BOM_DBS my_bom;

void user_interface();

int bom_init();
int add_random_cpt(unsigned int num,unsigned int level);
int add_random_product(unsigned int num);

int bom_insert_product(PRODUCT *);
int bom_remove_product(KEY);
int bom_query_product(PRODUCT *,unsigned int);
int bom_update_product(unsigned int,PRODUCT *);

int bom_insert_cpt(CPT *);
int bom_remove_cpt(KEY);
int bom_query_cpt(CPT *,unsigned int);
int bom_update_cpt(unsigned int,CPT *);
 
static char *rand_string(char *str, size_t size);
void update_interface();
void insert_interface();
void remove_interface();
void query_interface();
static void my_read_string(char *str,unsigned int len);

int main(){
    srand(time(NULL));   
    bom_init();
    user_interface();
    printf("Program exit\n");
	return 0;
}

void 
user_interface()
{
    usage();
    char cmd;
    while(1){
        printf("please input:\n");
        while ((cmd = getchar()) == '\n'){}
        
        switch(cmd){
            case 'a':
                add_random_cpt(100,5);
                add_random_product(10);   
                break;
            case 'i':
                insert_interface();
                break;
            case 'q':
                query_interface();
                break;
            case 'r':
                remove_interface();
                break;
            case 'u':
                update_interface();
                break;
            case 's':
                print_products(&my_bom,-1);
                print_cpts(&my_bom,SHADOW_UNKNOWN,-1 );
                break;
            case 'o':
                return ;
            default:
                usage();
                break;
        }
    }
}

void insert_interface()
{
    unsigned int num,i;
    KEY id;
    char name[MAX_NAME_LEN],model[MAX_MODEL_LEN],type;
    SUBCPT_DATA * tmp;
    PRODUCT p;
    CPT c;
    memset(&p,0,sizeof(PRODUCT));
    memset(&c,0,sizeof(CPT));

    printf("please input item type:(p for product, c for component)\n");
    while ((type = getchar()) != 'p' && type !='c'){}

    printf("please input the id of the item:\n");
    scanf("%d",&id);

    printf("please input item name:\n");
    my_read_string(name,MAX_NAME_LEN);


    printf("please input the number of item's subcomponents:\n");
    scanf("%d",&num);

    tmp = calloc(num,sizeof(SUBCPT_DATA));
    for(i=0;i<num;i++){
        printf("please input the id for the %dth item of %d subcomponent\n",i+1,num);
        scanf("%d",&(tmp[i].id));

        printf("please input the number for the %dth item of %d subcomponent\n",i+1,num);
        scanf("%d",&(tmp[i].num));
    }
    
    if(type=='c'){

        printf("please input item model:\n");        
        my_read_string(model,MAX_MODEL_LEN);
        c.id = id;
        strcpy(c.data.name,name);
        strcpy(c.data.model,model);
        c.data.subcpt_num = num;
        c.data.subcpt = tmp;
        bom_insert_cpt(&c);
    }else{
        p.id = id;
        strcpy(p.data.name,name);
        p.data.subcpt_num = num;
        p.data.subcpt = tmp;
        bom_insert_product(&p);
    }
    if(tmp!=NULL){
        free(tmp);
        tmp=NULL;
    }

    printf("finish inserting!\n");
}


void remove_interface()
{
    KEY id;
    char type;
    int ret;

    printf("please input the type of item to be removed:(p for product, c for component)\n");
    while ((type = getchar()) != 'p' && type !='c'){}

    printf("please input the id of item to be removed\n");
    scanf("%d",&id);

    if(type=='c')
        ret = bom_remove_cpt(id);
    else
        ret = bom_remove_product(id);
    if(ret ==0)
        printf("finish remove!\n");
    else
        printf("removal failed!\n");
}


void query_interface(){
    KEY id;
    char type,name[MAX_NAME_LEN];
    int ret,i;
    int method;
    PRODUCT p;
    CPT c;
    memset(&p,0,sizeof(PRODUCT));
    memset(&c,0,sizeof(CPT));
    
    printf("please input the type of item you want to query:(p for product, c for component)\n");
    while ((type = getchar()) != 'p' && type !='c'){}

    do{
        printf("which way do you want to query by(1 for id, 2 for name)\n");
        scanf("%d",&method);
    }while(method!=1&&method!=2);

    if(method ==QUERY_BY_NAME){
        printf("please input the name of item you want to query\n");
        my_read_string(name,MAX_NAME_LEN);
    }
    else{
        printf("please input the id of item you want to query\n");
        scanf("%d",&id);
    }
    if(type=='c' && method==QUERY_BY_NAME){
        strcpy(c.data.name,name);
        ret = bom_query_cpt(&c,method);
    }else if (type=='c' && method==QUERY_BY_ID){
        c.id = id;
        ret = bom_query_cpt(&c,method);
    }else if(type=='p' && method==QUERY_BY_ID){
        p.id =id;
        ret = bom_query_product(&p,method);
    }else {
        strcpy(p.data.name,name);
        ret = bom_query_product(&p,method);
    }
    if(ret==0){
        if(p.data.subcpt!=NULL){
            free(p.data.subcpt);
            p.data.subcpt=NULL;
        }
        if(c.data.subcpt!=NULL){
            free(c.data.subcpt);
            c.data.subcpt=NULL;
        }
        printf("finish query!\n");
    }
    else
        printf("Not Found! \n");
}


void update_interface()
{
    KEY id;
    char type;
    int ret,i,flag=0;
    int method;
    PRODUCT p;
    CPT c;
    memset(&p,0,sizeof(PRODUCT));
    memset(&c,0,sizeof(CPT));
    
    printf("please input the type of item you want to update:(p for product, c for component)\n");
    while ((type = getchar()) != 'p' && type !='c'){}

    printf("please input the id of item you want to update\n");
    scanf("%d",&id);

    printf("Original item is:\n");
    switch(type){
        case 'c':
            c.id =id;
            bom_query_cpt(&c,QUERY_BY_ID);
            break;
        case 'p':
            p.id = id;
            bom_query_product(&p,QUERY_BY_ID);
            break;
        default:
            break;
    }

    do{
        printf("which do you want to update\n\
            1. update id\n\
            2. update name\n\
            3. upadte model(only for component)\n");
        scanf("%d",&method);
        
        switch(method){
            case 1:
                printf("please input new id:\n");
                if(type=='c')
                    scanf("%d",&c.id);
                else
                    scanf("%d",&p.id);
                break;
            case 2:
                printf("please input new name:\n");
                if(type =='c'){
                    my_read_string(c.data.name,MAX_NAME_LEN);
                }else{
                    my_read_string(p.data.name,MAX_NAME_LEN);
                }
                break;
            case 3:
                if(type=='p')
                    flag = 1;
                else{
                    printf("please input new model:\n");
                    my_read_string(c.data.model,MAX_NAME_LEN);
                }
                break;
            default:
                flag=1;
                break;
        }
    }while(flag);

    if(type=='p')
        ret =bom_update_product(id,&p);
    else
        ret = bom_update_cpt(id,&c);

    if(ret==0){
        if(p.data.subcpt!=NULL){
            free(p.data.subcpt);
            p.data.subcpt=NULL;
        }
        if(c.data.subcpt!=NULL){
            free(c.data.subcpt);
            c.data.subcpt=NULL;
        }
        printf("finish update!\n");
    }
    else
        printf("update failed！\n");
}


int 
bom_init()
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


int 
add_random_cpt(unsigned int num,unsigned int level)
{
    CPT tmp;
    memset(&tmp,0,sizeof(CPT));
    int i,j,k;
    for(i=0;i<level;i++){
        for(j=0;j<num;j++){
            if(tmp.data.subcpt!=NULL){
                free(tmp.data.subcpt);
                tmp.data.subcpt=NULL;
            }
            memset(&tmp,0,sizeof(CPT));
            tmp.id = i*10000+rand()%10000;
            rand_string(tmp.data.name,10);
            rand_string(tmp.data.model,10);
            if(i!=level-1){
                tmp.data.subcpt_num=rand()%8;
                if(tmp.data.subcpt_num!=0)
                    tmp.data.subcpt = calloc( tmp.data.subcpt_num,sizeof(SUBCPT_DATA) );
                for(k=0;k<tmp.data.subcpt_num;k++){
                    tmp.data.subcpt[k].id = (1+i)*10000+rand()%10000;
                    tmp.data.subcpt[k].num = rand()%10+1;
                }
            }
            bom_insert_cpt(&tmp);
        }
    }
    printf("add %d random components\n",num);
    return 0;
}

int 
add_random_product(unsigned int num)
{
    int j,k;
    PRODUCT tmp;
    memset(&tmp,0,sizeof(PRODUCT));
    for(j=0;j<num;j++){
        if(tmp.data.subcpt!=NULL){
            free(tmp.data.subcpt);
            tmp.data.subcpt=NULL;
        }
        memset(&tmp,0,sizeof(PRODUCT));
        tmp.id = rand()%10000;
        rand_string(tmp.data.name,10);
        tmp.data.subcpt_num=rand()%8;
        if(tmp.data.subcpt_num!=0)
            tmp.data.subcpt = calloc( tmp.data.subcpt_num,sizeof(SUBCPT_DATA) );
        for(k=0;k<tmp.data.subcpt_num;k++){
            tmp.data.subcpt[k].id = 10000+rand()%10000;
            tmp.data.subcpt[k].num = rand()%10+1;
        }
        bom_insert_product(&tmp);
    }
    printf("add %d random product\n",num);
    return 0;
}


static char *rand_string(char *str, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyz";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}


int 
bom_insert_product(PRODUCT * p)
{
    return insert_product(&my_bom,p);
}

int 
bom_remove_product(KEY id)
{
    return secure_remove_product(&my_bom,id);
}

int 
bom_query_product(PRODUCT *p, unsigned int method)
{
    int ret;
    switch(method){
        case QUERY_BY_NAME:
            ret =  get_product_by_name(&my_bom,p);
            break;
        case QUERY_BY_ID:
            ret = get_product_by_id(&my_bom,p);
            break;
        default:
            return -1;
    }
    if(ret==0)
        print_product(&my_bom,p);
    return ret;
}

int 
bom_update_product(KEY id,PRODUCT *p)
{
    int ret;
    PRODUCT tmp;
    memset(&tmp,0,sizeof(PRODUCT));
    tmp.id = id;
    get_product_by_id(&my_bom,&tmp);

    ret = bom_remove_product(id);
    if(ret ==0)
        ret = bom_insert_product(p);
    else
        return -1;
    if(ret != 0)
        bom_insert_product(&tmp);
    
    if(tmp.data.subcpt!=NULL){
        free(tmp.data.subcpt);
        tmp.data.subcpt=NULL;
    }
    return ret;
}

int 
bom_insert_cpt(CPT *c)
{
    return insert_cpt(&my_bom,c);
}

int 
bom_remove_cpt(KEY id)
{
    return secure_remove_cpt(&my_bom,id);
}

int 
bom_update_cpt(KEY id, CPT*c)
{
    int ret,count;
    CPT tmp;
    memset(&tmp,0,sizeof(CPT));
    tmp.id =id;
    ret = get_cpt_by_id(&my_bom,&tmp,&count);
    if(ret!=0)
        return ret;

    ret = bom_remove_cpt(id);
    if(ret!=0)
        return ret;

    ret = bom_insert_cpt(c);

    if(ret != 0)
        bom_insert_cpt(&tmp);
    
    if(tmp.data.subcpt!=NULL){
        free(tmp.data.subcpt);
        tmp.data.subcpt=NULL;
    }
    return ret;
}

int 
bom_query_cpt(CPT *c,unsigned int method)
{
    unsigned int count;
    int ret;
    switch(method){
        case QUERY_BY_ID:
            ret = get_cpt_by_id(&my_bom,c,&count);
            break;
        case QUERY_BY_NAME:
            ret = get_cpt_by_name(&my_bom,c,&count);
            break;
        default:
            return -1;
    }
    if(ret==0){
        print_cpt(&my_bom,c);
        if(c->data.subcpt!=NULL){
            free(c->data.subcpt);
            c->data.subcpt=NULL;
        }
    }
    return ret;
}
static void my_read_string(char *str,unsigned int len){
    int i;
    memset(str,0,len);
    do{
        fgets(str, len-1, stdin);
    }while(str[0]=='\n'||str[0]=='\0');
    i = strlen(str)-1;
    if( str[i] == '\n') 
        str[i] = '\0';
}