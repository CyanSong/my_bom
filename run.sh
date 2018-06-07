rm mydb_manager.o bom_manager.o bom_manager *.db
gcc -g -c mydb_manager.c -o mydb_manager.o -ldb
gcc -g -c bom_manager.c -o bom_manager.o 
gcc -g bom_manager.o mydb_manager.o -o bom_manager -ldb
gdb bom_manager
#./bom_manager
