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

    std::string comm1 = "", comm2 = "";
    int len = user_input.size();
    for(int i = 0; i < len; i++) {
      if(user_input[i] == ' ') {
        comm1 = user_input.substr(0, i);
        comm2 = user_input.substr(i + 1, len - i - 1);
        break;
      }
    }

    if(comm1 == "exit") {
      return 0;
    }
    else if(comm1 == "echo") {
      std::cout << comm2 << '\n';
    }
    else std::cout << user_input << ": " << "command not found\n";
  }

}
