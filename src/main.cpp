#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  std::string user_input;
  
  while(true) {
    std::cout << "$ ";
    std::cin >> user_input;

    if(user_input == "exit") {
      return 0;
    }
    else if(user_input == "echo") {
      std::cout << user_input << '\n';
    }
    else std::cout << user_input << ": " << "command not found\n";
  }

}
