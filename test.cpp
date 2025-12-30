#include <bits/stdc++.h>
#include <unistd.h>

using namespace std;

signed main()
{    
    pid_t pid = fork();
    if(pid == 0) {
        cout << pid << endl;
        cout << "child process\n";
        exit(0);
    } 
    else if(pid > 0) {
        cout << pid << endl;
        cout << "parent process\n";
        wait(nullptr);
    }
    else {
        perror("fork");
    }
}