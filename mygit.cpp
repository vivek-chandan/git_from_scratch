#include <iostream>
#include <filesystem>
#include <dirent.h>
#include <fstream>
#include <string>
#include <vector>
#include <zlib.h>
#include <algorithm>
#include <openssl/sha.h>
#include <fcntl.h>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cstring>

using namespace std;

string callCreatingBlobObject(const string &fileName, const string &flag);
string writeTree(const string &directoryPath);
void lsTree(const string &treeHash, const string &flag);
void mygitAdd(const vector<string> &files);
void mygitCommit(const string &message);

string callCreatingBlobObject(const string &fileName, const string &flag)
{
    ifstream file(fileName, ios::binary); // Binary mode
    if (!file.is_open())
    {
        cerr << "Error: No such file or directory exists.\n";
        return "";
    }

    
    stringstream buffer;
    buffer << file.rdbuf();
    string content = buffer.str();
    unsigned long long size = content.size();

    
    string blob_obj = "blob " + to_string(size) + '\0' + content;

    
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(blob_obj.c_str()), blob_obj.size(), hash);

    
    stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
    {
        ss << hex << setw(2) << setfill('0') << static_cast<unsigned int>(hash[i]);
    }
    string hashStr = ss.str();

    // If the -w flag is present, write the object to the .git/objects directory
    if (flag == "-w")
    {
        string folder = hashStr.substr(0, 2);
        string fileNamePart = hashStr.substr(2);
        string folderPath = ".git/objects/" + folder;

        if (!filesystem::exists(folderPath))
        {
            if (!filesystem::create_directory(folderPath))
            {
                cerr << "Error: Failed to create directory " << folderPath << "\n";
                return "";
            }
        }

        // Compress the blob object
        uLongf compressedSize = compressBound(blob_obj.size());
        vector<unsigned char> compressedBuffer(compressedSize);

        int result = compress(compressedBuffer.data(), &compressedSize,
                              reinterpret_cast<const Bytef *>(blob_obj.data()), blob_obj.size());
        if (result != Z_OK)
        {
            cerr << "Error: Failed to compress data. Result code: " << result << "\n";
            return "";
        }

        compressedBuffer.resize(compressedSize);

        ofstream outFile(folderPath + "/" + fileNamePart, ios::binary);
        if (!outFile.is_open())
        {
            cerr << "Error: Failed to create file.\n";
            return "";
        }

        outFile.write(reinterpret_cast<const char *>(compressedBuffer.data()),
                      static_cast<std::streamsize>(compressedSize));
        outFile.close();
    }
    // else
    // {
    //     // Print the hash to the console if flag is not "-w"
    //     cout << hashStr << endl;
    // }

    // Return the hash string
    return hashStr;
}

// Write tree function

void hexStringToBinary(const string &hexStr, unsigned char *binaryHash)
{
    for (size_t i = 0; i < SHA_DIGEST_LENGTH; ++i)
    {
        string byteString = hexStr.substr(i * 2, 2);
        binaryHash[i] = (unsigned char)strtol(byteString.c_str(), nullptr, 16);
    }
}

