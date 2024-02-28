#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <random>

struct Book
{
    uint32_t key = 0;
    uint32_t genre_code = 0;
    uint32_t isbn = 0;

    char name[25] = {};

    Book() : key(0), genre_code(0), isbn(0)
    {
        name[0] = '\0';
    }

    Book(size_t k, uint32_t gc, uint32_t i, const char* n) : key(k), genre_code(gc), isbn(i)
    {
        strncpy_s(name, n, _TRUNCATE);
    }
};

struct GarbageBook
{
    bool IsFree = false;
    long position;
};

struct Genre
{
    uint32_t key = 0;
    char name[25] = {};

    Genre() : key(0)
    {
        name[0] = '\0';
    }

    Genre(uint32_t k, const char* n) : key(k)
    {
        strncpy_s(name, n, _TRUNCATE);
    }
};

struct IndexGenre 
{
    uint32_t key = 0;
    long position; // � ���� key * sizeof(Genre) + sizeof(uint32_t)
};

struct GarbageGenre
{
    bool IsFree = false;
    long position;
};

static Genre GetM(uint32_t GenreKey) { // ����������� �������� �������. �������� ���� ����, ���� ���� ������ � fl ���� � ������� ��'��� �����
    std::ifstream indexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in);
    
    if (!indexGenresFile.is_open())
    {
        std::ofstream createIndexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::trunc);
        if (!indexGenresFile.is_open())
        {
            std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
            throw std::runtime_error("Unable to open file indexGenresFile.ind");
        }
        createIndexGenresFile.close();
        indexGenresFile = std::ifstream("indexGenresFile.ind", std::ios::binary | std::ios::in);
    }

    // ������� ������ � �����
    IndexGenre currentIndex;
    bool notFound = true;

    indexGenresFile.seekg(0, std::ios::beg);

    while (true)
    {
        indexGenresFile.read(reinterpret_cast<char*>(&currentIndex), sizeof(IndexGenre));
        if (indexGenresFile.eof())
        {
            std::cerr << "The Genre has been does not found." << std::endl;
            break;
        }
        if (currentIndex.key == GenreKey)
        {
            notFound = false;
            break;
        }        
    }
    if (notFound)
    {
        return { 0, "\0" };
    }

    indexGenresFile.close();

    std::ifstream genresFile("genresFile.fl", std::ios::binary | std::ios::in);
    if (!genresFile.is_open())
    {
        std::cerr << "Unable to open file genresFile.fl" << std::endl;
        throw std::runtime_error("Unable to open file genresFile.fl");
    }

    // ������� �� ������� ������ � ��������� ����
    genresFile.seekg(currentIndex.position, std::ios::beg);
    Genre res;
    genresFile.read(reinterpret_cast<char*>(&res), sizeof(Genre));

    genresFile.close();

    return res;
}

std::vector<Book> GetS(uint32_t genreCode) {
    std::ifstream booksFile("booksFile.fl", std::ios::binary | std::ios::in);
    if (!booksFile.is_open())
    {
        std::cerr << "Unable to open file booksFile.fl" << std::endl;
        throw std::runtime_error("Unable to open file booksFile.fl");
    }

    booksFile.seekg(sizeof(uint32_t), std::ios::beg); // ���������� ����� header, �� ������ ���������� ��� ������� ������

    Book currentBook;
    std::vector<Book> neededBooks;
    bool foundAll = false;

    while (true)
    {
        booksFile.read(reinterpret_cast<char*>(&currentBook), sizeof(Book));
        if (booksFile.eof())
        {
            break;
        }
        if (currentBook.genre_code == genreCode)
        {
            neededBooks.push_back(currentBook);
            foundAll = true;
        }       
    }

    booksFile.close();

    if (foundAll)
    {
        return neededBooks;
    }
    else
    {
        std::cerr << "Book not found" << std::endl;
        booksFile.close();
    }
}

