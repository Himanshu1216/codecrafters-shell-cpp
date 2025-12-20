#include <iostream>
#include <string>
#include <vector>


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

    std::string word = "";
    for(int i = 0; i < input.size(); i++) {
      if(input[i] == ' ') {
        user_input.push_back(word);
        word = "";
      }
      else word += input[i];
    }
    user_input.push_back(word);

    std::string cmd1 = user_input[0];

    if(cmd1 == "type") {
      for(int i = 1; i < user_input.size(); i++) {
        if(user_input[i] == "exit" || user_input[i] == "echo") {
          std::cout << user_input[i] << " is a shell builtin\n";
        } 
        else {
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
    else {
      std::cout << "command not found\n";
    }
  }

}