string writeTree(const string &directoryPath)
{
    struct TreeEntry
    {
        string mode;
        string filename;
        unsigned char hash[SHA_DIGEST_LENGTH];
    };

    vector<TreeEntry> entries;

    for (const auto &entry : filesystem::directory_iterator(directoryPath))
    {
        if (entry.path().filename() == ".git")
            continue; // need to skip the .git directory

        string filename = entry.path().filename().string();
        string mode;
        unsigned char hash[SHA_DIGEST_LENGTH];

        if (entry.is_directory())
        {
            // Recursively for subdirectories
            string subtreeHashHex = writeTree(entry.path().string());

            
            hexStringToBinary(subtreeHashHex, hash);

            mode = "40000";
        }
        else if (entry.is_regular_file())
        {
            
            string filePath = entry.path().string();
            string blobHashHex = callCreatingBlobObject(filePath, "-w");

            
            hexStringToBinary(blobHashHex, hash);

            mode = "100644";
        }
        else
        {
            continue; // need to skip other type
        }

        TreeEntry treeEntry = {mode, filename, {0}};
        memcpy(treeEntry.hash, hash, SHA_DIGEST_LENGTH);
        entries.push_back(treeEntry);
    }


    sort(entries.begin(), entries.end(), [](const TreeEntry &a, const TreeEntry &b)
              { return a.filename < b.filename; });


    vector<char> treeContent;
    for (const auto &entry : entries)
    {
        string entryStr = entry.mode + ' ' + entry.filename;
        treeContent.insert(treeContent.end(), entryStr.begin(), entryStr.end());
        treeContent.push_back('\0'); // Null terminator

        
        treeContent.insert(treeContent.end(), entry.hash, entry.hash + SHA_DIGEST_LENGTH);
    }

    string header = "tree " + to_string(treeContent.size()) + '\0';
    string treeObj = header + string(treeContent.begin(), treeContent.end());

    // Computing SHA-1
    unsigned char treeHash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(treeObj.c_str()), treeObj.size(), treeHash);


    stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
    {
        ss << hex << setw(2) << setfill('0') << static_cast<unsigned int>(treeHash[i]);
    }
    string treeHashHex = ss.str();

    // Now we need to write the tree object to the .git/objects directory
    string folder = treeHashHex.substr(0, 2);
    string fileNamePart = treeHashHex.substr(2);
    string folderPath = ".git/objects/" + folder;

    if (!filesystem::exists(folderPath))
    {
        filesystem::create_directory(folderPath);
    }

    uLongf compressedSize = compressBound(treeObj.size());
    vector<unsigned char> compressedBuffer(compressedSize);

    int result = compress(compressedBuffer.data(), &compressedSize,
                          reinterpret_cast<const Bytef *>(treeObj.data()), treeObj.size());
    if (result != Z_OK)
    {
        cerr << "Error: Failed to compress tree object. Result code: " << result << "\n";
        return "";
    }

    compressedBuffer.resize(compressedSize);

    ofstream outFile(folderPath + "/" + fileNamePart, ios::binary);
    if (!outFile.is_open())
    {
        cerr << "Error: Failed to write tree object.\n";
        return "";
    }

    outFile.write(reinterpret_cast<const char *>(compressedBuffer.data()),
                  static_cast<std::streamsize>(compressedSize));
    outFile.close();

    return treeHashHex;
}

// ls-tree function

void lsTree(const string &treeHash, const string &flag)
{
    string folderName = treeHash.substr(0, 2);
    string fileName = treeHash.substr(2);
    string completeFilePath = ".git/objects/" + folderName + "/" + fileName;

    if (!filesystem::exists(completeFilePath))
    {
        cerr << "Not a valid object name " << treeHash << "\n";
        return;
    }

    ifstream file(completeFilePath, ios::binary);
    if (!file.is_open())
    {
        cerr << "Error: Failed to open tree object file.\n";
        return;
    }

    vector<unsigned char> compressedData((istreambuf_iterator<char>(file)),
                                         istreambuf_iterator<char>());
    file.close();

    uLongf decompressedSize = compressedData.size() * 100; // Estimating decompressed size
    vector<unsigned char> decompressedBuffer(decompressedSize);

    int result = uncompress(decompressedBuffer.data(), &decompressedSize,
                            compressedData.data(), compressedData.size());
    if (result != Z_OK)
    {
        cerr << "Error: Failed to decompress tree object. Result code: " << result << "\n";
        return;
    }

    decompressedBuffer.resize(decompressedSize);

    size_t index = 0;

    // need to skip the header
    while (index < decompressedBuffer.size() && decompressedBuffer[index] != '\0')
    {
        index++;
    }
    index++; 

    while (index < decompressedBuffer.size())
    {
        
        string mode;
        while (index < decompressedBuffer.size() && decompressedBuffer[index] != ' ')
        {
            mode += decompressedBuffer[index];
            index++;
        }
        index++; // need to skip the space

        string filename;
        while (index < decompressedBuffer.size() && decompressedBuffer[index] != '\0')
        {
            filename += decompressedBuffer[index];
            index++;
        }
        index++; // need to skip the null byte


        unsigned char hash[SHA_DIGEST_LENGTH];
        if (index + SHA_DIGEST_LENGTH > decompressedBuffer.size())
        {
            cerr << "Error: Unexpected end of data while reading hash.\n";
            return;
        }
        memcpy(hash, &decompressedBuffer[index], SHA_DIGEST_LENGTH);
        index += SHA_DIGEST_LENGTH;

        stringstream ss;
        for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
        {
            ss << hex << setw(2) << setfill('0') << static_cast<unsigned int>(hash[i]);
        }
        string hashHex = ss.str();

        string type = (mode == "40000") ? "tree" : "blob";

        if (flag == "--name-only")
        {
            cout << filename << endl;
        }
        else
        {
            cout << mode << " " << type << " " << hashHex << " " << filename << endl;
        }
    }
}