static void DelS(uint32_t recordNumber) // ��������� ���������� ������ ����������� �� ������� ����� ����� ���������� (��� �� �����) �� � ����������� ���� ������� � booksGarbageFile.gb ����������� ����������. ��� �������� �������� ��������� ����� �������� ����� booksGarbageFile.gb ���������� �� ���� �������� ����� �����
{
    std::ifstream booksFileForRead("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksFileForRead.is_open())
    {
        booksFileForRead = std::ifstream("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksFileForRead.is_open())
        {
            std::cerr << "Unable to open file booksFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file booksFile.fl");
        }
    }

    // ĳ������� ������� ������ � ����. �� ���������� �� ������� ����� (header)
    booksFileForRead.seekg(0, std::ios::beg);
    uint32_t recordsCount = 0;
    booksFileForRead.read(reinterpret_cast<char*>(&recordsCount), sizeof(uint32_t));

    booksFileForRead.close();

    // ���� ���������� ���� �������� ����� �� ������� ������ �� ������� ������
    if (recordNumber > recordsCount)
    {
        std::cerr << "The record number exceeds the number of records." << std::endl;
        return;
    }
    if (recordNumber == 0)
    {
        std::cerr << "We do not have 0th record." << std::endl;
        return;
    }

    std::ofstream booksFile("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksFile.is_open())
    {
        booksFile = std::ofstream("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksFile.is_open())
        {
            std::cerr << "Unable to open file booksFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file booksFile.fl");
        }
    }
    booksFile.seekp(0, std::ios::beg);
    recordsCount--;
    booksFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(uint32_t)); // ������� �������� ������� ������ � booksFile.fl

    Book emptyBook;
    std::streampos delRecordPos = (recordNumber - 1) * sizeof(Book) + sizeof(uint32_t);

    booksFile.seekp(delRecordPos, std::ios::beg);
    booksFile.write(reinterpret_cast<const char*>(&emptyBook), sizeof(Book));

    booksFile.flush();
    booksFile.close();

    // ��������� � ��� ������� � �������� (������� ��������� ����������) ���'���
    std::ofstream booksGarbageFile("booksGarbageFile.gb", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksGarbageFile.is_open())
    {
        booksGarbageFile = std::ofstream("booksGarbageFile.gb", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksGarbageFile.is_open())
        {
            std::cerr << "Unable to open file booksGarbageFile.gb" << std::endl;
            throw std::runtime_error("Unable to open file booksGarbageFile.gb");
        }
    }
    booksGarbageFile.seekp(0, std::ios::end);

    GarbageBook garbagebook = { false, delRecordPos };
    booksGarbageFile.write(reinterpret_cast<const char*>(&garbagebook), sizeof(GarbageBook));

    booksGarbageFile.flush();
    booksGarbageFile.close();
}

// ��������� ������(�������) ������ � ��������� ���� ����� �������� �������, ������� ����� � ������� �������, �������� �� �� � �������� �������
static void DelM(uint32_t recordNumber)
{
    std::ifstream genresFileForRead("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!genresFileForRead.is_open()) {
        genresFileForRead = std::ifstream("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!genresFileForRead.is_open())
        {
            std::cerr << "Unable to open file genresFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file genresFile.fl");
        }
    }

    // ĳ������� ������� ������ � ����. �� ���������� �� ������� ����� (header)
    genresFileForRead.seekg(0, std::ios::beg);
    uint32_t recordsCount = 0;
    genresFileForRead.read(reinterpret_cast<char*>(&recordsCount), sizeof(uint32_t));

    genresFileForRead.close();

    if (recordNumber > recordsCount)
    {
        std::cerr << "The record number exceeds the number of records" << std::endl;
        return;
    }
    if (recordNumber == 0)
    {
        std::cerr << "We do not have 0th record." << std::endl;
        return;
    }

    // ��������� �������� ������� � ���������� ���'���. � std::vector<IndexGenre> IndexGenreTable
    std::ifstream indexGenresFileForRead("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::app);

    if (!indexGenresFileForRead.is_open()) {
        std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
        throw std::runtime_error("Unable to open file indexGenresFile.ind");
    }
    indexGenresFileForRead.seekg(0, std::ios::end);

    IndexGenre tmp;
    std::vector<IndexGenre> IndexGenreTable;

    while (!indexGenresFileForRead.eof())
    {
        indexGenresFileForRead.read(reinterpret_cast<char*>(&tmp), sizeof(IndexGenre));
        IndexGenreTable.push_back(tmp);
    }

    indexGenresFileForRead.close();
    
    // ������� master �����
    std::ofstream genresFile("genresFile.fl", std::ios::binary | std::ios::out);


    // �������� ����������� IndexGenreTable � �������� �������
}

void AddGenre(Genre& newGenre) {
    std::ifstream genresFileForRead("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!genresFileForRead.is_open()) {
        genresFileForRead = std::ifstream("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!genresFileForRead.is_open())
        {
            std::cerr << "Unable to open file genresFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file genresFile.fl");
        }
    }

    // ĳ������� ������� ������ � ����. �� ���������� �� ������� ����� (header)
    genresFileForRead.seekg(0, std::ios::beg);
    uint32_t recordsCount = 0;
    genresFileForRead.read(reinterpret_cast<char*>(&recordsCount), sizeof(uint32_t));

    genresFileForRead.close();

    std::ofstream genresFile("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!genresFile.is_open()) {
        genresFile = std::ofstream("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!genresFile.is_open())
        {
            std::cerr << "Unable to open file genresFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file genresFile.fl");
        }
    }

    if (recordsCount == 0)
    {
        genresFile.seekp(0, std::ios::beg);
        uint32_t i = 1;
        genresFile.write(reinterpret_cast<const char*>(&i), sizeof(uint32_t));
    }

    genresFile.seekp(0, std::ios::end);  // ������� � ������ �������

    recordsCount++;
    newGenre.key = recordsCount;

    std::streampos newPosition = genresFile.tellp();  // ������ ������� ������ ������

    genresFile.write(reinterpret_cast<const char*>(&newGenre), sizeof(Genre));

    genresFile.seekp(0, std::ios::beg);
    genresFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(uint32_t)); // ������ ����� ����� � ��������� recordsCount �� �������

    genresFile.flush();
    genresFile.close();

    // ��������� �������� �������
    std::ifstream indexGenresFileForRead("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::app);

    if (!indexGenresFileForRead.is_open()) {
        std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
        throw std::runtime_error("Unable to open file indexGenresFile.ind");
    }
    indexGenresFileForRead.seekg(0, std::ios::end);

    // �������� ����� ����� (�������� ������� ������� � ���� �����)
    std::streampos endFilePos = indexGenresFileForRead.tellg();

    // ��������� ������� ������ � ����
    uint32_t indexRecordsCount = endFilePos / sizeof(IndexGenre);

    // ������� �� ���������� ������
    indexGenresFileForRead.seekg(-static_cast<std::streamoff>(sizeof(IndexGenre)), std::ios::cur);

    // ��������� ������� �����
    IndexGenre lastIndex;
    indexGenresFileForRead.read(reinterpret_cast<char*>(&lastIndex), sizeof(IndexGenre));

    indexGenresFileForRead.close();

    // �������� ������� ����
    uint32_t lastKey = indexRecordsCount > 0 ? lastIndex.key : 0;

    // �������� ���� ��� ������ ������
    uint32_t newKey = lastKey + 1;

    // ������ ����� ����� � ������
    IndexGenre newIndex;
    newIndex.key = newKey;
    newIndex.position = newPosition;

    std::ofstream indexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::app);

    if (!indexGenresFile.is_open()) {
        std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
        throw std::runtime_error("Unable to open file indexGenresFile.ind");
    }

    indexGenresFile.seekp(0, std::ios::end);  // ������� � ������ �������
    indexGenresFile.write(reinterpret_cast<const char*>(&newIndex), sizeof(IndexGenre));

    indexGenresFile.flush();
    indexGenresFile.close();
}

void AddBook(Book& newBook) {
    std::ifstream booksFileForRead("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksFileForRead.is_open()) {
        booksFileForRead = std::ifstream("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksFileForRead.is_open())
        {
            std::cerr << "Unable to open file booksFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file booksFile.fl");
        }
    }

    // ĳ������� ������� ������ � ����. �� ���������� �� ������� ����� (header)
    booksFileForRead.seekg(0, std::ios::beg);
    uint32_t recordsCount = 0;
    booksFileForRead.read(reinterpret_cast<char*>(&recordsCount), sizeof(uint32_t));

    booksFileForRead.close();

    std::ifstream booksGarbageFile("booksGarbageFile.gb", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksGarbageFile.is_open()) {
        booksGarbageFile = std::ifstream("booksGarbageFile.gb", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksGarbageFile.is_open())
        {
            std::cerr << "Unable to open file booksGarbageFile.gb" << std::endl;
            throw std::runtime_error("Unable to open file booksGarbageFile.gb");
        }
    }

    // ĳ������� �� ��������� booksGarbageFile.gb �� � ���� ��� ������ ����� ������ � ���� booksFile.fl ��� �������� ���� ����� �����
    booksGarbageFile.seekg(0, std::ios::beg);
    GarbageBook garbageBook;
    bool found = false;

    while (true)
    {
        booksGarbageFile.read(reinterpret_cast<char*>(&garbageBook), sizeof(GarbageBook));
        if (garbageBook.IsFree == 1)
        {
            found = true;
            break;
        }
        if (booksGarbageFile.eof())
        {
            break;
        }
    }

    booksFileForRead.close();

    std::ofstream booksFile("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksFile.is_open()) {
        booksFile = std::ofstream("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksFile.is_open())
        {
            std::cerr << "Unable to open file booksFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file booksFile.fl");
        }
    }

    if (recordsCount == 0)
    {
        booksFile.seekp(0, std::ios::beg);
        uint32_t i = 1;
        booksFile.write(reinterpret_cast<const char*>(&i), sizeof(uint32_t));
    }

    if (found)
    {
        booksFile.seekp(garbageBook.position, std::ios::beg);
        newBook.key = (garbageBook.position - 4) / sizeof(Book) + 1;
        booksFile.write(reinterpret_cast<const char*>(&newBook), sizeof(Book));

        recordsCount++;
        booksFile.seekp(0, std::ios::beg);
        booksFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(uint32_t)); // ������ ����� ����� � ��������� recordsCount �� �������

        booksFile.flush();
        booksFile.close();
    }
    else
    {
        booksFile.seekp(0, std::ios::end);  // ������� � ������ �������
        recordsCount++;
        newBook.key = recordsCount;

        booksFile.write(reinterpret_cast<const char*>(&newBook), sizeof(Book));

        booksFile.seekp(0, std::ios::beg);
        booksFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(uint32_t)); // ������ ����� ����� � ��������� recordsCount �� �������

        booksFile.flush();
        booksFile.close();
    }
}

