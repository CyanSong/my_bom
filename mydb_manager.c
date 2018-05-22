
#include "mydb_manager.h"

#define ADD_REF 1
#define RED_REF 2
int _insert_products(DOM_DBS *,PRODUCT *);
int _insert_cpt(DOM_DBS *,CPT *,unsigned int);
int _remove_cpt(DOM_DBS *my_dom, KEY id);
int _remove_product(DOM_DBS *my_dom, KEY id);
int _modify_reference(DOM_DBS *my_dom, KEY id,unsigned int flags);
void pack_cpt(CPT_DATA *c_data,char * dataBuf,unsigned int *count);
void _add_unknown_cpt(DOM_DBS *my_dom, KEY id,unsigned int count);
int get_product_name(DB *, const DBT *, const DBT *, DBT *);
int get_cpt_name(DB *, const DBT *, const DBT *, DBT *);

int
unit_test()
{
   DOM_DBS my_dom;
    int ret,i;
    unsigned int count = 0;
    initialize_stockdbs(&my_dom);
    set_db_filenames(&my_dom);
    /* Open all databases */
    ret = databases_setup(&my_dom, "mydb_manager", stderr);
    if (ret) {
        fprintf(stderr, "Error opening databases\n");
        databases_close(&my_dom);
        return (ret);
    }

    PRODUCT p1,p2;
    memset(&p1,0,sizeof(PRODUCT));
    memset(&p2,0,sizeof(PRODUCT));
    p1.id = 6;
    memcpy(p1.data.name, "src",4);
    memcpy(p2.data.name, "src",4);
    for( i=0;i<PRODUCT_CPT_NUM;i++)
    { 
      p1.data.subcpt[i].num = i;
      p1.data.subcpt[i].id = i*2;
    }  
    printf("Done initialize product.\n");
    ret = insert_products(&my_dom,&p1);
    if(ret !=0)
      printf("error insert\n");
    else
      printf("Done insert product.\n");
    ret = get_product_by_name(&my_dom,&p2);
    
    if(ret == DB_NOTFOUND)
      printf("Key not find\n");
    else
      printf("Done get product.\n");
    print_product(&p2);  

    CPT c1,c2;
    memset(&c1,0,sizeof(CPT));
    memset(&c2,0,sizeof(CPT));
    c1.id = 100;
    
    memcpy(c1.data.name,"song",5);
    memcpy(c2.data.name,"song",5);
    c1.data.subcpt_num = 5;
    if(c1.data.subcpt_num!=0)
      c1.data.subcpt = malloc(sizeof(SUBCPT_DATA) * c1.data.subcpt_num);
    for(i=0;i<c1.data.subcpt_num;++i){
      memset(&(c1.data.subcpt[i]),0,sizeof(SUBCPT_DATA));
      c1.data.subcpt[i].num = i;
      c1.data.subcpt[i].id =i*22;
    }

    ret = insert_cpt(&my_dom,&c1);
    //secure_remove_cpt(&my_dom,100);
    if(ret ==REDEFINE_CPT)
      printf("error insert\n");
    else
      printf("Done insert cpt.\n");
    ret = get_cpt_by_name(&my_dom,&c2,&count);
    printf("count = %d\n",count);
    if(ret == DB_NOTFOUND)
      printf("Key not find\n");
    else{
      print_cpt(&c2);  
      printf("Done get cpt.\n");
    }

    /* close our environment and databases */
    databases_close(&my_dom);

    printf("Done.\n");
    return (ret);
}


/*
 * Used to extract an product's name from an
 * product database record. This function is used to create
 * keys for secondary database records.
 */
int
get_product_name(DB *dbp, const DBT *pkey, const DBT *pdata, DBT *skey)
{
    PRODUCT_DATA * product;
    
    product = pdata->data;
    memset(skey, 0, sizeof(DBT));
    skey->data = product->name;
    skey->size = (u_int32_t)strlen(skey->data) + 1;

    return (0);
}