// Log function
void mygitLog()
{
    // We need to read the HEAD file to get the current branch reference
    ifstream headFile(".git/HEAD");
    if (!headFile.is_open())
    {
        cerr << "Error: Failed to read HEAD.\n";
        return;
    }

    string refLine;
    getline(headFile, refLine);
    headFile.close();

    string refPath = refLine.substr(5); // Skipping "ref: "
    string headRefPath = ".git/" + refPath;

    ifstream refFile(headRefPath);
    if (!refFile.is_open())
    {
        cerr << "Error: Failed to read current branch reference.\n";
        return;
    }

    string commitHash;
    getline(refFile, commitHash);
    refFile.close();

    // We need to traverse the commit objects
    while (!commitHash.empty())
    {
        string folderName = commitHash.substr(0, 2);
        string fileName = commitHash.substr(2);
        string path = ".git/objects/" + folderName + "/" + fileName;

        if (!filesystem::exists(path))
        {
            cerr << "Error: Commit object not found " << commitHash << "\n";
            return;
        }

        ifstream file(path, ios::binary);
        if (!file.is_open())
        {
            cerr << "Error: Failed to open commit object file.\n";
            return;
        }

        file.seekg(0, ios::end);
        long long sizeOfFile = file.tellg();
        file.seekg(0, ios::beg);

        vector<Bytef> buffer(sizeOfFile);
        file.read(reinterpret_cast<char *>(buffer.data()), sizeOfFile);

        if (file.fail())
        {
            cerr << "Error: Failed to read commit object file.\n";
            return;
        }
        file.close();

        uLongf decompressedSize = buffer.size() * 100;
        vector<Bytef> decompressedBuffer(decompressedSize);

        int result = uncompress(decompressedBuffer.data(), &decompressedSize, buffer.data(), buffer.size());
        if (result != Z_OK)
        {
            cerr << "Error: Failed to decompress commit object. Result code: " << result << "\n";
            return;
        }

        decompressedBuffer.resize(decompressedSize);

        string commitContent(decompressedBuffer.begin(), decompressedBuffer.end());

        // we need to skip the header
        size_t nullPos = commitContent.find('\0');
        if (nullPos == string::npos)
        {
            cerr << "Invalid commit object format.\n";
            return;
        }

        string content = commitContent.substr(nullPos + 1);

        // Parsing the commit content
        stringstream ss(content);
        string line;
        string treeHash, parentHash, author, committer, message;
        bool messageStarted = false;

        while (getline(ss, line))
        {
            if (line.rfind("tree ", 0) == 0)
            {
                treeHash = line.substr(5);
            }
            else if (line.rfind("parent ", 0) == 0)
            {
                parentHash = line.substr(7);
            }
            else if (line.rfind("author ", 0) == 0)
            {
                author = line.substr(7);
            }
            else if (line.rfind("committer ", 0) == 0)
            {
                committer = line.substr(10);
            }
            else if (line.empty())
            {
                // Start of the message
                messageStarted = true;
                continue;
            }
            else if (messageStarted)
            {
                message += line + "\n";
            }
        }

        cout << "commit " << commitHash << endl;
        if (!parentHash.empty())
        {
            cout << "Parent: " << parentHash << endl;
        }
        cout << "Author: " << author << endl;
        cout << "Date:   " << committer << endl;
        cout << "\n    " << message << endl;

        // Now we need to move to parent commit
        commitHash = parentHash;
    }
}

void mygitAdd(const vector<string> &files)
{
    
    ofstream indexFile(".git/index", ios::app);
    if (!indexFile.is_open())
    {
        cerr << "Error: Could not open index file.\n";
        return;
    }

    for (const string &file : files)
    {
        if (filesystem::is_directory(file))
        {
            // Recursively adding files
            for (const auto &entry : filesystem::recursive_directory_iterator(file))
            {
                if (entry.is_regular_file())
                {
                    string filePath = entry.path().string();
                    if (filePath.find(".git") != string::npos)
                        continue; // Exclude files inside .git directory

                    string hash = callCreatingBlobObject(filePath, "-w");
                    indexFile << hash << " " << filePath << endl;
                }
            }
        }
        else if (filesystem::is_regular_file(file))
        {
            string hash = callCreatingBlobObject(file, "-w");
            indexFile << hash << " " << file << endl;
        }
        else if (file == ".")
        {
            
            for (const auto &entry : filesystem::recursive_directory_iterator("."))
            {
                if (entry.is_regular_file())
                {
                    string filePath = entry.path().string();
                    if (filePath.find(".git") != string::npos)
                        continue;

                    string hash = callCreatingBlobObject(filePath, "-w");
                    indexFile << hash << " " << filePath << endl;
                }
            }
        }
        else
        {
            cerr << "Error: " << file << " is not a valid file or directory.\n";
        }
    }

    indexFile.close();
}

