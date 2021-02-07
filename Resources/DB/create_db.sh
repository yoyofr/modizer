rm database.modizerdb
rm databaseMAIN.modizerdb
sqlite3 databaseMAIN.modizerdb ".read 'create_dbMAIN.sql.txt'"
sqlite3 database.modizerdb ".read 'create_dbUSER.sql.txt'"