/*
 * Used to extract an cpt's name from an
 * cpt database record. This function is used to create
 * keys for secondary database records.
 */
int
get_cpt_name(DB *dbp, const DBT *pkey, const DBT *pdata, DBT *skey)
{
    u_int offset;

    dbp = NULL;
    pkey = NULL;

    offset = sizeof(unsigned int) + sizeof(unsigned int) ;
    if (pdata->size < offset)
      return -1;

    memset(skey,0,sizeof(DBT));
    skey->data = (u_int8_t *) pdata->data + offset;
    skey->size = (u_int32_t)strlen(skey->data) + 1;

    return (0);
}


/* Opens a database */
int
open_database(DB **dbpp, const char *file_name,
  const char *program_name, FILE *error_file_pointer,
  int is_secondary)
{
    DB *dbp;    /* For convenience */
    u_int32_t open_flags;
    int ret;

    /* Initialize the DB handle */
    ret = db_create(&dbp, NULL, 0);
    if (ret != 0) {
	fprintf(error_file_pointer, "%s: %s\n", program_name,
		db_strerror(ret));
	return (ret);
    }
    /* Point to the memory malloc'd by db_create() */
    *dbpp = dbp;

    /* Set up error handling for this database */
    dbp->set_errfile(dbp, error_file_pointer);
    dbp->set_errpfx(dbp, program_name);

    /*
     * If this is a secondary database, then we want to allow
     * sorted duplicates.
     */
    if (is_secondary) {
	ret = dbp->set_flags(dbp, DB_DUPSORT);
	if (ret != 0) {
	    dbp->err(dbp, ret, "Attempt to set DUPSORT flags failed.",
	      file_name);
	    return (ret);
	}
    }

    /* Set the open flags */
    open_flags = DB_CREATE;    /* Allow database creation */

    /* Now open the database */
    ret = dbp->open(dbp,        /* Pointer to the database */
		    NULL,       /* Txn pointer */
		    file_name,  /* File name */
		    NULL,       /* Logical db name */
		    DB_BTREE,   /* Database type (using btree) */
		    open_flags, /* Open flags */
		    0);         /* File mode. Using defaults */
    if (ret != 0) {
	dbp->err(dbp, ret, "Database '%s' open failed.", file_name);
	return (ret);
    }

    return (0);
}

/* opens all databases */
int
databases_setup(DOM_DBS *my_dom, const char *program_name,
  FILE *error_file_pointer)
{
    int ret;

    /* Open the product database */
    ret = open_database(&(my_dom->product_dbp),
      my_dom->product_db_name,
      program_name, error_file_pointer,
      PRIMARY_DB);
    if (ret != 0)
	/*
	 * Error reporting is handled in open_database() so just return
	 * the return code.
	 */
	return (ret);

    /* Open the cpt database */
    ret = open_database(&(my_dom->cpt_dbp),
      my_dom->cpt_db_name,
      program_name, error_file_pointer,
      PRIMARY_DB);
    if (ret != 0)
	/*
	 * Error reporting is handled in open_database() so just return
	 * the return code.
	 */
	return (ret);

    /*
     * Open the product_name secondary database. This is used to
     * index the product names found in the product
     * database.
     */
    ret = open_database(&(my_dom->product_name_sdbp),
      my_dom->product_name_db_name,
      program_name, error_file_pointer,
      SECONDARY_DB);
    if (ret != 0)
	/*
	 * Error reporting is handled in open_database() so just return
	 * the return code.
	 */
	return (ret);

    /*
     * Open the cpt_name secondary database. This is used to
     * index the cpt names found in the cpt
     * database.
     */
  	ret = open_database(&(my_dom->cpt_name_sdbp),
      my_dom->cpt_name_db_name,
      program_name, error_file_pointer,
      SECONDARY_DB);
    if (ret != 0)
	/*
	 * Error reporting is handled in open_database() so just return
	 * the return code.
	 */
	return (ret);

    /*
     * Associate the product_name db with its primary db
     * (product db).
     */
     my_dom->product_dbp->associate(
       my_dom->product_dbp,    /* Primary db */
       NULL,                       /* txn id */
       my_dom->product_name_sdbp,    /* Secondary db */
       get_product_name,              /* Secondary key creator */
       0);                         /* Flags */

    /*
     * Associate the cpt_name db with its primary db
     * (cpt db).
     */
     my_dom->cpt_dbp->associate(
       my_dom->cpt_dbp,    /* Primary db */
       NULL,                       /* txn id */
       my_dom->cpt_name_sdbp,    /* Secondary db */
       get_cpt_name,              /* Secondary key creator */
       0);                         /* Flags */

    printf("databases opened successfully\n");
    return (0);
}