void mygitCommit(const string &message)
{

    ifstream indexFile(".git/index");
    if (!indexFile.is_open())
    {
        cerr << "Error: No changes added to commit.\n";
        return;
    }


    string treeHash = writeTree(".");

    
    string commitContent;
    commitContent += "tree " + treeHash + "\n";

    // parent commit SHA
    ifstream headFile(".git/HEAD");
    string refLine;
    getline(headFile, refLine);
    headFile.close();

    string refPath = refLine.substr(5); // Skipping "ref: "
    string headRefPath = ".git/" + refPath;

    ifstream refFile(headRefPath);
    if (refFile.is_open())
    {
        string parentHash;
        getline(refFile, parentHash);
        refFile.close();
        commitContent += "parent " + parentHash + "\n";
    }

    
    time_t now = time(nullptr);
    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S %z", localtime(&now));

    commitContent += "author : <VaibhavGupta@gmail.com> " + string(timeStr) + "\n";
    commitContent += "committer : <VaibhavGupta@gmail.com> " + string(timeStr) + "\n\n";
    commitContent += message + "\n";

    
    string commitObj = "commit " + to_string(commitContent.size()) + '\0' + commitContent;

    
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(commitObj.c_str()), commitObj.size(), hash);

    stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
    {
        ss << hex << setw(2) << setfill('0') << static_cast<unsigned int>(hash[i]);
    }
    string commitHash = ss.str();

    // Writing the commit object to the .git/objects directory
    string folder = commitHash.substr(0, 2);
    string fileNamePart = commitHash.substr(2);
    string folderPath = ".git/objects/" + folder;

    if (!filesystem::exists(folderPath))
    {
        filesystem::create_directory(folderPath);
    }

    uLongf compressedSize = compressBound(commitObj.size());
    vector<unsigned char> compressedBuffer(compressedSize);

    int result = compress(compressedBuffer.data(), &compressedSize,
                          reinterpret_cast<const Bytef *>(commitObj.data()), commitObj.size());
    if (result != Z_OK)
    {
        cerr << "Error: Failed to compress commit object. Result code: " << result << "\n";
        return;
    }

    compressedBuffer.resize(compressedSize);

    // Write the compressed commit object to the file
    ofstream outFile(folderPath + "/" + fileNamePart, ios::binary);
    if (!outFile.is_open())
    {
        cerr << "Error: Failed to write commit object.\n";
        return;
    }

    outFile.write(reinterpret_cast<const char *>(compressedBuffer.data()),
                  static_cast<std::streamsize>(compressedSize));
    outFile.close();

    // Update the HEAD reference
    ofstream refFileOut(headRefPath);
    if (!refFileOut.is_open())
    {
        cerr << "Error: Failed to update HEAD reference.\n";
        return;
    }
    refFileOut << commitHash << endl;
    refFileOut.close();

    //  Now Clearing the index file
    ofstream indexFileOut(".git/index", ofstream::trunc);
    indexFileOut.close();

    cout << commitHash << endl;
}

// Checkout function



