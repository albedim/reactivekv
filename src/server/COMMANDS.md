gcc -pthread kvdb_client.c -o server      # Compile command for single file
netstat -ano | findstr :6379              # See all processes working on port 6379
taskkill /PID <PID> /F                    # Destroy process

cd src/server
make        # Compiles all files into 'server' executable
make run    # Compiles and runs the server
make clean  # Removes build artifacts