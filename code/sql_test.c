#include <stdio.h>
#include <string.h>
#include "sqlite3.h"

int print_record(void *params,int n_column,char **column_value,char **column_name)
{
    int i;
    
    for(i=0;i<n_column;i++){
        printf("\t%s",column_value[i]);
    }
    
    printf("\n");
    
    return 0;
}

int main(int argc,char *argv[])
{
    const char *sql_create_table="create table t(id int primary key,msg TINYTEXT)";
    char *errmsg = 0;
    int ret = 0;
    int i;
    char sql_cmd_buf[256];

    sqlite3 *db = 0;
    ret = sqlite3_open("./sqlite3-demo.db",&db);
    if(ret != SQLITE_OK){
        fprintf(stderr,"Cannot open db: %s\n",sqlite3_errmsg(db));
        return 1;
    }
    printf("Open database\n");

    ret = sqlite3_exec(db,sql_create_table,NULL,NULL,&errmsg);
    if(ret != SQLITE_OK){
        fprintf(stderr,"create table fail: %s\n",errmsg);
    }

	for (i = 0; i < 100; i++)
	{
		memset(sql_cmd_buf, 9, 256);
		sprintf(sql_cmd_buf, "insert into t (id , msg) values(%d,\'name%d\')", i, i);
		printf(sql_cmd_buf);
		printf("\n");
		ret = sqlite3_exec(db,sql_cmd_buf,NULL,NULL,&errmsg);
		if(ret != SQLITE_OK)
		{
			fprintf(stderr,"insert into t fail: %s\n",errmsg);
		}  
	}
    
    ret = sqlite3_exec(db,"select * from t",print_record,NULL,&errmsg);
    if(ret != SQLITE_OK){
        fprintf(stderr,"select * from t fail: %s\n",errmsg);
    }  
    
    sqlite3_free(errmsg);
    sqlite3_close(db);

    printf("Close database\n");

    return 0;
}

