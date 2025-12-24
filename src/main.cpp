#include <iostream>
#include <string>
#include <vector>
#include <unistd.h> // chdir, 
#include <sys/wait.h> // for waitpid
#include <filesystem> // to get current working directory
#include <fcntl.h> // open, O_* flags

#include <readline/readline.h>
#include <readline/history.h>

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
    string token = "";
    bool open_single = false, open_double = false;
    input = '*' + input + '*';        
    for(int i = 1; i < input.size() - 1; i++) {
        if(input[i] == ' ') {
            if(open_single || open_double || input[i - 1] == '\\') token += ' ';
            else {
                if(token == "") continue;
                user_input.push_back(token);
                token = "";
            }
        }
        else if(input[i] == '\'') {
            if(open_double || input[i - 1] == '\\') token += '\'';
            else open_single = !open_single;
        }
        else if(open_single) token += input[i];
        else if(input[i] == '\"') {
            if(input[i - 1] == '\\' && (!open_double || i != input.size() - 3)) token += '\"';
            else open_double = !open_double;
        }
        else if(input[i] == '\\') {
            if(open_double) {
                if(input[i + 1] == '\\' || (input[i + 1] == '\"' && i != input.size() - 4)) continue;
                else token += '\\';
            }
            else if(input[i - 1] == '\\' && input[i + 1] != '\\') token += input[i];
        }
        else {
            // cout << input[i] << ' ';
            token += input[i];
        }
    }
    // cout << "token: " << token << endl;
    if(token != "") user_input.push_back(token);
    return user_input;
}

void run_external(vector<string>& input) {
    // check for redirect as well, if redirect then write command output to output file
    vector<char*> args;
    string out_file, err_file;
    bool redirect_out = false, append_out = false;
    bool redirect_err = false, append_err = false;

    for(int i = 0; i < input.size(); i++) {
        if(input[i] == ">" || input[i] == "1>") {
            redirect_out = true;
            out_file = input[++i];
            continue;
        }
        else if(input[i] == ">>" || input[i] == "1>>") {
            append_out = true;
            out_file = input[++i];
            continue;
        }
        else if(input[i] == "2>") {
            redirect_err = true;
            err_file = input[++i];
            continue;
        }
        else if(input[i] == "2>>") {
            append_err = true;
            err_file = input[++i];
            continue;
        }
        args.push_back(const_cast<char*>(input[i].c_str()));
    }
    args.push_back(nullptr);
    pid_t pid = fork();

    // cout << "output_file: " << out_file << endl;
    // cout << input[2] << endl;

    if(pid == 0) {
        // child process

        if(redirect_out || append_out) {
            int flags = O_WRONLY | O_CREAT | (append_out ? O_APPEND : O_TRUNC);
            int fd = open(out_file.c_str(), flags, 0644);
            if(fd < 0) {
                // perror("open");
                exit(1);
            }   
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if(redirect_err || append_err) {
            int flags = O_WRONLY | O_CREAT | (append_err ? O_APPEND : O_TRUNC);
            int fd = open(err_file.c_str(), flags, 0644);
            if(fd < 0) {
                // perror("open");
                exit(1);
            }   
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        execvp(args[0], args.data());
        // perror("execvp");
        exit(127);
    }
    else if(pid > 0) {
        // parent process
        int status;
        waitpid(pid, &status, 0);
        if(WIFEXITED(status) && WEXITSTATUS(status) == 127) {
            cout << args[0] << ": command not found\n";
        }
    }
    else {
        perror("fork");
    }
    // cout << "world\n";
}

bool is_builtin(string cmd) {
    if(cmd == "type" || cmd == "echo" || cmd == "exit" || cmd == "pwd" || cmd == "cd") return true;
    return false;
}

void run_builtin(vector<string>& args) {
    string cmd = args[0];
    if(cmd == "type") {
        for(int i = 1; i < args.size(); i++) {
            if(!checkCommand(args[i])) {
                std::cout << args[i] << ": not found\n";
            }
        }   
    }
    else if(cmd == "exit") {
        exit(0);
    }
    else if(cmd == "echo") {
        for(int i = 1; i < args.size(); i++) {
            cout << args[i] << ' ';
        }
        cout << '\n';
    }
    else if(cmd == "cd") {
        const char* path;
        if(args[1] == "~") {
            path = getenv("HOME");
        }
        else path = args[1].c_str();
        if(chdir(path) != 0) {
            cout << "cd: " << args[1] << ": No such file or directory\n";
        }
    }
    else if(cmd == "pwd") {
        cout << filesystem::current_path().string() << endl; // without .string() path is output with "" to safely represents paths with spaces.
        // string pwd = filesystem::current_path();
        // cout << pwd << endl;
    }
}

const char* builtins[] = {
    "echo",
    "exit",
    nullptr
};

/**
 * Generator function
 * Called repeatedly by readline when TAB is pressed
 */
char* builtin_generator(const char* text, int state) {
    static int index;
    static size_t len;

    if (state == 0) {
        index = 0;
        len = strlen(text);
    }

    while (builtins[index]) {
        const char* cmd = builtins[index++];
        if (strncmp(cmd, text, len) == 0) {
            // Add trailing space as required
            std::string completion = std::string(cmd) + " ";
            return strdup(completion.c_str());
        }
    }
    return nullptr;
}

char** completion(const char* text, int start, int end) {
    // Only complete the first word (command position)
    if (start != 0)
        return nullptr;

    return rl_completion_matches(text, builtin_generator);
}


int main() {
    // Flush after every std::cout / std:cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // TODO: Uncomment the code below to pass the first stage
    std::string input;
  
    // Register completion function
    rl_attempted_completion_function = completion;

  while(true) {
    // std::cout << "$ ";
    // std::getline(std::cin, input);

    char* line = readline("$ ");
    if (!line) break;           // Ctrl+D exits shell

    if (*line)
        add_history(line);      // optional but recommended

    input = line;
    free(line);

    // input += ' ';

    bool redirect_out = false, append_out = false;
    bool redirect_err = false, append_err = false;
    string out_file, err_file;
    vector<string> tokens = tokenize(input);
    vector<string> args;
    
    for(int i = 0; i < tokens.size(); i++) {
        if(tokens[i] == ">" || tokens[i] == "1>") {
            redirect_out = true;
            out_file = tokens[++i];
            continue;
        }
        else if(tokens[i] == ">>" || tokens[i] == "1>>") {
            append_out = true;
            out_file = tokens[++i];
            continue;
        }
        else if(tokens[i] == "2>") {
            redirect_err = true;
            err_file = tokens[++i];
            continue;
        }
        else if(tokens[i] == "2>>") {
            append_err = true;
            err_file = tokens[++i];
            continue;
        }
        args.push_back(tokens[i]);
    }

    if(is_builtin(args[0])) {
        int saved_stdout = -1;
        int saved_stderr = -1;

        if(redirect_out || append_out) {
            saved_stdout = dup(STDOUT_FILENO);
            int flags = O_WRONLY | O_CREAT | (append_out ? O_APPEND : O_TRUNC);
            int fd = open(out_file.c_str(), flags, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        if(redirect_err || append_err) {
            saved_stderr = dup(STDERR_FILENO);
            int flags = O_WRONLY | O_CREAT | (append_err ? O_APPEND : O_TRUNC);
            int fd = open(err_file.c_str(), flags, 0644);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        run_builtin(args);

        if(saved_stdout != -1) {
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }

        if(saved_stderr != -1) {
            dup2(saved_stderr, STDERR_FILENO);
            close(saved_stderr);
        }

    }
    else run_external(tokens);
  }
}
