rm mydb_manager.o dbtest.o dbtest *.db
gcc -g -c mydb_manager.c -o mydb_manager.o -ldb
gcc -g -c dbtest.c -o dbtest.o 
gcc -g dbtest.o mydb_manager.o -o dbtest -ldb
./dbtest