/* Closes all the databases and secondary databases. */
int
databases_close(DOM_DBS *my_dom)
{
    int ret;
    /*
     * Note that closing a database automatically flushes its cached data
     * to disk, so no sync is required here.
     */
    if (my_dom->product_name_sdbp != NULL) {
	ret = my_dom->product_name_sdbp->close(my_dom->product_name_sdbp, 0);
	if (ret != 0)
	    fprintf(stderr, "product name database close failed: %s\n",
	      db_strerror(ret));
    }

    if (my_dom->product_dbp != NULL) {
	ret = my_dom->product_dbp->close(my_dom->product_dbp, 0);
	if (ret != 0)
	    fprintf(stderr, "product database close failed: %s\n",
	      db_strerror(ret));
    }

     if (my_dom->cpt_name_sdbp != NULL) {
	ret = my_dom->cpt_name_sdbp->close(my_dom->cpt_name_sdbp, 0);
	if (ret != 0)
	    fprintf(stderr, "cpt name database close failed: %s\n",
	      db_strerror(ret));
    }


    if (my_dom->cpt_dbp != NULL) {
	ret = my_dom->cpt_dbp->close(my_dom->cpt_dbp, 0);
	if (ret != 0)
	    fprintf(stderr, "cpt database close failed: %s\n",
	      db_strerror(ret));
    }

    printf("databases closed.\n");
    return (0);
}

/* Identify all the files that will hold our databases. */
void
set_db_filenames(DOM_DBS *my_dom)
{
    size_t size;

    /* Create the product DB file name */
    size = strlen(my_dom->db_home_dir) + strlen(PRODUCTDB) + 1;
    my_dom->product_db_name = malloc(size);
    snprintf(my_dom->product_db_name, size, "%s%s",
      my_dom->db_home_dir, PRODUCTDB);

    /* Create the cpt DB file name */
    size = strlen(my_dom->db_home_dir) + strlen(CPTDB) + 1;
    my_dom->cpt_db_name = malloc(size);
    snprintf(my_dom->cpt_db_name, size, "%s%s",
      my_dom->db_home_dir, CPTDB);

    /* Create the product name DB file name */
    size = strlen(my_dom->db_home_dir) + strlen(PRODUCTNAMEDB) + 1;
    my_dom->product_name_db_name = malloc(size);
    snprintf(my_dom->product_name_db_name, size, "%s%s",
      my_dom->db_home_dir, PRODUCTNAMEDB);

    /* Create the cpt name DB file name */
    size = strlen(my_dom->db_home_dir) + strlen(CPTNAMEDB) + 1;
    my_dom->cpt_name_db_name = malloc(size);
    snprintf(my_dom->cpt_name_db_name, size, "%s%s",
      my_dom->db_home_dir, CPTNAMEDB);

}

/* Initializes the DOM_DBS struct.*/
void
initialize_stockdbs(DOM_DBS *my_dom)
{
    my_dom->db_home_dir = DEFAULT_HOMEDIR;
    my_dom->product_dbp = NULL;
    my_dom->cpt_dbp = NULL;
    my_dom->product_name_sdbp = NULL;
    my_dom->cpt_name_sdbp = NULL;
    
    my_dom->product_db_name = NULL;
    my_dom->cpt_db_name = NULL;
    my_dom->product_name_db_name = NULL;
	  my_dom->product_name_db_name = NULL;
}

