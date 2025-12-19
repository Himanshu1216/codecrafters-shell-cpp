#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  std::cout << "$ ";
  std::string user_input;
  
  while(true) {
    std::cin >> user_input;
    std::cout << user_input << ": " << "command not found\n";
  }

}
