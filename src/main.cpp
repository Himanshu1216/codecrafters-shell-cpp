#include <iostream>
#include <string>
#include <vector>
#include <unistd.h> // chdir, 
#include <sys/wait.h> // for waitpid
#include <filesystem> // to get current working directory

using namespace std;

bool checkCommand(const std::string& cmd) {
  if(cmd == "type" || cmd == "echo" || cmd == "exit" || cmd == "pwd" || cmd == "cd") {
    std::cout << cmd << " is a shell builtin\n";
    return true;
  }

  char* pathenv = getenv("PATH");
  if(!pathenv) {
    return false;
  }

  std::string path(pathenv);
  std::string currpath = "";

  for(int i = 0; i < path.size(); i++) {
    if(path[i] == ':') {
      std::string fullpath = currpath + '/' + cmd;
      if(access(fullpath.c_str(), X_OK) == 0) {
        std::cout << cmd << " is " << fullpath << '\n';
        return true;
      }
      currpath = "";
    }
    else currpath += path[i];
  }

  return false;
}

vector<string> tokenize(string& input) {
    vector<string> user_input;
    string word = "";
    for(int i = 0; i < input.size(); i++) {
      if(input[i] == ' ') {
        user_input.push_back(word);
        word = "";
      }
      else word += input[i];
    }
    user_input.push_back(word);
    return user_input;
}

void run_external(vector<string>& user_input) {
    vector<char*> input;
    for(string& s : user_input) {
        input.push_back(const_cast<char*>(s.c_str()));
    }
    input.push_back(nullptr);
    pid_t pid = fork();
    if(pid == 0) {
        // child process
        execvp(input[0], input.data());
        // perror("execvp");
        exit(127);
    }
    else if(pid > 0) {
        // parent process
        int status;
        waitpid(pid, &status, 0);
        if(WIFEXITED(status) && WEXITSTATUS(status) == 127) {
            cout << input[0] << ": command not found\n";
        }
    }
    else {
        perror("fork");
    }
    // cout << "world\n";
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  std::vector<std::string> user_input;
  std::string input;
  
  while(true) {
    user_input.clear();
    std::cout << "$ ";
    std::getline(std::cin, input);

    vector<string> user_input = tokenize(input);

    std::string cmd1 = user_input[0];

    if(cmd1 == "type") {
      for(int i = 1; i < user_input.size(); i++) {
        if(!checkCommand(user_input[i])) {
          std::cout << user_input[i] << ": not found\n";
        }
      }      
    }
    else if(cmd1 == "exit") {
      return 0;
    }
    else if(cmd1 == "echo") {
      for(int i = 1; i < user_input.size(); i++) {
        std::cout << user_input[i] << ' ';
      }
      std::cout << '\n';
    }
    else if(cmd1 == "pwd") {
        cout << filesystem::current_path().string() << endl; // without .string() path is output with "" to safely represents paths with spaces.
        // string pwd = filesystem::current_path();
        // cout << pwd << endl;
    }
    else if(cmd1 == "cd") {
        if(chdir(user_input[1].c_str()) != 0) {
            cout << "cd: " << user_input[1] << ": No such file or directory\n";
        }
    }
    else run_external(user_input);
  }
}
