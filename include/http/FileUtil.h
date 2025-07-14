#pragma once

#include <fstream>
#include <string>
#include <vector>

#include <mymuduo/Logger.h>

class FileUtil
{

public:
    FileUtil(std::string filePath) : filePath_(filePath), file_(filePath, std::ios::binary) {}
    ~FileUtil() { file_.close(); }

    bool isValid() const { return file_.is_open(); }

    void resetDefaultFile()
    {
        file_.close();
        file_.open("../muduo/Web/Source/NotFound.html", std::ios::binary);
    }

    uint64_t size()
    {
        file_.seekg(0, std::ios::end);
        uint64_t fileSize = file_.tellg();
        file_.seekg(0, std::ios::beg);
        return fileSize;
    }

    void readFile(std::vector<char> &buffer)
    {

        if (file_.read(buffer.data(), size()))
        {

            LOG_INFO("File content load into memory ( %d bytes )", size());
        }
        else
        {
            LOG_ERROR("File read failed");
        }
    }

private:
    std::string filePath_;
    std::ifstream file_;
};