int main(int argc, char *argv[])
{
    
    cout << std::unitbuf;
    cerr << std::unitbuf;

    if (argc < 2)
    {
        cerr << "No command provided.\n";
        return EXIT_FAILURE;
    }

    string command = argv[1];

    if (command == "init")
    {
        try
        {
            filesystem::create_directory(".git");
            filesystem::create_directory(".git/objects");
            filesystem::create_directory(".git/refs");
            filesystem::create_directory(".git/refs/heads");

            ofstream headFile(".git/HEAD");
            if (headFile.is_open())
            {
                headFile << "ref: refs/heads/main\n";
                headFile.close();
            }
            else
            {
                cerr << "Failed to create .git/HEAD file.\n";
                return EXIT_FAILURE;
            }

            cout << "Initialized git directory\n";
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }
    else if (command == "cat-file")
    {
        if (argc <= 3)
        {
            cerr << "No hash provided.\n";
            return EXIT_FAILURE;
        }

        string shaOfBlob = argv[3];
        string folderName = shaOfBlob.substr(0, 2);
        string fileName = shaOfBlob.substr(2);
        string path = ".git/objects/" + folderName + "/" + fileName;

        if (!filesystem::exists(path))
        {
            cerr << "Not a valid object name " << shaOfBlob << "\n";
            return EXIT_FAILURE;
        }

        ifstream file(path, ios::binary);
        if (!file.is_open())
        {
            cerr << "Error: Failed to open file.\n";
            return EXIT_FAILURE;
        }

        file.seekg(0, ios::end);
        long long sizeOfFile = file.tellg();
        file.seekg(0, ios::beg);

        if (sizeOfFile > 0)
        {
            
            vector<Bytef> buffer(sizeOfFile);
            file.read(reinterpret_cast<char *>(buffer.data()), sizeOfFile);
            file.close();

            if (file.fail())
            {
                cerr << "Error: Failed to read file.\n";
                return EXIT_FAILURE;
            }

            
            uLongf decompressedSize = buffer.size() * 100;
            vector<Bytef> decompressedBuffer(decompressedSize);

            int result = uncompress(decompressedBuffer.data(), &decompressedSize, buffer.data(), buffer.size());
            if (result != Z_OK)
            {
                cerr << "Error: Failed to decompress data. Result code: " << result << "\n";
                return EXIT_FAILURE;
            }

            
            decompressedBuffer.resize(decompressedSize);

            
            if (string(argv[2]) == "-p")
            {
                
                auto start = find(decompressedBuffer.begin(), decompressedBuffer.end(), '\0');
                if (start != decompressedBuffer.end())
                {
                    ++start; // Move to the character after the null character
                    for (auto it = start; it != decompressedBuffer.end(); ++it)
                    {
                        cout << *it;
                    }
                    cout << endl;
                }
            }
            else if (string(argv[2]) == "-t")
            {
            
                auto end = find(decompressedBuffer.begin(), decompressedBuffer.end(), ' ');
                string type(decompressedBuffer.begin(), end);
                cout << "Type: " << type << endl;
            }
            else if (string(argv[2]) == "-s")
            {
                // Extract size by reading until the first null character after the first space
                auto spaceIt = find(decompressedBuffer.begin(), decompressedBuffer.end(), ' ');
                if (spaceIt != decompressedBuffer.end())
                {
                    ++spaceIt; // Move to the start of the size
                    auto nullIt = find(spaceIt, decompressedBuffer.end(), '\0');
                    string sizeStr(spaceIt, nullIt);
                    cout << "Size: " << sizeStr << endl;
                }
            }
            else
            {
                cerr << "Invalid flag provided. Use -p, -t, or -s.\n";
                return EXIT_FAILURE;
            }
        }
    }

    else if (command == "hash-object")
    {
        if (argc == 2)
        {
            cerr << "No file provided.\n";
            return EXIT_FAILURE;
        }
        else if (argc > 4)
        {
            cerr << "Error: Too many arguments\n";
            return EXIT_FAILURE;
        }
        else
        {
            string fileName;
            string flag;

            if (argc == 3)
            {
                fileName = argv[2];
                flag = "";
            }
            else
            {
                flag = argv[2];
                fileName = argv[3];
            }

            string s = callCreatingBlobObject(fileName, flag);
            cout << s << endl;
        }
    }
    else if (command == "write-tree")
    {
        string treeHash = writeTree(".");
        if (!treeHash.empty())
        {
            cout << treeHash << endl;
        }
        else
        {
            cerr << "Failed to write tree.\n";
            return EXIT_FAILURE;
        }
    }
    else if (command == "ls-tree")
    {
        if (argc < 3)
        {
            cerr << "No tree hash provided.\n";
            return EXIT_FAILURE;
        }

        string flag = "";
        string treeHash;

        if (argc == 4)
        {
            flag = argv[2];
            treeHash = argv[3];
        }
        else
        {
            treeHash = argv[2];
        }

        lsTree(treeHash, flag);
    }
    else if (command == "add")
    {
        if (argc < 3)
        {
            cerr << "No files provided to add.\n";
            return EXIT_FAILURE;
        }

        vector<string> files;
        for (int i = 2; i < argc; ++i)
        {
            files.push_back(argv[i]);
        }

        mygitAdd(files);
    }
    else if (command == "commit")
    {
        string message = "No commit message";
        if (argc == 4 && string(argv[2]) == "-m")
        {
            message = argv[3];
        }
        else if (argc != 2)
        {
            cerr << "Usage: ./mygit commit -m \"Commit message\"\n";
            return EXIT_FAILURE;
        }

        mygitCommit(message);
    }
    // else if (command == "checkout")
    // {
    //     if (argc != 3)
    //     {
    //         cerr << "Usage: ./mygit checkout <commit_hash>\n";
    //         return EXIT_FAILURE;
    //     }

    //     string commitHash = argv[2];
    //     mygitCheckout(commitHash);
    // }

    else if (command == "log")
    {
        mygitLog();
    }
    else
    {
        cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