/*
 * Simple little convenience function that takes a buffer, a string,
 * and an offset and copies that string into the buffer at the
 * appropriate location. Used to ensure that all our strings
 * are contained in a single contiguous chunk of memory.
 */
size_t
pack_string(char *buffer, char *string, size_t start_pos)
{
    size_t string_size;

    string_size = strlen(string) + 1;
    memcpy(buffer+start_pos, string, string_size);

    return (start_pos + string_size);
}

int 
insert_products(DOM_DBS *my_dom, PRODUCT *p)
{
  CPT tmp;
  int i,count=0;
  for(i=0;i<PRODUCT_CPT_NUM;i++){
    memset(&tmp,0,sizeof(CPT));
    tmp.id = p->data.subcpt[i].id;
    if(get_cpt_by_id(my_dom,&tmp,&count) == DB_NOTFOUND)
      _add_unknown_cpt(my_dom,p->data.subcpt[i].id,1);
    else{
      _modify_reference(my_dom,p->data.subcpt[i].id,ADD_REF);
      if(tmp.data.subcpt!=NULL)
        free(tmp.data.subcpt);
    }
  }
  return _insert_products(my_dom,p);
}
int 
_modify_reference(DOM_DBS *my_dom, KEY id,unsigned int flags){
  CPT tmp;
  int count,ret = 0;
  tmp.id = id;
  get_cpt_by_id(my_dom,&tmp,&count);

  switch(flags){
    case RED_REF:
      _remove_cpt(my_dom,id);
      if(count != 1)
        _insert_cpt(my_dom,&tmp,count-1);
      break;
    case ADD_REF:
      _remove_cpt(my_dom,id);
      _insert_cpt(my_dom,&tmp,count+1);
      break;
    default:
      ret = -1;
  } 
  if(tmp.data.subcpt !=NULL)
    free(tmp.data.subcpt);
  return ret;
}

int
_remove_product(DOM_DBS *my_dom, KEY id){
  DBT key; 
  memset(&key, 0, sizeof(DBT));
  key.data = &id; 
  key.size = sizeof(KEY);
  return my_dom->product_dbp->del(my_dom->product_dbp, NULL, &key, 0);
}

int
_remove_cpt(DOM_DBS *my_dom, KEY id){
  DBT key; 
  memset(&key, 0, sizeof(DBT));
  key.data = &id; 
  key.size = sizeof(KEY);
  return my_dom->cpt_dbp->del(my_dom->cpt_dbp, NULL, &key, 0);
}

int 
insert_cpt(DOM_DBS *my_dom,  CPT *c)
{
  CPT tmp;
  int i,count = 0;
  size_t ret;
  memset(&tmp,0,sizeof(CPT));
  tmp.id = c->id;
  ret = get_cpt_by_id(my_dom,&tmp,&count);
  if(ret ==DB_NOTFOUND){
    _insert_cpt(my_dom,c,0);
  }
  else{
    if(strcmp(tmp.data.name,"unknown")==0){
      _remove_cpt(my_dom,tmp.id);
      _insert_cpt(my_dom,c,count);
    }else
      return REDEFINE_CPT;
  }

  for(i=0;i<c->data.subcpt_num;i++){
    memset(&tmp,0,sizeof(CPT));
    tmp.id = c->data.subcpt[i].id;
    if(get_cpt_by_id(my_dom,&tmp,&count) == DB_NOTFOUND)
      _add_unknown_cpt(my_dom,c->data.subcpt[i].id,1);
    else{
      _modify_reference(my_dom,tmp.id,ADD_REF);
      if(tmp.data.subcpt!=NULL)
        free(tmp.data.subcpt);
    }
  }
  return 0;
}

