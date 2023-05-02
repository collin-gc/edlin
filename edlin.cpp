#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <algorithm>
#include <sys/wait.h>

using namespace std;

const string PROMPT = "edlin> ";
const int MAX_LINE_LENGTH = 1024;

int getFileLength(const string& fileName) {
    int count = 0;
    ifstream file;
    string junk;
    file.open(fileName);
    while(file.peek()!=EOF)
    {
        getline(file, junk);
        count++;
    }
    file.close();
    return count;
}

void listFile(vector<string> lines) {
    for(string::size_type l = 0; l < lines.size(); l++) {
        cout << lines[l] << endl;
    }
}

void readFile(const string& fileName, vector<string>& lines) {
    ifstream file;
    string line;
    file.open(fileName);
    int lineNumber = 0;
    int lineCount = getFileLength(fileName);
    lines.resize(lineCount, "");
    while(file.peek() != EOF){
        getline(file, lines[lineNumber]);
        lineNumber++;
    }
    file.close();
}

void saveFile(const string& fileName, vector<string> lines) {
    ofstream file;
    file.open(fileName);
    for(string::size_type l = 0; l < lines.size(); l++){
        file << lines[l] << endl;
    }
    file.close();
}

void editLine(const string& input, vector<string>& lines) {
    istringstream iss(input);
    string::size_type lineNumber;
    string lineText;
    iss >> lineNumber;
    //if the vector isn't big enough, make it big enough. fill the empty slots with empty strings.
    if(lineNumber >= lines.size()){
        lines.resize(lineNumber+1, "");
    }
    getline(iss, lineText);
    //if the text to edit isn't empty, remove the leading space character and push the text to the vector.
    if (!lineText.empty()){
        lineText = lineText.substr(1, lineText.size() - 1);
    }
    lines[lineNumber] = lineText;
}

void filter(vector<string>& lines, const string& user_cmd) {
    int lno = -1;
    string ucmd;
    if (user_cmd.empty() || user_cmd[0] != '!') {
        cout << "Error: no line number or Unix command given" << endl;
        return;
    }
    size_t pos = user_cmd.find(' ');
    if (pos == std::string::npos || pos == user_cmd.size() - 1) {
        cout << "Error: no Unix command given" << endl;
        return;
    }
    try {
        lno = std::stoi(user_cmd.substr(pos, pos + 1));
    } catch (const std::exception&) {
        cout << "Error: not an integer" << endl;
        return;
    }
    if (lno < 0 || lno >= static_cast<int>(lines.size())) {
        cout << "Error: line number out of range" << endl;
        return;
    }

    ucmd = user_cmd.substr(pos + 3);
    
    int from_parent[2], to_parent[2];
    if (pipe(from_parent) < 0 || pipe(to_parent) < 0) {
        cout << "Error: failed to create pipes" << endl;
        return;
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        cout << "Error: failed to fork" << endl;
        return;
    } else if (pid == 0) {
        // Child process
        close(from_parent[1]);
        close(to_parent[0]);
        
        dup2(from_parent[0], STDIN_FILENO);
        dup2(to_parent[1], STDOUT_FILENO);
        
        close(from_parent[0]);
        close(to_parent[1]);
        
        char* args[] = {(char*)"/bin/sh", (char*)"-c", const_cast<char*>(ucmd.c_str()), nullptr};
        if (execve(args[0], args, nullptr) < 0) {
            cout << "Error: failed to execute Unix command" << endl;
            exit(1);
        }
    } else {
        // Parent process
        close(from_parent[0]);
        close(to_parent[1]);
        
        FILE* to_child = fdopen(from_parent[1], "w");
        if (to_child == nullptr) {
            cout << "Error: failed to open write end of pipe to child" << endl;
            close(from_parent[1]);
            close(to_parent[0]);
            waitpid(pid, nullptr, 0);
            return;
        }
        fprintf(to_child, "%s", lines[lno].c_str());
        fclose(to_child);
        
        FILE* from_child = fdopen(to_parent[0], "r");
        if (from_child == nullptr) {
            cout << "Error: failed to open read end of pipe from child" << endl;
            close(from_parent[1]);
            close(to_parent[0]);
            waitpid(pid, nullptr, 0);
            return;
        }
        char buf[1024];
        if (fgets(buf, sizeof(buf), from_child) != nullptr) {
            lines[lno] = std::string(buf);
        }
        fclose(from_child);
        
        close(from_parent[1]);
        close(to_parent[0]);
        waitpid(pid, nullptr, 0);
        cout << "Child pid is: " << getpid() << endl;
    }
}

int main() {
    string fileName;
    vector<string> lines;
    bool running = true;
    cout << "Line Editor" << endl;
    while (running) {
        cout << PROMPT;
        string command;
        getline(cin, command);
        if (command.length() == 0) {
            continue;
        }
        switch (command[0]) {
            case 'l':
                listFile(lines);
                break;
            case 'r':
                if (command.length() > 1) {
                    fileName = command.substr(2);
                    readFile(fileName, lines);
                } else {
                    cout << "ERROR: no file name specified" << endl;
                }
                break;
            case 's':
                if (command.length() > 1) {
                    fileName = command.substr(2);
                    saveFile(fileName, lines);
                } else {
                    cout << "ERROR: no file name specified" << endl;
                }
                break;
            case 'e':
                if (command.length() > 1) {
                    editLine(command.substr(2), lines);
                } else {
                    cout << "ERROR: no line number and text specified" << endl;
                }
                break;
            case '!':
                if (command.length() > 1) {
                    filter(lines, command);
                } else {
                    cout << "ERROR: no line number and command specified" << endl;
                }
                break;
            case 'q':
                cout << "* Exiting Editor..." << endl;
                running = false;
                break;
            default:
                cout << "invalid command" << endl;
                break;
        }
    }
    return 0;
}