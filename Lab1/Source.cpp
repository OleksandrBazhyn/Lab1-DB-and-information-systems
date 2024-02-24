#include <fstream>
#include <iostream>
#include <string>

struct Genre // об'єкт Жанр
{
    uint32_t code = 0;

    char name[25] = {};
};

struct Author // об'єкт Автор
{
    uint32_t code = 0;

    char name[25] = {};
};

struct Book // зв'язок
{
    uint32_t author_code = 0;
    uint32_t genre_code = 0;
    uint32_t isbn = 0;

    char name[25] = {};

    int64_t next = -1;
};

void insert_genre(const Genre& genre)
{
    std::fstream file("genres.dat", std::ios::binary | std::ios::in | std::ios::out | std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Unable to open the file!" << std::endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(&genre), sizeof(Genre));
    file.flush();

    if (file.fail()) {
        std::cerr << "Failed to write data to the file!" << std::endl;
    }

    file.close();
}

void insert_author(const Author& author)
{
    std::fstream file("authors.dat", std::ios::binary | std::ios::in | std::ios::out | std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Unable to open the file!" << std::endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(&author), sizeof(Author));
    file.flush();

    if (file.fail()) {
        std::cerr << "Failed to write data to the file!" << std::endl;
    }

    file.close();
}

void insert_book(const Book& book)
{
    std::fstream file("books.dat", std::ios::binary | std::ios::in | std::ios::out | std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Unable to open the file!" << std::endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(&book), sizeof(Book));
    file.flush();

    if (file.fail()) {
        std::cerr << "Failed to write data to the file!" << std::endl;
    }

    file.close();
}

void read_books()
{
    std::fstream file("books.dat", std::ios::binary | std::ios::in);

    Book book;
    while (file.read(reinterpret_cast<char*>(&book), sizeof(Book))) {
        std::cout << "ISBN: " << book.isbn << ", Author Code: " << book.author_code
            << ", Genre Code: " << book.genre_code << ", Name: " << book.name << std::endl;
    }
}

std::ostream& operator<<(std::ostream& os, const Book& book) {
    os << "Author Code: " << book.author_code << ", "
        << "Genre Code: " << book.genre_code << ", "
        << "Name: " << book.name << ", "
        << "ISBN: " << book.isbn;
    return os;
}

bool Read(Book& record, std::fstream& file, const std::streampos& pos)
{
    if (!file)
        return false;

    file.seekg(pos);
    file.read(reinterpret_cast<char*>(&record), sizeof(Book));

    return !file.fail();
}

bool Write(const Book& record, std::fstream& file, const std::streampos& pos)
{
    if (!file)
        return false;

    file.seekp(pos);
    file.write(reinterpret_cast<const char*>(&record), sizeof(Book));
    file.flush();

    return !file.fail();
}

bool SetNextPtr(std::fstream& file, const std::streampos& record_pos, const std::streampos& next_record_pos)
{
    Book tmp;

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

bool AddNode(const Book& record, std::fstream& file, const std::streampos& pos, const std::streampos& prev_record_pos = -1)
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
    Book tmp;

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


/*int main()
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
        Book record = { static_cast<uint32_t>(i) % 3 + 1, 456, static_cast<uint32_t>(100) * i, 10 };
        strncpy_s(record.name, ("Item " + std::to_string(i)).c_str(), std::size(record.name));

        AddNode(record, file, write_pos, prev_pos);

        prev_pos = write_pos;
        write_pos = write_pos + static_cast<std::streamoff>(sizeof(Book));
    }

    PrintNodes(file, 0);

    return 0;
}
*/

int main() {
    Genre genre1 = { 1, "Fiction" };
    Genre genre2 = { 2, "Non-Fiction" };

    Author author1 = { 101, "John Doe" };
    Author author2 = { 102, "Jane Doe" };

    Book book1 = { 101, 1, 123456789, "Book1", -1 };
    Book book2 = { 102, 2, 987654321, "Book2", -1 };

    // Inserting genres, authors, and books
    insert_genre(genre1);
    insert_genre(genre2);

    insert_author(author1);
    insert_author(author2);

    insert_book(book1);
    insert_book(book2);

    // Reading and displaying books
    read_books();

    return 0;
}
