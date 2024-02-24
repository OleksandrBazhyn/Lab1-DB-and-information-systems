#include <fstream>
#include <iostream>
#include <string>

struct Books
{
    uint32_t author_code = 0;
    uint32_t genre_code = 0;
    uint32_t language_code = 0;
    uint32_t isbn = 0;
    uint32_t quantity = 0;

    char name[25] = {};

    int64_t next = -1;
};

std::ostream& operator<<(std::ostream& os, const Books& books) {
    os << "Author Code: " << books.author_code << ", "
        << "Genre Code: " << books.genre_code << ", "
        << "Language Code: " << books.language_code << ", "
        << "Name: " << books.name << ", "
        << "ISBN: " << books.isbn;
    return os;
}

bool Read(Books& record, std::fstream& file, const std::streampos& pos)
{
    if (!file)
        return false;

    file.seekg(pos);
    file.read(reinterpret_cast<char*>(&record), sizeof(Books));

    return !file.fail();
}

bool Write(const Books& record, std::fstream& file, const std::streampos& pos)
{
    if (!file)
        return false;

    file.seekp(pos);
    file.write(reinterpret_cast<const char*>(&record), sizeof(Books));
    file.flush();

    return !file.fail();
}

bool SetNextPtr(std::fstream& file, const std::streampos& record_pos, const std::streampos& next_record_pos)
{
    Books tmp;

    if (!Read(tmp, file, record_pos))
    {
        std::cerr << "Unable to set next ptr. Read failed" << std::endl;
        return false;
    }

    tmp.next = next_record_pos;

    if (!Write(tmp, file, record_pos))
    {
        std::cerr << "Unable to set next ptr. Read failed" << std::endl;
        return false;
    }

    return true;
}

bool AddNode(const Books& record, std::fstream& file, const std::streampos& pos, const std::streampos& prev_record_pos = -1)
{
    if (!Write(record, file, pos))
    {
        return false;
    }

    if (prev_record_pos == -1)
        return true;

    return SetNextPtr(file, prev_record_pos, pos);
}

void PrintNodes(std::fstream& file, const std::streampos& record_pos)
{
    Books tmp;

    std::streampos read_pos = record_pos;

    while (read_pos != -1)
    {
        if (!Read(tmp, file, read_pos))
        {
            std::cerr << "Unable to update next_ptr. Error: read failed" << std::endl;
            return;
        }

        std::cout << tmp << std::endl;
        read_pos = tmp.next;
    }
}


int main()
{
    const std::string filename = "file.bin";

    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
    auto err = errno;

    if (err == ENOENT)
    {
        file = std::fstream(filename, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
    }

    if (!file) {
        std::cerr << "Unable to open file=" << filename << std::endl;

        return -1;
    }

    std::streampos write_pos = 0;
    std::streampos prev_pos = -1;

    for (int i = 0; i < 10; ++i)
    {
        Books record = { static_cast<uint32_t>(i) % 3 + 1, 456, static_cast<uint32_t>(100) * i, 10 };
        strncpy_s(record.name, ("Item " + std::to_string(i)).c_str(), std::size(record.name));

        AddNode(record, file, write_pos, prev_pos);

        prev_pos = write_pos;
        write_pos = write_pos + static_cast<std::streamoff>(sizeof(Books));
    }

    PrintNodes(file, 0);

    return 0;
}