int 
_insert_products(DOM_DBS *my_dom, PRODUCT *p)
{
  int ret;
  DBT key, data;
  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));
  key.data = &p->id;
  key.size = sizeof(KEY);

  data.data = &p->data;
  data.size = sizeof(PRODUCT_DATA);

  return my_dom->product_dbp->put(my_dom->product_dbp,0,&key,&data,0);
}

int 
_insert_cpt(DOM_DBS *my_dom, CPT *c, unsigned int count)
{
  int ret;
  DBT key,data;
  char dataBuf[MAXDATABUF];
  size_t bufLen, dataLen,i;
  memset(dataBuf, 0, MAXDATABUF);

  bufLen = 0;
  dataLen = 0;

  dataLen = sizeof(unsigned int);
  memcpy(dataBuf, &count, dataLen);
  bufLen += dataLen;

  dataLen = sizeof(unsigned int);
  memcpy(dataBuf+bufLen, &(c->data.subcpt_num), dataLen);
  bufLen += dataLen;

  bufLen = pack_string(dataBuf, c->data.name, bufLen);

  for(i = 0;i < c->data.subcpt_num; i++){
    dataLen = sizeof(SUBCPT_DATA);
    memcpy(dataBuf+bufLen, &(c->data.subcpt[i]), dataLen);
    bufLen += dataLen;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  key.data = &(c->id);
  key.size = sizeof(KEY);

  /* The data is the information that we packed into dataBuf. */
  data.data = dataBuf;
  data.size = (u_int32_t)bufLen;

  /* Put the data into the database */
  return my_dom->cpt_dbp->put(my_dom->cpt_dbp, 0, &key, &data, 0); 
}

int 
get_product_by_id(DOM_DBS *my_dom,PRODUCT *p)
{
  DBT key, data;
  int ret;
  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));
  key.data = &p->id;
  key.size = sizeof(KEY);

  data.data = &(p->data);
  data.ulen = sizeof(PRODUCT_DATA);
  data.flags = DB_DBT_USERMEM;
  ret = my_dom->product_dbp->get(my_dom->product_dbp,NULL,&key,&data,0);
  return ret;
}

int 
get_product_by_name(DOM_DBS *my_dom,PRODUCT *p)
{

  DBT key, pkey,pdata;
  int ret;
  memset(&key, 0, sizeof(DBT)); 
  memset(&pkey, 0, sizeof(DBT)); 
  memset(&pdata, 0, sizeof(DBT));

  key.data = p->data.name;
  key.size = strlen(p->data.name)+1;
  pdata.data = &(p->data);
  pdata.ulen = sizeof(PRODUCT_DATA);
  pdata.flags = DB_DBT_USERMEM;

  ret = my_dom->product_name_sdbp->pget(my_dom->product_name_sdbp,NULL,\
    &key,&pkey,&pdata,0);
  p->id = *(KEY*)pkey.data;
  return ret;
}


int 
get_cpt_by_id(DOM_DBS *my_dom,CPT *c,unsigned int * count)
{
  DBT key, data;
  int ret;
  char dataBuf[MAXDATABUF];
  memset(dataBuf, 0, MAXDATABUF);
  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));
  key.data = &(c->id);
  key.size = sizeof(KEY);
  data.data = dataBuf;
  data.ulen = MAXDATABUF;
  data.flags = DB_DBT_USERMEM;
  ret = my_dom->cpt_dbp->get(my_dom->cpt_dbp,NULL,&key,&data,0);
  if(ret == 0)
    pack_cpt(&(c->data),dataBuf,count);
  return ret;
}

int 
get_cpt_by_name(DOM_DBS *my_dom,CPT *c,unsigned int * count)
{
  DBT key, pkey,pdata;
  int ret;
  char dataBuf[MAXDATABUF];
  memset(dataBuf, 0, MAXDATABUF);
  memset(&key, 0, sizeof(DBT));
  memset(&pdata, 0, sizeof(DBT));
  memset(&pkey, 0, sizeof(DBT));
  key.data = c->data.name;
  key.size = strlen(c->data.name)+1;
  pdata.data = dataBuf;
  pdata.ulen = MAXDATABUF;
  pdata.flags = DB_DBT_USERMEM;
  ret = my_dom->cpt_name_sdbp->pget(my_dom->cpt_name_sdbp,NULL,\
    &key,&pkey,&pdata,0);
  if(ret == 0){
    c->id = *(KEY*)pkey.data;
    pack_cpt(&(c->data),dataBuf,count);
  }
  return ret;
}


