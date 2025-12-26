#include <iostream>
#include <string>
#include <vector>
#include <unistd.h> // chdir, 
#include <sys/wait.h> // for waitpid
#include <filesystem> // to get current working directory
#include <fcntl.h> // open, O_* flags
#include <dirent.h> // for get_path_exec
#include <sys/stat.h> //for get_path_exec

#include <readline/readline.h>
#include <readline/history.h>

#include <fstream>

using namespace std;

vector<string> cmd_history;

bool checkCommand(const std::string& cmd) {
  if(cmd == "type" || cmd == "echo" || cmd == "exit" || cmd == "pwd" || cmd == "cd" || cmd == "history") {
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
    if(cmd == "type" || cmd == "echo" || cmd == "exit" || cmd == "pwd" || cmd == "cd" || cmd == "history") return true;
    return false;
}

void read_history_file(string path) {
    ifstream file(path);
    if(!file.is_open()) {
        perror("history");
    }
    string line;
    while(getline(file, line)) {
        if(!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if(line.empty()) continue;
        cmd_history.push_back(line);
    }
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
        // cout << "hello\n";
        for(int i = 1; i < args.size(); i++) {
            cout << args[i];
            if(i != args.size() - 1) cout << ' '; // otherwise giving wrong in case of wc (counting extra space)
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
    else if(cmd == "history") {
        int len = cmd_history.size();
        int n = len;
        if(args.size() == 3 && args[1] == "-r") {
            read_history_file(args[2]);
            return;
        }
        if(args.size() > 1) {
            n = stoi(args[1]);
        }
        for(int i = len - n; i < cmd_history.size(); i++) {
            cout << '\t' << i + 1 << ' ';
            cout << cmd_history[i] << endl;
        }
    }
}

std::vector<std::string> get_path_executables() {
    std::vector<std::string> result;

    char* path_env = getenv("PATH");
    if (!path_env) return result;

    std::string path(path_env);
    std::string dir;

    for (char c : path) {
        if (c == ':') {
            if (!dir.empty()) {
                DIR* dp = opendir(dir.c_str());
                if (dp) {
                    struct dirent* entry;
                    while ((entry = readdir(dp)) != nullptr) {
                        std::string name = entry->d_name;
                        std::string full = dir + "/" + name;

                        struct stat st;
                        if (stat(full.c_str(), &st) == 0 &&
                            S_ISREG(st.st_mode) &&
                            (st.st_mode & S_IXUSR)) {
                            result.push_back(name);
                        }
                    }
                    closedir(dp);
                }
            }
            dir.clear();
        } else {
            dir += c;
        }
    }

    // last PATH entry
    if (!dir.empty()) {
        DIR* dp = opendir(dir.c_str());
        if (dp) {
            struct dirent* entry;
            while ((entry = readdir(dp)) != nullptr) {
                std::string name = entry->d_name;
                std::string full = dir + "/" + name;

                struct stat st;
                if (stat(full.c_str(), &st) == 0 &&
                    S_ISREG(st.st_mode) &&
                    (st.st_mode & S_IXUSR)) {
                    result.push_back(name);
                }
            }
            closedir(dp);
        }
    }

    return result;
}


const char* builtins[] = {
    "echo",
    "exit",
    nullptr
};

char* command_generator(const char* text, int state) {
    static size_t index;
    static size_t len;
    static std::vector<std::string> matches;

    if (state == 0) {
        index = 0;
        len = strlen(text);
        matches.clear();

        // builtins
        for (int i = 0; builtins[i]; i++) {
            if (strncmp(builtins[i], text, len) == 0) {
                matches.push_back(builtins[i]);
            }
        }

        // PATH executables
        for (const auto& exe : get_path_executables()) {
            if (exe.compare(0, len, text) == 0) {
                matches.push_back(exe);
            }
        }
    }

    if (index < matches.size()) {
        std::string completion = matches[index++];
        return strdup(completion.c_str());
    }

    return nullptr;
}


char** completion(const char* text, int start, int end) {
    // Only complete the first word (command position)
    if (start != 0)
        return nullptr;

    return rl_completion_matches(text, command_generator);
}

char** to_argv(const std::vector<std::string>& cmd) {
    char** argv = new char*[cmd.size() + 1];
    for (size_t i = 0; i < cmd.size(); i++) {
        argv[i] = strdup(cmd[i].c_str());
    }
    argv[cmd.size()] = nullptr;
    return argv;
}

void run_pipeline_single(
     std::vector<std::string>& left,
     std::vector<std::string>& right
) {
    int pipefd[2]; //pipefd[0] -> read end, pipefd[1] -> write end
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        // Child 1 → cmd1
        dup2(pipefd[1], STDOUT_FILENO);

        close(pipefd[0]);
        close(pipefd[1]);

        // to handle builtin commands
        if(is_builtin(left[0])) {
            run_builtin(left);
            exit(0);
        }
        else {
            char** argv = to_argv(left);
            execvp(argv[0], argv);
            perror("execvp");
            exit(1);
        }
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // Child 2 → cmd2
        dup2(pipefd[0], STDIN_FILENO);

        close(pipefd[1]);
        close(pipefd[0]);

        // to handle builtin commands
        if(is_builtin(right[0])) {
            run_builtin(right);
            exit(0);
        }
        else {
            char** argv = to_argv(right);
            execvp(argv[0], argv);

            perror("execvp");
            exit(1); // exit(1) means there has been certain failure executing child process
        }
    }

    // Parent
    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);
}

void run_pipeline(vector<vector<string>>& cmds) {
    int n = cmds.size();
    int prev_read = -1;              // read end of previous pipe
    vector<pid_t> pids;

    for (int i = 0; i < n; i++) {
        int pipefd[2];

        // Create pipe except for last command
        if (i < n - 1) {
            if (pipe(pipefd) == -1) {
                perror("pipe");
                return;
            }
        }

        pid_t pid = fork();
        if (pid == 0) {
            // -------- CHILD --------

            // If not first command → read from previous pipe
            if (prev_read != -1) {
                dup2(prev_read, STDIN_FILENO);
            }

            // If not last command → write to next pipe
            if (i < n - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
            }

            // Close all fds in child
            if (prev_read != -1) close(prev_read);
            if (i < n - 1) {
                close(pipefd[0]);
                close(pipefd[1]);
            }

            // Execute
            if (is_builtin(cmds[i][0])) {
                run_builtin(cmds[i]);
                exit(0);
            } else {
                char** argv = to_argv(cmds[i]);
                execvp(argv[0], argv);
                perror("execvp");
                exit(1);
            }
        }

        // -------- PARENT --------
        pids.push_back(pid);

        if (prev_read != -1) close(prev_read);
        if (i < n - 1) {
            close(pipefd[1]);         // parent doesn’t write
            prev_read = pipefd[0];    // next command reads from here
        }
    }

    // Wait for all children
    for (pid_t pid : pids) {
        waitpid(pid, nullptr, 0);
    }
}

vector<vector<string>> split_pipeline(const vector<string>& tokens) {
    vector<vector<string>> cmds;
    vector<string> current;

    for (const auto& tok : tokens) {
        if (tok == "|") {
            cmds.push_back(current);
            current.clear();
        } else {
            current.push_back(tok);
        }
    }
    cmds.push_back(current);
    return cmds;
}


void execute_pipe(std::vector<std::string>& tokens) {
    // auto it = std::find(tokens.begin(), tokens.end(), "|");

    // if (it != tokens.end()) {
    //     std::vector<std::string> left(tokens.begin(), it);
    //     std::vector<std::string> right(it + 1, tokens.end());

    //     run_pipeline(left, right);
    //     return;
    // }

    auto cmds = split_pipeline(tokens);
    run_pipeline(cmds);
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
    cmd_history.push_back(input);
    free(line);

    input += ' ';
    // cout << input.size() << endl;
    // cout << input << endl;

    bool redirect_out = false, append_out = false;
    bool redirect_err = false, append_err = false;
    string out_file, err_file;
    vector<string> tokens = tokenize(input);
    vector<string> args;
    
    bool pipe = false;

    for(int i = 0; i < tokens.size(); i++) {
        if(tokens[i] == "|") {
            pipe = true;
            // break;
        }
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

    if(pipe) {
        execute_pipe(tokens);
        continue;
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