int main()
{
    /*
    for (size_t i = 1; i < 50; i++)
    {
        std::string name = "Genre" + std::to_string(i);
        Genre f(i, name.c_str());
        AddGenre(f);
    }
    */
    /*
    for (size_t i = 1; i < 50; i++)
    {
        std::string name = "Book" + std::to_string(i);
        uint32_t gc = rand() % (50 - 1 + 1) + 1;
        Book f(i, gc, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, name.c_str());
        AddBook(f);
    }*/
    
    /*
    Book f(0, 1, 22222222, "testBook");
    AddBook(f); // ������ ������� ����� � ����� ����� 1
    */

    /*DelS(1);
    Book f(0, 1, 22222222, "testBook");
    AddBook(f);*/

    // ��������� GetM
    Genre resGetM = GetM(100);

    std::cout << resGetM.key << " " << resGetM.name << std::endl;

    std::vector<Book> resGetS = GetS(1);

    std::cout << "Key\tName\t    GenreCode\tISBN" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;
    for (const Book& book : resGetS) {
        std::cout << book.key << "\t" << book.name << "\t    " << book.genre_code << "\t\t" << book.isbn << std::endl;
    }
    
    std::cout << std::endl << std::endl;


    // Book f(0, 1, 32222222, "testBook2");
    // AddBook(f);



    return 0;
}