void 
pack_cpt(CPT_DATA *c_data,char * dataBuf,unsigned int *count)
{
  size_t buf_pos;
  int i;
  *count = *(unsigned int *)(dataBuf);
  buf_pos = sizeof(unsigned int);

  c_data->subcpt_num = *((unsigned int *)(dataBuf+buf_pos));
  buf_pos += sizeof(unsigned int);
  memcpy(c_data->name,dataBuf + buf_pos,strlen(dataBuf + buf_pos)+1);
  buf_pos +=strlen(c_data->name) + 1;
  c_data->subcpt= malloc(sizeof(SUBCPT_DATA) * c_data->subcpt_num);
  for(i = 0; i < c_data->subcpt_num; i++){
    c_data->subcpt[i] = *((SUBCPT_DATA *)(dataBuf + buf_pos));
    buf_pos += sizeof(SUBCPT_DATA);
  }
}
int 
secure_remove_product(DOM_DBS *my_dom,KEY id)
{
  int i,ret;
  PRODUCT tmp;
  tmp.id = id;
  ret = get_product_by_id(my_dom,&tmp);
  if (ret == DB_NOTFOUND)
    return ret;
  else{
    for(i=0;i<PRODUCT_CPT_NUM;i++)
      _modify_reference(my_dom,tmp.data.subcpt[i].id,RED_REF);
    return _remove_product(my_dom,id);
  }
}

int 
secure_remove_cpt(DOM_DBS *my_dom,KEY id)
{
  int i,count,ret;
  CPT tmp;
  tmp.id = id;
  ret = get_cpt_by_id(my_dom,&tmp,&count);
  if (ret == DB_NOTFOUND)
    return ret;
  else{
    for(i=0;i<tmp.data.subcpt_num;i++){
      _modify_reference(my_dom,tmp.data.subcpt[i].id,RED_REF);
    }
    if (count != 0)
      _add_unknown_cpt(my_dom,id,count);
    if(tmp.data.subcpt!=NULL)
      free(tmp.data.subcpt);
    return _remove_cpt(my_dom,id);
  }
}

void 
_add_unknown_cpt(DOM_DBS *my_dom, KEY id,unsigned int count){
  CPT tmp;
  tmp.id = id;
  strcpy(tmp.data.name,"unknown");
  _insert_cpt(my_dom,&tmp,count);
}


void 
print_product(PRODUCT *p)
{
  printf("=============================\n");
  PRODUCT_DATA *p_data = &(p->data);
  printf("p id:%d\n",p->id);
  printf("pname:%s\nid:%d\tnum:%d\nid:%d\tnum:%d\nid:%d\tnum:%d\nid:%d\tnum:%d\n",\
    p_data->name,\
    p_data->subcpt[0].id,\
    p_data->subcpt[0].num,\
    p_data->subcpt[1].id,\
    p_data->subcpt[1].num,\
    p_data->subcpt[2].id,\
    p_data->subcpt[2].num,\
    p_data->subcpt[3].id,\
    p_data->subcpt[3].num);
  printf("=============================\n");
}

void 
print_cpt(CPT * c)
{
  printf("=============================\n");
  CPT_DATA *c_data = &(c->data);
  printf("c id:%d\n",c->id);
  int i;
  printf("cptname:%s, has %d subcpt\n",c_data->name,c_data->subcpt_num);
  for(i = 0; i < c_data->subcpt_num; i++){
    printf("num:%d\tid:%d\n",c_data->subcpt[i].num,c_data->subcpt[i].id);
  }
  printf("=============================\n");
}
