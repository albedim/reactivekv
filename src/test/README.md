gcc -pthread kvdb_client.c -o server
netstat -ano | findstr :6379
taskkill /PID <PID> /F