
#include "mydb_manager.h"

#define ADD_REF 1
#define RED_REF 2
int _insert_product(BOM_DBS *,PRODUCT *);
int _insert_cpt(BOM_DBS *,CPT *,unsigned int);
int _remove_cpt(BOM_DBS *my_bom, KEY id);
int _remove_product(BOM_DBS *my_bom, KEY id);
int _modify_reference(BOM_DBS *my_bom, KEY id,unsigned int flags);
void pack_cpt(CPT_DATA *c_data,char * dataBuf,unsigned int *count);
void _add_unknown_cpt(BOM_DBS *my_bom, KEY id,unsigned int count);
int get_product_name(DB *, const DBT *, const DBT *, DBT *);
int get_cpt_name(DB *, const DBT *, const DBT *, DBT *);
void _print_cpt(BOM_DBS * my_bom, CPT * c);
void  pack_product(PRODUCT_DATA *p_data,char * dataBuf);


/*
 * Used to extract an product's name from an
 * product database record. This function is used to create
 * keys for secondary database records.
 */
int
get_product_name(DB *dbp, const DBT *pkey, const DBT *pdata, DBT *skey)
{
    u_int offset;

    dbp = NULL;
    pkey = NULL;

    offset = sizeof(unsigned int);
    if (pdata->size < offset)
      return -1;

    memset(skey,0,sizeof(DBT));
    skey->data = (u_int8_t *) pdata->data + offset;
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
databases_setup(BOM_DBS *my_bom, const char *program_name,
  FILE *error_file_pointer)
{
    int ret;

    /* Open the product database */
    ret = open_database(&(my_bom->product_dbp),
      my_bom->product_db_name,
      program_name, error_file_pointer,
      PRIMARY_DB);
    if (ret != 0)
	/*
	 * Error reporting is handled in open_database() so just return
	 * the return code.
	 */
	return (ret);

    /* Open the cpt database */
    ret = open_database(&(my_bom->cpt_dbp),
      my_bom->cpt_db_name,
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
    ret = open_database(&(my_bom->product_name_sdbp),
      my_bom->product_name_db_name,
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
  	ret = open_database(&(my_bom->cpt_name_sdbp),
      my_bom->cpt_name_db_name,
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
     my_bom->product_dbp->associate(
       my_bom->product_dbp,    /* Primary db */
       NULL,                       /* txn id */
       my_bom->product_name_sdbp,    /* Secondary db */
       get_product_name,              /* Secondary key creator */
       0);                         /* Flags */

    /*
     * Associate the cpt_name db with its primary db
     * (cpt db).
     */
     my_bom->cpt_dbp->associate(
       my_bom->cpt_dbp,    /* Primary db */
       NULL,                       /* txn id */
       my_bom->cpt_name_sdbp,    /* Secondary db */
       get_cpt_name,              /* Secondary key creator */
       0);                         /* Flags */

    printf("databases opened successfully\n");
    return (0);
}

/* Closes all the databases and secondary databases. */
int
databases_close(BOM_DBS *my_bom)
{
    int ret;
    /*
     * Note that closing a database automatically flushes its cached data
     * to disk, so no sync is required here.
     */
    if (my_bom->product_name_sdbp != NULL) {
	ret = my_bom->product_name_sdbp->close(my_bom->product_name_sdbp, 0);
	if (ret != 0)
	    fprintf(stderr, "product name database close failed: %s\n",
	      db_strerror(ret));
    }

    if (my_bom->product_dbp != NULL) {
	ret = my_bom->product_dbp->close(my_bom->product_dbp, 0);
	if (ret != 0)
	    fprintf(stderr, "product database close failed: %s\n",
	      db_strerror(ret));
    }

     if (my_bom->cpt_name_sdbp != NULL) {
	ret = my_bom->cpt_name_sdbp->close(my_bom->cpt_name_sdbp, 0);
	if (ret != 0)
	    fprintf(stderr, "cpt name database close failed: %s\n",
	      db_strerror(ret));
    }


    if (my_bom->cpt_dbp != NULL) {
	ret = my_bom->cpt_dbp->close(my_bom->cpt_dbp, 0);
	if (ret != 0)
	    fprintf(stderr, "cpt database close failed: %s\n",
	      db_strerror(ret));
    }

    printf("databases closed.\n");
    return (0);
}

/* Identify all the files that will hold our databases. */
void
set_db_filenames(BOM_DBS *my_bom)
{
    size_t size;

    /* Create the product DB file name */
    size = strlen(my_bom->db_home_dir) + strlen(PRODUCTDB) + 1;
    my_bom->product_db_name = malloc(size);
    snprintf(my_bom->product_db_name, size, "%s%s",
      my_bom->db_home_dir, PRODUCTDB);

    /* Create the cpt DB file name */
    size = strlen(my_bom->db_home_dir) + strlen(CPTDB) + 1;
    my_bom->cpt_db_name = malloc(size);
    snprintf(my_bom->cpt_db_name, size, "%s%s",
      my_bom->db_home_dir, CPTDB);

    /* Create the product name DB file name */
    size = strlen(my_bom->db_home_dir) + strlen(PRODUCTNAMEDB) + 1;
    my_bom->product_name_db_name = malloc(size);
    snprintf(my_bom->product_name_db_name, size, "%s%s",
      my_bom->db_home_dir, PRODUCTNAMEDB);

    /* Create the cpt name DB file name */
    size = strlen(my_bom->db_home_dir) + strlen(CPTNAMEDB) + 1;
    my_bom->cpt_name_db_name = malloc(size);
    snprintf(my_bom->cpt_name_db_name, size, "%s%s",
      my_bom->db_home_dir, CPTNAMEDB);

}

/* Initializes the BOM_DBS struct.*/
void
initialize_stockdbs(BOM_DBS *my_bom)
{
    my_bom->db_home_dir = DEFAULT_HOMEDIR;
    my_bom->product_dbp = NULL;
    my_bom->cpt_dbp = NULL;
    my_bom->product_name_sdbp = NULL;
    my_bom->cpt_name_sdbp = NULL;
    
    my_bom->product_db_name = NULL;
    my_bom->cpt_db_name = NULL;
    my_bom->product_name_db_name = NULL;
	  my_bom->product_name_db_name = NULL;
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
insert_product(BOM_DBS *my_bom, PRODUCT *p)
{
  CPT tmp;
  int i,count=0;
  for(i=0;i<p->data.subcpt_num;i++){
    memset(&tmp,0,sizeof(CPT));
    tmp.id = p->data.subcpt[i].id;
    if(get_cpt_by_id(my_bom,&tmp,&count) == DB_NOTFOUND)
      _add_unknown_cpt(my_bom,p->data.subcpt[i].id,1);
    else{
      _modify_reference(my_bom,p->data.subcpt[i].id,ADD_REF);
      if(tmp.data.subcpt!=NULL)
        free(tmp.data.subcpt);
    }
  }
  return _insert_product(my_bom,p);
}
int 
_modify_reference(BOM_DBS *my_bom, KEY id,unsigned int flags){
  CPT tmp;
  int count,ret = 0;
  tmp.id = id;
  get_cpt_by_id(my_bom,&tmp,&count);

  switch(flags){
    case RED_REF:
      _remove_cpt(my_bom,id);
      if(count != 1)
        _insert_cpt(my_bom,&tmp,count-1);
      break;
    case ADD_REF:
      _remove_cpt(my_bom,id);
      _insert_cpt(my_bom,&tmp,count+1);
      break;
    default:
      ret = -1;
  } 
  if(tmp.data.subcpt !=NULL)
    free(tmp.data.subcpt);
  return ret;
}

int
_remove_product(BOM_DBS *my_bom, KEY id){
  DBT key; 
  memset(&key, 0, sizeof(DBT));
  key.data = &id; 
  key.size = sizeof(KEY);
  return my_bom->product_dbp->del(my_bom->product_dbp, NULL, &key, 0);
}

int
_remove_cpt(BOM_DBS *my_bom, KEY id){
  DBT key; 
  memset(&key, 0, sizeof(DBT));
  key.data = &id; 
  key.size = sizeof(KEY);
  return my_bom->cpt_dbp->del(my_bom->cpt_dbp, NULL, &key, 0);
}

int 
insert_cpt(BOM_DBS *my_bom,  CPT *c)
{
  CPT tmp;
  int i,count = 0;
  size_t ret;
  memset(&tmp,0,sizeof(CPT));
  tmp.id = c->id;
  ret = get_cpt_by_id(my_bom,&tmp,&count);
  if(ret ==DB_NOTFOUND){
    _insert_cpt(my_bom,c,0);
  }
  else{
    if(strcmp(tmp.data.name,"unknown")==0){
      _remove_cpt(my_bom,tmp.id);
      _insert_cpt(my_bom,c,count);
    }else
      return REDEFINE_CPT;
  }

  for(i=0;i<c->data.subcpt_num;i++){
    memset(&tmp,0,sizeof(CPT));
    tmp.id = c->data.subcpt[i].id;
    if(get_cpt_by_id(my_bom,&tmp,&count) == DB_NOTFOUND)
      _add_unknown_cpt(my_bom,c->data.subcpt[i].id,1);
    else{
      _modify_reference(my_bom,tmp.id,ADD_REF);
      if(tmp.data.subcpt!=NULL)
        free(tmp.data.subcpt);
    }
  }
  return 0;
}

int 
_insert_product(BOM_DBS *my_bom, PRODUCT *p)
{
  int ret;
  DBT key,data;
  char dataBuf[MAXDATABUF];
  size_t bufLen, dataLen,i;
  memset(dataBuf, 0, MAXDATABUF);

  bufLen = 0;
  dataLen = 0;

  dataLen = sizeof(unsigned int);
  memcpy(dataBuf+bufLen, &(p->data.subcpt_num), dataLen);
  bufLen += dataLen;

  bufLen = pack_string(dataBuf, p->data.name, bufLen);

  for(i = 0;i < p->data.subcpt_num; i++){
    dataLen = sizeof(SUBCPT_DATA);
    memcpy(dataBuf+bufLen, &(p->data.subcpt[i]), dataLen);
    bufLen += dataLen;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  key.data = &(p->id);
  key.size = sizeof(KEY);

  /* The data is the information that we packed into dataBuf. */
  data.data = dataBuf;
  data.size = (u_int32_t)bufLen;

  /* Put the data into the database */
  return my_bom->product_dbp->put(my_bom->product_dbp, 0, &key, &data, 0); 
}

int 
_insert_cpt(BOM_DBS *my_bom, CPT *c, unsigned int count)
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
  bufLen = pack_string(dataBuf, c->data.model, bufLen);

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
  return my_bom->cpt_dbp->put(my_bom->cpt_dbp, 0, &key, &data, 0); 
}

int 
get_product_by_id(BOM_DBS *my_bom,PRODUCT *p)
{

  DBT key, data;
  int ret;
  char dataBuf[MAXDATABUF];
  memset(dataBuf, 0, MAXDATABUF);
  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));
  key.data = &(p->id);
  key.size = sizeof(KEY);
  data.data = dataBuf;
  data.ulen = MAXDATABUF;
  data.flags = DB_DBT_USERMEM;
  ret = my_bom->product_dbp->get(my_bom->product_dbp,NULL,&key,&data,0);
  if(ret == 0)
    pack_product(&(p->data),dataBuf);
  return ret;
}

int 
get_product_by_name(BOM_DBS *my_bom,PRODUCT *p)
{
  DBT key, pkey,pdata;
  int ret;
  char dataBuf[MAXDATABUF];
  memset(dataBuf, 0, MAXDATABUF);
  memset(&key, 0, sizeof(DBT));
  memset(&pdata, 0, sizeof(DBT));
  memset(&pkey, 0, sizeof(DBT));
  key.data = p->data.name;
  key.size = strlen(p->data.name)+1;
  pdata.data = dataBuf;
  pdata.ulen = MAXDATABUF;
  pdata.flags = DB_DBT_USERMEM;
  ret = my_bom->product_name_sdbp->pget(my_bom->product_name_sdbp,NULL,\
    &key,&pkey,&pdata,0);
  if(ret == 0){
    p->id = *(KEY*)pkey.data;
    pack_product(&(p->data),dataBuf);
  }
  return ret;
}


int 
get_cpt_by_id(BOM_DBS *my_bom,CPT *c,unsigned int * count)
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
  ret = my_bom->cpt_dbp->get(my_bom->cpt_dbp,NULL,&key,&data,0);
  if(ret == 0)
    pack_cpt(&(c->data),dataBuf,count);
  return ret;
}

int 
get_cpt_by_name(BOM_DBS *my_bom,CPT *c,unsigned int * count)
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
  ret = my_bom->cpt_name_sdbp->pget(my_bom->cpt_name_sdbp,NULL,\
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
  memcpy(c_data->model,dataBuf + buf_pos,strlen(dataBuf + buf_pos)+1);
  buf_pos +=strlen(c_data->model) + 1;  
  c_data->subcpt= calloc( c_data->subcpt_num,sizeof(SUBCPT_DATA) );
  for(i = 0; i < c_data->subcpt_num; i++){
    c_data->subcpt[i] = *((SUBCPT_DATA *)(dataBuf + buf_pos));
    buf_pos += sizeof(SUBCPT_DATA);
  }
}

void 
pack_product(PRODUCT_DATA *p_data,char * dataBuf)
{
  size_t buf_pos;
  int i;
  p_data->subcpt_num = *((unsigned int *)(dataBuf));
  buf_pos = sizeof(unsigned int);
  memcpy(p_data->name,dataBuf + buf_pos,strlen(dataBuf + buf_pos)+1);
  buf_pos +=strlen(p_data->name) + 1; 
  p_data->subcpt= calloc( p_data->subcpt_num,sizeof(SUBCPT_DATA));
  for(i = 0; i < p_data->subcpt_num; i++){
    p_data->subcpt[i] = *((SUBCPT_DATA *)(dataBuf + buf_pos));
    buf_pos += sizeof(SUBCPT_DATA);
  }
}


int 
secure_remove_product(BOM_DBS *my_bom,KEY id)
{
  int i,ret;
  PRODUCT tmp;
  tmp.id = id;
  ret = get_product_by_id(my_bom,&tmp);
  if (ret == DB_NOTFOUND)
    return ret;
  else{
    for(i=0;i<tmp.data.subcpt_num;i++)
      _modify_reference(my_bom,tmp.data.subcpt[i].id,RED_REF);
    return _remove_product(my_bom,id);
  }
}

int 
secure_remove_cpt(BOM_DBS *my_bom,KEY id)
{
  int i,count,ret;
  CPT tmp;
  tmp.id = id;
  ret = get_cpt_by_id(my_bom,&tmp,&count);
  if (ret == DB_NOTFOUND)
    return ret;
  else{
    for(i=0;i<tmp.data.subcpt_num;i++){
      _modify_reference(my_bom,tmp.data.subcpt[i].id,RED_REF);
    }
    if (count != 0)
      _add_unknown_cpt(my_bom,id,count);
    if(tmp.data.subcpt!=NULL)
      free(tmp.data.subcpt);
    return _remove_cpt(my_bom,id);
  }
}

void 
_add_unknown_cpt(BOM_DBS *my_bom, KEY id,unsigned int count){
  CPT tmp;
  tmp.id = id;
  strcpy(tmp.data.name,"unknown");
  _insert_cpt(my_bom,&tmp,count);
}


void 
print_product_raw(PRODUCT *p)
{
  printf("=============================\n");
  PRODUCT_DATA *p_data = &(p->data);
  printf("p id:%d\n",p->id);
  int i;
  printf("product name:%s, has %d subcomponants\n",p_data->name,p_data->subcpt_num);
  for(i = 0; i < p_data->subcpt_num; i++){
    printf("num:%d\tid:%d\n",p_data->subcpt[i].num,p_data->subcpt[i].id);
  }
  printf("=============================\n");
}

void 
print_cpt_raw(CPT * c)
{
  printf("=============================\n");
  CPT_DATA *c_data = &(c->data);
  printf("c id:%d\n",c->id);
  int i;
  printf("Componant name:%s, has %d subcomponants\n",c_data->name,c_data->subcpt_num);
  for(i = 0; i < c_data->subcpt_num; i++){
    printf("num:%d\tid:%d\n",c_data->subcpt[i].num,c_data->subcpt[i].id);
  }
  printf("=============================\n");
}

void 
print_cpt(BOM_DBS * my_bom, CPT * c)
{
  printf("===============================================================\n");
  _print_cpt(my_bom,c);
  printf("===============================================================\n");
}

void 
_print_cpt(BOM_DBS * my_bom, CPT * c)
{
  int i,count;
  CPT tmp;
  printf("Componant id: \"%d\", name: \"%s\", model: \"%s\", has \"%d\" subcomponants\n",\
    c->id,\
    c->data.name,\
    c->data.model,\
    c->data.subcpt_num);
  for(i = 0; i < c->data.subcpt_num; i++)
  {
    printf("\ttotal number: \"%d\", ",c->data.subcpt[i].num);
    memset(&tmp,0,sizeof(CPT));
    tmp.id = c->data.subcpt[i].id;
    get_cpt_by_id(my_bom,&tmp,&count);
    _print_cpt(my_bom,&tmp);
    if(tmp.data.subcpt!=NULL)
      free(tmp.data.subcpt);
  }
}

void 
print_product(BOM_DBS * my_bom, PRODUCT *p)
{
  printf("===============================================================\n");
  int i,count;
  CPT tmp;
  printf("Product id: \"%d\", name: \"%s\", has \"%d\" subcomponants\n",\
    p->id,\
    p->data.name,\
    p->data.subcpt_num);
  for(i = 0; i < p->data.subcpt_num; i++)
  {
    printf("\ttotal number \"%d\", ",p->data.subcpt[i].num);
    memset(&tmp,0,sizeof(CPT));
    tmp.id = p->data.subcpt[i].id;
    get_cpt_by_id(my_bom,&tmp,&count);
    _print_cpt(my_bom,&tmp);
    if(tmp.data.subcpt!=NULL)
      free(tmp.data.subcpt);
  }
  printf("===============================================================\n");
}

int print_products(BOM_DBS * my_bom,unsigned int limit)
{
    DBC *product_cursorp;
    DBT key, data;
    PRODUCT p;
    int exit_value, ret;

    /* Initialize our DBTs. */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    /* Get a cursor to the inventory db */
    my_bom->product_dbp->cursor(my_bom->product_dbp, NULL,
      &product_cursorp, 0);

    /*
     * Iterate over the inventory database, from the first record
     * to the last, displaying each in turn.
     */
    exit_value = 0;
    if( limit==-1)
        limit =999;
    while ((ret = product_cursorp->get(product_cursorp, &key, &data, DB_NEXT)) == 0 && limit>0){
        limit--;
        memset(&p,0,sizeof(PRODUCT));
        if(ret == 0)
          pack_product(&(p.data),data.data);
        p.id = *((KEY *)key.data);
        
        print_product_raw(&p);
        printf("\n");
        if(p.data.subcpt!=NULL)
        {
          free(p.data.subcpt);
          p.data.subcpt=NULL;
        }
        if (ret) {
            exit_value = ret;
            break;
        }
    }
    /* Close the cursor */
    product_cursorp->close(product_cursorp);
    return (exit_value);
}


int print_cpts(BOM_DBS * my_bom,int flag,unsigned int limit)
{
    DBC *cpt_cursorp;
    DBT key, data;
    CPT c;
    int exit_value, ret,count;

    /* Initialize our DBTs. */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    /* Get a cursor to the inventory db */
    my_bom->cpt_dbp->cursor(my_bom->cpt_dbp, NULL,
      &cpt_cursorp, 0);

    /*
     * Iterate over the inventory database, from the first record
     * to the last, displaying each in turn.
     */
    exit_value = 0;
    if( limit==-1)
        limit =999;
    while ((ret = cpt_cursorp->get(cpt_cursorp, &key, &data, DB_NEXT)) == 0 && limit>0){
        limit--;
        memset(&c,0,sizeof(CPT));
        if(ret == 0)
          pack_cpt(&(c.data),data.data,&count);
        c.id = *((KEY *)key.data);
        if(flag==SHADOW_UNKNOWN && strcmp(c.data.name,"unknown")!=0){
          print_cpt_raw(&c);
          printf("\n");
        }
        if(c.data.subcpt!=NULL)
        {
          free(c.data.subcpt);
          c.data.subcpt=NULL;
        }
        if (ret) {
            exit_value = ret;
            break;
        }
    }
    /* Close the cursor */
    cpt_cursorp->close(cpt_cursorp);
    return (exit_value);
}